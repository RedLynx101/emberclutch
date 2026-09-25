"""Emberclutch egg: a speckled shell with light inside and three stages of glowing cracks.

  python tools/blender/egg_model.py --out romfs/models/egg.ecm
  python tools/blender/egg_model.py --lod 1 --out romfs/models/egg_lod1.ecm   (~300 triangles, for the den)
  blender -b -P tools/blender/egg_model.py -- --render C:/abs/prefix     (preview renders)

Writes romfs/models/egg.ecm (the dragons' format, src/core/model.cpp) with two bones:
"root" (the whole egg, rocked by src/core/egg.cpp around a pivot low in the shell) and
"cap", the top that pops off when it hatches along a zigzag seam. The geometry is a surface
of revolution computed here directly (no Blender scene), so the script also runs under plain
Python; Blender is only needed for previews.

A second mesh ("shards", kind part, group 8) is the same shell broken into SHARDS jagged
pieces for the hatching (Alpha 2 WP12a: the egg bursts into bits), each on its own bone
("s00".., after root and cap, resting at the piece's centre) so src/core/shell_burst.cpp can
throw them about and the renderer draws them all in one call.

One mesh ("shell", kind body): the outer shell, its inner surface and the rim at the seam
(seen once the cap is off), speckle decals and crack decals. Colours are palette slots set
per egg at runtime (src/core/egg.cpp eggPalette):
  Base = shell, Accent = speckles, Pattern / Horn / Membrane = cracks 1 / 2 / 3 (the seam),
  Iris = the inside of the shell, Glow = the light inside.
A crack is invisible (shell-coloured, no glow) until its stage, then glows: the dragon
shader scales each vertex's glow by its palette slot's alpha.
"""
import math
import random
import struct
import sys
from pathlib import Path

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:]


def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


OUT = arg("--out")
RENDER = arg("--render")
LOD = int(arg("--lod", "0"))  # 1: the den's egg (small on the top screen, two of them in a full den)

H = 1.0          # egg height (adult units: a newborn hatchling is about this tall curled up)
R = 0.37         # widest radius (low on the egg)
SEAM = 0.63      # where the cap splits off (fraction of the height)
ZIG = 0.04       # zigzag of the seam
THICK = 0.022    # shell thickness
SEGS = 16 if LOD == 0 else 10  # even: the seam zigzags
LIFT = 0.004     # decals float this far off the shell
FRONT = math.radians(-105)  # cracks face the den and close-up cameras (-Y, a little left)

BASE, ACCENT, CRACK1, CRACK2, CRACK3, INNER, GLOW = 0, 1, 2, 3, 4, 5, 8  # palette slots (kPal*)
ROOT, CAP = 0, 1                                                        # palette-local bones
BANDS, SECTORS = 4, 6          # the shell breaks into BANDS x SECTORS pieces
SHARDS = BANDS * SECTORS       # (src/core/shell_burst.hpp kShards; <= 25 bones per draw)
SHARD_GRID = 3 if LOD == 0 else 1  # quads along each side of a piece (the den's: plain quads, <= the egg's 300 triangles)
GROUP_SHARDS = 8               # src/core/model.hpp kGroupShards


def radius(u):
    """Profile: wider low down, narrower toward the top."""
    x = 2 * u - 1
    return R * math.sqrt(max(0.0, 1 - x * x)) * (1 - 0.14 * x)


def slope(u):
    e = 1e-3
    return (radius(min(1, u + e)) - radius(max(0, u - e))) / ((min(1, u + e) - max(0, u - e)) * H)


def surface(u, theta, inset=0.0):
    """Point and outward normal on the shell at height fraction u and angle theta."""
    r = max(0.0, radius(u) - inset)
    c, s = math.cos(theta), math.sin(theta)
    n = (c, s, -slope(u))
    ln = math.sqrt(n[0] ** 2 + n[1] ** 2 + n[2] ** 2)
    return (r * c, r * s, u * H), (n[0] / ln, n[1] / ln, n[2] / ln)


def glow_band(u):
    """How much the light inside shows through (0..~0.55): a soft band low on the egg."""
    return 0.55 * math.exp(-((u - 0.28) / 0.17) ** 2)


