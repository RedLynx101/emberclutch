"""Emberclutch dragon model — organic base mesh, rig, starter parts and growth stages.

Run headless:
  blender -b -P tools/blender/dragon_model.py -- --breed ember --stages hatchling,adult --out tools/blender/out/r1
  blender -b -P tools/blender/dragon_model.py -- --sheet --out tools/blender/out/r1     (all starters x stages)

How it works (docs/plan/alpha-1.md WP2):
  * The body is ONE mesh: a Skin modifier wraps a skeleton graph (nodes with radii), then
    subdivision smooths it and a decimate brings it to the triangle budget.
  * One armature (<= 24 bones for the body draw; wings are a second draw with their own set).
  * The mesh is modelled as an ADULT. Younger stages are pose-space bone scales
    (girth, length) with scale inheritance turned off, exactly what the runtime does
    (architecture section 4): big head, short thick neck, round belly, stubby legs, tiny wings.
  * Parts (horns, frills, wings, tail tips, spikes, eyes, heartglow) are separate meshes
    bound to fixed bones, chosen by the genome.
  * Preview shading is a toon ramp + warm rim to approximate the in-game look. The mask
    colours (base / accent) come from a per-vertex attribute that later bakes into the
    mask texture's R/G channels.
"""
import math
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

# ------------------------------------------------------------------------------ arguments
argv = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []


def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


BREED = arg("--breed", "ember")
STAGES = arg("--stages", "hatchling,adolescent,adult").split(",")
OUT = arg("--out", "//dragon")
SHEET = "--sheet" in argv
EXPORT = "--export" in argv
RES = int(arg("--res", "560"))

# Body triangle budgets (architecture section 1): body mesh share of the adult LOD0 budget.
BODY_TRIS = 2000

# ------------------------------------------------------------------------------ breeds
# Starter defaults (src/core/genetics.cpp): build, horns, frill, wings, tail, colours.
BREEDS = {
    "ember": dict(build="sturdy", horns="swept", frill="none", wings="membrane", tail="spade",
                  base=(0.86, 0.30, 0.10), accent=(0.98, 0.84, 0.55), horn=(0.96, 0.74, 0.30),
                  glow=(1.0, 0.55, 0.16), eye=(0.95, 0.62, 0.15)),
    "tide": dict(build="long", horns="nubs", frill="fin", wings="fin", tail="fan",
                 base=(0.10, 0.60, 0.62), accent=(0.70, 0.93, 0.86), horn=(0.62, 0.90, 0.85),
                 glow=(0.35, 0.95, 0.85), eye=(0.20, 0.55, 0.85)),
    "gale": dict(build="sleek", horns="swept", frill="feather", wings="feathered", tail="tuft",
                 base=(0.55, 0.72, 0.95), accent=(0.95, 0.97, 1.00), horn=(0.85, 0.92, 1.00),
                 glow=(0.62, 0.92, 1.00), eye=(0.25, 0.45, 0.90)),
}

# ------------------------------------------------------------------------------ skeleton graph
# Adult proportions. Dragon faces -Y, Z up, units ~ metres. radius = (side, vertical).
NODES = {
    "tail_tip": ((0, 3.55, 0.26), (0.03, 0.03)),
    "tail4": ((0, 2.95, 0.34), (0.11, 0.10)),
    "tail3": ((0, 2.30, 0.50), (0.19, 0.18)),
    "tail2": ((0, 1.62, 0.74), (0.30, 0.29)),
    "hips": ((0, 0.92, 0.98), (0.44, 0.46)),
    "belly": ((0, 0.20, 1.00), (0.52, 0.58)),
    "chest": ((0, -0.50, 1.10), (0.50, 0.62)),
    "neck1": ((0, -0.98, 1.52), (0.31, 0.33)),
    "neck2": ((0, -1.28, 1.92), (0.23, 0.25)),
    "neck3": ((0, -1.42, 2.32), (0.20, 0.21)),
    "head": ((0, -1.58, 2.62), (0.28, 0.26)),
    "muzzle": ((0, -1.84, 2.55), (0.19, 0.16)),
    "snout": ((0, -2.12, 2.47), (0.12, 0.10)),
}
for side, s in (("L", -1), ("R", 1)):
    NODES.update({
        f"shoulder_{side}": ((s * 0.38, -0.50, 0.86), (0.24, 0.27)),
        f"elbow_{side}": ((s * 0.46, -0.62, 0.46), (0.16, 0.16)),
        f"wrist_{side}": ((s * 0.46, -0.70, 0.15), (0.12, 0.12)),
        f"toe_f_{side}": ((s * 0.48, -0.96, 0.07), (0.13, 0.07)),
        f"hipj_{side}": ((s * 0.42, 0.98, 0.78), (0.31, 0.35)),
        f"knee_{side}": ((s * 0.52, 0.64, 0.44), (0.20, 0.20)),
        f"ankle_{side}": ((s * 0.52, 1.04, 0.18), (0.13, 0.13)),
        f"toe_b_{side}": ((s * 0.54, 0.78, 0.07), (0.14, 0.07)),
    })

