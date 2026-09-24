"""The app icon (Alpha 2 WP10, D48): a designed emblem, independent of the dragons' style. A
glowing heart inside an ember-lit egg with a curl of wing around it, on dusk plum lit by an
ember glow, in the game's palette (src/app/theme.hpp). Square and full-bleed like other 3DS
icons: the first one, a round badge with a gold rim, left black corners on the HOME Menu
(Noah, 2026-09-24). Rendered big and averaged down, so it reads at 48x48.

  blender -b -P tools/blender/emblem.py -- [--out-dir assets] [--review build/review]

Writes <out-dir>/icon.png (48x48) and, for review R5, <review>/emblem_256.png.
"""
import math
import os
import sys

import bmesh
import bpy
from mathutils import Vector

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(name, default):
    return argv[argv.index(name) + 1] if name in argv else default


OUT_DIR = os.path.abspath(arg("--out-dir", os.path.join(ROOT, "assets")))
REVIEW = os.path.abspath(arg("--review", os.path.join(ROOT, "build", "review")))
WORK = os.path.join(ROOT, "build", "icon")

PLUM = (0.075, 0.035, 0.095)     # theme kDenPlum, deep
DUSK = (0.22, 0.11, 0.24)
GOLD = (0.96, 0.72, 0.25)        # kClutchGold
SHELL = (0.98, 0.90, 0.78)       # kShell
EMBER = (1.0, 0.36, 0.08)        # kEmber
HEART = (0.95, 0.26, 0.07)
WING = (0.52, 0.12, 0.10)
WING_WEB = (0.92, 0.38, 0.14)


# ------------------------------------------------------------------------------ materials
def emission(name, color, strength):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    e = nt.nodes.new("ShaderNodeEmission")
    e.inputs["Color"].default_value = (*color, 1)
    e.inputs["Strength"].default_value = strength
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(e.outputs[0], out.inputs["Surface"])
    return m


def principled(name, color, rough=0.45, glow=None, glow_strength=0.0):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Roughness"].default_value = rough
    if glow:
        key = "Emission Color" if "Emission Color" in bsdf.inputs else "Emission"
        bsdf.inputs[key].default_value = (*glow, 1)
        bsdf.inputs["Emission Strength"].default_value = glow_strength
    return m


def halo_material():
    """An ember glow fading out into the badge's plum."""
    m = bpy.data.materials.new("halo")
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    coord = nt.nodes.new("ShaderNodeTexCoord")
    mapping = nt.nodes.new("ShaderNodeMapping")
    mapping.inputs["Scale"].default_value = (0.78, 0.78, 0.78)
    grad = nt.nodes.new("ShaderNodeTexGradient")
    grad.gradient_type = "SPHERICAL"
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].color = (*DUSK, 1)
    ramp.color_ramp.elements[0].position = 0.05
    ramp.color_ramp.elements[1].position = 0.62
    ramp.color_ramp.elements[1].color = (0.95, 0.34, 0.08, 1)
    e = nt.nodes.new("ShaderNodeEmission")
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(coord.outputs["Object"], mapping.inputs["Vector"])
    nt.links.new(mapping.outputs["Vector"], grad.inputs["Vector"])
    nt.links.new(grad.outputs["Fac"], ramp.inputs["Fac"])
    nt.links.new(ramp.outputs["Color"], e.inputs["Color"])
    nt.links.new(e.outputs[0], out.inputs["Surface"])
    return m


def egg_glow_material():
    """The shell lit from within: cream at the rim, warming to an ember glow where the heart
    sits (a radial gradient in object space, centred low on the egg)."""
    m = bpy.data.materials.new("shell")
    m.use_nodes = True
    nt = m.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = 0.35
    coord = nt.nodes.new("ShaderNodeTexCoord")
    mapping = nt.nodes.new("ShaderNodeMapping")
    mapping.inputs["Location"].default_value = (-0.03, 0, 0.1)
    mapping.inputs["Scale"].default_value = (1.45, 0.0, 1.15)  # across the face: y ignored
    grad = nt.nodes.new("ShaderNodeTexGradient")
    grad.gradient_type = "SPHERICAL"
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].position = 0.12
    ramp.color_ramp.elements[0].color = (*SHELL, 1)
    ramp.color_ramp.elements[1].position = 0.78
    ramp.color_ramp.elements[1].color = (1.0, 0.46, 0.14, 1)
    nt.links.new(coord.outputs["Object"], mapping.inputs["Vector"])
    nt.links.new(mapping.outputs["Vector"], grad.inputs["Vector"])
    nt.links.new(grad.outputs["Fac"], ramp.inputs["Fac"])
    nt.links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])
    key = "Emission Color" if "Emission Color" in bsdf.inputs else "Emission"
    nt.links.new(ramp.outputs["Color"], bsdf.inputs[key])
    bsdf.inputs["Emission Strength"].default_value = 0.35
    return m


