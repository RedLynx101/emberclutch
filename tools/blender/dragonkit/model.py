"""The dragon kit's builder: any kind's two body forms, rig, parts, wings and growth.

Previews (headless; absolute --out paths):
  blender -b -P tools/blender/dragonkit/model.py -- --kind pouncer --stages hatchling,adult --out C:/abs/prefix
  ... -- --kind pouncer --lineup --out C:/abs/prefix            (growth lineup at true size)
  ... -- --kind pouncer --stages adult --variant 3 --turntable 8 --texture

How a kind is built (docs/tech/dragon-kit.md; the classic dragon's tools/blender/dragon_model.py
is where all of this comes from):
  * Two FORMS share the plan's skeleton (same bone names and hierarchy, own positions):
      hatchling  the baby body (usually metaballs: soft and round), the whole hatchling stage
      grown      the young-to-adult body (usually a skin-modifier graph), juvenile to adult
    The stage-up to juvenile swaps them behind a glow (the first molt, D36).
  * Within a form, growth is pose-space bone scales (girth, length, girth) with scale
    inheritance off, exactly what the runtime does (architecture section 4).
  * Parts (eyes, horns, frills, ridges, tail tips, the heartglow, anything else) are separate
    rigid meshes bound to one bone each, seated on the skin at every growth key. Wings are a
    second skinned draw. The mouth opens on the jaw bone.

A kind module (tools/dragons/kinds/<kind>.py) provides META, VARIANTS, FORMS and optional hooks
(parts, wings, sculpt, accent, texture, egg); see tools/dragons/kinds/__init__.py for the
contract. Hooks receive this module as `kit` and use its helpers (kit.horn_mesh, kit.blade,
kit.lobed_fin, kit.flat_fan, kit.add_dome, kit.build_ridge, kit.mirror, kit.V, kit.lod, kit.F ...).
"""
import math
import os
import sys

import bmesh
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
for _p in (os.path.join(ROOT, "tools"), os.path.join(ROOT, "tools", "blender"), os.path.join(ROOT, "tools", "anim")):
    if _p not in sys.path:
        sys.path.insert(0, _p)
import dragons  # noqa: E402
from dragonkit import texture as tex  # noqa: E402

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


kit = sys.modules[__name__]  # what hooks receive

# ------------------------------------------------------------------------------ kind & plan
KIND = None        # the kind module
PLAN = None        # its body plan module
F = None           # the form being built (a dict from KIND.FORMS)
LOD = 0            # 0 = full (LOD0), 1 = den background (LOD1)
BONES, WING_CHAIN, WING_BONES = [], [], []

# Classic bat-style wing in its own plane: u = out along the span, v = back along the chord.
# A form's wing may give its own "layout" (any points its plan's WING_CHAIN names, plus "root"
# and "body", the flank point where the membrane meets the body).
WING_LAYOUT = {"root": (0.0, 0.0), "elbow": (0.95, 0.35), "wrist": (1.75, -0.15), "thumb": (1.82, -0.42),
               "f1": (3.40, 0.15), "f2": (3.30, 1.20), "f3": (2.65, 2.05), "f4": (1.60, 2.45),
               "body": (0.0, 1.40)}
WING_SINK = 0.6    # the wing's root joint sits this many root radii under the skin
WING_STUB = 1.8    # the arm tube carries on this many root radii into the body
WEB_SINK = 4.5     # the membrane's inner edge sits this many membrane thicknesses under the skin

FORM_DEFAULTS = dict(
    edges=None, meta=None, body="skin", body_tris=1600, body_tris_lod1=600, export_scale=1.0,
    young_pose={}, base_pose={}, key_ts=(0.0, 0.35, 0.70, 1.0),
    builds={"neutral": {}, "sturdy": {}, "sleek": {}, "long": {}},
    inset={"eyes": 0.02, "horns": 0.03, "spikes": 0.045, "frill": 0.09, "heart": -0.065, "runes": -0.012},
    tail_k=1.0, runes=[], sculpt=None, mouth=True, teeth=True,
)
PART_GROUPS = ("eyes", "horns", "frill", "spikes", "tail_tip", "heart", "mouth", "runes")


def use_kind(name):
    """Load a kind and its body plan; everything after builds that kind."""
    global KIND, PLAN, BONES, WING_CHAIN, WING_BONES
    KIND = dragons.kind(name)
    PLAN = dragons.plan(KIND.META["plan"])
    BONES, WING_CHAIN, WING_BONES = PLAN.BONES, PLAN.WING_CHAIN, PLAN.WING_BONES
    for form in KIND.FORMS.values():
        for k, v in FORM_DEFAULTS.items():
            form.setdefault(k, v)
        for b in ("neutral", "sturdy", "sleek", "long"):
            form["builds"].setdefault(b, {})
        form["wing"].setdefault("layout", WING_LAYOUT)
        form["wing"].setdefault("style", "classic")
    return KIND


def use_form(name):
    global F
    F = KIND.FORMS[name]
    return F


def set_lod(level):
    global LOD
    LOD = level


def lod(full, low):
    """The LOD0 value, or the LOD1 one."""
    return low if LOD else full


def lerp(a, b, t):
    return a + (b - a) * t


def base_key(name):
    return name.rsplit("_", 1)[0] if name.endswith(("_L", "_R")) else name


SCALE_LIKE = {"jaw": "snout", "eyes": "head"}  # bones that grow like another (see extra_points)


def scale_key(name):
    """The growth and build table entry a bone uses."""
    return SCALE_LIKE.get(base_key(name), base_key(name))


def scales_for_t(t, build):
    """Bone scales (girth_x, length, girth_z) and part scales at growth t within the current
    form. The runtime computes exactly this from the tables exported in the .ecm."""
    bones = {}
    for name, *_ in BONES:
        h = F["young"]["bones"].get(scale_key(name), (1.0, 1.0))
        gx, ln, gz = (h[0], h[1], h[0]) if len(h) == 2 else h
        gx, ln, gz = lerp(gx, 1.0, t), lerp(ln, 1.0, t), lerp(gz, 1.0, t)
        bg, bl = F["builds"][build].get(scale_key(name), (1.0, 1.0))
        bones[name] = (gx * bg, ln * bl, gz * bg)
    parts = {k: lerp(v, 1.0, t) for k, v in F["young"]["parts"].items()}
    for name in WING_BONES:
        bones[name] = (parts.get("wings", 1.0),) * 3
    return bones, parts


def young_tables():
    """(bone name -> t = 0 scale triple) for the exporter, wings included."""
    bones, _ = scales_for_t(0.0, "neutral")
    return bones


# ------------------------------------------------------------------------------ helpers
def V(p):
    return Vector(p)


def link(obj):
    bpy.context.collection.objects.link(obj)
    return obj


def smooth(obj):
    """Smooth shading everywhere: clear any sharp-edge/face data modifiers leave behind."""
    me = obj.data
    for name in ("sharp_edge", "sharp_face"):
        if name in me.attributes:
            me.attributes.remove(me.attributes[name])
    for p in me.polygons:
        p.use_smooth = True


def apply_modifiers(obj):
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    for m in list(obj.modifiers):
        bpy.ops.object.modifier_apply(modifier=m.name)
    obj.select_set(False)


def tri_count(obj):
    dg = bpy.context.evaluated_depsgraph_get()
    me = obj.evaluated_get(dg).to_mesh()
    n = sum(len(p.vertices) - 2 for p in me.polygons)
    obj.evaluated_get(dg).to_mesh_clear()
    return n


def decimate_to(obj, target):
    cur = tri_count(obj)
    if cur > target:
        m = obj.modifiers.new("dec", "DECIMATE")
        m.ratio = target / cur
        m.use_collapse_triangulate = True
        apply_modifiers(obj)


def mesh_object(name, bm, location=(0, 0, 0)):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = link(bpy.data.objects.new(name, me))
    obj.location = location
    return obj


def join(objs, name):
    """Join objects into one (vertex groups cleared first: Blender 5.2 crashes joining meshes
    that still carry them)."""
    for o in objs:
        for vg in list(o.vertex_groups):
            o.vertex_groups.remove(vg)
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.select_set(False)
    obj.name = name
    return obj


def mirror(p, s):
    return Vector((s * p[0], p[1], p[2]))


def node(name):
    """A node's position in the current form."""
    return V(F["nodes"][name][0])


# ------------------------------------------------------------------------------ materials
# Material names the exporter turns into vertex paint (palette slots, see export.MATERIAL_PAINT).
# A kind's parts use these names: "body" (base, accent by the mask), "body_plain" (base),
# "accent_flat", "pattern_flat", "membrane", "horn", "iris", "pupil", "glint", "heart",
# "rune" (glowing in the pattern colour), "glow_flat" (glowing in the heart colour), "tooth",
# "tongue", "mouth".
def toon_material(name, color, accent=None, emission=0.0, rim=(1.0, 0.72, 0.45)):
    """Toon ramp x colour (+ accent via the 'mask' colour attribute) + warm rim."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    if emission:
        em = nt.nodes.new("ShaderNodeEmission")
        em.inputs["Color"].default_value = (*color, 1)
        em.inputs["Strength"].default_value = emission
        nt.links.new(em.outputs[0], out.inputs["Surface"])
        return mat
    diff = nt.nodes.new("ShaderNodeBsdfDiffuse")
    s2r = nt.nodes.new("ShaderNodeShaderToRGB")
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.interpolation = "CONSTANT"
    els = ramp.color_ramp.elements
    els[0].position, els[0].color = 0.0, (0.46, 0.40, 0.48, 1)  # plum-tinted shadow, softer (the storybook look)
    els[1].position, els[1].color = 0.2, (0.80, 0.76, 0.78, 1)
    e = els.new(0.52)
    e.color = (1.0, 0.98, 0.95, 1)
    nt.links.new(diff.outputs[0], s2r.inputs[0])
    nt.links.new(s2r.outputs["Color"], ramp.inputs["Fac"])
    base = nt.nodes.new("ShaderNodeRGB")
    base.outputs[0].default_value = (*color, 1)
    col = base.outputs[0]
    if accent is not None:
        attr = nt.nodes.new("ShaderNodeAttribute")
        attr.attribute_name = "mask"
        sep = nt.nodes.new("ShaderNodeSeparateColor")
        acc = nt.nodes.new("ShaderNodeRGB")
        acc.outputs[0].default_value = (*accent, 1)
        mix = nt.nodes.new("ShaderNodeMix")
        mix.data_type = "RGBA"
        nt.links.new(attr.outputs["Color"], sep.inputs[0])
        nt.links.new(sep.outputs["Green"], mix.inputs["Factor"])
        nt.links.new(base.outputs[0], mix.inputs["A"])
        nt.links.new(acc.outputs[0], mix.inputs["B"])
        col = mix.outputs["Result"]
    mul = nt.nodes.new("ShaderNodeMix")
    mul.data_type = "RGBA"
    mul.blend_type = "MULTIPLY"
    mul.inputs["Factor"].default_value = 1.0
    nt.links.new(col, mul.inputs["A"])
    nt.links.new(ramp.outputs["Color"], mul.inputs["B"])
    lw = nt.nodes.new("ShaderNodeLayerWeight")
    lw.inputs["Blend"].default_value = 0.35
    rr = nt.nodes.new("ShaderNodeValToRGB")
    rr.color_ramp.interpolation = "CONSTANT"
    rr.color_ramp.elements[0].position = 0.0
    rr.color_ramp.elements[0].color = (0, 0, 0, 1)
    rr.color_ramp.elements[1].position = 0.72
    rr.color_ramp.elements[1].color = (*[c * 0.35 for c in rim], 1)
    nt.links.new(lw.outputs["Facing"], rr.inputs["Fac"])
    add = nt.nodes.new("ShaderNodeMix")
    add.data_type = "RGBA"
    add.blend_type = "ADD"
    add.inputs["Factor"].default_value = 1.0
    nt.links.new(mul.outputs["Result"], add.inputs["A"])
    nt.links.new(rr.outputs["Color"], add.inputs["B"])
    em = nt.nodes.new("ShaderNodeEmission")
    nt.links.new(add.outputs["Result"], em.inputs["Color"])
    nt.links.new(em.outputs[0], out.inputs["Surface"])
    return mat


def variant_colors(index):
    """A variant's colours with every slot filled (tools/dragons/kinds contract)."""
    v = dict(KIND.VARIANTS[index])
    v.setdefault("pupil", (0.06, 0.03, 0.05))
    v.setdefault("glint", (1.0, 1.0, 1.0))
    v.setdefault("tongue", (0.93, 0.45, 0.55))
    v.setdefault("horn", v["accent"])
    v.setdefault("membrane", tuple(0.55 * c + 0.45 * a for c, a in zip(v["base"], v["accent"])))
    v.setdefault("pattern", v["base"])
    return v


