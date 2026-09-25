"""Emberclutch dragon model: two body forms, rig, parts and growth.

Run headless:
  blender -b -P tools/blender/dragon_model.py -- --breed ember --stages hatchling,adult --out tools/blender/out/r1b
  blender -b -P tools/blender/dragon_model.py -- --breed tide --stages hatchling --views three_quarter,front,side
  blender -b -P tools/blender/dragon_model.py -- --breed gale --lineup --out tools/blender/out/r1b     (growth lineup)
  ... -- --style v1 --sex female --texture --turntable 12     (review R5: a style, a sex, a turntable)

How it works (docs/plan/alpha-1.md WP2; decisions D36-D38):
  * Two FORMS share one skeleton layout (same bone names and hierarchy):
      hatchling  a metaball-sculpted baby: round head, big eyes, short snout, chubby body,
                 stubby legs, tiny wings. Used for the whole hatchling stage.
      grown      the skin-modifier body used from juvenile to adult.
    The stage-up to juvenile swaps forms behind a glow (the first molt).
  * Within a form, growth is pose-space bone scales (girth, length, girth) with scale
    inheritance off, exactly what the runtime does (architecture section 4).
  * Parts (eyes, horns, frills, dorsal ridge, tail tips, heartglow) are separate meshes bound
    to one bone each and chosen by the genome. Wings are a second skinned draw.
  * Preview shading is a toon ramp + warm rim to approximate the in-game look. The belly
    accent is a per-vertex mask that the exporter turns into vertex paint.
"""
import math
import os
import sys

import bmesh
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

# ------------------------------------------------------------------------------ arguments
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
argv = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []


def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


BREED = arg("--breed", "ember")
STAGES = arg("--stages", "hatchling,adolescent,adult").split(",")
OUT = arg("--out", "//dragon")
RES = int(arg("--res", "560"))

# ------------------------------------------------------------------------------ breeds
# Starter defaults (src/core/genetics.cpp): build, horns, frill, wings, tail, colours.
# The frill gene also picks the dorsal ridge: none -> spikes, fin -> fin sail, feather -> plumes.
BREEDS = {
    "ember": dict(build="sturdy", horns="swept", frill="none", wings="classic", tail="spade",
                  base=(0.86, 0.30, 0.10), accent=(0.98, 0.84, 0.55), horn=(0.96, 0.74, 0.30),
                  glow=(1.0, 0.55, 0.16), eye=(0.95, 0.62, 0.15),
                  pattern="stripes", pattern_color=(0.58, 0.16, 0.06)),
    "tide": dict(build="long", horns="nubs", frill="fin", wings="sail", tail="fan",
                 base=(0.10, 0.60, 0.62), accent=(0.70, 0.93, 0.86), horn=(0.62, 0.90, 0.85),
                 glow=(0.35, 0.95, 0.85), eye=(0.20, 0.55, 0.85),
                 pattern="spots", pattern_color=(0.05, 0.36, 0.46)),
    "gale": dict(build="sleek", horns="swept", frill="feather", wings="plumed", tail="tuft",
                 base=(0.55, 0.72, 0.95), accent=(0.95, 0.97, 1.00), horn=(0.85, 0.92, 1.00),
                 glow=(0.62, 0.92, 1.00), eye=(0.25, 0.45, 0.90),
                 pattern="dapple", pattern_color=(0.36, 0.50, 0.82)),
    # The other three elements (WP12: every breed), for previews and review sheets (R6).
    "grove": dict(build="sturdy", horns="antler", frill="leaf", wings="classic", tail="fan",
                  base=(0.42, 0.62, 0.27), accent=(0.78, 0.84, 0.45), horn=(0.62, 0.48, 0.30),
                  glow=(0.60, 0.95, 0.30), eye=(0.55, 0.80, 0.20),
                  pattern="dapple", pattern_color=(0.26, 0.40, 0.16)),
    "frost": dict(build="sleek", horns="crystal", frill="none", wings="classic", tail="spade",
                  base=(0.84, 0.80, 0.94), accent=(0.94, 0.92, 1.00), horn=(0.80, 0.86, 1.00),
                  glow=(0.75, 0.60, 1.00), eye=(0.55, 0.45, 0.95),
                  pattern="runes", pattern_color=(0.62, 0.50, 0.98)),
    "lumen": dict(build="sleek", horns="crown", frill="feather", wings="plumed", tail="tuft",
                  base=(0.97, 0.90, 0.72), accent=(1.00, 0.97, 0.86), horn=(1.00, 0.86, 0.45),
                  glow=(1.00, 0.86, 0.43), eye=(0.95, 0.70, 0.25),
                  pattern="runes", pattern_color=(1.00, 0.78, 0.35)),
}
RIDGE_OF_FRILL = {"none": "spikes", "leaf": "spikes", "fin": "fin", "feather": "feather"}

# ------------------------------------------------------------------------------ skeleton
# The bone layout (names, hierarchy, export order) lives in rig_layout.py, shared with the
# animation tools, which run without Blender.
from rig_layout import BONES, WING_BONES, WING_CHAIN  # noqa: E402
import dragon_texture  # noqa: E402

WING_DRAW_BODY_BONES = ("chest", "belly", "hips")  # the membrane's flank edge follows these
# Classic bat-style wing in its own plane: u = out along the span, v = back along the chord.
# Four fingers fan across the whole membrane down to the flank ("body"), so no panel is bare.
WING_LAYOUT = {"root": (0.0, 0.0), "elbow": (0.95, 0.35), "wrist": (1.75, -0.15), "thumb": (1.82, -0.42),
               "f1": (3.40, 0.15), "f2": (3.30, 1.20), "f3": (2.65, 2.05), "f4": (1.60, 2.45),
               "body": (0.0, 1.40)}
# Where the wing meets the body (Noah, 2026-09-24: on many adults "the very beginning of the
# wing meant to come from their back isn't attached"; tools/blender/wing_gap.py measured the
# root 0.05-0.11 off the skin and the membrane's inner edge up to 0.22). The authored root
# and flank point are only guides: seat_wings() moves both onto the body's surface.
WING_SINK = 0.6    # the wing's root joint sits this many root radii under the skin
WING_STUB = 1.8    # the arm tube carries on this many root radii into the body, on the chest
WEB_SINK = 4.5     # the membrane's inner edge sits this many membrane thicknesses under the skin (the
                   # flank moves with the legs as the dragon walks: 1.5 left it 0.06 proud on sturdy builds)


def mirrored_nodes(center, sides):
    """Node table from centre-line nodes + right-side nodes (x > 0), mirrored to L (x < 0)."""
    nodes = dict(center)
    for name, (p, r) in sides.items():
        nodes[f"{name}_R"] = ((p[0], p[1], p[2]), r)
        nodes[f"{name}_L"] = ((-p[0], p[1], p[2]), r)
    return nodes


# ------------------------------------------------------------------------------ grown form
# Adult proportions. Faces -Y, Z up, units ~ metres. radius = (side, vertical).
GROWN_NODES = mirrored_nodes({
    "tail_tip": ((0, 3.55, 0.26), (0.03, 0.03)),
    "tail4": ((0, 2.95, 0.34), (0.11, 0.10)),
    "tail3": ((0, 2.30, 0.50), (0.19, 0.18)),
    "tail2": ((0, 1.62, 0.74), (0.30, 0.29)),
    "hips": ((0, 0.92, 0.98), (0.44, 0.46)),
    "belly": ((0, 0.20, 1.00), (0.52, 0.58)),
    "chest": ((0, -0.50, 1.10), (0.50, 0.62)),
    "neck1": ((0, -0.98, 1.52), (0.31, 0.33)),
    "neck2": ((0, -1.28, 1.92), (0.23, 0.25)),
    "neck3": ((0, -1.42, 2.32), (0.20, 0.21)),
    "head": ((0, -1.58, 2.62), (0.28, 0.26)),
    "muzzle": ((0, -1.84, 2.55), (0.19, 0.16)),
    "snout": ((0, -2.12, 2.47), (0.12, 0.10)),
}, {
    "shoulder": ((0.38, -0.50, 0.86), (0.24, 0.27)),
    "elbow": ((0.46, -0.62, 0.46), (0.16, 0.16)),
    "wrist": ((0.46, -0.70, 0.15), (0.12, 0.12)),
    "toe_f": ((0.48, -0.96, 0.07), (0.13, 0.07)),
    "hipj": ((0.42, 0.98, 0.78), (0.31, 0.35)),
    "knee": ((0.52, 0.64, 0.44), (0.20, 0.20)),
    "ankle": ((0.52, 1.04, 0.18), (0.13, 0.13)),
    "toe_b": ((0.54, 0.78, 0.07), (0.14, 0.07)),
})
GROWN_EDGES = [("tail_tip", "tail4"), ("tail4", "tail3"), ("tail3", "tail2"), ("tail2", "hips"),
               ("hips", "belly"), ("belly", "chest"), ("chest", "neck1"), ("neck1", "neck2"),
               ("neck2", "neck3"), ("neck3", "head"), ("head", "muzzle"), ("muzzle", "snout")]
for _side in ("L", "R"):
    GROWN_EDGES += [("chest", f"shoulder_{_side}"), (f"shoulder_{_side}", f"elbow_{_side}"),
                    (f"elbow_{_side}", f"wrist_{_side}"), (f"wrist_{_side}", f"toe_f_{_side}"),
                    ("hips", f"hipj_{_side}"), (f"hipj_{_side}", f"knee_{_side}"),
                    (f"knee_{_side}", f"ankle_{_side}"), (f"ankle_{_side}", f"toe_b_{_side}")]

# Build multipliers (girth, length) on top of growth. Strong on purpose (R1: breeds must read
# as different silhouettes, not just colours).
GROWN_BUILDS = {
    "neutral": {},
    "sturdy": {"chest": (1.22, 0.95), "belly": (1.18, 0.95), "hips": (1.12, 1.0),
               "neck1": (1.18, 0.88), "neck2": (1.16, 0.88), "neck3": (1.14, 0.88),
               "head": (1.10, 0.98), "snout": (1.15, 0.85),
               "arm_up": (1.2, 0.92), "arm_lo": (1.2, 0.92), "hand": (1.15, 1.0),
               "leg_up": (1.2, 0.92), "leg_lo": (1.2, 0.92), "foot": (1.15, 1.0),
               "tail1": (1.12, 0.9), "tail2": (1.1, 0.88), "tail3": (1.08, 0.88), "tail4": (1.05, 0.88)},
    "sleek": {"chest": (0.86, 1.05), "belly": (0.80, 1.08), "hips": (0.86, 1.0),
              "neck1": (0.84, 1.12), "neck2": (0.84, 1.14), "neck3": (0.84, 1.14),
              "head": (0.94, 1.06), "snout": (0.86, 1.2),
              "arm_up": (0.86, 1.14), "arm_lo": (0.86, 1.16), "hand": (0.9, 1.05),
              "leg_up": (0.86, 1.14), "leg_lo": (0.86, 1.16), "foot": (0.9, 1.05),
              "tail1": (0.86, 1.1), "tail2": (0.86, 1.1), "tail3": (0.86, 1.1), "tail4": (0.86, 1.1)},
    "long": {"neck1": (0.9, 1.4), "neck2": (0.9, 1.45), "neck3": (0.9, 1.45),
             "head": (0.96, 1.08), "snout": (0.92, 1.12),
             "chest": (0.95, 1.12), "belly": (0.9, 1.3), "hips": (0.95, 1.1),
             "tail1": (0.92, 1.3), "tail2": (0.9, 1.45), "tail3": (0.9, 1.5), "tail4": (0.9, 1.55),
             "arm_up": (0.95, 0.82), "arm_lo": (0.95, 0.8), "leg_up": (0.95, 0.82), "leg_lo": (0.95, 0.8)},
}

GROWN_BASE_POSE = {"neck1": (-4, 0, 0), "neck2": (6, 0, 0), "neck3": (10, 0, 0), "head": (-6, 0, 0),
                   "tail1": (6, 0, 0), "tail2": (-4, 0, 6), "tail3": (-6, 0, 10), "tail4": (-4, 0, 12)}