# ------------------------------------------------------------------------------ shapes
def link(obj):
    bpy.context.scene.collection.objects.link(obj)
    return obj


def mesh_object(name, bm):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    return link(bpy.data.objects.new(name, me))


def disc(name, radius, z, mat, segs=96):
    bm = bmesh.new()
    bmesh.ops.create_circle(bm, cap_ends=True, radius=radius, segments=segs)
    o = mesh_object(name, bm)
    o.rotation_euler = (math.radians(90), 0, 0)
    o.location = (0, z, 0)
    o.data.materials.append(mat)
    return o


def square(name, size, y, mat):
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=1, y_segments=1, size=size / 2)
    o = mesh_object(name, bm)
    o.rotation_euler = (math.radians(90), 0, 0)
    o.location = (0, y, 0)
    o.data.materials.append(mat)
    return o


def ring(name, r0, r1, y, mat, segs=96):
    bm = bmesh.new()
    inner = [bm.verts.new((r0 * math.cos(2 * math.pi * k / segs), 0, r0 * math.sin(2 * math.pi * k / segs)))
             for k in range(segs)]
    outer = [bm.verts.new((r1 * math.cos(2 * math.pi * k / segs), 0, r1 * math.sin(2 * math.pi * k / segs)))
             for k in range(segs)]
    for k in range(segs):
        j = (k + 1) % segs
        bm.faces.new((inner[k], outer[k], outer[j], inner[j]))
    o = mesh_object(name, bm)
    o.location = (0, y, 0)
    mod = o.modifiers.new("bevel", "SOLIDIFY")
    mod.thickness = 0.06
    o.data.materials.append(mat)
    return o


def egg(name, height, width, mat):
    """An egg: a sphere pulled narrower toward the top."""
    bpy.ops.mesh.primitive_uv_sphere_add(segments=48, ring_count=32, radius=1.0)
    o = bpy.context.object
    o.name = name
    for v in o.data.vertices:
        z = v.co.z
        taper = 1.0 - 0.18 * max(0.0, z)  # narrower at the top
        v.co.x *= width * taper
        v.co.y *= width * taper
        v.co.z *= height
    for p in o.data.polygons:
        p.use_smooth = True
    o.data.materials.append(mat)
    return o


