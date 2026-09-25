"""The Curlstone (Stone, common): an armoured stone dragon that curls up into a ball, like a
pangolin. Concept: docs/art/concept/dragons/dragon_curlstone.jpg (R11; reference only, D74:
the model is lower and longer than the drawing, a pangolin rather than a turtle).

Design notes
  * Baby: a little round pebble, half curled: a big round head with huge eyes and a long
    button snout, a round tummy, stubby legs, the thick little tail curled round its side,
    tiny overlapping plates down its back and three amber crystal buds on its head.
  * Adult: low, long and sturdy under a high domed back, a calm and dependable mountain
    guardian. Rounded stone plates overlap down the back and the thick tail like a pangolin's
    scales, in staggered rows, each draped on the skin with its free rear edge lifted over the
    next and rimmed in the pattern colour (the rims keep plates apart where the toon light
    alone would merge them); the skin between them is painted with softer scutes and shadowed
    under the plates. A long, gentle digging snout with a small, kind, fangless mouth, big
    eyes, big digging front claws, a thick tail ending in a rounded stone club, a crown of
    warm amber crystals (glowing with its heart) and a few crystal points peeking out between
    the plates. Short, broad wings fold flat along its flanks, tucked under the lowest plates.
  * Motion (plans/curlstone.py): a low, steady waddle and a determined trot; it tucks and
    rolls like a boulder to run; it curls into a ball to sleep and to sulk, hides its face in
    a half curl when shy, sits up like a pangolin and rocks on its round back for a rub.
  * Colours: Sandstone (sandstone plates rimmed in sienna on slate-grey skin with faint
    strata), Granite (grey plates rimmed in charcoal, speckled pale skin, a pink feldspar
    belly), Basalt (near-black stone with rusty rims, veins and belly) and the rare Geode:
    dusky violet stone, plates rimmed in amethyst, glowing amethyst seams and bigger
    amethyst crystal clusters on its crown and between its plates (heart and crystals
    glowing violet).
  * Egg: a smooth river stone with sandy bands, in each colouring's own stones.
"""
import math

# ------------------------------------------------------------------------------ who it is
META = dict(
    name="curlstone", title="Curlstone", dex=3, element="Stone", parents=(), rarity="common",
    plan="curlstone", size=1.0,
    stats=dict(wing=3, wit=5, might=8, breath=4, stamina=8),
    manners=("Gentle", "Stubborn", "Shy", "Sleepy", "Brave"),
    traits=("Sturdy", "Keen Nose", "Sure-Footed", "Deep Sleeper", "Gentle Giant", "Ironhide"),
    rare_variant=3, rare_replaces=True,
    blurb="Slow, steady and kind; when the world gets loud it curls up into a stone and waits.",
)

# Plates are the horn colour with rims of the pattern colour (which also paints the skin's
# pattern): the rims keep neighbouring plates apart where the toon light alone would merge them.
VARIANTS = [
    dict(name="Sandstone", base=(0.50, 0.52, 0.56), accent=(0.93, 0.82, 0.64), pattern=(0.55, 0.33, 0.20),
         horn=(0.80, 0.58, 0.38), membrane=(0.90, 0.62, 0.38), iris=(0.45, 0.28, 0.12), glow=(1.0, 0.58, 0.16),
         pattern_channel="g", glow_channel=None),
    dict(name="Granite", base=(0.68, 0.67, 0.66), accent=(0.92, 0.76, 0.72), pattern=(0.26, 0.26, 0.28),
         horn=(0.56, 0.55, 0.56), membrane=(0.82, 0.70, 0.68), iris=(0.36, 0.30, 0.26), glow=(1.0, 0.58, 0.16),
         pattern_channel="r", glow_channel=None),
    dict(name="Basalt", base=(0.30, 0.28, 0.30), accent=(0.78, 0.44, 0.24), pattern=(0.56, 0.27, 0.14),
         horn=(0.21, 0.20, 0.22), membrane=(0.60, 0.32, 0.20), iris=(0.95, 0.62, 0.20), glow=(1.0, 0.58, 0.16),
         pattern_channel="b", glow_channel=None),
    dict(name="Geode", base=(0.40, 0.36, 0.46), accent=(0.84, 0.76, 0.92), pattern=(0.68, 0.46, 0.94),
         horn=(0.30, 0.26, 0.36), membrane=(0.60, 0.46, 0.78), iris=(0.72, 0.42, 0.96), glow=(0.80, 0.50, 1.0),
         pattern_channel="r", glow_channel="b"),
]

EGG = dict(height=0.96, width=0.41, point=1.0, asym=0.07, speckle="bands",
           speckle_params=dict(levels=[0.16, 0.30, 0.44, 0.78, 0.88], size=(0.02, 0.05)),
           colors=[((0.86, 0.76, 0.62), (0.66, 0.48, 0.32)), ((0.74, 0.73, 0.72), (0.52, 0.50, 0.52)),
                   ((0.32, 0.31, 0.33), (0.72, 0.40, 0.22)), ((0.42, 0.37, 0.48), (0.72, 0.52, 0.94))])


