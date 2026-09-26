"""The Flurrytail (Frost, uncommon): a snow-fox of a dragon.

Design notes
  Baby     a round fluffball: a big round head sitting right on a round body, huge eyes, fluffy
           cheeks, a tiny button muzzle, stubby legs, fox ears with coloured tips, two little
           crystal antler buds, and a HUGE pom-pom tail (a ball ringed with soft fur points) on
           a short stalk, nearly as big as its head.
  Adult    elegant and fox-like, its neck carried upright and its head high (base_pose): slender
           legs with neat paws and fluffy trousers, a deep chest under a full round ruff, a
           mane of bold locks down the back of the neck, cheek fluff
           flaring back, a short pointed muzzle, tall fox ears and branching ice-crystal
           antlers. Its signature is the great plumed tail, bigger than its body, that rises
           behind it and curls forward over its back, dressed in soft locks along its outer
           edge. Majestic like a snow spirit; kind, bright eyes. The heartglow sits on the
           chest just under the ruff's hem, in plain view.
  Wings    medium bat wings (their own layout: a long arm and forearm and a short hand, so
           they fold into a neat cape that ends at the rump), with a frosted band and an
           icicle hanging from every scallop of the trailing edge.
  Marks    "points" (socks, ear tips and the plume's tip) in the pattern colour; snowflakes on
           the thighs; snow-leopard rosettes; the rare one's glowing aurora ribbons.
  Variants Snowdrift (snow white, ice-blue points: the natural one), Moonmist (silver-grey,
           lilac rosettes, amber eyes: the subtle one), Foxfire (pale fox-orange, cream bib and
           points, ice-blue eyes and antlers: the surprise), and the rare Aurora: a midnight
           coat with violet points, glowing seafoam aurora ribbons across the plume, bigger
           crystal antlers with glowing tines, glowing snowflakes and icicles.
  Motion   light and bouncy; a fox's pouncing dive; a big, slow, luxurious tail sway; asleep
           curled round with its nose tucked into its tail (plans/flurrytail.py).
  Build    both forms are metaballs, so the fluff (ruff, mane, cheeks, plume) is body
           geometry that bends with the bones; the sculpt hook re-meshes them into even,
           flowing quads (QuadriFlow) so the fur shades in clean toon bands. Six tail bones
           carry the plume.
Concept: docs/art/concept/dragons/dragon_flurrytail.jpg (reference only, D74).
"""
import math

META = dict(
    name="flurrytail", title="Flurrytail", dex=6, element="Frost", parents=(), rarity="uncommon",
    plan="flurrytail", size=0.9,
    stats=dict(wing=7, wit=8, might=4, breath=6, stamina=6),
    manners=("Curious", "Playful", "Gentle", "Mischievous"),
    traits=("Sure-Footed", "Quick Learner", "Keen Nose", "Warm-Blooded", "Tidy"),
    rare_variant=3, rare_replaces=True,
    blurb="Light as new snow; it pounces on anything that moves and sleeps with its nose tucked in its tail.",
)

VARIANTS = [
    dict(name="Snowdrift", base=(0.84, 0.88, 0.98), accent=(1.0, 1.0, 1.0), pattern=(0.42, 0.62, 0.95),
         horn=(0.56, 0.78, 1.0), membrane=(0.66, 0.80, 0.98), iris=(0.10, 0.26, 0.66), glow=(0.30, 0.56, 1.0),
         pattern_channel="r", glow_channel=None),
    dict(name="Moonmist", base=(0.58, 0.60, 0.68), accent=(0.93, 0.90, 1.0), pattern=(0.36, 0.28, 0.62),
         horn=(0.80, 0.70, 1.0), membrane=(0.70, 0.62, 0.92), iris=(0.85, 0.58, 0.16), glow=(0.56, 0.44, 1.0),
         pattern_channel="g", glow_channel=None),
    dict(name="Foxfire", base=(0.96, 0.60, 0.34), accent=(1.0, 0.94, 0.84), pattern=(1.0, 0.95, 0.88),
         horn=(0.70, 0.88, 1.0), membrane=(0.98, 0.78, 0.60), iris=(0.20, 0.50, 0.95), glow=(0.36, 0.64, 1.0),
         pattern_channel="r", glow_channel=None),
    dict(name="Aurora", base=(0.06, 0.08, 0.22), accent=(0.78, 0.88, 1.0), pattern=(0.40, 0.24, 0.80),
         horn=(0.50, 0.92, 0.86), membrane=(0.16, 0.20, 0.48), iris=(0.30, 0.95, 0.80), glow=(0.32, 1.0, 0.74),
         pattern_channel="r", glow_channel="b"),
]

EGG = dict(height=1.0, width=0.41, asym=0.08, point=1.0, speckle="flakes",
           speckle_params=dict(count=19, size=(0.04, 0.075)),
           colors=[((0.95, 0.97, 1.0), (0.45, 0.65, 0.95)), ((0.80, 0.80, 0.88), (0.62, 0.48, 0.88)),
                   ((1.0, 0.90, 0.80), (0.95, 0.60, 0.34)), ((0.18, 0.22, 0.46), (0.36, 1.0, 0.74))])


def _mirrored(center, sides):
    nodes = dict(center)
    for name, (p, r) in sides.items():
        nodes[f"{name}_R"] = ((p[0], p[1], p[2]), r)
        nodes[f"{name}_L"] = ((-p[0], p[1], p[2]), r)
    return nodes


