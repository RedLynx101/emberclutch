"""The Cindershell (Ember and Stone, uncommon; Dragondex 11): the crossbreed of the Pouncer and
the Curlstone. A friendly volcano: a lava-pangolin panther. Concept:
docs/art/concept/dragons/dragon_cindershell.jpg (reference only, D74).

Design notes
  * Parents. The Pouncer gives it a cat's round head, tall pointed ears, big eyes, a short soft
    muzzle and a panther's legs and bearing; the Curlstone gives it its body plan (it curls into
    a ball and rolls like a boulder) and its armour of overlapping plates. What is new is the
    fire inside the stone: the plates are dark obsidian and magma glows in every seam.
  * The adult. A sturdy black panther, its round cat head carried proudly above an arched pangolin
    back: rounded obsidian plates overlap down the neck, the back and the tail like shingles, with
    a big round plate on each haunch and shoulder, each one lifted at its free rear edge over a
    crescent of glowing magma; the skin between them is a warm ember, and thin glowing seams run
    on down the flanks and legs. The ears are capped in smooth stone with warm orange insides; the
    head stays a cat's, smooth and round. Short, broad, stone-edged wings fold flat along its
    flanks under the plates, and the tail of stacked plates ends in a glowing coal. Curled up (the
    Curlstone's ball) it is a ball of obsidian laced with glowing seams, its ears peeking out.
  * The hatchling. A round pebble-kitten: a big round kitten head with huge eyes and little
    stone-capped ears on a round pebble of a body, stubby legs, tiny wing buds perked up off its
    round back, one glowing crack zigzagging down its back and a small glowing coal on its tail.
  * Pattern: B the magma seams (glowing on every variant); R the skin under the plates and the
    seams drawn wider (a warm ember halo on the commons, the Molten Core's glow); G soft
    sandstone strata (the Sandstone's pattern).
  * Variants: Magma (black plates, orange seams: the natural one), Sandstone (tan plates, gold
    glow: the surprise), Basalt (blue-grey plates, cyan-white glow: the subtle one), and the rare
    Molten Core: bright bronze plates spaced wider over wide glowing seams, wider glowing rims
    and a flame burning on its tail coal.
  * Egg: black with glowing orange cracks (in each colouring's stone and glow).
"""
import math

META = dict(
    name="cindershell", title="Cindershell", dex=11, element=("Ember", "Stone"), parents=("pouncer", "curlstone"),
    rarity="uncommon", plan="curlstone", size=1.1,
    stats=dict(wing=4, wit=5, might=8, breath=9, stamina=7),
    manners=("Brave", "Stubborn", "Proud", "Gentle"),
    traits=("Ironhide", "Sturdy", "Warm-Blooded", "Brave Heart", "Elemental", "Ancient Blood"),
    rare_variant=3, rare_replaces=True,
    blurb="A friendly little volcano: brave, warm as a hearthstone, and happiest curled up in a glowing ball.",
)

# Palette use: base = the skin, accent = belly, throat and muzzle, horn = the stone plates, ear
# caps and wing edges, membrane = the wings and the inner ears, pattern = the warm halo round the
# seams (or the strata), glow = the magma (seams, plate rims, the coal, the heart).
VARIANTS = [
    dict(name="Magma", base=(0.075, 0.066, 0.072), accent=(0.22, 0.16, 0.14), pattern=(0.30, 0.05, 0.015),
         horn=(0.10, 0.095, 0.11), membrane=(0.80, 0.20, 0.03), iris=(1.0, 0.45, 0.06), glow=(1.0, 0.30, 0.035),
         pattern_channel="r", glow_channel="b"),
    dict(name="Sandstone", base=(0.40, 0.25, 0.14), accent=(0.80, 0.60, 0.38), pattern=(0.26, 0.14, 0.07),
         horn=(0.74, 0.54, 0.32), membrane=(0.98, 0.66, 0.20), iris=(0.55, 0.30, 0.06), glow=(1.0, 0.74, 0.20),
         pattern_channel="g", glow_channel="b"),
    dict(name="Basalt", base=(0.12, 0.14, 0.18), accent=(0.40, 0.45, 0.52), pattern=(0.06, 0.20, 0.30),
         horn=(0.25, 0.29, 0.36), membrane=(0.32, 0.64, 0.80), iris=(0.40, 0.88, 1.0), glow=(0.62, 0.96, 1.0),
         pattern_channel="r", glow_channel="b"),
    dict(name="Molten Core", base=(0.08, 0.032, 0.016), accent=(0.36, 0.15, 0.05), pattern=(0.95, 0.40, 0.06),
         horn=(0.36, 0.13, 0.034), membrane=(1.0, 0.26, 0.03), iris=(1.0, 0.64, 0.1), glow=(1.0, 0.24, 0.02),
         pattern_channel=None, glow_channel="r"),
]

