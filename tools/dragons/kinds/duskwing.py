"""The Duskwing (Shade, rare): a bat-winged night dragon, a wyvern.

Design notes
- The baby is a round fluffball: a big round head on a round body, enormous bat ears, huge
  eyes, tiny wing-arms, stubby hind legs and a little tail with a crescent at its end. It
  stands up like a bat pup and toddles, wobbly, with its wing-arms out for balance.
- The adult is a sleek, elegant wyvern: tall bat ears, a fluffy ruff round the neck, strong
  hind legs, a long whip tail ending in a crescent moon, and huge wing-arms that are also
  its forelegs: it walks on the wrists of its folded wings, like a bat. Folded, the wings
  hang at its sides like a cloak; spread, the membranes are a night sky full of little stars.
  Mysterious and majestic, but kind: big gentle eyes, a calm brow, no horns.
- Colours are a sky at dusk: darker on top, the horizon's glow on the belly and ruff.
  Variants: Dusk (indigo and lavender, a night-blue mantle down the back), Twilight (plum and
  rose, gold star freckles), Moonshadow (charcoal and silver, silver freckles), and the rare
  Eclipse: black-violet, its stars and star-dust glowing gold, a golden corona along the
  wings' trailing edges, bigger ears rimmed in glowing gold, a gold crescent tail and a
  crescent-moon crown on its brow (its frill, tail tip and horns groups replace the common).
- Motion (plans/duskwing.py): a bat's crawl-walk on wrists and feet, a bounding run, sitting up
  with the folded wings round it like a cloak, sleeping with them drawn over it, quiet glides
  and deep wingbeats, ears that swivel.
How it's built: the ears sit on their own bones (the skin there follows the head); the stars
are poked into the membranes and pinned to them, so they stay on the wing however it folds;
the ruff is a ring of fur tufts; the tail's crescent and the crown are crescent meshes.
Concept: docs/art/concept/dragons/dragon_duskwing.jpg (reference only, D74).
"""
import math


def _mirrored(center, sides):
    nodes = dict(center)
    for name, (p, r) in sides.items():
        nodes[f"{name}_R"] = ((p[0], p[1], p[2]), r)
        nodes[f"{name}_L"] = ((-p[0], p[1], p[2]), r)
    return nodes


META = dict(
    name="duskwing", title="Duskwing", dex=8, element="Shade", parents=(), rarity="rare",
    plan="duskwing", size=1.05,
    stats=dict(wing=9, wit=8, might=5, breath=6, stamina=6),
    manners=("Gentle", "Curious", "Shy", "Sleepy", "Proud"),
    traits=("Night Owl", "Skydancer", "Moonlit", "Strong Wings", "Quick Learner", "Songbird", "Starborn"),
    rare_variant=3, rare_replaces=True,
    blurb="A quiet wyvern of the night sky; it glides without a sound, and its wings are full of stars.",
)

VARIANTS = [
    dict(name="Dusk", base=(0.085, 0.085, 0.30), accent=(0.62, 0.52, 0.92), pattern=(0.03, 0.03, 0.12),
         horn=(0.86, 0.84, 1.0), membrane=(0.09, 0.075, 0.28), iris=(0.82, 0.48, 0.10), glow=(0.70, 0.55, 1.0),
         pattern_channel="r", glow_channel=None),
    dict(name="Twilight", base=(0.30, 0.10, 0.27), accent=(1.0, 0.60, 0.62), pattern=(1.0, 0.82, 0.55),
         horn=(1.0, 0.86, 0.78), membrane=(0.24, 0.07, 0.24), iris=(0.14, 0.50, 0.48), glow=(0.78, 0.55, 1.0),
         pattern_channel="g", glow_channel=None),
    dict(name="Moonshadow", base=(0.075, 0.075, 0.09), accent=(0.70, 0.73, 0.80), pattern=(0.90, 0.92, 1.0),
         horn=(0.88, 0.90, 0.96), membrane=(0.075, 0.075, 0.105), iris=(0.90, 0.72, 0.26), glow=(0.70, 0.62, 1.0),
         pattern_channel="g", glow_channel=None),
    dict(name="Eclipse", base=(0.035, 0.022, 0.06), accent=(0.26, 0.12, 0.32), pattern=(1.0, 0.74, 0.28),
         horn=(1.0, 0.80, 0.40), membrane=(0.022, 0.012, 0.045), iris=(1.0, 0.76, 0.22), glow=(1.0, 0.70, 0.22),
         pattern_channel=None, glow_channel="b"),
]

EGG = dict(height=1.02, width=0.36, point=1.0, speckle="stars",
           speckle_params=dict(count=46, size=(0.008, 0.02), count_lod1=14),
           colors=[((0.07, 0.07, 0.24), (0.86, 0.86, 1.0)), ((0.24, 0.08, 0.22), (1.0, 0.84, 0.62)),
                   ((0.16, 0.16, 0.19), (0.90, 0.92, 1.0)), ((0.03, 0.02, 0.05), (1.0, 0.76, 0.30))])

