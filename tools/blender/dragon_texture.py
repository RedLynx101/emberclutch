"""The dragon's skin texture (Alpha 1 WP2, review R2): body regions, UVs, and a baked
256x256 RGBA texture per body form (128x128 for LOD1).

Channels (src/app/render3d.cpp picks them on the GPU per dragon):
  R = Stripes mask, G = Spots mask, B = Dapple mask (the Pattern gene picks one; Solid none),
  A = scale detail x ambient occlusion (multiplies the colour).
Every pattern is a 3D procedural in the body's rest pose, baked through the UVs by Cycles,
so patterns and scales run straight across UV seams. Faces that aren't skin (the mouth,
nostrils, pupils) and every non-body mesh point at a reserved clean corner of the texture
(no pattern, detail 1). Dust (D46) isn't in the texture: each vertex carries its body
region (dragon.hpp BodyRegion) and the game looks up that region's dirt per dragon.
"""
import math
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector

# Must match src/core/dragon.hpp BodyRegion; REGION_CLEAN is never dirty.
REGIONS = ["head", "neck", "back", "belly", "left", "right", "tail", "wings"]
REGION_CLEAN = len(REGIONS)
SKIN_AREA = 0.9                  # skin islands fill [0, 0.9]^2; the top-right strip stays clean
CLEAN_UV = (0.97, 0.97)          # the middle of the clean corner
CLEAN_FROM = 0.93                # texels past this in u and v are forced clean

# Per form: sizes in model units (the hatchling is modelled ~2.5x smaller than the adult).
SKIN = {
    "grown": dict(scale_cell=0.11, plate=0.19, stripe=0.5, spot_cell=0.34, dapple=2.6, ao=0.6),
    "hatchling": dict(scale_cell=0.05, plate=0.085, stripe=0.22, spot_cell=0.15, dapple=5.5, ao=0.25),
}


# Review R5 styles (dragon_model.py STYLE): v1 bolder scale plates, a banded belly and a
# darker spine; v3 veins in place of the dapple channel (B), which the game lights up.
_ARGV = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
STYLE = _ARGV[_ARGV.index("--style") + 1] if "--style" in _ARGV else "current"
SKIN_STYLE = {"v1": dict(cell=1.8, edge=0.42, plates=0.4, spine=0.36),
              "v3": dict(cell=0.9, edge=0.26, veins=True)}.get(STYLE, {})


