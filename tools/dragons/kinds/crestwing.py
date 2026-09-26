"""The Crestwing (Gale, uncommon): a feathered, elegant dragon, a crane or a peacock made dragon.
Concept: docs/art/concept/dragons/dragon_crestwing.jpg (R11). Plan: plans/crestwing.py.

A dragon, not a bird: a dragon's rounded snout with nostrils and a gentle smile, a smooth
scaled body, four legs and a long tail. Feathers only where a dragon would wear them: a crest
on the head (on its own bone, so it rises when it is pleased or curious and lies flat when it
is sulking or being petted), feathered wings, and a fan of plumes at the tail's end.

  Hatchling  a round, downy baby: a big round head (a third of it), huge dark eyes, a short
             button muzzle, a spiky tuft of down on the crown and a ruff of down round the
             back of the head, stubby legs, tiny folded wing buds, a little golden tail tuft.
  Grown      tall and elegant (META size 1.2): long legs (strong hind legs, slimmer front
             ones), a short deep body, a long neck held in a proud S, a small rounded head with
             big kind eyes, a fan of feather-like spines swept up and back, cheek feathers and
             a few small feathers down the nape; a long tail ending in a cascading fan of long
             golden plumes, each a narrow shaft opening into a rounded paddle.
  Wings      bony arms (a short forearm, a little wrist claw) and a fan of flight feathers
             radiating from the wrist: long rounded primaries, secondaries and tertials whose
             scalloped trailing edge is the wing's edge, under a thicker sheet of coverts in
             the covert colour. The four "finger" bones are feather rays spread across the fan,
             so the fan closes and the wing folds down over the back and flanks like a bird's
             (coverts along the top, the golden tips toward the tail), and opens by lifting as
             a closed fan and spreading.
  Colour     base (body), accent (belly, throat, chin), pattern (R soft bars, G a darker
             mantle over the back, neck and head, B fawn-like spots along the back), horn (the
             flight feathers and plumes), membrane (the coverts and the crest).
  Variants   Skyfeather: slate teal-blue with golden feathers (the concept). Dawnplume: warm
             peach with rose feathers and pale fawn spots. Stormcrest: storm grey with silver
             feathers and soft charcoal bars. Starplume (rare): midnight violet with a lavender
             belly, turquoise feathers and moonlit eyes; its spots glow like stars, its bigger
             crest has glowing tips and its longer, seven-plumed peacock fan has a glowing eye
             on every plume.
  Egg        tall and pointed, pale and speckled like a bird's: robin-blue with brown specks,
             blush with rose, pearl with slate, lavender with gold.
"""
import math


def _mirrored(center, sides):
    nodes = dict(center)
    for name, (p, r) in sides.items():
        nodes[f"{name}_R"] = ((p[0], p[1], p[2]), r)
        nodes[f"{name}_L"] = ((-p[0], p[1], p[2]), r)
    return nodes


META = dict(
    name="crestwing", title="Crestwing", dex=4, element="Gale", parents=(), rarity="uncommon",
    plan="crestwing", size=1.2,
    stats=dict(wing=9, wit=6, might=3, breath=6, stamina=7),
    manners=("Proud", "Gentle", "Curious", "Shy"),
    traits=("Tidy", "Showoff", "Strong Wings", "Swift", "Skydancer"),
    rare_variant=3, rare_replaces=True,
    blurb="A proud strutter that preens every feather, then glides for miles on the lightest breeze.",
)

GALE = (0.62, 0.92, 1.0)
VARIANTS = [
    dict(name="Skyfeather", base=(0.10, 0.27, 0.36), accent=(0.55, 0.74, 0.74), pattern=(0.04, 0.13, 0.22),
         horn=(1.0, 0.66, 0.16), membrane=(0.13, 0.33, 0.50), iris=(0.17, 0.085, 0.035), glow=GALE,
         pattern_channel="g", glow_channel=None),
    dict(name="Dawnplume", base=(0.95, 0.50, 0.38), accent=(1.0, 0.86, 0.74), pattern=(1.0, 0.86, 0.76),
         horn=(0.98, 0.42, 0.50), membrane=(1.0, 0.66, 0.52), iris=(0.40, 0.16, 0.20), glow=(1.0, 0.80, 0.70),
         pattern_channel="b", glow_channel=None),
    dict(name="Stormcrest", base=(0.34, 0.37, 0.42), accent=(0.78, 0.80, 0.84), pattern=(0.14, 0.15, 0.19),
         horn=(0.82, 0.86, 0.92), membrane=(0.24, 0.27, 0.33), iris=(0.26, 0.48, 0.66), glow=GALE,
         pattern_channel="r", glow_channel=None),
    dict(name="Starplume", base=(0.035, 0.04, 0.14), accent=(0.46, 0.44, 0.78), pattern=(0.10, 0.06, 0.30),
         horn=(0.06, 0.52, 0.52), membrane=(0.08, 0.10, 0.34), iris=(0.36, 0.16, 0.62), glow=(0.70, 0.95, 1.0),
         pattern_channel="g", glow_channel="b"),
]