EGG = dict(height=0.98, width=0.39, asym=0.1, speckle="swirl", speckle_params=dict(size=(0.02, 0.052), strokes=5),
           colors=[((0.17, 0.15, 0.16), (1.0, 0.50, 0.12)), ((0.82, 0.66, 0.46), (1.0, 0.82, 0.32)),
                   ((0.34, 0.38, 0.45), (0.66, 0.96, 1.0)), ((0.62, 0.38, 0.18), (1.0, 0.62, 0.20))])


def _mirrored(center, sides):
    nodes = dict(center)
    for name, (p, r) in sides.items():
        nodes[f"{name}_R"] = ((p[0], p[1], p[2]), r)
        nodes[f"{name}_L"] = ((-p[0], p[1], p[2]), r)
    return nodes


# ------------------------------------------------------------------------------ grown
# Faces -Y, Z up; about 6 units long. radius = (side, vertical). The Curlstone's long, bendy
# back (so the ball closes the same way) on a panther's longer legs, with a cat's round head on
# a short neck and a longer, slimmer tail of stacked plates.
GROWN_NODES = _mirrored({
    "tail_tip": ((0, 3.42, 0.86), (0.15, 0.15)),
    "tail4": ((0, 3.02, 0.76), (0.21, 0.21)),
    "tail3": ((0, 2.52, 0.90), (0.29, 0.28)),
    "tail2": ((0, 1.95, 1.16), (0.41, 0.40)),
    "hips": ((0, 1.24, 1.48), (0.60, 0.66)),
    "loin": ((0, 0.60, 1.68), (0.72, 0.82)),
    "belly": ((0, -0.06, 1.62), (0.74, 0.82)),
    "chest": ((0, -0.72, 1.44), (0.69, 0.78)),
    "neck2": ((0, -1.22, 1.62), (0.47, 0.49)),
    "neck3": ((0, -1.50, 1.94), (0.42, 0.43)),
    "head": ((0, -1.80, 2.24), (0.62, 0.57)),
    "muzzle": ((0, -2.22, 2.10), (0.29, 0.235)),
    "snout": ((0, -2.43, 2.03), (0.17, 0.135)),
}, {
    "shoulder": ((0.44, -0.66, 1.16), (0.29, 0.33)),
    "elbow": ((0.54, -0.76, 0.66), (0.25, 0.25)),
    "wrist": ((0.55, -0.82, 0.25), (0.21, 0.21)),
    "toe_f": ((0.57, -1.10, 0.11), (0.25, 0.12)),
    "hipj": ((0.46, 1.18, 1.20), (0.38, 0.44)),
    "knee": ((0.57, 0.90, 0.66), (0.30, 0.30)),
    "ankle": ((0.57, 1.22, 0.27), (0.22, 0.22)),
    "toe_b": ((0.59, 0.95, 0.11), (0.25, 0.12)),
})
SPINE = ["tail_tip", "tail4", "tail3", "tail2", "hips", "loin", "belly", "chest", "neck2", "neck3", "head", "muzzle",
         "snout"]
GROWN_EDGES = list(zip(SPINE, SPINE[1:]))
for _side in ("L", "R"):
    GROWN_EDGES += [("chest", f"shoulder_{_side}"), (f"shoulder_{_side}", f"elbow_{_side}"),
                    (f"elbow_{_side}", f"wrist_{_side}"), (f"wrist_{_side}", f"toe_f_{_side}"),
                    ("hips", f"hipj_{_side}"), (f"hipj_{_side}", f"knee_{_side}"),
                    (f"knee_{_side}", f"ankle_{_side}"), (f"ankle_{_side}", f"toe_b_{_side}")]

BUILDS = {
    "neutral": {},
    "sturdy": {"chest": (1.07, 0.98), "belly": (1.06, 0.98), "loin": (1.05, 0.98), "hips": (1.05, 1.0),
               "arm_up": (1.07, 0.97), "leg_up": (1.07, 0.97), "tail2": (1.05, 0.97)},
    "sleek": {"chest": (0.95, 1.02), "belly": (0.93, 1.03), "loin": (0.93, 1.03), "tail3": (0.94, 1.04),
              "tail4": (0.94, 1.05)},
    "long": {"belly": (0.97, 1.07), "loin": (0.97, 1.07), "tail2": (0.97, 1.06), "tail3": (0.96, 1.08),
             "tail4": (0.96, 1.08)},
}