def make_materials(variant=0):
    c = variant_colors(variant)
    return {
        "body": toon_material("body", c["base"], accent=c["accent"]),
        "body_plain": toon_material("body_plain", c["base"]),
        "accent_flat": toon_material("accent_flat", c["accent"]),
        "pattern_flat": toon_material("pattern_flat", c["pattern"]),
        "membrane": toon_material("membrane", c["membrane"]),
        "horn": toon_material("horn", c["horn"]),
        "iris": toon_material("iris", c["iris"]),
        "pupil": toon_material("pupil", c["pupil"]),
        "glint": toon_material("glint", c["glint"], emission=2.0),
        "heart": toon_material("heart", c["glow"], emission=1.4),
        "rune": toon_material("rune", c["pattern"], emission=1.2),
        "glow_flat": toon_material("glow_flat", c["glow"], emission=1.2),
        "tooth": toon_material("tooth", (0.97, 0.95, 0.90)),
        "tongue": toon_material("tongue", c["tongue"]),
        "mouth": toon_material("mouth", (0.36, 0.11, 0.16)),
    }


# ------------------------------------------------------------------------------ body
def build_body():
    obj = build_skin_body() if F["body"] == "skin" else build_meta_body()
    if F.get("sculpt"):
        F["sculpt"](kit, obj)
    decimate_to(obj, lod(F["body_tris"], F["body_tris_lod1"]))
    remove_loose(obj)
    smooth(obj)
    if hasattr(KIND, "accent"):
        KIND.accent(kit, obj)
    else:
        paint_mask(obj)
    return obj


def remove_loose(obj):
    """Drop vertices that belong to no face (decimation can leave one behind, and a loose
    vertex under the body still counts for the floor contact: the classic dragon's "limp")."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    loose = [v for v in bm.verts if not v.link_faces]
    if loose:
        bmesh.ops.delete(bm, geom=loose, context="VERTS")
        bm.to_mesh(obj.data)
    bm.free()


def build_skin_body():
    """A skin-modifier body from the form's node graph: nodes {name: (pos, (rx, rz))} joined by
    edges [(a, b)], subdivided and relaxed. Good for long, lean bodies with clear limbs."""
    nodes = F["nodes"]
    names = list(nodes)
    idx = {n: i for i, n in enumerate(names)}
    me = bpy.data.meshes.new("body")
    me.from_pydata([nodes[n][0] for n in names], [(idx[a], idx[b]) for a, b in F["edges"]], [])
    obj = link(bpy.data.objects.new("body", me))
    skin = obj.modifiers.new("skin", "SKIN")
    skin.use_smooth_shade = True
    skin.branch_smoothing = F.get("branch_smoothing", 0.6)
    for i, n in enumerate(names):
        sv = me.skin_vertices[0].data[i]
        sv.radius = nodes[n][1]
        sv.use_root = n == F.get("skin_root", "hips")
    apply_modifiers(obj)
    close_holes(obj)
    sub = obj.modifiers.new("sub", "SUBSURF")
    sub.levels = sub.render_levels = int(arg("--subd", "2"))
    sm = obj.modifiers.new("relax", "SMOOTH")
    sm.factor = F.get("relax", 0.6)
    sm.iterations = 6
    apply_modifiers(obj)
    return obj


def close_holes(obj):
    """The skin modifier can fail to hull a node where branches meet and leave a hole there
    (and a stray wire edge): the grown Kindlemoss's and Curlstone's left shoulders were open,
    a see-through gap in the front leg (the close-up check, run 18). Fill any hole in the
    coarse hull before it's subdivided, so the patch smooths in with the rest."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    wires = [e for e in bm.edges if not e.link_faces]
    if wires:
        bmesh.ops.delete(bm, geom=wires, context="EDGES")
    rims = [e for e in bm.edges if len(e.link_faces) == 1]
    if rims:
        bmesh.ops.holes_fill(bm, edges=rims, sides=0)
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)  # closed now: all outward
        print(f"[kit] {KIND.META['name']}: filled a hole in the skin hull ({len(rims)} edges)")
    if wires or rims:
        bm.to_mesh(obj.data)
    bm.free()


META_VISIBLE = 0.575  # a lone metaball of radius 1 (stiffness 2, threshold 0.6) is visible out to 0.575


def build_meta_body():
    """Metaballs blend soft round volumes: ideal for chubby babies and round bodies. The form's
    "meta" list holds ("ball", centre, radius), ("ell", centre, (rx, ry, rz)) and
    ("chain", [points], [radii]) elements (sizes are visible radii). Voxel remesh evens the
    topology for decimation and weighting."""
    mb = bpy.data.metaballs.new("body")
    mb.resolution = mb.render_resolution = F.get("meta_resolution", 0.018)

    def ball(c, r):
        e = mb.elements.new(type="BALL")
        e.co, e.radius = c, r / META_VISIBLE

    for kind, c, size in F["meta"]:
        if kind == "ball":
            ball(c, size)
        elif kind == "ell":
            e = mb.elements.new(type="ELLIPSOID")
            m = max(size)
            e.co, e.radius = c, m / META_VISIBLE
            e.size_x, e.size_y, e.size_z = (r / m for r in size)
        else:  # chain of balls with linearly tapering radii
            pts, radii = [V(p) for p in c], size
            for (a, ra), (b, rb) in zip(zip(pts, radii), zip(pts[1:], radii[1:])):
                n = max(1, int((b - a).length / (0.35 * min(ra, rb))))
                for k in range(n):
                    ball(a.lerp(b, k / n), ra + (rb - ra) * k / n)
            ball(pts[-1], radii[-1])
    obj = link(bpy.data.objects.new("body", mb))
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    obj = bpy.context.view_layer.objects.active
    obj.select_set(False)
    rm = obj.modifiers.new("remesh", "REMESH")
    rm.mode = "VOXEL"
    rm.voxel_size = F.get("voxel", 0.03)
    sm = obj.modifiers.new("relax", "SMOOTH")
    sm.factor, sm.iterations = 0.5, 6
    apply_modifiers(obj)
    obj.name = obj.data.name = "body"
    return obj


def paint_mask(obj):
    """Per-vertex mask colour: R = base weight, G = accent (belly, throat, under-tail), from
    the form's "mask": max_x (how far out the belly reaches), max_z/min_z (height band),
    tail_cut (y, z) (no accent past y below z), down (the direction the accent faces)."""
    me = obj.data
    mk = F["mask"]
    attr = me.color_attributes.new("mask", "FLOAT_COLOR", "POINT")
    down = Vector(mk.get("down", (0, -0.45, -0.89))).normalized()
    for v in me.vertices:
        x, y, z = v.co
        a = min(1.0, max(0.0, (v.normal.dot(down) - mk.get("from", 0.25)) / mk.get("soft", 0.45)))
        if abs(x) > mk["max_x"] or z > mk["max_z"] or z < mk["min_z"]:
            a = 0.0
        if mk.get("tail_cut") and y > mk["tail_cut"][0] and z < mk["tail_cut"][1]:
            a = 0.0
        attr.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


def mask_attr(obj):
    """The body's accent mask attribute (for accent hooks): created white-base if missing."""
    me = obj.data
    return me.color_attributes.get("mask") or me.color_attributes.new("mask", "FLOAT_COLOR", "POINT")


# ------------------------------------------------------------------------------ wings
def wing_points(side, authored=False):
    """The form's wing layout placed in 3D: the span axis rises by the dihedral, the chord axis
    droops. Once seat_wings() has run, the root and the flank point are the seated ones
    (authored=True gives the layout as written)."""
    w = F["wing"]
    s = -1 if side == "L" else 1
    th, ph = math.radians(w["dihedral"]), math.radians(w["droop"])
    span = Vector((s * math.cos(th), 0, math.sin(th)))
    chord = Vector((0, math.cos(ph), -math.sin(ph)))
    seat = None if authored else w.get("seat")
    root = mirror(seat["root"] if seat else w["root"], s)
    pts = {k: root + (span * u + chord * v) * w["scale"] for k, (u, v) in w["layout"].items()}
    if seat:
        pts["body"] = mirror(seat["body"], s)
    return pts


def seat_wings(body):
    """Moves the wing's root joint just under the body's skin and its flank point onto the
    skin, from the nearest points on the surface (the classic dragon's floating wing roots,
    measured by tools/blender/wing_gap.py)."""
    w = F["wing"]
    if "seat" in w:
        return
    probe = body
    if LOD:
        was = LOD
        set_lod(0)
        probe = build_body()
        set_lod(was)
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    bvh = BVHTree.FromObject(probe.evaluated_get(dg), dg)
    to_local, to_world = probe.matrix_world.inverted(), probe.matrix_world

    def onto_skin(p, sink):
        hit, nrm, _, _ = bvh.find_nearest(to_local @ p)
        n = (to_world.to_3x3() @ nrm).normalized()
        return (to_world @ hit) - n * sink, n

    guide = wing_points("R", authored=True)
    back = F["nodes"].get(PLAN.WING_BODY[-1]) if PLAN.WING_BODY else None
    if back is not None:
        guide["body"].y = min(guide["body"].y, back[0][1])
    root, normal = onto_skin(guide["root"], WING_SINK * w["radii"]["root"])
    flank, _ = onto_skin(guide["body"], WEB_SINK * w["thickness"])
    w["seat"] = dict(root=tuple(root), body=tuple(flank), inward=tuple(-normal))
    if probe is not body:
        bpy.data.objects.remove(probe, do_unlink=True)


def wing_edge(w, style):
    """Trailing edge from the flank to the leading finger tip (the classic four-finger layout).
    classic: scalloped; sail: smooth and rounded; plumed: frilled into feather points."""
    def sag(a, b, depths):
        n = len(depths)
        return [a.lerp(b, j / (n + 1)).lerp(w["wrist"], depths[j - 1]) for j in range(1, n + 1)]

    edge = [w["body"]]
    for a, b, depth in (("body", "f4", 0.20), ("f4", "f3", 0.26), ("f3", "f2", 0.26), ("f2", "f1", 0.22)):
        n = lod(3, 1)
        if style == "sail":
            depths = [-0.05 * math.sin(math.pi * j / (n + 1)) for j in range(1, n + 1)]
        elif style == "plumed":
            m = lod(5, 3)
            depths = [depth * math.sin(math.pi * j / (m + 1)) * (1.0 if j % 2 else 0.35) for j in range(1, m + 1)]
        else:
            depths = [depth * math.sin(math.pi * j / (n + 1)) for j in range(1, n + 1)]
        edge += sag(w[a], w[b], depths) + [w[b]]
    return edge


def wing_arm(side, names, radii, tris, mats, material="body_plain", claws=True):
    """The wing's bony arm as a skin-modifier tube: a stub into the body, then the layout
    points `names` in order ("root", "elbow", "wrist", then branches from the wrist).
    radii: one per name. Branch points (after the wrist) all start at the wrist."""
    w = wing_points(side)
    s = -1 if side == "L" else 1
    inward = mirror(F["wing"]["seat"]["inward"], s) if "seat" in F["wing"] else Vector((-s, 0, -1)).normalized()
    stub = w["root"] + inward * (WING_STUB * F["wing"]["radii"]["root"])
    pos = [stub] + [w[n] for n in names]
    rad = [radii[0]] + list(radii)
    wrist = names.index("wrist") + 1 if "wrist" in names else None
    edges = [(0, 1)]
    for i in range(2, len(pos)):
        prev = wrist if (wrist is not None and i > wrist) else i - 1
        edges.append((prev, i))
    if claws and wrist is not None:
        tips = list(range(wrist + 1, len(pos)))
        for fi in tips:
            pos.append(pos[fi] + (pos[fi] - pos[wrist]) * 0.07)
            rad.append(F["wing"]["radii"]["tip"])
            edges.append((fi, len(pos) - 1))
    me = bpy.data.meshes.new(f"wingarm_{side}")
    me.from_pydata(pos, edges, [])
    arm = link(bpy.data.objects.new(f"wingarm_{side}", me))
    arm.modifiers.new("skin", "SKIN")
    for i, r in enumerate(rad):
        me.skin_vertices[0].data[i].radius = (r, r)
        me.skin_vertices[0].data[i].use_root = i == 0
    sub = arm.modifiers.new("sub", "SUBSURF")
    sub.levels = 1
    apply_modifiers(arm)
    decimate_to(arm, tris)
    smooth(arm)
    arm.data.materials.append(mats[material])
    return arm