def _both(elements):
    """Mirror side elements (x > 0) onto both sides."""
    out = []
    for kind, c, size in elements:
        if kind == "chain":
            out += [(kind, [(s * p[0], p[1], p[2]) for p in c], size) for s in (-1, 1)]
        else:
            out += [(kind, (s * c[0], c[1], c[2]), size) for s in (-1, 1)]
    return out


def _add(a, b, k=1.0):
    return tuple(x + y * k for x, y in zip(a, b))


def _unit(v):
    ln = math.sqrt(sum(x * x for x in v)) or 1.0
    return tuple(x / ln for x in v)


def tuft(root, tip, r0, r1=None):
    """A lock of fur: a tapered chain from inside the body out to a soft, rounded point."""
    return ("chain", [root, tip], [r0, r1 if r1 is not None else r0 * 0.45])


def _plume(points, radii, rows, start=0.12, stop=0.97, reach=1.05, sweep=0.75, root_k=0.5, tip_k=0.1):
    """A fluffy tail: a chain of balls along the curve, dressed in rows of pointed locks
    lying back toward the tip. rows: [(angle, count, stagger)]: the angle round the tail
    from its outer (convex) edge, 0; +-90 are its sides. Mirrored rows keep it symmetric."""
    els = [("chain", points, radii)]
    m = len(points)
    for ang, n, stagger in rows:
        for j in range(n):
            f = (start + (stop - start) * (j + stagger) / max(1, n - 1 + stagger)) * (m - 1)
            i = min(m - 2, int(f))
            u = f - i
            c = tuple(points[i][k] + (points[i + 1][k] - points[i][k]) * u for k in range(3))
            r = radii[i] + (radii[i + 1] - radii[i]) * u
            d = _unit(tuple(points[i + 1][k] - points[i][k] for k in range(3)))
            outer = _unit((0.0, d[2], -d[1]))  # the convex side of a tail curling forward
            th = math.radians(ang)
            o = _unit(_add(_add((0, 0, 0), outer, math.cos(th)), (1.0, 0.0, 0.0), math.sin(th)))
            root = _add(c, o, r * 0.35)
            tip = _add(_add(c, o, r * reach), d, r * sweep)
            els.append(tuft(root, tip, r * root_k, r * tip_k))
    return els


# The wing in its own plane (u out along the span, v back along the chord): a long arm and
# forearm, which fold onto each other, and a short hand with a shallow membrane, so the folded
# wing's fingers end at the rump instead of trailing to the ground.
WING_LAYOUT = {"root": (0.0, 0.0), "elbow": (1.05, 0.35), "wrist": (2.0, -0.1), "thumb": (2.06, -0.36),
               "f1": (3.3, 0.25), "f2": (3.15, 0.95), "f3": (2.75, 1.35), "f4": (2.15, 1.45), "body": (0.0, 1.15)}


def _even(kit, obj):
    """The form's sculpt hook: retopologise the metaball body into even, flowing quads
    (QuadriFlow, mirrored) at about twice the triangle target, then decimate it (mirrored)
    to a little under the target, so the kit doesn't decimate again. Toon bands run straight
    across each triangle, so a collapse-decimated blob of fur shades like crumpled paper;
    a mesh that follows the forms shades in clean, soft bands, the same on both sides.
    The mirrored mesh has an edge seam exactly on the midline, and the kit's seating ray for a
    part on the midline (the heart) runs in that plane and can slip between the seam's
    triangles, leaving the part unseated: the hearts are authored 2 mm off the midline."""
    import bpy
    target = int(kit.lod(kit.F["body_tris"], kit.F["body_tris_lod1"]) * 0.97)
    sm = obj.modifiers.new("soften", "SMOOTH")
    sm.factor, sm.iterations = 0.5, 1
    kit.apply_modifiers(obj)
    obj.data.use_mirror_x = True
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.quadriflow_remesh(mode="FACES", target_faces=int(target * 1.1), use_mesh_symmetry=True,
                                     use_preserve_sharp=False, use_preserve_boundary=False, smooth_normals=False,
                                     seed=3)
    obj.select_set(False)
    cur = kit.tri_count(obj)
    if cur > target:
        m = obj.modifiers.new("even", "DECIMATE")
        m.ratio = target / cur
        m.use_collapse_triangulate = True
        m.use_symmetry = True
        m.symmetry_axis = "X"
        kit.apply_modifiers(obj)


# ------------------------------------------------------------------------------ grown
# Faces -Y, Z up; units ~ metres at adult size. The tail rises behind the rump and curls
# forward over the back.
GROWN_TAIL = [(0, 0.58, 1.24), (0, 0.98, 1.52), (0, 1.30, 1.92), (0, 1.44, 2.42), (0, 1.34, 2.88),
              (0, 1.02, 3.18), (0, 0.62, 3.20)]
GROWN_NODES = _mirrored({
    "snout": ((0, -1.80, 2.23), None),
    "muzzle": ((0, -1.58, 2.31), None),
    "head": ((0, -1.34, 2.42), None),
    "neck3": ((0, -1.16, 2.08), None),
    "neck2": ((0, -0.94, 1.74), None),
    "chest": ((0, -0.50, 1.28), None),
    "hips": (GROWN_TAIL[0], None),
    **{f"tail{k}": (GROWN_TAIL[k - 1], None) for k in range(2, 7)},
    "tail_tip": (GROWN_TAIL[6], None),
}, {
    "shoulder": ((0.27, -0.52, 1.04), None),
    "elbow": ((0.29, -0.60, 0.58), None),
    "wrist": ((0.29, -0.64, 0.18), None),
    "toe_f": ((0.29, -0.86, 0.07), None),
    "hipj": ((0.27, 0.62, 1.04), None),
    "knee": ((0.31, 0.40, 0.62), None),
    "ankle": ((0.31, 0.76, 0.24), None),
    "toe_b": ((0.31, 0.56, 0.07), None),
})