EGG = dict(height=1.08, width=0.335, asym=0.24, speckle="spots",
           speckle_params=dict(count=40, size=(0.012, 0.034), count_lod1=8),
           colors=[((0.78, 0.92, 0.96), (0.36, 0.22, 0.14)), ((1.0, 0.90, 0.84), (0.80, 0.36, 0.40)),
                   ((0.88, 0.89, 0.90), (0.26, 0.27, 0.32)), ((0.62, 0.62, 0.86), (1.0, 0.82, 0.34))])

# ------------------------------------------------------------------------------ grown
# Faces -Y, Z up. radius = (side, vertical). A tall body on long legs, a long neck.
GROWN_NODES = _mirrored({
    "tail_tip": ((0, 2.46, 0.94), (0.06, 0.06)),
    "tail4": ((0, 2.12, 1.28), (0.12, 0.12)),
    "tail3": ((0, 1.62, 1.62), (0.18, 0.18)),
    "tail2": ((0, 1.02, 1.84), (0.26, 0.26)),
    "hips": ((0, 0.42, 1.78), (0.50, 0.66)),
    "belly": ((0, 0.04, 1.72), (0.50, 0.70)),
    "chest": ((0, -0.36, 1.76), (0.62, 0.78)),
    "neck1": ((0, -0.74, 2.18), (0.39, 0.43)),
    "neck2": ((0, -0.86, 2.52), (0.27, 0.30)),
    "neck3": ((0, -0.90, 2.82), (0.23, 0.25)),
    "neck4": ((0, -0.92, 3.08), (0.22, 0.23)),
    "head": ((0, -0.99, 3.33), (0.37, 0.36)),
    "muzzle": ((0, -1.25, 3.26), (0.215, 0.195)),
    "snout": ((0, -1.40, 3.21), (0.14, 0.12)),
    "crest": ((0, -0.96, 3.40), (0.01, 0.01)),
    "crest_tip": ((0, -0.83, 3.58), (0.01, 0.01)),
}, {
    "shoulder": ((0.29, -0.34, 1.54), (0.25, 0.32)),
    "elbow": ((0.31, -0.40, 0.96), (0.15, 0.15)),
    "wrist": ((0.31, -0.43, 0.27), (0.105, 0.105)),
    "toe_f": ((0.32, -0.60, 0.06), (0.12, 0.07)),
    "hipj": ((0.30, 0.46, 1.56), (0.34, 0.44)),
    "knee": ((0.34, 0.14, 1.02), (0.19, 0.19)),
    "ankle": ((0.34, 0.58, 0.38), (0.12, 0.12)),
    "toe_b": ((0.35, 0.36, 0.065), (0.13, 0.078)),
})
GROWN_EDGES = [("tail_tip", "tail4"), ("tail4", "tail3"), ("tail3", "tail2"), ("tail2", "hips"),
               ("hips", "belly"), ("belly", "chest"), ("chest", "neck1"), ("neck1", "neck2"),
               ("neck2", "neck3"), ("neck3", "neck4"), ("neck4", "head"), ("head", "muzzle"), ("muzzle", "snout")]
for _side in ("L", "R"):
    GROWN_EDGES += [("chest", f"shoulder_{_side}"), (f"shoulder_{_side}", f"elbow_{_side}"),
                    (f"elbow_{_side}", f"wrist_{_side}"), (f"wrist_{_side}", f"toe_f_{_side}"),
                    ("hips", f"hipj_{_side}"), (f"hipj_{_side}", f"knee_{_side}"),
                    (f"knee_{_side}", f"ankle_{_side}"), (f"ankle_{_side}", f"toe_b_{_side}")]

BUILDS = {
    "neutral": {},
    "sturdy": {"chest": (1.07, 0.98), "hips": (1.06, 0.98), "neck1": (1.06, 0.96), "leg_up": (1.06, 0.97),
               "arm_up": (1.05, 0.97)},
    "sleek": {"chest": (0.94, 1.02), "hips": (0.94, 1.02), "leg_lo": (0.95, 1.04), "arm_lo": (0.95, 1.04),
              "neck2": (0.95, 1.03), "neck3": (0.95, 1.03)},
    "long": {"neck1": (0.97, 1.06), "neck2": (0.97, 1.07), "neck3": (0.97, 1.07), "neck4": (0.97, 1.06),
             "tail2": (0.96, 1.08), "tail3": (0.96, 1.08)},
}


def _islands(bm):
    """Connected vertex sets of a bmesh."""
    seen, out = set(), []
    for v in bm.verts:
        if v.index in seen:
            continue
        stack, comp = [v], []
        seen.add(v.index)
        while stack:
            a = stack.pop()
            comp.append(a)
            for e in a.link_edges:
                b = e.other_vert(a)
                if b.index not in seen:
                    seen.add(b.index)
                    stack.append(b)
        out.append(comp)
    return out


def _drop_small_islands(bm):
    """The crest bone's nodes are skin vertices too: keep only the body (the biggest island)."""
    bm.verts.index_update()
    isl = _islands(bm)
    if len(isl) > 1:
        keep = max(isl, key=len)
        drop = [v for comp in isl if comp is not keep for v in comp]
        import bmesh
        bmesh.ops.delete(bm, geom=drop, context="VERTS")