def build_wings(style, mats):
    """Classic four-finger dragon wings (styles classic, plumed, sail): an arm, forearm, a thumb
    claw and four fingers spread across the whole membrane, which reaches back along the
    flank. Needs the classic wing layout and chain (plans/pouncer.py). The form's wing
    "claws": False leaves off the claw tips (a baby's soft wing buds)."""
    wr = F["wing"]["radii"]
    objs = []
    for side in ("L", "R"):
        w = wing_points(side)
        arm = wing_arm(side, ["root", "elbow", "wrist", "thumb", "f1", "f2", "f3", "f4"],
                       [wr["root"], wr["elbow"], wr["wrist"], wr["tip"] * 2.5] + [wr["finger"]] * 4,
                       lod(F["wing"]["arm_tris"], 56), mats, claws=F["wing"].get("claws", True))
        objs.append(arm)
        pts = [w["wrist"], w["elbow"], w["root"]] + wing_edge(w, style)
        mem = flat_fan(f"membrane_{side}", pts, F["wing"]["thickness"])
        mem.data.materials.append(mats["membrane"])
        objs.append(mem)
    return objs


def weight_membrane(mem, side):
    """Membrane (and feather) weights from the wing's own frame: each vertex follows the two
    struts nearest to it, blended by distance. Struts are the plan's wing bones (head point
    to tail point) and its WING_BODY bones along the flank (root to the "body" point)."""
    w = wing_points(side)
    struts = [(f"{name}_{side}", w[h], w[t]) for name, h, t, _ in WING_CHAIN]
    body = list(PLAN.WING_BODY)
    cuts = [0.0, 0.45, 0.8, 1.0] if len(body) == 3 else [i / len(body) for i in range(len(body) + 1)]
    for i, b in enumerate(body):  # the flank from the root back to the "body" point
        struts.append((b, w["root"].lerp(w["body"], cuts[i]), w["root"].lerp(w["body"], cuts[i + 1])))
    pivots = {}
    for name, h, t, parent in WING_CHAIN:  # a point several bones grow from: the parent carries it
        pivots.setdefault(h, set()).add(parent)
    pivot_bone = {h: f"{next(iter(ps))}_{side}" for h, ps in pivots.items()
                  if len([n for n, hh, _, _ in WING_CHAIN if hh == h]) > 1 and next(iter(ps)).startswith("wing")}

    def gap(p, a, b):
        ab = b - a
        t = max(0.0, min(1.0, (p - a).dot(ab) / max(ab.length_squared, 1e-9)))
        return (p - (a + ab * t)).length

    for vg in list(mem.vertex_groups):
        mem.vertex_groups.remove(vg)
    groups = {name: mem.vertex_groups.new(name=name) for name, _, _ in struts}
    for v in mem.data.vertices:
        p = mem.matrix_world @ v.co
        done = False
        for h, bone in pivot_bone.items():
            if (p - w[h]).length < 0.03:
                groups[bone].add([v.index], 1.0, "REPLACE")
                done = True
                break
        if done:
            continue
        (d0, n0), (d1, n1) = sorted((gap(p, a, b), name) for name, a, b in struts)[:2]
        w0 = d1 / (d0 + d1) if d0 + d1 > 1e-6 else 1.0
        groups[n0].add([v.index], w0, "REPLACE")
        if w0 < 1.0:
            groups[n1].add([v.index], 1.0 - w0, "REPLACE")


def weight_wingarm(arm_obj, side):
    """The arm tube's stub (behind the root joint, inside the body) follows the first wing
    bone's parent (the chest) and hands over to the first wing bone across the joint, so the
    arm stays rooted however far the shoulder turns."""
    name, h, t, parent = WING_CHAIN[0]
    w = wing_points(side)
    root, axis = w[h], (w[t] - w[h]).normalized()
    r = F["wing"]["radii"]["root"]
    lo, hi = -0.2 * r, 0.5 * r
    body = arm_obj.vertex_groups.get(parent) or arm_obj.vertex_groups.new(name=parent)
    upper = arm_obj.vertex_groups.get(f"{name}_{side}") or arm_obj.vertex_groups.new(name=f"{name}_{side}")
    for v in arm_obj.data.vertices:
        tt = (arm_obj.matrix_world @ v.co - root).dot(axis)
        if tt >= hi:
            continue
        for g in list(v.groups):
            arm_obj.vertex_groups[g.group].remove([v.index])
        a = max(0.0, min(1.0, (tt - lo) / (hi - lo)))
        body.add([v.index], 1.0 - a, "REPLACE")
        if a > 0.0:
            upper.add([v.index], a, "REPLACE")


def wing_keep(name):
    return name.startswith("wing") or name in PLAN.WING_BODY


def bind_wing(obj, arm):
    """Binds one wing object: heat weights, then (by name) the arm's chest-rooted stub
    ("wingarm_<side>") or strut weights for everything else (membranes, feathers, fins).
    Name every wing object "<something>_<L|R>..."; the side is the letter after the first "_"."""
    bind(obj, arm, wing_keep)
    side = obj.name.split("_")[1][0]
    if obj.name.startswith("wingarm"):
        weight_wingarm(obj, side)
    else:
        weight_membrane(obj, side)


# ------------------------------------------------------------------------------ armature
def extra_points():
    """Bones that are not body nodes, each parallel to a body bone, as long, parented to it
    and growing like it: the jaw (from the hinge behind the mouth corners, like the snout)
    and the eyes (from between the eyes, like the head; the runtime squashes it to blink)."""
    head, muzzle, tip = node("head"), node("muzzle"), node("snout")
    hinge = V(F["jaw_hinge"])
    eyes = V((0.0, F["eyes"]["at"][1], F["eyes"]["at"][2]))
    return {"jaw_hinge": hinge, "jaw_tip": hinge + (tip - muzzle), "eyes_c": eyes, "eyes_tip": eyes + (muzzle - head)}


def build_armature():
    arm_data = bpy.data.armatures.new("rig")
    arm = link(bpy.data.objects.new("rig", arm_data))
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.mode_set(mode="EDIT")
    eb = {}
    nodes = F["nodes"]

    def add(name, head, tail, parent):
        b = arm_data.edit_bones.new(name)
        b.head, b.tail = head, tail
        b.roll = 0
        if parent:
            b.parent = eb[parent]
            b.use_connect = (b.head - eb[parent].tail).length < 1e-4
        b.inherit_scale = "NONE"
        eb[name] = b

    extra = extra_points()
    for name, h, t, parent in BONES:
        add(name, V(extra[h] if h in extra else nodes[h][0]), V(extra[t] if t in extra else nodes[t][0]), parent)
    for side in ("L", "R"):
        w = wing_points(side)
        for name, h, t, parent in WING_CHAIN:
            add(f"{name}_{side}", w[h], w[t], f"{parent}_{side}" if parent.startswith("wing") else parent)
    bpy.ops.object.mode_set(mode="OBJECT")
    return arm


def body_keep(name):
    return not name.startswith("wing") and name not in ("jaw", "eyes")


def bind(mesh_obj, arm, keep):
    """Automatic (heat) weights from the bones this draw may use, at most 2 per vertex."""
    saved = {b.name: b.use_deform for b in arm.data.bones}
    for b in arm.data.bones:
        b.use_deform = keep(b.name)
    bpy.ops.object.select_all(action="DESELECT")
    mesh_obj.select_set(True)
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.parent_set(type="ARMATURE_AUTO")
    bpy.ops.object.select_all(action="DESELECT")
    for b in arm.data.bones:
        b.use_deform = saved[b.name]
    for vg in list(mesh_obj.vertex_groups):
        if not keep(vg.name):
            mesh_obj.vertex_groups.remove(vg)
    bpy.context.view_layer.objects.active = mesh_obj
    mesh_obj.select_set(True)
    bpy.ops.object.mode_set(mode="WEIGHT_PAINT")
    bpy.ops.object.vertex_group_normalize_all(lock_active=False)
    bpy.ops.object.vertex_group_limit_total(group_select_mode="ALL", limit=2)  # the shader blends 2 bones
    bpy.ops.object.vertex_group_normalize_all(lock_active=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    mesh_obj.select_set(False)
    unweighted = [v.index for v in mesh_obj.data.vertices if not any(g.weight > 0 for g in v.groups)]
    if unweighted:  # heat weighting can miss a few thin bits: give them their nearest bone
        nearest_bone_weights(mesh_obj, arm, keep, unweighted)
    unweighted = sum(1 for v in mesh_obj.data.vertices if not any(g.weight > 0 for g in v.groups))
    assert unweighted == 0, f"{mesh_obj.name}: {unweighted} vertices without bone weights"


def nearest_bone_weights(mesh_obj, arm, keep, indices):
    """Weight these vertices fully to the nearest allowed bone (by segment distance)."""
    segs = [(b.name, arm.matrix_world @ b.head_local, arm.matrix_world @ b.tail_local)
            for b in arm.data.bones if keep(b.name)]

    def gap(p, a, b):
        ab = b - a
        t = max(0.0, min(1.0, (p - a).dot(ab) / max(ab.length_squared, 1e-9)))
        return (p - (a + ab * t)).length

    for i in indices:
        p = mesh_obj.matrix_world @ mesh_obj.data.vertices[i].co
        name = min(segs, key=lambda s: gap(p, s[1], s[2]))[0]
        vg = mesh_obj.vertex_groups.get(name) or mesh_obj.vertex_groups.new(name=name)
        vg.add([i], 1.0, "REPLACE")


def parent_to_bone(obj, arm, bone):
    """Attach a rigid part at the bone's JOINT (pose matrix origin), like the runtime does."""
    bpy.context.view_layer.update()
    c = obj.constraints.new("CHILD_OF")
    c.target = arm
    c.subtarget = bone
    c.inverse_matrix = (arm.matrix_world @ arm.pose.bones[bone].matrix).inverted()
    obj["base_loc"] = list(obj.location)


# ------------------------------------------------------------------------------ part shapes
def horn_point(length, curve, t):
    """horn_mesh's centre line at t (0 base .. 1 tip), in its local frame."""
    ang = curve * t
    return Vector((0, math.sin(ang) * length * t * 0.9, math.cos(ang) * length * t))


def horn_mesh(name, length, radius, curve, segments=6, ring=5, faceted=False, taper=0.9, tip=0.06):
    """A tapered horn swept backward along a curve (local +Y back, +Z up). Faceted: flat
    shading (crystals)."""
    bm = bmesh.new()
    rings = []
    for i in range(segments + 1):
        t = i / segments
        ang = curve * t
        c = Vector((0, math.sin(ang) * length * t * 0.9, math.cos(ang) * length * t))
        r = radius * (1 - t) ** taper + radius * tip
        rings.append([bm.verts.new(c + Vector((math.cos(a) * r, math.sin(a) * r * 0.8, 0)))
                      for a in (2 * math.pi * k / ring for k in range(ring))])
    for a, b in zip(rings, rings[1:]):
        for k in range(ring):
            bm.faces.new((a[k], a[(k + 1) % ring], b[(k + 1) % ring], b[k]))
    bm.faces.new(list(reversed(rings[0])))
    obj = mesh_object(name, bm)
    if not faceted:
        smooth(obj)
    return obj


def flat_fan(name, pts, thickness=0.015):
    """A flat fan from pts[0] (two sheets, no rim); the origin sits at pts[0]."""
    origin = Vector(pts[0])
    bm = bmesh.new()
    vs = [bm.verts.new(Vector(p) - origin) for p in pts]
    for i in range(1, len(vs) - 1):
        bm.faces.new((vs[0], vs[i], vs[i + 1]))
    obj = mesh_object(name, bm, origin)
    m = obj.modifiers.new("thick", "SOLIDIFY")
    m.thickness = thickness
    m.offset = 0
    m.use_rim = False
    apply_modifiers(obj)
    return obj


def blade(name, base, direction, side, length, width, thickness=0.01):
    """A feather / leaf blade from base along direction: widest a third of the way out, with
    a rounded tip."""
    d = Vector(direction).normalized()
    s = (Vector(side) - d * Vector(side).dot(d)).normalized()
    b = Vector(base)
    if LOD:
        return flat_fan(name, [b, b + d * length * 0.35 + s * width * 0.5, b + d * length,
                               b + d * length * 0.35 - s * width * 0.5], thickness)
    return flat_fan(name, [b, b + d * length * 0.3 + s * width * 0.5, b + d * length * 0.75 + s * width * 0.38,
                           b + d * length * 0.95 + s * width * 0.14, b + d * length,
                           b + d * length * 0.95 - s * width * 0.14, b + d * length * 0.75 - s * width * 0.38,
                           b + d * length * 0.3 - s * width * 0.5], thickness)


def lobed_fin(name, base, out, up, radius, a0, a1, lobes, thickness, n=None):
    """A rounded fin fanning from base between angles a0..a1 (degrees, in the out/up plane)
    with a gently lobed edge."""
    base, out, up = Vector(base), Vector(out).normalized(), Vector(up).normalized()
    n = n or lod(10, 5)
    pts = [base]
    for j in range(n + 1):
        f = j / n
        a = math.radians(a0 + (a1 - a0) * f)
        r = radius * (1 - 0.14 * abs(math.sin(math.pi * lobes * f))) * (0.75 + 0.25 * math.sin(math.pi * f))
        pts.append(base + (out * math.cos(a) + up * math.sin(a)) * r)
    return flat_fan(name, pts, thickness)


def blob(name, center, radii, segments=None, rings=None):
    """A smooth ellipsoid (a tuft, a plate, a pom-pom, a club): radii (rx, ry, rz)."""
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segments or lod(10, 6), v_segments=rings or lod(6, 4), radius=1.0)
    for v in bm.verts:
        v.co = Vector((v.co.x * radii[0], v.co.y * radii[1], v.co.z * radii[2]))
    obj = mesh_object(name, bm, center)
    smooth(obj)
    return obj