def _grown_sculpt(kit, obj):
    """A cat's shapes the node graph can't give: a deep, proud chest over a tucked belly, round
    cheeks and a short soft muzzle, a little flatter on top."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for v in bm.verts:
        x, y, z = v.co
        if -1.05 < y < -0.2 and z < 1.3 and abs(x) < 0.42:  # chest keel
            k = (1 - abs(x) / 0.42) * max(0.0, (1.3 - z) / 0.5)
            v.co.z -= 0.07 * k
        if 0.1 < y < 0.95 and z < 1.1 and abs(x) < 0.4:  # tucked belly
            k = (1 - abs(x) / 0.4) * max(0.0, (1.1 - z) / 0.35)
            v.co.z += 0.06 * k
        if -2.08 < y < -1.68 and 2.02 < z < 2.40:  # round cheeks
            v.co.x *= 1.1
        if y < -2.12 and z > 2.11:  # the muzzle a little flatter on top
            t = min(1.0, (-2.12 - y) / 0.3)
            v.co.z -= 0.03 * t
    bm.to_mesh(obj.data)
    bm.free()


# Plate bands: (bone, t along it, angles round the spine (0 on top, + to its right), (width,
# length, height), lift of the rear edge). Staggered like shingles down the neck, the back and
# the tail (the head stays a cat's: smooth and round).
GROWN_PLATES = [
    ("neck3", 0.45, (-34, 34), (0.50, 0.54, 0.13), 0.06),
    ("neck2", 0.5, (0, 64, -64), (0.54, 0.58, 0.15), 0.08),
    ("chest", 0.22, (-28, 28, 80, -80), (0.74, 0.88, 0.22), 0.10),
    ("belly", 0.50, (0, 56, -56, 104, -104), (0.84, 1.04, 0.25), 0.11),
    ("loin", 0.50, (-28, 28, 82, -82), (0.86, 1.04, 0.25), 0.11),
    ("hips", 0.50, (0, 56, -56, 102, -102), (0.78, 0.98, 0.24), 0.10),
    ("tail1", 0.50, (-34, 34, 92, -92), (0.60, 0.80, 0.18), 0.08),
    ("tail2", 0.50, (0, 64, -64), (0.50, 0.70, 0.15), 0.07),
    ("tail3", 0.50, (-38, 38), (0.40, 0.58, 0.12), 0.06),
    # a big round plate on each haunch and shoulder (outward: + on the right, - on the left)
    ("leg_up_R", 0.32, (90,), (0.62, 0.70, 0.17), 0.05), ("leg_up_L", 0.32, (-90,), (0.62, 0.70, 0.17), 0.05),
    ("arm_up_R", 0.30, (90,), (0.52, 0.60, 0.15), 0.04), ("arm_up_L", 0.30, (-90,), (0.52, 0.60, 0.15), 0.04),
]

GROWN = dict(
    name="grown", nodes=GROWN_NODES, edges=GROWN_EDGES, body="skin", relax=0.45,
    body_tris=1100, body_tris_lod1=400, export_scale=0.86,
    young={
        "bones": {
            "head": (1.1, 1.0, 1.1), "snout": (0.9, 0.76, 0.94),
            "neck2": (0.8, 0.62), "neck3": (0.82, 0.62),
            "chest": (0.72, 0.62), "belly": (0.70, 0.60), "loin": (0.70, 0.60), "hips": (0.72, 0.62),
            "tail1": (0.72, 0.58), "tail2": (0.72, 0.56), "tail3": (0.74, 0.56), "tail4": (0.8, 0.6),
            "arm_up": (0.76, 0.64), "arm_lo": (0.78, 0.64), "hand": (0.86, 0.8),
            "leg_up": (0.76, 0.64), "leg_lo": (0.78, 0.64), "foot": (0.86, 0.8),
        },
        "parts": {"eyes": 1.3, "horns": 0.6, "frill": 0.82, "wings": 0.6, "spikes": 0.92,
                  "tail_tip": 0.8, "heart": 0.85, "runes": 0.6},
    },
    young_pose={"neck2": -8, "head": -6},
    base_pose={"neck2": (-8, 0, 0), "neck3": (-4, 0, 0), "head": (6, 0, 0),
               "tail2": (4, 0, 0), "tail3": (10, 0, 4), "tail4": (16, 0, 8)},
    builds=BUILDS,
    eyes=dict(at=(0.29, -2.24, 2.27), out=(0.46, -0.86, 0.12), iris=(0.18, 0.2, 0.085),
              pupil=(0.118, 0.142, 0.03), slit=(0.28, 1.08),
              glints=((-0.03, 0.044, 0.026), (0.022, -0.044, 0.012)), seg=(12, 2, 8, 2)),
    head=dict(origin=(0, -1.80, 2.24), k=1.55, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=False),
    tail_k=1.0,
    heart=dict(at=(0, -1.40, 1.40), size=0.13),
    wing=dict(root=(0.44, -0.52, 1.98), scale=1.0, dihedral=30, droop=8,
              layout={"root": (0.0, 0.0), "elbow": (0.62, 0.22), "wrist": (1.12, -0.02), "f1": (1.92, 0.30),
                      "f2": (1.74, 0.94), "f3": (1.22, 1.38), "body": (0.0, 1.15)},
              radii={"root": 0.11, "elbow": 0.088, "wrist": 0.072, "finger": 0.04, "tip": 0.016},
              arm_tris=110, thickness=0.014, style="sail"),
    mask=dict(max_x=0.5, max_z=2.34, min_z=-1.0, tail_cut=None),
    inset={"eyes": 0.02, "horns": 0.03, "spikes": 0.012, "frill": 0.05, "heart": -0.05, "runes": 0.03,
           "tail_tip": 0.08},
    face=dict(nostril=(0.05, -2.59, 2.09), nostril_r=(0.028, 0.018, 0.009), mouth_r=0.012,
              mouth=lambda side, a: (side * 0.16 * a ** 0.7, -2.54 + 0.40 * a ** 1.5, 1.96 + 0.08 * a * a)),
    jaw_hinge=(0, -2.03, 2.01),
    mouth_detail=dict(depth=0.2, fade=0.16, width=0.26, tooth=(0.01, 0.017), fang=(0.013, 0.028),
                      tongue=(0.065, 0.13, 0.016), fangs=[]),
    skin=dict(crack_cell=0.62, crack_w=0.04, halo_w=0.075, vein_z=(0.55, 1.05), under_nz=(0.05, 0.4),
              under_z=(0.8, 1.15), body_y=-1.7, strata=0.24, ao=0.6),
    sculpt=_grown_sculpt,
    plates=GROWN_PLATES,
)

# ------------------------------------------------------------------------------ hatchling
# A round pebble with a kitten's head: the head a third of it, a round high back, stubby legs;
# the short tail is modelled straight and curled round its side in the idle pose.
HATCH_NODES = _mirrored({
    "tail_tip": ((0, 1.12, 0.18), None),
    "tail4": ((0, 0.97, 0.19), None),
    "tail3": ((0, 0.81, 0.23), None),
    "tail2": ((0, 0.62, 0.31), None),
    "hips": ((0, 0.40, 0.46), None),
    "loin": ((0, 0.22, 0.62), None),
    "belly": ((0, 0.02, 0.64), None),
    "chest": ((0, -0.16, 0.58), None),
    "neck2": ((0, -0.28, 0.68), None),
    "neck3": ((0, -0.38, 0.82), None),
    "head": ((0, -0.48, 0.98), None),
    "muzzle": ((0, -0.72, 0.93), None),
    "snout": ((0, -0.87, 0.90), None),
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
    ("ell", (0, -0.52, 1.05), (0.35, 0.32, 0.31)),      # big round kitten head
    ("ell", (0, -0.77, 0.92), (0.165, 0.145, 0.115)),   # short button muzzle
    ("ell", (0, -0.73, 0.86), (0.125, 0.115, 0.07)),    # chin
    ("chain", [(0, -0.24, 0.62), (0, -0.36, 0.77), (0, -0.46, 0.90)], [0.2, 0.2, 0.19]),
    ("ell", (0, 0.10, 0.60), (0.37, 0.37, 0.41)),       # the pebble: a round, high back
    ("ell", (0, -0.12, 0.52), (0.28, 0.24, 0.30)),      # chest
    ("ell", (0, 0.33, 0.47), (0.30, 0.23, 0.30)),       # haunches
    ("ell", (0, 0.06, 0.37), (0.26, 0.28, 0.20)),       # tummy
    ("chain", [(0, 0.50, 0.38), (0, 0.66, 0.28), (0, 0.82, 0.22), (0, 0.97, 0.19), (0, 1.09, 0.18)],
     [0.14, 0.11, 0.09, 0.075, 0.065]),               # a short tail
]
for _s in (-1, 1):
    HATCH_META += [
        ("ball", (_s * 0.17, -0.71, 0.90), 0.125),      # cheeks
        ("chain", [(_s * 0.19, -0.12, 0.34), (_s * 0.22, -0.16, 0.21), (_s * 0.23, -0.19, 0.08)],
         [0.1, 0.09, 0.087]),
        ("ell", (_s * 0.235, -0.26, 0.055), (0.095, 0.12, 0.06)),   # front paws
        ("ell", (_s * 0.19, 0.34, 0.31), (0.13, 0.16, 0.16)),       # thighs
        ("chain", [(_s * 0.22, 0.32, 0.21), (_s * 0.23, 0.38, 0.09)], [0.09, 0.08]),
        ("ell", (_s * 0.235, 0.28, 0.055), (0.095, 0.13, 0.06)),    # hind paws
    ]

HATCH = dict(
    name="hatchling", nodes=HATCH_NODES, meta=HATCH_META, body="meta",
    body_tris=1450, body_tris_lod1=540, export_scale=1.0,
    young={
        "bones": {name: ((0.76, 0.76) if name in ("head", "snout") else (0.64, 0.64))
                  for name in ("hips", "loin", "belly", "chest", "neck2", "neck3", "head", "snout", "tail1",
                               "tail2", "tail3", "tail4", "arm_up", "arm_lo", "hand", "leg_up", "leg_lo", "foot")},
        "parts": {"eyes": 1.12, "horns": 0.7, "frill": 0.9, "wings": 0.56, "spikes": 0.9,
                  "tail_tip": 0.9, "heart": 1.0, "runes": 0.9},
    },
    # The tail curled round its side; the little wings turned up and out off its round body (run 17:
    # folded as the grown one's, they sink into its flanks; this plan's fold wants its own turn).
    base_pose={"neck2": (6, 0, 0), "head": (-8, 0, 0), "tail1": (8, 0, 20), "tail2": (0, 0, 40),
               "tail3": (0, 0, 45), "tail4": (0, 0, 45), "wing_arm_R": (90, 0, -20), "wing_arm_L": (90, 0, 20)},
    builds={"neutral": {}, "sturdy": {}, "sleek": {}, "long": {}},
    eyes=dict(at=(0.17, -0.78, 1.05), out=(0.42, -0.90, 0.07), iris=(0.12, 0.134, 0.066),
              pupil=(0.082, 0.096, 0.03), slit=(0.32, 1.1),
              glints=((-0.028, 0.045, 0.028), (0.024, -0.042, 0.013)), seg=(14, 3, 12, 2)),
    head=dict(origin=(0, -0.50, 1.17), k=0.9, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=True),
    tail_k=0.36,
    heart=dict(at=(0, -0.36, 0.50), size=0.074),
    # A stubby bud: the grown wing's arm with short fingers (curled up, it wraps small).
    wing=dict(root=(0.18, -0.04, 0.78), scale=0.22, dihedral=25, droop=5,
              layout={"root": (0.0, 0.0), "elbow": (0.62, 0.22), "wrist": (1.12, -0.02), "f1": (1.60, 0.24),
                      "f2": (1.46, 0.72), "f3": (1.08, 1.02), "body": (0.0, 0.9)},
              radii={"root": 0.045, "elbow": 0.036, "wrist": 0.03, "finger": 0.014, "tip": 0.006},
              arm_tris=90, thickness=0.008, style="sail"),
    mask=dict(max_x=0.24, max_z=1.0, min_z=0.1, tail_cut=None),
    inset={"eyes": 0.032, "horns": 0.02, "spikes": 0.006, "frill": 0.03, "heart": -0.03, "runes": 0.015,
           "tail_tip": 0.03},
    face=dict(nostril=(0.042, -0.90, 0.955), nostril_r=(0.024, 0.016, 0.009), mouth_r=0.011,
              mouth=lambda side, a: (side * 0.11 * a ** 0.8, -0.925 + 0.17 * a ** 1.6, 0.875 + 0.03 * a * a)),
    jaw_hinge=(0, -0.70, 0.88),
    mouth_detail=dict(depth=0.15, fade=0.11, width=0.28, tooth=(0.007, 0.012), fang=(0.010, 0.022),
                      tongue=(0.052, 0.08, 0.012), fangs=[]),
    skin=dict(zig=0.07, zig_amp=0.028, crack_w=0.026, crack_y=(-0.30, 0.62), strata=0.1, ao=0.25),
    plates=[],
)

FORMS = {"grown": GROWN, "hatchling": HATCH}


# ------------------------------------------------------------------------------ helpers
# The plate machinery is the Curlstone's (kinds/curlstone.py), kept here so this kind stands on
# its own: plates cast out from a bone's axis, draped on the skin, their free edge lifted.
def _bone_axis(kit, bone):
    """(head, tail) of a plan bone in the current form's rest pose."""
    extra = kit.extra_points()
    for name, h, t, _ in kit.PLAN.BONES:
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