def _grown_sculpt(kit, obj):
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    _drop_small_islands(bm)
    bm.to_mesh(obj.data)
    bm.free()


GROWN = dict(
    name="grown", nodes=GROWN_NODES, edges=GROWN_EDGES, body="skin",
    body_tris=1540, body_tris_lod1=540, export_scale=1.0,
    young={
        "bones": {
            "head": (0.95, 0.9, 1.0), "snout": (0.9, 0.72, 0.95), "crest": (1.0, 1.0),
            "neck1": (0.78, 0.55), "neck2": (0.8, 0.52), "neck3": (0.82, 0.52), "neck4": (0.84, 0.55),
            "chest": (0.66, 0.54), "hips": (0.68, 0.54),
            "tail1": (0.66, 0.52), "tail2": (0.68, 0.5), "tail3": (0.72, 0.5), "tail4": (0.8, 0.54),
            "arm_up": (0.7, 0.56), "arm_lo": (0.72, 0.56), "hand": (0.8, 0.72),
            "leg_up": (0.7, 0.56), "leg_lo": (0.72, 0.55), "foot": (0.8, 0.72),
        },
        "parts": {"eyes": 1.18, "horns": 0.5, "frill": 0.7, "wings": 0.45, "spikes": 0.6,
                  "tail_tip": 0.6, "heart": 0.85, "runes": 0.7},
    },
    young_pose={"neck1": -10, "neck2": -4, "head": 12},
    base_pose={"neck1": (-24, 0, 0), "neck2": (-4, 0, 0), "neck3": (18, 0, 0), "neck4": (18, 0, 0), "head": (-12, 0, 0),
               "tail1": (4, 0, 0), "tail2": (-3, 0, 4), "tail3": (-5, 0, 6), "tail4": (2, 0, 8)},
    builds=BUILDS,
    eyes=dict(at=(0.15, -1.15, 3.39), out=(0.64, -0.72, 0.16), iris=(0.095, 0.108, 0.052),
              pupil=(0.058, 0.074, 0.02), slit=(0.3, 1.12),
              glints=((-0.02, 0.03, 0.015), (0.015, -0.03, 0.007)), seg=(12, 2, 8, 1)),
    head=dict(origin=(0, -0.99, 3.33), k=1.0),
    tail_k=1.0,
    heart=dict(at=(0, -0.84, 1.80), size=0.1),
    relax=0.35,
    wing=dict(root=(0.25, -0.22, 2.28), scale=0.84, dihedral=48, droop=10,
              radii={"root": 0.075, "elbow": 0.058, "wrist": 0.048, "finger": 0.018, "tip": 0.008},
              arm_tris=96, thickness=0.012,
              layout={"root": (0.0, 0.0), "elbow": (0.62, 0.24), "wrist": (1.02, 0.06), "thumb": (1.08, -0.10),
                      "f1": (3.05, 0.52), "f2": (2.72, 1.45), "f3": (1.95, 2.02), "f4": (0.40, 1.80),
                      "body": (0.0, 0.12)},
              flight=[((0.62, 0.30), (0.10, 1.30), 0.40), ((0.82, 0.24), (0.45, 1.68), 0.40),
                      ((0.98, 0.16), (0.86, 1.92), 0.40), ((1.10, 0.10), (1.30, 2.02), 0.40),
                      ((1.20, 0.06), (1.74, 1.98), 0.40), ((1.30, 0.06), (2.14, 1.82), 0.38),
                      ((1.45, 0.09), (2.50, 1.54), 0.36), ((1.62, 0.12), (2.78, 1.18), 0.34),
                      ((1.85, 0.16), (2.98, 0.80), 0.32), ((2.10, 0.20), (3.10, 0.42), 0.28)],
              lead=[(1.75, 0.10), (2.40, 0.22)], covert=0.38),
    mask=dict(max_x=0.3, max_z=3.9, min_z=-1.0, tail_cut=(1.2, 1.4)),
    inset={"eyes": 0.02, "horns": 0.03, "spikes": 0.02, "frill": 0.03, "tail_tip": 0.0, "heart": -0.05,
           "runes": -0.012},
    face=dict(nostril=(0.036, -1.49, 3.245), nostril_r=(0.018, 0.012, 0.007), mouth_r=0.012,
              mouth=lambda side, a: (side * 0.11 * a ** 0.7, -1.515 + 0.30 * a ** 1.5, 3.165 + 0.045 * a * a)),
    jaw_hinge=(0, -1.11, 3.19),
    mouth_detail=dict(depth=0.2, fade=0.16, width=0.28, tooth=(0.008, 0.013), fang=(0.009, 0.018), fangs=[],
                      tongue=(0.038, 0.062, 0.008)),
    skin=dict(stripe=0.46, spot_cell=0.24, ao=0.5, plate=0.17),
    sculpt=_grown_sculpt,
)