GROWN = dict(
    name="grown",
    nodes=GROWN_NODES,
    edges=GROWN_EDGES,
    body="skin",
    body_tris=1630,  # + ~160 for the nostrils, mouth line and mouth pocket joined into the body
    body_tris_lod1=600,
    export_scale=1.0,
    # Bone (girth_x, length[, girth_z]) and part scales at t = 0: the juvenile. Lerps to 1 (adult).
    young={
        "bones": {
            "head": (0.9, 0.85, 1.0), "snout": (0.9, 0.65, 0.95),
            "neck1": (0.72, 0.48), "neck2": (0.74, 0.46), "neck3": (0.78, 0.46),
            "chest": (0.64, 0.5), "belly": (0.62, 0.48), "hips": (0.66, 0.5),
            "tail1": (0.64, 0.48), "tail2": (0.66, 0.46), "tail3": (0.7, 0.46), "tail4": (0.78, 0.5),
            "arm_up": (0.66, 0.55), "arm_lo": (0.68, 0.55), "hand": (0.78, 0.68),
            "leg_up": (0.66, 0.55), "leg_lo": (0.68, 0.53), "foot": (0.78, 0.68),
        },
        "parts": {"eyes": 1.3, "horns": 0.45, "frill": 0.6, "wings": 0.38, "spikes": 0.55,
                  "tail_tip": 0.65, "heart": 0.85},
    },
    young_pose={"neck1": -20, "neck2": -6, "neck3": 4, "head": 20},  # juveniles hold heads high
    base_pose=GROWN_BASE_POSE,
    builds=GROWN_BUILDS,
    key_ts=(0.0, 0.35, 0.70, 1.0),
    eyes=dict(at=(0.155, -1.78, 2.62), out=(0.55, -0.83, 0.08), iris=(0.066, 0.074, 0.040),
              pupil=(0.022, 0.058, 0.016), glints=((-0.018, 0.030, 0.014), (0.014, -0.030, 0.007)),
              seg=(12, 2, 8, 2)),
    # Horns and frills are laid out relative to a head frame: origin + offsets * k.
    head=dict(origin=(0, -1.58, 2.62), k=1.0, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=False,
              frill_k=1.0, feather_w=1.0),
    ridge=dict(path=[("neck3", 0.19), ("neck2", 0.23), ("neck1", 0.30), ("chest", 0.57), ("belly", 0.53),
                     ("hips", 0.42), ("tail2", 0.27), ("tail3", 0.17), ("tail4", 0.09)],
               size=(0.08, 0.06, 0.3), spike=(1.9, 0.7, 40), fin=(2.2, 1.9, 0.02), plume=(3.2, 1.1, 0.012)),
    tail_k=1.25,
    heart=dict(at=(0, -1.06, 1.18), size=0.11),
    # The Runes pattern: (bone, where on the right side, glyph, size); mirrored to the left.
    # Where the folded wings don't cover them: the neck, the lower hip, the base of the tail.
    runes=[("neck2", (0.2, -1.2, 1.9), 1, 0.2), ("neck1", (0.28, -0.9, 1.5), 0, 0.26),
           ("hips", (0.44, 0.9, 0.82), 2, 0.28), ("tail2", (0.28, 1.62, 0.8), 3, 0.24)],
    # The rest pose IS the idle: wings half-raised in a V (dihedral), leading edge on top.
    wing=dict(root=(0.36, -0.40, 1.52), scale=1.0, dihedral=50, droop=10,
              radii={"root": 0.10, "elbow": 0.075, "wrist": 0.06, "finger": 0.022, "tip": 0.008},
              arm_tris=180, thickness=0.014),
    mask=dict(max_x=0.34, max_z=2.35, min_z=-1.0, tail_cut=(1.2, 0.3)),
    inset={"eyes": 0.02, "horns": 0.03, "spikes": 0.02, "frill": 0.09, "heart": -0.065,  # heart: proud of the chest, which bulges when sitting (part_clearance.py)
           "runes": -0.012},
    # Face details (R1b): nostrils on the snout tip and a jaw line, projected onto the body.
    face=dict(nostril=(0.05, -2.20, 2.52), nostril_r=(0.024, 0.015, 0.008), mouth_r=0.012,
              mouth=lambda side, a: (side * 0.15 * a ** 0.7, -2.25 + 0.47 * a ** 1.5, 2.41 + 0.06 * a * a)),
    # The opening mouth (Noah 2026-09-24): the jaw's hinge (behind the mouth corners); how
    # deep and wide the lower jaw reaches, how far behind the corners the throat stretches
    # with it; teeth (radius, length), front fangs, tongue half-size.
    jaw_hinge=(0, -1.68, 2.45),
    mouth_detail=dict(depth=0.30, fade=0.22, width=0.40, tooth=(0.010, 0.018), fang=(0.014, 0.036),
                      tongue=(0.07, 0.14, 0.016)),
)

# ------------------------------------------------------------------------------ hatchling form
# Designed at a comfortable scale (head radius ~0.32); the exporter scales it by
# export_scale so a fresh hatchling is ~1/4 of the adult's length.
HATCH_NODES = mirrored_nodes({
    "tail_tip": ((0, 1.28, 0.28), None),
    "tail4": ((0, 1.06, 0.30), None),
    "tail3": ((0, 0.86, 0.34), None),
    "tail2": ((0, 0.62, 0.42), None),
    "hips": ((0, 0.34, 0.50), None),
    "belly": ((0, 0.08, 0.50), None),
    "chest": ((0, -0.18, 0.56), None),
    "neck1": ((0, -0.30, 0.68), None),
    "neck2": ((0, -0.39, 0.80), None),
    "neck3": ((0, -0.45, 0.92), None),
    "head": ((0, -0.50, 1.04), None),
    "muzzle": ((0, -0.80, 1.00), None),
    "snout": ((0, -1.00, 0.96), None),
}, {
    "shoulder": ((0.21, -0.16, 0.42), None),
    "elbow": ((0.22, -0.21, 0.25), None),
    "wrist": ((0.23, -0.25, 0.10), None),
    "toe_f": ((0.23, -0.40, 0.05), None),
    "hipj": ((0.21, 0.36, 0.44), None),
    "knee": ((0.23, 0.30, 0.26), None),
    "ankle": ((0.24, 0.41, 0.12), None),
    "toe_b": ((0.24, 0.24, 0.05), None),
})

# Metaball body: (kind, centre, size). Sizes are visible radii (see build_meta_body).
HATCH_META = [
    ("ell", (0, -0.55, 1.14), (0.325, 0.30, 0.29)),   # round cranium
    ("ell", (0, -0.85, 0.97), (0.19, 0.18, 0.13)),    # short muzzle
    ("ell", (0, -0.79, 0.90), (0.15, 0.14, 0.07)),    # chin
    ("chain", [(0, -0.30, 0.66), (0, -0.40, 0.80), (0, -0.47, 0.92)], [0.18, 0.165, 0.16]),  # neck
    ("ell", (0, -0.16, 0.56), (0.28, 0.25, 0.27)),    # chest
    ("ell", (0, 0.10, 0.50), (0.31, 0.30, 0.29)),     # round belly
    ("ell", (0, 0.34, 0.52), (0.25, 0.22, 0.24)),     # hips
    ("ell", (0, 0.02, 0.40), (0.24, 0.26, 0.18)),     # belly underside
    ("chain", [(0, 0.52, 0.46), (0, 0.72, 0.38), (0, 0.92, 0.32), (0, 1.10, 0.29), (0, 1.26, 0.28)],
     [0.14, 0.11, 0.085, 0.06, 0.04]),                # tail
]
for _s in (-1, 1):
    HATCH_META += [
        ("ball", (_s * 0.16, -0.75, 0.96), 0.14),     # chubby cheeks
        ("chain", [(_s * 0.21, -0.16, 0.42), (_s * 0.22, -0.21, 0.25), (_s * 0.23, -0.25, 0.10)],
         [0.11, 0.095, 0.09]),                        # front leg
        ("ell", (_s * 0.23, -0.32, 0.06), (0.095, 0.125, 0.06)),   # front paw
        ("ell", (_s * 0.21, 0.34, 0.38), (0.13, 0.18, 0.18)),      # thigh
        ("chain", [(_s * 0.23, 0.40, 0.26), (_s * 0.24, 0.41, 0.12)], [0.095, 0.085]),
        ("ell", (_s * 0.24, 0.30, 0.06), (0.095, 0.135, 0.06)),    # hind foot
    ]


def softened(builds, amount):
    """Hatchlings show their build, but gently."""
    return {name: {bone: tuple(1 + (v - 1) * amount for v in gl) for bone, gl in table.items()}
            for name, table in builds.items()}


HATCH_BASE_POSE = {"head": (4, 0, 0), "tail1": (4, 0, 0), "tail2": (-4, 0, 10), "tail3": (-4, 0, 14),
                   "tail4": (-2, 0, 18)}

HATCH = dict(
    name="hatchling",
    nodes=HATCH_NODES,
    edges=None,
    meta=HATCH_META,
    body="meta",
    body_tris=1600,
    body_tris_lod1=560,
    export_scale=1.0,
    young={  # t = 0 is hatch day, t = 1 the end of the hatchling stage
        "bones": {name: ((0.72, 0.72) if name in ("head", "snout") else (0.62, 0.62))
                  for name in ("hips", "belly", "chest", "neck1", "neck2", "neck3", "head", "snout", "tail1",
                               "tail2", "tail3", "tail4", "arm_up", "arm_lo", "hand", "leg_up", "leg_lo", "foot")},
        "parts": {"eyes": 1.15, "horns": 0.6, "frill": 0.9, "wings": 0.56, "spikes": 0.85,
                  "tail_tip": 0.9, "heart": 1.0},
    },
    young_pose={},
    base_pose=HATCH_BASE_POSE,
    builds=softened(GROWN_BUILDS, 0.4),
    key_ts=(0.0, 0.35, 0.70, 1.0),
    eyes=dict(at=(0.165, -0.80, 1.08), out=(0.38, -0.92, 0.05), iris=(0.105, 0.12, 0.06),
              pupil=(0.07, 0.088, 0.03), glints=((-0.024, 0.040, 0.026), (0.022, -0.040, 0.012)),
              seg=(14, 3, 12, 2)),
    head=dict(origin=(0, -0.51, 1.236), k=0.9, horn_len=0.3, horn_r=0.75, horn_curve=0.6, buds=True,
              frill_k=0.5, feather_w=1.9),  # baby frills: small ear fins, a fluffy tuft
    ridge=dict(path=[("neck3", 0.15), ("neck2", 0.16), ("neck1", 0.18), ("chest", 0.26), ("belly", 0.29),
                     ("hips", 0.24), ("tail2", 0.13), ("tail3", 0.10), ("tail4", 0.07)],
               size=(0.035, 0.02, 0.2), spike=(1.3, 0.8, 20), fin=(1.8, 1.5, 0.012), plume=(2.4, 1.0, 0.008)),
    tail_k=0.42,
    heart=dict(at=(0, -0.40, 0.53), size=0.075),  # low enough that the chin clears it sitting (Noah, run 3)
    runes=[("chest", (0.2, -0.22, 0.6), 0, 0.11), ("belly", (0.23, 0.08, 0.52), 2, 0.13),
           ("hips", (0.19, 0.34, 0.5), 3, 0.11)],
    wing=dict(root=(0.13, -0.10, 0.77), scale=0.19, dihedral=35, droop=5,
              radii={"root": 0.045, "elbow": 0.035, "wrist": 0.03, "finger": 0.011, "tip": 0.005},
              arm_tris=120, thickness=0.008),
    mask=dict(max_x=0.22, max_z=1.06, min_z=0.13, tail_cut=None),
    inset={"eyes": 0.032, "horns": 0.02, "spikes": 0.012, "frill": 0.06, "heart": -0.03},  # heart: see the grown form
    face=dict(nostril=(0.05, -1.00, 1.03), nostril_r=(0.026, 0.017, 0.009), mouth_r=0.011,
              mouth=lambda side, a: (side * 0.15 * a ** 0.8, -1.03 + 0.2 * a ** 1.6, 0.915 + 0.035 * a * a)),
    jaw_hinge=(0, -0.76, 0.93),
    mouth_detail=dict(depth=0.16, fade=0.12, width=0.30, tooth=(0.007, 0.012), fang=(0.010, 0.024),
                      tongue=(0.055, 0.085, 0.012)),
)

FORMS = {"grown": GROWN, "hatchling": HATCH}

