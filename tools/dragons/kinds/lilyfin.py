"""The Lilyfin (Grove and Tide, uncommon; Dragondex 13): the crossbreed of the Puffback and the
Ribbontail. A marsh axolotl-dragon. Concept: docs/art/concept/dragons/dragon_lilyfin.jpg
(reference only, D74).

Design notes
  * Parents. The Ribbontail gives it the water: a soft, long, low body, a finned paddle of a
    tail and frilly gills; the Puffback gives it the garden: little leafy wings, leaves for
    gills and a lily pad it wears as a hat. What is new is the marsh newt: a broad, flat,
    smiling axolotl's face, short splayed legs with webbed toes and a pond's colours.
  * Plan. The Pouncer's (plans/pouncer.py): a four-legged body with a neck, a four-bone tail
    and a four-finger wing, whose classic clips suit a slow, graceful newt (a lazy tail sway,
    a low stalk, an upright sit like the concept's). The Puffback's plan is made for a round
    hill of a body and the Curlstone's curls into a ball; neither is a newt.
  * The adult. Long and low on four short webbed legs splayed out to the sides, a plump soft
    body, a moderate neck carrying a broad, flat-topped head with a wide smile and big eyes set
    wide. Round the back of the head three fern fronds a side sweep back like an axolotl's
    gills. On the crown sits a lily pad, notched and a little tipped back like a beret, with a
    pink lotus in the middle. Small leafy wings of four leaves on slender veins fold into a neat
    bundle along the back and flutter open. The tail is a paddle: flattened side to side, with a
    leaf-edged fin along its top and a shorter one underneath, ending in a leaf-shaped paddle.
  * The hatchling. A tadpole of a newt: a big round head (a third of it), huge eyes, a wide
    smile, a round little body on stubby legs, a finned tadpole's tail curled round its side,
    two little fronds a side, tiny leaf buds for wings and a little lily pad with a lotus bud.
  * Parts. Hat and lotus (group horns, on the head); fronds (frill, on the head); the tail's
    fins (spikes, one piece a tail bone so the ribbon bends); the paddle (tail_tip); webbed toes
    (runes, one web a foot). The rare variant replaces the hat with a bigger one bearing an open
    glowing lotus and a bud, the fins and the paddle with glowing-edged ones and the fronds with
    longer ones glowing at the tips.
  * Palette use: base the body, accent the pale belly, chin and the lotus's heart, pattern the
    skin's dapples (and the pad's veins), membrane the leaves (wings, fronds, pad, fins), horn the
    lotus petals, glow the heart (and the rare one's lotus, fin edges, frond tips and glowing
    speckles).
  * Pattern: R round dapples on the back and flanks (Lotus's), G a mottle of blotches (Bayou's),
    B fine speckles with a row of dots down each flank like a fish's lateral line (Moonpond's
    pale speckles and the Spirit Lotus's glow).
  * Variants: Lotus (teal-green, pale belly, deep teal dapples, a pink lotus: the natural
    one), Bayou (olive with brown mottling, a yellow flower: the surprise), Moonpond (deep blue
    with pale moon speckles and a white water lily: the subtle one) and the rare Spirit Lotus
    (pale jade, a glowing lotus on a bigger pad, glowing fin edges and frond tips, glowing
    speckles).
  * Egg: pale jade with lily pads floating on it.
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
    name="lilyfin", title="Lilyfin", dex=13, element=("Grove", "Tide"), parents=("puffback", "ribbontail"),
    rarity="uncommon", plan="pouncer", size=1.0,
    stats=dict(wing=6, wit=8, might=4, breath=8, stamina=7),
    manners=("Gentle", "Curious", "Sleepy", "Playful"),
    traits=("Water-Lover", "Tidy", "Keen Nose", "Songbird", "Moonlit", "Elemental"),
    rare_variant=3, rare_replaces=True,
    blurb="A gentle marsh newt in a lily-pad hat: it paddles the reeds, hums to the frogs and naps in the sun.",
)

VARIANTS = [
    dict(name="Lotus", base=_srgb("#57B08E"), accent=_srgb("#EEF3C8"), pattern=_srgb("#2F7F6B"),
         horn=_srgb("#F7A1B9"), membrane=_srgb("#86C95A"), iris=_srgb("#6B4A22"), glow=(0.95, 0.9, 0.42),
         tongue=_srgb("#F08FA2"), pattern_channel="r", glow_channel=None),
    dict(name="Bayou", base=_srgb("#8C9444"), accent=_srgb("#EFE6B4"), pattern=_srgb("#77552F"),
         horn=_srgb("#F6D04C"), membrane=_srgb("#6F9A3C"), iris=_srgb("#B8682A"), glow=(1.0, 0.8, 0.32),
         tongue=_srgb("#E98A8A"), pattern_channel="g", glow_channel=None),
    dict(name="Moonpond", base=_srgb("#2F4F92"), accent=_srgb("#CFDDF4"), pattern=_srgb("#C8E8FF"),
         horn=_srgb("#F8F6EE"), membrane=_srgb("#3F8A86"), iris=_srgb("#E6C46A"), glow=(0.62, 0.86, 1.0),
         tongue=_srgb("#E6A0B8"), pattern_channel="b", glow_channel=None),
    dict(name="Spirit Lotus", base=_srgb("#A9DCC4"), accent=_srgb("#F4FBF4"), pattern=_srgb("#78BFA4"),
         horn=_srgb("#FFE3EE"), membrane=_srgb("#9FDDBE"), iris=_srgb("#D99A2B"), glow=(1.0, 0.48, 0.76),
         tongue=_srgb("#F2A0B6"), pattern_channel="r", glow_channel="b"),
]

# Pale jade with lily pads floating on it (display colours).
EGG = dict(height=1.0, width=0.39, asym=0.1, speckle="spots", speckle_params=dict(count=17, size=(0.055, 0.095)),
           colors=[(_hex("#CDEBD6"), _hex("#6FB85A")), (_hex("#E4E3BE"), _hex("#8A8F3E")),
                   (_hex("#C9D6EE"), _hex("#3F7E8A")), (_hex("#E6F6EC"), _hex("#9FE3C8"))])

# ------------------------------------------------------------------------------ grown
# Faces -Y, Z up; units ~ metres at adult size (before export_scale). radius = (side, vertical).
HEAD = (0.0, -1.5, 1.8)
HS = 1.1  # the face was drawn round a head at (0, -1.56, 1.56); this one is that much bigger


def _h(x, y, z):
    """A point of the face as drawn, moved and scaled onto this head."""
    return (x * HS, HEAD[1] + (y + 1.56) * HS, HEAD[2] + (z - 1.56) * HS)


GROWN_NODES = _mirrored({
    "tail_tip": ((0, 3.46, 0.63), (0.045, 0.11)),
    "tail4": ((0, 2.94, 0.60), (0.105, 0.21)),
    "tail3": ((0, 2.36, 0.64), (0.18, 0.27)),
    "tail2": ((0, 1.70, 0.74), (0.28, 0.33)),
    "hips": ((0, 0.98, 0.85), (0.47, 0.44)),
    "belly": ((0, 0.24, 0.88), (0.56, 0.5)),
    "chest": ((0, -0.50, 0.92), (0.52, 0.49)),
    "neck1": ((0, -0.96, 1.10), (0.41, 0.39)),
    "neck2": ((0, -1.14, 1.34), (0.36, 0.35)),
    "neck3": ((0, -1.30, 1.57), (0.34, 0.33)),
    "head": (HEAD, (0.52, 0.375)),
    "muzzle": (_h(0, -1.81, 1.47), (0.37, 0.21)),
    "snout": (_h(0, -1.97, 1.41), (0.21, 0.125)),
}, {
    "shoulder": ((0.38, -0.50, 0.62), (0.25, 0.27)),
    "elbow": ((0.55, -0.58, 0.38), (0.16, 0.16)),
    "wrist": ((0.58, -0.64, 0.14), (0.13, 0.125)),
    "toe_f": ((0.62, -0.85, 0.065), (0.18, 0.065)),
    "hipj": ((0.38, 0.98, 0.62), (0.29, 0.31)),
    "knee": ((0.55, 0.80, 0.38), (0.17, 0.17)),
    "ankle": ((0.58, 1.04, 0.14), (0.135, 0.13)),
    "toe_b": ((0.62, 0.84, 0.065), (0.19, 0.065)),
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
    "sturdy": {"chest": (1.07, 0.98), "belly": (1.07, 0.98), "hips": (1.05, 1.0), "neck1": (1.05, 0.97),
               "leg_up": (1.06, 0.97), "arm_up": (1.06, 0.97), "tail2": (1.05, 0.97)},
    "sleek": {"chest": (0.94, 1.02), "belly": (0.93, 1.03), "hips": (0.95, 1.01), "tail2": (0.94, 1.03),
              "tail3": (0.94, 1.04), "leg_lo": (0.95, 1.03), "arm_lo": (0.95, 1.03)},
    "long": {"neck1": (0.97, 1.07), "neck2": (0.97, 1.07), "belly": (0.97, 1.08), "tail2": (0.97, 1.08),
             "tail3": (0.97, 1.08), "tail4": (0.97, 1.08)},
}


def _grown_sculpt(kit, obj):
    """An axolotl's head the node graph can't give: a broad, flat crown, full round cheeks and a
    soft rounded muzzle; a plump body a little flattened underneath so it sits low."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    hx, hy, hz = HEAD
    for v in bm.verts:
        x, y, z = v.co
        if hy - 0.48 < y < hy + 0.34 and z > hz + 0.13:  # the flat crown
            k = min(1.0, (z - hz - 0.13) / 0.22)
            v.co.z -= 0.055 * k
        if hy - 0.32 < y < hy + 0.24 and hz - 0.22 < z < hz + 0.18:  # full round cheeks
            v.co.x *= 1.07
        if -0.6 < y < 1.2 and z < 0.5 and abs(x) < 0.42:  # a soft belly, a touch flatter
            v.co.z += 0.03 * min(1.0, (0.5 - z) / 0.12)
    bm.to_mesh(obj.data)
    bm.free()