def _plate(kit, bvh, name, centre, normal, back, size, lift, rim_w=0.12, domed=True):
    """A rounded obsidian plate draped over the skin: broad and round in front, a softly pointed
    free edge behind, domed on top. Its front edge lies on the skin, its rear edge is lifted by
    `lift` so it rides over the plate behind, and the free edge is a crescent of glowing magma
    (material 1, `rim_w` of the way in). A plain plate (not domed: the lowest rows, LOD1) has no
    crescent, so its underside glows instead. Top and underside don't share rim vertices (a crisp
    edge, no rim-lit outline). Its origin is the skin point under its middle (where the kit seats
    it). The Curlstone's plate, with glowing rims."""
    import bmesh
    V = kit.V
    w, ln, h = size
    domed = domed and not kit.LOD
    n = kit.lod(7, 5)  # LOD1: a plain pentagon, its point at the free edge (four corners read as shards)
    z = V(normal).normalized()
    y = (V(back) - z * V(back).dot(z)).normalized()
    x = y.cross(z).normalized()
    c = V(centre)

    def drape(px, py, up):
        q = c + x * px + y * py
        hit, _, _, _ = bvh.ray_cast(q + z * 0.6, -z, 1.4)
        base = hit if hit is not None and (hit - q).length < 0.5 else q
        return base + z * up

    def lift_at(py):
        u = min(1.0, max(0.0, (py / (ln * 0.5) + 0.15) / 1.15))
        return lift * u ** 1.4

    def outline(a, k):
        sx = math.sin(a) * w * 0.5 * (1.0 - 0.16 * max(0.0, -math.cos(a))) * k
        sy = (-math.cos(a) * ln * 0.5 * (1.0 + 0.08 * max(0.0, -math.cos(a))) + ln * 0.04) * k
        return sx, sy

    bm = bmesh.new()
    angles = [2 * math.pi * (i + (0.0 if n == 4 else 0.5)) / n for i in range(n)]
    rim_top, rim_bot, mid_top, mid_rim = [], [], [], []
    for a in angles:
        sx, sy = outline(a, 1.0)
        p = drape(sx, sy, lift_at(sy) - 0.006) - c
        rim_top.append(bm.verts.new(p))
        rim_bot.append(bm.verts.new(p))
        if domed:
            mx, my = outline(a, 1.0 - rim_w)
            q = drape(mx, my, h * 0.72 + lift_at(my)) - c
            mid_top.append(bm.verts.new(q))
            mid_rim.append(bm.verts.new(q))
    apex = bm.verts.new(drape(0.0, ln * 0.06, h + lift_at(ln * 0.06)) - c)
    bottom = bm.verts.new(drape(0.0, 0.0, -h * 0.4) - c)
    inner = mid_top if domed else rim_top
    for k in range(n):
        j = (k + 1) % n
        bm.faces.new((apex, inner[k], inner[j])).material_index = 0
        if domed:
            rear = -math.cos(angles[k] + math.pi / n) > -0.2
            if rear:
                bm.faces.new((rim_top[k], rim_top[j], mid_rim[j], mid_rim[k])).material_index = 1
            else:
                bm.faces.new((rim_top[k], rim_top[j], mid_top[j], mid_top[k])).material_index = 0
        bm.faces.new((bottom, rim_bot[j], rim_bot[k])).material_index = 0 if domed else 1
    bm.normal_update()
    for f in bm.faces:
        want = -1.0 if bottom in f.verts else 1.0
        if f.normal.dot(z) * want < 0:
            f.normal_flip()
    bmesh.ops.triangulate(bm, faces=list(bm.faces))
    obj = kit.mesh_object(name, bm, c)
    kit.smooth(obj)
    return obj


