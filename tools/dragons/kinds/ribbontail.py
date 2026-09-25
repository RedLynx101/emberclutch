"""The Ribbontail (Tide, uncommon): a long, low, serpentine water dragon.

Design notes (R11; concept docs/art/concept/dragons/dragon_ribbontail.jpg, reference only):
  * The baby is a little noodle: a big round head (a third of it), huge eyes, a wide smile,
    frilly axolotl-like fin-ears, four tiny legs and a tail it keeps curled round its side.
  * The adult is a river spirit: a long body low on four short legs, a swan's S of a neck
    carrying a slender head with a frilled crown of fins and two long soft whiskers, a
    ribbon of fin running down its back, a tail that sweeps in an S and curls up into a big
    flowing fan fin, and a pair of frilled fin-wings at the shoulders that lie swept back at
    rest and spread and ripple when it swims through the air.
  * Silhouette: the only kind that is long rather than tall; a wave from nose to tail.
  * Colour: a pale belly stripe from chin to tail tip, dappled spots along the back.
    Seaglass (teal and pearl), Coral (coral and cream), Deepsea (navy and cyan); the rare
    Moonpearl is pearl-white and lilac with glowing pearl spots, glowing fin ribbons, longer
    trailing whiskers, extra ribbons on its tail and a pearl on its brow.
  * Body plan: tools/dragons/plans/ribbontail.py (a serpent's skeleton and clips).
"""
import math

from dragons.plans import ribbontail as plan


def _mirrored(center, sides):
    nodes = dict(center)
    for name, (p, r) in sides.items():
        nodes[f"{name}_R"] = ((p[0], p[1], p[2]), r)
        nodes[f"{name}_L"] = ((-p[0], p[1], p[2]), r)
    return nodes


META = dict(
    name="ribbontail", title="Ribbontail", dex=5, element="Tide", parents=(), rarity="uncommon",
    plan="ribbontail", size=1.25,
    stats=dict(wing=7, wit=7, might=4, breath=8, stamina=5),
    manners=("Gentle", "Curious", "Sleepy", "Playful", "Proud"),
    traits=("Water-Lover", "Skydancer", "Songbird", "Cool-Headed", "Tidy", "Deep Sleeper"),
    rare_variant=3, rare_replaces=True,
    blurb="Drifts through the air as a river drifts through a valley; loves a bath more than anything.",
)

VARIANTS = [
    dict(name="Seaglass", base=(0.16, 0.60, 0.56), accent=(0.90, 0.95, 0.84), pattern=(0.06, 0.38, 0.40),
         horn=(0.86, 0.96, 0.88), membrane=(0.42, 0.84, 0.74), iris=(0.10, 0.30, 0.34), glow=(0.35, 0.95, 0.85),
         pattern_channel="r", glow_channel=None),
    dict(name="Coral", base=(0.96, 0.50, 0.44), accent=(1.0, 0.92, 0.80), pattern=(1.0, 0.86, 0.72),
         horn=(1.0, 0.90, 0.78), membrane=(1.0, 0.68, 0.58), iris=(0.40, 0.16, 0.14), glow=(0.35, 0.95, 0.85),
         pattern_channel="g", glow_channel=None),
    dict(name="Deepsea", base=(0.10, 0.17, 0.40), accent=(0.56, 0.90, 0.94), pattern=(0.30, 0.66, 0.82),
         horn=(0.70, 0.94, 0.98), membrane=(0.26, 0.62, 0.84), iris=(0.40, 0.86, 0.96), glow=(0.35, 0.95, 0.85),
         pattern_channel="r", glow_channel=None),
    dict(name="Moonpearl", base=(0.56, 0.50, 0.86), accent=(0.97, 0.94, 1.0), pattern=(0.44, 0.38, 0.76),
         horn=(1.0, 0.94, 0.80), membrane=(0.80, 0.84, 1.0), iris=(0.86, 0.56, 0.16), glow=(0.40, 1.0, 0.90),
         pattern_channel=None, glow_channel="b"),
]

# A pearly egg ringed like a seashell: soft growth bands in the colouring's tint.
EGG = dict(height=1.0, width=0.36, point=1.0, asym=0.2, speckle="bands",
           # (clear of the seam and of the cracks, 0.64..0.87 at the front, whose decals cover a band)
           speckle_params=dict(size=(0.01, 0.015), levels=[0.1, 0.17, 0.24, 0.31, 0.38, 0.45, 0.52, 0.91]),
           colors=[((0.94, 0.98, 0.95), (0.56, 0.84, 0.78)), ((1.0, 0.96, 0.92), (0.98, 0.72, 0.64)),
                   ((0.90, 0.94, 0.99), (0.46, 0.60, 0.86)), ((0.97, 0.95, 1.0), (0.74, 0.68, 0.96))])

