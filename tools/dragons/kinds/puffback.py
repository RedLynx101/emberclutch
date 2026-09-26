"""The Puffback (Grove, common): a round, gentle giant; a walking hill of moss and flowers.
Concept: docs/art/concept/dragons/dragon_puffback.jpg (R11), reference only.

Design notes
  Baby     a bun: nearly spherical, head and body one soft round shape with a two-leaf sprout
           on top, huge eyes low on the face, a tiny smile, stubby nub legs, tiny leaf-bud
           wings and a short thick tail.
  Adult    the biggest of the first four (META size 1.35): a broad domed back like a hill,
           covered in rounded mossy plates with little flowers and ferns growing between
           them; a big soft pale belly; short stout legs like tree stumps; a short thick neck
           and a big round head with a broad muzzle, big kind eyes under a heavy calm brow
           and a wide smile; two short rounded horns. The wings are small leaf-like fans,
           comically too small for it. A short thick tail ends in a round mossy club with a
           sprig of leaves. Awe from its size and calm, never from anything sharp.
  Motion   slow, heavy and bouncy (plans/puffback.py): a rolling waddle, a trundling run,
           plopping down to sit, flopping on its belly to nap; in the air the little wings
           buzz fast and the body stays level.
  Garden  the back's plates, moss cushions, flowers (on the cushions and on tall stems) and
           ferns are parts on the spine bones; each cushion is joined with what grows on it, so
           it seats on the skin as one piece. A gap in the ridge mid-back is a moss saddle for
           a rider (the plan's SEAT). The garden starts small on the juvenile and grows.
  Wings    small leaf-like fans: a short arm, three finger veins, a rounded lobed membrane
           free of the flank (so it folds as a closed fan flat along the upper flank, under
           the garden). The fold is solved from the grown body's skin (plans/puffback.py);
           the bun turns its buds in a little more (the hatchling's base_pose).
  Variants Moss (natural green, cream belly, darker moss spots, pink flowers), Blossom (a
           surprise: petal pink with green moss patches and pale cherry blossoms), Autumn
           (amber with russet moss and coral flowers), and the rare Moonbloom: a deep
           night-teal with pale mint, golden eyes, glowing moss spots, a crown of big glowing
           moonflowers on tall stems and glowing crystal buds on its back and a moonflower on
           its tail club (its garden and club replace the common ones); the rare baby's sprout
           holds a little glowing moonflower.
  Egg      nearly round, mossy, with pale spots, in each colouring.
"""
import math

from dragons.plans import puffback as plan


def _mirrored(center, sides):
    nodes = dict(center)
    for name, (p, r) in sides.items():
        nodes[f"{name}_R"] = ((p[0], p[1], p[2]), r)
        nodes[f"{name}_L"] = ((-p[0], p[1], p[2]), r)
    return nodes


def _srgb(h):
    """A hex colour as linear RGB (the variants are linear, like the Pouncer's)."""
    def lin(c):
        c /= 255.0
        return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    return tuple(round(lin(int(h[i:i + 2], 16)), 3) for i in (1, 3, 5))


def _hex(h):
    """A hex colour as display (sRGB) 0..1 values: the egg's colours are given this way (the
    egg preview, tools/blender/egg_model.py, treats them as display colours, as the Pouncer's)."""
    return tuple(round(int(h[i:i + 2], 16) / 255.0, 3) for i in (1, 3, 5))


META = dict(
    name="puffback", title="Puffback", dex=2, element="Grove", parents=(), rarity="common",
    plan="puffback", size=1.35,
    stats=dict(wing=3, wit=5, might=7, breath=5, stamina=8),
    manners=("Gentle", "Sleepy", "Greedy", "Shy"),
    traits=("Sturdy", "Hearty Eater", "Sunbather", "Deep Sleeper", "Gentle Giant", "Mossback"),
    rare_variant=3, rare_replaces=True,
    blurb="A walking hill of moss and flowers: slow, strong and kind, and asleep wherever it stops.",
)

