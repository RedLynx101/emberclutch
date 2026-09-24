"""Render the hands-on care sprites (tool cursors and foods) for the 3DS.

  blender -b -P tools/blender/care_sprites.py -- --out C:/abs/assets/sprites/care [--size 64]

Each sprite is a small toon-shaded prop (the dragons' own material, tools/blender/
dragon_model.py toon_material) with a soft outline, rendered front-on with a transparent
background. gfx/care.t3s packs them into one atlas (src/app/care_ui.cpp draws them at the
stylus). Original art: nothing here comes from the AI concept images.
"""
import math
import os
import sys

import bmesh
import bpy
from mathutils import Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dragon_model as dm  # noqa: E402

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = argv[argv.index("--out") + 1] if "--out" in argv else os.path.abspath("assets/sprites/care")
SIZE = int(argv[argv.index("--size") + 1]) if "--size" in argv else 64


def mat(name, color, emission=0.0):
    return dm.toon_material(name, color, emission=emission)


def add(obj, material):
    obj.data.materials.append(material)
    for p in obj.data.polygons:
        p.use_smooth = True
    return obj


def sphere(loc, scale, material, seg=20, rings=12):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=seg, ring_count=rings, location=loc)
    o = bpy.context.object
    o.scale = scale
    return add(o, material)


def box(loc, scale, material, bevel=0.3):
    bpy.ops.mesh.primitive_cube_add(location=loc)
    o = bpy.context.object
    o.scale = scale
    m = o.modifiers.new("round", "BEVEL")
    m.width = bevel * min(scale)
    m.segments = 3
    return add(o, material)


def cylinder(loc, radius, depth, material, rot=(0, 0, 0), verts=20):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=depth, location=loc, rotation=rot)
    return add(bpy.context.object, material)


def cone(loc, r1, r2, depth, material, rot=(0, 0, 0), verts=16):
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=r1, radius2=r2, depth=depth, location=loc, rotation=rot)
    return add(bpy.context.object, material)


def star(loc, radius, depth, material, points=5, inner=0.45, rot=(0, 0, 0)):
    bm = bmesh.new()
    ring = []
    for k in range(points * 2):
        a = math.pi / 2 + k * math.pi / points
        r = radius if k % 2 == 0 else radius * inner
        ring.append((r * math.cos(a), r * math.sin(a)))
    top = [bm.verts.new((x, -depth / 2, y)) for x, y in ring]
    bot = [bm.verts.new((x, depth / 2, y)) for x, y in ring]
    bm.faces.new(top)
    bm.faces.new(list(reversed(bot)))
    for i in range(len(ring)):
        j = (i + 1) % len(ring)
        bm.faces.new((top[i], top[j], bot[j], bot[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new("star")
    bm.to_mesh(me)
    bm.free()
    o = bpy.data.objects.new("star", me)
    bpy.context.collection.objects.link(o)
    o.location = loc
    o.rotation_euler = rot
    o.data.materials.append(material)
    return o


# ------------------------------------------------------------------------------ props
SKIN = (0.98, 0.80, 0.68)
WOOD = (0.72, 0.47, 0.27)
CREAM = (0.97, 0.92, 0.78)


def hand(pressing=False):
    m = mat("hand", SKIN)
    sphere((0, 0, 0), (0.5, 0.2, 0.52), m)  # palm
    for i, x in enumerate((-0.33, -0.11, 0.11, 0.33)):
        length = (0.34 if i in (1, 2) else 0.28) * (0.55 if pressing else 1.0)
        z = 0.42 + length * 0.5
        cylinder((x, 0, z), 0.1, length, m)
        sphere((x, 0, z + length / 2), (0.1, 0.1, 0.1), m)
    cylinder((-0.52, 0, 0.02), 0.11, 0.3, m, rot=(0, math.radians(55), 0))  # thumb
    sphere((-0.64, 0, 0.1), (0.11, 0.11, 0.11), m)
    cylinder((0, 0, -0.62), 0.24, 0.3, mat("cuff", (0.95, 0.72, 0.38)))  # a cosy sleeve cuff


def brush():
    box((0, 0, 0.42), (0.14, 0.12, 0.5), mat("wood", WOOD))
    box((0, 0, -0.22), (0.38, 0.2, 0.16), mat("wood2", (0.62, 0.38, 0.22)))
    box((0, 0, -0.48), (0.36, 0.18, 0.13), mat("bristle", CREAM), bevel=0.15)


def cloth():
    bpy.ops.mesh.primitive_grid_add(x_subdivisions=8, y_subdivisions=8, size=1.2, rotation=(math.radians(90), 0, 0))
    o = bpy.context.object
    for v in o.data.vertices:
        v.co.z += 0.08 * math.sin(v.co.x * 5.0) + 0.05 * math.cos(v.co.y * 4.0)
    m = o.modifiers.new("thick", "SOLIDIFY")
    m.thickness = 0.06
    add(o, mat("cloth", (0.96, 0.64, 0.74)))
    glint = mat("glint", (1, 1, 1), emission=2.0)
    star((0.38, -0.12, 0.34), 0.16, 0.04, glint)
    star((-0.3, -0.12, -0.3), 0.1, 0.04, glint)


def sponge():
    box((0, 0, 0), (0.55, 0.3, 0.38), mat("sponge", (1.0, 0.85, 0.34)), bevel=0.4)
    pore = mat("pore", (0.86, 0.64, 0.2))
    for x, z, r in ((-0.3, 0.15, 0.07), (0.1, 0.2, 0.06), (0.32, -0.1, 0.07), (-0.1, -0.15, 0.05), (0.35, 0.22, 0.05)):
        sphere((x, -0.28, z), (r, r * 0.5, r), pore)
    bub = mat("bubble", (0.9, 0.97, 1.0))
    for x, z, r in ((-0.45, 0.45, 0.12), (-0.2, 0.52, 0.08), (0.4, 0.45, 0.1)):
        sphere((x, -0.1, z), (r, r, r), bub)


def ladle():
    wood = mat("wood", WOOD)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=20, ring_count=12, location=(0, 0, -0.25))
    cup = bpy.context.object
    cup.scale = (0.42, 0.42, 0.3)
    bm = bmesh.new()
    bm.from_mesh(cup.data)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if v.co.z > 0.2], context="VERTS")
    bm.to_mesh(cup.data)
    bm.free()
    m = cup.modifiers.new("thick", "SOLIDIFY")
    m.thickness = 0.12
    add(cup, wood)
    cylinder((0, 0, -0.21), 0.36, 0.02, mat("water", (0.45, 0.75, 0.95)))
    cylinder((0.3, 0, 0.3), 0.07, 0.9, wood, rot=(0, math.radians(-35), 0))