GROWN_META = [
    # torso: a deep chest, a slim waist, round haunches
    ("ell", (0, -0.44, 1.26), (0.38, 0.44, 0.43)),
    ("ell", (0, 0.06, 1.28), (0.32, 0.46, 0.34)),
    ("ell", (0, 0.54, 1.28), (0.35, 0.36, 0.38)),
    # neck and head: a round skull, a short pointed fox muzzle
    ("chain", [(0, -0.62, 1.50), (0, -0.94, 1.78), (0, -1.14, 2.06), (0, -1.28, 2.28)], [0.27, 0.22, 0.2, 0.2]),
    ("ell", (0, -1.33, 2.45), (0.32, 0.31, 0.30)),
    ("ell", (0, -1.57, 2.33), (0.16, 0.19, 0.13)),
    ("ell", (0, -1.73, 2.26), (0.085, 0.11, 0.075)),
    ("ell", (0, -1.53, 2.22), (0.11, 0.15, 0.07)),
    # the mane: bold clumps lying back down the neck, from the nape to the withers
    ("ball", (0, -1.00, 2.18), 0.22),
    ("ball", (0, -0.80, 1.92), 0.24),
    tuft((0, -1.12, 2.30), (0, -0.82, 2.48), 0.2, 0.11),
    tuft((0, -0.92, 2.08), (0, -0.58, 2.20), 0.23, 0.12),
    tuft((0, -0.72, 1.84), (0, -0.38, 1.86), 0.22, 0.11),
    # the chest ruff: a full, round bib under the throat (the heartglow sits on its front,
    # where every camera sees it), its locks hanging either side
    ("ball", (0, -0.88, 1.62), 0.27),
]
GROWN_META += _both([
    tuft((0.16, -1.28, 2.26), (0.39, -1.11, 2.13), 0.15, 0.075),     # fluffy cheeks flaring back
    tuft((0.15, -1.00, 2.16), (0.33, -0.72, 2.18), 0.19, 0.095),     # the mane's sides
    tuft((0.17, -0.78, 1.88), (0.37, -0.48, 1.80), 0.2, 0.1),
    tuft((0.16, -0.86, 1.56), (0.26, -0.96, 1.22), 0.19, 0.095),    # ruff clumps
    tuft((0.30, -0.74, 1.62), (0.43, -0.78, 1.30), 0.16, 0.08),
    # legs: slender, with neat paws; a tuft at each elbow, fluffy trousers behind the thighs
    ("ell", (0.23, -0.50, 1.04), (0.17, 0.22, 0.27)),
    ("chain", [(0.28, -0.56, 0.88), (0.29, -0.60, 0.58), (0.29, -0.64, 0.18)], [0.16, 0.115, 0.09]),
    ("ell", (0.29, -0.72, 0.08), (0.105, 0.15, 0.08)),
    ("ell", (0.24, 0.58, 1.02), (0.20, 0.30, 0.34)),
    tuft((0.27, 0.72, 0.94), (0.32, 0.92, 0.66), 0.17, 0.09),
    ("chain", [(0.28, 0.54, 0.88), (0.31, 0.40, 0.62)], [0.17, 0.125]),
    ("chain", [(0.31, 0.40, 0.62), (0.31, 0.76, 0.24), (0.31, 0.64, 0.10)], [0.12, 0.09, 0.085]),
    ("ell", (0.31, 0.56, 0.08), (0.105, 0.15, 0.08)),
])
GROWN_META += _plume(GROWN_TAIL[1:], [0.17, 0.30, 0.40, 0.40, 0.32, 0.2], [(0, 5, 0.3), (60, 5, 0.0), (-60, 5, 0.0)],
                     reach=1.2, sweep=1.05, root_k=0.52, tip_k=0.26)
GROWN_META += [("chain", [(0, 0.42, 1.32), (0, 0.78, 1.40), (0, 0.98, 1.52)], [0.2, 0.15, 0.17]),
               tuft((0, 0.72, 3.20), (0, 0.34, 3.08), 0.19, 0.09)]      # the brush's soft point