SPINE_BONES = ("hips", "loin", "belly", "chest", "neck", "tail")


def plates(kit, d, bands, scale=1.0, rim_w=0.12, tag=""):
    """Plate bands: [(object, bone)] (obsidian tops, glowing magma rims). Plates round the spine
    below the flanks (past 95 degrees) are plain; those on the legs are domed."""
    bvh = _body_bvh(kit, d["body"])
    mats = d["mats"]
    out = []
    for bone, t, angles, size, lift in bands:
        for ang in angles:
            hit, normal, axis = surface_point(kit, bvh, bone, t, ang)
            if hit is None:
                continue
            grow = kit.lod(1.0, 1.15)  # LOD1's plain plates are grown a little to cover as much
            sz = (size[0] * scale * grow, size[1] * scale * grow, size[2] * scale)
            domed = sz[0] >= kit.F.get("domed_from", 0.3) and (abs(ang) < 95 or not bone.startswith(SPINE_BONES))
            o = _plate(kit, bvh, f"plate{tag}_{bone}_{ang}", hit, normal, axis, sz, lift * scale, rim_w, domed)
            o.data.materials.append(mats["horn"])
            o.data.materials.append(mats["glow_flat"])
            out.append((o, bone))
    return out


def lit_glow(kit, d, name="glow_flat", colour="glow", k=0.5):
    """A preview material for glowing parts as the game draws them (lit colour plus the glow
    added on top), so facets read in the review. Named "<name>.001": the exporter reads the name
    before the dot for the palette. (The Curlstone's.)"""
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