EDGES = [("tail_tip", "tail4"), ("tail4", "tail3"), ("tail3", "tail2"), ("tail2", "hips"),
         ("hips", "belly"), ("belly", "chest"), ("chest", "neck1"), ("neck1", "neck2"),
         ("neck2", "neck3"), ("neck3", "head"), ("head", "muzzle"), ("muzzle", "snout")]
for side in ("L", "R"):
    EDGES += [("chest", f"shoulder_{side}"), (f"shoulder_{side}", f"elbow_{side}"),
              (f"elbow_{side}", f"wrist_{side}"), (f"wrist_{side}", f"toe_f_{side}"),
              ("hips", f"hipj_{side}"), (f"hipj_{side}", f"knee_{side}"),
              (f"knee_{side}", f"ankle_{side}"), (f"ankle_{side}", f"toe_b_{side}")]

# Bones: (name, head node, tail node, parent). 24 bones -> one body draw call.
BONES = [
    ("hips", "hips", "belly", None),
    ("belly", "belly", "chest", "hips"),
    ("chest", "chest", "neck1", "belly"),
    ("neck1", "neck1", "neck2", "chest"),
    ("neck2", "neck2", "neck3", "neck1"),
    ("neck3", "neck3", "head", "neck2"),
    ("head", "head", "muzzle", "neck3"),
    ("snout", "muzzle", "snout", "head"),
    ("tail1", "hips", "tail2", "hips"),
    ("tail2", "tail2", "tail3", "tail1"),
    ("tail3", "tail3", "tail4", "tail2"),
    ("tail4", "tail4", "tail_tip", "tail3"),
]
for side in ("L", "R"):
    BONES += [
        (f"arm_up_{side}", f"shoulder_{side}", f"elbow_{side}", "chest"),
        (f"arm_lo_{side}", f"elbow_{side}", f"wrist_{side}", f"arm_up_{side}"),
        (f"hand_{side}", f"wrist_{side}", f"toe_f_{side}", f"arm_lo_{side}"),
        (f"leg_up_{side}", f"hipj_{side}", f"knee_{side}", "hips"),
        (f"leg_lo_{side}", f"knee_{side}", f"ankle_{side}", f"leg_up_{side}"),
        (f"foot_{side}", f"ankle_{side}", f"toe_b_{side}", f"leg_lo_{side}"),
    ]

# ------------------------------------------------------------------------------ growth tables
# Pose-space scales per bone: (girth, length). Unlisted bones = (1, 1). "parts" scales the
# attached part meshes. Juvenile/adolescent interpolate toward the adult.
HATCHLING = {
    "bones": {
        "head": (1.9, 1.5, 2.25), "snout": (1.8, 0.38, 1.9),
        "neck1": (1.30, 0.28), "neck2": (1.45, 0.22), "neck3": (1.55, 0.22),
        "chest": (1.12, 0.45), "belly": (1.16, 0.34), "hips": (1.10, 0.40),
        "tail1": (1.00, 0.50), "tail2": (1.05, 0.42), "tail3": (1.15, 0.42), "tail4": (1.30, 0.48),
        "arm_up": (1.25, 0.55), "arm_lo": (1.35, 0.55), "hand": (1.45, 0.85),
        "leg_up": (1.20, 0.55), "leg_lo": (1.35, 0.50), "foot": (1.45, 0.85),
    },
    "parts": {"eyes": 1.45, "horns": 0.35, "frill": 0.55, "wings": 0.28, "spikes": 0.45,
              "tail_tip": 0.85, "heart": 1.2},
}
STAGE_T = {"hatchling": 0.0, "juvenile": 0.35, "adolescent": 0.70, "adult": 1.0}

BUILDS = {  # multiplies (girth, length) on top of the stage
    "sturdy": {"chest": (1.10, 1.0), "belly": (1.10, 1.0), "arm_up": (1.10, 0.95), "leg_up": (1.10, 0.95),
               "neck1": (1.08, 0.95), "neck2": (1.08, 0.95), "neck3": (1.08, 0.95)},
    "sleek": {"chest": (0.92, 1.05), "belly": (0.88, 1.05), "neck1": (0.9, 1.08), "neck2": (0.9, 1.1),
              "neck3": (0.9, 1.1), "arm_lo": (0.95, 1.05), "leg_lo": (0.95, 1.05)},
    "long": {"neck1": (0.88, 1.25), "neck2": (0.88, 1.3), "neck3": (0.88, 1.3), "tail1": (0.9, 1.2),
             "tail2": (0.9, 1.3), "tail3": (0.9, 1.35), "tail4": (0.9, 1.4), "belly": (0.9, 1.1),
             "leg_up": (0.95, 0.9), "leg_lo": (0.95, 0.9)},
}


def lerp(a, b, t):
    return a + (b - a) * t