GROWN = dict(
    name="grown", nodes=GROWN_NODES, meta=GROWN_META, body="meta", meta_resolution=0.035, voxel=0.05,
    # DR2 sizing: grown at the common scale, the Pouncer's bulk and length (the geometric mean of the
    # cube root of the body's volume and its length); META size then sizes it in the game.
    body_tris=1700, body_tris_lod1=560, export_scale=1.15,
    young={
        "bones": {
            "head": (1.1, 1.0, 1.1), "snout": (0.92, 0.72, 0.95),
            "neck2": (0.8, 0.55), "neck3": (0.82, 0.55),
            "chest": (0.68, 0.55), "hips": (0.68, 0.52),
            "tail1": (0.66, 0.55), "tail2": (0.7, 0.55), "tail3": (0.72, 0.58), "tail4": (0.74, 0.6),
            "tail5": (0.76, 0.62), "tail6": (0.78, 0.65),
            "arm_up": (0.7, 0.6), "arm_lo": (0.72, 0.6), "hand": (0.82, 0.74),
            "leg_up": (0.7, 0.6), "leg_lo": (0.72, 0.58), "foot": (0.82, 0.74),
        },
        "parts": {"eyes": 1.32, "horns": 0.45, "frill": 0.82, "wings": 0.45, "spikes": 0.6,
                  "tail_tip": 0.7, "heart": 0.85, "runes": 0.7},
    },
    young_pose={"neck2": -6, "neck3": 2, "head": 8},
    base_pose={"neck2": (-12, 0, 0), "neck3": (-6, 0, 0), "head": (16, 0, 0)},  # a proud, upright neck (plans: LIFT)
    eyes=dict(at=(0.16, -1.53, 2.49), out=(0.62, -0.78, 0.12), iris=(0.102, 0.114, 0.055),
              pupil=(0.06, 0.076, 0.022), slit=(0.3, 1.12),
              glints=((-0.028, 0.042, 0.02), (0.02, -0.042, 0.01)), seg=(12, 2, 8, 2)),
    head=dict(origin=(0, -1.33, 2.44), k=1.0, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=False,
              frill_k=1.0, feather_w=1.0),
    tail_k=1.0,
    heart=dict(at=(0.002, -1.12, 1.52), size=0.13, tilt=22),  # x: see _even (the midline seam)
    wing=dict(root=(0.26, -0.40, 1.58), scale=0.8, dihedral=50, droop=10,
              radii={"root": 0.08, "elbow": 0.06, "wrist": 0.048, "finger": 0.02, "tip": 0.008},
              arm_tris=120, thickness=0.014, frost=0.12, icicle=0.22, layout=WING_LAYOUT),
    mask=dict(max_x=0.3, max_z=2.3, min_z=-1.0, tail_cut=(0.8, 5.0)),
    inset={"eyes": 0.02, "horns": 0.03, "spikes": 0.03, "frill": 0.04, "heart": -0.022, "runes": -0.012},
    face=dict(nostril=(0.032, -1.83, 2.285), nostril_r=(0.02, 0.013, 0.008), mouth_r=0.01,
              mouth=lambda side, a: (side * 0.12 * a ** 0.7, -1.825 + 0.32 * a ** 1.5, 2.20 + 0.035 * a * a)),
    jaw_hinge=(0, -1.44, 2.21),
    mouth_detail=dict(depth=0.2, fade=0.16, width=0.3, tooth=(0.008, 0.014), fang=(0.011, 0.026),
                      tongue=(0.045, 0.085, 0.01)),
    skin=dict(stripe=0.4, spot_cell=0.26, ao=0.3),
    sculpt=_even,
)

# ------------------------------------------------------------------------------ hatchling
HATCH_TAIL = [(0, 0.22, 0.48), (0, 0.42, 0.56), (0, 0.54, 0.70), (0, 0.60, 0.86), (0, 0.58, 1.02),
              (0, 0.50, 1.14), (0, 0.38, 1.20)]
HATCH_NODES = _mirrored({
    "snout": ((0, -0.76, 0.88), None),
    "muzzle": ((0, -0.62, 0.91), None),
    "head": ((0, -0.38, 0.98), None),
    "neck3": ((0, -0.29, 0.80), None),
    "neck2": ((0, -0.21, 0.66), None),
    "chest": ((0, -0.12, 0.54), None),
    "hips": (HATCH_TAIL[0], None),
    **{f"tail{k}": (HATCH_TAIL[k - 1], None) for k in range(2, 7)},
    "tail_tip": (HATCH_TAIL[6], None),
}, {
    "shoulder": ((0.17, -0.14, 0.38), None),
    "elbow": ((0.18, -0.18, 0.23), None),
    "wrist": ((0.19, -0.20, 0.10), None),
    "toe_f": ((0.19, -0.33, 0.05), None),
    "hipj": ((0.17, 0.24, 0.38), None),
    "knee": ((0.19, 0.18, 0.24), None),
    "ankle": ((0.19, 0.28, 0.12), None),
    "toe_b": ((0.19, 0.14, 0.05), None),
})

POM = (0, 0.58, 0.98)  # the pom-pom's centre
HATCH_META = [
    ("ell", (0, -0.38, 1.00), (0.36, 0.33, 0.32)),     # big round head, sitting on the body
    ("ell", (0, -0.64, 0.91), (0.115, 0.10, 0.085)),   # tiny button muzzle
    ("ell", (0, -0.60, 0.85), (0.095, 0.09, 0.06)),    # chin
    ("ball", (0, 0.02, 0.47), 0.34),                   # a round fluffball of a body
    ("ell", (0, -0.16, 0.56), (0.26, 0.22, 0.26)),     # chest
    ("ball", (0, -0.26, 0.60), 0.19),                  # the ruff
    tuft((0, -0.28, 0.56), (0, -0.38, 0.40), 0.14, 0.07),
    ("chain", [(0, 0.28, 0.50), (0, 0.44, 0.58), (0, 0.54, 0.72)], [0.12, 0.1, 0.12]),   # the pom-pom's stalk
    ("ball", POM, 0.28),
]
HATCH_META += _both([
    ("ball", (0.21, -0.50, 0.89), 0.13),                          # fluffy cheeks
    tuft((0.22, -0.44, 0.90), (0.37, -0.34, 0.85), 0.12, 0.065),
    tuft((0.13, -0.24, 0.54), (0.20, -0.32, 0.38), 0.12, 0.06),   # ruff clumps
    ("chain", [(0.17, -0.14, 0.38), (0.18, -0.18, 0.23), (0.19, -0.20, 0.11)], [0.1, 0.085, 0.08]),
    ("ell", (0.19, -0.26, 0.06), (0.085, 0.11, 0.06)),  # front paws
    ("ell", (0.16, 0.24, 0.36), (0.13, 0.16, 0.17)),   # thighs
    ("chain", [(0.19, 0.28, 0.25), (0.19, 0.28, 0.12)], [0.09, 0.08]),
    ("ell", (0.19, 0.18, 0.06), (0.085, 0.11, 0.06)),   # hind paws
])
for _j in range(9):  # the pom-pom's soft points, none into its stalk
    _z = 1 - 2 * (_j + 0.5) / 9
    _rr = math.sqrt(max(0.0, 1 - _z * _z))
    _th = math.radians(137.5 * _j + 30)
    _o = (_rr * math.cos(_th), 0.3 + _rr * math.sin(_th) * 0.8, _z)
    _o = _unit(_o)
    if _o[2] < -0.5 and _o[1] < 0.2:
        continue
    HATCH_META.append(tuft(_add(POM, _o, 0.1), _add(POM, _o, 0.38), 0.13, 0.07))