# ------------------------------------------------------------------------------ hatchling
HATCH_NODES = _mirrored({
    "tail_tip": ((0, 0.98, 0.44), None),
    "tail4": ((0, 0.86, 0.38), None),
    "tail3": ((0, 0.72, 0.36), None),
    "tail2": ((0, 0.56, 0.40), None),
    "hips": ((0, 0.28, 0.50), None),
    "belly": ((0, 0.06, 0.50), None),
    "chest": ((0, -0.14, 0.56), None),
    "neck1": ((0, -0.22, 0.66), None),
    "neck2": ((0, -0.28, 0.75), None),
    "neck3": ((0, -0.33, 0.84), None),
    "neck4": ((0, -0.37, 0.93), None),
    "head": ((0, -0.43, 1.08), None),
    "muzzle": ((0, -0.70, 0.99), None),
    "snout": ((0, -0.82, 0.97), None),
    "crest": ((0, -0.42, 1.26), None),
    "crest_tip": ((0, -0.36, 1.42), None),
}, {
    "shoulder": ((0.17, -0.13, 0.42), None),
    "elbow": ((0.18, -0.17, 0.26), None),
    "wrist": ((0.19, -0.20, 0.10), None),
    "toe_f": ((0.19, -0.32, 0.05), None),
    "hipj": ((0.18, 0.28, 0.44), None),
    "knee": ((0.20, 0.22, 0.27), None),
    "ankle": ((0.21, 0.32, 0.12), None),
    "toe_b": ((0.21, 0.18, 0.05), None),
})

HATCH_META = [
    ("ell", (0, -0.44, 1.10), (0.36, 0.33, 0.33)),     # big round head
    ("ell", (0, -0.69, 0.99), (0.155, 0.14, 0.115)),   # a short, soft button of a muzzle
    ("ell", (0, -0.64, 0.93), (0.13, 0.12, 0.07)),     # chin
    ("chain", [(0, -0.22, 0.66), (0, -0.30, 0.79), (0, -0.37, 0.92)], [0.17, 0.165, 0.16]),
    ("ell", (0, -0.13, 0.56), (0.26, 0.24, 0.26)),     # chest
    ("ell", (0, 0.07, 0.50), (0.28, 0.27, 0.27)),      # round tummy
    ("ell", (0, 0.28, 0.52), (0.23, 0.21, 0.22)),      # hips
    ("ell", (0, 0.01, 0.40), (0.22, 0.24, 0.17)),
    ("chain", [(0, 0.44, 0.46), (0, 0.60, 0.40), (0, 0.74, 0.37), (0, 0.87, 0.39), (0, 0.98, 0.44)],
     [0.11, 0.085, 0.07, 0.058, 0.048]),
]
for _s in (-1, 1):
    HATCH_META += [
        ("ball", (_s * 0.16, -0.62, 0.97), 0.12),       # chubby cheeks
        ("chain", [(_s * 0.17, -0.13, 0.42), (_s * 0.18, -0.17, 0.26), (_s * 0.19, -0.20, 0.10)],
         [0.09, 0.08, 0.075]),
        ("ell", (_s * 0.19, -0.26, 0.06), (0.085, 0.11, 0.06)),    # front paws
        ("ell", (_s * 0.18, 0.28, 0.38), (0.13, 0.16, 0.17)),      # thighs
        ("chain", [(_s * 0.20, 0.32, 0.26), (_s * 0.21, 0.33, 0.12)], [0.085, 0.075]),
        ("ell", (_s * 0.21, 0.24, 0.06), (0.085, 0.12, 0.06)),     # hind paws
    ]

HATCH = dict(
    name="hatchling", nodes=HATCH_NODES, meta=HATCH_META, body="meta",
    body_tris=1600, body_tris_lod1=560, export_scale=1.0,
    young={
        "bones": {name: ((0.74, 0.74) if name in ("head", "snout", "crest") else (0.62, 0.62))
                  for name in ("hips", "chest", "neck1", "neck2", "neck3", "neck4", "head", "snout", "crest",
                               "tail1", "tail2", "tail3", "tail4", "arm_up", "arm_lo", "hand", "leg_up", "leg_lo",
                               "foot")},
        "parts": {"eyes": 1.12, "horns": 0.6, "frill": 0.8, "wings": 0.6, "spikes": 0.85,
                  "tail_tip": 0.85, "heart": 1.0, "runes": 0.9},
    },
    base_pose={"head": (4, 0, 0), "tail1": (6, 0, 0), "tail2": (4, 0, 10), "tail3": (8, 0, 12),
               "tail4": (12, 0, 14)},
    builds={"neutral": {}, "sturdy": {}, "sleek": {}, "long": {}},
    eyes=dict(at=(0.16, -0.70, 1.10), out=(0.45, -0.88, 0.10), iris=(0.13, 0.145, 0.07),
              pupil=(0.1, 0.116, 0.034), slit=(0.3, 1.1),
              glints=((-0.032, 0.05, 0.03), (0.028, -0.048, 0.014)), seg=(14, 3, 12, 2)),
    head=dict(origin=(0, -0.43, 1.08), k=1.0),
    tail_k=0.42,
    heart=dict(at=(0, -0.37, 0.52), size=0.075),
    wing=dict(root=(0.12, -0.06, 0.76), scale=0.19, dihedral=48, droop=10,
              radii={"root": 0.04, "elbow": 0.032, "wrist": 0.028, "finger": 0.011, "tip": 0.005},
              arm_tris=72, thickness=0.008,
              # the grown wing's layout (so the same folding keys fold it as neatly), fewer and
              # wider feathers: downy
              layout=GROWN["wing"]["layout"],
              flight=[(b, t, w * 1.7) for b, t, w in GROWN["wing"]["flight"][::2]] + [GROWN["wing"]["flight"][-1]],
              lead=GROWN["wing"]["lead"][:1], covert=0.45),
    mask=dict(max_x=0.2, max_z=1.0, min_z=0.13, tail_cut=None),
    inset={"eyes": 0.032, "horns": 0.02, "spikes": 0.012, "frill": 0.03, "tail_tip": 0.0, "heart": -0.03,
           "runes": -0.01},
    face=dict(nostril=(0.036, -0.835, 1.01), nostril_r=(0.02, 0.014, 0.008), mouth_r=0.01,
              mouth=lambda side, a: (side * 0.11 * a ** 0.8, -0.845 + 0.17 * a ** 1.6, 0.935 + 0.03 * a * a)),
    jaw_hinge=(0, -0.62, 0.93),
    mouth_detail=dict(depth=0.14, fade=0.1, width=0.26, tooth=(0.006, 0.01), fang=(0.008, 0.016), fangs=[],
                      tongue=(0.048, 0.075, 0.011)),
    skin=dict(stripe=0.2, spot_cell=0.12, ao=0.25, plate=0.085),
)