def ball():
    sphere((0, 0, 0), (0.55, 0.55, 0.55), mat("ball", (0.92, 0.32, 0.26)), seg=24, rings=16)
    bpy.ops.mesh.primitive_torus_add(major_radius=0.53, minor_radius=0.07, location=(0, 0, 0),
                                     rotation=(0, math.radians(90), math.radians(20)))
    add(bpy.context.object, mat("band", CREAM))


def firepepper():
    c = cone((0, 0, -0.1), 0.26, 0.02, 1.1, mat("pepper", (0.9, 0.18, 0.12)), rot=(0, 0, 0))
    m = c.modifiers.new("bend", "SIMPLE_DEFORM")
    m.deform_method = "BEND"
    m.angle = math.radians(70)
    m.deform_axis = "Y"
    cylinder((0, 0, 0.5), 0.06, 0.22, mat("stem", (0.35, 0.62, 0.25)))
    sphere((0, 0, 0.42), (0.2, 0.2, 0.08), mat("cap", (0.35, 0.62, 0.25)))


def river_fish():
    blue = mat("fish", (0.38, 0.6, 0.86))
    sphere((0, 0, 0), (0.55, 0.22, 0.3), blue)
    cone((0.62, 0, 0), 0.26, 0.02, 0.36, blue, rot=(0, math.radians(-90), 0), verts=4)
    sphere((-0.3, -0.18, 0.07), (0.07, 0.04, 0.07), mat("eye", (0.08, 0.05, 0.08)))
    sphere((0, -0.05, -0.22), (0.3, 0.12, 0.08), mat("belly", (0.85, 0.92, 0.95)))


def skyberry():
    berry = mat("berry", (0.52, 0.52, 0.96))
    for x, z in ((-0.2, -0.12), (0.2, -0.12), (0, 0.16), (-0.22, 0.28), (0.22, 0.28)):
        sphere((x, 0, z), (0.22, 0.22, 0.22), berry)
    leaf = mat("leaf", (0.4, 0.7, 0.35))
    sphere((0.05, 0, 0.55), (0.22, 0.05, 0.1), leaf)


def honeyroot():
    cone((0, 0, -0.1), 0.28, 0.04, 1.0, mat("root", (1.0, 0.72, 0.25)), rot=(math.pi, 0, 0))
    leaf = mat("leaf", (0.4, 0.7, 0.35))
    for a in (-30, 0, 30):
        cylinder((math.sin(math.radians(a)) * 0.12, 0, 0.55), 0.05, 0.36, leaf, rot=(0, math.radians(a), 0))


def frostmelon():
    bpy.ops.mesh.primitive_cylinder_add(vertices=32, radius=0.62, depth=0.26, rotation=(math.radians(90), 0, 0))
    o = bpy.context.object
    bm = bmesh.new()
    bm.from_mesh(o.data)
    bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=(0, 0, 0), plane_no=(0, 1, 0),
                           clear_outer=True)  # local Y becomes up once the object is rotated
    bmesh.ops.contextual_create(bm, geom=[e for e in bm.edges if e.is_boundary])
    bm.to_mesh(o.data)
    bm.free()
    o.location.z = -0.25
    add(o, mat("melon", (0.72, 0.92, 0.95)))
    cylinder((0, 0.02, -0.25), 0.64, 0.24, mat("rind", (0.35, 0.6, 0.72)), rot=(math.radians(90), 0, 0))
    seed = mat("seed", (0.2, 0.3, 0.45))
    for x, z in ((-0.25, 0.05), (0.0, 0.14), (0.25, 0.05)):
        sphere((x, -0.14, z - 0.1), (0.04, 0.02, 0.06), seed)


