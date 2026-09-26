"""The Pouncer (Ember, common): a sleek, cat-like dragon. A kitten of a hatchling with a big
round head, huge eyes and a leaf-tipped tail grows into a lithe, panther-like adult: long
legs, a small round head with pointed ears and swept-back horns, big scalloped bat wings and a
long tail ending in twin fins. Playful and quick; the "classic dragon" of the eight.
Concept: docs/art/concept/dragons/dragon_pouncer.jpg (R11).
"""
import math


def _mirrored(center, sides):
    nodes = dict(center)
    for name, (p, r) in sides.items():
        nodes[f"{name}_R"] = ((p[0], p[1], p[2]), r)
        nodes[f"{name}_L"] = ((-p[0], p[1], p[2]), r)
    return nodes


META = dict(
    name="pouncer", title="Pouncer", dex=1, element="Ember", parents=(), rarity="common",
    plan="pouncer", size=1.0,
    stats=dict(wing=7, wit=5, might=5, breath=6, stamina=5),
    manners=("Playful", "Curious", "Brave", "Mischievous"),
    traits=("Swift", "Keen Nose", "Night Owl", "Sure-Footed"),
    rare_variant=3, rare_replaces=False,
    blurb="Quick, curious and always ready to pounce; happiest chasing something that rolls.",
)

VARIANTS = [
    dict(name="Ember", base=(0.93, 0.42, 0.12), accent=(1.0, 0.88, 0.66), pattern=(0.62, 0.20, 0.07),
         horn=(0.52, 0.24, 0.12), membrane=(0.96, 0.58, 0.30), iris=(0.55, 0.30, 0.08), glow=(1.0, 0.62, 0.20),
         pattern_channel="r", glow_channel=None),
    dict(name="Tabby", base=(0.92, 0.66, 0.36), accent=(1.0, 0.93, 0.78), pattern=(0.55, 0.30, 0.13),
         horn=(0.50, 0.33, 0.20), membrane=(0.93, 0.72, 0.48), iris=(0.36, 0.52, 0.18), glow=(1.0, 0.68, 0.28),
         pattern_channel="r", glow_channel=None),
    dict(name="Cinder", base=(0.30, 0.27, 0.29), accent=(0.92, 0.62, 0.40), pattern=(0.16, 0.14, 0.16),
         horn=(0.14, 0.12, 0.13), membrane=(0.52, 0.34, 0.30), iris=(0.95, 0.62, 0.18), glow=(1.0, 0.55, 0.18),
         pattern_channel="g", glow_channel=None),
    dict(name="Emberblaze", base=(0.58, 0.10, 0.08), accent=(1.0, 0.78, 0.34), pattern=(1.0, 0.72, 0.26),
         horn=(0.26, 0.06, 0.05), membrane=(0.86, 0.26, 0.10), iris=(1.0, 0.72, 0.22), glow=(1.0, 0.58, 0.14),
         pattern_channel=None, glow_channel="b"),
]

EGG = dict(height=1.0, width=0.37, point=1.0, speckle="spots",
           colors=[((1.0, 0.94, 0.82), (0.95, 0.55, 0.20)), ((1.0, 0.95, 0.86), (0.78, 0.52, 0.28)),
                   ((0.46, 0.42, 0.44), (0.90, 0.58, 0.32)), ((0.72, 0.16, 0.10), (1.0, 0.78, 0.30))])