# ------------------------------------------------------------------------------ grown
# Faces -Y, Z up; radius = (side, vertical). About 6.5 units from the nose to the fin's tip.
GROWN_NODES = _mirrored({
    "tail_tip": ((0, 4.30, 0.46), (0.045, 0.05)),
    "tail8": ((0, 4.00, 0.46), (0.08, 0.085)),
    "tail7": ((0, 3.66, 0.48), (0.11, 0.115)),
    "tail6": ((0, 3.28, 0.51), (0.145, 0.15)),
    "tail5": ((0, 2.87, 0.55), (0.18, 0.19)),
    "tail4": ((0, 2.43, 0.60), (0.22, 0.23)),
    "tail3": ((0, 1.96, 0.65), (0.26, 0.27)),
    "tail2": ((0, 1.46, 0.70), (0.30, 0.31)),
    "hips": ((0, 0.88, 0.72), (0.34, 0.35)),
    "belly": ((0, 0.20, 0.78), (0.38, 0.41)),
    "chest": ((0, -0.44, 0.86), (0.39, 0.45)),
    "neck1": ((0, -0.88, 1.22), (0.28, 0.30)),
    "neck2": ((0, -1.07, 1.58), (0.22, 0.23)),
    "neck3": ((0, -1.12, 1.91), (0.195, 0.2)),
    "head": ((0, -1.26, 2.20), (0.33, 0.30)),
    "muzzle": ((0, -1.57, 2.14), (0.2, 0.175)),
    "snout": ((0, -1.81, 2.08), (0.135, 0.12)),
}, {
    "shoulder": ((0.27, -0.42, 0.62), (0.19, 0.21)),
    "wrist": ((0.35, -0.46, 0.15), (0.13, 0.13)),
    "toe_f": ((0.36, -0.67, 0.075), (0.16, 0.085)),
    "hipj": ((0.27, 0.86, 0.52), (0.21, 0.23)),
    "ankle": ((0.35, 0.90, 0.15), (0.135, 0.135)),
    "toe_b": ((0.36, 0.69, 0.075), (0.16, 0.085)),
})
GROWN_EDGES = [("tail_tip", "tail8"), ("tail8", "tail7"), ("tail7", "tail6"), ("tail6", "tail5"),
               ("tail5", "tail4"), ("tail4", "tail3"), ("tail3", "tail2"), ("tail2", "hips"),
               ("hips", "belly"), ("belly", "chest"), ("chest", "neck1"), ("neck1", "neck2"),
               ("neck2", "neck3"), ("neck3", "head"), ("head", "muzzle"), ("muzzle", "snout")]
for _side in ("L", "R"):
    GROWN_EDGES += [("chest", f"shoulder_{_side}"), (f"shoulder_{_side}", f"wrist_{_side}"),
                    (f"wrist_{_side}", f"toe_f_{_side}"),
                    ("hips", f"hipj_{_side}"), (f"hipj_{_side}", f"ankle_{_side}"),
                    (f"ankle_{_side}", f"toe_b_{_side}")]

BUILDS = {
    "neutral": {},
    "sturdy": {"chest": (1.07, 0.98), "belly": (1.07, 0.98), "hips": (1.06, 0.98), "neck1": (1.05, 0.97),
               "tail1": (1.05, 0.97), "tail2": (1.04, 0.98), "arm_up": (1.08, 0.97), "leg_up": (1.08, 0.97)},
    "sleek": {"chest": (0.93, 1.02), "belly": (0.92, 1.03), "hips": (0.94, 1.02), "tail2": (0.94, 1.03),
              "tail3": (0.94, 1.04), "tail4": (0.94, 1.04), "neck2": (0.95, 1.03)},
    "long": {"neck1": (0.97, 1.07), "neck2": (0.97, 1.08), "belly": (0.97, 1.07), "tail3": (0.97, 1.07),
             "tail4": (0.97, 1.08), "tail5": (0.97, 1.08), "tail6": (0.97, 1.08)},
}


