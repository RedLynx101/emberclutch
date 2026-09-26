"""The Frostcurl (Stone and Frost, uncommon; Dragondex 14): the crossbreed of the Curlstone and
the Flurrytail. A glacier fox that curls up into a snowball. Concept:
docs/art/concept/dragons/dragon_frostcurl.jpg (reference only, D74).

Design notes
  * Parents. The Flurrytail gives it the fox: a short pointed fox face, tall fox ears, fluffy
    snow-white fur on the face, chest and belly (a full ruff, cheek fluff, fluffy trousers)
    and its great plume of a tail. The Curlstone gives it its body plan (plans/curlstone.py),
    its back of plates and its way of curling up into a ball. What is new is ice: the plates
    are smooth domes of ice-blue, each free edge a frosted pale crescent like thin ice, set in
    a band down the middle of a white fur back, and the crest, the ear tips, the wings and the
    points between the plates are faceted crystal; curled up (the plates outside, the plume
    wrapped round) it is a snowball studded with ice.
  * The adult. A proud, calm glacier fox, lower and sturdier than the Flurrytail, higher on
    its legs than the Curlstone: a slim fox's body under a crystal-plated back, a deep ruff on
    its chest framing the heartglow, a fox's short snout and kind, big eyes, tall ears with
    faceted ice tips, a dark button nose, a little crest of ice crystals on the brow, crystal
    points peeking out between the plates and a huge white plume sweeping out behind it and
    curling up at its tip. Small crystal wings: a slim ice arm fanning five faceted shards from
    the wrist, which fold flat along its flanks under the plates. Curled up, the plates are
    outside and the plume wraps round the ball.
  * The hatchling. A round snowball: a big round head on a round, fluffy body, huge eyes,
    fluffy cheeks, a tiny fox muzzle with a button nose, small fox ears with ice tips, a few
    tiny ice-plate nubs on its back, stubby legs, tiny crystal wing buds turned out off its
    shoulders (base_pose, so they don't sink into its round sides) and a fluffy tail curled
    round beside it.
  * Pattern: R frost points (socks fading up the legs), G aurora ribbons wavering across the
    back and the plume, B glints (the rare one's glowing sparkles).
  * Variants: Glacier (white fur, pale ice-blue plates and socks, a warm golden heart: the
    natural one), Aurora (white fur, teal plates frosted lilac, lilac ribbons on the plume:
    the surprise), Slate (grey fur, a white front, deep blue plates, amber eyes: the subtle
    one) and the rare Diamond Dust: lilac-silver with glowing prismatic plates and wing
    shards, glowing glints in its fur, a taller glowing crest and clusters of glowing
    crystals between its plates.
  * Build: both forms are metaballs, so the fluff (ruff, cheeks, trousers, the plume) is body
    geometry that bends with the bones; the sculpt hook re-meshes them into even quads
    (QuadriFlow, mirrored) and then WELDS THE MIDLINE (_weld_midline): QuadriFlow's mirrored
    result leaves the two halves unjoined along x = 0 (a hairline crack of open edges, split
    normals: the seam run 18 saw down the Flurrytail hatchling), so the halves are joined
    again before decimation.
  * Egg: a round snow-white egg set with ice-blue crystal facets.
"""
import math

# ------------------------------------------------------------------------------ who it is
META = dict(
    name="frostcurl", title="Frostcurl", dex=14, element=("Stone", "Frost"), parents=("curlstone", "flurrytail"),
    rarity="uncommon", plan="curlstone", size=1.05,
    stats=dict(wing=5, wit=8, might=8, breath=6, stamina=6),
    manners=("Proud", "Gentle", "Curious", "Brave"),
    traits=("Cool-Headed", "Sure-Footed", "Ironhide", "Loyal", "Starborn", "Moonlit"),
    rare_variant=3, rare_replaces=True,
    blurb="A proud, loyal glacier fox; when the wind bites, it wraps its great plume round its nose "
          "and curls up into a snowball.",
)

# Palette use: base = the fur, accent = its white front (bib, belly, muzzle, cheeks), pattern =
# the frost points and ribbons, horn = the ice plates, crest and ear tips, membrane = the ice's
# frosted pale facets (plate bevels, shard facets), glow = the heart (and the rare one's ice).
VARIANTS = [
    dict(name="Glacier", base=(0.86, 0.90, 0.97), accent=(1.0, 1.0, 1.0), pattern=(0.44, 0.66, 0.96),
         horn=(0.40, 0.70, 0.98), membrane=(0.78, 0.90, 1.0), iris=(0.10, 0.24, 0.58), glow=(1.0, 0.74, 0.30),
         pattern_channel="r", glow_channel=None),
    dict(name="Aurora", base=(0.92, 0.92, 0.97), accent=(1.0, 1.0, 1.0), pattern=(0.66, 0.50, 0.94),
         horn=(0.24, 0.78, 0.76), membrane=(0.80, 0.70, 1.0), iris=(0.14, 0.44, 0.50), glow=(0.40, 1.0, 0.80),
         pattern_channel="g", glow_channel=None),
    dict(name="Slate", base=(0.40, 0.43, 0.50), accent=(0.88, 0.90, 0.94), pattern=(0.17, 0.20, 0.30),
         horn=(0.12, 0.24, 0.62), membrane=(0.44, 0.62, 0.92), iris=(0.92, 0.60, 0.16), glow=(1.0, 0.70, 0.28),
         pattern_channel="r", glow_channel=None),
    dict(name="Diamond Dust", base=(0.78, 0.78, 0.92), accent=(0.97, 0.96, 1.0), pattern=(0.62, 0.56, 0.92),
         horn=(0.70, 0.92, 1.0), membrane=(1.0, 0.70, 0.90), iris=(0.36, 0.26, 0.78), glow=(0.50, 0.92, 1.0),
         pattern_channel="r", glow_channel="b"),
]

EGG = dict(height=0.98, width=0.42, asym=0.06, point=1.0, speckle="spots",
           speckle_params=dict(count=22, count_lod1=7, size=(0.035, 0.08)),
           colors=[((0.97, 0.98, 1.0), (0.52, 0.76, 1.0)), ((0.98, 0.97, 1.0), (0.40, 0.82, 0.80)),
                   ((0.70, 0.72, 0.78), (0.25, 0.40, 0.78)), ((0.98, 0.97, 1.0), (0.84, 0.74, 1.0))])


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