# ------------------------------------------------------------------------------ grown
# Faces -Y, Z up; units ~ metres at adult size. radius = (side, vertical). The idle stance:
# the hind feet and the folded wings' wrists on the ground, the chest carried high.
GROWN_NODES = _mirrored({
    "tail_tip": ((0, 4.18, 0.84), (0.03, 0.03)),
    "tail6": ((0, 3.74, 0.77), (0.046, 0.044)),
    "tail5": ((0, 3.24, 0.78), (0.064, 0.06)),
    "tail4": ((0, 2.70, 0.87), (0.092, 0.088)),
    "tail3": ((0, 2.12, 1.01), (0.135, 0.128)),
    "tail2": ((0, 1.52, 1.17), (0.20, 0.195)),
    "hips": ((0, 0.80, 1.30), (0.36, 0.39)),
    "belly": ((0, 0.14, 1.42), (0.37, 0.42)),
    "chest": ((0, -0.46, 1.62), (0.42, 0.50)),
    "neck1": ((0, -0.82, 1.98), (0.31, 0.32)),
    "neck2": ((0, -1.03, 2.26), (0.235, 0.24)),
    "neck3": ((0, -1.14, 2.49), (0.215, 0.215)),
    "head": ((0, -1.26, 2.76), (0.37, 0.335)),
    "muzzle": ((0, -1.55, 2.67), (0.19, 0.152)),
    "snout": ((0, -1.725, 2.62), (0.112, 0.092)),
}, {
    "hipj": ((0.31, 0.82, 1.12), (0.31, 0.36)),
    "knee": ((0.37, 0.48, 0.66), (0.19, 0.19)),
    "ankle": ((0.37, 0.92, 0.24), (0.10, 0.104)),
    "toe_b": ((0.39, 0.66, 0.065), (0.13, 0.065)),
    "ear": ((0.16, -1.20, 2.82), (0.01, 0.01)),  # bone only (not joined to the body): the
    "ear_tip": ((0.34, -1.10, 3.54), (0.01, 0.01)),  # ear's pivot under its base, and its tip
})
GROWN_EDGES = [("tail_tip", "tail6"), ("tail6", "tail5"), ("tail5", "tail4"), ("tail4", "tail3"),
               ("tail3", "tail2"), ("tail2", "hips"), ("hips", "belly"), ("belly", "chest"), ("chest", "neck1"),
               ("neck1", "neck2"), ("neck2", "neck3"), ("neck3", "head"), ("head", "muzzle"), ("muzzle", "snout")]
for _side in ("L", "R"):
    GROWN_EDGES += [("hips", f"hipj_{_side}"), (f"hipj_{_side}", f"knee_{_side}"),
                    (f"knee_{_side}", f"ankle_{_side}"), (f"ankle_{_side}", f"toe_b_{_side}")]

# The wing in its own plane (u out along the span, v back along the chord), a bat's hand:
# a long forearm, the thumb forward at the wrist, four fingers fanning to a scalloped edge.
WING_LAYOUT = {"root": (0.0, 0.0), "elbow": (0.46, 0.16), "wrist": (2.34, -0.10), "thumb": (2.49, -0.34),
               "f1": (4.18, 0.25), "f2": (3.88, 1.10), "f3": (3.22, 1.70), "f4": (2.34, 1.86),
               "body": (0.10, 0.62)}

BUILDS = {
    "neutral": {},
    "sturdy": {"chest": (1.07, 0.98), "belly": (1.06, 0.98), "hips": (1.05, 1.0), "neck1": (1.05, 0.97),
               "leg_up": (1.07, 0.98), "tail2": (1.05, 0.97)},
    "sleek": {"chest": (0.95, 1.02), "belly": (0.93, 1.03), "hips": (0.95, 1.0), "leg_lo": (0.95, 1.03),
              "tail3": (0.94, 1.05), "tail4": (0.94, 1.05)},
    "long": {"neck1": (0.97, 1.07), "neck2": (0.97, 1.08), "neck3": (0.97, 1.06), "belly": (0.97, 1.06),
             "tail3": (0.96, 1.08), "tail4": (0.96, 1.1), "tail5": (0.96, 1.1)},
}


def _soften(obj, iterations):
    """Even out the lumps where the metaballs meet (a gentle Laplacian smooth)."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for _ in range(iterations):
        bmesh.ops.smooth_vert(bm, verts=bm.verts, factor=0.5, use_axis_x=True, use_axis_y=True, use_axis_z=True)
    bm.to_mesh(obj.data)
    bm.free()


def _keep_largest(bm):
    """Drop every piece but the body (the ear nodes are bone points, not joined to it)."""
    import bmesh
    seen, parts = set(), []
    for v in bm.verts:
        if v in seen:
            continue
        stack, part = [v], []
        seen.add(v)
        while stack:
            a = stack.pop()
            part.append(a)
            for e in a.link_edges:
                b = e.other_vert(a)
                if b not in seen:
                    seen.add(b)
                    stack.append(b)
        parts.append(part)
    parts.sort(key=len)
    for part in parts[:-1]:
        bmesh.ops.delete(bm, geom=part, context="VERTS")


def _grown_sculpt(kit, obj):
    """A deep chest and a tucked waist, a soft round collar for the ruff, round cheeks."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    _keep_largest(bm)
    for v in bm.verts:
        x, y, z = v.co
        if -0.95 < y < -0.1 and z < 1.55 and abs(x) < 0.34:  # chest keel
            k = (1 - abs(x) / 0.34) * max(0.0, (1.55 - z) / 0.45)
            v.co.z -= 0.07 * k
        if 0.0 < y < 0.7 and z < 1.3 and abs(x) < 0.3:  # tucked belly
            k = (1 - abs(x) / 0.3) * max(0.0, (1.3 - z) / 0.3)
            v.co.z += 0.06 * k
        if -1.48 < y < -1.14 and 2.64 < z < 2.86:  # round cheeks
            v.co.x *= 1.07
        dy, dz = y + 1.32, z - 2.90  # a rounded brow and crown over the eyes
        if abs(x) < 0.3 and z > 2.76 and abs(dy) < 0.3:
            k = max(0.0, 1 - (dy / 0.3) ** 2) * max(0.0, 1 - (x / 0.3) ** 2) * min(1.0, (z - 2.76) / 0.12)
            v.co.z += 0.05 * k
    bm.to_mesh(obj.data)
    bm.free()