def _mirrored(center, sides):
    nodes = dict(center)
    for name, (p, r) in sides.items():
        nodes[f"{name}_R"] = ((p[0], p[1], p[2]), r)
        nodes[f"{name}_L"] = ((-p[0], p[1], p[2]), r)
    return nodes


# ------------------------------------------------------------------------------ grown
# Faces -Y, Z up; about 6 units long. radius = (side, vertical). Low and long on short legs:
# the back arches highest over the loins, the head held up on a short, thick neck.
GROWN_NODES = _mirrored({
    "tail_tip": ((0, 3.36, 0.28), (0.19, 0.19)),
    "tail4": ((0, 2.98, 0.38), (0.28, 0.27)),
    "tail3": ((0, 2.50, 0.60), (0.39, 0.37)),
    "tail2": ((0, 1.92, 0.88), (0.52, 0.50)),
    "hips": ((0, 1.22, 1.16), (0.68, 0.76)),
    "loin": ((0, 0.58, 1.36), (0.82, 0.94)),
    "belly": ((0, -0.08, 1.28), (0.84, 0.92)),
    "chest": ((0, -0.74, 1.02), (0.74, 0.78)),
    "neck2": ((0, -1.32, 1.02), (0.52, 0.52)),
    "neck3": ((0, -1.68, 1.20), (0.42, 0.42)),
    "head": ((0, -2.02, 1.36), (0.50, 0.44)),
    "muzzle": ((0, -2.44, 1.20), (0.27, 0.25)),
    "snout": ((0, -2.78, 1.04), (0.18, 0.16)),
}, {
    "shoulder": ((0.50, -0.66, 0.80), (0.31, 0.34)),
    "elbow": ((0.64, -0.76, 0.46), (0.25, 0.25)),
    "wrist": ((0.64, -0.82, 0.19), (0.20, 0.20)),
    "toe_f": ((0.66, -1.12, 0.10), (0.24, 0.10)),
    "hipj": ((0.50, 1.16, 0.94), (0.39, 0.44)),
    "knee": ((0.62, 0.90, 0.53), (0.29, 0.29)),
    "ankle": ((0.62, 1.20, 0.21), (0.21, 0.21)),
    "toe_b": ((0.64, 0.94, 0.10), (0.24, 0.10)),
})
SPINE = ["tail_tip", "tail4", "tail3", "tail2", "hips", "loin", "belly", "chest", "neck2", "neck3", "head", "muzzle",
         "snout"]
GROWN_EDGES = list(zip(SPINE, SPINE[1:]))
for _side in ("L", "R"):
    GROWN_EDGES += [("chest", f"shoulder_{_side}"), (f"shoulder_{_side}", f"elbow_{_side}"),
                    (f"elbow_{_side}", f"wrist_{_side}"), (f"wrist_{_side}", f"toe_f_{_side}"),
                    ("hips", f"hipj_{_side}"), (f"hipj_{_side}", f"knee_{_side}"),
                    (f"knee_{_side}", f"ankle_{_side}"), (f"ankle_{_side}", f"toe_b_{_side}")]

# Individual variety (the genome's build): gentle, every Curlstone still reads as one.
BUILDS = {
    "neutral": {},
    "sturdy": {"chest": (1.07, 0.98), "belly": (1.07, 0.98), "loin": (1.06, 0.98), "hips": (1.05, 1.0),
               "arm_up": (1.08, 0.97), "leg_up": (1.07, 0.97), "tail2": (1.06, 0.97)},
    "sleek": {"chest": (0.95, 1.02), "belly": (0.93, 1.03), "loin": (0.93, 1.03), "tail3": (0.94, 1.04),
              "tail4": (0.94, 1.05)},
    "long": {"belly": (0.97, 1.07), "loin": (0.97, 1.07), "tail2": (0.97, 1.06), "tail3": (0.96, 1.08),
             "tail4": (0.96, 1.08), "snout": (0.97, 1.08)},
}

# Plate bands: (bone, t along it, angles round the spine (0 top, + to its right), (width,
# length, height), lift of the rear edge). Staggered like shingles; each band's rear edge
# rides over the next band.
GROWN_PLATES = [
    ("head", -0.30, (0, 48, -48), (0.40, 0.42, 0.10), 0.06),
    ("neck3", 0.40, (-28, 28, 82, -82), (0.46, 0.50, 0.12), 0.07),
    ("neck2", 0.45, (0, 58, -58), (0.56, 0.60, 0.14), 0.09),
    ("chest", 0.45, (-26, 26, 78, -78), (0.76, 0.96, 0.22), 0.12),
    ("belly", 0.50, (0, 50, -50, 98, -98), (0.80, 1.02, 0.24), 0.13),
    ("loin", 0.50, (-25, 25, 75, -75), (0.82, 1.02, 0.24), 0.13),
    ("hips", 0.50, (0, 50, -50, 98, -98), (0.74, 0.96, 0.22), 0.12),
    ("tail1", 0.50, (-30, 30, 88, -88), (0.62, 0.84, 0.17), 0.10),
    ("tail2", 0.50, (0, 60, -60), (0.54, 0.76, 0.15), 0.09),
    ("tail3", 0.50, (-34, 34), (0.44, 0.66, 0.13), 0.08),
]

