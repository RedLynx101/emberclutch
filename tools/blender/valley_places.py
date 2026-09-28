"""Emberclutch: the valley's places (Beta WP4, docs/plan/beta-builders.md A), built by script.

  blender -b -P tools/blender/valley_places.py -- --out C:/abs/romfs/valley/places
        [--only market,den] [--preview C:/abs/build/places] [--json C:/abs/tools/valley/places.json]
        [--draft (report budgets, don't stop)] [--cam tx,ty,tz,radius,azimuth,elevation (close-ups)]

Writes one static mesh per place, <out>/<id>.esm (.esm v1, the den room's format: see
den_model.py write_esm and src/core/static_mesh.cpp), and tools/valley/places.json (the data
the game needs to stand each place in the valley; 1.0's places add named "anchors", turned into
src/core/places_data.inc by tools/valley/gen_places.py). With --preview, renders each place from
a three-quarter view above (day, evening and night side by side: <preview>/<id>.png) and a
contact sheet of all eighteen (<preview>/contact_sheet.png). Pass absolute paths.

It reads the landscape (romfs/valley/skyreach.evl) for each place's anchor and the ground the
game draws there (Place.gz): the arena's floor and 1.0's four places (the caldera, the glade, the
cove, the hollow) sit on it, so rebuild them when the landscape changes. See docs/tech/places.md.

Each place is in its own frame: metres, Z up, the origin on the ground at its anchor, +Y its
front (the way it faces). Parts (file order): `solid` (opaque; back faces culled, so thin
things are built double-sided), `lantern` (the festival lantern's post and lantern, opaque),
`sails` (the mill's, opaque, turned by the game round places.json "hub" about "hub_axis"),
`glow` (additive: windows and lamps, black by day, warm by night) and `lantern_light`
(additive and flickered: the lit lantern's glow, drawn once it is lit).

Lighting is baked into the vertex colours for the three sets (day, evening, night; the den's
order, src/core/daylight.hpp): albedo x (a soft sky ambient + the sun, or the low evening sun,
or the moon, with soft ray-traced shadows) x ambient occlusion (rays against the place and the
ground), plus the warm light of windows and lamps in the evening and at night. The sun is set
in each place's frame: from the front and a little to its right, high, as the valley's sun
(tools/valley/make_valley.py SUN) falls on a place facing south; so every front is lit.
"""
import json
import math
import os
import struct
import sys

import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree
from mathutils.geometry import tessellate_polygon
from mathutils.noise import noise

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


OUT = arg("--out")
CAM = arg("--cam")  # preview close-ups: "tx,ty,tz,radius,azimuth,elevation"
ONLY = [s for s in arg("--only", "").split(",") if s]
PREVIEW = arg("--preview")
DRAFT = "--draft" in argv  # iterate: report the budget, don't stop on it
JSON_OUT = arg("--json", os.path.join(ROOT, "tools", "valley", "places.json"))

# ValleyPlace order (src/core/valley.hpp); 1.0 (D90) adds the caldera, the glade, the cove and
# the hollow.
PLACE_IDS = ("den", "market", "stone", "sanctuary", "vault", "trailhead", "arena", "lake",
             "keeper", "isles", "orchard", "mill", "grotto", "ruins", "caldera", "glade", "cove", "hollow")
BUDGET = {"market": 2500}  # triangles, every part counted; the others 1,500
DEFAULT_BUDGET = 1500
EVL = os.path.join(ROOT, "romfs", "valley", "skyreach.evl")  # the landscape (tools/valley/make_valley.py)

# ------------------------------------------------------------------------------ lighting
SETS = ("day", "evening", "night")
# Toward the light (place frame), its colour and strength.
SUN = {"day": ((0.45, 0.50, 0.74), (1.0, 0.95, 0.84), 0.46),
       "evening": ((0.82, 0.36, 0.26), (1.0, 0.64, 0.40), 0.56),
       "night": ((-0.38, 0.52, 0.76), (0.60, 0.70, 1.0), 0.24)}
SKY = {"day": (0.73, 0.73, 0.78), "evening": (0.57, 0.46, 0.53), "night": (0.25, 0.29, 0.47)}
BOUNCE = {"day": (0.46, 0.42, 0.32), "evening": (0.40, 0.28, 0.24), "night": (0.10, 0.11, 0.18)}
GLOW = {"day": 0.0, "evening": 0.55, "night": 1.0}        # windows and lamps
LIT = {"day": 0.70, "evening": 0.88, "night": 1.0}        # a lit festival lantern
CRYSTAL = {"day": 0.40, "evening": 0.70, "night": 1.0}    # the grotto's crystals (a dark cave)
LAVA = {"day": 0.62, "evening": 0.85, "night": 1.0}       # the caldera's lava and braziers (always alight)
MOONPETAL = {"day": 0.22, "evening": 0.62, "night": 1.0}  # the glade's flowers: soft by day, bright at night
FROST = {"day": 0.30, "evening": 0.62, "night": 1.0}      # the hollow's cold glow (a shaded bowl)
LAMP_LIGHT = {"day": 0.0, "evening": 0.45, "night": 1.0}  # windows' and lamps' light on things near
BACKDROP = {"day": (0.66, 0.82, 0.95), "evening": (0.96, 0.70, 0.56), "night": (0.10, 0.12, 0.26)}
WARM = (1.0, 0.70, 0.36)       # window and lamp light
LANTERN_GLOW = (1.0, 0.60, 0.26)

AO_REACH, AO_STRENGTH = 2.4, 0.42  # ambient occlusion: ray length (m) and how dark it gets
ADDITIVE, FLICKER = 1, 2  # mesh flags (.esm)
NOSHADOW, NOAO = 1, 2     # vertex flags (bake)
PART_ORDER = ("solid", "lantern", "sails", "glow", "lantern_light")
PART_FLAGS = {"glow": ADDITIVE, "lantern_light": ADDITIVE | FLICKER}

# ------------------------------------------------------------------------------ palette
CREAM = (0.97, 0.89, 0.72)
PEACH = (0.98, 0.80, 0.64)
BUTTER = (0.98, 0.90, 0.60)
MINT = (0.80, 0.90, 0.76)
TIMBER = (0.50, 0.32, 0.20)
DARKWOOD = (0.38, 0.25, 0.16)
WOOD = (0.64, 0.44, 0.27)
LIGHTWOOD = (0.80, 0.62, 0.40)
TERRACOTTA = (0.88, 0.44, 0.28)
ORANGE_ROOF = (0.94, 0.58, 0.30)
BERRY_ROOF = (0.74, 0.36, 0.40)
SLATE = (0.44, 0.52, 0.68)
THATCH = (0.90, 0.72, 0.42)
STONE = (0.74, 0.70, 0.64)
STONE_WARM = (0.82, 0.74, 0.62)
STONE_COOL = (0.66, 0.68, 0.74)
COBBLE = (0.82, 0.72, 0.56)
EARTH = (0.66, 0.52, 0.36)
SOIL = (0.46, 0.33, 0.22)
SAND = (0.86, 0.74, 0.54)
MOSS = (0.46, 0.66, 0.30)
LEAF = (0.40, 0.64, 0.30)
LEAF_DARK = (0.28, 0.50, 0.26)
GRASS = (0.54, 0.76, 0.38)
STRAW = (0.92, 0.76, 0.42)
DOOR_TEAL = (0.28, 0.58, 0.58)
DOOR_GREEN = (0.40, 0.62, 0.36)
DOOR_BLUE = (0.36, 0.50, 0.74)
DOOR_RED = (0.80, 0.36, 0.30)
GLASS = (0.56, 0.70, 0.80)
CLOTH_RED = (0.92, 0.40, 0.32)
CLOTH_TEAL = (0.38, 0.72, 0.66)
CLOTH_YELLOW = (0.99, 0.80, 0.34)
CLOTH_CREAM = (0.99, 0.95, 0.82)
CLOTH_BLUE = (0.40, 0.56, 0.86)
CLOTH_PINK = (0.96, 0.58, 0.66)
CLOTH_LILAC = (0.70, 0.58, 0.90)
PAPER = (0.90, 0.42, 0.34)      # the festival lantern's paper, unlit
GOLD = (0.98, 0.78, 0.30)
IRON = (0.30, 0.30, 0.34)
SNOW = (0.95, 0.97, 1.0)
ICE = (0.66, 0.86, 0.98)
WATER = (0.34, 0.62, 0.72)
FLOWERS = ((0.98, 0.52, 0.64), (1.0, 0.86, 0.34), (0.99, 0.98, 0.94), (0.74, 0.60, 0.94), (0.99, 0.62, 0.36))


# ------------------------------------------------------------------------------ geometry
class Emit:
    """An emissive vertex colour: rgb x fade(p) x table[set]."""

    def __init__(self, rgb, table=GLOW, fade=None):
        self.rgb, self.table, self.fade = rgb, table, fade


class Part:
    def __init__(self, name):
        self.name, self.flags = name, PART_FLAGS.get(name, 0)
        self.P, self.N, self.C, self.E, self.F, self.T = [], [], [], [], [], []

    def vert(self, p, n, col, emit, vflags):
        self.P.append(p.copy())
        self.N.append(n.normalized() if n.length > 1e-9 else Vector((0, 0, 1)))
        self.C.append(col)
        self.E.append(emit)
        self.F.append(vflags)
        return len(self.P) - 1

    def tri(self, a, b, c):
        self.T.append((a, b, c))


def paint(col, p, n, jit):
    """(albedo, emit) for a vertex from a colour spec: rgb, fn(p, n) -> rgb, or Emit."""
    if isinstance(col, Emit):
        k = col.fade(p) if col.fade else 1.0
        return None, (tuple(c * k for c in col.rgb), col.table)
    c = col(p, n) if callable(col) else col
    if jit:
        j = 1.0 + jit * noise(p * 1.37 + Vector((3.1, 7.7, 1.3)))
        c = tuple(max(0.0, min(1.0, v * j)) for v in c)
    return c, None


I4 = Matrix.Identity(4)


def T(x=0.0, y=0.0, z=0.0, rz=0.0, s=None, rx=0.0, ry=0.0):
    """Translate, then rotate (Z, then Y, then X, radians), then scale (number or triple)."""
    m = Matrix.Translation(Vector((x, y, z))) @ Matrix.Rotation(rz, 4, "Z")
    if ry:
        m = m @ Matrix.Rotation(ry, 4, "Y")
    if rx:
        m = m @ Matrix.Rotation(rx, 4, "X")
    if s is not None:
        s = (s, s, s) if isinstance(s, (int, float)) else s
        m = m @ Matrix.Diagonal((s[0], s[1], s[2], 1.0))
    return m


def V(*a):
    return Vector(a if len(a) == 3 else a[0])


def _nmat(M):
    return M.to_3x3().inverted_safe().transposed()


def face(part, pts, normal, col, M=I4, jit=0.05, vflags=0, double=False):
    """A flat polygon (planar, any winding): tessellated, oriented along `normal` (local)."""
    P = [M @ V(p) for p in pts]
    n = (_nmat(M) @ V(normal)).normalized()
    tris = tessellate_polygon([P])
    for side in ((n, 1), (-n, -1)) if double else ((n, 1),):
        nn = side[0]
        ids = [part.vert(p, nn, *paint(col, p, nn, jit), vflags) for p in P]
        for a, b, c in tris:
            fn = (P[b] - P[a]).cross(P[c] - P[a])
            if fn.dot(nn) < 0:
                b, c = c, b
            part.tri(ids[a], ids[b], ids[c])


def quad(part, a, b, c, d, col, M=I4, jit=0.05, vflags=0, double=False):
    P = [V(a), V(b), V(c), V(d)]
    n = (P[1] - P[0]).cross(P[2] - P[0]) + (P[2] - P[0]).cross(P[3] - P[0])
    face(part, P, n, col, M, jit, vflags, double)


def box(part, lo, hi, col, M=I4, skip=(), jit=0.05, vflags=0):
    x0, y0, z0 = lo
    x1, y1, z1 = hi
    faces = {"-z": ([(x0, y0, z0), (x0, y1, z0), (x1, y1, z0), (x1, y0, z0)], (0, 0, -1)),
             "+z": ([(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)], (0, 0, 1)),
             "-y": ([(x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1)], (0, -1, 0)),
             "+y": ([(x1, y1, z0), (x0, y1, z0), (x0, y1, z1), (x1, y1, z1)], (0, 1, 0)),
             "-x": ([(x0, y1, z0), (x0, y0, z0), (x0, y0, z1), (x0, y1, z1)], (-1, 0, 0)),
             "+x": ([(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)], (1, 0, 0))}
    for k, (pts, n) in faces.items():
        if k not in skip:
            face(part, pts, n, col, M, jit, vflags)


def walls(part, x0, y0, x1, y1, zs, col, M=I4, jit=0.05, sides="+x-x+y-y"):
    """Four walls of a box, each split into rows at heights zs (so shade under eaves stays up
    under the eaves)."""
    corners = {"+y": ((x1, y1), (x0, y1)), "-x": ((x0, y1), (x0, y0)), "-y": ((x0, y0), (x1, y0)),
               "+x": ((x1, y0), (x1, y1))}
    for k, (a, b) in corners.items():
        if k in sides:
            grid(part, [[Vector((a[0], a[1], z)), Vector((b[0], b[1], z))] for z in zs], col, M, jit=jit)


def grid(part, rows, col, M=I4, closed=False, smooth=True, flip=False, jit=0.05, vflags=0):
    """A surface through rows of points (local). Quads (r,c)(r,c+1)(r+1,c+1)(r+1,c) face the
    side given by the right-hand rule (columns x rows); flip turns them round."""
    R, K = len(rows), len(rows[0])
    P = [[M @ V(p) for p in row] for row in rows]
    cols = K if closed else K - 1
    eps = 1e-6

    def corners(r, c):
        c1 = (c + 1) % K
        q = [(r, c), (r, c1), (r + 1, c1), (r + 1, c)]
        return q[::-1] if flip else q

    quads = []
    for r in range(R - 1):
        for c in range(cols):
            q = corners(r, c)
            pts = [P[i][j] for i, j in q]
            keep = [k for k in range(4) if (pts[k] - pts[(k + 1) % 4]).length > eps]
            if len(keep) < 3:
                continue
            idx = [q[k] for k in keep]
            pp = [P[i][j] for i, j in idx]
            n = Vector((0, 0, 0))
            for k in range(len(pp)):
                n += pp[k].cross(pp[(k + 1) % len(pp)])
            quads.append((idx, n))
    if smooth:
        acc = {}
        for idx, n in quads:
            for ij in idx:
                acc[ij] = acc.get(ij, Vector((0, 0, 0))) + n
        for r in range(R):  # poles: one normal for the whole row
            row = P[r]
            if all((row[c] - row[0]).length < eps for c in range(K)):
                tot = sum((acc.get((r, c), Vector()) for c in range(K)), Vector((0, 0, 0)))
                for c in range(K):
                    acc[(r, c)] = tot
        ids = {}
        for idx, n in quads:
            vs = []
            for ij in idx:
                if ij not in ids:
                    p, nn = P[ij[0]][ij[1]], acc[ij]
                    ids[ij] = part.vert(p, nn, *paint(col, p, nn.normalized(), jit), vflags)
                vs.append(ids[ij])
            for k in range(1, len(vs) - 1):
                part.tri(vs[0], vs[k], vs[k + 1])
    else:
        for idx, n in quads:
            pp = [P[i][j] for i, j in idx]
            nn = n.normalized()
            vs = [part.vert(p, nn, *paint(col, p, nn, jit), vflags) for p in pp]
            for k in range(1, len(vs) - 1):
                part.tri(vs[0], vs[k], vs[k + 1])


def lathe(part, profile, segs, col, M=I4, sharp=(), smooth=True, a0=0.0, jit=0.05, vflags=0, lump=0.0,
          seed=0.0, flip=False, arc=None, caps=False):
    """A surface of revolution round local Z: profile [(r, z), ...] bottom to top (outside
    faces out). `sharp`: profile indices where the surface creases. `arc`: (from, to) angles
    (radians, counter-clockwise from +X) for part of a turn, with `caps` closing its ends."""
    cuts = [0] + sorted(i for i in sharp if 0 < i < len(profile) - 1) + [len(profile) - 1]

    def angle(k):
        return arc[0] + (arc[1] - arc[0]) * k / segs if arc else a0 + 2 * math.pi * k / segs

    def ring(r, z):
        out = []
        for k in range(segs + 1 if arc else segs):
            a = angle(k)
            d = Vector((math.cos(a), math.sin(a), 0))
            rr = r
            if lump and r > 1e-6:
                rr = r * (1 + lump * noise(d * 1.9 + Vector((seed, seed * 0.7, z * 0.8))))
            out.append(Vector((d.x * rr, d.y * rr, z)))
        return out
    for s in range(len(cuts) - 1):
        rows = [ring(r, z) for r, z in profile[cuts[s]:cuts[s + 1] + 1]]
        grid(part, rows, col, M, closed=not arc, smooth=smooth, flip=flip, jit=jit, vflags=vflags)
    if arc and caps:
        for a, sgn in ((arc[0], -1), (arc[1], 1)):
            pts = [Vector((math.cos(a) * r, math.sin(a) * r, z)) for r, z in profile]
            face(part, pts, Vector((-math.sin(a), math.cos(a), 0)) * sgn * (-1 if flip else 1), col, M, jit, vflags)