def heart(name, size, depth, mat):
    """The classic heart curve, extruded and rounded."""
    pts = []
    for k in range(64):
        t = 2 * math.pi * k / 64
        x = 16 * math.sin(t) ** 3
        y = 13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t)
        pts.append((x / 17 * size, y / 17 * size))
    bm = bmesh.new()
    front = [bm.verts.new((x, -depth / 2, y)) for x, y in pts]
    back = [bm.verts.new((x, depth / 2, y)) for x, y in pts]
    bm.faces.new(list(reversed(front)))
    bm.faces.new(back)
    for i in range(len(pts)):
        j = (i + 1) % len(pts)
        bm.faces.new((front[i], front[j], back[j], back[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    o = mesh_object(name, bm)
    bev = o.modifiers.new("round", "BEVEL")
    bev.width = depth * 0.45
    bev.segments = 4
    o.data.materials.append(mat)
    return o


def wing_curl(name, mats):
    """A dragon wing curled round the egg from behind: the arm rises on the left to the
    wrist, three fingers fan out over the top of the shell, the membrane scalloped between
    them. It sits just behind the egg's middle, so the shell hides what's in front of it."""
    spar, web = mats
    y = 0.12
    root = Vector((-0.5, y, -0.72))
    wrist = Vector((-1.02, y, 0.3))
    tips = [Vector((-0.86, y, 1.02)), Vector((-0.34, y, 1.24)), Vector((0.22, y, 1.12))]
    outline = [root, wrist, tips[0]]
    for a, b in zip(tips, tips[1:]):  # the scallops, pulled in toward the wrist
        for s in range(1, 6):
            f = s / 6
            p = a.lerp(b, f)
            p = p.lerp(wrist, 0.26 * math.sin(math.pi * f))
            outline.append(p)
        outline.append(b)
    outline.append(Vector((0.3, y, 0.55)))  # the trailing edge meets the shell's shoulder
    outline.append(Vector((-0.1, y, -0.2)))
    bm = bmesh.new()
    verts = [bm.verts.new(p) for p in outline]
    bm.faces.new(verts)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    o = mesh_object(name + "_web", bm)
    sol = o.modifiers.new("thick", "SOLIDIFY")
    sol.thickness = 0.03
    o.data.materials.append(web)
    objs = [o]
    for k, (a, b) in enumerate([(root, wrist)] + [(wrist, tip) for tip in tips]):  # the arm and the fingers
        curve = bpy.data.curves.new(f"{name}_spar{k}", "CURVE")
        curve.dimensions = "3D"
        curve.bevel_depth = 0.06 if k == 0 else 0.04
        curve.bevel_resolution = 3
        sp = curve.splines.new("POLY")
        sp.points.add(1)
        sp.points[0].co = (*(a + Vector((0, -0.05, 0))), 1)
        sp.points[1].co = (*(b + Vector((0, -0.05, 0))), 1)
        so = link(bpy.data.objects.new(f"{name}_spar{k}", curve))
        so.data.materials.append(spar)
        objs.append(so)
    bpy.ops.mesh.primitive_cone_add(vertices=12, radius1=0.075, depth=0.2, location=wrist + Vector((-0.06, -0.05, 0.1)),
                                    rotation=(0, math.radians(-30), 0))
    bpy.context.object.data.materials.append(spar)  # the thumb claw
    objs.append(bpy.context.object)
    return objs


# ------------------------------------------------------------------------------ scene
def build():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE"
    scene.render.film_transparent = False  # every pixel is the icon's own: no clear corners
    scene.view_settings.view_transform = "Standard"
    scene.eevee.taa_render_samples = 64
    world = bpy.data.worlds.new("w")
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.3, 0.2, 0.3, 1)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.6
    bpy.ops.object.light_add(type="SUN", rotation=(math.radians(55), math.radians(-20), math.radians(-25)))
    bpy.context.object.data.energy = 3.0

    square("glow", 4.0, 0.45, halo_material())  # past the frame's edges: the plum fills the corners
    shell = egg("egg", 1.02, 0.74, egg_glow_material())
    shell.location = (0.08, 0, -0.08)
    h = heart("heart", 0.43, 0.12, principled("heart", HEART, 0.35, glow=(1.0, 0.3, 0.04), glow_strength=0.45))
    h.location = (0.1, -0.72, -0.16)
    wing_curl("wing", (principled("spar", WING, 0.5), principled("web", WING_WEB, 0.55)))

    cam = link(bpy.data.objects.new("cam", bpy.data.cameras.new("cam")))
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = 2.8  # the egg and its wing fill the square
    cam.location = (0, -6, 0.07)
    cam.rotation_euler = (math.radians(90), 0, 0)
    scene.camera = cam
    return scene


def render(path, size, final):
    scene = bpy.context.scene
    scene.render.resolution_x = scene.render.resolution_y = size
    os.makedirs(os.path.dirname(path), exist_ok=True)
    big = path if final is None else os.path.join(WORK, "big_" + os.path.basename(path))
    os.makedirs(WORK, exist_ok=True)
    scene.render.filepath = big
    bpy.ops.render.render(write_still=True)
    if final is not None:
        img = bpy.data.images.load(big)
        img.scale(final, final)  # averaged down: clean at icon size
        img.filepath_raw = path
        img.file_format = "PNG"
        img.save()
    print(f"[emblem] wrote {path}")


def main():
    build()
    render(os.path.join(REVIEW, "emblem_256.png"), 256, None)
    render(os.path.join(OUT_DIR, "icon.png"), 384, 48)


main()