class Mesh:
    def __init__(self):
        self.pos, self.nrm, self.skin, self.paint, self.idx = [], [], [], [], []

    def vert(self, p, n, bone, paint):
        self.pos.append(p)
        self.nrm.append(n)
        self.skin.append((bone, bone, 255, 0))
        self.paint.append(paint)
        return len(self.pos) - 1

    def quad(self, a, b, c, d):  # counter-clockwise seen from the front
        self.idx += [a, b, c, a, c, d]

    def tri(self, a, b, c):
        self.idx += [a, b, c]


def shell_paint(u):
    f = glow_band(u)
    return (BASE, GLOW, int(round(255 * f)), int(round(255 * f)))


def seam_u(k):
    return SEAM + (ZIG if k % 2 == 0 else -ZIG)


def build():
    m = Mesh()
    lower_us = [0.04, 0.1, 0.18, 0.27, 0.37, 0.47, 0.55] if LOD == 0 else [0.06, 0.2, 0.4]
    upper_us = [0.71, 0.79, 0.87, 0.94] if LOD == 0 else [0.78, 0.91]
    thetas = [2 * math.pi * k / SEGS for k in range(SEGS)]

    def ring(us_of_k, bone, inset=0.0, paint=None, inward=False):
        out = []
        for k, th in enumerate(thetas):
            u = us_of_k(k)
            p, n = surface(u, th, inset)
            if inward:
                n = (-n[0], -n[1], -n[2])
            out.append(m.vert(p, n, bone, paint(u) if paint else shell_paint(u)))
        return out

    def band(lo, hi, inward=False):
        for k in range(SEGS):
            k1 = (k + 1) % SEGS
            if inward:
                m.quad(lo[k], hi[k], hi[k1], lo[k1])
            else:
                m.quad(lo[k], lo[k1], hi[k1], hi[k])

    inner_paint = (INNER, GLOW, 140, 255)
    # outer lower shell: bottom pole .. the seam
    bottom = m.vert((0, 0, 0), (0, 0, -1), ROOT, shell_paint(0))
    rings = [ring(lambda k, u=u: u, ROOT) for u in lower_us] + [ring(seam_u, ROOT)]
    for k in range(SEGS):
        m.tri(bottom, rings[0][(k + 1) % SEGS], rings[0][k])
    for lo, hi in zip(rings, rings[1:]):
        band(lo, hi)
    # outer cap: the seam .. top pole
    rings = [ring(seam_u, CAP)] + [ring(lambda k, u=u: u, CAP) for u in upper_us]
    for lo, hi in zip(rings, rings[1:]):
        band(lo, hi)
    top = m.vert((0, 0, H), (0, 0, 1), CAP, shell_paint(1))
    for k in range(SEGS):
        m.tri(top, rings[-1][k], rings[-1][(k + 1) % SEGS])
    # inside of the lower shell and of the cap (facing in), and the rims between
    inner = [ring(lambda k, u=u: u, ROOT, THICK, lambda u: inner_paint, True) for u in lower_us[1:]]
    inner.append(ring(seam_u, ROOT, THICK, lambda u: inner_paint, True))
    inner_bottom = m.vert((0, 0, THICK), (0, 0, 1), ROOT, inner_paint)
    for k in range(SEGS):
        m.tri(inner_bottom, inner[0][k], inner[0][(k + 1) % SEGS])
    for lo, hi in zip(inner, inner[1:]):
        band(lo, hi, inward=True)
    rim_out = ring(seam_u, ROOT, 0.0, lambda u: (BASE, BASE, 0, 0))
    rim_in = ring(seam_u, ROOT, THICK, lambda u: (BASE, BASE, 0, 0))
    for k in range(SEGS):  # the lower rim faces up
        k1 = (k + 1) % SEGS
        m.quad(rim_out[k], rim_out[k1], rim_in[k1], rim_in[k])
    cap_inner = [ring(seam_u, CAP, THICK, lambda u: inner_paint, True)]
    cap_inner += [ring(lambda k, u=u: u, CAP, THICK, lambda u: inner_paint, True) for u in upper_us[:-1]]
    for lo, hi in zip(cap_inner, cap_inner[1:]):
        band(lo, hi, inward=True)
    cap_top_in = m.vert((0, 0, H - THICK), (0, 0, -1), CAP, inner_paint)
    for k in range(SEGS):
        m.tri(cap_top_in, cap_inner[-1][(k + 1) % SEGS], cap_inner[-1][k])
    rim_out = ring(seam_u, CAP, 0.0, lambda u: (BASE, BASE, 0, 0))
    rim_in = ring(seam_u, CAP, THICK, lambda u: (BASE, BASE, 0, 0))
    for k in range(SEGS):  # the cap's rim faces down
        k1 = (k + 1) % SEGS
        m.quad(rim_out[k], rim_in[k], rim_in[k1], rim_out[k1])

    # speckles: small hexagons scattered over the shell, clear of the seam
    rng = random.Random(7)
    placed = 0
    while placed < (22 if LOD == 0 else 6):
        u, th = 0.12 + 0.78 * rng.random(), rng.random() * 2 * math.pi
        if abs(u - SEAM) < ZIG + 0.06:
            continue
        size = 0.014 + 0.024 * rng.random()
        decal_disc(m, u, th, size, CAP if u > SEAM else ROOT, (ACCENT, ACCENT, 0, 0))
        placed += 1

    # cracks: 1 and 2 zigzag down the front of the cap, 3 runs round the seam
    crack1 = [(0.86, FRONT + 0.05), (0.81, FRONT - 0.06), (0.77, FRONT + 0.03), (0.73, FRONT - 0.08)]
    crack2 = [(0.73, FRONT - 0.08), (0.70, FRONT + 0.06), (0.675, FRONT - 0.02), (0.655, FRONT + 0.1)]
    crack2b = [(0.81, FRONT - 0.06), (0.79, FRONT - 0.22), (0.76, FRONT - 0.3)]
    decal_line(m, crack1, 0.018, CAP, (CRACK1, CRACK1, 0, 255))
    decal_line(m, crack2, 0.016, CAP, (CRACK2, CRACK2, 0, 255))
    decal_line(m, crack2b, 0.014, CAP, (CRACK2, CRACK2, 0, 255))
    seam = [(seam_u(k), 2 * math.pi * k / SEGS) for k in range(SEGS + 1)]
    decal_line(m, seam, 0.02, ROOT, (CRACK3, CRACK3, 0, 255))
    return m