# ------------------------------------------------------------------------------ regions
def tag_regions(body):
    """A 'region' integer per vertex from its strongest bone and which way it faces: head,
    neck and tail by bone; the torso splits into back (facing up), belly (facing down) and
    the flanks; legs belong to their side."""
    me = body.data
    names = {g.index: g.name for g in body.vertex_groups}
    attr = me.attributes.get("region") or me.attributes.new("region", "INT", "POINT")
    for v in me.vertices:
        best = max(v.groups, key=lambda g: g.weight, default=None)
        bone = names[best.group] if best else "chest"
        base = bone[:-2] if bone.endswith(("_L", "_R")) else bone
        if base in ("head", "snout", "jaw", "eyes"):
            r = "head"
        elif base.startswith("neck"):
            r = "neck"
        elif base.startswith("tail"):
            r = "tail"
        elif base in ("arm_up", "arm_lo", "hand", "leg_up", "leg_lo", "foot"):
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
    # A wide angle limit: few, large islands (fewer seam vertices on these low-poly bodies);
    # the patterns are 3D procedurals baked through the UVs, so stretching barely shows.
    bpy.ops.uv.smart_project(angle_limit=math.radians(86), island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")
    uv = me.uv_layers["UVMap"].data
    for p in me.polygons:
        for li in p.loop_indices:
            if p.material_index == 0:
                uv[li].uv = uv[li].uv * SKIN_AREA
            else:
                uv[li].uv = CLEAN_UV


# ------------------------------------------------------------------------------ bake
def _node(nt, kind, **props):
    n = nt.nodes.new(kind)
    for k, v in props.items():
        setattr(n, k, v)
    return n


def _smooth(nt, value, lo, hi):
    """0 below lo, 1 above hi (smoothstep); lo > hi flips it."""
    m = _node(nt, "ShaderNodeMapRange", interpolation_type="SMOOTHSTEP", clamp=True)
    nt.links.new(value, m.inputs["Value"])
    m.inputs["From Min"].default_value = lo
    m.inputs["From Max"].default_value = hi
    return m.outputs["Result"]


def _math(nt, op, a, b=None):
    m = _node(nt, "ShaderNodeMath", operation=op)
    for i, x in enumerate((a, b)):
        if x is None:
            continue
        if isinstance(x, (int, float)):
            m.inputs[i].default_value = x
        else:
            nt.links.new(x, m.inputs[i])
    return m.outputs[0]


WAVE_PERIOD = 2 * math.pi / 20  # a Blender Wave texture repeats every 2*pi/20 units at scale 1


def _madd(nt, a, mul, add):
    m = _node(nt, "ShaderNodeMath", operation="MULTIPLY_ADD")
    nt.links.new(a, m.inputs[0])
    m.inputs[1].default_value = mul
    m.inputs[2].default_value = add
    return m.outputs[0]


def _channels(nt, p):
    """Node outputs for each channel: stripes, spots, dapple, scales (0..1)."""
    coord = _node(nt, "ShaderNodeTexCoord").outputs["Object"]
    geo = _node(nt, "ShaderNodeNewGeometry")
    nz = _node(nt, "ShaderNodeSeparateXYZ")
    nt.links.new(geo.outputs["Normal"], nz.inputs[0])
    nz = nz.outputs["Z"]
    top = _smooth(nt, nz, -0.15, 0.45)        # the back and upper flanks
    upper = _smooth(nt, nz, -0.4, 0.15)       # down to mid-flank
    belly = _smooth(nt, nz, -0.3, -0.65)      # facing down

    # Scales: cells darken toward their edges; belly plates run across the body.
    vor = _node(nt, "ShaderNodeTexVoronoi", feature="DISTANCE_TO_EDGE")
    vor.inputs["Scale"].default_value = 1.0 / (p["scale_cell"] * SKIN_STYLE.get("cell", 1.0))
    nt.links.new(coord, vor.inputs["Vector"])
    edge = SKIN_STYLE.get("edge", 0.18)
    cells = _madd(nt, _smooth(nt, vor.outputs["Distance"], 0.0, 0.16), edge, 1.0 - edge)
    wave = _node(nt, "ShaderNodeTexWave", wave_type="BANDS", bands_direction="Y", wave_profile="SAW")
    wave.inputs["Scale"].default_value = WAVE_PERIOD / p["plate"]
    wave.inputs["Distortion"].default_value = 0.0
    nt.links.new(coord, wave.inputs["Vector"])
    band = SKIN_STYLE.get("plates", 0.16)
    plates = _madd(nt, _smooth(nt, wave.outputs["Fac"], 0.97, 0.80), band, 1.0 - band)
    scales = _node(nt, "ShaderNodeMix", data_type="FLOAT")
    nt.links.new(belly, scales.inputs["Factor"])
    nt.links.new(cells, scales.inputs["A"])
    nt.links.new(plates, scales.inputs["B"])
    scales = scales.outputs["Result"]
    if SKIN_STYLE.get("spine"):  # v1: the back darker along the spine
        ridge = _smooth(nt, nz, 0.5, 0.95)
        scales = _math(nt, "MULTIPLY", scales, _madd(nt, ridge, -SKIN_STYLE["spine"], 1.0))

    # Stripes: soft bands across the spine, on the back and upper flanks.
    sw = _node(nt, "ShaderNodeTexWave", wave_type="BANDS", bands_direction="Y", wave_profile="SIN")
    sw.inputs["Scale"].default_value = WAVE_PERIOD / p["stripe"]
    sw.inputs["Distortion"].default_value = 1.2
    sw.inputs["Detail"].default_value = 0.5
    nt.links.new(coord, sw.inputs["Vector"])
    stripes = _math(nt, "MULTIPLY", _smooth(nt, sw.outputs["Fac"], 0.62, 0.74), top)

    # Spots: round spots on about half the cells.
    sv = _node(nt, "ShaderNodeTexVoronoi", feature="F1")
    sv.inputs["Scale"].default_value = 1.0 / p["spot_cell"]
    nt.links.new(coord, sv.inputs["Vector"])
    rnd = _node(nt, "ShaderNodeSeparateColor")
    nt.links.new(sv.outputs["Color"], rnd.inputs[0])
    keep = _smooth(nt, rnd.outputs["Red"], 0.42, 0.46)
    dot = _smooth(nt, sv.outputs["Distance"], 0.30, 0.22)
    spots = _math(nt, "MULTIPLY", _math(nt, "MULTIPLY", dot, keep), upper)

    # Dapple: soft blotches.
    noise = _node(nt, "ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = p["dapple"]
    noise.inputs["Detail"].default_value = 2.0
    nt.links.new(coord, noise.inputs["Vector"])
    dapple = _math(nt, "MULTIPLY", _smooth(nt, noise.outputs["Fac"], 0.52, 0.60), upper)
    if SKIN_STYLE.get("veins"):  # v3: glowing cracks between big scales, wandering a little
        warp = _node(nt, "ShaderNodeTexNoise")
        warp.inputs["Scale"].default_value = 1.0 / (p["scale_cell"] * 3.0)
        mix = _node(nt, "ShaderNodeMix", data_type="VECTOR")
        mix.inputs["Factor"].default_value = 0.35
        nt.links.new(coord, warp.inputs["Vector"])
        nt.links.new(coord, mix.inputs["A"])
        nt.links.new(warp.outputs["Color"], mix.inputs["B"])
        cracks = _node(nt, "ShaderNodeTexVoronoi", feature="DISTANCE_TO_EDGE")
        cracks.inputs["Scale"].default_value = 1.0 / (p["scale_cell"] * 4.2)
        nt.links.new(mix.outputs["Result"], cracks.inputs["Vector"])
        veins = _smooth(nt, cracks.outputs["Distance"], 0.028, 0.0)  # thin, bright cracks
        dapple = _math(nt, "MULTIPLY", veins, _madd(nt, upper, 0.6, 0.4))  # fainter on the belly
    return {"stripes": stripes, "spots": spots, "dapple": dapple, "scales": scales}


def _bake(obj, image, kind, value=None, samples=1):
    """Bake one channel (an emission value, or 'AO') into `image` through every material
    slot of `obj` (each slot gets the same temporary material)."""
    mat = bpy.data.materials.new("bake")
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = _node(nt, "ShaderNodeOutputMaterial")
    img = _node(nt, "ShaderNodeTexImage")
    img.image = image
    nt.nodes.active = img
    if kind == "EMIT":
        em = _node(nt, "ShaderNodeEmission")
        nt.links.new(value(nt), em.inputs["Color"])
        nt.links.new(em.outputs[0], out.inputs["Surface"])
    else:
        nt.links.new(_node(nt, "ShaderNodeBsdfDiffuse").outputs[0], out.inputs["Surface"])
    saved = [s.material for s in obj.material_slots]
    for s in obj.material_slots:
        s.material = mat
    scene = bpy.context.scene
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
    px = np.array(image.pixels[:], dtype=np.float32).reshape(image.size[1], image.size[0], 4)
    return px[:, :, 0].copy()


def bake_skin(body, form, size):
    """The RGBA texture as a float array (rows bottom-up, like Blender images)."""
    p = SKIN[form]
    hidden = []
    for o in bpy.context.scene.objects:  # only the body occludes itself
        if o is not body and not o.hide_render:
            o.hide_render = True
            hidden.append(o)
    world = bpy.context.scene.world
    try:
        world.light_settings.distance = p["ao"]
    except AttributeError:
        pass
    image = bpy.data.images.new("skin_bake", size, size, alpha=False, float_buffer=True)
    rgba = np.zeros((size, size, 4), dtype=np.float32)
    for c, name in enumerate(("stripes", "spots", "dapple")):
        rgba[:, :, c] = _bake(body, image, "EMIT", lambda nt, n=name: _channels(nt, p)[n])
    scales = _bake(body, image, "EMIT", lambda nt: _channels(nt, p)["scales"])
    ao = _blur(_bake(body, image, "AO", samples=64))
    rgba[:, :, 3] = scales * (0.68 + 0.32 * ao)
    for o in hidden:
        o.hide_render = False
    bpy.data.images.remove(image)
    lo = int(CLEAN_FROM * size)
    rgba[lo:, lo:, :3] = 0.0  # the clean corner (rows are bottom-up, so this is the top right)
    rgba[lo:, lo:, 3] = 1.0
    return np.clip(rgba, 0.0, 1.0)


def _blur(a):
    """A 3x3 box blur: takes the grain out of the occlusion bake (the bake margin keeps
    island edges from pulling in empty texels)."""
    p = np.pad(a, 1, mode="edge")
    return sum(p[1 + dy:1 + dy + a.shape[0], 1 + dx:1 + dx + a.shape[1]] for dy in (-1, 0, 1) for dx in (-1, 0, 1)) / 9.0


def save_png(rgba, path):
    size = rgba.shape[0]
    img = bpy.data.images.new("skin_png", size, size, alpha=True)
    img.colorspace_settings.name = "Non-Color"  # the channels are data, stored as-is
    img.pixels[:] = rgba.ravel()
    img.filepath_raw = str(path)
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


# ------------------------------------------------------------------------------ preview
PATTERN_CHANNEL = {"stripes": 0, "spots": 1, "dapple": 2, "veins": 2}


def preview_material(body_mat, rgba, pattern, pattern_color, dirt=0.0):
    """Put the baked skin on the body's toon material for Blender renders, as the game
    shades it: colour -> pattern -> dust -> x scale detail, before the toon light."""
    size = rgba.shape[0]
    img = bpy.data.images.new("skin_preview", size, size, alpha=True, float_buffer=True)
    img.colorspace_settings.name = "Non-Color"  # before the pixels: changing it regenerates the image
    img.pixels[:] = rgba.ravel()
    img.pack()
    nt = body_mat.node_tree
    mul = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeMix" and n.blend_type == "MULTIPLY")
    col = mul.inputs["A"].links[0].from_socket
    tex = _node(nt, "ShaderNodeTexImage", interpolation="Linear")
    tex.image = img
    uv = _node(nt, "ShaderNodeUVMap", uv_map="UVMap")
    nt.links.new(uv.outputs["UV"], tex.inputs["Vector"])
    sep = _node(nt, "ShaderNodeSeparateColor")
    nt.links.new(tex.outputs["Color"], sep.inputs[0])
    if pattern in PATTERN_CHANNEL and pattern != "veins":  # (v3's veins glow instead, below)
        pat = _node(nt, "ShaderNodeMix", data_type="RGBA")
        nt.links.new(sep.outputs[("Red", "Green", "Blue")[PATTERN_CHANNEL[pattern]]], pat.inputs["Factor"])
        nt.links.new(col, pat.inputs["A"])
        pat.inputs["B"].default_value = (*pattern_color, 1)
        col = pat.outputs["Result"]
    if dirt > 0:
        dust = _node(nt, "ShaderNodeMix", data_type="RGBA")
        dust.inputs["Factor"].default_value = DIRT_MAX * dirt
        nt.links.new(col, dust.inputs["A"])
        dust.inputs["B"].default_value = (*DIRT_COLOR, 1)
        col = dust.outputs["Result"]
    det = _node(nt, "ShaderNodeMix", data_type="RGBA", blend_type="MULTIPLY")
    det.inputs["Factor"].default_value = 1.0
    nt.links.new(col, det.inputs["A"])
    nt.links.new(tex.outputs["Alpha"], det.inputs["B"])
    nt.links.new(det.outputs["Result"], mul.inputs["A"])
    if pattern == "veins":  # v3: the cracks glow, after the light (as the game adds them)
        add = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeMix" and n.blend_type == "ADD")
        glow = _node(nt, "ShaderNodeMix", data_type="RGBA", blend_type="ADD")
        glow.inputs["Factor"].default_value = 1.0
        vein = _node(nt, "ShaderNodeMix", data_type="RGBA", blend_type="MULTIPLY")
        vein.inputs["Factor"].default_value = 1.0
        nt.links.new(sep.outputs["Blue"], vein.inputs["A"])
        vein.inputs["B"].default_value = (*[min(1.0, c * 1.4) for c in pattern_color], 1)
        nt.links.new(mul.outputs["Result"], glow.inputs["A"])
        nt.links.new(vein.outputs["Result"], glow.inputs["B"])
        nt.links.new(glow.outputs["Result"], add.inputs["A"])


DIRT_COLOR = (0.58, 0.52, 0.46)  # dusty grey-brown (src/app/render3d.cpp kDirtColor)
DIRT_MAX = 0.4                   # a fully dusty region moves this far toward it