def _unit(v):
    ln = math.sqrt(sum(x * x for x in v)) or 1.0
    return tuple(x / ln for x in v)


def _add(a, b, k=1.0):
    return tuple(x + y * k for x, y in zip(a, b))


def tuft(root, tip, r0, r1=None):
    """A lock of fur: a tapered chain from inside the body out to a soft, rounded point."""
    return ("chain", [root, tip], [r0, r1 if r1 is not None else r0 * 0.45])


def _plume(points, radii, rows, start=0.1, stop=0.96, reach=1.0, sweep=0.8, root_k=0.5, tip_k=0.2):
    """A fluffy brush: a chain of balls along the tail, dressed in rows of pointed locks lying
    back toward the tip. rows: [(angle round the tail from its top (0) toward +x (90), count,
    stagger)]; mirrored rows keep it symmetric."""
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
            up = _unit((0.0, -d[2], d[1]))
            if up[2] < 0:
                up = tuple(-x for x in up)
            th = math.radians(ang)
            o = _unit(_add(tuple(x * math.cos(th) for x in up), (1.0, 0.0, 0.0), math.sin(th)))
            root = _add(c, o, r * 0.35)
            tip = _add(_add(c, o, r * reach), d, r * sweep)
            els.append(tuft(root, tip, r * root_k, r * tip_k))
    return els