def _split_by_material(obj):
    """Split the edges between faces of different materials (a crisp line where the stone cap
    meets the skin: the exporter paints each vertex by one material)."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    edges = [e for e in bm.edges if len(e.link_faces) == 2 and
             e.link_faces[0].material_index != e.link_faces[1].material_index]
    bmesh.ops.split_edges(bm, edges=edges)
    bm.to_mesh(obj.data)
    bm.free()


# ------------------------------------------------------------------------------ parts
def _ear_frame(kit, out, up):
    """The ear's axes: s across it, front (facing forward) and u along it."""
    V = kit.V
    o, u = V(out).normalized(), V(up).normalized()
    s = o.cross(u).normalized()
    front = u.cross(s).normalized()
    if front.y > 0:
        front, s = -front, -s
    return s, front, u


def _ear(kit, name, base, out, up, length, width, depth, mats, cap=0.46):
    """A tall pointed cat ear (the Pouncer's flattened, gently curved cone), its upper part
    capped in smooth stone."""
    import mathutils
    s, front, u = _ear_frame(kit, out, up)
    o = kit.horn_mesh(name, length, width * 0.5, 0.35, kit.lod(5, 3), kit.lod(7, 5), taper=0.75, tip=0.04)
    o.data.materials.append(mats["body_plain"])
    o.data.materials.append(mats["horn"])
    for p in o.data.polygons:
        p.material_index = 1 if p.center.z > length * cap else 0
    _split_by_material(o)
    kit.smooth(o)
    o.rotation_euler = mathutils.Matrix((-s, -front, u)).transposed().to_euler()
    o.location = kit.V(base)
    o.scale = (1.0, depth / (width * 0.4), 1.0)
    return o, s, front, u


