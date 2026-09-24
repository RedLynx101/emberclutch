"""Emberclutch den: a cozy cave room, lit by vertex colours for day, evening and night.

  blender -b -P tools/blender/den_model.py -- --out C:/abs/romfs/models/den.esm
  blender -b -P tools/blender/den_model.py -- --render C:/abs/prefix [--dragons]

The room is built around the spots in src/core/behavior.hpp DenLayout (keep them in sync;
tests/test_den.cpp checks den.esm against it) and written as romfs/models/den.esm (read by
src/core/static_mesh.cpp).

It is a cutaway diorama. The den camera always looks from the front-left
(src/app/render3d.cpp) and follows the dragons, often from outside the round wall. So the
walls go all the way round and face inward: back-face culling hides whatever part stands
between the camera and the dragons. The floor runs on past the walls into the dark (the
backdrop colour), so the frame never shows an edge. Solid props stay in the back two
thirds, where they can never hide a dragon.

Lighting is baked analytically per vertex (no Blender bake): albedo x (ambient + sky fill +
the pool of light under the skylight + hearth), with wrap lighting and a little occlusion low
on the walls and in the nook, for each lighting set. The sky and the embers are emissive. Two
additive meshes face the fixed camera direction: the sunbeam and the hearth flames.
Budget ~2,250 triangles: the den frame must stay under 8k with three dragons.

--render writes <prefix>_<shot>_<set>.png from the game camera; --dragons adds real dragons
posed with the game's clips (tools/anim/clips.py) for scale. Pass absolute paths.
"""
import math
import os
import struct
import sys

import bmesh
import bpy
from mathutils import Matrix, Quaternion, Vector
from mathutils.noise import noise

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


OUT = arg("--out")
RENDER = arg("--render")
WITH_DRAGONS = "--dragons" in argv
ONLY_SHOT = arg("--shot")  # render just this shot (a fresh Blender per shot builds cleanest)

# ------------------------------------------------------------------------------ layout
# Keep in sync with src/core/behavior.hpp DenLayout (adult units, Z up, the camera side is -Y).
R = 9.5                              # floor and wall radius
HOME = Vector((0.0, 0.6, 0))         # the rug (the walkable circle, radius 6, is centred here)
NEST = Vector((4.4, 3.6, 0))         # beds[0], the big nest
BEDS = [Vector((-0.6, 5.4, 0)), Vector((2.8, -3.2, 0))]  # beds[1], beds[2]: straw beds
EGG_NEST = Vector((5.4, -1.6, 0))
EGG_NEST_2 = Vector((6.5, -3.7, 0))  # Alpha 2: a second egg (DenLayout::eggNests)
NOOK = Vector((-4.6, 3.0, 0))        # sulkSpots[0] (a sulking dragon faces +Y: its nose is ~2.2 ahead)
HEARTH = Vector((7.9, 0.8, 0))
HOARD = Vector((2.4, 7.4, 0))
SUN_SPOT = Vector((1.0, 4.6, 0))     # where the sunbeam lands
SKY_A, SKY_Z, SKY_R = math.radians(12), 4.8, 1.3   # skylight: angle from +Y, height, radius
SHELF_A = math.radians(-8)   # between the nook rocks and the skylight
CAM_DIR = Vector((-0.35, -0.9, 0.32)).normalized()  # render3d.cpp drawDen: toward the camera
CAM_RIGHT = Vector((0.0, 0.0, 1.0)).cross(CAM_DIR).normalized()  # screen right, seen from the camera
FOV_Y = math.radians(38)

# Lighting sets (src/core/daylight.hpp names them in this order).
SETS = ("day", "evening", "night")
AMBIENT = {"day": (0.74, 0.68, 0.78), "evening": (0.56, 0.44, 0.52), "night": (0.30, 0.29, 0.46)}
SKY_FILL = {"day": ((0.95, 0.92, 0.85), 0.6), "evening": ((1.0, 0.66, 0.45), 0.4),
            "night": ((0.45, 0.55, 0.85), 0.3)}
SUN_POOL = {"day": ((1.0, 0.94, 0.78), 0.9), "evening": ((1.0, 0.62, 0.36), 0.6),
            "night": ((0.55, 0.65, 0.95), 0.35)}
HEARTH_LIGHT = {"day": ((1.0, 0.58, 0.28), 0.45), "evening": ((1.0, 0.56, 0.26), 0.8),
                "night": ((1.0, 0.5, 0.22), 1.15)}