GROWN = dict(
    name="grown", nodes=GROWN_NODES, edges=GROWN_EDGES, body="skin",
    # DR2 sizing: grown at the common scale, the Pouncer's bulk and length (the geometric mean of the
    # cube root of the body's volume and its length); META size then sizes it in the game.
    body_tris=1300, body_tris_lod1=480, export_scale=1.05,
    young={
        "bones": {
            "head": (0.95, 0.9, 1.0), "snout": (0.9, 0.75, 0.95),
            "neck1": (0.76, 0.56), "neck2": (0.78, 0.55), "neck3": (0.8, 0.55),
            "chest": (0.7, 0.58), "belly": (0.68, 0.56), "hips": (0.7, 0.58),
            "tail1": (0.68, 0.52), "tail2": (0.7, 0.5), "tail3": (0.72, 0.5), "tail4": (0.76, 0.5),
            "tail5": (0.8, 0.52), "tail6": (0.85, 0.55),
            "leg_up": (0.72, 0.62), "leg_lo": (0.74, 0.62), "foot": (0.82, 0.72),
        },
        "parts": {"eyes": 1.3, "horns": 0.6, "frill": 0.9, "wings": 0.62, "spikes": 0.7,
                  "tail_tip": 0.7, "heart": 0.85, "runes": 0.7},
    },
    young_pose={"neck1": -10, "neck2": -4, "head": 12},
    base_pose={"tail1": (4, 0, 0), "tail2": (2, 0, 4), "tail3": (4, 0, 6), "tail4": (6, 0, 8),
               "tail5": (8, 0, 8), "tail6": (10, 0, 6)},
    builds=BUILDS,
    eyes=dict(at=(0.2, -1.52, 2.84), out=(0.62, -0.76, 0.16), iris=(0.122, 0.132, 0.06),
              pupil=(0.09, 0.104, 0.024), slit=(0.3, 1.06),
              glints=((-0.03, 0.045, 0.021), (0.022, -0.045, 0.011)), seg=(12, 2, 8, 2)),
    head=dict(origin=(0, -1.26, 2.76), k=1.0, buds=False),
    tail_k=1.0,
    heart=dict(at=(0, -0.92, 1.58), size=0.12),
    wing=dict(root=(0.28, -0.42, 1.95), scale=1.12, dihedral=20, droop=5, layout=WING_LAYOUT,
              radii={"root": 0.115, "elbow": 0.095, "wrist": 0.085, "thumb": 0.055, "finger": 0.024, "tip": 0.009},
              arm_tris=130, thickness=0.016),
    mask=dict(max_x=0.34, max_z=2.9, min_z=-1.0, tail_cut=(1.3, 1.2)),
    inset={"eyes": 0.02, "horns": 0.02, "spikes": 0.05, "frill": 0.03, "heart": -0.06, "runes": -0.012},
    face=dict(nostril=(0.045, -1.825, 2.675), nostril_r=(0.021, 0.014, 0.008), mouth_r=0.012,
              mouth=lambda side, a: (side * 0.16 * a ** 0.7, -1.845 + 0.40 * a ** 1.5, 2.575 + 0.09 * a * a)),
    jaw_hinge=(0, -1.40, 2.60),
    mouth_detail=dict(depth=0.24, fade=0.18, width=0.34, tooth=(0.008, 0.014), fang=(0.012, 0.03),
                      tongue=(0.06, 0.12, 0.014)),
    skin=dict(stripe=0.4, spot_cell=0.34, ao=0.6),
    sculpt=_grown_sculpt,
)

# ------------------------------------------------------------------------------ hatchling
HATCH_NODES = _mirrored({
    "tail_tip": ((0, 1.10, 0.35), None),
    "tail6": ((0, 1.04, 0.25), None),
    "tail5": ((0, 0.93, 0.17), None),
    "tail4": ((0, 0.78, 0.12), None),
    "tail3": ((0, 0.60, 0.12), None),
    "tail2": ((0, 0.40, 0.18), None),
    "hips": ((0, 0.08, 0.34), None),
    "belly": ((0, 0.02, 0.52), None),
    "chest": ((0, -0.06, 0.70), None),
    "neck1": ((0, -0.11, 0.82), None),
    "neck2": ((0, -0.14, 0.90), None),
    "neck3": ((0, -0.17, 0.98), None),
    "head": ((0, -0.22, 1.10), None),
    "muzzle": ((0, -0.52, 1.02), None),
    "snout": ((0, -0.64, 0.99), None),
}, {
    "hipj": ((0.19, 0.10, 0.28), None),
    "knee": ((0.22, -0.02, 0.18), None),
    "ankle": ((0.23, 0.08, 0.08), None),
    "toe_b": ((0.24, -0.08, 0.04), None),
    "ear": ((0.15, -0.20, 1.26), None),
    "ear_tip": ((0.36, -0.12, 1.76), None),
})

HATCH_META = [
    ("ell", (0, -0.24, 1.10), (0.40, 0.36, 0.35)),     # big round head
    ("ell", (0, -0.52, 1.02), (0.15, 0.12, 0.10)),     # button muzzle
    ("ell", (0, -0.48, 0.96), (0.12, 0.11, 0.07)),     # chin
    ("ell", (0, 0.02, 0.46), (0.37, 0.35, 0.38)),      # round body
    ("ell", (0, 0.0, 0.30), (0.34, 0.33, 0.22)),       # round bottom
    ("chain", [(0, 0.30, 0.26), (0, 0.50, 0.16), (0, 0.70, 0.12), (0, 0.88, 0.14), (0, 1.00, 0.22),
               (0, 1.08, 0.32)], [0.1, 0.075, 0.06, 0.05, 0.04, 0.03]),  # a little tail curling up
]
for _s in (-1, 1):
    HATCH_META += [
        ("ball", (_s * 0.16, -0.42, 1.01), 0.14),                     # cheeks
        ("ell", (_s * 0.20, 0.08, 0.26), (0.13, 0.16, 0.14)),         # thighs
        ("ell", (_s * 0.23, -0.06, 0.05), (0.09, 0.13, 0.055)),       # hind feet
    ]

