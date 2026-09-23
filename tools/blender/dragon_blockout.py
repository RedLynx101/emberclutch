"""Emberclutch dragon blockout — a proportion/silhouette study built from primitives.

Run headless:
  blender -b -P tools/blender/dragon_blockout.py -- --growth 0 --out tools/blender/out/hatchling
  blender -b -P tools/blender/dragon_blockout.py -- --growth 1 --out tools/blender/out/adult

`--growth` (0 = hatchling, 1 = adult) drives the cute -> majestic proportion curve from
docs/design/theme-and-art-direction.md §4. Writes <out>.png (render), <out>.glb and
prints the triangle count against the old-3DS budget. Not final art: Phase 1 replaces this
with a single skinned mesh on the shared rig.
"""
import math
import sys

import bpy
import bmesh
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
growth = float(argv[argv.index("--growth") + 1]) if "--growth" in argv else 0.0
out = argv[argv.index("--out") + 1] if "--out" in argv else "//dragon"
g = max(0.0, min(1.0, growth))


def lerp(a, b, t=g):
    return a + (b - a) * t


# Palette (Ember breed defaults; the game tints per genome via mask channels).
BASE = (0.86, 0.33, 0.12, 1)
ACCENT = (0.98, 0.86, 0.62, 1)
GOLD = (0.96, 0.72, 0.28, 1)
EYE_DARK = (0.12, 0.06, 0.08, 1)
WHITE = (1, 0.97, 0.92, 1)
GLOW = (1.0, 0.55, 0.12, 1)

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
parts = []


def material(name, color, emission=0.0):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = color
    bsdf.inputs["Roughness"].default_value = 0.55
    if emission:
        bsdf.inputs["Emission Color"].default_value = color
        bsdf.inputs["Emission Strength"].default_value = emission
    mat.diffuse_color = color
    return mat


M_BASE, M_ACCENT, M_GOLD = material("base", BASE), material("accent", ACCENT), material("gold", GOLD)
M_DARK, M_WHITE, M_GLOW = material("eye", EYE_DARK), material("white", WHITE), material("heartglow", GLOW, 6.0)


def finish(obj, mat, smooth=True):
    obj.data.materials.append(mat)
    if smooth:
        for poly in obj.data.polygons:
            poly.use_smooth = True
    parts.append(obj)
    return obj


def sphere(loc, scale, mat, seg=14, rings=9):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=seg, ring_count=rings, location=loc)
    obj = bpy.context.object
    obj.scale = scale
    return finish(obj, mat)


def cone(loc, radius, depth, rot, mat, verts=8):
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=radius, radius2=0, depth=depth, location=loc, rotation=rot)
    return finish(bpy.context.object, mat, smooth=False)


def wing(side, span, mat):
    """A three-finger membrane wing as a flat fan, thickened slightly."""
    mesh = bpy.data.meshes.new(f"wing{side}")
    obj = bpy.data.objects.new(f"wing{side}", mesh)
    bpy.context.collection.objects.link(obj)
    bm = bmesh.new()
    root = bm.verts.new((0, 0, 0))
    tips = [(0.15, 0.25, 1.0), (0.55, 0.45, 0.75), (0.85, 0.35, 0.35), (0.9, 0.05, 0.05)]
    ring = [bm.verts.new((side * x * span, y * span, z * span)) for x, y, z in tips]
    for a, b in zip(ring, ring[1:]):
        bm.faces.new((root, a, b))
    bm.to_mesh(mesh)
    bm.free()
    mod = obj.modifiers.new("thick", "SOLIDIFY")
    mod.thickness = 0.02
    return finish(obj, mat, smooth=False)


# --- proportions (hatchling -> adult) ------------------------------------------------
body_len = lerp(0.55, 1.05)
body_h = lerp(0.42, 0.55)
body_z = lerp(0.45, 0.85)
head_r = lerp(0.40, 0.26)
neck_len = lerp(0.05, 0.75)
eye_r = lerp(0.13, 0.05)
horn_len = lerp(0.10, 0.55)
wing_span = lerp(0.35, 1.9)
tail_len = lerp(0.6, 1.8)
leg_len = lerp(0.22, 0.6)

