"""The Glimmermoth (Lumen, rare, Dragondex 7): a moth dragon of light, one of the two rare base
breeds. Concept: docs/art/concept/dragons/dragon_glimmermoth.jpg (reference only, D74).

The baby is a pom-pom: a round fuzzy body under a big round head, huge dark shiny eyes, a big
fluffy collar round its neck, two tiny feathery antennae and four little rounded wing buds,
stubby legs and a short tail with a fluff at its end.

The adult is slender and graceful, like a fawn with wings: long legs, a long upright neck
rising from a big fluffy collar ruff, a small round head with a short soft muzzle and big dark
eyes, and two long feathery antennae swept up and back like a pair of fern fronds. Four broad
moth wings (on plans/glimmermoth.py, which poses them as stiff plates): long forewings with a
coloured leading edge, a pale margin and a sun-ring eyespot (a ring round a softly glowing
centre); rounded hindwings with a smaller eyespot and a long trailing luna-moth tail. At rest
the wings fold back over the body into a moth's roof, the hindwings tucked under the
forewings, the tails trailing along the tail, and the idle fans them gently. A long thin tail
ends in a soft fluffy tuft. Luminous, ethereal, a little majestic; never menacing.

Its colours (the common three): Lumen (cream, soft gold and butter), Rosy Maple (butter
yellow with rose-pink wings and points, like the rosy maple moth) and Luna (moon-white and
pale green with lavender points and a lavender leading edge, like the luna moth). The rare
Sunburst: a warm amber body with glowing sun bands, flame-orange wings whose edges and
eyespots glow strongly, longer hindwing tails and a crown of glowing sun-ray plumes behind the
antennae.

The body's texture: R soft moth bands round the abdomen and tail (Lumen's gold stripes), G the
points (lower legs, tail tip, the end of the muzzle: Rosy Maple's and Luna's legs), B the
rare variant's glowing sun bands and rings. The wings aren't textured (the game paints them
by vertex colour): their bands are faces in the membrane, margin, leading-edge and root
colours, and the eyespots separate thin discs.
"""
import math

from dragons.plans import glimmermoth as plan


def _mirrored(center, sides):
    nodes = dict(center)
    for name, (p, r) in sides.items():
        nodes[f"{name}_R"] = ((p[0], p[1], p[2]), r)
        nodes[f"{name}_L"] = ((-p[0], p[1], p[2]), r)
    return nodes


META = dict(
    name="glimmermoth", title="Glimmermoth", dex=7, element="Lumen", parents=(), rarity="rare",
    plan="glimmermoth", size=0.85,
    stats=dict(wing=8, wit=7, might=3, breath=9, stamina=7),
    manners=("Gentle", "Curious", "Shy", "Proud"),
    traits=("Sunbather", "Early Riser", "Skydancer", "Glowheart", "Songbird", "Sunkissed", "Starborn"),
    rare_variant=3, rare_replaces=False,
    blurb="Drawn to every light it sees; on still evenings its wings glow softly like paper lanterns.",
)

VARIANTS = [
    dict(name="Lumen", base=(1.0, 0.85, 0.54), accent=(1.0, 0.96, 0.84), pattern=(0.93, 0.62, 0.22),
         horn=(0.98, 0.78, 0.38), membrane=(1.0, 0.85, 0.46), iris=(0.075, 0.042, 0.026), glow=(1.0, 0.84, 0.40),
         pupil=(0.04, 0.02, 0.02), pattern_channel="r", glow_channel=None),
    dict(name="Rosy Maple", base=(1.0, 0.9, 0.4), accent=(1.0, 0.97, 0.76), pattern=(0.92, 0.42, 0.56),
         horn=(0.95, 0.56, 0.62), membrane=(0.97, 0.60, 0.68), iris=(0.085, 0.03, 0.045), glow=(1.0, 0.88, 0.52),
         pupil=(0.05, 0.02, 0.03), pattern_channel="g", glow_channel=None),
    dict(name="Luna", base=(0.84, 0.95, 0.82), accent=(0.97, 1.0, 0.94), pattern=(0.60, 0.44, 0.66),
         horn=(0.93, 0.86, 0.58), membrane=(0.66, 0.92, 0.66), iris=(0.035, 0.05, 0.075), glow=(0.86, 1.0, 0.70),
         pupil=(0.02, 0.03, 0.04), pattern_channel="g", glow_channel=None),
    dict(name="Sunburst", base=(1.0, 0.66, 0.30), accent=(1.0, 0.90, 0.60), pattern=(1.0, 0.86, 0.36),
         horn=(1.0, 0.80, 0.36), membrane=(0.97, 0.42, 0.20), iris=(0.1, 0.035, 0.012), glow=(1.0, 0.90, 0.44),
         pupil=(0.05, 0.02, 0.01), pattern_channel=None, glow_channel="b"),
]

EGG = dict(height=1.0, width=0.37, asym=0.12, point=1.0, speckle="rings",
           speckle_params=dict(levels=[0.22, 0.42, 0.905], size=(0.014, 0.03)),
           colors=[((1.0, 0.96, 0.86), (0.96, 0.76, 0.34)), ((1.0, 0.92, 0.80), (0.94, 0.52, 0.62)),
                   ((0.93, 0.98, 0.90), (0.66, 0.52, 0.74)), ((0.98, 0.66, 0.36), (1.0, 0.95, 0.68))])


# ------------------------------------------------------------------------------ wings
# Each plate is a fan from the root: rays (angle in the layout, radius) from its leading edge
# round to its inner edge (plan.FORE_EDGES / HIND_EDGES), and the hindwing's tail. Adult units.
def _pt(theta, r):
    t = math.radians(theta)
    return (r * math.cos(t), r * math.sin(t))


FORE_RAYS = [(-12.0, 1.84), (-8.0, 2.04), (-2.0, 2.10), (5.0, 2.04), (12.0, 1.92), (19.0, 1.76),
             (26.0, 1.60), (34.0, 1.44)]
HIND_RAYS = [(14.0, 0.92), (24.0, 1.16), (35.0, 1.32), (47.0, 1.38), (58.0, 1.33), (68.0, 1.22),
             (79.0, 1.10), (88.0, 0.94), (96.0, 0.70)]
HIND_TAIL = dict(base=(5, 6), length=1.5, bend=16.0, widths=(0.12, 0.1, 0.09, 0.11, 0.15, 0.13), heading=76.0)
BUD_FORE = [(-12.0, 0.34), (-6.0, 0.41), (2.0, 0.445), (10.0, 0.44), (18.0, 0.41), (26.0, 0.37), (34.0, 0.32)]
BUD_HIND = [(14.0, 0.24), (28.0, 0.30), (42.0, 0.33), (56.0, 0.32), (68.0, 0.30), (79.0, 0.27),
            (88.0, 0.23), (96.0, 0.17)]