# Palette use: base body, accent belly and chin, pattern the moss (skin dapples and the back
# plates), membrane the fresh leaves (wings, ferns, the sprout), horn the horns, tongue the
# flowers' petals (kept pink to coral so it still suits a tongue), glow the heart (and the
# rare one's bloom).
VARIANTS = [
    dict(name="Moss", base=_srgb("#7FB04A"), accent=_srgb("#F1E6A8"), pattern=_srgb("#4E8634"),
         horn=_srgb("#EFE2BE"), membrane=_srgb("#A9D46A"), iris=_srgb("#8C5A2B"), glow=(0.72, 0.95, 0.34),
         tongue=_srgb("#F0939E"), pattern_channel="r", glow_channel=None),
    dict(name="Blossom", base=_srgb("#E7A6B4"), accent=_srgb("#FBEBDD"), pattern=_srgb("#86B25C"),
         horn=_srgb("#F6E4CC"), membrane=_srgb("#BEDC8E"), iris=_srgb("#7A3C4C"), glow=(0.95, 0.85, 0.45),
         tongue=_srgb("#FFD3DB"), pattern_channel="g", glow_channel=None),
    dict(name="Autumn", base=_srgb("#D6974C"), accent=_srgb("#F7E3BA"), pattern=_srgb("#A45530"),
         horn=_srgb("#F3DDB4"), membrane=_srgb("#E4B04E"), iris=_srgb("#5E6B2A"), glow=(1.0, 0.8, 0.35),
         tongue=_srgb("#E9817A"), pattern_channel="g", glow_channel=None),
    dict(name="Moonbloom", base=_srgb("#2E5D63"), accent=_srgb("#CBE9D8"), pattern=_srgb("#2A5A4C"),
         horn=_srgb("#D8F0E6"), membrane=_srgb("#5FA79A"), iris=_srgb("#E8B84A"), glow=(0.55, 1.0, 0.82),
         tongue=_srgb("#D9B8F0"), pattern_channel="g", glow_channel="b"),
]

# Nearly round, mossy, with pale spots.
EGG = dict(height=1.0, width=0.45, asym=0.05, speckle="spots", speckle_params=dict(count=32, size=(0.035, 0.072)),
           colors=[(_hex("#8CBF5A"), _hex("#F4EEC4")), (_hex("#E9A9B8"), _hex("#FFF6EE")),
                   (_hex("#DB9D52"), _hex("#FAEBC8")), (_hex("#2F6468"), _hex("#B8F7DC"))])

# ------------------------------------------------------------------------------ grown
# Faces -Y, Z up. Metaballs: a hill of a body. radius entries are unused (meta body).
GROWN_NODES = _mirrored({
    "tail_tip": ((0, 2.64, 0.86), None),
    "tail4": ((0, 2.40, 0.94), None),
    "tail3": ((0, 2.10, 1.10), None),
    "tail2": ((0, 1.72, 1.36), None),
    "hips": ((0, 0.85, 1.75), None),
    "belly": ((0, 0.15, 1.85), None),
    "chest": ((0, -0.60, 1.90), None),
    "neck1": ((0, -0.98, 2.28), None),
    "neck2": ((0, -1.18, 2.56), None),
    "neck3": ((0, -1.36, 2.80), None),
    "head": ((0, -1.58, 3.00), None),
    "muzzle": ((0, -2.12, 2.86), None),
    "snout": ((0, -2.70, 2.72), None),
}, {
    "shoulder": ((0.86, -0.58, 1.30), None),
    "elbow": ((0.92, -0.62, 0.78), None),
    "wrist": ((0.94, -0.66, 0.26), None),
    "toe_f": ((0.96, -0.96, 0.08), None),
    "hipj": ((0.88, 0.90, 1.30), None),
    "knee": ((0.96, 0.72, 0.74), None),
    "ankle": ((0.98, 0.92, 0.26), None),
    "toe_b": ((1.00, 0.66, 0.08), None),
})

GROWN_META = [
    ("ell", (0, 0.25, 2.02), (1.40, 1.25, 1.20)),     # the hill
    ("ell", (0, -0.62, 1.78), (1.16, 0.82, 1.00)),    # the big soft front
    ("ell", (0, 0.02, 1.26), (1.20, 1.10, 0.62)),     # the belly
    ("ell", (0, 1.15, 1.70), (1.08, 0.60, 0.90)),     # the rump
    ("chain", [(0, -1.00, 2.32), (0, -1.45, 2.84)], [0.68, 0.62]),  # the short thick neck
    ("ell", (0, -1.80, 3.04), (0.80, 0.74, 0.64)),    # the big round head
    ("ell", (0, -2.40, 2.82), (0.52, 0.60, 0.42)),    # the broad rounded muzzle
    ("ell", (0, -2.10, 2.62), (0.50, 0.50, 0.28)),    # chin and jowls
    ("chain", [(0, 1.45, 1.62), (0, 1.80, 1.34), (0, 2.12, 1.10), (0, 2.40, 0.94), (0, 2.56, 0.88)],
     [0.58, 0.47, 0.38, 0.32, 0.29]),               # the short thick tail
]
for _s in (-1, 1):
    GROWN_META += [
        ("ball", (_s * 0.42, -2.00, 2.78), 0.32),                       # cheeks
        ("ell", (_s * 0.43, -2.04, 3.44), (0.22, 0.20, 0.075)),        # a heavy, calm brow
        ("ell", (_s * 0.80, -0.58, 1.35), (0.49, 0.52, 0.60)),         # shoulders
        ("chain", [(_s * 0.90, -0.62, 1.00), (_s * 0.94, -0.64, 0.38)], [0.41, 0.37]),
        ("ell", (_s * 0.94, -0.74, 0.16), (0.41, 0.47, 0.17)),         # front feet
        ("ell", (_s * 0.86, 0.88, 1.35), (0.57, 0.66, 0.72)),          # haunches
        ("chain", [(_s * 0.94, 0.84, 0.90), (_s * 0.97, 0.90, 0.38)], [0.45, 0.38]),
        ("ell", (_s * 0.98, 0.80, 0.16), (0.42, 0.50, 0.17)),          # hind feet
    ]