# ------------------------------------------------------------------------------ grown
# Faces -Y, Z up; units ~ metres at adult size. radius = (side, vertical).
GROWN_NODES = _mirrored({
    "tail_tip": ((0, 3.88, 1.22), (0.04, 0.04)),
    "tail4": ((0, 3.26, 1.02), (0.09, 0.09)),
    "tail3": ((0, 2.50, 0.90), (0.15, 0.14)),
    "tail2": ((0, 1.72, 0.92), (0.24, 0.23)),
    "hips": ((0, 0.92, 1.06), (0.44, 0.46)),
    "belly": ((0, 0.20, 1.06), (0.46, 0.50)),
    "chest": ((0, -0.50, 1.12), (0.48, 0.56)),
    "neck1": ((0, -1.00, 1.42), (0.31, 0.33)),
    "neck2": ((0, -1.20, 1.60), (0.27, 0.28)),
    "neck3": ((0, -1.34, 1.76), (0.25, 0.25)),
    "head": ((0, -1.52, 1.95), (0.38, 0.34)),
    "muzzle": ((0, -1.80, 1.87), (0.22, 0.18)),
    "snout": ((0, -2.02, 1.81), (0.13, 0.11)),
}, {
    "shoulder": ((0.36, -0.50, 0.90), (0.22, 0.26)),
    "elbow": ((0.40, -0.56, 0.50), (0.15, 0.15)),
    "wrist": ((0.40, -0.62, 0.14), (0.115, 0.115)),
    "toe_f": ((0.41, -0.86, 0.07), (0.13, 0.07)),
    "hipj": ((0.36, 0.94, 0.90), (0.30, 0.34)),
    "knee": ((0.42, 0.66, 0.52), (0.18, 0.18)),
    "ankle": ((0.42, 1.02, 0.18), (0.12, 0.12)),
    "toe_b": ((0.43, 0.80, 0.07), (0.13, 0.07)),
})
GROWN_EDGES = [("tail_tip", "tail4"), ("tail4", "tail3"), ("tail3", "tail2"), ("tail2", "hips"),
               ("hips", "belly"), ("belly", "chest"), ("chest", "neck1"), ("neck1", "neck2"),
               ("neck2", "neck3"), ("neck3", "head"), ("head", "muzzle"), ("muzzle", "snout")]
for _side in ("L", "R"):
    GROWN_EDGES += [("chest", f"shoulder_{_side}"), (f"shoulder_{_side}", f"elbow_{_side}"),
                    (f"elbow_{_side}", f"wrist_{_side}"), (f"wrist_{_side}", f"toe_f_{_side}"),
                    ("hips", f"hipj_{_side}"), (f"hipj_{_side}", f"knee_{_side}"),
                    (f"knee_{_side}", f"ankle_{_side}"), (f"ankle_{_side}", f"toe_b_{_side}")]

# Individual variety (the genome's build): gentle, so every Pouncer still reads as a Pouncer.
BUILDS = {
    "neutral": {},
    "sturdy": {"chest": (1.08, 0.98), "belly": (1.07, 0.98), "hips": (1.05, 1.0), "neck1": (1.06, 0.96),
               "leg_up": (1.07, 0.97), "arm_up": (1.07, 0.97), "tail2": (1.05, 0.96)},
    "sleek": {"chest": (0.94, 1.02), "belly": (0.92, 1.03), "hips": (0.95, 1.0), "leg_lo": (0.95, 1.04),
              "arm_lo": (0.95, 1.04), "tail3": (0.94, 1.05), "tail4": (0.94, 1.05)},
    "long": {"neck1": (0.97, 1.08), "neck2": (0.97, 1.08), "belly": (0.97, 1.08), "tail2": (0.96, 1.08),
             "tail3": (0.96, 1.1), "tail4": (0.96, 1.1)},
}