GROWN = dict(
    name="grown", nodes=GROWN_NODES, edges=GROWN_EDGES, body="skin", relax=0.45,
    body_tris=1150, body_tris_lod1=380, export_scale=1.0,
    young={
        "bones": {
            "head": (1.08, 1.0, 1.08), "snout": (0.9, 0.74, 0.94),
            "neck2": (0.8, 0.62), "neck3": (0.82, 0.62),
            "chest": (0.72, 0.62), "belly": (0.70, 0.60), "loin": (0.70, 0.60), "hips": (0.72, 0.62),
            "tail1": (0.72, 0.58), "tail2": (0.72, 0.56), "tail3": (0.74, 0.56), "tail4": (0.8, 0.6),
            "arm_up": (0.76, 0.66), "arm_lo": (0.78, 0.66), "hand": (0.86, 0.8),
            "leg_up": (0.76, 0.66), "leg_lo": (0.78, 0.66), "foot": (0.86, 0.8),
        },
        "parts": {"eyes": 1.3, "horns": 0.5, "frill": 0.7, "wings": 0.6, "spikes": 0.92,
                  "tail_tip": 0.75, "heart": 0.85, "runes": 0.6},
    },
    # base_pose is Euler in each bone's own frame: +x tips a forward bone (neck, head) down
    # and a backward one (the tail) up.
    young_pose={"neck2": -8, "head": -6},
    base_pose={"neck2": (-14, 0, 0), "neck3": (-8, 0, 0), "head": (12, 0, 0)},
    builds=BUILDS,
    eyes=dict(at=(0.29, -2.28, 1.48), out=(0.52, -0.80, 0.26), iris=(0.165, 0.185, 0.08),
              pupil=(0.10, 0.124, 0.03), slit=(0.3, 1.1),
              glints=((-0.022, 0.034, 0.016), (0.016, -0.034, 0.008)), seg=(12, 2, 8, 2)),
    head=dict(origin=(0, -2.02, 1.36), k=1.45, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=False),
    tail_k=1.0,
    heart=dict(at=(0, -1.42, 0.68), size=0.13),
    wing=dict(root=(0.46, -0.55, 1.48), scale=1.1, dihedral=30, droop=8,
              layout={"root": (0.0, 0.0), "elbow": (0.62, 0.22), "wrist": (1.12, -0.02), "f1": (1.92, 0.30),
                      "f2": (1.74, 0.94), "f3": (1.22, 1.38), "body": (0.0, 1.15)},
              radii={"root": 0.10, "elbow": 0.078, "wrist": 0.064, "finger": 0.034, "tip": 0.014},
              arm_tris=120, thickness=0.014, style="sail"),
    mask=dict(max_x=0.56, max_z=1.0, min_z=-1.0, tail_cut=None),
    inset={"eyes": 0.02, "horns": 0.03, "spikes": 0.012, "frill": 0.03, "heart": -0.05, "runes": 0.03,
           "tail_tip": 0.1},
    face=dict(nostril=(0.062, -2.93, 1.12), nostril_r=(0.03, 0.02, 0.01), mouth_r=0.013,
              mouth=lambda side, a: (side * 0.19 * a ** 0.7, -2.88 + 0.52 * a ** 1.5, 0.96 + 0.10 * a * a)),
    jaw_hinge=(0, -2.26, 1.07),
    mouth_detail=dict(depth=0.18, fade=0.16, width=0.28, tooth=(0.01, 0.017), fang=(0.013, 0.028),
                      tongue=(0.065, 0.15, 0.016), fangs=[]),
    skin=dict(stripe=0.26, spot_cell=0.09, crack_cell=0.42, scute=0.2, strata_z=0.8, strata_y=-1.45, ao=0.6),
    plates=GROWN_PLATES,
    # (bone, t, angle, length, how many): the commons' few points, the Geode's clusters
    crystals=([("chest", 0.95, 52, 0.40, 1), ("chest", 0.95, -52, 0.40, 1), ("loin", 0.95, 50, 0.46, 1),
               ("loin", 0.95, -50, 0.46, 1)],
              [("chest", 0.95, 52, 0.46, 2), ("chest", 0.95, -52, 0.46, 2), ("loin", 0.95, 50, 0.56, 2),
               ("loin", 0.95, -50, 0.56, 2)]),
)