def _grown_sculpt(kit, obj):
    """A dragon-horse's head: a rounded brow and full cheeks; a gently flattened belly so it
    sits low and long."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for v in bm.verts:
        x, y, z = v.co
        if -1.46 < y < -1.1 and 2.1 < z < 2.48:  # round cheeks
            v.co.x *= 1.08
        if -1.62 < y < -1.3 and z > 2.36 and abs(x) < 0.26:  # a soft brow over the eyes
            v.co.z += 0.03 * (1 - abs(x) / 0.26)
        if -0.7 < y < 1.1 and z < 0.56:  # the belly flattens a touch underneath
            v.co.z += 0.03 * min(1.0, (0.56 - z) / 0.1)
    bm.to_mesh(obj.data)
    bm.free()


GROWN = dict(
    name="grown", nodes=GROWN_NODES, edges=GROWN_EDGES, body="skin",
    body_tris=1430, body_tris_lod1=500, export_scale=1.0,
    young={
        "bones": {
            "head": (0.95, 0.9, 1.0), "snout": (0.9, 0.72, 0.95),
            "neck1": (0.74, 0.6), "neck2": (0.76, 0.6), "neck3": (0.8, 0.6),
            "chest": (0.72, 0.62), "belly": (0.7, 0.6), "hips": (0.72, 0.62),
            "tail1": (0.7, 0.62), "tail2": (0.72, 0.6), "tail3": (0.74, 0.6), "tail4": (0.76, 0.6),
            "tail5": (0.78, 0.6), "tail6": (0.8, 0.6), "tail7": (0.84, 0.62), "tail8": (0.88, 0.66),
            "arm_up": (0.74, 0.72), "arm_lo": (0.82, 0.8), "leg_up": (0.74, 0.72), "leg_lo": (0.82, 0.8),
        },
        "parts": {"eyes": 1.3, "horns": 0.55, "frill": 0.8, "wings": 0.5, "spikes": 0.6,
                  "tail_tip": 0.65, "heart": 0.85, "runes": 0.7},
    },
    young_pose={"neck1": -10, "neck2": -4, "head": 10},
    # bone-local Euler degrees (x: the tip up, z: the tip sideways): the tail sweeps in an S and
    # its end curls up so the fan fin stands clear of the floor
    base_pose={"neck1": (2, 0, 0), "head": (-4, 0, 0),
               "tail1": (2, 0, 10), "tail2": (0, 0, 16), "tail3": (0, 0, -12), "tail4": (0, 0, -24),
               "tail5": (6, 0, -22), "tail6": (12, 0, -6), "tail7": (16, 0, 16), "tail8": (20, 0, 22)},
    builds=BUILDS,
    eyes=dict(at=(0.19, -1.48, 2.29), out=(0.62, -0.7, 0.2), iris=(0.1, 0.11, 0.052),
              pupil=(0.058, 0.072, 0.02), slit=(0.3, 1.12),
              glints=((-0.021, 0.032, 0.016), (0.016, -0.032, 0.008)), seg=(12, 2, 8, 2)),
    head=dict(origin=(0, -1.26, 2.2), k=0.95, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=False,
              frill_k=1.0, feather_w=1.0),
    tail_k=1.0,
    heart=dict(at=(0, -0.84, 0.8), size=0.105),
    wing=dict(root=(0.18, -0.32, 1.25), scale=0.78, dihedral=plan.FIN_DIHEDRAL, droop=plan.FIN_DROOP,
              layout=plan.FIN_LAYOUT,
              radii={"root": 0.07, "elbow": 0.05, "wrist": 0.05, "finger": 0.018, "tip": 0.008},
              arm_tris=110, thickness=0.012, style="fin"),
    mask=dict(max_x=0.3, max_z=2.3, min_z=-1.0, tail_cut=None),
    inset={"eyes": 0.02, "horns": 0.012, "spikes": 0.03, "frill": 0.04, "heart": -0.05, "runes": -0.008,
           "tail_tip": 0.03},
    face=dict(nostril=(0.055, -1.92, 2.13), nostril_r=(0.02, 0.013, 0.007), mouth_r=0.01,
              mouth=lambda side, a: (side * 0.13 * a ** 0.7, -1.94 + 0.38 * a ** 1.5, 2.015 + 0.05 * a * a)),
    jaw_hinge=(0, -1.5, 2.03),
    mouth_detail=dict(depth=0.2, fade=0.16, width=0.3, tooth=(0.008, 0.014), fang=(0.01, 0.022),
                      tongue=(0.05, 0.075, 0.012), fangs=[]),
    skin=dict(stripe=0.36, spot_cell=0.22, ao=0.5),
    sculpt=_grown_sculpt,
)

# ------------------------------------------------------------------------------ hatchling
HATCH_NODES = _mirrored({
    "tail_tip": ((0, 1.48, 0.26), None),
    "tail8": ((0, 1.38, 0.25), None),
    "tail7": ((0, 1.27, 0.24), None),
    "tail6": ((0, 1.15, 0.24), None),
    "tail5": ((0, 1.02, 0.25), None),
    "tail4": ((0, 0.88, 0.27), None),
    "tail3": ((0, 0.73, 0.29), None),
    "tail2": ((0, 0.58, 0.32), None),
    "hips": ((0, 0.38, 0.35), None),
    "belly": ((0, 0.12, 0.36), None),
    "chest": ((0, -0.13, 0.40), None),
    "neck1": ((0, -0.25, 0.52), None),
    "neck2": ((0, -0.32, 0.63), None),
    "neck3": ((0, -0.38, 0.74), None),
    "head": ((0, -0.45, 0.88), None),
    "muzzle": ((0, -0.72, 0.84), None),
    "snout": ((0, -0.86, 0.81), None),
}, {
    "shoulder": ((0.14, -0.13, 0.30), None),
    "wrist": ((0.16, -0.16, 0.09), None),
    "toe_f": ((0.17, -0.27, 0.045), None),
    "hipj": ((0.14, 0.38, 0.28), None),
    "ankle": ((0.16, 0.40, 0.09), None),
    "toe_b": ((0.17, 0.29, 0.045), None),
})

HATCH_META = [
    ("ell", (0, -0.48, 0.92), (0.33, 0.30, 0.29)),       # big round head
    ("ell", (0, -0.70, 0.83), (0.19, 0.15, 0.12)),       # a wide soft muzzle
    ("ell", (0, -0.62, 0.76), (0.15, 0.14, 0.08)),       # chin
    ("chain", [(0, -0.24, 0.54), (0, -0.33, 0.64), (0, -0.40, 0.76)], [0.15, 0.15, 0.16]),
    ("ell", (0, -0.13, 0.40), (0.19, 0.21, 0.19)),       # chest
    ("ell", (0, 0.12, 0.36), (0.19, 0.24, 0.18)),        # a round little tummy
    ("ell", (0, 0.36, 0.35), (0.165, 0.2, 0.165)),       # hips
    ("chain", [(0, 0.52, 0.33), (0, 0.66, 0.31), (0, 0.80, 0.28), (0, 0.95, 0.26), (0, 1.09, 0.245),
               (0, 1.22, 0.24), (0, 1.33, 0.245), (0, 1.43, 0.255), (0, 1.49, 0.26)],
     [0.13, 0.115, 0.1, 0.085, 0.072, 0.06, 0.05, 0.04, 0.03]),  # the noodle of a tail
]
for _s in (-1, 1):
    HATCH_META += [
        ("ball", (_s * 0.17, -0.66, 0.84), 0.11),                    # cheeks
        ("chain", [(_s * 0.14, -0.13, 0.28), (_s * 0.16, -0.16, 0.10)], [0.075, 0.07]),
        ("ell", (_s * 0.165, -0.22, 0.05), (0.075, 0.095, 0.05)),    # front paws
        ("chain", [(_s * 0.14, 0.38, 0.26), (_s * 0.16, 0.40, 0.10)], [0.08, 0.07]),
        ("ell", (_s * 0.165, 0.33, 0.05), (0.075, 0.095, 0.05)),     # hind paws
    ]

HATCH = dict(
    name="hatchling", nodes=HATCH_NODES, meta=HATCH_META, body="meta",
    body_tris=1500, body_tris_lod1=540, export_scale=1.0,
    young={
        "bones": {name: ((0.76, 0.76) if name in ("head", "snout") else (0.64, 0.64))
                  for name in ["hips", "belly", "chest", "neck1", "neck2", "neck3", "head", "snout",
                               "arm_up", "arm_lo", "leg_up", "leg_lo"] + [f"tail{k}" for k in range(1, 9)]},
        "parts": {"eyes": 1.1, "horns": 0.7, "frill": 0.85, "wings": 0.6, "spikes": 0.8,
                  "tail_tip": 0.8, "heart": 1.0, "runes": 0.9},
    },
    base_pose={"head": (2, 0, 0), "tail1": (0, 0, 10), "tail2": (0, 0, 20), "tail3": (0, 0, 26),
               "tail4": (2, 0, 30), "tail5": (4, 0, 32), "tail6": (6, 0, 30), "tail7": (8, 0, 28),
               "tail8": (10, 0, 26)},
    builds={"neutral": {}, "sturdy": {}, "sleek": {}, "long": {}},
    eyes=dict(at=(0.165, -0.70, 0.95), out=(0.46, -0.86, 0.1), iris=(0.112, 0.126, 0.062),
              pupil=(0.078, 0.092, 0.028), slit=(0.32, 1.1),
              glints=((-0.027, 0.042, 0.027), (0.023, -0.04, 0.012)), seg=(14, 3, 12, 2)),
    head=dict(origin=(0, -0.48, 0.92), k=0.62, horn_len=0.5, horn_r=0.8, horn_curve=0.6, buds=True,
              frill_k=0.7, feather_w=1.4),
    tail_k=0.34,
    heart=dict(at=(0, -0.30, 0.38), size=0.07),
    wing=dict(root=(0.1, -0.10, 0.54), scale=0.2, dihedral=plan.FIN_DIHEDRAL, droop=plan.FIN_DROOP,
              layout=plan.FIN_LAYOUT,
              radii={"root": 0.035, "elbow": 0.026, "wrist": 0.026, "finger": 0.009, "tip": 0.004},
              arm_tris=100, thickness=0.008, style="fin"),
    mask=dict(max_x=0.2, max_z=0.9, min_z=0.0, tail_cut=None),
    inset={"eyes": 0.03, "horns": 0.01, "spikes": 0.012, "frill": 0.025, "heart": -0.03, "runes": -0.006,
           "tail_tip": 0.015},
    face=dict(nostril=(0.04, -0.865, 0.85), nostril_r=(0.02, 0.013, 0.008), mouth_r=0.01,
              mouth=lambda side, a: (side * 0.15 * a ** 0.8, -0.87 + 0.2 * a ** 1.6, 0.775 + 0.035 * a * a)),
    jaw_hinge=(0, -0.64, 0.77),
    mouth_detail=dict(depth=0.13, fade=0.1, width=0.3, tooth=(0.007, 0.012), fang=(0.01, 0.02),
                      tongue=(0.05, 0.075, 0.012), fangs=[]),
    teeth=False,
    skin=dict(stripe=0.16, spot_cell=0.1, ao=0.22),
)

FORMS = {"grown": GROWN, "hatchling": HATCH}


# ------------------------------------------------------------------------------ part shapes
def _fan(kit, name, base, rays, notch=0.7, round_=0.28, width=0.5, thickness=0.012, cup=None):
    """A fin fanned from `base` over rays [(direction, length)]: each ray ends in a soft round
    lobe (width: its half-width as a share of the gap to the next ray; round_: how much its
    sides fall back), with a notch between neighbours (notch: how far out it sits, 0..1).
    cup: (direction, amount): the fin curls that way toward its edge, so it isn't flat card."""
    V = kit.V
    b = V(base)
    tips = [(V(d).normalized(), ln) for d, ln in rays]
    offs = kit.lod((-1.0, -0.62, -0.28, 0.0, 0.28, 0.62, 1.0), (-1.0, 0.0, 1.0))
    pts = [b]
    for j, (d, ln) in enumerate(tips):
        if j > 0:
            pd, pl = tips[j - 1]
            pts.append(b + (pd.lerp(d, 0.5)).normalized() * (pl + ln) * 0.5 * notch)
        prev_d = tips[j - 1][0] if j > 0 else d.lerp(tips[j + 1][0], -1.0).normalized()
        next_d = tips[j + 1][0] if j + 1 < len(tips) else d.lerp(tips[j - 1][0], -1.0).normalized()
        for o in offs:
            side = next_d if o > 0 else prev_d
            dd = d.lerp(side, abs(o) * width * 0.5).normalized()
            pts.append(b + dd * ln * (1 - round_ * (1 - math.cos(o * math.pi / 2))))
    if cup:
        reach = max(ln for _, ln in tips)
        cd, amount = V(cup[0]).normalized(), cup[1]
        pts = [pts[0]] + [p + cd * amount * ((p - b).length / reach) ** 2 for p in pts[1:]]
    return kit.flat_fan(name, pts, thickness)