# ------------------------------------------------------------------------------ styles (R5, D47)
# Review R5 puts the current look next to three variants of it. A style adjusts the forms
# before anything is built: proportions go into the build tables (so the runtime scales the
# bones the same way at every growth stage and the same clips animate them), and the sizes
# of the eyes, horns, ridge and wings into the geometry. The skin is dragon_texture's
# (SKIN_STYLE) and the game's shading per style is src/app/render3d.cpp's. "current" changes
# nothing.
#   v1 surface  today's shapes, a new surface: bold scale plates, a banded belly, a darker
#               spine, rounder pupils with a third glint; softer three-band shading in game
#   v2 shape    new proportions: chubbier babies with bigger heads and eyes and short legs;
#               adults with long necks, deep chests, slim waists, long tails, big wings
#   v3 bold     the ember-veined dragon: dark scales with the fire showing through glowing
#               cracks, glowing eyes and wings, spikier, a leaner neck and a long tail
STYLE = arg("--style", "current")
STYLE_BUILDS = {  # (girth, length) multipliers on every build, per form
    "v2": {"hatchling": {"head": (1.14, 1.08), "snout": (1.0, 0.78), "chest": (1.12, 0.92), "belly": (1.16, 0.92),
                         "hips": (1.08, 1.0), "arm_up": (1.05, 0.85), "arm_lo": (1.05, 0.85), "leg_up": (1.08, 0.82),
                         "leg_lo": (1.08, 0.82), "tail1": (0.95, 1.12), "tail2": (0.95, 1.12), "tail3": (0.95, 1.12),
                         "tail4": (0.95, 1.12)},
           "grown": {"neck1": (0.9, 1.4), "neck2": (0.9, 1.42), "neck3": (0.9, 1.42), "head": (0.94, 1.06),
                     "snout": (0.88, 1.24), "chest": (1.22, 1.04), "belly": (0.8, 1.08), "hips": (0.94, 1.0),
                     "arm_up": (0.94, 1.22), "arm_lo": (0.94, 1.22), "leg_up": (0.94, 1.2), "leg_lo": (0.94, 1.2),
                     "tail1": (0.88, 1.28), "tail2": (0.88, 1.3), "tail3": (0.86, 1.32), "tail4": (0.86, 1.32)}},
    "v3": {"hatchling": {"snout": (0.95, 1.12), "tail1": (0.92, 1.1), "tail2": (0.92, 1.12), "tail3": (0.92, 1.12),
                         "tail4": (0.92, 1.12)},
           "grown": {"neck1": (0.94, 1.1), "neck2": (0.94, 1.1), "neck3": (0.94, 1.1), "snout": (0.92, 1.14),
                     "chest": (1.06, 1.0), "belly": (0.9, 1.04), "tail1": (0.9, 1.15), "tail2": (0.9, 1.2),
                     "tail3": (0.88, 1.22), "tail4": (0.88, 1.25)}},
}
STYLE_SIZES = {  # geometry: iris scale, pupil (x, y) scale, a third glint, horns, ridge, wings
    "v1": {"hatchling": dict(iris=1.1, pupil=(1.28, 1.2), glint3=True),
           "grown": dict(iris=1.1, pupil=(1.9, 1.05), glint3=True)},
    "v2": {"hatchling": dict(iris=1.2, pupil=(1.12, 1.12), wings=0.8, horns=0.85),
           "grown": dict(iris=1.05, wings=1.3, horns=1.4)},
    "v3": {"hatchling": dict(iris=1.05, ridge=1.35, horns=1.1),
           "grown": dict(iris=0.95, pupil=(0.75, 1.0), ridge=1.5, horns=1.25, wings=1.1)},
}


def apply_style():
    for name, f in FORMS.items():
        for build in f["builds"].values():
            for bone, (g, l) in STYLE_BUILDS.get(STYLE, {}).get(name, {}).items():
                bg, bl = build.get(bone, (1.0, 1.0))
                build[bone] = (bg * g, bl * l)
        s = STYLE_SIZES.get(STYLE, {}).get(name, {})
        e = f["eyes"]
        if "iris" in s:
            e["iris"] = tuple(v * s["iris"] for v in e["iris"])
        if "pupil" in s:
            px, py = s["pupil"]
            e["pupil"] = (e["pupil"][0] * px, e["pupil"][1] * py, e["pupil"][2])
        if s.get("glint3"):  # a little third glint, low on the other side
            gx, gy, gr = e["glints"][0]
            e["glints"] = tuple(e["glints"]) + ((-gx * 0.9, -gy * 1.25, gr * 0.45),)
        if "wings" in s:
            f["wing"]["scale"] *= s["wings"]
        if "horns" in s:
            f["head"]["horn_len"] *= s["horns"]
            f["head"]["horn_r"] *= s["horns"] ** 0.5
        if "ridge" in s:
            f["ridge"]["size"] = tuple(v * s["ridge"] for v in f["ridge"]["size"])
    if STYLE == "v3":  # dark scales, the fire showing through (the game's palette does the same)
        for b in BREEDS.values():
            base, glow = b["base"], b["glow"]
            b["base"] = (base[0] * 0.2 + 0.03, base[1] * 0.2 + 0.02, base[2] * 0.2 + 0.035)
            b["accent"] = tuple(0.5 * c + 0.12 * a for c, a in zip(b["base"], b["accent"]))
            b["horn"] = (0.13, 0.09, 0.1)
            b["eye"] = glow
            b["pattern"], b["pattern_color"] = "veins", tuple(min(1.0, c * 1.1) for c in glow)


apply_style()
# Review stages -> (form, growth t within the form).
STAGE = {"newborn": ("hatchling", 0.0), "hatchling": ("hatchling", 0.75), "juvenile": ("grown", 0.0),
         "adolescent": ("grown", 0.45), "adult": ("grown", 1.0)}
F = GROWN  # the form being built
# Level of detail: 0 = full (LOD0, <= 3,000 triangles), 1 = den background (LOD1, ~1,200).
# LOD1 is the same skeleton and shapes with fewer segments; the exporter writes both.
LOD = int(arg("--lod", "0"))


def use_form(name):
    global F
    F = FORMS[name]
    return F


def set_lod(level):
    global LOD
    LOD = level


def lod(full, low):
    """The LOD0 value, or the LOD1 one."""
    return low if LOD else full


def lerp(a, b, t):
    return a + (b - a) * t


def base_key(name):
    return name.rsplit("_", 1)[0] if name.endswith(("_L", "_R")) else name


SCALE_LIKE = {"jaw": "snout", "eyes": "head"}  # bones that grow like another (see extra_points)


def scale_key(name):
    """The growth and build table entry a bone uses."""
    return SCALE_LIKE.get(base_key(name), base_key(name))


def scales_for_t(t, build):
    """Bone scales (girth_x, length, girth_z) and part scales at growth t within the current
    form. The runtime computes exactly this from the tables exported in the .ecm."""
    bones = {}
    for name, *_ in BONES:
        h = F["young"]["bones"].get(scale_key(name), (1.0, 1.0))
        gx, l, gz = (h[0], h[1], h[0]) if len(h) == 2 else h
        gx, l, gz = lerp(gx, 1.0, t), lerp(l, 1.0, t), lerp(gz, 1.0, t)
        bg, bl = F["builds"][build].get(scale_key(name), (1.0, 1.0))
        bones[name] = (gx * bg, l * bl, gz * bg)
    parts = {k: lerp(v, 1.0, t) for k, v in F["young"]["parts"].items()}
    for name in WING_BONES:
        bones[name] = (parts["wings"],) * 3
    return bones, parts


def young_tables():
    """(bone name -> t = 0 scale triple) for the exporter, wings included."""
    bones, _ = scales_for_t(0.0, "neutral")
    return bones


# ------------------------------------------------------------------------------ helpers
def V(p):
    return Vector(p)


def link(obj):
    bpy.context.collection.objects.link(obj)
    return obj


def smooth(obj):
    """Smooth shading everywhere: clear any sharp-edge/face data modifiers leave behind."""
    me = obj.data
    for name in ("sharp_edge", "sharp_face"):
        if name in me.attributes:
            me.attributes.remove(me.attributes[name])
    for p in me.polygons:
        p.use_smooth = True


def apply_modifiers(obj):
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    for m in list(obj.modifiers):
        bpy.ops.object.modifier_apply(modifier=m.name)
    obj.select_set(False)


def tri_count(obj):
    dg = bpy.context.evaluated_depsgraph_get()
    me = obj.evaluated_get(dg).to_mesh()
    n = sum(len(p.vertices) - 2 for p in me.polygons)
    obj.evaluated_get(dg).to_mesh_clear()
    return n


def decimate_to(obj, target):
    cur = tri_count(obj)
    if cur > target:
        m = obj.modifiers.new("dec", "DECIMATE")
        m.ratio = target / cur
        m.use_collapse_triangulate = True
        apply_modifiers(obj)


def mesh_object(name, bm, location=(0, 0, 0)):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = link(bpy.data.objects.new(name, me))
    obj.location = location
    return obj