# ------------------------------------------------------------------------------ hatchling
# A round pebble: the head a third of it, a round tummy, stubby legs; the tail is modelled
# straight and curled round its side in the idle pose (base_pose).
HATCH_NODES = _mirrored({
    "tail_tip": ((0, 1.10, 0.14), None),
    "tail4": ((0, 0.96, 0.16), None),
    "tail3": ((0, 0.80, 0.21), None),
    "tail2": ((0, 0.62, 0.30), None),
    "hips": ((0, 0.40, 0.46), None),
    "loin": ((0, 0.22, 0.62), None),
    "belly": ((0, 0.02, 0.64), None),
    "chest": ((0, -0.16, 0.56), None),
    "neck2": ((0, -0.28, 0.64), None),
    "neck3": ((0, -0.38, 0.76), None),
    "head": ((0, -0.50, 0.92), None),
    "muzzle": ((0, -0.76, 0.84), None),
    "snout": ((0, -0.98, 0.77), None),
}, {
    "shoulder": ((0.19, -0.12, 0.34), None),
    "elbow": ((0.22, -0.16, 0.21), None),
    "wrist": ((0.23, -0.19, 0.08), None),
    "toe_f": ((0.24, -0.32, 0.05), None),
    "hipj": ((0.19, 0.36, 0.36), None),
    "knee": ((0.22, 0.28, 0.21), None),
    "ankle": ((0.23, 0.38, 0.09), None),
    "toe_b": ((0.24, 0.24, 0.05), None),
})

HATCH_META = [
    ("ell", (0, -0.52, 0.95), (0.34, 0.31, 0.31)),      # big round head
    ("chain", [(0, -0.72, 0.86), (0, -0.87, 0.80), (0, -0.98, 0.77)], [0.15, 0.105, 0.085]),  # long snout
    ("ball", (0, -1.00, 0.78), 0.094),                  # the button at its tip
    ("ell", (0, -0.66, 0.80), (0.14, 0.14, 0.09)),      # chin
    ("chain", [(0, -0.24, 0.60), (0, -0.36, 0.75)], [0.22, 0.23]),
    ("ell", (0, 0.10, 0.60), (0.37, 0.37, 0.41)),       # the pebble: a round, high back
    ("ell", (0, -0.12, 0.52), (0.28, 0.24, 0.30)),      # chest
    ("ell", (0, 0.33, 0.47), (0.30, 0.23, 0.30)),       # haunches
    ("ell", (0, 0.06, 0.37), (0.26, 0.28, 0.20)),       # tummy
    ("chain", [(0, 0.50, 0.38), (0, 0.66, 0.28), (0, 0.82, 0.21), (0, 0.97, 0.16), (0, 1.09, 0.14)],
     [0.15, 0.12, 0.10, 0.085, 0.075]),               # a thick little tail
]
for _s in (-1, 1):
    HATCH_META += [
        ("ball", (_s * 0.16, -0.70, 0.84), 0.11),       # cheeks
        ("chain", [(_s * 0.19, -0.12, 0.34), (_s * 0.22, -0.16, 0.21), (_s * 0.23, -0.19, 0.08)],
         [0.1, 0.09, 0.087]),
        ("ell", (_s * 0.235, -0.26, 0.055), (0.095, 0.12, 0.06)),   # front paws
        ("ell", (_s * 0.19, 0.34, 0.31), (0.13, 0.16, 0.16)),       # thighs
        ("chain", [(_s * 0.22, 0.32, 0.21), (_s * 0.23, 0.38, 0.09)], [0.09, 0.08]),
        ("ell", (_s * 0.235, 0.28, 0.055), (0.095, 0.13, 0.06)),    # hind paws
    ]

HATCH_PLATES = [
    ("chest", 0.5, (-27, 27, 80, -80), (0.31, 0.32, 0.065), 0.03),
    ("belly", 0.5, (0, 52, -52, 102, -102), (0.33, 0.33, 0.07), 0.03),
    ("loin", 0.5, (-27, 27, 80, -80), (0.33, 0.33, 0.07), 0.03),
    ("hips", 0.5, (0, 52, -52), (0.30, 0.31, 0.065), 0.028),
    ("tail1", 0.5, (-30, 30), (0.22, 0.25, 0.05), 0.025),
    ("tail2", 0.55, (0,), (0.18, 0.21, 0.04), 0.02),
]