HATCH = dict(
    name="hatchling", nodes=HATCH_NODES, meta=HATCH_META, body="meta",
    body_tris=1600, body_tris_lod1=560, export_scale=1.0,
    young={
        "bones": {name: ((0.74, 0.74) if name in ("head", "snout") else (0.62, 0.62))
                  for name in ("hips", "chest", "neck2", "neck3", "head", "snout", "tail1", "tail2", "tail3",
                               "tail4", "tail5", "tail6", "arm_up", "arm_lo", "hand", "leg_up", "leg_lo", "foot")},
        "parts": {"eyes": 1.12, "horns": 0.6, "frill": 0.75, "wings": 0.56, "spikes": 0.85,
                  "tail_tip": 0.9, "heart": 1.0, "runes": 0.9},
    },
    base_pose={"head": (4, 0, 0)},
    eyes=dict(at=(0.155, -0.68, 1.02), out=(0.45, -0.89, 0.08), iris=(0.108, 0.122, 0.06),
              pupil=(0.064, 0.078, 0.028), slit=(0.32, 1.1),
              glints=((-0.026, 0.04, 0.025), (0.022, -0.038, 0.012)), seg=(14, 3, 12, 2)),
    head=dict(origin=(0, -0.38, 1.08), k=0.8, horn_len=0.45, horn_r=0.9, horn_curve=0.6, buds=True,
              frill_k=0.7, feather_w=1.4),
    tail_k=0.4,
    heart=dict(at=(0.002, -0.36, 0.44), size=0.085, tilt=18),
    wing=dict(root=(0.13, -0.08, 0.74), scale=0.18, dihedral=50, droop=10,
              radii={"root": 0.042, "elbow": 0.033, "wrist": 0.028, "finger": 0.011, "tip": 0.005},
              arm_tris=90, thickness=0.008, frost=0.12, icicle=0.2, layout=WING_LAYOUT),
    mask=dict(max_x=0.22, max_z=1.06, min_z=0.13, tail_cut=(0.35, 5.0)),
    inset={"eyes": 0.04, "horns": 0.02, "spikes": 0.012, "frill": 0.025, "heart": -0.012, "runes": -0.01},
    face=dict(nostril=(0.03, -0.765, 0.915), nostril_r=(0.02, 0.014, 0.008), mouth_r=0.009,
              mouth=lambda side, a: (side * 0.075 * a ** 0.8, -0.765 + 0.11 * a ** 1.6, 0.85 + 0.02 * a * a)),
    jaw_hinge=(0, -0.62, 0.85),
    mouth_detail=dict(depth=0.12, fade=0.09, width=0.17, tooth=(0.005, 0.009), fang=(0.008, 0.016),
                      tongue=(0.03, 0.045, 0.008)),
    skin=dict(stripe=0.2, spot_cell=0.13, ao=0.2),
    sculpt=_even,
)

FORMS = {"grown": GROWN, "hatchling": HATCH}


# ------------------------------------------------------------------------------ hooks
def _loft(kit, name, rings, cap_base=True):
    """A closed mesh through rings of points (same count each); a ring of one point is a tip."""
    import bmesh
    bm = bmesh.new()
    vs = [[bm.verts.new(p) for p in ring] for ring in rings]
    for a, b in zip(vs, vs[1:]):
        n = len(a)
        if len(b) == 1:
            for k in range(n):
                bm.faces.new((a[k], a[(k + 1) % n], b[0]))
        else:
            for k in range(n):
                bm.faces.new((a[k], a[(k + 1) % n], b[(k + 1) % n], b[k]))
    if cap_base and len(vs[0]) > 2:
        bm.faces.new(list(reversed(vs[0])))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return kit.mesh_object(name, bm)


def _ear(kit, name, base, up, front, height, width, depth, tip_bend=0.0, tip_from=None, seg=8, steps=4):
    """A tall fox ear: an almond cross-section tapering to a soft point, flattened front to
    back; origin at its base. tip_from: faces above this fraction of its height take the
    second material (the fox's coloured ear tips)."""
    V = kit.V
    b, u, f = V(base), V(up).normalized(), V(front).normalized()
    s = u.cross(f).normalized()
    f = s.cross(u).normalized()
    rings = []
    seg = kit.lod(seg, 5)
    for t in kit.lod([k / steps for k in range(steps)], [0.0, 0.45]):
        w = width * (0.5 + 0.5 * math.cos(math.pi * t * 0.5)) * (1 - t) ** 0.55
        dp = depth * (1 - t) ** 0.8 + depth * 0.15
        c = f * (-tip_bend * height * t * t) + u * height * t
        rings.append([c + s * math.cos(2 * math.pi * k / seg) * w + f * math.sin(2 * math.pi * k / seg) * dp
                      for k in range(seg)])
    rings.append([u * height + f * (-tip_bend * height)])
    obj = _loft(kit, name, rings)
    obj.location = b
    kit.smooth(obj)
    if tip_from is not None:
        for p in obj.data.polygons:
            if V(p.center).dot(u) > tip_from * height:
                p.material_index = 1
    return obj