def tube(name, points, radii, ring=None, cap=True):
    """A smooth tube along points with a radius at each (antennae, whiskers, tail ribbons,
    antler beams). Its origin is the first point."""
    ring = ring or lod(6, 4)
    pts = [Vector(p) for p in points]
    origin = pts[0]
    bm = bmesh.new()
    rings = []
    for i, (p, r) in enumerate(zip(pts, radii)):
        d = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        a = Vector((0, 0, 1)) if abs(d.z) < 0.9 else Vector((1, 0, 0))
        s = d.cross(a).normalized()
        u = s.cross(d).normalized()
        rings.append([bm.verts.new(p - origin + (s * math.cos(t) + u * math.sin(t)) * r)
                      for t in (2 * math.pi * k / ring for k in range(ring))])
    for a, b in zip(rings, rings[1:]):
        for k in range(ring):
            bm.faces.new((a[k], a[(k + 1) % ring], b[(k + 1) % ring], b[k]))
    if cap:
        bm.faces.new(list(reversed(rings[0])))
        bm.faces.new(rings[-1])
    obj = mesh_object(name, bm, origin)
    smooth(obj)
    return obj


def add_dome(bm, center, out, up, rx, ry, depth, seg, rings, material):
    """Front hemisphere (the back is buried), faces pointing along `out`."""
    out = Vector(out).normalized()
    right = Vector(up).cross(out).normalized()
    up = out.cross(right).normalized()
    center = Vector(center)
    layers = []
    for k in range(rings):
        phi = 0.5 * math.pi * k / rings
        layers.append([bm.verts.new(center + right * math.cos(a) * rx * math.cos(phi) +
                                    up * math.sin(a) * ry * math.cos(phi) + out * depth * math.sin(phi))
                       for a in (2 * math.pi * j / seg for j in range(seg))])
    pole = bm.verts.new(center + out * depth)
    faces = []
    for a, b in zip(layers, layers[1:]):
        faces += [bm.faces.new((a[j], a[(j + 1) % seg], b[(j + 1) % seg], b[j])) for j in range(seg)]
    faces += [bm.faces.new((layers[-1][j], layers[-1][(j + 1) % seg], pole)) for j in range(seg)]
    for f in faces:
        f.material_index = material
        f.normal_update()
        if f.normal.dot(f.calc_center_median() - center) < 0:
            f.normal_flip()


def head_point(off, s=1):
    """A point in the form's head frame: head.origin + mirrored offset * head.k."""
    h = F["head"]
    return V(h["origin"]) + mirror(off, s) * h["k"]


# ------------------------------------------------------------------------------ eyes & heart
def build_eyes(mats, pupil="round"):
    """Big glossy eyes: an iris dome, a pupil dome on it and glints, one object per eye
    (origin at the iris centre). pupil "round" (happy, calm) or "slit" (startled, cross):
    the game swaps them with the mood (D77). The form's "eyes" dict: at, out, iris
    (rx, ry, depth), pupil (rx, ry, depth), slit (x, y scale of the slit pupil), glints
    [(x, y, r)], seg (iris seg, rings, pupil seg, rings)."""
    e = F["eyes"]
    iseg, irings, pseg, prings = lod(e["seg"], (8, 1, 6, 1))
    irx, iry, idepth = e["iris"]
    prx, pry, pdepth = e["pupil"]
    if pupil == "slit":
        sx, sy = e.get("slit", (0.28, 1.12))
        prx, pry = prx * sx, pry * sy
    poff = idepth * math.sqrt(max(0.0, 1 - (prx / irx) ** 2)) - 0.1 * pdepth
    eyes = []
    for s in (-1, 1):
        at, out = mirror(e["at"], s), mirror(e["out"], s).normalized()
        right = Vector((0, 0, 1)).cross(out).normalized()
        up = out.cross(right).normalized()
        bm = bmesh.new()
        add_dome(bm, (0, 0, 0), out, up, irx, iry, idepth, iseg, irings, 0)
        add_dome(bm, out * poff, out, up, prx, pry, pdepth, pseg, prings, 1)
        for gx, gy, gr in lod(e["glints"], e["glints"][:1]):
            gx *= -s
            h = poff + pdepth * math.sqrt(max(0.0, 1 - min(1.0, (gx / max(prx, 1e-4)) ** 2 + (gy / pry) ** 2)))
            add_dome(bm, out * (h + 0.003) + right * gx + up * gy, out, up, gr, gr, gr * 0.3, lod(8, 4), 1, 2)
        obj = mesh_object(f"eye_{pupil}_{s}", bm, at)
        for m in ("iris", "pupil", "glint"):
            obj.data.materials.append(mats[m])
        smooth(obj)
        eyes.append(obj)
    return eyes


def build_heart(mats):
    """The heartglow: a flat heart emblem on the chest (emissive, white-hot core)."""
    c = V(F["heart"]["at"])
    size = F["heart"]["size"]
    pts = [c]
    steps = lod(16, 8)
    for j in range(steps + 1):
        t = 2 * math.pi * j / steps
        x = 16 * math.sin(t) ** 3
        zz = 13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t)
        pts.append(c + Vector((x, 0, zz + 2)) * (size / 17))
    obj = flat_fan("heart", pts, 0.08 * size)  # (thin: it lies on the chest now, bent to it)
    if F["heart"].get("tilt"):
        obj.rotation_euler.x = math.radians(F["heart"]["tilt"])
    obj.data.materials.append(mats["heart"])
    return [obj]


# ------------------------------------------------------------------------------ part library
HORN_KINDS = {  # offsets in the head frame; rot = (x, y mirrored, z) degrees
    "swept": [dict(len=0.62, r=0.085, curve=55, seg=5, ring=5, at=(0.13, 0.10, 0.16), rot=(-18, 12, 0)),
              dict(len=0.28, r=0.042, curve=70, seg=4, ring=4, at=(0.22, 0.02, 0.02), rot=(-40, 55, 0), minor=True)],
    "nubs": [dict(len=0.22, r=0.075, curve=35, seg=4, ring=5, at=(0.12, 0.08, 0.17), rot=(-20, 15, 0))],
    "crown": [dict(len=0.27, r=0.045, curve=8, seg=3, ring=4, at=(0.0, 0.07, 0.25), rot=(-6, 0, 0), centre=True),
              dict(len=0.23, r=0.04, curve=10, seg=3, ring=4, at=(0.09, 0.06, 0.23), rot=(-8, 20, 0)),
              dict(len=0.18, r=0.036, curve=12, seg=3, ring=4, at=(0.16, 0.02, 0.18), rot=(-10, 42, 0), minor=True)],
    "crystal": [dict(len=0.5, r=0.085, curve=14, seg=2, ring=4, at=(0.12, 0.08, 0.17), rot=(-24, 10, 0), faceted=True),
                dict(len=0.24, r=0.05, curve=6, seg=2, ring=4, at=(0.19, 0.03, 0.09), rot=(-46, 42, 0), faceted=True,
                     minor=True)],
    "antler": [dict(len=0.56, r=0.055, curve=48, seg=5, ring=4, at=(0.12, 0.08, 0.17), rot=(-26, 22, 0)),
               dict(len=0.2, r=0.03, curve=20, seg=2, ring=4, on=(0, 0.45), rot=(10, 34, 0), minor=True),
               dict(len=0.16, r=0.026, curve=25, seg=2, ring=4, on=(0, 0.75), rot=(-60, 46, 0), minor=True)],
}


def build_horns(specs, mats, material="horn", babies_minor=True):
    """Horns from a list of specs (or a HORN_KINDS name): each dict(len, r, curve, seg, ring,
    at (head frame), rot (x, y mirrored, z) degrees, [faceted], [minor: not on babies],
    [centre: one piece on the midline], [on=(beam index, t): a tine along a beam])."""
    if isinstance(specs, str):
        specs = HORN_KINDS[specs]
    h = F["head"]
    horns = []
    for s in (-1, 1):
        beams = []
        for spec in specs:
            if (h.get("buds") and babies_minor and spec.get("minor")) or (spec.get("centre") and s < 0):
                beams.append(None)
                continue
            seg = max(2 if spec.get("faceted") or spec.get("on") else 3, spec["seg"] - (2 if h.get("buds") else 0))
            length = spec["len"] * h["k"] * h.get("horn_len", 1.0)
            curve = math.radians(spec["curve"]) * h.get("horn_curve", 1.0)
            o = horn_mesh(f"horn_{s}", length, spec["r"] * h["k"] * h.get("horn_r", 1.0), curve,
                          lod(seg, max(2, seg - 3)), lod(spec["ring"], 4), faceted=spec.get("faceted", False))
            rx, ry, rz = spec["rot"]
            if "on" in spec:
                beam, t = spec["on"]
                b = beams[beam]
                if b is None:
                    bpy.data.objects.remove(o, do_unlink=True)
                    beams.append(None)
                    continue
                bo, blen, bcurve = b
                o.location = bo.location + bo.rotation_euler.to_matrix() @ horn_point(blen, bcurve, t)
            else:
                o.location = head_point(spec["at"], s)
            o.rotation_euler = (math.radians(rx), s * math.radians(ry), math.radians(rz))
            o.data.materials.append(mats[spec.get("material", material)])
            horns.append(o)
            beams.append((o, length, curve))
    return horns


def spine_up(node_name, nxt):
    """Direction perpendicular to the spine at a node, pointing up/back."""
    d = (node(nxt) - node(node_name)).normalized()
    n = Vector((0, -d.z, d.y))
    return n if n.z + 0.3 * n.y > 0 else -n