BACKDROP = {"day": (0.17, 0.12, 0.20), "evening": (0.15, 0.10, 0.15), "night": (0.06, 0.05, 0.10)}
EMBER = {"day": (1.0, 0.55, 0.2), "evening": (1.0, 0.5, 0.18), "night": (1.0, 0.46, 0.16)}
FIRE = {"day": 0.8, "evening": 1.0, "night": 1.1}
SKY_TOP = {"day": (0.46, 0.72, 0.98), "evening": (0.62, 0.40, 0.62), "night": (0.08, 0.10, 0.26)}
SKY_LOW = {"day": (0.86, 0.94, 1.0), "evening": (1.0, 0.66, 0.42), "night": (0.18, 0.22, 0.44)}
BEAM = {"day": ((1.0, 0.92, 0.75), 0.38), "evening": ((1.0, 0.62, 0.36), 0.22), "night": ((0.50, 0.60, 0.95), 0.14)}
# Dragon tint in previews: (ambient + key) per set relative to day, as src/core/daylight.cpp.
DRAGON_TINT = {"day": (1.0, 1.0, 1.0), "evening": (1.05, 0.80, 0.70), "night": (0.59, 0.63, 0.97)}

ADDITIVE, FLICKER = 1, 2  # mesh flags (.esm)
# (name, object, albedo fn(p, n) -> rgb, emissive: fn(p, set) -> rgb | None, or a per-vertex list)
PARTS = []


def link(name, bm):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new(name, me)
    bpy.context.collection.objects.link(obj)
    return obj


def add(name, bm, albedo=None, emissive=None, flags=0):
    obj = link(name, bm)
    PARTS.append((name, obj, albedo, emissive, flags))
    return obj


def jitter(c, amount, p, scale=1.3):
    n = noise(p * scale) * amount
    return tuple(max(0.0, min(1.0, v * (1.0 + n))) for v in c)


def mix(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))