# ------------------------------------------------------------------------------ materials
def toon_material(name, color, accent=None, emission=0.0, rim=(1.0, 0.72, 0.45)):
    """Toon ramp x colour (+ accent via the 'mask' colour attribute) + warm rim."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    if emission:
        em = nt.nodes.new("ShaderNodeEmission")
        em.inputs["Color"].default_value = (*color, 1)
        em.inputs["Strength"].default_value = emission
        nt.links.new(em.outputs[0], out.inputs["Surface"])
        return mat
    diff = nt.nodes.new("ShaderNodeBsdfDiffuse")
    s2r = nt.nodes.new("ShaderNodeShaderToRGB")
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.interpolation = "CONSTANT"
    els = ramp.color_ramp.elements
    els[0].position, els[0].color = 0.0, (0.42, 0.36, 0.44, 1)  # plum-tinted shadow
    els[1].position, els[1].color = 0.22, (0.78, 0.74, 0.76, 1)
    e = els.new(0.55)
    e.color = (1.0, 0.98, 0.95, 1)
    nt.links.new(diff.outputs[0], s2r.inputs[0])
    nt.links.new(s2r.outputs["Color"], ramp.inputs["Fac"])
    base = nt.nodes.new("ShaderNodeRGB")
    base.outputs[0].default_value = (*color, 1)
    col = base.outputs[0]
    if accent is not None:
        attr = nt.nodes.new("ShaderNodeAttribute")
        attr.attribute_name = "mask"
        sep = nt.nodes.new("ShaderNodeSeparateColor")
        acc = nt.nodes.new("ShaderNodeRGB")
        acc.outputs[0].default_value = (*accent, 1)
        mix = nt.nodes.new("ShaderNodeMix")
        mix.data_type = "RGBA"
        nt.links.new(attr.outputs["Color"], sep.inputs[0])
        nt.links.new(sep.outputs["Green"], mix.inputs["Factor"])
        nt.links.new(base.outputs[0], mix.inputs["A"])
        nt.links.new(acc.outputs[0], mix.inputs["B"])
        col = mix.outputs["Result"]
    mul = nt.nodes.new("ShaderNodeMix")
    mul.data_type = "RGBA"
    mul.blend_type = "MULTIPLY"
    mul.inputs["Factor"].default_value = 1.0
    nt.links.new(col, mul.inputs["A"])
    nt.links.new(ramp.outputs["Color"], mul.inputs["B"])
    # warm rim
    lw = nt.nodes.new("ShaderNodeLayerWeight")
    lw.inputs["Blend"].default_value = 0.35
    rr = nt.nodes.new("ShaderNodeValToRGB")
    rr.color_ramp.interpolation = "CONSTANT"
    rr.color_ramp.elements[0].position = 0.0
    rr.color_ramp.elements[0].color = (0, 0, 0, 1)
    rr.color_ramp.elements[1].position = 0.72
    rr.color_ramp.elements[1].color = (*[c * 0.35 for c in rim], 1)
    nt.links.new(lw.outputs["Facing"], rr.inputs["Fac"])
    add = nt.nodes.new("ShaderNodeMix")
    add.data_type = "RGBA"
    add.blend_type = "ADD"
    add.inputs["Factor"].default_value = 1.0
    nt.links.new(mul.outputs["Result"], add.inputs["A"])
    nt.links.new(rr.outputs["Color"], add.inputs["B"])
    em = nt.nodes.new("ShaderNodeEmission")
    nt.links.new(add.outputs["Result"], em.inputs["Color"])
    nt.links.new(em.outputs[0], out.inputs["Surface"])
    return mat


# ------------------------------------------------------------------------------ body
def build_body():
    obj = build_skin_body() if F["body"] == "skin" else build_meta_body()
    decimate_to(obj, lod(F["body_tris"], F["body_tris_lod1"]))
    remove_loose(obj)
    smooth(obj)
    paint_mask(obj)
    return obj


def remove_loose(obj):
    """Drop vertices that belong to no face. Decimation can leave one behind; it never
    shows, but it still gets skinned (to whatever bone is nearest) and the floor contact
    counts it: a loose vertex under the grown body swung with the left foreleg and lifted
    the dragon every stride (Noah's "limp", 2026-09-24)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    loose = [v for v in bm.verts if not v.link_faces]
    if loose:
        bmesh.ops.delete(bm, geom=loose, context="VERTS")
        bm.to_mesh(obj.data)
    bm.free()


def build_skin_body():
    nodes = F["nodes"]
    names = list(nodes)
    idx = {n: i for i, n in enumerate(names)}
    me = bpy.data.meshes.new("body")
    me.from_pydata([nodes[n][0] for n in names], [(idx[a], idx[b]) for a, b in F["edges"]], [])
    obj = link(bpy.data.objects.new("body", me))
    skin = obj.modifiers.new("skin", "SKIN")
    skin.use_smooth_shade = True
    skin.branch_smoothing = 0.6
    for i, n in enumerate(names):
        sv = me.skin_vertices[0].data[i]
        sv.radius = nodes[n][1]
        sv.use_root = n == "hips"
    sub = obj.modifiers.new("sub", "SUBSURF")
    sub.levels = sub.render_levels = int(arg("--subd", "2"))
    sm = obj.modifiers.new("relax", "SMOOTH")  # even out skin-modifier lumps before decimating
    sm.factor = 0.6
    sm.iterations = 6
    apply_modifiers(obj)
    sculpt_details(obj)
    return obj


# A lone metaball of radius 1 (stiffness 2, threshold 0.6) is visible out to 0.575.
META_VISIBLE = 0.575


def build_meta_body():
    """Metaballs blend soft round volumes: ideal for a chubby baby. Voxel remesh evens the
    topology for decimation and weighting."""
    mb = bpy.data.metaballs.new("body")
    mb.resolution = mb.render_resolution = 0.018

    def ball(c, r):
        e = mb.elements.new(type="BALL")
        e.co, e.radius = c, r / META_VISIBLE

    for kind, c, size in F["meta"]:
        if kind == "ball":
            ball(c, size)
        elif kind == "ell":
            e = mb.elements.new(type="ELLIPSOID")
            m = max(size)
            e.co, e.radius = c, m / META_VISIBLE
            e.size_x, e.size_y, e.size_z = (r / m for r in size)
        else:  # chain of balls with linearly tapering radii
            pts, radii = [V(p) for p in c], size
            for (a, ra), (b, rb) in zip(zip(pts, radii), zip(pts[1:], radii[1:])):
                n = max(1, int((b - a).length / (0.35 * min(ra, rb))))
                for k in range(n):
                    ball(a.lerp(b, k / n), ra + (rb - ra) * k / n)
            ball(pts[-1], radii[-1])
    obj = link(bpy.data.objects.new("body", mb))
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    obj = bpy.context.view_layer.objects.active
    obj.select_set(False)
    rm = obj.modifiers.new("remesh", "REMESH")
    rm.mode = "VOXEL"
    rm.voxel_size = 0.03
    sm = obj.modifiers.new("relax", "SMOOTH")
    sm.factor, sm.iterations = 0.5, 6
    apply_modifiers(obj)
    obj.name = obj.data.name = "body"
    return obj


def sculpt_details(obj):
    """Grown form only. Procedural shaping the skin graph can't do: deep chest keel, brow,
    cheeks, tapered snout."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for v in bm.verts:
        x, y, z = v.co
        # chest keel: push the underside of the chest down/forward
        if -0.9 < y < 0.2 and z < 1.15 and abs(x) < 0.35:
            k = (1 - abs(x) / 0.35) * max(0.0, (1.15 - z) / 0.5)
            v.co.z -= 0.10 * k
            v.co.y -= 0.05 * k
        # brow ridge over the eyes
        if -1.45 < y < -1.1 and z > 3.12 and abs(x) > 0.08:
            v.co.z += 0.05
        # cheeks: widen the back of the head a little
        if -1.35 < y < -1.05 and 2.95 < z < 3.2:
            v.co.x *= 1.12
        # snout: flatten the top and taper to the nose
        if y < -1.5 and z > 2.8:
            t = min(1.0, (-1.5 - y) / 0.35)
            v.co.z -= 0.03 * t
            v.co.x *= 1.0 - 0.18 * t
    bm.to_mesh(obj.data)
    bm.free()


def paint_mask(obj):
    """Per-vertex mask colour: R = base weight, G = accent (belly, throat, under-tail)."""
    me = obj.data
    mk = F["mask"]
    attr = me.color_attributes.new("mask", "FLOAT_COLOR", "POINT")
    down = Vector((0, -0.45, -0.89)).normalized()
    for v in me.vertices:
        x, y, z = v.co
        a = min(1.0, max(0.0, (v.normal.dot(down) - 0.25) / 0.45))
        if abs(x) > mk["max_x"] or z > mk["max_z"] or z < mk["min_z"]:
            a = 0.0
        if mk["tail_cut"] and y > mk["tail_cut"][0] and z < mk["tail_cut"][1]:
            a = 0.0
        attr.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


# ------------------------------------------------------------------------------ armature
def wing_points(side, authored=False):
    """WING_LAYOUT placed in 3D: the span axis rises by the dihedral, the chord axis droops.
    Once seat_wings() has run, the root and the flank point are the seated ones (authored=True
    gives the layout as written, which seat_wings() starts from)."""
    w = F["wing"]
    s = -1 if side == "L" else 1
    th, ph = math.radians(w["dihedral"]), math.radians(w["droop"])
    span = Vector((s * math.cos(th), 0, math.sin(th)))
    chord = Vector((0, math.cos(ph), -math.sin(ph)))
    seat = None if authored else w.get("seat")
    root = mirror(seat["root"] if seat else w["root"], s)
    pts = {k: root + (span * u + chord * v) * w["scale"] for k, (u, v) in WING_LAYOUT.items()}
    if seat:
        pts["body"] = mirror(seat["body"], s)
    return pts


def seat_wings(body):
    """Moves the wing's root joint just under the body's skin (WING_SINK root radii) and its
    flank point onto the skin (WEB_SINK), from the nearest points on the surface, so the arm
    grows out of the back and the membrane's inner edge lies along the flank. Measured on
    the LOD0 body once per form (both LODs share one skeleton): an LOD1 build makes a LOD0
    body to measure and throws it away."""
    w = F["wing"]
    if "seat" in w:
        return
    probe = body
    if LOD:
        was = LOD
        set_lod(0)
        probe = build_body()
        set_lod(was)
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    bvh = BVHTree.FromObject(probe.evaluated_get(dg), dg)
    to_local, to_world = probe.matrix_world.inverted(), probe.matrix_world

    def onto_skin(p, sink):
        hit, nrm, _, _ = bvh.find_nearest(to_local @ p)
        n = (to_world.to_3x3() @ nrm).normalized()
        return (to_world @ hit) - n * sink, n

    guide = wing_points("R", authored=True)
    # No further back than the hips: bigger wings (the v2 look) put it over the tail, where
    # the skin follows the tail and legs, not the three body bones the wing draw can use.
    guide["body"].y = min(guide["body"].y, F["nodes"]["hips"][0][1])
    root, normal = onto_skin(guide["root"], WING_SINK * w["radii"]["root"])
    flank, _ = onto_skin(guide["body"], WEB_SINK * w["thickness"])
    w["seat"] = dict(root=tuple(root), body=tuple(flank), inward=tuple(-normal))
    if probe is not body:
        bpy.data.objects.remove(probe, do_unlink=True)
    print(f"[dragon] {F['name']}: wing root seated {(root - guide['root']).length:.3f} from its guide, "
          f"flank point {(flank - guide['body']).length:.3f}")


def extra_points():
    """Bones that are not body nodes, each parallel to a body bone, as long, parented to
    it and growing like it (SCALE_LIKE): until it moves on its own, it skins exactly like
    that bone.
      * jaw: from the hinge behind the mouth corners, like the snout (the lips meet at every
        stage and build);
      * eyes: from the point between the eyes, like the head (the runtime squashes its
        vertical axis to blink)."""
    nodes = F["nodes"]
    head, muzzle, tip = V(nodes["head"][0]), V(nodes["muzzle"][0]), V(nodes["snout"][0])
    hinge = V(F["jaw_hinge"])
    eyes = V((0.0, F["eyes"]["at"][1], F["eyes"]["at"][2]))
    return {"jaw_hinge": hinge, "jaw_tip": hinge + (tip - muzzle), "eyes_c": eyes, "eyes_tip": eyes + (muzzle - head)}


def build_armature():
    arm_data = bpy.data.armatures.new("rig")
    arm = link(bpy.data.objects.new("rig", arm_data))
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.mode_set(mode="EDIT")
    eb = {}
    nodes = F["nodes"]

    def add(name, head, tail, parent):
        b = arm_data.edit_bones.new(name)
        b.head, b.tail = head, tail
        b.roll = 0
        if parent:
            b.parent = eb[parent]
            b.use_connect = (b.head - eb[parent].tail).length < 1e-4
        b.inherit_scale = "NONE"
        eb[name] = b

    extra = extra_points()
    for name, h, t, parent in BONES:
        add(name, V(extra[h] if h in extra else nodes[h][0]), V(extra[t] if t in extra else nodes[t][0]), parent)
    for side in ("L", "R"):
        w = wing_points(side)
        for name, h, t, parent in WING_CHAIN:
            add(f"{name}_{side}", w[h], w[t], parent if parent == "chest" else f"{parent}_{side}")
    bpy.ops.object.mode_set(mode="OBJECT")
    return arm


def bind(mesh_obj, arm, keep):
    """Automatic (heat) weights from the bones this draw may use, at most 2 per vertex."""
    saved = {b.name: b.use_deform for b in arm.data.bones}
    for b in arm.data.bones:
        b.use_deform = keep(b.name)
    bpy.ops.object.select_all(action="DESELECT")
    mesh_obj.select_set(True)
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.parent_set(type="ARMATURE_AUTO")
    bpy.ops.object.select_all(action="DESELECT")
    for b in arm.data.bones:
        b.use_deform = saved[b.name]
    for vg in list(mesh_obj.vertex_groups):
        if not keep(vg.name):
            mesh_obj.vertex_groups.remove(vg)
    bpy.context.view_layer.objects.active = mesh_obj
    mesh_obj.select_set(True)
    bpy.ops.object.mode_set(mode="WEIGHT_PAINT")
    bpy.ops.object.vertex_group_normalize_all(lock_active=False)
    bpy.ops.object.vertex_group_limit_total(group_select_mode="ALL", limit=2)  # shader blends 2 bones
    bpy.ops.object.vertex_group_normalize_all(lock_active=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    mesh_obj.select_set(False)
    unweighted = sum(1 for v in mesh_obj.data.vertices if not any(g.weight > 0 for g in v.groups))
    assert unweighted == 0, f"{mesh_obj.name}: {unweighted} vertices without bone weights"


def parent_to_bone(obj, arm, bone):
    """Attach a rigid part at the bone's JOINT (pose matrix origin), like the runtime does.
    Plain bone-parenting in Blender is relative to the bone's tail, which pushes parts away
    from the surface when a growth stage scales the bone."""
    bpy.context.view_layer.update()
    c = obj.constraints.new("CHILD_OF")
    c.target = arm
    c.subtarget = bone
    c.inverse_matrix = (arm.matrix_world @ arm.pose.bones[bone].matrix).inverted()
    obj["base_loc"] = list(obj.location)


# ------------------------------------------------------------------------------ part shapes
def horn_point(length, curve, t):
    """horn_mesh's centre line at t (0 base .. 1 tip), in its local frame."""
    ang = curve * t
    return Vector((0, math.sin(ang) * length * t * 0.9, math.cos(ang) * length * t))


def horn_mesh(name, length, radius, curve, segments=6, ring=5, faceted=False):
    """A tapered horn swept backward along a curve (in local +Y back, +Z up). Faceted: flat
    shading (crystal horns)."""
    bm = bmesh.new()
    rings = []
    for i in range(segments + 1):
        t = i / segments
        ang = curve * t
        c = Vector((0, math.sin(ang) * length * t * 0.9, math.cos(ang) * length * t))
        r = radius * (1 - t) ** 0.9 + radius * 0.06
        rings.append([bm.verts.new(c + Vector((math.cos(a) * r, math.sin(a) * r * 0.8, 0)))
                      for a in (2 * math.pi * k / ring for k in range(ring))])
    for a, b in zip(rings, rings[1:]):
        for k in range(ring):
            bm.faces.new((a[k], a[(k + 1) % ring], b[(k + 1) % ring], b[k]))
    bm.faces.new(list(reversed(rings[0])))
    obj = mesh_object(name, bm)
    if not faceted:
        smooth(obj)
    return obj


def flat_fan(name, pts, thickness=0.015):
    """Flat fan from pts[0]; the object origin sits at pts[0] so part scaling pivots there."""
    origin = Vector(pts[0])
    bm = bmesh.new()
    vs = [bm.verts.new(Vector(p) - origin) for p in pts]
    for i in range(1, len(vs) - 1):
        bm.faces.new((vs[0], vs[i], vs[i + 1]))
    obj = mesh_object(name, bm, origin)
    m = obj.modifiers.new("thick", "SOLIDIFY")
    m.thickness = thickness
    m.offset = 0
    m.use_rim = False  # two sheets; a rim on something this thin costs triangles nobody sees
    apply_modifiers(obj)
    return obj


def blade(name, base, direction, side, length, width, thickness=0.01):
    """A feather / leaf blade from base along direction: widest a third of the way out, with a
    rounded tip (a sharp diamond read as ice shards in R1b drafts)."""
    d = Vector(direction).normalized()
    s = (Vector(side) - d * Vector(side).dot(d)).normalized()
    b = Vector(base)
    if LOD:
        return flat_fan(name, [b, b + d * length * 0.35 + s * width * 0.5, b + d * length,
                               b + d * length * 0.35 - s * width * 0.5], thickness)
    return flat_fan(name, [b, b + d * length * 0.3 + s * width * 0.5, b + d * length * 0.75 + s * width * 0.38,
                           b + d * length * 0.95 + s * width * 0.14, b + d * length,
                           b + d * length * 0.95 - s * width * 0.14, b + d * length * 0.75 - s * width * 0.38,
                           b + d * length * 0.3 - s * width * 0.5], thickness)


def lobed_fin(name, base, out, up, radius, a0, a1, lobes, thickness, n=None):
    """A rounded fin fanning from base between angles a0..a1 (degrees, in the out/up plane)
    with a gently lobed edge."""
    base, out, up = Vector(base), Vector(out).normalized(), Vector(up).normalized()
    n = n or lod(10, 5)
    pts = [base]
    for j in range(n + 1):
        f = j / n
        a = math.radians(a0 + (a1 - a0) * f)
        r = radius * (1 - 0.14 * abs(math.sin(math.pi * lobes * f))) * (0.75 + 0.25 * math.sin(math.pi * f))
        pts.append(base + (out * math.cos(a) + up * math.sin(a)) * r)
    return flat_fan(name, pts, thickness)


def add_dome(bm, center, out, up, rx, ry, depth, seg, rings, material):
    """Front hemisphere (the back is buried in the head), faces pointing along `out`."""
    out = Vector(out).normalized()
    right = Vector(up).cross(out).normalized()
    up = out.cross(right).normalized()
    center = Vector(center)
    layers = []
    for k in range(rings):
        phi = 0.5 * math.pi * k / rings
        layers.append([bm.verts.new(center + right * math.cos(a) * rx * math.cos(phi) +
                                    up * math.sin(a) * ry * math.cos(phi) + out * depth * math.sin(phi))
                       for a in (2 * math.pi * j / seg for j in range(seg))])
    pole = bm.verts.new(center + out * depth)
    faces = []
    for a, b in zip(layers, layers[1:]):
        faces += [bm.faces.new((a[j], a[(j + 1) % seg], b[(j + 1) % seg], b[j])) for j in range(seg)]
    faces += [bm.faces.new((layers[-1][j], layers[-1][(j + 1) % seg], pole)) for j in range(seg)]
    for f in faces:
        f.material_index = material
        f.normal_update()
        if f.normal.dot(f.calc_center_median() - center) < 0:
            f.normal_flip()


# ------------------------------------------------------------------------------ parts
def mirror(p, s):
    return Vector((s * p[0], p[1], p[2]))


def head_point(off, s=1):
    h = F["head"]
    return V(h["origin"]) + mirror(off, s) * h["k"]


def build_eyes(mats):
    """Big glossy eyes: an iris dome, a pupil dome on it and two glints, one object per eye
    (origin at the iris centre, so growth scaling keeps the pieces together). No white
    eyeball: that read as a frog."""
    e = F["eyes"]
    iseg, irings, pseg, prings = lod(e["seg"], (8, 1, 6, 1))
    irx, iry, idepth = e["iris"]
    prx, pry, pdepth = e["pupil"]
    # The pupil's rim sits on the iris surface.
    poff = idepth * math.sqrt(max(0.0, 1 - (prx / irx) ** 2)) - 0.1 * pdepth
    eyes = []
    for s in (-1, 1):
        at, out = mirror(e["at"], s), mirror(e["out"], s).normalized()
        right = Vector((0, 0, 1)).cross(out).normalized()
        up = out.cross(right).normalized()
        bm = bmesh.new()
        add_dome(bm, (0, 0, 0), out, up, irx, iry, idepth, iseg, irings, 0)
        add_dome(bm, out * poff, out, up, prx, pry, pdepth, pseg, prings, 1)
        for gx, gy, gr in lod(e["glints"], e["glints"][:1]):
            gx *= -s  # glints sit toward the nose on both eyes
            h = poff + pdepth * math.sqrt(max(0.0, 1 - (gx / prx) ** 2 - (gy / pry) ** 2))
            add_dome(bm, out * (h + 0.003) + right * gx + up * gy, out, up, gr, gr, gr * 0.3, lod(8, 4), 1, 2)
        obj = mesh_object(f"eye_{s}", bm, at)
        for m in ("iris", "pupil", "glint"):
            obj.data.materials.append(mats[m])
        smooth(obj)
        eyes.append(obj)
    return eyes


HORN_KINDS = {  # offsets in the head frame; rot = (x, y mirrored, z) degrees
    "swept": [dict(len=0.62, r=0.085, curve=55, seg=5, ring=5, at=(0.13, 0.10, 0.16), rot=(-18, 12, 0)),
              dict(len=0.28, r=0.042, curve=70, seg=4, ring=4, at=(0.22, 0.02, 0.02), rot=(-40, 55, 0),
                   minor=True)],
    "nubs": [dict(len=0.22, r=0.075, curve=35, seg=4, ring=5, at=(0.12, 0.08, 0.17), rot=(-20, 15, 0))],
    # Lumen's halo crest: a circlet of short upright spikes round the crown of the head.
    "crown": [dict(len=0.27, r=0.045, curve=8, seg=3, ring=4, at=(0.0, 0.07, 0.25), rot=(-6, 0, 0), centre=True),
              dict(len=0.23, r=0.04, curve=10, seg=3, ring=4, at=(0.09, 0.06, 0.23), rot=(-8, 20, 0)),
              dict(len=0.18, r=0.036, curve=12, seg=3, ring=4, at=(0.16, 0.02, 0.18), rot=(-10, 42, 0), minor=True)],
    # Frost: faceted crystals, a tall one and a small one beside it.
    "crystal": [dict(len=0.5, r=0.085, curve=14, seg=2, ring=4, at=(0.12, 0.08, 0.17), rot=(-24, 10, 0), faceted=True),
                dict(len=0.24, r=0.05, curve=6, seg=2, ring=4, at=(0.19, 0.03, 0.09), rot=(-46, 42, 0), faceted=True,
                     minor=True)],
    # Grove: bark antlers, a beam swept back with two tines off it (on=(beam, t): where along it).
    "antler": [dict(len=0.56, r=0.055, curve=48, seg=5, ring=4, at=(0.12, 0.08, 0.17), rot=(-26, 22, 0)),
               dict(len=0.2, r=0.03, curve=20, seg=2, ring=4, on=(0, 0.45), rot=(10, 34, 0), minor=True),
               dict(len=0.16, r=0.026, curve=25, seg=2, ring=4, on=(0, 0.75), rot=(-60, 46, 0), minor=True)],
}


def build_horns(kind, mats):
    h = F["head"]
    horns = []
    for s in (-1, 1):
        beams = []  # this side's pieces, for tines that grow off a beam
        for spec in HORN_KINDS[kind]:
            if (h["buds"] and spec.get("minor")) or (spec.get("centre") and s < 0):
                beams.append(None)
                continue  # hatchlings only have the main horn buds; a centre spike is one piece
            seg = max(2 if spec.get("faceted") or spec.get("on") else 3, spec["seg"] - (2 if h["buds"] else 0))
            length = spec["len"] * h["k"] * h["horn_len"]
            curve = math.radians(spec["curve"]) * h["horn_curve"]
            o = horn_mesh(f"horn_{s}", length, spec["r"] * h["k"] * h["horn_r"], curve,
                          lod(seg, max(2, seg - 3)), lod(spec["ring"], 4), faceted=spec.get("faceted", False))
            rx, ry, rz = spec["rot"]
            if "on" in spec:  # a tine: on its beam's centre line
                beam, t = spec["on"]
                b = beams[beam]
                if b is None:
                    bpy.data.objects.remove(o, do_unlink=True)
                    beams.append(None)
                    continue
                bo, blen, bcurve = b
                o.location = bo.location + bo.rotation_euler.to_matrix() @ horn_point(blen, bcurve, t)
            else:
                o.location = head_point(spec["at"], s)
            o.rotation_euler = (math.radians(rx), s * math.radians(ry), math.radians(rz))
            o.data.materials.append(mats["horn"])
            horns.append(o)
            beams.append((o, length, curve))
    return horns


def build_frill(kind, mats):
    """Head frills. Fin: big ear fins + cheek fins. Feather: a crest of long plumes sweeping
    back from the crown + cheek feathers."""
    h = F["head"]
    k, wk = h["frill_k"], h["feather_w"]
    parts = []

    def hp(off, s=1):  # frills scale about the head frame's side anchor, not its origin
        return head_point((off[0] * k + 0.20 * (1 - k), off[1] * k, off[2] * k), s)

    if kind == "fin":  # rounded, lobed ear fins sweeping back + small cheek fins
        for s in (-1, 1):
            out = Vector((s * 0.5, 0.86, 0.0))
            parts.append(lobed_fin(f"fin_{s}", hp((0.20, 0.06, 0.0), s), out, (0, 0, 1),
                                   0.56 * k * h["k"], 70, -45, 3, 0.016 * h["k"]))
            parts.append(lobed_fin(f"cheekfin_{s}", hp((0.18, -0.08, -0.14), s), Vector((s * 0.6, 0.8, 0.0)),
                                   (0, 0, 1), 0.3 * k * h["k"], 10, -60, 2, 0.014 * h["k"]))
    elif kind == "leaf":  # Grove: two big leaves for ears, swept back, and a small leaf on each cheek
        for s in (-1, 1):
            for j, (off, d, length, width) in enumerate((((0.19, 0.05, 0.02), (s * 0.55, 0.78, 0.30), 0.62, 0.30),
                                                           ((0.18, -0.06, -0.13), (s * 0.7, 0.66, -0.1), 0.34, 0.2))):
                parts.append(blade(f"leaf_{s}_{j}", hp(off, s), Vector(d), Vector((0, -0.25, 1)),
                                   length * k * h["k"], width * k * h["k"]))
    elif kind == "feather":
        for x, length, spread in ((0.0, 0.80, 0.0), (0.07, 0.66, 0.28), (-0.07, 0.66, -0.28)):
            base = head_point((x, 0.04, 0.22))
            d = Vector((spread, 0.82, 0.55))
            parts.append(blade(f"crest_{x:+.2f}", base, d, Vector((1, 0, -0.3)), length * k * h["k"],
                               0.16 * wk * k * h["k"]))
        for s in (-1, 1):
            for j, (dz, length) in enumerate(((0.02, 0.46), (-0.12, 0.36))):
                base = head_point((0.18, 0.02, dz), s)
                d = Vector((s * 0.45, 0.88, 0.12 - j * 0.1))
                parts.append(blade(f"cheekfeather_{s}_{j}", base, d, Vector((0, -0.2, 1)), length * k * h["k"],
                                   0.12 * wk * k * h["k"]))
    for p in parts:
        p.data.materials.append(mats["accent_flat"])
    return parts


# The Runes pattern (Frost, Lumen): glowing glyphs of a few thin strokes, in a unit box.
RUNE_GLYPHS = [
    [((0.2, 0.0), (0.2, 1.0)), ((0.2, 1.0), (0.75, 0.78)), ((0.75, 0.78), (0.2, 0.55)), ((0.2, 0.55), (0.8, 0.0))],
    [((0.5, 0.0), (0.5, 1.0)), ((0.5, 0.55), (0.1, 1.0)), ((0.5, 0.55), (0.9, 1.0))],
    [((0.5, 1.0), (0.9, 0.62)), ((0.9, 0.62), (0.5, 0.25)), ((0.5, 0.25), (0.1, 0.62)), ((0.1, 0.62), (0.5, 1.0)),
     ((0.5, 0.25), (0.15, 0.0)), ((0.5, 0.25), (0.85, 0.0))],
    [((0.25, 0.0), (0.25, 1.0)), ((0.25, 0.75), (0.75, 0.5)), ((0.75, 0.5), (0.25, 0.25))],
]


def build_runes(mats):
    """Runes: small glowing glyphs along the neck and flanks, a few thin strokes each, lying
    on the scales (seated on the surface like the heart, a touch proud of it). [(object, bone)]"""
    pieces = []
    for i, (bone, at, glyph, size) in enumerate(F["runes"]):
        for s in (-1, 1):
            c = mirror(at, s)
            joint = V(F["nodes"][bone][0])
            out = (c - joint).normalized()
            right = Vector((0, 0, 1)).cross(out)
            right = (right if right.length > 1e-4 else Vector((0, 1, 0))).normalized()
            up = out.cross(right).normalized()
            bm = bmesh.new()
            w = 0.12  # stroke width, of the glyph's size
            for (x0, y0), (x1, y1) in RUNE_GLYPHS[glyph % len(RUNE_GLYPHS)]:
                a = right * ((x0 - 0.5) * size * s) + up * ((y0 - 0.5) * size)
                b = right * ((x1 - 0.5) * size * s) + up * ((y1 - 0.5) * size)
                along = (b - a).normalized()
                side = out.cross(along).normalized() * (w * size * 0.5)
                a -= along * (w * size * 0.4)
                b += along * (w * size * 0.4)
                vs = [bm.verts.new(p) for p in (a - side, b - side, b + side, a + side)]
                f = bm.faces.new(vs)
                f.normal_update()
                if f.normal.dot(out) < 0:
                    f.normal_flip()
            obj = mesh_object(f"rune_{i}_{s}", bm, c)
            obj.data.materials.append(mats["rune"])
            pieces.append((obj, bone))
    return pieces


def spine_up(node, nxt):
    """Direction perpendicular to the spine at a node, pointing up/back."""
    d = (V(F["nodes"][nxt][0]) - V(F["nodes"][node][0])).normalized()
    n = Vector((0, -d.z, d.y))
    return n if n.z + 0.3 * n.y > 0 else -n


def build_ridge(kind, mats):
    """Dorsal ridge along the spine (group 'spikes'; the variant follows the frill gene).
    Returns [(object, bone)]. Every piece is rigid on one spine bone."""
    r = F["ridge"]
    path, nodes = r["path"], F["nodes"]
    s0, s1, top_ref = r["size"]
    pieces = []
    for i, (node, top) in enumerate(path):
        nxt = path[i + 1][0] if i + 1 < len(path) else "tail_tip"
        up = spine_up(node, nxt)
        c = V(nodes[node][0])
        base = c + up * top * 0.92
        size = s0 + s1 * min(1.0, top / top_ref)
        if kind == "spikes":
            lf, rf, curve = r["spike"]
            o = horn_mesh(f"spike_{i}", size * lf, size * rf, math.radians(curve), lod(3, 2), lod(4, 3))
            o.location = base
            o.rotation_euler = (-math.atan2(up.y, up.z) - math.radians(10), 0, 0)
            o.data.materials.append(mats["horn"])
            pieces.append((o, node))
        elif kind == "fin":
            h0f, h1f, thick = r["fin"]
            ntop = path[i + 1][1] if i + 1 < len(path) else top * 0.5
            nbase = V(nodes[nxt][0]) + up * ntop * 0.92
            nsize = s0 + s1 * min(1.0, ntop / top_ref)
            h0, h1 = size * h0f, nsize * h1f
            end = base.lerp(nbase, 1.12)  # overlap the next panel so bends don't open gaps
            pts = [base, base + up * h0, base.lerp(end, 0.5) + up * (h0 + h1) * 0.55, end + up * h1, end]
            o = flat_fan(f"finridge_{i}", pts, thick)
            o.data.materials.append(mats["membrane"])
            pieces.append((o, node))
        else:  # feather plumes: a mane on the neck, single plumes down the back
            lf, wf, thick = r["plume"]
            back = (V(nodes[nxt][0]) - c).normalized()
            d = (back * 0.8 + up * 0.6).normalized()
            for j, xo in enumerate((-1, 1) if node.startswith("neck") else (0,)):
                o = blade(f"plume_{i}_{j}", base + Vector((xo * size * 0.35, 0, 0)),
                          d + Vector((xo * 0.25, 0, 0)), Vector((1, 0, 0)), size * lf, size * wf, thick)
                o.data.materials.append(mats["accent_flat"])
                pieces.append((o, node))
    return pieces


def build_tail_tip(kind, mats):
    x, y, z = F["nodes"]["tail_tip"][0]
    k = F["tail_k"]
    if kind == "spade":
        obj = flat_fan("tail_tip", [Vector((0, y - 0.05 * k, z)), Vector((-0.24 * k, y + 0.14 * k, z)),
                                    Vector((0, y + 0.5 * k, z)), Vector((0.24 * k, y + 0.14 * k, z))], 0.04 * k)
        obj.data.materials.append(mats["horn"])
    elif kind == "fan":
        pts = [Vector((0, y - 0.05 * k, z))]
        rays = lod(7, 4)
        for j in range(rays):
            a = math.radians(-65 + j * 130 / (rays - 1))
            pts.append(Vector((math.sin(a) * 0.40 * k, y + math.cos(a) * 0.46 * k, z + 0.02 * k)))
        obj = flat_fan("tail_tip", pts, 0.02 * k)
        obj.data.materials.append(mats["membrane"])
    else:  # tuft: a plume of feathers fanning from the tip
        blades = []
        base = Vector((0, y - 0.03 * k, z))
        for j, (ax, az) in enumerate(lod(((0, 0), (-32, 8), (32, 8), (-16, -14), (16, -14)),
                                         ((0, 0), (-28, 4), (28, 4)))):
            d = Vector((math.sin(math.radians(ax)), math.cos(math.radians(ax)), math.sin(math.radians(az))))
            side = Vector((math.cos(math.radians(ax)), -math.sin(math.radians(ax)), 0.3))
            blades.append(blade(f"tuft_{j}", base, d, side, (0.5 if j == 0 else 0.42) * k, 0.16 * k, 0.012 * k))
        bpy.ops.object.select_all(action="DESELECT")
        for b in blades:
            b.select_set(True)
        bpy.context.view_layer.objects.active = blades[0]
        bpy.ops.object.join()
        obj = bpy.context.view_layer.objects.active
        obj.select_set(False)
        obj.name = "tail_tip"
        obj.data.materials.clear()
        obj.data.materials.append(mats["accent_flat"])
    return obj


def build_heart(mats):
    """The heartglow: a flat heart emblem on the chest (emissive)."""
    c = V(F["heart"]["at"])
    size = F["heart"]["size"]
    pts = [c]
    steps = lod(16, 8)
    for j in range(steps + 1):
        t = 2 * math.pi * j / steps
        x = 16 * math.sin(t) ** 3
        zz = 13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t)
        pts.append(c + Vector((x, 0, zz + 2)) * (size / 17))
    obj = flat_fan("heart", pts, 0.2 * size)
    obj.data.materials.append(mats["heart"])
    return [obj]