GROWN = dict(
    name="grown", nodes=GROWN_NODES, edges=GROWN_EDGES, body="skin",
    body_tris=1320, body_tris_lod1=470, export_scale=0.95,
    young={
        "bones": {
            "head": (1.02, 0.95, 1.05), "snout": (0.92, 0.78, 0.97),
            "neck1": (0.74, 0.55), "neck2": (0.76, 0.52), "neck3": (0.8, 0.52),
            "chest": (0.68, 0.55), "belly": (0.66, 0.52), "hips": (0.68, 0.55),
            "tail1": (0.68, 0.52), "tail2": (0.7, 0.5), "tail3": (0.74, 0.5), "tail4": (0.8, 0.54),
            "arm_up": (0.72, 0.66), "arm_lo": (0.74, 0.66), "hand": (0.82, 0.78),
            "leg_up": (0.72, 0.66), "leg_lo": (0.74, 0.66), "foot": (0.82, 0.78),
        },
        "parts": {"eyes": 1.12, "horns": 0.72, "frill": 0.8, "wings": 0.55, "spikes": 0.6,
                  "tail_tip": 0.65, "heart": 0.85, "runes": 0.8},
    },
    young_pose={"neck1": -14, "neck2": -4, "head": 6},
    # The folded leaves turned out onto the upper flank (the plan's fold is made for a slimmer
    # body: on this plump one they sank in), the tail in a lazy S.
    base_pose={"neck1": (-4, 0, 0), "neck2": (2, 0, 0), "neck3": (6, 0, 0), "head": (-2, 0, 0),
               "tail1": (2, 0, 4), "tail2": (0, 0, 8), "tail3": (3, 0, -6), "tail4": (5, 0, -12),
               "wing_arm_R": (-10, 0, -35), "wing_arm_L": (-10, 0, 35)},
    builds=BUILDS,
    eyes=dict(at=_h(0.285, -1.8, 1.565), out=(0.62, -0.74, 0.22), iris=(0.135, 0.15, 0.074),
              pupil=(0.093, 0.108, 0.027), slit=(0.3, 1.1),
              glints=((-0.029, 0.043, 0.02), (0.022, -0.043, 0.01)), seg=(12, 2, 8, 2)),
    head=dict(origin=HEAD, k=1.22, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=False, frill_k=1.0, feather_w=1.0),
    tail_k=1.0,
    heart=dict(at=(0, -1.02, 0.96), size=0.12, tilt=24),
    wing=dict(root=(0.3, -0.42, 1.28), scale=0.5, dihedral=40, droop=4,
              radii={"root": 0.06, "elbow": 0.045, "wrist": 0.036, "finger": 0.013, "tip": 0.006},
              arm_tris=72, thickness=0.012, style="classic"),
    mask=dict(max_x=0.36, max_z=2.1, min_z=-1.0, tail_cut=None),
    inset={"eyes": 0.02, "horns": -0.012, "spikes": 0.02, "frill": 0.03, "tail_tip": 0.03, "heart": -0.035,
           "runes": 0.1},
    face=dict(nostril=_h(0.055, -2.125, 1.47), nostril_r=(0.026, 0.016, 0.009), mouth_r=0.013,
              mouth=lambda side, a: _h(side * 0.265 * a ** 0.7, -2.135 + 0.47 * a ** 1.5, 1.34 + 0.08 * a * a)),
    jaw_hinge=_h(0, -1.66, 1.37),
    mouth_detail=dict(depth=0.28, fade=0.22, width=0.55, tooth=(0.009, 0.016), fang=(0.013, 0.032),
                      tongue=(0.09, 0.09, 0.017), fangs=[]),
    teeth=False,
    skin=dict(spot_cell=0.26, dapple=1.6, ao=0.5, face_y=-1.74),
    sculpt=_grown_sculpt,
)