HATCH = dict(
    name="hatchling", nodes=HATCH_NODES, meta=HATCH_META, body="meta",
    body_tris=1300, body_tris_lod1=480, export_scale=1.0,
    young={
        "bones": {name: ((0.76, 0.76) if name in ("head", "snout") else (0.64, 0.64))
                  for name in ("hips", "belly", "chest", "neck1", "neck2", "neck3", "head", "snout", "tail1",
                               "tail2", "tail3", "tail4", "tail5", "tail6", "leg_up", "leg_lo", "foot")},
        "parts": {"eyes": 1.1, "horns": 0.6, "frill": 0.85, "wings": 0.62, "spikes": 0.8,
                  "tail_tip": 0.85, "heart": 1.0, "runes": 0.9},
    },
    base_pose={"tail1": (4, 0, 0), "tail2": (2, 0, 8), "tail3": (4, 0, 10), "tail4": (6, 0, 10)},
    builds={"neutral": {}, "sturdy": {}, "sleek": {}, "long": {}},
    eyes=dict(at=(0.18, -0.54, 1.14), out=(0.45, -0.88, 0.1), iris=(0.122, 0.136, 0.064),
              pupil=(0.09, 0.106, 0.028), slit=(0.32, 1.06),
              glints=((-0.03, 0.046, 0.028), (0.026, -0.044, 0.013)), seg=(14, 3, 12, 2)),
    head=dict(origin=(0, -0.24, 1.10), k=0.9, buds=True),
    tail_k=0.4,
    heart=dict(at=(0, -0.33, 0.60), size=0.075),
    wing=dict(root=(0.27, -0.07, 0.76), scale=0.30, dihedral=20, droop=5, layout=WING_LAYOUT,
              radii={"root": 0.05, "elbow": 0.044, "wrist": 0.042, "thumb": 0.032, "finger": 0.014, "tip": 0.006},
              arm_tris=90, thickness=0.01),
    mask=dict(max_x=0.24, max_z=1.08, min_z=0.12, tail_cut=None),
    inset={"eyes": 0.03, "horns": 0.02, "spikes": 0.02, "frill": 0.02, "heart": -0.03, "runes": -0.01},
    face=dict(nostril=(0.04, -0.665, 1.01), nostril_r=(0.022, 0.015, 0.009), mouth_r=0.011,
              mouth=lambda side, a: (side * 0.095 * a ** 0.8, -0.675 + 0.15 * a ** 1.6, 0.94 + 0.03 * a * a)),
    jaw_hinge=(0, -0.47, 0.94),
    mouth_detail=dict(depth=0.13, fade=0.09, width=0.2, tooth=(0.006, 0.01), fang=(0.008, 0.018),
                      tongue=(0.04, 0.05, 0.01)),
    skin=dict(stripe=0.2, spot_cell=0.16, ao=0.25),
    sculpt=lambda kit, obj: _soften(obj, 8),
)

FORMS = {"grown": GROWN, "hatchling": HATCH}


# ------------------------------------------------------------------------------ shapes
def _frame(kit, up, across):
    """An orthonormal frame (across, up, normal) from rough up and across directions."""
    u = kit.V(up).normalized()
    a = kit.V(across)
    a = (a - u * a.dot(u)).normalized()
    return a, u, a.cross(u).normalized()


def _origin_to(kit, obj, point):
    """Move an object's origin to `point`, keeping its shape where it is (parts that belong
    together share an origin, so the seating moves them as one)."""
    from mathutils import Matrix
    shift = obj.location - kit.V(point)
    obj.data.transform(Matrix.Translation(shift))
    obj.location = kit.V(point)
    return obj


def _shell(kit, name, base, up, across, outline, width, height, cup, thickness, rings=1, origin=None):
    """A cupped leaf-like shell (an ear): outline [(x, y)] in units of (width, height), x
    across and y up from the base; the middle dips back by `cup` (the hollow). thickness 0:
    one-sided, facing the hollow's way (+normal). Its origin is `origin` (default the base)."""
    import bmesh
    a, u, n = _frame(kit, up, across)
    c2 = (sum(p[0] for p in outline) / len(outline), sum(p[1] for p in outline) / len(outline))
    bm = bmesh.new()
    lift = kit.V(base) - kit.V(origin if origin is not None else base)

    def at(x, y, depth):  # relative to the origin
        return lift + a * (x * width) + u * (y * height) - n * depth

    layers = []
    for k in range(rings + 1):
        f = 1.0 - k / (rings + 1)
        layers.append([bm.verts.new(at(c2[0] + (x - c2[0]) * f, c2[1] + (y - c2[1]) * f, cup * (1 - f * f)))
                       for x, y in outline])
    centre = bm.verts.new(at(c2[0], c2[1], cup))
    m = len(outline)
    for la, lb in zip(layers, layers[1:]):
        for j in range(m):
            bm.faces.new((la[j], la[(j + 1) % m], lb[(j + 1) % m], lb[j]))
    for j in range(m):
        bm.faces.new((layers[-1][j], layers[-1][(j + 1) % m], centre))
    bm.normal_update()
    for f in bm.faces:
        if f.normal.dot(n) < 0:
            f.normal_flip()
    obj = kit.mesh_object(name, bm, kit.V(origin if origin is not None else base))
    if thickness:
        mod = obj.modifiers.new("thick", "SOLIDIFY")
        mod.thickness = thickness
        mod.offset = 0
        kit.apply_modifiers(obj)
    kit.smooth(obj)
    return obj