def wing_edge(w, style):
    """Trailing edge from the flank to the leading finger tip. Every membrane panel is bounded
    by fingers, so there is no bare stretch (R1). classic: scalloped; sail: smooth and
    rounded; plumed: frilled into feather points."""
    def sag(a, b, depths):
        n = len(depths)
        return [a.lerp(b, j / (n + 1)).lerp(w["wrist"], depths[j - 1]) for j in range(1, n + 1)]

    edge = [w["body"]]
    for a, b, depth in (("body", "f4", 0.20), ("f4", "f3", 0.26), ("f3", "f2", 0.26), ("f2", "f1", 0.22)):
        n = lod(3, 1)
        if style == "sail":
            depths = [-0.05 * math.sin(math.pi * j / (n + 1)) for j in range(1, n + 1)]
        elif style == "plumed":
            m = lod(5, 3)
            depths = [depth * math.sin(math.pi * j / (m + 1)) * (1.0 if j % 2 else 0.35) for j in range(1, m + 1)]
        else:
            depths = [depth * math.sin(math.pi * j / (n + 1)) for j in range(1, n + 1)]
        edge += sag(w[a], w[b], depths) + [w[b]]
    return edge


def build_wings(style, mats):
    """Classic dragon wings for every breed: arm, forearm, a thumb claw and four long fingers
    spread across the whole membrane, which reaches back along the flank."""
    wr = F["wing"]["radii"]
    objs = []
    for side in ("L", "R"):
        w = wing_points(side)
        s = -1 if side == "L" else 1
        # A stub into the body first (WING_STUB, weighted to the chest: weight_wingarm), so the
        # arm always comes out of the skin, however far the shoulder turns.
        inward = mirror(F["wing"]["seat"]["inward"], s) if "seat" in F["wing"] else Vector((-s, 0, -1)).normalized()
        names = ["root", "elbow", "wrist", "thumb", "f1", "f2", "f3", "f4"]
        pos = [w["root"] + inward * (WING_STUB * wr["root"])] + [w[n] for n in names]
        rad = [wr["root"], wr["root"], wr["elbow"], wr["wrist"], wr["tip"] * 2.5] + [wr["finger"]] * 4
        edges = [(0, 1), (1, 2), (2, 3), (3, 4), (3, 5), (3, 6), (3, 7), (3, 8)]
        for fi in range(5, 9):  # claw tips poke past the membrane
            pos.append(pos[fi] + (pos[fi] - w["wrist"]) * 0.07)
            rad.append(wr["tip"])
            edges.append((fi, len(pos) - 1))
        me = bpy.data.meshes.new(f"wingarm_{side}")
        me.from_pydata(pos, edges, [])
        arm = link(bpy.data.objects.new(f"wingarm_{side}", me))
        arm.modifiers.new("skin", "SKIN")
        for i, r in enumerate(rad):
            me.skin_vertices[0].data[i].radius = (r, r)
            me.skin_vertices[0].data[i].use_root = i == 0
        sub = arm.modifiers.new("sub", "SUBSURF")
        sub.levels = 1
        apply_modifiers(arm)
        decimate_to(arm, lod(F["wing"]["arm_tris"], 56))
        smooth(arm)
        arm.data.materials.append(mats["body_plain"])
        objs.append(arm)

        pts = [w["wrist"], w["elbow"], w["root"]] + wing_edge(w, style)
        mem = flat_fan(f"membrane_{side}", pts, F["wing"]["thickness"])
        mem.data.materials.append(mats["membrane"])
        objs.append(mem)
    return objs