def smoothstep(a, b, x):
    t = max(0.0, min(1.0, (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


def radial(a):
    """Unit direction at angle a from +Y (a > 0 turns toward +X)."""
    return Vector((math.sin(a), math.cos(a), 0))


# ------------------------------------------------------------------------------ floor & walls
def ring_disc(bm, centre, radii, segs, z, sy=1.0):
    """Concentric rings around a centre vertex, facing up."""
    c = bm.verts.new(centre + Vector((0, 0, z)))
    prev = None
    for r in radii:
        ring = [bm.verts.new(centre + Vector((math.cos(2 * math.pi * k / segs) * r,
                                              math.sin(2 * math.pi * k / segs) * r * sy, z))) for k in range(segs)]
        for k in range(segs):
            if prev is None:
                bm.faces.new((c, ring[k], ring[(k + 1) % segs]))
            else:
                bm.faces.new((prev[k], ring[k], ring[(k + 1) % segs], prev[(k + 1) % segs]))
        prev = ring


def floor():
    bm = bmesh.new()
    ring_disc(bm, Vector((0, 0, 0)), [3.0, 4.5, 6.0, 7.5, 8.6, R, 16.0], 20, 0.0)  # the rug covers the middle

    def albedo(p, n):
        base = (0.64, 0.50, 0.38)  # warm sandstone
        straw = (0.80, 0.68, 0.40)
        near = max(0.0, 1.0 - min([(p - NEST).length / 2.8, (p - EGG_NEST).length / 2.0,
                                   (p - EGG_NEST_2).length / 2.0] +
                                  [(p - b).length / 2.4 for b in BEDS]))
        c = mix(base, straw, near * 0.7)
        c = mix(c, (0.40, 0.33, 0.30), max(0.0, 1.0 - (p - HEARTH).length / 2.2) * 0.6)  # soot
        return jitter(c, 0.12, p)
    add("floor", bm, albedo)


WALL_H = [-0.3, 1.4, 3.3, 5.5, 7.8, 10.2, 12.4]
WALL_R = [R - 0.2, R + 0.1, R + 0.05, R - 0.3, R - 1.1, R - 2.6, R - 4.8]
WALL_SEGS = 24  # Alpha 2 trimmed the room for a full den and its toys and decor (<= 2,000 with them)
WALL_TOP = 13.6
WALL_GRID = []  # rows of vertex positions, for wall_point()


def calm(a, h):
    """No bumps where things are fitted to the wall (skylight, shelves)."""
    def near(a0, h0, da, dh):
        d = abs((a - a0 + math.pi) % (2 * math.pi) - math.pi)
        return d < da and abs(h - h0) < dh
    return near(SKY_A, SKY_Z, math.radians(20), 3.2) or near(SHELF_A, 2.2, math.radians(16), 2.6)


def walls():
    """A round cave wall rising into a dome, all the way round, facing inward."""
    bm = bmesh.new()
    grid = []
    for h, r in zip(WALL_H, WALL_R):
        row = []
        for k in range(WALL_SEGS):
            a = -math.pi + 2 * math.pi * k / WALL_SEGS
            d = radial(a)
            bump = 0.0
            if 0.5 < h < 11 and not calm(a, h):
                bump = noise(Vector((d.x * 2.1, d.y * 2.1, h * 0.35))) * 0.45
            p = d * (r + bump)
            p.z = h
            row.append(p)
        grid.append(row)
    WALL_GRID[:] = grid
    verts = [[bm.verts.new(p) for p in row] for row in grid]
    # Seen from inside, increasing angle runs left to right: these faces are counter-clockwise
    # from the room, so they face it.
    for j in range(len(WALL_H) - 1):
        for k in range(WALL_SEGS):
            k1 = (k + 1) % WALL_SEGS
            bm.faces.new((verts[j][k], verts[j][k1], verts[j + 1][k1], verts[j + 1][k]))
    apex = bm.verts.new((0, 0, WALL_TOP))
    top = verts[-1]
    for k in range(WALL_SEGS):
        bm.faces.new((top[k], top[(k + 1) % WALL_SEGS], apex))

    def albedo(p, n):
        stone = (0.44, 0.34, 0.46)  # den plum stone
        high = (0.52, 0.42, 0.50)
        strata = 1.0 + 0.09 * math.sin(p.z * 2.6 + noise(p * 0.5) * 2.0)  # faint rock layers
        c = mix(stone, high, min(1.0, max(0.0, p.z) / 7.0))
        return jitter(tuple(v * strata for v in c), 0.18, p, 0.9)
    add("walls", bm, albedo)


def wall_point(a, h, inset):
    """A point on the wall surface (bilinear over the grid), moved `inset` toward the room."""
    u = ((a + math.pi) / (2 * math.pi)) * WALL_SEGS
    k0 = int(math.floor(u)) % WALL_SEGS
    f = u - math.floor(u)
    j = next((i for i in range(len(WALL_H) - 1) if WALL_H[i + 1] >= h), len(WALL_H) - 2)
    g = (h - WALL_H[j]) / (WALL_H[j + 1] - WALL_H[j])
    k1 = (k0 + 1) % WALL_SEGS
    lo = WALL_GRID[j][k0].lerp(WALL_GRID[j][k1], f)
    hi = WALL_GRID[j + 1][k0].lerp(WALL_GRID[j + 1][k1], f)
    p = lo.lerp(hi, g)
    return p - Vector((p.x, p.y, 0)).normalized() * inset


def skylight():
    """The round opening in the back wall: the sky (emissive) inside a stone rim."""
    segs = 12
    r_wall = wall_point(SKY_A, SKY_Z, 0).to_2d().length

    def on_wall(rad, theta, inset):
        return wall_point(SKY_A + rad * math.cos(theta) / r_wall, SKY_Z + rad * math.sin(theta), inset)
    # Seen from the room, theta runs counter-clockwise: the faces face the room.
    bm = bmesh.new()
    centre = bm.verts.new(on_wall(0, 0, 0.12))
    ring = [bm.verts.new(on_wall(SKY_R, 2 * math.pi * k / segs, 0.12)) for k in range(segs)]
    for k in range(segs):
        bm.faces.new((centre, ring[k], ring[(k + 1) % segs]))

    def sky(p, s):
        t = max(0.0, min(1.0, (p.z - (SKY_Z - SKY_R)) / (2 * SKY_R)))
        return mix(SKY_LOW[s], SKY_TOP[s], t)
    add("sky", bm, None, sky)
    bm = bmesh.new()
    inner = [bm.verts.new(on_wall(SKY_R, 2 * math.pi * k / segs, 0.14)) for k in range(segs)]
    outer = [bm.verts.new(on_wall(SKY_R + 0.42, 2 * math.pi * k / segs, 0.34)) for k in range(segs)]
    back = [bm.verts.new(on_wall(SKY_R + 0.62, 2 * math.pi * k / segs, 0.0)) for k in range(segs)]
    for k in range(segs):
        k1 = (k + 1) % segs
        bm.faces.new((inner[k], outer[k], outer[k1], inner[k1]))
        bm.faces.new((outer[k], back[k], back[k1], outer[k1]))
    add("sky_rim", bm, lambda p, n: jitter((0.46, 0.38, 0.46), 0.15, p, 2.0))
    return on_wall(0, 0, 0.12)


# ------------------------------------------------------------------------------ props
def torus(bm, centre, major, minor, segs=16, sides=6, squash=0.6, lumpy=0.12):
    rings = []
    for i in range(segs):
        a = 2 * math.pi * i / segs
        d = Vector((math.cos(a), math.sin(a), 0))
        ring = []
        for j in range(sides):
            b = 2 * math.pi * j / sides
            lump = 1 + lumpy * noise(centre * 0.3 + d * 3 + Vector((0, 0, j)))
            p = centre + d * (major + math.cos(b) * minor * lump) + Vector(
                (0, 0, (math.sin(b) * squash + squash) * minor * lump))
            ring.append(bm.verts.new(p))
        rings.append(ring)
    for i in range(segs):
        a, b = rings[i], rings[(i + 1) % segs]
        for j in range(sides):
            bm.faces.new((a[j], b[j], b[(j + 1) % sides], a[(j + 1) % sides]))


def nests():
    def straw(p, n):
        return jitter((0.84, 0.70, 0.40), 0.2, p, 4.0)
    bm = bmesh.new()
    torus(bm, NEST, 1.5, 0.45, 10, 5)
    ring_disc(bm, NEST, [1.45], 10, 0.06)
    add("nest", bm, straw)
    bm = bmesh.new()
    torus(bm, EGG_NEST, 0.85, 0.28, 10, 4)
    ring_disc(bm, EGG_NEST, [0.85], 10, 0.05)
    add("egg_nest", bm, straw)
    bm = bmesh.new()
    torus(bm, EGG_NEST_2, 0.85, 0.28, 10, 4)
    ring_disc(bm, EGG_NEST_2, [0.85], 10, 0.05)
    add("egg_nest_2", bm, straw)
    for i, at in enumerate(BEDS, 1):  # a bed for each of the other den dragons
        bm = bmesh.new()
        torus(bm, at, 1.25, 0.36, 10, 4)
        ring_disc(bm, at, [1.2], 10, 0.05)
        add(f"bed_{i}", bm, straw)


def box(bm, lo, hi):
    v = [bm.verts.new((x, y, z)) for x in (lo[0], hi[0]) for y in (lo[1], hi[1]) for z in (lo[2], hi[2])]
    for f in ((0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)):
        bm.faces.new([v[i] for i in f])


def cylinder(bm, centre, r, h, segs):
    lo = [bm.verts.new(centre + Vector((math.cos(2 * math.pi * k / segs) * r, math.sin(2 * math.pi * k / segs) * r, 0)))
          for k in range(segs)]
    hi = [bm.verts.new(centre + Vector((math.cos(2 * math.pi * k / segs) * r, math.sin(2 * math.pi * k / segs) * r, h)))
          for k in range(segs)]
    for k in range(segs):
        bm.faces.new((lo[k], lo[(k + 1) % segs], hi[(k + 1) % segs], hi[k]))
    bm.faces.new(hi)


def rock(bm, centre, radius, stretch=(1.0, 1.0, 0.8), seed=0.0):
    """A low-poly boulder: an icosahedron, stretched and lumped."""
    geom = bmesh.ops.create_icosphere(bm, subdivisions=1, radius=radius, matrix=Matrix.Translation(centre))
    for v in geom["verts"]:
        d = v.co - centre
        lump = 1 + 0.18 * noise(d * 1.7 + Vector((seed, seed * 2, 0)))
        v.co = centre + Vector((d.x * stretch[0], d.y * stretch[1], d.z * stretch[2])) * lump


def hearth():
    bm = bmesh.new()
    for k in range(8):
        a = 2 * math.pi * k / 8
        rock(bm, HEARTH + Vector((math.cos(a) * 1.05, math.sin(a) * 1.05, 0.16)), 0.34, (1.0, 1.0, 0.8), k)
    add("hearth_stones", bm, lambda p, n: jitter((0.40, 0.36, 0.40), 0.2, p, 2.0))
    bm = bmesh.new()
    for ang in (0.3, 1.9, 3.4):
        g = bmesh.ops.create_cube(bm, size=1.0)
        for v in g["verts"]:
            v.co = Vector((v.co.x, v.co.y * 0.18, v.co.z * 0.17 + 0.14))
        bmesh.ops.rotate(bm, verts=g["verts"], cent=(0, 0, 0), matrix=Matrix.Rotation(ang, 3, "Z"))
        bmesh.ops.translate(bm, verts=g["verts"], vec=HEARTH + Vector((math.cos(ang), math.sin(ang), 0)) * 0.2)
    add("logs", bm, lambda p, n: (0.36, 0.22, 0.14))
    bm = bmesh.new()
    ring_disc(bm, HEARTH, [0.8], 12, 0.07)
    add("embers", bm, None, lambda p, s: mix(EMBER[s], (1.0, 0.85, 0.4), max(0.0, 1.0 - (p - HEARTH).length / 0.8) * 0.6))


def hoard():
    bm = bmesh.new()
    geom = bmesh.ops.create_uvsphere(bm, u_segments=12, v_segments=6, radius=1.0, matrix=Matrix.Translation(HOARD))
    for v in [v for v in geom["verts"] if v.co.z < HOARD.z - 0.01]:
        bm.verts.remove(v)
    for v in bm.verts:
        v.co.z *= 0.55
        v.co.x = HOARD.x + (v.co.x - HOARD.x) * 1.4
        v.co.z += 0.08 * noise(v.co * 3)
    add("hoard", bm, lambda p, n: jitter((0.98, 0.78, 0.30), 0.18, p, 5.0))


def shelves():
    """Two plank shelves with jars, fitted to the back-left wall."""
    at = wall_point(SHELF_A, 2.2, 0.0)
    at.z = 0
    face = Matrix.Rotation(-SHELF_A, 3, "Z")  # local +Y (the back) points at the wall
    offset = at - radial(SHELF_A) * 0.35

    def place(bm):
        for v in bm.verts:
            v.co = offset + face @ v.co
    bm = bmesh.new()
    for z in (1.5, 2.6):
        box(bm, (-1.3, -0.25, z), (1.3, 0.25, z + 0.12))
    place(bm)
    add("shelves", bm, lambda p, n: (0.46, 0.30, 0.18))
    bm = bmesh.new()
    jars = ((-0.9, 1.62, 0.18, 0.45), (-0.3, 1.62, 0.22, 0.34), (0.5, 1.62, 0.16, 0.5),
            (-0.6, 2.72, 0.2, 0.4), (0.4, 2.72, 0.24, 0.3))
    for dx, z, r, h in jars:
        cylinder(bm, Vector((dx, 0, z)), r, h, 5)
    place(bm)
    colours = [(0.30, 0.56, 0.58), (0.72, 0.48, 0.30), (0.52, 0.40, 0.66), (0.80, 0.68, 0.42), (0.36, 0.50, 0.36)]
    centres = [offset + face @ Vector((dx, 0, z + h * 0.5)) for dx, z, _, h in jars]

    def albedo(p, n):
        return colours[min(range(len(centres)), key=lambda k: (p - centres[k]).length)]
    add("jars", bm, albedo)


def nook():
    """A shadowy alcove: rocks close round the back and sides; it gets little light."""
    bm = bmesh.new()
    for a, r, z, size, st in ((-28, 9.0, 0.9, 1.3, (1.1, 1.0, 1.0)), (-44, 8.9, 0.8, 1.2, (1.0, 1.1, 0.9)),
                              (-60, 8.7, 0.9, 1.3, (1.0, 1.2, 1.0)), (-44, 9.2, 2.9, 1.4, (1.2, 1.0, 0.7))):
        rock(bm, radial(math.radians(a)) * r + Vector((0, 0, z)), size, st, a)
    add("nook_rocks", bm, lambda p, n: jitter((0.34, 0.28, 0.38), 0.2, p, 1.7))
    bm = bmesh.new()
    ring_disc(bm, NOOK, [1.0], 12, 0.03)  # a moss patch to lie on
    add("nook_moss", bm, lambda p, n: (0.30, 0.40, 0.30))


def boulders():
    bm = bmesh.new()
    for a, r, size in ((48, 8.7, 1.0), (108, 8.8, 0.9)):
        rock(bm, radial(math.radians(a)) * r + Vector((0, 0, size * 0.55)), size, (1.1, 1.0, 0.9), a)
    add("boulders", bm, lambda p, n: jitter((0.42, 0.34, 0.44), 0.2, p, 1.7))


def ribbon(bm, rows, cols_u, centre_of, half_width_of):
    """A strip facing the (fixed) camera: rows along its length, columns across (u in -1..1).
    Returns the vertices row by row."""
    grid = [[bm.verts.new(centre_of(t) + CAM_RIGHT * u * half_width_of(t)) for u in cols_u] for t in rows]
    for i in range(len(rows) - 1):
        for j in range(len(cols_u) - 1):
            bm.faces.new((grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]))
    return grid


def sunbeam(sky_centre):
    """Light from the skylight to the floor: additive, soft at the edges and toward the floor."""
    bm = bmesh.new()
    top, low = sky_centre - radial(SKY_A) * 0.2, SUN_SPOT + Vector((0, 0, 0.02))
    ribbon(bm, (0.0, 0.35, 0.7, 1.0), (-1.0, -0.45, 0.0, 0.45, 1.0), lambda t: top.lerp(low, t),
           lambda t: 1.0 + 0.9 * t)
    along = low - top

    def emissive(p, s):
        t = max(0.0, min(1.0, (p - top).dot(along) / along.length_squared))
        across = abs((p - top.lerp(low, t)).dot(CAM_RIGHT)) / (1.0 + 0.9 * t)
        k = (1.0 - smoothstep(0.35, 1.0, across)) * (0.95 - 0.8 * t)
        colour, strength = BEAM[s]
        return tuple(c * strength * k for c in colour)
    add("sunbeam", bm, None, emissive, ADDITIVE)


def flames():
    """Three flame tongues over the embers: additive, flickered at runtime."""
    bm = bmesh.new()
    colours = []
    for off, hw, h in ((-0.3, 0.45, 1.0), (0.05, 0.55, 1.35), (0.35, 0.4, 0.9)):  # right offset, half width, height
        base = HEARTH + CAM_RIGHT * off + Vector((0, 0, 0.1))
        ribbon(bm, (0.0, 0.45, 1.0), (-1.0, 0.0, 1.0), lambda t, b=base, hh=h: b + Vector((0, 0, t * hh)),
               lambda t, w=hw: w * (1.0 - 0.85 * t))
        colours += [(0.55, 0.20, 0.03), (1.0, 0.86, 0.50), (0.55, 0.20, 0.03),  # base: edge, core, edge
                    (0.0, 0.0, 0.0), (1.0, 0.55, 0.14), (0.0, 0.0, 0.0),        # middle
                    (0.0, 0.0, 0.0), (0.0, 0.0, 0.0), (0.0, 0.0, 0.0)]          # tip
    add("flames", bm, None, colours, ADDITIVE | FLICKER)


# ------------------------------------------------------------------------------ lighting
SKY_CENTRE = Vector()


def light_vertex(p, n, albedo, set_name):
    col = list(AMBIENT[set_name])
    lights = ((SKY_FILL[set_name], SKY_CENTRE - radial(SKY_A) * 1.5, 16.0),
              (HEARTH_LIGHT[set_name], HEARTH + Vector((0, 0, 0.8)), 8.5))
    for (lc, li), pos, rng in lights:
        d = pos - p
        wrap = 0.35 + 0.65 * max(0.0, n.dot(d.normalized()))
        fall = max(0.0, 1.0 - d.length / rng) ** 2
        for i in range(3):
            col[i] += lc[i] * li * wrap * fall
    # the pool of sun (or moon) light the skylight throws across the floor
    along = SUN_SPOT - SKY_CENTRE
    t = max(0.0, min(1.0, (p - SKY_CENTRE).dot(along) / along.length_squared))
    off = (p - SKY_CENTRE.lerp(SUN_SPOT, t)).length
    pool = math.exp(-(off / 1.7) ** 2) * (0.4 + 0.6 * max(0.0, n.dot(-along.normalized())))
    pc, pi = SUN_POOL[set_name]
    for i in range(3):
        col[i] += pc[i] * pi * pool
    ao = 0.55 + 0.45 * min(1.0, p.z / 1.4) if 0.05 < p.z < 1.4 else 1.0
    dn = (p - NOOK).length
    if dn < 3.2:
        ao *= 0.45 + 0.55 * dn / 3.2
    lit = tuple(min(1.0, albedo[i] * col[i] * ao) for i in range(3))
    r = p.to_2d().length
    if r > R and p.z < 0.01:  # the floor beyond the walls fades into the dark
        lit = mix(lit, BACKDROP[set_name], smoothstep(R, 15.0, r))
    return lit


def bake(obj, albedo, emissive):
    """Per vertex: (position, [rgb per set])."""
    me = obj.data
    me.calc_loop_triangles()
    out = []
    for v in me.vertices:
        p = obj.matrix_world @ v.co
        n = v.normal.copy()
        cols = []
        for s in SETS:
            if isinstance(emissive, list):
                cols.append(tuple(min(1.0, c * FIRE[s]) for c in emissive[v.index]))
                continue
            e = emissive(p, s) if emissive else None
            cols.append(e if e is not None else light_vertex(p, n, albedo(p, n), s))
        out.append((p, cols))
    return out, [tuple(t.vertices) for t in me.loop_triangles]


# ------------------------------------------------------------------------------ output
def write_esm(path, baked):
    """.esm v1 (src/core/static_mesh.cpp), little-endian:
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


# ------------------------------------------------------------------------------ preview
def vertex_colour_material(additive):
    mat = bpy.data.materials.new("add" if additive else "vc")
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


def tint_dragon_materials(part_names, tints):
    """Multiply each dragon material's final emission colour by a shared tint (time of day)."""
    done = set()
    for obj in bpy.data.objects:
        if obj.type != "MESH" or obj.name in part_names:
            continue
        for slot in obj.material_slots:
            mat = slot.material
            if not mat or mat.name in done or not mat.use_nodes:
                continue
            done.add(mat.name)
            nt = mat.node_tree
            for em in [n for n in nt.nodes if n.type == "EMISSION"]:
                mul = nt.nodes.new("ShaderNodeMix")
                mul.data_type = "RGBA"
                mul.blend_type = "MULTIPLY"
                mul.inputs["Factor"].default_value = 1.0
                if em.inputs["Color"].links:
                    nt.links.new(em.inputs["Color"].links[0].from_socket, mul.inputs["A"])
                else:
                    mul.inputs["A"].default_value = em.inputs["Color"].default_value
                tint = nt.nodes.new("ShaderNodeRGB")
                tints.append(tint)
                nt.links.new(tint.outputs[0], mul.inputs["B"])
                nt.links.new(mul.outputs["Result"], em.inputs["Color"])


def place_dragons(specs):
    """Real dragons posed with the game's clips: (breed, form, stage, (x, y), heading, clip, time)."""
    here = os.path.dirname(os.path.abspath(__file__))
    for p in (here, os.path.join(here, "..", "anim")):
        if p not in sys.path:
            sys.path.insert(0, p)
    import clips as clip_lib  # noqa: E402
    import dragon_model as dm  # noqa: E402
    for breed, form, stage, at, heading, clip_name, when in specs:
        before = set(bpy.data.objects)
        d = dm.build_dragon(breed, form)
        _, t_growth = dm.STAGE[stage]
        dm.pose_stage(d, t_growth, d["breed"]["build"])
        arm = d["arm"]
        idle = {pb.name: pb.rotation_euler.to_quaternion() for pb in arm.pose.bones}
        rest = {b.name: b.matrix_local.to_quaternion() for b in arm.data.bones}
        clip = next(c for c in clip_lib.CLIPS if c.name == clip_name)
        for pb in arm.pose.bones:
            q = Quaternion(clip.sample_q(pb.name, when))
            pb.rotation_mode = "QUATERNION"
            pb.rotation_quaternion = idle[pb.name] @ (rest[pb.name].conjugated() @ q @ rest[pb.name])
        # Move the dragon by its root (the armature; the body and wings are its children). Parts
        # follow their bones through Child Of constraints, so they must not be parented too.
        holder = bpy.data.objects.new(f"place_{breed}", None)
        bpy.context.collection.objects.link(holder)
        for o in set(bpy.data.objects) - before - {holder}:
            if o.parent is None and not o.constraints:
                o.parent = holder
        holder.rotation_euler = (0, 0, heading)
        holder.location = (at[0], at[1], 0)
        bpy.context.view_layer.update()
        ev = d["body"].evaluated_get(bpy.context.evaluated_depsgraph_get())
        me = ev.to_mesh()
        low = min((ev.matrix_world @ v.co).z for v in me.vertices)
        ev.to_mesh_clear()
        holder.location.z = -low
    bpy.context.view_layer.update()


def game_camera(cam, mid, radius):
    """render3d.cpp drawDen: target over the dragons' middle, distance from the framing radius."""
    target = Vector((mid[0], mid[1], radius * 0.62))
    dist = radius / math.tan(FOV_Y * 0.5) * 0.95
    cam.location = target + CAM_DIR * dist
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()


ADULT_R, HATCH_R = 4.03, 1.28  # framing radii (render3d.cpp framingRadius: adult, hatchling stage)

SHOTS = [
    # name, dragons (breed, form, stage, pos, heading, clip, time), framing radius (None: as for 3)
    ("home", [("ember", "grown", "adult", (0.0, 0.6), 0.0, "idle", 0.0)], ADULT_R),
    ("nest", [("tide", "grown", "adult", (4.4, 3.6), -1.9, "sleep", 0.5)], ADULT_R),  # side-on
    ("nook", [("gale", "grown", "adult", (NOOK.x, NOOK.y), math.pi, "sulk_loop", 0.5)], ADULT_R),
    ("baby", [("ember", "hatchling", "hatchling", (0.0, 0.6), 0.3, "idle", 0.0)], 0.8 * HATCH_R + 0.2 * ADULT_R),
    ("three", [("ember", "grown", "adult", (0.0, 0.6), 0.2, "idle", 0.0),
               ("tide", "grown", "adult", (-3.4, 2.8), -0.6, "sit_loop", 0.3),
               ("gale", "grown", "adult", (3.2, -0.6), 0.9, "walk", 0.2)], None),
]


def preview(baked, prefix):
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_EEVEE"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x, scene.render.resolution_y = 800, 480
    scene.view_settings.view_transform = "Standard"
    world = bpy.data.worlds.new("w")
    scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes["Background"]
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    bpy.context.collection.objects.link(cam)
    scene.camera = cam
    cam.data.sensor_fit = "VERTICAL"
    cam.data.angle_y = FOV_Y
    cam.data.clip_start, cam.data.clip_end = 0.05, 200
    mats = {False: vertex_colour_material(False), True: vertex_colour_material(True)}
    part_names = {p[0] for p in PARTS}
    for name, obj, _, _, flags in PARTS:
        obj.data.materials.clear()
        obj.data.materials.append(mats[bool(flags & ADDITIVE)])
    by_name = {n: v for n, _, v, _ in baked}
    shots = SHOTS if WITH_DRAGONS else [("room", [], None)]
    for shot, specs, radius in [s for s in shots if ONLY_SHOT in (None, s[0])]:
        for o in [o for o in bpy.data.objects if o.name not in part_names and o.name != "cam"]:
            bpy.data.objects.remove(o, do_unlink=True)
        tints = []
        if specs:
            place_dragons(specs)
            tint_dragon_materials(part_names, tints)
            bpy.ops.object.light_add(type="SUN", rotation=(math.radians(48), math.radians(12), math.radians(-38)))
            bpy.context.object.data.energy = 4.0
            mid = (sum(s[3][0] for s in specs) / len(specs), sum(s[3][1] for s in specs) / len(specs))
            if radius is None:
                spread = max(math.hypot(s[3][0] - mid[0], s[3][1] - mid[1]) for s in specs)
                radius = ADULT_R + spread * 0.9
            game_camera(cam, mid, radius)
        else:
            game_camera(cam, (0.0, 1.5), 7.5)
        for si, s in enumerate(SETS):
            bg.inputs["Color"].default_value = (*[c ** 2.2 for c in BACKDROP[s]], 1)
            for tint in tints:
                tint.outputs[0].default_value = (*[c ** 2.2 for c in DRAGON_TINT[s]], 1)
            for name, obj, _, _, _ in PARTS:
                me = obj.data
                if "vc" in me.color_attributes:
                    me.color_attributes.remove(me.color_attributes["vc"])
                ca = me.color_attributes.new("vc", "FLOAT_COLOR", "POINT")
                for i, (_, cols) in enumerate(by_name[name]):
                    ca.data[i].color = (*[c ** 2.2 for c in cols[si]], 1.0)  # vertex colours are sRGB
            scene.render.filepath = f"{prefix}_{shot}_{s}.png"
            bpy.ops.render.render(write_still=True)


def main():
    global SKY_CENTRE
    bpy.ops.wm.read_factory_settings(use_empty=True)
    floor()
    walls()
    SKY_CENTRE = skylight()
    # the rug is a prop now (core/prop_mesh: lit like the dragons, and swapped for a bought one)
    nests()
    hearth()
    hoard()
    shelves()
    nook()
    boulders()
    sunbeam(SKY_CENTRE)
    flames()
    bpy.context.view_layer.update()
    baked, tris = [], 0
    # opaque meshes first, then the additive ones (drawn after the dragons)
    for name, obj, albedo, emissive, flags in sorted(PARTS, key=lambda p: p[4] & ADDITIVE):
        verts, t = bake(obj, albedo, emissive)
        baked.append((name, flags, verts, t))
        tris += len(t)
        print(f"[den]   {name:14s} {len(t):5d} tris {len(verts):5d} verts")
    print(f"[den] {len(baked)} meshes, {tris} triangles, {sum(len(v) for _, _, v, _ in baked)} vertices; "
          f"skylight at ({SKY_CENTRE.x:.2f}, {SKY_CENTRE.y:.2f}, {SKY_CENTRE.z:.2f})")
    if OUT:
        print(f"[den] {OUT}: {write_esm(OUT, baked)} bytes")
    if RENDER:
        preview(baked, RENDER)


main()