def _inner_ear(kit, name, base, s, front, u, ear_len, ear_w, depth, material, reach=0.72):
    """The inner ear: a rounded triangle laid on the ear's front face (following its taper and
    its backward curve, just proud of it), warm orange; it stops short of the stone cap's tip."""
    b = kit.V(base)
    ky = depth / (ear_w * 0.4)  # the ear's squash front to back (see _ear), which bends its curve too

    def on_front(t, across):
        a = 0.35 * t
        centre = b + u * (math.cos(a) * ear_len * t) - front * (math.sin(a) * ear_len * t * 0.9 * ky)
        half = ear_w * 0.5 * (1 - t) ** 0.75 + ear_w * 0.5 * 0.04
        return centre + s * (across * half * 0.62) + front * (depth * ((1 - t) ** 0.75 + 0.04) + 0.012)

    pts = [on_front(0.08, 0.0), on_front(0.06, 1.0), on_front(reach * 0.55, 0.9), on_front(reach, 0.0),
           on_front(reach * 0.55, -0.9), on_front(0.06, -1.0)]
    o = kit.flat_fan(name, pts, 0.008)
    o.data.materials.append(material)
    return o


def _coal(kit, name, centre, radii, material, seed=3):
    """A glowing coal: a lumpy, faceted nugget of 20 facets (flat: split vertices, so the game's
    vertex normals keep them)."""
    import bmesh
    import random
    rng = random.Random(seed)
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=1, radius=1.0)
    for v in bm.verts:  # lumpy: each corner pushed in or out, a little more along its own axis
        j = 1.0 + rng.uniform(-0.26, 0.18)
        k = [1.0 + rng.uniform(-0.12, 0.12) for _ in range(3)]
        v.co = kit.V((v.co.x * radii[0] * j * k[0], v.co.y * radii[1] * j * k[1], v.co.z * radii[2] * j * k[2]))
    bmesh.ops.split_edges(bm, edges=list(bm.edges))
    obj = kit.mesh_object(name, bm, kit.V(centre))
    obj.data.materials.append(material)
    return obj


def _flame(kit, name, base, height, width, material, turn=0.0):
    """A flame on the tail coal: two crossed flame-shaped sheets (a broad tongue with two small
    side licks), so it reads as a flame from every side."""
    V = kit.V
    b = V(base)
    shape = [(0.0, 0.0), (0.42, 0.10), (0.56, 0.32), (0.52, 0.50), (0.72, 0.62), (0.36, 0.70), (0.20, 1.0),
             (-0.02, 0.78), (-0.30, 0.86), (-0.34, 0.62), (-0.60, 0.50), (-0.50, 0.30), (-0.40, 0.10)]
    if kit.LOD:
        shape = [(0.0, 0.0), (0.5, 0.2), (0.45, 0.55), (0.2, 1.0), (-0.3, 0.7), (-0.5, 0.25)]
    sheets = []
    for k, a in enumerate((turn, turn + math.pi / 2)):
        side = V((math.cos(a), math.sin(a), 0.0))
        pts = [b + V((0, 0, -0.05 * height))] + [b + side * (sx * width) + V((0, 0, sy * height))
                                                 for sx, sy in shape[1:]]
        sheets.append(kit.flat_fan(f"{name}_{k}", pts, 0.012))
    obj = kit.join(sheets, name)
    obj.data.materials.clear()
    obj.data.materials.append(material)
    return obj


def parts(kit, d):
    F, mats, V = kit.F, d["mats"], kit.V
    baby = d["form"] == "hatchling"
    hk = F["head"]["k"]
    out = []
    glow = lit_glow(kit, d, k=0.35)
    # Obsidian plates down the neck, back and tail, each over a crescent of glowing magma; the
    # rare Molten Core's are smaller and spaced wider, over wider glowing rims.
    if F["plates"]:
        out.append(("spikes", 0, plates(kit, d, F["plates"])))
        out.append(("spikes", 1, plates(kit, d, F["plates"], scale=0.86, rim_w=0.24, tag="r")))
    # Ears: tall and pointed (the Pouncer's), capped in smooth stone, warm orange inside.
    ears = []
    for sd in (-1, 1):
        base = kit.head_point((0.2, 0.06, 0.24) if not baby else (0.2, 0.06, 0.22), sd)
        length, width = (0.46 if not baby else 0.34) * hk, (0.34 if not baby else 0.3) * hk
        e, s_ax, front, u = _ear(kit, f"ear_{sd}", base, (sd * 0.35, 0.25, 0.9), (sd * 0.3, 0.3, 0.9),
                                 length, width, 0.08 * hk, mats)
        inner = _inner_ear(kit, f"earin_{sd}", base, s_ax, front, u, length, width, 0.08 * hk, mats["membrane"])
        ears.append((kit.join([e, inner], f"ear_{sd}"), "head"))  # one piece: seated on the head together
    out.append(("frill", 0, ears))
    # The tail's end: a glowing coal; the Molten Core's burns with a flame.
    x, y, z = F["nodes"]["tail_tip"][0]
    k = F["tail_k"]
    tip = (0.0, y + 0.02 * k, z + 0.03 * k)
    r = (0.26 * k, 0.30 * k, 0.25 * k) if not baby else (0.2 * k, 0.22 * k, 0.19 * k)
    out.append(("tail_tip", 0, [(_coal(kit, "coal", tip, r, glow), "tail4")]))
    coal = _coal(kit, "coal_r", tip, tuple(v * 1.12 for v in r), glow, seed=5)
    flame = _flame(kit, "flame", V(tip) + V((0, 0.02 * k, r[2] * 0.5)), (0.9 if not baby else 0.8) * k,
                   (0.3 if not baby else 0.28) * k, glow)
    out.append(("tail_tip", 1, [(coal, "tail4"), (flame, "tail4")]))
    return out