# A bat's ear, tall with a rounded tip leaning to the outer side (x > 0 is the outer edge).
EAR_OUTLINE = [(-0.50, 0.0), (-0.54, 0.30), (-0.40, 0.62), (-0.18, 0.88), (0.0, 1.0), (0.16, 0.90),
               (0.36, 0.62), (0.50, 0.30), (0.48, 0.02)]
EAR_OUTLINE_LOD1 = [(-0.50, 0.0), (-0.50, 0.42), (-0.22, 0.86), (0.0, 1.0), (0.30, 0.70), (0.50, 0.30), (0.48, 0.02)]


def _ear(kit, d, s, scale, rim=False):
    """A tall bat ear on its ear bone, its hollow facing forward and out: the outer ear in the
    body colour, the hollow in the accent colour; the Eclipse's is bigger with a glowing rim."""
    V = kit.V
    mats = d["mats"]
    baby = d["form"] == "hatchling"
    base = kit.node(f"ear_{'R' if s > 0 else 'L'}")
    base = base + V((0.0, 0.0, 0.08 if not baby else 0.06))  # on the skin above the pivot (seated)
    up = V((s * (0.42 if not baby else 0.5), 0.2, 0.88)).normalized()
    facing = V((s * 0.5, -1.0, 0.15)).normalized()
    across = up.cross(facing)
    h = (0.70 if not baby else 0.52) * scale
    w = (0.36 if not baby else 0.34) * scale
    th = (0.035 if not baby else 0.03) * scale
    outline = kit.lod(EAR_OUTLINE, EAR_OUTLINE_LOD1)
    if s < 0:  # the frame's across axis flips on the left: mirror the outline to match
        outline = [(-x, y) for x, y in reversed(outline)]
    outer = _shell(kit, f"ear_{s}", base, up, across, outline, w, h, 0.10 * w, th, kit.lod(1, 0))
    outer.data.materials.append(mats["body_plain"])
    a, u, n = _frame(kit, up, across)
    inner = _shell(kit, f"earin_{s}", base + u * (0.07 * h) + n * (th * 0.9), up, across, outline,
                   w * 0.66, h * 0.8, 0.06 * w, 0.0, 0, origin=base)
    inner.data.materials.append(mats["accent_flat"])
    pieces = [outer, inner]
    if rim:
        pts = [base + a * (x * w * 1.0) + u * (y * h * 1.0) - n * (0.1 * w * 0.2) for x, y in outline]
        if kit.LOD:  # the den's far view: a lighter rim
            pts = pts[::2] + ([pts[-1]] if len(pts) % 2 == 0 else [])
        glow = _origin_to(kit, kit.tube(f"earrim_{s}", pts, [th * 0.6] * len(pts), ring=3), base)
        glow.data.materials.append(mats["glow_flat"])
        pieces.append(glow)
    return pieces


def _crescent(kit, name, centre, up, across, radius, bite, offset, thickness, n=None):
    """A crescent moon lying in the plane of (across, up): a disc of `radius` less a disc of
    radius `bite` shifted by `offset` along `across` (its horns point that way)."""
    import bmesh
    n = n or kit.lod(8, 4)
    a, u, nrm = _frame(kit, up, across)
    c = kit.V(centre)
    # where the two circles cross; both arcs run from the upper crossing round the back
    dx = (offset * offset + radius * radius - bite * bite) / (2 * offset)
    hy = math.sqrt(max(1e-6, radius * radius - dx * dx))
    t0 = math.atan2(hy, dx)
    t1 = math.atan2(hy, dx - offset)
    outer = [(radius * math.cos(t), radius * math.sin(t)) for t in
             (t0 + (2 * math.pi - 2 * t0) * k / n for k in range(n + 1))]
    inner = [(offset + bite * math.cos(t), bite * math.sin(t)) for t in
             (t1 + (2 * math.pi - 2 * t1) * k / n for k in range(n + 1))]
    bm = bmesh.new()
    top = [bm.verts.new(a * x + u * y) for x, y in outer]
    bot = [top[0]] + [bm.verts.new(a * x + u * y) for x, y in inner[1:-1]] + [top[-1]]
    bm.faces.new((top[0], top[1], bot[1]))
    for j in range(1, n - 1):
        bm.faces.new((top[j], top[j + 1], bot[j + 1], bot[j]))
    bm.faces.new((top[n - 1], top[n], bot[n - 1]))
    obj = kit.mesh_object(name, bm, c)
    mod = obj.modifiers.new("thick", "SOLIDIFY")
    mod.thickness = thickness
    mod.offset = 0
    mod.use_rim = True
    kit.apply_modifiers(obj)
    kit.smooth(obj)
    return obj


def _star(bm, centre, a, u, r, sparkle):
    """A little star in the plane (a, u): a four-pointed sparkle (6 triangles) or a tiny
    diamond (2)."""
    if not sparkle:
        vs = [bm.verts.new(centre + (a * math.cos(t) * (1.0 if k % 2 == 0 else 0.6) +
                                     u * math.sin(t) * (1.0 if k % 2 == 0 else 0.6)) * r)
              for k, t in enumerate(math.pi * j / 2 for j in range(4))]
        bm.faces.new(vs)
        return
    tips = [bm.verts.new(centre + (a * math.cos(t) + u * math.sin(t)) * r) for t in (math.pi * j / 2 for j in range(4))]
    waist = [bm.verts.new(centre + (a * math.cos(t) + u * math.sin(t)) * r * 0.28)
             for t in (math.pi * (j + 0.5) / 2 for j in range(4))]
    for j in range(4):
        bm.faces.new((waist[j - 1], tips[j], waist[j]))
    bm.faces.new(waist)