# ------------------------------------------------------------------------------ hatchling
HATCH_NODES = _mirrored({
    "tail_tip": ((0, 1.24, 0.36), None),
    "tail4": ((0, 1.06, 0.34), None),
    "tail3": ((0, 0.84, 0.33), None),
    "tail2": ((0, 0.60, 0.35), None),
    "hips": ((0, 0.34, 0.38), None),
    "belly": ((0, 0.10, 0.39), None),
    "chest": ((0, -0.14, 0.42), None),
    "neck1": ((0, -0.25, 0.52), None),
    "neck2": ((0, -0.32, 0.62), None),
    "neck3": ((0, -0.38, 0.72), None),
    "head": ((0, -0.46, 0.84), None),
    "muzzle": ((0, -0.72, 0.77), None),
    "snout": ((0, -0.87, 0.74), None),
}, {
    "shoulder": ((0.19, -0.14, 0.30), None),
    "elbow": ((0.23, -0.18, 0.18), None),
    "wrist": ((0.24, -0.21, 0.08), None),
    "toe_f": ((0.26, -0.32, 0.04), None),
    "hipj": ((0.19, 0.34, 0.30), None),
    "knee": ((0.23, 0.30, 0.18), None),
    "ankle": ((0.24, 0.37, 0.08), None),
    "toe_b": ((0.26, 0.26, 0.04), None),
})

HATCH_META = [
    ("ell", (0, -0.50, 0.86), (0.37, 0.32, 0.30)),     # a big, broad, round head
    ("ell", (0, -0.74, 0.77), (0.23, 0.15, 0.115)),    # a wide soft muzzle
    ("ell", (0, -0.64, 0.71), (0.19, 0.15, 0.08)),     # chin
    ("chain", [(0, -0.25, 0.52), (0, -0.33, 0.63), (0, -0.40, 0.74)], [0.17, 0.17, 0.17]),
    ("ell", (0, -0.13, 0.42), (0.23, 0.22, 0.22)),     # chest
    ("ell", (0, 0.11, 0.39), (0.25, 0.27, 0.22)),      # a round little tummy
    ("ell", (0, 0.34, 0.38), (0.2, 0.21, 0.19)),       # hips
    ("chain", [(0, 0.50, 0.37), (0, 0.66, 0.35), (0, 0.84, 0.34), (0, 1.02, 0.345), (0, 1.16, 0.355),
               (0, 1.25, 0.36)],
     [0.15, 0.12, 0.095, 0.07, 0.05, 0.035]),          # the tadpole's tail
]
for _s in (-1, 1):
    HATCH_META += [
        ("ball", (_s * 0.2, -0.66, 0.8), 0.125),                     # round cheeks
        ("chain", [(_s * 0.19, -0.14, 0.30), (_s * 0.23, -0.18, 0.18), (_s * 0.24, -0.21, 0.09)],
         [0.09, 0.08, 0.075]),
        ("ell", (_s * 0.25, -0.27, 0.05), (0.095, 0.115, 0.055)),    # front paws
        ("ell", (_s * 0.2, 0.33, 0.30), (0.12, 0.15, 0.14)),         # thighs
        ("chain", [(_s * 0.23, 0.33, 0.18), (_s * 0.24, 0.36, 0.09)], [0.085, 0.075]),
        ("ell", (_s * 0.25, 0.29, 0.05), (0.095, 0.12, 0.055)),      # hind paws
    ]