def _ribbon(kit, name, base, path, width, face, thickness=0.01):
    """A flowing ribbon from `base` through the relative points `path`: a strip `width` wide at
    its fullest (narrow at the root, a rounded end), its flat side toward `face`."""
    import bmesh
    V = kit.V
    b = V(base)
    pts = [V((0, 0, 0))] + [V(p) for p in path]
    # a smooth centre line: each span halved (Chaikin once) at full detail
    if kit.LOD == 0:
        fine = [pts[0]]
        for a, c in zip(pts, pts[1:]):
            fine += [a.lerp(c, 0.25), a.lerp(c, 0.75)]
        pts = fine + [pts[-1]]
    n = len(pts)
    bm = bmesh.new()
    rows = []
    for i, p in enumerate(pts):
        d = (pts[min(i + 1, n - 1)] - pts[max(i - 1, 0)]).normalized()
        sd = d.cross(V(face)).normalized()
        f = i / (n - 1)
        w = width * (0.45 + 0.55 * math.sin(math.pi * min(1.0, f * 1.6) / 2)) * (1 - 0.75 * f ** 3)
        rows.append((bm.verts.new(p + sd * w * 0.5), bm.verts.new(p - sd * w * 0.5)))
    for (a0, a1), (c0, c1) in zip(rows, rows[1:]):
        bm.faces.new((a0, c0, c1, a1))
    obj = kit.mesh_object(name, bm, b)
    m = obj.modifiers.new("thick", "SOLIDIFY")
    m.thickness = thickness
    m.offset = 0
    m.use_rim = False
    kit.apply_modifiers(obj)
    kit.smooth(obj)
    return obj