def build_shards():
    """The shell broken into SHARDS pieces: a jittered grid of BANDS x SECTORS patches on the
    egg (shared corners, so they fit back together), each edge zigzagged, each piece with its
    outside (the shell's colours), its inside (glowing) and thin walls between. Returns the
    mesh and each piece's centre (its bone's rest position)."""
    rng = random.Random(11)
    n = SHARD_GRID
    band_u = [0.0, 0.25, 0.49, 0.72, 1.0]
    # corners: (band boundary b, sector boundary k), jittered away from the poles
    cu = [[band_u[b] + (rng.uniform(-0.045, 0.045) if 0 < b < BANDS else 0.0) for k in range(SECTORS)]
          for b in range(BANDS + 1)]
    ct = [[2 * math.pi * (k + rng.uniform(-0.18, 0.18)) / SECTORS for k in range(SECTORS)] for b in range(BANDS + 1)]

    def corner(b, k):
        kk = k % SECTORS
        th = ct[b][kk] + (2 * math.pi if k == SECTORS else 0.0)
        return cu[b][kk], th

    # zigzag along each shared edge: (u, theta) offsets for its inner points, keyed by the edge
    zig = {}

    def edge(a, b):
        key = (a, b) if a <= b else (b, a)
        if key not in zig:
            zig[key] = [(rng.uniform(-0.03, 0.03), rng.uniform(-0.09, 0.09)) for _ in range(n - 1)]
        pts = zig[key]
        return pts if key == (a, b) else pts[::-1]

    def edge_curve(a, b, pa, pb, flat=False):
        """n + 1 (u, theta) points from corner pa to pb, the inner ones zigzagged (not round
        the poles: nothing meets them there, so a zigzag would leave a hole)."""
        offs = [(0.0, 0.0)] * (n - 1) if flat else edge(a, b)
        out = [pa]
        for i in range(1, n):
            f = i / n
            du, dt = offs[i - 1]
            out.append((pa[0] + (pb[0] - pa[0]) * f + du, pa[1] + (pb[1] - pa[1]) * f + dt))
        return out + [pb]

    m = Mesh()
    centres = []
    for b in range(BANDS):
        for k in range(SECTORS):
            bone = b * SECTORS + k
            c00, c10, c01, c11 = corner(b, k), corner(b, k + 1), corner(b + 1, k), corner(b + 1, k + 1)
            ids = [("c", b, k), ("c", b, k + 1), ("c", b + 1, k), ("c", b + 1, k + 1)]
            bottom = edge_curve(("h", b, k), ("h", b, k + 1), c00, c10, flat=b == 0)
            top = edge_curve(("h", b + 1, k), ("h", b + 1, k + 1), c01, c11, flat=b + 1 == BANDS)
            left = edge_curve(("v", b, k), ("v", b + 1, k), c00, c01)
            right = edge_curve(("v", b, k + 1), ("v", b + 1, k + 1), c10, c11)
            del ids

            def param(i, j):
                """Coons patch over the four zigzag edges: i across the sector, j up the band."""
                s, t = i / n, j / n
                u = ((1 - t) * bottom[i][0] + t * top[i][0] + (1 - s) * left[j][0] + s * right[j][0]
                     - ((1 - s) * (1 - t) * c00[0] + s * (1 - t) * c10[0] + (1 - s) * t * c01[0] + s * t * c11[0]))
                th = ((1 - t) * bottom[i][1] + t * top[i][1] + (1 - s) * left[j][1] + s * right[j][1]
                      - ((1 - s) * (1 - t) * c00[1] + s * (1 - t) * c10[1] + (1 - s) * t * c01[1] + s * t * c11[1]))
                return min(1.0, max(0.0, u)), th

            grid = [[param(i, j) for i in range(n + 1)] for j in range(n + 1)]
            outer = [[m.vert(*surface(u, th), bone, shell_paint(u)) for (u, th) in row] for row in grid]
            inner_paint = (INNER, GLOW, 140, 255)
            inner = []
            for row in grid:
                r = []
                for (u, th) in row:
                    p, nn = surface(u, th, THICK)
                    r.append(m.vert(p, (-nn[0], -nn[1], -nn[2]), bone, inner_paint))
                inner.append(r)
            for j in range(n):
                for i in range(n):
                    m.quad(outer[j][i], outer[j][i + 1], outer[j + 1][i + 1], outer[j + 1][i])
                    m.quad(inner[j][i], inner[j + 1][i], inner[j + 1][i + 1], inner[j][i + 1])
            # the walls round the piece (the broken edge, shell-coloured)
            ring = ([(i, 0) for i in range(n)] + [(n, j) for j in range(n)] +
                    [(i, n) for i in range(n, 0, -1)] + [(0, j) for j in range(n, 0, -1)])
            pts = [m.pos[v] for row in outer for v in row]
            centre = tuple(sum(p[x] for p in pts) / len(pts) for x in range(3))
            centres.append(centre)
            sub = lambda p, q: (p[0] - q[0], p[1] - q[1], p[2] - q[2])  # noqa: E731
            cross = lambda p, q: (p[1] * q[2] - p[2] * q[1], p[2] * q[0] - p[0] * q[2], p[0] * q[1] - p[1] * q[0])  # noqa: E731
            dot = lambda p, q: p[0] * q[0] + p[1] * q[1] + p[2] * q[2]  # noqa: E731
            for a, c in zip(ring, ring[1:] + ring[:1]):
                pa, pc = m.pos[outer[a[1]][a[0]]], m.pos[outer[c[1]][c[0]]]
                qa, qc = m.pos[inner[a[1]][a[0]]], m.pos[inner[c[1]][c[0]]]
                if dot(sub(pc, pa), sub(pc, pa)) < 1e-12:
                    continue  # a pole: nothing to wall
                na = m.nrm[outer[a[1]][a[0]]]
                mid = tuple((pa[x] + pc[x]) / 2 for x in range(3))
                away = sub(mid, centre)  # out of the piece, along the shell
                away = sub(away, tuple(na[x] * dot(away, na) for x in range(3)))
                ls = math.sqrt(dot(away, away)) or 1.0
                side = tuple(v / ls for v in away)
                wall = (BASE, BASE, 0, 0)
                o0, o1 = m.vert(pa, side, bone, wall), m.vert(pc, side, bone, wall)
                i0, i1 = m.vert(qa, side, bone, wall), m.vert(qc, side, bone, wall)
                if dot(cross(sub(qa, pa), sub(qc, pa)), side) >= 0:  # front face out of the piece
                    m.quad(o0, i0, i1, o1)
                else:
                    m.quad(o0, o1, i1, i0)
    return m, centres