def build_ridge(kind, mats, path=None, size=None, last="tail_tip", material=None):
    """A dorsal ridge along the spine: [(object, bone)], each piece rigid on one spine bone.
    path: [(node, top)] (top: how far above the node the skin is), defaults to the form's
    "ridge"; kinds: "spikes" (horn-like), "fin" (a membrane sail), "feather" (plumes),
    "plates" (rounded overlapping plates), "tufts" (soft fur tufts)."""
    r = F.get("ridge", {})
    path = path or r["path"]
    s0, s1, top_ref = size or r["size"]
    pieces = []
    for i, (nd, top) in enumerate(path):
        nxt = path[i + 1][0] if i + 1 < len(path) else last
        up = spine_up(nd, nxt)
        c = node(nd)
        base = c + up * top * 0.92
        sz = s0 + s1 * min(1.0, top / top_ref)
        bone = r.get("bones", {}).get(nd, nd)
        if kind == "spikes":
            lf, rf, curve = r.get("spike", (1.9, 0.7, 40))
            o = horn_mesh(f"spike_{i}", sz * lf, sz * rf, math.radians(curve), lod(3, 2), lod(4, 3))
            o.location = base
            o.rotation_euler = (-math.atan2(up.y, up.z) - math.radians(10), 0, 0)
            o.data.materials.append(mats[material or "horn"])
            pieces.append((o, bone))
        elif kind == "fin":
            h0f, h1f, thick = r.get("fin", (2.2, 1.9, 0.02))
            ntop = path[i + 1][1] if i + 1 < len(path) else top * 0.5
            nbase = node(nxt) + up * ntop * 0.92
            nsize = s0 + s1 * min(1.0, ntop / top_ref)
            h0, h1 = sz * h0f, nsize * h1f
            end = base.lerp(nbase, 1.12)
            pts = [base, base + up * h0, base.lerp(end, 0.5) + up * (h0 + h1) * 0.55, end + up * h1, end]
            o = flat_fan(f"finridge_{i}", pts, thick)
            o.data.materials.append(mats[material or "membrane"])
            pieces.append((o, bone))
        elif kind == "plates":
            back = (node(nxt) - c).normalized()
            wf, lf, hf = r.get("plate", (1.6, 2.0, 0.5))
            o = blob(f"plate_{i}", base, (sz * wf, sz * lf, sz * hf), lod(8, 6), lod(5, 3))
            o.rotation_euler = (-math.atan2(back.z, back.y) * 0.6, 0, 0)
            o.data.materials.append(mats[material or "horn"])
            pieces.append((o, bone))
        elif kind == "tufts":
            back = (node(nxt) - c).normalized()
            d = (back * 0.6 + up * 0.8).normalized()
            lf, wf, thick = r.get("tuft", (2.2, 1.4, 0.01))
            for j, xo in enumerate((-1, 0, 1) if not LOD else (0,)):
                o = blade(f"tuft_{i}_{j}", base + Vector((xo * sz * 0.4, 0, 0)), d + Vector((xo * 0.35, 0, 0)),
                          Vector((1, 0, 0)), sz * lf * (0.8 if xo else 1.0), sz * wf, thick)
                o.data.materials.append(mats[material or "accent_flat"])
                pieces.append((o, bone))
        else:  # feather plumes
            lf, wf, thick = r.get("plume", (3.2, 1.1, 0.012))
            back = (node(nxt) - c).normalized()
            d = (back * 0.8 + up * 0.6).normalized()
            for j, xo in enumerate((-1, 1) if nd.startswith("neck") else (0,)):
                o = blade(f"plume_{i}_{j}", base + Vector((xo * sz * 0.35, 0, 0)),
                          d + Vector((xo * 0.25, 0, 0)), Vector((1, 0, 0)), sz * lf, sz * wf, thick)
                o.data.materials.append(mats[material or "accent_flat"])
                pieces.append((o, bone))
    return pieces


RUNE_GLYPHS = [
    [((0.2, 0.0), (0.2, 1.0)), ((0.2, 1.0), (0.75, 0.78)), ((0.75, 0.78), (0.2, 0.55)), ((0.2, 0.55), (0.8, 0.0))],
    [((0.5, 0.0), (0.5, 1.0)), ((0.5, 0.55), (0.1, 1.0)), ((0.5, 0.55), (0.9, 1.0))],
    [((0.5, 1.0), (0.9, 0.62)), ((0.9, 0.62), (0.5, 0.25)), ((0.5, 0.25), (0.1, 0.62)), ((0.1, 0.62), (0.5, 1.0)),
     ((0.5, 0.25), (0.15, 0.0)), ((0.5, 0.25), (0.85, 0.0))],
    [((0.25, 0.0), (0.25, 1.0)), ((0.25, 0.75), (0.75, 0.5)), ((0.75, 0.5), (0.25, 0.25))],
]


def glow_marks(mats, marks, material="rune", glyphs=None):
    """Small glowing marks lying on the skin (runes, stars, snowflakes: any glyph of strokes in
    a unit box). marks: [(bone, point on the right side, glyph index, size)], mirrored to the
    left. [(object, bone)]; attach them to group "runes" (seated on the skin like the heart)."""
    glyphs = glyphs or RUNE_GLYPHS
    pieces = []
    for i, (bone, at, glyph, size) in enumerate(marks):
        for s in (-1, 1):
            c = mirror(at, s)
            joint = node(bone) if bone in F["nodes"] else c - Vector((0, 0, 0.1))
            out = (c - joint).normalized()
            right = Vector((0, 0, 1)).cross(out)
            right = (right if right.length > 1e-4 else Vector((0, 1, 0))).normalized()
            up = out.cross(right).normalized()
            bm = bmesh.new()
            w = 0.12
            for (x0, y0), (x1, y1) in glyphs[glyph % len(glyphs)]:
                a = right * ((x0 - 0.5) * size * s) + up * ((y0 - 0.5) * size)
                b = right * ((x1 - 0.5) * size * s) + up * ((y1 - 0.5) * size)
                along = (b - a).normalized()
                side = out.cross(along).normalized() * (w * size * 0.5)
                a -= along * (w * size * 0.4)
                b += along * (w * size * 0.4)
                vs = [bm.verts.new(p) for p in (a - side, b - side, b + side, a + side)]
                f = bm.faces.new(vs)
                f.normal_update()
                if f.normal.dot(out) < 0:
                    f.normal_flip()
            obj = mesh_object(f"mark_{i}_{s}", bm, c)
            obj.data.materials.append(mats[material])
            pieces.append((obj, bone))
    return pieces


def build_frill(kind, mats):
    """Head frills from the classic library: "fin" (ear and cheek fins), "leaf" (leaf ears),
    "feather" (a crest of plumes and cheek feathers). Uses the form's head frame (frill_k,
    feather_w)."""
    h = F["head"]
    k, wk = h.get("frill_k", 1.0), h.get("feather_w", 1.0)
    parts = []

    def hp(off, s=1):
        return head_point((off[0] * k + 0.20 * (1 - k), off[1] * k, off[2] * k), s)

    if kind == "fin":
        for s in (-1, 1):
            out = Vector((s * 0.5, 0.86, 0.0))
            parts.append(lobed_fin(f"fin_{s}", hp((0.20, 0.06, 0.0), s), out, (0, 0, 1),
                                   0.56 * k * h["k"], 70, -45, 3, 0.016 * h["k"]))
            parts.append(lobed_fin(f"cheekfin_{s}", hp((0.18, -0.08, -0.14), s), Vector((s * 0.6, 0.8, 0.0)),
                                   (0, 0, 1), 0.3 * k * h["k"], 10, -60, 2, 0.014 * h["k"]))
    elif kind == "leaf":
        for s in (-1, 1):
            for j, (off, d, length, width) in enumerate((((0.19, 0.05, 0.02), (s * 0.55, 0.78, 0.30), 0.62, 0.30),
                                                           ((0.18, -0.06, -0.13), (s * 0.7, 0.66, -0.1), 0.34, 0.2))):
                parts.append(blade(f"leaf_{s}_{j}", hp(off, s), Vector(d), Vector((0, -0.25, 1)),
                                   length * k * h["k"], width * k * h["k"]))
    elif kind == "feather":
        for x, length, spread in ((0.0, 0.80, 0.0), (0.07, 0.66, 0.28), (-0.07, 0.66, -0.28)):
            base = head_point((x, 0.04, 0.22))
            d = Vector((spread, 0.82, 0.55))
            parts.append(blade(f"crest_{x:+.2f}", base, d, Vector((1, 0, -0.3)), length * k * h["k"],
                               0.16 * wk * k * h["k"]))
        for s in (-1, 1):
            for j, (dz, length) in enumerate(((0.02, 0.46), (-0.12, 0.36))):
                base = head_point((0.18, 0.02, dz), s)
                d = Vector((s * 0.45, 0.88, 0.12 - j * 0.1))
                parts.append(blade(f"cheekfeather_{s}_{j}", base, d, Vector((0, -0.2, 1)), length * k * h["k"],
                                   0.12 * wk * k * h["k"]))
    for p in parts:
        p.data.materials.append(mats["accent_flat"])
    return parts


def build_tail_tip(kind, mats, at_node="tail_tip"):
    """Classic tail tips at the tail's last node: "spade", "fan" (a membrane fan), "tuft" (a
    plume of feathers). Scaled by the form's tail_k."""
    x, y, z = F["nodes"][at_node][0]
    k = F["tail_k"]
    if kind == "spade":
        obj = flat_fan("tail_tip", [Vector((0, y - 0.05 * k, z)), Vector((-0.24 * k, y + 0.14 * k, z)),
                                    Vector((0, y + 0.5 * k, z)), Vector((0.24 * k, y + 0.14 * k, z))], 0.04 * k)
        obj.data.materials.append(mats["horn"])
    elif kind == "fan":
        pts = [Vector((0, y - 0.05 * k, z))]
        rays = lod(7, 4)
        for j in range(rays):
            a = math.radians(-65 + j * 130 / (rays - 1))
            pts.append(Vector((math.sin(a) * 0.40 * k, y + math.cos(a) * 0.46 * k, z + 0.02 * k)))
        obj = flat_fan("tail_tip", pts, 0.02 * k)
        obj.data.materials.append(mats["membrane"])
    else:
        blades = []
        base = Vector((0, y - 0.03 * k, z))
        for j, (ax, az) in enumerate(lod(((0, 0), (-32, 8), (32, 8), (-16, -14), (16, -14)),
                                         ((0, 0), (-28, 4), (28, 4)))):
            d = Vector((math.sin(math.radians(ax)), math.cos(math.radians(ax)), math.sin(math.radians(az))))
            side = Vector((math.cos(math.radians(ax)), -math.sin(math.radians(ax)), 0.3))
            blades.append(blade(f"tuft_{j}", base, d, side, (0.5 if j == 0 else 0.42) * k, 0.16 * k, 0.012 * k))
        obj = join(blades, "tail_tip")
        obj.data.materials.clear()
        obj.data.materials.append(mats["accent_flat"])
    return obj


# ------------------------------------------------------------------------------ mouth
def mouth_plane():
    """The plane through the mouth line (its front and both corners): a point on it, its up
    normal, and the corners' y (the slit runs in front of them)."""
    m = F["face"]["mouth"]
    a, b, c = Vector(m(1, 0.0)), Vector(m(1, 1.0)), Vector(m(-1, 1.0))
    n = (b - a).cross(c - a).normalized()
    return a, (n if n.z > 0 else -n), b.y


def cut_mouth(body):
    """Slit the snout along the mouth line so the jaw can open."""
    co, no, y_corner = mouth_plane()
    md = F["mouth_detail"]
    bm = bmesh.new()
    bm.from_mesh(body.data)
    faces = []
    for f in bm.faces:
        c = f.calc_center_median()
        if c.y < y_corner and abs(c.x) < md["width"] and abs((c - co).dot(no)) < md["depth"]:
            faces.append(f)
    geom = faces + list({e for f in faces for e in f.edges}) + list({v for f in faces for v in f.verts})
    cut = bmesh.ops.bisect_plane(bm, geom=geom, plane_co=co, plane_no=no, dist=1e-6)["geom_cut"]
    bmesh.ops.split_edges(bm, edges=[e for e in cut if isinstance(e, bmesh.types.BMEdge)])
    bm.to_mesh(body.data)
    bm.free()