GROWN = dict(
    name="grown", nodes=GROWN_NODES, meta=GROWN_META, body="meta", voxel=0.04, meta_resolution=0.03,
    # DR2 sizing: grown at the common scale, the Pouncer's bulk and length (the geometric mean of the
    # cube root of the body's volume and its length); META size then sizes it in the game.
    body_tris=1340, body_tris_lod1=450, export_scale=0.64,
    young={
        "bones": {
            "head": (0.8, 0.8, 0.8), "snout": (0.74, 0.7, 0.74),
            "neck1": (0.54, 0.52), "neck2": (0.54, 0.52), "neck3": (0.56, 0.52),
            "chest": (0.52, 0.55), "belly": (0.5, 0.55), "hips": (0.52, 0.55),
            "tail1": (0.52, 0.55), "tail2": (0.54, 0.55), "tail3": (0.58, 0.55), "tail4": (0.62, 0.58),
            "arm_up": (0.56, 0.62), "arm_lo": (0.58, 0.64), "hand": (0.64, 0.66),
            "leg_up": (0.56, 0.62), "leg_lo": (0.58, 0.64), "foot": (0.64, 0.66),
        },
        "parts": {"eyes": 1.4, "horns": 0.4, "frill": 0.7, "wings": 0.58, "spikes": 0.62,
                  "tail_tip": 0.65, "heart": 0.8, "runes": 0.7},
    },
    young_pose={"neck1": -8, "head": 10},
    base_pose={},
    builds={"neutral": {},
            "sturdy": {"chest": (1.07, 0.97), "belly": (1.08, 0.97), "hips": (1.06, 0.98), "leg_up": (1.06, 0.96),
                       "arm_up": (1.06, 0.96), "tail2": (1.05, 0.96)},
            "sleek": {"chest": (0.94, 1.02), "belly": (0.93, 1.02), "hips": (0.95, 1.01), "leg_lo": (0.95, 1.04),
                      "arm_lo": (0.95, 1.04)},
            "long": {"belly": (0.97, 1.07), "neck1": (0.97, 1.08), "neck2": (0.97, 1.08), "tail2": (0.97, 1.08),
                     "tail3": (0.97, 1.08)}},
    eyes=dict(at=(0.44, -2.12, 3.24), out=(0.64, -0.72, 0.2), iris=(0.21, 0.225, 0.095),
              pupil=(0.12, 0.14, 0.036), slit=(0.3, 1.1),
              glints=((-0.05, 0.075, 0.036), (0.04, -0.075, 0.017)), seg=(12, 2, 8, 2)),
    head=dict(origin=(0, -1.80, 3.04), k=1.0, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=False),
    tail_k=1.0,
    heart=dict(at=(0, -1.42, 1.62), size=0.2, tilt=25),
    wing=dict(root=(1.0, -0.48, 2.42), scale=0.76, dihedral=plan.WING_DIHEDRAL, droop=plan.WING_DROOP,
              layout=plan.WING_LAYOUT,
              radii={"root": 0.08, "elbow": 0.062, "wrist": 0.05, "finger": 0.026, "tip": 0.012},
              arm_tris=100, thickness=0.016),
    inset={"eyes": 0.02, "horns": 0.04, "spikes": 0.025, "frill": 0.0, "heart": -0.035, "runes": -0.012,
           "tail_tip": 0.12},
    face=dict(nostril=(0.16, -2.92, 3.02), nostril_r=(0.042, 0.028, 0.014), mouth_r=0.017,
              mouth=lambda side, a: (side * 0.44 * a ** 0.6, -3.02 + 0.97 * a ** 1.3, 2.70 + 0.16 * a ** 2.2)),
    jaw_hinge=(0, -1.98, 2.67),
    mouth_detail=dict(depth=0.3, fade=0.25, width=0.5, tooth=(0.01, 0.018), fang=(0.014, 0.03),
                      tongue=(0.13, 0.2, 0.03)),
    teeth=False,
    skin=dict(stripe=0.5, spot_cell=0.42, dapple=1.1, ao=0.9, face_y=-1.3),
)