def _grown_sculpt(kit, obj):
    """A cat's shapes the node graph can't give: a deep chest over a tucked belly, round
    cheeks, a short soft muzzle and a flat brow."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for v in bm.verts:
        x, y, z = v.co
        if -0.9 < y < -0.1 and z < 1.05 and abs(x) < 0.36:  # chest keel
            k = (1 - abs(x) / 0.36) * max(0.0, (1.05 - z) / 0.45)
            v.co.z -= 0.07 * k
        if 0.0 < y < 0.75 and z < 0.9 and abs(x) < 0.34:  # tucked belly
            k = (1 - abs(x) / 0.34) * max(0.0, (0.9 - z) / 0.3)
            v.co.z += 0.06 * k
        if -1.66 < y < -1.36 and 1.79 < z < 2.09:  # round cheeks
            v.co.x *= 1.1
        if y < -1.71 and z > 1.89:  # soft short muzzle, a little flatter on top
            t = min(1.0, (-1.71 - y) / 0.3)
            v.co.z -= 0.025 * t
    bm.to_mesh(obj.data)
    bm.free()


GROWN = dict(
    name="grown", nodes=GROWN_NODES, edges=GROWN_EDGES, body="skin",
    body_tris=1650, body_tris_lod1=620, export_scale=1.0,
    young={
        "bones": {
            "head": (0.95, 0.9, 1.0), "snout": (0.9, 0.7, 0.95),
            "neck1": (0.74, 0.52), "neck2": (0.76, 0.5), "neck3": (0.8, 0.5),
            "chest": (0.66, 0.52), "belly": (0.64, 0.5), "hips": (0.68, 0.52),
            "tail1": (0.66, 0.5), "tail2": (0.68, 0.48), "tail3": (0.72, 0.48), "tail4": (0.8, 0.52),
            "arm_up": (0.68, 0.58), "arm_lo": (0.7, 0.58), "hand": (0.8, 0.72),
            "leg_up": (0.68, 0.58), "leg_lo": (0.7, 0.56), "foot": (0.8, 0.72),
        },
        "parts": {"eyes": 1.35, "horns": 0.4, "frill": 0.8, "wings": 0.4, "spikes": 0.55,
                  "tail_tip": 0.7, "heart": 0.85, "runes": 0.7},
    },
    young_pose={"neck1": -18, "neck2": -6, "neck3": 4, "head": 18},
    base_pose={"neck1": (-2, 0, 0), "neck2": (4, 0, 0), "neck3": (6, 0, 0), "head": (-6, 0, 0),
               "tail1": (6, 0, 0), "tail2": (2, 0, 6), "tail3": (4, 0, 10), "tail4": (8, 0, 12)},
    builds=BUILDS,
    eyes=dict(at=(0.19, -1.78, 2.02), out=(0.60, -0.78, 0.15), iris=(0.105, 0.118, 0.058),
              pupil=(0.068, 0.084, 0.02), slit=(0.26, 1.08),
              glints=((-0.022, 0.034, 0.016), (0.016, -0.034, 0.008)), seg=(12, 2, 8, 2)),
    head=dict(origin=(0, -1.52, 1.95), k=1.0, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=False,
              frill_k=1.0, feather_w=1.0),
    tail_k=1.0,
    heart=dict(at=(0, -0.98, 1.18), size=0.11),
    wing=dict(root=(0.32, -0.42, 1.52), scale=1.1, dihedral=50, droop=10,
              radii={"root": 0.095, "elbow": 0.07, "wrist": 0.056, "finger": 0.021, "tip": 0.008},
              arm_tris=180, thickness=0.014, style="classic", claws=False),
    mask=dict(max_x=0.32, max_z=1.9, min_z=-1.0, tail_cut=(1.3, 0.8)),
    inset={"eyes": 0.02, "horns": 0.03, "spikes": 0.03, "frill": 0.05, "heart": -0.06, "runes": -0.012},
    face=dict(nostril=(0.045, -2.13, 1.86), nostril_r=(0.024, 0.015, 0.008), mouth_r=0.011,
              mouth=lambda side, a: (side * 0.15 * a ** 0.7, -2.14 + 0.42 * a ** 1.5, 1.75 + 0.06 * a * a)),
    jaw_hinge=(0, -1.64, 1.79),
    mouth_detail=dict(depth=0.26, fade=0.2, width=0.36, tooth=(0.009, 0.016), fang=(0.013, 0.032),
                      tongue=(0.065, 0.13, 0.015)),
    skin=dict(stripe=0.42, spot_cell=0.3, ao=0.6),
    ridge=dict(path=[("neck2", 0.24), ("neck1", 0.31), ("chest", 0.54), ("belly", 0.48), ("hips", 0.44),
                     ("tail2", 0.22), ("tail3", 0.14)],
               size=(0.07, 0.06, 0.3), fin=(2.4, 2.0, 0.018)),
    sculpt=_grown_sculpt,
)

# ------------------------------------------------------------------------------ hatchling
HATCH_NODES = _mirrored({
    "tail_tip": ((0, 1.30, 0.44), None),
    "tail4": ((0, 1.10, 0.36), None),
    "tail3": ((0, 0.88, 0.34), None),
    "tail2": ((0, 0.64, 0.40), None),
    "hips": ((0, 0.34, 0.50), None),
    "belly": ((0, 0.08, 0.50), None),
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
    "hipj": ((0.21, 0.36, 0.44), None),
    "knee": ((0.23, 0.30, 0.26), None),
    "ankle": ((0.24, 0.41, 0.12), None),
    "toe_b": ((0.24, 0.24, 0.05), None),
})

HATCH_META = [
    ("ell", (0, -0.52, 1.12), (0.345, 0.315, 0.305)),  # big round kitten head
    ("ell", (0, -0.78, 0.98), (0.17, 0.15, 0.12)),      # short button muzzle
    ("ell", (0, -0.74, 0.92), (0.13, 0.12, 0.07)),      # chin
    ("chain", [(0, -0.30, 0.66), (0, -0.40, 0.80), (0, -0.47, 0.92)], [0.17, 0.16, 0.155]),
    ("ell", (0, -0.16, 0.56), (0.27, 0.25, 0.27)),      # chest
    ("ell", (0, 0.10, 0.50), (0.29, 0.29, 0.28)),       # round tummy
    ("ell", (0, 0.34, 0.52), (0.24, 0.22, 0.23)),       # hips
    ("ell", (0, 0.02, 0.40), (0.23, 0.25, 0.17)),
    ("chain", [(0, 0.52, 0.46), (0, 0.72, 0.38), (0, 0.92, 0.35), (0, 1.12, 0.38), (0, 1.28, 0.44)],
     [0.11, 0.085, 0.07, 0.055, 0.04]),               # a thin tail curling up
]
for _s in (-1, 1):
    HATCH_META += [
        ("ball", (_s * 0.17, -0.72, 0.96), 0.13),       # cheeks
        ("chain", [(_s * 0.21, -0.16, 0.42), (_s * 0.22, -0.21, 0.25), (_s * 0.23, -0.25, 0.10)],
         [0.1, 0.088, 0.085]),
        ("ell", (_s * 0.23, -0.32, 0.06), (0.095, 0.12, 0.06)),    # front paws
        ("ell", (_s * 0.21, 0.34, 0.38), (0.13, 0.17, 0.17)),      # thighs
        ("chain", [(_s * 0.23, 0.40, 0.26), (_s * 0.24, 0.41, 0.12)], [0.09, 0.08]),
        ("ell", (_s * 0.24, 0.30, 0.06), (0.095, 0.13, 0.06)),     # hind paws
    ]

HATCH = dict(
    name="hatchling", nodes=HATCH_NODES, meta=HATCH_META, body="meta",
    body_tris=1600, body_tris_lod1=560, export_scale=1.0,
    young={
        "bones": {name: ((0.74, 0.74) if name in ("head", "snout") else (0.62, 0.62))
                  for name in ("hips", "belly", "chest", "neck1", "neck2", "neck3", "head", "snout", "tail1",
                               "tail2", "tail3", "tail4", "arm_up", "arm_lo", "hand", "leg_up", "leg_lo", "foot")},
        "parts": {"eyes": 1.12, "horns": 0.6, "frill": 0.9, "wings": 0.56, "spikes": 0.85,
                  "tail_tip": 0.9, "heart": 1.0, "runes": 0.9},
    },
    # The baby's little wings turned up and back off its round body (run 17: folded as the adult's,
    # they sank into its back and flank).
    base_pose={"head": (4, 0, 0), "tail1": (6, 0, 0), "tail2": (4, 0, 10), "tail3": (8, 0, 12),
               "tail4": (12, 0, 14), "wing_arm_R": (-20, 0, -55), "wing_arm_L": (-20, 0, 55)},
    builds={"neutral": {}, "sturdy": {}, "sleek": {}, "long": {}},
    eyes=dict(at=(0.17, -0.78, 1.09), out=(0.42, -0.90, 0.07), iris=(0.118, 0.132, 0.066),
              pupil=(0.08, 0.094, 0.03), slit=(0.32, 1.1),
              glints=((-0.028, 0.044, 0.028), (0.024, -0.042, 0.013)), seg=(14, 3, 12, 2)),
    head=dict(origin=(0, -0.50, 1.23), k=0.9, horn_len=0.4, horn_r=0.8, horn_curve=0.6, buds=True,
              frill_k=0.6, feather_w=1.6),
    tail_k=0.42,
    heart=dict(at=(0, -0.40, 0.53), size=0.075),
    wing=dict(root=(0.15, 0.02, 0.72), scale=0.16, dihedral=30, droop=5,
              radii={"root": 0.045, "elbow": 0.035, "wrist": 0.03, "finger": 0.011, "tip": 0.005},
              arm_tris=120, thickness=0.008, style="sail", claws=False),
    mask=dict(max_x=0.22, max_z=1.06, min_z=0.13, tail_cut=None),
    inset={"eyes": 0.032, "horns": 0.02, "spikes": 0.012, "frill": 0.03, "heart": -0.03, "runes": -0.01},
    face=dict(nostril=(0.042, -0.905, 1.005), nostril_r=(0.024, 0.016, 0.009), mouth_r=0.011,
              mouth=lambda side, a: (side * 0.11 * a ** 0.8, -0.93 + 0.17 * a ** 1.6, 0.925 + 0.03 * a * a)),
    jaw_hinge=(0, -0.70, 0.93),
    mouth_detail=dict(depth=0.15, fade=0.11, width=0.28, tooth=(0.007, 0.012), fang=(0.010, 0.022),
                      tongue=(0.052, 0.08, 0.012)),
    skin=dict(stripe=0.2, spot_cell=0.14, ao=0.25),
    ridge=dict(path=[("neck1", 0.16), ("chest", 0.26), ("belly", 0.28), ("hips", 0.23), ("tail2", 0.1)],
               size=(0.03, 0.02, 0.2), fin=(2.0, 1.6, 0.01)),
)

FORMS = {"grown": GROWN, "hatchling": HATCH}


# ------------------------------------------------------------------------------ hooks
def _ear_frame(kit, out, up):
    """The ear's axes: s across it, front (facing forward) and u along it."""
    V = kit.V
    o, u = V(out).normalized(), V(up).normalized()
    s = o.cross(u).normalized()
    front = u.cross(s).normalized()
    if front.y > 0:
        front, s = -front, -s
    return s, front, u


