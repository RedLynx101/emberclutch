"""The Blazeplume (Ember and Gale, uncommon crossbreed of the Pouncer and the Crestwing): a
phoenix-cat. Dragondex 9. Concept: docs/art/concept/dragons/dragon_blazeplume.jpg (reference only).

Design notes
  * Parents. The Pouncer gives it the lithe cat's body, the round cat head with tall pointed
    ears and a cat's face; the Crestwing gives it feathers: feathered wings, a crest and a
    plumed tail. What is new is fire: every feather is a flame, shading from gold at the root
    through scarlet to a hot orange tip, and the tail ends in a spray of plumes whose tips glow.
  * The adult. Proud and warm: a deep chest carried high on a longer, upright neck (a bird's
    bearing on a cat), a golden bib and belly, flame licks rising up its flanks from the belly
    like a fire seen from below. On the head a crest of flame feathers sweeps back between the
    ears; each ear ends in a little flame tuft (the lynx in it); flame tufts on the cheeks and
    a short flame mane down the back of the neck. Big layered feathered wings (gold coverts over
    scarlet flight feathers with hot orange tips) that fold into a neat bundle on the flank.
    A long cat's tail ending in a fan of long curling plumes with glowing tips.
  * The hatchling. A round kitten-chick: a big round head, huge eyes, a fluffy down bib and
    fluffy cheeks, stubby legs, a stumpy tail with a little flame tuft that glows like a
    candle, a tiny three-feather crest and tiny feathery wing buds.
  * Feathers are built here (the kit has blades and plumed membranes; these are layered,
    gradient-painted feather sheets): each feather is a strip whose vertices are painted from
    the palette row by row (gold -> scarlet -> hot orange -> glow), so the game shades them
    smoothly from root to tip. Wings are a feather vane (flight feathers, notched trailing
    edge) and a row of gold coverts on a slim bony arm, weighted to the Pouncer plan's classic
    wing chain so they fold and spread with its clips.
  * Variants: Blaze (orange, scarlet, gold: the natural one), Sunset (gold and rose: the
    surprise), Cinder (charcoal and ash with ember-bright feather tips: the subtle one), and the
    rare Phoenix (white-gold, glowing flame veins, glowing plume and crest tips, a bigger crest
    and longer, more flame-like tail plumes).
"""
import math


def _mirrored(center, sides):
    nodes = dict(center)
    for name, (p, r) in sides.items():
        nodes[f"{name}_R"] = ((p[0], p[1], p[2]), r)
        nodes[f"{name}_L"] = ((-p[0], p[1], p[2]), r)
    return nodes


META = dict(
    name="blazeplume", title="Blazeplume", dex=9, element=("Ember", "Gale"), parents=("pouncer", "crestwing"),
    rarity="uncommon", plan="pouncer", size=1.1,
    stats=dict(wing=8, wit=6, might=5, breath=8, stamina=6),
    manners=("Proud", "Brave", "Playful", "Curious"),
    traits=("Swift", "Sunbather", "Warm-Blooded", "Showoff", "Skydancer", "Phoenix Heart"),
    rare_variant=3, rare_replaces=True,
    blurb="A phoenix-cat with plumes of living flame: proud and warm-hearted, it basks in the sun and rides the updrafts.",
)

# Palette use: base = body, accent = bib, belly, muzzle and the wings' gold coverts,
# membrane = the flight feathers and plumes, horn = the hot feather tips, pattern = the flame
# licks (or the mantle), glow = glowing plume tips (and the rare one's glowing flames).
VARIANTS = [
    dict(name="Blaze", base=(0.88, 0.2, 0.04), accent=(1.0, 0.64, 0.16), pattern=(1.0, 0.56, 0.08),
         horn=(1.0, 0.40, 0.03), membrane=(0.72, 0.05, 0.03), iris=(0.32, 0.10, 0.03), glow=(1.0, 0.72, 0.2),
         pattern_channel="r", glow_channel=None),
    dict(name="Sunset", base=(1.0, 0.5, 0.14), accent=(1.0, 0.85, 0.55), pattern=(0.8, 0.22, 0.32),
         horn=(1.0, 0.6, 0.3), membrane=(0.82, 0.2, 0.3), iris=(0.28, 0.1, 0.24), glow=(1.0, 0.7, 0.45),
         pattern_channel="g", glow_channel=None),
    dict(name="Cinder", base=(0.12, 0.105, 0.115), accent=(0.36, 0.23, 0.2), pattern=(0.36, 0.1, 0.05),
         horn=(1.0, 0.38, 0.04), membrane=(0.1, 0.085, 0.095), iris=(1.0, 0.56, 0.1), glow=(1.0, 0.46, 0.08),
         pattern_channel="r", glow_channel=None),
    dict(name="Phoenix", base=(1.0, 0.84, 0.55), accent=(1.0, 0.95, 0.82), pattern=(1.0, 0.6, 0.16),
         horn=(0.95, 0.2, 0.04), membrane=(1.0, 0.42, 0.06), iris=(0.9, 0.3, 0.04), glow=(1.0, 0.78, 0.28),
         pattern_channel="r", glow_channel="b"),
]

EGG = dict(height=1.0, width=0.36, asym=0.18, speckle="swirl", speckle_params=dict(size=(0.02, 0.085)),
           colors=[((1.0, 0.76, 0.30), (0.84, 0.16, 0.07)), ((1.0, 0.88, 0.58), (0.92, 0.40, 0.46)),
                   ((0.30, 0.26, 0.27), (1.0, 0.46, 0.12)), ((1.0, 0.96, 0.86), (1.0, 0.60, 0.18))])

# ------------------------------------------------------------------------------ grown
# Faces -Y, Z up; units ~ metres at adult size. radius = (side, vertical). The Pouncer's frame
# with a prouder chest, a longer upright neck and the head carried higher.
HEAD = (0.0, -1.44, 2.60)
_DY, _DZ = HEAD[1] + 1.46, HEAD[2] - 2.50  # from the Pouncer's head


def _h(x, y, z):
    """A point of the Pouncer's head moved onto this head."""
    return (x, y + _DY, z + _DZ)