# ------------------------------------------------------------------------------ hatchling
HATCH_NODES = _mirrored({
    "tail_tip": ((0, 0.62, 0.16), None),
    "tail4": ((0, 0.55, 0.17), None),
    "tail3": ((0, 0.47, 0.20), None),
    "tail2": ((0, 0.37, 0.25), None),
    "hips": ((0, 0.16, 0.38), None),
    "belly": ((0, 0.00, 0.40), None),
    "chest": ((0, -0.14, 0.44), None),
    "neck1": ((0, -0.17, 0.52), None),
    "neck2": ((0, -0.18, 0.58), None),
    "neck3": ((0, -0.19, 0.64), None),
    "head": ((0, -0.20, 0.70), None),
    "muzzle": ((0, -0.36, 0.60), None),
    "snout": ((0, -0.47, 0.57), None),
}, {
    "shoulder": ((0.25, -0.16, 0.20), None),
    "elbow": ((0.27, -0.18, 0.13), None),
    "wrist": ((0.28, -0.20, 0.06), None),
    "toe_f": ((0.29, -0.30, 0.03), None),
    "hipj": ((0.25, 0.16, 0.20), None),
    "knee": ((0.27, 0.13, 0.13), None),
    "ankle": ((0.28, 0.18, 0.06), None),
    "toe_b": ((0.29, 0.08, 0.03), None),
})

HATCH_META = [
    ("ell", (0, 0.02, 0.36), (0.43, 0.42, 0.34)),     # the bun's round bottom
    ("ell", (0, -0.06, 0.62), (0.37, 0.36, 0.32)),    # its top (the head)
    ("ell", (0, -0.30, 0.55), (0.20, 0.14, 0.13)),    # a soft little muzzle
    ("chain", [(0, 0.30, 0.26), (0, 0.44, 0.20), (0, 0.56, 0.17)], [0.13, 0.10, 0.075]),  # the tail
]
for _s in (-1, 1):
    HATCH_META += [
        ("ball", (_s * 0.20, -0.26, 0.52), 0.12),                     # cheeks
        ("chain", [(_s * 0.27, -0.17, 0.20), (_s * 0.28, -0.21, 0.08)], [0.09, 0.085]),
        ("ell", (_s * 0.27, -0.25, 0.05), (0.10, 0.12, 0.06)),        # front paws
        ("chain", [(_s * 0.27, 0.15, 0.20), (_s * 0.28, 0.17, 0.08)], [0.10, 0.09]),
        ("ell", (_s * 0.27, 0.11, 0.05), (0.10, 0.12, 0.06)),         # hind paws
    ]

HATCH = dict(
    name="hatchling", nodes=HATCH_NODES, meta=HATCH_META, body="meta",
    body_tris=1500, body_tris_lod1=520, export_scale=1.0,
    young={
        "bones": {name: ((0.72, 0.72) if name in ("head", "snout", "neck3") else (0.64, 0.64))
                  for name in ("hips", "belly", "chest", "neck1", "neck2", "neck3", "head", "snout", "tail1",
                               "tail2", "tail3", "tail4", "arm_up", "arm_lo", "hand", "leg_up", "leg_lo", "foot")},
        "parts": {"eyes": 1.1, "horns": 0.7, "frill": 0.9, "wings": 0.6, "spikes": 0.85,
                  "tail_tip": 0.9, "heart": 1.0, "runes": 0.9},
    },
    # The plan's wing fold is made for the grown body; the bun curves in sooner, so the buds
    # turn in a little more (solved from the skin: a 23 degree turn about the wing's root).
    base_pose={"wing_arm_R": (-16.3, 4.3, 14.6), "wing_arm_L": (-16.3, -4.3, -14.6)},
    builds={"neutral": {}, "sturdy": {}, "sleek": {}, "long": {}},
    eyes=dict(at=(0.17, -0.34, 0.66), out=(0.42, -0.9, 0.06), iris=(0.105, 0.12, 0.055),
              pupil=(0.068, 0.085, 0.025), slit=(0.32, 1.1),
              glints=((-0.026, 0.04, 0.024), (0.022, -0.04, 0.011)), seg=(14, 3, 12, 2)),
    head=dict(origin=(0, -0.06, 0.62), k=0.5, horn_len=0.5, horn_r=0.8, horn_curve=0.6, buds=True),
    tail_k=0.3,
    heart=dict(at=(0, -0.42, 0.32), size=0.075, tilt=24),
    wing=dict(root=(0.24, -0.02, 0.62), scale=0.17, dihedral=plan.WING_DIHEDRAL, droop=plan.WING_DROOP,
              layout=plan.WING_LAYOUT,
              radii={"root": 0.03, "elbow": 0.024, "wrist": 0.02, "finger": 0.01, "tip": 0.005},
              arm_tris=80, thickness=0.008),
    inset={"eyes": 0.03, "horns": 0.01, "spikes": 0.012, "frill": 0.0, "heart": -0.016, "runes": -0.01,
           "tail_tip": 0.03},
    face=dict(nostril=(0.035, -0.445, 0.595), nostril_r=(0.012, 0.008, 0.005), mouth_r=0.008,
              mouth=lambda side, a: (side * 0.078 * a ** 0.8, -0.436 + 0.05 * a ** 1.4, 0.522 + 0.036 * a * a)),
    jaw_hinge=(0, -0.30, 0.49),
    mouth_detail=dict(depth=0.12, fade=0.08, width=0.14, tooth=(0.006, 0.01), fang=(0.008, 0.016),
                      tongue=(0.045, 0.06, 0.012)),
    teeth=False,
    skin=dict(stripe=0.2, spot_cell=0.13, dapple=3.6, ao=0.25, face_y=-0.12),
)