HATCH = dict(
    name="hatchling", nodes=HATCH_NODES, meta=HATCH_META, body="meta",
    body_tris=1350, body_tris_lod1=520, export_scale=1.0,
    young={
        "bones": {name: ((0.76, 0.76) if name in ("head", "snout") else (0.64, 0.64))
                  for name in ("hips", "loin", "belly", "chest", "neck2", "neck3", "head", "snout", "tail1",
                               "tail2", "tail3", "tail4", "arm_up", "arm_lo", "hand", "leg_up", "leg_lo", "foot")},
        "parts": {"eyes": 1.12, "horns": 0.6, "frill": 0.9, "wings": 0.56, "spikes": 0.9,
                  "tail_tip": 0.9, "heart": 1.0, "runes": 0.9},
    },
    base_pose={"neck2": (8, 0, 0), "head": (-12, 0, 0), "tail1": (8, 0, 20), "tail2": (0, 0, 40),
               "tail3": (0, 0, 45), "tail4": (0, 0, 45)},
    builds={"neutral": {}, "sturdy": {}, "sleek": {}, "long": {}},
    eyes=dict(at=(0.17, -0.74, 0.99), out=(0.46, -0.88, 0.12), iris=(0.118, 0.132, 0.066),
              pupil=(0.08, 0.094, 0.03), slit=(0.32, 1.1),
              glints=((-0.028, 0.044, 0.028), (0.024, -0.042, 0.013)), seg=(14, 3, 12, 2)),
    head=dict(origin=(0, -0.52, 0.95), k=0.9, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=True),
    tail_k=0.36,
    heart=dict(at=(0, -0.34, 0.46), size=0.072),
    wing=dict(root=(0.16, -0.06, 0.68), scale=0.15, dihedral=25, droop=5,
              layout={"root": (0.0, 0.0), "elbow": (0.62, 0.22), "wrist": (1.12, -0.02), "f1": (1.92, 0.30),
                      "f2": (1.74, 0.94), "f3": (1.22, 1.38), "body": (0.0, 1.15)},
              radii={"root": 0.04, "elbow": 0.032, "wrist": 0.028, "finger": 0.012, "tip": 0.006},
              arm_tris=100, thickness=0.008, style="sail"),
    mask=dict(max_x=0.24, max_z=1.0, min_z=0.1, tail_cut=None),
    inset={"eyes": 0.03, "horns": 0.02, "spikes": 0.006, "frill": 0.02, "heart": -0.03, "runes": 0.015,
           "tail_tip": 0.04},
    face=dict(nostril=(0.036, -1.075, 0.80), nostril_r=(0.02, 0.014, 0.008), mouth_r=0.010,
              mouth=lambda side, a: (side * 0.10 * a ** 0.8, -1.02 + 0.2 * a ** 1.6, 0.715 + 0.03 * a * a)),
    jaw_hinge=(0, -0.78, 0.74),
    mouth_detail=dict(depth=0.12, fade=0.1, width=0.2, tooth=(0.006, 0.01), fang=(0.009, 0.018),
                      tongue=(0.045, 0.08, 0.011), fangs=[]),
    skin=dict(stripe=0.12, spot_cell=0.045, crack_cell=0.18, scute=0.09, strata_z=9.0, strata_y=-0.3, ao=0.25),
    plates=HATCH_PLATES, domed_from=0.28,
    crystals=([], [("loin", 0.9, 45, 0.1, 1), ("loin", 0.9, -45, 0.1, 1)]),
)

FORMS = {"grown": GROWN, "hatchling": HATCH}


# ------------------------------------------------------------------------------ part shapes
def _bone_axis(kit, bone):
    """(head, tail) of a plan bone in the current form's rest pose."""
    PLAN = kit.PLAN
    extra = kit.extra_points()
    for name, h, t, _ in PLAN.BONES:
        if name == bone:
            get = lambda n: kit.V(extra[n]) if n in extra else kit.node(n)  # noqa: E731
            return get(h), get(t)
    raise KeyError(bone)


def _body_bvh(kit, body):
    from mathutils.bvhtree import BVHTree
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(body.data)
    bm.transform(body.matrix_world)
    bvh = BVHTree.FromBMesh(bm)
    bm.free()
    return bvh


def _outer_hit(bvh, origin, direction, far=10.0):
    """The outermost surface hit along a ray (the last of several), with its normal."""
    hit, nrm, o = None, None, origin.copy()
    for _ in range(8):
        h, n, _, _ = bvh.ray_cast(o, direction, far)
        if h is None:
            break
        hit, nrm, o = h, n, h + direction * 1e-4
    return hit, nrm


def _first_hit(bvh, origin, direction, far=10.0):
    h, n, _, _ = bvh.ray_cast(origin, direction, far)
    return h, n