def _whisker(kit, name, root, pts_rel, r0, lod_ring=(4, 3)):
    """A soft whisker: a tapering tube through root + the relative points."""
    V = kit.V
    pts = [V(root)] + [V(root) + V(p) for p in pts_rel]
    n = len(pts)
    radii = [r0 * (1 - 0.8 * (i / (n - 1)) ** 1.2) for i in range(n)]
    return kit.tube(name, pts, radii, ring=kit.lod(*lod_ring))


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


def _dorsal(kit, d, mats, path, material, rare=False):
    """A ribbon of fin down the back: one piece per spine bone, each a rounded lobe that sweeps
    back and overlaps the next, so the ribbon stays whole as the body waves.
    path: [(node, next node, bone, height)]."""
    V = kit.V
    pieces = []
    for i, (a, b, bone, h) in enumerate(path):
        spine = kit.node(b) - kit.node(a)
        up = V((0, -spine.z, spine.y)).normalized()
        if up.z + 0.3 * up.y < 0:  # up, or back along an upright neck
            up = -up
        pa, pb = _skin_top(kit, d, kit.node(a), up), _skin_top(kit, d, kit.node(b), up)
        along = pb - pa
        back = along.normalized()
        end = pa + along * 1.18
        n = kit.lod(5, 2)
        pts = [pa]
        for j in range(n + 1):
            f = j / n
            # a lobe: rising from the front, highest past the middle, swept back at the top
            hh = h * math.sin(math.pi * (0.12 + 0.8 * f)) ** 0.8
            pts.append(pa.lerp(end, f) + up * hh + back * hh * 0.35)
        pts.append(end)
        o = kit.flat_fan(f"dorsal{'r' if rare else ''}_{i}", pts, 0.012 if kit.F["name"] == "grown" else 0.008)
        o.data.materials.append(mats[material])
        pieces.append((o, bone))
    return pieces