def _weld_midline(kit, obj, tol=0.006):
    """Join the two mirrored halves along x = 0: every vertex within tol of the midline on an
    open (one-faced) edge moves onto it, and pairs that then coincide are merged. QuadriFlow's
    mirrored remesh leaves the halves apart by up to a millimetre or two (open edges down the
    middle, split normals: a visible seam); after this the body is closed."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    open_verts = {v for e in bm.edges if len(e.link_faces) == 1 for v in e.verts if abs(v.co.x) < tol}
    for v in open_verts:
        v.co.x = 0.0
    bmesh.ops.remove_doubles(bm, verts=list(open_verts), dist=tol * 0.5)
    # Tidy what the merge can leave on a coarse mesh: faces that collapsed, a face doubled on
    # the midline (an edge with three faces), and any pinhole left open there.
    bmesh.ops.dissolve_degenerate(bm, dist=1e-6, edges=list(bm.edges))
    seen, doubled = {}, []
    for f in bm.faces:
        key = frozenset(v.index for v in f.verts)
        if key in seen:
            doubled.append(f)
        else:
            seen[key] = f
    if doubled:
        bmesh.ops.delete(bm, geom=doubled, context="FACES_ONLY")
    for e in [e for e in bm.edges if len(e.link_faces) > 2]:
        extra = sorted(e.link_faces, key=lambda f: f.calc_area())[:len(e.link_faces) - 2]
        bmesh.ops.delete(bm, geom=extra, context="FACES_ONLY")
    loose = [v for v in bm.verts if not v.link_faces]
    if loose:
        bmesh.ops.delete(bm, geom=loose, context="VERTS")
    holes = [e for e in bm.edges if len(e.link_faces) == 1]
    if holes:
        bmesh.ops.holes_fill(bm, edges=holes, sides=8)
        bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    bm.normal_update()
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


def _even(kit, obj):
    """The forms' sculpt hook (the Flurrytail's): retopologise the metaball body into even,
    flowing quads (QuadriFlow, mirrored) at about twice the triangle target, weld the midline
    shut, then decimate it (mirrored) to a little under the target so the kit doesn't decimate
    again. Toon bands run straight across each triangle, so a collapse-decimated blob of fur
    shades like crumpled paper; a mesh that follows the forms shades in clean, soft bands."""
    import bpy
    target = int(kit.lod(kit.F["body_tris"], kit.F["body_tris_lod1"]) * 0.97)
    sm = obj.modifiers.new("soften", "SMOOTH")
    sm.factor, sm.iterations = 0.5, 3
    kit.apply_modifiers(obj)
    obj.data.use_mirror_x = True
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.quadriflow_remesh(mode="FACES", target_faces=int(target * 1.1), use_mesh_symmetry=True,
                                     use_preserve_sharp=False, use_preserve_boundary=False, smooth_normals=False,
                                     seed=3)
    obj.select_set(False)
    _weld_midline(kit, obj)
    cur = kit.tri_count(obj)
    if cur > target:
        m = obj.modifiers.new("even", "DECIMATE")
        m.ratio = target / cur
        m.use_collapse_triangulate = True
        m.use_symmetry = True
        m.symmetry_axis = "X"
        kit.apply_modifiers(obj)
    _weld_midline(kit, obj)
    sm = obj.modifiers.new("settle", "SMOOTH")  # soften the decimation's creases a little
    sm.factor, sm.iterations = 0.25, 2
    kit.apply_modifiers(obj)


# ------------------------------------------------------------------------------ grown
# Faces -Y, Z up; about 6 units long (the Curlstone's frame: the spine from the hips to the
# neck is the Curlstone's, lifted 0.15 so the legs are longer, so its ball and its roll fit).
# A slimmer, fox-like body; the plume sweeping out behind and curling up.
GROWN_TAIL = [(0, 1.22, 1.31), (0, 1.84, 1.20), (0, 2.36, 1.22), (0, 2.76, 1.46), (0, 2.96, 1.86)]
GROWN_NODES = _mirrored({
    "tail_tip": (GROWN_TAIL[4], None),
    "tail4": (GROWN_TAIL[3], None),
    "tail3": (GROWN_TAIL[2], None),
    "tail2": (GROWN_TAIL[1], None),
    "hips": (GROWN_TAIL[0], None),
    "loin": ((0, 0.58, 1.51), None),
    "belly": ((0, -0.08, 1.43), None),
    "chest": ((0, -0.74, 1.19), None),
    "neck2": ((0, -1.30, 1.23), None),
    "neck3": ((0, -1.66, 1.43), None),
    "head": ((0, -2.00, 1.63), None),
    "muzzle": ((0, -2.42, 1.49), None),
    "snout": ((0, -2.70, 1.41), None),
}, {
    "shoulder": ((0.40, -0.68, 1.02), None),
    "elbow": ((0.44, -0.78, 0.58), None),
    "wrist": ((0.44, -0.84, 0.20), None),
    "toe_f": ((0.45, -1.08, 0.07), None),
    "hipj": ((0.40, 1.12, 1.10), None),
    "knee": ((0.46, 0.84, 0.62), None),
    "ankle": ((0.46, 1.18, 0.22), None),
    "toe_b": ((0.47, 0.96, 0.07), None),
})

GROWN_META = [
    # the torso: a deep chest, a slim fox's waist, round haunches (the plates cover the top)
    ("ell", (0, -0.72, 1.20), (0.52, 0.56, 0.56)),
    ("ell", (0, -0.06, 1.40), (0.48, 0.60, 0.49)),
    ("ell", (0, 0.56, 1.48), (0.50, 0.54, 0.51)),
    ("ell", (0, 1.08, 1.33), (0.48, 0.44, 0.49)),
    # the neck and head: a round skull, a short pointed fox muzzle
    ("chain", [(0, -0.92, 1.28), (0, -1.30, 1.32), (0, -1.64, 1.49)], [0.42, 0.37, 0.35]),
    ("ell", (0, -2.02, 1.66), (0.46, 0.43, 0.42)),
    ("ell", (0, -2.42, 1.50), (0.19, 0.27, 0.16)),
    ("ell", (0, -2.66, 1.43), (0.095, 0.13, 0.085)),
    ("ell", (0, -2.40, 1.38), (0.14, 0.20, 0.085)),
    # the ruff: a full, round bib under the throat (the heartglow sits on its front)
    ("ball", (0, -1.32, 1.12), 0.42),
    tuft((0, -1.36, 1.04), (0, -1.48, 0.66), 0.3, 0.13),
]
GROWN_META += _both([
    tuft((0.24, -2.12, 1.56), (0.58, -1.86, 1.38), 0.2, 0.085),     # cheek fluff flaring back
    tuft((0.22, -1.98, 1.46), (0.50, -1.70, 1.24), 0.18, 0.075),
    tuft((0.24, -1.24, 1.10), (0.40, -1.34, 0.70), 0.27, 0.11),     # the ruff's locks
    tuft((0.36, -1.08, 1.26), (0.58, -1.16, 0.92), 0.23, 0.095),
    tuft((0.20, -0.30, 1.02), (0.26, -0.20, 0.74), 0.2, 0.09),      # a fluffy belly
    # legs: slim, with neat paws; a tuft at each elbow, fluffy trousers behind the thighs
    ("ell", (0.34, -0.68, 1.06), (0.20, 0.26, 0.32)),
    ("chain", [(0.41, -0.74, 0.88), (0.44, -0.78, 0.58), (0.44, -0.84, 0.20)], [0.2, 0.15, 0.115]),
    ("ell", (0.45, -0.94, 0.08), (0.135, 0.19, 0.09)),
    tuft((0.46, -0.72, 0.66), (0.53, -0.58, 0.44), 0.13, 0.055),
    ("ell", (0.34, 1.06, 1.12), (0.24, 0.36, 0.42)),
    ("chain", [(0.41, 1.00, 0.96), (0.46, 0.84, 0.62)], [0.21, 0.16]),
    ("chain", [(0.46, 0.84, 0.62), (0.46, 1.18, 0.22), (0.46, 1.06, 0.10)], [0.145, 0.115, 0.105]),
    ("ell", (0.47, 0.98, 0.08), (0.135, 0.19, 0.09)),
    tuft((0.40, 1.26, 1.02), (0.47, 1.48, 0.66), 0.21, 0.1),
])
# the plume: a great bushy brush sweeping back and curling up behind, its locks lying back
# toward the tip
GROWN_PLUME = [(0, 1.42, 1.28), (0, 1.84, 1.20), (0, 2.36, 1.22), (0, 2.76, 1.46), (0, 2.96, 1.86)]
GROWN_META += _plume(GROWN_PLUME, [0.20, 0.34, 0.50, 0.55, 0.42],
                     [(0, 4, 0.3), (70, 4, 0.0), (-70, 4, 0.0), (140, 3, 0.5), (-140, 3, 0.5)],
                     reach=1.25, sweep=1.0, root_k=0.5, tip_k=0.24)
GROWN_META += [tuft((0, 2.94, 1.84), (0, 2.96, 2.40), 0.34, 0.09)]   # the brush's soft point

BUILDS = {
    "neutral": {},
    "sturdy": {"chest": (1.07, 0.98), "belly": (1.07, 0.98), "loin": (1.06, 0.98), "hips": (1.05, 1.0),
               "arm_up": (1.08, 0.97), "leg_up": (1.07, 0.97), "tail2": (1.05, 0.98)},
    "sleek": {"chest": (0.95, 1.02), "belly": (0.93, 1.03), "loin": (0.93, 1.03), "leg_lo": (0.95, 1.04),
              "arm_lo": (0.95, 1.04)},
    "long": {"belly": (0.97, 1.07), "loin": (0.97, 1.07), "tail2": (0.97, 1.06), "tail3": (0.96, 1.08),
             "tail4": (0.96, 1.08), "snout": (0.97, 1.06)},
}

# Ice plates: (bone, t along it, angles round the spine (0 on top, + to its right), (width,
# length, height), lift of the rear edge): a band down the middle of the back, staggered like
# shingles; the flanks stay fur.
GROWN_PLATES = [
    ("neck3", 0.3, (0,), (0.38, 0.42, 0.10), 0.03),
    ("neck2", 0.45, (-24, 24), (0.44, 0.50, 0.11), 0.035),
    ("chest", 0.45, (0, 46, -46), (0.58, 0.66, 0.13), 0.04),
    ("belly", 0.5, (-23, 23, 66, -66), (0.60, 0.70, 0.14), 0.045),
    ("loin", 0.5, (0, 46, -46), (0.62, 0.72, 0.14), 0.045),
    ("hips", 0.45, (-24, 24, 64, -64), (0.56, 0.64, 0.13), 0.04),
]
GROWN_PLATES_LOD1 = [GROWN_PLATES[i] for i in (1, 2, 3, 4, 5)]

GROWN = dict(
    name="grown", nodes=GROWN_NODES, meta=GROWN_META, body="meta", meta_resolution=0.04, voxel=0.045,
    # DR2 sizing: grown at the common scale, the Pouncer's bulk and length (measure.py); META size
    # then sizes it in the game.
    body_tris=1500, body_tris_lod1=540, export_scale=0.79,
    young={
        "bones": {
            "head": (1.08, 1.0, 1.08), "snout": (0.9, 0.74, 0.94),
            "neck2": (0.8, 0.62), "neck3": (0.82, 0.62),
            "chest": (0.72, 0.62), "belly": (0.70, 0.60), "loin": (0.70, 0.60), "hips": (0.72, 0.62),
            "tail1": (0.72, 0.6), "tail2": (0.72, 0.6), "tail3": (0.74, 0.6), "tail4": (0.8, 0.64),
            "arm_up": (0.76, 0.66), "arm_lo": (0.78, 0.66), "hand": (0.86, 0.8),
            "leg_up": (0.76, 0.66), "leg_lo": (0.78, 0.66), "foot": (0.86, 0.8),
        },
        "parts": {"eyes": 1.3, "horns": 0.55, "frill": 0.8, "wings": 0.6, "spikes": 0.9,
                  "tail_tip": 0.75, "heart": 0.85, "runes": 0.6},
    },
    young_pose={"neck2": -8, "head": -6},
    base_pose={"neck2": (-20, 0, 0), "neck3": (-10, 0, 0), "head": (14, 0, 0)},  # a proud head
    builds=BUILDS,
    eyes=dict(at=(0.22, -2.34, 1.72), out=(0.46, -0.86, 0.20), iris=(0.17, 0.19, 0.08),
              pupil=(0.105, 0.125, 0.03), slit=(0.3, 1.1),
              glints=((-0.026, 0.04, 0.019), (0.016, -0.034, 0.008)), seg=(12, 2, 8, 2)),
    head=dict(origin=(0, -2.02, 1.66), k=1.4, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=False),
    tail_k=1.0,
    heart=dict(at=(0.002, -1.72, 1.10), size=0.15, tilt=18),  # x: 2 mm off the midline (the kit's seam gotcha)
    wing=dict(root=(0.40, -0.55, 1.62), scale=0.9, dihedral=30, droop=8,
              layout={"root": (0.0, 0.0), "elbow": (0.62, 0.22), "wrist": (1.12, -0.02), "f1": (1.92, 0.30),
                      "f2": (1.74, 0.94), "f3": (1.22, 1.38), "body": (0.0, 1.15)},
              radii={"root": 0.09, "elbow": 0.07, "wrist": 0.058, "finger": 0.03, "tip": 0.012},
              arm_tris=80, thickness=0.014, style="sail"),
    mask=dict(max_x=0.45, max_z=1.5, min_z=-1.0, tail_cut=None),
    inset={"eyes": 0.02, "horns": 0.03, "spikes": 0.012, "frill": 0.03, "heart": -0.03, "runes": 0.03,
           "tail_tip": 0.05},
    face=dict(nostril=(0.04, -2.78, 1.47), nostril_r=(0.024, 0.016, 0.01), mouth_r=0.012,
              mouth=lambda side, a: (side * 0.15 * a ** 0.7, -2.76 + 0.34 * a ** 1.5, 1.35 + 0.05 * a * a)),
    jaw_hinge=(0, -2.32, 1.36),
    nose=dict(at=(0, -2.80, 1.47), size=(0.075, 0.05, 0.055)),
    mouth_detail=dict(depth=0.16, fade=0.14, width=0.24, tooth=(0.009, 0.015), fang=(0.012, 0.025),
                      tongue=(0.06, 0.12, 0.014), fangs=[]),
    skin=dict(sock=(0.66, 0.42), tail_y=1.45, ribbon=0.55, glint_cell=0.16, ao=0.5),
    sculpt=_even,
    plates=(GROWN_PLATES, GROWN_PLATES_LOD1),
    # (bone, t, angle, length, how many): crystal points peeking out between the plates (the
    # commons' few; the rare one's glowing clusters)
    crystals=([("chest", 0.95, 62, 0.30, 1), ("chest", 0.95, -62, 0.30, 1), ("loin", 0.95, 62, 0.34, 1),
               ("loin", 0.95, -62, 0.34, 1)],
              [("chest", 0.95, 62, 0.36, 2), ("chest", 0.95, -62, 0.36, 2), ("loin", 0.95, 62, 0.44, 2),
               ("loin", 0.95, -62, 0.44, 2), ("neck2", 0.95, 0, 0.26, 1)]),
)

# ------------------------------------------------------------------------------ hatchling
# A round snowball: the head a third of it, a round fluffy body, stubby legs; the fluffy tail is
# modelled straight and curled round beside it in the idle pose (base_pose).
HATCH_TAIL = [(0, 0.40, 0.46), (0, 0.60, 0.34), (0, 0.78, 0.28), (0, 0.94, 0.26), (0, 1.08, 0.28)]
HATCH_NODES = _mirrored({
    "tail_tip": (HATCH_TAIL[4], None),
    "tail4": (HATCH_TAIL[3], None),
    "tail3": (HATCH_TAIL[2], None),
    "tail2": (HATCH_TAIL[1], None),
    "hips": (HATCH_TAIL[0], None),
    "loin": ((0, 0.24, 0.58), None),
    "belly": ((0, 0.04, 0.60), None),
    "chest": ((0, -0.14, 0.56), None),
    "neck2": ((0, -0.26, 0.66), None),
    "neck3": ((0, -0.34, 0.80), None),
    "head": ((0, -0.44, 0.96), None),
    "muzzle": ((0, -0.66, 0.89), None),
    "snout": ((0, -0.80, 0.86), None),
}, {
    "shoulder": ((0.18, -0.14, 0.36), None),
    "elbow": ((0.20, -0.18, 0.22), None),
    "wrist": ((0.21, -0.21, 0.09), None),
    "toe_f": ((0.22, -0.33, 0.05), None),
    "hipj": ((0.18, 0.34, 0.36), None),
    "knee": ((0.21, 0.28, 0.22), None),
    "ankle": ((0.22, 0.38, 0.10), None),
    "toe_b": ((0.22, 0.24, 0.05), None),
})

HATCH_META = [
    ("ell", (0, -0.44, 0.98), (0.35, 0.32, 0.32)),     # big round head
    ("ell", (0, -0.68, 0.89), (0.12, 0.11, 0.09)),     # tiny fox muzzle
    ("ell", (0, -0.64, 0.83), (0.10, 0.09, 0.06)),     # chin
    ("ball", (0, 0.10, 0.50), 0.36),                   # a round snowball of a body
    ("ell", (0, -0.14, 0.54), (0.26, 0.22, 0.26)),     # chest
    ("ball", (0, -0.26, 0.60), 0.18),                  # the ruff
    tuft((0, -0.28, 0.56), (0, -0.38, 0.40), 0.14, 0.07),
]
HATCH_META += _both([
    ("ball", (0.20, -0.56, 0.88), 0.12),                          # fluffy cheeks
    tuft((0.22, -0.50, 0.90), (0.37, -0.40, 0.84), 0.12, 0.06),
    tuft((0.13, -0.22, 0.54), (0.20, -0.30, 0.38), 0.12, 0.06),   # ruff clumps
    ("chain", [(0.18, -0.14, 0.38), (0.20, -0.18, 0.23), (0.21, -0.21, 0.10)], [0.1, 0.085, 0.08]),
    ("ell", (0.21, -0.26, 0.06), (0.085, 0.11, 0.06)),  # front paws
    ("ell", (0.17, 0.32, 0.36), (0.13, 0.16, 0.17)),   # thighs
    ("chain", [(0.21, 0.30, 0.25), (0.22, 0.34, 0.11)], [0.09, 0.08]),
    ("ell", (0.22, 0.24, 0.06), (0.085, 0.11, 0.06)),   # hind paws
])
HATCH_PLUME = [(0, 0.40, 0.44), (0, 0.60, 0.36), (0, 0.78, 0.31), (0, 0.94, 0.29), (0, 1.06, 0.30)]
HATCH_META += _plume(HATCH_PLUME, [0.10, 0.18, 0.23, 0.22, 0.15],
                     [(0, 3, 0.4), (75, 3, 0.0), (-75, 3, 0.0), (150, 2, 0.5), (-150, 2, 0.5)], start=0.3,
                     reach=1.25, sweep=0.9, root_k=0.5, tip_k=0.26)
HATCH_META += [tuft((0, 1.02, 0.30), (0, 1.24, 0.36), 0.15, 0.06)]

HATCH_PLATES = [
    ("chest", 0.5, (0,), (0.17, 0.19, 0.07), 0.02),
    ("belly", 0.5, (-28, 28), (0.18, 0.21, 0.075), 0.024),
    ("loin", 0.5, (0,), (0.19, 0.22, 0.08), 0.024),
    ("hips", 0.4, (-28, 28), (0.17, 0.19, 0.07), 0.02),
]

HATCH = dict(
    name="hatchling", nodes=HATCH_NODES, meta=HATCH_META, body="meta",
    body_tris=1500, body_tris_lod1=540, export_scale=1.0,
    young={
        "bones": {name: ((0.76, 0.76) if name in ("head", "snout") else (0.64, 0.64))
                  for name in ("hips", "loin", "belly", "chest", "neck2", "neck3", "head", "snout", "tail1",
                               "tail2", "tail3", "tail4", "arm_up", "arm_lo", "hand", "leg_up", "leg_lo", "foot")},
        "parts": {"eyes": 1.12, "horns": 0.6, "frill": 0.85, "wings": 0.56, "spikes": 0.9,
                  "tail_tip": 0.9, "heart": 1.0, "runes": 0.9},
    },
    # The tail curled round beside it; the little crystal wings turned out off its round sides.
    base_pose={"neck2": (6, 0, 0), "head": (-8, 0, 0), "tail1": (6, 0, 14), "tail2": (0, 0, 26),
               "tail3": (0, 0, 30), "tail4": (0, 0, 30), "wing_arm_R": (0, 30, -60), "wing_arm_L": (0, -30, 60)},
    builds={"neutral": {}, "sturdy": {}, "sleek": {}, "long": {}},
    eyes=dict(at=(0.16, -0.70, 1.02), out=(0.45, -0.88, 0.10), iris=(0.114, 0.128, 0.064),
              pupil=(0.076, 0.09, 0.03), slit=(0.32, 1.1),
              glints=((-0.028, 0.044, 0.028), (0.024, -0.042, 0.013)), seg=(14, 3, 12, 2)),
    head=dict(origin=(0, -0.44, 0.98), k=0.85, horn_len=1.0, horn_r=1.0, horn_curve=1.0, buds=True),
    tail_k=0.36,
    heart=dict(at=(0.002, -0.40, 0.46), size=0.08, tilt=18),
    wing=dict(root=(0.15, -0.06, 0.70), scale=0.22, dihedral=25, droop=5,
              layout={"root": (0.0, 0.0), "elbow": (0.62, 0.22), "wrist": (1.12, -0.02), "f1": (1.92, 0.30),
                      "f2": (1.74, 0.94), "f3": (1.22, 1.38), "body": (0.0, 1.15)},
              radii={"root": 0.04, "elbow": 0.032, "wrist": 0.028, "finger": 0.012, "tip": 0.006},
              arm_tris=60, thickness=0.008, style="sail"),
    mask=dict(max_x=0.24, max_z=1.0, min_z=0.1, tail_cut=None),
    inset={"eyes": 0.035, "horns": 0.02, "spikes": 0.006, "frill": 0.02, "heart": -0.012, "runes": 0.015,
           "tail_tip": 0.03},
    face=dict(nostril=(0.03, -0.795, 0.915), nostril_r=(0.02, 0.014, 0.008), mouth_r=0.009,
              mouth=lambda side, a: (side * 0.08 * a ** 0.8, -0.795 + 0.12 * a ** 1.6, 0.85 + 0.02 * a * a)),
    jaw_hinge=(0, -0.64, 0.85),
    nose=dict(at=(0, -0.80, 0.92), size=(0.042, 0.03, 0.032)),
    mouth_detail=dict(depth=0.12, fade=0.09, width=0.17, tooth=(0.005, 0.009), fang=(0.008, 0.016),
                      tongue=(0.032, 0.05, 0.008), fangs=[]),
    skin=dict(sock=(0.17, 0.10), tail_y=0.42, ribbon=0.2, glint_cell=0.06, ao=0.2),
    sculpt=_even,
    plates=(HATCH_PLATES, HATCH_PLATES),
    crystals=([], [("loin", 0.9, 50, 0.1, 1), ("loin", 0.9, -50, 0.1, 1)]),
)

FORMS = {"grown": GROWN, "hatchling": HATCH}


# ------------------------------------------------------------------------------ part shapes
def _bone_axis(kit, bone):
    """(head, tail) of a plan bone in the current form's rest pose."""
    extra = kit.extra_points()
    for name, h, t, _ in kit.PLAN.BONES:
        if name == bone:
            get = lambda n: kit.V(extra[n]) if n in extra else kit.node(n)  # noqa: E731
            return get(h), get(t)
    raise KeyError(bone)