def lip_chains(body):
    """The slit's two lips, each ordered corner -> front -> corner (from -x): (upper, lower)
    indices, sharing the two end vertices where the slit stops.

    Walked along the slit's own open edges. The cut takes whole faces whose centre is in front
    of the corners and within the width, so the slit runs a little past y_corner, or stops at
    the width short of it; lips picked by position (on the plane, in front of the corners,
    within the width, until run 18) missed those ends, and the missing bit of slit opened
    straight into the head when the jaw dropped: see-through at the mouth's edges in the
    close-up (a petted Blazeplume). The pocket, its walls and the mouth line follow these."""
    co, no, _ = mouth_plane()
    bm = bmesh.new()
    bm.from_mesh(body.data)
    bm.verts.ensure_lookup_table()
    chains = []
    for sign in (1, -1):  # the lip above the plane, then the one below
        adj = {}
        for e in bm.edges:
            if len(e.link_faces) != 1 or any(abs((v.co - co).dot(no)) > 1e-4 for v in e.verts):
                continue
            if (e.link_faces[0].calc_center_median() - co).dot(no) * sign <= 0:
                continue
            a, b = (v.index for v in e.verts)
            adj.setdefault(a, []).append(b)
            adj.setdefault(b, []).append(a)
        best, seen = [], set()
        for start in sorted((v for v, n in adj.items() if len(n) == 1), key=lambda i: bm.verts[i].co.x):
            if start in seen:
                continue
            chain = [start]
            while True:
                nxt = [n for n in adj[chain[-1]] if n not in chain]
                if not nxt:
                    break
                chain.append(nxt[0])
            seen.update(chain)
            if len(chain) > len(best):  # the slit (a stray open edge elsewhere is shorter)
                best = chain
        if bm.verts[best[0]].co.x > bm.verts[best[-1]].co.x:
            best.reverse()
        chains.append(best)
    bm.free()
    upper, lower = chains
    assert upper and lower, f"{KIND.META['name']}: no mouth slit"
    if {upper[0], upper[-1]} != {lower[0], lower[-1]}:
        print(f"[kit] {KIND.META['name']} {F is KIND.FORMS['grown'] and 'grown' or 'hatchling'}: "
              f"the lips don't meet at the slit's ends")
    return upper, lower


def chain_point(pts, f):
    """The point a fraction f of the way along a polyline (by length)."""
    lengths = [0.0]
    for a, b in zip(pts, pts[1:]):
        lengths.append(lengths[-1] + (b - a).length)
    t = f * lengths[-1]
    for k in range(len(pts) - 1):
        if lengths[k + 1] >= t:
            u = (t - lengths[k]) / max(1e-6, lengths[k + 1] - lengths[k])
            return pts[k].lerp(pts[k + 1], u)
    return pts[-1].copy()


def mouth_back():
    co, no, y_corner = mouth_plane()
    return Vector((0, y_corner, co.z - no.y * (y_corner - co.y) / no.z))


def weight_jaw(body):
    """The lower lip and chin follow the jaw; behind the corners the throat fades back."""
    co, no, y_corner = mouth_plane()
    md = F["mouth_detail"]
    me = body.data
    jaw = body.vertex_groups.get("jaw") or body.vertex_groups.new(name="jaw")
    below = {}
    for p in me.polygons:
        s = (Vector(p.center) - co).dot(no)
        for v in p.vertices:
            below.setdefault(v, []).append((s, p.material_index))
    for v in me.vertices:
        info = below.get(v.index, [])
        if not info or any(m == 1 for _, m in info):
            continue
        s = (v.co - co).dot(no)
        if abs(s) < 1e-4:
            s = sum(x for x, _ in info) / len(info)
        if s >= 0 or s < -md["depth"] or abs(v.co.x) > md["width"] or v.co.y > y_corner + md["fade"]:
            continue
        w = 1.0 if v.co.y <= y_corner else 1.0 - (v.co.y - y_corner) / md["fade"]
        old = sorted(((g.group, g.weight) for g in v.groups if g.group != jaw.index), key=lambda g: -g[1])
        for gi, _ in old:
            body.vertex_groups[gi].remove([v.index])
        if old and w < 1.0:
            body.vertex_groups[old[0][0]].add([v.index], 1.0 - w, "REPLACE")
        jaw.add([v.index], w, "REPLACE")


def build_mouth_pocket(body, upper, lower):
    """The inside of the mouth (dark): a roof and a floor running from the lips back to a line
    across the back of the mouth as wide as the lips (a little behind the corners), and a
    wall at each corner, so an open mouth shows its dark inside everywhere and no way through
    to the inside of the head. (It fanned to one point at the back until run 15: the sides of
    a wide mouth showed the head's inside, which the game culls, as a hole.)"""
    co, no, y_corner = mouth_plane()
    md = F["mouth_detail"]
    me = body.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.verts.ensure_lookup_table()
    deform = bm.verts.layers.deform.verify()
    head, jaw = body.vertex_groups["head"].index, body.vertex_groups["jaw"].index
    y_ends = max(bm.verts[i].co.y for i in upper + lower)  # the slit can end a little past the corners
    y_back = max(y_corner, y_ends) + md.get("pocket", 0.35) * md["width"]

    def on_plane(x, y):
        return Vector((x, y, co.z - (no.x * (x - co.x) + no.y * (y - co.y)) / no.z))

    def target(src):  # its point on the back line, halfway between the head and the jaw
        v = bm.verts.new(on_plane(src.co.x * 0.95, y_back))
        v[deform][head] = v[deform][jaw] = 0.5
        return v

    def strip(lip, facing):
        rim = []
        for src in lip:
            v = bm.verts.new(src.co)
            for k, x in src[deform].items():
                v[deform][k] = x
            rim.append(v)
        backs = [target(src) for src in lip]
        for (a, b), (ta, tb) in zip(zip(rim, rim[1:]), zip(backs, backs[1:])):
            f = bm.faces.new((a, b, tb, ta))
            f.normal_update()
            if f.normal.dot(facing) < 0:
                f.normal_flip()
            f.material_index = 2
        return rim, backs

    def cheek(up, lo, end):
        """The side of the mouth from the corner forward along the lips (their outer 45% across):
        a dark wall from the upper lip down to the lower one, facing in, so a mouth opened wide
        and seen from the side shows its far side instead of a way through (run 17: the
        Blazeplume's yawn). Folded flat while the mouth is shut; seen from outside, culled."""
        n_up, n_lo = len(up), len(lo)
        if n_up < 2 or n_lo < 2:
            return
        order = range(n_up) if end == 0 else range(n_up - 1, -1, -1)
        side = 1.0 if up[0 if end == 0 else -1].co.x > 0 else -1.0
        widest = max(abs(v.co.x) for v in up)
        pairs = []
        for i in order:
            a = up[i]
            if abs(a.co.x) < 0.55 * widest and pairs:
                break
            j = round(i * (n_lo - 1) / (n_up - 1))
            pairs.append((a, lo[j]))
        facing = Vector((-side, 0, 0))
        for (a, b), (c, e) in zip(pairs, pairs[1:]):
            verts = [a, c, e, b]
            if (a.co - b.co).length < 1e-6:  # the corner: the lips meet
                verts = [a, c, e]
            if len({id(v) for v in verts}) < len(verts):
                continue
            f = bm.faces.new(verts)
            f.normal_update()
            if f.normal.dot(facing) < 0:
                f.normal_flip()
            f.material_index = 2

    upper, lower = [bm.verts[i] for i in upper], [bm.verts[i] for i in lower]  # before any are added
    up_rim, up_back = strip(upper, -no)
    lo_rim, lo_back = strip(lower, no)
    cheek(up_rim, lo_rim, 0)
    cheek(up_rim, lo_rim, -1)
    for u, l, t in ((up_rim[0], lo_rim[0], up_back[0]), (up_rim[-1], lo_rim[-1], up_back[-1])):
        if (u.co - l.co).length < 1e-6 and (u.co - t.co).length < 1e-6:
            continue
        f = bm.faces.new((u, l, t))  # the corner's wall, facing into the mouth
        f.normal_update()
        if f.normal.dot(Vector((-u.co.x, 0, 0))) < 0:
            f.normal_flip()
        f.material_index = 2
    bm.to_mesh(me)
    bm.free()


def build_mouth_parts(body, upper, lower, mats):
    """Little teeth along both lips (two front fangs peek out; the form's "teeth": False
    leaves them out) and a tongue on the floor of the mouth. [(object, bone)]"""
    co, no, _ = mouth_plane()
    md = F["mouth_detail"]
    me = body.data
    back = mouth_back()
    front = Vector(F["face"]["mouth"](1, 0.0))
    along = lambda chain, f: chain_point([me.vertices[i].co for i in chain], f)  # noqa: E731

    def tooth(bm, base, down, r, length):
        inward = back - base
        inward = (inward - no * inward.dot(no)).normalized()
        side = inward.cross(no).normalized()
        ring = [bm.verts.new(base + (inward * math.cos(a) + side * math.sin(a)) * r) for a in (0.0, 2.0944, 4.1888)]
        tip = bm.verts.new(base + inward * r * 0.4 + down * length)
        for k in range(3):
            f = bm.faces.new((ring[k], ring[(k + 1) % 3], tip))
            f.normal_update()
            if f.normal.dot(f.calc_center_median() - base - down * length * 0.3) < 0:
                f.normal_flip()

    out = []
    if F["teeth"]:
        for chain, bone, down, fracs, fangs in (
                (upper, "snout", -no, lod([0.14, 0.25, 0.36, 0.64, 0.75, 0.86], [0.2, 0.8]), md.get("fangs", [0.43, 0.57])),
                (lower, "jaw", no, lod([0.2, 0.32, 0.68, 0.8], [0.3, 0.7]), [])):
            bm = bmesh.new()
            for fr in fracs:
                p = along(chain, fr)
                tooth(bm, p + (back - p).normalized() * md["tooth"][0] * 2.4, down, *md["tooth"])
            for fr in fangs:
                tooth(bm, along(chain, fr) + (back - along(chain, fr)).normalized() * md["fang"][0] * 0.6,
                      down, *md["fang"])
            obj = mesh_object(f"teeth_{bone}", bm)
            obj.data.materials.append(mats["tooth"])
            out.append((obj, bone))
    w, ln, h = md["tongue"]
    bm = bmesh.new()
    geom = bmesh.ops.create_uvsphere(bm, u_segments=lod(6, 5), v_segments=lod(4, 3), radius=1.0)
    fwd = (front - back)
    fwd = (fwd - no * fwd.dot(no)).normalized()
    side = fwd.cross(no).normalized()
    centre = back.lerp(front, 0.33) + no * h * 0.1
    for v in geom["verts"]:
        x, y, z = v.co
        v.co = centre + side * (x * w) + fwd * (y * ln) + no * (z * h)
    obj = mesh_object("tongue", bm)
    obj.data.materials.append(mats["tongue"])
    out.append((obj, "jaw"))
    return out