FORMS = {"grown": GROWN, "hatchling": HATCH}


# ------------------------------------------------------------------------------ helpers
def _sheet(kit, name, pts, thickness):
    """A flat (possibly concave) polygon, triangulated, made two-sided; origin at pts[0]."""
    import bmesh
    V = kit.V
    origin = V(pts[0])
    bm = bmesh.new()
    vs = [bm.verts.new(V(p) - origin) for p in pts]
    face = bm.faces.new(vs)
    bmesh.ops.triangulate(bm, faces=[face], quad_method="BEAUTY", ngon_method="EAR_CLIP")
    obj = kit.mesh_object(name, bm, origin)
    m = obj.modifiers.new("thick", "SOLIDIFY")
    m.thickness = thickness
    m.offset = 0
    m.use_rim = False
    kit.apply_modifiers(obj)
    return obj


def _plume(kit, name, base, direction, side, length, width, thickness):
    """A long tail plume: narrow at the base, a broad rounded paddle toward the tip."""
    V = kit.V
    d = V(direction).normalized()
    s = V(side)
    s = (s - d * s.dot(d)).normalized()
    b = V(base)
    if kit.LOD:
        prof = [(0.5, 0.42), (0.88, 0.34), (1.0, 0.0)]
    else:
        prof = [(0.28, 0.16), (0.58, 0.44), (0.82, 0.46), (0.95, 0.28), (1.0, 0.0)]
    right = [b + d * length * f + s * width * w for f, w in prof]
    left = [b + d * length * f - s * width * w for f, w in reversed(prof[:-1])]
    return kit.flat_fan(name, [b] + right + left, thickness)


def _disc(kit, name, center, normal, radius, thickness, n=None):
    V = kit.V
    n = n or kit.lod(6, 3)
    nrm = V(normal).normalized()
    a = V((0, 0, 1)) if abs(nrm.z) < 0.9 else V((1, 0, 0))
    u = nrm.cross(a).normalized()
    w = nrm.cross(u).normalized()
    c = V(center)
    pts = [c] + [c + (u * math.cos(2 * math.pi * k / n) + w * math.sin(2 * math.pi * k / n)) * radius
                 for k in range(n + 1)]
    return kit.flat_fan(name, pts, thickness)


# ------------------------------------------------------------------------------ wings
def _wing_frame(kit, side):
    """(point(u, v, lift), top normal) in the wing's own plane, seated like the kit's."""
    w = kit.F["wing"]
    s = -1 if side == "L" else 1
    th, ph = math.radians(w["dihedral"]), math.radians(w["droop"])
    span = kit.V((s * math.cos(th), 0, math.sin(th)))
    chord = kit.V((0, math.cos(ph), -math.sin(ph)))
    seat = w.get("seat")
    root = kit.mirror(seat["root"] if seat else w["root"], s)
    n = span.cross(chord).normalized()
    if n.z < 0:
        n = -n

    def P(u, v, lift=0.0):
        return root + (span * u + chord * v) * w["scale"] + n * lift
    return P, n


def _tip_arc(P, base, tip, width, lift, angles):
    """The rounded end of one feather, from its outer side to its inner side."""
    bu, bv = base
    tu, tv = tip
    du, dv = tu - bu, tv - bv
    ln = math.hypot(du, dv)
    du, dv = du / ln, dv / ln
    pu, pv = dv, -du          # toward the wing's outer (leading) side
    r = width * 0.5
    cu, cv = tu - du * r, tv - dv * r
    return [P(cu + (du * math.cos(math.radians(a)) + pu * math.sin(math.radians(a))) * r,
              cv + (dv * math.cos(math.radians(a)) + pv * math.sin(math.radians(a))) * r, lift) for a in angles]


