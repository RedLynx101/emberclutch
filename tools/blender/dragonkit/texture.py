"""The dragon kit's skin texture: body regions, UVs, and a baked RGBA texture per body form
(256x256; 128x128 for LOD1), in the storybook look (D75): soft painted shading, no scales.

Channels (the game picks them per dragon; the variant says which):
  R, G, B = pattern masks (0..1), painted per kind: the kind's texture() hook makes them,
            e.g. R stripes, G spots, B glowing marks for the rare variant
  A       = painted value: soft occlusion x a gentle light from above x a faint brush grain,
            multiplying the colour (1 = full colour)
Every pattern is a 3D procedural in the body's rest pose, baked through the UVs by Cycles, so
it runs straight across UV seams. Faces that aren't skin (the mouth, nostrils) and every
non-body mesh point at the clean corner of the texture (no pattern, value 1).

The texture hook: KIND.texture(tx, nt, p, form) -> {"r": socket, "g": socket, "b": socket
[, "value": socket]} using the node helpers below (tx is this module; p is the form's
"skin" dict: sizes in model units, see SKIN_DEFAULTS).
"""
import math
import sys

import bmesh
import bpy
import numpy as np

# Must match src/core/dragon.hpp BodyRegion; REGION_CLEAN is never dirty.
REGIONS = ["head", "neck", "back", "belly", "left", "right", "tail", "wings"]
REGION_CLEAN = len(REGIONS)
SKIN_AREA = 0.9
CLEAN_UV = (0.97, 0.97)
CLEAN_FROM = 0.93
SKIN_DEFAULTS = dict(ao=0.6, stripe=0.5, spot_cell=0.34, dapple=2.6, grain=9.0, light=0.12, grain_amount=0.06)


# ------------------------------------------------------------------------------ regions
def tag_regions(body, overrides=None):
    """A 'region' integer per vertex from its strongest bone and which way it faces: head,
    neck and tail by bone; the torso splits into back, belly and flanks; limbs belong to
    their side. overrides: {bone name prefix: region}."""
    overrides = overrides or {}
    me = body.data
    names = {g.index: g.name for g in body.vertex_groups}
    attr = me.attributes.get("region") or me.attributes.new("region", "INT", "POINT")
    for v in me.vertices:
        best = max(v.groups, key=lambda g: g.weight, default=None)
        bone = names[best.group] if best else "chest"
        base = bone[:-2] if bone.endswith(("_L", "_R")) else bone
        r = next((reg for pre, reg in overrides.items() if base.startswith(pre)), None)
        if r is None:
            if base in ("head", "snout", "jaw", "eyes"):
                r = "head"
            elif base.startswith("neck"):
                r = "neck"
            elif base.startswith("tail"):
                r = "tail"
            elif base.startswith(("arm", "hand", "leg", "foot")):
                r = "left" if bone.endswith("_L") else "right"
            elif v.normal.z < -0.4:
                r = "belly"
            elif v.normal.z > 0.35:
                r = "back"
            else:
                r = "left" if v.co.x < 0 else "right"
        attr.data[v.index].value = REGIONS.index(r)