def add_face_details(body, lip):
    """Nostrils and a mouth line, projected onto the finished body and joined into it."""
    f = F["face"]
    bpy.context.view_layer.update()
    bvh = BVHTree.FromObject(body, bpy.context.evaluated_depsgraph_get())
    bm = bmesh.new()
    bm.from_mesh(body.data)
    rx, ry, depth = f["nostril_r"]
    for s in (-1, 1):
        loc, nrm, _, _ = bvh.find_nearest(mirror(f["nostril"], s))
        hint = Vector((0, 0, 1)) if abs(nrm.z) < 0.8 else Vector((0, -1, 0))
        add_dome(bm, loc - nrm * depth * 0.6, nrm, hint, rx, ry, depth, lod(8, 4), 1, 1)
    r, n = f["mouth_r"], lod(16, 6)
    path = []
    for k in range(n + 1):
        loc = chain_point(lip, k / n)
        _, nrm, _, _ = bvh.find_nearest(loc)
        path.append((loc - nrm * r * 0.35, nrm))
    rings = []
    for k, (p, nrm) in enumerate(path):
        tangent = (path[min(k + 1, n)][0] - path[max(k - 1, 0)][0]).normalized()
        side = tangent.cross(nrm).normalized()
        up = side.cross(tangent).normalized()
        rings.append([bm.verts.new(p + (up * math.cos(a) + side * math.sin(a)) * r) for a in (0.0, 2.0944, 4.1888)])
    for a, b in zip(rings, rings[1:]):
        for j in range(3):
            face = bm.faces.new((a[j], a[(j + 1) % 3], b[(j + 1) % 3], b[j]))
            face.material_index = 1
    for ring, out in ((rings[0], path[0][0] - path[1][0]), (rings[-1], path[-1][0] - path[-2][0])):
        face = bm.faces.new(ring)
        face.material_index = 1
        face.normal_update()
        if face.normal.dot(out) < 0:  # the end caps face out of the tube too (one faced in)
            face.normal_flip()
    bm.normal_update()
    for face in bm.faces:
        if face.material_index == 1 and len(face.verts) == 4:
            mid = sum((v.co for v in face.verts), Vector()) / 4
            k = min(range(len(path)), key=lambda i: (path[i][0] - mid).length)
            if face.normal.dot(mid - path[k][0]) < 0:
                face.normal_flip()
    bm.to_mesh(body.data)
    bm.free()
    smooth(body)


# ------------------------------------------------------------------------------ assembly
def new_dragon(form, variant):
    """The body, rig, mouth and wings of a form, before parts: d (see build_dragon)."""
    use_form(form)
    mats = make_materials(variant)
    body = build_body()
    seat_wings(body)
    body.data.materials.append(mats["body"])
    body.data.materials.append(mats["pupil"])
    body.data.materials.append(mats["mouth"])
    if F["mouth"]:
        cut_mouth(body)
        upper, lower = lip_chains(body)
        add_face_details(body, [body.data.vertices[i].co.copy() for i in upper])
    arm = build_armature()
    bind(body, arm, body_keep)
    mouth_parts = []
    if F["mouth"]:
        weight_jaw(body)
        mouth_parts = build_mouth_parts(body, upper, lower, mats)
        build_mouth_pocket(body, upper, lower)
    tex.tag_regions(body, PLAN.REGION)
    tex.unwrap(body, lod(256, 128))
    groups = {g: [] for g in PART_GROUPS}
    snap = {g: [] for g in F["inset"]}
    d = dict(body=body, arm=arm, wings=[], rare_wings=[], groups=groups, snap=snap, mats=mats, form=form,
             variant=variant, tagged=[], kind=KIND.META["name"])
    for o, bone in mouth_parts:
        attach(d, "mouth", o, bone)
    return d


def build_dragon(form="grown", variant=0, for_export=False):
    """A whole dragon of the current kind: d = dict(body, arm, wings, rare_wings, groups
    {group: [objects shown]}, tagged [(group, variant, [objects])], mats, form, variant).
    Previews show the variant's parts (the rare variant's replace or add to the common ones);
    for_export builds every part of every variant, tagged, and both pupils."""
    d = new_dragon(form, variant)
    mats = d["mats"]
    rare = KIND.META.get("rare_variant", 3)
    # Wings: the kind's own, or the classic builder in the form's style.
    wings = KIND.wings(kit, d, rare=False) if hasattr(KIND, "wings") else build_wings(F["wing"]["style"], mats)
    for wobj in wings:
        bind_wing(wobj, d["arm"])
    d["wings"] = wings
    if hasattr(KIND, "wings") and (for_export or variant == rare):
        rw = KIND.wings(kit, d, rare=True)
        if rw:
            for wobj in rw:
                bind_wing(wobj, d["arm"])
            d["rare_wings"] = rw
            if not for_export:  # the preview shows the rare wings instead
                for o in d["wings"]:
                    o.hide_render = o.hide_viewport = True
    # Eyes (both pupils when exporting), the heart.
    for pupil, v in (("round", 0), ("slit", 1)):
        if pupil == "slit" and not for_export and "--slit" not in argv:
            continue
        eyes = build_eyes(mats, pupil)
        for e in eyes:
            attach(d, "eyes", e, "eyes")
        d["tagged"].append(("eyes", v, eyes))
        if pupil == "slit" and not for_export:  # preview: show the slit instead
            for o in d["groups"]["eyes"][:-2]:
                o.hide_render = o.hide_viewport = True
    heart = build_heart(mats)
    for h in heart:
        attach(d, "heart", h, "chest")
    d["tagged"].append(("heart", 0, heart))
    # The kind's own parts: [(group, variant, [(object, bone)])].
    parts = KIND.parts(kit, d) if hasattr(KIND, "parts") else []
    shown_rare_groups = {g for g, v, _ in parts if v == 1} if variant == rare else set()
    for group, v, pieces in parts:
        assert group in PART_GROUPS, f"unknown part group {group}"
        objs = []
        for o, bone in pieces:
            attach(d, group, o, bone)
            objs.append(o)
        d["tagged"].append((group, v, objs))
        if not for_export:
            hide = (v == 1 and variant != rare) or (v == 0 and group in shown_rare_groups and
                                                     KIND.META.get("rare_replaces", True))
            for o in objs:
                o.hide_render = o.hide_viewport = hide
    d["tagged"].append(("mouth", 0, list(d["groups"]["mouth"])))
    return d


def attach(d, group, obj, bone):
    parent_to_bone(obj, d["arm"], bone)
    d["groups"][group].append(obj)
    if group in d["snap"]:
        d["snap"][group].append((obj, [obj]))


def rest_pose(d, t):
    """The idle pose: the form's Euler table plus the young-pose lift fading out with growth."""
    pb = d["arm"].pose.bones
    rot = dict(F["base_pose"])
    for name, extra in F["young_pose"].items():
        x, y, z = rot.get(name, (0, 0, 0))
        rot[name] = (x + extra * (1 - t), y, z)
    for b in pb:
        b.rotation_mode = "XYZ"
        x, y, z = rot.get(b.name, (0, 0, 0))
        b.rotation_euler = (math.radians(x), math.radians(y), math.radians(z))


def apply_t(d, t, build):
    """Bone and part scales for growth t, then seat the parts on the body surface. A build other
    than neutral seats them for neutral first (the conformed parts take their shape there), then
    moves them for the build (as the game shifts a part per build)."""
    if build != "neutral":
        _scale_for(d, t, "neutral")
        snap_parts(d)
        _scale_for(d, t, build)
        snap_parts(d, conform=False)
        return
    _scale_for(d, t, build)
    snap_parts(d)


def _scale_for(d, t, build):
    bones, parts = scales_for_t(t, build)
    pb = d["arm"].pose.bones
    for name, sc in bones.items():
        pb[name].scale = sc
    bpy.context.view_layer.update()
    for key, objs in d["groups"].items():
        s = parts.get(key, 1.0)
        for o in objs:
            if "base_scale" not in o:
                o["base_scale"] = list(o.scale)
            o.scale = Vector(o["base_scale"]) * s


SNAP_FROM = {"eyes": "head"}  # parts on these bones are seated along rays from this joint
SETTLE = {"spikes"}           # parts brought down onto the skin after the ray snap
DEBUG_CONFORM = False
CONFORM = {"heart"}           # flat emblems laid along the skin's slope under them, just off it


def conform_part(d, anchor, members, body, bvh, inv_body, direction):
    """Lay a flat emblem (the heartglow) along the chest: turn it so its face follows the skin's
    slope across its own width (four rays round its centre, not one face's normal), then set it
    down so its nearest vertex sits just off the skin. Noah (2026-09-28): an upright heart before a
    sloping chest touched it at one point and hung in the air below it."""
    c = anchor.constraints[0]
    m = d["arm"].matrix_world @ d["arm"].pose.bones[c.subtarget].matrix @ c.inverse_matrix
    q_m = m.to_quaternion()
    for o in members:
        o.rotation_euler = (0, 0, 0)
        if "flat_co" not in o:  # (its flat shape, kept on the object: names repeat between builds)
            o["flat_co"] = [c for v in o.data.vertices for c in v.co]
        flat = o["flat_co"]
        for i, v in enumerate(o.data.vertices):
            v.co = Vector(flat[3 * i:3 * i + 3])
        o.data.update()
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    centre = anchor.matrix_world.translation.copy()
    radius = 0.0
    for o in members:
        ev = o.evaluated_get(dg)
        me = ev.to_mesh()
        radius = max([radius] + [(ev.matrix_world @ v.co - centre).length for v in me.vertices])
        ev.to_mesh_clear()
    if radius <= 0:
        return
    up = Vector((0, 0, 1)) - direction * direction.z
    if up.length < 1e-4:
        return
    up.normalize()
    side = direction.cross(up).normalized()
    ldir = (inv_body.to_3x3() @ direction).normalized()

    def surface(offset):
        origin = inv_body @ (centre - direction * (3.0 * radius) + offset)
        hit, _, _, _ = bvh.ray_cast(origin, ldir, 6.0 * radius)
        return body.matrix_world @ hit if hit is not None else None
    r = 0.85 * radius
    s_pos, s_neg, u_pos, u_neg = surface(side * r), surface(-side * r), surface(up * r), surface(-up * r)
    mid = surface(Vector((0, 0, 0)))
    # (a sample off the skin's edge, a belly curving away under the heart: its middle stands in)
    s_pos, s_neg, u_pos, u_neg = (x if x is not None else mid for x in (s_pos, s_neg, u_pos, u_neg))
    if None in (s_pos, s_neg, u_pos, u_neg) or (s_pos - s_neg).length < 1e-5 or (u_pos - u_neg).length < 1e-5:
        return
    n = (s_pos - s_neg).cross(u_pos - u_neg)
    if n.dot(direction) < 0:  # (outward: the way from the joint out to the part)
        n = -n
    n.x = 0.0  # (the hearts sit on the midline: kept square to it)
    if n.length < 1e-6:
        return
    n.normalize()
    horizontal = Vector((0, n.y, 0)).normalized() if abs(n.y) > 1e-4 else Vector((0, -1, 0))
    if n.angle(horizontal) > math.radians(60) and n.z < 0:  # (a chest facing mostly down: not under it)
        n = (horizontal * math.cos(math.radians(60)) - Vector((0, 0, math.sin(math.radians(60))))).normalized()
    front = q_m @ Vector((0, -1, 0))
    turn = (q_m.inverted() @ front.rotation_difference(n) @ q_m).to_euler()
    for o in members:
        o.rotation_euler = turn
    bpy.context.view_layer.update()
    gap = skin_gap(members, body, bvh, inv_body)
    if gap is None:
        return
    lift = 0.05 * radius
    delta = m.to_3x3().inverted() @ (n * (lift - gap))
    for o in members:
        o["conform_add"] = list(delta)
        off = Vector(o["snap_off"]) + delta if "snap_off" in o else delta
        o["snap_off"] = list(off)
        o.location = Vector(o["base_loc"]) + off
    # Then bent onto the curve of the chest: each vertex brought down along the face's normal to
    # the skin under it (a flat heart on a round chest touched at its middle, its rim in the air),
    # its two sheets kept apart as they were. The export reads the mesh at each growth key.
    bpy.context.view_layer.update()
    for o in members:
        me = o.data
        mw = o.matrix_world.copy()
        inv_o = mw.inverted()
        world = [mw @ v.co for v in me.vertices]
        centre_now = o.matrix_world.translation
        sheet = [(q - centre_now).dot(n) for q in world]
        back = min(sheet) if sheet else 0.0
        ldir_n = (inv_body.to_3x3() @ -n).normalized()
        for i, q in enumerate(world):
            planar = q - n * sheet[i]
            hit, _, _, _ = bvh.ray_cast(inv_body @ (planar + n * (1.5 * radius)), ldir_n, 3.0 * radius)
            if hit is not None:
                surf = body.matrix_world @ hit
                me.vertices[i].co = inv_o @ (surf + n * (lift + sheet[i] - back))
                continue
            # (past the skin's edge, a chest curving under: wrapped onto the nearest skin instead)
            near, nrm, _, _ = bvh.find_nearest(inv_body @ planar)
            if near is not None:
                nw = (body.matrix_world.to_3x3() @ nrm).normalized()
                me.vertices[i].co = inv_o @ (body.matrix_world @ near + nw * (lift + sheet[i] - back))
        me.update()
    if DEBUG_CONFORM:
        bpy.context.view_layer.update()
        print(f"[conform] n {tuple(round(x, 3) for x in n)} r {radius:.3f} gap {gap:+.3f} -> {skin_gap(members, body, bvh, inv_body):+.3f}")