# ------------------------------------------------------------------------------ hooks
def parts(kit, d):
    _ears_on_head(kit, d)
    _pin_stars(kit, d)
    F, mats = kit.F, d["mats"]
    baby = d["form"] == "hatchling"
    out = []
    # Ears: tall bat ears on their own bones (they swivel); the Eclipse's bigger, rimmed in gold.
    for rare in (False, True):
        pieces = []
        for s in (-1, 1):
            bone = f"ear_{'R' if s > 0 else 'L'}"
            for o in _ear(kit, d, s, 1.12 if rare else 1.0, rim=rare):
                pieces.append((o, bone))
        out.append(("frill", 1 if rare else 0, pieces))
    # The ruff: soft tufts round the base of the neck (a fluffy chest and a crown on the baby).
    out.append(("spikes", 0, _ruff(kit, d)))
    # The tail's crescent moon (glowing on the Eclipse).
    for rare in (False, True):
        x, y, z = F["nodes"]["tail_tip"][0]
        k = F["tail_k"]
        cres = _crescent(kit, f"crescent_{int(rare)}", (0, y + 0.05 * k, z + 0.02 * k), (0, 0.35, 1.0),
                         (0, 1.0, -0.35), 0.30 * k, 0.26 * k, 0.16 * k, 0.05 * k)
        cres.data.materials.append(mats["glow_flat" if rare else "horn"])
        out.append(("tail_tip", 1 if rare else 0, [(cres, "tail6")]))
    # The Eclipse's crest: a crescent moon standing on its brow, horns up, facing forward.
    r = 0.15 if not baby else 0.12
    centre = kit.head_point((0, -0.10, 0.36) if not baby else (0, -0.06, 0.42))
    crest = _crescent(kit, "crest", centre, (1, 0, 0), (0, 0.25, 1.0), r, r * 0.84, r * 0.52,
                      0.035 if not baby else 0.03)
    _origin_to(kit, crest, centre - kit.V((0, 0.25, 1.0)).normalized() * r)  # seated by its lowest point
    crest.data.materials.append(mats["glow_flat"])
    out.append(("horns", 1, [(crest, "head")]))
    return out


def _pin_stars(kit, d):
    """Each star follows exactly the membrane vertex at its centre (the poke under it), so it
    moves as one piece with the membrane however far the wing folds (the kit weights every
    vertex by its own nearest struts, which would stretch a star lying across two)."""
    import bmesh
    from mathutils import kdtree
    for objs in (d["wings"], d["rare_wings"]):
        by = {o.name.split(".")[0]: o for o in objs}
        for side in ("L", "R"):
            stars, mem = by.get(f"stars_{side}"), by.get(f"membrane_{side}")
            if stars is None or mem is None:
                continue
            tree = kdtree.KDTree(len(mem.data.vertices))
            for v in mem.data.vertices:
                tree.insert(mem.matrix_world @ v.co, v.index)
            tree.balance()
            names = {g.index: g.name for g in mem.vertex_groups}
            bm = bmesh.new()
            bm.from_mesh(stars.data)
            bm.verts.ensure_lookup_table()
            seen = set()
            for v0 in bm.verts:
                if v0.index in seen:
                    continue
                island, stack = [], [v0]
                seen.add(v0.index)
                while stack:
                    a = stack.pop()
                    island.append(a.index)
                    for e in a.link_edges:
                        b = e.other_vert(a)
                        if b.index not in seen:
                            seen.add(b.index)
                            stack.append(b)
                centre = sum((stars.matrix_world @ stars.data.vertices[i].co for i in island),
                             kit.V((0, 0, 0))) / len(island)
                _, j, _ = tree.find(centre)
                weights = [(names[g.group], g.weight) for g in mem.data.vertices[j].groups if g.weight > 0]
                for g in stars.vertex_groups:
                    g.remove(island)
                for name, w in weights:
                    g = stars.vertex_groups.get(name) or stars.vertex_groups.new(name=name)
                    g.add(island, w, "REPLACE")
            bm.free()


def _ears_on_head(kit, d):
    """The ear bones carry only the ear parts: the skin around them follows the head."""
    body = d["body"]
    head = body.vertex_groups["head"]
    for side in ("L", "R"):
        g = body.vertex_groups.get(f"ear_{side}")
        if g is None:
            continue
        for v in body.data.vertices:
            for e in v.groups:
                if e.group == g.index and e.weight > 0:
                    head.add([v.index], e.weight, "ADD")
        body.vertex_groups.remove(g)


def _tuft(kit, name, base, direction, side, length, width, thickness):
    """A tuft of fur: a flat fan from its base ending in three soft points (a leaf at LOD1)."""
    d = kit.V(direction).normalized()
    s = kit.V(side)
    s = (s - d * s.dot(d)).normalized()
    b = kit.V(base)
    if kit.LOD:
        return kit.blade(name, b, d, s, length, width, thickness)
    shape = [(-0.5, 0.0), (-0.56, 0.42), (-0.36, 0.8), (-0.12, 0.62), (0.0, 1.0), (0.12, 0.62), (0.36, 0.8),
             (0.56, 0.42), (0.5, 0.0)]
    return kit.flat_fan(name, [b] + [b + s * (x * width) + d * (y * length) for x, y in shape], thickness)