def stage_scales(stage, build):
    """Bone scales (girth, length) and part scales for a stage + build."""
    t = STAGE_T[stage]
    bones = {}
    for name, *_ in BONES:
        key = name.rsplit("_", 1)[0] if name.endswith(("_L", "_R")) else name
        h = HATCHLING["bones"].get(key, (1.0, 1.0))
        gx, l, gz = (h[0], h[1], h[0]) if len(h) == 2 else h
        gx, l, gz = lerp(gx, 1.0, t), lerp(l, 1.0, t), lerp(gz, 1.0, t)
        bg, bl = BUILDS[build].get(key, (1.0, 1.0))
        bones[name] = (gx * bg, l * bl, gz * bg)
    parts = {k: lerp(v, 1.0, t) for k, v in HATCHLING["parts"].items()}
    return bones, parts


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


# ------------------------------------------------------------------------------ materials
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
    els[0].position, els[0].color = 0.0, (0.42, 0.36, 0.44, 1)  # plum-tinted shadow
    els[1].position, els[1].color = 0.22, (0.78, 0.74, 0.76, 1)
    e = els.new(0.55)
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
    # warm rim
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


# ------------------------------------------------------------------------------ body
def build_body():
    names = list(NODES)
    idx = {n: i for i, n in enumerate(names)}
    me = bpy.data.meshes.new("body")
    me.from_pydata([NODES[n][0] for n in names], [(idx[a], idx[b]) for a, b in EDGES], [])
    obj = link(bpy.data.objects.new("body", me))
    skin = obj.modifiers.new("skin", "SKIN")
    skin.use_smooth_shade = True
    skin.branch_smoothing = 0.6
    for i, n in enumerate(names):
        sv = me.skin_vertices[0].data[i]
        sv.radius = NODES[n][1]
        sv.use_root = n == "hips"
    sub = obj.modifiers.new("sub", "SUBSURF")
    sub.levels = sub.render_levels = int(arg("--subd", "2"))
    sm = obj.modifiers.new("relax", "SMOOTH")  # even out skin-modifier lumps before decimating
    sm.factor = 0.6
    sm.iterations = 6
    apply_modifiers(obj)
    sculpt_details(obj)
    decimate_to(obj, BODY_TRIS)
    smooth(obj)
    paint_mask(obj)
    return obj