# ------------------------------------------------------------------------------ UVs
def unwrap(body):
    """Smart-project the skin faces (material slot 0) into [0, SKIN_AREA]^2; everything else
    sits on the clean corner."""
    for o in bpy.context.selected_objects:
        o.select_set(False)
    body.select_set(True)
    bpy.context.view_layer.objects.active = body
    me = body.data
    if "UVMap" not in me.uv_layers:
        me.uv_layers.new(name="UVMap")
    bpy.ops.object.mode_set(mode="EDIT")
    bm = bmesh.from_edit_mesh(me)
    for f in bm.faces:
        f.select = f.material_index == 0
    bmesh.update_edit_mesh(me)
    bpy.ops.uv.smart_project(angle_limit=math.radians(86), island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")
    uv = me.uv_layers["UVMap"].data
    for p in me.polygons:
        for li in p.loop_indices:
            if p.material_index == 0:
                uv[li].uv = uv[li].uv * SKIN_AREA
            else:
                uv[li].uv = CLEAN_UV


# ------------------------------------------------------------------------------ node helpers
def node(nt, kind, **props):
    n = nt.nodes.new(kind)
    for k, v in props.items():
        setattr(n, k, v)
    return n


def smoothstep(nt, value, lo, hi):
    """0 below lo, 1 above hi (smooth); lo > hi flips it."""
    m = node(nt, "ShaderNodeMapRange", interpolation_type="SMOOTHSTEP", clamp=True)
    nt.links.new(value, m.inputs["Value"])
    m.inputs["From Min"].default_value = lo
    m.inputs["From Max"].default_value = hi
    return m.outputs["Result"]


def math_op(nt, op, a, b=None):
    m = node(nt, "ShaderNodeMath", operation=op)
    for i, x in enumerate((a, b)):
        if x is None:
            continue
        if isinstance(x, (int, float)):
            m.inputs[i].default_value = x
        else:
            nt.links.new(x, m.inputs[i])
    return m.outputs[0]


def mul(nt, a, b):
    return math_op(nt, "MULTIPLY", a, b)


def add(nt, a, b):
    return math_op(nt, "ADD", a, b)


def maxi(nt, a, b):
    return math_op(nt, "MAXIMUM", a, b)


def madd(nt, a, m, c):
    n = node(nt, "ShaderNodeMath", operation="MULTIPLY_ADD")
    nt.links.new(a, n.inputs[0])
    n.inputs[1].default_value = m
    n.inputs[2].default_value = c
    return n.outputs[0]


def const(nt, value):
    v = node(nt, "ShaderNodeValue")
    v.outputs[0].default_value = value
    return v.outputs[0]


def coords(nt):
    """Object-space position (the rest pose)."""
    return node(nt, "ShaderNodeTexCoord").outputs["Object"]


def axis(nt, which):
    """One component (0 x, 1 y, 2 z) of the object-space position."""
    sep = node(nt, "ShaderNodeSeparateXYZ")
    nt.links.new(coords(nt), sep.inputs[0])
    return sep.outputs["XYZ"[which]]


def normal_z(nt):
    geo = node(nt, "ShaderNodeNewGeometry")
    nz = node(nt, "ShaderNodeSeparateXYZ")
    nt.links.new(geo.outputs["Normal"], nz.inputs[0])
    return nz.outputs["Z"]


def top(nt):
    """1 on the back and upper flanks, 0 below."""
    return smoothstep(nt, normal_z(nt), -0.15, 0.45)


def upper(nt):
    """1 down to mid-flank."""
    return smoothstep(nt, normal_z(nt), -0.4, 0.15)


def belly(nt):
    """1 facing down."""
    return smoothstep(nt, normal_z(nt), -0.3, -0.65)


WAVE_PERIOD = 2 * math.pi / 20  # a Wave texture repeats every 2*pi/20 units at scale 1


def stripes(nt, period, direction="Y", distortion=1.2, width=(0.62, 0.74), where=None):
    """Soft bands across the body (direction: the axis they repeat along), on `where`
    (default: the back and upper flanks)."""
    sw = node(nt, "ShaderNodeTexWave", wave_type="BANDS", bands_direction=direction, wave_profile="SIN")
    sw.inputs["Scale"].default_value = WAVE_PERIOD / period
    sw.inputs["Distortion"].default_value = distortion
    sw.inputs["Detail"].default_value = 0.5
    nt.links.new(coords(nt), sw.inputs["Vector"])
    return mul(nt, smoothstep(nt, sw.outputs["Fac"], *width), where if where is not None else top(nt))


def spots(nt, cell, keep=0.44, size=(0.30, 0.22), where=None, seed_offset=0.0):
    """Round spots on about (1 - keep) of the cells of a Voronoi grid of this cell size."""
    sv = node(nt, "ShaderNodeTexVoronoi", feature="F1")
    sv.inputs["Scale"].default_value = 1.0 / cell
    nt.links.new(coords(nt), sv.inputs["Vector"])
    if seed_offset:  # a different grid: shift the lookup
        off = node(nt, "ShaderNodeVectorMath", operation="ADD")
        nt.links.new(coords(nt), off.inputs[0])
        off.inputs[1].default_value = (seed_offset, seed_offset * 0.7, seed_offset * 1.3)
        nt.links.new(off.outputs[0], sv.inputs["Vector"])
    rnd = node(nt, "ShaderNodeSeparateColor")
    nt.links.new(sv.outputs["Color"], rnd.inputs[0])
    kept = smoothstep(nt, rnd.outputs["Red"], keep - 0.02, keep + 0.02)
    dot = smoothstep(nt, sv.outputs["Distance"], *size)
    return mul(nt, mul(nt, dot, kept), where if where is not None else upper(nt))


def blotches(nt, scale, threshold=(0.52, 0.60), detail=2.0, where=None):
    """Soft irregular blotches (dapple, moss, clouds)."""
    n = node(nt, "ShaderNodeTexNoise")
    n.inputs["Scale"].default_value = scale
    n.inputs["Detail"].default_value = detail
    nt.links.new(coords(nt), n.inputs["Vector"])
    return mul(nt, smoothstep(nt, n.outputs["Fac"], *threshold), where if where is not None else upper(nt))


def band(nt, which, lo, hi, soft=0.05):
    """1 between lo and hi along an axis (0 x, 1 y, 2 z), soft edges."""
    a = axis(nt, which)
    return mul(nt, smoothstep(nt, a, lo - soft, lo + soft), smoothstep(nt, a, hi + soft, hi - soft))


def near(nt, point, radius, soft=0.5):
    """1 within radius of a point (object space), fading over soft * radius."""
    vec = node(nt, "ShaderNodeVectorMath", operation="DISTANCE")
    nt.links.new(coords(nt), vec.inputs[0])
    vec.inputs[1].default_value = point
    return smoothstep(nt, vec.outputs["Value"], radius, radius * (1 - soft))


def cracks(nt, cell, width=0.03, warp=0.35):
    """Thin wandering cracks between big cells (glowing veins, geode seams)."""
    wn = node(nt, "ShaderNodeTexNoise")
    wn.inputs["Scale"].default_value = 1.0 / (cell * 0.7)
    mix = node(nt, "ShaderNodeMix", data_type="VECTOR")
    mix.inputs["Factor"].default_value = warp
    nt.links.new(coords(nt), wn.inputs["Vector"])
    nt.links.new(coords(nt), mix.inputs["A"])
    nt.links.new(wn.outputs["Color"], mix.inputs["B"])
    v = node(nt, "ShaderNodeTexVoronoi", feature="DISTANCE_TO_EDGE")
    v.inputs["Scale"].default_value = 1.0 / cell
    nt.links.new(mix.outputs["Result"], v.inputs["Vector"])
    return smoothstep(nt, v.outputs["Distance"], width, 0.0)


def cells(nt, cell, edge=0.2):
    """Cell edges darkening (plates, scutes): 1 - edge at the edges, 1 inside."""
    v = node(nt, "ShaderNodeTexVoronoi", feature="DISTANCE_TO_EDGE")
    v.inputs["Scale"].default_value = 1.0 / cell
    nt.links.new(coords(nt), v.inputs["Vector"])
    return madd(nt, smoothstep(nt, v.outputs["Distance"], 0.0, 0.16), edge, 1.0 - edge)


def grain(nt, scale, amount):
    """A faint brush grain: 1 +- amount, low frequency (the storybook paint)."""
    n = node(nt, "ShaderNodeTexNoise")
    n.inputs["Scale"].default_value = scale
    n.inputs["Detail"].default_value = 3.0
    n.inputs["Roughness"].default_value = 0.55
    nt.links.new(coords(nt), n.inputs["Vector"])
    return madd(nt, n.outputs["Fac"], 2 * amount, 1.0 - amount)


def light_from_above(nt, amount):
    """A gentle painted light: 1 on top, 1 - amount underneath."""
    return madd(nt, smoothstep(nt, normal_z(nt), -0.9, 0.7), amount, 1.0 - amount)


def default_channels(nt, p):
    """The classic three patterns: stripes (R), spots (G), dapple (B)."""
    return {"r": stripes(nt, p["stripe"]), "g": spots(nt, p["spot_cell"]), "b": blotches(nt, p["dapple"])}


# ------------------------------------------------------------------------------ bake
def _bake(obj, image, kind, value=None, samples=1):
    mat = bpy.data.materials.new("bake")
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = node(nt, "ShaderNodeOutputMaterial")
    img = node(nt, "ShaderNodeTexImage")
    img.image = image
    nt.nodes.active = img
    if kind == "EMIT":
        em = node(nt, "ShaderNodeEmission")
        nt.links.new(value(nt), em.inputs["Color"])
        nt.links.new(em.outputs[0], out.inputs["Surface"])
    else:
        nt.links.new(node(nt, "ShaderNodeBsdfDiffuse").outputs[0], out.inputs["Surface"])
    saved = [s.material for s in obj.material_slots]
    for s in obj.material_slots:
        s.material = mat
    scene = bpy.context.scene
    was = scene.render.engine
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = samples
    scene.render.bake.margin = 6
    for o in bpy.context.selected_objects:
        o.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.bake(type=kind, use_clear=True)
    for s, m in zip(obj.material_slots, saved):
        s.material = m
    bpy.data.materials.remove(mat)
    scene.render.engine = was
    px = np.array(image.pixels[:], dtype=np.float32).reshape(image.size[1], image.size[0], 4)
    return px[:, :, 0].copy()


def bake_skin(body, form, size, kind):
    """The RGBA texture as a float array (rows bottom-up, like Blender images)."""
    f = kind.FORMS[form]
    p = dict(SKIN_DEFAULTS)
    p.update(f.get("skin", {}))
    hidden = []
    for o in bpy.context.scene.objects:
        if o is not body and not o.hide_render:
            o.hide_render = True
            hidden.append(o)
    world = bpy.context.scene.world
    try:
        world.light_settings.distance = p["ao"]
    except AttributeError:
        pass

    tx = sys.modules[__name__]

    def channels(nt):
        ch = kind.texture(tx, nt, p, form) if hasattr(kind, "texture") else default_channels(nt, p)
        if "value" not in ch:
            ch["value"] = mul(nt, grain(nt, p["grain"], p["grain_amount"]), light_from_above(nt, p["light"]))
        return ch

    image = bpy.data.images.new("skin_bake", size, size, alpha=False, float_buffer=True)
    rgba = np.zeros((size, size, 4), dtype=np.float32)
    for c, name in enumerate(("r", "g", "b")):
        rgba[:, :, c] = _bake(body, image, "EMIT", lambda nt, n=name: channels(nt)[n])
    value = _bake(body, image, "EMIT", lambda nt: channels(nt)["value"])
    ao = _blur(_bake(body, image, "AO", samples=64))
    rgba[:, :, 3] = value * (0.72 + 0.28 * ao)
    for o in hidden:
        o.hide_render = False
    bpy.data.images.remove(image)
    lo = int(CLEAN_FROM * size)
    rgba[lo:, lo:, :3] = 0.0
    rgba[lo:, lo:, 3] = 1.0
    return np.clip(rgba, 0.0, 1.0)


def _blur(a):
    p = np.pad(a, 1, mode="edge")
    return sum(p[1 + dy:1 + dy + a.shape[0], 1 + dx:1 + dx + a.shape[1]] for dy in (-1, 0, 1) for dx in (-1, 0, 1)) / 9.0


def save_png(rgba, path):
    size = rgba.shape[0]
    img = bpy.data.images.new("skin_png", size, size, alpha=True)
    img.colorspace_settings.name = "Non-Color"
    img.pixels[:] = rgba.ravel()
    img.filepath_raw = str(path)
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


# ------------------------------------------------------------------------------ preview
CHANNEL = {"r": 0, "g": 1, "b": 2}


def preview_material(body_mat, rgba, colors):
    """Put the baked skin on the body's toon material for review renders, as the game shades
    it: colour -> the variant's pattern channel in its pattern colour -> x painted value,
    before the toon light; a glow channel adds light after it."""
    size = rgba.shape[0]
    img = bpy.data.images.new("skin_preview", size, size, alpha=True, float_buffer=True)
    img.colorspace_settings.name = "Non-Color"
    img.pixels[:] = rgba.ravel()
    img.pack()
    nt = body_mat.node_tree
    mulnode = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeMix" and n.blend_type == "MULTIPLY")
    col = mulnode.inputs["A"].links[0].from_socket
    t = node(nt, "ShaderNodeTexImage", interpolation="Linear")
    t.image = img
    uv = node(nt, "ShaderNodeUVMap", uv_map="UVMap")
    nt.links.new(uv.outputs["UV"], t.inputs["Vector"])
    sep = node(nt, "ShaderNodeSeparateColor")
    nt.links.new(t.outputs["Color"], sep.inputs[0])
    pc = colors.get("pattern_channel")
    if pc in CHANNEL:
        pat = node(nt, "ShaderNodeMix", data_type="RGBA")
        nt.links.new(sep.outputs[("Red", "Green", "Blue")[CHANNEL[pc]]], pat.inputs["Factor"])
        nt.links.new(col, pat.inputs["A"])
        pat.inputs["B"].default_value = (*colors["pattern"], 1)
        col = pat.outputs["Result"]
    det = node(nt, "ShaderNodeMix", data_type="RGBA", blend_type="MULTIPLY")
    det.inputs["Factor"].default_value = 1.0
    nt.links.new(col, det.inputs["A"])
    nt.links.new(t.outputs["Alpha"], det.inputs["B"])
    nt.links.new(det.outputs["Result"], mulnode.inputs["A"])
    gc = colors.get("glow_channel")
    if gc in CHANNEL:
        addn = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeMix" and n.blend_type == "ADD")
        glow = node(nt, "ShaderNodeMix", data_type="RGBA", blend_type="ADD")
        glow.inputs["Factor"].default_value = 1.0
        vein = node(nt, "ShaderNodeMix", data_type="RGBA", blend_type="MULTIPLY")
        vein.inputs["Factor"].default_value = 1.0
        nt.links.new(sep.outputs[("Red", "Green", "Blue")[CHANNEL[gc]]], vein.inputs["A"])
        vein.inputs["B"].default_value = (*[min(1.0, c * 1.3) for c in colors["glow"]], 1)
        nt.links.new(mulnode.outputs["Result"], glow.inputs["A"])
        nt.links.new(vein.outputs["Result"], glow.inputs["B"])
        nt.links.new(glow.outputs["Result"], addn.inputs["A"])