def tangent_frame(u, th):
    p, n = surface(u, th)
    up = (-n[2] * n[0], -n[2] * n[1], 1 - n[2] * n[2])  # z projected onto the tangent plane
    lu = math.sqrt(sum(c * c for c in up)) or 1.0
    up = tuple(c / lu for c in up)
    right = (up[1] * n[2] - up[2] * n[1], up[2] * n[0] - up[0] * n[2], up[0] * n[1] - up[1] * n[0])
    return p, n, right, up


def decal_disc(m, u, th, size, bone, paint):
    p, n, right, up = tangent_frame(u, th)
    centre = m.vert(tuple(p[i] + n[i] * LIFT for i in range(3)), n, bone, paint)
    ring = []
    for k in range(6):
        a = 2 * math.pi * k / 6
        q = tuple(p[i] + (right[i] * math.cos(a) + up[i] * math.sin(a)) * size + n[i] * LIFT for i in range(3))
        ring.append(m.vert(q, n, bone, paint))
    for k in range(6):
        m.tri(centre, ring[k], ring[(k + 1) % 6])


def decal_line(m, pts, width, bone, paint):
    """A strip along (u, theta) points on the shell, facing out."""
    verts = []
    for i, (u, th) in enumerate(pts):
        p, n = surface(u, th)
        a = pts[max(0, i - 1)]
        b = pts[min(len(pts) - 1, i + 1)]
        pa, _ = surface(*a)
        pb, _ = surface(*b)
        t = tuple(pb[k] - pa[k] for k in range(3))
        side = (n[1] * t[2] - n[2] * t[1], n[2] * t[0] - n[0] * t[2], n[0] * t[1] - n[1] * t[0])
        ls = math.sqrt(sum(c * c for c in side)) or 1.0
        side = tuple(c / ls * width * 0.5 for c in side)
        lift = tuple(p[k] + n[k] * LIFT for k in range(3))
        left = m.vert(tuple(lift[k] + side[k] for k in range(3)), n, bone, paint)
        right = m.vert(tuple(lift[k] - side[k] for k in range(3)), n, bone, paint)
        verts.append((left, right))
    for (al, ar), (bl, br) in zip(verts, verts[1:]):
        m.quad(ar, br, bl, al)