def starfruit():
    star((0, 0, 0), 0.6, 0.24, mat("starfruit", (1.0, 0.88, 0.33)))


def hearth_bread():
    box((0, 0, 0), (0.62, 0.34, 0.34), mat("bread", (0.84, 0.56, 0.3)), bevel=0.6)
    score = mat("score", (0.96, 0.8, 0.52))
    for x in (-0.3, 0, 0.3):
        box((x, -0.02, 0.3), (0.05, 0.3, 0.05), score, bevel=0.3)


def roast_drumstick():
    sphere((-0.18, 0, 0.05), (0.42, 0.3, 0.34), mat("meat", (0.74, 0.38, 0.2)))
    bone = mat("bone", CREAM)
    cylinder((0.3, 0, -0.15), 0.08, 0.5, bone, rot=(0, math.radians(60), 0))
    sphere((0.52, 0, -0.3), (0.11, 0.11, 0.11), bone)
    sphere((0.46, 0, -0.38), (0.11, 0.11, 0.11), bone)


def ember_candy():
    orange = mat("candy", (1.0, 0.55, 0.16))
    sphere((0, 0, 0), (0.34, 0.3, 0.3), orange)
    wrap = mat("wrap", (1.0, 0.82, 0.4))
    cone((-0.48, 0, 0), 0.2, 0.02, 0.3, wrap, rot=(0, math.radians(90), 0))
    cone((0.48, 0, 0), 0.2, 0.02, 0.3, wrap, rot=(0, math.radians(-90), 0))
    star((0.1, -0.3, 0.1), 0.1, 0.03, mat("glint", (1, 1, 1), emission=2.0))


def glimmer_cookie():
    cylinder((0, 0, 0), 0.55, 0.16, mat("cookie", (0.9, 0.72, 0.45)), rot=(math.radians(90), 0, 0), verts=28)
    chip = mat("chip", (0.45, 0.28, 0.2))
    for x, z in ((-0.2, 0.18), (0.18, 0.2), (0.0, -0.1), (-0.28, -0.22), (0.28, -0.18)):
        sphere((x, -0.09, z), (0.07, 0.04, 0.07), chip)
    star((0.3, -0.12, 0.3), 0.14, 0.03, mat("glint", (1, 1, 1), emission=2.0))


SPRITES = [
    ("hand", hand), ("hand_press", lambda: hand(pressing=True)), ("brush", brush), ("cloth", cloth),
    ("sponge", sponge), ("ladle", ladle), ("ball", ball),
    # foods, in core/care.hpp Food order
    ("food_firepepper", firepepper), ("food_riverfish", river_fish), ("food_skyberry", skyberry),
    ("food_honeyroot", honeyroot), ("food_frostmelon", frostmelon), ("food_starfruit", starfruit),
    ("food_hearthbread", hearth_bread), ("food_drumstick", roast_drumstick), ("food_embercandy", ember_candy),
    ("food_glimmercookie", glimmer_cookie),
]


# ------------------------------------------------------------------------------ render
def setup():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_EEVEE"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = scene.render.resolution_y = SIZE
    scene.render.film_transparent = True
    scene.view_settings.view_transform = "Standard"
    scene.render.use_freestyle = True
    scene.render.line_thickness_mode = "ABSOLUTE"
    scene.render.line_thickness = 1.2
    ls = scene.view_layers[0].freestyle_settings.linesets.new("outline")
    ls.select_by_visibility = True
    ls.select_by_edge_types = True
    ls.select_silhouette = ls.select_border = True
    ls.select_crease = False
    ls.linestyle.color = (0.18, 0.1, 0.16)
    world = bpy.data.worlds.new("w")
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.0
    bpy.ops.object.light_add(type="SUN", rotation=(math.radians(50), math.radians(10), math.radians(-30)))
    bpy.context.object.data.energy = 4.0
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    scene.collection.objects.link(cam)
    cam.data.type = "ORTHO"
    cam.location = (0, -5, 0)
    cam.rotation_euler = (math.radians(90), 0, 0)
    scene.camera = cam
    return scene, cam


def frame(cam):
    pts = []
    dg = bpy.context.evaluated_depsgraph_get()
    for o in bpy.context.scene.objects:
        if o.type != "MESH":
            continue
        ev = o.evaluated_get(dg)
        me = ev.to_mesh()
        pts += [ev.matrix_world @ v.co for v in me.vertices]
        ev.to_mesh_clear()
    lo = Vector((min(p.x for p in pts), 0, min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), 0, max(p.z for p in pts)))
    c = (lo + hi) / 2
    cam.location = (c.x, -5, c.z)
    cam.data.ortho_scale = max(hi.x - lo.x, hi.z - lo.z) * 1.12


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, build in SPRITES:
        scene, cam = setup()
        build()
        frame(cam)
        scene.render.filepath = os.path.join(OUT, f"{name}.png")
        bpy.ops.render.render(write_still=True)
        print(f"[sprites] {name}")


main()