def _plate(kit, bvh, name, centre, normal, back, size, lift, domed=True):
    """A rounded stone plate draped over the skin, shaped like a pangolin's scale: broad and
    round in front, a softly pointed free edge behind, domed on top (a ring and an apex; a
    plain lens at LOD1 or when not domed). Its front edge lies on the skin and its rear edge
    is lifted by `lift`, so it rides over the plate behind. The top and the underside don't
    share their rim vertices: the top's normals stay upward (no rim-lit outline), and the rim
    is a crisp edge. Its origin is the skin point under its middle (where the kit seats it)."""
    import bmesh
    V = kit.V
    w, ln, h = size
    domed = domed and not kit.LOD
    n = kit.lod(7 if not domed else 6, 4)
    z = V(normal).normalized()
    y = (V(back) - z * V(back).dot(z)).normalized()
    x = y.cross(z).normalized()
    c = V(centre)

    def drape(px, py, up):
        """The skin point under plate coordinates (px, py) plus `up` along the plate normal."""
        q = c + x * px + y * py
        hit, _ = _first_hit(bvh, q + z * 0.6, -z, 1.4)
        base = hit if hit is not None and (hit - q).length < 0.5 else q
        return base + z * up

    def lift_at(py):
        u = min(1.0, max(0.0, (py / (ln * 0.5) + 0.15) / 1.15))
        return lift * u ** 1.4

    def outline(a, k):
        sx = math.sin(a) * w * 0.5 * (1.0 - 0.2 * max(0.0, -math.cos(a))) * k
        sy = (-math.cos(a) * ln * 0.5 * (1.0 + 0.1 * max(0.0, -math.cos(a))) + ln * 0.04) * k
        return sx, sy

    bm = bmesh.new()
    # (four corners at LOD1: a broad diamond, front, sides and the free rear point)
    angles = [2 * math.pi * (i + (0.0 if n == 4 else 0.5)) / n for i in range(n)]
    rim_top, rim_bot, mid_top, mid_rim = [], [], [], []
    for a in angles:
        sx, sy = outline(a, 1.0)
        p = drape(sx, sy, lift_at(sy) - 0.006) - c
        rim_top.append(bm.verts.new(p))
        rim_bot.append(bm.verts.new(p))
        if domed:
            mx, my = outline(a, 0.8)
            q = drape(mx, my, h * 0.55 + lift_at(my)) - c
            mid_top.append(bm.verts.new(q))
            mid_rim.append(bm.verts.new(q))
    apex = bm.verts.new(drape(0.0, ln * 0.06, h + lift_at(ln * 0.06)) - c)
    bottom = bm.verts.new(drape(0.0, 0.0, -h * 0.4) - c)
    inner = mid_top if domed else rim_top
    for k in range(n):
        j = (k + 1) % n
        bm.faces.new((apex, inner[k], inner[j])).material_index = 0
        if domed:  # the free (rear) edge is rimmed in the pattern colour: a scale's crescent
            rear = -math.cos(angles[k] + math.pi / n) > -0.35
            if rear:
                bm.faces.new((rim_top[k], rim_top[j], mid_rim[j], mid_rim[k])).material_index = 1
            else:
                bm.faces.new((rim_top[k], rim_top[j], mid_top[j], mid_top[k])).material_index = 0
        bm.faces.new((bottom, rim_bot[j], rim_bot[k])).material_index = 1
    bm.normal_update()
    for f in bm.faces:
        want = -1.0 if bottom in f.verts else 1.0
        if f.normal.dot(z) * want < 0:
            f.normal_flip()
    bmesh.ops.triangulate(bm, faces=list(bm.faces))
    obj = kit.mesh_object(name, bm, c)
    kit.smooth(obj)
    return obj


def surface_point(kit, bvh, bone, t, angle):
    """Where a ray from a bone's axis (a fraction t along it) at an angle round the body (0 on
    top, + to its right) leaves the skin: (point, outward normal, the bone's axis)."""
    V = kit.V
    head, tail = _bone_axis(kit, bone)
    axis = (tail - head).normalized()
    up = V((0, -axis.z, axis.y))
    if up.z < 0:
        up = -up
    side = axis.cross(up).normalized()
    if side.x < 0:
        side = -side
    a = math.radians(angle)
    direction = (up * math.cos(a) + side * math.sin(a)).normalized()
    hit, nrm = _outer_hit(bvh, head.lerp(tail, t), direction)
    if hit is None:
        return None, None, axis
    return hit, (V(nrm).normalized() + direction).normalized(), axis


def plates(kit, d, bands, material="horn", rim="pattern_flat", scale=1.0, bvh=None):
    """Plate bands along the spine: [(object, bone)]. Each plate is cast from the bone's axis
    out through the skin at its angle round the body, draped on the skin there. Plates wider
    than `domed_from` are domed; the rest (the tail, the lowest rows) plain lenses."""
    bvh = bvh or _body_bvh(kit, d["body"])
    mats = d["mats"]
    out = []
    for bone, t, angles, size, lift in bands:
        for ang in angles:
            hit, normal, axis = surface_point(kit, bvh, bone, t, ang)
            if hit is None:
                continue
            grow = kit.lod(1.0, 1.3)  # LOD1's four-cornered plates are grown to cover as much
            sz = (size[0] * scale * grow, size[1] * scale * grow, size[2] * scale)
            domed = sz[0] >= kit.F.get("domed_from", 0.6) and abs(ang) < 90
            o = _plate(kit, bvh, f"plate_{bone}_{ang}", hit, normal, axis, sz, lift * scale, domed=domed)
            o.data.materials.append(mats[material])
            o.data.materials.append(mats[rim])
            out.append((o, bone))
    return out


def crystal(kit, name, base, direction, length, radius, sides=None, tip=0.34):
    """A faceted crystal: a prism with a pointed tip, every face flat (split vertices, so the
    game's vertex normals keep the facets). Its origin is the base."""
    import bmesh
    V = kit.V
    sides = sides or kit.lod(5, 3)
    d = V(direction).normalized()
    a = V((1, 0, 0)) if abs(d.x) < 0.9 else V((0, 1, 0))
    s = d.cross(a).normalized()
    u = s.cross(d).normalized()
    bm = bmesh.new()
    lo, hi = [], []
    for k in range(sides):
        ang = 2 * math.pi * k / sides + 0.3
        off = (s * math.cos(ang) + u * math.sin(ang)) * radius
        lo.append(bm.verts.new(off * 0.92 - d * radius * 0.6))
        hi.append(bm.verts.new(off + d * length * (1 - tip)))
    apex = bm.verts.new(d * length)
    for k in range(sides):
        bm.faces.new((lo[k], lo[(k + 1) % sides], hi[(k + 1) % sides], hi[k]))
        bm.faces.new((hi[k], hi[(k + 1) % sides], apex))
    bm.normal_update()
    for f in bm.faces:
        if f.normal.dot(f.calc_center_median()) < 0:
            f.normal_flip()
    bmesh.ops.split_edges(bm, edges=list(bm.edges))
    obj = kit.mesh_object(name, bm, V(base))
    return obj