HATCH = dict(
    name="hatchling", nodes=HATCH_NODES, meta=HATCH_META, body="meta",
    body_tris=1300, body_tris_lod1=500, export_scale=1.0,
    young={
        "bones": {name: ((0.74, 0.74) if name in ("head", "snout") else (0.62, 0.62))
                  for name in ("hips", "belly", "chest", "neck1", "neck2", "neck3", "head", "snout", "tail1",
                               "tail2", "tail3", "tail4", "arm_up", "arm_lo", "hand", "leg_up", "leg_lo", "foot")},
        "parts": {"eyes": 1.12, "horns": 0.75, "frill": 0.85, "wings": 0.6, "spikes": 0.8,
                  "tail_tip": 0.8, "heart": 1.0, "runes": 0.85},
    },
    # The baby's leaf buds turned up and out off its round body (run 17: folded as the adult's,
    # a round baby's wings sink into its back); the tadpole tail curls round its side.
    base_pose={"head": (4, 0, 0), "tail1": (2, 0, 8), "tail2": (2, 0, 16), "tail3": (4, 0, 20),
               "tail4": (8, 0, 22), "wing_arm_R": (-20, 0, -55), "wing_arm_L": (-20, 0, 55)},
    builds={"neutral": {}, "sturdy": {}, "sleek": {}, "long": {}},
    eyes=dict(at=(0.19, -0.74, 0.9), out=(0.46, -0.86, 0.12), iris=(0.118, 0.132, 0.066),
              pupil=(0.082, 0.096, 0.03), slit=(0.32, 1.1),
              glints=((-0.028, 0.045, 0.028), (0.024, -0.042, 0.013)), seg=(14, 3, 12, 2)),
    head=dict(origin=(0, -0.50, 0.86), k=0.7, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=True,
              frill_k=0.7, feather_w=1.0),
    tail_k=0.34,
    heart=dict(at=(0, -0.33, 0.42), size=0.072),
    wing=dict(root=(0.13, -0.08, 0.58), scale=0.17, dihedral=40, droop=4,
              radii={"root": 0.03, "elbow": 0.024, "wrist": 0.02, "finger": 0.009, "tip": 0.004},
              arm_tris=64, thickness=0.008, style="classic"),
    mask=dict(max_x=0.24, max_z=0.95, min_z=0.1, tail_cut=None),
    inset={"eyes": 0.03, "horns": 0.008, "spikes": 0.012, "frill": 0.02, "tail_tip": 0.015, "heart": -0.03,
           "runes": 0.045},
    face=dict(nostril=(0.042, -0.9, 0.79), nostril_r=(0.018, 0.012, 0.007), mouth_r=0.01,
              mouth=lambda side, a: (side * 0.16 * a ** 0.8, -0.905 + 0.2 * a ** 1.6, 0.715 + 0.04 * a * a)),
    jaw_hinge=(0, -0.66, 0.71),
    mouth_detail=dict(depth=0.13, fade=0.1, width=0.3, tooth=(0.007, 0.012), fang=(0.01, 0.02),
                      tongue=(0.05, 0.075, 0.012), fangs=[]),
    teeth=False,
    skin=dict(spot_cell=0.11, dapple=4.0, ao=0.22, face_y=-0.64),
)

FORMS = {"grown": GROWN, "hatchling": HATCH}


# ------------------------------------------------------------------------------ shapes
def _band(kit, name, base, edge, mats, slots=("membrane", "membrane"), frac=0.8, thickness=0.012):
    """A fin between a base line and an edge line (lists of points, one per column): the inner
    band in slots[0], the outer (frac..1 of the way out) in slots[1] on vertices of its own, so
    a glowing edge stays crisp. Two sheets (the game culls back faces); origin at base[0]."""
    import bmesh
    V = kit.V
    o = V(base[0])
    bm = bmesh.new()
    B = [bm.verts.new(V(p) - o) for p in base]
    M = [bm.verts.new(V(p).lerp(V(q), frac) - o) for p, q in zip(base, edge)]
    two = slots[1] != slots[0]
    M2 = [bm.verts.new(V(p).lerp(V(q), frac) - o) for p, q in zip(base, edge)] if two else M
    E = [bm.verts.new(V(q) - o) for q in edge]
    for i in range(len(base) - 1):
        bm.faces.new((B[i], B[i + 1], M[i + 1], M[i]))
    for i in range(len(base) - 1):
        bm.faces.new((M2[i], M2[i + 1], E[i + 1], E[i]))
    n_inner = len(base) - 1
    obj = kit.mesh_object(name, bm, o)
    m = obj.modifiers.new("thick", "SOLIDIFY")
    m.thickness = thickness
    m.offset = 0
    m.use_rim = False
    kit.apply_modifiers(obj)
    obj.data.materials.append(mats[slots[0]])
    if two:
        obj.data.materials.append(mats[slots[1]])
        # solidify doubles the faces: the copies follow the originals in the same order
        nf = len(obj.data.polygons)
        per = nf // 2
        for p in obj.data.polygons:
            if (p.index % per) >= n_inner:
                p.material_index = 1
    kit.smooth(obj)
    return obj


LEAF = [(0.12, 0.62), (0.32, 1.0), (0.55, 0.9), (0.76, 0.6), (0.92, 0.26)]
PETAL = [(0.3, 0.92), (0.68, 0.74)]
LOW = [(0.3, 1.0), (0.7, 0.62)]