def _crystal(kit, bm, pts, radii, material, ring=4):
    """A faceted crystal beam along pts (a square section turned to face out, a sharp tip)
    added into bm, faces on the given material slot."""
    V = kit.V
    pts = [V(p) for p in pts]
    rings = []
    for i, (p, r) in enumerate(zip(pts, radii)):
        d = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        a = V((0, 0, 1)) if abs(d.z) < 0.9 else V((1, 0, 0))
        s = d.cross(a).normalized()
        u = s.cross(d).normalized()
        if i == len(pts) - 1:
            rings.append([bm.verts.new(p)])
        else:
            rings.append([bm.verts.new(p + (s * math.cos(t) + u * math.sin(t)) * r)
                          for t in (2 * math.pi * (k + 0.5) / ring for k in range(ring))])
    faces = [bm.faces.new(list(reversed(rings[0])))]
    for a, b in zip(rings, rings[1:]):
        for k in range(ring):
            faces.append(bm.faces.new((a[k], a[(k + 1) % ring], b[0])) if len(b) == 1 else
                         bm.faces.new((a[k], a[(k + 1) % ring], b[(k + 1) % ring], b[k])))
    for f in faces:
        f.material_index = material


# Antlers in the head frame (right side; offsets from the base, x out, y back, z up): a beam
# rising up and back, curving forward at the top, and its tines: (from beam point, offset, radii).
ANTLER = dict(base=(0.075, 0.12, 0.25),
              beam=[(0, 0, 0), (0.05, 0.08, 0.20), (0.12, 0.13, 0.40), (0.19, 0.10, 0.60), (0.23, 0.02, 0.76)],
              radii=[0.055, 0.048, 0.04, 0.03, 0.01],
              tines=[(1, (0.10, -0.16, 0.13), (0.034, 0.007)), (2, (0.03, 0.17, 0.19), (0.03, 0.007)),
                     (3, (0.14, -0.05, 0.15), (0.026, 0.006))])
ANTLER_RARE = dict(base=(0.075, 0.12, 0.25),
                   beam=[(0, 0, 0), (0.06, 0.09, 0.24), (0.15, 0.15, 0.50), (0.25, 0.12, 0.76), (0.30, 0.02, 0.98)],
                   radii=[0.062, 0.054, 0.045, 0.034, 0.012],
                   tines=[(1, (0.12, -0.20, 0.15), (0.038, 0.008)), (2, (0.04, 0.22, 0.22), (0.034, 0.008)),
                          (2, (0.16, -0.10, 0.18), (0.03, 0.007)), (3, (0.17, -0.06, 0.20), (0.03, 0.007)),
                          (3, (-0.04, 0.16, 0.16), (0.026, 0.006))])
BUD = dict(base=(0.08, 0.12, 0.26), beam=[(0, 0, 0), (0.03, 0.04, 0.12), (0.045, 0.05, 0.22)],
           radii=[0.05, 0.036, 0.008], tines=[])
BUD_RARE = dict(base=(0.08, 0.12, 0.26), beam=[(0, 0, 0), (0.03, 0.04, 0.14), (0.05, 0.05, 0.26)],
                radii=[0.056, 0.04, 0.009], tines=[(1, (0.06, -0.08, 0.08), (0.026, 0.006))])


def _antlers(kit, spec, mats, tine_material):
    """Both antlers, one object per side (seated by its base, so the tines stay up on the
    beam), the tines on their own material."""
    import bmesh
    k = kit.F["head"]["k"]
    objs = []
    for s in (-1, 1):
        bm = bmesh.new()
        m = lambda p: kit.V((s * p[0] * k, p[1] * k, p[2] * k))  # noqa: E731
        beam = [m(p) for p in spec["beam"]]
        _crystal(kit, bm, beam, [r * k for r in spec["radii"]], 0, kit.lod(4, 3))
        for i, off, (r0, r1) in spec["tines"]:
            a = beam[i]
            b = a + m(off)
            _crystal(kit, bm, [a, a.lerp(b, 0.5), b], [r0 * k, (r0 + r1) * 0.5 * k, r1 * k], 1, kit.lod(4, 3))
        obj = kit.mesh_object(f"antler_{s}", bm, kit.head_point(spec["base"], s))
        obj.data.materials.append(mats["horn"])
        obj.data.materials.append(mats[tine_material])
        objs.append((obj, "head"))
    return objs


SNOWFLAKE = []
for _a in (90, 30, 150):
    _c, _s = math.cos(math.radians(_a)) * 0.5, math.sin(math.radians(_a)) * 0.5
    SNOWFLAKE.append(((0.5 - _c, 0.5 - _s), (0.5 + _c, 0.5 + _s)))
for _a in range(6):  # each arm forks into a little V toward its tip
    _t = math.radians(90 + 60 * _a)
    _cx, _cy = 0.5 + 0.28 * math.cos(_t), 0.5 + 0.28 * math.sin(_t)
    for _f in (-1, 1):
        _u = _t + _f * math.radians(45)
        SNOWFLAKE.append(((_cx, _cy), (_cx + 0.17 * math.cos(_u), _cy + 0.17 * math.sin(_u))))