def _feather_edge(P, feathers, frac, widen, lift, angles, notch):
    """Scalloped edge over feathers (listed inner to outer), walked outer to inner. frac: how far
    along each feather its row ends (1 = the flight feathers, less for the coverts)."""
    edge = []
    prev = None
    for base, tip, width in reversed(feathers):
        t = (base[0] + (tip[0] - base[0]) * frac, base[1] + (tip[1] - base[1]) * frac)
        arc = _tip_arc(P, base, t, width * widen, lift, angles)
        if prev is not None and notch:
            (pb, pt), here = prev, (base, t)
            mid_u = (pt[0] + t[0]) * 0.5
            mid_v = (pt[1] + t[1]) * 0.5
            back_u = ((pt[0] - pb[0]) + (t[0] - base[0])) * 0.5
            back_v = ((pt[1] - pb[1]) + (t[1] - base[1])) * 0.5
            edge.append(P(mid_u - back_u * notch, mid_v - back_v * notch, lift))
        edge += arc
        prev = (base, t)
    return edge


def _wing_side(kit, side, mats, rare=False):
    F, V = kit.F, kit.V
    w = F["wing"]
    wr = w["radii"]
    P, n = _wing_frame(kit, side)
    L = w["layout"]
    pts3 = kit.wing_points(side)
    full = not kit.LOD
    feathers = w["flight"] if full else w["flight"][::2] + ([w["flight"][-1]] if len(w["flight"]) % 2 == 0 else [])
    th = w["thickness"]
    objs = []
    arm = kit.wing_arm(side, ["root", "elbow", "wrist", "thumb"],
                       [wr["root"], wr["elbow"], wr["wrist"], wr["tip"] * 2.2],
                       kit.lod(w["arm_tris"], 48), mats)
    objs.append(arm)
    # Flight feathers: the golden trailing edge, one rounded tip per feather.
    lead = [P(u, v) for u, v in w["lead"]]
    outline = [pts3["root"], pts3["elbow"], pts3["wrist"]] + lead
    outline += _feather_edge(P, feathers, 1.0, 1.0, 0.0, kit.lod((58, 20, -20, -58), (45, -45)), 0.1)
    outline += [pts3["body"]]
    fl = _sheet(kit, f"flight_{side}", outline, th)
    fl.data.materials.append(mats["horn"])
    objs.append(fl)
    # Coverts: the wing's shoulder in the covert colour, a thicker sheet round the flight
    # feathers, so it shows on both faces.
    c = w["covert"]
    body_in = P(L["body"][0] * 0.5, L["body"][1] * c)
    cov = [pts3["root"], pts3["elbow"], pts3["wrist"]] + [P(u, v) for u, v in w["lead"][:1]]
    cov += _feather_edge(P, feathers, c, 0.92, 0.0, kit.lod((50, 0, -50), (0,)), 0.08)
    cov += [body_in]
    cv = _sheet(kit, f"covert_{side}", cov, th * 3.2)
    cv.data.materials.append(mats["membrane"])
    objs.append(cv)
    return objs


def wings(kit, d, rare):
    if rare:
        return []
    return _wing_side(kit, "L", d["mats"]) + _wing_side(kit, "R", d["mats"])


# ------------------------------------------------------------------------------ parts
def _merge_crest_weights(kit, d):
    """The crest bone moves the crest only: its share of the head's skin goes to the head."""
    body = d["body"]
    crest = body.vertex_groups.get("crest")
    if crest is None:
        return
    head = body.vertex_groups.get("head") or body.vertex_groups.new(name="head")
    for v in body.data.vertices:
        wc = next((g.weight for g in v.groups if g.group == crest.index), 0.0)
        if wc > 0:
            wh = next((g.weight for g in v.groups if g.group == head.index), 0.0)
            head.add([v.index], wh + wc, "REPLACE")
    body.vertex_groups.remove(crest)


def _reorigin(kit, obj, point):
    """Move an object's origin (its seating anchor) to `point`, keeping the mesh in place: a
    glowing tip or an eye on a plume seats with the feather it belongs to, not on its own."""
    import mathutils
    p = kit.V(point)
    obj.data.transform(mathutils.Matrix.Translation(obj.location - p))
    obj.location = p
    return obj


def _tip(kit, name, base, d, s, length, width, thickness):
    """A small rounded feather tip (six points: cheaper than a blade)."""
    if kit.LOD:
        pts = [base, base + d * length * 0.4 + s * width * 0.5, base + d * length, base + d * length * 0.4 - s * width * 0.5]
    else:
        pts = [base, base + d * length * 0.3 + s * width * 0.5, base + d * length * 0.8 + s * width * 0.36,
               base + d * length, base + d * length * 0.8 - s * width * 0.36, base + d * length * 0.3 - s * width * 0.5]
    return kit.flat_fan(name, pts, thickness)


