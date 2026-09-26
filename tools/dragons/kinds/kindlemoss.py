"""The Kindlemoss (Ember and Grove, uncommon; Dragondex 10): a crossbreed of the Pouncer and the
Puffback. A cosy moss-lynx. Concept: docs/art/concept/dragons/dragon_kindlemoss.jpg (reference
only, D74).

Design notes
  * Parents. The Pouncer gives it the cat: its body plan, the round cat head with a short soft
    muzzle and tall pointed ears; the Puffback gives it the moss, a heavier, softer build and
    small leaf wings. What is new is the hearth: the moss smoulders. Every fern frond on it
    shades from fresh green at the root through ember orange to a glowing tip, and glowing
    ember berries sit in its moss like coals in a banked fire.
  * The adult. A plump, round-bellied lynx carpeted in moss (the skin is painted as soft moss
    cushions): a short barrel of a body low on thick soft legs, a short thick neck and a big
    round head (the Pouncer's head scaled by HK, its muzzle shortened) with fluffy cheeks (the
    lynx's ruff), big warm eyes, a pink button nose and a small smile. Lynx ear tufts, each a
    sprig of three little leaves; a crest of fern fronds down the back, each lying back along
    the spine with its tip curling up like a flame's lick and glowing like an ember; a few
    glowing ember berries in the moss of its shoulders, flanks and hips; small stubby wings of
    four leaves (the Puffback's leaf wings, here a fan of leaves off a short twig of an arm,
    made for the Pouncer plan's fold, turned out by the base pose so they sit on the plump
    flank); a bushy fern-frond tail (leaflets down both sides) rising right behind the rump and
    curling over like a question mark into a tuft of fronds, with glowing ember spots along
    its top. The skin graph's radii are large and its relax gentle (the skin modifier, subsurf
    and relax shrink a body a lot: the Pouncer's own radii give a lean cat).
  * The hatchling. A fluffy round moss kitten: a big round head, huge eyes, a pink nose, fluffy
    cheeks and bib, soft moss tufts on its back, stubby legs, kitten ears with tiny leaf tufts,
    tiny leaf buds for wings (turned out off its round body), a short bushy tail curling up to
    a little pair of fronds, a glowing berry on each hip, and one sprout on its head whose bud
    glows like an ember.
  * Fronds and leaves are strips whose vertices are painted from the palette row by row (the
    Blazeplume's method: the exporter paints a vertex by its last face, so faces are listed
    tip-first), so the game shades them smoothly root -> tip: membrane (fern green) -> horn
    (ember) -> glow.
  * Pattern: R darker moss patches over the back (Hearth, Ash; the Wildfire's bright moss),
    G scattered golden leaf flecks (Autumn), B many small glowing embers (the Wildfire's glow).
    The painted value carries soft moss cushions (cell edges shaded) on every variant.
  * Variants: Hearth (moss green, ember orange: the natural one), Autumn (russet with gold leaf
    flecks, gold fronds, amber glow: the surprise), Ash (grey-green moss, pale blue embers: the
    subtle one), and the rare Wildfire: brighter moss glowing with embers all over, many more
    ember berries, its fronds and tail tuft longer and tipped in curling flame tongues and its
    leaves ember-tipped (its hatchling: three little flames on its back, more berries and a
    sprout that holds a little flame).
"""
import math


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
    name="kindlemoss", title="Kindlemoss", dex=10, element=("Ember", "Grove"), parents=("pouncer", "puffback"),
    rarity="uncommon", plan="pouncer", size=1.15,
    stats=dict(wing=5, wit=6, might=6, breath=8, stamina=8),
    manners=("Gentle", "Sleepy", "Playful", "Curious"),
    traits=("Warm-Blooded", "Mossback", "Cuddly", "Sunbather", "Hearty Eater", "Glowheart"),
    rare_variant=3, rare_replaces=True,
    blurb="A cosy moss-lynx that smoulders like a banked hearth: it naps in every sunny patch and keeps its friends warm on cold nights.",
)

# Palette use: base = the moss, accent = the belly, bib and muzzle, pattern = the moss patches
# (or leaf flecks), membrane = the fern fronds and leaves, horn = their ember band, glow = the
# ember tips, berries and spots (and the Wildfire's glowing embers).
VARIANTS = [
    dict(name="Hearth", base=_srgb("#6CA03A"), accent=_srgb("#D6E39A"), pattern=_srgb("#467A2B"),
         horn=_srgb("#F2782A"), membrane=_srgb("#8EC34C"), iris=_srgb("#D88A22"), glow=(1.0, 0.5, 0.1),
         pattern_channel="r", glow_channel=None),
    dict(name="Autumn", base=_srgb("#B2582E"), accent=_srgb("#F1D59C"), pattern=_srgb("#E4A83C"),
         horn=_srgb("#D8401C"), membrane=_srgb("#E2A63C"), iris=_srgb("#5E7A28"), glow=(1.0, 0.68, 0.2),
         pattern_channel="g", glow_channel=None),
    dict(name="Ash", base=_srgb("#8B9985"), accent=_srgb("#DADED1"), pattern=_srgb("#64735E"),
         horn=_srgb("#8FC4E8"), membrane=_srgb("#A6B69A"), iris=_srgb("#5B8DB8"), glow=(0.55, 0.82, 1.0),
         pattern_channel="r", glow_channel=None),
    dict(name="Wildfire", base=_srgb("#7CC23C"), accent=_srgb("#EEF1A6"), pattern=_srgb("#B4E158"),
         horn=_srgb("#FF6814"), membrane=_srgb("#9CD851"), iris=_srgb("#FFB43A"), glow=(1.0, 0.58, 0.1),
         pattern_channel="r", glow_channel="b"),
]

# Mossy green with ember speckles (display colours).
EGG = dict(height=1.0, width=0.4, asym=0.1, speckle="spots", speckle_params=dict(count=28, size=(0.022, 0.052)),
           colors=[(_hex("#79A84A"), _hex("#F28A2C")), (_hex("#B55F30"), _hex("#F6C552")),
                   (_hex("#8F9C88"), _hex("#A8D6F4")), (_hex("#86C84A"), _hex("#FFB436"))])

# ------------------------------------------------------------------------------ grown
# Faces -Y, Z up; units ~ metres at adult size (export_scale brings it to the common scale).
# radius = (side, vertical). The Pouncer's frame, plumper: a rounder, deeper body, thicker legs,
# a short thick neck and a bigger round head (the Pouncer's head scaled by HK).
P_HEAD = (0.0, -1.52, 1.95)   # the Pouncer's head
HEAD = (0.0, -1.24, 1.98)
HK = 1.62
SNOUT_Y, SNOUT_K = -1.6, 0.8   # in front of this (the Pouncer's frame) the muzzle is 20% shorter


def _h(x, y, z):
    """A point of the Pouncer's head moved and scaled onto this head, its muzzle shortened (a
    rounder cat's face)."""
    if y < SNOUT_Y:
        y = SNOUT_Y + (y - SNOUT_Y) * SNOUT_K
    return (x * HK, HEAD[1] + (y - P_HEAD[1]) * HK, HEAD[2] + (z - P_HEAD[2]) * HK)