def sculpt_details(obj):
    """Procedural shaping the skin graph can't do: deep chest keel, brow, cheeks, jaw line,
    tapered snout, slimmer ankles."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for v in bm.verts:
        x, y, z = v.co
        # chest keel: push the underside of the chest down/forward
        if -0.9 < y < 0.2 and z < 1.15 and abs(x) < 0.35:
            k = (1 - abs(x) / 0.35) * max(0.0, (1.15 - z) / 0.5)
            v.co.z -= 0.10 * k
            v.co.y -= 0.05 * k
        # brow ridge over the eyes
        if -1.45 < y < -1.1 and z > 3.12 and abs(x) > 0.08:
            v.co.z += 0.05
        # cheeks: widen the back of the head a little
        if -1.35 < y < -1.05 and 2.95 < z < 3.2:
            v.co.x *= 1.12
        # snout: flatten the top and taper to the nose
        if y < -1.5 and z > 2.8:
            t = min(1.0, (-1.5 - y) / 0.35)
            v.co.z -= 0.03 * t
            v.co.x *= 1.0 - 0.18 * t
    bm.to_mesh(obj.data)
    bm.free()


def paint_mask(obj):
    """Per-vertex mask colour: R = base weight, G = accent (belly plates, throat, under-tail)."""
    me = obj.data
    attr = me.color_attributes.new("mask", "FLOAT_COLOR", "POINT")
    down = Vector((0, -0.45, -0.89)).normalized()
    for v in me.vertices:
        n = v.normal
        x, y, z = v.co
        a = max(0.0, (n.dot(down) - 0.25) / 0.45)
        a = min(1.0, a)
        # only the torso/neck/tail underside, not legs or head top
        if abs(x) > 0.34 or z > 2.35 or (y > 1.2 and z < 0.3):
            a = 0.0
        attr.data[v.index].color = (1.0 - a, a, 0.0, 1.0)


# ------------------------------------------------------------------------------ armature
def build_armature():
    arm_data = bpy.data.armatures.new("rig")
    arm = link(bpy.data.objects.new("rig", arm_data))
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.mode_set(mode="EDIT")
    eb = {}
    for name, h, t, parent in BONES:
        b = arm_data.edit_bones.new(name)
        b.head, b.tail = V(NODES[h][0]), V(NODES[t][0])
        b.roll = 0
        if parent:
            b.parent = eb[parent]
            b.use_connect = (b.head - eb[parent].tail).length < 1e-4
        b.inherit_scale = "NONE"
        eb[name] = b
    add_wing_bones(arm_data, eb)
    bpy.ops.object.mode_set(mode="OBJECT")
    return arm


WING_ROOT = (0.36, -0.40, 1.52)


def wing_points(side):
    s = -1 if side == "L" else 1
    P = lambda x, y, z: Vector((s * x, y, z))  # noqa: E731
    return {
        "root": P(*WING_ROOT),
        "elbow": P(1.05, -0.05, 2.10),
        "wrist": P(1.85, -0.40, 2.42),
        "f1": P(3.10, 0.15, 2.85),
        "f2": P(3.05, 1.05, 2.10),
        "f3": P(2.25, 1.55, 1.55),
        "body": P(0.32, 0.85, 1.42),
    }


def add_wing_bones(arm_data, eb):
    for side in ("L", "R"):
        w = wing_points(side)
        chain = [("wing_arm", "root", "elbow", "chest"), ("wing_fore", "elbow", "wrist", "wing_arm"),
                 ("wing_f1", "wrist", "f1", "wing_fore"), ("wing_f2", "wrist", "f2", "wing_fore"),
                 ("wing_f3", "wrist", "f3", "wing_fore")]
        for name, h, t, parent in chain:
            b = arm_data.edit_bones.new(f"{name}_{side}")
            b.head, b.tail = w[h], w[t]
            p = eb[parent] if parent == "chest" else eb[f"{parent}_{side}"]
            b.parent = p
            b.use_connect = (b.head - p.tail).length < 1e-4
            b.inherit_scale = "NONE"
            eb[f"{name}_{side}"] = b


def bind(mesh_obj, arm, keep):
    """Automatic weights, then keep only the bones this draw may use (body or wing set)."""
    bpy.ops.object.select_all(action="DESELECT")
    mesh_obj.select_set(True)
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.parent_set(type="ARMATURE_AUTO")
    bpy.ops.object.select_all(action="DESELECT")
    for vg in list(mesh_obj.vertex_groups):
        if not keep(vg.name):
            mesh_obj.vertex_groups.remove(vg)
    bpy.context.view_layer.objects.active = mesh_obj
    mesh_obj.select_set(True)
    bpy.ops.object.mode_set(mode="WEIGHT_PAINT")
    bpy.ops.object.vertex_group_normalize_all(lock_active=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    mesh_obj.select_set(False)


def parent_to_bone(obj, arm, bone):
    """Attach a rigid part at the bone's JOINT (pose matrix origin), like the runtime does.
    Plain bone-parenting in Blender is relative to the bone's tail, which pushes parts away
    from the surface when a growth stage scales the bone."""
    bpy.context.view_layer.update()
    c = obj.constraints.new("CHILD_OF")
    c.target = arm
    c.subtarget = bone
    c.inverse_matrix = (arm.matrix_world @ arm.pose.bones[bone].matrix).inverted()
    obj["base_loc"] = list(obj.location)


# ------------------------------------------------------------------------------ parts
def sphere(name, loc, scale, seg=16, rings=10):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=seg, v_segments=rings, radius=1.0)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = link(bpy.data.objects.new(name, me))
    obj.location = loc
    obj.scale = scale
    smooth(obj)
    return obj


def horn_mesh(name, length, radius, curve, segments=7, ring=6):
    """A tapered horn swept backward along a curve (in local +Y back, +Z up)."""
    bm = bmesh.new()
    rings = []
    for i in range(segments + 1):
        t = i / segments
        ang = curve * t
        c = Vector((0, math.sin(ang) * length * t * 0.9, math.cos(ang) * length * t))
        r = radius * (1 - t) ** 0.9 + 0.004
        rings.append([bm.verts.new(c + Vector((math.cos(a) * r, math.sin(a) * r * 0.8, 0)))
                      for a in (2 * math.pi * k / ring for k in range(ring))])
    for a, b in zip(rings, rings[1:]):
        for k in range(ring):
            bm.faces.new((a[k], a[(k + 1) % ring], b[(k + 1) % ring], b[k]))
    bm.faces.new(list(reversed(rings[0])))
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = link(bpy.data.objects.new(name, me))
    smooth(obj)
    return obj


def flat_fan(name, pts, thickness=0.015):
    """Flat fan from pts[0]; the object origin sits at pts[0] so part scaling pivots there."""
    origin = Vector(pts[0])
    bm = bmesh.new()
    vs = [bm.verts.new(Vector(p) - origin) for p in pts]
    for i in range(1, len(vs) - 1):
        bm.faces.new((vs[0], vs[i], vs[i + 1]))
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = link(bpy.data.objects.new(name, me))
    obj.location = origin
    m = obj.modifiers.new("thick", "SOLIDIFY")
    m.thickness = thickness
    m.offset = 0
    apply_modifiers(obj)
    return obj


def build_eyes(mats):
    """Cute-but-noble eyes: a large amber iris dome, a big dark pupil and two glints, set
    into the face with a thin lid rim in the body colour. No white eyeball (that read as frog)."""
    eyes = []
    hx, hy, hz = NODES["head"][0]
    for s in (-1, 1):
        out = Vector((s * 0.55, -0.83, 0.08)).normalized()
        c = Vector((s * 0.155, hy - 0.20, hz + 0.0))
        lid = sphere(f"lid_{s}", c - out * 0.018, (0.068, 0.068, 0.074), 10, 6)
        lid.data.materials.append(mats["body_plain"])
        iris = sphere(f"iris_{s}", c, (0.060, 0.060, 0.066), 10, 7)
        iris.data.materials.append(mats["iris"])
        pupil = sphere(f"pupil_{s}", c + out * 0.036, (0.036, 0.036, 0.046), 8, 5)
        pupil.data.materials.append(mats["pupil"])
        up = Vector((0, 0, 1))
        g1 = sphere(f"glint_{s}", c + out * 0.074 + up * 0.022 - Vector((s * 0.012, 0, 0)), (0.013,) * 3, 6, 4)
        g2 = sphere(f"glint2_{s}", c + out * 0.074 - up * 0.02 + Vector((s * 0.01, 0, 0)), (0.006,) * 3, 6, 4)
        for g in (g1, g2):
            g.data.materials.append(mats["glint"])
        for o in (lid, iris, pupil, g1, g2):
            eyes.append(o)
    return eyes


def build_horns(kind, mats):
    hx, hy, hz = NODES["head"][0]
    horns = []
    for s in (-1, 1):
        if kind == "swept":
            h = horn_mesh(f"horn_{s}", 0.56, 0.075, math.radians(55))
            h.location = (s * 0.13, hy + 0.10, hz + 0.16)
            h.rotation_euler = (math.radians(-18), s * math.radians(12), 0)
            h2 = horn_mesh(f"hornlet_{s}", 0.26, 0.04, math.radians(70))
            h2.location = (s * 0.22, hy + 0.02, hz + 0.02)
            h2.rotation_euler = (math.radians(-40), s * math.radians(55), 0)
            horns += [h, h2]
        elif kind == "nubs":
            h = horn_mesh(f"horn_{s}", 0.20, 0.07, math.radians(35))
            h.location = (s * 0.12, hy + 0.08, hz + 0.17)
            h.rotation_euler = (math.radians(-20), s * math.radians(15), 0)
            horns.append(h)
    for h in horns:
        h.data.materials.append(mats["horn"])
    return horns


def build_frill(kind, mats):
    hx, hy, hz = NODES["head"][0]
    parts = []
    for s in (-1, 1):
        if kind == "fin":
            f = flat_fan(f"fin_{s}", [Vector((s * 0.2, hy + 0.06, hz - 0.02)),
                                      Vector((s * 0.46, hy + 0.20, hz + 0.18)),
                                      Vector((s * 0.52, hy + 0.34, hz + 0.02)),
                                      Vector((s * 0.44, hy + 0.36, hz - 0.14)),
                                      Vector((s * 0.2, hy + 0.20, hz - 0.12))])
            parts.append(f)
        elif kind == "feather":
            for k in range(3):
                f = flat_fan(f"feather_{s}_{k}", [Vector((s * 0.18, hy + 0.10, hz - 0.02 - k * 0.06)),
                                                  Vector((s * 0.40, hy + 0.28 + k * 0.05, hz + 0.08 - k * 0.1)),
                                                  Vector((s * 0.46, hy + 0.42 + k * 0.05, hz - 0.02 - k * 0.1)),
                                                  Vector((s * 0.22, hy + 0.20, hz - 0.08 - k * 0.06))])
                parts.append(f)
    for p in parts:
        p.data.materials.append(mats["accent_flat"])
    return parts


def build_spikes(mats):
    spikes = []
    path = [("neck3", 0.16), ("neck2", 0.2), ("neck1", 0.26), ("chest", 0.5), ("belly", 0.52),
            ("hips", 0.44), ("tail2", 0.27), ("tail3", 0.17), ("tail4", 0.1)]
    for i, (node, top) in enumerate(path):
        x, y, z = NODES[node][0]
        size = 0.07 + 0.05 * min(1.0, top / 0.3)
        h = horn_mesh(f"spike_{i}", size * 1.8, size * 0.7, math.radians(40), 3, 5)
        h.location = (0, y + 0.05, z + top * 0.92)
        h.rotation_euler = (math.radians(-10), 0, 0)
        h.data.materials.append(mats["horn"])
        spikes.append((h, node))
    return spikes


def build_tail_tip(kind, mats):
    x, y, z = NODES["tail_tip"][0]
    if kind == "spade":
        obj = flat_fan("tail_tip", [Vector((0, y - 0.05, z)), Vector((-0.2, y + 0.12, z)),
                                    Vector((0, y + 0.42, z)), Vector((0.2, y + 0.12, z))], 0.04)
    elif kind == "fan":
        pts = [Vector((0, y - 0.05, z))]
        for k in range(7):
            a = math.radians(-65 + k * 21.6)
            pts.append(Vector((math.sin(a) * 0.36, y + math.cos(a) * 0.40, z + 0.02)))
        obj = flat_fan("tail_tip", pts, 0.02)
    else:  # tuft
        obj = sphere("tail_tip", (0, y + 0.18, z + 0.02), (0.14, 0.24, 0.12), 10, 7)
    obj.data.materials.append(mats["horn" if kind == "spade" else "accent_flat"])
    return obj


def build_wings(kind, mats):
    """Wing arm (skinned tube) + membrane/fin/feathers, bound to the wing bones."""
    objs = []
    for side in ("L", "R"):
        w = wing_points(side)
        # arm and finger bones as a thin skin mesh
        names = ["root", "elbow", "wrist", "f1", "f2", "f3"]
        rad = {"root": 0.09, "elbow": 0.065, "wrist": 0.05, "f1": 0.012, "f2": 0.012, "f3": 0.012}
        me = bpy.data.meshes.new(f"wingarm_{side}")
        me.from_pydata([w[n] for n in names], [(0, 1), (1, 2), (2, 3), (2, 4), (2, 5)], [])
        arm = link(bpy.data.objects.new(f"wingarm_{side}", me))
        sk = arm.modifiers.new("skin", "SKIN")
        for i, n in enumerate(names):
            me.skin_vertices[0].data[i].radius = (rad[n], rad[n])
            me.skin_vertices[0].data[i].use_root = i == 0
        sub = arm.modifiers.new("sub", "SUBSURF")
        sub.levels = 1
        apply_modifiers(arm)
        decimate_to(arm, 260)
        smooth(arm)
        arm.data.materials.append(mats["body_plain"])
        objs.append(arm)

        if kind in ("membrane", "fin"):
            def scallop(a, b, depth, n=3):
                """Points along a trailing edge that sags toward the wrist between two tips."""
                return [a.lerp(b, k / (n + 1)).lerp(w["wrist"], depth * math.sin(math.pi * k / (n + 1)))
                        for k in range(1, n + 1)]
            if kind == "membrane":
                edge = [w["body"]] + scallop(w["body"], w["f3"], 0.22) + [w["f3"]] +                        scallop(w["f3"], w["f2"], 0.30) + [w["f2"]] + scallop(w["f2"], w["f1"], 0.30) + [w["f1"]]
            else:  # fin: shorter, rounded, webbed
                f3, f2, f1 = (w[k].lerp(w["wrist"], 0.2) for k in ("f3", "f2", "f1"))
                edge = [w["body"]] + scallop(w["body"], f3, 0.12) + [f3] + scallop(f3, f2, 0.1) + [f2] +                        scallop(f2, f1, 0.1) + [f1]
            pts = [w["wrist"], w["elbow"], w["root"]] + edge
            mem = flat_fan(f"membrane_{side}", pts, 0.012)
            mem.data.materials.append(mats["membrane"])
            objs.append(mem)
        else:  # feathered: overlapping feather blades along the arm and fingers
            anchors = [w["wrist"].lerp(w["f1"], t) for t in (0.2, 0.5, 0.8)] + \
                      [w["elbow"].lerp(w["wrist"], t) for t in (0.2, 0.6, 1.0)] + \
                      [w["root"].lerp(w["elbow"], t) for t in (0.5, 1.0)]
            for k, a in enumerate(anchors):
                length = 1.1 - 0.07 * k
                tip = a + Vector((0, 0.9, -0.55)).normalized() * length
                s = -1 if side == "L" else 1
                side_off = Vector((s * 0.12, 0, 0))
                f = flat_fan(f"feather_{side}_{k}", [a, a + side_off, tip + side_off * 0.5, tip], 0.01)
                f.data.materials.append(mats["membrane"])
                objs.append(f)
    return objs


def build_heart(mats, color):
    x, y, z = NODES["chest"][0]
    c = Vector((0, y - 0.55, z + 0.05))
    parts = []
    for s in (-1, 1):
        parts.append(sphere(f"heart_lobe_{s}", c + Vector((s * 0.05, 0, 0.03)), (0.06, 0.02, 0.06), 8, 5))
    tip = flat_fan("heart_tip", [c + Vector((-0.1, 0, 0.02)), c + Vector((0.1, 0, 0.02)), c + Vector((0, 0, -0.11))], 0.03)
    parts.append(tip)
    for p in parts:
        p.data.materials.append(mats["heart"])
    return parts


# ------------------------------------------------------------------------------ assembly
def build_dragon(breed):
    b = BREEDS[breed]
    mats = {
        "body": toon_material("body", b["base"], accent=b["accent"]),
        "body_plain": toon_material("body_plain", b["base"]),
        "accent_flat": toon_material("accent_flat", b["accent"]),
        "membrane": toon_material("membrane", tuple(0.55 * c + 0.45 * a for c, a in zip(b["base"], b["accent"]))),
        "horn": toon_material("horn", b["horn"]),
        "eye_white": toon_material("eye_white", (1.0, 0.97, 0.9)),
        "iris": toon_material("iris", b["eye"]),
        "pupil": toon_material("pupil", (0.06, 0.03, 0.05)),
        "glint": toon_material("glint", (1, 1, 1), emission=2.0),
        "heart": toon_material("heart", b["glow"], emission=1.4),
    }
    body = build_body()
    body.data.materials.append(mats["body"])
    arm = build_armature()
    bind(body, arm, lambda n: not n.startswith("wing"))

    wings = build_wings(b["wings"], mats)
    for wobj in wings:
        bind(wobj, arm, lambda n: n.startswith("wing") or n == "chest")

    groups = {"eyes": [], "horns": [], "frill": [], "spikes": [], "tail_tip": [], "heart": []}
    snap = {"eyes": [], "horns": [], "frill": [], "spikes": [], "heart": []}
    eyes = build_eyes(mats)
    for e in eyes:
        parent_to_bone(e, arm, "head")
        groups["eyes"].append(e)
    for s in (-1, 1):
        members = [e for e in eyes if e.name.endswith(f"_{s}")]
        snap["eyes"].append((next(e for e in members if e.name.startswith("iris")), members))
    for h in build_horns(b["horns"], mats):
        parent_to_bone(h, arm, "head")
        groups["horns"].append(h)
        snap["horns"].append((h, [h]))
    for f in build_frill(b["frill"], mats):
        parent_to_bone(f, arm, "head")
        groups["frill"].append(f)
        snap["frill"].append((f, [f]))
    for s, node in build_spikes(mats):
        bone = {"neck3": "neck3", "neck2": "neck2", "neck1": "neck1", "chest": "chest", "belly": "belly",
                "hips": "hips", "tail2": "tail2", "tail3": "tail3", "tail4": "tail4"}[node]
        parent_to_bone(s, arm, bone)
        groups["spikes"].append(s)
        snap["spikes"].append((s, [s]))
    tip = build_tail_tip(b["tail"], mats)
    parent_to_bone(tip, arm, "tail4")
    groups["tail_tip"].append(tip)
    hearts = build_heart(mats, b["glow"])
    for h in hearts:
        parent_to_bone(h, arm, "chest")
        groups["heart"].append(h)
    snap["heart"].append((hearts[0], hearts))
    return dict(body=body, arm=arm, wings=wings, groups=groups, snap=snap, breed=b)


HATCH_POSE = {"neck1": float(arg("--hn1", "-46")), "neck2": float(arg("--hn2", "-12")),
              "neck3": float(arg("--hn3", "8")), "head": float(arg("--hh", "46"))}


def rest_pose(d, stage="adult"):
    """A proud idle: neck S-curve, head level, wings half-folded. Babies hold their big heads
    up over the body (HATCH_POSE), blending toward the adult pose as they grow."""
    t = STAGE_T[stage]
    pb = d["arm"].pose.bones
    rot = {"neck1": (-4, 0, 0), "neck2": (6, 0, 0), "neck3": (10, 0, 0), "head": (-6, 0, 0),
           "tail1": (6, 0, 0), "tail2": (-4, 0, 6), "tail3": (-6, 0, 10), "tail4": (-4, 0, 12)}
    for side, s in (("L", 1), ("R", -1)):
        rot[f"wing_arm_{side}"] = (18, 0, s * -25)
        rot[f"wing_fore_{side}"] = (0, 0, s * 35)
    for name, extra in HATCH_POSE.items():
        x, y, z = rot.get(name, (0, 0, 0))
        rot[name] = (x + extra * (1 - t), y, z)
    for name, (x, y, z) in rot.items():
        b = pb[name]
        b.rotation_mode = "XYZ"
        b.rotation_euler = (math.radians(x), math.radians(y), math.radians(z))


def sit_pose(d, amount=1.0):
    """Sitting: the body tips back on the haunches, hind legs fold forward, front legs stay
    straight. Rotation signs were found empirically (--sit-* args override for tuning)."""
    arm, pb = d["arm"], d["arm"].pose.bones
    tilt = float(arg("--sit-tilt", "-28")) * amount
    arm.rotation_euler.x = math.radians(tilt)
    adj = {"arm_up": float(arg("--sit-armup", "60")), "leg_up": float(arg("--sit-legup", "70")),
           "leg_lo": float(arg("--sit-leglo", "-95")), "foot": float(arg("--sit-foot", "60")),
           "tail1": float(arg("--sit-tail", "28"))}
    for side in ("", "_L", "_R"):
        for key, ang in adj.items():
            name = key + side
            if name in pb:
                b = pb[name]
                b.rotation_mode = "XYZ"
                b.rotation_euler.x += math.radians(ang * amount)
    for n, extra in (("neck1", 30), ("head", 20)):  # the tilt already lifts the head
        pb[n].rotation_euler.x += math.radians(extra * amount)


def apply_stage(d, stage):
    bones, parts = stage_scales(stage, d["breed"]["build"])
    pb = d["arm"].pose.bones
    for name, (gx, l, gz) in bones.items():
        pb[name].scale = (gx, l, gz)
    wscale = parts["wings"]
    for side in ("L", "R"):
        for n in ("wing_arm", "wing_fore", "wing_f1", "wing_f2", "wing_f3"):
            pb[f"{n}_{side}"].scale = (wscale,) * 3
    bpy.context.view_layer.update()
    for key, objs in d["groups"].items():
        s = parts.get(key, 1.0)
        for o in objs:
            if "base_scale" not in o:
                o["base_scale"] = list(o.scale)
            o.scale = Vector(o["base_scale"]) * s
    snap_parts(d)
    ground(d)


SNAP_INSET = {"eyes": 0.04, "horns": 0.03, "spikes": 0.02, "frill": 0.03, "heart": -0.012}


def snap_parts(d):
    """Seat each part on the body surface for the current stage: ray from the bone joint
    toward the part's anchor, place the anchor at the hit (minus a small inset). Eye pieces
    move together (anchored on the iris). The converter exports these per-stage offsets."""
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    body = d["body"].evaluated_get(dg)
    bvh = BVHTree.FromObject(body, dg)
    inv_body = body.matrix_world.inverted()
    for key, inset in SNAP_INSET.items():
        for unit in d["snap"].get(key, []):
            anchor, members = unit
            c = anchor.constraints[0]
            joint = (d["arm"].matrix_world @ d["arm"].pose.bones[c.subtarget].matrix).translation
            for o in members:  # reset last stage's snap offset
                if "snap_off" in o:
                    o.location = Vector(o["base_loc"])
            bpy.context.view_layer.update()
            here = anchor.matrix_world.translation.copy()
            direction = (here - joint).normalized()
            # Outermost hit: compressed growth stages fold some surface inside (e.g. a very
            # short neck inside a big head), so keep casting past each hit.
            origin, ldir, hit = inv_body @ joint, (inv_body.to_3x3() @ direction).normalized(), None
            for _ in range(8):
                h, _, _, _ = bvh.ray_cast(origin, ldir, 10.0)
                if h is None:
                    break
                hit, origin = h, h + ldir * 1e-4
            if hit is None:
                continue
            target = body.matrix_world @ hit - direction * inset
            if key == "eyes" and "--debug" in argv:
                print(f"[snap] {anchor.name} joint={tuple(round(v,3) for v in joint)} here={tuple(round(v,3) for v in here)} hit={tuple(round(v,3) for v in body.matrix_world @ hit)} dist_here={(here-joint).length:.3f} dist_hit={(body.matrix_world @ hit - joint).length:.3f}")
            delta_world = target - here
            m = d["arm"].matrix_world @ d["arm"].pose.bones[c.subtarget].matrix @ c.inverse_matrix
            delta = m.to_3x3().inverted() @ delta_world
            for o in members:
                o["snap_off"] = list(delta)
                o.location = Vector(o["base_loc"]) + delta
    bpy.context.view_layer.update()


def ground(d):
    """Stand the dragon on the floor: shorter legs would otherwise leave it floating."""
    d["arm"].location.z = 0.0
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    ev = d["body"].evaluated_get(dg)
    me = ev.to_mesh()
    low = min((ev.matrix_world @ v.co).z for v in me.vertices)
    ev.to_mesh_clear()
    d["arm"].location.z = -low
    bpy.context.view_layer.update()


# ------------------------------------------------------------------------------ scene & render
def setup_scene():
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_EEVEE"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = scene.render.resolution_y = RES
    scene.render.film_transparent = False
    world = bpy.data.worlds.new("world")
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.20, 0.13, 0.25, 1)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 1.0
    scene.view_settings.view_transform = "Standard"
    bpy.ops.object.light_add(type="SUN", rotation=(math.radians(48), math.radians(12), math.radians(-38)))
    bpy.context.object.data.energy = 4.0
    bpy.ops.mesh.primitive_circle_add(vertices=48, radius=3.4, fill_type="NGON", location=(0, 0.8, 0))
    floor = bpy.context.object
    floor.data.materials.append(toon_material("floor", (0.30, 0.20, 0.34)))
    cam_data = bpy.data.cameras.new("cam")
    cam = link(bpy.data.objects.new("cam", cam_data))
    scene.camera = cam
    return scene, cam


def frame_camera(cam, d, view):
    """Frame the visible dragon from a three-quarter, side or front view."""
    bpy.context.view_layer.update()
    pts = []
    dg = bpy.context.evaluated_depsgraph_get()
    for o in [d["body"]] + d["wings"]:
        ev = o.evaluated_get(dg)
        me = ev.to_mesh()
        pts += [ev.matrix_world @ v.co for v in me.vertices]
        ev.to_mesh_clear()
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    center = (lo + hi) / 2
    size = (hi - lo).length
    dirs = {"three_quarter": Vector((-0.75, -0.95, 0.38)), "side": Vector((-1, 0, 0.12)),
            "front": Vector((-0.15, -1, 0.2))}
    dvec = dirs[view].normalized()
    cam.data.lens = 55
    cam.location = center + dvec * size * 1.55
    cam.rotation_euler = (center - cam.location).to_track_quat("-Z", "Y").to_euler()


def render(path):
    scene = bpy.context.scene
    scene.render.filepath = bpy.path.abspath(path)
    bpy.ops.render.render(write_still=True)


def report(d, stage):
    body = tri_count(d["body"])
    parts = sum(tri_count(o) for objs in d["groups"].values() for o in objs)
    wings = sum(tri_count(o) for o in d["wings"])
    bones = len([b for b in d["arm"].data.bones if not b.name.startswith("wing")])
    print(f"[model] {BREED} {stage}: body {body} + parts {parts} tris; wings {wings} tris; "
          f"body bones {bones}, wing bones {len(d['arm'].data.bones) - bones}")


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene, cam = setup_scene()
    d = build_dragon(BREED)
    views = arg("--views", "three_quarter").split(",")
    for stage in STAGES:
        d["arm"].rotation_euler = (0, 0, 0)
        rest_pose(d, stage)
        if "--sit" in argv:
            sit_pose(d)
        apply_stage(d, stage)
        report(d, stage)
        for view in views:
            frame_camera(cam, d, view)
            render(f"{OUT}_{BREED}_{stage}_{view}.png")
    if EXPORT:
        bpy.ops.export_scene.gltf(filepath=bpy.path.abspath(f"{OUT}_{BREED}.glb"), export_apply=False)


main()