def _leaf_outline(kit, base, direction, side, length, width, prof=None):
    """A pointed leaf's rim (base, one side to the tip, back down the other), widest a third of
    the way out: [points], the base first. prof: [(t along, half-width share)]."""
    V = kit.V
    d = V(direction).normalized()
    s = (V(side) - d * V(side).dot(d)).normalized()
    b = V(base)
    prof = prof or kit.lod(LEAF, LOW)
    right = [b + d * (length * t) + s * (width * 0.5 * w) for t, w in prof]
    left = [b + d * (length * t) - s * (width * 0.5 * w) for t, w in reversed(prof)]
    return [b] + right + [b + d * length] + left


def _fern(kit, base, direction, side, length, width, lobes=4):
    """A fern frond's rim: a midrib from base with `lobes` pairs of rounded side lobes shrinking
    toward a pointed tip (a single leaf at LOD1): [points], the base first."""
    V = kit.V
    d = V(direction).normalized()
    s = (V(side) - d * V(side).dot(d)).normalized()
    b = V(base)
    if kit.LOD:
        return _leaf_outline(kit, base, direction, side, length, width)
    right, left = [], []
    for k in range(lobes):
        f0 = 0.1 + 0.74 * k / lobes
        f1 = 0.1 + 0.74 * (k + 1) / lobes
        w = width * 0.5 * (1.0 - 0.5 * k / lobes)
        for pts, sg in ((right, 1), (left, -1)):  # a soft round lobe, then a shallow notch
            pts += [b + d * (length * (f0 + (f1 - f0) * 0.3)) + s * (sg * w * 0.86),
                    b + d * (length * (f0 + (f1 - f0) * 0.62)) + s * (sg * w),
                    b + d * (length * (f1 + 0.01)) + s * (sg * w * 0.5)]
    return [b, b + s * width * 0.14] + right + [b + d * length] + list(reversed(left)) + [b - s * width * 0.14]


# ------------------------------------------------------------------------------ wings
# Small leafy wings in their own layout (u out along the span, v back along the chord), made for
# the Pouncer plan's fold (the classic clips' WINGS_FOLDED, see kinds/blazeplume.py): the arm
# points straight out, the forearm reaches forward to the wrist, and the four fingers fan about
# 34 degrees apart round the wrist, so folded they all point back together and the four leaves
# they carry close into one neat bundle along the back.
def _polar(c, ang, r):
    return (c[0] + r * math.cos(math.radians(ang)), c[1] + r * math.sin(math.radians(ang)))


WRIST = (1.02, -0.4)
WING_LAYOUT = {"root": (0.0, 0.0), "elbow": (0.64, 0.03), "wrist": WRIST, "body": (0.0, 0.3)}
for _i, (_a, _r) in enumerate([(4, 1.9), (38, 2.05), (72, 1.9), (104, 1.6)]):
    WING_LAYOUT[f"f{_i + 1}"] = _polar(WRIST, _a, _r)
for _form in (GROWN, HATCH):
    _form["wing"]["layout"] = WING_LAYOUT


def _wing_normal(kit, side):
    w = kit.F["wing"]
    s = -1 if side == "L" else 1
    th, ph = math.radians(w["dihedral"]), math.radians(w["droop"])
    span = kit.V((s * math.cos(th), 0, math.sin(th)))
    chord = kit.V((0, math.cos(ph), -math.sin(ph)))
    n = span.cross(chord).normalized()
    return n if n.z > 0 else -n


def wings(kit, d, rare):
    """Leafy wings: a slender green arm and four vein-fingers, each carrying a pointed leaf
    (cupped a little along its midrib, each lying just over the next like shingles)."""
    if rare:
        return []
    F, mats = kit.F, d["mats"]
    wr = F["wing"]["radii"]
    baby = d["form"] == "hatchling"
    objs = []
    for side in ("L", "R"):
        w = kit.wing_points(side)
        arm = kit.wing_arm(side, ["root", "elbow", "wrist", "f1", "f2", "f3", "f4"],
                           [wr["root"], wr["elbow"], wr["wrist"]] + [wr["finger"]] * 4,
                           kit.lod(F["wing"]["arm_tris"], 40), mats, material="membrane", claws=False)
        objs.append(arm)
        n = _wing_normal(kit, side)
        leaves = []
        for k, f in enumerate(("f1", "f2", "f3", "f4")):
            if kit.LOD and baby and k == 3:
                continue
            a, b = w["wrist"], w[f]
            dvec = b - a
            length = dvec.length * 1.04
            side_v = n.cross(dvec).normalized()
            base = a + dvec.normalized() * length * 0.02 + n * (0.004 + 0.006 * (3 - k)) * (1 if not baby else 0.5)
            pts = _leaf_outline(kit, base, dvec, side_v, length, length * (0.44 if not baby else 0.56))
            cup = length * 0.05
            pts = [pts[0]] + [p + n * cup * min(1.0, ((p - base) - dvec.normalized() * (p - base).dot(
                dvec.normalized())).length / (length * 0.22)) ** 2 for p in pts[1:]]
            leaf = kit.flat_fan(f"leaf_{side}_{k}", pts, F["wing"]["thickness"])
            leaf.data.materials.append(mats["membrane"])
            leaves.append(leaf)
        objs.append(kit.join(leaves, f"leaves_{side}"))
    return objs


# ------------------------------------------------------------------------------ parts
def _skin_top(kit, d, p, up):
    """Where the body's skin is, out from p along `up` (the rest pose): a ray cast back in."""
    import bmesh
    from mathutils.bvhtree import BVHTree
    if "bvh" not in d:
        bm = bmesh.new()
        bm.from_mesh(d["body"].data)
        d["bvh"] = BVHTree.FromBMesh(bm)
        bm.free()
    V = kit.V
    hit, _, _, _ = d["bvh"].ray_cast(V(p) + V(up) * 2.0, -V(up), 4.0)
    return hit if hit is not None else V(p)