def blob(part, rx, ry, rz, segs, rings, col, M=I4, lump=0.0, seed=0.0, smooth=True, jit=0.05, vflags=0,
         bottom=True, zcut=None):
    """An ellipsoid (centre at the origin); bottom=False stops at the equator (a dome)."""
    t_end = math.pi
    t0 = 0.0 if bottom else math.pi / 2
    rows = []
    n = rings if bottom else max(1, rings // 2)
    for i in range(n + 1):
        t = t0 + (t_end - t0) * i / n
        r, z = math.sin(t), -math.cos(t)
        row = []
        for k in range(segs):
            a = 2 * math.pi * k / segs
            d = Vector((math.cos(a) * r, math.sin(a) * r, z))
            k2 = 1 + lump * noise(d * 1.7 + Vector((seed, seed * 1.3, seed * 0.4))) if lump and 0 < i < n else 1
            p = Vector((d.x * rx * k2, d.y * ry * k2, d.z * rz * k2))
            if zcut is not None:
                p.z = max(p.z, zcut)
            row.append(p)
        rows.append(row)
    grid(part, rows, col, M, closed=True, smooth=smooth, jit=jit, vflags=vflags)


def dome(part, r, h, segs, col, M=I4, rings=2, lump=0.0, seed=0.0, smooth=True, jit=0.05, vflags=0):
    """A mound: half an ellipsoid standing on the ground, open underneath."""
    blob(part, r, r, h, segs, rings * 2, col, M, lump, seed, smooth, jit, vflags, bottom=False)


def puff(part, at, r, col, M=I4, segs=5, h=None, vflags=0):
    """A little rounded pyramid (a flower head, a fruit): segs + segs triangles... cheap."""
    h = r * 0.8 if h is None else h
    p0 = V(at)
    ring = [p0 + Vector((math.cos(2 * math.pi * k / segs) * r, math.sin(2 * math.pi * k / segs) * r, 0))
            for k in range(segs)]
    top = p0 + Vector((0, 0, h))
    grid(part, [ring, [top] * segs], col, M, closed=True, smooth=True, jit=0.02, vflags=vflags)


def disc(part, r, segs, col, M=I4, z=0.0, rings=None, jit=0.05, vflags=0, smooth=True):
    """A flat disc facing up (concentric rings, for baking across it)."""
    radii = sorted(rings or [r], reverse=True)
    rows = [[Vector((math.cos(2 * math.pi * k / segs) * rr, math.sin(2 * math.pi * k / segs) * rr, z))
             for k in range(segs)] for rr in radii]
    rows.append([Vector((0, 0, z))] * segs)
    grid(part, rows, col, M, closed=True, smooth=smooth, jit=jit, vflags=vflags)


def cylinder(part, r, h, segs, col, M=I4, r_top=None, top=True, bottom=False, smooth=True, jit=0.05,
             vflags=0, a0=0.0):
    rt = r if r_top is None else r_top
    prof = ([(0, 0)] if bottom else []) + [(r, 0), (rt, h)] + ([(0, h)] if top else [])
    sharp = [i for i in range(1, len(prof) - 1)]
    lathe(part, prof, segs, col, M, sharp=sharp, smooth=smooth, jit=jit, vflags=vflags, a0=a0)


def prism(part, outline, depth, col, M=I4, sides=True, back=False, jit=0.05, vflags=0, side_col=None):
    """An outline in local XZ extruded from y=0 to y=depth: its front faces +Y."""
    front = [Vector((x, depth, z)) for x, z in outline]
    face(part, front, (0, 1, 0), col, M, jit, vflags)
    if back:
        face(part, [Vector((x, 0, z)) for x, z in outline], (0, -1, 0), col, M, jit, vflags)
    if sides:
        n = len(outline)
        area = sum(outline[i][0] * outline[(i + 1) % n][1] - outline[(i + 1) % n][0] * outline[i][1]
                   for i in range(n))
        for i in range(n):
            (x0, z0), (x1, z1) = outline[i], outline[(i + 1) % n]
            ex, ez = x1 - x0, z1 - z0
            out = Vector((ez, 0, -ex)) if area > 0 else Vector((-ez, 0, ex))
            face(part, [(x0, 0, z0), (x1, 0, z1), (x1, depth, z1), (x0, depth, z0)], out,
                 side_col or col, M, jit, vflags)


def sweep(part, path, radius, sides, col, M=I4, smooth=True, radii=None, jit=0.05, vflags=0, twist=0.0,
          caps=False, squash=1.0):
    """A tube along a path (parallel-transported frames)."""
    pts = [V(p) for p in path]
    rows, prev = [], None
    for i, p in enumerate(pts):
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        if prev is None:
            up = Vector((0, 0, 1)) if abs(t.z) < 0.9 else Vector((1, 0, 0))
            nrm = t.cross(up).normalized()
        else:
            nrm = (prev - t * prev.dot(t)).normalized()
        bi = t.cross(nrm)
        r = radii[i] if radii else radius
        rows.append([p + (nrm * math.cos(twist + 2 * math.pi * k / sides) +
                          bi * math.sin(twist + 2 * math.pi * k / sides) * squash) * r for k in range(sides)])
        prev = nrm
    grid(part, rows, col, M, closed=True, smooth=smooth, jit=jit, vflags=vflags)
    if caps:
        face(part, rows[0], pts[0] - pts[1], col, M, jit, vflags)
        face(part, rows[-1], pts[-1] - pts[-2], col, M, jit, vflags)


def ribbon(part, path, width, col, M=I4, normal_hint=(0, 1, 0), jit=0.03, vflags=0, widths=None):
    """A flat strip along a path, double-sided (vines, cords, cloth)."""
    pts = [V(p) for p in path]
    hint = V(normal_hint)
    left, right = [], []
    for i, p in enumerate(pts):
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        side = t.cross(hint)
        if side.length < 1e-6:
            side = t.cross(Vector((1, 0, 0)))
        side.normalize()
        w = (widths[i] if widths else width) * 0.5
        left.append(p - side * w)
        right.append(p + side * w)
    grid(part, [left, right], col, M, smooth=True, jit=jit, vflags=vflags)
    grid(part, [right, left], col, M, smooth=True, jit=jit, vflags=vflags)


def hug_ring(pl, r0, r1, segs, z, col):
    """A flat band round the origin from r0 to r1, facing up, z above the landscape's ground."""
    rows = []
    for rr in (r1, r0):
        row = []
        for k in range(segs):
            x, y = math.cos(2 * math.pi * k / segs) * rr, math.sin(2 * math.pi * k / segs) * rr
            row.append(Vector((x, y, pl.gz(x, y) + z)))
        rows.append(row)
    grid(pl.s, rows, col, I4, closed=True, smooth=False, jit=0.03)


def world(M, p):
    q = M @ V(p)
    return [round(q.x, 3), round(q.y, 3), round(q.z, 3)]


# ------------------------------------------------------------------------------ the landscape
class Landscape:
    """The valley's ground as the game has it (romfs/valley/skyreach.evl, written by
    tools/valley/make_valley.py): heights on a 4 m grid, each quad drawn as two triangles cut
    from its south-west corner to its north-east (core/valley buildValleyTile), its colours and
    the places' anchors. A model can then sit on the real ground (the arena's floor, the cove's
    jetty and shells) and the previews show the ground the game draws."""

    def __init__(self, path):
        data = open(path, "rb").read()
        assert data[:4] == b"EVL2", path
        n = struct.unpack_from("<H", data, 6)[0]
        self.n = n
        self.spacing, self.x0, self.y0, hmin, hmax, self.water, _ = struct.unpack_from("<7f", data, 8)
        off = 36
        k = (hmax - hmin) / 65535.0
        self.h = [hmin + v * k for v in struct.unpack_from(f"<{n * n}H", data, off)]
        off += 2 * n * n
        self.rgb = data[off:off + 3 * n * n]
        off += 3 * n * n
        off += 2 + struct.unpack_from("<H", data, off)[0] * 12  # props
        off += 2 + struct.unpack_from("<H", data, off)[0] * 16  # islands
        count = struct.unpack_from("<H", data, off)[0]
        off += 2
        self.anchors = {}
        for _ in range(count):
            pid, x, y, z, hd = struct.unpack_from("<Bffff", data, off)
            off += 17
            self.anchors[pid] = (x, y, z, hd)

    def _cell(self, x, y):
        n = self.n
        fx = min(max((x - self.x0) / self.spacing, 0.0), n - 1.001)
        fy = min(max((y - self.y0) / self.spacing, 0.0), n - 1.001)
        i, j = int(fx), int(fy)
        return i, j, fx - i, fy - j

    def height(self, x, y):
        """The drawn ground at (x, y): its triangle's plane (as the game's tiles, full detail)."""
        i, j, u, w = self._cell(x, y)
        n, h = self.n, self.h
        h00, h10, h01, h11 = h[j * n + i], h[j * n + i + 1], h[(j + 1) * n + i], h[(j + 1) * n + i + 1]
        if u >= w:
            return h00 + u * (h10 - h00) + w * (h11 - h10)
        return h00 + w * (h01 - h00) + u * (h11 - h01)

    def colour(self, x, y):
        i, j, u, w = self._cell(x, y)
        n, c = self.n, self.rgb
        out = []
        for k in range(3):
            a = c[(j * n + i) * 3 + k] * (1 - u) + c[(j * n + i + 1) * 3 + k] * u
            b = c[((j + 1) * n + i) * 3 + k] * (1 - u) + c[((j + 1) * n + i + 1) * 3 + k] * u
            out.append((a * (1 - w) + b * w) / 255.0)
        return tuple(out)


LAND = Landscape(EVL) if os.path.exists(EVL) else None


# ------------------------------------------------------------------------------ the place
class Place:
    def __init__(self, pid, title):
        self.id, self.title = pid, title
        # Its anchor in the valley (x, y, z, heading), from the landscape's file (None without it).
        self.anchor = LAND.anchors.get(PLACE_IDS.index(pid)) if LAND and pid in PLACE_IDS else None
        self.parts = {}
        self.lamps = []            # (pos, rgb, range, strength): warm light in the evening and night
        self.flat = 10.0
        self.door = None
        self.lantern = None
        self.solids = []
        self.extra = {}
        self.ground = [((-150, -150, 0), (150, -150, 0), (150, 150, 0), (-150, 150, 0))]  # occluders (AO)
        self.view = dict(radius=12.0, target=(0, 0, 1.5), azimuth=25.0, elevation=36.0)
        self.terrain = None        # preview only: fn(x, y) -> (z, rgb) for the stand-in ground
        self.terrain_size = 60.0
        self.water = None          # preview only: water level (z) for a stand-in sheet
        self.preview_extra = None  # preview only: fn(place) builds stand-ins (not exported)
        self.marks, self.last_mark = [], 0
        self.ao_reach, self.ao_strength = AO_REACH, AO_STRENGTH  # a cave: longer reach, darker
        self.ao_ground = []        # occluders for ambient occlusion only (the landscape's walls round it)

    def part(self, name):
        if name not in self.parts:
            self.parts[name] = Part(name)
        return self.parts[name]

    @property
    def s(self):
        return self.part("solid")

    @property
    def g(self):
        return self.part("glow")

    def solid(self, M, x, y, r):
        q = M @ Vector((x, y, 0))
        self.solids.append([round(q.x, 2), round(q.y, 2), round(r, 2)])

    def lamp(self, M, p, rng=4.5, strength=0.55, rgb=WARM, table=None):
        self.lamps.append((M @ V(p), rgb, rng, strength, table or LAMP_LIGHT))

    def world_xy(self, x, y):
        """A point in its frame, in the valley (forward = (sin h, -cos h), as core/place_layout)."""
        ax, ay, _, hd = self.anchor
        fx, fy = math.sin(hd), -math.cos(hd)
        return ax + fy * x + fx * y, ay - fx * x + fy * y

    def gz(self, x, y):
        """The landscape's ground under (x, y) of its frame, from its anchor's height (0 without
        the landscape's file): things set on it sit on the ground the game draws."""
        if not self.anchor:
            return 0.0
        return LAND.height(*self.world_xy(x, y)) - self.anchor[2]

    def water_z(self):
        """The valley's water level in its frame."""
        return LAND.water - self.anchor[2] if self.anchor else None

    def triangles(self):
        return sum(len(p.T) for p in self.parts.values())

    def mark(self, label):
        """Draft reports: triangles added since the last mark."""
        now = self.triangles()
        self.marks.append((label, now - self.last_mark))
        self.last_mark = now


# ------------------------------------------------------------------------------ shared props
def wall_col(base, z0=0.0, z1=3.0):
    """Plaster: a touch lighter up high, warmer low down."""
    def f(p, n):
        t = max(0.0, min(1.0, (p.z - z0) / max(0.1, z1 - z0)))
        return tuple(v * (0.92 + 0.08 * t) for v in base)
    return f


def moss_top(rock, moss, k0=0.35, k1=0.75):
    """Rock with moss (or snow) where it faces up."""
    def f(p, n):
        t = max(0.0, min(1.0, (n.z - k0) / (k1 - k0)))
        wob = 0.5 + 0.5 * noise(p * 0.9 + Vector((5.0, 1.0, 2.0)))
        t = max(0.0, min(1.0, t * (0.7 + 0.6 * wob)))
        return tuple(a + (b - a) * t for a, b in zip(rock, moss))
    return f


def window_round(pl, M, x, z, r=0.42, frame=WOOD, rng=4.0):
    """A round window on a wall facing local +Y at y=0: a bevelled frame, glass, the night glow."""
    W = M @ T(x, 0, z) @ Matrix.Rotation(-math.pi / 2, 4, "X")  # local Z -> wall normal (+Y)
    lathe(pl.s, [(r + 0.13, 0.01), (r, 0.08)], 8, frame, W, smooth=False, a0=math.pi / 8)
    pts = [(math.cos(2 * math.pi * k / 8 + math.pi / 8) * r, math.sin(2 * math.pi * k / 8 + math.pi / 8) * r, 0.05)
           for k in range(8)]
    face(pl.s, pts, (0, 0, 1), GLASS, W, jit=0.02)
    face(pl.g, [(px * 0.96, py * 0.96, 0.065) for px, py, _ in pts], (0, 0, 1), Emit(WARM), W)
    pl.lamp(M, (x, 0.7, z), rng)


def window_square(pl, M, x, z, w=0.8, h=0.95, frame=WOOD, shutters=None, rng=4.0):
    """A square window with a cross, on a wall facing local +Y at y=0."""
    s, hw, hh, f = pl.s, w / 2, h / 2, 0.12
    W = M @ T(x, 0, z)
    outer = [(-hw - f, 0.01, -hh - f), (hw + f, 0.01, -hh - f), (hw + f, 0.01, hh + f), (-hw - f, 0.01, hh + f)]
    inner = [(-hw, 0.08, -hh), (hw, 0.08, -hh), (hw, 0.08, hh), (-hw, 0.08, hh)]
    for k in range(4):
        quad(s, outer[k], outer[(k + 1) % 4], inner[(k + 1) % 4], inner[k], frame, W)
    face(s, [(-hw, 0.05, -hh), (hw, 0.05, -hh), (hw, 0.05, hh), (-hw, 0.05, hh)], (0, 1, 0), GLASS, W, jit=0.02)
    face(pl.g, [(-hw, 0.062, -hh), (hw, 0.062, -hh), (hw, 0.062, hh), (-hw, 0.062, hh)], (0, 1, 0), Emit(WARM), W)
    m = 0.045  # the cross, in front of the glow
    face(s, [(-m, 0.075, -hh), (m, 0.075, -hh), (m, 0.075, hh), (-m, 0.075, hh)], (0, 1, 0), frame, W)
    face(s, [(-hw, 0.075, -m), (hw, 0.075, -m), (hw, 0.075, m), (-hw, 0.075, m)], (0, 1, 0), frame, W)
    if shutters:
        for sx in (-1, 1):
            x0, x1 = sx * (hw + f + 0.02), sx * (hw + f + 0.02 + hw * 0.9)
            face(s, [(min(x0, x1), 0.04, -hh - 0.05), (max(x0, x1), 0.04, -hh - 0.05),
                     (max(x0, x1), 0.04, hh + 0.05), (min(x0, x1), 0.04, hh + 0.05)], (0, 1, 0), shutters, W)
    pl.lamp(M, (x, 0.7, z), rng)


def arch_outline(w, h, steps=4):
    """A door's outline in XZ: straight sides and a round top, from the bottom left, counter-clockwise."""
    r = w / 2
    pts = [(-r, 0.0), (r, 0.0)]
    for k in range(steps + 1):
        a = math.pi * k / steps
        pts.append((math.cos(a) * r, h - r + math.sin(a) * r))
    pts.append((-r, 0.0))
    # drop duplicates (the arc's ends sit on the sides)
    out = []
    for p in pts:
        if not out or abs(p[0] - out[-1][0]) + abs(p[1] - out[-1][1]) > 1e-6:
            out.append(p)
    if abs(out[0][0] - out[-1][0]) + abs(out[0][1] - out[-1][1]) < 1e-6:
        out.pop()
    return out


def door(pl, M, x, z0, w=1.1, h=2.05, col=DOOR_TEAL, frame=TIMBER, round_top=True, step=True):
    """A door on a wall facing local +Y at y=0 (bottom at z0): frame, door, knob, a step."""
    s = pl.s
    W = M @ T(x, 0, z0)
    if round_top:
        fo = arch_outline(w + 0.3, h + 0.15, 4)
        do = arch_outline(w, h, 4)
    else:
        fo = [(-w / 2 - 0.15, 0), (w / 2 + 0.15, 0), (w / 2 + 0.15, h + 0.15), (-w / 2 - 0.15, h + 0.15)]
        do = [(-w / 2, 0), (w / 2, 0), (w / 2, h), (-w / 2, h)]
    prism(s, fo, 0.07, frame, W)
    prism(s, do, 0.11, col, W, sides=False)
    puff(s, (0, 0, 0), 0.08, GOLD, W @ T(w * 0.3, 0.11, h * 0.46) @ Matrix.Rotation(-math.pi / 2, 4, "X"),
         segs=4, h=0.08)
    if step and z0 > 0.05:
        box(s, (-w / 2 - 0.3, 0.0, -0.05), (w / 2 + 0.3, 0.55, z0), STONE_WARM, W @ T(0, 0, -z0), skip=("-z",))


def gable_roof(pl, M, w, d, h, rh, col, ov=0.45, ovx=0.35, thick=0.24, bands=3, ridge=None):
    """A thick pitched roof, its ridge along local X, stepped in bands like big shingles."""
    s = pl.s
    b = d / 2 + ov
    slope = rh / (d / 2)
    ze = h - ov * slope
    x0, x1 = -w / 2 - ovx, w / 2 + ovx
    top = rh + h
    for sy in (1, -1):
        eave = Vector((0, sy * b, ze))
        ridge_p = Vector((0, 0, top))
        nrm = Vector((0, sy * rh, d / 2)).normalized()
        for i in range(bands):
            u0, u1 = i / bands, (i + 1) / bands
            p0 = eave.lerp(ridge_p, u0) + nrm * thick
            p1 = eave.lerp(ridge_p, u1) + nrm * thick
            lip = nrm * 0.07 if i > 0 else Vector()
            shade = 1.0 - 0.07 * (i % 2)
            c = tuple(v * shade for v in col)
            lo = p0 + lip
            face(s, [(x0, lo.y, lo.z), (x1, lo.y, lo.z), (x1, p1.y, p1.z), (x0, p1.y, p1.z)], nrm, c, M, jit=0.04)
            if i > 0:  # the step's face, looking down the slope
                a, bq = p0, p0 + lip
                face(s, [(x0, a.y, a.z), (x1, a.y, a.z), (x1, bq.y, bq.z), (x0, bq.y, bq.z)], (0, sy, -0.2), c, M,
                     jit=0.04)
        # underside and eave edge
        e_top = eave + nrm * thick
        under = [(x0, eave.y, eave.z), (x1, eave.y, eave.z), (x1, 0, top), (x0, 0, top)]
        face(s, under, (0, 0, -1), tuple(v * 0.8 for v in col), M)
        face(s, [(x0, eave.y, eave.z), (x1, eave.y, eave.z), (x1, e_top.y, e_top.z), (x0, e_top.y, e_top.z)],
             (0, sy, -0.3), tuple(v * 0.9 for v in col), M)
    # the ends: the roof's thickness as a chevron
    nf = Vector((0, rh, d / 2)).normalized()
    nb = Vector((0, -rh, d / 2)).normalized()
    ridge_top = Vector((0, 0, top)) + Vector((0, 0, thick / max(0.3, nf.z)))
    for x, sx in ((x0, -1), (x1, 1)):
        pts = [(x, b, ze), (x, b + nf.y * thick, ze + nf.z * thick), (x, 0, ridge_top.z),
               (x, -b + nb.y * thick, ze + nb.z * thick), (x, -b, ze), (x, 0, top)]
        face(s, pts, (sx, 0, 0), tuple(v * 0.86 for v in col), M)
    if ridge:
        sweep(s, [(x0 - 0.05, 0, ridge_top.z - 0.02), (x1 + 0.05, 0, ridge_top.z - 0.02)], 0.16, 5, ridge, M,
              smooth=True, caps=True)


def gable_walls(pl, M, w, d, h, rh, col):
    """The triangles of wall under a ridge along X, at x = +-w/2."""
    for sx in (-1, 1):
        x = sx * w / 2
        face(pl.s, [(x, -d / 2, h), (x, d / 2, h), (x, 0, h + rh)], (sx, 0, 0), col, M)


def thatch_roof(pl, M, w, d, h, rh, col=THATCH, ov=0.55, seed=0.0):
    """A soft rounded thatch, its ridge along local X, hipped at both ends."""
    s = pl.s
    b = d / 2 + ov
    ze = h - 0.25
    prof = [(b * 0.96, -0.28), (b, 0.0), (b * 0.78, rh * 0.62), (0.0, rh * 1.0)]
    half = [(-y, z) for y, z in prof] + prof[::-1][1:]  # back eave -> ridge -> front eave
    L = w / 2 + ov * 0.6
    stations = [(-1.0, 0.36, 0.6), (-0.72, 0.9, 0.95), (0.0, 1.0, 1.0), (0.72, 0.9, 0.95), (1.0, 0.36, 0.6)]
    rows = []
    for u, sy, sz in stations:
        row = []
        for y, z in half:
            zz = ze + z * sz if z > 0 else ze + z
            row.append(Vector((u * L, y * sy, zz)))
        rows.append(row)
    for row in rows:
        for p in row:
            p += Vector((0, 0, 0.12 * noise(p * 0.8 + Vector((seed, 0, 0)))))
    grid(pl.s, rows, lambda p, n: tuple(c * (0.9 + 0.12 * max(0.0, n.z)) for c in col), M, smooth=True, flip=True,
         jit=0.08)
    # hip ends: fans to the end stations' middle, pushed out a little
    for row, sx in ((rows[0], -1), (rows[-1], 1)):
        c = sum(row, Vector()) / len(row)
        c.x += sx * 0.35
        for k in range(len(row) - 1):
            a, bq = row[k], row[k + 1]
            if (bq - a).cross(c - a).x * sx < 0:
                a, bq = bq, a
            _tri(pl.s, M, a, bq, c, col)
    # underside
    face(s, [(-L, -b * 0.96, ze - 0.28), (L, -b * 0.96, ze - 0.28), (L, b * 0.96, ze - 0.28),
             (-L, b * 0.96, ze - 0.28)], (0, 0, -1), tuple(v * 0.7 for v in col), M)


def _tri(part, M, a, b, c, col, jit=0.08):
    P = [M @ a, M @ b, M @ c]
    n = (P[1] - P[0]).cross(P[2] - P[0])
    ids = [part.vert(p, n, *paint(col, p, n.normalized(), jit), 0) for p in P]
    part.tri(*ids)


def chimney(pl, M, x, y, z0, z1, col=STONE, tilt=0.0):
    W = M @ T(x, y, z0, ry=tilt)
    box(pl.s, (-0.33, -0.33, 0), (0.33, 0.33, z1 - z0), col, W, skip=("-z", "+z"))
    box(pl.s, (-0.42, -0.42, z1 - z0), (0.42, 0.42, z1 - z0 + 0.22), tuple(c * 0.85 for c in col), W, skip=("-z",))


def flower_box(pl, M, x, z, w=1.1, box_col=WOOD, seed=0):
    s = pl.s
    W = M @ T(x, 0, z)
    box(s, (-w / 2, 0.02, -0.22), (w / 2, 0.36, 0.0), box_col, W, skip=("-z", "-y", "+z"))
    face(s, [(-w / 2, 0.02, -0.04), (w / 2, 0.02, -0.04), (w / 2, 0.36, -0.04), (-w / 2, 0.36, -0.04)], (0, 0, 1),
         LEAF_DARK, W)
    for k in range(3):
        px = -w / 2 + w * (k + 0.5) / 3
        puff(s, (px, 0.2, -0.04), 0.17, FLOWERS[(seed + k) % len(FLOWERS)] if k != 1 else LEAF, W, segs=4, h=0.2)


def flower_clump(pl, M, x, y, r=0.55, seed=0, heads=3, part=None):
    s = part or pl.s
    W = M @ T(x, y, 0)
    dome(s, r, r * 0.75, 6, LEAF, W, rings=2, lump=0.2, seed=seed)
    for k in range(heads):
        a = seed * 1.7 + k * 2.1
        rr = r * 0.5
        puff(s, (math.cos(a) * rr, math.sin(a) * rr, r * 0.55), 0.16, FLOWERS[(seed + k) % len(FLOWERS)], W,
             segs=4, h=0.12)


def bush(pl, M, x, y, r=1.0, seed=0, col=LEAF):
    W = M @ T(x, y, 0)
    blob(pl.s, r, r, r * 0.8, 7, 4, col, W @ T(0, 0, r * 0.55), lump=0.18, seed=seed, zcut=-r * 0.55)


def rock(pl, M, x, y, r, col=STONE, stretch=(1, 1, 0.7), seed=0.0, segs=7, rings=4, part=None, moss=None):
    W = M @ T(x, y, r * stretch[2] * 0.45)
    c = moss_top(col, moss) if moss else col
    blob(part or pl.s, r * stretch[0], r * stretch[1], r * stretch[2], segs, rings, c, W, lump=0.22, seed=seed,
         zcut=-r * stretch[2] * 0.45)


def festival_lantern(pl, M, x, y, rz=0.0, scale=1.0):
    """The festival lantern: a wooden crook post on a stone foot, a round paper lantern hanging
    from it. `lantern` holds it all; `lantern_light` the lit glow (shell and halo)."""
    s, L = pl.part("lantern"), pl.part("lantern_light")
    W = M @ T(x, y, 0, rz=rz, s=scale)
    lathe(s, [(0.46, -0.1), (0.42, 0.24), (0.0, 0.34)], 6, STONE, W, sharp=[1], smooth=True)
    box(s, (-0.13, -0.13, 0.28), (0.13, 0.13, 2.95), WOOD, W, skip=("-z",))
    sweep(s, [(0.0, 0, 2.6), (0.4, 0, 2.9), (0.86, 0, 2.88), (1.02, 0, 2.64)], 0.065, 4, DARKWOOD,
          W, smooth=False, twist=math.pi / 4)
    box(s, (1.005, -0.012, 2.26), (1.035, 0.012, 2.64), DARKWOOD, W, skip=("-z", "+z"))
    c = Vector((1.02, 0, 1.92))
    blob(s, 0.36, 0.36, 0.32, 6, 4, PAPER, W @ T(*c), smooth=False, jit=0.03)
    cylinder(s, 0.2, 0.09, 6, DARKWOOD, W @ T(c.x, c.y, c.z + 0.28), smooth=False)
    cylinder(s, 0.18, 0.001, 6, DARKWOOD, W @ T(c.x, c.y, c.z - 0.3), r_top=0.05, top=False, smooth=False)
    cylinder(s, 0.04, -0.26, 4, GOLD, W @ T(c.x, c.y, c.z - 0.3), r_top=0.09, top=False, bottom=True, smooth=False)
    # the light: a shell over the paper, and two crossed halo discs (bright middle, black rim)
    blob(L, 0.385, 0.385, 0.345, 6, 4, Emit(LANTERN_GLOW, LIT), W @ T(*c), smooth=False, jit=0)
    for a in (0.0, math.pi / 2):
        Wh = W @ T(*c) @ Matrix.Rotation(a, 4, "Z") @ Matrix.Rotation(math.pi / 2, 4, "X")
        cw = W @ c
        disc(L, 1.05, 8, Emit((1.0, 0.56, 0.22), LIT, fade=lambda p, cw=cw: 0.55 * max(0.0, 1 - (p - cw).length /
                                                                                      (1.0 * scale)) ** 1.5),
             Wh, jit=0)
    pl.lantern = world(W, c)
    pl.solid(W, 0, 0, 0.5 * scale)
    return W


def bench(pl, M, x, y, rz, w=1.8, col=WOOD):
    s = pl.s
    W = M @ T(x, y, 0, rz=rz)
    box(s, (-w / 2, -0.25, 0.42), (w / 2, 0.25, 0.52), col, W, skip=("-z",))
    box(s, (-w / 2, -0.3, 0.52), (w / 2, -0.2, 0.98), col, W, skip=("-z",))
    for sx in (-1, 1):
        box(s, (sx * w * 0.4 - 0.07, -0.22, 0), (sx * w * 0.4 + 0.07, 0.2, 0.42), DARKWOOD, W, skip=("-z", "+z"))
    pl.solid(W, 0, 0, w * 0.5)


def barrel(pl, M, x, y, r=0.38, h=0.9, col=WOOD):
    W = M @ T(x, y, 0)
    lathe(pl.s, [(r * 0.86, 0), (r, h * 0.5), (r * 0.86, h), (0, h)], 6, col, W, sharp=[2])
    lathe(pl.s, [(r * 1.02, h * 0.62), (r * 0.98, h * 0.72)], 6, IRON, W, smooth=False)
    pl.solid(W, 0, 0, r + 0.1)


def crate(pl, M, x, y, s=0.7, rz=0.0, col=LIGHTWOOD, fill=None):
    W = M @ T(x, y, 0, rz=rz)
    h = s * 0.75
    box(pl.s, (-s / 2, -s / 2, 0), (s / 2, s / 2, h), col, W, skip=("-z", "+z") if fill else ("-z",))
    if fill:
        face(pl.s, [(-s / 2, -s / 2, h * 0.85), (s / 2, -s / 2, h * 0.85), (s / 2, s / 2, h * 0.85),
                    (-s / 2, s / 2, h * 0.85)], (0, 0, 1), tuple(c * 0.6 for c in col), W)
        for k in range(3):
            px, py = (-0.2 + 0.4 * (k % 2)) * s, (-0.16 + 0.34 * (k // 2)) * s
            puff(pl.s, (px, py, h * 0.8), s * 0.26, fill, W, segs=5, h=s * 0.24)
    pl.solid(W, 0, 0, s * 0.7)


def fence_run(pl, M, pts, col=WOOD, post_col=None, rails=(0.45, 0.85), post_h=1.05, gap_every=None):
    """Posts at the points and rails between them (rails are double-sided planks)."""
    s = pl.s
    pc = post_col or tuple(c * 0.85 for c in col)
    for p in pts:
        box(s, (p[0] - 0.09, p[1] - 0.09, 0), (p[0] + 0.09, p[1] + 0.09, post_h), pc, M, skip=("-z",))
    for i in range(len(pts) - 1):
        a, b = V(pts[i][0], pts[i][1], 0), V(pts[i + 1][0], pts[i + 1][1], 0)
        span = M @ T(a.x, a.y, 0, rz=math.atan2(b.y - a.y, b.x - a.x))
        for z in rails:
            box(s, (0.0, -0.05, z - 0.08), ((b - a).length, 0.05, z + 0.08), col, span, skip=("-x", "+x"))


def awning(pl, M, w, depth, z_back, z_front, stripes, cols, sag=0.18, valance=0.28):
    """A striped cloth awning, double-sided: from the back edge (y=-depth/2) down to the front,
    puffed, with a scalloped valance hanging at the front."""
    s = pl.s
    n = stripes
    for i in range(n):
        x0, x1 = -w / 2 + w * i / n, -w / 2 + w * (i + 1) / n
        col = cols[i % len(cols)]
        rows = []
        for t in (0.0, 0.5, 1.0):
            y = -depth / 2 + depth * t
            z = z_back + (z_front - z_back) * t + sag * math.sin(math.pi * t)
            rows.append([Vector((x0, y, z)), Vector((x1, y, z))])
        grid(s, rows, col, M, smooth=True, flip=True, jit=0.02)
        grid(s, rows, tuple(c * 0.9 for c in col), M, smooth=True, flip=False, jit=0.02)
        # the scallop: a rounded tongue hanging under the front edge
        yf = depth / 2
        xm = (x0 + x1) / 2
        pts = [(x0, yf, z_front), (x1, yf, z_front), (xm + (x1 - x0) * 0.3, yf, z_front - valance * 0.75),
               (xm - (x1 - x0) * 0.3, yf, z_front - valance * 0.75)]
        face(s, pts, (0, 1, 0), col, M, double=True, jit=0.02)


def bunting(pl, M, a, b, sag=0.6, flags=7, cols=None, size=0.42):
    """A cord from a to b with little triangle flags (double-sided) hanging from it."""
    s = pl.s
    a, b = V(a), V(b)
    cols = cols or (CLOTH_RED, CLOTH_YELLOW, CLOTH_TEAL, CLOTH_CREAM, CLOTH_BLUE, CLOTH_PINK)
    along = (b - a)
    side = along.cross(Vector((0, 0, 1))).normalized()

    def at(t):
        return a.lerp(b, t) - Vector((0, 0, sag * 4 * t * (1 - t)))
    cord = [at(t / 3) for t in range(4)]
    ribbon(s, cord, 0.05, DARKWOOD, M, normal_hint=(0, 0, 1))
    dirn = along.normalized()
    for k in range(flags):
        t = (k + 0.5) / flags
        c = at(t)
        p0, p1 = c - dirn * size * 0.5, c + dirn * size * 0.5
        tip = c - Vector((0, 0, size * 1.1))
        face(s, [p0, p1, tip], side, cols[k % len(cols)], M, double=True, jit=0.02)


# ------------------------------------------------------------------------------ places
PLACES = {}


def place(pid, title):
    def deco(fn):
        PLACES[pid] = (title, fn)
        return fn
    return deco


def cottage(pl, M, w=7.0, d=5.4, h=3.0, ridge="x", roof="shingle", roof_col=TERRACOTTA, wall=CREAM,
            door_col=DOOR_TEAL, door_x=0.0, windows=("round", "round"), back_windows=1, side_window=False,
            chimney_at=None, flowers=(True, False), rh=None, sign=None, seed=0, timber=True, round_door=True,
            win_x=None, ridge_cap=False):
    """A storybook cottage, its door on the front (+Y), resting on a stone footing."""
    s = pl.s
    fz = 0.38
    rh = rh if rh is not None else (d / 2 if ridge == "x" else w / 2) * 0.95
    box(s, (-w / 2 - 0.14, -d / 2 - 0.14, -0.3), (w / 2 + 0.14, d / 2 + 0.14, fz), STONE, M, skip=("-z",))
    walls(s, -w / 2, -d / 2, w / 2, d / 2, (fz, h - 0.7, h), wall_col(wall, fz, h), M)
    if ridge == "x":
        gable_walls(pl, M, w, d, h, rh, wall_col(wall, fz, h))
        R = M
        rw, rd = w, d
    else:
        R = M @ Matrix.Rotation(math.pi / 2, 4, "Z")
        rw, rd = d, w
        gable_walls(pl, R, rw, rd, h, rh, wall_col(wall, fz, h))
    if roof == "thatch":
        thatch_roof(pl, R, rw, rd, h, rh * 1.05, THATCH if roof_col is None else roof_col, seed=seed)
    else:
        gable_roof(pl, R, rw, rd, h, rh, roof_col, ridge=tuple(c * 0.8 for c in roof_col) if ridge_cap else None)
    # timber: corner strips and a top beam on the front and back
    if timber:
        for sy, yy in ((1, d / 2), (-1, -d / 2)):
            F = M @ T(0, yy, 0) if sy > 0 else M @ T(0, yy, 0, rz=math.pi)
            for x in (-w / 2 + 0.12, w / 2 - 0.12):
                face(s, [(x - 0.12, 0.025, fz), (x + 0.12, 0.025, fz), (x + 0.12, 0.025, h), (x - 0.12, 0.025, h)],
                     (0, 1, 0), TIMBER, F, jit=0.04)
            face(s, [(-w / 2, 0.03, h - 0.24), (w / 2, 0.03, h - 0.24), (w / 2, 0.03, h), (-w / 2, 0.03, h)],
                 (0, 1, 0), TIMBER, F, jit=0.04)
    front = M @ T(0, d / 2, 0)
    door(pl, front, door_x, fz, col=door_col, round_top=round_door)
    wz = min(h - 0.95, 1.75)
    slots = win_x or (-w * 0.3, w * 0.3)
    for i, kind in enumerate(windows):
        x = slots[i]
        if kind == "round":
            window_round(pl, front, x, wz + 0.1, 0.44)
        else:
            window_square(pl, front, x, wz, shutters=DOOR_GREEN if seed % 2 else DOOR_BLUE)
        if flowers[i % len(flowers)]:
            flower_box(pl, front, x, wz - 0.62 if kind != "round" else wz - 0.5, seed=seed + i)
    if ridge == "y":  # a round window up in the front gable
        window_round(pl, front, 0, h + rh * 0.42, 0.36)
    back = M @ T(0, -d / 2, 0, rz=math.pi)
    for k in range(back_windows):
        x = 0 if back_windows == 1 else (-w * 0.25 if k == 0 else w * 0.25)
        window_square(pl, back, x, wz, shutters=None)
    if side_window:
        side = M @ T(w / 2, 0, 0, rz=-math.pi / 2)
        window_square(pl, side, 0, wz, w=0.7, h=0.8)
    if chimney_at is not None:
        cx, cy = chimney_at
        chimney(pl, M, cx, cy, h, h + rh + 0.9)
    if sign:
        sign(pl, front)
    # walk-round circles: the footprint as two or three circles
    n = max(1, int(round(w / d)))
    for k in range(n + 1):
        x = -w / 2 + d / 2 + (w - d) * k / max(1, n)
        pl.solid(M, x, 0, d / 2 + 0.3)


def well(pl, M):
    s = pl.s
    lathe(s, [(1.15, -0.1), (1.15, 0.85), (0.8, 0.95), (0.8, 0.5)], 9, STONE_WARM, M, sharp=[1, 2], lump=0.03)
    face(s, [(math.cos(2 * math.pi * k / 9) * 0.8, math.sin(2 * math.pi * k / 9) * 0.8, 0.55) for k in range(9)],
         (0, 0, 1), (0.20, 0.36, 0.46), M, jit=0.02)
    for sx in (-1, 1):
        box(s, (sx * 0.95 - 0.1, -0.1, 0.85), (sx * 0.95 + 0.1, 0.1, 2.3), WOOD, M, skip=("-z", "+z"))
    sweep(s, [(-1.0, 0, 1.75), (1.0, 0, 1.75)], 0.07, 4, DARKWOOD, M)
    cylinder(s, 0.18, 0.28, 5, WOOD, M @ T(0.1, 0, 1.15), r_top=0.21, bottom=True, smooth=False)
    box(s, (0.09, -0.01, 1.43), (0.11, 0.01, 1.75), DARKWOOD, M, skip=("-z", "+z"))
    # a little four-sided roof
    lathe(s, [(1.5, 2.25), (1.45, 2.4), (0.0, 3.35)], 4, ORANGE_ROOF, M, sharp=[1], smooth=False, a0=math.pi / 4)
    face(s, [(1.5 * math.cos(math.pi / 4 + k * math.pi / 2), 1.5 * math.sin(math.pi / 4 + k * math.pi / 2), 2.25)
             for k in range(4)], (0, 0, -1), tuple(c * 0.7 for c in ORANGE_ROOF), M)
    puff(s, (0, 0, 3.3), 0.12, GOLD, M, segs=4, h=0.2)
    pl.solid(M, 0, 0, 1.4)


def stall(pl, M, cols, kind, w=2.8, d=1.5):
    """A market stall: counter, four posts and a striped awning. kind: 'egg', 'goods', 'fruit'."""
    s = pl.s
    ch = 1.0
    box(s, (-w / 2, -d / 2 + 0.25, 0), (w / 2, d / 2, ch), WOOD, M, skip=("-z",))
    face(s, [(-w / 2 + 0.05, d / 2 + 0.01, 0.15), (w / 2 - 0.05, d / 2 + 0.01, 0.15),
             (w / 2 - 0.05, d / 2 + 0.01, 0.3), (-w / 2 + 0.05, d / 2 + 0.01, 0.3)], (0, 1, 0), DARKWOOD, M)
    box(s, (-w / 2 - 0.06, -d / 2 + 0.2, ch), (w / 2 + 0.06, d / 2 + 0.06, ch + 0.08), LIGHTWOOD, M, skip=("-z",))
    for sx in (-1, 1):
        for y, top in ((d / 2 + 0.02, 2.45), (-d / 2 - 0.25, 2.95)):
            box(s, (sx * w / 2 - 0.08, y - 0.08, 0), (sx * w / 2 + 0.08, y + 0.08, top), TIMBER, M, skip=("-z", "+z"))
    awning(pl, M @ T(0, 0.05, 0), w + 0.5, d + 0.9, 3.0, 2.45, 5, cols)
    spots = []
    if kind == "egg":
        # the round stand in the middle, a straw ring on a red cushion: the egg of the day sits here
        cylinder(s, 0.3, 0.3, 7, LIGHTWOOD, M @ T(0, 0.1, ch + 0.08), r_top=0.22, top=False)
        blob(s, 0.3, 0.3, 0.1, 7, 3, CLOTH_RED, M @ T(0, 0.1, ch + 0.4), jit=0.02)
        torus_nest(pl, M @ T(0, 0.1, ch + 0.45), 0.2, 0.06, 7, 3, STRAW)
        spots.append(world(M, (0, 0.1, ch + 0.5)))
        for sx in (-1, 1):  # a sign on the front of each post: a little painted egg (it faced into the post)
            B = M @ T(sx * w / 2, d / 2 + 0.105, 1.9)
            egg = [(math.cos(2 * math.pi * k / 7) * 0.13 * (1 - 0.15 * math.sin(2 * math.pi * k / 7)),
                    math.sin(2 * math.pi * k / 7) * 0.18) for k in range(7)]
            face(s, [(x, 0.0, z) for x, z in egg], (0, 1, 0), (0.98, 0.86, 0.52), B)
    elif kind == "goods":
        for k in range(4):
            x = -w / 2 + w * (k + 0.5) / 4
            cylinder(s, 0.26, 0.06, 6, CLOTH_CREAM if k % 2 else (0.86, 0.80, 0.94), M @ T(x, 0.12, ch + 0.08),
                     bottom=False, smooth=False, a0=math.pi / 6)
            spots.append(world(M, (x, 0.12, ch + 0.14)))
    else:
        for k, fc in enumerate(((0.98, 0.56, 0.22), (0.86, 0.26, 0.24), (0.62, 0.80, 0.30))):
            x = -w / 2 + w * (k + 0.5) / 3
            box(s, (x - 0.38, -0.2, ch + 0.08), (x + 0.38, 0.45, ch + 0.3), LIGHTWOOD, M, skip=("-z", "+z"))
            dome(s, 0.36, 0.2, 6, fc, M @ T(x, 0.12, ch + 0.26), rings=1, lump=0.1)
    pl.solid(M, -w / 4, 0, d * 0.6)
    pl.solid(M, w / 4, 0, d * 0.6)
    return spots


def torus_nest(pl, M, major, minor, segs, sides, col, part=None):
    """A ring (straw, iron, a pool's rim) round local Z, facing out. (Run 19: it was built inside
    out, flipped, so the Nesting Stone's straw ring showed only its far inside.)"""
    rows = []
    for j in range(sides + 1):
        b = 2 * math.pi * j / sides
        rows.append([Vector((math.cos(2 * math.pi * i / segs) * (major + math.cos(b) * minor),
                             math.sin(2 * math.pi * i / segs) * (major + math.cos(b) * minor),
                             math.sin(b) * minor * 0.8)) for i in range(segs)])
    grid(part or pl.s, rows, col, M, closed=True, smooth=True, jit=0.1)


def ground_patch(pl, M, r, segs, col, rings=None, z=0.05, skirt=0.25, hug=False):
    """A flat patch of paving or trodden earth, a hand's width proud of the ground, with a sloped
    edge down into it so it never shows a gap. hug: it follows the landscape's ground (pl.gz)
    instead of lying flat at the anchor's height (the ground there isn't quite level)."""
    s = pl.s
    radii = sorted(rings or [r], reverse=True)

    def ring(rr, zz):
        out = []
        for k in range(segs):
            p = M @ Vector((math.cos(2 * math.pi * k / segs) * rr, math.sin(2 * math.pi * k / segs) * rr, zz))
            if hug:
                p.z += pl.gz(p.x, p.y)
            out.append(p)
        return out
    rows = [ring(r + skirt, -0.12)] + [ring(rr, z) for rr in radii]
    centre = M @ Vector((0, 0, z))
    if hug:
        centre.z += pl.gz(centre.x, centre.y)
    rows.append([centre] * segs)
    grid(s, rows, col, I4, closed=True, smooth=False, jit=0.06, flip=False)


# ---------------------------------------------------------------------- Market Village
@place("market", "Market Village")
def build_market(pl):
    M = I4
    s = pl.s
    ground_patch(pl, M, 9.0, 16, lambda p, n: tuple(c * (0.95 + 0.05 * math.sin(p.to_2d().length * 2.2))
                                                    for c in COBBLE), rings=[9.0, 4.6])
    # (No cobbled road out of the square: the landscape's path to the lake runs there, run 19.)
    pl.mark("paving")
    well(pl, M)
    pl.mark("well")
    # cottages round the square, facing it (angle from +Y, radius)
    homes = [(180, 12.9, dict(w=7.4, d=5.4, roof_col=TERRACOTTA, wall=CREAM, door_col=DOOR_TEAL,
                              windows=("round", "round"), chimney_at=(1.9, -1.0), sign=shop_sign, seed=1)),
             (-124, 13.2, dict(w=5.8, d=6.4, ridge="y", roof="thatch", roof_col=None, wall=PEACH, door_col=DOOR_RED,
                               windows=("square", "square"), back_windows=0, seed=2, flowers=(True, True))),
             (124, 13.0, dict(w=6.4, d=5.2, roof_col=ORANGE_ROOF, wall=BUTTER, door_col=DOOR_BLUE,
                              windows=("square", "round"), chimney_at=(-1.8, -0.8), seed=3, back_windows=0)),
             (-64, 13.2, dict(w=6.6, d=5.2, roof_col=BERRY_ROOF, wall=CREAM, door_col=DOOR_GREEN,
                              windows=("round", "square"), seed=4)),
             (64, 13.2, dict(w=5.6, d=6.2, ridge="y", roof_col=SLATE, wall=MINT, door_col=DOOR_RED,
                             windows=("square", "square"), seed=5, flowers=(True, True)))]
    for ang, r, kw in homes:
        a = math.radians(ang)
        C = T(math.sin(a) * r, math.cos(a) * r, 0, rz=math.pi - a)
        cottage(pl, C, **kw)
        pl.mark(f"home{ang}")
        if ang == 180:
            pl.door = world(C, (0, kw["d"] / 2 + 0.9, 0))[:2]
    # the stalls, facing the well
    specs = [(-40, 6.0, (CLOTH_RED, CLOTH_CREAM), "egg"), (40, 6.0, (CLOTH_TEAL, CLOTH_CREAM), "goods"),
             (152, 6.4, (CLOTH_YELLOW, CLOTH_CREAM), "fruit")]
    for ang, r, cols, kind in specs:
        a = math.radians(ang)
        C = T(math.sin(a) * r, math.cos(a) * r, 0, rz=math.pi - a)
        spots = stall(pl, C, cols, kind)
        pl.mark(kind)
        if kind == "egg":
            pl.extra["egg_stand"] = spots[0]
        elif kind == "goods":
            pl.extra["goods_spots"] = spots
    # bunting from the well's roof out to posts round the square
    posts = [(16, 8.5), (94, 8.4), (-158, 8.4), (-94, 8.4), (-16, 8.5)]
    for ang, r in posts:
        a = math.radians(ang)
        x, y = math.sin(a) * r, math.cos(a) * r
        box(s, (x - 0.1, y - 0.1, 0), (x + 0.1, y + 0.1, 3.9), WOOD, M, skip=("-z", "+z"))
        puff(s, (x, y, 3.9), 0.17, GOLD, M, segs=4, h=0.24)
        bunting(pl, M, (x, y, 3.75), (math.sin(a) * 1.0, math.cos(a) * 1.0, 3.05), sag=0.55, flags=6)
        pl.solid(M, x, y, 0.3)
    pl.mark("bunting")
    # benches by the well, flower tubs, a barrel and crates
    bench(pl, M, -3.3, 0.4, math.radians(-95))
    bench(pl, M, 3.3, 0.4, math.radians(95))
    for x, y, sd in ((-2.5, 9.4, 1),):
        lathe(s, [(0.5, 0), (0.58, 0.5), (0.0, 0.5)], 6, TERRACOTTA, T(x, y, 0), sharp=[1])
        flower_clump(pl, T(x, y, 0.45), 0, 0, 0.5, seed=sd)
        pl.solid(M, x, y, 0.6)
    barrel(pl, M, 5.2, -6.9)
    crate(pl, M, 6.9, 5.9, 0.7, 0.4, fill=(0.62, 0.80, 0.30))
    crate(pl, M, -6.9, 5.8, 0.7, -0.3)
    pl.mark("props")
    festival_lantern(pl, M, 2.5, 9.9, rz=math.pi)
    pl.mark("lantern")
    pl.flat = 22.0
    pl.view = dict(radius=18.5, target=(0, -1.0, 1.5), azimuth=22.0, elevation=40.0)
    pl.terrain_size = 70


def shop_sign(pl, M):
    """The Market keeper's sign: a round board with an egg on it, hung from a bracket."""
    s = pl.s
    W = M @ T(-1.35, 0, 2.55)
    box(s, (-0.04, 0, -0.04), (0.04, 0.9, 0.04), IRON, W, skip=())
    box(s, (-0.01, 0.6, -0.12), (0.01, 0.64, 0.0), IRON, W, skip=("-z", "+z"))
    B = W @ T(0, 0.62, -0.5) @ Matrix.Rotation(math.pi / 2, 4, "Z")
    pts = [(math.cos(2 * math.pi * k / 8) * 0.38, math.sin(2 * math.pi * k / 8) * 0.38) for k in range(8)]
    prism(s, [(x, z) for x, z in pts], 0.06, LIGHTWOOD, B @ T(0, -0.03, 0), back=True, sides=False)
    egg = [(math.cos(2 * math.pi * k / 8) * 0.17 * (1 - 0.18 * math.sin(2 * math.pi * k / 8)),
            math.sin(2 * math.pi * k / 8) * 0.24) for k in range(8)]
    face(s, [(x, 0.035, z) for x, z in egg], (0, 1, 0), (0.98, 0.84, 0.50), B)
    face(s, [(x, -0.035, z) for x, z in egg], (0, -1, 0), (0.98, 0.84, 0.50), B)


# ------------------------------------------------------------------------------ shared: rock, lamps, stand-ins
ROCK_WARM = (0.76, 0.66, 0.56)
ROCK_COOL = (0.64, 0.68, 0.78)
ROCK_CAVE = (0.40, 0.46, 0.52)
BRASS = (0.88, 0.66, 0.32)


def smooth01(a, b, x):
    t = max(0.0, min(1.0, (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


def mixc(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))


def cave_facade(pl, M, open_w, open_h, spring, ring_w, rock, deep=(0.12, 0.09, 0.13), n=11, seed=0.0,
                depth=3.4, back=None, lump=0.32, smooth=True):
    """A chunky rock arch round a cave mouth, the rock behind it and a short dark tunnel into it,
    facing +Y (the opening spans x = +-open_w/2). back=(depth, scale) closes the rock behind into
    a free-standing mound. Returns the lip (the arch's inner edge) and the bulge row, in place
    coordinates, from the left foot over the top to the right foot."""
    hw, rz = open_w / 2, open_h - spring
    base = [(-hw, 0.0, -1.0, 0.0)]
    for i in range(n - 2):
        u = math.pi * i / (n - 3)
        nx, nz = -math.cos(u) / hw, math.sin(u) / rz
        ln = math.hypot(nx, nz)
        base.append((-hw * math.cos(u), spring + rz * math.sin(u), nx / ln, nz / ln))
    base.append((hw, 0.0, 1.0, 0.0))

    def loop(off, y, zdrop=0.0, amp=lump, zs=1.0):
        out = []
        for i, (x, z, nx, nz) in enumerate(base):
            p = Vector((x + nx * off, y, (z + nz * off) * zs))
            foot = i in (0, len(base) - 1)
            if foot:
                p.z = zdrop
            if amp:
                k = noise(Vector((p.x * 0.45 + seed, p.y * 0.45, p.z * 0.45)))
                p += Vector((nx, 0.0, 0.0 if foot else nz)) * amp * k
                p.y += amp * 0.6 * noise(Vector((p.x * 0.6, seed + 3.0, p.z * 0.6)))
            out.append(p)
        return out
    rows = [loop(-0.55, -depth, 0.0, lump * 0.5), loop(-0.3, -depth * 0.5, 0.0, lump * 0.5),
            loop(0.0, 0.35, 0.0, lump * 0.3), loop(ring_w * 0.45, 0.95, 0.0), loop(ring_w * 0.9, 0.45, -0.2),
            loop(ring_w * 1.25, -0.8, -0.35), loop(ring_w * 1.45, -2.2, -0.4)]
    if back:
        bd, bs = back
        rows += [loop(ring_w * 1.5, -bd * 0.55, -0.4, zs=0.9 * bs), loop(ring_w * 1.1, -bd, -0.4, zs=0.5 * bs)]
        last = rows[-1]
        cx = sum(p.x for p in last) / len(last)
        rows.append([Vector((cx + (p.x - cx) * 0.3, -bd - 1.4, -0.4)) for p in last])
    Minv = M.inverted()

    def col(p, nrm):
        ly = (Minv @ p).y
        t = smooth01(0.3, -depth * 0.7, ly)
        c = rock(p, nrm) if callable(rock) else rock
        return mixc(c, deep, t * 0.92)
    grid(pl.s, rows, col, M, smooth=smooth, flip=True, jit=0.08)
    face(pl.s, rows[0], (0, 1, 0), deep, M, jit=0.05)
    f0, f1, b0, b1 = rows[2][0], rows[2][-1], rows[0][0], rows[0][-1]
    face(pl.s, [(f1.x, f1.y, 0.03), (f0.x, f0.y, 0.03), (b0.x, b0.y, 0.03), (b1.x, b1.y, 0.03)], (0, 0, 1),
         mixc(EARTH, deep, 0.55), M)
    return [M @ p for p in rows[2]], [M @ p for p in rows[3]]


def lamp_post(pl, M, x, y, h=2.2, post=DARKWOOD, cage=IRON, rz=0.0, rng=5.0, strength=0.6):
    """A little lamp on a post, glowing at night."""
    s = pl.s
    W = M @ T(x, y, 0, rz=rz)
    box(s, (-0.08, -0.08, 0), (0.08, 0.08, h), post, W, skip=("-z",))
    box(s, (-0.2, -0.2, h), (0.2, 0.2, h + 0.4), cage, W, skip=("-z",))
    lathe(s, [(0.32, h + 0.4), (0.0, h + 0.68)], 4, cage, W, smooth=False, a0=math.pi / 4)
    for k in range(4):
        L = W @ T(0, 0, 0, rz=k * math.pi / 2)
        face(pl.g, [(-0.14, 0.205, h + 0.06), (0.14, 0.205, h + 0.06), (0.14, 0.205, h + 0.34),
                    (-0.14, 0.205, h + 0.34)], (0, 1, 0), Emit(WARM), L)
    pl.lamp(W, (0, 0, h + 0.2), rng, strength)
    pl.solid(W, 0, 0, 0.3)


def crystal(pl, M, x, y, z, length, r, tilt=0.0, yaw=0.0, col=(0.62, 0.96, 0.76), glow=(0.30, 1.0, 0.55),
            table=CRYSTAL):
    W = M @ T(x, y, z, rz=yaw) @ Matrix.Rotation(tilt, 4, "X")
    lathe(pl.s, [(r, -0.2), (r * 1.15, length * 0.72), (0, length)], 4, col, W, sharp=[1], smooth=False, jit=0.04)
    if glow:
        lathe(pl.g, [(r * 1.28, -0.2), (r * 1.4, length * 0.72), (0, length * 1.08)], 4, Emit(glow, table), W,
              sharp=[1], smooth=False, jit=0)


def icicle(pl, p, length, r=0.13):
    lathe(pl.s, [(0.0, -length), (r, 0.0)], 4, ICE, T(p.x, p.y, p.z), smooth=False, jit=0.03)


def heart_outline(size):
    pts = []
    for k in range(10):
        t = 2 * math.pi * k / 10
        x = 16 * math.sin(t) ** 3
        z = 13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t)
        pts.append((x * size / 32, z * size / 32))
    return pts


def star_outline(r_out, r_in, points=5, a0=math.pi / 2):
    return [((r_out if k % 2 == 0 else r_in) * math.cos(a0 + math.pi * k / points),
             (r_out if k % 2 == 0 else r_in) * math.sin(a0 + math.pi * k / points)) for k in range(points * 2)]


def reeds(pl, M, x, y, blades=5, h=1.4, seed=0):
    s = pl.s
    for k in range(blades):
        a = seed * 2.3 + k * 2.39
        r = 0.15 + 0.12 * (k % 3)
        bx, by = x + math.cos(a) * r, y + math.sin(a) * r
        lean = Vector((math.cos(a) * 0.25, math.sin(a) * 0.25, 0))
        tip = Vector((bx, by, h * (0.75 + 0.3 * ((k * 7) % 5) / 4))) + lean
        side = Vector((-math.sin(a), math.cos(a), 0)) * 0.07
        face(s, [Vector((bx, by, 0)) - side, Vector((bx, by, 0)) + side, tip],
             Vector((math.cos(a), math.sin(a), 0.2)), (0.46, 0.66, 0.30), M, double=True, jit=0.06)
    # a cattail
    top = Vector((x + 0.05, y - 0.05, h * 1.05))
    face(s, [Vector((x, y, 0)), Vector((x + 0.05, y, 0)), top + Vector((0.02, 0, 0)), top],
         (0, 1, 0), (0.44, 0.60, 0.28), M, double=True)
    cylinder(s, 0.07, 0.34, 4, (0.52, 0.34, 0.20), M @ T(top.x, top.y, top.z - 0.3), smooth=False)


def preview_mesh(name, part, mat, emissive=None):
    """Preview only: a stand-in lit simply (sky and sun, no shadows), or a flat emissive colour."""
    sets = []
    for s in SETS:
        L, sc, si = Vector(SUN[s][0]).normalized(), SUN[s][1], SUN[s][2]
        per = []
        for p, n, c in zip(part.P, part.N, part.C):
            if emissive:
                per.append(tuple(min(1.0, v * (0.35 + 0.65 * (s == "day")) + 0.1) for v in emissive))
                continue
            wrap = max(0.0, (n.dot(L) + 0.3) / 1.3)
            hemi = 0.5 + 0.5 * n.z
            per.append(tuple(min(1.0, c[k] * (SKY[s][k] * (0.72 + 0.28 * hemi) + sc[k] * si * wrap))
                             for k in range(3)))
        sets.append(per)
    return mesh_object(name, part.P, part.T, mat), sets


def sign_posts(pl, M, half_w, h, col=WOOD):
    """A sign's two posts, one beside each end of its board (half_w: the board's half width), so
    neither stands over its face (run 19: one post up the middle showed over the den's heart)."""
    for sx in (-1, 1):
        x = sx * (half_w + 0.07)
        box(pl.s, (x - 0.07, -0.07, -0.2), (x + 0.07, 0.07, h), col, M, skip=("-z",))
        puff(pl.s, (x, 0, h), 0.09, tuple(c * 0.85 for c in col), M, segs=4, h=0.1)


# ---------------------------------------------------------------------- Your den
def den_terrain(x, y):
    notch = 1.0 - smooth01(4.6, 6.0, abs(x))  # the landscape leaves the cave mouth open
    t = smooth01(-1.2 - 2.9 * notch, -3.6 - 2.9 * notch, y)
    z = 15.0 * t + 1.0 * noise(Vector((x * 0.12, y * 0.12, 0.5))) * t
    rockness = smooth01(0.03, 0.2, t) * (1 - smooth01(0.92, 1.0, t))
    return z, mixc(GRASS, ROCK_WARM, rockness)


@place("den", "Your Den")
def build_den(pl):
    M = I4
    s = pl.s
    lip, rim = cave_facade(pl, M, 7.2, 6.8, 3.0, 2.3, moss_top(ROCK_WARM, MOSS, 0.32, 0.68), n=13, seed=1.0,
                           depth=3.6)
    pl.mark("arch")
    for k, sd, r in ((3, 1, 0.8), (5, 2, 1.0), (8, 3, 0.9), (10, 4, 0.7)):  # moss cushions on top
        p = rim[k] + Vector((0, -0.25, 0.2))
        dome(s, r, r * 0.55, 6, MOSS, T(p.x, p.y, p.z), rings=2, lump=0.2, seed=sd)
    for k, fl in ((4, 0), (9, 2)):
        p = rim[k] + Vector((0, 0.1, 0.35))
        for j in range(3):
            puff(s, (p.x + (j - 1) * 0.35, p.y + 0.1 * j, p.z + 0.05 * j), 0.16, FLOWERS[(fl + j) % 5], I4, segs=4,
                 h=0.14)
    # vines hanging in front of the dark mouth
    for k, length in ((3, 1.6), (4, 2.6), (6, 1.9), (7, 2.9), (9, 2.2)):
        p = lip[k] + Vector((0, 0.18, 0.05))
        path = [p + Vector((0.05 * math.sin(i * 1.7 + k), 0.06 * i, -length * i / 3)) for i in range(4)]
        ribbon(s, path, 0.12, LEAF_DARK, I4, normal_hint=(0, 1, 0))
        for i in (1, 2, 3):
            q = path[i] + Vector((0.09 * (1 if (i + k) % 2 else -1), 0.03, 0.12))
            face(s, [q + Vector((-0.17, 0, 0)), q + Vector((0, 0, -0.13)), q + Vector((0.17, 0, 0)),
                     q + Vector((0, 0, 0.15))], (0, 1, 0), LEAF, double=True)
    pl.mark("vines")
    # the den's warm light deep inside, at night
    disc(pl.g, 2.6, 8, Emit((1.0, 0.60, 0.28), GLOW, fade=lambda p: 0.85 * max(0.0, 1 - (p - Vector(
        (0, -3.2, 2.2))).length / 2.6)), T(0, -3.25, 2.2) @ Matrix.Rotation(-math.pi / 2, 4, "X"), jit=0)
    pl.lamp(M, (0, -2.2, 1.8), 6.5, 0.75)
    # boulders at its feet, stepping stones out, a sign with a heart, lamps, flowers
    rock(pl, M, -6.3, 0.9, 1.4, ROCK_WARM, (1.2, 1.0, 0.85), seed=2, moss=MOSS)
    rock(pl, M, 6.5, 0.6, 1.6, ROCK_WARM, (1.1, 1.0, 0.8), seed=5, moss=MOSS)
    rock(pl, M, -5.0, 2.6, 0.6, ROCK_WARM, (1.1, 1.0, 0.8), seed=7, segs=6, rings=3, moss=MOSS)
    rock(pl, M, 5.3, 2.9, 0.5, ROCK_WARM, (1.1, 1.0, 0.8), seed=9, segs=6, rings=3, moss=MOSS)
    for x, y, sd in ((-6.3, 0.9, 1.4), (6.5, 0.6, 1.6), (-5.0, 2.6, 0.6), (5.3, 2.9, 0.5)):
        pl.solid(M, x, y, sd * 1.1)
    for i, (x, y, r) in enumerate(((0.3, 1.7, 0.62), (-0.5, 3.2, 0.66), (0.4, 4.7, 0.6), (-0.3, 6.2, 0.68),
                                   (0.5, 7.7, 0.6), (0.0, 9.2, 0.64))):
        lathe(s, [(r, -0.08), (r * 0.88, 0.13), (0, 0.17)], 6, (0.68, 0.64, 0.60), T(x, y, 0, rz=i), lump=0.08,
              seed=i)
    pl.mark("stones")
    W = T(-2.4, 4.3, 0, rz=0.35)
    sign_posts(pl, W, 0.55, 1.62)  # (beside the board, not over the heart: run 19)
    box(s, (-0.55, -0.05, 0.85), (0.55, 0.05, 1.45), LIGHTWOOD, W, skip=())
    hrt = heart_outline(0.46)
    face(s, [(x, 0.056, 1.15 + z) for x, z in hrt], (0, 1, 0), (0.92, 0.34, 0.42), W)
    face(s, [(x, -0.056, 1.15 + z) for x, z in hrt], (0, -1, 0), (0.92, 0.34, 0.42), W)
    pl.solid(W, 0, 0, 0.5)
    for sx in (-1, 1):
        lamp_post(pl, M, sx * 4.3, 2.1, h=1.5, post=(0.62, 0.58, 0.54), cage=DARKWOOD)
    for x, y, sd in ((-3.3, 1.5, 1), (3.1, 1.3, 2), (-1.7, 6.6, 3), (1.9, 8.4, 4), (-4.9, 4.6, 5), (2.2, 3.6, 0)):
        flower_clump(pl, M, x, y, 0.5, seed=sd)
    pl.mark("props")
    festival_lantern(pl, M, 2.7, 5.6, rz=math.pi)
    pl.door = [0.0, -1.0]
    pl.flat = 10.0
    pl.solid(M, -5.2, 0.0, 1.8)
    pl.solid(M, 5.2, 0.0, 1.8)
    # the cliff behind stops the light (the mouth left open)
    y = -1.9
    pl.ground += [((-60, y, 0), (-4.6, y, 0), (-4.6, y, 40), (-60, y, 40)),
                  ((4.6, y, 0), (60, y, 0), (60, y, 40), (4.6, y, 40)),
                  ((-4.6, y, 7.6), (4.6, y, 7.6), (4.6, y, 40), (-4.6, y, 40))]
    pl.terrain = den_terrain
    pl.terrain_size = 36
    pl.view = dict(radius=10.5, target=(0, 2.2, 2.6), azimuth=22.0, elevation=30.0)


# ---------------------------------------------------------------------- The Nesting Stone
def hill_terrain(x, y):
    r = math.hypot(x, y)
    return -0.05 * max(0.0, r - 11.0) ** 2, GRASS


@place("stone", "The Nesting Stone")
def build_stone(pl):
    M = I4
    s = pl.s
    lathe(s, [(3.0, -0.25), (3.15, 0.45), (2.8, 0.95), (2.1, 1.12), (0.0, 1.16)], 12, (0.82, 0.78, 0.72), M,
          lump=0.04, seed=2)
    torus_nest(pl, T(0, 0, 1.1), 1.75, 0.42, 12, 4, STRAW)
    disc(s, 1.8, 12, (0.84, 0.64, 0.36), T(0, 0, 1.18), jit=0.12)
    lathe(s, [(0.8, -0.1), (0.72, 0.5), (0.0, 0.55)], 6, (0.76, 0.72, 0.66), T(0.1, 3.5, 0), lump=0.06, seed=3)
    pl.solid(M, 0, 0, 3.3)
    pl.mark("stone")
    stones = 8
    for k in range(stones):
        a = 2 * math.pi * (k + 0.5) / stones
        x, y = math.sin(a) * 7.6, math.cos(a) * 7.6
        h = 2.8 + 0.7 * noise(Vector((k * 1.3, 0.5, 0.2)))
        if k in (3, 4):  # the pair at the back, lintelled
            h = 3.3
        W = T(x, y, 0, rz=-a + 0.12 * noise(Vector((k, 2, 0))), s=(1.0, 0.62, 1.0))
        lathe(s, [(0.78, -0.3), (0.82, h * 0.45), (0.64, h * 0.85), (0.3, h), (0.0, h * 1.02)], 5,
              moss_top(STONE_WARM, MOSS, 0.2, 0.6), W, lump=0.12, seed=k)
        pl.solid(M, x, y, 0.95)
    a3, a4 = 2 * math.pi * 3.5 / stones, 2 * math.pi * 4.5 / stones
    mid = Vector(((math.sin(a3) + math.sin(a4)) / 2 * 7.6, (math.cos(a3) + math.cos(a4)) / 2 * 7.6, 3.55))
    blob(s, 3.7, 0.75, 0.42, 7, 3, moss_top(STONE_WARM, MOSS, 0.3, 0.7), T(mid.x, mid.y, mid.z), lump=0.12, seed=4)
    pl.mark("ring")
    for k, (x, y) in enumerate(((-4.0, 3.2), (4.3, 2.6), (-5.0, -2.2), (5.2, -2.8), (-2.0, -5.4), (2.6, -5.2),
                                (-6.2, 5.8), (6.8, 4.9), (-8.8, -1.0), (8.9, 0.8), (-1.6, 8.8))):
        flower_clump(pl, M, x, y, 0.45, seed=k)
    for k, (x, y, r) in enumerate(((3.9, -0.4, 0.35), (-3.7, 0.8, 0.3), (-6.4, 2.4, 0.4), (6.0, -5.6, 0.45))):
        rock(pl, M, x, y, r, STONE_WARM, seed=k, segs=6, rings=3, moss=MOSS)
    pl.mark("flowers")
    festival_lantern(pl, M, 2.6, 5.3, rz=math.radians(200))
    pl.door = [0.0, 3.6]
    pl.flat = 11.0
    pl.terrain = hill_terrain
    pl.terrain_size = 30
    pl.view = dict(radius=11.0, target=(0, 0, 1.2), azimuth=20.0, elevation=38.0)


# ---------------------------------------------------------------------- Sanctuary Meadow
def round_hut(pl, M, r=2.5, h=2.4, wall=CREAM, door_col=DOOR_GREEN, roof_col=THATCH, seed=0):
    """A keeper's round hut: stone footing, round walls, a round-topped door, a conical thatch."""
    s = pl.s
    segs = 10
    a0 = math.pi / 2 - math.pi / segs  # a flat face looking +Y for the door
    ap = math.cos(math.pi / segs)
    lathe(s, [(r + 0.16, -0.2), (r, 0.35)], segs, STONE, M, a0=a0, smooth=False)
    lathe(s, [(r, 0.35), (r, h - 0.6), (r, h)], segs, wall_col(wall, 0.35, h), M, a0=a0, smooth=True)
    lathe(s, [(r + 0.8, h - 0.35), (r + 0.85, h - 0.1), (r * 0.74, h + 1.1), (r * 0.32, h + 2.0), (0, h + 2.4)],
          segs, lambda p, n: tuple(c * (0.9 + 0.12 * max(0.0, n.z)) for c in roof_col), M, a0=a0, lump=0.07,
          seed=seed, jit=0.08)
    face(s, [((r + 0.8) * math.cos(a0 + 2 * math.pi * k / segs), (r + 0.8) * math.sin(a0 + 2 * math.pi * k / segs),
              h - 0.35) for k in range(segs)], (0, 0, -1), tuple(c * 0.7 for c in roof_col), M)
    door(pl, M @ T(0, r * ap, 0), 0, 0.35, w=1.15, h=1.9, col=door_col)
    for k in (-2, 2):
        a = math.pi / 2 + 2 * math.pi * k / segs
        F = M @ T(math.cos(a) * r * ap, math.sin(a) * r * ap, 0, rz=a - math.pi / 2)
        window_round(pl, F, 0, 1.45, 0.34)
    back = math.pi / 2 + math.pi
    cylinder(s, 0.16, 1.4, 5, IRON, M @ T(math.cos(back) * r * 0.55, math.sin(back) * r * 0.55, h + 1.0),
             smooth=False)
    pl.solid(M, 0, 0, r + 0.7)


@place("sanctuary", "Sanctuary Meadow")
def build_sanctuary(pl):
    M = I4
    s = pl.s
    H1 = T(-8.4, 1.2, 0, rz=-0.35)
    round_hut(pl, H1, wall=CREAM, door_col=DOOR_GREEN, seed=1)
    pl.door = world(H1, (0, 3.2, 0))[:2]
    round_hut(pl, T(-4.8, -5.2, 0, rz=-0.9), r=2.2, wall=PEACH, door_col=DOOR_BLUE, seed=2)
    pl.mark("huts")
    fence_run(pl, M, [(4.2, 3.6), (-0.6, 3.6), (-0.6, -1.4), (-0.6, -6.6), (5.4, -6.6), (11.4, -6.6), (11.4, -1.4),
                      (11.4, 3.6), (7.2, 3.6)])
    for x in (4.2, 7.2):  # the gate's posts, taller, with little knobs
        box(s, (x - 0.12, 3.48, 0), (x + 0.12, 3.72, 1.4), WOOD, M, skip=("-z",))
        puff(s, (x, 3.6, 1.4), 0.14, GOLD, M, segs=4, h=0.16)
    for a, b in (((-0.6, 3.6), (4.2, 3.6)), ((7.2, 3.6), (11.4, 3.6)), ((-0.6, -6.6), (11.4, -6.6)),
                 ((-0.6, -6.6), (-0.6, 3.6)), ((11.4, -6.6), (11.4, 3.6))):
        n = max(1, int(round(math.hypot(b[0] - a[0], b[1] - a[1]) / 2.5)))
        for k in range(n + 1):
            pl.solid(M, a[0] + (b[0] - a[0]) * k / n, a[1] + (b[1] - a[1]) * k / n, 0.5)
    pl.mark("fence")
    # hay: two round bales and a square one; a water trough
    for x, y, rz in ((9.4, 1.8, 0.3), (8.2, 2.4, -0.4)):
        W = T(x, y, 0.62, rz=rz) @ Matrix.Rotation(math.pi / 2, 4, "Y")
        cylinder(s, 0.62, 1.1, 8, STRAW, W @ T(0, 0, -0.55), bottom=True, jit=0.1)
        pl.solid(M, x, y, 0.8)
    box(s, (9.2, -5.6, 0), (10.6, -4.8, 0.62), STRAW, M, skip=("-z",), jit=0.1)
    box(s, (9.35, -5.5, 0.62), (10.45, -4.9, 1.2), (0.86, 0.70, 0.40), M, skip=("-z",), jit=0.1)
    pl.solid(M, 9.9, -5.2, 0.9)
    box(s, (1.6, -5.2, 0), (4.0, -4.4, 0.7), WOOD, M, skip=("-z", "+z"))
    o = [(1.6, -5.2), (4.0, -5.2), (4.0, -4.4), (1.6, -4.4)]
    i_ = [(1.74, -5.06), (3.86, -5.06), (3.86, -4.54), (1.74, -4.54)]
    for k in range(4):
        a, b, c, d_ = o[k], o[(k + 1) % 4], i_[(k + 1) % 4], i_[k]
        face(s, [(a[0], a[1], 0.7), (b[0], b[1], 0.7), (c[0], c[1], 0.7), (d_[0], d_[1], 0.7)], (0, 0, 1), LIGHTWOOD,
             M)
    face(s, [(x, y, 0.64) for x, y in i_], (0, 0, 1), WATER, M)
    pl.solid(M, 2.8, -4.8, 1.2)
    pl.mark("hay")
    # the lookout: four legs, a deck with a rail, a ladder and a little roof
    L = T(-1.8, -9.0, 0, rz=0.25)
    for x, y in ((-1.0, -1.0), (1.0, -1.0), (1.0, 1.0), (-1.0, 1.0)):
        box(s, (x - 0.1, y - 0.1, 0), (x + 0.1, y + 0.1, 3.0), WOOD, L, skip=("-z", "+z"))
    box(s, (-1.3, -1.3, 2.8), (1.3, 1.3, 3.05), LIGHTWOOD, L, skip=())
    for k in range(4):
        R = L @ T(0, 0, 0, rz=k * math.pi / 2)
        if k == 0:
            continue  # the ladder side stays open
        quad(s, (-1.25, 1.25, 3.3), (1.25, 1.25, 3.3), (1.25, 1.25, 3.5), (-1.25, 1.25, 3.5), WOOD, R, double=True)
        box(s, (1.16, 1.16, 3.05), (1.28, 1.28, 3.55), WOOD, R, skip=("-z",))
    for sx in (-0.35, 0.35):
        box(s, (sx - 0.05, 1.5, 0), (sx + 0.05, 1.6, 3.1), DARKWOOD, L @ T(0, 0, 0, rx=0.0), skip=("-z",))
    for z in (0.6, 1.2, 1.8, 2.4):
        quad(s, (-0.35, 1.55, z), (0.35, 1.55, z), (0.35, 1.55, z + 0.08), (-0.35, 1.55, z + 0.08), DARKWOOD, L,
             double=True)
    for x, y in ((-1.1, -1.1), (1.1, 1.1)):
        box(s, (x - 0.06, y - 0.06, 3.05), (x + 0.06, y + 0.06, 4.5), DARKWOOD, L, skip=("-z", "+z"))
    lathe(s, [(1.9, 4.4), (0.0, 5.2)], 4, (0.62, 0.40, 0.28), L, smooth=False, a0=math.pi / 4)
    face(s, [(1.9 * math.cos(math.pi / 4 + k * math.pi / 2), 1.9 * math.sin(math.pi / 4 + k * math.pi / 2), 4.4)
             for k in range(4)], (0, 0, -1), (0.46, 0.30, 0.20), L)
    pl.solid(L, 0, 0, 1.7)
    pl.mark("lookout")
    for x, y, sd in ((-6.2, 3.4, 1), (-10.8, -1.2, 2), (-2.4, -3.0, 3), (-3.0, 5.6, 4)):
        flower_clump(pl, M, x, y, 0.5, seed=sd)
    for x, y, sd in ((13.2, 1.0, 1), (-11.6, 3.6, 2)):
        bush(pl, M, x, y, 1.1, seed=sd)
        pl.solid(M, x, y, 1.2)
    # Stepping stones from the hut's door and the paddock's gate to the landscape's path (which
    # crosses the front: no modelled road doubling it, run 19).
    for i, (x, y, r) in enumerate(((-7.0, 4.6, 0.5), (-5.9, 5.3, 0.46), (-4.7, 5.8, 0.5), (5.6, 4.6, 0.5),
                                   (4.3, 5.4, 0.46), (3.0, 6.0, 0.5))):
        lathe(s, [(r, -0.08), (r * 0.88, 0.12), (0, 0.15)], 5, (0.78, 0.74, 0.68), T(x, y, pl.gz(x, y), rz=i),
              lump=0.08, seed=i)
    pl.mark("props")
    festival_lantern(pl, M, -2.6, 3.4, rz=math.radians(-20))
    pl.flat = 16.0
    pl.terrain_size = 40
    pl.view = dict(radius=14.0, target=(1.0, -1.5, 1.2), azimuth=20.0, elevation=40.0)


# ---------------------------------------------------------------------- The Cold Vault
def snow_terrain(x, y):
    t = smooth01(-5.0, -14.0, y)
    return 10.0 * t + 0.6 * noise(Vector((x * 0.1, y * 0.1, 0.3))), mixc((0.90, 0.93, 0.98), (0.82, 0.86, 0.94), t)


@place("vault", "The Cold Vault")
def build_vault(pl):
    M = I4
    s = pl.s
    lip, rim = cave_facade(pl, M, 5.4, 5.0, 2.3, 2.6, moss_top((0.50, 0.54, 0.64), SNOW, 0.45, 0.72),
                           deep=(0.06, 0.08, 0.14), n=13, seed=4.0, depth=3.8, back=(7.5, 1.0), lump=0.6,
                           smooth=False)
    pl.mark("rock")
    # the heavy door, ajar: planks, iron bands, a ring
    D = T(-2.25, -0.7, 0, rz=-1.05)
    outline = [(x + 2.25, z) for x, z in arch_outline(4.5, 4.3, 4)]
    prism(s, outline, 0.22, (0.50, 0.33, 0.21), D @ T(0, -0.11, 0), back=True)
    for px in (1.1, 2.25, 3.4):
        face(s, [(px - 0.03, 0.115, 0.1), (px + 0.03, 0.115, 0.1), (px + 0.03, 0.115, 3.3), (px - 0.03, 0.115, 3.3)],
             (0, 1, 0), (0.34, 0.22, 0.14), D, jit=0)
    for z in (0.9, 2.8):
        face(s, [(0.05, 0.12, z), (4.45, 0.12, z), (4.45, 0.12, z + 0.2), (0.05, 0.12, z + 0.2)], (0, 1, 0), IRON, D)
    torus_nest(pl, D @ T(3.6, 0.2, 1.7) @ Matrix.Rotation(math.pi / 2, 4, "X"), 0.16, 0.04, 6, 3, IRON)
    disc(pl.g, 2.2, 8, Emit((0.45, 0.72, 1.0), GLOW, fade=lambda p: 0.7 * max(0.0, 1 - (p - Vector(
        (0, -3.4, 1.7))).length / 2.2)), T(0, -3.45, 1.7) @ Matrix.Rotation(-math.pi / 2, 4, "X"), jit=0)
    pl.lamp(M, (0, -2.4, 1.6), 5.0, 0.5, rgb=(0.5, 0.75, 1.0))
    pl.mark("door")
    for k, length in ((2, 0.6), (3, 1.1), (4, 0.7), (5, 1.3), (6, 0.9), (7, 1.2), (8, 0.6), (9, 1.0), (10, 0.7)):
        icicle(pl, lip[k] + Vector((0, 0.3, -0.05)), length)
    for x, y, z, sc, yaw in ((-3.9, 0.9, 0.0, 1.4, 0.3), (4.0, 0.8, 0.0, 1.5, -0.4), (-5.4, -0.8, 2.4, 1.1, 0.9),
                             (5.2, -1.2, 3.0, 1.0, -1.1)):
        for j in range(3):
            crystal_ice = (0.56, 0.82, 1.0)
            W = T(x + 0.3 * (j - 1), y + 0.15 * j, z, rz=yaw + j * 2.1) @ Matrix.Rotation(0.35 + 0.15 * j, 4, "X")
            lathe(s, [(0.2 * sc, -0.2), (0.24 * sc, 0.8 * sc), (0, 1.25 * sc * (1.2 - 0.2 * j))], 4, crystal_ice, W,
                  sharp=[1], smooth=False, jit=0.04)
        pl.solid(M, x, y, 0.8)
    pl.mark("ice")
    for x, y, r, h in ((-3.2, 2.6, 1.3, 0.55), (3.4, 3.4, 1.6, 0.6), (-6.4, 3.8, 1.8, 0.7), (6.8, 1.2, 1.5, 0.8),
                       (0.8, 6.0, 1.2, 0.4), (-1.6, 7.2, 1.0, 0.35)):
        dome(s, r, h * 1.5, 8, SNOW, T(x, y, -0.15), rings=2, lump=0.22, seed=x)
    for k, (x, y, r) in enumerate(((-6.9, 0.2, 1.1), (7.3, -1.8, 1.3), (5.8, 5.0, 0.6))):
        rock(pl, M, x, y, r, ROCK_COOL, (1.1, 1.0, 0.8), seed=k + 3, segs=6, rings=3, moss=SNOW)
        pl.solid(M, x, y, r * 1.1)
    pl.mark("snow")
    Sg = T(-3.4, 3.6, 0, rz=0.3)
    sign_posts(pl, Sg, 0.45, 1.55)
    box(s, (-0.45, -0.05, 0.85), (0.45, 0.05, 1.45), LIGHTWOOD, Sg, skip=())
    dome(s, 0.5, 0.14, 5, SNOW, Sg @ T(0, 0, 1.45) @ T(0, 0, 0, s=(1.0, 0.2, 1.0)), rings=1)
    for sy in (1, -1):
        face(s, [(x, sy * 0.056, 1.15 + z) for x, z in star_outline(0.22, 0.08, 6)], (0, sy, 0), (0.40, 0.66, 0.96),
             Sg)
    pl.solid(Sg, 0, 0, 0.5)
    festival_lantern(pl, M, 3.3, 3.0, rz=math.radians(150))
    pl.door = [0.0, -0.4]
    pl.flat = 12.0
    pl.solid(M, -4.6, -1.0, 2.2)
    pl.solid(M, 4.6, -1.0, 2.2)
    pl.solid(M, 0.0, -5.5, 5.0)
    pl.terrain = snow_terrain
    pl.terrain_size = 34
    pl.view = dict(radius=11.0, target=(0, 0, 2.4), azimuth=24.0, elevation=30.0)


# ---------------------------------------------------------------------- Wanderers' Trailhead
@place("trailhead", "Wanderers' Trailhead")
def build_trailhead(pl):
    M = I4
    s = pl.s
    # (No modelled road through the gate: the landscape's path arrives here, run 19.)
    for sx in (-1, 1):  # the gate: log posts, a beam, braces
        lathe(s, [(0.36, -0.2), (0.3, 5.3), (0.0, 5.42)], 7, WOOD, T(sx * 3.3, 0, 0), sharp=[1], lump=0.05)
        sweep(s, [(sx * 3.25, 0, 3.6), (sx * 2.35, 0, 4.72)], 0.1, 4, DARKWOOD, M, smooth=False)
        box(s, (sx * 3.3 - 0.03, -0.03, 5.4), (sx * 3.3 + 0.03, 0.03, 6.4), DARKWOOD, M, skip=("-z",))
        face(s, [(sx * 3.3, 0, 6.35), (sx * 3.3, 0, 5.9), (sx * 3.3 + sx * 0.9, 0, 6.12)], (0, 1, 0),
             CLOTH_RED if sx < 0 else CLOTH_TEAL, M, double=True)
        pl.solid(M, sx * 3.3, 0, 0.5)
    sweep(s, [(-3.9, 0, 4.72), (0, 0, 5.0), (3.9, 0, 4.72)], 0.27, 6, WOOD, M, caps=True)
    for sx in (-0.9, 0.9):
        box(s, (sx - 0.02, -0.02, 4.1), (sx + 0.02, 0.02, 4.75), IRON, M, skip=("-z", "+z"))
    box(s, (-1.7, -0.08, 3.3), (1.7, 0.08, 4.2), LIGHTWOOD, M, skip=())
    st = star_outline(0.36, 0.14, 4, 0.0)
    face(s, [(x, 0.085, 3.75 + z) for x, z in st], (0, 1, 0), GOLD, M)
    face(s, [(x, -0.085, 3.75 + z) for x, z in st], (0, -1, 0), GOLD, M)
    for sx in (-1, 1):
        for sy in (1, -1):
            face(s, [(sx * 1.1 - 0.26, sy * 0.085, 3.55), (sx * 1.1 + 0.26, sy * 0.085, 3.55),
                     (sx * 1.1, sy * 0.085, 3.95)], (0, sy, 0), (0.40, 0.62, 0.32), M)
    pl.mark("gate")
    # the signpost
    P = T(3.8, 3.8, 0)
    box(s, (-0.09, -0.09, 0), (0.09, 0.09, 2.5), WOOD, P, skip=("-z",))
    puff(s, (0, 0, 2.5), 0.14, CLOTH_RED, P, segs=4, h=0.18)
    # (The arms fixed to the post's side, their faces clear of it: run 19.)
    arrow = [(0.13, -0.13), (0.8, -0.13), (0.8, -0.24), (1.1, 0.0), (0.8, 0.24), (0.8, 0.13), (0.13, 0.13)]
    for z, rz, c in ((2.2, math.radians(95), CLOTH_RED), (1.82, math.radians(-150), CLOTH_TEAL),
                     (1.44, math.radians(10), CLOTH_YELLOW)):
        prism(s, [(x, zz) for x, zz in arrow], 0.08, c, P @ T(0, 0, z, rz=rz) @ T(0, -0.04, 0), back=True)
    pl.solid(P, 0, 0, 0.4)
    bench(pl, M, 3.9, -4.6, math.pi / 2)
    pl.mark("sign")
    # the traveller's tent, its flaps tied back, a pennant on its pole; a pack; a campfire
    Tn = T(-6.0, 2.0, 0, rz=-math.pi / 2 + 0.45)
    w, d, h = 1.8, 2.0, 2.5
    for sx in (-1, 1):
        for i in range(3):
            y0, y1 = -d + 2 * d * i / 3, -d + 2 * d * (i + 1) / 3
            c = CLOTH_CREAM if i % 2 == 0 else (0.80, 0.52, 0.40)
            pts = [(sx * w, y0, 0), (sx * w, y1, 0), (0, y1, h), (0, y0, h)]
            face(s, pts, (sx * h, 0, w), c, Tn)
            face(s, pts, (-sx * h, 0, -w), tuple(v * 0.7 for v in c), Tn)
    face(s, [(-w, -d, 0), (w, -d, 0), (0, -d, h)], (0, -1, 0), CLOTH_CREAM, Tn, double=True)
    for sx in (-1, 1):
        face(s, [(0, d, h), (sx * w, d, 0), (sx * w * 1.25, d + 0.6, 0.5)], (0, 1, 0), (0.80, 0.52, 0.40), Tn,
             double=True)
    face(s, [(-w * 0.9, -d, 0.03), (w * 0.9, -d, 0.03), (w * 0.9, d, 0.03), (-w * 0.9, d, 0.03)], (0, 0, 1),
         (0.42, 0.36, 0.44), Tn)
    box(s, (-0.05, d - 0.05, 0), (0.05, d + 0.05, 3.1), DARKWOOD, Tn, skip=("-z",))
    face(s, [(0, d, 3.05), (0, d, 2.7), (0, d + 0.9, 2.88)], (1, 0, 0), CLOTH_YELLOW, Tn, double=True)
    for sx in (-1, 1):
        ribbon(s, [(0, d, 2.9), (sx * 1.4, d + 1.4, 0.0)], 0.03, (0.80, 0.74, 0.60), Tn, normal_hint=(0, 0, 1))
    pl.solid(Tn, 0, 0, 2.0)
    pl.solid(Tn, 0, 1.6, 1.4)
    Pk = T(-4.0, -0.2, 0, rz=0.4)
    box(s, (-0.4, -0.28, 0), (0.4, 0.28, 1.0), (0.50, 0.40, 0.66), Pk, skip=("-z",))
    dome(s, 0.42, 0.3, 6, (0.50, 0.40, 0.66), Pk @ T(0, 0, 1.0) @ T(0, 0, 0, s=(1, 0.7, 1)), rings=1)
    W = Pk @ T(0, 0, 1.38) @ Matrix.Rotation(math.pi / 2, 4, "Y")
    cylinder(s, 0.2, 1.0, 6, (0.80, 0.34, 0.30), W @ T(0, 0, -0.5), bottom=True, smooth=True)
    box(s, (-0.3, 0.28, 0.2), (0.3, 0.4, 0.6), (0.44, 0.34, 0.58), Pk, skip=("-z",))
    pl.solid(Pk, 0, 0, 0.6)
    F = T(-3.0, 3.9, 0)
    for k in range(6):
        a = 2 * math.pi * k / 6
        puff(s, (math.cos(a) * 0.62, math.sin(a) * 0.62, 0), 0.22, (0.60, 0.58, 0.56), F, segs=5, h=0.2)
    for k in range(3):
        box(s, (-0.5, -0.07, 0.02), (0.5, 0.07, 0.16), (0.40, 0.26, 0.16), F @ T(0, 0, 0, rz=k * math.pi / 3),
            skip=("-z",))
    face(s, [(math.cos(2 * math.pi * k / 6) * 0.42, math.sin(2 * math.pi * k / 6) * 0.42, 0.04) for k in range(6)],
         (0, 0, 1), (0.22, 0.16, 0.14), F)
    fire = Emit((1.0, 0.62, 0.22), GLOW, fade=lambda p, c=F @ Vector((0, 0, 0.1)): max(0.0, 1 - (p.z - c.z) / 0.9))
    for a in (0.0, math.pi / 2):
        face(pl.g, [(-0.3, 0, 0.1), (0.3, 0, 0.1), (0, 0, 1.0)], (0, 1, 0), fire, F @ T(0, 0, 0, rz=a))
    disc(pl.g, 0.5, 6, Emit((1.0, 0.45, 0.15), GLOW, fade=lambda p: 0.8), F @ T(0, 0, 0.18), jit=0)
    pl.lamp(F, (0, 0, 0.6), 5.5, 0.9)
    pl.solid(F, 0, 0, 0.9)
    pl.mark("camp")
    N = T(3.8, -1.8, 0, rz=math.radians(-100))
    for sx in (-1.02, 1.02):  # (at its ends, clear of the notices)
        box(s, (sx - 0.07, -0.07, -0.2), (sx + 0.07, 0.07, 1.9), WOOD, N, skip=("-z",))
    box(s, (-0.95, -0.05, 0.9), (0.95, 0.05, 1.75), LIGHTWOOD, N, skip=())
    box(s, (-1.1, -0.25, 1.9), (1.1, 0.25, 2.0), (0.62, 0.40, 0.28), N, skip=())
    for x, z, c in ((-0.5, 1.4, CLOTH_CREAM), (0.1, 1.2, (0.96, 0.90, 0.70)), (0.55, 1.45, CLOTH_PINK)):
        face(s, [(x - 0.2, 0.06, z - 0.22), (x + 0.2, 0.06, z - 0.22), (x + 0.2, 0.06, z + 0.2),
                 (x - 0.2, 0.06, z + 0.2)], (0, 1, 0), c, N)
    pl.solid(N, 0, 0, 1.0)
    crate(pl, M, -6.6, -0.8, 0.7, 0.3)
    barrel(pl, M, -7.6, -0.4, 0.34, 0.8)
    for x, y, sd in ((-2.0, 6.8, 1), (2.4, -6.6, 2), (5.8, 1.8, 3), (-2.2, -3.4, 4)):
        flower_clump(pl, M, x, y, 0.5, seed=sd)
    bush(pl, M, -6.2, 6.6, 1.0, seed=1)
    bush(pl, M, 6.6, -6.2, 1.1, seed=2)
    pl.solid(M, -6.2, 6.6, 1.1)
    pl.solid(M, 6.6, -6.2, 1.2)
    pl.mark("props")
    festival_lantern(pl, M, 4.5, 1.0, rz=math.pi)
    pl.door = [0.0, 0.0]
    pl.flat = 12.0
    pl.terrain_size = 34
    pl.view = dict(radius=12.0, target=(-1.0, -0.8, 1.8), azimuth=28.0, elevation=34.0)


# ---------------------------------------------------------------------- The Arena
FESTIVAL = (CLOTH_RED, CLOTH_YELLOW, CLOTH_TEAL, CLOTH_BLUE, CLOTH_PINK, CLOTH_LILAC, (0.56, 0.80, 0.40),
            (0.98, 0.62, 0.30))


@place("arena", "The Arena")
def build_arena(pl):
    M = I4
    s = pl.s
    R0 = 15.0
    # The sand floor follows the landscape's ground (a few hands off level across it): flat at
    # the anchor's height, the ground covered its front (run 19, "partly under the ground").
    ground_patch(pl, M, R0, 20, SAND, rings=[R0, 11.6, 8.0, 4.4], z=0.08, hug=True)
    hug_ring(pl, 8.6, 9.4, 20, 0.11, CLOTH_CREAM)
    z0 = pl.gz(0, 0)
    face(s, [(x * 2.6, y * 2.6, z0 + 0.12) for x, y in star_outline(1.0, 0.45, 5)], (0, 0, 1), (0.92, 0.46, 0.34), M)
    face(s, [(math.cos(2 * math.pi * k / 10) * 0.9, math.sin(2 * math.pi * k / 10) * 0.9, z0 + 0.13)
          for k in range(10)], (0, 0, 1), GOLD, M)
    pl.mark("floor")
    deg = math.radians
    wall = [(15.7, -0.55), (15.7, 0.95), (15.2, 0.95), (15.2, -0.55)]  # (their feet below the ground's dips)
    for a0, a1 in ((deg(103), deg(248)), (deg(292), deg(437))):
        lathe(s, wall, 12, STONE_WARM, M, sharp=[1, 2], smooth=True, arc=(a0, a1), caps=True, lump=0.02)
    stands = [(18.2, -0.55), (18.2, 1.75), (17.0, 1.75), (17.0, 1.02), (15.72, 1.02)]

    def seat(p, n):
        return LIGHTWOOD if n.z > 0.5 else (WOOD if n.x * p.x + n.y * p.y < 0 else DARKWOOD)
    for a0, a1 in ((deg(112), deg(238)), (deg(302), deg(428))):
        lathe(s, stands, 11, seat, M, sharp=[1, 2, 3], smooth=True, arc=(a0, a1), caps=True)
    for a0, a1 in ((deg(103), deg(248)), (deg(292), deg(437))):
        n = 18
        for k in range(n + 1):
            a = a0 + (a1 - a0) * k / n
            pl.solid(M, math.cos(a) * 16.6, math.sin(a) * 16.6, 1.6)
    pl.mark("walls")
    for k, a in enumerate((deg(125), deg(150), deg(175), deg(200), deg(225), deg(-45), deg(-20), deg(5), deg(30),
                           deg(55))):
        x, y = math.cos(a) * 18.5, math.sin(a) * 18.5
        cylinder(s, 0.09, 6.0, 5, WOOD, T(x, y, -0.4), smooth=False)
        puff(s, (x, y, 5.6), 0.16, GOLD, M, segs=4, h=0.24)
        out = Vector((math.cos(a), math.sin(a), 0))
        tip = Vector((x, y, 0)) + out * 1.5
        face(s, [(x, y, 5.45), (x, y, 4.55), (tip.x, tip.y, 4.75), (tip.x - out.x * 0.35, tip.y - out.y * 0.35, 5.0),
                 (tip.x, tip.y, 5.25)], Vector((-out.y, out.x, 0)), FESTIVAL[k % len(FESTIVAL)], M, double=True)
    pl.mark("flags")
    # the entrance arch (dragon-sized) with an emblem and banners
    E = T(0, 15.45, 0)
    for sx in (-1, 1):
        box(s, (sx * 4.0 - 0.65, -0.65, -0.4), (sx * 4.0 + 0.65, 0.65, 6.0), STONE_WARM, E, skip=("-z",))
        box(s, (sx * 4.0 - 0.8, -0.8, 6.0), (sx * 4.0 + 0.8, 0.8, 6.45), STONE, E, skip=("-z",))
        puff(s, (sx * 4.0, 0, 6.45), 0.3, GOLD, E, segs=4, h=0.45)
        pl.solid(E, sx * 4.0, 0, 1.1)
    arc_pts = [(-4.0 + 8.0 * k / 6, 0, 6.2 + 1.9 * math.sin(math.pi * k / 6)) for k in range(7)]
    sweep(s, arc_pts, 0.42, 5, WOOD, E, caps=True)
    sun = [(math.cos(2 * math.pi * k / 8) * 0.9, math.sin(2 * math.pi * k / 8) * 0.9) for k in range(8)]
    prism(s, [(x, 8.1 + z) for x, z in sun], 0.16, GOLD, E @ T(0, 0.34, 0), back=True, sides=False)
    face(s, [(x * 0.8, 0.51, 8.1 + z * 0.8) for x, z in star_outline(0.62, 0.3, 5)], (0, 1, 0), CLOTH_RED, E)
    for sx, c in ((-2.2, CLOTH_RED), (2.2, CLOTH_TEAL)):
        face(s, [(sx - 0.6, 0.05, 7.1), (sx + 0.6, 0.05, 7.1), (sx + 0.6, 0.05, 5.0), (sx, 0.05, 4.5),
                 (sx - 0.6, 0.05, 5.0)], (0, 1, 0), c, E, double=True)
    for sx in (-1, 1):
        lamp_post(pl, E, sx * 5.6, 1.4, h=2.3)
    pl.mark("arch")
    # the festival stage at the back, with the great lantern
    S = T(0, -17.0, 0)
    box(s, (-4.0, -2.2, -0.35), (4.0, 2.2, 1.0), WOOD, S, skip=("-z",))
    face(s, [(-4.0, 2.21, 0.8), (4.0, 2.21, 0.8), (4.0, 2.21, 0.95), (-4.0, 2.21, 0.95)], (0, 1, 0), CLOTH_RED, S)
    box(s, (-1.3, 2.2, 0), (1.3, 2.8, 0.34), LIGHTWOOD, S, skip=("-z",))
    box(s, (-1.3, 2.2, 0.34), (1.3, 2.5, 0.67), LIGHTWOOD, S, skip=("-z",))
    for sx in (-1, 1):
        box(s, (sx * 3.7 - 0.12, -2.1, 1.0), (sx * 3.7 + 0.12, -1.86, 5.2), DARKWOOD, S, skip=("-z", "+z"))
    box(s, (-4.0, -2.12, 5.2), (4.0, -1.84, 5.45), DARKWOOD, S, skip=())
    face(s, [(-3.58, -1.95, 1.0), (3.58, -1.95, 1.0), (3.58, -1.95, 5.2), (-3.58, -1.95, 5.2)], (0, 1, 0),
         (0.34, 0.40, 0.70), S, double=True)
    face(s, [(x * 1.6, -1.93, 3.6 + z * 1.6) for x, z in star_outline(0.62, 0.3, 5)], (0, 1, 0), GOLD, S)
    bunting(pl, S, (-3.7, -1.9, 5.0), (3.7, -1.9, 5.0), sag=0.5, flags=6)
    pl.solid(S, -2.4, 0, 2.3)
    pl.solid(S, 2.4, 0, 2.3)
    pl.mark("stage")
    festival_lantern(pl, S @ T(0, -0.7, 1.0), 0, 0, rz=math.pi / 2, scale=2.0)
    pl.door = [0.0, 15.4]
    pl.flat = 21.0
    pl.terrain = "land"
    pl.terrain_size = 50
    pl.view = dict(radius=21.0, target=(0, -1.0, 1.5), azimuth=18.0, elevation=42.0)


# ---------------------------------------------------------------------- Mirror Lake
LAKE_WATER = -0.6


def lake_terrain(x, y):
    z = -2.6 * smooth01(0.6, -3.5, y) + 0.3 * noise(Vector((x * 0.2, y * 0.2, 0)))
    c = mixc(GRASS, SAND, smooth01(2.5, 0.5, y))
    c = mixc(c, (0.46, 0.56, 0.50), smooth01(-0.8, -2.2, y))
    return z, c


@place("lake", "Mirror Lake")
def build_lake(pl):
    M = I4
    s = pl.s
    top = 0.3
    box(s, (-1.1, -8.6, top - 0.18), (1.1, 0.9, top), WOOD, M, skip=("+z",))
    for i in range(10):
        y0, y1 = 0.9 - 0.95 * i, 0.9 - 0.95 * (i + 1) + 0.03
        face(s, [(-1.1, y1, top), (1.1, y1, top), (1.1, y0, top), (-1.1, y0, top)], (0, 0, 1),
             LIGHTWOOD if i % 2 else tuple(c * 0.92 for c in LIGHTWOOD), M)
    for y in (-3.0, -6.0, -8.4):
        for x in (-1.0, 1.0):
            cylinder(s, 0.13, 2.1, 5, DARKWOOD, T(x, y, -1.7), smooth=False)
    cylinder(s, 0.15, 2.5, 5, DARKWOOD, T(1.0, -8.4, -1.7), smooth=False)
    lathe(s, [(0.19, 0.55), (0.19, 0.7)], 5, (0.80, 0.72, 0.56), T(1.0, -8.4, 0), smooth=False)
    lamp_post(pl, M, -0.95, -8.4, h=1.6, rng=4.5)
    pl.mark("jetty")
    B = T(2.5, -6.2, LAKE_WATER - 0.08, rz=0.12)
    st = [(-1.9, 0.06, 0.62), (-1.2, 0.5, 0.5), (0.0, 0.74, 0.45), (1.2, 0.58, 0.5), (1.9, 0.14, 0.64)]
    outer, inner = [], []
    for y, hw, zt in st:
        prof = [(-hw, zt), (-hw * 0.8, 0.16), (0, -0.04), (hw * 0.8, 0.16), (hw, zt)]
        outer.append([Vector((x, y, z)) for x, z in prof])
        inner.append([Vector((x * 0.86, y * 0.95, z + 0.07)) for x, z in prof])
    grid(s, outer, (0.38, 0.58, 0.80), B, smooth=True, flip=True)
    grid(s, inner, (0.56, 0.40, 0.26), B, smooth=True)
    for side in (0, 4):
        for i in range(len(st) - 1):
            face(s, [outer[i][side], outer[i + 1][side], inner[i + 1][side], inner[i][side]], (0, 0, 1),
                 (0.94, 0.90, 0.80), B)
    for y in (-0.6, 0.7):
        box(s, (-0.5, y - 0.14, 0.3), (0.5, y + 0.14, 0.38), LIGHTWOOD, B, skip=("-z",))
    for sx in (-1, 1):
        O = B @ T(sx * 0.2, 0.2, 0.42, rz=sx * 0.18)
        box(s, (-0.03, -1.3, -0.03), (0.03, 0.9, 0.03), WOOD, O, skip=())
        face(s, [(-0.12, -1.3, 0), (0.12, -1.3, 0), (0.1, -1.9, 0), (-0.1, -1.9, 0)], (0, 0, 1), WOOD, O, double=True)
    ribbon(s, [B @ Vector((0, 1.85, 0.62)), Vector((1.4, -8.2, 0.1)), Vector((1.1, -8.4, 0.62))], 0.03,
           (0.86, 0.80, 0.62), I4, normal_hint=(0, 0, 1))
    pl.solid(B, 0, 0, 1.3)
    pl.mark("boat")
    for k, (x, y) in enumerate(((-3.2, -0.5), (-5.0, 0.1), (-6.9, -0.7), (3.6, -0.6), (5.4, 0.0), (7.2, -0.9))):
        reeds(pl, T(0, 0, LAKE_WATER + 0.1 if y < -0.3 else -0.1), x, y, 5, 1.4, seed=k)
    pl.mark("reeds")
    for k, (x, y, r) in enumerate(((-2.6, -3.4, 0.5), (-4.2, -4.6, 0.62), (-3.4, -6.4, 0.45), (-5.6, -2.6, 0.4),
                                   (4.4, -3.2, 0.55), (5.8, -4.8, 0.5), (4.0, -9.4, 0.6), (-1.8, -10.2, 0.5))):
        a0 = k * 1.3
        pts = [(x, y, LAKE_WATER + 0.02)] + [(x + math.cos(a0 + 0.35 + (2 * math.pi - 0.7) * j / 5) * r,
                                               y + math.sin(a0 + 0.35 + (2 * math.pi - 0.7) * j / 5) * r,
                                               LAKE_WATER + 0.02) for j in range(6)]
        face(s, pts, (0, 0, 1), (0.40, 0.66, 0.36), M, jit=0.08)
        if k % 3 == 0:
            puff(s, (x + 0.1, y + 0.1, LAKE_WATER + 0.03), 0.18, CLOTH_PINK, M, segs=5, h=0.16)
    for k, (x, y, r) in enumerate(((-1.9, 0.6, 0.55), (2.1, 0.9, 0.7), (-8.6, 0.8, 0.8), (8.4, 0.4, 0.65))):
        rock(pl, M, x, y, r, STONE, seed=k, segs=6, rings=3, moss=MOSS)
        pl.solid(M, x, y, r)
    bench(pl, M, -3.0, 2.8, math.pi)
    fr = [Vector((0.8, -8.3, 0.35)), Vector((1.4, -9.2, 1.6)), Vector((1.9, -10.4, 2.3))]
    sweep(s, fr, 0.03, 3, DARKWOOD, M, smooth=False)
    ribbon(s, [fr[-1], Vector((2.2, -11.0, LAKE_WATER))], 0.015, (0.9, 0.9, 0.9), M, normal_hint=(1, 0, 0))
    cylinder(s, 0.18, 0.3, 6, (0.60, 0.62, 0.66), T(-0.5, -7.8, top), r_top=0.22, top=False, bottom=True,
             smooth=False)
    pl.mark("props")
    pl.extra["water_z"] = LAKE_WATER
    pl.flat = 6.0
    pl.ground = [((-150, 0, 0), (150, 0, 0), (150, 150, 0), (-150, 150, 0)),
                 ((-150, -150, LAKE_WATER), (150, -150, LAKE_WATER), (150, 0, LAKE_WATER), (-150, 0, LAKE_WATER))]
    pl.terrain = lake_terrain
    pl.terrain_size = 26
    pl.water = LAKE_WATER
    pl.view = dict(radius=9.0, target=(0.5, -3.8, 0.0), azimuth=38.0, elevation=36.0)


# ---------------------------------------------------------------------- The Keeper's Lodge
def keeper_terrain(x, y):
    t = smooth01(-8.5, -11.0, y)
    return 12.0 * t + 0.8 * noise(Vector((x * 0.1, y * 0.1, 0.7))) * t, mixc(GRASS, ROCK_WARM,
                                                                             smooth01(0.05, 0.25, t))


def keeper_extras(pl, mat):
    fall = Part("fall")
    face(fall, [(-6.5, -9.3, 0.0), (-3.0, -9.3, 0.0), (-3.0, -9.3, 12.0), (-6.5, -9.3, 12.0)], (0, 1, 0),
         (0.9, 0.95, 1.0), I4)
    disc(fall, 3.2, 10, (0.8, 0.9, 1.0), T(-4.8, -7.4, 0.06))
    return [preview_mesh("fall", fall, mat, emissive=(0.80, 0.90, 0.98))]


@place("keeper", "The Keeper's Lodge")
def build_keeper(pl):
    M = I4
    s = pl.s
    w, d, h, rh = 8.4, 6.0, 3.2, 2.7
    box(s, (-w / 2 - 0.16, -d / 2 - 0.16, -0.3), (w / 2 + 0.16, d / 2 + 0.16, 0.3), STONE, M, skip=("-z",))
    walls(s, -w / 2 - 0.07, -d / 2 - 0.07, w / 2 + 0.07, d / 2 + 0.07, (0.3, 1.3), moss_top(STONE, MOSS, 0.5, 0.9), M,
          jit=0.12)
    walls(s, -w / 2, -d / 2, w / 2, d / 2, (1.3, h - 0.7, h), wall_col(CREAM, 1.3, h), M)
    gable_walls(pl, M, w, d, h, rh, wall_col(CREAM, 1.3, h))
    gable_roof(pl, M, w, d, h, rh, (0.44, 0.58, 0.40), ov=0.55, ovx=0.45)
    for sy in (1, -1):
        F = M @ T(0, sy * d / 2, 0, rz=0 if sy > 0 else math.pi)
        face(s, [(-w / 2, 0.08, 1.24), (w / 2, 0.08, 1.24), (w / 2, 0.08, 1.42), (-w / 2, 0.08, 1.42)], (0, 1, 0),
             TIMBER, F)
    # the crooked chimney up the right gable
    C = T(w / 2 + 0.42, -0.6, 0)
    box(s, (-0.5, -0.55, 0), (0.45, 0.55, h + 0.6), STONE, C, skip=("-z",))
    Cu = C @ T(-0.05, 0, h + 0.6, ry=-0.2)
    box(s, (-0.34, -0.38, 0), (0.34, 0.38, rh + 0.4), STONE, Cu, skip=("-z", "+z"))
    box(s, (-0.44, -0.46, rh + 0.4), (0.44, 0.46, rh + 0.65), tuple(c * 0.85 for c in STONE), Cu, skip=("-z",))
    front = M @ T(0, d / 2, 0)
    door(pl, front, 0, 0.3, col=DOOR_RED, w=1.15, h=2.1)
    for x in (-2.4, 2.4):
        window_square(pl, front, x, 2.05, shutters=DOOR_GREEN)
    flower_box(pl, front, -2.4, 1.43, seed=2)
    back = M @ T(0, -d / 2, 0, rz=math.pi)
    for x in (-2.0, 2.0):
        window_square(pl, back, x, 2.05)
    window_round(pl, M @ T(-w / 2, 0, 0, rz=math.pi / 2), 0, 2.1, 0.4)
    pl.solid(M, -2.5, 0, 3.4)
    pl.solid(M, 2.5, 0, 3.4)
    pl.mark("lodge")
    # the porch: deck, posts, a lean-to roof, a hanging lamp, the rocking chair
    box(s, (-2.4, d / 2, 0), (2.4, d / 2 + 2.1, 0.32), LIGHTWOOD, M, skip=("-z",))
    for x in (-2.2, 2.2):
        box(s, (x - 0.1, d / 2 + 1.85, 0.32), (x + 0.1, d / 2 + 2.05, 2.85), WOOD, M, skip=("-z", "+z"))
    P0, P1 = Vector((0, d / 2, 3.1)), Vector((0, d / 2 + 2.3, 2.75))
    for pts, n, c in (([(-2.6, P0.y, P0.z + 0.12), (2.6, P0.y, P0.z + 0.12), (2.6, P1.y, P1.z + 0.12),
                        (-2.6, P1.y, P1.z + 0.12)], (0, 0.2, 1), (0.44, 0.58, 0.40)),
                      ([(-2.6, P0.y, P0.z), (2.6, P0.y, P0.z), (2.6, P1.y, P1.z), (-2.6, P1.y, P1.z)], (0, 0, -1),
                       WOOD),
                      ([(-2.6, P1.y, P1.z), (2.6, P1.y, P1.z), (2.6, P1.y, P1.z + 0.12), (-2.6, P1.y, P1.z + 0.12)],
                       (0, 1, 0), (0.36, 0.48, 0.32))):
        face(s, pts, n, c, M)
    for sx in (-1, 1):
        face(s, [(sx * 2.6, P0.y, P0.z), (sx * 2.6, P1.y, P1.z), (sx * 2.6, P1.y, P1.z + 0.12),
                 (sx * 2.6, P0.y, P0.z + 0.12)], (sx, 0, 0), (0.36, 0.48, 0.32), M)
    box(s, (-1.45, d / 2 + 1.1, 2.1), (-1.15, d / 2 + 1.4, 2.45), IRON, M, skip=())
    for k in range(4):
        L = T(-1.3, d / 2 + 1.25, 0, rz=k * math.pi / 2)
        face(pl.g, [(-0.1, 0.155, 2.14), (0.1, 0.155, 2.14), (0.1, 0.155, 2.4), (-0.1, 0.155, 2.4)], (0, 1, 0),
             Emit(WARM), L)
    pl.lamp(M, (-1.3, d / 2 + 1.25, 2.2), 5.0, 0.6)
    R = T(1.2, d / 2 + 1.0, 0.32, rz=0.35)
    box(s, (-0.3, -0.28, 0.4), (0.3, 0.28, 0.48), WOOD, R, skip=())
    box(s, (-0.3, -0.34, 0.48), (0.3, -0.26, 1.2), WOOD, R @ T(0, 0, 0, rx=-0.15), skip=())
    for sx in (-0.27, 0.27):
        rk = [(sx, -0.55 + 1.1 * k / 3, 0.05 + 0.12 * (2 * k / 3 - 1) ** 2) for k in range(4)]
        ribbon(s, rk, 0.06, DARKWOOD, R, normal_hint=(1, 0, 0))
        box(s, (sx - 0.03, -0.2, 0.1), (sx + 0.03, 0.2, 0.4), DARKWOOD, R, skip=("-z", "+z"))
    pl.solid(M, 0, d / 2 + 1.0, 2.2)
    pl.mark("porch")
    # the vegetable patch
    V0 = T(7.2, 2.6, 0)
    box(s, (-1.6, -2.1, 0), (1.6, 2.1, 0.22), SOIL, V0, skip=("-z",))
    for k in range(4):
        puff(s, (-0.9, -1.5 + k, 0.2), 0.3, (0.56, 0.80, 0.40), V0, segs=5, h=0.3)
        puff(s, (0.0, -1.5 + k, 0.2), 0.14, (0.38, 0.66, 0.30), V0, segs=4, h=0.3)
        puff(s, (0.0, -1.5 + k, 0.2), 0.08, (0.98, 0.56, 0.22), V0, segs=4, h=0.06)
    for k in range(3):
        blob(s, 0.34, 0.34, 0.26, 6, 3, (0.98, 0.58, 0.20), V0 @ T(0.95, -1.2 + 1.2 * k, 0.45), jit=0.06)
    pl.solid(V0, 0, -1.0, 1.6)
    pl.solid(V0, 0, 1.0, 1.6)
    # the dragon's perch: a stout post with a crossbar, rope bound
    Pp = T(-7.4, 2.8, 0, rz=0.4)
    lathe(s, [(0.55, -0.2), (0.46, 5.0), (0.0, 5.12)], 7, WOOD, Pp, sharp=[1], lump=0.05)
    sweep(s, [(-1.8, 0, 5.15), (1.8, 0, 5.15)], 0.28, 6, WOOD, Pp, caps=True)
    for sx in (-1, 1):
        sweep(s, [(sx * 0.4, 0, 4.0), (sx * 1.2, 0, 5.0)], 0.11, 4, DARKWOOD, Pp, smooth=False)
    for z in (1.4, 3.6):
        lathe(s, [(0.52, z), (0.52, z + 0.28)], 7, (0.86, 0.76, 0.52), Pp, smooth=False)
    pl.solid(Pp, 0, 0, 0.7)
    pl.mark("garden")
    L0 = T(-w / 2 - 0.6, 1.8, 0)
    for k, (y, z) in enumerate(((0.0, 0.2), (0.45, 0.2), (0.9, 0.2), (0.22, 0.56), (0.67, 0.56))):
        W = L0 @ T(0, y, z) @ Matrix.Rotation(math.pi / 2, 4, "Y")
        cylinder(s, 0.2, 1.3, 5, (0.62, 0.46, 0.30), W @ T(0, 0, -0.65), bottom=True, smooth=False)
    pl.solid(L0, 0, 0.45, 0.9)
    for i, (x, y) in enumerate(((0.2, 6.2), (-0.4, 7.6), (0.3, 9.0), (-0.2, 10.4))):
        lathe(s, [(0.55, -0.08), (0.48, 0.12), (0, 0.15)], 6, (0.78, 0.74, 0.68), T(x, y, 0, rz=i), lump=0.08,
              seed=i)
    for x, y, sd in ((-3.4, 3.8, 1), (3.6, 4.0, 3), (5.2, -2.4, 2), (-5.4, -2.8, 4)):
        flower_clump(pl, M, x, y, 0.55, seed=sd)
    pl.mark("props")
    pl.door = [0.0, round(d / 2 + 2.6, 2)]
    pl.flat = 14.0
    pl.terrain = keeper_terrain
    pl.terrain_size = 34
    pl.preview_extra = keeper_extras
    pl.view = dict(radius=12.0, target=(0, 0.5, 2.0), azimuth=24.0, elevation=32.0)


# ---------------------------------------------------------------------- The Floating Isles
def isle_extras(pl, mat):
    isle = Part("isle")
    rows = []
    for r, z in ((0.0, -13.0), (3.0, -9.0), (7.5, -5.0), (11.0, -2.2), (13.2, -0.6), (13.6, 0.0)):
        rows.append([Vector((math.cos(2 * math.pi * k / 16) * r, math.sin(2 * math.pi * k / 16) * r, z))
                     for k in range(16)])
    for row in rows[1:-1]:
        for p in row:
            p += Vector((0, 0, 1)) * 0.8 * noise(p * 0.3)
    grid(isle, rows, lambda p, n: EARTH if p.z < -0.4 else GRASS, I4, closed=True, smooth=False, jit=0.1)
    disc(isle, 13.6, 16, GRASS, I4, rings=[13.6, 8.0], smooth=True)
    return [preview_mesh("isle", isle, mat)]


@place("isles", "The Floating Isles")
def build_isles(pl):
    M = I4
    s = pl.s
    D = T(0, -1.6, 0, s=1.3)
    for sx in (-1, 1):
        blob(s, 0.5, 0.36, 1.25, 6, 3, moss_top(STONE, MOSS, 0.4, 0.8), D @ T(sx * 0.95, 0, 1.0, rz=sx * 0.1),
             lump=0.1, seed=sx)
    blob(s, 1.65, 0.62, 0.3, 7, 3, moss_top(STONE, MOSS, 0.3, 0.7), D @ T(0, 0, 2.35, rz=0.05), lump=0.1, seed=5)
    lathe(s, [(0.22, 0.0), (0.18, 0.4), (0.45, 0.62), (0.38, 0.66), (0.0, 0.5)], 7, (0.80, 0.78, 0.74), D,
          sharp=[1, 2, 3])
    puff(s, (0, 0, 0.58), 0.2, CLOTH_PINK, D, segs=5, h=0.18)
    pl.solid(D, 0, 0, 1.8)
    for k, r in enumerate((0.75, 0.6, 0.46, 0.32)):
        blob(s, r, r * 0.9, r * 0.36, 6, 3, (0.78, 0.76, 0.72), T(2.9, 0.4, 0.2 + 0.46 * k, rz=k), lump=0.1,
             seed=k)
    pl.solid(M, 2.9, 0.4, 0.9)
    for k in range(7):
        a = 2 * math.pi * k / 7 + 0.4
        if abs(math.sin(a) - 1) < 0.1:
            continue
        rock(pl, M, math.cos(a) * 4.2, math.sin(a) * 4.2 - 0.6, 0.38, STONE, seed=k, segs=5, rings=3, moss=MOSS)
    pl.mark("shrine")
    for x, y in ((-3.6, -3.6), (3.4, -3.0)):
        box(s, (x - 0.07, y - 0.07, 0), (x + 0.07, y + 0.07, 3.6), WOOD, M, skip=("-z",))
        puff(s, (x, y, 3.6), 0.14, GOLD, M, segs=4, h=0.2)
        pl.solid(M, x, y, 0.3)
    bunting(pl, M, (-3.6, -3.6, 3.45), (3.4, -3.0, 3.45), sag=0.7, flags=8)
    for i, (x, y, r) in enumerate(((0.2, 2.2, 0.5), (-0.3, 3.5, 0.55), (0.3, 4.8, 0.5), (-0.1, 6.1, 0.55))):
        lathe(s, [(r, -0.08), (r * 0.88, 0.12), (0, 0.15)], 6, (0.80, 0.78, 0.72), T(x, y, 0, rz=i), lump=0.08,
              seed=i)
    for x, y, sd in ((-2.0, 0.8, 1), (1.8, -3.0, 2), (-2.8, -3.2, 3), (3.8, 2.6, 4), (-4.4, 2.4, 0), (4.6, -1.6, 5),
                     (1.4, 6.0, 1)):
        flower_clump(pl, M, x, y, 0.45, seed=sd)
    for k, (x, y, r) in enumerate(((-5.6, -1.4, 1.0), (5.2, 1.4, 0.9), (-3.6, 4.6, 0.8))):
        bush(pl, M, x, y, r, seed=k + 3, col=LEAF if k % 2 else (0.46, 0.70, 0.34))
        pl.solid(M, x, y, r + 0.1)
    pl.mark("flowers")
    festival_lantern(pl, M, 1.9, 3.0, rz=math.radians(200))
    pl.flat = 7.0
    pl.terrain = "none"
    pl.preview_extra = isle_extras
    pl.view = dict(radius=10.0, target=(0, 0, 0.6), azimuth=24.0, elevation=30.0)


# ---------------------------------------------------------------------- Honeyroot Orchard
def orchard_extras(pl, mat):
    trees = Part("trees")
    for k, (x, y) in enumerate(((-6.5, -4.4), (-1.5, -4.8), (3.4, -5.2), (7.6, -3.8), (-8.2, 5.4))):
        cylinder(trees, 0.3, 2.2, 6, (0.50, 0.36, 0.24), T(x, y, 0))
        blob(trees, 2.1, 2.1, 1.8, 8, 5, LEAF, T(x, y, 3.3), lump=0.12, seed=k)
        for j in range(4):
            a = j * 1.6 + k
            puff(trees, (x + math.cos(a) * 1.6, y + math.sin(a) * 1.6, 3.0 + 0.3 * j), 0.22, (0.92, 0.30, 0.26), I4)
    return [preview_mesh("trees", trees, mat)]


def ladder(pl, M, length=3.2, width=0.55, rungs=5):
    s = pl.s
    for sx in (-1, 1):
        box(s, (sx * width / 2 - 0.05, -0.04, 0), (sx * width / 2 + 0.05, 0.04, length), WOOD, M, skip=("-z",))
    for k in range(rungs):
        z = length * (k + 0.8) / (rungs + 0.6)
        quad(s, (-width / 2, 0, z), (width / 2, 0, z), (width / 2, 0, z + 0.07), (-width / 2, 0, z + 0.07),
             LIGHTWOOD, M, double=True)


def basket(pl, M, x, y, fruit, r=0.38):
    W = M @ T(x, y, 0)
    lathe(pl.s, [(r * 0.8, 0), (r, 0.36), (r * 0.9, 0.36)], 6, STRAW, W, sharp=[1], jit=0.1)
    dome(pl.s, r * 0.9, 0.22, 6, fruit, W @ T(0, 0, 0.3), rings=1, lump=0.15)
    pl.solid(W, 0, 0, r + 0.1)


@place("orchard", "Honeyroot Orchard")
def build_orchard(pl):
    M = I4
    s = pl.s
    fence_run(pl, M, [(-10, 7.5), (-10, 2.5), (-10, -2.5), (-10, -7.5), (-5, -7.5), (0, -7.5), (5, -7.5),
                      (10, -7.5)])
    for k in range(9):
        pl.solid(M, -10, 7.5 - 15 * k / 8, 0.4)
        pl.solid(M, -10 + 20 * k / 8, -7.5, 0.4)
    pl.mark("fence")
    # the fruit cart
    C = T(1.4, 0.6, 0, rz=0.35)
    zb = 0.75
    face(s, [(-1.2, -0.7, zb), (1.2, -0.7, zb), (1.2, 0.7, zb), (-1.2, 0.7, zb)], (0, 0, 1), WOOD, C)
    face(s, [(-1.2, -0.7, zb), (1.2, -0.7, zb), (1.2, 0.7, zb), (-1.2, 0.7, zb)], (0, 0, -1), DARKWOOD, C)
    for k in range(4):
        R = C @ T(0, 0, 0, rz=k * math.pi / 2)
        half = 1.2 if k % 2 == 0 else 0.7
        other = 0.7 if k % 2 == 0 else 1.2
        box(s, (-half, other - 0.08, zb), (half, other, zb + 0.5), WOOD if k % 2 else LIGHTWOOD, R, skip=("-z",))
    for sy in (-1, 1):
        Wh = C @ T(0.1, sy * 0.86, 0.62) @ Matrix.Rotation(-sy * math.pi / 2, 4, "X")
        cylinder(s, 0.62, 0.12, 8, DARKWOOD, Wh, bottom=False, smooth=False)
        puff(s, (0, 0, 0.12), 0.14, IRON, Wh, segs=4, h=0.1)
    for sy in (-0.5, 0.5):
        box(s, (1.2, sy - 0.04, zb + 0.1), (2.6, sy + 0.04, zb + 0.18), WOOD, C, skip=())
    box(s, (-1.1, -0.05, 0), (-1.0, 0.05, zb), DARKWOOD, C, skip=("-z", "+z"))
    for k, (x, fc) in enumerate(((-0.6, (0.92, 0.28, 0.24)), (0.2, (0.98, 0.78, 0.30)), (0.9, (0.70, 0.84, 0.32)))):
        basket(pl, C @ T(0, 0, zb), x, 0.1 * (k - 1), fc, 0.34)
    pl.solid(C, 0, 0, 1.4)
    pl.solid(C, 1.8, 0, 0.6)
    pl.mark("cart")
    ladder(pl, T(-9.2, -1.2, 0, rz=math.pi / 2) @ Matrix.Rotation(-0.3, 4, "X"), 3.1)
    for sgn in (-1, 1):
        ladder(pl, T(5.0, -2.8, 0, rz=0.3) @ T(0, sgn * 0.55, 0) @ Matrix.Rotation(sgn * 0.2, 4, "X"), 2.6, 0.6, 4)
    pl.solid(M, 5.0, -2.8, 0.8)
    pl.mark("ladders")
    # the scarecrow
    S = T(-5.0, 3.4, 0, rz=0.25)
    box(s, (-0.06, -0.06, 0), (0.06, 0.06, 2.3), DARKWOOD, S, skip=("-z",))
    box(s, (-1.1, -0.05, 1.7), (1.1, 0.05, 1.8), DARKWOOD, S, skip=())
    prism(s, [(-0.42, 0.95), (0.42, 0.95), (0.5, 1.85), (-0.5, 1.85)], 0.36, (0.48, 0.60, 0.80), S @ T(0, -0.18, 0),
          back=True)
    face(s, [(-0.3, 0.19, 1.1), (-0.05, 0.19, 1.1), (-0.05, 0.19, 1.35), (-0.3, 0.19, 1.35)], (0, 1, 0),
         (0.90, 0.40, 0.34), S)
    for sx in (-1, 1):
        box(s, (sx * 0.5, -0.12, 1.62), (sx * 1.0, 0.12, 1.86), (0.48, 0.60, 0.80), S, skip=())
        puff(s, (sx * 1.02, 0, 1.58), 0.14, STRAW, S, segs=4, h=0.24)
    blob(s, 0.34, 0.3, 0.34, 6, 3, (0.90, 0.80, 0.60), S @ T(0, 0, 2.2), jit=0.06)
    lathe(s, [(0.68, 2.36), (0.66, 2.42), (0.34, 2.44), (0.3, 2.8), (0.0, 2.84)], 7, STRAW, S, sharp=[1, 2, 3])
    puff(s, (0.12, 0.3, 2.22), 0.05, DARKWOOD, S, segs=3, h=0.03)
    puff(s, (-0.12, 0.3, 2.22), 0.05, DARKWOOD, S, segs=3, h=0.03)
    blob(s, 0.14, 0.2, 0.12, 5, 3, (0.20, 0.20, 0.26), S @ T(0.8, 0, 1.94), jit=0.02)
    pl.solid(S, 0, 0, 0.6)
    pl.mark("scarecrow")
    basket(pl, M, 3.4, 2.4, (0.92, 0.28, 0.24))
    basket(pl, M, 4.1, 1.7, (0.98, 0.78, 0.30))
    crate(pl, M, -1.4, 2.6, 0.72, 0.5, fill=(0.92, 0.28, 0.24))
    for k, (x, y) in enumerate(((2.6, 3.4), (-0.4, 1.6), (4.8, 0.4), (-2.4, -1.6))):
        puff(s, (x, y, 0), 0.12, (0.92, 0.28, 0.24), M, segs=4, h=0.18)
    # Honeyroot's bees: two straw skeps on a bench
    Bk = T(-1.8, 4.8, 0, rz=0.15)
    box(s, (-1.0, -0.3, 0.45), (1.0, 0.3, 0.55), WOOD, Bk, skip=())
    for sx in (-0.8, 0.8):
        box(s, (sx - 0.15, -0.22, 0), (sx + 0.15, 0.22, 0.45), STONE, Bk, skip=("-z", "+z"))
    for sx in (-0.5, 0.5):
        lathe(s, [(0.42, 0.55), (0.45, 0.78), (0.38, 1.02), (0.22, 1.2), (0.0, 1.26)], 7, STRAW, Bk @ T(sx, 0, 0),
              sharp=[], jit=0.1)
        face(s, [(sx - 0.1, 0.43, 0.58), (sx + 0.1, 0.43, 0.58), (sx, 0.43, 0.72)], (0, 1, 0), (0.24, 0.18, 0.12), Bk)
    for k in range(3):
        puff(s, (-0.3 + 0.4 * k, 0.6 + 0.1 * k, 1.5 + 0.2 * (k % 2)), 0.06, (1.0, 0.84, 0.20), Bk, segs=3, h=0.06)
    pl.solid(Bk, 0, 0, 1.1)
    for x, y, sd in ((-7.8, 6.2, 1), (7.8, -6.2, 2), (-8.6, -3.4, 3)):
        flower_clump(pl, M, x, y, 0.5, seed=sd)
    pl.mark("props")
    pl.flat = 12.0
    pl.terrain_size = 32
    pl.preview_extra = orchard_extras
    pl.view = dict(radius=10.0, target=(-1.5, -0.4, 1.2), azimuth=22.0, elevation=36.0)


# ---------------------------------------------------------------------- Windmill Bridge
MILL_WATER = -1.3
RIVER_HALF = 6.0


def mill_terrain(x, y):
    t = smooth01(RIVER_HALF + 1.2, RIVER_HALF - 0.4, abs(x))
    z = -2.3 * t + 0.2 * noise(Vector((x * 0.2, y * 0.2, 0)))
    return z, mixc(GRASS, EARTH, smooth01(0.1, 0.5, t))


@place("mill", "Windmill Bridge")
def build_mill(pl):
    M = I4
    s = pl.s
    xs = [-9.0, -7.4, -6.0, -4.8, -3.2, -1.6, 0.0, 1.6, 3.2, 4.8, 6.0, 7.4, 9.0]

    def top(x):
        return 0.12 + 1.45 * (1 - (x / 9.0) ** 2)

    def bottom(x):
        if abs(x) <= 4.8:
            return MILL_WATER - 0.05 + 2.35 * math.sqrt(max(0.0, 1 - (x / 4.8) ** 2))
        return MILL_WATER - 0.1 if abs(x) <= 6.0 else -0.3
    hw, pw, ph = 1.8, 0.35, 0.6
    stone = (0.80, 0.74, 0.64)
    for i in range(len(xs) - 1):
        a, b = xs[i], xs[i + 1]
        for sy in (1, -1):
            y = sy * hw
            face(s, [(a, y, bottom(a)), (b, y, bottom(b)), (b, y, top(b) + ph), (a, y, top(a) + ph)], (0, sy, 0),
                 stone, M, jit=0.08)
            face(s, [(a, y, top(a) + ph), (b, y, top(b) + ph), (b, y - sy * pw, top(b) + ph),
                     (a, y - sy * pw, top(a) + ph)], (0, 0, 1), (0.86, 0.80, 0.70), M)
            face(s, [(a, y - sy * pw, top(a) + ph), (b, y - sy * pw, top(b) + ph), (b, y - sy * pw, top(b)),
                     (a, y - sy * pw, top(a))], (0, -sy, 0), stone, M, jit=0.08)
        face(s, [(a, -hw + pw, top(a)), (b, -hw + pw, top(b)), (b, hw - pw, top(b)), (a, hw - pw, top(a))],
             (0, 0, 1), (0.70, 0.64, 0.56), M, jit=0.1)
        if abs(a) < 4.81 and abs(b) < 4.81:
            face(s, [(a, -hw, bottom(a)), (b, -hw, bottom(b)), (b, hw, bottom(b)), (a, hw, bottom(a))], (0, 0, -1),
                 tuple(c * 0.8 for c in stone), M)
    for sx in (-1, 1):
        x = sx * 9.0
        face(s, [(x, -hw, -0.3), (x, hw, -0.3), (x, hw, top(x)), (x, -hw, top(x))], (sx, 0, 0), stone, M)
        for sy in (1, -1):
            face(s, [(x, sy * hw, top(x)), (x, sy * (hw - pw), top(x)), (x, sy * (hw - pw), top(x) + ph),
                     (x, sy * hw, top(x) + ph)], (sx, 0, 0), stone, M)
    for sy in (1, -1):  # the arch ring's stones and the keystone
        arc = [-4.8, -3.6, -2.4, -1.2, 0.0, 1.2, 2.4, 3.6, 4.8]
        for i in range(len(arc) - 1):
            a, b = arc[i], arc[i + 1]
            ca = Vector((a / 4.8, (bottom(a) - MILL_WATER) / 2.35, 0)).normalized()
            cb = Vector((b / 4.8, (bottom(b) - MILL_WATER) / 2.35, 0)).normalized()
            y = sy * (hw + 0.03)
            c = (0.88, 0.82, 0.72) if i % 2 else (0.72, 0.66, 0.58)
            face(s, [(a, y, bottom(a)), (b, y, bottom(b)), (b + cb.x * 0.5, y, bottom(b) + cb.y * 0.5),
                     (a + ca.x * 0.5, y, bottom(a) + ca.y * 0.5)], (0, sy, 0), c, M)
        box(s, (-0.3, sy * hw - 0.1, bottom(0) - 0.05), (0.3, sy * hw + 0.1, bottom(0) + 0.65), (0.90, 0.84, 0.74), M,
            skip=())
    lamp_post(pl, M @ T(0, 0, top(8.6) + ph), -8.6, hw - pw / 2, h=1.3)
    lamp_post(pl, M @ T(0, 0, top(8.6) + ph), 8.6, -hw + pw / 2, h=1.3)
    for x in (-8.6, -7.4, 7.4, 8.6):
        for sy in (1, -1):
            pl.solid(M, x, sy * hw, 0.3)
    pl.mark("bridge")
    # the windmill
    mx, my = -12.8, -3.2
    W = T(mx, my, 0)
    segs = 10
    a0 = math.pi / 2 - math.pi / segs
    lathe(s, [(3.15, -0.3), (3.08, 0.9), (2.98, 0.9)], segs, (0.66, 0.62, 0.58), W, sharp=[1], a0=a0, smooth=False)
    lathe(s, [(2.98, 0.9), (2.7, 4.5), (2.42, 7.5)], segs, (0.90, 0.84, 0.74), W, a0=a0, jit=0.06)
    lathe(s, [(2.8, 7.2), (2.9, 7.5), (2.3, 8.7), (1.2, 9.7), (0.0, 10.1)], segs,
          lambda p, n: tuple(c * (0.9 + 0.12 * max(0.0, n.z)) for c in TERRACOTTA), W, a0=a0, lump=0.03)
    face(s, [(2.8 * math.cos(a0 + 2 * math.pi * k / segs), 2.8 * math.sin(a0 + 2 * math.pi * k / segs), 7.2)
             for k in range(segs)], (0, 0, -1), (0.6, 0.3, 0.2), W)
    ap = math.cos(math.pi / segs)
    door(pl, W @ T(0, 2.9 * ap, 0), 0, 0.9, w=1.15, h=2.0, col=DOOR_BLUE)
    for z, ang in ((4.3, math.pi / 2 + 2 * math.pi / segs), (6.1, math.pi / 2 - 2 * math.pi / segs)):
        rr = (2.98 + (2.42 - 2.98) * (z - 0.9) / 6.6) * ap
        window_round(pl, W @ T(math.cos(ang) * rr, math.sin(ang) * rr, 0, rz=ang - math.pi / 2), 0, z, 0.3)
    window_square(pl, W @ T(0, -2.7 * ap, 0, rz=math.pi), 0, 3.6, w=0.6, h=0.8)
    hub = Vector((mx, my + 3.05, 8.4))
    sweep(s, [(mx, my + 1.6, 8.4), (hub.x, hub.y - 0.1, hub.z)], 0.18, 5, DARKWOOD, M, smooth=False)
    sails = pl.part("sails")
    H = T(hub.x, hub.y, hub.z)
    lathe(sails, [(0.36, -0.05), (0.32, 0.25), (0.0, 0.46)], 6, WOOD, H @ Matrix.Rotation(-math.pi / 2, 4, "X"),
          vflags=NOSHADOW | NOAO)
    for k in range(4):
        A = H @ T(0, 0.3, 0) @ Matrix.Rotation(math.pi / 4 + k * math.pi / 2, 4, "Y")
        box(sails, (0.2, -0.07, -0.07), (5.8, 0.07, 0.07), DARKWOOD, A, vflags=NOSHADOW | NOAO)
        for x0, x1, c in ((1.1, 3.3, CLOTH_CREAM), (3.4, 5.6, (0.98, 0.90, 0.78))):
            quad(sails, (x0, 0.02, 0.1), (x1, 0.02, 0.1), (x1, 0.02, 1.2), (x0, 0.02, 1.2), c, A, double=True,
                 vflags=NOSHADOW | NOAO)
        box(sails, (1.0, -0.04, 1.2), (5.7, 0.04, 1.28), WOOD, A, skip=(), vflags=NOSHADOW | NOAO)
    pl.extra["hub"] = [round(hub.x, 3), round(hub.y, 3), round(hub.z, 3)]
    pl.extra["hub_axis"] = [0, 1, 0]
    pl.solid(W, 0, 0, 3.4)
    pl.mark("mill")
    for k, (x, y) in enumerate(((-11.4, 0.6), (-10.9, 0.1))):
        blob(s, 0.36, 0.3, 0.42, 6, 3, (0.94, 0.90, 0.80), T(x, y, 0.36, rz=k), jit=0.05)
    pl.solid(M, -11.2, 0.4, 0.7)
    # (No modelled road to its door: the landscape's path runs beside it, run 19.)
    reeds(pl, T(0, 0, MILL_WATER + 0.1), 5.5, 4.2, 5, 1.3, seed=1)
    reeds(pl, T(0, 0, MILL_WATER + 0.1), -5.5, -4.6, 5, 1.3, seed=2)
    for x, y, sd in ((-8.2, 3.2, 1), (8.4, 3.0, 2), (-15.8, 0.6, 3), (8.0, -3.4, 4)):
        flower_clump(pl, M, x, y, 0.5, seed=sd)
    pl.mark("props")
    pl.extra["water_z"] = MILL_WATER
    pl.extra["river"] = {"axis": "y", "half_width": RIVER_HALF}
    pl.flat = 17.0
    pl.ground = [((-150, -150, 0), (-RIVER_HALF, -150, 0), (-RIVER_HALF, 150, 0), (-150, 150, 0)),
                 ((RIVER_HALF, -150, 0), (150, -150, 0), (150, 150, 0), (RIVER_HALF, 150, 0)),
                 ((-RIVER_HALF, -150, MILL_WATER), (RIVER_HALF, -150, MILL_WATER), (RIVER_HALF, 150, MILL_WATER),
                  (-RIVER_HALF, 150, MILL_WATER))]
    pl.terrain = mill_terrain
    pl.terrain_size = 34
    pl.water = MILL_WATER
    pl.view = dict(radius=15.0, target=(-3.5, -0.8, 2.5), azimuth=24.0, elevation=30.0)


# ---------------------------------------------------------------------- The Hidden Grotto
def cave_terrain(x, y):
    return 0.0, (0.30, 0.34, 0.36)


@place("grotto", "The Hidden Grotto")
def build_grotto(pl):
    M = I4
    s = pl.s
    deg = math.radians
    disc(s, 6.7, 14, lambda p, n: mixc((0.46, 0.50, 0.54), (0.34, 0.38, 0.42), p.to_2d().length / 7), M, z=0.03,
         rings=[6.7, 3.4], smooth=True)
    prof = [(6.6, -0.2), (6.9, 1.8), (6.3, 3.8), (4.6, 5.6), (2.2, 6.7), (0.0, 7.0)]

    def cave(p, n):
        return mixc(ROCK_CAVE, (0.30, 0.34, 0.40), smooth01(1.0, 6.0, p.z))
    lathe(s, prof, 12, cave, M, arc=(deg(306), deg(594)), flip=True, lump=0.1, seed=3)
    lathe(s, [(6.4, 3.5), (6.3, 3.8), (4.6, 5.6), (2.2, 6.7), (0.0, 7.0)], 4, cave, M, arc=(deg(234), deg(306)),
          flip=True, lump=0.1, seed=3)
    for sx in (-1, 1):
        a = deg(270 + sx * 36)
        rock(pl, M, math.cos(a) * 6.9, math.sin(a) * 6.9, 1.4, ROCK_CAVE, (1.0, 1.0, 1.9), seed=sx + 4,
             moss=LEAF_DARK)
    blob(s, 2.6, 0.9, 0.7, 7, 3, moss_top(ROCK_CAVE, LEAF_DARK, 0.6, 0.95), T(0, -6.9, 3.9), lump=0.15, seed=7)
    for k in range(24):
        a = deg(306) + (deg(594) - deg(306)) * k / 23
        pl.solid(M, math.cos(a) * 6.9, math.sin(a) * 6.9, 0.9)
    pl.mark("chamber")
    for ang, n, sd in ((20, 4, 1), (62, 3, 2), (108, 4, 3), (150, 3, 4), (200, 3, 5)):
        a = deg(ang)
        cx, cy = math.cos(a) * 5.3, math.sin(a) * 5.3
        for j in range(n):
            off = (j - (n - 1) / 2) * 0.5
            px, py = cx - math.sin(a) * off, cy + math.cos(a) * off
            crystal(pl, M, px, py, 0.0, 1.1 + 0.5 * ((j + sd) % 3), 0.2 + 0.04 * (j % 2), tilt=0.35 + 0.1 * j,
                    yaw=a + math.pi / 2 + (j - 1) * 0.4)
        pl.lamp(M, (cx * 0.8, cy * 0.8, 1.2), 7.0, 1.1, rgb=(0.45, 1.0, 0.6), table=CRYSTAL)
        pl.solid(M, cx, cy, 1.0)
    pl.mark("crystals")
    P = T(-1.6, 1.4, 0)
    torus_nest(pl, P, 1.8, 0.3, 10, 3, (0.56, 0.58, 0.60))
    disc(s, 1.8, 10, (0.20, 0.58, 0.64), P, z=0.14, jit=0.03)
    disc(pl.g, 1.7, 10, Emit((0.2, 0.6, 0.6), CRYSTAL, fade=lambda p: 0.35 * max(0.0, 1 - (p - Vector(
        (-1.6, 1.4, 0))).length / 1.8)), P, z=0.16, jit=0)
    pl.solid(P, 0, 0, 2.1)
    C = T(2.2, -0.6, 0, rz=0.5, s=1.3)
    box(s, (-0.62, -0.4, 0), (0.62, 0.4, 0.58), (0.58, 0.36, 0.22), C, skip=("-z", "+z"))
    face(s, [(-0.58, -0.36, 0.5), (0.58, -0.36, 0.5), (0.58, 0.36, 0.5), (-0.58, 0.36, 0.5)], (0, 0, 1),
         (0.30, 0.18, 0.12), C)
    dome(s, 0.5, 0.2, 6, GOLD, C @ T(0, 0, 0.48) @ T(0, 0, 0, s=(1.1, 0.66, 1.0)), rings=1, lump=0.2)
    dome(pl.g, 0.56, 0.3, 6, Emit((1.0, 0.78, 0.30), CRYSTAL, fade=lambda p: 0.5), C @ T(0, 0, 0.48) @
         T(0, 0, 0, s=(1.1, 0.66, 1.0)), rings=1)
    Lh = C @ T(0, -0.4, 0.58) @ Matrix.Rotation(1.9, 4, "X") @ T(0, 0.4, 0)  # the lid, thrown open
    lid = [[Vector((x, -0.4 * math.cos(math.pi * k / 5), 0.36 * math.sin(math.pi * k / 5))) for x in
            (-0.62, 0.62)] for k in range(6)]
    grid(s, lid, (0.64, 0.40, 0.24), Lh, smooth=True, flip=True)
    grid(s, lid, (0.36, 0.22, 0.14), Lh, smooth=True, flip=False)
    for sx in (-1, 1):
        face(s, [(sx * 0.62, -0.4 * math.cos(math.pi * k / 5), 0.36 * math.sin(math.pi * k / 5))
                 for k in range(6)], (sx, 0, 0), (0.58, 0.36, 0.22), Lh)
    for sx in (-0.35, 0.35):
        face(s, [(sx - 0.07, 0.41, 0.0), (sx + 0.07, 0.41, 0.0), (sx + 0.07, 0.41, 0.58), (sx - 0.07, 0.41, 0.58)],
             (0, 1, 0), GOLD, C)
    box(s, (-0.12, 0.38, 0.3), (0.12, 0.48, 0.52), GOLD, C, skip=())
    for k, (x, y) in enumerate(((2.0, 0.2), (2.9, 0.0), (1.6, -0.3), (3.2, -1.6))):
        puff(s, (x, y, 0.03), 0.12, GOLD, M, segs=4, h=0.05)
    puff(s, (1.8, 0.5, 0.03), 0.1, (0.90, 0.30, 0.40), M, segs=4, h=0.14)
    pl.solid(C, 0, 0, 0.9)
    pl.lamp(C, (0, 0.2, 1.0), 3.0, 0.5, rgb=(1.0, 0.8, 0.4), table=CRYSTAL)
    pl.mark("pool, chest")
    for k, (r, a) in enumerate(((2.4, 30), (3.8, 80), (1.6, 150), (4.2, 200), (3.0, 330), (4.4, 120))):
        z = 6.8 - 0.25 * r * r
        lathe(s, [(0.0, -0.9 - 0.3 * (k % 3)), (0.22, 0.2)], 4, ROCK_CAVE, T(math.cos(deg(a)) * r,
                                                                             math.sin(deg(a)) * r, z), smooth=False)
    for k, ang in enumerate((40, 88, 132, 176)):
        a = deg(ang)
        x, y = math.cos(a) * 5.9, math.sin(a) * 5.9
        cylinder(s, 0.08, 0.5, 4, (0.86, 0.84, 0.80), T(x, y, 0), smooth=False)
        puff(s, (x, y, 0.46), 0.32, (0.60, 0.52, 0.96), M, segs=5, h=0.2)
        puff(pl.g, (x, y, 0.47), 0.36, Emit((0.5, 0.4, 1.0), CRYSTAL), M, segs=5, h=0.23)
        pl.lamp(M, (x * 0.9, y * 0.9, 0.8), 2.5, 0.5, rgb=(0.6, 0.5, 1.0), table=CRYSTAL)
    pl.mark("details")
    pl.flat = 7.0
    pl.ao_reach, pl.ao_strength = 9.0, 0.8
    pl.ground = [((-150, -150, 0), (150, -150, 0), (150, 150, 0), (-150, 150, 0))]
    pl.terrain = cave_terrain
    pl.terrain_size = 22
    pl.view = dict(radius=8.5, target=(0, 0.6, 1.4), azimuth=200.0, elevation=48.0)


# ---------------------------------------------------------------------- Starwatch Ruins
@place("ruins", "Starwatch Ruins")
def build_ruins(pl):
    M = I4
    s = pl.s
    deg = math.radians
    tc = Vector((0, -1.0, 0))
    ro, ri = 3.3, 2.6
    a0, a1 = deg(90 + 16), deg(90 - 16 + 360)
    n = 11
    heights = []
    for k in range(n + 1):
        a = a0 + (a1 - a0) * k / n
        base = 2.3 + 5.0 * (0.5 - 0.5 * math.sin(a))
        jag = (0.9, -0.7, 0.4, -1.0, 0.8, -0.4)[k % 6]
        heights.append(max(1.3, base + 0.5 * noise(Vector((k * 1.7, 0.3, 0.0))) + jag))

    def ringpts(r, zfn):
        out = []
        for k in range(n + 1):
            a = a0 + (a1 - a0) * k / n
            out.append(Vector((tc.x + math.cos(a) * r, tc.y + math.sin(a) * r, zfn(k))))
        return out
    star_stone = (0.70, 0.70, 0.75)
    mossy = moss_top(star_stone, MOSS, 0.45, 0.85)

    def wall_colour(p, nrm):
        c = mossy(p, nrm)
        return mixc(c, MOSS, 0.35 * smooth01(1.0, 0.1, p.z) * (0.5 + 0.5 * noise(p * 0.8)))
    rows_o = [ringpts(ro, lambda k: -0.2), ringpts(ro, lambda k: min(2.0, heights[k] * 0.45)),
              ringpts(ro, lambda k: heights[k])]
    rows_i = [ringpts(ri, lambda k: 0.0), ringpts(ri, lambda k: min(2.0, heights[k] * 0.45)),
              ringpts(ri, lambda k: heights[k])]
    grid(s, rows_o, wall_colour, M, smooth=True, jit=0.1)
    grid(s, rows_i, wall_colour, M, smooth=True, flip=True, jit=0.1)
    for k in range(n):
        face(s, [rows_o[2][k], rows_o[2][k + 1], rows_i[2][k + 1], rows_i[2][k]], (0, 0, 1),
             mixc(STONE, MOSS, 0.45), M, jit=0.1)
    for k, sgn in ((0, -1), (n, 1)):
        a = a0 if k == 0 else a1
        face(s, [rows_o[0][k], rows_o[2][k], rows_i[2][k], rows_i[0][k]],
             Vector((-math.sin(a), math.cos(a), 0)) * sgn, STONE, M)
    for k in range(14):  # proud stones in the masonry
        a = deg(118 + 23.5 * k)
        z = 0.5 + (k * 1.37) % 3.4
        if z > heights[min(n, max(0, int(round((a - a0) / (a1 - a0) * n))))] - 0.5:
            continue
        box(s, (-0.3, -0.02, -0.16), (0.3, 0.14, 0.16), (0.64, 0.64, 0.70),
            T(tc.x + math.cos(a) * ro, tc.y + math.sin(a) * ro, z, rz=a - math.pi / 2), skip=("-y",))
    for ang, z in ((200, 4.2), (330, 3.6)):
        a = deg(ang)
        F = T(tc.x + math.cos(a) * (ro + 0.02), tc.y + math.sin(a) * (ro + 0.02), 0, rz=a - math.pi / 2)
        face(s, [(x, 0.0, z + zz) for x, zz in arch_outline(0.6, 1.1, 3)], (0, 1, 0), (0.18, 0.18, 0.24), F)
    for k in range(12):
        a = deg(100) + deg(340) * k / 11
        pl.solid(M, tc.x + math.cos(a) * 2.95, tc.y + math.sin(a) * 2.95, 0.6)
    pl.mark("tower")
    disc(s, 2.6, 10, (0.72, 0.70, 0.66), T(tc.x, tc.y, 0), z=0.05)
    lathe(s, [(1.05, 0.0), (1.05, 0.45), (0.0, 0.45)], 10, (0.80, 0.78, 0.74), T(tc.x, tc.y, 0), sharp=[1])
    star = star_outline(0.8, 0.34, 5)
    face(s, [(tc.x + x, tc.y + y, 0.462) for x, y in star], (0, 0, 1), (0.96, 0.88, 0.60), M)
    face(pl.g, [(tc.x + x, tc.y + y, 0.472) for x, y in star], (0, 0, 1), Emit((0.70, 0.80, 1.0)), M)
    # the telescope on its tripod, looking up through the open top
    Tl = T(tc.x, tc.y, 0.45, rz=math.radians(75))
    for k in range(3):
        a = 2 * math.pi * k / 3
        leg = Tl @ T(math.cos(a) * 0.35, math.sin(a) * 0.35, 0, rz=a) @ Matrix.Rotation(-0.25, 4, "Y")
        box(s, (-0.04, -0.04, 0), (0.04, 0.04, 1.35), DARKWOOD, leg, skip=("-z", "+z"))
    box(s, (-0.12, -0.12, 1.25), (0.12, 0.12, 1.45), DARKWOOD, Tl, skip=())
    tube = Tl @ T(0, 0, 1.55) @ Matrix.Rotation(-0.75, 4, "X")
    lathe(s, [(0.0, -1.1), (0.15, -1.1), (0.24, 1.1), (0.28, 1.1), (0.28, 1.3), (0.0, 1.3)], 6, BRASS, tube,
          sharp=[1, 2, 3, 4], smooth=True)
    lathe(s, [(0.08, -1.4), (0.08, -1.1)], 5, DARKWOOD, tube, smooth=False)
    lathe(s, [(0.25, -0.1), (0.25, 0.1)], 6, (0.30, 0.26, 0.40), tube, smooth=False)
    pl.solid(M, tc.x, tc.y, 1.2)
    crate(pl, M, tc.x + 1.5, tc.y + 0.9, 0.5, 0.4)
    puff(s, (tc.x + 1.5, tc.y + 0.9, 0.38), 0.1, BRASS, M, segs=4, h=0.12)
    puff(pl.g, (tc.x + 1.5, tc.y + 0.9, 0.5), 0.12, Emit(WARM), M, segs=4, h=0.2)
    pl.lamp(M, (tc.x + 1.5, tc.y + 0.9, 0.8), 3.5, 0.6)
    for k in range(3):
        a = deg(215 + 22 * k)
        r = 2.3
        box(s, (-0.35, -0.3, 0), (0.35, 0.3, 0.35 + 0.35 * k), STONE, T(tc.x + math.cos(a) * r,
                                                                         tc.y + math.sin(a) * r, 0, rz=a),
            skip=("-z",))
    pl.mark("telescope")
    for k, (x, y, sz, rz, tilt) in enumerate(((4.4, 1.6, 0.8, 0.3, 0.2), (5.6, -0.6, 0.6, 1.1, -0.3),
                                              (-4.6, 1.2, 0.9, 0.7, 0.25), (-4.2, 3.0, 0.5, 0.2, 0.0),
                                              (3.0, 3.6, 0.55, 0.9, 0.35), (-5.8, -2.6, 0.7, 0.4, -0.2),
                                              (1.8, -5.6, 0.6, 1.4, 0.3))):
        B = T(x, y, sz * 0.3, rz=rz) @ Matrix.Rotation(tilt, 4, "X")
        box(s, (-sz, -sz * 0.7, -sz * 0.5), (sz, sz * 0.7, sz * 0.5), mossy, B, skip=(), jit=0.1)
        pl.solid(M, x, y, sz * 1.1)
    for k, (x, y, rz) in enumerate(((3.6, -3.4, 0.4), (-2.8, 4.6, -0.8))):
        Cd = T(x, y, 0.55, rz=rz) @ Matrix.Rotation(math.pi / 2, 4, "Y")
        cylinder(s, 0.55, 1.4, 8, mossy, Cd @ T(0, 0, -0.7), bottom=True)
        pl.solid(M, x, y, 0.9)
    Ar = T(6.4, -4.6, 0, rz=0.5)
    for sx in (-1, 1):
        box(s, (sx * 1.4 - 0.45, -0.45, 0), (sx * 1.4 + 0.45, 0.45, 2.8 if sx < 0 else 1.6), mossy, Ar,
            skip=("-z",))
    sweep(s, [(-1.4, 0, 2.7), (-0.9, 0, 3.5), (-0.1, 0, 3.8)], 0.42, 4, mossy, Ar, smooth=False, caps=True,
          twist=math.pi / 4)
    pl.solid(Ar, -1.3, 0, 0.6)
    pl.solid(Ar, 1.3, 0, 0.6)
    for x, y, sd in ((-3.2, -3.4, 1), (4.2, -1.8, 2), (-1.4, 3.6, 3), (2.4, 5.4, 4)):
        flower_clump(pl, M, x, y, 0.5, seed=sd)
    pl.mark("rubble")
    pl.flat = 12.0
    pl.terrain_size = 30
    pl.view = dict(radius=11.0, target=(0.3, -0.8, 2.4), azimuth=24.0, elevation=36.0)


# ------------------------------------------------------------------------------ 1.0 (D90): shared
BASALT = (0.34, 0.30, 0.33)
BASALT_LIGHT = (0.50, 0.44, 0.45)
ASH_STONE = (0.66, 0.60, 0.56)
LAVA_CRUST = (0.34, 0.12, 0.08)
LAVA_GLOW = (1.0, 0.44, 0.12)
EMBER = (1.0, 0.62, 0.22)
BRAZIER_LIGHT = {"day": 0.2, "evening": 0.7, "night": 1.0}  # braziers' and lava's light on the stone near them


def on_ground(pl, x, y, rz=0.0, s=None, sink=0.0):
    """A frame at (x, y) standing on the landscape's ground (sunk `sink` into it)."""
    return T(x, y, pl.gz(x, y) - sink, rz=rz, s=s)


def land_ao(pl, radius, step=4.0):
    """The landscape round the place (its bowl's walls, its beach) as occluders for ambient
    occlusion only, in place of the flat ground plane; quads in its frame."""
    n = int(radius / step)
    pts = {(i, j): (i * step, j * step, pl.gz(i * step, j * step)) for j in range(-n, n + 1) for i in range(-n, n + 1)}
    pl.ground = []
    pl.ao_ground = [(pts[i, j], pts[i + 1, j], pts[i + 1, j + 1], pts[i, j + 1]) for j in range(-n, n) for i in range(-n, n)]


def pebble(pl, M, x, y, r, col=STONE, h=None, seed=0.0, segs=5, part=None):
    """A low round stone, its foot in the ground (3 x segs triangles)."""
    hh = h or r * 0.55
    lathe(part or pl.s, [(r, -0.12), (r * 0.8, hh * 0.75), (0, hh)], segs, col, M @ T(x, y, 0, rz=seed), lump=0.12,
          seed=seed)


def bead(part, M, p, r, col, h=None, segs=4):
    """A little double pyramid round p (a float, a paper lantern), facing out."""
    hh = r * 1.2 if h is None else h
    lathe(part, [(0.0, -hh), (r, 0.0), (0.0, hh)], segs, col, M @ T(*p), smooth=False, jit=0.03)


def flame(pl, M, h=0.9, r=0.3, table=GLOW, rgb=EMBER):
    """A fire's flames (glow part): two crossed tongues fading to their tips over a glowing bed."""
    base = M @ Vector((0, 0, 0))
    fire = Emit(rgb, table, fade=lambda p, c=base, hh=h: max(0.0, 1 - (p.z - c.z) / hh))
    for a in (0.0, math.pi / 2):
        face(pl.g, [(-r, 0, 0.0), (r, 0, 0.0), (0, 0, h)], (0, 1, 0), fire, M @ T(0, 0, 0, rz=a))
    disc(pl.g, r * 1.5, 6, Emit((1.0, 0.45, 0.15), table, fade=lambda p: 0.8), M @ T(0, 0, 0.04), jit=0)


def flame_outline(h=1.0):
    """A flame's outline in XZ (the battle league's emblem), counter-clockwise from its foot."""
    pts = [(0.0, 0.0), (0.34, 0.1), (0.44, 0.4), (0.3, 0.72), (0.16, 0.56), (0.02, 1.0), (-0.18, 0.64),
           (-0.34, 0.76), (-0.44, 0.38), (-0.3, 0.1)]
    return [(x * h, z * h) for x, z in pts]


def crescent_outline(r=0.4):
    """A crescent moon in XZ (the pageant's emblem), counter-clockwise."""
    outer = [(math.cos(a) * r, math.sin(a) * r) for a in (math.radians(d) for d in range(60, 330, 30))]
    inner = [(0.3 * r + math.cos(a) * r * 0.78, 0.12 * r + math.sin(a) * r * 0.78)
             for a in (math.radians(d) for d in range(300, 60, -30))]
    return outer + inner


def board_sign(pl, M, w, h_top, col, emblem, emblem_col, paper=CLOTH_CREAM, post=DARKWOOD):
    """A notice board for a league (its sign): two posts at its ends, the board, a paper panel and
    an emblem on top, facing +Y. The feature that owns it stands an invisible spot there."""
    s = pl.s
    sign_posts(pl, M, w / 2, h_top + 0.25, post)
    box(s, (-w / 2, -0.07, h_top - 1.2), (w / 2, 0.07, h_top), col, M, skip=())
    face(s, [(-w / 2 + 0.12, 0.075, h_top - 1.08), (w / 2 - 0.12, 0.075, h_top - 1.08), (w / 2 - 0.12, 0.075, h_top - 0.12),
             (-w / 2 + 0.12, 0.075, h_top - 0.12)], (0, 1, 0), paper, M)
    for k in range(4):  # the four leagues' marks, Ember to Starfire
        c = ((0.96, 0.56, 0.26), (0.92, 0.36, 0.26), (0.80, 0.26, 0.40), (0.98, 0.84, 0.36))[k]
        x = -w / 2 + 0.36 + (w - 0.72) * k / 3
        face(s, [(x - 0.1, 0.08, h_top - 0.95), (x + 0.1, 0.08, h_top - 0.95), (x + 0.1, 0.08, h_top - 0.75),
                 (x - 0.1, 0.08, h_top - 0.75)], (0, 1, 0), c, M)
    prism(s, emblem, 0.1, emblem_col, M @ T(0, -0.05, h_top - 0.05), back=True, sides=False)
    pl.solid(M, 0, 0, w * 0.55)


# ---------------------------------------------------------------------- Emberpeak Caldera
def lava_crack(pl, pts, width=0.55):
    """A crack across the crater's floor with lava in it: a dark crust strip and a glowing seam
    over it (both a hand above the floor; tapered at the ends)."""
    P = [Vector((x, y, 0.0)) for x, y in pts]
    for part, w, z, col in ((pl.s, width, 0.05, LAVA_CRUST), (pl.g, width * 0.5, 0.08, Emit(LAVA_GLOW, LAVA))):
        L, R = [], []
        for i, p in enumerate(P):
            t = (P[min(i + 1, len(P) - 1)] - P[max(i - 1, 0)]).normalized()
            side = Vector((-t.y, t.x, 0)) * (w / 2) * (0.3 if i in (0, len(P) - 1) else 1.0)
            L.append(p + side + Vector((0, 0, z)))
            R.append(p - side + Vector((0, 0, z)))
        grid(part, [R, L], col, I4, smooth=False, jit=0.04)


def lava_pool(pl, x, y, r, segs=8, seed=0.0):
    """A pool of lava in a basalt rim, glowing hottest in its middle, lighting the stone near it."""
    W = T(x, y, 0, rz=seed)
    lathe(pl.s, [(r + 0.6, -0.25), (r + 0.4, 0.22), (r, 0.1)], segs, BASALT, W, lump=0.14, seed=seed)
    disc(pl.s, r * 1.02, segs, LAVA_CRUST, W, z=0.06)
    c = W @ Vector((0, 0, 0))
    disc(pl.g, r, segs, Emit(LAVA_GLOW, LAVA, fade=lambda p, c=c, r=r: 0.55 + 0.45 * max(0.0, 1 - (p - c).length / r)),
         W, z=0.09, rings=[r, r * 0.5], jit=0)
    pl.lamp(W, (0, 0, 0.9), r * 2.5 + 2.5, 0.8, rgb=EMBER, table=BRAZIER_LIGHT)
    pl.solid(W, 0, 0, r + 0.5)


@place("caldera", "Emberpeak Caldera")
def build_caldera(pl):
    """The battle league's grand stage on the crater's flat floor (radius ~30 m; the inner walls
    rise steeply behind it to the rim; the way in is the rim's gap at +Y, the path ending ~29 m
    out). The ring in the middle, the champion's dais behind it, terraces for a crowd against the
    far wall, basalt pillars with braziers, lava in cracks and pools at the sides."""
    M = I4
    s = pl.s
    deg = math.radians
    # The battle ring: a round stone floor a low step up, a dark inlay round it, the league's flame.
    RR, RH = 7.0, 0.25
    lathe(s, [(RR + 0.35, -0.3), (RR + 0.35, RH - 0.1), (RR + 0.1, RH), (0, RH)], 16,
          lambda p, n: STONE_WARM if n.z > 0.7 else (0.64, 0.56, 0.50), M, sharp=[1, 2], jit=0.06)
    lathe(s, [(5.7, RH + 0.006), (5.3, RH + 0.006)], 16, (0.66, 0.30, 0.24), M, smooth=False, jit=0.03)
    face(s, [(x, z - 0.8, RH + 0.01) for x, z in flame_outline(1.6)], (0, 0, 1), GOLD, M)
    for sx in (-1, 1):  # where the two trainers stand, just off the ring's ends
        lathe(s, [(1.0, -0.2), (1.0, 0.04), (0.0, 0.06)], 7, BASALT_LIGHT, T(sx * 8.9, 0, 0), sharp=[1], smooth=False)
    pl.extra["anchors"] = {"ring": [0.0, 0.0, RH], "sides": [[-8.9, 0.0], [8.9, 0.0]]}
    pl.mark("ring")
    # Basalt pillars at the ring's corners, a brazier on each (always alight).
    for k, a in enumerate((deg(40), deg(140), deg(220), deg(320))):
        W = T(math.cos(a) * 10.6, math.sin(a) * 10.6, 0, rz=0.4 * k)
        lathe(s, [(0.95, -0.3), (0.84, 3.9), (1.05, 4.1), (1.05, 4.3)], 6,
              lambda p, n: BASALT_LIGHT if n.z > 0.6 else BASALT, W, sharp=[1, 2], smooth=False, jit=0.08)
        lathe(s, [(0.4, 4.3), (0.95, 4.75), (0.82, 4.82), (0.0, 4.6)], 6, IRON, W, sharp=[1, 2], smooth=False)
        flame(pl, W @ T(0, 0, 4.62), h=1.3, r=0.42, table=LAVA)
        pl.lamp(W, (0, 0, 5.2), 8.5, 0.9, rgb=EMBER, table=BRAZIER_LIGHT)
        pl.solid(W, 0, 0, 1.2)
    pl.mark("pillars")
    # Terraces for the crowd against the far wall (stepping up into it).
    steps = [(21.0, -0.4), (21.0, 0.45), (23.0, 0.45), (23.0, 0.9), (25.0, 0.9), (25.0, 1.35), (27.0, 1.35),
             (27.0, 1.8), (29.5, 1.8), (29.5, -0.4)]

    def terrace(p, n):  # warm stone seats over dark basalt risers (they read as steps against the wall)
        return (0.80, 0.68, 0.56) if n.z > 0.5 else (BASALT_LIGHT if n.x * p.x + n.y * p.y < 0 else BASALT)
    lathe(s, steps, 10, terrace, M, sharp=list(range(1, 9)), arc=(deg(222), deg(318)), caps=True, jit=0.07)
    for k in range(13):
        a = deg(224) + (deg(316) - deg(224)) * k / 12
        pl.solid(M, math.cos(a) * 22.6, math.sin(a) * 22.6, 1.7)
    for k, a in enumerate((deg(240), deg(270), deg(300))):  # pennants on the top tier
        x, y = math.cos(a) * 28.2, math.sin(a) * 28.2
        cylinder(s, 0.07, 3.4, 4, DARKWOOD, T(x, y, 1.7), smooth=False)
        puff(s, (x, y, 5.1), 0.13, GOLD, M, segs=4, h=0.2)
        tang = Vector((-math.sin(a), math.cos(a), 0))
        tip = Vector((x, y, 0)) + tang * 1.4
        face(s, [(x, y, 5.0), (x, y, 4.1), (tip.x, tip.y, 4.55)], Vector((math.cos(a), math.sin(a), 0)),
             (CLOTH_RED, (0.98, 0.62, 0.22), CLOTH_YELLOW)[k], M, double=True)
    pl.mark("terraces")
    # The champion's dais behind the ring: stepped stone, a throne with the league's flame.
    D = T(0, -14.0, 0)
    lathe(s, [(3.4, -0.3), (3.4, 0.3), (2.6, 0.3), (2.6, 0.62), (1.8, 0.62), (1.8, 0.94), (0, 0.94)], 8,
          lambda p, n: ASH_STONE if n.z > 0.5 else BASALT_LIGHT, D, sharp=[1, 2, 3, 4, 5], smooth=False,
          a0=math.pi / 8)
    box(s, (-0.6, -0.45, 0.94), (0.6, 0.35, 1.38), (0.62, 0.30, 0.26), D, skip=("-z",))
    box(s, (-0.66, -0.62, 0.94), (0.66, -0.42, 2.7), BASALT_LIGHT, D, skip=("-z",))
    for sx in (-1, 1):
        box(s, (sx * 0.62 - 0.12, -0.45, 1.38), (sx * 0.62 + 0.12, 0.3, 1.72), BASALT_LIGHT, D, skip=("-z",))
    prism(s, flame_outline(0.9), 0.08, GOLD, D @ T(0, -0.41, 1.72), back=False, sides=False)
    pl.solid(D, 0, 0, 3.5)
    pl.mark("dais")
    # The banner arch at the way in, the league's flame on top.
    E = T(0, 25.5, 0)
    for sx in (-1, 1):
        box(s, (sx * 4.8 - 0.55, -0.55, -0.4), (sx * 4.8 + 0.55, 0.55, 5.2), BASALT, E, skip=("-z",))
        box(s, (sx * 4.8 - 0.7, -0.7, 5.2), (sx * 4.8 + 0.7, 0.7, 5.6), BASALT_LIGHT, E, skip=("-z",))
        pl.solid(E, sx * 4.8, 0, 0.95)
    sweep(s, [(-4.8 + 9.6 * k / 6, 0, 5.45 + 1.5 * math.sin(math.pi * k / 6)) for k in range(7)], 0.36, 5, DARKWOOD,
          E, caps=True)
    prism(s, flame_outline(1.3), 0.14, GOLD, E @ T(0, -0.07, 6.85), back=True, sides=False)
    for sx, c in ((-2.5, CLOTH_RED), (2.5, (0.98, 0.62, 0.22))):
        face(s, [(sx - 0.62, 0.02, 6.15), (sx + 0.62, 0.02, 6.15), (sx + 0.62, 0.02, 4.0), (sx, 0.02, 3.5),
                 (sx - 0.62, 0.02, 4.0)], (0, 1, 0), c, E, double=True)
    pl.mark("arch")
    # The league's board beside the way in, turned to the path.
    B = T(-7.2, 20.8, 0, rz=-0.5)
    board_sign(pl, B, 2.2, 2.35, DARKWOOD, flame_outline(0.6), GOLD, post=BASALT)
    pl.extra["anchors"]["board"] = world(B, (0, 0, 0))[:2]
    pl.mark("board")
    # Lava at the sides (never on the ring or the way in), boulders by the walls.
    for pts in (((-13.5, 6.0), (-15.2, 8.2), (-14.6, 10.6), (-16.8, 12.8), (-18.6, 12.2)),
                ((-12.8, -7.0), (-15.0, -8.4), (-17.6, -7.6), (-19.4, -9.8)),
                ((13.0, 9.4), (15.2, 8.2), (17.4, 9.8), (19.8, 9.0), (21.4, 11.2)),
                ((14.0, -9.6), (15.8, -12.0), (18.4, -12.6)),
                ((-8.6, 15.8), (-11.0, 17.6), (-11.6, 20.2))):
        lava_crack(pl, pts)
    lava_pool(pl, -19.6, 3.2, 2.1, seed=0.4)
    lava_pool(pl, 20.2, -2.6, 1.6, seed=1.3)
    pl.mark("lava")
    for k, (x, y, r) in enumerate(((-24.0, -10.0, 1.4), (24.6, 6.4, 1.2), (-22.6, 14.6, 1.0), (21.0, 16.6, 1.1),
                                   (8.6, 22.8, 0.8))):
        rock(pl, M, x, y, r, BASALT, (1.2, 1.0, 0.8), seed=k + 2, segs=6, rings=3, moss=ASH_STONE)
        pl.solid(M, x, y, r * 1.1)
    pl.mark("rocks")
    pl.flat = 30.0
    land_ao(pl, 56.0)
    pl.terrain = "land"
    pl.terrain_size = 58
    pl.view = dict(radius=30.0, target=(0, -3.0, 1.5), azimuth=20.0, elevation=42.0)


# ---------------------------------------------------------------------- Moonpetal Glade
PETALS = ((0.70, 0.84, 1.0), (0.80, 0.70, 1.0), (1.0, 0.74, 0.88))          # soft blue, violet, pink
PETAL_GLOW = ((0.24, 0.50, 1.0), (0.50, 0.28, 1.0), (1.0, 0.32, 0.66))      # (added over them: keep them coloured)
LEAF_NIGHT = (0.26, 0.50, 0.42)
WILLOW = (0.44, 0.64, 0.46)


def moonpetals(pl, M, heads=3, seed=0, r=0.5):
    """A clump of moonpetals: pale flowers on stems over a star of leaves, glowing softly by day
    and brightly at night."""
    s = pl.s
    leaf = []
    for k in range(8):
        a = seed * 0.9 + math.pi * k / 4
        rr = r * (1.0 if k % 2 == 0 else 0.34)
        leaf.append((math.cos(a) * rr, math.sin(a) * rr, 0.05))
    face(s, leaf, (0, 0, 1), LEAF_NIGHT, M, jit=0.08)
    for k in range(heads):
        a = seed * 1.7 + k * 2.1
        c = (seed + k) % 3
        hx, hy, hz = math.cos(a) * r * 0.42, math.sin(a) * r * 0.42, 0.34 + 0.12 * (k % 2)
        face(s, [(hx - 0.025, hy, 0.05), (hx + 0.025, hy, 0.05), (hx, hy, hz)], (math.cos(a), math.sin(a), 0),
             LEAF_NIGHT, M, double=True)
        puff(s, (hx, hy, hz - 0.04), 0.13, PETALS[c], M, segs=4, h=0.1)
        puff(pl.g, (hx, hy, hz - 0.09), 0.34, Emit(PETAL_GLOW[c], MOONPETAL), M, segs=4, h=0.26)
    pl.lamp(M, (0, 0, 0.5), 2.8, 0.35, rgb=PETAL_GLOW[seed % 3], table=MOONPETAL)


def paper_lantern(pl, p, r=0.16, col=CLOTH_CREAM, glow=(1.0, 0.72, 0.40)):
    """A little paper lantern hanging at p: two pyramids (solid) in a glowing shell."""
    bead(pl.s, I4, p, r, col)
    bead(pl.g, I4, p, r * 1.35, Emit(glow, GLOW), h=r * 1.9)
    pl.lamp(I4, p, 3.0, 0.35)


def lantern_string(pl, a, b, sag, count):
    a, b = V(a), V(b)

    def at(t):
        return a.lerp(b, t) - Vector((0, 0, sag * 4 * t * (1 - t)))
    ribbon(pl.s, [at(t / 3) for t in range(4)], 0.04, DARKWOOD, I4, normal_hint=(0, 0, 1))
    for k in range(count):
        paper_lantern(pl, at((k + 0.5) / count) - Vector((0, 0, 0.22)))


def willow(pl, M, h=6.0, r=2.8, seed=0.0):
    """A simple weeping willow: a leaning trunk, a soft canopy, fronds hanging from its rim."""
    s = pl.s
    cylinder(s, 0.3, h * 0.58, 5, (0.46, 0.36, 0.28), M @ T(0, 0, -0.3), r_top=0.2, top=False)
    blob(s, r, r * 0.92, h * 0.26, 7, 4, WILLOW, M @ T(0, 0, h * 0.64), lump=0.16, seed=seed, zcut=-h * 0.1)
    for k in range(9):
        a = seed + k * 2 * math.pi / 9
        d = Vector((math.cos(a), math.sin(a), 0))
        top = d * r * 0.86 + Vector((0, 0, h * 0.6))
        mid = d * r * 1.0 + Vector((0, 0, h * 0.34))
        bot = d * r * 1.04 + Vector((0, 0, h * (0.06 + 0.05 * (k % 3))))
        ribbon(s, [M @ top, M @ mid, M @ bot], 0.62, (0.36, 0.56, 0.38), I4, normal_hint=tuple(d))
    pl.solid(M, 0, 0, 0.6)


def glade_stall(pl, M, cols, goods):
    """A small stall for the pageant's wares: a counter, four posts, a striped awning."""
    s = pl.s
    w, d, ch = 2.6, 1.2, 0.95
    box(s, (-w / 2, -d / 2, -0.2), (w / 2, d / 2, ch), WOOD, M, skip=("-z",))
    box(s, (-w / 2 - 0.06, -d / 2 - 0.04, ch), (w / 2 + 0.06, d / 2 + 0.08, ch + 0.07), LIGHTWOOD, M, skip=("-z",))
    for sx in (-1, 1):
        for y, top in ((d / 2 + 0.02, 2.3), (-d / 2 - 0.2, 2.7)):
            box(s, (sx * w / 2 - 0.07, y - 0.07, -0.2), (sx * w / 2 + 0.07, y + 0.07, top), TIMBER, M, skip=("-z", "+z"))
    awning(pl, M, w + 0.4, d + 0.8, 2.8, 2.28, 4, cols, sag=0.14, valance=0.24)
    goods(M @ T(0, 0.1, ch + 0.07))
    pl.solid(M, -w / 4, 0, d * 0.7)
    pl.solid(M, w / 4, 0, d * 0.7)


def accessory_goods(pl, M):
    s = pl.s
    # a little pointed hat, a bow, a flower crown, a scarf over the edge
    lathe(s, [(0.2, 0.0), (0.07, 0.28), (0.0, 0.36)], 5, CLOTH_LILAC, M @ T(-0.8, 0, 0.02), smooth=False)
    disc(s, 0.28, 5, CLOTH_LILAC, M @ T(-0.8, 0, 0.02), jit=0.02)
    for sx in (-1, 1):
        face(s, [(0.0, 0.0, 0.12), (sx * 0.2, 0.0, 0.24), (sx * 0.2, 0.0, 0.02)], (0, 1, 0), CLOTH_PINK,
             M @ T(-0.15, 0.1, 0.0), double=True)
    for k in range(3):  # a flower crown's blooms
        a = 2 * math.pi * k / 3
        puff(s, (0.45 + math.cos(a) * 0.17, math.sin(a) * 0.17, 0.02), 0.09, PETALS[k], M, segs=4, h=0.07)
    ribbon(s, [(0.95, 0.2, 0.01), (1.0, 0.45, 0.0), (1.02, 0.62, -0.35)], 0.22, CLOTH_TEAL, M, normal_hint=(1, 0, 0.3))


def dye_goods(pl, M):
    s = pl.s
    for k, c in enumerate(((0.90, 0.34, 0.40), (0.40, 0.56, 0.92), (0.98, 0.80, 0.30), (0.52, 0.78, 0.44))):
        x = -0.9 + 0.6 * k
        cylinder(s, 0.16, 0.26, 5, lambda p, n, c=c: c if n.z > 0.5 else (0.78, 0.66, 0.54), M @ T(x, -0.05, 0),
                 r_top=0.18, smooth=False)


@place("glade", "Moonpetal Glade")
def build_glade(pl):
    """The pageant's hall: a night garden in the west woods (flat radius ~30 m; the path arrives
    at +Y, ~28 m out). A round wooden stage under a flowered arch at the back, benches before it,
    the judges' table at its side, the accessory and dye stalls by the way in, strings of paper
    lanterns, a willow, moonpetals glowing everywhere (softly by day, brightly at night)."""
    M = I4
    s = pl.s
    SC = Vector((0.0, -8.0, 0.0))
    SR, SH = 4.6, 0.55
    St = on_ground(pl, SC.x, SC.y)
    zs = pl.gz(SC.x, SC.y)

    def stage_col(p, n):
        return LIGHTWOOD if n.z > 0.7 else WOOD
    lathe(s, [(SR, -0.35), (SR, SH - 0.08), (SR - 0.12, SH), (0, SH)], 14, stage_col, St, sharp=[1, 2], jit=0.05)
    for k in (-3, -1.5, 0, 1.5, 3):  # boards across its top
        half = math.sqrt(SR * SR - k * k) - 0.3
        face(s, [(k - 0.03, -half, SH + 0.004), (k + 0.03, -half, SH + 0.004), (k + 0.03, half, SH + 0.004),
                 (k - 0.03, half, SH + 0.004)], (0, 0, 1), (0.60, 0.42, 0.26), St, jit=0.0)
    for y0, y1, top in ((SR - 0.3, SR + 0.62, 0.19), (SR - 0.4, SR + 0.2, 0.37)):  # steps up at its front
        box(s, (-1.3, y0, -0.3), (1.3, y1, top), LIGHTWOOD, St, skip=("-z",))
    pl.solid(St, 0, 0, SR - 0.3)
    anchors = {"stage": [SC.x, SC.y, round(zs + SH, 3)],
               "rivals": [[round(SC.x + x, 2), round(SC.y + y, 2)] for x, y in ((-3.0, 0.6), (-1.0, 1.2), (1.0, 1.2),
                                                                                  (3.0, 0.6))]}
    pl.mark("stage")
    # The flowered arch at the stage's back.
    A = St @ T(0, -SR + 1.1, SH)
    for sx in (-1, 1):
        box(s, (sx * 2.6 - 0.1, -0.1, 0), (sx * 2.6 + 0.1, 0.1, 2.9), LIGHTWOOD, A, skip=("-z", "+z"))
    arc = [Vector((-2.6 + 5.2 * k / 6, 0, 2.9 + 1.3 * math.sin(math.pi * k / 6))) for k in range(7)]
    sweep(s, arc, 0.13, 4, LIGHTWOOD, A, twist=math.pi / 4)
    for k in range(7):
        t = (k + 0.5) / 7
        p = Vector((-2.6 + 5.2 * t, 0.12 * (1 if k % 2 else -1), 2.9 + 1.3 * math.sin(math.pi * t) + 0.12))
        c = k % 3
        puff(s, p, 0.2, PETALS[c], A, segs=4, h=0.14)
        puff(pl.g, p - Vector((0, 0, 0.05)), 0.42, Emit(PETAL_GLOW[c], MOONPETAL), A, segs=4, h=0.3)
    pl.lamp(A, (0, 0.8, 3.4), 6.0, 0.45, rgb=(0.62, 0.62, 1.0), table=MOONPETAL)
    for sx in (-1, 1):  # vines up the posts
        ribbon(s, [(sx * 2.6, 0.12, 0.1), (sx * 2.5, 0.13, 1.4), (sx * 2.7, 0.12, 2.8)], 0.2, LEAF_NIGHT, A,
               normal_hint=(0, 1, 0))
    pl.mark("arch")
    # Benches for the audience, an aisle down the middle.
    for y in (-0.6, 1.8, 4.2):
        for sx in (-1, 1):
            Bn = on_ground(pl, sx * 2.7, y)
            box(s, (-1.2, -0.22, 0.38), (1.2, 0.22, 0.48), WOOD, Bn, skip=("-z",))
            box(s, (-0.9, -0.14, -0.2), (0.9, 0.14, 0.38), DARKWOOD, Bn, skip=("-z", "+z"))
            pl.solid(Bn, -0.6, 0, 0.6)
            pl.solid(Bn, 0.6, 0, 0.6)
    pl.mark("benches")
    # The judges' table beside the stage, turned to it; three stools behind it.
    jx, jy = 7.4, -2.8
    J = on_ground(pl, jx, jy, rz=math.atan2(-(SC.x - jx), SC.y - jy))  # its front (+Y) to the stage
    box(s, (-1.3, -0.4, 0.74), (1.3, 0.4, 0.84), LIGHTWOOD, J, skip=())
    face(s, [(1.28, 0.42, 0.2), (-1.28, 0.42, 0.2), (-1.28, 0.42, 0.8), (1.28, 0.42, 0.8)], (0, 1, 0),
         (0.52, 0.40, 0.76), J, jit=0.02)
    for lx in (-1.15, 1.15):
        box(s, (lx - 0.06, -0.3, -0.2), (lx + 0.06, 0.3, 0.74), DARKWOOD, J, skip=("-z", "+z"))
    for lx in (-0.8, 0.0, 0.8):
        cylinder(s, 0.22, 0.46, 4, WOOD, J @ T(lx, -0.85, -0.1), smooth=False, a0=math.pi / 4)
    moonpetals(pl, J @ T(0.0, 0.0, 0.8), heads=2, seed=4, r=0.22)
    anchors["judges"] = world(J, (0, 0, 0))[:2]
    pl.solid(J, 0, -0.3, 1.4)
    pl.mark("judges")
    # The accessory and dye stalls by the way in, facing each other across the approach.
    stalls = []
    for x, rz, cols, goods in ((-9.6, -math.pi / 2, (CLOTH_LILAC, CLOTH_CREAM), accessory_goods),
                               (9.6, math.pi / 2, (CLOTH_TEAL, CLOTH_CREAM), dye_goods)):
        W = on_ground(pl, x, 8.0, rz=rz)
        glade_stall(pl, W, cols, lambda G, goods=goods: goods(pl, G))
        front = world(W, (0, 1.2, 0))
        stalls.append([front[0], front[1], round(rz, 3)])
    anchors["stalls"] = stalls
    pl.mark("stalls")
    # Paper lanterns strung over the benches and on to the arch.
    posts = [(-6.4, 1.0), (6.4, 1.0)]
    for x, y in posts:
        P = on_ground(pl, x, y)
        box(s, (-0.09, -0.09, -0.2), (0.09, 0.09, 4.1), DARKWOOD, P, skip=("-z",))
        puff(s, (0, 0, 4.1), 0.12, GOLD, P, segs=4, h=0.16)
        pl.solid(P, 0, 0, 0.3)
    za, zb = pl.gz(*posts[0]) + 3.95, pl.gz(*posts[1]) + 3.95
    lantern_string(pl, (posts[0][0], posts[0][1], za), (posts[1][0], posts[1][1], zb), 0.7, 4)
    arch_top = A @ Vector((2.6, 0, 2.9))
    lantern_string(pl, (posts[1][0], posts[1][1], zb), tuple(arch_top), 0.6, 3)
    pl.mark("lanterns")
    willow(pl, on_ground(pl, -12.5, -7.0, sink=0.1), seed=0.7)
    pl.mark("willow")
    # Moonpetals: round the stage, along the approach, under the willow.
    for k, (x, y, h) in enumerate(((-5.4, -6.2, 3), (5.2, -5.6, 3), (-4.4, -11.6, 2), (4.6, -11.2, 2), (-2.2, 11.8, 3),
                                   (2.4, 14.6, 2), (-11.0, -3.2, 3), (11.6, 1.6, 2), (-7.0, 12.6, 2), (12.0, -8.4, 3))):
        moonpetals(pl, on_ground(pl, x, y, rz=k * 0.7), heads=h, seed=k)
    pl.mark("moonpetals")
    # Stepping stones in from where the path ends.
    for i, (x, y, r) in enumerate(((0.4, 25.2, 0.6), (-0.3, 22.0, 0.55), (0.3, 18.8, 0.6), (-0.3, 15.6, 0.55),
                                   (0.2, 12.4, 0.6))):
        pebble(pl, on_ground(pl, x, y), 0, 0, r, (0.74, 0.74, 0.76), h=0.14, seed=i, segs=4)
    pl.mark("stones")
    Bd = on_ground(pl, 4.4, 18.2, rz=0.64)
    board_sign(pl, Bd, 2.0, 2.2, (0.52, 0.38, 0.62), crescent_outline(0.4), (0.98, 0.90, 0.62))
    anchors["board"] = world(Bd, (0, 0, 0))[:2]
    pl.extra["anchors"] = anchors
    pl.mark("board")
    pl.flat = 30.0
    land_ao(pl, 40.0)
    pl.terrain = "land"
    pl.terrain_size = 34
    pl.view = dict(radius=18.0, target=(0, -1.0, 1.2), azimuth=24.0, elevation=38.0)


# ---------------------------------------------------------------------- Driftwood Cove
DRIFTWOOD = (0.76, 0.70, 0.62)
SHACK = (0.62, 0.50, 0.38)
NET = (0.72, 0.64, 0.48)


def driftwood(pl, M, length, r, seed=0.0):
    """A bleached log lying on the sand along local X, a broken branch off it."""
    s = pl.s
    sweep(s, [(-length / 2, 0, r * 0.8), (0, 0.08, r * 0.95), (length / 2, 0, r * 0.7)], r, 5, DRIFTWOOD, M,
          radii=[r * 0.9, r, r * 0.75], caps=True, twist=seed)
    sweep(s, [(length * 0.15, 0, r * 1.2), (length * 0.28, 0.3, r * 2.4)], r * 0.35, 4, DRIFTWOOD, M, smooth=False)
    pl.solid(M, -length / 4, 0, r + 0.3)
    pl.solid(M, length / 4, 0, r + 0.3)


def rowboat(pl, B):
    """A little rowboat (as Mirror Lake's), its bow toward local +Y."""
    s = pl.s
    st = [(-1.9, 0.06, 0.62), (-1.2, 0.5, 0.5), (0.0, 0.74, 0.45), (1.2, 0.58, 0.5), (1.9, 0.14, 0.64)]
    outer, inner = [], []
    for y, hw, zt in st:
        prof = [(-hw, zt), (-hw * 0.8, 0.16), (0, -0.04), (hw * 0.8, 0.16), (hw, zt)]
        outer.append([Vector((x, y, z)) for x, z in prof])
        inner.append([Vector((x * 0.86, y * 0.95, z + 0.07)) for x, z in prof])
    grid(s, outer, (0.86, 0.40, 0.34), B, smooth=True, flip=True)
    grid(s, inner, (0.56, 0.40, 0.26), B, smooth=True)
    for side in (0, 4):
        for i in range(len(st) - 1):
            face(s, [outer[i][side], outer[i + 1][side], inner[i + 1][side], inner[i][side]], (0, 0, 1),
                 (0.94, 0.90, 0.80), B)
    box(s, (-0.5, -0.14, 0.3), (0.5, 0.14, 0.38), LIGHTWOOD, B, skip=("-z",))
    O = B @ T(0.25, -0.2, 0.42, rz=0.12)
    box(s, (-0.03, -1.3, -0.03), (0.03, 0.9, 0.03), WOOD, O, skip=())
    face(s, [(-0.12, -1.3, 0), (0.12, -1.3, 0), (0.1, -1.9, 0), (-0.1, -1.9, 0)], (0, 0, 1), WOOD, O, double=True)
    pl.solid(B, 0, -0.9, 0.9)
    pl.solid(B, 0, 0.9, 0.9)


@place("cove", "Driftwood Cove")
def build_cove(pl):
    """A sandy cove on Mirror Lake's south shore, facing the water (+Y; the water's edge ~43 m out,
    a low dune at ~30 m; the path arrives from the back left). The fisher's shack with its nets, a
    long jetty out over the water (you fish from its end), a rowboat drawn up on the sand, driftwood,
    a campfire, tide pools at the water's edge."""
    M = I4
    s = pl.s
    wz = pl.water_z()  # (-1.45 below the anchor)
    # The fisher's shack, its door to the water; nets on a rack beside it.
    Sh = on_ground(pl, -8.4, 17.2, rz=0.12, sink=0.05)
    w, d, hf, hb = 3.6, 3.0, 2.6, 2.1
    for pts, n in (([(-w / 2, d / 2, -0.3), (w / 2, d / 2, -0.3), (w / 2, d / 2, hf), (-w / 2, d / 2, hf)], (0, 1, 0)),
                   ([(w / 2, -d / 2, -0.3), (-w / 2, -d / 2, -0.3), (-w / 2, -d / 2, hb), (w / 2, -d / 2, hb)], (0, -1, 0)),
                   ([(w / 2, d / 2, -0.3), (w / 2, -d / 2, -0.3), (w / 2, -d / 2, hb), (w / 2, d / 2, hf)], (1, 0, 0)),
                   ([(-w / 2, -d / 2, -0.3), (-w / 2, d / 2, -0.3), (-w / 2, d / 2, hf), (-w / 2, -d / 2, hb)], (-1, 0, 0))):
        face(s, pts, n, wall_col(SHACK, 0.0, hf), Sh, jit=0.08)
    for x in (-w / 2 + 0.1, w / 2 - 0.1):  # corner boards
        face(s, [(x - 0.1, d / 2 + 0.02, -0.1), (x + 0.1, d / 2 + 0.02, -0.1), (x + 0.1, d / 2 + 0.02, hf),
                 (x - 0.1, d / 2 + 0.02, hf)], (0, 1, 0), DRIFTWOOD, Sh)
    rn = Vector((0, -(hf - hb), d)).normalized()  # the lean-to roof, over-hanging, high at the front
    ov, th = 0.45, 0.14
    y0, y1 = -d / 2 - ov, d / 2 + ov
    z0, z1 = hb - ov * (hf - hb) / d, hf + ov * (hf - hb) / d
    xs = (-w / 2 - 0.3, w / 2 + 0.3)
    face(s, [(xs[0], y0, z0 + th), (xs[1], y0, z0 + th), (xs[1], y1, z1 + th), (xs[0], y1, z1 + th)], rn,
         (0.46, 0.58, 0.62), Sh, jit=0.05)
    face(s, [(xs[0], y0, z0), (xs[0], y1, z1), (xs[1], y1, z1), (xs[1], y0, z0)], (0, 0, -1), (0.40, 0.34, 0.28), Sh)
    face(s, [(xs[0], y1, z1), (xs[0], y1, z1 + th), (xs[1], y1, z1 + th), (xs[1], y1, z1)], (0, 1, 0),
         (0.36, 0.46, 0.50), Sh)
    for sx in (-1, 1):
        x = xs[0] if sx < 0 else xs[1]
        face(s, [(x, y0, z0), (x, y1, z1), (x, y1, z1 + th), (x, y0, z0 + th)], (sx, 0, 0), (0.36, 0.46, 0.50), Sh)
    front = Sh @ T(0, d / 2, 0)
    door(pl, front, -0.55, 0.0, w=0.95, h=1.85, col=DOOR_TEAL, frame=DRIFTWOOD, round_top=False)
    window_square(pl, Sh @ T(0, -d / 2, 0, rz=math.pi), 0.2, 1.3, w=0.7, h=0.5, frame=DRIFTWOOD, shutters=DOOR_TEAL)
    window_square(pl, front, 0.95, 1.55, w=0.62, h=0.56, frame=DRIFTWOOD)
    for k, c in enumerate((CLOTH_RED, CLOTH_CREAM, CLOTH_RED)):  # floats hung by the door
        bead(s, front, (-1.35, 0.12, 1.9 - 0.32 * k), 0.12, c, h=0.13)
    pl.solid(Sh, 0, 0, 2.3)
    R = on_ground(pl, -12.7, 19.2, rz=1.2)  # the net rack
    for sx in (-1.2, 1.2):
        box(s, (sx - 0.07, -0.07, -0.25), (sx + 0.07, 0.07, 1.9), DRIFTWOOD, R, skip=("-z",))
    sweep(s, [(-1.4, 0, 1.85), (1.4, 0, 1.85)], 0.05, 4, DARKWOOD, R, smooth=False)
    rows = [[Vector((-1.15 + 2.3 * k / 3, 0.04 * (k % 2), z - 0.18 * math.sin(math.pi * k / 3) * (1.85 - z) / 1.2))
             for k in range(4)] for z in (1.82, 1.2, 0.62)]
    grid(s, rows, NET, R, smooth=True, flip=True, jit=0.06)
    grid(s, rows, tuple(c * 0.9 for c in NET), R, smooth=True, jit=0.06)
    for k in range(3):
        puff(s, (-0.8 + 0.8 * k, 0.05, 0.66), 0.09, (CLOTH_RED, CLOTH_CREAM, CLOTH_YELLOW)[k], R, segs=4, h=0.1)
    pl.solid(R, 0, 0, 1.3)
    for k, tilt in enumerate((0.25, 0.4)):  # rods leaning on the shack's side
        Rd = Sh @ T(w / 2 + 0.06 + 2.9 * math.sin(tilt), 0.6 + 0.35 * k, -0.1) @ Matrix.Rotation(-tilt, 4, "Y")
        box(s, (-0.025, -0.025, 0), (0.025, 0.025, 2.9), DARKWOOD, Rd, skip=("-z", "+z"))
    crate(pl, M @ T(0, 0, pl.gz(-5.8, 15.9)), -5.8, 15.9, 0.7, 0.3, fill=(0.56, 0.72, 0.80))
    barrel(pl, M @ T(0, 0, pl.gz(-6.0, 17.1) - 0.05), -6.0, 17.1, 0.34, 0.8)
    lamp_post(pl, on_ground(pl, -5.4, 19.9), 0, 0, h=2.0, rng=5.0)
    pl.extra["anchors"] = {"fisher": world(Sh, (0.3, d / 2 + 1.9, 0))[:2]}
    pl.mark("shack")
    # The jetty: from beyond the dune, sloping down, then level out over the water.
    J0, JB, J1 = 33.5, 41.0, 53.0
    zf, ze = pl.gz(0, J0) + 0.3, wz + 0.9

    def deck(y):
        return zf + (ze - zf) * min(1.0, max(0.0, (y - J0) / (JB - J0)))
    hw = 0.95
    n = int(round(J1 - J0))
    for i in range(n):
        ya, yb = J0 + (J1 - J0) * i / n, J0 + (J1 - J0) * (i + 1) / n - 0.04
        za, zb = deck(ya), deck(yb)
        face(s, [(-hw, ya, za), (hw, ya, za), (hw, yb, zb), (-hw, yb, zb)], (0, -(zb - za), yb - ya),
             LIGHTWOOD if i % 2 else tuple(c * 0.9 for c in LIGHTWOOD), M, jit=0.04)
    for sx in (-1, 1):
        for ya, yb in ((J0, JB), (JB, J1)):
            za, zb = deck(ya), deck(yb)
            pts = [(sx * hw, ya, za - 0.2), (sx * hw, yb, zb - 0.2), (sx * hw, yb, zb), (sx * hw, ya, za)]
            face(s, pts if sx > 0 else pts[::-1], (sx, 0, 0), WOOD, M)
    for ya, yb in ((J0, JB), (JB, J1)):
        za, zb = deck(ya), deck(yb)
        face(s, [(-hw, ya, za - 0.2), (-hw, yb, zb - 0.2), (hw, yb, zb - 0.2), (hw, ya, za - 0.2)], (0, 0, -1),
             DARKWOOD, M)
    face(s, [(-hw, J1, ze - 0.2), (hw, J1, ze - 0.2), (hw, J1, ze), (-hw, J1, ze)], (0, 1, 0), WOOD, M)
    for y in (37.5, 43.0, 48.0, 52.7):
        for sx in (-1, 1):
            zb = pl.gz(sx * hw, y) - 0.5
            cylinder(s, 0.12, deck(y) - zb + 0.12, 5, DARKWOOD, T(sx * (hw + 0.02), y, zb), top=True, smooth=False)
    lamp_post(pl, M @ T(0, 0, ze), -hw + 0.1, J1 - 0.4, h=1.4, rng=4.5)
    cylinder(s, 0.14, 0.34, 5, (0.62, 0.62, 0.66), T(hw - 0.25, J1 - 0.5, ze), r_top=0.18, top=True, smooth=False)
    # (walkers walk out along it: the game has it as a deck, core/place_layout addPlaceDecks)
    pl.extra["anchors"]["fish_spot"] = [0.2, round(J1 - 0.9, 2), round(ze, 3)]
    pl.extra["anchors"]["jetty"] = [0.0, round(J0 - 1.1, 2)]
    pl.mark("jetty")
    bx, by, brz = 8.4, 28.4, 0.5
    bow, stern = (bx - math.sin(brz) * 1.9, by + math.cos(brz) * 1.9), (bx + math.sin(brz) * 1.9, by - math.cos(brz) * 1.9)
    pitch = math.atan2(pl.gz(*bow) - pl.gz(*stern), 3.8)
    rowboat(pl, on_ground(pl, bx, by, rz=brz, sink=-0.02) @ Matrix.Rotation(pitch, 4, "X"))
    pl.mark("boat")
    for x, y, rz, length, r in ((6.0, 15.6, 0.9, 3.0, 0.26), (1.0, 16.2, -0.4, 3.2, 0.26), (-13.8, 1.6, 1.1, 3.8, 0.3),
                                (13.4, 22.4, -0.5, 2.8, 0.24)):
        driftwood(pl, on_ground(pl, x, y, rz=rz, sink=0.08), length, r, seed=x)
    pl.mark("driftwood")
    F = on_ground(pl, 3.4, 13.6)  # the campfire, the logs round it for seats
    for k in range(6):
        a = 2 * math.pi * k / 6
        puff(s, (math.cos(a) * 0.62, math.sin(a) * 0.62, -0.02), 0.22, (0.62, 0.60, 0.58), F, segs=5, h=0.2)
    for k in range(3):
        box(s, (-0.5, -0.07, 0.02), (0.5, 0.07, 0.16), (0.40, 0.26, 0.16), F @ T(0, 0, 0, rz=k * math.pi / 3),
            skip=("-z",))
    face(s, [(math.cos(2 * math.pi * k / 6) * 0.42, math.sin(2 * math.pi * k / 6) * 0.42, 0.04) for k in range(6)],
         (0, 0, 1), (0.22, 0.16, 0.14), F)
    flame(pl, F @ T(0, 0, 0.1), h=0.9, r=0.3, table=LAVA)  # (alight by day too)
    pl.lamp(F, (0, 0, 0.6), 5.5, 0.9)
    pl.solid(F, 0, 0, 0.9)
    pl.mark("campfire")
    # Tide pools at the water's edge, a starfish in one.
    for k, (x, y, r) in enumerate(((-12.6, 40.2, 1.1), (13.4, 39.0, 0.9))):
        P = on_ground(pl, x, y)
        face(s, [(math.cos(2 * math.pi * j / 7) * r, math.sin(2 * math.pi * j / 7) * r, 0.1) for j in range(7)],
             (0, 0, 1), (0.30, 0.56, 0.62), P, jit=0.04)
        for j in range(4):
            a = 2 * math.pi * j / 4 + k
            pebble(pl, P, math.cos(a) * (r + 0.1), math.sin(a) * (r + 0.1), 0.34 + 0.06 * (j % 2), (0.62, 0.64, 0.66),
                   seed=j + k)
        if k == 0:
            face(s, [(x2 * 0.6, z2 * 0.6, 0.12) for x2, z2 in star_outline(0.3, 0.12, 5)], (0, 0, 1),
                 (0.96, 0.52, 0.40), P)
        pl.solid(P, 0, 0, r + 0.3)
    pl.mark("tide pools")
    for k, (x, y) in enumerate(((-4.6, 22.6), (-3.8, 23.2), (8.6, 11.6), (-11.2, 21.8))):  # a few shells for show
        puff(s, (x, y, pl.gz(x, y) - 0.01), 0.1, ((0.98, 0.86, 0.80), (0.96, 0.74, 0.70))[k % 2], M, segs=4, h=0.07)
    for k, (x, y) in enumerate(((-5.2, 27.4), (3.8, 27.0), (-17.0, 26.0), (16.4, 27.6), (-11.0, 29.2), (11.8, 30.4))):
        reeds(pl, T(0, 0, pl.gz(x, y) - 0.05), x, y, 4, 0.9, seed=k)  # dune grass
    for k, (x, y, r) in enumerate(((10.6, 3.6, 0.9), (-6.2, 5.2, 0.7), (14.8, 12.6, 1.1))):  # rocks on the sand
        rock(pl, T(0, 0, pl.gz(x, y)), x, y, r, (0.70, 0.66, 0.62), (1.2, 1.0, 0.7), seed=k + 5, segs=6, rings=3)
        pl.solid(M, x, y, r * 1.1)
    Sg = on_ground(pl, -14.2, -8.2, rz=2.36)  # a sign where the path comes in: a fish
    sign_posts(pl, Sg, 0.55, 1.55, DRIFTWOOD)
    box(s, (-0.55, -0.05, 0.85), (0.55, 0.05, 1.4), (0.40, 0.58, 0.64), Sg, skip=())
    fish = [(-0.34, 0.0), (-0.1, 0.13), (0.14, 0.1), (0.3, 0.0), (0.14, -0.1), (-0.1, -0.13)]
    for sy in (1, -1):
        face(s, [(x, sy * 0.056, 1.12 + z) for x, z in fish], (0, sy, 0), (0.98, 0.84, 0.52), Sg)
        face(s, [(0.28, sy * 0.056, 1.12), (0.42, sy * 0.056, 1.24), (0.42, sy * 0.056, 1.0)], (0, sy, 0),
             (0.98, 0.84, 0.52), Sg)
    pl.solid(Sg, 0, 0, 0.7)
    pl.extra["anchors"]["shells"] = [[-16.5, 40.0], [-7.0, 41.0], [5.6, 40.2], [9.6, 39.6], [18.4, 38.2]]
    pl.mark("props")
    pl.flat = 18.0
    land_ao(pl, 56.0)
    pl.terrain = "land"
    pl.terrain_size = 56
    pl.water = wz
    pl.view = dict(radius=22.0, target=(0, 22.0, 0.0), azimuth=200.0, elevation=34.0)


# ---------------------------------------------------------------------- Frostspire Hollow
SPIRE_ICE = (0.72, 0.88, 0.99)
SPIRE_DEEP = (0.50, 0.70, 0.94)
FROST_GLOW = (0.45, 0.72, 1.0)


def ice_spire(pl, M, h, r, seed=0.0, glow=False):
    """A tall spire of ice, deep blue at its foot, paler up high; now and then a cold glow."""
    def col(p, n, z0=(M @ Vector((0, 0, 0))).z):
        return mixc(SPIRE_DEEP, SPIRE_ICE, smooth01(0.0, h, p.z - z0))
    lathe(pl.s, [(r, -0.6), (r * 0.8, h * 0.52), (r * 0.32, h * 0.88), (0, h)], 5, col, M, smooth=False, lump=0.1,
          seed=seed, jit=0.04)
    if glow:
        lathe(pl.g, [(r * 1.12, h * 0.1), (r * 0.9, h * 0.52), (0, h * 0.9)], 5, Emit(FROST_GLOW, FROST, fade=lambda p,
              z0=(M @ Vector((0, 0, 0))).z: 0.5 * (1 - smooth01(0.0, h, p.z - z0))), M, smooth=False, jit=0)


@place("hollow", "Frostspire Hollow")
def build_hollow(pl):
    """The training ground: a bowl in the cold heights (flat radius ~15 m, the rim rising steeply
    from 13-19 m out to ~16 m up; the corridor out through the rim at +Y, east). Ice spires ring
    the bowl, a cave door in the back wall (-Y) glows blue where wild dragons come out, the
    keeper's camp by the corridor, frost crystals, snow drifts."""
    M = I4
    s = pl.s
    deg = math.radians
    # The cave door: a rock arch standing out of the back wall, its short mouth dark and glowing.
    D = T(0, -10.4, 0)
    lip, rim = cave_facade(pl, D, 4.6, 4.8, 2.2, 2.2, moss_top(ROCK_COOL, SNOW, 0.45, 0.72),
                           deep=(0.04, 0.07, 0.16), n=11, seed=6.0, depth=1.6, back=(4.5, 1.0), lump=0.45,
                           smooth=False)
    gc = D @ Vector((0, -1.52, 1.9))
    disc(pl.g, 2.1, 8, Emit(FROST_GLOW, FROST, fade=lambda p, c=gc: 0.8 * max(0.0, 1 - (p - c).length / 2.1)),
         D @ T(0, -1.52, 1.9) @ Matrix.Rotation(-math.pi / 2, 4, "X"), jit=0)
    pl.lamp(D, (0, 0.6, 1.6), 6.0, 0.7, rgb=(0.5, 0.75, 1.0), table=FROST)
    for k, length in ((2, 0.6), (3, 1.0), (4, 0.7), (5, 1.2), (6, 0.8), (7, 1.1), (8, 0.6)):
        icicle(pl, lip[k] + Vector((0, 0.3, -0.05)), length)
    pl.solid(D, -3.4, -0.6, 1.8)
    pl.solid(D, 3.4, -0.6, 1.8)
    pl.solid(D, 0.0, -2.6, 3.0)
    anchors = {"arena": [0.0, -1.0, 0.0], "wild_door": [0.0, -8.2]}
    pl.mark("door")
    # Ice spires round the bowl (clear of the corridor and the door), two taller at the corridor.
    for k, (ang, r, h) in enumerate(((10, 13.4, 9.0), (34, 13.0, 7.0), (54, 13.2, 10.5), (70, 13.2, 11.5),
                                     (110, 13.2, 11.0), (126, 13.2, 8.0), (148, 13.4, 9.5), (172, 13.0, 7.5),
                                     (198, 13.4, 10.0), (220, 13.2, 8.5), (238, 12.8, 6.5), (302, 12.8, 7.0),
                                     (320, 13.0, 9.5), (342, 13.4, 8.0))):
        a = deg(ang)
        x, y = math.cos(a) * r, math.sin(a) * r
        W = on_ground(pl, x, y, rz=k * 1.3, sink=0.2) @ Matrix.Rotation(-0.08, 4, "X") @ Matrix.Rotation(
            0.06 * (1 if k % 2 else -1), 4, "Y")
        ice_spire(pl, W, h, 0.85 + 0.1 * (k % 3), seed=k, glow=k in (3, 4, 8, 12))
        if k % 2 == 0:
            ice_spire(pl, W @ T(0.9, 0.5, 0), h * 0.45, 0.4, seed=k + 20)
        pl.solid(M, x, y, 1.2)
    pl.mark("spires")
    # Frost crystals in clusters at the bowl's edge.
    for k, (x, y, n) in enumerate(((-9.6, -5.6, 3), (9.8, -4.2, 3), (10.2, 5.0, 2), (-10.4, 2.4, 2), (8.6, -7.8, 2))):
        z = pl.gz(x, y)
        for j in range(n):
            crystal(pl, M, x + 0.45 * (j - 1), y + 0.3 * ((j * 7) % 3 - 1), z, 1.1 + 0.4 * ((j + k) % 3), 0.2,
                    tilt=0.35 + 0.12 * j, yaw=j * 2.1 + k, col=(0.66, 0.86, 1.0), glow=FROST_GLOW if j == 0 else None,
                    table=FROST)
        pl.solid(M, x, y, 0.9)
    pl.mark("crystals")
    # The keeper's camp by the corridor: a tent, a brazier, a tally board of floors, crates.
    Tn = on_ground(pl, -7.0, 7.0, rz=-0.7)
    tw, td, th = 1.6, 1.7, 2.1
    for sx in (-1, 1):
        pts = [(sx * tw, -td, 0), (sx * tw, td, 0), (0, td, th), (0, -td, th)]
        face(s, pts, (sx * th, 0, tw), (0.40, 0.52, 0.78), Tn)
        face(s, pts, (-sx * th, 0, -tw), (0.30, 0.38, 0.56), Tn)
    face(s, [(-tw, -td, 0), (tw, -td, 0), (0, -td, th)], (0, -1, 0), (0.40, 0.52, 0.78), Tn, double=True)
    for sx in (-1, 1):
        face(s, [(0, td, th), (sx * tw, td, 0), (sx * tw * 1.2, td + 0.5, 0.45)], (0, 1, 0), (0.86, 0.80, 0.66), Tn,
             double=True)
    face(s, [(-tw * 0.9, -td, 0.03), (tw * 0.9, -td, 0.03), (tw * 0.9, td, 0.03), (-tw * 0.9, td, 0.03)], (0, 0, 1),
         (0.36, 0.30, 0.34), Tn)
    box(s, (-0.05, td - 0.05, 0), (0.05, td + 0.05, 2.6), DARKWOOD, Tn, skip=("-z",))
    face(s, [(0, td, 2.55), (0, td, 2.2), (0, td + 0.8, 2.38)], (1, 0, 0), (0.60, 0.80, 1.0), Tn, double=True)
    pl.solid(Tn, 0, 0, 1.9)
    Br = on_ground(pl, -4.4, 4.6)
    for k in range(3):
        a = 2 * math.pi * k / 3
        leg = Br @ T(math.cos(a) * 0.36, math.sin(a) * 0.36, -0.1, rz=a) @ Matrix.Rotation(0.22, 4, "Y")
        box(s, (-0.04, -0.04, 0), (0.04, 0.04, 1.0), IRON, leg, skip=("-z", "+z"))
    lathe(s, [(0.2, 0.86), (0.52, 1.12), (0.46, 1.16), (0.0, 1.0)], 6, IRON, Br, sharp=[1, 2], smooth=False)
    flame(pl, Br @ T(0, 0, 1.06), h=0.8, r=0.26, table=LAVA)  # (alight by day too)
    pl.lamp(Br, (0, 0, 1.6), 6.0, 0.9)
    pl.solid(Br, 0, 0, 0.7)
    Tb = on_ground(pl, -9.6, 3.2, rz=-1.1)
    sign_posts(pl, Tb, 0.6, 1.7, DARKWOOD)
    box(s, (-0.6, -0.05, 0.75), (0.6, 0.05, 1.55), (0.28, 0.30, 0.34), Tb, skip=())
    for k in range(5):  # the floors' tally, chalked
        x = -0.4 + 0.2 * k
        face(s, [(x - 0.02, 0.055, 0.95), (x + 0.02, 0.055, 0.95), (x + 0.02, 0.055, 1.35), (x - 0.02, 0.055, 1.35)],
             (0, 1, 0), (0.92, 0.94, 0.98), Tb, jit=0)
    face(s, [(-0.5, 0.055, 1.12), (0.5, 0.055, 1.2), (0.5, 0.055, 1.24), (-0.5, 0.055, 1.16)], (0, 1, 0),
         (0.92, 0.94, 0.98), Tb, jit=0)
    pl.solid(Tb, 0, 0, 0.7)
    crate(pl, M @ T(0, 0, pl.gz(-9.6, 5.3)), -9.6, 5.3, 0.7, 0.4)
    crate(pl, M @ T(0, 0, pl.gz(-9.6, 5.3) + 0.52), -9.55, 5.3, 0.5, 0.9)
    anchors["keeper"] = [-4.6, 6.4]
    pl.mark("camp")
    # Snow drifted against the wall's foot, a few frosted rocks.
    for k, (x, y, r, h) in enumerate(((-11.6, -7.8, 2.2, 0.8), (11.4, -8.4, 2.4, 0.9), (-12.4, 3.8, 1.8, 0.7),
                                      (12.2, 2.2, 2.0, 0.8), (7.6, 10.4, 1.6, 0.6))):
        dome(s, r, h * 1.4, 8, SNOW, on_ground(pl, x, y, sink=0.2), rings=2, lump=0.22, seed=k)
    for k, (x, y, r) in enumerate(((-8.9, -8.9, 0.9), (10.9, 7.2, 1.0), (-11.2, -1.0, 0.8))):
        rock(pl, T(0, 0, pl.gz(x, y)), x, y, r, ROCK_COOL, (1.1, 1.0, 0.8), seed=k + 3, segs=6, rings=3, moss=SNOW)
        pl.solid(M, x, y, r * 1.1)
    # A ring of frost on the floor where the training bouts are fought.
    lathe(s, [(6.6, 0.045), (6.2, 0.045)], 16, (0.86, 0.94, 1.0), T(0, -1.0, 0), smooth=False, jit=0.03)
    pl.extra["anchors"] = anchors
    pl.mark("props")
    pl.flat = 15.0
    land_ao(pl, 36.0)
    pl.terrain = "land"
    pl.terrain_size = 30
    pl.view = dict(radius=17.0, target=(0, -2.5, 3.0), azimuth=12.0, elevation=40.0)


# ------------------------------------------------------------------------------ bake
def fib_hemisphere(n):
    out = []
    for i in range(n):
        u = (i + 0.5) / n
        z = math.sqrt(1 - u)          # cosine-weighted
        r = math.sqrt(u)
        a = i * 2.399963
        out.append(Vector((math.cos(a) * r, math.sin(a) * r, z)))
    return out


AO_DIRS = fib_hemisphere(18)



def bake(pl):
    casters = [pl.parts[n] for n in ("solid", "lantern") if n in pl.parts]
    verts, polys = [], []
    for part in casters:
        base = len(verts)
        verts += [tuple(p) for p in part.P]
        polys += [(a + base, b + base, c + base) for a, b, c in part.T]
    for q in pl.ground:
        base = len(verts)
        verts += [tuple(p) for p in q]
        polys.append(tuple(range(base, base + len(q))))
    bvh = BVHTree.FromPolygons(verts, polys, all_triangles=False, epsilon=0.0)
    ao_bvh = bvh
    if pl.ao_ground:  # (the landscape shades its corners, but casts no sun shadow: nor does the game's ground)
        for q in pl.ao_ground:
            base = len(verts)
            verts += [tuple(p) for p in q]
            polys.append(tuple(range(base, base + len(q))))
        ao_bvh = BVHTree.FromPolygons(verts, polys, all_triangles=False, epsilon=0.0)
    suns = {}
    for s in SETS:
        d = Vector(SUN[s][0]).normalized()
        side = d.cross(Vector((0, 0, 1))).normalized()
        up = side.cross(d)
        suns[s] = [d] + [(d + (side * math.cos(a) + up * math.sin(a)) * 0.045).normalized()
                         for a in (0.0, 1.57, 3.14, 4.71)]
    out = {}
    for name, part in pl.parts.items():
        cols = []
        for i, p in enumerate(part.P):
            n, alb, em, vf = part.N[i], part.C[i], part.E[i], part.F[i]
            if em is not None:
                rgb, table = em
                cols.append([tuple(min(1.0, c * table[s]) for c in rgb) for s in SETS])
                continue
            o = p + n * 0.03
            ao = 1.0
            if not vf & NOAO:
                t = Vector((1, 0, 0)) if abs(n.x) < 0.9 else Vector((0, 1, 0))
                bx = n.cross(t).normalized()
                by = n.cross(bx)
                occ = 0.0
                for d in AO_DIRS:
                    w = bx * d.x + by * d.y + n * d.z
                    hit = ao_bvh.ray_cast(o, w, pl.ao_reach)
                    if hit[0] is not None:
                        occ += (1.0 - hit[3] / pl.ao_reach) ** 0.7
                ao = 1.0 - pl.ao_strength * occ / len(AO_DIRS)
            per = []
            for s in SETS:
                L, sc, si = Vector(SUN[s][0]).normalized(), SUN[s][1], SUN[s][2]
                lit = 1.0
                if not vf & NOSHADOW and n.dot(L) > -0.3:
                    hits = sum(1 for d in suns[s] if bvh.ray_cast(o, d, 80.0)[0] is not None)
                    lit = 1.0 - hits / len(suns[s])
                wrap = max(0.0, (n.dot(L) + 0.3) / 1.3)
                hemi = 0.5 + 0.5 * n.z
                col = []
                for k in range(3):
                    amb = SKY[s][k] * (0.72 + 0.28 * hemi) + BOUNCE[s][k] * (1 - hemi) * 0.3
                    col.append(amb * ao + sc[k] * si * wrap * lit)
                if True:
                    for lp, lrgb, rng, strength, table in pl.lamps:
                        if table[s] <= 0:
                            continue
                        d = lp - p
                        dist = d.length
                        if dist >= rng or dist < 1e-4:
                            continue
                        fall = (1 - dist / rng) ** 2
                        wr = 0.25 + 0.75 * max(0.0, n.dot(d) / dist)
                        if bvh.ray_cast(o, d.normalized(), dist - 0.05)[0] is not None:
                            continue
                        for k in range(3):
                            col[k] += lrgb[k] * strength * table[s] * fall * wr
                per.append(tuple(min(1.0, alb[k] * col[k]) for k in range(3)))
            cols.append(per)
        out[name] = cols
    return out


# ------------------------------------------------------------------------------ output
def write_esm(path, baked):
    """.esm v1 (src/core/static_mesh.cpp), little-endian, as den_model.py writes it:
    "ESM1" u16 version, u16 meshCount, u16 setCount, u16 reserved; setCount x RGBA8 backdrop;
    per mesh: char[16] name, u8 flags, u8 reserved, u16 vertexCount, u16 indexCount,
    float3 x vertexCount, setCount x vertexCount x RGBA8, u16 x indexCount (mesh-local)."""
    def rgba(c):
        return struct.pack("<4B", *(int(round(max(0.0, min(1.0, v)) * 255)) for v in c), 255)
    out = bytearray(b"ESM1" + struct.pack("<HHHH", 1, len(baked), len(SETS), 0))
    for s in SETS:
        out += rgba(BACKDROP[s])
    for name, flags, verts, tris in baked:
        idx = [i for t in tris for i in t]
        assert len(verts) < 65536 and len(idx) < 65536
        out += struct.pack("<16sBBHH", name.encode()[:15], flags, 0, len(verts), len(idx))
        for p, _ in verts:
            out += struct.pack("<3f", p.x, p.y, p.z)
        for s in range(len(SETS)):
            for _, cols in verts:
                out += rgba(cols[s])
        out += struct.pack(f"<{len(idx)}H", *idx)
    with open(path, "wb") as f:
        f.write(bytes(out))
    return len(out)


def check_esm(path):
    """Read a file back as src/core/static_mesh.cpp does; returns [(name, flags, verts, tris)]."""
    data = open(path, "rb").read()
    assert data[:4] == b"ESM1"
    version, count, sets, _ = struct.unpack_from("<HHHH", data, 4)
    assert version == 1 and 1 <= sets <= 4 and count <= 64
    off = 12 + 4 * sets
    parts, total = [], 0
    for _ in range(count):
        name, flags, _, nv, ni = struct.unpack_from("<16sBBHH", data, off)
        off += 22
        assert ni % 3 == 0 and total + nv <= 0xFFFF
        off += 12 * nv + 4 * nv * sets
        idx = struct.unpack_from(f"<{ni}H", data, off)
        off += 2 * ni
        assert all(i < nv for i in idx)
        total += nv
        parts.append((name.split(b"\0")[0].decode(), flags, nv, ni // 3))
    assert off == len(data)
    return parts


def split_part(part, cols):
    """Chunks of at most 65,535 vertices and indices (mesh-local u16 indices)."""
    chunks, cur_v, cur_t, remap = [], [], [], {}
    for tri in part.T:
        if len(cur_v) + 3 > 65535 or (len(cur_t) + 1) * 3 > 65535:
            chunks.append((cur_v, cur_t))
            cur_v, cur_t, remap = [], [], {}
        t = []
        for i in tri:
            if i not in remap:
                remap[i] = len(cur_v)
                cur_v.append((part.P[i], cols[i]))
            t.append(remap[i])
        cur_t.append(tuple(t))
    if cur_t:
        chunks.append((cur_v, cur_t))
    return chunks


def export(pl, baked_cols, path):
    baked = []
    for name in PART_ORDER:
        part = pl.parts.get(name)
        if not part or not part.T:
            continue
        for verts, tris in split_part(part, baked_cols[name]):
            baked.append((name, part.flags, verts, tris))
    size = write_esm(path, baked)
    parts = check_esm(path)
    return size, parts


def place_json(pl):
    solids = []
    for c in pl.solids:  # one circle per spot (fence corners meet twice)
        same = [o for o in solids if abs(o[0] - c[0]) < 0.05 and abs(o[1] - c[1]) < 0.05]
        if same:
            same[0][2] = max(same[0][2], c[2])
        else:
            solids.append(list(c))
    d = {"flat": round(pl.flat, 2), "door": pl.door, "lantern": pl.lantern, "solids": solids}
    d.update(pl.extra)
    return d


# ------------------------------------------------------------------------------ preview
def vertex_colour_material(name, additive):
    mat = bpy.data.materials.new(name)
    mat.use_backface_culling = not additive  # as on the 3DS
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    attr = nt.nodes.new("ShaderNodeAttribute")
    attr.attribute_name = "vc"
    em = nt.nodes.new("ShaderNodeEmission")
    outn = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(attr.outputs["Color"], em.inputs["Color"])
    if additive:
        tr = nt.nodes.new("ShaderNodeBsdfTransparent")
        addn = nt.nodes.new("ShaderNodeAddShader")
        nt.links.new(em.outputs[0], addn.inputs[0])
        nt.links.new(tr.outputs[0], addn.inputs[1])
        nt.links.new(addn.outputs[0], outn.inputs["Surface"])
        try:
            mat.surface_render_method = "BLENDED"
        except AttributeError:
            mat.blend_method = "BLEND"
    else:
        nt.links.new(em.outputs[0], outn.inputs["Surface"])
    return mat


def mesh_object(name, verts, tris, mat):
    me = bpy.data.meshes.new(name)
    me.from_pydata([tuple(v) for v in verts], [], [tuple(t) for t in tris])
    me.update()
    obj = bpy.data.objects.new(name, me)
    bpy.context.collection.objects.link(obj)
    me.materials.append(mat)
    return obj


def set_colours(obj, cols):
    me = obj.data
    if "vc" in me.color_attributes:
        me.color_attributes.remove(me.color_attributes["vc"])
    ca = me.color_attributes.new("vc", "FLOAT_COLOR", "POINT")
    flat = []
    for c in cols:
        flat += [c[0] ** 2.2, c[1] ** 2.2, c[2] ** 2.2, 1.0]
    ca.data.foreach_set("color", flat)


def default_terrain(x, y):
    return 0.0, GRASS


def stand_in_ground(pl):
    """Preview only: the landscape round the place, lit per set (no shadows: the game's ground
    gets none from the places either)."""
    fn = pl.terrain or default_terrain
    if fn == "land":  # the landscape's own ground and colours (its light baked in: roughly undone)
        def fn(x, y):
            if not pl.anchor:
                return 0.0, GRASS
            return pl.gz(x, y), tuple(min(1.0, c * 0.95) for c in LAND.colour(*pl.world_xy(x, y)))
    size = pl.terrain_size
    n = 48
    verts, tris, alb, nrm = [], [], [], []
    for j in range(n + 1):
        for i in range(n + 1):
            x, y = -size + 2 * size * i / n, -size + 2 * size * j / n
            z, c = fn(x, y)
            verts.append(Vector((x, y, z)))
            alb.append(c)
    for j in range(n + 1):
        for i in range(n + 1):
            a = verts[j * (n + 1) + max(0, i - 1)]
            b = verts[j * (n + 1) + min(n, i + 1)]
            c = verts[max(0, j - 1) * (n + 1) + i]
            d = verts[min(n, j + 1) * (n + 1) + i]
            nrm.append((b - a).cross(d - c).normalized())
    for j in range(n):
        for i in range(n):
            k = j * (n + 1) + i
            tris += [(k, k + 1, k + n + 2), (k, k + n + 2, k + n + 1)]
    sets = []
    for s in SETS:
        L, sc, si = Vector(SUN[s][0]).normalized(), SUN[s][1], SUN[s][2]
        per = []
        for p, c, nn in zip(verts, alb, nrm):
            wrap = max(0.0, (nn.dot(L) + 0.3) / 1.3)
            hemi = 0.5 + 0.5 * nn.z
            col = [c[k] * (SKY[s][k] * (0.72 + 0.28 * hemi) + sc[k] * si * wrap) for k in range(3)]
            fade = max(0.0, min(1.0, (p.to_2d().length - size * 0.55) / (size * 0.4)))
            per.append(tuple(min(1.0, col[k] + (BACKDROP[s][k] - col[k]) * fade) for k in range(3)))
        sets.append(per)
    return verts, tris, sets


def water_sheet(pl, size):
    z = pl.water
    verts = [Vector((-size, -size, z)), Vector((size, -size, z)), Vector((size, size, z)), Vector((-size, size, z))]
    sets = []
    for s in SETS:
        sky = BACKDROP[s]
        c = tuple(min(1.0, WATER[k] * (SKY[s][k] * 0.9 + SUN[s][1][k] * SUN[s][2] * 0.6) * 0.85 + sky[k] * 0.12)
                  for k in range(3))
        sets.append([c] * 4)
    return verts, [(0, 1, 2), (0, 2, 3)], sets


def clear_scene():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for coll in (bpy.data.meshes, bpy.data.materials, bpy.data.curves, bpy.data.cameras, bpy.data.images):
        for d in list(coll):
            if d.users == 0:
                coll.remove(d)


def label(cam, text, fov_y, aspect):
    th = math.tan(fov_y / 2)
    for dx, dz, col, zoff in ((0.004, -0.004, (0.0, 0.0, 0.0), -1.001), (0, 0, (1.0, 1.0, 1.0), -1.0)):
        cu = bpy.data.curves.new("label", "FONT")
        cu.body = text
        cu.size = 0.075
        ob = bpy.data.objects.new("label", cu)
        bpy.context.collection.objects.link(ob)
        ob.parent = cam
        ob.location = (-aspect * th + 0.04 + dx, th - 0.1 + dz, zoff)
        mat = bpy.data.materials.new("label")
        mat.use_nodes = True
        nt = mat.node_tree
        nt.nodes.clear()
        em = nt.nodes.new("ShaderNodeEmission")
        em.inputs["Color"].default_value = (*col, 1)
        outn = nt.nodes.new("ShaderNodeOutputMaterial")
        nt.links.new(em.outputs[0], outn.inputs["Surface"])
        cu.materials.append(mat)


def render_place(pl, baked_cols, out_dir):
    clear_scene()
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_EEVEE"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    try:
        scene.eevee.taa_render_samples = 16
    except AttributeError:
        pass
    W, H = 800, 480
    scene.render.resolution_x, scene.render.resolution_y = W, H
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = "Standard"
    world_ = bpy.data.worlds.get("w") or bpy.data.worlds.new("w")
    scene.world = world_
    world_.use_nodes = True
    bg = world_.node_tree.nodes["Background"]
    opaque, add = vertex_colour_material("vc", False), vertex_colour_material("add", True)
    objs = []
    lantern_light = None
    for name in PART_ORDER:
        part = pl.parts.get(name)
        if not part or not part.T:
            continue
        ob = mesh_object(name, part.P, part.T, add if part.flags & ADDITIVE else opaque)
        objs.append((ob, baked_cols[name]))
        if name == "lantern_light":
            lantern_light = ob
    extras = []
    if pl.terrain != "none":
        gv, gt, gsets = stand_in_ground(pl)
        extras.append((mesh_object("ground", gv, gt, opaque), gsets))
    if pl.water is not None:
        wv, wt, wsets = water_sheet(pl, pl.terrain_size)
        extras.append((mesh_object("water", wv, wt, opaque), wsets))
    if pl.preview_extra:
        for ob, sets in pl.preview_extra(pl, opaque):
            extras.append((ob, sets))
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    bpy.context.collection.objects.link(cam)
    scene.camera = cam
    fov = math.radians(40)
    cam.data.sensor_fit = "VERTICAL"
    cam.data.angle_y = fov
    cam.data.clip_start, cam.data.clip_end = 0.1, 400
    v = pl.view
    if CAM:
        tx, ty, tz, rad, azi, ele = (float(x) for x in CAM.split(","))
        v = dict(target=(tx, ty, tz), radius=rad, azimuth=azi, elevation=ele)
    az, el = math.radians(v["azimuth"]), math.radians(v["elevation"])
    dirn = Vector((math.sin(az) * math.cos(el), math.cos(az) * math.cos(el), math.sin(el)))
    dist = v["radius"] / math.tan(fov / 2) * 0.92
    target = Vector(v["target"])
    cam.location = target + dirn * dist
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
    label(cam, pl.title, fov, W / H)
    shots = []
    for si, s in enumerate(SETS):
        bg.inputs["Color"].default_value = (*[c ** 2.2 for c in BACKDROP[s]], 1)
        for ob, cols in objs:
            set_colours(ob, [c[si] for c in cols])
        for ob, sets in extras:
            set_colours(ob, sets[si])
        if lantern_light:
            lantern_light.hide_render = (s == "day")  # unlit by day, lit at dusk and night
        path = os.path.join(out_dir, f"{pl.id}_{s}.png")
        scene.render.filepath = path
        bpy.ops.render.render(write_still=True)
        shots.append(path)
    compose([shots], os.path.join(out_dir, f"{pl.id}.png"), 1)


def load_pixels(path):
    import numpy as np
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    buf = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(buf)
    bpy.data.images.remove(img)
    return buf.reshape(h, w, 4)


def compose(rows, path, shrink):
    """Tile images (rows of paths, top row first) into one PNG, each shrunk by an integer factor."""
    import numpy as np
    tiles = [[load_pixels(p) if p and os.path.exists(p) else None for p in row] for row in rows]
    ref = next(t for row in tiles for t in row if t is not None)
    h, w = ref.shape[0] // shrink, ref.shape[1] // shrink
    cols = max(len(r) for r in rows)
    out = np.zeros((h * len(rows), w * cols, 4), dtype=np.float32)
    out[..., 3] = 1.0
    for r, row in enumerate(tiles):
        for c, t in enumerate(row):
            if t is None:
                continue
            small = t[:h * shrink, :w * shrink].reshape(h, shrink, w, shrink, 4).mean(axis=(1, 3))
            y0 = (len(rows) - 1 - r) * h  # Blender images are bottom-up
            out[y0:y0 + h, c * w:(c + 1) * w] = small
    img = bpy.data.images.new("sheet", out.shape[1], out.shape[0], alpha=True)
    img.pixels.foreach_set(out.ravel())
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


# ------------------------------------------------------------------------------ main
def main():
    ids = ONLY or list(PLACE_IDS)
    unknown = [i for i in ids if i not in PLACES]
    if unknown:
        raise SystemExit(f"[places] not built yet / unknown: {unknown}")
    if OUT:
        os.makedirs(OUT, exist_ok=True)
    if PREVIEW:
        os.makedirs(PREVIEW, exist_ok=True)
    data = {}
    if os.path.exists(JSON_OUT):
        with open(JSON_OUT) as f:
            data = json.load(f).get("places", {})
    report = []
    for pid in ids:
        title, fn = PLACES[pid]
        pl = Place(pid, title)
        fn(pl)
        tris = pl.triangles()
        budget = BUDGET.get(pid, DEFAULT_BUDGET)
        cols = bake(pl)
        line = f"[places] {pid:10s} {tris:5d} / {budget} tris  " + "  ".join(
            f"{n}:{len(p.T)}" for n, p in pl.parts.items() if p.T)
        print(line)
        report.append(line)
        if DRAFT and pl.marks:
            print("[places]   " + ", ".join(f"{k} {v}" for k, v in pl.marks))
        if tris > budget and not DRAFT:
            raise SystemExit(f"[places] {pid} is over its budget: {tris} > {budget}")
        if OUT:
            size, parts = export(pl, cols, os.path.join(OUT, f"{pid}.esm"))
            print(f"[places]   {pid}.esm {size} bytes, parts {[(n, f, t) for n, f, _, t in parts]}")
        data[pid] = place_json(pl)
        if PREVIEW:
            render_place(pl, cols, PREVIEW)
    if OUT or ONLY:
        ordered = {k: data[k] for k in PLACE_IDS if k in data}
        doc = {"about": "Per place (local frame: metres, Z up, origin on the ground at the anchor, +Y the front): "
                        "flat = radius to flatten the ground to; door = [x, y] where you walk in (null: none); "
                        "lantern = [x, y, z] of the festival lantern's light (null: none); solids = [[x, y, r]] "
                        "circles to walk round. Market: egg_stand = [x, y, z] where the egg of the day stands, "
                        "goods_spots = four [x, y, z] on the goods stall's counter. Mill: hub = [x, y, z] the sails "
                        "turn round, about hub_axis; river = its axis and half width (keep it cut through the flat "
                        "ground). Lake and mill: water_z = the water's level. 1.0's places: anchors = named spots, "
                        "each [x, y] or [x, y, z] or a list of them (a third value: a height in the frame, or the "
                        "glade's stalls' facing). See docs/tech/places.md. Written by tools/blender/valley_places.py.",
               "places": ordered}
        os.makedirs(os.path.dirname(JSON_OUT), exist_ok=True)
        with open(JSON_OUT, "w", newline="\n") as f:
            f.write(json_dump(doc) + "\n")
        print(f"[places] {JSON_OUT}")
    if PREVIEW:
        rows = []
        for k in range(0, len(PLACE_IDS), 2):
            row = []
            for pid in PLACE_IDS[k:k + 2]:
                row += [os.path.join(PREVIEW, f"{pid}_day.png"), os.path.join(PREVIEW, f"{pid}_night.png")]
            rows.append(row)
        compose(rows, os.path.join(PREVIEW, "contact_sheet.png"), 2)
        print(f"[places] {os.path.join(PREVIEW, 'contact_sheet.png')}")


def json_dump(doc):
    """Readable JSON with short lists kept on one line."""
    text = json.dumps(doc, indent=2)
    import re

    def squash(m):
        return "[" + ", ".join(x.strip() for x in m.group(1).split(",")) + "]"
    for _ in range(3):
        text = re.sub(r"\[\s*([^\[\]{}]*?)\s*\]", squash, text)
    return text


main()