BUD_TAIL = dict(base=(4, 5), length=0.17, bend=6.0, widths=(0.04, 0.036, 0.042), heading=74.0)


def _tail_line(rays, tail, grow=1.0):
    """The hindwing tail's centre line: from the middle of its base (between two outline
    rays) out along `heading`, curving back by `bend` degrees."""
    a, b = (_pt(*rays[i]) for i in tail["base"])
    base = ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2)
    n = len(tail["widths"])
    pts = [base]
    p = base
    step = tail["length"] * grow / n
    for k in range(1, n + 1):
        h = math.radians(tail["heading"] + tail["bend"] * k / n)
        p = (p[0] + step * math.cos(h), p[1] + step * math.sin(h))
        pts.append(p)
    return pts


def _layout(fore, hind, tail):
    """The wing layout (u, v) the plan's WING_CHAIN names, from the plates' shapes."""
    t = _tail_line(hind, tail)
    return {"root": (0.0, 0.0), "elbow": _pt(fore[0][0], fore[0][1] * 0.45), "apex": _pt(*fore[2]),
            "tornus": _pt(*fore[-1]), "hmid": _pt(48.0, hind[3][1] * 0.6), "tailbase": t[0], "tailtip": t[-1],
            "body": (0.02, 0.8 * hind[-1][1])}


# ------------------------------------------------------------------------------ grown
# Faces -Y, Z up; adult units (the game scales it by META size). radius = (side, vertical).
HEAD = (0.0, -1.18, 2.56)   # the adult's head: the face's features are all placed from here
NECK_BASE = (0.0, -0.76, 1.62)


def _h(dx, dy, dz):
    return (HEAD[0] + dx, HEAD[1] + dy, HEAD[2] + dz)


GROWN_NODES = _mirrored({
    "tail_tip": ((0, 3.12, 0.78), (0.026, 0.026)),
    "tail4": ((0, 2.58, 0.72), (0.05, 0.05)),
    "tail3": ((0, 1.98, 0.82), (0.082, 0.082)),
    "tail2": ((0, 1.36, 1.0), (0.13, 0.13)),
    "hips": ((0, 0.66, 1.18), (0.33, 0.34)),
    "belly": ((0, 0.06, 1.18), (0.34, 0.37)),
    "chest": ((0, -0.46, 1.24), (0.37, 0.45)),
    "neck1": (NECK_BASE, (0.23, 0.24)),
    "neck2": ((0, -0.92, 1.94), (0.18, 0.185)),
    "neck3": ((0, -1.03, 2.25), (0.16, 0.165)),
    "head": (HEAD, (0.27, 0.26)),
    "muzzle": (_h(0, -0.22, -0.07), (0.155, 0.135)),
    "snout": (_h(0, -0.37, -0.12), (0.09, 0.078)),
}, {
    "shoulder": ((0.22, -0.46, 1.02), (0.15, 0.18)),
    "elbow": ((0.245, -0.52, 0.62), (0.095, 0.1)),
    "wrist": ((0.245, -0.56, 0.21), (0.07, 0.07)),
    "toe_f": ((0.245, -0.70, 0.05), (0.078, 0.052)),
    "hipj": ((0.22, 0.68, 1.02), (0.18, 0.22)),
    "knee": ((0.255, 0.48, 0.62), (0.11, 0.11)),
    "ankle": ((0.255, 0.76, 0.26), (0.072, 0.072)),
    "toe_b": ((0.255, 0.63, 0.05), (0.078, 0.052)),
    "ant": (_h(0.08, -0.04, 0.08), (0.01, 0.01)),      # the antennae's joints, inside the head top
    "anttip": (_h(0.30, 0.16, 0.94), (0.01, 0.01)),
})
GROWN_EDGES = [("tail_tip", "tail4"), ("tail4", "tail3"), ("tail3", "tail2"), ("tail2", "hips"),
               ("hips", "belly"), ("belly", "chest"), ("chest", "neck1"), ("neck1", "neck2"),
               ("neck2", "neck3"), ("neck3", "head"), ("head", "muzzle"), ("muzzle", "snout")]
for _side in ("L", "R"):
    GROWN_EDGES += [("chest", f"shoulder_{_side}"), (f"shoulder_{_side}", f"elbow_{_side}"),
                    (f"elbow_{_side}", f"wrist_{_side}"), (f"wrist_{_side}", f"toe_f_{_side}"),
                    ("hips", f"hipj_{_side}"), (f"hipj_{_side}", f"knee_{_side}"),
                    (f"knee_{_side}", f"ankle_{_side}"), (f"ankle_{_side}", f"toe_b_{_side}")]

BUILDS = {
    "neutral": {},
    "sturdy": {"chest": (1.07, 0.98), "belly": (1.06, 0.98), "hips": (1.05, 1.0), "neck1": (1.05, 0.97),
               "leg_up": (1.06, 0.98), "arm_up": (1.06, 0.98)},
    "sleek": {"chest": (0.95, 1.02), "belly": (0.93, 1.03), "hips": (0.95, 1.0), "leg_lo": (0.95, 1.04),
              "arm_lo": (0.95, 1.04), "tail3": (0.94, 1.05)},
    "long": {"neck1": (0.97, 1.07), "neck2": (0.97, 1.08), "neck3": (0.97, 1.06), "tail2": (0.96, 1.07),
             "tail3": (0.96, 1.08), "tail4": (0.96, 1.08)},
}

# The collar ruff (both forms): a puffy scalloped shell round the base of the neck, added to
# the body mesh so it bends with the neck and takes the skin's texture. at: the neck point it
# rings; axis: up the neck; profile: (out from the skin, along the axis) from its top edge
# (tucked under the neck's skin) round to its bottom edge (tucked into the chest); lobes and
# lobe: the scallops; back: how much of it reaches round the back (where the wings grow).
COLLAR = {
    "grown": dict(at=NECK_BASE, axis=(0.0, -0.44, 0.9), seg=18,
                  profile=[(-0.03, 0.2), (0.09, 0.15), (0.19, 0.05), (0.23, -0.06), (0.2, -0.16), (0.1, -0.22),
                           (-0.04, -0.24)],
                  lobes=7, lobe=0.3, back=0.4),
    "hatchling": dict(at=(0.0, -0.27, 0.70), axis=(0.0, -0.36, 0.93), seg=16,
                      profile=[(-0.03, 0.1), (0.05, 0.08), (0.1, 0.02), (0.115, -0.05), (0.09, -0.11),
                               (0.04, -0.15), (-0.03, -0.16)],
                      lobes=8, lobe=0.45, back=0.2),
}