def write_ecm(path, m, shards, centres):
    """.ecm v3 (src/core/model.cpp): the root and cap bones and a bone per shard, the shell
    mesh (kind body) and the shards mesh (kind part, group GROUP_SHARDS)."""
    bones = [("root", -1, (0.0, 0.0, 0.0)), ("cap", 0, (0.0, 0.0, SEAM * H))]
    bones += [(f"s{i:02d}", 0, c) for i, c in enumerate(centres)]
    out = bytearray(b"ECM1" + struct.pack("<HH", 3, len(bones)))  # v2: 32-byte palettes; v3: UVs and regions
    for name, parent, (x, y, z) in bones:
        out += struct.pack("<16sbB2x", name.encode(), parent, 0)
        for row in ((1, 0, 0, x), (0, 1, 0, y), (0, 0, 1, z)):
            out += struct.pack("<4f", *row)
    nb = len(bones)
    out += struct.pack("<3f", 1, 1, 1) * nb               # hatchling bone scales
    out += struct.pack("<2f", 1, 1) * nb * 3              # build multipliers
    out += struct.pack("<3f", 0, 0, 0) * nb               # idle pose
    out += struct.pack("<f", 0) * nb                      # young head lift
    out += struct.pack("<H", 2)
    write_mesh(out, m, b"shell", 0, 255, [ROOT, CAP])
    write_mesh(out, shards, b"shards", 2, GROUP_SHARDS, [2 + i for i in range(len(centres))])
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    Path(path).write_bytes(bytes(out))
    return len(out)


def write_mesh(out, m, name, kind, group, palette):
    assert len(m.pos) < 65536 and len(m.idx) < 65536 and len(palette) <= 25
    out += struct.pack("<16sBBBBB32sB3x4fHH", name, kind, group, 0, 0, len(palette),
                       bytes(palette + [0] * (32 - len(palette))), 1, 1.0, 0.0, 0.0, 0.0, len(m.pos), len(m.idx))
    for p in m.pos:
        out += struct.pack("<3f", *p)
    for n in m.nrm:
        out += struct.pack("<3f", *n)
    for s in m.skin:
        out += struct.pack("<4B", *s)
    for pt in m.paint:
        out += struct.pack("<4B", *pt)
    # The dragon skin texture's clean corner and the never-dirty region (dragon_texture.py
    # CLEAN_UV, REGION_CLEAN): the egg is shaded exactly as before.
    out += struct.pack("<2f", 0.97, 0.97) * len(m.pos)
    out += bytes([8] * len(m.pos))
    out += struct.pack(f"<{len(m.idx)}H", *m.idx)