FORMS = {"grown": GROWN, "hatchling": HATCH}


# ------------------------------------------------------------------------------ hooks
def _dome(kit, name, out, up, rx, ry, depth, seg, rings, material, mats):
    """A rounded bump (a mossy plate, a mound) facing `out`, its origin at the base centre."""
    import bmesh
    bm = bmesh.new()
    kit.add_dome(bm, (0, 0, 0), out, up, rx, ry, depth, seg, rings, 0)
    obj = kit.mesh_object(name, bm)
    obj.data.materials.append(mats[material])
    kit.smooth(obj)
    return obj


def _flower(kit, name, centre, out, radius, petals, mats, petal="tongue", heart="horn", seg=None):
    """A little flat flower facing `out`: rounded petals round a raised centre."""
    import bmesh
    V = kit.V
    out = V(out).normalized()
    a = V((0, 0, 1)) if abs(out.z) < 0.9 else V((1, 0, 0))
    s = out.cross(a).normalized()
    u = s.cross(out).normalized()
    shape = kit.lod(((0.0, 0.38), (0.3, 0.95), (0.7, 0.95)), ((0.0, 0.45), (0.5, 1.0)))
    pts = [V(centre)]
    for k in range(petals):
        for f, rr in shape:
            t = 2 * math.pi * (k + f) / petals
            pts.append(V(centre) + (s * math.cos(t) + u * math.sin(t)) * radius * rr)
    pts.append(pts[1])
    fl = kit.flat_fan(name, pts, radius * 0.12)
    fl.data.materials.append(mats[petal])
    bm = bmesh.new()
    kit.add_dome(bm, V(centre) + out * radius * 0.05, out, u, radius * 0.34, radius * 0.34, radius * 0.25,
                 kit.lod(6, 4), 1, 0)
    c = kit.mesh_object(name + "_c", bm)
    c.data.materials.append(mats[heart])
    kit.smooth(c)
    return [fl, c]


def _fern(kit, name, base, out, length, width, mats, side_hint=(1, 0, 0)):
    """A little fern frond: a leaf with three pairs of rounded side lobes (a simple blade at LOD1)."""
    V = kit.V
    d = V(out).normalized()
    s = V(side_hint)
    s = (s - d * s.dot(d)).normalized()
    b = V(base)
    if kit.LOD:
        obj = kit.blade(name, b, d, s, length, width, 0.012)
    else:
        left, right = [], []
        for k, (f, w) in enumerate(((0.22, 1.0), (0.46, 0.85), (0.68, 0.62))):
            for pts, sg in ((left, 1), (right, -1)):
                pts += [b + d * (length * (f + 0.1)) + s * (sg * width * w * 0.5), b + d * (length * (f + 0.16))
                        + s * (sg * width * 0.12)]
        tip = b + d * length
        obj = kit.flat_fan(name, [b, b + s * width * 0.1] + left + [tip] + list(reversed(right)) +
                           [b - s * width * 0.1], 0.012)
    obj.data.materials.append(mats["membrane"])
    return obj