def _feathers(kit, mats, specs, bone, mat, thick, tag, glow=None):
    """Blades from specs [(at (head frame, x mirrored), direction, length, width)], each flat
    in the plane of its direction and the body's up-down. glow: (material, fraction, the
    specs that get it): a glowing tip over the outer part of those blades (the rare variant)."""
    V = kit.V
    out = []
    for i, (at, dr, ln, wd) in enumerate(specs):
        for s in ((-1, 1) if at[0] else (1,)):
            base = kit.head_point(at, s)
            dd = kit.mirror(dr, s).normalized()
            side = V((1, 0, 0)).cross(dd)
            if side.length < 1e-3:
                side = V((0, 1, 0))
            side = (side - dd * side.dot(dd)).normalized()
            o = kit.blade(f"{tag}_{i}_{s}", base, dd, side, ln, wd, thick)
            o.data.materials.append(mats[mat])
            out.append((o, bone))
            if glow and i in glow[2]:
                gm, f, _ = glow
                g = _tip(kit, f"{tag}glow_{i}_{s}", base + dd * ln * (1 - f), dd, side, ln * f, wd * 0.62, thick * 2.2)
                _reorigin(kit, g, base)
                g.data.materials.append(mats[gm])
                out.append((g, bone))
    return out


# The crest: feather-like spines fanning up and back from the crown (head frame offsets).
CREST = [((0, -0.04, 0.29), (0, 0.22, 0.97), 0.50, 0.125), ((0, 0.06, 0.28), (0, 0.58, 0.81), 0.64, 0.135),
         ((0, 0.15, 0.22), (0, 0.88, 0.47), 0.58, 0.13), ((0.07, 0.00, 0.27), (0.32, 0.36, 0.88), 0.44, 0.115),
         ((0.09, 0.10, 0.24), (0.36, 0.74, 0.57), 0.50, 0.12)]
CREST_RARE = CREST + [((0, 0.22, 0.14), (0, 0.97, 0.22), 0.52, 0.12), ((0.12, 0.18, 0.18), (0.45, 0.84, 0.30), 0.42, 0.11)]
CHEEKS = [((0.23, 0.10, -0.04), (0.50, 0.85, 0.16), 0.30, 0.10), ((0.21, 0.13, -0.14), (0.46, 0.88, -0.10), 0.24, 0.09)]
BABY_CREST = [((0, -0.05, 0.31), (0, 0.10, 1.0), 0.20, 0.13), ((0, 0.07, 0.30), (0, 0.62, 0.78), 0.22, 0.13),
              ((0.08, 0.0, 0.29), (0.50, 0.15, 0.85), 0.17, 0.12), ((0.10, 0.12, 0.25), (0.55, 0.62, 0.56), 0.16, 0.11)]
BABY_CREST_RARE = BABY_CREST + [((0, 0.16, 0.25), (0, 0.9, 0.42), 0.18, 0.12)]


def _ruff(kit, mats):
    """The hatchling's fluffy ruff: soft round tufts in a collar behind the cheeks."""
    V = kit.V
    out = []
    for j, a in enumerate((-128, -104, -80, -56, 56, 80, 104, 128)):
        r = math.radians(a)
        base = kit.head_point((0.27 * math.sin(r), 0.14, -0.04 - 0.22 * math.cos(r)))
        out_dir = V((math.sin(r) * 0.8, 0.75, -math.cos(r) * 0.6)).normalized()
        tangent = V((math.cos(r), 0, math.sin(r)))
        ln = 0.2 if 40 < abs(a) < 110 else 0.16
        o = kit.blade(f"ruff_{j}", base, out_dir, tangent, ln, 0.1, 0.012)
        o.data.materials.append(mats["body_plain"])
        out.append((o, "head"))
    return out


def _plumes(kit, mats, angles, length, width, ocelli=False):
    """The fan of plumes at the tail's end: spread up and down in the body's own plane (it reads
    from the side and the den's camera), a little splayed to the sides, each plume a narrow
    shaft opening into a rounded paddle. angles: each plume's tilt from the tail's line
    (degrees, + up). ocelli: a glowing eye on each (the rare variant)."""
    F, V = kit.F, kit.V
    tip, t4 = V(F["nodes"]["tail_tip"][0]), V(F["nodes"]["tail4"][0])
    dt = (tip - t4).normalized()
    x = V((1, 0, 0))
    up = x.cross(dt).normalized()
    base = tip - dt * 0.05 * length
    out = []
    n = len(angles)
    for j, deg in enumerate(angles):
        f = (j / (n - 1)) * 2 - 1 if n > 1 else 0.0          # -1 .. 1 across the fan
        a = math.radians(deg)
        splay = 0.16 * (1 if j % 2 else -1) * (0.4 + 0.6 * abs(f))
        dd = (dt * math.cos(a) + up * math.sin(a) + x * splay).normalized()
        side = x.cross(dd).normalized()
        lk = 1.0 - 0.24 * abs(f) ** 1.6
        o = _plume(kit, f"plume_{j}", base, dd, side, length * lk, width, 0.012)
        o.data.materials.append(mats["horn"])
        out.append((o, "tail4"))
        if ocelli:
            c = base + dd * length * lk * 0.74
            e = _disc(kit, f"ocellus_{j}", c, dd.cross(side).normalized(), width * 0.34, 0.03)
            _reorigin(kit, e, base)
            e.data.materials.append(mats["glow_flat"])
            out.append((e, "tail4"))
    return out