def _p(x, y, z):
    """The inverse of _h: a point of this head in the Pouncer's head frame."""
    y = P_HEAD[1] + (y - HEAD[1]) / HK
    if y < SNOUT_Y:
        y = SNOUT_Y + (y - SNOUT_Y) / SNOUT_K
    return (x / HK, y, P_HEAD[2] + (z - HEAD[2]) / HK)


GROWN_NODES = _mirrored({
    "tail_tip": ((0, 1.66, 2.26), (0.12, 0.12)),
    "tail4": ((0, 1.82, 1.84), (0.17, 0.165)),
    "tail3": ((0, 1.6, 1.36), (0.22, 0.21)),
    "tail2": ((0, 1.16, 1.08), (0.29, 0.28)),
    "hips": ((0, 0.6, 1.02), (0.66, 0.72)),
    "belly": ((0, 0.08, 0.98), (0.74, 0.76)),
    "chest": ((0, -0.42, 1.04), (0.7, 0.78)),
    "neck1": ((0, -0.82, 1.4), (0.52, 0.54)),
    "neck2": ((0, -0.95, 1.6), (0.44, 0.44)),
    "neck3": ((0, -1.06, 1.76), (0.4, 0.38)),
    "head": (HEAD, (0.37 * HK, 0.37 * HK)),
    "muzzle": (_h(0, -1.80, 1.87), (0.2 * HK, 0.165 * HK)),
    "snout": (_h(0, -2.02, 1.81), (0.13 * HK, 0.11 * HK)),
}, {
    "shoulder": ((0.44, -0.4, 0.86), (0.32, 0.36)),
    "elbow": ((0.47, -0.46, 0.46), (0.24, 0.24)),
    "wrist": ((0.47, -0.52, 0.15), (0.19, 0.19)),
    "toe_f": ((0.48, -0.76, 0.085), (0.2, 0.1)),
    "hipj": ((0.44, 0.64, 0.88), (0.4, 0.44)),
    "knee": ((0.49, 0.42, 0.5), (0.27, 0.27)),
    "ankle": ((0.49, 0.74, 0.19), (0.19, 0.19)),
    "toe_b": ((0.5, 0.52, 0.085), (0.2, 0.1)),
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
    "sturdy": {"chest": (1.06, 0.98), "belly": (1.06, 0.98), "hips": (1.05, 1.0), "neck1": (1.05, 0.97),
               "leg_up": (1.06, 0.97), "arm_up": (1.06, 0.97), "tail2": (1.05, 0.97)},
    "sleek": {"chest": (0.95, 1.02), "belly": (0.93, 1.03), "hips": (0.95, 1.0), "leg_lo": (0.95, 1.04),
              "arm_lo": (0.95, 1.04), "tail3": (0.95, 1.04), "tail4": (0.95, 1.04)},
    "long": {"neck1": (0.97, 1.07), "neck2": (0.97, 1.07), "belly": (0.97, 1.07), "tail2": (0.96, 1.08),
             "tail3": (0.96, 1.08), "tail4": (0.96, 1.08)},
}


def _grown_sculpt(kit, obj):
    """A cat's shapes the node graph can't give (the Pouncer's, on this bigger head): a deep
    chest, a soft round belly, fluffy lynx cheeks, a short soft muzzle."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for v in bm.verts:
        x, y, z = v.co
        if -0.9 < y < -0.05 and z < 0.95 and abs(x) < 0.4:  # chest keel
            k = (1 - abs(x) / 0.4) * max(0.0, (0.95 - z) / 0.45)
            v.co.z -= 0.04 * k
        if -0.25 < y < 0.5 and z < 0.7 and abs(x) < 0.44:  # a soft round belly, hanging a little
            k = (1 - abs(x) / 0.44) * math.sin(math.pi * (y + 0.25) / 0.75)
            v.co.z -= 0.035 * k
        px, py, pz = _p(x, y, z)
        if -1.7 < py < -1.3 and 1.74 < pz < 2.06:  # fluffy lynx cheeks (the ruff)
            k = math.sin(math.pi * (py + 1.7) / 0.4) * math.sin(math.pi * (pz - 1.74) / 0.32)
            v.co.x *= 1.0 + 0.16 * k
        if py < -1.71 and pz > 1.89:  # soft short muzzle, a little flatter on top
            t = min(1.0, (-1.71 - py) / 0.3)
            v.co.z -= 0.025 * HK * t
    bm.to_mesh(obj.data)
    bm.free()


GROWN = dict(
    name="grown", nodes=GROWN_NODES, edges=GROWN_EDGES, body="skin", relax=0.45,
    body_tris=1130, body_tris_lod1=510, export_scale=1.15,  # (targets with no sliver faces left black by the bake)
    young={
        "bones": {
            "head": (0.95, 0.9, 1.0), "snout": (0.9, 0.72, 0.95),
            "neck1": (0.76, 0.54), "neck2": (0.78, 0.52), "neck3": (0.8, 0.52),
            "chest": (0.66, 0.54), "belly": (0.64, 0.52), "hips": (0.68, 0.54),
            "tail1": (0.66, 0.52), "tail2": (0.68, 0.5), "tail3": (0.72, 0.5), "tail4": (0.8, 0.54),
            "arm_up": (0.7, 0.6), "arm_lo": (0.72, 0.6), "hand": (0.82, 0.74),
            "leg_up": (0.7, 0.6), "leg_lo": (0.72, 0.58), "foot": (0.82, 0.74),
        },
        "parts": {"eyes": 1.25, "horns": 0.6, "frill": 0.85, "wings": 0.55, "spikes": 0.55,
                  "tail_tip": 0.65, "heart": 0.85, "runes": 0.75},
    },
    young_pose={"neck1": -16, "neck2": -6, "neck3": 4, "head": 16},
    base_pose={"neck1": (-2, 0, 0), "neck2": (4, 0, 0), "neck3": (6, 0, 0), "head": (-6, 0, 0),
               "tail1": (4, 0, 0), "tail2": (2, 0, 4), "tail3": (4, 0, 6), "tail4": (10, 0, 6),
               # the folded leaf wings turned out and up off the plump flank (folded as the Pouncer's, they sink in)
               "wing_arm_R": (-15, 0, -45), "wing_arm_L": (-15, 0, 45)},
    builds=BUILDS,
    eyes=dict(at=_h(0.185, -1.78, 2.03), out=(0.5, -0.85, 0.14), iris=(0.108 * HK, 0.122 * HK, 0.068 * HK),
              pupil=(0.067 * HK, 0.076 * HK, 0.022 * HK), slit=(0.3, 1.1),
              glints=((-0.03, 0.045, 0.022), (0.022, -0.045, 0.011)), seg=(12, 2, 12, 2)),
    head=dict(origin=HEAD, k=HK, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=False, frill_k=1.0, feather_w=1.0),
    tail_k=1.0,
    heart=dict(at=(0.002, -0.98, 1.16), size=0.13),
    wing=dict(root=(0.38, -0.3, 1.62), scale=0.5, dihedral=40, droop=4,
              radii={"root": 0.075, "elbow": 0.06, "wrist": 0.05, "finger": 0.02, "tip": 0.008},
              arm_tris=44, thickness=0.012, style="classic"),
    mask=dict(max_x=0.36, max_z=2.4, min_z=-1.0, tail_cut=(1.05, 1.0)),
    inset={"eyes": 0.0, "horns": 0.03, "spikes": 0.02, "frill": 0.05, "tail_tip": 0.02, "heart": -0.06,
           "runes": 0.022},
    face=dict(nostril=_h(0.03, -2.12, 1.86), nostril_r=(0.022, 0.014, 0.008), mouth_r=0.012,
              mouth=lambda side, a: _h(side * 0.115 * a ** 0.7, -2.07 + 0.23 * a ** 1.5, 1.75 + 0.05 * a * a)),
    jaw_hinge=_h(0, -1.64, 1.79),
    mouth_detail=dict(depth=0.26 * HK, fade=0.2 * HK, width=0.25 * HK, tooth=(0.009, 0.016), fang=(0.013, 0.032),
                      tongue=(0.06, 0.05, 0.014)),
    teeth=False,
    skin=dict(clump=0.22, patch=0.36, dapple=1.3, leaf_cell=0.2, ember_cell=0.2, face_y=-1.0, ao=0.6),
    sculpt=_grown_sculpt,
)

# ------------------------------------------------------------------------------ hatchling
HATCH_NODES = _mirrored({
    "tail_tip": ((0, 1.08, 0.70), None),
    "tail4": ((0, 1.02, 0.54), None),
    "tail3": ((0, 0.88, 0.42), None),
    "tail2": ((0, 0.66, 0.40), None),
    "hips": ((0, 0.32, 0.48), None),
    "belly": ((0, 0.06, 0.50), None),
    "chest": ((0, -0.18, 0.56), None),
    "neck1": ((0, -0.30, 0.68), None),
    "neck2": ((0, -0.38, 0.80), None),
    "neck3": ((0, -0.44, 0.92), None),
    "head": ((0, -0.49, 1.04), None),
    "muzzle": ((0, -0.74, 0.99), None),
    "snout": ((0, -0.90, 0.96), None),
}, {
    "shoulder": ((0.21, -0.16, 0.42), None),
    "elbow": ((0.22, -0.21, 0.25), None),
    "wrist": ((0.23, -0.25, 0.10), None),
    "toe_f": ((0.23, -0.40, 0.05), None),
    "hipj": ((0.21, 0.34, 0.42), None),
    "knee": ((0.23, 0.28, 0.25), None),
    "ankle": ((0.24, 0.38, 0.12), None),
    "toe_b": ((0.24, 0.22, 0.05), None),
})

HATCH_META = [
    ("ell", (0, -0.52, 1.12), (0.37, 0.34, 0.33)),     # big round kitten head
    ("ell", (0, -0.78, 0.98), (0.17, 0.15, 0.12)),     # short button muzzle
    ("ell", (0, -0.74, 0.92), (0.13, 0.12, 0.07)),     # chin
    ("chain", [(0, -0.30, 0.66), (0, -0.40, 0.80), (0, -0.47, 0.92)], [0.19, 0.18, 0.17]),
    ("ell", (0, -0.17, 0.55), (0.31, 0.29, 0.31)),     # chest
    ("ell", (0, 0.06, 0.49), (0.35, 0.32, 0.32)),      # a round fluffy tummy
    ("ell", (0, 0.28, 0.51), (0.29, 0.23, 0.27)),      # hips
    ("ell", (0, 0.0, 0.38), (0.27, 0.28, 0.19)),
    ("ball", (0, -0.1, 0.84), 0.095), ("ball", (0, 0.12, 0.82), 0.1), ("ball", (0, 0.31, 0.77), 0.085),  # moss tufts
    ("chain", [(0, 0.48, 0.46), (0, 0.68, 0.40), (0, 0.86, 0.42), (0, 1.0, 0.52), (0, 1.07, 0.66)],
     [0.12, 0.095, 0.085, 0.075, 0.065]),             # a short bushy tail curling up
    ("ell", (0, -0.31, 0.70), (0.21, 0.15, 0.18)),     # the fluffy bib
    ("ball", (0, -0.34, 0.58), 0.135),
]
for _s in (-1, 1):
    HATCH_META += [
        ("ball", (_s * 0.18, -0.72, 0.96), 0.14),       # fluffy cheeks
        ("ball", (_s * 0.25, -0.62, 1.0), 0.09),        # the cheek fluff
        ("ball", (_s * 0.14, -0.30, 0.62), 0.125),      # bib fluff
        ("ball", (_s * 0.25, -0.08, 0.72), 0.09), ("ball", (_s * 0.28, 0.3, 0.64), 0.09),  # moss on shoulders, hips
        ("ball", (_s * 0.16, 0.02, 0.8), 0.08),
        ("chain", [(_s * 0.21, -0.16, 0.42), (_s * 0.22, -0.21, 0.25), (_s * 0.23, -0.25, 0.10)],
         [0.105, 0.092, 0.088]),
        ("ell", (_s * 0.23, -0.32, 0.06), (0.1, 0.125, 0.062)),    # front paws
        ("ell", (_s * 0.21, 0.32, 0.38), (0.14, 0.17, 0.17)),      # thighs
        ("chain", [(_s * 0.23, 0.37, 0.26), (_s * 0.24, 0.38, 0.12)], [0.095, 0.085]),
        ("ell", (_s * 0.24, 0.28, 0.06), (0.1, 0.135, 0.062)),     # hind paws
    ]

HATCH = dict(
    name="hatchling", nodes=HATCH_NODES, meta=HATCH_META, body="meta",
    body_tris=1500, body_tris_lod1=520, export_scale=1.0,
    young={
        "bones": {name: ((0.74, 0.74) if name in ("head", "snout") else (0.62, 0.62))
                  for name in ("hips", "belly", "chest", "neck1", "neck2", "neck3", "head", "snout", "tail1",
                               "tail2", "tail3", "tail4", "arm_up", "arm_lo", "hand", "leg_up", "leg_lo", "foot")},
        "parts": {"eyes": 1.12, "horns": 0.75, "frill": 0.9, "wings": 0.6, "spikes": 0.85,
                  "tail_tip": 0.85, "heart": 1.0, "runes": 0.9},
    },
    # The baby's leaf buds turned up and back off its round body (run 17: folded as the adult's,
    # a round baby's wings sink into its flank).
    base_pose={"head": (4, 0, 0), "tail1": (8, 0, 0), "tail2": (6, 0, 8), "tail3": (8, 0, 10),
               "tail4": (14, 0, 10), "wing_arm_R": (-20, 0, -55), "wing_arm_L": (-20, 0, 55)},
    builds={"neutral": {}, "sturdy": {}, "sleek": {}, "long": {}},
    eyes=dict(at=(0.17, -0.78, 1.09), out=(0.42, -0.90, 0.07), iris=(0.122, 0.136, 0.067),
              pupil=(0.08, 0.09, 0.03), slit=(0.32, 1.1),
              glints=((-0.028, 0.045, 0.028), (0.024, -0.042, 0.013)), seg=(14, 3, 14, 2)),
    head=dict(origin=(0, -0.50, 1.23), k=0.9, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=True,
              frill_k=0.6, feather_w=1.6),
    tail_k=0.4,
    heart=dict(at=(0.002, -0.44, 0.55), size=0.08),
    wing=dict(root=(0.15, -0.10, 0.78), scale=0.2, dihedral=40, droop=4,
              radii={"root": 0.04, "elbow": 0.032, "wrist": 0.028, "finger": 0.011, "tip": 0.005},
              arm_tris=48, thickness=0.008, style="classic"),
    mask=dict(max_x=0.24, max_z=1.06, min_z=0.13, tail_cut=None),
    inset={"eyes": 0.032, "horns": 0.02, "spikes": 0.012, "frill": 0.03, "tail_tip": 0.015, "heart": -0.03,
           "runes": 0.012},
    face=dict(nostril=(0.03, -0.91, 1.0), nostril_r=(0.02, 0.014, 0.008), mouth_r=0.011,
              mouth=lambda side, a: (side * 0.095 * a ** 0.8, -0.93 + 0.12 * a ** 1.6, 0.925 + 0.025 * a * a)),
    jaw_hinge=(0, -0.70, 0.93),
    mouth_detail=dict(depth=0.15, fade=0.11, width=0.15, tooth=(0.007, 0.012), fang=(0.010, 0.022),
                      tongue=(0.052, 0.08, 0.012)),
    teeth=False,
    skin=dict(clump=0.1, patch=0.15, dapple=4.0, leaf_cell=0.09, ember_cell=0.09, face_y=-0.32, ao=0.25),
)

FORMS = {"grown": GROWN, "hatchling": HATCH}

# ------------------------------------------------------------------------------ painted strips
# Material slots -> the variant colour a vertex shows (the exporter's MATERIAL_PAINT) and its
# glow (emissive / 255).
SLOT_COLOR = {"body_plain": ("base", 0.0), "accent_flat": ("accent", 0.0), "pattern_flat": ("pattern", 0.0),
              "membrane": ("membrane", 0.0), "horn": ("horn", 0.0), "glow_flat": ("glow", 230 / 255)}


def _vc_material(kit, slot):
    """A material named like a palette slot (the exporter paints by the name) whose preview
    colour comes from the per-vertex attributes "vc" (colour) and "ve" (glow), so the review
    renders shade fronds smoothly from root to tip as the game does (the Blazeplume's)."""
    mat = kit.toon_material(slot, (1.0, 1.0, 1.0))
    nt = mat.node_tree
    rgb = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeRGB")
    mul = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeMix" and n.blend_type == "MULTIPLY")
    add = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeMix" and n.blend_type == "ADD")
    em = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeEmission")
    col = nt.nodes.new("ShaderNodeAttribute")
    col.attribute_name = "vc"
    glow = nt.nodes.new("ShaderNodeAttribute")
    glow.attribute_name = "ve"
    nt.links.new(col.outputs["Color"], mul.inputs["A"])
    nt.nodes.remove(rgb)
    lit = nt.nodes.new("ShaderNodeMix")
    lit.data_type, lit.blend_type = "RGBA", "MULTIPLY"
    lit.inputs["Factor"].default_value = 1.0
    nt.links.new(col.outputs["Color"], lit.inputs["A"])
    k = nt.nodes.new("ShaderNodeMath")
    k.operation = "MULTIPLY"
    nt.links.new(glow.outputs["Fac"], k.inputs[0])
    k.inputs[1].default_value = 1.25
    nt.links.new(k.outputs[0], lit.inputs["B"])
    total = nt.nodes.new("ShaderNodeMix")
    total.data_type, total.blend_type = "RGBA", "ADD"
    total.inputs["Factor"].default_value = 1.0
    nt.links.new(add.outputs["Result"], total.inputs["A"])
    nt.links.new(lit.outputs["Result"], total.inputs["B"])
    nt.links.new(total.outputs["Result"], em.inputs["Color"])
    return mat


def _mats(kit, d):
    if "vc_mats" not in d:
        d["vc_mats"] = {slot: _vc_material(kit, slot) for slot in SLOT_COLOR}
    return d["vc_mats"]


def _paint_preview(kit, d, obj):
    """vc / ve attributes: each vertex's colour and glow as the exporter will paint it."""
    me = obj.data
    colors = kit.variant_colors(d["variant"])
    names = [m.name.split(".")[0] for m in me.materials]
    vslot = [names[0]] * len(me.vertices)
    for p in me.polygons:
        for v in p.vertices:
            vslot[v] = names[p.material_index]
    vc = me.color_attributes.new("vc", "FLOAT_COLOR", "POINT")
    ve = me.attributes.new("ve", "FLOAT", "POINT")
    for i, s in enumerate(vslot):
        key, glow = SLOT_COLOR[s]
        vc.data[i].color = (*colors[key], 1.0)
        ve.data[i].value = glow


def _sheet(kit, d, name, verts, faces, origin, thickness, normals=None):
    """A painted sheet: verts (world points), faces [(indices, slot)] listed tip-first (the
    exporter paints a vertex by the LAST face using it, so each row takes the colour of the band
    on its root side). Two sheets `thickness` apart (the game culls back faces). The origin (the
    point seated on the skin) is `origin`."""
    import bmesh
    from mathutils import Vector
    mats = _mats(kit, d)
    slots = list(dict.fromkeys(s for _, s in faces))
    V = [Vector(p) for p in verts]
    if normals is None:
        acc = [Vector((0, 0, 0)) for _ in V]
        for idx, _ in faces:
            a, b, c = V[idx[0]], V[idx[1]], V[idx[2]]
            n = (b - a).cross(c - a)
            for i in idx:
                acc[i] += n
        normals = [n.normalized() if n.length > 1e-9 else Vector((0, 0, 1)) for n in acc]
    else:
        normals = [Vector(n).normalized() for n in normals]
    o = Vector(origin)
    bm = bmesh.new()
    for flip, off in ((1, 0.5), (-1, -0.5)):
        vs = [bm.verts.new(p - o + n * thickness * off) for p, n in zip(V, normals)]
        for idx, slot in faces:
            ids = list(idx) if flip > 0 else list(reversed(idx))
            f = bm.faces.new([vs[i] for i in ids])
            f.material_index = slots.index(slot)
    obj = kit.mesh_object(name, bm, o)
    for s in slots:
        obj.data.materials.append(mats[s])
    kit.smooth(obj)
    _paint_preview(kit, d, obj)
    return obj


# Strip profiles: rows [(t along the length, width factor)] ending in the point (width 0).
# A fern frond: lobed leaflets shrinking toward the tip, the whole shaped like a flame.
FROND = [(0.0, 0.36), (0.12, 0.96), (0.24, 0.6), (0.37, 1.0), (0.5, 0.58), (0.63, 0.86), (0.76, 0.46),
         (0.88, 0.42), (1.0, 0.0)]
FROND_SLOTS = ["membrane"] * 6 + ["horn", "glow_flat"]
# The Wildfire's: the same lobed root, then a long wavy flame tongue.
FLAME_FROND = [(0.0, 0.36), (0.12, 0.96), (0.24, 0.6), (0.36, 0.9), (0.48, 0.56), (0.61, 0.62), (0.74, 0.4),
               (0.87, 0.26), (1.0, 0.0)]
FLAME_SLOTS = ["membrane"] * 3 + ["horn", "horn", "glow_flat", "glow_flat", "glow_flat"]
SPRIG_SLOTS = ["membrane"] * 3 + ["pattern_flat", "pattern_flat"]  # the ear tufts' leaves
LEAF = [(0.0, 0.2), (0.16, 0.74), (0.42, 1.0), (0.7, 0.82), (0.88, 0.44), (1.0, 0.0)]
LOW = [(0.0, 0.4), (0.45, 1.0), (1.0, 0.0)]  # LOD1
FROND_LOW = [(0.0, 0.4), (0.25, 1.0), (0.55, 0.7), (0.8, 0.42), (1.0, 0.0)]  # LOD1 fronds
FROND_LOW_SLOTS = ["membrane", "membrane", "horn", "glow_flat"]


def _strip(base, direction, normal, length, width, rows, slots, curl=0.0, lean=0.0, cup=0.0):
    """One frond or leaf as (verts, faces listed tip-first): a strip from base along direction,
    lying in the plane with this normal. Its centre line turns in that plane toward the side
    (normal x direction) by lean * t + curl * t^2 radians (a flame's lick, a fiddlehead's curl);
    cup bends it out of its plane toward the normal. Fewer rows than slots (LOD1): the root
    band's paint and the tip band's."""
    from mathutils import Vector
    if len(slots) != len(rows) - 1:
        slots = [slots[0]] * (len(rows) - 2) + [slots[-1]]
    dv = Vector(direction).normalized()
    nv = Vector(normal)
    nv = (nv - dv * nv.dot(dv)).normalized()
    side = nv.cross(dv).normalized()
    c = Vector(base)
    prev = 0.0
    verts, ids = [], []
    for t, wf in rows:
        steps = max(1, int(round((t - prev) * 24)))
        for k in range(steps):
            tm = prev + (t - prev) * (k + 0.5) / steps
            a = lean * tm + curl * tm * tm
            c = c + (dv * math.cos(a) + side * math.sin(a)) * (length * (t - prev) / steps)
        prev = t
        a = lean * t + curl * t * t
        across = side * math.cos(a) - dv * math.sin(a)
        p = c + nv * (cup * length * t * t)
        hw = wf * width * 0.5
        if hw <= 1e-6:
            ids.append((len(verts),))
            verts.append(p)
        else:
            ids.append((len(verts), len(verts) + 1))
            verts += [p - across * hw, p + across * hw]
    bands = []
    for k in range(len(ids) - 1):
        a, b = ids[k], ids[k + 1]
        bands.append(((a[0], a[1], b[0]) if len(b) == 1 else (a[0], a[1], b[1], b[0]), slots[k]))
    return verts, list(reversed(bands))


def _fronds(kit, d, name, origin, strips, thickness=0.012):
    """Several strips (each a dict of _strip's arguments) as one part object seated at origin."""
    verts, faces = [], []
    for s in strips:
        v, f = _strip(**s)
        off = len(verts)
        verts += v
        faces += [(tuple(i + off for i in idx), sl) for idx, sl in f]
    return _sheet(kit, d, name, verts, faces, origin, thickness)


# ------------------------------------------------------------------------------ wings
# Small stubby leaf wings: a short twig of an arm and a fan of four leaves off the wrist, one
# along each finger, in the Blazeplume's wing layout (made for the Pouncer plan's fold at a
# 40 degree dihedral: the fingers sit about 35 degrees apart round the wrist, so folded they all
# point back together and the leaves close into one neat bundle along the upper flank). Every
# leaf stays in the sector its fingers sweep, within 2.45 of the wrist.
def _polar(c, ang, r):
    return (c[0] + r * math.cos(math.radians(ang)), c[1] + r * math.sin(math.radians(ang)))


def _wing_layout(elbow, wrist, fingers):
    L = {"root": (0.0, 0.0), "elbow": elbow, "wrist": wrist, "thumb": (wrist[0] + 0.07, wrist[1] - 0.27),
         "body": (0.0, 0.42)}
    for i, (a, r) in enumerate(fingers):
        L[f"f{i + 1}"] = _polar(wrist, a, r)
    return L


WING_LAYOUT = _wing_layout((0.66, 0.03), (1.06, -0.42), [(5, 2.1), (40.5, 2.2), (72, 2.05), (103.5, 1.9)])
for _form in (GROWN, HATCH):
    _form["wing"]["layout"] = WING_LAYOUT
# Each leaf: (finger, length as a fraction of the finger, width as a fraction of its length).
LEAVES = [("f1", 0.9, 0.46), ("f2", 1.0, 0.5), ("f3", 0.92, 0.5), ("f4", 0.74, 0.5)]


def _wing_frame(kit, side):
    """The wing plane's upper normal."""
    from mathutils import Vector
    w = kit.F["wing"]
    s = -1 if side == "L" else 1
    th, ph = math.radians(w["dihedral"]), math.radians(w["droop"])
    span = Vector((s * math.cos(th), 0, math.sin(th)))
    chord = Vector((0, math.cos(ph), -math.sin(ph)))
    n = span.cross(chord).normalized()
    return n if n.z > 0 else -n


def wings(kit, d, rare):
    """Leaf wings: a twig arm (root, elbow, wrist) and a fan of leaves in fern green (the rare
    Wildfire's ember-tipped, glowing at the points)."""
    wr = kit.F["wing"]["radii"]
    objs = []
    rows = kit.lod(LEAF, LOW)
    slots = ["membrane", "membrane", "membrane", "horn", "glow_flat"] if rare else \
        ["membrane", "membrane", "membrane", "membrane", "pattern_flat"]
    leaves = kit.lod(LEAVES, LEAVES[:3])
    for side in ("L", "R"):
        w = kit.wing_points(side)
        arm = kit.wing_arm(side, ["root", "elbow", "wrist"], [wr["root"], wr["elbow"], wr["wrist"]],
                           kit.lod(kit.F["wing"]["arm_tris"], 36), d["mats"], material="body_plain", claws=False)
        objs.append(arm)
        n = _wing_frame(kit, side)
        strips = []
        for finger, lf, wf in leaves:
            dirn = w[finger] - w["wrist"]
            length = dirn.length * lf
            strips.append(dict(base=w["wrist"] + dirn.normalized() * wr["wrist"] * 0.5, direction=tuple(dirn),
                               normal=tuple(n), length=length, width=length * wf, rows=rows, slots=slots,
                               cup=-0.06))
        objs.append(_fronds(kit, d, f"leaves_{side}", w["wrist"], strips, kit.F["wing"]["thickness"]))
    return objs


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


def _ears(kit, d, baby):
    """Tall pointed cat ears (the Pouncer's, a real volume), the inner ear in the accent colour,
    each tipped with a lynx tuft, a sprig of three little leaves (seated with the ear: same origin)."""
    import mathutils
    F, mats = kit.F, d["mats"]
    k = F["head"]["k"]
    out = []
    for sd in (-1, 1):
        base = kit.head_point((0.2, 0.07, 0.24) if not baby else (0.2, 0.06, 0.22), sd)
        length, width, depth = (0.35 if not baby else 0.32) * k, (0.3 if not baby else 0.3) * k, 0.066 * k
        s, front, u = _ear_frame(kit, (sd * 0.3, 0.25, 0.92), (sd * 0.38, 0.36, 0.85))
        e = kit.horn_mesh(f"ear_{sd}", length, width * 0.5, 0.35, kit.lod(4, 3), kit.lod(6, 5), taper=0.75, tip=0.04)
        e.rotation_euler = mathutils.Matrix((-s, -front, u)).transposed().to_euler()
        e.location = base
        e.scale = (1.0, depth / (width * 0.4), 1.0)
        e.data.materials.append(mats["body_plain"])
        b = base + front * 0.036 * k + u * 0.03 * k
        il, iw = length * 0.72, width * 0.6
        inner = kit.flat_fan(f"earin_{sd}", [b, b + s * iw * 0.5, b + s * iw * 0.4 + u * il * 0.55, b + u * il,
                                              b - s * iw * 0.4 + u * il * 0.55, b - s * iw * 0.5], 0.008)
        inner.data.materials.append(mats["accent_flat"])
        # The lynx tuft: a little sprig of three leaves fanning up and back from the tip.
        tip = base + u * length * 0.8
        tl = length * (0.5 if not baby else 0.46)
        spec = [(u - front * 0.3, 1.0, -0.5), (u * 0.7 - front * 0.5 + s * 0.55, 0.8, -0.4),
                (u * 0.7 - front * 0.5 - s * 0.55, 0.8, -0.4)]
        strips = [dict(base=tip, direction=tuple(dv), normal=tuple(front), length=tl * f, width=0.46 * tl * f,
                       rows=kit.lod(LEAF, LOW), slots=SPRIG_SLOTS, curl=cu)
                  for j, (dv, f, cu) in enumerate(spec) if not (kit.LOD and j)]
        tuft = _fronds(kit, d, f"eartuft_{sd}", base, strips, 0.012 * k)
        out += [(e, "head"), (inner, "head"), (tuft, "head")]
    return out


def _nose(kit, d, baby):
    """A little button nose (a rounded triangle, in the tongue's pink) on the tip of the snout,
    over the nostrils: the cat in the face. Its base is built out by the group's inset, so it
    sits on the skin once seated along the ray from the snout's joint."""
    import bmesh
    F, V = kit.F, kit.V
    at = V((0.0, -0.912, 1.0) if baby else _h(0.0, -2.1, 1.884)) + V((0.002, 0, 0))  # off the midline seam
    joint = kit.node("muzzle")
    out = (at - joint).normalized()
    w, h, dp = (0.05, 0.034, 0.022) if baby else (0.048 * HK, 0.034 * HK, 0.02 * HK)
    bm = bmesh.new()
    right = V((1, 0, 0))
    up = out.cross(right).normalized()
    if up.z < 0:
        up = -up
    kit.add_dome(bm, out * F["inset"]["frill"], out, up, w, h, dp, kit.lod(8, 6), 2, 0)
    for v in bm.verts:  # pinch the lower half into a soft point
        rel = v.co - out * F["inset"]["frill"]
        k = rel.dot(up) / h
        if k < 0:
            v.co -= right * rel.dot(right) * min(0.75, -k * 0.75)
    obj = kit.mesh_object("nose", bm, at)
    obj.data.materials.append(d["mats"]["tongue"])
    kit.smooth(obj)
    return [(obj, "snout")]


def _surface(kit, bone, nxt, t, around, radius=None):
    """A point on the skin graph's surface t along the segment bone -> nxt, `around` degrees
    round it from the top toward +x, and the outward direction there. radius: for bodies
    without node radii (the hatchling's metaballs), the distance out from the segment."""
    a, b = kit.node(bone), kit.node(nxt)
    up = kit.spine_up(bone, nxt)
    side = (b - a).normalized().cross(up).normalized()
    if side.x < 0:
        side = -side
    ang = math.radians(around)
    out = up * math.cos(ang) + side * math.sin(ang)
    if radius is None:
        ra, rb = kit.F["nodes"][bone][1], kit.F["nodes"][nxt][1]
        rx, rz = ra[0] * (1 - t) + rb[0] * t, ra[1] * (1 - t) + rb[1] * t
        radius = math.hypot(rz * math.cos(ang), rx * math.sin(ang))
    return a.lerp(b, t) + out * radius, out, up, (b - a).normalized()


# Back fronds: (bone, next node, t along, length); the hatchling has none (the rare one's three
# little flames: (..., the distance out from the spine)).
BACK = [("neck1", "neck2", 0.25, 0.42), ("chest", "belly", 0.1, 0.54), ("chest", "belly", 0.62, 0.6),
        ("belly", "hips", 0.3, 0.62), ("hips", "tail2", 0.05, 0.54), ("hips", "tail2", 0.55, 0.44)]
BACK_LOD1 = [BACK[1], BACK[3], BACK[4]]
BABY_BACK = [("chest", "belly", 0.4, 0.2, 0.26), ("belly", "hips", 0.5, 0.22, 0.3), ("hips", "tail2", 0.2, 0.18, 0.26)]


def _back_fronds(kit, d, rare, baby):
    """A crest of fern fronds down the back, each lying back along the spine with its tip
    curling up like a flame's lick, glowing like an ember (the rare Wildfire's: longer, tipped in
    flame). [(object, bone)]"""
    from mathutils import Vector
    if baby:
        if not rare:
            return []
        spec = BABY_BACK
    else:
        spec = [s + (None,) for s in kit.lod(BACK, BACK_LOD1)]
    rows = kit.lod(FLAME_FROND if rare else FROND, FROND_LOW)
    slots = kit.lod(FLAME_SLOTS if rare else FROND_SLOTS, FROND_LOW_SLOTS)
    grow = 1.22 if rare and not baby else 1.0
    pieces = []
    for i, (bone, nxt, t, length, rad) in enumerate(spec):
        s = 1 if i % 2 else -1  # a gentle zigzag, so the crest reads from the front too
        base, out, up, back = _surface(kit, bone, nxt, t, 6 * s, rad)
        dirn = (up * 0.62 + back * 0.78 + Vector((0.1 * s, 0, 0))).normalized()
        ln = length * grow
        st = dict(base=base, direction=tuple(dirn), normal=(1.0, 0.0, -0.12 * s), length=ln, width=ln * 0.42,
                  rows=rows, slots=slots, curl=1.5 if not rare else 1.9, lean=0.2)
        pieces.append((_fronds(kit, d, f"frond{int(rare)}_{i}", base, [st], 0.014 if not baby else 0.008), bone))
    return pieces


# Tail leaflets: (bone, its head node, next node, t along, length).
TAIL_LEAVES = [("tail1", "hips", "tail2", 0.82, 0.36), ("tail2", "tail2", "tail3", 0.2, 0.46),
               ("tail2", "tail2", "tail3", 0.66, 0.44), ("tail3", "tail3", "tail4", 0.25, 0.38),
               ("tail3", "tail3", "tail4", 0.72, 0.32)]
# The tuft at the tail's end: (sideways, backward lean off the tail's line, length, curl).
TUFT_FRONDS = [(0.0, 0.0, 1.0, -1.3), (0.62, 0.3, 0.8, 1.0), (-0.62, 0.3, 0.8, 1.0)]
BABY_TUFT = [(0.0, 0.0, 1.0, -1.3), (0.0, 0.7, 0.82, -1.1)]


def _tail(kit, d, rare, baby):
    """The bushy fern-frond tail: narrow leaflets down both sides of the tail, shrinking toward
    the tip, and a tuft of fronds at its curled-up end, their tips glowing (the Wildfire's in
    flame)."""
    from mathutils import Vector
    pieces = []
    k = kit.F["tail_k"]
    leaf_slots = ["membrane", "membrane", "membrane", "membrane", "pattern_flat"]
    if not baby:
        for i, (bone, a, b, t, length) in enumerate(TAIL_LEAVES):
            if kit.LOD and i % 2 == 1:
                continue
            for s in (-1, 1):
                base, out, up, back = _surface(kit, a, b, t, 70 * s)
                dirn = (out * 0.55 + back * 0.75 + up * 0.38).normalized()
                st = dict(base=base, direction=tuple(dirn), normal=tuple(up * 0.9 - out * 0.4), length=length,
                          width=length * 0.36, rows=kit.lod(LEAF, LOW), slots=leaf_slots, lean=-s * 0.25, cup=0.1)
                pieces.append((_fronds(kit, d, f"tleaf{int(rare)}_{i}_{s}", base, [st], 0.012), bone))
    a, tip = kit.node("tail4"), kit.node("tail_tip")
    tipdir = (tip - a).normalized()
    back = Vector((0.0, 1.0, 0.0))
    back = (back - tipdir * back.dot(tipdir)).normalized()
    base = tip - tipdir * 0.06 * k + Vector((0.004, 0, 0))
    ln = (0.62 if not baby else 0.5) * k * (1.25 if rare else 1.0)
    spec = BABY_TUFT if baby else kit.lod(TUFT_FRONDS, TUFT_FRONDS[:1])
    strips = []
    for sx, lb, f, cu in spec:
        dv = (tipdir + back * lb + Vector((sx, 0.0, 0.0))).normalized()
        nrm = Vector((1.0, 0.0, 0.0)) if not sx else back  # the side ones fan out across the tail
        strips.append(dict(base=base, direction=tuple(dv), normal=tuple(nrm), length=ln * f, width=ln * f * 0.42,
                           rows=kit.lod(FLAME_FROND if rare else FROND, FROND_LOW),
                           slots=kit.lod(FLAME_SLOTS if rare else FROND_SLOTS, FROND_LOW_SLOTS),
                           curl=cu if sx >= 0 else -cu, lean=0.1))
    pieces.append((_fronds(kit, d, f"ttuft{int(rare)}", base, strips, 0.014 if not baby else 0.01), "tail4"))
    return pieces


def _berries(kit, d, spots, name, radius):
    """Glowing ember berries half set in the moss: [(object, bone)]. spots: [(bone, point on the
    right side, berries, mirrored, outward or None)]; a bone name ending in "_" takes the side.
    Each is a little cluster (1-3 berries) facing outward (default: away from its bone's joint),
    its centre seated on the skin (group runes, inset below it)."""
    import bmesh
    V = kit.V
    pieces = []
    offsets = [(0.0, 0.0, 1.0), (1.75, 0.45, 0.78), (-0.7, -1.6, 0.7)]
    for i, (bone, at, count, mirrored, out0) in enumerate(spots):
        for s in (-1, 1) if mirrored else (1,):
            c = kit.mirror(at, s)
            b = f"{bone}{'L' if s < 0 else 'R'}" if bone.endswith("_") else bone
            joint = next(kit.node(h) for n, h, t, p in kit.BONES if n == b)
            out = kit.mirror(out0, s).normalized() if out0 is not None else (c - joint).normalized()
            hint = V((0, 0, 1)) if abs(out.z) < 0.9 else V((0, 1, 0))
            right = hint.cross(out).normalized()
            up = out.cross(right).normalized()
            bm = bmesh.new()
            for j, (ox, oy, sz) in enumerate(offsets[:kit.lod(count, 1)]):
                r = radius * sz
                p = (right * ox * s + up * oy) * radius
                kit.add_dome(bm, p - out * r * 0.3, out, up, r, r, r * 1.2, kit.lod(6, 5), kit.lod(2, 1), 0)
            obj = kit.mesh_object(f"{name}_{i}_{s}", bm, c)
            obj.data.materials.append(d["mats"]["glow_flat"])
            kit.smooth(obj)
            pieces.append((obj, b))
    return pieces


# Ember berries on the body: (bone, point on the right side, berries); the Wildfire's many
# more replace them.
BERRIES = [("chest", (0.52, -0.46, 1.34), 1), ("belly", (0.62, 0.04, 1.26), 2), ("hips", (0.54, 0.66, 1.3), 2),
           ("leg_up_", (0.52, 0.6, 0.72), 1)]
BERRIES_RARE = [("chest", (0.52, -0.46, 1.34), 1), ("belly", (0.62, 0.04, 1.26), 3), ("hips", (0.54, 0.66, 1.3), 2),
                ("leg_up_", (0.5, 0.6, 0.72), 1), ("belly", (0.44, 0.28, 1.56), 1),
                ("neck1", (0.36, -0.84, 1.66), 1)]
# Ember spots along the tail's top and the outside of its curl: (bone, head node, next, t, around).
TAIL_SPOTS = [("tail1", "hips", "tail2", 0.86, 0.0), ("tail2", "tail2", "tail3", 0.5, 0.0),
              ("tail3", "tail3", "tail4", 0.5, 0.0)]
TAIL_SPOTS_RARE = TAIL_SPOTS + [("tail2", "tail2", "tail3", 0.3, 55.0), ("tail3", "tail3", "tail4", 0.3, -55.0)]

BABY_BERRIES = [("hips", (0.2, 0.36, 0.72), 1)]
BABY_BERRIES_RARE = BABY_BERRIES + [("chest", (0.25, -0.1, 0.74), 1), ("belly", (0.3, 0.1, 0.58), 1)]


def _tail_spots(kit, spec):
    out = []
    for bone, a, b, t, around in spec:
        p, o, _, _ = _surface(kit, a, b, t, around)
        p = p + kit.V((0.004, 0, 0)) if around == 0 else p
        out.append((bone, tuple(p), 1, False, tuple(o)))
    return out


def _sprout(kit, d, rare):
    """The hatchling's sprout: a little stem, two round leaves and a bud that glows like an
    ember (the Wildfire's holds a little flame). One object seated at the stem's foot."""
    V = kit.V
    mats = d["mats"]
    top = kit.head_point((0.0, 0.02, 0.25)) + V((0.002, 0, 0))
    stem = kit.tube(f"sprout_{int(rare)}", [top, top + V((0, 0.012, 0.08)), top + V((0, 0.03, 0.15))],
                    [0.02, 0.016, 0.013], ring=kit.lod(5, 4))
    stem.data.materials.append(mats["membrane"])
    objs = [stem]
    for s in (-1, 1):
        dl = V((s * 0.8, 0.12, 0.55)).normalized()
        lf = kit.blade(f"sprout_leaf_{s}", top + V((0, 0.02, 0.09)), dl, V((-dl.z * s, -0.3, dl.x * s)), 0.15,
                       0.1, 0.01)
        lf.data.materials.append(mats["membrane"])
        objs.append(lf)
    tip = top + V((0, 0.03, 0.15))
    if rare:
        slots = ["horn", "horn", "glow_flat", "glow_flat", "glow_flat", "glow_flat", "glow_flat", "glow_flat"]
        flame = [dict(base=tip, direction=(0, -0.1, 1), normal=(1, 0, 0), length=0.24, width=0.1,
                      rows=kit.lod(FLAME_FROND, LOW), slots=slots, curl=-1.0, lean=0.2),
                 dict(base=tip, direction=(0, 0.1, 1), normal=(0, 1, 0), length=0.18, width=0.08,
                      rows=kit.lod(FLAME_FROND, LOW), slots=slots, curl=0.8, lean=-0.1)]
        objs.append(_fronds(kit, d, "sprout_flame", tip, flame, 0.008))
    bud = kit.blob("sprout_bud", tip + V((0, 0, 0.045)), (0.05, 0.05, 0.07), kit.lod(8, 6), kit.lod(5, 4))
    bud.data.materials.append(mats["glow_flat"])
    objs.append(bud)
    return kit.join(objs, f"sprout_{int(rare)}")


def parts(kit, d):
    baby = d["form"] == "hatchling"
    out = []
    # Ears with fern tufts (everyone).
    out.append(("frill", 0, _ears(kit, d, baby) + _nose(kit, d, baby)))
    # The baby's sprout (group horns: the Wildfire's holds a flame).
    if baby:
        for v in (0, 1):
            out.append(("horns", v, [(_sprout(kit, d, bool(v)), "head")]))
    # The back's fronds (group spikes; the Wildfire's flame-tipped ones replace them).
    for v in (0, 1):
        pieces = _back_fronds(kit, d, bool(v), baby)
        if pieces:
            out.append(("spikes", v, pieces))
    # The fern tail and its tuft (group tail_tip).
    for v in (0, 1):
        out.append(("tail_tip", v, _tail(kit, d, bool(v), baby)))
    # Ember berries in the moss and ember spots along the tail (group runes).
    if baby:
        out.append(("runes", 0, _berries(kit, d, [s + (True, None) for s in BABY_BERRIES], "berry0", 0.036)))
        out.append(("runes", 1, _berries(kit, d, [s + (True, None) for s in BABY_BERRIES_RARE], "berry1", 0.036)))
    else:
        common = [s + (True, None) for s in kit.lod(BERRIES, BERRIES[:3])] + _tail_spots(kit, TAIL_SPOTS)
        rare = [s + (True, None) for s in kit.lod(BERRIES_RARE, BERRIES_RARE[:6])] + \
            _tail_spots(kit, kit.lod(TAIL_SPOTS_RARE, TAIL_SPOTS))
        out.append(("runes", 0, _berries(kit, d, common, "berry0", 0.075)))
        out.append(("runes", 1, _berries(kit, d, rare, "berry1", 0.075)))
    return out


def accent(kit, body):
    """A paler moss bib down the throat and chest, the belly and the muzzle."""
    import mathutils
    mask = kit.mask_attr(body)
    F = kit.F
    mk = F["mask"]
    down = mathutils.Vector((0, -0.45, -0.89)).normalized()
    front = mathutils.Vector((0, -0.9, -0.25)).normalized()
    baby = F["name"] == "hatchling"
    chest_y = F["nodes"]["chest"][0][1]
    for v in body.data.vertices:
        x, y, z = v.co
        a = min(1.0, max(0.0, (v.normal.dot(down) - 0.2) / 0.45))
        if y < chest_y + (0.1 if baby else 0.2):  # the bib: the chest and throat facing forward
            a = max(a, min(1.0, max(0.0, (v.normal.dot(front) - 0.4) / 0.35)) * (1.0 if abs(x) < mk["max_x"] else 0))
        if abs(x) > mk["max_x"] * (1.2 if y < chest_y else 1.0) or z > mk["max_z"] or z < mk["min_z"]:
            a = 0.0
        if mk.get("tail_cut") and y > mk["tail_cut"][0] and z < mk["tail_cut"][1]:
            a = 0.0
        mask.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


def texture(tx, nt, p, form):
    """R: darker moss patches over the back and flanks; G: golden leaf flecks scattered over the
    upper body; B: many small glowing embers (the Wildfire's). The painted value carries soft
    moss cushions (their edges shaded) over the whole body."""
    body = tx.mul(nt, tx.top(nt), tx.band(nt, 1, p["face_y"], 10.0, soft=p["patch"] * 0.8))  # not on the face
    r = tx.maxi(nt, tx.blotches(nt, p["dapple"], threshold=(0.5, 0.57), where=body),
                tx.spots(nt, p["patch"], keep=0.6, size=(0.32, 0.22), where=body, seed_offset=2.3))
    g = tx.spots(nt, p["leaf_cell"], keep=0.45, size=(0.3, 0.2), where=tx.upper(nt), seed_offset=1.1)
    embers = tx.mul(nt, tx.upper(nt), tx.band(nt, 1, p["face_y"], 10.0, soft=p["patch"] * 0.8))
    b = tx.maxi(nt, tx.spots(nt, p["ember_cell"], keep=0.5, size=(0.24, 0.14), where=embers, seed_offset=4.2),
                tx.spots(nt, p["ember_cell"] * 0.6, keep=0.62, size=(0.22, 0.1), where=embers, seed_offset=6.7))
    # Moss cushions: each Voronoi cell a soft pillow, bright in the middle and gently shaded
    # toward its edge (no hard lines: the storybook look has no scales).
    cell = tx.node(nt, "ShaderNodeTexVoronoi", feature="F1")
    cell.inputs["Scale"].default_value = 1.0 / p["clump"]
    nt.links.new(tx.coords(nt), cell.inputs["Vector"])
    cushion = tx.madd(nt, tx.smoothstep(nt, cell.outputs["Distance"], 0.15, 0.7), -0.26, 1.0)
    value = tx.mul(nt, tx.mul(nt, tx.grain(nt, p["grain"], p["grain_amount"]), tx.light_from_above(nt, p["light"])),
                   cushion)
    return {"r": r, "g": g, "b": b, "value": value}