# The back: soft rounded plates standing along the spine like mossy stones (bone, anchor, size),
# and moss cushions over the upper flanks (bone, anchor, size, what grows on it, what grows on
# the rare one), a little different on each side, as a garden would be.
RIDGE = [("chest", (0, -0.62, 3.0), 0.74), ("belly", (0, -0.14, 3.22), 0.95),
         ("hips", (0, 0.92, 3.0), 1.02), ("tail1", (0, 1.44, 2.44), 0.78), ("tail2", (0, 1.9, 1.9), 0.52)]
SADDLE = ("belly", (0, 0.36, 3.21), 1.0)  # a gap in the ridge, a soft moss seat for a rider (plan SEAT)
CUSHIONS = [("chest", (0.64, -0.45, 2.8), 0.85, "fern", "moon"),
            ("chest", (-0.64, -0.45, 2.8), 0.85, "flower", "crystals"),
            ("belly", (0.83, 0.15, 2.86), 1.1, "tall", "moon"),
            ("belly", (-0.83, 0.15, 2.86), 1.1, "fern", "moon"),
            ("hips", (0.76, 0.85, 2.62), 1.05, "flower", "crystals"),
            ("hips", (-0.76, 0.85, 2.62), 1.05, "tall", "moon"),
            ("hips", (0.58, 1.35, 2.32), 0.8, "tall", "crystals"),
            ("hips", (-0.62, 1.38, 2.3), 0.75, None, "moon")]


def _joint(kit, bone):
    """A body bone's joint (its head node) in the current form."""
    head = next(h for name, h, t, parent in kit.BONES if name == bone)
    return kit.node(head)


def _crystal(kit, name, base, out, length, mats):
    """A glowing crystal bud: a short faceted point (the rare variant)."""
    o = kit.horn_mesh(name, length, length * 0.3, 0.15, 2, 4, faceted=True, taper=1.0, tip=0.05)
    o.data.materials.append(mats["glow_flat"])
    o.location = kit.V(base)
    o.rotation_euler = kit.V((0, 0, 1)).rotation_difference(kit.V(out).normalized()).to_euler()
    return o


def _stem_flower(kit, name, top, out, side, sz, mats, height, radius, petal, heart, right):
    """A flower on a stem rising from a cushion, leaning back a little and turned outward."""
    V = kit.V
    lean = (out + V((0, 0.25, 0.9))).normalized()
    stem_top = top + lean * height * sz
    stem = kit.tube(f"stem_{name}", [top - out * 0.04, top + lean * height * 0.52 * sz + side * 0.02, stem_top],
                    [0.03, 0.026, 0.022], ring=kit.lod(4, 3))
    stem.data.materials.append(mats["membrane"])
    face = (lean + side * 0.35 * (1 if right else -1) + V((0, -0.35, 0))).normalized()
    return [stem] + _flower(kit, f"flower_{name}", stem_top, face, radius * sz, 5, mats, petal=petal, heart=heart)


def _grow(kit, mats, grows, i, top, out, side, sz, right):
    """What grows on a cushion: a flower, a tall flower, a fern, the rare one's glowing
    moonflower or its glowing crystal buds (or nothing)."""
    V = kit.V
    if grows == "flower":
        return _flower(kit, f"flower_{i}", top, out, 0.2 * sz, 5, mats, petal="tongue", heart="accent_flat")
    if grows == "tall":
        return _stem_flower(kit, i, top, out, side, sz, mats, 0.42, 0.22, "tongue", "accent_flat", right)
    if grows == "moon":
        return _stem_flower(kit, i, top, out, side, sz, mats, 0.6, 0.3, "glow_flat", "horn", right)
    if grows == "fern":
        return [_fern(kit, f"fern_{i}_{j}", top - out * 0.03, (out + side * lean + V((0, 0.2, 0.5))).normalized(),
                      0.62 * sz * (1.0 if j == 0 else 0.72), 0.26 * sz, mats, side_hint=side)
                for j, lean in enumerate((0.0, 0.62)) if not (kit.LOD and j)]
    if grows == "crystals":
        return [_crystal(kit, f"bud_{i}_{j}", top + side * a * 0.12 * sz - out * 0.03,
                         out + side * a * 0.5 + V((0, 0.2, 0)), ln * sz, mats)
                for j, (a, ln) in enumerate(((0.7, 0.34), (-0.6, 0.24))) if not (kit.LOD and j)]
    return []