# Body and belly (dragon faces -Y).
sphere((0, 0, body_z), (body_len * 0.55, body_len * 0.85, body_h), M_BASE, 16, 10)
sphere((0, -body_len * 0.35, body_z - 0.04), (body_len * 0.38, body_len * 0.45, body_h * 0.82), M_ACCENT, 14, 9)

# Heartglow on the chest: two lobes and a point.
hz = body_z + body_h * 0.1
hy = -body_len * 0.35 - body_len * 0.42
hs = lerp(0.07, 0.10)
sphere((-hs * 0.55, hy, hz + hs * 0.2), (hs * 0.6, hs * 0.3, hs * 0.6), M_GLOW, 10, 6)
sphere((hs * 0.55, hy, hz + hs * 0.2), (hs * 0.6, hs * 0.3, hs * 0.6), M_GLOW, 10, 6)
cone((0, hy, hz - hs * 0.45), hs * 1.1, hs * 1.4, (math.pi, 0, 0), M_GLOW, 6)

# Legs.
for sx in (-1, 1):
    for sy, lz in ((-1, 1.0), (1, 0.9)):
        x = sx * body_len * 0.36
        y = sy * body_len * 0.45
        sphere((x, y, leg_len * 0.55 * lz), (0.11 + 0.08 * g, 0.12 + 0.08 * g, leg_len * 0.6), M_BASE, 10, 7)
        sphere((x, y - 0.06, 0.05), (0.12 + 0.06 * g, 0.16 + 0.08 * g, 0.06), M_BASE, 10, 6)

# Neck and head.
neck_base = Vector((0, -body_len * 0.55, body_z + body_h * 0.45))
head_pos = neck_base + Vector((0, -neck_len * 0.45, neck_len * 0.9 + head_r * 0.6))
if neck_len > 0.1:
    steps = 4
    for i in range(1, steps):
        t = i / steps
        p = neck_base.lerp(head_pos, t)
        sphere(p, (0.2 * (1 - 0.3 * t),) * 3, M_BASE, 12, 8)
sphere(head_pos, (head_r, head_r * 1.05, head_r * 0.95), M_BASE, 16, 10)
snout = head_pos + Vector((0, -head_r * 0.95, -head_r * 0.25))
sphere(snout, (head_r * 0.55, head_r * lerp(0.55, 0.95), head_r * 0.42), M_BASE, 12, 8)
sphere(snout + Vector((0, -head_r * 0.1, -head_r * 0.18)), (head_r * 0.45, head_r * 0.6, head_r * 0.2), M_ACCENT, 10, 6)

# Eyes: huge and sparkly when small, calm when grown.
for sx in (-1, 1):
    e = head_pos + Vector((sx * head_r * 0.52, -head_r * 0.62, head_r * 0.12))
    sphere(e, (eye_r, eye_r * 0.7, eye_r * 1.1), M_WHITE, 12, 8)
    sphere(e + Vector((0, -eye_r * 0.35, -eye_r * 0.1)), (eye_r * 0.72, eye_r * 0.5, eye_r * 0.85), M_DARK, 12, 8)
    sphere(e + Vector((sx * eye_r * -0.2, -eye_r * 0.72, eye_r * 0.35)), (eye_r * 0.22,) * 3, M_WHITE, 8, 5)

# Swept horns and frill spikes.
for sx in (-1, 1):
    base = head_pos + Vector((sx * head_r * 0.45, head_r * 0.35, head_r * 0.75))
    cone(base + Vector((0, horn_len * 0.35, horn_len * 0.3)), head_r * 0.2, horn_len, (-math.radians(55 + 15 * g), 0, 0), M_GOLD)
    cone(head_pos + Vector((sx * head_r * 0.85, head_r * 0.2, head_r * 0.1)), head_r * 0.14, horn_len * 0.5,
         (0, sx * math.radians(70), 0), M_BASE)