def _ear(kit, name, base, out, up, length, width, depth, material):
    """A pointed cat ear: a flattened, gently curved cone (a real volume: flat fans showed
    star-shaped shading edge-on), leaning back."""
    import mathutils
    s, front, u = _ear_frame(kit, out, up)
    o = kit.horn_mesh(name, length, width * 0.5, 0.35, kit.lod(5, 3), kit.lod(7, 5), taper=0.75, tip=0.04)
    o.rotation_euler = mathutils.Matrix((-s, -front, u)).transposed().to_euler()  # a proper rotation: +Y (its curve) back
    o.location = kit.V(base)
    o.scale = (1.0, depth / (width * 0.4), 1.0)
    o.data.materials.append(material)
    return o, s, front, u


def _inner_ear(kit, name, base, s, front, u, length, width, material):
    """The inner ear: a flat, rounded triangle on the ear's front face."""
    b = kit.V(base)
    pts = [b, b + s * width * 0.5, b + s * width * 0.4 + u * length * 0.55, b + u * length,
           b - s * width * 0.4 + u * length * 0.55, b - s * width * 0.5]
    o = kit.flat_fan(name, pts, 0.008)
    o.data.materials.append(material)
    return o


def parts(kit, d):
    F, mats, V = kit.F, d["mats"], kit.V
    baby = d["form"] == "hatchling"
    out = []
    # Ears: tall and pointed, leaning out and back; the inner ear in the accent colour.
    ears = []
    k = F["head"]["k"]
    for sd in (-1, 1):
        base = kit.head_point((0.2, 0.06, 0.24) if not baby else (0.2, 0.06, 0.22), sd)
        length, width = (0.5 if not baby else 0.34) * k, (0.34 if not baby else 0.3) * k
        e, s_ax, front, u = _ear(kit, f"ear_{sd}", base, (sd * 0.35, 0.25, 0.9), (sd * 0.55, 0.45, 0.72),
                                 length, width, 0.07 * k, mats["body_plain"])
        inner = _inner_ear(kit, f"earin_{sd}", base + front * 0.034 * k + u * 0.03 * k, s_ax, front, u,
                           length * 0.72, width * 0.6, mats["accent_flat"])
        ears += [(e, "head"), (inner, "head")]
    out.append(("frill", 0, ears))
    # Swept-back horns behind the ears (the grown only: a kitten's head stays round and soft).
    if not baby:
        horns = kit.build_horns([dict(len=0.46, r=0.085, curve=70, seg=5, ring=5, at=(0.11, 0.18, 0.1),
                                      rot=(-42, 10, 0))], mats)
        out.append(("horns", 0, [(h, "head") for h in horns]))
    # The tail's end: twin fins, a leaf on the baby.
    x, y, z = F["nodes"]["tail_tip"][0]
    k = F["tail_k"]
    if baby:
        leaf = kit.blade("tail_leaf", (0, y - 0.04, z - 0.01), (0, 0.9, 0.45), (1, 0, 0), 0.34 * k * 2.2,
                         0.2 * k * 2.2, 0.012)
        leaf.data.materials.append(mats["accent_flat"])
        out.append(("tail_tip", 0, [(leaf, "tail4")]))
    else:
        fins = []
        for s in (-1, 1):
            f = kit.lobed_fin(f"tailfin_{s}", (0, y - 0.1 * k, z - 0.01), (s * 0.5, 0.86, 0.1), (0, 0, 1),
                              0.28 * k, 55, -30, 2, 0.016)
            f.data.materials.append(mats["membrane"])
            fins.append((f, "tail4"))
        out.append(("tail_tip", 0, fins))
    # Rare (Emberblaze): a glowing ember sail down the neck and back, and glowing marks.
    sail = kit.build_ridge("fin", mats, material="glow_flat")
    out.append(("spikes", 1, sail))
    if not baby:
        marks = kit.glow_marks(mats, [("hips", (0.36, 0.9, 1.18), 2, 0.22), ("neck1", (0.24, -0.98, 1.72), 0, 0.18)],
                               material="glow_flat")
        out.append(("runes", 1, marks))
    return out