def _back(kit, mats, rare):
    """The mossy plates, cushions, flowers and ferns on the back: [(object, bone)]. The rare
    one's cushions bloom with big glowing moonflowers on tall stems and glowing crystal buds."""
    V = kit.V
    pieces = []
    for i, (bone, at, sz) in enumerate(RIDGE):
        out = (V(at) - _joint(kit, bone)).normalized()
        o = _dome(kit, f"plate_{i}", out, (0, 1, -0.4), 0.12 * sz, 0.3 * sz, 0.42 * sz, kit.lod(8, 5),
                  2, "pattern_flat", mats)
        o.location = V(at)
        pieces.append((o, bone))
    bone, at, sz = SADDLE
    out = (V(at) - _joint(kit, bone)).normalized()
    o = _dome(kit, "saddle", out, (0, 1, 0), 0.34 * sz, 0.4 * sz, 0.07 * sz, kit.lod(8, 5), kit.lod(2, 1),
              "pattern_flat", mats)
    o.location = V(at)
    pieces.append((o, bone))
    for i, (bone, at, sz, grows, rare_grows) in enumerate(CUSHIONS):
        p = V(at)
        out = (p - _joint(kit, bone)).normalized()
        depth = 0.13 * sz
        parts_ = [_dome(kit, f"cushion_{i}", out, (0, 1, 0), 0.36 * sz, 0.32 * sz, depth, kit.lod(8, 5),
                        kit.lod(2, 1), "pattern_flat", mats)]
        top = out * depth * 0.85
        side = V((0, 1, 0)).cross(out).normalized()
        parts_ += _grow(kit, mats, rare_grows if rare else grows, i, top, out, side, sz, at[0] > 0)
        o = kit.join(parts_, f"cushion_{i}") if len(parts_) > 1 else parts_[0]
        o.location = p
        pieces.append((o, bone))
    return pieces


def _toes(kit, mats, baby):
    """Three round pale toenails at the front of each foot (full detail only): [(object, bone)]."""
    if kit.LOD:
        return []
    V = kit.V
    k = 0.3 if baby else 1.0
    pieces = []
    for s in (-1, 1):
        side = "R" if s > 0 else "L"
        for bone, toe, reach in ((f"hand_{side}", f"toe_f_{side}", -0.2), (f"foot_{side}", f"toe_b_{side}", -0.3)):
            t = kit.node(toe)
            for j in (-1, 0, 1):
                at = t + V((j * 0.2 * k, reach * k, 0.04 * k))
                out = (at - _joint(kit, bone)).normalized()
                o = _dome(kit, f"toe_{side}_{bone}_{j}", out, (0, 0, 1), 0.1 * k, 0.075 * k, 0.07 * k, 6, 1, "horn",
                          mats)
                o.location = at
                pieces.append((o, bone))
    return pieces


def parts(kit, d):
    F, mats, V = kit.F, d["mats"], kit.V
    baby = d["form"] == "hatchling"
    out = []
    # Toenails ride in the frill group (the Puffback has no frill).
    out.append(("frill", 0, _toes(kit, mats, baby)))
    if baby:
        # The sprout: a little stem and two round leaves on top of the head.
        top = V((0, -0.12, 0.9))
        # The rare one's sprout holds a little glowing moonflower between its leaves.
        for variant in (0, 1):
            stem = kit.tube(f"sprout_{variant}", [top, top + V((0, 0.005, 0.06)), top + V((0, 0.02, 0.11))],
                            [0.016, 0.013, 0.011])
            stem.data.materials.append(mats["membrane"])
            leaves = []
            for s in (-1, 1):
                dl = V((s * 0.78, 0.08, 0.62)).normalized()
                lf = kit.blade(f"sprout_leaf_{s}", top + V((0, 0.02, 0.1)), dl, V((-dl.z * s, -0.35, dl.x * s)), 0.18,
                               0.13, 0.01)
                lf.data.materials.append(mats["membrane"])
                leaves.append(lf)
            if variant:
                leaves += _flower(kit, "sprout_bloom", top + V((0, -0.01, 0.15)), (0, -0.55, 1), 0.115, 5, mats,
                                  petal="glow_flat", heart="horn")
            sprout = kit.join([stem] + leaves, f"sprout_{variant}")
            out.append(("horns", variant, [(sprout, "head")]))
        return out
    # Horns: two short, rounded horns on top of the head.
    horns = kit.build_horns([dict(len=0.44, r=0.115, curve=38, seg=4, ring=5, at=(0.25, 0.14, 0.44), rot=(-24, 16, 0))],
                            mats)
    out.append(("horns", 0, [(h, "head") for h in horns]))
    # The back's moss, flowers and ferns; the rare one's bigger, glowing bloom replaces them.
    out.append(("spikes", 0, _back(kit, mats, False)))
    out.append(("spikes", 1, _back(kit, mats, True)))
    # The tail's round mossy club with a sprig of leaves (the rare one's holds a moonflower).
    x, y, z = F["nodes"]["tail_tip"][0]
    for variant in (0, 1):
        club = kit.blob(f"club_{variant}", (0, 0, 0), (0.33, 0.38, 0.31), kit.lod(8, 6), kit.lod(5, 4))
        club.data.materials.append(mats["pattern_flat"])
        sprig = []
        if variant:
            sprig = _flower(kit, "club_bloom", V((0, 0.02, 0.3)), (0, 0.35, 1), 0.2, 5, mats, petal="glow_flat",
                            heart="horn")
        else:
            for s in (-1, 1):
                lf = kit.blade(f"club_leaf_{s}", (0, -0.05, 0.26), (s * 0.55, 0.35, 0.75), (0, 1, 0), 0.34, 0.2, 0.014)
                lf.data.materials.append(mats["membrane"])
                sprig.append(lf)
        tip = kit.join([club] + sprig, f"club_{variant}")
        tip.location = V((0, y + 0.1, z))
        out.append(("tail_tip", variant, [(tip, "tail4")]))
    return out