def claw(kit, name, base, direction, length, radius, big=True):
    """A curved digging claw (a small horn bent down over the toe)."""
    V = kit.V
    seg, ring = (kit.lod(2, 1), 3) if big else (1, 3)
    o = kit.horn_mesh(name, length, radius, math.radians(55 if big else 30), seg, ring, taper=0.8, tip=0.1)
    d = V(direction).normalized()
    # horn_mesh grows along +Z and curls toward +Y: point +Z along the claw, curl downward
    o.rotation_euler = d.to_track_quat("Z", "Y").to_euler()
    o.rotation_euler.rotate_axis("Z", math.pi)
    o.location = V(base)
    return o


def lit_glow(kit, d, name="glow_flat", colour="glow", k=0.5):
    """A preview material for glowing parts as the game draws them (lit colour plus the glow
    added on top), not the kit's flat emission, so crystal facets read in the review. It is
    named "<name>.001": the exporter reads the name before the dot for the palette."""
    c = kit.variant_colors(d["variant"])[colour]
    mat = kit.toon_material(name, c)
    nt = mat.node_tree
    add = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeMix" and n.blend_type == "ADD")
    em = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeEmission")
    glow = nt.nodes.new("ShaderNodeMix")
    glow.data_type, glow.blend_type = "RGBA", "ADD"
    glow.inputs["Factor"].default_value = 1.0
    glow.inputs["B"].default_value = (*[x * k for x in c], 1)
    nt.links.new(add.outputs["Result"], glow.inputs["A"])
    nt.links.new(glow.outputs["Result"], em.inputs["Color"])
    return mat


# ------------------------------------------------------------------------------ hooks
def parts(kit, d):
    F, mats, V = kit.F, d["mats"], kit.V
    baby = d["form"] == "hatchling"
    out = []
    # Plates down the back and the tail (and a few on the forehead and haunches).
    out.append(("spikes", 0, plates(kit, d, F["plates"])))
    # The crown: warm amber crystals fanning over the brow (three buds on a baby). The rare
    # Geode's crown is a bigger amethyst cluster.
    hk = F["head"]["k"]
    crown_specs = [((0.0, 0.10, 0.25), (0.0, 0.3, 1.0), 0.34, 0.068),
                   ((0.10, 0.05, 0.23), (0.35, 0.15, 1.0), 0.28, 0.06),
                   ((0.17, -0.02, 0.17), (0.75, 0.05, 0.85), 0.21, 0.05),
                   ((0.07, 0.19, 0.20), (0.3, 0.8, 0.8), 0.2, 0.048)]
    if baby:
        crown_specs = crown_specs[:2]

    def crown(specs, k_len, material, tag):
        objs = []
        for i, (at, dirn, length, r) in enumerate(specs):
            for s in (-1, 1):
                if at[0] == 0.0 and s < 0:
                    continue
                o = crystal(kit, f"crown{tag}_{i}_{s}", kit.head_point(at, s), kit.mirror(dirn, s),
                            length * hk * k_len, r * hk)
                o.data.materials.append(material)
                objs.append((o, "head"))
        return objs

    glow = lit_glow(kit, d, k=0.28)
    out.append(("horns", 0, crown(crown_specs, 0.55 if baby else 1.0, glow, "a")))
    geode = crown_specs + ([((0.12, 0.13, 0.22), (0.5, 0.5, 1.0), 0.28, 0.06)] if not baby else [])
    out.append(("horns", 1, crown(geode, 0.7 if baby else 1.25, glow, "g")))
    # Crystal points peeking out between the plates; the Geode's are bigger amethyst clusters.
    bvh = _body_bvh(kit, d["body"])
    for variant, spots in ((0, F["crystals"][0]), (1, F["crystals"][1])):
        pieces = []
        for i, (bone, t, ang, length, count) in enumerate(spots):
            hit, normal, axis = surface_point(kit, bvh, bone, t, ang)
            if hit is None:
                continue
            side = axis.cross(normal).normalized()
            for j in range(count):
                spread = (j - (count - 1) / 2) * 0.55
                dirn = (normal * 1.0 + axis * 0.35 + side * spread).normalized()
                base = hit + side * spread * length * 0.35 - normal * length * 0.12
                o = crystal(kit, f"gem{variant}_{i}_{j}", base, dirn, length * (1.0 - 0.25 * abs(spread)),
                            length * 0.2)
                o.data.materials.append(glow)
                pieces.append((o, bone))
        out.append(("runes", variant, pieces))
    # Big digging claws on the front paws, small ones behind.
    claws = []
    for s in (-1, 1) if not baby else ():  # a baby's paws are soft
        for foot, bone, k in (("toe_f", "hand", 1.0), ("toe_b", "foot", 0.6))[:kit.lod(2, 1)]:
            tip = kit.mirror(F["nodes"][f"{foot}_R"][0], s)
            r = F["nodes"][f"{foot}_R"][1]
            rr = r[0] if r else (0.1 if baby else 0.15)
            for j, dx in enumerate((-0.55, 0.0, 0.55)):
                base = tip + V((s * dx * rr * 0.9, -rr * 0.55, 0.02 * k))
                c = claw(kit, f"claw_{foot}_{s}_{j}", base, (s * dx * 0.3, -1.0, -0.25),
                         (0.27 if not baby else 0.06) * k, (0.052 if not baby else 0.018) * k, big=k > 0.8)
                c.data.materials.append(mats["tooth"])
                claws.append((c, f"{bone}_{'R' if s > 0 else 'L'}"))
    out.append(("frill", 0, claws))
    # The tail's rounded stone club.
    x, y, z = F["nodes"]["tail_tip"][0]
    k = F["tail_k"]
    club = kit.blob("tail_club", (0, y + 0.04 * k, z + 0.03 * k), (0.34 * k, 0.40 * k, 0.29 * k),
                    kit.lod(8, 6), kit.lod(5, 4))
    club.data.materials.append(mats["horn"])
    out.append(("tail_tip", 0, [(club, "tail4")]))
    return out