def snap_parts(d, conform=True):
    """Seat each part on the body surface for the current stage: ray from the bone joint
    toward the part's anchor, place the anchor at the outermost hit (minus the group's inset),
    or at the first one if a member is marked o["snap_first"] (a part whose ray would go on
    through the body into a leg or the chin, and float there)."""
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    body = d["body"].evaluated_get(dg)
    bvh = BVHTree.FromObject(body, dg)
    inv_body = body.matrix_world.inverted()
    for key, inset in F["inset"].items():
        for anchor, members in d["snap"].get(key, []):
            c = anchor.constraints[0]
            ray_bone = SNAP_FROM.get(c.subtarget, c.subtarget)
            joint = (d["arm"].matrix_world @ d["arm"].pose.bones[ray_bone].matrix).translation
            for o in members:
                if "snap_off" in o:
                    o.location = Vector(o["base_loc"])
            bpy.context.view_layer.update()
            here = anchor.matrix_world.translation.copy()
            direction = (here - joint).normalized()
            origin, ldir, hit = inv_body @ joint, (inv_body.to_3x3() @ direction).normalized(), None
            first = any(o.get("snap_first") for o in members)
            for _ in range(1 if first else 8):
                h, _, _, _ = bvh.ray_cast(origin, ldir, 10.0)
                if h is None:
                    break
                hit, origin = h, h + ldir * 1e-4
            if hit is None:
                continue
            target = body.matrix_world @ hit - direction * inset
            delta_world = target - here
            m = d["arm"].matrix_world @ d["arm"].pose.bones[c.subtarget].matrix @ c.inverse_matrix
            delta = m.to_3x3().inverted() @ delta_world
            for o in members:
                o["snap_off"] = list(delta)
                o.location = Vector(o["base_loc"]) + delta
            if key in CONFORM and conform:
                conform_part(d, anchor, members, body, bvh, inv_body, direction)
            elif key in CONFORM:  # (a build: the neutral shape, lifted as it was, moved with the snap)
                for o in members:
                    add = Vector(o.get("conform_add", (0, 0, 0)))
                    o["snap_off"] = list(Vector(o["snap_off"]) + add)
                    o.location = Vector(o["base_loc"]) + Vector(o["snap_off"])
            if key in SETTLE:
                bpy.context.view_layer.update()
                gap = skin_gap(members, body, bvh, inv_body)
                if gap is not None and gap > -inset:
                    delta = delta + m.to_3x3().inverted() @ (-direction * (gap + inset))
                    for o in members:
                        o["snap_off"] = list(delta)
                        o.location = Vector(o["base_loc"]) + delta
    bpy.context.view_layer.update()


def skin_gap(objs, body, bvh, inv_body):
    """How far the part's nearest vertex stands off the body's surface (negative: sunk in)."""
    dg = bpy.context.evaluated_depsgraph_get()
    gap = None
    for o in objs:
        ev = o.evaluated_get(dg)
        me = ev.to_mesh()
        to_body = inv_body @ ev.matrix_world
        for v in me.vertices:
            p = to_body @ v.co
            hit, nrm, _, _ = bvh.find_nearest(p)
            if hit is not None:
                s = (p - hit).dot(nrm)
                gap = s if gap is None else min(gap, s)
        ev.to_mesh_clear()
    return gap


def ground(d):
    """Stand the dragon on the floor."""
    d["arm"].location.z = 0.0
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    ev = d["body"].evaluated_get(dg)
    me = ev.to_mesh()
    low = min((ev.matrix_world @ v.co).z for v in me.vertices)
    ev.to_mesh_clear()
    d["arm"].location.z -= low
    bpy.context.view_layer.update()


def pose_stage(d, t, build="neutral"):
    d["arm"].rotation_euler = (0, 0, 0)
    rest_pose(d, t)
    apply_t(d, t, build)
    ground(d)


def open_jaw(d, degrees):
    arm = d["arm"]
    pb = arm.pose.bones["jaw"]
    pb.rotation_mode = "XYZ"
    base = pb.rotation_euler.x
    bpy.context.view_layer.update()
    chin = (arm.matrix_world @ pb.tail).z
    pb.rotation_euler.x = base + math.radians(degrees)
    bpy.context.view_layer.update()
    if (arm.matrix_world @ pb.tail).z > chin:
        pb.rotation_euler.x = base - math.radians(degrees)
        bpy.context.view_layer.update()


def report(d, label):
    def tris(objs):
        return sum(tri_count(o) for o in objs if not o.hide_render)
    parts = {k: tris(v) for k, v in d["groups"].items()}
    body, wings = tri_count(d["body"]), tris(d["wings"]) + tris(d["rare_wings"])
    total = body + wings + sum(parts.values())
    print(f"[kit] {KIND.META['name']} {label}: {total} tris = body {body} + wings {wings} + parts {parts}")
    return total


# ------------------------------------------------------------------------------ scene & render
# Review renders: a warm storybook backdrop, a toon look close to the game's.
STAGE = {"newborn": ("hatchling", 0.0), "hatchling": ("hatchling", 0.75), "juvenile": ("grown", 0.0),
         "adolescent": ("grown", 0.45), "adult": ("grown", 1.0)}
VIEWS = {"three_quarter": Vector((-0.75, -0.95, 0.38)), "side": Vector((-1, 0, 0.12)),
         "front": Vector((-0.15, -1, 0.2)), "top": Vector((-0.2, 0.3, 1.0)),
         "back_quarter": Vector((-0.8, 0.9, 0.5)), "back": Vector((0.1, 1, 0.3)),
         "mouth": Vector((-0.55, -1, -0.05))}


def setup_scene(res=560, background=(0.93, 0.87, 0.76)):
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_EEVEE"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = scene.render.resolution_y = res
    scene.render.film_transparent = False
    world = bpy.data.worlds.new("world")
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (*background, 1)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 1.0
    scene.view_settings.view_transform = "Standard"
    bpy.ops.object.light_add(type="SUN", rotation=(math.radians(48), math.radians(12), math.radians(-38)))
    bpy.context.object.data.energy = 4.0
    bpy.ops.mesh.primitive_circle_add(vertices=64, radius=10, fill_type="NGON", location=(0, 0.8, 0))
    floor = bpy.context.object
    floor.name = "floor"
    floor.data.materials.append(toon_material("floor", (0.80, 0.70, 0.56)))
    cam_data = bpy.data.cameras.new("cam")
    cam = link(bpy.data.objects.new("cam", cam_data))
    scene.camera = cam
    return scene, cam


def visible_points(ds):
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    pts = []
    for d in ds:
        objs = [d["body"]] + d["wings"] + d["rare_wings"] + [p for objs in d["groups"].values() for p in objs]
        for o in objs:
            if o.hide_render:
                continue
            ev = o.evaluated_get(dg)
            me = ev.to_mesh()
            pts += [ev.matrix_world @ v.co for v in me.vertices]
            ev.to_mesh_clear()
    return pts


def head_points(d):
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    pts = []
    for o in d["groups"]["eyes"] + d["groups"]["horns"] + d["groups"]["frill"]:
        if o.hide_render:
            continue
        ev = o.evaluated_get(dg)
        me = ev.to_mesh()
        pts += [ev.matrix_world @ v.co for v in me.vertices]
        ev.to_mesh_clear()
    arm = d["arm"]
    for b in ("head", "snout"):
        pb = arm.pose.bones[b]
        pts += [arm.matrix_world @ pb.head, arm.matrix_world @ pb.tail]
    return pts


def frame_camera(cam, ds, view, lens=55, margin=1.55):
    if view == "portrait":
        pts, view, margin = head_points(ds[0]), "three_quarter", 1.3
    else:
        pts = visible_points(ds)
    frame_points(cam, pts, view, lens, margin)


def frame_points(cam, pts, view, lens=55, margin=1.55):
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    center = (lo + hi) / 2
    size = (hi - lo).length
    dvec = VIEWS[view].normalized()
    floor = bpy.data.objects.get("floor")
    if floor:
        floor.location = (center.x, center.y, 0)
        floor.scale = (size * 0.06,) * 3
    cam.data.lens = lens
    cam.location = center + dvec * size * margin
    cam.rotation_euler = (center - cam.location).to_track_quat("-Z", "Y").to_euler()


def render(path):
    scene = bpy.context.scene
    scene.render.filepath = bpy.path.abspath(path)
    bpy.ops.render.render(write_still=True)


def clear_dragons():
    for o in list(bpy.data.objects):
        if o.name not in ("floor", "cam") and o.type != "LIGHT":
            bpy.data.objects.remove(o, do_unlink=True)


def textured(d, variant=None):
    """Previews: bake the body's skin and put the variant's pattern on it, as the game does."""
    v = variant_colors(d["variant"] if variant is None else variant)
    rgba = tex.bake_skin(d["body"], d["form"], 256, KIND)
    tex.preview_material(d["mats"]["body"], rgba, v)
    d["skin_rgba"] = rgba
    return rgba


def lineup(cam, variant=0, texture=False, out=None):
    """Growth lineup at true relative size: hatch day, late hatchling, juvenile, adolescent,
    adult (side view)."""
    ds, front = [], 0.0
    for stage in ("newborn", "hatchling", "juvenile", "adolescent", "adult"):
        form, t = STAGE[stage]
        d = build_dragon(form, variant)
        if texture:
            textured(d)
        pose_stage(d, t)
        k = F["export_scale"] * KIND.META.get("size", 1.0)
        d["arm"].scale = (k, k, k)
        d["arm"].location.z *= k
        bpy.context.view_layer.update()
        ys = [p.y for p in visible_points([d])]
        d["arm"].location.y += front - 0.3 - max(ys)
        bpy.context.view_layer.update()
        front = min(p.y for p in visible_points([d]))
        ds.append(d)
    frame_camera(cam, ds, "side", lens=50, margin=1.45)
    bpy.context.scene.render.resolution_x, bpy.context.scene.render.resolution_y = 1400, 560
    render(out or f"{arg('--out')}_{KIND.META['name']}_lineup.png")


def main():
    use_kind(arg("--kind", "pouncer"))
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene, cam = setup_scene(int(arg("--res", "560")))
    out = arg("--out", os.path.join(ROOT, "build", "kit", KIND.META["name"]))
    variant = int(arg("--variant", "0"))
    if "--lineup" in argv:
        return lineup(cam, variant, "--texture" in argv, f"{out}_{KIND.META['name']}_lineup.png")
    views = arg("--views", "three_quarter").split(",")
    built = None
    for stage in arg("--stages", "hatchling,adult").split(","):
        form, t = STAGE[stage]
        if built is None or built["form"] != form:
            if built is not None:
                clear_dragons()
            built = build_dragon(form, variant)
            if "--texture" in argv:
                textured(built)
        pose_stage(built, t)
        if arg("--jaw"):
            open_jaw(built, float(arg("--jaw")))
        report(built, stage)
        for view in views:
            frame_camera(cam, [built], view)
            render(f"{out}_{KIND.META['name']}_{stage}_{view}.png")
        if arg("--turntable"):
            n = int(arg("--turntable"))
            arm = built["arm"]
            pts = []
            for k in range(n):
                arm.rotation_euler.z = 2 * math.pi * k / n
                bpy.context.view_layer.update()
                pts += visible_points([built])
            frame_points(cam, pts, "side", lens=55, margin=1.4)
            for k in range(n):
                arm.rotation_euler.z = 2 * math.pi * k / n
                bpy.context.view_layer.update()
                render(f"{out}_{KIND.META['name']}_{stage}_turn{k:02d}.png")
            arm.rotation_euler.z = 0


if __name__ == "__main__":
    main()