# ------------------------------------------------------------------------------ assembly
def make_materials(b):
    return {
        "body": toon_material("body", b["base"], accent=b["accent"]),
        "body_plain": toon_material("body_plain", b["base"]),
        "accent_flat": toon_material("accent_flat", b["accent"]),
        "membrane": toon_material("membrane", tuple(0.62 * g for g in b["glow"]), emission=1.0) if STYLE == "v3" else
        toon_material("membrane", tuple(0.55 * c + 0.45 * a for c, a in zip(b["base"], b["accent"]))),
        "horn": toon_material("horn", b["horn"]),
        "iris": toon_material("iris", b["eye"], emission=1.3) if STYLE == "v3" else toon_material("iris", b["eye"]),
        "pupil": toon_material("pupil", (0.06, 0.03, 0.05)),
        "glint": toon_material("glint", (1, 1, 1), emission=2.0),
        "heart": toon_material("heart", b["glow"], emission=1.4),
        "rune": toon_material("rune", b["glow"], emission=1.2),
        "tooth": toon_material("tooth", (0.97, 0.95, 0.90)),
        "tongue": toon_material("tongue", (0.93, 0.45, 0.55)),
        "mouth": toon_material("mouth", (0.36, 0.11, 0.16)),
    }


# ------------------------------------------------------------------------------ mouth
def mouth_plane():
    """The plane through the mouth line (its front and both corners): a point on it, its up
    normal, and the corners' y (the slit runs in front of them)."""
    m = F["face"]["mouth"]
    a, b, c = Vector(m(1, 0.0)), Vector(m(1, 1.0)), Vector(m(-1, 1.0))
    n = (b - a).cross(c - a).normalized()
    return a, (n if n.z > 0 else -n), b.y


def cut_mouth(body):
    """Slit the snout along the mouth line so the jaw can open: bisect the snout with the
    mouth plane in front of the corners, then rip the cut into two lips. (The body object
    sits at the origin, so its mesh is in rig space.)"""
    co, no, y_corner = mouth_plane()
    md = F["mouth_detail"]
    bm = bmesh.new()
    bm.from_mesh(body.data)
    faces = []
    for f in bm.faces:
        c = f.calc_center_median()
        if c.y < y_corner and abs(c.x) < md["width"] and abs((c - co).dot(no)) < md["depth"]:
            faces.append(f)
    geom = faces + list({e for f in faces for e in f.edges}) + list({v for f in faces for v in f.verts})
    cut = bmesh.ops.bisect_plane(bm, geom=geom, plane_co=co, plane_no=no, dist=1e-6)["geom_cut"]
    bmesh.ops.split_edges(bm, edges=[e for e in cut if isinstance(e, bmesh.types.BMEdge)])
    bm.to_mesh(body.data)
    bm.free()


def lip_chains(body):
    """The slit's two lips, each ordered corner -> front -> corner: (upper, lower) vertex
    indices. The corners, where the lips meet, are in both."""
    co, no, y_corner = mouth_plane()
    me = body.data
    sides = {}
    for p in me.polygons:
        s = (Vector(p.center) - co).dot(no)
        for v in p.vertices:
            sides.setdefault(v, []).append(s)
    upper, lower = [], []
    for v in me.vertices:
        if abs((v.co - co).dot(no)) > 1e-4 or v.co.y > y_corner + 1e-3 or abs(v.co.x) > F["mouth_detail"]["width"]:
            continue
        s = sides.get(v.index, [0.0])
        if max(s) > 0:
            upper.append(v.index)
        if min(s) < 0:
            lower.append(v.index)
    behind = Vector((0, y_corner + 1.0, 0))
    order = lambda i: math.atan2(me.vertices[i].co.x, behind.y - me.vertices[i].co.y)  # noqa: E731
    return sorted(upper, key=order), sorted(lower, key=order)


def chain_point(pts, f):
    """The point a fraction f of the way along a polyline (by length)."""
    lengths = [0.0]
    for a, b in zip(pts, pts[1:]):
        lengths.append(lengths[-1] + (b - a).length)
    t = f * lengths[-1]
    for k in range(len(pts) - 1):
        if lengths[k + 1] >= t:
            u = (t - lengths[k]) / max(1e-6, lengths[k + 1] - lengths[k])
            return pts[k].lerp(pts[k + 1], u)
    return pts[-1].copy()


def mouth_back():
    """The middle of the mouth's back edge (between the corners), on the mouth plane."""
    co, no, y_corner = mouth_plane()
    return Vector((0, y_corner, co.z - no.y * (y_corner - co.y) / no.z))


def weight_jaw(body):
    """The lower lip and chin follow the jaw; behind the corners the throat fades back to
    its own weights, so it stretches as the mouth opens. The dark mouth line (and nostrils)
    stay with the upper snout."""
    co, no, y_corner = mouth_plane()
    md = F["mouth_detail"]
    me = body.data
    jaw = body.vertex_groups.get("jaw") or body.vertex_groups.new(name="jaw")
    below = {}
    for p in me.polygons:
        s = (Vector(p.center) - co).dot(no)
        for v in p.vertices:
            below.setdefault(v, []).append((s, p.material_index))
    for v in me.vertices:
        info = below.get(v.index, [])
        if not info or any(m == 1 for _, m in info):  # the mouth line and nostrils
            continue
        s = (v.co - co).dot(no)
        if abs(s) < 1e-4:
            s = sum(x for x, _ in info) / len(info)  # a lip: which side its faces are on
        if s >= 0 or s < -md["depth"] or abs(v.co.x) > md["width"] or v.co.y > y_corner + md["fade"]:
            continue
        w = 1.0 if v.co.y <= y_corner else 1.0 - (v.co.y - y_corner) / md["fade"]
        # Two bones per vertex (the runtime's limit): the jaw and the strongest of the old.
        old = sorted(((g.group, g.weight) for g in v.groups if g.group != jaw.index), key=lambda g: -g[1])
        for gi, _ in old:
            body.vertex_groups[gi].remove([v.index])
        if old and w < 1.0:
            body.vertex_groups[old[0][0]].add([v.index], 1.0 - w, "REPLACE")
        jaw.add([v.index], w, "REPLACE")