# ------------------------------------------------------------------------------ hooks
def parts(kit, d):
    F, mats, V = kit.F, d["mats"], kit.V
    baby = d["form"] == "hatchling"
    out = []
    # ---- the frilled crown: fin-ears sweeping back from the back of the head
    for rare in (False, True):
        frills = []
        for s in (-1, 1):
            side = V((s, 0, 0))
            if baby:  # frilly axolotl gills: three round lobes fanned out behind each cheek
                base = kit.head_point((0.24, 0.14, 0.04), s)
                f = _fan(kit, f"gill_{s}", base,
                         [(V((s * 0.55, 0.45, 0.72)), 0.3), (V((s * 0.7, 0.62, 0.2)), 0.34),
                          (V((s * 0.66, 0.6, -0.3)), 0.27)],
                         notch=0.6, round_=0.32, width=0.75, thickness=0.012, cup=(side, 0.05))
                f.data.materials.append(mats["glow_flat" if rare else "membrane"])
                frills.append((f, "head"))
            else:  # the adult's crown: long lobes swept back from behind the eyes, a cheek fin below
                k = 1.15 if rare else 1.0
                base = kit.head_point((0.16, 0.14, 0.1), s)
                ear = _fan(kit, f"earfin_{s}", base,
                           [(V((s * 0.24, 0.55, 0.8)), 0.62 * k), (V((s * 0.32, 0.86, 0.4)), 0.84 * k),
                            (V((s * 0.34, 0.94, 0.02)), 0.74 * k)],
                           notch=0.58, round_=0.34, width=0.8, thickness=0.013, cup=(side, 0.14))
                ear.data.materials.append(mats["membrane"])
                frills.append((ear, "head"))
                cheek = _fan(kit, f"cheekfin_{s}", kit.head_point((0.19, 0.04, -0.12), s),
                             [(V((s * 0.5, 0.82, -0.05)), 0.34 * k), (V((s * 0.45, 0.8, -0.45)), 0.3 * k)],
                             notch=0.55, round_=0.3, width=0.62, thickness=0.011, cup=(side, 0.06))
                cheek.data.materials.append(mats["membrane"])
                frills.append((cheek, "head"))
                if rare:  # a long glowing ribbon streaming back from the crown
                    rib = _ribbon(kit, f"crownribbon_{s}", kit.head_point((0.12, 0.2, 0.2), s),
                                  [(s * 0.1, 0.3, 0.22), (s * 0.22, 0.62, 0.3), (s * 0.3, 0.98, 0.22),
                                   (s * 0.34, 1.3, 0.06)], 0.12, side)
                    rib.data.materials.append(mats["glow_flat"])
                    frills.append((rib, "head"))
        out.append(("frill", 1 if rare else 0, frills))
    # ---- whiskers: long and soft, curling down from the lip (longer and glowing on the rare one)
    if not baby:
        for rare in (False, True):
            wh = []
            k = 1.4 if rare else 1.0
            for s in (-1, 1):
                root = (s * 0.11, -1.84, 2.08)
                # out from the lip, curving down past the chin, then trailing back like river weed
                rel = [(s * 0.06, -0.05, -0.01), (s * 0.13, -0.08, -0.1), (s * 0.17, -0.05, -0.28 * k),
                       (s * 0.18, 0.06, -0.46 * k), (s * 0.17, 0.24 * k, -0.6 * k),
                       (s * 0.15, 0.44 * k, -0.68 * k), (s * 0.13, 0.62 * k, -0.7 * k)]
                w = _whisker(kit, f"whisker_{s}", root, rel, 0.022)
                w.data.materials.append(mats["glow_flat" if rare else "membrane"])
                wh.append((w, "snout"))
            out.append(("horns", 1 if rare else 0, wh))
    # ---- the rare one's pearl, set in its brow
    pearl = kit.blob("pearl", kit.head_point((0.0, -0.16, 0.25) if not baby else (0.0, -0.12, 0.3)),
                     (0.075, 0.075, 0.075) if not baby else (0.055, 0.055, 0.055), kit.lod(8, 6), kit.lod(4, 3))
    pearl.data.materials.append(mats["glow_flat"])
    out.append(("runes", 1, [(pearl, "head")]))
    # ---- a ribbon of fin down the back and tail
    if baby:
        path = [("chest", "belly", "chest", 0.07), ("belly", "hips", "belly", 0.08), ("hips", "tail2", "tail1", 0.07),
                ("tail2", "tail3", "tail2", 0.06), ("tail3", "tail4", "tail3", 0.05)]
    else:
        path = [("neck3", "head", "neck3", 0.12), ("neck2", "neck3", "neck2", 0.14),
                ("neck1", "neck2", "neck1", 0.17), ("chest", "belly", "chest", 0.26), ("belly", "hips", "belly", 0.28),
                ("hips", "tail2", "tail1", 0.26), ("tail2", "tail3", "tail2", 0.24), ("tail3", "tail4", "tail3", 0.22),
                ("tail4", "tail5", "tail4", 0.2), ("tail5", "tail6", "tail5", 0.18), ("tail6", "tail7", "tail6", 0.16)]
    out.append(("spikes", 0, _dorsal(kit, d, mats, path, "membrane")))
    out.append(("spikes", 1, _dorsal(kit, d, mats, path, "glow_flat", rare=True)))
    # ---- the tail's flowing fan fin
    x, y, z = F["nodes"]["tail_tip"][0]
    k = F["tail_k"]
    for rare in (False, True):
        kk = k * (1.15 if rare else 1.0)
        base = V((0, y - 0.3 * k, z + 0.02 * k))
        rays = [(V((0, 0.45, 0.9)), 0.9 * kk), (V((0, 0.95, 0.3)), 1.25 * kk), (V((0, 0.9, -0.42)), 1.1 * kk)]
        fin = _fan(kit, "tailfan", base, rays, notch=0.55, round_=0.3, width=0.7, thickness=0.014 * max(k, 0.5),
                   cup=(V((1, 0, 0)), 0.04 * k))
        fin.data.materials.append(mats["glow_flat" if rare else "membrane"])
        fins = [(fin, "tail8")]
        for s in (-1, 1):  # a pair of lower side lobes, so the fin reads from above too
            side = _fan(kit, f"tailside_{s}", base + V((0, 0.06 * k, -0.01 * k)),
                        [(V((s * 0.62, 0.78, -0.08)), 0.58 * kk), (V((s * 0.3, 0.95, -0.2)), 0.72 * kk)],
                        notch=0.6, round_=0.3, width=0.7, thickness=0.012 * max(k, 0.5),
                        cup=(V((0, 0, 1)), 0.05 * k))
            side.data.materials.append(mats["membrane"])
            fins.append((side, "tail8"))
        if rare:  # two long streamers trailing from the fan
            for s in (-1, 1):
                st = _ribbon(kit, f"streamer_{s}", base + V((s * 0.04 * k, 0.1 * k, -0.02 * k)),
                             [(s * 0.1 * k, 0.5 * k, -0.2 * k), (s * 0.18 * k, 1.0 * k, -0.26 * k),
                              (s * 0.22 * k, 1.5 * k, -0.12 * k), (s * 0.2 * k, 1.9 * k, 0.08 * k)],
                             0.13 * k, V((0, 0, 1)))
                st.data.materials.append(mats["membrane"])
                fins.append((st, "tail8"))
        out.append(("tail_tip", 1 if rare else 0, fins))
    return out