def parts(kit, d):
    F, mats, V = kit.F, d["mats"], kit.V
    baby = d["form"] == "hatchling"
    k = F["head"]["k"]
    out = []
    # Tall fox ears, the inner ear in the accent colour (one object each, seated by the base).
    ears = []
    for s in (-1, 1):
        base = kit.head_point((0.17, 0.05, 0.19) if not baby else (0.2, 0.06, 0.2), s)
        up = (s * 0.34, 0.22, 0.91)
        front = (s * 0.42, -0.9, 0.08)
        h = (0.56 if not baby else 0.42) * k
        e = _ear(kit, f"ear_{s}", base, up, front, h, 0.16 * k, 0.05 * k, tip_bend=0.06, tip_from=0.58)
        e.data.materials.append(mats["body_plain"])
        e.data.materials.append(mats["pattern_flat"])
        inner = _ear(kit, f"earin_{s}", base + V(front).normalized() * 0.03 * k + V((0, 0, 0.02 * k)),
                     up, front, h * 0.78, 0.1 * k, 0.03 * k, tip_bend=0.06, seg=6, steps=3)
        inner.data.materials.append(mats["accent_flat"])
        ears.append((kit.join([e, inner], f"ear_{s}"), "head"))
    out.append(("frill", 0, ears))
    # Branching ice-crystal antlers (buds on a hatchling); the rare Aurora's are bigger, with
    # glowing tines.
    out.append(("horns", 0, _antlers(kit, BUD if baby else ANTLER, mats, "horn")))
    out.append(("horns", 1, _antlers(kit, BUD_RARE if baby else ANTLER_RARE, mats, "glow_flat")))
    # Snowflakes on the thighs, below the folded wings (glowing on the rare one). glow_marks
    # faces them out from the hips' joint; each then rides its own thigh.
    if not baby:
        flakes = [("hipj_R", (0.41, 0.62, 1.04), 0, 0.26)]  # faced out from the thigh joint
        for v, material in ((0, "pattern_flat"), (1, "glow_flat")):
            marks = kit.glow_marks(mats, flakes, material=material, glyphs=[SNOWFLAKE])
            out.append(("runes", v, [(o, "leg_up_L" if o.location.x < 0 else "leg_up_R") for o, _ in marks]))
    return out


def _wing_edge(kit, w):
    """The trailing edge from the flank to the leading finger: scalloped between the finger
    tips. Returns the points and, per scallop, its deepest point's index."""
    edge, lows = [w["body"]], []
    for a, b, depth in (("body", "f4", 0.16), ("f4", "f3", 0.22), ("f3", "f2", 0.22), ("f2", "f1", 0.18)):
        n = kit.lod(3, 1)
        for j in range(1, n + 1):
            f = j / (n + 1)
            edge.append(w[a].lerp(w[b], f).lerp(w["wrist"], depth * math.sin(math.pi * f)))
            if j == (n + 1) // 2:
                lows.append(len(edge) - 1)
        edge.append(w[b].copy())
    return edge, lows


def wings(kit, d, rare):
    """Medium bat wings, frosted: a scalloped membrane, and along its trailing edge a band of
    frost with an icicle hanging from every scallop (glowing on the rare Aurora)."""
    F, mats, V = kit.F, d["mats"], kit.V
    wr = F["wing"]["radii"]
    objs = []
    for side in ("L", "R"):
        w = kit.wing_points(side)
        arm = kit.wing_arm(side, ["root", "elbow", "wrist", "thumb", "f1", "f2", "f3", "f4"],
                           [wr["root"], wr["elbow"], wr["wrist"], wr["tip"] * 2.5] + [wr["finger"]] * 4,
                           kit.lod(F["wing"]["arm_tris"], 56), mats)
        objs.append(arm)
        edge, lows = _wing_edge(kit, w)
        mem = kit.flat_fan(f"membrane_{side}", [w["wrist"], w["elbow"], w["root"]] + edge, F["wing"]["thickness"])
        mem.data.materials.append(mats["membrane"])
        objs.append(mem)
        # The frost: a band just inside the trailing edge, and icicles off each scallop.
        import bmesh
        bm = bmesh.new()
        band = F["wing"].get("frost", 0.1) * F["wing"]["scale"]
        start = 2 if not kit.LOD else 1
        inner = [p.lerp(w["wrist"], min(0.9, band / max(1e-4, (p - w["wrist"]).length))) for p in edge]
        for i in range(start, len(edge) - 1):
            if kit.LOD and i % 2:
                continue
            j = min(i + (2 if kit.LOD else 1), len(edge) - 1)
            bm.faces.new([bm.verts.new(p) for p in (edge[i], edge[j], inner[j], inner[i])])
        for i in lows[1:] if kit.LOD else lows:
            p = edge[i]
            along = (edge[i + 1] - edge[i - 1]).normalized()
            out = (p - w["wrist"]).normalized()
            L = F["wing"].get("icicle", 0.2) * F["wing"]["scale"]
            wd = L * 0.28
            bm.faces.new([bm.verts.new(q) for q in (p - along * wd, p + along * wd, p + out * L)])
        frost = kit.mesh_object(f"frost_{side}", bm)
        m = frost.modifiers.new("thick", "SOLIDIFY")
        m.thickness, m.offset, m.use_rim = F["wing"]["thickness"] * 1.4, 0, False
        kit.apply_modifiers(frost)
        frost.data.materials.append(mats["glow_flat" if rare else "accent_flat"])
        objs.append(frost)
    return objs