GROWN_NODES = _mirrored({
    "tail_tip": ((0, 3.86, 1.78), (0.035, 0.035)),
    "tail4": ((0, 3.26, 1.36), (0.08, 0.08)),
    "tail3": ((0, 2.52, 1.10), (0.135, 0.125)),
    "tail2": ((0, 1.74, 1.04), (0.21, 0.20)),
    "hips": ((0, 0.92, 1.18), (0.40, 0.42)),
    "belly": ((0, 0.18, 1.19), (0.41, 0.46)),
    "chest": ((0, -0.52, 1.30), (0.46, 0.57)),
    "neck1": ((0, -1.00, 1.70), (0.33, 0.35)),
    "neck2": ((0, -1.18, 2.02), (0.27, 0.28)),
    "neck3": ((0, -1.30, 2.32), (0.235, 0.235)),
    "head": (HEAD, (0.335, 0.30)),
    "muzzle": (_h(0, -1.685, 2.42), (0.18, 0.15)),
    "snout": (_h(0, -1.88, 2.36), (0.11, 0.09)),
}, {
    "shoulder": ((0.34, -0.52, 1.04), (0.21, 0.25)),
    "elbow": ((0.37, -0.58, 0.56), (0.13, 0.13)),
    "wrist": ((0.37, -0.64, 0.16), (0.10, 0.10)),
    "toe_f": ((0.38, -0.88, 0.07), (0.115, 0.065)),
    "hipj": ((0.34, 0.94, 0.98), (0.28, 0.32)),
    "knee": ((0.40, 0.62, 0.56), (0.16, 0.16)),
    "ankle": ((0.40, 1.02, 0.20), (0.105, 0.105)),
    "toe_b": ((0.41, 0.78, 0.07), (0.115, 0.065)),
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
    "sturdy": {"chest": (1.07, 0.98), "belly": (1.06, 0.98), "hips": (1.05, 1.0), "neck1": (1.06, 0.96),
               "leg_up": (1.06, 0.97), "arm_up": (1.06, 0.97), "tail2": (1.05, 0.96)},
    "sleek": {"chest": (0.95, 1.02), "belly": (0.93, 1.03), "hips": (0.95, 1.0), "leg_lo": (0.95, 1.04),
              "arm_lo": (0.95, 1.04), "tail3": (0.94, 1.05), "tail4": (0.94, 1.05)},
    "long": {"neck1": (0.97, 1.08), "neck2": (0.97, 1.08), "belly": (0.97, 1.07), "tail2": (0.96, 1.08),
             "tail3": (0.96, 1.1), "tail4": (0.96, 1.1)},
}


def _grown_sculpt(kit, obj):
    """A cat's shapes the node graph can't give (the Pouncer's, on this head): a deep chest over
    a tucked belly, round cheeks, a short soft muzzle and a flat brow."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for v in bm.verts:
        x, y, z = v.co
        if y < -0.62 and 1.0 < z < 2.0 and abs(x) < 0.4:  # a proud, round breast
            k = (1 - abs(x) / 0.4) * math.sin(math.pi * (z - 1.0) / 1.0) * min(1.0, (-0.62 - y) / 0.25)
            v.co.y -= 0.075 * k
        if -0.95 < y < -0.1 and z < 1.24 and abs(x) < 0.36:  # chest keel
            k = (1 - abs(x) / 0.36) * max(0.0, (1.24 - z) / 0.45)
            v.co.z -= 0.08 * k
        if 0.0 < y < 0.75 and z < 1.0 and abs(x) < 0.32:  # tucked belly
            k = (1 - abs(x) / 0.32) * max(0.0, (1.0 - z) / 0.3)
            v.co.z += 0.07 * k
        if -1.62 + _DY < y < -1.3 + _DY and 2.38 + _DZ < z < 2.62 + _DZ:  # round cheeks
            v.co.x *= 1.1
        if y < -1.62 + _DY and z > 2.44 + _DZ:  # soft short muzzle, a little flatter on top
            t = min(1.0, (-1.62 + _DY - y) / 0.3)
            v.co.z -= 0.025 * t
    bm.to_mesh(obj.data)
    bm.free()


GROWN = dict(
    name="grown", nodes=GROWN_NODES, edges=GROWN_EDGES, body="skin",
    body_tris=1400, body_tris_lod1=540, export_scale=1.0,
    young={
        "bones": {
            "head": (0.95, 0.9, 1.0), "snout": (0.9, 0.7, 0.95),
            "neck1": (0.74, 0.5), "neck2": (0.76, 0.48), "neck3": (0.8, 0.48),
            "chest": (0.66, 0.52), "belly": (0.64, 0.5), "hips": (0.68, 0.52),
            "tail1": (0.66, 0.5), "tail2": (0.68, 0.48), "tail3": (0.72, 0.48), "tail4": (0.8, 0.52),
            "arm_up": (0.68, 0.58), "arm_lo": (0.7, 0.58), "hand": (0.8, 0.72),
            "leg_up": (0.68, 0.58), "leg_lo": (0.7, 0.56), "foot": (0.8, 0.72),
        },
        "parts": {"eyes": 1.35, "horns": 0.55, "frill": 0.8, "wings": 0.42, "spikes": 0.6,
                  "tail_tip": 0.6, "heart": 0.85, "runes": 0.7},
    },
    young_pose={"neck1": -18, "neck2": -6, "neck3": 4, "head": 18},
    base_pose={"neck1": (-6, 0, 0), "neck2": (4, 0, 0), "neck3": (10, 0, 0), "head": (-6, 0, 0),
               "tail1": (8, 0, 0), "tail2": (4, 0, 6), "tail3": (6, 0, 10), "tail4": (10, 0, 12)},
    builds=BUILDS,
    eyes=dict(at=_h(0.16, -1.70, 2.56), out=(0.58, -0.80, 0.12), iris=(0.092, 0.103, 0.05),
              pupil=(0.054, 0.068, 0.019), slit=(0.3, 1.12),
              glints=((-0.022, 0.035, 0.016), (0.016, -0.035, 0.008)), seg=(12, 2, 8, 2)),
    head=dict(origin=HEAD, k=1.0, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=False, frill_k=1.0, feather_w=1.0),
    tail_k=1.0,
    heart=dict(at=(0, -1.02, 1.36), size=0.105),
    wing=dict(root=(0.30, -0.46, 1.72), scale=1.1, dihedral=40, droop=4,
              radii={"root": 0.09, "elbow": 0.066, "wrist": 0.052, "finger": 0.02, "tip": 0.008},
              arm_tris=80, thickness=0.012, style="classic"),
    mask=dict(max_x=0.3, max_z=2.5, min_z=-1.0, tail_cut=(1.3, 0.95)),
    inset={"eyes": 0.02, "horns": 0.03, "spikes": 0.02, "frill": 0.04, "tail_tip": 0.03, "heart": -0.06,
           "runes": -0.012},
    face=dict(nostril=_h(0.045, -1.97, 2.41), nostril_r=(0.022, 0.014, 0.008), mouth_r=0.011,
              mouth=lambda side, a: (side * 0.14 * a ** 0.7, -2.0 + _DY + 0.36 * a ** 1.5, 2.32 + _DZ + 0.07 * a * a)),
    jaw_hinge=_h(0, -1.54, 2.35),
    mouth_detail=dict(depth=0.26, fade=0.2, width=0.36, tooth=(0.009, 0.016), fang=(0.013, 0.032),
                      tongue=(0.055, 0.085, 0.012)),
    skin=dict(lick=0.42, flame_z=(0.86, 1.62), flame_y=(-1.1, 1.5), flame_fade=0.14, ao=0.6),
    sculpt=_grown_sculpt,
)

# ------------------------------------------------------------------------------ hatchling
HATCH_NODES = _mirrored({
    "tail_tip": ((0, 1.18, 0.52), None),
    "tail4": ((0, 1.02, 0.42), None),
    "tail3": ((0, 0.84, 0.36), None),
    "tail2": ((0, 0.62, 0.38), None),
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
    ("ell", (0, -0.52, 1.12), (0.36, 0.33, 0.32)),     # big round kitten head
    ("ell", (0, -0.78, 0.98), (0.17, 0.15, 0.12)),     # short button muzzle
    ("ell", (0, -0.74, 0.92), (0.13, 0.12, 0.07)),     # chin
    ("chain", [(0, -0.30, 0.66), (0, -0.40, 0.80), (0, -0.47, 0.92)], [0.18, 0.17, 0.16]),
    ("ell", (0, -0.17, 0.55), (0.30, 0.28, 0.30)),     # chest
    ("ell", (0, 0.06, 0.49), (0.33, 0.31, 0.31)),      # round chick tummy
    ("ell", (0, 0.28, 0.51), (0.27, 0.22, 0.26)),      # hips
    ("ell", (0, 0.0, 0.38), (0.26, 0.27, 0.19)),
    ("ball", (0, -0.1, 0.84), 0.09), ("ball", (0, 0.12, 0.81), 0.09), ("ball", (0, 0.31, 0.77), 0.08),  # down
    ("chain", [(0, 0.48, 0.46), (0, 0.66, 0.38), (0, 0.84, 0.36), (0, 1.02, 0.42), (0, 1.16, 0.50)],
     [0.12, 0.09, 0.075, 0.06, 0.045]),              # a short stumpy tail
    ("ell", (0, -0.31, 0.70), (0.2, 0.14, 0.17)),      # the fluffy bib
    ("ball", (0, -0.34, 0.58), 0.13),
]
for _s in (-1, 1):
    HATCH_META += [
        ("ball", (_s * 0.17, -0.72, 0.96), 0.135),      # fluffy cheeks
        ("ball", (_s * 0.14, -0.30, 0.62), 0.12),       # bib fluff
        ("ball", (_s * 0.25, -0.08, 0.72), 0.085), ("ball", (_s * 0.27, 0.3, 0.64), 0.085),  # down on shoulders, hips
        ("chain", [(_s * 0.21, -0.16, 0.42), (_s * 0.22, -0.21, 0.25), (_s * 0.23, -0.25, 0.10)],
         [0.1, 0.088, 0.085]),
        ("ell", (_s * 0.23, -0.32, 0.06), (0.095, 0.12, 0.06)),    # front paws
        ("ell", (_s * 0.21, 0.32, 0.38), (0.13, 0.17, 0.17)),      # thighs
        ("chain", [(_s * 0.23, 0.37, 0.26), (_s * 0.24, 0.38, 0.12)], [0.09, 0.08]),
        ("ell", (_s * 0.24, 0.28, 0.06), (0.095, 0.13, 0.06)),     # hind paws
    ]

HATCH = dict(
    name="hatchling", nodes=HATCH_NODES, meta=HATCH_META, body="meta",
    body_tris=1500, body_tris_lod1=520, export_scale=1.0,
    young={
        "bones": {name: ((0.74, 0.74) if name in ("head", "snout") else (0.62, 0.62))
                  for name in ("hips", "belly", "chest", "neck1", "neck2", "neck3", "head", "snout", "tail1",
                               "tail2", "tail3", "tail4", "arm_up", "arm_lo", "hand", "leg_up", "leg_lo", "foot")},
        "parts": {"eyes": 1.12, "horns": 0.7, "frill": 0.9, "wings": 0.6, "spikes": 0.85,
                  "tail_tip": 0.85, "heart": 1.0, "runes": 0.9},
    },
    base_pose={"head": (4, 0, 0), "tail1": (10, 0, 0), "tail2": (8, 0, 8), "tail3": (10, 0, 10),
               "tail4": (12, 0, 12)},
    builds={"neutral": {}, "sturdy": {}, "sleek": {}, "long": {}},
    eyes=dict(at=(0.17, -0.78, 1.09), out=(0.42, -0.90, 0.07), iris=(0.12, 0.134, 0.066),
              pupil=(0.082, 0.096, 0.03), slit=(0.32, 1.1),
              glints=((-0.028, 0.045, 0.028), (0.024, -0.042, 0.013)), seg=(14, 3, 12, 2)),
    head=dict(origin=(0, -0.50, 1.23), k=0.9, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=True,
              frill_k=0.6, feather_w=1.6),
    tail_k=0.42,
    heart=dict(at=(0, -0.42, 0.55), size=0.075),
    wing=dict(root=(0.15, -0.10, 0.78), scale=0.24, dihedral=40, droop=4,
              radii={"root": 0.045, "elbow": 0.035, "wrist": 0.03, "finger": 0.011, "tip": 0.005},
              arm_tris=80, thickness=0.008, style="classic"),
    mask=dict(max_x=0.24, max_z=1.06, min_z=0.13, tail_cut=None),
    inset={"eyes": 0.032, "horns": 0.02, "spikes": 0.012, "frill": 0.03, "tail_tip": 0.02, "heart": -0.03,
           "runes": -0.01},
    face=dict(nostril=(0.042, -0.905, 1.005), nostril_r=(0.024, 0.016, 0.009), mouth_r=0.011,
              mouth=lambda side, a: (side * 0.13 * a ** 0.8, -0.93 + 0.19 * a ** 1.6, 0.925 + 0.03 * a * a)),
    jaw_hinge=(0, -0.70, 0.93),
    mouth_detail=dict(depth=0.15, fade=0.11, width=0.28, tooth=(0.007, 0.012), fang=(0.010, 0.022),
                      tongue=(0.052, 0.08, 0.012)),
    skin=dict(lick=0.3, flame_z=(0.28, 0.78), flame_y=(-0.5, 0.62), flame_fade=0.1, ao=0.25),
)

FORMS = {"grown": GROWN, "hatchling": HATCH}

# ------------------------------------------------------------------------------ feathers
# Material slots -> the variant colour a vertex shows (the exporter's MATERIAL_PAINT) and its
# glow (emissive / 255).
SLOT_COLOR = {"body_plain": ("base", 0.0), "accent_flat": ("accent", 0.0), "pattern_flat": ("pattern", 0.0),
              "membrane": ("membrane", 0.0), "horn": ("horn", 0.0), "glow_flat": ("glow", 230 / 255)}


def _vc_material(kit, slot):
    """A material named like a palette slot (the exporter paints by the name) whose preview
    colour comes from the per-vertex attributes "vc" (colour) and "ve" (glow), so the review
    renders shade feathers smoothly from root to tip as the game does."""
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
    """The feather materials of this build (one per slot), made once per dragon."""
    if "vc_mats" not in d:
        d["vc_mats"] = {slot: _vc_material(kit, slot) for slot in SLOT_COLOR}
    return d["vc_mats"]


def _sheet(kit, d, name, verts, faces, origin, thickness, double=True, normals=None):
    """A feather sheet: verts (world points), faces [(indices, slot)] listed tip-first. The
    exporter paints a vertex by the LAST face using it, so listing the faces from the tip
    inward paints each row of vertices by the face on its root side: the colours run smoothly
    root -> tip. double: two sheets (the game culls back faces). normals: the offset
    direction per vertex (default: from the faces)."""
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
    sheets = [(1, +0.5)] + ([(-1, -0.5)] if double else [])
    for flip, off in sheets:
        vs = [bm.verts.new(p - o + n * (thickness * off if double else 0.0)) for p, n in zip(V, normals)]
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


def _frame(a, b, up):
    """Unit direction a->b, a side vector (perpendicular, in the plane with `up`) and the
    sheet normal."""
    from mathutils import Vector
    d = (Vector(b) - Vector(a)).normalized()
    n = Vector(up) - d * Vector(up).dot(d)
    n = n.normalized()
    return d, d.cross(n).normalized(), n


# Feather strips: rows [(t along the length, width factor)] ending in the point (width 0), and
# one palette slot per band between rows (root band first).
FEATHER = [(0.0, 0.4), (0.3, 1.0), (0.66, 0.72), (1.0, 0.0)]
PLUME = [(0.0, 0.32), (0.24, 1.0), (0.52, 0.86), (0.78, 0.52), (1.0, 0.0)]
FLAME = [(0.0, 0.3), (0.15, 0.95), (0.3, 0.62), (0.46, 1.0), (0.62, 0.5), (0.78, 0.62), (1.0, 0.0)]
ROUND = [(0.0, 0.45), (0.3, 1.0), (0.7, 0.85), (0.92, 0.45), (1.0, 0.0)]
LOW = [(0.0, 0.38), (0.4, 1.0), (1.0, 0.0)]  # LOD1


def _strip(base, direction, normal, length, width, rows, slots, lean=0.0, wave=0.0, cup=0.0):
    """One flame-shaped feather as (verts, faces listed tip-first): a strip from base along
    direction, lying in the plane with this normal; lean sweeps it sideways in its plane
    (quadratically, a gentle curve), wave swings it in an S like a flame tongue, cup curls it
    out of its plane toward the normal. See _sheet for the paint order."""
    from mathutils import Vector
    if len(slots) != len(rows) - 1:  # fewer rows (LOD1): the root band's and the tip band's paint
        slots = [slots[0]] * (len(rows) - 2) + [slots[-1]]
    dv = Vector(direction).normalized()
    nv = Vector(normal)
    nv = (nv - dv * nv.dot(dv)).normalized()
    side = nv.cross(dv).normalized()
    b = Vector(base)
    verts, ids = [], []
    for t, wf in rows:
        c = (b + dv * (t * length) + side * (lean * length * t * t + wave * length * math.sin(math.pi * t) * t)
             + nv * (cup * length * t * t))
        hw = wf * width * 0.5
        if hw <= 1e-6:
            ids.append((len(verts),))
            verts.append(c)
        else:
            ids.append((len(verts), len(verts) + 1))
            verts += [c - side * hw, c + side * hw]
    bands = []
    for k in range(len(ids) - 1):
        a, bb = ids[k], ids[k + 1]
        if len(bb) == 1:
            bands.append(((a[0], a[1], bb[0]), slots[k]))
        else:
            bands.append(((a[0], a[1], bb[1], bb[0]), slots[k]))
    return verts, list(reversed(bands))


def _feathers(kit, d, name, origin, strips, thickness=0.012, extra=None):
    """Several strips (each a dict of _strip's arguments) as one part object with its origin
    (the point seated on the skin) at `origin`; extra = (verts, faces) to add (an ear)."""
    verts, faces = [], []
    pieces = ([extra] if extra else []) + [_strip(**s) for s in strips]
    for v, f in pieces:
        off = len(verts)
        verts += v
        faces += [(tuple(i + off for i in idx), sl) for idx, sl in f]
    return _sheet(kit, d, name, verts, faces, origin, thickness)


# ------------------------------------------------------------------------------ wings
# The feathered wing, in its own layout (u out along the span, v back along the chord) made
# for the plan's fold (the classic clips' WINGS_FOLDED). The fold lays the wing's plane flat on
# the flank and turns each wing bone in that plane by a fixed angle, whatever the layout: the
# arm +3 degrees, the forearm -114, the four fingers 0, -41, -80 and -116. So:
#   * the arm points straight out (folded, it lies along the back line instead of rising);
#   * the forearm reaches forward to the wrist (folded, the wrist comes to the shoulder front);
#   * the fingers sit 37-41 degrees apart round the wrist, so folded they all point the same
#     way, a little down: the closed fan is one neat bundle on the upper flank;
#   * the feathers stay in the sector the fingers sweep (from the first finger to a little past
#     the last), each weighted to the fingers or the forearm on either side of it, and within
#     reach of the wrist, so folded their tips end together over the rump.
# The arm itself carries only short coverts pointing out along it (they fold along the arm).
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
HAND = ("elbow", "wrist", "f1")          # the leading line the flight feathers root on
# Flight feathers, inner to outer: (root along the hand line 0..1, direction in degrees from
# the span toward the chord, length), in layout units: tertials and secondaries off the
# forearm lying almost parallel, primaries off the hand fanning out to the rounded tip. Every
# tip stays within 2.45 of the wrist (the folded bundle's length).
VANE = [(0.00, 116, 1.42), (0.08, 111, 1.55), (0.15, 106, 1.64), (0.22, 100, 1.74), (0.30, 91, 1.78),
        (0.39, 80, 1.86), (0.48, 67, 1.9), (0.57, 54, 1.76), (0.66, 42, 1.42), (0.75, 30, 1.12),
        (0.84, 18, 0.86), (0.92, 9, 0.62), (0.985, 3, 0.42)]
VANE_LOD1 = [VANE[i] for i in (0, 2, 4, 6, 8, 10, 12)]
BUD = [(0.0, 112, 1.35), (0.22, 98, 1.6), (0.45, 70, 1.75), (0.7, 36, 1.3), (0.95, 6, 0.7)]


def _line_point(w, names, a):
    """The point a fraction a (0..1) along the polyline of layout points `names` (by length)."""
    pts = [w[n] for n in names]
    lengths = [0.0]
    for p, q in zip(pts, pts[1:]):
        lengths.append(lengths[-1] + (q - p).length)
    t = a * lengths[-1]
    for k in range(len(pts) - 1):
        if lengths[k + 1] >= t - 1e-9:
            u = (t - lengths[k]) / max(1e-9, lengths[k + 1] - lengths[k])
            return pts[k].lerp(pts[k + 1], u)
    return pts[-1].copy()


def _wing_frame(kit, side):
    """The wing plane's span and chord axes (unit) and its upper normal."""
    from mathutils import Vector
    w = kit.F["wing"]
    s = -1 if side == "L" else 1
    th, ph = math.radians(w["dihedral"]), math.radians(w["droop"])
    span = Vector((s * math.cos(th), 0, math.sin(th)))
    chord = Vector((0, math.cos(ph), -math.sin(ph)))
    n = span.cross(chord).normalized()
    if n.z < 0:
        n = -n
    return span, chord, n


def _feather_wing(kit, d, side, rays, slots, covert=True, notch=(0.07, 0.13), thickness=0.012):
    """One wing's feathers: the flight-feather vane (rounded tips, each overlapping the next,
    with notches between them, painted root -> tip by `slots` = (root band, tip band)), a row
    of coverts over its root half (gold, their edges hot orange)."""
    w = kit.wing_points(side)
    span, chord, n = _wing_frame(kit, side)
    k = kit.F["wing"]["scale"]

    def along(p, phi, length):
        return p + (span * math.cos(math.radians(phi)) + chord * math.sin(math.radians(phi))) * length * k

    def facing(idx, verts):
        a, b, c = verts[idx[0]], verts[idx[1]], verts[idx[2]]
        return idx if (b - a).cross(c - a).dot(n) >= 0 else tuple(reversed(idx))

    R = [_line_point(w, HAND, a) for a, _, _ in rays]
    T = [along(r, phi, length) for r, (_, phi, length) in zip(R, rays)]
    M = [r.lerp(t, 0.5) for r, t in zip(R, T)]
    verts = R + M + T
    nr = len(rays)
    iR, iM, iT = range(0, nr), range(nr, 2 * nr), range(2 * nr, 3 * nr)
    iQ, iB = [], []
    for i in range(nr - 1):
        depth = notch[1] if i >= nr // 3 else notch[0]
        inner = M[i].lerp(M[i + 1], 0.5)
        q = T[i].lerp(T[i + 1], 0.5).lerp(inner, depth / 0.5)
        mid = T[i].lerp(q, 0.5)
        bulge = mid + (mid - inner).normalized() * (T[i] - q).length * 0.22
        iQ.append(len(verts))
        verts.append(q)
        iB.append(len(verts))
        verts.append(bulge)
    tips, inner = [], []
    for i in range(nr - 1):
        tips += [((iM[i], iB[i], iT[i]), slots[1]), ((iM[i], iQ[i], iB[i]), slots[1]),
                 ((iM[i], iM[i + 1], iQ[i]), slots[1]), ((iM[i + 1], iT[i + 1], iQ[i]), slots[1])]
        inner += [((iR[i], iR[i + 1], iM[i + 1], iM[i]), slots[0])]
    faces = [(facing(idx, verts), s) for idx, s in tips + inner]
    objs = [_sheet(kit, d, f"vane_{side}", verts, faces, w["root"], thickness, normals=[n] * len(verts))]
    if not covert:
        return objs
    # Coverts: rounded scallops over the root half of the flight feathers, gold with hot edges.
    lift = n * thickness * 2.2
    C = [r.lerp(t, 0.42) for r, t in zip(R, T)]
    cv = [p + lift for p in R + C]
    iC = range(nr, 2 * nr)
    iN = []
    for i in range(nr - 1):
        iN.append(len(cv))
        cv.append(C[i].lerp(C[i + 1], 0.5).lerp(R[i].lerp(R[i + 1], 0.5), 0.3) + lift)
    edge, root = [], []
    for i in range(nr - 1):
        edge += [(facing((i, iN[i], iC[i]), cv), "horn"), (facing((i + 1, iC[i + 1], iN[i]), cv), "horn")]
        root += [(facing((i, i + 1, iN[i]), cv), "accent_flat")]
    objs.append(_sheet(kit, d, f"covert_{side}", cv, edge + root, w["root"], thickness, normals=[n] * len(cv)))
    return objs


def wings(kit, d, rare):
    """Feathered wings: a slim bony arm along the leading edge, the flight-feather vane (scarlet
    to hot orange tips; the rare one's tips glow) and gold coverts."""
    baby = d["form"] == "hatchling"
    if rare and baby:
        return []
    wr = kit.F["wing"]["radii"]
    objs = []
    for side in ("L", "R"):
        arm = kit.wing_arm(side, ["root", "elbow", "wrist", "f1"], [wr["root"], wr["elbow"], wr["wrist"], wr["finger"]],
                           kit.lod(kit.F["wing"]["arm_tris"], 48), d["mats"], material="body_plain", claws=False)
        objs.append(arm)
        rays = BUD if baby else kit.lod(VANE, VANE_LOD1)
        slots = ("membrane", "glow_flat" if rare else "horn")
        objs += _feather_wing(kit, d, side, rays, slots, covert=not kit.LOD, thickness=kit.F["wing"]["thickness"])
    return objs


# ------------------------------------------------------------------------------ parts
def _ear(kit, base, out, up, length, width):
    """A pointed cat ear (the Pouncer's shape) as a fan (verts, faces), and its tip and axes."""
    V = kit.V
    b, o, u = V(base), V(out).normalized(), V(up).normalized()
    s = o.cross(u).normalized()
    pts = [b, b + s * width * 0.5, b + s * width * 0.42 + u * length * 0.55, b + u * length,
           b - s * width * 0.42 + u * length * 0.55, b - s * width * 0.5]
    faces = [((0, i, i + 1), "body_plain") for i in range(1, len(pts) - 1)]
    return (pts, faces), b + u * length, u, o


def _crest(kit, rare, baby):
    """The flame crest: feathers rising from the brow and sweeping back over the crown, the
    middle ones tallest (the rare Phoenix's: more, longer and wavier, their tips glowing)."""
    hk = kit.F["head"]["k"]
    x_axis = (1.0, 0.0, 0.0)
    if baby:
        spec = [((0.0, -0.06, 0.3), (0.0, 0.2, 1.0), 0.3, 0.15, -0.35),
                ((0.07, 0.02, 0.28), (0.25, 0.55, 0.85), 0.24, 0.13, -0.3),
                ((-0.07, 0.02, 0.28), (-0.25, 0.55, 0.85), 0.24, 0.13, -0.3)]
        if rare:  # a taller tuft, its tips aglow
            spec = [(at, dirn, length * 1.3, width * 1.1, lean) for at, dirn, length, width, lean in spec] + \
                [((0.0, 0.08, 0.27), (0.0, 0.8, 0.6), 0.28, 0.13, -0.3)]
    elif rare:
        spec = [((0.0, -0.17, 0.24), (0.0, -0.05, 1.0), 0.42, 0.16, -0.35),
                ((0.0, -0.06, 0.28), (0.0, 0.3, 1.0), 0.78, 0.2, -0.4),
                ((0.0, 0.06, 0.27), (0.0, 0.62, 0.8), 0.7, 0.19, -0.35),
                ((0.0, 0.16, 0.22), (0.0, 0.9, 0.45), 0.52, 0.16, -0.25),
                ((0.075, -0.04, 0.26), (0.3, 0.45, 0.9), 0.56, 0.16, -0.35),
                ((-0.075, -0.04, 0.26), (-0.3, 0.45, 0.9), 0.56, 0.16, -0.35),
                ((0.09, 0.08, 0.23), (0.35, 0.75, 0.6), 0.44, 0.14, -0.3),
                ((-0.09, 0.08, 0.23), (-0.35, 0.75, 0.6), 0.44, 0.14, -0.3)]
    else:
        spec = [((0.0, -0.15, 0.25), (0.0, 0.1, 1.0), 0.34, 0.16, -0.4),
                ((0.0, -0.05, 0.28), (0.0, 0.45, 1.0), 0.68, 0.2, -0.5),
                ((0.0, 0.08, 0.26), (0.0, 0.85, 0.62), 0.54, 0.18, -0.4),
                ((0.08, -0.02, 0.26), (0.32, 0.55, 0.8), 0.46, 0.16, -0.4),
                ((-0.08, -0.02, 0.26), (-0.32, 0.55, 0.8), 0.46, 0.16, -0.4)]
    rows = kit.lod(FLAME if rare and not baby else FEATHER, LOW)
    slots = ["accent_flat", "membrane", "horn", "glow_flat", "glow_flat", "glow_flat"] if rare else \
        ["accent_flat", "membrane", "horn"]
    if rare and baby:
        slots = ["accent_flat", "horn", "glow_flat"]
    out = []
    for i, (at, dirn, length, width, lean) in enumerate(spec):
        if kit.LOD and (i >= 5 or (i == 0 and not baby)):
            continue
        base = kit.head_point(at)
        normal = (1.0, 0.0, -0.35 * (1 if at[0] > 0 else -1) if at[0] else 0.0) if at[0] else x_axis
        out.append(dict(base=base, direction=dirn, normal=normal, length=length * hk, width=width * hk, rows=rows,
                        slots=slots[:len(rows) - 1], lean=lean, wave=0.1 if rare else 0.08))
    return out


def _tail_plumes(kit, rare, baby):
    """The tail's end: a spray of long plumes curling up like flames, their tips glowing."""
    k = kit.F["tail_k"]
    if baby:
        spec = [((0.0, 0.55, 0.84), 0.62, 0.3, -0.3), ((0.45, 0.6, 0.66), 0.48, 0.26, -0.25),
                ((-0.45, 0.6, 0.66), 0.48, 0.26, -0.25)]
        if rare:
            spec = [(dirn, length * 1.3, width, lean) for dirn, length, width, lean in spec]
    else:
        spec = [((0.0, 0.52, 0.86), 1.0, 0.3, -0.34), ((0.0, 0.88, 0.48), 0.9, 0.29, -0.32),
                ((0.0, 0.99, 0.05), 0.62, 0.25, -0.26), ((0.5, 0.74, 0.46), 0.76, 0.27, -0.3),
                ((-0.5, 0.74, 0.46), 0.76, 0.27, -0.3), ((0.42, 0.9, 0.06), 0.54, 0.23, -0.22),
                ((-0.42, 0.9, 0.06), 0.54, 0.23, -0.22)]
        if rare:  # longer, wavier flames
            spec = [(dirn, length * 1.35, width * 1.1, lean) for dirn, length, width, lean in spec]
    rows = kit.lod(FLAME if rare else PLUME, LOW)
    slots = ["membrane", "membrane", "horn", "horn", "glow_flat", "glow_flat"] if rare else \
        ["membrane", "horn", "glow_flat", "glow_flat"]
    if baby:
        rows, slots = kit.lod(FEATHER, LOW), ["horn" if rare else "membrane", "glow_flat", "glow_flat"]
    x, y, z = kit.F["nodes"]["tail_tip"][0]
    base = kit.V((0.0, y - 0.04 * k, z - 0.02 * k))
    out = []
    for i, (dirn, length, width, lean) in enumerate(spec):
        if kit.LOD and i >= 5:
            continue
        dv = kit.V(dirn).normalized()
        nrm = kit.V((0.0, 0.0, 1.0)).cross(dv)
        nrm = nrm.normalized() if nrm.length > 0.3 else kit.V((1.0, 0.0, 0.0))
        out.append(dict(base=base, direction=dirn, normal=tuple(nrm), length=length * k, width=width * k, rows=rows,
                        slots=slots, lean=lean, wave=0.07 if rare else 0.05))
    return base, out


def _ruff(kit, d, baby):
    """A soft collar of broad feathers round the base of the neck (above the heartglow), lying
    down over the shoulders: the lion in the phoenix."""
    from mathutils import Vector
    n1, n2 = kit.node("neck1"), kit.node("neck2")
    axis = (n2 - n1).normalized()
    c = n1.lerp(n2, 0.3 if not baby else 0.2)
    back = Vector((0.0, 1.0, 0.0))
    back = (back - axis * back.dot(axis)).normalized()
    side = Vector((1.0, 0.0, 0.0))
    rad = kit.F["nodes"]["neck1"][1][0] if kit.F["nodes"]["neck1"][1] else 0.2
    pieces = []
    angles = kit.lod((0, 45, -45, 95, -95, 140, -140), (0, 70, -70, 135, -135))
    for i, a in enumerate(angles):
        r = back * math.cos(math.radians(a)) + side * math.sin(math.radians(a))
        base = c + r * rad * 0.85
        dirn = (-axis * 0.8 + r * 0.36 + Vector((0, 0.12, 0))).normalized()
        normal = r - dirn * r.dot(dirn)
        length = (0.4 if abs(a) < 120 else 0.33) * (1.0 if not baby else 0.45)
        st = dict(base=base, direction=tuple(dirn), normal=tuple(normal), length=length,
                  width=(0.34 if not baby else 0.16), rows=kit.lod(ROUND, LOW),
                  slots=["body_plain", "body_plain", "accent_flat", "horn"], lean=0.0, cup=-0.18)
        pieces.append((_feathers(kit, d, f"ruff_{i}", base, [st]), "neck1"))
    return pieces


def parts(kit, d):
    F, mats, V = kit.F, d["mats"], kit.V
    baby = d["form"] == "hatchling"
    hk = F["head"]["k"]
    out = []
    # Ears: the Pouncer's tall pointed ears, each tipped with a little flame tuft (the lynx in
    # it); the inner ear in the accent colour.
    ears = []
    for s in (-1, 1):
        base = kit.head_point((0.17, 0.08, 0.2) if not baby else (0.2, 0.06, 0.22), s)
        o, u = (s * 0.35, 0.25, 0.9), (s * 0.55, 0.45, 0.72)
        length = (0.42 if not baby else 0.32) * hk
        fan, tip, uv, ov = _ear(kit, base, o, u, length, (0.28 if not baby else 0.3) * hk)
        tuft = dict(base=tip - uv * length * 0.12, direction=uv + V((0, 0.35, 0)), normal=tuple(ov),
                    length=length * (0.42 if not baby else 0.36), width=0.1 * hk, rows=kit.lod(FEATHER, LOW),
                    slots=["membrane", "horn", "horn"], lean=-0.25)
        e = _feathers(kit, d, f"ear_{s}", base, [tuft], thickness=0.045 * hk, extra=fan)
        inner = kit.flat_fan(f"earin_{s}", [base + V((0, -0.02, 0.02)) * hk] + _ear(
            kit, base + V((0, -0.02, 0.02)) * hk, o, u, length * 0.72, (0.17 if not baby else 0.19) * hk)[0][0][1:],
            0.06 * hk)
        inner.data.materials.append(mats["accent_flat"])
        ears += [(e, "head"), (inner, "head")]
    # Cheek tufts: flame feathers sweeping back from the cheeks.
    cheeks = []
    for s in (-1, 1):
        at = kit.head_point((0.21, 0.02, -0.06) if not baby else (0.24, -0.02, -0.1), s)
        strips = [dict(base=at, direction=(s * 0.5, 0.85, 0.12), normal=(s * 0.3, -0.1, 1.0),
                       length=(0.34 if not baby else 0.2) * hk, width=0.13 * hk, rows=kit.lod(FEATHER, LOW),
                       slots=["body_plain", "membrane", "horn"], lean=s * 0.2)]
        if not kit.LOD:
            strips.append(dict(base=at + V((0, 0.0, -0.06)) * hk, direction=(s * 0.45, 0.8, -0.2),
                               normal=(s * 0.3, -0.1, 1.0), length=(0.26 if not baby else 0.16) * hk,
                               width=0.11 * hk, rows=FEATHER, slots=["body_plain", "membrane", "horn"],
                               lean=s * 0.15))
        cheeks.append((_feathers(kit, d, f"cheek_{s}", at, strips), "head"))
    out.append(("frill", 0, ears + cheeks))
    # The ruff (group spikes: seated down onto the skin); the baby has its fluffy bib instead.
    if not baby:
        out.append(("spikes", 0, _ruff(kit, d, baby)))
    # The crest (group horns: the rare one's replaces it).
    for v, rare in ((0, False), (1, True)):
        pieces = []
        for i, st in enumerate(_crest(kit, rare, baby)):
            pieces.append((_feathers(kit, d, f"crest{v}_{i}", st["base"], [st]), "head"))
        out.append(("horns", v, pieces))
    # The tail's plume spray (group tail_tip: the rare one's longer flames replace it).
    for v, rare in ((0, False), (1, True)):
        base, strips = _tail_plumes(kit, rare, baby)
        out.append(("tail_tip", v, [(_feathers(kit, d, f"plume{v}_{i}", base, [st]), "tail4")
                                     for i, st in enumerate(strips)]))
    return out


def accent(kit, body):
    """A golden bib down the throat and chest, the belly and the muzzle."""
    import mathutils
    mask = kit.mask_attr(body)
    F = kit.F
    mk = F["mask"]
    down = mathutils.Vector((0, -0.45, -0.89)).normalized()
    front = mathutils.Vector((0, -0.9, -0.2)).normalized()
    baby = F["name"] == "hatchling"
    chest_y = F["nodes"]["chest"][0][1]
    for v in body.data.vertices:
        x, y, z = v.co
        a = min(1.0, max(0.0, (v.normal.dot(down) - 0.2) / 0.45))
        if y < chest_y + (0.1 if baby else 0.2):  # the bib: the chest and throat facing forward
            a = max(a, min(1.0, max(0.0, (v.normal.dot(front) - 0.35) / 0.35)) * (1.0 if abs(x) < mk["max_x"] else 0))
        if abs(x) > mk["max_x"] * (1.25 if y < chest_y else 1.0) or z > mk["max_z"] or z < mk["min_z"]:
            a = 0.0
        if mk.get("tail_cut") and y > mk["tail_cut"][0] and z < mk["tail_cut"][1]:
            a = 0.0
        mask.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


def _fire(tx, nt, p, hot=0.0):
    """A field of flame tongues rising from the belly line: a wave across the body (their
    rhythm) and vertically stretched noise (their wandering), thresholded against the height
    on the flank, so each tongue is broad at its root and licks up to a point. hot > 0 keeps
    only their hot cores."""
    lo, hi = p["flame_z"]
    z = tx.axis(nt, 2)
    h = tx.node(nt, "ShaderNodeMapRange", clamp=True)
    nt.links.new(z, h.inputs["Value"])
    h.inputs["From Min"].default_value, h.inputs["From Max"].default_value = lo, hi
    wave = tx.node(nt, "ShaderNodeTexWave", wave_type="BANDS", bands_direction="Y", wave_profile="SIN")
    wave.inputs["Scale"].default_value = tx.WAVE_PERIOD / p["lick"]
    wave.inputs["Distortion"].default_value = 1.4
    wave.inputs["Detail"].default_value = 0.5
    nt.links.new(tx.coords(nt), wave.inputs["Vector"])
    stretch = tx.node(nt, "ShaderNodeVectorMath", operation="MULTIPLY")
    nt.links.new(tx.coords(nt), stretch.inputs[0])
    c = p["lick"]
    stretch.inputs[1].default_value = (1.0 / c, 1.0 / c, 0.35 / c)
    noise = tx.node(nt, "ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 1.0
    noise.inputs["Detail"].default_value = 1.0
    nt.links.new(stretch.outputs[0], noise.inputs["Vector"])
    v = tx.add(nt, tx.mul(nt, wave.outputs["Fac"], 0.45), tx.mul(nt, noise.outputs["Fac"], 0.55))
    edge = tx.math_op(nt, "SUBTRACT", v, tx.madd(nt, h.outputs["Result"], 0.72 + hot, 0.12 + hot * 0.5))
    f = tx.smoothstep(nt, edge, 0.16, 0.2)
    below = tx.smoothstep(nt, z, lo - p["flame_fade"], lo - p["flame_fade"] * 0.4)  # not down the legs
    y0, y1 = p["flame_y"]
    return tx.mul(nt, tx.mul(nt, f, below), tx.band(nt, 1, y0, y1, 0.12))


def texture(tx, nt, p, form):
    """R: flame tongues licking up the flanks from the belly; G: a soft mantle over the back,
    the top of the head and the tail; B: the rare one's glowing flames (the tongues' hot
    cores)."""
    r = _fire(tx, nt, p)
    b = _fire(tx, nt, p, hot=0.12)
    nz = tx.normal_z(nt)
    noise = tx.node(nt, "ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 0.5 / p["lick"]
    nt.links.new(tx.coords(nt), noise.inputs["Vector"])
    g = tx.smoothstep(nt, tx.add(nt, nz, tx.madd(nt, noise.outputs["Fac"], 0.6, -0.3)), 0.1, 0.55)
    return {"r": r, "g": g, "b": b}
