"""The Bloomstone (Grove and Stone, uncommon; Dragondex 12): a walking rock garden, child of the
Puffback and the Curlstone. Concept: docs/art/concept/dragons/dragon_bloomstone.jpg (reference
only, D74).

Design notes
  * Parents. The Puffback gives it its body plan (the slow trundling hill), its round gentle
    face, the moss and flowers and the tiny leaf wings; the Curlstone gives it stone: a domed
    stone back ringed with rounded plates, a crest of stone spikes down the neck and a stone
    club on its tail. Still a dragon, never a turtle: a short rounded snout, two stubby stone
    horns, the crest, a heartglow, wings and a club tail.
  * The adult. Stout and low like a tortoise, under a high domed shell: the dome is its own
    stone skin, painted with big soft scutes and patches of moss, ringed at its rim by rounded
    stones whose lower edges stand off like a shell's lip (moss under the lip). On top
    grows a tiny garden: one small sapling (a trunk and three round leafy puffs) leaning back
    over the rear of the dome, two mossy boulders, a clutch of pebbles, flowers in little
    clumps and a few leaf rosettes; the middle of the dome stays a soft moss lawn (the rider's
    seat, the plan's SEAT). Stumpy column legs with round flat feet and pale toenails; a short
    thick neck reaching forward; a round head with heavy sleepy lids and a slow, sleepy smile;
    the tail short and thick, ending in a mossy rock club with a sprig of leaves. Tiny leaf
    wings (green stems, leaf membranes) fold low on the dome's front flanks. Awe from its age
    and calm: an old garden that decided to go for a walk. Each garden piece, rim stone and
    crest spike rides the bone that moves the skin under it most (_skin_bone), so it keeps to
    the skin through the clips.
  * The hatchling. A little pebble: one smooth, flattened river stone with big eyes on its
    front, a button snout, tiny stub legs peeking out underneath, a nub of a tail, two tiny
    leaf-bud wings turned out off its round sides, a patch of moss on top and one flower
    growing out of it (the rare baby's flower glows, with a little crystal beside it).
  * Motion is the Puffback's (plans/puffback.py): a rolling waddle, a trundle, plopping down
    to sit, flopping on its belly to nap; it never rolls onto its back (its garden).
  * Colours (petals are the tongue colour, the sapling's trunk the iris colour, the stones
    horn, the moss pattern, leaves membrane): Meadow (grey stone, green moss, pink flowers),
    Desert (sandstone with terracotta strata, blue-green succulent leaves, yellow flowers),
    Ruin (dark stone, ivy, white flowers) and the rare Crystal Garden: pale stone growing
    glowing blue crystals where the boulders were, crystal horns and a crystal on its club,
    glowing veins in the stone and blue flowers.
  * Egg: grey speckled stone with a five-petalled flower (in each colouring's stones).
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
    """A hex colour as linear RGB (the variants are linear)."""
    def lin(c):
        c /= 255.0
        return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    return tuple(round(lin(int(h[i:i + 2], 16)), 3) for i in (1, 3, 5))


def _hex(h):
    """A hex colour as display (sRGB) 0..1 values (the egg's colours)."""
    return tuple(round(int(h[i:i + 2], 16) / 255.0, 3) for i in (1, 3, 5))


META = dict(
    name="bloomstone", title="Bloomstone", dex=12, element=("Grove", "Stone"), parents=("puffback", "curlstone"),
    rarity="uncommon", plan="puffback", size=1.35,
    stats=dict(wing=2, wit=6, might=9, breath=6, stamina=10),
    manners=("Sleepy", "Gentle", "Stubborn", "Greedy"),
    traits=("Mossback", "Sturdy", "Gentle Giant", "Deep Sleeper", "Treasure Hunter", "Ancient Blood"),
    rare_variant=3, rare_replaces=True,
    blurb="A walking rock garden: flowers bloom on its back while it naps in the sun, and it has never once hurried.",
)

# Palette use: base the stone skin, accent the belly, muzzle and toenails, pattern the moss
# (the dome's patches, under the rim stones' lips, the moss cushions, the club's cap; the
# Desert's strata), horn the stones (rim, boulders, pebbles, horns, crest, club), membrane the
# leaves (wings, sapling, rosettes, stems), tongue the petals, iris the eyes and the sapling's
# trunk, glow the heart (and the rare one's crystals and veins).
VARIANTS = [
    dict(name="Meadow", base=_srgb("#8C979C"), accent=_srgb("#EEE4C9"), pattern=_srgb("#6E9E43"),
         horn=_srgb("#C7C1B5"), membrane=_srgb("#93C35B"), iris=_srgb("#7A5230"), glow=(0.78, 0.95, 0.38),
         tongue=_srgb("#F49AB4"), pattern_channel="g", glow_channel=None),
    dict(name="Desert", base=_srgb("#DCC095"), accent=_srgb("#F7EAD0"), pattern=_srgb("#C98E5E"),
         horn=_srgb("#C68E5E"), membrane=_srgb("#7FB592"), iris=_srgb("#5B6E34"), glow=(1.0, 0.8, 0.35),
         tongue=_srgb("#F7CD4A"), pattern_channel="r", glow_channel=None),
    dict(name="Ruin", base=_srgb("#6C6D72"), accent=_srgb("#BDB7AF"), pattern=_srgb("#3F6B3A"),
         horn=_srgb("#55565C"), membrane=_srgb("#5F9B4E"), iris=_srgb("#C99A45"), glow=(0.75, 0.95, 0.55),
         tongue=_srgb("#F6F0F2"), pattern_channel="g", glow_channel=None),
    dict(name="Crystal Garden", base=_srgb("#DAD6D2"), accent=_srgb("#F5F2F8"), pattern=_srgb("#A6D5C4"),
         horn=_srgb("#BFB9C8"), membrane=_srgb("#9CD6C0"), iris=_srgb("#6E58C4"), glow=(0.5, 0.82, 1.0),
         tongue=_srgb("#6F9CF2"), pattern_channel="g", glow_channel="b"),
]

# A grey speckled river stone with a flower on it, in each colouring's stones.
EGG = dict(height=0.96, width=0.43, asym=0.06, speckle="spots",
           speckle_params=dict(count=26, count_lod1=2, size=(0.018, 0.042), flower=(0.8, -82, 0.072)),
           colors=[(_hex("#B5B3AA"), _hex("#6E6D68")), (_hex("#E0C79E"), _hex("#B07A4E")),
                   (_hex("#76777C"), _hex("#4B7A44")), (_hex("#E3E0E8"), _hex("#7FA8E8"))])

# ------------------------------------------------------------------------------ grown
# Faces -Y, Z up, in the Puffback's units (its plan's clips and seat fit): a hill of a body
# under a domed shell. Metaballs; the nodes are the skeleton.
GROWN_NODES = _mirrored({
    "tail_tip": ((0, 2.62, 0.84), None),
    "tail4": ((0, 2.38, 0.92), None),
    "tail3": ((0, 2.08, 1.08), None),
    "tail2": ((0, 1.72, 1.32), None),
    "hips": ((0, 0.85, 1.75), None),
    "belly": ((0, 0.15, 1.85), None),
    "chest": ((0, -0.60, 1.90), None),
    "neck1": ((0, -0.98, 2.16), None),
    "neck2": ((0, -1.22, 2.40), None),
    "neck3": ((0, -1.44, 2.60), None),
    "head": ((0, -1.66, 2.78), None),
    "muzzle": ((0, -2.18, 2.64), None),
    "snout": ((0, -2.70, 2.52), None),
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
    ("ell", (0, 0.26, 1.70), (1.55, 1.50, 1.50)),     # the shell's dome
    ("ell", (0, 0.30, 1.28), (1.74, 1.66, 0.28)),     # its rim, flared into a soft lip
    ("ell", (0, -0.66, 1.30), (0.98, 0.78, 0.72)),    # the chest under the front of the shell
    ("ell", (0, 0.05, 0.98), (1.20, 1.14, 0.52)),     # the belly
    ("ell", (0, 1.14, 1.20), (0.96, 0.58, 0.64)),     # the rump
    ("chain", [(0, -1.00, 1.84), (0, -1.52, 2.52)], [0.56, 0.52]),  # a short thick neck reaching forward
    ("ell", (0, -1.90, 2.84), (0.74, 0.70, 0.60)),    # the round head
    ("ell", (0, -2.40, 2.62), (0.48, 0.52, 0.38)),    # the short rounded snout
    ("ell", (0, -2.10, 2.44), (0.46, 0.46, 0.26)),    # chin and jowls
    ("chain", [(0, 1.45, 1.55), (0, 1.80, 1.30), (0, 2.10, 1.08), (0, 2.38, 0.93), (0, 2.52, 0.86)],
     [0.52, 0.42, 0.34, 0.29, 0.26]),               # the short thick tail
]
for _s in (-1, 1):
    GROWN_META += [
        ("ball", (_s * 0.40, -2.06, 2.58), 0.30),                      # cheeks
        ("ell", (_s * 0.42, -2.26, 3.12), (0.24, 0.18, 0.085)),        # heavy, sleepy lids
        ("ell", (_s * 0.84, -0.60, 1.26), (0.50, 0.54, 0.58)),         # shoulders
        ("chain", [(_s * 0.92, -0.63, 0.98), (_s * 0.95, -0.66, 0.36)], [0.47, 0.45]),  # stumpy columns
        ("ell", (_s * 0.95, -0.72, 0.15), (0.49, 0.53, 0.16)),         # round flat feet
        ("ell", (_s * 0.86, 0.90, 1.28), (0.56, 0.64, 0.68)),          # haunches
        ("chain", [(_s * 0.94, 0.86, 0.90), (_s * 0.97, 0.90, 0.36)], [0.49, 0.46]),
        ("ell", (_s * 0.98, 0.82, 0.15), (0.49, 0.55, 0.16)),          # hind feet
    ]

GROWN = dict(
    name="grown", nodes=GROWN_NODES, meta=GROWN_META, body="meta", voxel=0.04, meta_resolution=0.03,
    body_tris=1200, body_tris_lod1=450, export_scale=0.64,
    young={
        "bones": {
            "head": (0.8, 0.8, 0.8), "snout": (0.74, 0.7, 0.74),
            "neck1": (0.54, 0.52), "neck2": (0.54, 0.52), "neck3": (0.56, 0.52),
            "chest": (0.52, 0.55), "belly": (0.5, 0.55), "hips": (0.52, 0.55),
            "tail1": (0.52, 0.55), "tail2": (0.54, 0.55), "tail3": (0.58, 0.55), "tail4": (0.62, 0.58),
            "arm_up": (0.56, 0.62), "arm_lo": (0.58, 0.64), "hand": (0.64, 0.66),
            "leg_up": (0.56, 0.62), "leg_lo": (0.58, 0.64), "foot": (0.64, 0.66),
        },
        "parts": {"eyes": 1.4, "horns": 0.45, "frill": 0.7, "wings": 0.6, "spikes": 0.6,
                  "tail_tip": 0.65, "heart": 0.8, "runes": 0.6},
    },
    young_pose={"neck1": -8, "head": 10},
    # The tiny leaf wings sit low on the dome's front flanks, turned out a little so the plan's
    # fold lays them along the dome instead of into it (solved by their skin gap, idle and clips).
    base_pose={"wing_arm_R": (0, -20, -20), "wing_arm_L": (0, 20, 20)},
    builds={"neutral": {},
            "sturdy": {"chest": (1.07, 0.97), "belly": (1.08, 0.97), "hips": (1.06, 0.98), "leg_up": (1.06, 0.96),
                       "arm_up": (1.06, 0.96), "tail2": (1.05, 0.96)},
            "sleek": {"chest": (0.94, 1.02), "belly": (0.93, 1.02), "hips": (0.95, 1.01), "leg_lo": (0.95, 1.04),
                      "arm_lo": (0.95, 1.04)},
            "long": {"belly": (0.97, 1.07), "neck1": (0.97, 1.08), "neck2": (0.97, 1.08), "tail2": (0.97, 1.08),
                     "tail3": (0.97, 1.08)}},
    eyes=dict(at=(0.40, -2.34, 2.98), out=(0.62, -0.74, 0.18), iris=(0.2, 0.2, 0.09),
              pupil=(0.12, 0.13, 0.034), slit=(0.3, 1.1),
              glints=((-0.05, 0.07, 0.034), (0.04, -0.07, 0.016)), seg=(12, 2, 8, 2)),
    head=dict(origin=(0, -1.90, 2.84), k=1.0, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=False),
    tail_k=1.0,
    heart=dict(at=(0, -1.38, 1.55), size=0.2, tilt=22),
    wing=dict(root=(1.25, -0.75, 1.95), scale=0.62, dihedral=plan.WING_DIHEDRAL, droop=plan.WING_DROOP,
              layout=plan.WING_LAYOUT,
              radii={"root": 0.075, "elbow": 0.058, "wrist": 0.046, "finger": 0.024, "tip": 0.011},
              arm_tris=56, thickness=0.016),
    # the garden and the crest sit a little deep: rigid on one bone each, they drift a few
    # hundredths from the blended skin in the livelier clips (a bow, a pounce)
    inset={"eyes": 0.02, "horns": 0.04, "spikes": 0.06, "frill": 0.04, "heart": -0.022, "runes": -0.012,
           "tail_tip": 0.12},
    face=dict(nostril=(0.15, -2.86, 2.76), nostril_r=(0.04, 0.027, 0.014), mouth_r=0.017,
              mouth=lambda side, a: (side * 0.40 * a ** 0.6, -2.90 + 0.92 * a ** 1.3, 2.50 + 0.16 * a ** 2.2)),
    jaw_hinge=(0, -1.92, 2.46),
    mouth_detail=dict(depth=0.3, fade=0.25, width=0.46, tooth=(0.01, 0.018), fang=(0.014, 0.03),
                      tongue=(0.12, 0.19, 0.03), fangs=[]),
    teeth=False,
    skin=dict(stripe=0.7, spot_cell=0.4, dapple=0.9, scute=0.55, dome_z=1.4, face_y=-1.25, shell_y=-1.0, crown=0.86,
              moss=(0.47, 0.53), ao=0.9),
)

# ------------------------------------------------------------------------------ hatchling
# A pebble: one smooth flattened river stone, the face on its front, tiny stub legs beneath.
HATCH_NODES = _mirrored({
    "tail_tip": ((0, 0.62, 0.16), None),
    "tail4": ((0, 0.55, 0.17), None),
    "tail3": ((0, 0.48, 0.19), None),
    "tail2": ((0, 0.38, 0.23), None),
    "hips": ((0, 0.16, 0.30), None),
    "belly": ((0, 0.00, 0.32), None),
    "chest": ((0, -0.2, 0.29), None),    # low and forward: the heart's skin follows it, not the chin
    "neck1": ((0, -0.19, 0.39), None),
    "neck2": ((0, -0.18, 0.43), None),
    "neck3": ((0, -0.19, 0.47), None),
    "head": ((0, -0.20, 0.51), None),
    "muzzle": ((0, -0.36, 0.455), None),
    "snout": ((0, -0.47, 0.425), None),
}, {
    "shoulder": ((0.25, -0.17, 0.17), None),
    "elbow": ((0.26, -0.19, 0.11), None),
    "wrist": ((0.27, -0.21, 0.05), None),
    "toe_f": ((0.28, -0.30, 0.03), None),
    "hipj": ((0.25, 0.16, 0.17), None),
    "knee": ((0.26, 0.13, 0.11), None),
    "ankle": ((0.27, 0.18, 0.05), None),
    "toe_b": ((0.28, 0.08, 0.03), None),
})

HATCH_META = [
    ("ell", (0, 0.06, 0.30), (0.46, 0.47, 0.24)),     # the pebble
    ("ell", (0, -0.12, 0.42), (0.34, 0.34, 0.28)),    # its front, a little higher (the face)
    ("ell", (0, -0.34, 0.395), (0.18, 0.12, 0.10)),   # a button snout
    ("chain", [(0, 0.36, 0.22), (0, 0.48, 0.18), (0, 0.58, 0.16)], [0.1, 0.075, 0.06]),  # a nub of a tail
]
for _s in (-1, 1):
    HATCH_META += [
        ("ball", (_s * 0.18, -0.30, 0.365), 0.1),                     # cheeks
        ("chain", [(_s * 0.26, -0.18, 0.14), (_s * 0.27, -0.21, 0.06)], [0.075, 0.07]),
        ("ell", (_s * 0.27, -0.25, 0.04), (0.085, 0.1, 0.045)),       # front paws
        ("chain", [(_s * 0.26, 0.15, 0.14), (_s * 0.27, 0.17, 0.06)], [0.08, 0.075]),
        ("ell", (_s * 0.27, 0.11, 0.04), (0.085, 0.1, 0.045)),        # hind paws
    ]

HATCH = dict(
    name="hatchling", nodes=HATCH_NODES, meta=HATCH_META, body="meta",
    body_tris=1500, body_tris_lod1=520, export_scale=1.0,
    young={
        "bones": {name: ((0.72, 0.72) if name in ("head", "snout", "neck3") else (0.64, 0.64))
                  for name in ("hips", "belly", "chest", "neck1", "neck2", "neck3", "head", "snout", "tail1",
                               "tail2", "tail3", "tail4", "arm_up", "arm_lo", "hand", "leg_up", "leg_lo", "foot")},
        "parts": {"eyes": 1.1, "horns": 0.75, "frill": 0.9, "wings": 0.6, "spikes": 0.85,
                  "tail_tip": 0.9, "heart": 1.0, "runes": 0.9},
    },
    # The baby's leaf buds turned out off its round sides (run 17: wings sank into round bodies).
    base_pose={"wing_arm_R": (-20, 0, -55), "wing_arm_L": (-20, 0, 55)},
    builds={"neutral": {}, "sturdy": {}, "sleek": {}, "long": {}},
    eyes=dict(at=(0.125, -0.43, 0.485), out=(0.3, -0.95, 0.1), iris=(0.1, 0.114, 0.052),
              pupil=(0.066, 0.082, 0.024), slit=(0.32, 1.1),
              glints=((-0.026, 0.04, 0.024), (0.022, -0.04, 0.011)), seg=(14, 3, 12, 2)),
    head=dict(origin=(0, -0.12, 0.42), k=0.5, horn_len=0.5, horn_r=0.8, horn_curve=0.6, buds=True),
    tail_k=0.3,
    heart=dict(at=(0, -0.44, 0.18), size=0.054, tilt=30),
    wing=dict(root=(0.3, 0.0, 0.5), scale=0.13, dihedral=plan.WING_DIHEDRAL, droop=plan.WING_DROOP,
              layout=plan.WING_LAYOUT,
              radii={"root": 0.028, "elbow": 0.022, "wrist": 0.018, "finger": 0.009, "tip": 0.005},
              arm_tris=70, thickness=0.008),
    inset={"eyes": 0.03, "horns": 0.01, "spikes": 0.012, "frill": 0.0, "heart": -0.016, "runes": -0.01,
           "tail_tip": 0.03},
    face=dict(nostril=(0.033, -0.455, 0.42), nostril_r=(0.011, 0.008, 0.005), mouth_r=0.008,
              mouth=lambda side, a: (side * 0.072 * a ** 0.8, -0.45 + 0.05 * a ** 1.4, 0.37 + 0.032 * a * a)),
    jaw_hinge=(0, -0.31, 0.345),
    mouth_detail=dict(depth=0.07, fade=0.08, width=0.13, tooth=(0.006, 0.01), fang=(0.008, 0.016),
                      tongue=(0.042, 0.055, 0.011), fangs=[]),
    teeth=False,
    skin=dict(stripe=0.15, spot_cell=0.07, dapple=4.0, scute=0.0, dome_z=0.5, face_y=-0.3, crown=2.0,
              moss=(0.5, 0.58), cap=((0.0, -0.14, 0.69), 0.22), ao=0.25),
)

FORMS = {"grown": GROWN, "hatchling": HATCH}


# ------------------------------------------------------------------------------ helpers
def _body_bvh(kit, body):
    """The body (rest pose) as a BVH tree, to drape the stones and find the skin."""
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


def _skin_bone(d, point, radius=0.3, avoid=("jaw", "eyes")):
    """The bone that moves the skin round a point most (the body's weights averaged over the
    vertices within radius): a rigid part there rides it, so it keeps to the skin in the clips."""
    body = d["body"]
    names = {g.index: g.name for g in body.vertex_groups}
    total, n = {}, 0
    for v in body.data.vertices:
        if (v.co - point).length > radius:
            continue
        n += 1
        for g in v.groups:
            name = names[g.group]
            if name not in avoid:
                total[name] = total.get(name, 0.0) + g.weight
    if not n:
        return None
    return max(total, key=total.get)


def _joint(kit, bone):
    """A body bone's joint (its head node) in the current form."""
    head = next(h for name, h, t, parent in kit.BONES if name == bone)
    return kit.node(head)


def _shift(obj, offset):
    """Move a mesh's vertices (not its origin)."""
    from mathutils import Matrix
    obj.data.transform(Matrix.Translation(offset))


def _dome(kit, name, out, up, rx, ry, depth, seg, rings, material, mats, at=(0, 0, 0)):
    """A rounded bump (a moss cushion, a pebble) facing `out`, its origin at the base centre."""
    import bmesh
    bm = bmesh.new()
    kit.add_dome(bm, (0, 0, 0), out, up, rx, ry, depth, seg, rings, 0)
    obj = kit.mesh_object(name, bm, at)
    obj.data.materials.append(mats[material])
    kit.smooth(obj)
    return obj


def _stone(kit, name, out, size, material, mats, seed=0, at=(0, 0, 0), res=None):
    """A rounded boulder sitting on the surface facing `out`: a squashed, slightly lumpy
    ball, its base buried (origin at the base centre)."""
    import bmesh
    V = kit.V
    out = V(out).normalized()
    a = V((1, 0, 0)) if abs(out.x) < 0.9 else V((0, 1, 0))
    s = out.cross(a).normalized()
    u = s.cross(out).normalized()
    bm = bmesh.new()
    seg, rings = kit.lod(res or (6, 3), (5, 2))
    rx, ry, rz = size
    layers = []
    for k in range(rings):
        phi = -0.25 * math.pi + 0.75 * math.pi * k / rings
        ring = []
        for j in range(seg):
            t = 2 * math.pi * j / seg
            lump = 1.0 + 0.08 * math.sin(3 * t + seed * 1.7 + k)
            flat = (s * math.cos(t) * rx + u * math.sin(t) * ry) * math.cos(phi) * lump
            ring.append(bm.verts.new(flat + out * rz * (math.sin(phi) + 0.45)))
        layers.append(ring)
    pole = bm.verts.new(out * rz * 1.45)
    for a_, b_ in zip(layers, layers[1:]):
        for j in range(seg):
            bm.faces.new((a_[j], a_[(j + 1) % seg], b_[(j + 1) % seg], b_[j]))
    for j in range(seg):
        bm.faces.new((layers[-1][j], layers[-1][(j + 1) % seg], pole))
    bm.faces.new(list(reversed(layers[0])))
    bm.normal_update()
    for f in bm.faces:
        if f.normal.dot(f.calc_center_median() - out * rz * 0.45) < 0:
            f.normal_flip()
    obj = kit.mesh_object(name, bm, at)
    obj.data.materials.append(mats[material])
    kit.smooth(obj)
    return obj


def _flower(kit, name, centre, out, radius, petals, mats, petal="tongue", heart="accent_flat"):
    """A little flat flower facing `out`: rounded petals round a raised centre."""
    import bmesh
    V = kit.V
    out = V(out).normalized()
    a = V((0, 0, 1)) if abs(out.z) < 0.9 else V((1, 0, 0))
    s = out.cross(a).normalized()
    u = s.cross(out).normalized()
    shape = ((0.0, 0.42), (0.5, 1.0))
    pts = [V(centre)]
    for k in range(petals):
        for f, rr in shape:
            t = 2 * math.pi * (k + f) / petals
            pts.append(V(centre) + (s * math.cos(t) + u * math.sin(t)) * radius * rr)
    pts.append(pts[1])
    fl = kit.flat_fan(name, pts, radius * 0.12)
    fl.data.materials.append(mats[petal])
    bm = bmesh.new()
    kit.add_dome(bm, V(centre) + out * radius * 0.05, out, u, radius * 0.36, radius * 0.36, radius * 0.28,
                 kit.lod(5, 4), 1, 0)
    c = kit.mesh_object(name + "_c", bm)
    c.data.materials.append(mats[heart])
    kit.smooth(c)
    return [fl, c]


def _stem_flower(kit, name, base, lean, height, radius, mats, petal="tongue", heart="accent_flat", face=None):
    """A flower on a short stem rising from `base` along `lean`."""
    V = kit.V
    lean = V(lean).normalized()
    face = V(face).normalized() if face is not None else (lean + V((0, -0.3, 0.2))).normalized()
    if kit.LOD:  # the den's: the flower sits on the moss, no stem
        return _flower(kit, f"flower_{name}", V(base) + lean * height * 0.3, face, radius, 5, mats, petal=petal,
                       heart=heart)
    top = V(base) + lean * height
    side = lean.cross(V((0, 0, 1)) if abs(lean.z) < 0.9 else V((1, 0, 0))).normalized()
    stem = kit.tube(f"stem_{name}", [V(base) - lean * 0.03, V(base) + lean * height * 0.5 + side * 0.015, top],
                    [radius * 0.16, radius * 0.14, radius * 0.12], ring=3)
    stem.data.materials.append(mats["membrane"])
    return [stem] + _flower(kit, f"flower_{name}", top, face, radius, 5, mats, petal=petal, heart=heart)


def _rosette(kit, name, base, out, length, mats, leaves=5, turn=0.0):
    """A little rosette of plump pointed leaves (a succulent, a clover of a plant): leaves
    fanning up and out round `out` from its base."""
    V = kit.V
    out = V(out).normalized()
    a = V((1, 0, 0)) if abs(out.x) < 0.9 else V((0, 1, 0))
    s = out.cross(a).normalized()
    u = s.cross(out).normalized()
    objs = []
    n = kit.lod(leaves, 3)
    for k in range(n):
        t = 2 * math.pi * k / n + turn
        r = s * math.cos(t) + u * math.sin(t)
        d = (out * 0.8 + r * 0.75).normalized()
        o = kit.blade(f"{name}_{k}", V(base) + r * length * 0.05, d, r.cross(out), length, length * 0.5, length * 0.08)
        o.data.materials.append(mats["membrane"])
        objs.append(o)
    return objs


def _crystal(kit, name, base, direction, length, radius, sides=None, tip=0.34):
    """A faceted crystal: a prism with a pointed tip, every face flat (split vertices, so the
    game's vertex normals keep the facets). Its origin is the base (the Curlstone's)."""
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
    return kit.mesh_object(name, bm, V(base))


def _cluster(kit, name, base, out, length, glow, count=3):
    """A cluster of glowing crystals growing out of the stone (the rare Crystal Garden)."""
    V = kit.V
    out = V(out).normalized()
    a = V((1, 0, 0)) if abs(out.x) < 0.9 else V((0, 1, 0))
    s = out.cross(a).normalized()
    u = s.cross(out).normalized()
    objs = []
    spec = [(0.0, 0.0, 1.0), (0.5, 0.2, 0.72), (-0.45, 2.1, 0.62), (0.4, 4.0, 0.5)][:kit.lod(count, 2)]
    for k, (tilt, ang, f) in enumerate(spec):
        r = s * math.cos(ang) + u * math.sin(ang)
        d = (out + r * tilt).normalized()
        o = _crystal(kit, f"{name}_{k}", V(base) + r * abs(tilt) * length * 0.25 - out * length * 0.1, d,
                     length * f, length * 0.22 * (0.8 + 0.2 * f))
        o.data.materials.append(glow)
        objs.append(o)
    return objs


def _lit_glow(kit, d, k=0.3):
    """A preview material for glowing crystals as the game draws them (lit colour plus the
    glow on top), so the facets read in the review; named "glow_flat.001" (the exporter reads
    the name before the dot). The Curlstone's."""
    c = kit.variant_colors(d["variant"])["glow"]
    mat = kit.toon_material("glow_flat", c)
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


def _plate(kit, bvh, name, centre, normal, back, size, lift, domed=True, seed=0):
    """A rounded stone draped over the skin (the Curlstone's plate, made lumpier): broad and
    round, domed on top, its upper edge on the skin and its free edge (toward `back`) lifted by
    `lift`; the underside in the second material (moss under the lip). Origin: the skin point
    under its middle."""
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
        q = c + x * px + y * py
        hit, _, _, _ = bvh.ray_cast(q + z * 0.6, -z, 1.4)
        base = hit if hit is not None and (hit - q).length < 0.5 else q
        return base + z * up

    def lift_at(py):
        t = min(1.0, max(0.0, (py / (ln * 0.5) + 0.15) / 1.15))
        return lift * t ** 1.4

    def outline(a, k):
        lump = 1.0 + 0.1 * math.sin(2 * a + seed * 0.37) + 0.06 * math.sin(3 * a + seed)
        sx = math.sin(a) * w * 0.5 * (1.0 - 0.12 * max(0.0, -math.cos(a))) * k * lump
        sy = (-math.cos(a) * ln * 0.5 + ln * 0.04) * k * lump
        return sx, sy

    bm = bmesh.new()
    angles = [2 * math.pi * (i + (0.0 if n == 4 else 0.5)) / n for i in range(n)]
    rim_top, rim_bot, mid_top = [], [], []
    for a in angles:
        sx, sy = outline(a, 1.0)
        p = drape(sx, sy, lift_at(sy) - 0.006) - c
        rim_top.append(bm.verts.new(p))
        rim_bot.append(bm.verts.new(p))
        if domed:
            mx, my = outline(a, 0.78)
            q = drape(mx, my, h * 0.6 + lift_at(my)) - c
            mid_top.append(bm.verts.new(q))
    apex = bm.verts.new(drape(0.0, ln * 0.04, h + lift_at(ln * 0.04)) - c)
    bottom = bm.verts.new(drape(0.0, 0.0, -h * 0.4) - c)
    inner = mid_top if domed else rim_top
    for k in range(n):
        j = (k + 1) % n
        bm.faces.new((apex, inner[k], inner[j])).material_index = 0
        if domed:
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


# ------------------------------------------------------------------------------ the garden
# The shell's rim: rounded stones all round the dome's lower edge (azimuth from the front,
# degrees, + to its right), their free lower edges lifted like a shell's lip.
RIM = dict(centre=(0.0, 0.3, 1.3), rise=0.1, lift=0.07,
           stones=((38, 0.64, 0.0), (112, 0.76, 0.02), (140, 0.72, -0.01), (168, 0.66, 0.02),
                   (-38, 0.66, 0.02), (-112, 0.72, -0.01), (-140, 0.76, 0.02), (-168, 0.64, 0.0)))
# The garden on top: (what, bone, azimuth, elevation from the dome's centre, size, extra):
GARDEN = [
    ("sapling", "hips", 186, 48, 1.0, None),
    ("boulder", "belly", 70, 40, 1.0, 1),
    ("boulder", "hips", -130, 30, 0.85, 2),
    ("pebbles", "chest", -42, 38, 1.0, 3),
    ("flowers", "chest", 30, 48, 1.0, 2),
    ("flowers", "hips", 140, 40, 0.9, 2),
    ("flowers", "belly", -86, 40, 0.9, 2),
    ("rosette", "belly", 110, 56, 1.0, 4),
    ("rosette", "belly", -24, 62, 0.85, 3),
]
DOME = (0.0, 0.26, 1.75)   # where the garden's rays start


def _dome_point(kit, bvh, azimuth, elevation, centre=None):
    """Where a ray from inside the dome (azimuth from the front, + right; elevation up from
    level) leaves the skin: (point, outward normal)."""
    V = kit.V
    c = V(centre or DOME)
    a, e = math.radians(azimuth), math.radians(elevation)
    d = V((math.sin(a) * math.cos(e), -math.cos(a) * math.cos(e), math.sin(e)))
    hit, nrm = _outer_hit(bvh, c, d)
    if hit is None:
        return None, None
    return hit, (V(nrm).normalized() * 0.6 + d * 0.4).normalized()


def _rim_stones(kit, d, bvh, mats):
    """The shell's rim of stones: [(object, bone)]."""
    V = kit.V
    r = RIM
    c = V(r["centre"])
    out = []
    for a, w, dz in r["stones"]:
        t = math.radians(a)
        dirn = V((math.sin(t), -math.cos(t), r["rise"] + dz)).normalized()
        hit, nrm = _outer_hit(bvh, c, dirn)
        if hit is None:
            continue
        normal = (V(nrm).normalized() + dirn).normalized()
        o = _plate(kit, bvh, f"rim_{a}", hit, normal, (0, 0, -1), (w, w * 0.8, 0.14), r["lift"], seed=a)
        o.data.materials.append(mats["horn"])
        o.data.materials.append(mats["pattern_flat"])
        out.append((o, _skin_bone(d, hit) or "belly"))
    return out


def _sapling(kit, name, base, out, sz, mats, rare):
    """One small tree, like a bonsai: a stout trunk leaning back with a bend and three round
    leafy puffs dotted with blossoms (the rare one's blossoms glow)."""
    V = kit.V
    out = V(out).normalized()
    lean = (out + V((0, 0.45, 0.7))).normalized()
    side = lean.cross(V((1, 0, 0))).normalized()
    b = V(base)
    h = 0.56 * sz
    pts = [b - lean * 0.06, b + lean * h * 0.35 + V((0.05, 0, 0)), b + lean * h * 0.7 - side * 0.06 - V((0.02, 0, 0)),
           b + lean * h]
    trunk = kit.tube(f"{name}_trunk", pts, [0.11 * sz, 0.085 * sz, 0.07 * sz, 0.06 * sz], ring=kit.lod(5, 4))
    trunk.data.materials.append(mats["iris"])
    objs = [trunk]
    top = pts[-1]
    puffs = [(V((0.0, 0.02, 0.14)), (0.38, 0.35, 0.28)), (V((0.28, 0.12, -0.02)), (0.26, 0.25, 0.21)),
             (V((-0.27, -0.06, 0.02)), (0.27, 0.25, 0.22))][:kit.lod(3, 2)]
    for k, (off, rad) in enumerate(puffs):
        p = kit.blob(f"{name}_puff{k}", top + off * sz, tuple(x * sz for x in rad), kit.lod(6, 5), kit.lod(4, 3))
        p.data.materials.append(mats["membrane"])
        objs.append(p)
    if not kit.LOD:  # blossoms on the crown
        for k, (off, face) in enumerate(((V((0.1, -0.27, 0.3)), (0.25, -0.75, 0.6)),
                                         (V((-0.34, -0.2, 0.08)), (-0.7, -0.6, 0.3)))):
            objs += _flower(kit, f"{name}_bloom{k}", top + off * sz, face, 0.12 * sz, 5, mats,
                            petal="glow_flat" if rare else "tongue", heart="accent_flat")
    return objs


def _garden(kit, d, bvh, mats, rare, glow):
    """What grows on the dome: [(object, bone)], each piece joined round its own base (its
    origin, on the skin) so it seats on the skin as one."""
    V = kit.V
    pieces = []
    for i, (what, bone, az, el, sz, extra) in enumerate(GARDEN):
        at, out = _dome_point(kit, bvh, az, el)
        if at is None:
            continue
        side = V((0, 0, 1)).cross(out)
        side = side.normalized() if side.length > 1e-3 else V((1, 0, 0))
        up = out.cross(side).normalized()
        objs = []
        if what == "sapling":
            objs.append(_dome(kit, f"moss_{i}", out, up, 0.36 * sz, 0.32 * sz, 0.12 * sz, kit.lod(7, 5), 1,
                              "pattern_flat", mats))
            objs += _sapling(kit, f"sapling_{i}", out * 0.06, out, sz, mats, rare)
        elif what == "boulder":
            objs.append(_dome(kit, f"moss_{i}", out, up, 0.36 * sz, 0.32 * sz, 0.11 * sz, kit.lod(7, 5), 1,
                              "pattern_flat", mats))
            if rare:
                objs += _cluster(kit, f"gems_{i}", out * 0.02, out, 0.6 * sz, glow, count=3)
            else:
                objs.append(_stone(kit, f"rock_{i}", out, (0.3 * sz, 0.25 * sz, 0.2 * sz), "horn", mats, seed=extra,
                                   at=out * 0.04))
        elif what == "pebbles":
            stones = ((0.0, 0.0, 0.13), (0.2, 0.09, 0.1), (-0.07, 0.2, 0.09))[:kit.lod(extra, 2)]
            for k, (dx, dy, r) in enumerate(stones):
                objs.append(_stone(kit, f"pebble_{i}_{k}", out, (r * sz, r * 0.85 * sz, r * 0.55 * sz),
                                   "accent_flat" if k == 1 else "horn", mats, seed=k + 4,
                                   at=side * dx * sz + up * dy * sz + out * 0.05, res=(5, 2)))
        elif what == "flowers":
            objs.append(_dome(kit, f"moss_{i}", out, up, 0.26 * sz, 0.22 * sz, 0.1 * sz, kit.lod(6, 5), 1,
                              "pattern_flat", mats))
            blooms = ((0.0, 0.02, 0.22, 0.19), (0.19, -0.08, 0.12, 0.15))[:kit.lod(extra, 1)]
            for k, (dx, dy, hgt, r) in enumerate(blooms):
                base = side * dx * sz + up * dy * sz + out * 0.07
                lean = out + side * dx * 1.5 + up * 0.15
                objs += _stem_flower(kit, f"{i}_{k}", base, lean, hgt * sz, r * sz, mats,
                                     face=(out * 0.6 + V((0, -0.5, 0.4)) + side * dx * 2).normalized())
        elif what == "rosette":
            objs += _rosette(kit, f"rosette_{i}", out * 0.05, out, 0.22 * sz, mats, leaves=extra, turn=i)
        o = kit.join(objs, f"garden_{i}") if len(objs) > 1 else objs[0]
        _shift(o, o.location.copy())  # the piece's origin at its base (a join keeps the first object's)
        o.location = at
        pieces.append((o, _skin_bone(d, at) or bone))
    return pieces


# A short crest of rounded stone spikes from the back of the head down the neck (the dragon
# in it): (the bone whose axis casts it up through the skin, t along it, the bone it rides,
# length, radius).
CREST = [("neck3", 0.9, "head", 0.34, 0.1), ("neck3", 0.3, "neck3", 0.27, 0.088), ("neck2", 0.35, "neck2", 0.2, 0.074)]


def _crest(kit, d, bvh, mats):
    """[(object, bone)]: stubby stone spikes on the top of the neck, leaning back."""
    from mathutils import Matrix
    V = kit.V
    out = []
    for i, (bone, t, rides, length, radius) in enumerate(CREST):
        head = next(h for name, h, tl, parent in kit.BONES if name == bone)
        tail = next(tl for name, h, tl, parent in kit.BONES if name == bone)
        a, b = kit.node(head), kit.node(tail)
        axis = (b - a).normalized()
        up = V((0, -axis.z, axis.y))
        up = up if up.z > 0 else -up
        hit, nrm = _outer_hit(bvh, a.lerp(b, t) + V((0.002, 0, 0)), up)
        if hit is None:
            continue
        z = (V(nrm).normalized() * 0.65 + V((0, 0.45, 0.35))).normalized()
        y = V((0, 1, 0)) - z * z.y
        y.normalize()
        x = y.cross(z)
        o = kit.horn_mesh(f"crest_{i}", length, radius, math.radians(32), kit.lod(3, 2), kit.lod(5, 4), taper=0.85,
                          tip=0.3)
        o.matrix_world = Matrix.Translation(hit - z * radius * 0.5) @ Matrix((x, y, z)).transposed().to_4x4()
        o.data.materials.append(mats["horn"])
        out.append((o, rides if rides == "head" else (_skin_bone(d, hit, 0.2) or rides)))
    return out


def _toes(kit, d, mats):
    """Three round pale toenails at the front of each grown foot (full detail only), each on the
    bone that moves the skin there: [(object, bone)]. (The baby's paws are too small: rigid nails
    floated off them when a clip lifted a paw.)"""
    if kit.LOD:
        return []
    V = kit.V
    pieces = []
    for s in (-1, 1):
        side = "R" if s > 0 else "L"
        for bone, toe, reach in ((f"hand_{side}", f"toe_f_{side}", -0.2), (f"foot_{side}", f"toe_b_{side}", -0.3)):
            t = kit.node(toe)
            for j in (-1, 0, 1):
                at = t + V((j * 0.21, reach + 0.03 * abs(j), 0.05))
                out = (at - _joint(kit, bone)).normalized()
                o = _dome(kit, f"toe_{side}_{bone}_{j}", out, (0, 0, 1), 0.11, 0.08, 0.07, 6, 1, "accent_flat", mats)
                o.location = at
                o["snap_first"] = True  # (the hind toes' rays run on into the front feet)
                pieces.append((o, bone))
    return pieces


# ------------------------------------------------------------------------------ hooks
def parts(kit, d):
    F, mats, V = kit.F, d["mats"], kit.V
    baby = d["form"] == "hatchling"
    glow = _lit_glow(kit, d)
    out = []
    if baby:
        # One flower growing out of the moss on top (the rare baby's glows, with a crystal).
        top = V((0.004, -0.15, 0.62))
        for variant in (0, 1):
            stem = kit.tube(f"sprout_{variant}", [top - V((0, 0, 0.02)), top + V((0, 0.005, 0.07)),
                                                  top + V((0, 0.02, 0.13))], [0.015, 0.012, 0.01])
            stem.data.materials.append(mats["membrane"])
            objs = [stem]
            for s in (-1, 1):
                dl = V((s * 0.8, 0.1, 0.5)).normalized()
                lf = kit.blade(f"sprout_leaf_{s}", top + V((0, 0.01, 0.05)), dl, V((-dl.z * s, -0.35, dl.x * s)),
                               0.12, 0.08, 0.008)
                lf.data.materials.append(mats["membrane"])
                objs.append(lf)
            objs += _flower(kit, f"sprout_bloom_{variant}", top + V((0, 0.015, 0.14)), (0, -0.6, 1), 0.075, 5, mats,
                            petal="glow_flat" if variant else "tongue", heart="accent_flat")
            if variant:
                objs += _cluster(kit, "baby_gem", top + V((0.07, 0.04, -0.01)), (0.5, 0.3, 1), 0.1, glow, count=2)
            sprout = kit.join(objs, f"sprout_{variant}")
            # on the head (it moves the skin there), right above its joint so the kit's seating
            # ray goes straight up
            out.append(("horns", variant, [(sprout, "head")]))
        return out
    # Two stubby, rounded stone horns (the rare one's are crystals).
    horns = kit.build_horns([dict(len=0.52, r=0.115, curve=50, seg=4, ring=5, at=(0.25, 0.22, 0.42), rot=(-38, 18, 0))],
                            mats)
    out.append(("horns", 0, [(h, "head") for h in horns]))
    gems = []
    for s in (-1, 1):
        base = kit.head_point((0.26, 0.2, 0.4), s)
        dirn = kit.mirror((0.35, 0.45, 0.85), s)
        for k, (dd, ln, r) in enumerate(((V((0, 0, 0)), 0.44, 0.09), (kit.mirror((0.3, -0.1, -0.2), s), 0.24, 0.06))):
            o = _crystal(kit, f"hgem_{s}_{k}", base + dd * 0.12, (V(dirn) + dd).normalized(), ln, r)
            o.data.materials.append(glow)
            gems.append((o, "head"))
    out.append(("horns", 1, gems))
    # The shell's rim of stones and the garden on top (the rare one's boulders are crystals);
    # the stone crest down the neck (the frill group: everyone's).
    bvh = _body_bvh(kit, d["body"])
    out.append(("frill", 0, _toes(kit, d, mats) + _crest(kit, d, bvh, mats)))
    rim = _rim_stones(kit, d, bvh, mats)
    out.append(("spikes", 0, rim + _garden(kit, d, bvh, mats, False, glow)))
    out.append(("spikes", 1, _rim_stones(kit, d, bvh, mats) + _garden(kit, d, bvh, mats, True, glow)))
    # The tail's mossy rock club, a sprig of leaves on it (the rare one grows a crystal).
    x, y, z = F["nodes"]["tail_tip"][0]
    for variant in (0, 1):
        # (built round its middle: the kit seats a tail tip's origin just inside the tail's end)
        club = _stone(kit, f"club_{variant}", (0, 0.3, 1), (0.36, 0.4, 0.34), "horn", mats, seed=7)
        _shift(club, (0, -0.06, -0.2))
        cap = _dome(kit, f"clubmoss_{variant}", (0, 0.25, 1), (0, 1, 0), 0.3, 0.32, 0.1, kit.lod(8, 5), 1,
                    "pattern_flat", mats, at=V((0, 0.02, 0.2)))
        objs = [club, cap]
        if variant:
            objs += _cluster(kit, "club_gem", V((0, 0.05, 0.25)), (0, 0.4, 1), 0.34, glow, count=3)
        else:
            for s in (-1, 1):
                lf = kit.blade(f"club_leaf_{s}", (0, 0.0, 0.26), (s * 0.55, 0.3, 0.8), (0, 1, 0), 0.3, 0.17, 0.014)
                lf.data.materials.append(mats["membrane"])
                objs.append(lf)
        tip = kit.join(objs, f"club_{variant}")
        tip.location = V((0, y + 0.08, z))
        out.append(("tail_tip", variant, [(tip, "tail4")]))
    return out


def wings(kit, d, rare):
    """Tiny leaf wings (the Puffback's): a short arm and three finger veins holding a rounded
    membrane with gently lobed edges."""
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
                           kit.lod(w["arm_tris"], 30), mats, material="membrane", claws=False)
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
    """A pale belly, throat, chin and lower muzzle (the dome and the legs stay stone)."""
    import mathutils
    mask = kit.mask_attr(body)
    F = kit.F
    baby = F["name"] == "hatchling"
    fwd_down = mathutils.Vector((0, -0.62, -0.78)).normalized()
    for v in body.data.vertices:
        x, y, z = v.co
        a = min(1.0, max(0.0, (v.normal.dot(fwd_down) - 0.1) / 0.4))
        if baby:
            if z > 0.44 or abs(x) > 0.28 or y > 0.12:
                a = 0.0
        else:
            if y > 1.1 or abs(x) > 1.0 or (z > 2.1 and y > -1.9) or (z < 1.0 and abs(x) > 0.62):
                a = 0.0
        mask.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


def texture(tx, nt, p, form):
    """G: moss: soft patches over the dome (thickest on top), a little cap on the crown of the
    head and a few dapples down the flanks (the baby: a patch round its flower); R: sandstone
    strata (the Desert's), soft level bands; B: the rare one's glowing crystal veins over the
    dome and the flanks. Value: the dome's big soft scutes (painted seams), a faint grain and
    light from above."""
    face = p["face_y"]
    z, y, nz = tx.axis(nt, 2), tx.axis(nt, 1), tx.normal_z(nt)
    behind_face = tx.smoothstep(nt, y, face - 0.05, face + 0.15)
    shell_y = p.get("shell_y", face)
    on_shell = tx.smoothstep(nt, y, shell_y - 0.1, shell_y + 0.1)
    dome = tx.mul(nt, tx.mul(nt, tx.smoothstep(nt, z, p["dome_z"] - 0.08, p["dome_z"] + 0.2), tx.top(nt)), on_shell)
    patches = tx.blotches(nt, p["dapple"], threshold=p["moss"], where=tx.const(nt, 1.0))
    if "cap" in p:  # the baby: moss only round its flower
        (cx, cy, cz), rad = p["cap"]
        g = tx.maxi(nt, tx.near(nt, (cx, cy, cz), rad * 0.7, soft=0.25),
                    tx.mul(nt, patches, tx.near(nt, (cx, cy, cz), rad * 1.1, soft=0.4)))
        g = tx.maxi(nt, g, tx.spots(nt, p["spot_cell"], keep=0.66, size=(0.24, 0.15),
                                    where=tx.mul(nt, tx.top(nt), behind_face)))  # lichen dots
    else:
        thick = tx.smoothstep(nt, nz, 0.8, 0.98)  # the very top is a moss lawn
        g = tx.mul(nt, tx.maxi(nt, patches, thick), dome)
        crown = tx.mul(nt, tx.smoothstep(nt, y, face + 0.05, face - 0.1),
                       tx.smoothstep(nt, nz, p["crown"], p["crown"] + 0.12))
        g = tx.maxi(nt, g, crown)
        flank = tx.mul(nt, tx.upper(nt), tx.mul(nt, behind_face,
                                                tx.smoothstep(nt, z, p["dome_z"] + 0.1, p["dome_z"] - 0.1)))
        g = tx.maxi(nt, g, tx.spots(nt, p["spot_cell"], keep=0.62, size=(0.24, 0.14), where=flank))
    shell = dome if "cap" not in p else tx.mul(nt, tx.top(nt), behind_face)
    r = tx.mul(nt, tx.stripes(nt, p["stripe"], direction="Z", distortion=0.7, width=(0.72, 0.8), where=shell), 0.7)
    gems = tx.spots(nt, p["spot_cell"] * 0.55, keep=0.5, size=(0.2, 0.12), where=shell, seed_offset=2.3)
    b = tx.maxi(nt, gems, tx.mul(nt, tx.cracks(nt, max(p["scute"], p["spot_cell"] * 2.5) * 1.6, width=0.018,
                                              warp=0.45), shell))
    value = tx.mul(nt, tx.grain(nt, 9.0, 0.06), tx.light_from_above(nt, 0.14))
    if p["scute"]:
        seams = tx.add(nt, tx.mul(nt, tx.cells(nt, p["scute"], edge=0.25), dome), tx.madd(nt, dome, -1.0, 1.0))
        value = tx.mul(nt, seams, value)
    return {"r": r, "g": g, "b": b, "value": value}