def _ruff(kit, d):
    """A ring of soft fur tufts round the neck's base (on the baby, a fluffy chest and a
    little crown between the ears): [(object, bone)]."""
    V = kit.V
    mats = d["mats"]
    baby = d["form"] == "hatchling"
    pieces = []
    if baby:
        for j, (x, ang, ln) in enumerate(((0.0, 0, 0.17), (-0.09, -26, 0.14), (0.09, 26, 0.14))):
            base = V((x, -0.30, 0.77))
            dirn = V((math.sin(math.radians(ang)) * 0.5, -0.22, -0.97))
            o = _tuft(kit, f"tuft_{j}", base, dirn, V((1, 0, 0)), ln * 0.8, 0.12, 0.01)
            o.data.materials.append(mats["accent_flat"])
            pieces.append((o, "chest"))
        top = kit.head_point((0.0, 0.02, 0.33))
        for j, ang in enumerate((-30, 0, 30)):
            dirn = V((math.sin(math.radians(ang)), 0.25, 1.0))
            o = _tuft(kit, f"crown_{j}", top, dirn, V((1, 0, 0)), 0.13 if ang else 0.16, 0.09, 0.01)
            o.data.materials.append(mats["accent_flat"])
            pieces.append((o, "head"))
        return pieces
    n1, ch = kit.node("neck1"), kit.node("chest")
    axis = (n1 - ch).normalized()
    count = kit.lod(9, 5)
    for j in range(count):
        t = 2 * math.pi * (j + 0.5) / count
        out = V((math.sin(t), -math.cos(t) * 0.8, 0.0))
        out = (out - axis * out.dot(axis)).normalized()
        centre = n1.lerp(ch, 0.28)
        base = centre + out * 0.30
        dirn = (out * 0.55 - axis * 0.75).normalized()
        front = out.y < -0.3
        ln = 0.40 if not front else 0.32
        o = _tuft(kit, f"ruff_{j}", base, dirn, axis.cross(out), ln, 0.32, 0.014)
        o.data.materials.append(mats["accent_flat"])
        pieces.append((o, "neck1" if front else "chest"))
    return pieces


def _wing_frame(kit, side):
    """The wing's plane in 3D (seated root, span and chord axes, scale): layout (u, v) -> point."""
    w = kit.F["wing"]
    s = -1 if side == "L" else 1
    th, ph = math.radians(w["dihedral"]), math.radians(w["droop"])
    span = kit.V((s * math.cos(th), 0, math.sin(th)))
    chord = kit.V((0, math.cos(ph), -math.sin(ph)))
    root = kit.mirror(w["seat"]["root"] if "seat" in w else w["root"], s)
    return lambda u, v: root + (span * u + chord * v) * w["scale"], span, chord


def _bat_edge(kit, pts):
    """The trailing edge from the flank to the wingtip: a bat's deep scallops between the
    finger tips."""
    def sag(a, b, depth, n):
        return [a.lerp(b, j / (n + 1)).lerp(pts["wrist"], depth * math.sin(math.pi * j / (n + 1)))
                for j in range(1, n + 1)]

    edge = [pts["body"]]
    for a, b, depth in (("body", "f4", 0.24), ("f4", "f3", 0.24), ("f3", "f2", 0.22), ("f2", "f1", 0.16)):
        edge += sag(pts[a], pts[b], depth, kit.lod(3, 1)) + [pts[b]]
    return edge


STARS = [  # (u, v, size, sparkle) in the wing layout: the hand's panels, then the flank (the cloak)
    (3.30, 0.52, 0.075, True), (3.62, 0.60, 0.04, False), (3.05, 0.85, 0.04, False),
    (3.10, 1.30, 0.065, True), (2.75, 1.55, 0.04, False),
    (1.05, 0.42, 0.065, True), (1.75, 0.78, 0.07, True), (0.62, 0.24, 0.035, False),
    (2.05, 0.30, 0.04, False), (1.95, 1.22, 0.04, False),
    (1.25, 0.70, 0.035, False), (2.12, 1.52, 0.05, True),
]


def _poke_stars(kit, bm, stars, at, span, chord):
    """Put each star's centre into the membrane (the fan triangle under it is split there), so
    the membrane passes through the star however the wing folds: the star and the membrane
    round it follow the same struts. Returns [(point, normal, tangent)] per star."""
    from mathutils import geometry
    root, scale = at(0, 0), kit.F["wing"]["scale"]

    def uv(p):
        d = p - root
        return kit.V((d.dot(span) / scale, d.dot(chord) / scale, 0))

    placed = []
    for u, v, _, _ in stars:
        q = kit.V((u, v, 0))
        for f in list(bm.faces):
            a, b, c = f.verts
            if not geometry.intersect_point_tri_2d(q, uv(a.co), uv(b.co), uv(c.co)):
                continue
            w = geometry.barycentric_transform(q, uv(a.co), uv(b.co), uv(c.co), a.co, b.co, c.co)
            n = (b.co - a.co).cross(c.co - a.co).normalized()
            centre = bm.verts.new(w)
            bm.faces.remove(f)
            for x, y in ((a, b), (b, c), (c, a)):
                bm.faces.new((x, y, centre))
            t = (span - n * span.dot(n)).normalized()
            placed.append((w.copy(), n, t))
            break
        else:
            placed.append(None)
    return placed