def _body_bvh(kit, body):
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


def surface_point(kit, bvh, bone, t, angle):
    """Where a ray from a bone's axis (a fraction t along it) at an angle round the body (0 on
    top, + to its right) leaves the skin: (point, outward normal, the bone's axis)."""
    V = kit.V
    head, tail = _bone_axis(kit, bone)
    axis = (tail - head).normalized()
    up = V((0, -axis.z, axis.y))
    if up.z < 0:
        up = -up
    side = axis.cross(up).normalized()
    if side.x < 0:
        side = -side
    a = math.radians(angle)
    direction = (up * math.cos(a) + side * math.sin(a)).normalized()
    hit, nrm = _outer_hit(bvh, head.lerp(tail, t), direction)
    if hit is None:
        return None, None, axis
    return hit, (V(nrm).normalized() + direction).normalized(), axis


def _facet(kit, obj):
    """Flat facets (split every edge, so the game's vertex normals keep each face flat)."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.split_edges(bm, edges=list(bm.edges))
    bm.to_mesh(obj.data)
    bm.free()
    for p in obj.data.polygons:
        p.use_smooth = False


def _ice_plate(kit, bvh, name, centre, normal, back, size, lift, prism=False):
    """An ice plate draped over the fur, shaped like the Curlstone's scales: broad and round in
    front, a softly pointed free edge behind, domed on top (a ring and an apex; a plain lens at
    LOD1). The dome is ice (horn); its free rear edge is a frosted crescent (membrane: thin,
    pale ice). Its front edge lies on the skin and its rear edge is lifted by `lift`, so it rides
    over the plate behind. The top and the underside don't share their rim vertices (the top's
    normals stay upward, the rim a crisp edge), and the crescent has its own vertices (a crisp
    colour edge: the exporter paints by vertex). prism (the rare one's): the dome glows and the
    crescent is rose. Its origin is the skin point under its middle (where the kit seats it)."""
    import bmesh
    V = kit.V
    w, ln, h = size
    domed = not kit.LOD
    n = 6 if domed else 4
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
        u = min(1.0, max(0.0, (py / (ln * 0.5) + 0.15) / 1.15))
        return lift * u ** 1.4

    def outline(a, k):
        sx = math.sin(a) * w * 0.5 * (1.0 - 0.22 * max(0.0, -math.cos(a))) * k
        sy = (-math.cos(a) * ln * 0.5 * (1.0 + 0.12 * max(0.0, -math.cos(a))) + ln * 0.04) * k
        return sx, sy

    bm = bmesh.new()
    angles = [2 * math.pi * (i + (0.0 if n == 4 else 0.5)) / n for i in range(n)]
    rim_top, rim_bot, mid_top, mid_rim = [], [], [], []
    for a in angles:
        sx, sy = outline(a, 1.0)
        p = drape(sx, sy, lift_at(sy) - 0.006) - c
        rim_top.append(bm.verts.new(p))
        rim_bot.append(bm.verts.new(p))
        if domed:
            mx, my = outline(a, 0.72)
            q = drape(mx, my, h * 0.6 + lift_at(my)) - c
            mid_top.append(bm.verts.new(q))
            mid_rim.append(bm.verts.new(q))
    apex = bm.verts.new(drape(0.0, ln * 0.04, h + lift_at(ln * 0.04)) - c)
    bottom = bm.verts.new(drape(0.0, 0.0, -h * 0.4) - c)
    inner = mid_top if domed else rim_top
    for k in range(n):
        j = (k + 1) % n
        bm.faces.new((apex, inner[k], inner[j])).material_index = 0
        if domed:
            rear = -math.cos(angles[k] + math.pi / n) > -0.35
            if rear:
                bm.faces.new((rim_top[k], rim_top[j], mid_rim[j], mid_rim[k])).material_index = 1
            else:
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


def plates(kit, d, bands, prism=False, scale=1.0, bvh=None):
    """Ice plates down the back: [(object, bone)]. Each plate is cast from the bone's axis out
    through the skin at its angle round the body, draped on the skin there."""
    bvh = bvh or _body_bvh(kit, d["body"])
    mats = d["mats"]
    glow = lit_glow(kit, d, k=0.35) if prism else None
    out = []
    for bone, t, angles, size, lift in bands:
        for ang in angles:
            hit, normal, axis = surface_point(kit, bvh, bone, t, ang)
            if hit is None:
                continue
            grow = kit.lod(1.0, 1.25)
            sz = (size[0] * scale * grow, size[1] * scale * grow, size[2] * scale)
            o = _ice_plate(kit, bvh, f"plate_{bone}_{ang}_{int(prism)}", hit, normal, axis, sz, lift * scale,
                           prism=prism)
            if prism:
                for m in (glow, mats["membrane"]):
                    o.data.materials.append(m)
            else:
                o.data.materials.append(mats["horn"])
                o.data.materials.append(mats["membrane"])
            out.append((o, bone))
    return out


def crystal(kit, name, base, direction, length, radius, materials, sides=None, tip=0.34):
    """A faceted ice crystal: a prism with a pointed tip, every face flat; its sides in the first
    material, its tip facets in the second (frosted). Its origin is the base."""
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
        bm.faces.new((lo[k], lo[(k + 1) % sides], hi[(k + 1) % sides], hi[k])).material_index = 0
        bm.faces.new((hi[k], hi[(k + 1) % sides], apex)).material_index = 1 if len(materials) > 1 else 0
    bm.normal_update()
    for f in bm.faces:
        if f.normal.dot(f.calc_center_median()) < 0:
            f.normal_flip()
    obj = kit.mesh_object(name, bm, V(base))
    _facet(kit, obj)
    for m in materials:
        obj.data.materials.append(m)
    return obj


def lit_glow(kit, d, name="glow_flat", colour="glow", k=0.5):
    """A preview material for glowing parts as the game draws them (lit colour plus the glow
    added on top), not the kit's flat emission, so the facets read in the review (the
    Curlstone's). Named "<name>.001": the exporter reads the name before the dot."""
    if "lit_glow" in d:
        return d["lit_glow"]
    c = kit.variant_colors(d["variant"])[colour]
    mat = kit.toon_material(name, c)
    nt = mat.node_tree
    add = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeMix" and n.blend_type == "ADD")
    em = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeEmission")
    glow = nt.nodes.new("ShaderNodeMix")
    glow.data_type, glow.blend_type = "RGBA", "ADD"
    glow.inputs["Factor"].default_value = 1.0
    glow.inputs["B"].default_value = (*[x * k for x in c], 1)
    nt.links.new(add.outputs["Result"], glow.inputs["A"])
    nt.links.new(glow.outputs["Result"], em.inputs["Color"])
    d["lit_glow"] = mat
    return mat


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
    """A tall fox ear (the Flurrytail's): an almond cross-section tapering to a soft point,
    flattened front to back; origin at its base. tip_from: faces above this fraction of its
    height take the second material and are cut into flat facets (an ice tip)."""
    import bmesh
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
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        tip = [p for p in bm.faces if p.calc_center_median().dot(u) > tip_from * height]
        for p in tip:
            p.material_index = 1
            p.smooth = False
        edges = {e for p in tip for e in p.edges}
        bmesh.ops.split_edges(bm, edges=list(edges))
        bm.to_mesh(obj.data)
        bm.free()
    return obj


# ------------------------------------------------------------------------------ hooks
def parts(kit, d):
    F, mats, V = kit.F, d["mats"], kit.V
    baby = d["form"] == "hatchling"
    k = F["head"]["k"]
    out = []
    bvh = _body_bvh(kit, d["body"])
    # Tall fox ears with faceted ice tips, the inner ear in the accent colour.
    ears = []
    for s in (-1, 1):
        base = kit.head_point((0.19, 0.06, 0.20) if not baby else (0.21, 0.07, 0.20), s)
        up = (s * 0.36, 0.20, 0.91) if not baby else (s * 0.42, 0.16, 0.89)
        front = (s * 0.42, -0.9, 0.08)
        h = (0.52 if not baby else 0.36) * k
        e = _ear(kit, f"ear_{s}", base, up, front, h, 0.16 * k, 0.05 * k, tip_bend=0.05, tip_from=0.6)
        e.data.materials.append(mats["body_plain"])
        e.data.materials.append(mats["horn"])
        inner = _ear(kit, f"earin_{s}", base + V(front).normalized() * 0.03 * k + V((0, 0, 0.02 * k)),
                     up, front, h * 0.72, 0.1 * k, 0.03 * k, tip_bend=0.05, seg=6, steps=3)
        inner.data.materials.append(mats["accent_flat"])
        ears.append((kit.join([e, inner], f"ear_{s}"), "head"))
    # A fox's dark button nose over the nostrils.
    nose = kit.blob("nose", F["nose"]["at"], F["nose"]["size"], kit.lod(6, 5), kit.lod(4, 3))
    nose.data.materials.append(mats["pupil"])
    out.append(("frill", 0, ears + [(nose, "snout")]))
    # Ice plates down the back (tiny nubs on a baby); the rare one's glow like prisms.
    bands = F["plates"][1] if kit.LOD else F["plates"][0]
    out.append(("spikes", 0, plates(kit, d, bands, bvh=bvh)))
    out.append(("spikes", 1, plates(kit, d, bands, prism=True, bvh=bvh)))
    # A little crest of ice crystals on the brow (a baby has none); the rare one's taller and
    # glowing (on a baby, one glowing bud).
    ice = [mats["horn"], mats["membrane"]]
    glow = [lit_glow(kit, d, k=0.3), mats["membrane"]]
    crest = [((0.0, 0.02, 0.33), (0.0, 0.25, 1.0), 0.26, 0.05),
             ((0.07, 0.08, 0.31), (0.3, 0.45, 0.9), 0.19, 0.042)]
    crest_rare = [((0.0, 0.02, 0.33), (0.0, 0.22, 1.0), 0.38, 0.058),
                  ((0.08, 0.06, 0.31), (0.32, 0.35, 0.95), 0.28, 0.05),
                  ((0.05, 0.16, 0.30), (0.25, 0.8, 0.8), 0.2, 0.042)]
    if baby:
        crest, crest_rare = [], crest_rare[:1]

    def brow(specs, k_len, materials, tag):
        objs = []
        for i, (at, dirn, length, r) in enumerate(specs):
            for s in (-1, 1):
                if at[0] == 0.0 and s < 0:
                    continue
                o = crystal(kit, f"crest{tag}_{i}_{s}", kit.head_point(at, s), kit.mirror(dirn, s),
                            length * k * k_len, r * k, materials)
                objs.append((o, "head"))
        return objs

    out.append(("horns", 0, brow(crest, 0.6 if baby else 1.0, ice, "a")))
    out.append(("horns", 1, brow(crest_rare, 0.7 if baby else 1.0, glow, "g")))
    # Crystal points peeking out between the plates; the rare one's glowing clusters.
    for variant, spots, materials in ((0, F["crystals"][0], ice), (1, F["crystals"][1], glow)):
        pieces = []
        for i, (bone, t, ang, length, count) in enumerate(spots):
            hit, normal, axis = surface_point(kit, bvh, bone, t, ang)
            if hit is None:
                continue
            side = axis.cross(normal).normalized()
            for j in range(count if not kit.LOD else 1):
                spread = (j - (count - 1) / 2) * 0.55
                dirn = (normal * 1.0 + axis * 0.35 + side * spread).normalized()
                base = hit + side * spread * length * 0.35 - normal * length * 0.12
                o = crystal(kit, f"gem{variant}_{i}_{j}", base, dirn, length * (1.0 - 0.25 * abs(spread)),
                            length * 0.2, materials)
                pieces.append((o, bone))
        out.append(("runes", variant, pieces))
    return out


# The crystal wing: a slim ice arm (root, elbow, wrist and the leading finger) fanning faceted
# shards, in the Curlstone's wing layout (its plan's fold is solved for it): (from, to,
# width) in layout units; each shard's two nearest struts (the kit's weights) fold alike.
SHARDS = [((1.12, -0.02), (2.02, 0.30), 0.30), ((1.12, 0.0), (1.98, 0.68), 0.32),
          ((1.12, 0.02), (1.80, 1.00), 0.32), ((1.12, 0.04), (1.50, 1.24), 0.30),
          ((1.12, 0.06), (1.22, 1.40), 0.28)]
SHARDS_LOD1 = [SHARDS[i] for i in (0, 2, 4)]
BUD_SHARDS = [((1.12, -0.02), (2.02, 0.30), 0.42), ((1.10, 0.06), (1.80, 1.00), 0.46),
              ((0.90, 0.16), (1.16, 1.40), 0.42)]


def _shard(bm, a, b, width, thick, n_up, slots, frac=0.34):
    """A crystal shard from a to b: a flattened bipyramid (a four-sided ring a third of the way
    out, pointed at both ends), lying in the plane with normal n_up; its facets on one side of
    its ridge in slots[0], the other side in slots[1]."""
    from mathutils import Vector
    A, B = Vector(a), Vector(b)
    d = B - A
    L = d.length
    d.normalize()
    n = (n_up - d * n_up.dot(d)).normalized()
    s = n.cross(d).normalized()
    m = A + d * L * frac
    ring = [bm.verts.new(p) for p in (m + s * width * 0.5, m + n * thick, m - s * width * 0.5, m - n * thick)]
    ends = [bm.verts.new(A - d * L * 0.04), bm.verts.new(B)]
    for e, flip in ((ends[0], True), (ends[1], False)):
        for q in range(4):
            vs = (ring[q], ring[(q + 1) % 4], e)
            f = bm.faces.new(tuple(reversed(vs)) if flip else vs)
            f.material_index = slots[0] if q in (0, 3) else slots[1]
    return bm


def wings(kit, d, rare):
    """Small crystal wings: a slim ice arm fanning five faceted shards from the wrist, frosted
    pale on one face of each ridge (the rare one's shards glow like prisms)."""
    import bmesh
    from mathutils import Vector
    F, mats = kit.F, d["mats"]
    baby = d["form"] == "hatchling"
    wr = F["wing"]["radii"]
    specs = BUD_SHARDS if baby else kit.lod(SHARDS, SHARDS_LOD1)
    objs = []
    for side in ("L", "R"):
        s = -1 if side == "L" else 1
        w = kit.wing_points(side)
        arm = kit.wing_arm(side, ["root", "elbow", "wrist", "f1"], [wr["root"], wr["elbow"], wr["wrist"], wr["finger"]],
                           kit.lod(F["wing"]["arm_tris"], 30), mats, material="horn", claws=False)
        objs.append(arm)
        th, ph = math.radians(F["wing"]["dihedral"]), math.radians(F["wing"]["droop"])
        span = Vector((s * math.cos(th), 0, math.sin(th)))
        chord = Vector((0, math.cos(ph), -math.sin(ph)))
        normal = span.cross(chord).normalized()
        if normal.z < 0:
            normal = -normal
        root = w["root"]
        sc = F["wing"]["scale"]

        def at(uv):
            return root + (span * uv[0] + chord * uv[1]) * sc

        bm = bmesh.new()
        for (a, b, width) in specs:
            _shard(bm, at(a), at(b), width * sc, width * sc * 0.11, normal, (0, 1))
        sh = kit.mesh_object(f"{'prism' if rare else 'shards'}_{side}", bm)
        _facet(kit, sh)
        if rare:
            sh.data.materials.append(lit_glow(kit, d, k=0.35))
            sh.data.materials.append(mats["membrane"])
        else:
            sh.data.materials.append(mats["membrane"])
            sh.data.materials.append(mats["horn"])
        objs.append(sh)
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
            face = ramp(-y, 0.58, 0.68) * ramp(0.95 - z, 0.0, 0.06) * ramp(-n.z + 0.35, 0.0, 0.4)
            cheek = ramp(-y, 0.44, 0.54) * ramp(abs(x), 0.14, 0.22) * ramp(0.98 - z, 0.0, 0.08)
            a = max(belly, bib, face, cheek) * ramp(0.38 - y, 0.0, 0.1)
        else:
            belly = ramp(-n.z, 0.25, 0.6) * ramp(1.2 - z, 0.0, 0.2) * ramp(0.42 - abs(x), 0.0, 0.1) * \
                ramp(1.3 - y, 0.0, 0.2)
            bib = ramp(-y, 0.9, 1.1) * ramp(1.46 - z, 0.0, 0.14) * ramp(z, 0.45, 0.6) * ramp(-n.y, -0.2, 0.3)
            face = ramp(-y, 2.18, 2.3) * ramp(1.42 - z, 0.0, 0.06) * ramp(-n.z + 0.3, 0.0, 0.4)
            cheek = ramp(-y, 1.8, 1.95) * ramp(abs(x), 0.2, 0.3) * ramp(1.5 - z, 0.0, 0.1) * ramp(z, 1.0, 1.1)
            a = max(belly, bib, face, cheek)
        mask.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


def texture(tx, nt, p, form):
    """R: frost points (socks up the legs); G: aurora ribbons wavering
    across the back and down the plume; B: glints, a scatter of small sparkles (the rare
    one's glow)."""
    y, z = tx.axis(nt, 1), tx.axis(nt, 2)
    socks = tx.mul(nt, tx.smoothstep(nt, z, p["sock"][0], p["sock"][1]), tx.smoothstep(nt, y, p["tail_y"],
                                                                                        p["tail_y"] - 0.1))
    r = socks
    # ribbons: wavering bands across the body and the plume, slanting round it, not on the face
    wob = tx.node(nt, "ShaderNodeTexNoise")
    wob.inputs["Scale"].default_value = 0.6 / p["ribbon"]
    wob.inputs["Detail"].default_value = 1.5
    nt.links.new(tx.coords(nt), wob.inputs["Vector"])
    period = p["ribbon"]
    slant = tx.madd(nt, tx.axis(nt, 0), 0.8, 0.0)
    warped = tx.add(nt, tx.add(nt, y, slant), tx.madd(nt, wob.outputs["Fac"], period * 1.4, -period * 0.7))
    wave = tx.math_op(nt, "SINE", tx.madd(nt, warped, 2 * math.pi / period, 0.0))
    thick = tx.madd(nt, wob.outputs["Fac"], -0.5, 0.72)
    ribbons = tx.smoothstep(nt, tx.math_op(nt, "SUBTRACT", wave, thick), -0.05, 0.2)
    face_off = tx.smoothstep(nt, y, -1.2 if form == "grown" else -0.3, -0.9 if form == "grown" else -0.18)
    g = tx.mul(nt, tx.mul(nt, ribbons, tx.upper(nt)), face_off)
    # glints: small sparkles on a few cells of a fine grid
    b = tx.spots(nt, p["glint_cell"], keep=0.72, size=(0.2, 0.1), where=tx.const(nt, 1.0))
    return {"r": r, "g": g, "b": b}