# ------------------------------------------------------------------------------ wings
def wings(kit, d, rare):
    """Short, broad, stone-edged wings: a sturdy obsidian arm and three fingers under a rounded
    warm membrane (the Curlstone's layout, so they fold flat along the flanks with its clips)."""
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


# ------------------------------------------------------------------------------ paint
def accent(kit, body):
    """A warm ashen belly, throat and chin, and a soft muzzle."""
    import mathutils
    mask = kit.mask_attr(body)
    F = kit.F
    mk = F["mask"]
    down = mathutils.Vector((0, -0.3, -0.95)).normalized()
    baby = F["name"] == "hatchling"
    snout_y = kit.node("muzzle").y + (0.05 if baby else 0.08)
    for v in body.data.vertices:
        x, y, z = v.co
        a = min(1.0, max(0.0, (v.normal.dot(down) - 0.2) / 0.45))
        if abs(x) > mk["max_x"]:
            a *= max(0.0, 1.0 - (abs(x) - mk["max_x"]) / 0.08)
        if z > mk["max_z"] or z < mk["min_z"]:
            a = 0.0
        if y < snout_y:  # the muzzle
            a = max(a, min(1.0, (snout_y - y) / (0.1 if baby else 0.18)) * 0.85)
        mask.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


def _zigzag(tx, nt, p, width):
    """A crack zigzagging down the middle of the back (the hatchling's)."""
    x, y = tx.axis(nt, 0), tx.axis(nt, 1)
    per, amp = p["zig"], p["zig_amp"]
    zig = tx.madd(nt, tx.math_op(nt, "PINGPONG", y, per), 2 * amp / per, -amp)
    dist = tx.math_op(nt, "ABSOLUTE", tx.math_op(nt, "SUBTRACT", x, zig))
    y0, y1 = p["crack_y"]
    taper = tx.mul(nt, tx.smoothstep(nt, y, y0, y0 + 0.12), tx.smoothstep(nt, y, y1, y1 - 0.12))
    w = tx.mul(nt, taper, width)
    line = tx.smoothstep(nt, tx.math_op(nt, "DIVIDE", dist, tx.maxi(nt, w, 1e-4)), 1.0, 0.45)
    return tx.mul(nt, tx.mul(nt, line, tx.smoothstep(nt, y, y0, y0 + 0.04)), tx.top(nt))


def texture(tx, nt, p, form):
    """B: the magma seams (glowing on every variant): on the grown, a net of thin seams over the
    flanks and upper legs (not the head, the belly or the paws); on the hatchling, one crack down
    its back. R: the skin of the back where the plates lie and the seams drawn wider, with a few
    more cracks (a warm ember halo on the commons, the Molten Core's glow); on the hatchling, its
    crack drawn wider. G: soft sandstone strata."""
    strata = tx.mul(nt, tx.stripes(nt, p["strata"], direction="Z", distortion=2.0, width=(0.6, 0.74),
                                   where=tx.upper(nt)), 0.6)
    if form == "hatchling":
        return {"r": _zigzag(tx, nt, p, p["crack_w"] * 2.4), "g": strata, "b": _zigzag(tx, nt, p, p["crack_w"])}
    nz, y, z = tx.normal_z(nt), tx.axis(nt, 1), tx.axis(nt, 2)
    body = tx.smoothstep(nt, y, p["body_y"] - 0.1, p["body_y"] + 0.1)          # not on the head
    under = tx.mul(nt, tx.mul(nt, tx.smoothstep(nt, nz, *p["under_nz"]), body), tx.smoothstep(nt, z, *p["under_z"]))
    not_belly = tx.mul(nt, tx.smoothstep(nt, nz, -0.75, -0.45), tx.smoothstep(nt, z, *p["vein_z"]))
    b = tx.mul(nt, tx.mul(nt, tx.cracks(nt, p["crack_cell"], width=p["crack_w"], warp=0.35), not_belly), body)
    wide = tx.mul(nt, tx.mul(nt, tx.cracks(nt, p["crack_cell"], width=p["halo_w"], warp=0.35), not_belly), body)
    extra = tx.mul(nt, tx.mul(nt, tx.cracks(nt, p["crack_cell"] * 0.55, width=p["crack_w"], warp=0.5), not_belly),
                   body)
    r = tx.maxi(nt, under, tx.mul(nt, tx.maxi(nt, wide, extra), tx.top(nt)))
    value = tx.mul(nt, tx.grain(nt, 9.0, 0.06), tx.light_from_above(nt, 0.14))
    return {"r": r, "g": strata, "b": b, "value": value}