def accent(kit, body):
    """The accent (G of the mask): a fox's white front: the belly, the ruff and bib, the chin,
    the underside of the muzzle and the cheeks."""
    mask = kit.mask_attr(body)
    baby = kit.F["name"] == "hatchling"

    def ramp(x, lo, hi):
        return min(1.0, max(0.0, (x - lo) / (hi - lo)))

    for v in body.data.vertices:
        x, y, z = v.co
        n = v.normal
        if baby:
            belly = ramp(-n.z, 0.25, 0.6) * ramp(0.95 - z, 0.0, 0.2) * ramp(0.28 - abs(x), 0.0, 0.08)
            bib = ramp(-y, 0.18, 0.3) * ramp(0.86 - z, 0.0, 0.08) * ramp(z, 0.16, 0.26) * ramp(-n.y, -0.1, 0.35)
            face = ramp(-y, 0.52, 0.62) * ramp(0.95 - z, 0.0, 0.06) * ramp(-n.z + 0.35, 0.0, 0.4)
            cheek = ramp(-y, 0.4, 0.5) * ramp(abs(x), 0.14, 0.22) * ramp(0.98 - z, 0.0, 0.08)
            a = max(belly, bib, face, cheek) * ramp(0.34 - y, 0.0, 0.1)
        else:
            belly = ramp(-n.z, 0.25, 0.6) * ramp(1.3 - z, 0.0, 0.2) * ramp(0.34 - abs(x), 0.0, 0.1) * ramp(0.7 - y, 0, 0.2)
            bib = ramp(-y, 0.62, 0.8) * ramp(2.12 - z, 0.0, 0.14) * ramp(z, 0.95, 1.1) * ramp(-n.y, -0.2, 0.3)
            face = ramp(-y, 1.4, 1.5) * ramp(2.34 - z, 0.0, 0.05) * ramp(-n.z + 0.3, 0.0, 0.4)
            cheek = ramp(-y, 1.0, 1.1) * ramp(abs(x), 0.16, 0.26) * ramp(2.36 - z, 0.0, 0.1) * ramp(z, 1.9, 2.0)
            a = max(belly, bib, face, cheek)
        mask.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


def texture(tx, nt, p, form):
    """R: points (socks and the plume's tip); G: snow-leopard rosettes on the back, flanks and
    tail; B: aurora ribbons across the tail (the rare one's glow)."""
    baby = form == "hatchling"

    def dist(point):
        v = tx.node(nt, "ShaderNodeVectorMath", operation="DISTANCE")
        nt.links.new(tx.coords(nt), v.inputs[0])
        v.inputs[1].default_value = point
        return v.outputs["Value"]

    y, z = tx.axis(nt, 1), tx.axis(nt, 2)
    if baby:
        socks = tx.smoothstep(nt, z, 0.16, 0.10)
        base, reach, period = (0, 0.42, 0.56), (0.36, 0.6), 0.18
        tailness = tx.maxi(nt, tx.smoothstep(nt, y, 0.3, 0.4),
                           tx.mul(nt, tx.smoothstep(nt, z, 0.8, 0.9), tx.smoothstep(nt, y, 0.1, 0.2)))
        body = tx.smoothstep(nt, y, -0.2, -0.1)  # no rosettes on the face
    else:
        socks = tx.smoothstep(nt, z, 0.50, 0.36)
        base, reach, period = (0, 0.98, 1.52), (1.05, 1.55), 0.5
        tailness = tx.maxi(nt, tx.smoothstep(nt, y, 0.74, 0.92),
                           tx.mul(nt, tx.smoothstep(nt, z, 1.9, 2.15), tx.smoothstep(nt, y, 0.2, 0.4)))
        body = tx.smoothstep(nt, y, -0.9, -0.7)
    along = dist(base)
    r = tx.maxi(nt, socks, tx.mul(nt, tailness, tx.smoothstep(nt, along, *reach)))
    # rosettes: broken rings round a paler middle, on one Voronoi grid
    cell = p["spot_cell"]
    ring = tx.math_op(nt, "SUBTRACT", tx.spots(nt, cell, keep=0.3, size=(0.4, 0.3), where=tx.upper(nt)),
                      tx.spots(nt, cell, keep=0.3, size=(0.22, 0.14), where=tx.upper(nt)))
    g = tx.mul(nt, tx.maxi(nt, ring, tx.spots(nt, cell * 0.6, keep=0.55, size=(0.24, 0.14), where=tx.top(nt),
                                               seed_offset=3.7)), body)
    # aurora: wavering ribbons across the plume, from a little way up it to the tip
    wob = tx.node(nt, "ShaderNodeTexNoise")
    wob.inputs["Scale"].default_value = 2.4 if baby else 0.9
    wob.inputs["Detail"].default_value = 1.5
    nt.links.new(tx.coords(nt), wob.inputs["Vector"])
    # the ribbons slant round the plume (x twists them) and waver (noise), thick and thin
    slant = tx.madd(nt, tx.axis(nt, 0), period * 1.6, 0.0)
    warped = tx.add(nt, tx.add(nt, along, slant), tx.madd(nt, wob.outputs["Fac"], period * 1.6, -period * 0.8))
    wave = tx.math_op(nt, "SINE", tx.madd(nt, warped, 2 * math.pi / period, 0.0))
    thick = tx.madd(nt, wob.outputs["Fac"], -0.5, 0.75)
    ribbons = tx.smoothstep(nt, tx.math_op(nt, "SUBTRACT", wave, thick), -0.05, 0.2)
    b = tx.mul(nt, ribbons, tx.mul(nt, tailness, tx.smoothstep(nt, along, reach[0] * 0.25, reach[0] * 0.55)))
    return {"r": r, "g": g, "b": b}