def wings(kit, d, rare):
    """Frilled fin-wings: a short fleshy base and four fin rays fanning back, the membrane
    between them lobed at every ray's tip."""
    if rare:
        return []
    F, mats = kit.F, d["mats"]
    wr = F["wing"]["radii"]
    objs = []
    for side in ("L", "R"):
        w = kit.wing_points(side)
        arm = kit.wing_arm(side, ["root", "wrist", "t1", "t2", "t3", "t4"],
                           [wr["root"], wr["wrist"], wr["finger"], wr["finger"], wr["finger"], wr["finger"]],
                           kit.lod(F["wing"]["arm_tris"], 48), mats, material="body_plain", claws=False)
        objs.append(arm)
        k = w["wrist"]
        # the trailing edge: a soft round lobe at every ray's tip, a shallow notch between
        order = ["t4", "t3", "t2", "t1"]
        rays = [((w[t] - k).normalized(), (w[t] - k).length) for t in order]
        offs = kit.lod((-1.0, -0.5, 0.0, 0.5, 1.0), (0.0,))
        edge = [w["body"]]
        for j, (d, ln) in enumerate(rays):
            if j > 0:
                pd, pl = rays[j - 1]
                edge.append(k + pd.lerp(d, 0.5).normalized() * (pl + ln) * 0.5 * 0.8)
            prev_d = rays[j - 1][0] if j > 0 else (w["body"] - k).normalized()
            next_d = rays[j + 1][0] if j + 1 < len(rays) else d.lerp(rays[j - 1][0], -1.0).normalized()
            for o in offs:
                dd = d.lerp(next_d if o > 0 else prev_d, abs(o) * 0.3).normalized()
                edge.append(k + dd * ln * (1 - 0.2 * (1 - math.cos(o * math.pi / 2))))
        pts = [k, w["root"]] + edge
        mem = kit.flat_fan(f"finwing_{side}", pts, F["wing"]["thickness"])
        mem.data.materials.append(mats["membrane"])
        objs.append(mem)
    return objs