def accent(kit, body):
    """Cream belly, chest and chin, and cream socks on all four paws (a cat's markings)."""
    import mathutils
    mask = kit.mask_attr(body)
    F = kit.F
    mk = F["mask"]
    down = mathutils.Vector((0, -0.45, -0.89)).normalized()
    baby = F["name"] == "hatchling"
    sock_z = 0.13 if baby else 0.3
    for v in body.data.vertices:
        x, y, z = v.co
        a = min(1.0, max(0.0, (v.normal.dot(down) - 0.2) / 0.45))
        if abs(x) > mk["max_x"] or z > mk["max_z"] or z < mk["min_z"]:
            a = 0.0
        if mk.get("tail_cut") and y > mk["tail_cut"][0] and z < mk["tail_cut"][1]:
            a = 0.0
        if z < sock_z and abs(x) > (0.12 if baby else 0.22):  # socks
            a = max(a, min(1.0, (sock_z - z) / (sock_z * 0.4)))
        mask.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


def texture(tx, nt, p, form):
    """R: tabby stripes across the back and down the tail; G: soft leopard spots; B: the rare
    variant's glowing ember veins."""
    r = tx.stripes(nt, p["stripe"], distortion=1.6, width=(0.6, 0.72))
    rosette = tx.spots(nt, p["spot_cell"], keep=0.35, size=(0.34, 0.24))
    ring = tx.spots(nt, p["spot_cell"], keep=0.35, size=(0.2, 0.14))
    g = tx.maxi(nt, tx.math_op(nt, "SUBTRACT", rosette, ring), tx.mul(nt, ring, 0.35))
    b = tx.mul(nt, tx.cracks(nt, p["spot_cell"] * 2.2, width=0.035), tx.upper(nt))
    return {"r": r, "g": g, "b": b}