def build_mouth_pocket(body, upper, lower):
    """The inside of the mouth (dark): a roof under the upper lip and a floor on the lower
    one, fanned from the middle of the back edge. Closed, they lie flat on each other inside
    the head; as the jaw opens the floor drops with it. Each rim vertex moves exactly as its
    lip vertex. Material slot 2 is the mouth colour."""
    co, no, _ = mouth_plane()
    me = body.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.verts.ensure_lookup_table()
    deform = bm.verts.layers.deform.verify()
    head, jaw = body.vertex_groups["head"].index, body.vertex_groups["jaw"].index


    def fan(lip, facing):
        rim = []
        for src in lip:
            v = bm.verts.new(src.co)
            for k, x in src[deform].items():
                v[deform][k] = x
            rim.append(v)
        for a, b in zip(rim, rim[1:]):
            f = bm.faces.new((back, a, b))
            f.normal_update()
            if f.normal.dot(facing) < 0:
                f.normal_flip()
            f.material_index = 2
    upper, lower = [bm.verts[i] for i in upper], [bm.verts[i] for i in lower]
    back = bm.verts.new(mouth_back())
    back[deform][head] = back[deform][jaw] = 0.5
    fan(upper, -no)
    fan(lower, no)
    bm.to_mesh(me)
    bm.free()


def build_mouth_parts(body, upper, lower, mats):
    """Little teeth along both lips (two front fangs on the upper lip peek out even with the
    mouth closed) and a tongue on the floor of the mouth. Returns [(object, bone)]."""
    co, no, _ = mouth_plane()
    md = F["mouth_detail"]
    me = body.data
    back = mouth_back()
    front = Vector(F["face"]["mouth"](1, 0.0))
    along = lambda chain, f: chain_point([me.vertices[i].co for i in chain], f)  # noqa: E731

    def tooth(bm, base, down, r, length):
        inward = back - base
        inward = (inward - no * inward.dot(no)).normalized()
        side = inward.cross(no).normalized()
        ring = [bm.verts.new(base + (inward * math.cos(a) + side * math.sin(a)) * r)
                for a in (0.0, 2.0944, 4.1888)]
        tip = bm.verts.new(base + inward * r * 0.4 + down * length)
        for k in range(3):
            f = bm.faces.new((ring[k], ring[(k + 1) % 3], tip))
            f.normal_update()
            if f.normal.dot(f.calc_center_median() - base - down * length * 0.3) < 0:
                f.normal_flip()

    out = []
    for chain, bone, down, fracs, fangs in (
            (upper, "snout", -no, lod([0.14, 0.25, 0.36, 0.64, 0.75, 0.86], [0.2, 0.8]), [0.43, 0.57]),
            (lower, "jaw", no, lod([0.2, 0.32, 0.68, 0.8], [0.3, 0.7]), [])):
        bm = bmesh.new()
        for fr in fracs:
            p = along(chain, fr)
            tooth(bm, p + (back - p).normalized() * md["tooth"][0] * 2.4, down, *md["tooth"])  # hidden when closed
        for fr in fangs:  # right on the lip, so they peek over the lower lip when closed
            tooth(bm, along(chain, fr) + (back - along(chain, fr)).normalized() * md["fang"][0] * 0.6,
                  down, *md["fang"])
        obj = mesh_object(f"teeth_{bone}", bm)
        obj.data.materials.append(mats["tooth"])
        out.append((obj, bone))
    # The tongue: a soft flat oval lying on the floor of the mouth, pointing forward.
    w, l, h = md["tongue"]
    bm = bmesh.new()
    geom = bmesh.ops.create_uvsphere(bm, u_segments=lod(6, 5), v_segments=lod(4, 3), radius=1.0)
    fwd = (front - back)
    fwd = (fwd - no * fwd.dot(no)).normalized()
    side = fwd.cross(no).normalized()
    centre = back.lerp(front, 0.33) + no * h * 0.1  # sunk in the floor: hidden when closed
    for v in geom["verts"]:
        x, y, z = v.co
        v.co = centre + side * (x * w) + fwd * (y * l) + no * (z * h)
    obj = mesh_object("tongue", bm)
    obj.data.materials.append(mats["tongue"])
    out.append((obj, "jaw"))
    return out


def add_face_details(body, lip):
    """Nostrils and a mouth line (R1b: "certainly a nose", "consider a mouth"). They are
    projected onto the finished body and joined into it, so they deform exactly like the
    skin around them. The mouth line runs along the upper lip of the slit (lip: its points,
    corner to corner), covering the seam. Material slot 1 is the dark pupil colour."""
    f = F["face"]
    bpy.context.view_layer.update()
    bvh = BVHTree.FromObject(body, bpy.context.evaluated_depsgraph_get())
    bm = bmesh.new()
    bm.from_mesh(body.data)
    rx, ry, depth = f["nostril_r"]
    for s in (-1, 1):
        loc, nrm, _, _ = bvh.find_nearest(mirror(f["nostril"], s))
        hint = Vector((0, 0, 1)) if abs(nrm.z) < 0.8 else Vector((0, -1, 0))
        add_dome(bm, loc - nrm * depth * 0.6, nrm, hint, rx, ry, depth, lod(8, 4), 1, 1)
    # Mouth: a thin three-sided tube along the jaw, half sunk into the skin.
    r, n = f["mouth_r"], lod(16, 6)
    path = []
    for k in range(n + 1):
        loc = chain_point(lip, k / n)
        _, nrm, _, _ = bvh.find_nearest(loc)
        path.append((loc - nrm * r * 0.35, nrm))
    rings = []
    for k, (p, nrm) in enumerate(path):
        tangent = (path[min(k + 1, n)][0] - path[max(k - 1, 0)][0]).normalized()
        side = tangent.cross(nrm).normalized()
        up = side.cross(tangent).normalized()
        rings.append([bm.verts.new(p + (up * math.cos(a) + side * math.sin(a)) * r)
                      for a in (0.0, 2.0944, 4.1888)])
    for a, b in zip(rings, rings[1:]):
        for j in range(3):
            face = bm.faces.new((a[j], a[(j + 1) % 3], b[(j + 1) % 3], b[j]))
            face.material_index = 1
    for ring in (rings[0], rings[-1]):
        face = bm.faces.new(ring)
        face.material_index = 1
    bm.normal_update()
    for face in bm.faces:  # the tube's faces point away from its centre line
        if face.material_index == 1 and len(face.verts) == 4:
            mid = sum((v.co for v in face.verts), Vector()) / 4
            k = min(range(len(path)), key=lambda i: (path[i][0] - mid).length)
            if face.normal.dot(mid - path[k][0]) < 0:
                face.normal_flip()
    bm.to_mesh(body.data)
    bm.free()
    smooth(body)


def weight_membrane(mem, side):
    """Membrane weights from the wing's own frame: each vertex follows the two struts nearest
    to it (fingers, forearm, upper arm, or the flank), blended by distance. The membrane is
    a sparse fan (the wrist and its outline), and heat weights leaked its outline points onto
    the arm and body bones, so a folded wing left its panels behind its fingers."""
    w = wing_points(side)
    mid, back = w["root"].lerp(w["body"], 0.45), w["root"].lerp(w["body"], 0.8)
    struts = [(f"wing_f{k}_{side}", w["wrist"], w[f"f{k}"]) for k in range(1, 5)]
    struts += [(f"wing_fore_{side}", w["elbow"], w["wrist"]), (f"wing_arm_{side}", w["root"], w["elbow"]),
               ("chest", w["root"], mid), ("belly", mid, back), ("hips", back, w["body"])]  # the flank end sits by the hips

    def gap(p, a, b):
        ab = b - a
        t = max(0.0, min(1.0, (p - a).dot(ab) / ab.length_squared))
        return (p - (a + ab * t)).length

    for vg in list(mem.vertex_groups):
        mem.vertex_groups.remove(vg)
    groups = {name: mem.vertex_groups.new(name=name) for name, _, _ in struts}
    for v in mem.data.vertices:
        p = mem.matrix_world @ v.co
        if (p - w["wrist"]).length < 0.03:  # every finger pivots here
            groups[f"wing_fore_{side}"].add([v.index], 1.0, "REPLACE")
            continue
        (d0, n0), (d1, n1) = sorted((gap(p, a, b), name) for name, a, b in struts)[:2]
        w0 = d1 / (d0 + d1) if d0 + d1 > 1e-6 else 1.0
        groups[n0].add([v.index], w0, "REPLACE")
        if w0 < 1.0:
            groups[n1].add([v.index], 1.0 - w0, "REPLACE")


def weight_wingarm(arm_obj, side):
    """The arm tube's stub (behind the root joint, inside the body) follows the chest, and
    hands over to the upper arm across the joint, so the arm stays rooted in the back: heat
    weights gave the stub to the upper arm, which swung it out of the body when the wing
    folded."""
    w = wing_points(side)
    root, axis = w["root"], (w["elbow"] - w["root"]).normalized()
    r = F["wing"]["radii"]["root"]
    lo, hi = -0.2 * r, 0.5 * r  # the hand-over, along the arm, around the joint
    chest = arm_obj.vertex_groups.get("chest") or arm_obj.vertex_groups.new(name="chest")
    upper = arm_obj.vertex_groups[f"wing_arm_{side}"]
    for v in arm_obj.data.vertices:
        t = (arm_obj.matrix_world @ v.co - root).dot(axis)
        if t >= hi:
            continue
        for g in list(v.groups):
            arm_obj.vertex_groups[g.group].remove([v.index])
        a = max(0.0, min(1.0, (t - lo) / (hi - lo)))
        chest.add([v.index], 1.0 - a, "REPLACE")
        if a > 0.0:
            upper.add([v.index], a, "REPLACE")


def wing_keep(name):
    return name.startswith("wing") or name in WING_DRAW_BODY_BONES


def bind_wing(obj, arm):
    """Binds one wing object (every wing style: the exporter's plumed and sail variants too,
    which had heat weights alone): heat weights, then the membrane's strut weights or the arm's
    chest-rooted stub."""
    bind(obj, arm, wing_keep)
    side = obj.name.split("_")[1][0]
    if obj.name.startswith("membrane"):
        weight_membrane(obj, side)
    else:
        weight_wingarm(obj, side)


def build_dragon(breed, form="grown"):
    use_form(form)
    b = BREEDS[breed]
    mats = make_materials(b)
    body = build_body()
    seat_wings(body)  # before the rig: the wing bones start from the seated root
    body.data.materials.append(mats["body"])
    body.data.materials.append(mats["pupil"])
    body.data.materials.append(mats["mouth"])
    cut_mouth(body)
    upper, lower = lip_chains(body)  # vertex indices: adding the face details keeps them
    add_face_details(body, [body.data.vertices[i].co.copy() for i in upper])
    arm = build_armature()
    bind(body, arm, lambda n: not n.startswith("wing") and n not in ("jaw", "eyes"))  # weight_jaw paints the jaw
    weight_jaw(body)
    mouth_parts = build_mouth_parts(body, upper, lower, mats)
    build_mouth_pocket(body, upper, lower)
    dragon_texture.tag_regions(body)  # dirt regions (D46), from the final weights
    dragon_texture.unwrap(body)       # skin UVs; the texture is baked on demand (bake_skin)

    wings = build_wings(b["wings"], mats)
    for wobj in wings:
        bind_wing(wobj, arm)

    groups = {"eyes": [], "horns": [], "frill": [], "spikes": [], "tail_tip": [], "heart": [], "mouth": [], "runes": []}
    snap = {"eyes": [], "horns": [], "frill": [], "spikes": [], "heart": [], "runes": []}
    d = dict(body=body, arm=arm, wings=wings, groups=groups, snap=snap, breed=b, mats=mats, form=form)
    for e in build_eyes(mats):
        attach(d, "eyes", e, "eyes")
    for h in build_horns(b["horns"], mats):
        attach(d, "horns", h, "head")
    for f in build_frill(b["frill"], mats):
        attach(d, "frill", f, "head")
    for o, bone in build_ridge(RIDGE_OF_FRILL[b["frill"]], mats):
        attach(d, "spikes", o, bone)
    attach(d, "tail_tip", build_tail_tip(b["tail"], mats), "tail4")
    for h in build_heart(mats):
        attach(d, "heart", h, "chest")
    if b.get("pattern") == "runes":  # glowing glyphs (the exporter bakes them for every breed)
        for o, bone in build_runes(mats):
            attach(d, "runes", o, bone)
    for o, bone in mouth_parts:
        attach(d, "mouth", o, bone)
    return d


def attach(d, group, obj, bone):
    parent_to_bone(obj, d["arm"], bone)
    d["groups"][group].append(obj)
    if group in d["snap"]:
        d["snap"][group].append((obj, [obj]))