def accent(kit, body):
    """A pale stripe along the whole underside, from the chin down the throat and belly to the
    tail's tip, and a pale muzzle underneath."""
    import mathutils
    mask = kit.mask_attr(body)
    F = kit.F
    baby = F["name"] == "hatchling"
    down = mathutils.Vector((0, 0, -1))
    fwd = mathutils.Vector((0, -1, 0))
    for v in body.data.vertices:
        x, y, z = v.co
        n = v.normal
        a = min(1.0, max(0.0, (n.dot(down) - 0.4) / 0.3))
        # the throat and chest face forward as the neck rises
        neck_y = -0.25 if baby else -0.6
        if y < neck_y:
            a = max(a, min(1.0, max(0.0, (n.dot((fwd * 0.8 + down * 0.6).normalized()) - 0.35) / 0.35)))
        if z < (0.13 if baby else 0.3) and abs(x) > (0.1 if baby else 0.2):  # the paws stay base colour
            a = 0.0
        mask.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


def texture(tx, nt, p, form):
    """R: soft dapples along the back and flanks; G: koi-like patches; B: the rare variant's
    glowing pearl spots in rows down each flank."""
    r = tx.spots(nt, p["spot_cell"], keep=0.4, size=(0.32, 0.2), where=tx.top(nt))
    g = tx.blotches(nt, 1.0 / (p["spot_cell"] * 3.2), threshold=(0.54, 0.6), where=tx.top(nt))
    pearl = tx.spots(nt, p["spot_cell"] * 0.8, keep=0.2, size=(0.22, 0.12), seed_offset=3.1,
                     where=tx.upper(nt))
    return {"r": r, "g": g, "b": pearl}