def _hat(kit, d, mats, rare, baby):
    """The lily pad worn as a hat, notched at the back and tipped back a little, its rim turned up
    a touch as a floating pad's is, with a lotus in the middle (the rare one's: a bigger pad, an
    open glowing lotus and a glowing bud beside it; the baby's: a little pad with a lotus bud).
    Flat or turned up, it only touches the round crown at its middle, so nothing sinks in."""
    V = kit.V
    hk = kit.F["head"]["k"]
    R = (0.46 if not baby else 0.5) * hk * (1.12 if rare else 1.0)
    seg = kit.lod(18, 10)
    notch = math.radians(34)
    tilt = math.radians(-8)  # tipped back
    # the pad: a fan from its centre round to the notch (facing the tail), the rim turned up
    pts = [V((0, 0, 0.0))]
    for j in range(seg + 1):
        a = math.pi / 2 + notch / 2 + (2 * math.pi - notch) * j / seg
        r = R * (1.0 if j not in (0, seg) else 0.98)
        pts.append(V((math.cos(a) * r, math.sin(a) * r, 0.05 * R)))
    import mathutils
    tip_m = mathutils.Matrix.Rotation(tilt, 3, "X")
    pts = [tip_m @ p for p in pts]
    thick = (0.02 if not baby else 0.014) * hk
    pad = kit.flat_fan("pad", pts, thick)
    pad.data.materials.append(mats["membrane"])
    pieces = [pad]
    if not kit.LOD:  # a lily pad's veins: thin lines radiating from the middle, in the pattern colour
        lift = thick * 0.5 + 0.004 * hk
        for j in range(7):
            a = math.pi / 2 + notch / 2 + (2 * math.pi - notch) * (j + 0.5) / 7
            out_v = V((math.cos(a), math.sin(a), 0.05))
            side = V((-math.sin(a), math.cos(a), 0))
            w = R * 0.022
            vein = [V((0, 0, lift)) + out_v * R * 0.1, V((0, 0, lift)) + out_v * R * 0.84 + side * w * 0.3,
                    V((0, 0, lift)) + out_v * R * 0.84 - side * w * 0.3]
            vein = [vein[0] + side * w, vein[0] - side * w, vein[2], vein[1]]
            vo = kit.flat_fan(f"vein_{j}", [tip_m @ p for p in vein], 0.002)
            vo.data.materials.append(mats["pattern_flat"])
            pieces.append(vo)
    up = tip_m @ V((0, 0, 1))
    centre = up * (0.012 * hk)
    pieces += _lotus(kit, mats, centre, up, (0.2 if not baby else 0.16) * hk * (1.15 if rare else 1.0),
                     "glow_flat" if rare else "horn", open_=not baby or rare, rare=rare)
    if rare and not baby:  # a glowing bud beside it
        bud_at = tip_m @ V((R * 0.5, R * 0.38, 0.0))
        pieces += _lotus(kit, mats, bud_at, (up + V((0.3, 0.2, 0))).normalized(), 0.09 * hk, "glow_flat",
                         open_=False, rare=True)
    hat = kit.join(pieces, f"hat{'r' if rare else ''}")
    # on the crown (2 mm off the midline: a ray on it can slip through the seam)
    hat.location = kit.head_point((0.002, 0.16, 0.3) if not baby else (0.002, 0.04, 0.3))
    return hat


def _lotus(kit, mats, centre, up, size, petal, open_=True, rare=False):
    """A lotus: an outer ring of broad pointed petals leaning out, an inner ring standing up and
    a round heart; closed (open_=False), a bud of petals held together."""
    import bmesh
    V = kit.V
    c, u = V(centre), V(up).normalized()
    a = V((1, 0, 0)) if abs(u.x) < 0.9 else V((0, 1, 0))
    e1 = u.cross(a).normalized()
    e2 = u.cross(e1).normalized()
    rings = [(kit.lod(8, 5), 38 if open_ else 12, 1.0, 0.0), (kit.lod(6, 4), 62 if open_ else 8, 0.78, 0.5)]
    if not open_:
        rings = [(kit.lod(4, 3), 14, 1.0, 0.0), (kit.lod(3, 2), 6, 0.86, 0.5)]
    out = []
    for ri, (n, lean, k, phase) in enumerate(rings):
        for j in range(n):
            ang = 2 * math.pi * (j + phase) / n
            radial = e1 * math.cos(ang) + e2 * math.sin(ang)
            # petal direction: up, leaning out by `lean` degrees
            dirn = (u * math.cos(math.radians(lean)) + radial * math.sin(math.radians(lean))).normalized()
            side = u.cross(radial).normalized()
            base = c + radial * size * 0.08 + u * size * 0.04 * ri
            length = size * k * (1.0 if open_ else 1.25)
            pts = _leaf_outline(kit, base, dirn, side, length, length * (0.62 if open_ else 0.5),
                                prof=PETAL if not kit.LOD else [(0.45, 0.9)])
            # cup each petal toward the flower's middle
            pts = [pts[0]] + [p - radial * length * 0.06 * ((p - base).length / length) ** 2 for p in pts[1:]]
            fl = kit.flat_fan(f"petal_{ri}_{j}", pts, size * 0.035)
            fl.data.materials.append(mats[petal])
            out.append(fl)
    bm = bmesh.new()
    kit.add_dome(bm, c + u * size * 0.05, u, e1, size * 0.2, size * 0.2, size * 0.16, kit.lod(8, 5), 1, 0)
    heart = kit.mesh_object("lotus_heart", bm)
    heart.data.materials.append(mats["accent_flat" if not rare else "glow_flat"])
    kit.smooth(heart)
    out.append(heart)
    return out