def _add_collar(kit, bm, spec):
    """The collar shell: rings of points round the neck axis, each point `out` from the
    body's own surface (found by casting from the axis), scalloped by lobes that hang lower."""
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree
    bvh = BVHTree.FromBMesh(bm)
    c, ax = Vector(spec["at"]), Vector(spec["axis"]).normalized()
    ref = Vector((0, -1, 0))
    u = (ref - ax * ref.dot(ax)).normalized()     # forward, round the axis
    w = ax.cross(u).normalized()
    seg = kit.lod(spec["seg"], max(8, spec["seg"] // 2))
    prof = spec["profile"] if not kit.LOD else [spec["profile"][i] for i in (0, 2, 4, 6)]
    rings = []
    for j in range(seg):
        a = 2 * math.pi * j / seg
        d = u * math.cos(a) + w * math.sin(a)
        front = 0.5 + 0.5 * math.cos(a)           # 1 in front, 0 behind
        size = spec["back"] + (1 - spec["back"]) * front ** 0.7
        lobe = math.cos(spec["lobes"] * a)
        ring = []
        for k, (out, h) in enumerate(prof):
            origin = c + ax * h
            hit = bvh.ray_cast(origin, d, 2.0)[0]
            skin = (hit - origin).length if hit is not None else 0.2
            inner = k in (0, len(prof) - 1)
            o = out if inner else out * size * (1 + spec["lobe"] * lobe)
            hh = h if inner or k < len(prof) - 3 else h - abs(h) * 0.22 * spec["lobe"] * (1 + lobe) * size
            ring.append(bm.verts.new(origin + ax * (hh - h) + d * (skin + o)))
        rings.append(ring)
    faces = []
    for j in range(seg):
        a, b = rings[j], rings[(j + 1) % seg]
        for k in range(len(prof) - 1):
            faces.append(bm.faces.new((a[k], a[k + 1], b[k + 1], b[k])))
    for f in faces:
        f.normal_update()
        mid = f.calc_center_median()
        if f.normal.dot(mid - (c + ax * (mid - c).dot(ax))) < 0:
            f.normal_flip()


def _collar_on(kit, obj, spec):
    """Decimate the body to its budget less the collar's triangles, then add the collar, so
    the collar keeps its full puffy shape (the kit's decimation then has nothing left to do)."""
    import bmesh
    seg = kit.lod(spec["seg"], max(8, spec["seg"] // 2))
    rows = kit.lod(len(spec["profile"]) - 1, 3)
    kit.decimate_to(obj, kit.lod(kit.F["body_tris"], kit.F["body_tris_lod1"]) - 2 * seg * rows)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    _add_collar(kit, bm, spec)
    bm.to_mesh(obj.data)
    bm.free()


def _hatch_sculpt(kit, obj):
    """The baby's fluffy collar."""
    _collar_on(kit, obj, COLLAR["hatchling"])


def _pieces(me):
    """Vertex index sets of a mesh's connected pieces, largest first."""
    adj = {v.index: [] for v in me.vertices}
    for e in me.edges:
        a, b = e.vertices
        adj[a].append(b)
        adj[b].append(a)
    seen, out = set(), []
    for v in adj:
        if v in seen:
            continue
        stack, piece = [v], set()
        seen.add(v)
        while stack:
            a = stack.pop()
            piece.add(a)
            for b in adj[a]:
                if b not in seen:
                    seen.add(b)
                    stack.append(b)
        out.append(piece)
    return sorted(out, key=len, reverse=True)


def _grown_sculpt(kit, obj):
    """Drop the stray skin blobs the antenna joints make (only the body is kept), add the
    collar ruff, then the shapes the node graph can't give: a deeper chest over a tucked
    belly, round cheeks, a soft muzzle."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    _keep_largest(bm)
    hy, hz = HEAD[1], HEAD[2]
    for v in bm.verts:
        x, y, z = v.co
        if -0.85 < y < -0.2 and z < 1.0 and abs(x) < 0.32:  # the chest keel
            kk = (1 - abs(x) / 0.32) * max(0.0, (1.0 - z) / 0.4)
            v.co.z -= 0.05 * kk
        if 0.2 < y < 0.8 and z < 0.96 and abs(x) < 0.28:  # a tucked belly
            kk = (1 - abs(x) / 0.28) * max(0.0, (0.96 - z) / 0.3)
            v.co.z += 0.05 * kk
        if hy - 0.2 < y < hy + 0.12 and hz - 0.16 < z < hz + 0.1:  # round cheeks
            v.co.x *= 1.08
        if y < hy - 0.24 and z > hz - 0.08:  # the muzzle a little flatter on top
            v.co.z -= 0.02 * min(1.0, (hy - 0.24 - y) / 0.2)
    bm.to_mesh(obj.data)
    bm.free()
    _collar_on(kit, obj, COLLAR["grown"])


def _keep_largest(bm):
    """Delete every piece of the mesh but the largest."""
    import bmesh
    seen, pieces = set(), []
    for v in bm.verts:
        if v in seen:
            continue
        stack, piece = [v], []
        seen.add(v)
        while stack:
            a = stack.pop()
            piece.append(a)
            for e in a.link_edges:
                b = e.other_vert(a)
                if b not in seen:
                    seen.add(b)
                    stack.append(b)
        pieces.append(piece)
    pieces.sort(key=len)
    drop = [v for piece in pieces[:-1] for v in piece]
    if drop:
        bmesh.ops.delete(bm, geom=drop, context="VERTS")


GROWN = dict(
    name="grown", nodes=GROWN_NODES, edges=GROWN_EDGES, body="skin",
    body_tris=1480, body_tris_lod1=560, export_scale=1.0,
    young={
        "bones": {
            "head": (0.95, 0.9, 1.0), "snout": (0.9, 0.72, 0.95),
            "neck1": (0.8, 0.55), "neck2": (0.8, 0.52), "neck3": (0.82, 0.52),
            "chest": (0.72, 0.56), "belly": (0.7, 0.54), "hips": (0.72, 0.56),
            "tail1": (0.72, 0.52), "tail2": (0.74, 0.5), "tail3": (0.78, 0.5), "tail4": (0.84, 0.54),
            "arm_up": (0.74, 0.58), "arm_lo": (0.74, 0.58), "hand": (0.82, 0.74),
            "leg_up": (0.74, 0.58), "leg_lo": (0.74, 0.56), "foot": (0.82, 0.74),
        },
        "parts": {"eyes": 1.3, "horns": 0.62, "frill": 0.8, "wings": 0.5, "spikes": 0.6,
                  "tail_tip": 0.75, "heart": 0.85, "runes": 0.7},
    },
    young_pose={"neck1": -14, "neck2": -6, "neck3": 4, "head": 14},
    base_pose={"neck1": (2, 0, 0), "neck2": (4, 0, 0), "neck3": (6, 0, 0), "head": (-6, 0, 0),
               "tail1": (6, 0, 0), "tail2": (2, 0, 4), "tail3": (4, 0, 7), "tail4": (8, 0, 9)},
    builds=BUILDS,
    eyes=dict(at=_h(0.165, -0.17, 0.03), out=(0.6, -0.78, 0.1), iris=(0.108, 0.118, 0.052),
              pupil=(0.082, 0.094, 0.022), slit=(0.32, 1.1),
              glints=((-0.026, 0.036, 0.02), (0.018, -0.036, 0.009)), seg=(12, 2, 12, 2)),
    head=dict(origin=HEAD, k=1.0, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=False,
              frill_k=1.0, feather_w=1.0),
    tail_k=1.0,
    heart=dict(at=(0, -0.9, 1.04), size=0.1),
    wing=dict(root=(0.21, -0.30, 1.62), scale=1.0, dihedral=plan.DIHEDRAL, droop=plan.DROOP,
              radii={"root": 0.05, "elbow": 0.04, "wrist": 0.03, "finger": 0.02, "tip": 0.01},
              thickness=0.014, style="moth", layout=_layout(FORE_RAYS, HIND_RAYS, HIND_TAIL),
              fore=FORE_RAYS, hind=HIND_RAYS, tail=HIND_TAIL, rings=(0.3, 0.62, 0.9), rings_lod1=(0.5, 0.88),
              spots=dict(fore=(12.0, 1.24, 0.2, 0.13, 0.07), hind=(45.0, 0.86, 0.15, 0.095, 0.05)),
              under=0.03, lock=dict(fore=0.64, hind=0.52)),
    mask=dict(max_x=0.28, max_z=1.8, min_z=-1.0, tail_cut=(1.2, 0.9)),
    inset={"eyes": 0.022, "horns": 0.035, "spikes": 0.03, "frill": 0.04, "heart": -0.05, "runes": -0.012,
           "tail_tip": 0.03},
    face=dict(nostril=_h(0.036, -0.45, -0.105), nostril_r=(0.018, 0.012, 0.007), mouth_r=0.009,
              mouth=lambda side, a: (side * 0.1 * a ** 0.7, HEAD[1] - 0.46 + 0.25 * a ** 1.5,
                                     HEAD[2] - 0.175 + 0.05 * a * a)),
    jaw_hinge=_h(0, -0.18, -0.16),
    teeth=False,
    mouth_detail=dict(depth=0.2, fade=0.15, width=0.26, tooth=(0.008, 0.014), fang=(0.01, 0.02),
                      tongue=(0.028, 0.05, 0.008)),
    skin=dict(stripe=0.26, spot_cell=0.22, ao=0.5),
    sculpt=_grown_sculpt,
)

# ------------------------------------------------------------------------------ hatchling
HATCH_NODES = _mirrored({
    "tail_tip": ((0, 0.9, 0.55), None),
    "tail4": ((0, 0.77, 0.47), None),
    "tail3": ((0, 0.63, 0.42), None),
    "tail2": ((0, 0.49, 0.41), None),
    "hips": ((0, 0.26, 0.45), None),
    "belly": ((0, 0.04, 0.46), None),
    "chest": ((0, -0.15, 0.52), None),
    "neck1": ((0, -0.25, 0.64), None),
    "neck2": ((0, -0.31, 0.76), None),
    "neck3": ((0, -0.36, 0.88), None),
    "head": ((0, -0.42, 1.02), None),
    "muzzle": ((0, -0.66, 0.95), None),
    "snout": ((0, -0.78, 0.92), None),
}, {
    "shoulder": ((0.17, -0.14, 0.40), None),
    "elbow": ((0.18, -0.18, 0.24), None),
    "wrist": ((0.19, -0.21, 0.10), None),
    "toe_f": ((0.19, -0.33, 0.05), None),
    "hipj": ((0.18, 0.28, 0.40), None),
    "knee": ((0.20, 0.22, 0.24), None),
    "ankle": ((0.20, 0.32, 0.11), None),
    "toe_b": ((0.20, 0.18, 0.05), None),
    "ant": ((0.07, -0.47, 1.16), None),
    "anttip": ((0.17, -0.40, 1.50), None),
})


HATCH_META = [
    ("ell", (0, -0.44, 1.05), (0.335, 0.30, 0.305)),   # the big round head
    ("ell", (0, -0.665, 0.95), (0.13, 0.12, 0.105)),   # a small button muzzle
    ("ell", (0, -0.635, 0.905), (0.10, 0.10, 0.065)),  # chin
    ("chain", [(0, -0.26, 0.64), (0, -0.32, 0.76), (0, -0.38, 0.9)], [0.17, 0.165, 0.16]),
    ("ell", (0, -0.14, 0.53), (0.25, 0.23, 0.25)),     # chest
    ("ell", (0, 0.07, 0.48), (0.275, 0.30, 0.28)),     # the round pom-pom tummy
    ("ell", (0, 0.28, 0.48), (0.23, 0.21, 0.23)),      # hips
    ("chain", [(0, 0.46, 0.44), (0, 0.62, 0.42), (0, 0.76, 0.46), (0, 0.88, 0.54)], [0.09, 0.07, 0.058, 0.05]),
]
for _s in (-1, 1):
    HATCH_META += [
        ("ball", (_s * 0.15, -0.64, 0.95), 0.105),       # cheeks
        ("chain", [(_s * 0.17, -0.14, 0.40), (_s * 0.18, -0.18, 0.24), (_s * 0.19, -0.21, 0.10)],
         [0.095, 0.08, 0.075]),
        ("ell", (_s * 0.19, -0.28, 0.06), (0.085, 0.11, 0.058)),    # front paws
        ("ell", (_s * 0.19, 0.28, 0.36), (0.12, 0.15, 0.16)),       # thighs
        ("chain", [(_s * 0.2, 0.30, 0.24), (_s * 0.2, 0.31, 0.11)], [0.08, 0.072]),
        ("ell", (_s * 0.2, 0.22, 0.06), (0.085, 0.115, 0.058)),     # hind paws
    ]

HATCH = dict(
    name="hatchling", nodes=HATCH_NODES, meta=HATCH_META, body="meta",
    body_tris=1560, body_tris_lod1=540, export_scale=1.0,
    young={
        "bones": {name: ((0.76, 0.76) if name in ("head", "snout") else (0.64, 0.64))
                  for name in ("hips", "belly", "chest", "neck1", "neck2", "neck3", "head", "snout", "tail1",
                               "tail2", "tail3", "tail4", "arm_up", "arm_lo", "hand", "leg_up", "leg_lo", "foot")},
        "parts": {"eyes": 1.1, "horns": 0.7, "frill": 0.9, "wings": 0.6, "spikes": 0.8,
                  "tail_tip": 0.85, "heart": 1.0, "runes": 0.9},
    },
    base_pose={"head": (4, 0, 0), "tail1": (6, 0, 0), "tail2": (-8, 0, 6), "tail3": (-10, 0, 8),
               "tail4": (-12, 0, 10)},
    builds={"neutral": {}, "sturdy": {}, "sleek": {}, "long": {}},
    eyes=dict(at=(0.155, -0.69, 1.08), out=(0.38, -0.92, 0.07), iris=(0.13, 0.145, 0.07),
              pupil=(0.1, 0.116, 0.03), slit=(0.32, 1.1),
              glints=((-0.034, 0.05, 0.032), (0.028, -0.05, 0.015)), seg=(14, 3, 14, 2)),
    head=dict(origin=(0, -0.44, 1.05), k=0.9, horn_len=0.4, horn_r=0.8, horn_curve=0.6, buds=True,
              frill_k=0.6, feather_w=1.6),
    tail_k=0.4,
    heart=dict(at=(0, -0.40, 0.50), size=0.075),
    wing=dict(root=(0.12, 0.06, 0.74), scale=1.0, dihedral=plan.DIHEDRAL, droop=plan.DROOP, set=dict(fore=34, hind=30),
              radii={"root": 0.03, "elbow": 0.03, "wrist": 0.02, "finger": 0.01, "tip": 0.005},
              thickness=0.012, style="moth", layout=_layout(BUD_FORE, BUD_HIND, BUD_TAIL),
              fore=BUD_FORE, hind=BUD_HIND, tail=BUD_TAIL, rings=(0.45, 0.82), rings_lod1=(0.8,),
              spots=dict(fore=(10.0, 0.25, 0.07, 0.0, 0.0), hind=(46.0, 0.19, 0.05, 0.0, 0.0)), under=0.018,
              lock=dict(fore=0.22, hind=0.16)),
    mask=dict(max_x=0.2, max_z=1.0, min_z=0.13, tail_cut=None),
    inset={"eyes": 0.03, "horns": 0.02, "spikes": 0.012, "frill": 0.03, "heart": -0.03, "runes": -0.01,
           "tail_tip": 0.02},
    face=dict(nostril=(0.04, -0.795, 0.945), nostril_r=(0.02, 0.013, 0.008), mouth_r=0.01,
              mouth=lambda side, a: (side * 0.075 * a ** 0.8, -0.815 + 0.1 * a ** 1.6, 0.885 + 0.03 * a * a)),
    jaw_hinge=(0, -0.62, 0.89),
    teeth=False,
    mouth_detail=dict(depth=0.13, fade=0.1, width=0.12, tooth=(0.007, 0.012), fang=(0.009, 0.02),
                      tongue=(0.036, 0.05, 0.01)),
    skin=dict(stripe=0.16, spot_cell=0.1, ao=0.22),
    sculpt=_hatch_sculpt,
)

FORMS = {"grown": GROWN, "hatchling": HATCH}


# ------------------------------------------------------------------------------ hook helpers
def _frame(kit, side):
    """The wing's rest frame on one side: root (seated), span and chord axes, the upper-face
    normal and the layout scale."""
    w = kit.F["wing"]
    s = -1 if side == "L" else 1
    th, ph = math.radians(w["dihedral"]), math.radians(w["droop"])
    span = kit.V((s * math.cos(th), 0, math.sin(th)))
    chord = kit.V((0, math.cos(ph), -math.sin(ph)))
    root = kit.mirror(w["seat"]["root"] if "seat" in w else w["root"], s)
    n = span.cross(chord).normalized()
    if n.z < 0:
        n = -n
    return root, span, chord, n, w["scale"]


def _placer(kit, side, plate):
    """Where a plate's layout points go: at(u, v, lift) in the rest pose, with the form's
    `set` (degrees the plate is turned up about its inner edge, so a folded bud stands off a
    round back). Returns at, the plate's normal and its span axis."""
    from mathutils import Matrix
    root, span, chord, n, sc = _frame(kit, side)
    s = -1 if side == "L" else 1
    edge = math.radians(plan.FORE_EDGES[1] if plate == "fore" else plan.HIND_EDGES[1])
    axis = (span * math.cos(edge) + chord * math.sin(edge)).normalized()
    rot = Matrix.Rotation(math.radians(-kit.F["wing"].get("set", {}).get(plate, 0.0) * s), 3, axis)

    def at(u, v, lift=0.0):
        return root + rot @ ((span * u + chord * v) * sc + n * lift)

    return at, rot @ n, rot @ span


def _plate(kit, name, side, plate, rays, tail, bands, mats, lift, thickness, bones):
    """A wing plate: a fan of rays from the root, cut by rings into bands, and (hindwing) a
    long tail strip from two of its outline rays. bands(sector, ring) -> material name for
    the fan's faces; the tail's are tail["mats"]. Returns the object."""
    import bmesh
    place, n, _ = _placer(kit, side, plate)

    def at(u, v):
        return place(u, v, lift)

    if kit.LOD:  # the den's far wings: every other ray (the leading edge's and the tail's kept)
        keep = sorted(set(range(0, len(rays), 2)) | {len(rays) - 1} | ({1} if plate == "fore" else set()) |
                      (set(tail["base"]) if tail else set()))
        if tail:
            tail = dict(tail, base=tuple(keep.index(i) for i in tail["base"]), widths=tail["widths"][::2])
        rays = [rays[i] for i in keep]

    rings = list(kit.lod(kit.F["wing"]["rings"], kit.F["wing"]["rings_lod1"])) + [1.0]
    names = []

    def mat(m):
        if m not in names:
            names.append(m)
        return names.index(m)

    bm = bmesh.new()
    centre = bm.verts.new(at(0.0, 0.0))
    grid = []
    for theta, r in rays:
        grid.append([bm.verts.new(at(*_pt(theta, r * f))) for f in rings])
    faces = []
    for j in range(len(rings)):
        for i in range(len(rays) - 1):
            m = bands(i, j, len(rays) - 1, len(rings))
            if j == 0:
                f = bm.faces.new((centre, grid[i][0], grid[i + 1][0]))
            else:
                f = bm.faces.new((grid[i][j - 1], grid[i][j], grid[i + 1][j], grid[i + 1][j - 1]))
            f.material_index = mat(m)
            faces.append(f)
    if tail:
        line = _tail_line(rays, tail, tail.get("grow", 1.0))
        a, b = tail["base"]
        left, right = grid[a][-1], grid[b][-1]
        prev = (left, right)
        widths = tail["widths"]
        for k in range(1, len(line) - 1):
            p, q = line[k - 1], line[k + 1]
            du, dv = q[0] - p[0], q[1] - p[1]
            ln = math.sqrt(du * du + dv * dv) or 1.0
            nu, nv = -dv / ln, du / ln
            w = widths[k]
            c = line[k]
            vl = bm.verts.new(at(c[0] - nu * w, c[1] - nv * w))
            vr = bm.verts.new(at(c[0] + nu * w, c[1] + nv * w))
            f = bm.faces.new((prev[0], prev[1], vr, vl))
            f.material_index = mat(tail["mats"][0] if k < len(line) - 2 else tail["mats"][1])
            faces.append(f)
            prev = (vl, vr)
        tip = bm.verts.new(at(*line[-1]))
        f = bm.faces.new((prev[0], prev[1], tip))
        f.material_index = mat(tail["mats"][1])
        faces.append(f)
    for f in faces:
        f.normal_update()
        if f.normal.dot(n) < 0:
            f.normal_flip()
    obj = kit.mesh_object(name, bm)
    m = obj.modifiers.new("thick", "SOLIDIFY")
    m.thickness = thickness
    m.offset = 0
    m.use_rim = False
    kit.apply_modifiers(obj)
    kit.smooth(obj)
    for nm in names:
        obj.data.materials.append(mats[nm])
    obj["struts"] = ",".join(bones)
    return obj


def _spot(kit, name, side, plate, theta, r, outer, inner, dot, lift, thickness, ring_mat, dot_mat, mats, bones):
    """A sun-ring eyespot lying on a wing plate: a ring round a glowing centre (or, with no
    inner radius, a single glowing dot), a little thicker than the membrane so it shows on
    both faces."""
    import bmesh
    place, n, a = _placer(kit, side, plate)
    sc = kit.F["wing"]["scale"]
    c = place(*_pt(theta, r), lift)
    a = a.normalized()
    b = n.cross(a).normalized()
    seg = kit.lod(10, 6)
    if kit.LOD and inner > 0:  # far away the sun-ring is just its glowing heart
        outer, inner = 0.8 * inner, 0.0
    bm = bmesh.new()
    names = []

    def ring_pts(rad):
        return [bm.verts.new(c + (a * math.cos(2 * math.pi * k / seg) + b * math.sin(2 * math.pi * k / seg)) * rad * sc)
                for k in range(seg)]

    faces = []
    if inner > 0:
        o, i = ring_pts(outer), ring_pts(inner)
        names.append(ring_mat)
        for k in range(seg):
            f = bm.faces.new((i[k], o[k], o[(k + 1) % seg], i[(k + 1) % seg]))
            f.material_index = 0
            faces.append(f)
        d = ring_pts(dot)
        names.append(dot_mat)
        cc = bm.verts.new(c)
        for k in range(seg):
            f = bm.faces.new((cc, d[k], d[(k + 1) % seg]))
            f.material_index = 1
            faces.append(f)
    else:
        d = ring_pts(outer)
        names.append(dot_mat)
        cc = bm.verts.new(c)
        for k in range(seg):
            f = bm.faces.new((cc, d[k], d[(k + 1) % seg]))
            f.material_index = 0
            faces.append(f)
    for f in faces:
        f.normal_update()
        if f.normal.dot(n) < 0:
            f.normal_flip()
    obj = kit.mesh_object(name, bm)
    m = obj.modifiers.new("thick", "SOLIDIFY")
    m.thickness = thickness
    m.offset = 0
    m.use_rim = False
    kit.apply_modifiers(obj)
    kit.smooth(obj)
    for nm in names:
        obj.data.materials.append(mats[nm])
    obj["struts"] = ",".join(bones)
    return obj


def _plate_weights(kit, obj, side):
    """Strut weights for one plate (the kit's weight_membrane weights every surface to every
    wing strut on its side, which tears two wings that move apart): each vertex follows the
    two nearest struts of its own plate's bones, by distance. An eyespot, and the membrane
    round it (obj["locks"]: centre x, y, z, radius per spot), take the weights at the spot's
    centre, so the spot and the membrane under it move as one piece however the wing flexes."""
    from mathutils import Vector
    w = kit.wing_points(side)
    allowed = obj["struts"].split(",")
    struts = [(f"{name}_{side}", w[h], w[t]) for name, h, t, _ in kit.WING_CHAIN if name in allowed]

    def gap(p, a, b):
        ab = b - a
        t = max(0.0, min(1.0, (p - a).dot(ab) / max(ab.length_squared, 1e-9)))
        return (p - (a + ab * t)).length

    def weights(p):
        (d0, n0), (d1, n1) = sorted((gap(p, a, b), name) for name, a, b in struts)[:2]
        d0, d1 = d0 ** 1.5, d1 ** 1.5
        w0 = d1 / (d0 + d1) if d0 + d1 > 1e-9 else 1.0
        return (n0, w0), (n1, 1.0 - w0)

    locks = []
    for spec in obj.get("locks", "").split(";"):
        if spec:
            x, y, z, r = (float(c) for c in spec.split(","))
            locks.append((Vector((x, y, z)), r))
    for vg in list(obj.vertex_groups):
        obj.vertex_groups.remove(vg)
    groups = {name: obj.vertex_groups.new(name=name) for name, _, _ in struts}
    for v in obj.data.vertices:
        p = obj.matrix_world @ v.co
        for c, r in locks:
            if (p - c).length < r:
                p = c
                break
        for name, wt in weights(p):
            if wt > 0.0:
                groups[name].add([v.index], wt, "REPLACE")


def _install_plate_weights(kit):
    """Surfaces carrying a "struts" list are weighted to their own plate (see above); anything
    else keeps the kit's own membrane weights."""
    if getattr(kit, "_glimmer_weights", False):
        return
    original = kit.weight_membrane

    def weight_membrane(mem, side):
        if "struts" in mem:
            return _plate_weights(kit, mem, side)
        return original(mem, side)

    kit.weight_membrane = weight_membrane
    kit._glimmer_weights = True


# ------------------------------------------------------------------------------ hooks
def wings(kit, d, rare):
    """Four moth wings: forewing and hindwing plates on each side with sun-ring eyespots. The
    common wings: a body-coloured root, the membrane, a leading edge in the pattern colour and
    a pale margin; the rare (Sunburst) wings: longer tails, glowing margins and eyespots."""
    _install_plate_weights(kit)
    F, mats = kit.F, d["mats"]
    w = F["wing"]
    baby = F["name"] == "hatchling"
    margin = "glow_flat" if rare else "accent_flat"
    ring_mat, dot_mat = ("rune", "glow_flat") if rare else ("pattern_flat", "glow_flat")
    grow = 1.0 if not rare else (1.25 if baby else 1.32)
    t, spots = w["thickness"], w["spots"]
    objs = []
    for side in ("L", "R"):
        def fore_bands(i, j, ni, nj):
            if j == 0:
                return "body_plain"
            if j == nj - 1:
                return margin
            if i == 0:
                return "pattern_flat"
            return "membrane"

        def hind_bands(i, j, ni, nj):
            if j == 0:
                return "body_plain"
            if j == nj - 1:
                return margin
            return "membrane"

        tail = dict(w["tail"], grow=grow, mats=("membrane", "glow_flat" if rare else "pattern_flat"))
        for plate, rays, tl, bands, lift, bones in (("fore", w["fore"], None, fore_bands, 0.0, plan.FOREWING),
                                                    ("hind", w["hind"], tail, hind_bands, -w["under"], plan.HINDWING)):
            th, r, outer, inner, dot = spots[plate]
            c = _placer(kit, side, plate)[0](*_pt(th, r), lift)
            wing = _plate(kit, f"{plate}wing_{side}", side, plate, rays, tl, bands, mats, lift, t, bones)
            wing["locks"] = f"{c.x},{c.y},{c.z},{w['lock'][plate]}"
            spot = _spot(kit, f"spot_{side}_{plate}", side, plate, th, r, outer * (1.15 if rare else 1.0), inner, dot,
                         lift, t + 0.012, ring_mat, dot_mat, mats, bones)
            spot["locks"] = f"{c.x},{c.y},{c.z},10.0"
            objs += [wing, spot]
    return objs


def _frond(kit, name, base, up, side, bend, length, width, teeth, curl, thickness, cup=0.25, notch=0.8):
    """A feathery antenna: a leaf-shaped frond on a midrib that curves (curl radians over its
    length) from `up` toward `bend`, `side` across its width, its edges combed into little
    barbs and folded a little along the midrib (cup) so it catches the light."""
    import bmesh
    V = kit.V
    base, up = V(base), V(up).normalized()
    bend = (V(bend) - up * V(bend).dot(up)).normalized()
    side = (V(side) - up * V(side).dot(up)).normalized()
    n = kit.lod(max(4, 2 * teeth + 1), 4)
    bm = bmesh.new()
    mids, lefts, rights = [], [], []
    for k in range(n + 1):
        t = k / n
        a = curl * t
        if curl:
            c = base + (up * (math.sin(a) / curl) + bend * ((1 - math.cos(a)) / curl)) * length
        else:
            c = base + up * t * length
        along = (up * math.cos(a) + bend * math.sin(a)).normalized()
        across = (side - along * side.dot(along)).normalized()
        face = along.cross(across).normalized()
        wdt = width * math.sin(math.pi * (0.06 + 0.94 * t)) ** 0.45 * (0.6 + 0.4 * (1 - t))
        ahead = 0.0
        if kit.LOD == 0 and 0 < k < n:
            if k % 2 == 1:
                wdt *= notch                 # the soft dips between barbs
            else:
                ahead = 0.5 * length / n     # each barb leaning toward the tip
        mids.append(bm.verts.new(c + face * wdt * cup))
        lefts.append(bm.verts.new(c - across * wdt * 0.5 + along * ahead))
        rights.append(bm.verts.new(c + across * wdt * 0.5 + along * ahead))
    face = up.cross(side).normalized()
    faces = []
    for k in range(n):
        faces.append(bm.faces.new((mids[k], mids[k + 1], lefts[k + 1], lefts[k])))
        faces.append(bm.faces.new((mids[k], rights[k], rights[k + 1], mids[k + 1])))
    for f in faces:
        f.normal_update()
        if f.normal.dot(face) < 0:
            f.normal_flip()
    obj = kit.mesh_object(name, bm, (0, 0, 0))
    obj.location = (0, 0, 0)
    m = obj.modifiers.new("thick", "SOLIDIFY")
    m.thickness = thickness
    m.offset = 0
    m.use_rim = False
    kit.apply_modifiers(obj)
    kit.smooth(obj)
    # the origin at the frond's base (parts are seated from there)
    me = obj.data
    for v in me.vertices:
        v.co -= base
    obj.location = base
    return obj


def _puff(kit, name, base, axis, length, radius, lumps=5, lump=0.22):
    """A soft lumpy puff (the tail's fluffy tip): a teardrop from `base` along `axis`,
    widest two thirds out, its sides bumped into tufts. The origin is at the base."""
    import bmesh
    V = kit.V
    axis = V(axis).normalized()
    a = axis.cross(V((0, 0, 1)) if abs(axis.z) < 0.9 else V((1, 0, 0))).normalized()
    b = axis.cross(a).normalized()
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=kit.lod(7, 5), v_segments=kit.lod(5, 4), radius=1.0)
    for v in bm.verts:
        x, y, z = v.co                        # z: along the puff, -1 base .. 1 tip
        f = 0.5 * (z + 1)
        fat = math.sin(math.pi * min(1.0, 0.15 + 0.85 * f ** 0.8)) ** 0.8
        ang = math.atan2(y, x)
        bump = 1.0 + lump * math.cos(lumps * ang + 2.0 * z)
        v.co = base + axis * (f * length) + (a * x + b * y) * radius * fat * bump
    obj = kit.mesh_object(name, bm)
    kit.smooth(obj)
    for v in obj.data.vertices:
        v.co -= base
    obj.location = base
    return obj


def parts(kit, d):
    F, mats, V = kit.F, d["mats"], kit.V
    baby = d["form"] == "hatchling"
    _strip_antenna_weights(kit, d["body"])
    out = []
    # The antennae: feathery fronds on their own bones, swept up, back and out.
    ants = []
    for s, side in ((-1, "L"), (1, "R")):
        joint = kit.node(f"ant_{side}")
        tip = kit.node(f"anttip_{side}")
        up = (tip - joint).normalized()
        base = joint + up * (0.1 if baby else 0.14)
        across = V((s * 0.45, -1.0, 0.1)).cross(up)     # the broad face looks forward and out
        bend = V((s * 0.5, 1.0, 0.0))                     # the tips curl back and out
        if baby:
            fr = _frond(kit, f"antenna_{side}", base, up, across, bend, 0.36, 0.2, 3, 1.5, 0.012)
        else:
            fr = _frond(kit, f"antenna_{side}", base, up, across, bend, 0.92, 0.34, 6, 0.75, 0.016, notch=0.66)
        fr.data.materials.append(mats["horn"])
        ants.append((fr, f"antenna_{side}"))
    out.append(("horns", 0, ants))
    # The tail's fluffy tip: a soft lumpy puff.
    tdir = (kit.node("tail_tip") - kit.node("tail4")).normalized()
    k = 0.6 if baby else 1.0
    base = kit.node("tail_tip") - tdir * 0.06 * k
    puff = _puff(kit, "tail_puff", base, tdir, 0.36 * k, 0.12 * k)
    puff.data.materials.append(mats["accent_flat"])
    out.append(("tail_tip", 0, [(puff, "tail4")]))
    # Rare (Sunburst): a glowing sunburst halo behind the antennae, long and short rays
    # fanned in a half circle.
    crown = []
    rays = kit.lod([(-84, 0.62), (-56, 0.9), (-28, 0.66), (0, 1.0), (28, 0.66), (56, 0.9), (84, 0.62)],
                   [(-60, 0.8), (0, 1.0), (60, 0.8)])
    h = F["head"]
    for j, (ang, ln) in enumerate(rays):
        a = math.radians(ang)
        out_dir = V((math.sin(a), 0.5, math.cos(a))).normalized()    # up and out, leaning back
        basep = kit.head_point((0.13 * math.sin(a), 0.14, 0.06 + 0.12 * math.cos(a)))
        L = ln * (0.38 if baby else 0.66) * h["k"]
        pl = kit.blade(f"sunray_{j}", basep, out_dir, V((math.cos(a), 0.0, -math.sin(a))), L, L * 0.36, 0.012)
        pl.data.materials.append(mats["rune"])
        crown.append((pl, "head"))
    out.append(("spikes", 1, crown))
    return out


def _strip_antenna_weights(kit, body):
    """The skin never follows the antenna bones (they only carry the fronds): their heat
    weights go back to the head, and the regions are tagged again."""
    groups = [g for g in body.vertex_groups if g.name.startswith("antenna")]
    if not groups:
        return
    head = body.vertex_groups["head"]
    idx = {g.index for g in groups}
    for v in body.data.vertices:
        lost = sum(g.weight for g in v.groups if g.group in idx)
        if lost <= 0:
            continue
        w = {g.group: g.weight for g in v.groups if g.group not in idx}
        w[head.index] = w.get(head.index, 0.0) + lost
        total = sum(w.values()) or 1.0
        for gi, wt in w.items():
            body.vertex_groups[gi].add([v.index], wt / total, "REPLACE")
    for g in groups:
        body.vertex_groups.remove(g)
    kit.tex.tag_regions(body, kit.PLAN.REGION)


def accent(kit, body):
    """The collar ruff (the body's second piece), the chest and belly, and pale socks on all
    four legs."""
    import mathutils
    mask = kit.mask_attr(body)
    F = kit.F
    mk = F["mask"]
    baby = F["name"] == "hatchling"
    down = mathutils.Vector((0, -0.4, -0.92)).normalized()
    sock_z = 0.14 if baby else 0.3
    collar = set().union(*_pieces(body.data)[1:])
    for v in body.data.vertices:
        x, y, z = v.co
        a = min(1.0, max(0.0, (v.normal.dot(down) - 0.2) / 0.45))
        if abs(x) > mk["max_x"] or z > mk["max_z"] or z < mk["min_z"]:
            a = 0.0
        if mk.get("tail_cut") and y > mk["tail_cut"][0] and z < mk["tail_cut"][1]:
            a = 0.0
        if v.index in collar:
            a = 1.0
        if z < sock_z and abs(x) > (0.1 if baby else 0.17):
            a = max(a, min(1.0, (sock_z - z) / (sock_z * 0.45)))
        mask.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


def texture(tx, nt, p, form):
    """R: soft moth bands round the abdomen and down the tail; G: the points (lower legs, the
    tail's end, the tip of the muzzle); B: the rare variant's glowing sun bands and rings."""
    baby = form == "hatchling"
    y0 = 0.1 if baby else 0.3
    rear = tx.smoothstep(nt, tx.axis(nt, 1), y0, y0 + (0.12 if baby else 0.3))
    bands = tx.stripes(nt, p["stripe"], direction="Y", distortion=0.4, width=(0.55, 0.7), where=tx.upper(nt))
    r = tx.mul(nt, bands, rear)
    legs = tx.smoothstep(nt, tx.axis(nt, 2), 0.2 if baby else 0.5, 0.05 if baby else 0.22)
    tail_end = tx.smoothstep(nt, tx.axis(nt, 1), 0.68 if baby else 2.2, 0.84 if baby else 2.8)
    nose = tx.near(nt, (0.0, -0.8, 0.93) if baby else _h(0.0, -0.41, -0.13), 0.1 if baby else 0.13, 0.7)
    g = tx.maxi(nt, tx.maxi(nt, legs, tail_end), nose)
    sun = tx.stripes(nt, p["stripe"] * 1.4, direction="Y", distortion=0.2, width=(0.78, 0.86), where=tx.top(nt))
    rings = tx.spots(nt, p["spot_cell"] * 1.3, keep=0.5, size=(0.3, 0.24))
    inner = tx.spots(nt, p["spot_cell"] * 1.3, keep=0.5, size=(0.2, 0.15))
    ring = tx.math_op(nt, "SUBTRACT", rings, inner)
    b = tx.maxi(nt, tx.mul(nt, sun, rear), tx.mul(nt, ring, tx.smoothstep(nt, tx.axis(nt, 1), -0.2, 0.1)))
    return {"r": r, "g": g, "b": b}