def wings(kit, d, rare):
    """Bat wings that are also forelegs: a strong arm, a thumb claw to lean on, four slender
    fingers and a scalloped membrane with little stars on the hand; the Eclipse's stars glow
    and a golden corona runs along its wing's trailing edge."""
    import bmesh
    F, mats, V = kit.F, d["mats"], kit.V
    baby = d["form"] == "hatchling"
    wr = F["wing"]["radii"]
    objs = []
    for side in ("L", "R"):
        s = -1 if side == "L" else 1
        pts = kit.wing_points(side)
        arm = kit.wing_arm(side, ["root", "elbow", "wrist", "thumb", "f1", "f2", "f3", "f4"],
                           [wr["root"], wr["elbow"], wr["wrist"], wr["thumb"]] + [wr["finger"]] * 4,
                           kit.lod(F["wing"]["arm_tris"], 56), mats, claws=False)
        objs.append(arm)
        # the thumb claw it leans on
        th_dir = (pts["thumb"] - pts["wrist"]).normalized()
        claw = kit.horn_mesh(f"claw_{side}", wr["thumb"] * 2.4, wr["thumb"] * 0.8, math.radians(50),
                             kit.lod(3, 2), kit.lod(4, 3))
        claw.location = pts["thumb"] - th_dir * wr["thumb"] * 0.4
        claw.rotation_euler = V((0, 0, 1)).rotation_difference(th_dir).to_euler()
        claw.data.materials.append(mats["horn"])
        objs.append(claw)
        # the membrane: a fan from the wrist
        edge = _bat_edge(kit, pts)
        ring = [pts["wrist"], pts["elbow"], pts["root"]] + edge
        at, span, chord = _wing_frame(kit, side)
        bm = bmesh.new()
        vs = [bm.verts.new(p) for p in ring]
        for i in range(1, len(vs) - 1):
            bm.faces.new((vs[0], vs[i], vs[i + 1]))
        stars = STARS if not kit.LOD else STARS[::2]
        if baby:
            stars = [st for st in stars if st[3]]
        placed = _poke_stars(kit, bm, stars, at, span, chord)
        th = F["wing"]["thickness"]
        mem = kit.mesh_object(f"membrane_{side}", bm)
        mod = mem.modifiers.new("thick", "SOLIDIFY")
        mod.thickness = th
        mod.offset = 0
        mod.use_rim = False
        kit.apply_modifiers(mem)
        mem.data.materials.append(mats["membrane"])
        objs.append(mem)
        # stars on both faces of the membrane, centred on the pokes
        sb = bmesh.new()
        sides = []
        for (u, v, size, sparkle), spot in zip(stars, placed):
            if spot is None:
                continue
            c, n, t = spot
            size *= F["wing"]["scale"] * (1.6 if baby else 1.0)
            for face in (1, -1):
                before = len(sb.faces)
                _star(sb, c + n * face * th * 0.8, t, n.cross(t), size, sparkle)
                sides += [n * face] * (len(sb.faces) - before)
        sb.faces.ensure_lookup_table()
        sb.normal_update()
        for f, outward in zip(sb.faces, sides):
            if f.normal.dot(outward) < 0:
                f.normal_flip()
        star_obj = kit.mesh_object(f"stars_{side}", sb)
        star_obj.data.materials.append(mats["glow_flat" if rare else "horn"])
        objs.append(star_obj)
        if rare:  # the corona: a glowing band along the trailing edge
            band = edge[::2] + ([edge[-1]] if len(edge) % 2 == 0 else [])
            corona = kit.tube(f"corona_{side}", band, [th * 1.5] * len(band), ring=3, cap=False)
            corona.data.materials.append(mats["glow_flat"])
            objs.append(corona)
    return objs


def accent(kit, body):
    """The horizon's glow under a dusk sky: throat, chest and belly, the inner thighs and the
    ruff round the neck's base in the accent colour."""
    import mathutils
    mask = kit.mask_attr(body)
    F = kit.F
    mk = F["mask"]
    baby = F["name"] == "hatchling"
    down = mathutils.Vector((0, -0.5, -0.87)).normalized()
    n1, ch = kit.node("neck1"), kit.node("chest")
    for v in body.data.vertices:
        x, y, z = v.co
        a = min(1.0, max(0.0, (v.normal.dot(down) - 0.15) / 0.45))
        if abs(x) > mk["max_x"] or z > mk["max_z"] or z < mk["min_z"]:
            a = 0.0
        if mk.get("tail_cut") and y > mk["tail_cut"][0] and z < mk["tail_cut"][1]:
            a = 0.0
        if not baby:  # the ruff: a soft collar round the neck's base
            p = mathutils.Vector((x, y, z))
            axis = (n1 - ch).normalized()
            along = (p - ch).dot(axis) / (n1 - ch).length
            if 0.35 < along < 1.05:
                edge = min(along - 0.35, 1.05 - along) / 0.2
                a = max(a, min(1.0, edge))
        else:  # the baby's face: a pale muzzle
            if y < -0.42 and z < 1.08:
                a = max(a, min(1.0, (-0.42 - y) / 0.08))
        mask.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


def texture(tx, nt, p, form):
    """R: a dusky mantle over the back, head and tail (night on top of the sky's gradient);
    G: little star freckles over the back; B: the Eclipse's glowing star-dust."""
    r = tx.mul(nt, tx.smoothstep(nt, tx.normal_z(nt), -0.05, 0.6),
               tx.madd(nt, tx.blotches(nt, 1.2, (0.3, 0.7), 2.0, where=tx.const(nt, 1.0)), 0.25, 0.75))
    g = tx.maxi(nt, tx.spots(nt, p["spot_cell"] * 0.8, keep=0.5, size=(0.24, 0.15), where=tx.top(nt)),
                tx.spots(nt, p["spot_cell"] * 1.6, keep=0.75, size=(0.2, 0.12), where=tx.top(nt), seed_offset=3.7))
    b = tx.maxi(nt, tx.spots(nt, p["spot_cell"] * 0.7, keep=0.45, size=(0.24, 0.15), where=tx.upper(nt),
                             seed_offset=2.3),
                tx.spots(nt, p["spot_cell"] * 1.4, keep=0.7, size=(0.2, 0.12), where=tx.top(nt), seed_offset=5.1))
    return {"r": r, "g": g, "b": b}