def wings(kit, d, rare):
    """Small leaf-like wings: a short arm and three finger veins holding a rounded membrane
    with gently lobed edges (a leaf more than a bat's wing)."""
    if rare:
        return []
    F, mats = kit.F, d["mats"]
    w = F["wing"]
    r = w["radii"]
    objs = []
    for side in ("L", "R"):
        p = kit.wing_points(side)
        arm = kit.wing_arm(side, ["root", "elbow", "wrist", "f1", "f2", "f3"],
                           [r["root"], r["elbow"], r["wrist"], r["finger"], r["finger"], r["finger"]],
                           kit.lod(w["arm_tris"], 40), mats, claws=False)
        objs.append(arm)
        wrist = p["wrist"]

        def lobe(a, b, bulge, n):
            pts = []
            for j in range(1, n + 1):
                f = j / (n + 1)
                q = a.lerp(b, f)
                pts.append(q + (q - wrist).normalized() * bulge * (b - a).length * math.sin(math.pi * f))
            return pts

        n = kit.lod(2, 1)
        edge = [p["root"]] + lobe(p["root"], p["f3"], 0.10, n) + [p["f3"]] + lobe(p["f3"], p["f2"], 0.16, n) + \
            [p["f2"]] + lobe(p["f2"], p["f1"], 0.16, n) + [p["f1"]]
        mem = kit.flat_fan(f"membrane_{side}", [wrist, p["elbow"]] + edge, w["thickness"])
        mem.data.materials.append(mats["membrane"])
        objs.append(mem)
    return objs


def accent(kit, body):
    """The big pale belly and chest, the throat, chin and lower muzzle."""
    import mathutils
    mask = kit.mask_attr(body)
    F = kit.F
    baby = F["name"] == "hatchling"
    fwd_down = mathutils.Vector((0, -0.62, -0.78)).normalized()
    for v in body.data.vertices:
        x, y, z = v.co
        a = min(1.0, max(0.0, (v.normal.dot(fwd_down) - 0.1) / 0.4))
        if baby:
            if z > 0.5 or abs(x) > 0.3 or y > 0.15:
                a = 0.0
        else:
            if y > 1.1 or abs(x) > 1.0 or (z > 2.1 and y > -1.9):
                a = 0.0
        mask.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


def texture(tx, nt, p, form):
    """R: round moss spots of two sizes over the back and flanks (not on the face); G: broad
    soft moss patches over the top, like moss on a stone; B: the rare one's glowing spots."""
    face = p["face_y"]  # the spots stop short of the face
    body = tx.band(nt, 1, face, 10.0, soft=p["spot_cell"] * 0.6)
    where = tx.mul(nt, tx.upper(nt), body)
    big = tx.spots(nt, p["spot_cell"] * 1.8, keep=0.5, size=(0.3, 0.2), where=where)
    small = tx.spots(nt, p["spot_cell"], keep=0.55, size=(0.28, 0.18), where=where, seed_offset=1.7)
    r = tx.maxi(nt, big, small)
    g = tx.mul(nt, tx.blotches(nt, p["dapple"], threshold=(0.47, 0.55), where=tx.top(nt)), body)
    b = tx.maxi(nt, tx.spots(nt, p["spot_cell"] * 1.4, keep=0.5, size=(0.3, 0.17), where=where, seed_offset=3.1),
                tx.spots(nt, p["spot_cell"] * 0.6, keep=0.62, size=(0.24, 0.1), where=where, seed_offset=5.3))
    return {"r": r, "g": g, "b": b}