def _fronds(kit, d, mats, rare, baby):
    """Fern fronds round the back of the head, three a side (two on the baby), sweeping back
    like an axolotl's gills: the top one up and back, the middle straight back, the lowest back
    and down (the rare one's longer, glowing at the tips)."""
    V = kit.V
    hk = kit.F["head"]["k"]
    k = 1.18 if rare else 1.0
    pieces = []
    if baby:
        spec = [((0.7, 0.52, 0.5), 0.36, 0.2), ((0.8, 0.56, -0.16), 0.31, 0.18)]
        at = (0.28, 0.12, 0.04)
    else:
        spec = [((0.4, 0.78, 0.42), 0.56, 0.24), ((0.75, 0.62, 0.1), 0.62, 0.25), ((0.62, 0.55, -0.6), 0.46, 0.21)]
        at = (0.3, 0.2, -0.02)
    for s in (-1, 1):
        base = kit.head_point(at, s)
        objs = []
        for j, (dirn, length, width) in enumerate(spec):
            dv = V((s * dirn[0], dirn[1], dirn[2])).normalized()
            # the frond's face turned outward and a little forward, so it reads from the side and
            # from the front
            side = V((s * 0.9, -0.35, 0.15)).normalized().cross(dv).normalized()
            pts = _fern(kit, base + dv * 0.02 * hk, dv, side, length * hk * k, width * hk * k, lobes=3 if not baby else 2)
            # a soft curl: the tip bends out and back
            pts = [pts[0]] + [p + V((s * 0.12, 0.05, 0)) * ((p - base).length / (length * hk)) ** 2 * length * hk
                              for p in pts[1:]]
            fr = kit.flat_fan(f"frond_{s}_{j}", pts, (0.014 if not baby else 0.01) * hk)
            if rare:  # the outer third of each frond glows
                fr.data.materials.append(mats["membrane"])
                fr.data.materials.append(mats["glow_flat"])
                for p in fr.data.polygons:
                    c = fr.matrix_world @ p.center
                    if (c - base).length > length * hk * k * 0.64:
                        p.material_index = 1
            else:
                fr.data.materials.append(mats["membrane"])
            objs.append(fr)
        # one piece a side, its origin the shared root (seated on the skin behind the cheek)
        o = kit.join(objs, f"fronds{'r' if rare else ''}_{s}")
        pieces.append((o, "head"))
    return pieces


def _tail_fins(kit, d, mats, rare, baby):
    """The paddle tail's fins: a leaf-edged ribbon along the top of the tail from its root to
    its end, rising toward the end, and a shorter one underneath the back half; one piece per
    tail bone, each overlapping the next so the ribbon stays whole as the tail sways."""
    V = kit.V
    slots = ("membrane", "glow_flat" if rare else "membrane")
    k = 1.15 if rare else 1.0
    if baby:  # a tadpole's fin, all round the tail
        top = [("hips", "tail2", "tail1", 0.07), ("tail2", "tail3", "tail2", 0.1), ("tail3", "tail4", "tail3", 0.11),
               ("tail4", "tail_tip", "tail4", 0.1)]
        under = [("tail2", "tail3", "tail2", 0.06), ("tail3", "tail4", "tail3", 0.08), ("tail4", "tail_tip", "tail4", 0.08)]
    else:
        top = [("hips", "tail2", "tail1", 0.16), ("tail2", "tail3", "tail2", 0.26), ("tail3", "tail4", "tail3", 0.34),
               ("tail4", "tail_tip", "tail4", 0.32)]
        under = [("tail3", "tail4", "tail3", 0.2), ("tail4", "tail_tip", "tail4", 0.26)]
    pieces = []
    for label, path, sign in (("top", top, 1), ("under", under, -1)):
        for i, (a, b, bone, h) in enumerate(path):
            pa0, pb0 = kit.node(a), kit.node(b)
            spine = pb0 - pa0
            up = V((0, -spine.z, spine.y)).normalized()
            if up.z < 0:
                up = -up
            up = up * sign
            pa, pb = _skin_top(kit, d, pa0, up), _skin_top(kit, d, pb0, up)
            along = pb - pa
            end = pa + along * 1.16
            n = kit.lod(4, 2)
            base, edge = [], []
            hh = h * k
            for j in range(n + 1):
                f = j / n
                # a leaf-tipped lobe: rising from the front, fullest past the middle, a soft point
                # swept back at the end
                prof = math.sin(math.pi * min(1.0, 0.1 + 0.9 * f)) ** 0.7 * (0.55 + 0.45 * f)
                q = pa.lerp(end, f)
                base.append(q - up * 0.02)
                edge.append(q + up * hh * prof + along.normalized() * hh * prof * 0.45)
            o = _band(kit, f"fin{label}{'r' if rare else ''}_{i}", base, edge, mats, slots, frac=0.8,
                      thickness=0.012 if not baby else 0.008)
            pieces.append((o, bone))
    return pieces


def _paddle(kit, d, mats, rare, baby):
    """The tail's end: a leaf-shaped paddle standing upright (a newt's), carrying the tail on."""
    V = kit.V
    x, y, z = kit.F["nodes"]["tail_tip"][0]
    k = kit.F["tail_k"] * (1.15 if rare else 1.0)
    base = V((0, y - 0.26 * kit.F["tail_k"], z))
    length, width = 1.0 * k, 0.62 * k
    if baby:
        length, width = 0.95 * k, 0.62 * k
    # the outline: a pointed leaf lying back along the tail, a little up
    dirn = V((0, 1.0, 0.1)).normalized()
    pts = _leaf_outline(kit, base, dirn, V((0, 0, 1)), length, width)
    base_line, edge_line = [], []
    # as a band fin (glowing edge on the rare): each rim point paired with its foot on the midrib
    for p in pts + [pts[0]]:
        t = max(0.0, min(1.0, (p - base).dot(dirn) / length))
        base_line.append(base + dirn * (length * t))
        edge_line.append(p)
    o = _band(kit, f"paddle{'r' if rare else ''}", base_line, edge_line, mats,
              ("membrane", "glow_flat" if rare else "membrane"), frac=0.82, thickness=0.016 * max(k, 0.5))
    return [(o, "tail4")]