def wings(kit, d, rare):
    """Short, broad wings: a sturdy arm and three fingers under a rounded membrane."""
    if rare:
        return []
    F, mats = kit.F, d["mats"]
    wr = F["wing"]["radii"]
    objs = []
    for side in ("L", "R"):
        w = kit.wing_points(side)
        arm = kit.wing_arm(side, ["root", "elbow", "wrist", "f1", "f2", "f3"],
                           [wr["root"], wr["elbow"], wr["wrist"]] + [wr["finger"]] * 3,
                           kit.lod(F["wing"]["arm_tris"], 30), mats, material="horn")
        objs.append(arm)

        def edge(a, b, bulge, n):
            pts = []
            for j in range(1, n + 1):
                f = j / (n + 1)
                p = a.lerp(b, f)
                out = p - w["wrist"]
                pts.append(p + out.normalized() * bulge * math.sin(math.pi * f) * (a - b).length)
            return pts

        n = kit.lod(2, 1)
        rim = [w["body"]] + edge(w["body"], w["f3"], 0.10, n) + [w["f3"]] + edge(w["f3"], w["f2"], -0.06, n) + \
            [w["f2"]] + edge(w["f2"], w["f1"], -0.05, n) + [w["f1"]]
        pts = [w["wrist"], w["elbow"], w["root"]] + rim
        mem = kit.flat_fan(f"membrane_{side}", pts, F["wing"]["thickness"])
        mem.data.materials.append(mats["membrane"])
        objs.append(mem)
    return objs


def accent(kit, body):
    """A warm sandy belly, throat and chin, and a pale muzzle."""
    import mathutils
    mask = kit.mask_attr(body)
    F = kit.F
    mk = F["mask"]
    down = mathutils.Vector((0, -0.3, -0.95)).normalized()
    baby = F["name"] == "hatchling"
    snout_y = kit.node("muzzle").y + (0.06 if baby else 0.1)
    for v in body.data.vertices:
        x, y, z = v.co
        a = min(1.0, max(0.0, (v.normal.dot(down) - 0.15) / 0.45))
        if abs(x) > mk["max_x"]:
            a *= max(0.0, 1.0 - (abs(x) - mk["max_x"]) / 0.08)
        if y < snout_y:  # the muzzle
            a = max(a, min(1.0, (snout_y - y) / (0.12 if baby else 0.2)) * 0.8)
        mask.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


def texture(tx, nt, p, form):
    """R: granite speckles; G: sandstone strata (soft level bands on the body, not the legs);
    B: seams (rusty veins on Basalt, glowing amethyst on the Geode). Value: softly painted
    scutes over the skin, and a shadowed back where the plates lie (dark seams between)."""
    r = tx.mul(nt, tx.spots(nt, p["spot_cell"], keep=0.5, size=(0.34, 0.22), where=tx.const(nt, 1.0)), 0.85)
    body_only = tx.mul(nt, tx.smoothstep(nt, tx.axis(nt, 2), p["strata_z"] - 0.1, p["strata_z"] + 0.1),
                       tx.smoothstep(nt, tx.axis(nt, 1), p["strata_y"] - 0.1, p["strata_y"] + 0.1))
    g = tx.mul(nt, tx.stripes(nt, p["stripe"], direction="Z", distortion=2.0, width=(0.62, 0.76),
                              where=tx.mul(nt, tx.upper(nt), body_only)), 0.55)
    b = tx.mul(nt, tx.cracks(nt, p["crack_cell"], width=0.05, warp=0.4), tx.upper(nt))
    under_plates = tx.madd(nt, tx.top(nt), -0.38, 1.0)
    value = tx.mul(nt, tx.mul(nt, tx.cells(nt, p["scute"], edge=0.14), under_plates),
                   tx.mul(nt, tx.grain(nt, 9.0, 0.06), tx.light_from_above(nt, 0.14)))
    return {"r": r, "g": g, "b": b, "value": value}