def parts(kit, d):
    _merge_crest_weights(kit, d)
    F, mats, V = kit.F, d["mats"], kit.V
    baby = d["form"] == "hatchling"
    out = []
    if baby:
        out.append(("frill", 0, _feathers(kit, mats, BABY_CREST, "crest", "membrane", 0.014, "crest") + _ruff(kit, mats)))
        out.append(("frill", 1, _feathers(kit, mats, BABY_CREST_RARE, "crest", "membrane", 0.014, "crest",
                                          glow=("glow_flat", 0.36, (0, 1, 2, 3))) + _ruff(kit, mats)))
        out.append(("tail_tip", 0, _plumes(kit, mats, (-26, 0, 26), 0.36, 0.13)))
        out.append(("tail_tip", 1, _plumes(kit, mats, (-30, -8, 14, 34), 0.42, 0.15, ocelli=True)))
        return out
    cheeks = _feathers(kit, mats, CHEEKS, "head", "membrane", 0.012, "cheek")
    out.append(("frill", 0, _feathers(kit, mats, CREST, "crest", "membrane", 0.014, "crest") + cheeks))
    rare_cheeks = _feathers(kit, mats, CHEEKS, "head", "membrane", 0.012, "cheek")
    out.append(("frill", 1, _feathers(kit, mats, [(a, dr, ln * 1.22, wd * 1.08) for a, dr, ln, wd in CREST_RARE], "crest",
                                      "membrane", 0.014, "crest", glow=("glow_flat", 0.3, (0, 1, 2, 3))) + rare_cheeks))
    # A few small feathers down the nape, behind the crest.
    nape = []  # seated along the neck's own perpendicular: a ray straight back would find the rump
    for i, (bone, nxt, up, size) in enumerate((("neck4", "head", 0.2, 0.26), ("neck3", "neck4", 0.21, 0.22))):
        c = kit.node(bone)
        out_dir = kit.spine_up(bone, nxt)
        along = (kit.node(bone) - kit.node(nxt)).normalized()   # down the neck
        base = c + out_dir * up
        o = kit.blade(f"nape_{i}", base, (along * 0.75 + out_dir * 0.66).normalized(), V((1, 0, 0)), size, size * 0.42,
                      0.012)
        o.data.materials.append(mats["membrane"])
        nape.append((o, bone))
    out.append(("spikes", 0, nape))
    out.append(("tail_tip", 0, _plumes(kit, mats, (-16, 0, 16, 32, 48), 1.2, 0.27)))
    out.append(("tail_tip", 1, _plumes(kit, mats, (-8, 6, 20, 34, 48, 62, 76), 1.55, 0.3, ocelli=True)))
    return out


def texture(tx, nt, p, form):
    """R: soft bars across the back and tail; G: a darker mantle over the back and neck;
    B: round spots on the back and flanks (they glow on the rare Starplume)."""
    top = tx.smoothstep(nt, tx.normal_z(nt), 0.0, 0.5)
    r = tx.stripes(nt, p["stripe"], distortion=0.9, width=(0.66, 0.78), where=top)
    noise = tx.node(nt, "ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 2.2
    nt.links.new(tx.coords(nt), noise.inputs["Vector"])
    wobble = tx.madd(nt, noise.outputs["Fac"], 0.5, -0.25)
    g = tx.smoothstep(nt, tx.add(nt, tx.normal_z(nt), wobble), 0.05, 0.4)
    legs = 1.25 if form == "grown" else 0.36  # no spots below the body
    body = tx.mul(nt, tx.smoothstep(nt, tx.normal_z(nt), -0.25, 0.2), tx.smoothstep(nt, tx.axis(nt, 2), legs - 0.1, legs + 0.1))
    b = tx.spots(nt, p["spot_cell"], keep=0.46, size=(0.3, 0.2), where=body)
    # The painted value, with a dragon's belly plates: soft bands across the pale belly and
    # up the throat (along y - z: across the belly, up the neck), only where the accent is.
    value = tx.mul(nt, tx.grain(nt, p["grain"], p["grain_amount"]), tx.light_from_above(nt, p["light"]))
    s = tx.math_op(nt, "SUBTRACT", tx.axis(nt, 1), tx.axis(nt, 2))
    wave = tx.math_op(nt, "SINE", tx.mul(nt, s, 2 * math.pi / p["plate"]))
    accent = tx.node(nt, "ShaderNodeAttribute", attribute_name="mask")
    acc = tx.node(nt, "ShaderNodeSeparateColor")
    nt.links.new(accent.outputs["Color"], acc.inputs[0])
    plates = tx.mul(nt, tx.smoothstep(nt, wave, 0.45, 0.9), tx.smoothstep(nt, acc.outputs["Green"], 0.35, 0.75))
    value = tx.mul(nt, value, tx.madd(nt, plates, -0.1, 1.0))
    return {"r": r, "g": g, "b": b, "value": value}