def _webs(kit, d, mats, baby):
    """Webbed toes: four round toes on each foot joined by a web, fanning out of the front of the
    foot at mid-height (the snap seats the web's middle a little inside the toe's front, the
    group's inset): [(object, bone)]."""
    V = kit.V
    k = 0.42 if baby else 1.0
    pieces = []
    for s in (-1, 1):
        side = "R" if s > 0 else "L"
        for bone, toe in ((f"hand_{side}", f"toe_f_{side}"), (f"foot_{side}", f"toe_b_{side}")):
            c = kit.node(toe)
            pts = [c]
            n = kit.lod(4, 3)
            toes = [-56, -19, 19, 56] if n == 4 else [-44, 0, 44]
            for j, a in enumerate(toes):
                ang = math.radians(a + s * 8)
                dv = V((math.sin(ang), -math.cos(ang), 0))
                lr = 0.18 * k * (1.0 if abs(a) < 30 else 0.86)
                w = 0.056 * k
                sd = V((-dv.y, dv.x, 0))
                if j > 0:  # the web between two toes, a little way out
                    prev = toes[j - 1]
                    mid = math.radians((a + prev) / 2 + s * 8)
                    pts.append(c + V((math.sin(mid), -math.cos(mid), 0)) * lr * 0.62)
                pts += [c + dv * lr * 0.86 - sd * w, c + dv * lr + dv * w * 0.4, c + dv * lr * 0.86 + sd * w]
            web = kit.flat_fan(f"web_{side}_{bone}", pts, 0.02 * k)
            web.data.materials.append(mats["body_plain"])
            pieces.append((web, bone))
    return pieces


def parts(kit, d):
    mats = d["mats"]
    baby = d["form"] == "hatchling"
    out = []
    # The lily-pad hat (group horns: the rare one's replaces it).
    for v, rare in ((0, False), (1, True)):
        out.append(("horns", v, [(_hat(kit, d, mats, rare, baby), "head")]))
    # Fern-frond gills (group frill).
    for v, rare in ((0, False), (1, True)):
        out.append(("frill", v, _fronds(kit, d, mats, rare, baby)))
    # The paddle tail's fins (group spikes) and its paddle (tail_tip).
    for v, rare in ((0, False), (1, True)):
        out.append(("spikes", v, _tail_fins(kit, d, mats, rare, baby)))
        out.append(("tail_tip", v, _paddle(kit, d, mats, rare, baby)))
    # Webbed toes (group runes).
    out.append(("runes", 0, _webs(kit, d, mats, baby)))
    return out


def accent(kit, body):
    """A pale belly from the chin down the throat (a stripe down the front of the neck), chest and
    belly to under the tail; the legs and feet stay the body's colour."""
    import mathutils
    mask = kit.mask_attr(body)
    F = kit.F
    baby = F["name"] == "hatchling"
    down = mathutils.Vector((0, 0, -1))
    fwd = mathutils.Vector((0, -1, 0))
    neck_y = -0.22 if baby else -0.7
    for v in body.data.vertices:
        x, y, z = v.co
        n = v.normal
        a = min(1.0, max(0.0, (n.dot(down) - 0.25) / 0.35))
        if y < neck_y:  # the throat and chest face forward as the neck rises (a stripe down its front)
            w = 0.16 if baby else 0.26
            a = max(a, min(1.0, max(0.0, (n.dot((fwd * 0.75 + down * 0.66).normalized()) - 0.3) / 0.3))
                    * min(1.0, max(0.0, (w - abs(x)) / (w * 0.35))))
        if z < (0.14 if baby else 0.36) and abs(x) > (0.13 if baby else 0.3):  # the legs and feet
            a = 0.0
        mask.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


def texture(tx, nt, p, form):
    """R: round dapples of two sizes over the back and flanks; G: a mottle of soft blotches and
    small spots; B: fine speckles and a row of dots down each flank (the lateral line). All stop
    short of the muzzle, so the face stays clean."""
    c = p["spot_cell"]
    body = tx.band(nt, 1, p["face_y"], 10.0, soft=c * 0.5)
    upper, top = tx.mul(nt, tx.upper(nt), body), tx.mul(nt, tx.top(nt), body)
    big = tx.spots(nt, c * 1.6, keep=0.5, size=(0.3, 0.2), where=upper)
    small = tx.spots(nt, c, keep=0.55, size=(0.26, 0.16), where=upper, seed_offset=1.7)
    r = tx.maxi(nt, big, small)
    g = tx.maxi(nt, tx.blotches(nt, p["dapple"], threshold=(0.53, 0.6), where=top),
                tx.spots(nt, c * 0.8, keep=0.6, size=(0.22, 0.14), where=upper, seed_offset=2.9))
    speck = tx.spots(nt, c * 0.5, keep=0.55, size=(0.2, 0.1), where=upper, seed_offset=4.3)
    # the lateral line: a band along mid-flank, broken into dots
    flank = tx.mul(nt, tx.smoothstep(nt, tx.normal_z(nt), -0.35, -0.1), tx.smoothstep(nt, tx.normal_z(nt), 0.35, 0.1))
    dots = tx.spots(nt, c * 0.45, keep=0.15, size=(0.26, 0.14), where=tx.mul(nt, flank, body), seed_offset=6.1)
    b = tx.maxi(nt, speck, dots)
    return {"r": r, "g": g, "b": b}