# Back spikes.
for i in range(5):
    t = i / 4
    y = lerp(-body_len * 0.4, body_len * 0.6, t)
    cone((0, y, body_z + body_h * (0.95 - 0.2 * t)), 0.05 + 0.04 * g, 0.12 + 0.12 * g, (0, 0, 0), M_GOLD, 6)

# Wings.
for sx in (-1, 1):
    w = wing(sx, wing_span, M_ACCENT)
    w.location = (sx * body_len * 0.3, -body_len * 0.1, body_z + body_h * 0.7)
    w.rotation_euler = (math.radians(-10), 0, sx * math.radians(-15))

# Tail: tapering chain with a spade tip.
prev = Vector((0, body_len * 0.75, body_z - body_h * 0.2))
for i in range(1, 6):
    t = i / 5
    p = Vector((math.sin(t * 2.2) * 0.25 * tail_len, body_len * 0.75 + t * tail_len, body_z * (1 - t) * 0.7 + 0.06))
    r = lerp(0.18, 0.2) * (1 - 0.75 * t)
    sphere(p, (r, r * 1.6, r), M_BASE, 10, 7)
    prev = p
cone(prev + Vector((0, 0.12, 0)), 0.14 + 0.06 * g, 0.3, (-math.pi / 2, 0, 0), M_GOLD, 4)

# --- stats ------------------------------------------------------------------------------
dg = bpy.context.evaluated_depsgraph_get()
tris = 0
for obj in parts:
    mesh = obj.evaluated_get(dg).to_mesh()
    tris += sum(len(p.vertices) - 2 for p in mesh.polygons)
    obj.evaluated_get(dg).to_mesh_clear()
budget = 1800 if g < 0.5 else 3000
print(f"[blockout] growth={g:.2f} parts={len(parts)} triangles={tris} budget={budget} "
      f"{'OK' if tris <= budget else 'OVER (merge + retopo in Phase 1)'}")

# --- stage, camera, light, render ----------------------------------------------------------
height = body_z + body_h + neck_len + head_r * 2 + horn_len
bpy.ops.mesh.primitive_plane_add(size=20, location=(0, 0, 0))
floor = bpy.context.object
floor.data.materials.append(material("floor", (0.23, 0.15, 0.27, 1)))

world = bpy.data.worlds.new("world")
scene.world = world
world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.20, 0.13, 0.25, 1)
world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.6

bpy.ops.object.light_add(type="SUN", rotation=(math.radians(50), math.radians(10), math.radians(-35)))
bpy.context.object.data.energy = 3.5
bpy.context.object.data.color = (1.0, 0.9, 0.78)
bpy.ops.object.light_add(type="SUN", rotation=(math.radians(-60), 0, math.radians(160)))
bpy.context.object.data.energy = 1.5  # cool rim
bpy.context.object.data.color = (0.55, 0.8, 0.85)

target = Vector((0, 0, height * 0.45))
dist = max(3.0, height * 2.6)
cam_pos = target + Vector((-dist * 0.62, -dist * 0.75, dist * 0.28))
bpy.ops.object.camera_add(location=cam_pos)
cam = bpy.context.object
cam.rotation_euler = (target - cam_pos).to_track_quat("-Z", "Y").to_euler()
cam.data.lens = 50
scene.camera = cam

scene.render.resolution_x, scene.render.resolution_y = 800, 600
scene.render.filepath = bpy.path.abspath(out + ".png")
try:
    scene.render.engine = "BLENDER_EEVEE"
except TypeError:
    scene.render.engine = "BLENDER_EEVEE_NEXT"
try:
    bpy.ops.render.render(write_still=True)
except Exception as exc:  # no GPU context: fall back to Workbench
    print(f"[blockout] Eevee failed ({exc}); using Workbench")
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.color_type = "MATERIAL"
    bpy.ops.render.render(write_still=True)

bpy.ops.object.select_all(action="DESELECT")
for obj in parts:
    obj.select_set(True)
bpy.ops.export_scene.gltf(filepath=bpy.path.abspath(out + ".glb"), use_selection=True, export_apply=True)
print(f"[blockout] wrote {out}.png and {out}.glb")
