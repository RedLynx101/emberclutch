"""The app's interim icon and HOME Menu banner (Alpha 1 WP11), rendered from our own dragon
model in place of the AI-concept placeholder: the baby Ember's face on an ember glow (the
48x48 icon) and the hatchling sitting beside the wordmark (the 256x128 banner). Alpha 2
replaces both with the emblem and an animated 3D banner (D48, D50).

  blender -b -P tools/blender/app_icon.py -- [--out-dir assets]
"""
import math
import os
import sys

import bpy
from mathutils import Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dragon_model as dm  # noqa: E402
import dragon_texture  # noqa: E402

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT_DIR = os.path.abspath(dm.arg("--out-dir", os.path.join(ROOT, "assets")))
WORK = os.path.join(ROOT, "build", "icon")
FONT = os.path.join(ROOT, "assets", "fonts", "cinzel-decorative", "CinzelDecorative-Bold.ttf")
GOLD = (0.96, 0.77, 0.32)   # theme kClutchGold
EMBER = (0.95, 0.33, 0.08)  # an ember glow (theme kEmber, deeper)


def build():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene, cam = dm.setup_scene()
    scene.eevee.taa_render_samples = 64
    # The den's dusk plum (theme kDenPlum), and the colours as rich as they are on the 3DS.
    bg = scene.world.node_tree.nodes["Background"].inputs["Color"]
    bg.default_value = (0.034, 0.016, 0.052, 1)
    scene.view_settings.gamma = 0.72  # < 1 darkens the midtones: richer colours
    bpy.data.objects["floor"].hide_render = True
    form, t = dm.STAGE["hatchling"]
    d = dm.build_dragon("ember", form)
    dm.pose_stage(d, t, d["breed"]["build"])
    b = d["breed"]
    rgba = dragon_texture.bake_skin(d["body"], form, 256)
    dragon_texture.preview_material(d["mats"]["body"], rgba, b["pattern"], b["pattern_color"], 0.0)
    return scene, cam, d


def glow_material():
    """Emission fading out from the centre: a soft ember glow on a disc."""
    mat = bpy.data.materials.new("glow")
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    coord = nt.nodes.new("ShaderNodeTexCoord")
    grad = nt.nodes.new("ShaderNodeTexGradient")
    grad.gradient_type = "SPHERICAL"
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].position = 0.0
    ramp.color_ramp.elements[1].position = 1.0
    emit = nt.nodes.new("ShaderNodeEmission")
    emit.inputs["Color"].default_value = (*EMBER, 1)
    emit.inputs["Strength"].default_value = 1.5
    clear = nt.nodes.new("ShaderNodeBsdfTransparent")
    mix = nt.nodes.new("ShaderNodeMixShader")
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(coord.outputs["Object"], grad.inputs["Vector"])
    nt.links.new(grad.outputs["Fac"], ramp.inputs["Fac"])
    nt.links.new(ramp.outputs["Color"], mix.inputs["Fac"])
    nt.links.new(clear.outputs[0], mix.inputs[1])
    nt.links.new(emit.outputs[0], mix.inputs[2])
    nt.links.new(mix.outputs[0], out.inputs["Surface"])
    if hasattr(mat, "surface_render_method"):
        mat.surface_render_method = "BLENDED"
    else:
        mat.blend_method = "BLEND"
    return mat


def glow_behind(cam, center, radius, depth):
    """A glow disc `depth` behind `center` (clear of the subject), facing the camera; its
    radius is what it appears to be at `center`'s distance."""
    away = (center - cam.location).normalized()
    dist = (center - cam.location).length
    radius *= (dist + depth) / dist  # further back, larger, to look the same size
    bpy.ops.mesh.primitive_circle_add(vertices=64, radius=radius, fill_type="NGON",
                                      location=center + away * depth)
    disc = bpy.context.object
    disc.rotation_euler = (-away).to_track_quat("Z", "Y").to_euler()
    disc.data.materials.append(glow_material())
    return disc


def render_to(path, width, height, size):
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y = width, height
    os.makedirs(WORK, exist_ok=True)
    big = os.path.join(WORK, "big_" + os.path.basename(path))
    scene.render.filepath = big
    bpy.ops.render.render(write_still=True)
    img = bpy.data.images.load(big)
    img.scale(*size)  # rendered 4x, averaged down: clean edges at icon size
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    print(f"[icon] wrote {path} ({size[0]}x{size[1]})")


def head_center(d):
    pts = dm.head_points(d)
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    return (lo + hi) / 2, (hi - lo).length


def icon(cam, d):
    dm.frame_camera(cam, [d], "portrait")
    cam.location += (cam.location - head_center(d)[0]) * 0.22  # a touch wider: horns and chin in
    center, size = head_center(d)
    disc = glow_behind(cam, center, size * 0.75, size * 3.0)
    render_to(os.path.join(OUT_DIR, "icon.png"), 192, 192, (48, 48))
    bpy.data.objects.remove(disc, do_unlink=True)


def banner(cam, d):
    dm.frame_camera(cam, [d], "three_quarter", lens=55, margin=2.3)
    cam.data.shift_x = 0.22  # the dragon on the left, the wordmark on the right
    target = sum(dm.visible_points([d]), Vector()) / len(dm.visible_points([d]))
    reach = max((p - target).length for p in dm.visible_points([d]))
    glow_behind(cam, target, reach * 1.1, reach * 3.0)
    # The wordmark, parented to the camera so it sits flat in the frame.
    dist = (cam.location - target).length * 0.8
    width = dist * cam.data.sensor_width / cam.data.lens  # visible width at that distance
    curve = bpy.data.curves.new("wordmark", "FONT")
    curve.body = "Emberclutch"
    if os.path.exists(FONT):
        curve.font = bpy.data.fonts.load(FONT)
    curve.align_x, curve.align_y = "CENTER", "CENTER"
    curve.size = width * 0.052
    text = bpy.data.objects.new("wordmark", curve)
    bpy.context.scene.collection.objects.link(text)
    text.parent = cam
    text.location = (width * 0.47, width * 0.03, -dist)
    mat = bpy.data.materials.new("gold")
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    emit = nt.nodes.new("ShaderNodeEmission")
    emit.inputs["Color"].default_value = (*GOLD, 1)
    emit.inputs["Strength"].default_value = 1.4
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(emit.outputs[0], out.inputs["Surface"])
    curve.materials.append(mat)
    render_to(os.path.join(OUT_DIR, "banner.png"), 1024, 512, (256, 128))


def main():
    _, cam, d = build()
    icon(cam, d)
    banner(cam, d)


if __name__ == "__main__":
    main()