def preview(m, prefix):
    """Blender renders of the egg: warm and uncracked, all cracks glowing, and the cap off."""
    import bpy
    from mathutils import Matrix, Vector

    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_EEVEE"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = scene.render.resolution_y = 400
    scene.view_settings.view_transform = "Standard"
    world = bpy.data.worlds.new("w")
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.03, 0.02, 0.04, 1)
    me = bpy.data.meshes.new("egg")
    me.from_pydata([Vector(p) for p in m.pos], [], [m.idx[i:i + 3] for i in range(0, len(m.idx), 3)])
    obj = bpy.data.objects.new("egg", me)
    scene.collection.objects.link(obj)
    mat = bpy.data.materials.new("egg")
    mat.use_backface_culling = True
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    attr = nt.nodes.new("ShaderNodeAttribute")
    attr.attribute_name = "c"
    em = nt.nodes.new("ShaderNodeEmission")
    outn = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(attr.outputs["Color"], em.inputs["Color"])
    nt.links.new(em.outputs[0], outn.inputs["Surface"])
    me.materials.append(mat)
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam.data.lens = 60
    target = Vector((0, 0, 0.5))
    cam.location = target + Vector((-0.35, -0.9, 0.32)).normalized() * 3.2
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
    light_dir = Vector((-0.4, -0.6, 0.7)).normalized()
    shell, speck, inner, glow = (0.98, 0.93, 0.86), (0.62, 0.33, 0.22), (0.92, 0.82, 0.72), (1.0, 0.55, 0.16)
    hot = (0.82, 0.5, 0.2)  # as eggPalette: the glow, a little golden (the glow doubles it)
    for shot, cracks, lift in (("warm", 0, 0.0), ("cracked", 3, 0.0), ("open", 3, 1.0)):
        slot = {BASE: (shell, 0), ACCENT: (speck, 0), INNER: (inner, 0), GLOW: (glow, 0.9)}
        for k, s in enumerate((CRACK1, CRACK2, CRACK3)):
            slot[s] = (hot, 1.0) if cracks > k else (shell, 0.0)
        # positions: lift the cap (bone 1) like src/core/egg.cpp
        tilt = Matrix.Translation((0, 0.25, SEAM * H + 0.45 * lift)) @ Matrix.Rotation(-0.9 * lift, 4, "X") @ \
            Matrix.Translation((0, -0.25, -SEAM * H))
        for i, v in enumerate(me.vertices):
            p = Vector(m.pos[i])
            v.co = tilt @ p if m.skin[i][0] == CAP else p
        if "c" in me.color_attributes:
            me.color_attributes.remove(me.color_attributes["c"])
        ca = me.color_attributes.new("c", "FLOAT_COLOR", "POINT")
        for i in range(len(m.pos)):
            a, b, mixw, emissive = m.paint[i]
            t = mixw / 255
            ca_, aa = slot.get(a, ((1, 0, 1), 0))
            cb, ab = slot.get(b, ((1, 0, 1), 0))
            col = [ca_[k] + (cb[k] - ca_[k]) * t for k in range(3)]
            alpha = (aa + (ab - aa) * t) * emissive / 255
            n = Vector(m.nrm[i])
            if m.skin[i][0] == CAP:
                n = (tilt.to_3x3() @ n).normalized()
            lit = 0.42 + (0.57 if n.dot(light_dir) > 0.45 else (0.35 if n.dot(light_dir) > 0.12 else 0.0))
            ca.data[i].color = (*[min(1.0, c * lit + c * alpha) ** 2.2 for c in col], 1.0)
        me.update()
        scene.render.filepath = f"{prefix}_{shot}.png"
        bpy.ops.render.render(write_still=True)


def main():
    m = build()
    shards, centres = build_shards()
    print(f"[egg] {len(m.pos)} vertices, {len(m.idx) // 3} triangles; "
          f"{SHARDS} shards: {len(shards.pos)} vertices, {len(shards.idx) // 3} triangles")
    if OUT:
        print(f"[egg] {OUT}: {write_ecm(OUT, m, shards, centres)} bytes")
    if RENDER:
        preview(m, RENDER)


main()