def rest_pose(d, t):
    """The idle pose: the form's Euler table plus the young-pose lift fading out with growth."""
    pb = d["arm"].pose.bones
    rot = dict(F["base_pose"])
    for name, extra in F["young_pose"].items():
        x, y, z = rot.get(name, (0, 0, 0))
        rot[name] = (x + extra * (1 - t), y, z)
    for b in pb:
        b.rotation_mode = "XYZ"
        x, y, z = rot.get(b.name, (0, 0, 0))
        b.rotation_euler = (math.radians(x), math.radians(y), math.radians(z))


def sit_pose(d, amount=1.0):
    """Sitting: the body tips back on the haunches, hind legs fold forward, front legs stay
    straight. Rotation signs were found empirically (--sit-* args override for tuning)."""
    arm, pb = d["arm"], d["arm"].pose.bones
    tilt = float(arg("--sit-tilt", "-28")) * amount
    arm.rotation_euler.x = math.radians(tilt)
    adj = {"arm_up": float(arg("--sit-armup", "60")), "leg_up": float(arg("--sit-legup", "70")),
           "leg_lo": float(arg("--sit-leglo", "-95")), "foot": float(arg("--sit-foot", "60")),
           "tail1": float(arg("--sit-tail", "28"))}
    for side in ("", "_L", "_R"):
        for key, ang in adj.items():
            name = key + side
            if name in pb:
                b = pb[name]
                b.rotation_mode = "XYZ"
                b.rotation_euler.x += math.radians(ang * amount)
    for n, extra in (("neck1", 30), ("head", 20)):  # the tilt already lifts the head
        pb[n].rotation_euler.x += math.radians(extra * amount)


def apply_t(d, t, build):
    """Bone and part scales for growth t, then seat the parts on the body surface."""
    bones, parts = scales_for_t(t, build)
    pb = d["arm"].pose.bones
    for name, sc in bones.items():
        pb[name].scale = sc
    bpy.context.view_layer.update()
    for key, objs in d["groups"].items():
        s = parts.get(key, 1.0)
        for o in objs:
            if "base_scale" not in o:
                o["base_scale"] = list(o.scale)
            o.scale = Vector(o["base_scale"]) * s
    snap_parts(d)


SNAP_FROM = {"eyes": "head"}  # parts on these bones are seated along rays from this joint


def snap_parts(d):
    """Seat each part on the body surface for the current stage: ray from the bone joint
    toward the part's anchor, place the anchor at the hit (minus a small inset). The
    exporter bakes these per-key offsets."""
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    body = d["body"].evaluated_get(dg)
    bvh = BVHTree.FromObject(body, dg)
    inv_body = body.matrix_world.inverted()
    for key, inset in F["inset"].items():
        for anchor, members in d["snap"].get(key, []):
            c = anchor.constraints[0]
            ray_bone = SNAP_FROM.get(c.subtarget, c.subtarget)
            joint = (d["arm"].matrix_world @ d["arm"].pose.bones[ray_bone].matrix).translation
            for o in members:  # reset last stage's snap offset
                if "snap_off" in o:
                    o.location = Vector(o["base_loc"])
            bpy.context.view_layer.update()
            here = anchor.matrix_world.translation.copy()
            direction = (here - joint).normalized()
            # Outermost hit: compressed growth stages fold some surface inside, so keep
            # casting past each hit.
            origin, ldir, hit = inv_body @ joint, (inv_body.to_3x3() @ direction).normalized(), None
            for _ in range(8):
                h, _, _, _ = bvh.ray_cast(origin, ldir, 10.0)
                if h is None:
                    break
                hit, origin = h, h + ldir * 1e-4
            if hit is None:
                continue
            target = body.matrix_world @ hit - direction * inset
            delta_world = target - here
            m = d["arm"].matrix_world @ d["arm"].pose.bones[c.subtarget].matrix @ c.inverse_matrix
            delta = m.to_3x3().inverted() @ delta_world
            for o in members:
                o["snap_off"] = list(delta)
                o.location = Vector(o["base_loc"]) + delta
    bpy.context.view_layer.update()


def ground(d):
    """Stand the dragon on the floor: shorter legs would otherwise leave it floating."""
    d["arm"].location.z = 0.0
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    ev = d["body"].evaluated_get(dg)
    me = ev.to_mesh()
    low = min((ev.matrix_world @ v.co).z for v in me.vertices)
    ev.to_mesh_clear()
    d["arm"].location.z -= low
    bpy.context.view_layer.update()


def pose_stage(d, t, build, sit=False):
    d["arm"].rotation_euler = (0, 0, 0)
    rest_pose(d, t)
    if sit:
        sit_pose(d)
    apply_t(d, t, build)
    ground(d)


BLINK_SQUASH = 0.9  # shut eyes keep this much less of their height (src/core/den_actor.hpp kBlinkSquash)


def open_jaw(d, degrees):
    """Previews: open the mouth by this many degrees (the sign that lowers the chin)."""
    arm = d["arm"]
    pb = arm.pose.bones["jaw"]
    pb.rotation_mode = "XYZ"
    base = pb.rotation_euler.x
    bpy.context.view_layer.update()
    chin = (arm.matrix_world @ pb.tail).z
    pb.rotation_euler.x = base + math.radians(degrees)
    bpy.context.view_layer.update()
    if (arm.matrix_world @ pb.tail).z > chin:
        pb.rotation_euler.x = base - math.radians(degrees)
        bpy.context.view_layer.update()
    print(f"[model] jaw open {degrees} deg: local X {math.degrees(pb.rotation_euler.x - base):+.0f}")


# ------------------------------------------------------------------------------ scene & render
def setup_scene():
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_EEVEE"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = scene.render.resolution_y = RES
    scene.render.film_transparent = False
    world = bpy.data.worlds.new("world")
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.20, 0.13, 0.25, 1)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 1.0
    scene.view_settings.view_transform = "Standard"
    bpy.ops.object.light_add(type="SUN", rotation=(math.radians(48), math.radians(12), math.radians(-38)))
    bpy.context.object.data.energy = 4.0
    bpy.ops.mesh.primitive_circle_add(vertices=64, radius=10, fill_type="NGON", location=(0, 0.8, 0))
    floor = bpy.context.object
    floor.name = "floor"
    floor.data.materials.append(toon_material("floor", (0.30, 0.20, 0.34)))
    cam_data = bpy.data.cameras.new("cam")
    cam = link(bpy.data.objects.new("cam", cam_data))
    scene.camera = cam
    return scene, cam


def visible_points(ds):
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    pts = []
    for d in ds:
        for o in [d["body"]] + d["wings"] + [p for objs in d["groups"].values() for p in objs]:
            ev = o.evaluated_get(dg)
            me = ev.to_mesh()
            pts += [ev.matrix_world @ v.co for v in me.vertices]
            ev.to_mesh_clear()
    return pts


VIEWS = {"three_quarter": Vector((-0.75, -0.95, 0.38)), "side": Vector((-1, 0, 0.12)),
         "front": Vector((-0.15, -1, 0.2)), "top": Vector((-0.2, 0.3, 1.0)),
         "back_quarter": Vector((-0.8, 0.9, 0.5)), "mouth": Vector((-0.55, -1, -0.05))}


def head_points(d):
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    pts = []
    for o in d["groups"]["eyes"] + d["groups"]["horns"] + d["groups"]["frill"]:
        ev = o.evaluated_get(dg)
        me = ev.to_mesh()
        pts += [ev.matrix_world @ v.co for v in me.vertices]
        ev.to_mesh_clear()
    arm = d["arm"]
    for b in ("head", "snout", "neck3"):
        pb = arm.pose.bones[b]
        pts += [arm.matrix_world @ pb.head, arm.matrix_world @ pb.tail]
    return pts


def frame_camera(cam, ds, view, lens=55, margin=1.55):
    """Frame the visible dragon(s) from a named view direction ('portrait' = head close-up)."""
    if view == "portrait":
        pts, view, margin = head_points(ds[0]), "three_quarter", 1.3
    elif view == "mouth":  # the snout and jaw, from the front and a little below
        arm = ds[0]["arm"]
        pts = [arm.matrix_world @ getattr(arm.pose.bones[b], end) for b in ("snout", "jaw") for end in ("head", "tail")]
        view, margin = "mouth", 2.6
    else:
        pts = visible_points(ds)
    frame_points(cam, pts, view, lens, margin)


def frame_points(cam, pts, view, lens=55, margin=1.55):
    """Frame these points from a named view direction."""
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    center = (lo + hi) / 2
    size = (hi - lo).length
    dvec = VIEWS[view].normalized()
    floor = bpy.data.objects.get("floor")
    if floor:
        floor.location = (center.x, center.y, 0)
        floor.scale = (size * 0.06,) * 3
    cam.data.lens = lens
    cam.location = center + dvec * size * margin
    cam.rotation_euler = (center - cam.location).to_track_quat("-Z", "Y").to_euler()


def render(path):
    scene = bpy.context.scene
    scene.render.filepath = bpy.path.abspath(path)
    bpy.ops.render.render(write_still=True)


def report(d, label):
    def tris(objs):
        return sum(tri_count(o) for o in objs)
    parts = {k: tris(v) for k, v in d["groups"].items()}
    body, wings = tri_count(d["body"]), tris(d["wings"])
    total = body + wings + sum(parts.values())
    print(f"[model] {BREED} {label}: total {total} tris = body {body} + wings {wings} + parts {parts}")


SEX_PART_SCALE = {"male": {"horns": 1.15, "frill": 1.12}, "female": {"tail_tip": 1.15}}  # export_dragon SEX_SCALE


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene, cam = setup_scene()
    views = arg("--views", "three_quarter").split(",")
    if "--lineup" in argv:
        return lineup(cam)
    built = None
    for stage in STAGES:
        form, t = STAGE[stage]
        if built is None or built["form"] != form:
            if built is not None:
                for o in list(bpy.data.objects):
                    if o.name not in ("floor", "cam") and o.type != "LIGHT":
                        bpy.data.objects.remove(o, do_unlink=True)
            built = build_dragon(BREED, form)
        pose_stage(built, t, built["breed"]["build"], sit="--sit" in argv)
        if arg("--jaw"):
            open_jaw(built, float(arg("--jaw")))
        if "--texture" in argv and not built.get("skin_baked"):  # previews: the baked skin (R2)
            rgba = dragon_texture.bake_skin(built["body"], form, 256)
            b = built["breed"]
            dragon_texture.preview_material(built["mats"]["body"], rgba, arg("--pattern", b["pattern"]),
                                            b["pattern_color"], float(arg("--dirt", "0")))
            built["skin_baked"] = True
            if arg("--save-skin"):
                dragon_texture.save_png(rgba, bpy.path.abspath(arg("--save-skin")))
        if arg("--sex"):  # previews: the sexes' part sizes, as the exporter's SEX_SCALE
            for group, k in SEX_PART_SCALE.get(arg("--sex"), {}).items():
                for o in built["groups"][group]:
                    o.scale = o.scale * k
            snap_parts(built)
        if arg("--blink"):  # previews: 0 open .. 1 shut, as the runtime does it
            built["arm"].pose.bones["eyes"].scale[2] *= 1.0 - BLINK_SQUASH * float(arg("--blink"))
            bpy.context.view_layer.update()
        report(built, stage)
        for view in views:
            frame_camera(cam, [built], view)
            render(f"{OUT}_{BREED}_{stage}_{view}.png")
        if arg("--turntable"):  # previews: the dragon turning round, framed once to fit every angle
            n = int(arg("--turntable"))
            arm = built["arm"]
            pts = []
            for k in range(n):
                arm.rotation_euler.z = 2 * math.pi * k / n
                bpy.context.view_layer.update()
                pts += visible_points([built])
            frame_points(cam, pts, "side", lens=55, margin=1.4)
            for k in range(n):
                arm.rotation_euler.z = 2 * math.pi * k / n
                bpy.context.view_layer.update()
                render(f"{OUT}_{BREED}_{stage}_turn{k:02d}.png")
            arm.rotation_euler.z = 0


def lineup(cam):
    """Growth lineup at true relative size: hatch day, late hatchling, juvenile, adolescent,
    adult (side view)."""
    ds, front = [], 0.0
    for stage in ("newborn", "hatchling", "juvenile", "adolescent", "adult"):
        form, t = STAGE[stage]
        d = build_dragon(BREED, form)
        pose_stage(d, t, d["breed"]["build"])
        k = F["export_scale"]
        d["arm"].scale = (k, k, k)
        d["arm"].location.z *= k
        bpy.context.view_layer.update()
        ys = [p.y for p in visible_points([d])]
        d["arm"].location.y += front - 0.3 - max(ys)  # tail just ahead of the previous snout
        bpy.context.view_layer.update()
        front = min(p.y for p in visible_points([d]))
        ds.append(d)
    frame_camera(cam, ds, "side", lens=50, margin=1.45)
    bpy.context.scene.render.resolution_x, bpy.context.scene.render.resolution_y = 1400, 560
    render(f"{OUT}_{BREED}_lineup.png")


if __name__ == "__main__":
    main()
