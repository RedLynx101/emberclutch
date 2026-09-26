"""Every kind together at true size (R11b): the grown adults in rows of three by rarity, nose to
tail, all through one orthographic side camera at one scale, so their sizes compare directly.

  blender -b -P tools/blender/dragonkit/together.py -- --out C:/abs/build/review/together.png

Each dragon is posed as review.py poses it (the idle clip's first frame), scaled by its
export_scale and META size as the game scales it, and named above its head.
"""
import math
import os
import sys

import bpy
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from dragonkit import model as km  # noqa: E402
from dragonkit import review as rv  # noqa: E402

ROWS = [["pouncer", "puffback", "curlstone"], ["crestwing", "ribbontail", "flurrytail"],
        ["glimmermoth", "duskwing", "blazeplume"]]
ROW_DEPTH = 40.0   # rows sit apart along the camera's axis; each renders alone
GAP = 0.6
TILT = 8.0         # degrees the camera looks down, so the floor shows
INK = (0.17, 0.13, 0.20)


def label_material():
    mat = bpy.data.materials.new("label")
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    em = nt.nodes.new("ShaderNodeEmission")
    em.inputs["Color"].default_value = (*INK, 1.0)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(em.outputs["Emission"], out.inputs["Surface"])
    return mat


def place_row(r, names):
    """Build a row nose to tail along -Y at depth X = r * ROW_DEPTH; return its objects and dragons."""
    objs, ds, front = set(), [], 0.0
    for i, name in enumerate(names):
        before = set(bpy.data.objects)
        km.use_kind(name)
        p = rv.Posed("adult", 0)
        p.idle_pose()
        d = p.d
        s = km.F["export_scale"] * km.KIND.META.get("size", 1.0)
        d["arm"].scale = (s, s, s)
        d["arm"].location.z *= s
        bpy.context.view_layer.update()
        ys = [q.y for q in km.visible_points([d])]
        d["arm"].location.y += front - (GAP if i else 0.0) - max(ys)
        d["arm"].location.x = r * ROW_DEPTH
        bpy.context.view_layer.update()
        pts = km.visible_points([d])
        front = min(q.y for q in pts)
        new = set(bpy.data.objects) - before
        for o in new:  # later kinds build with clean names
            o.name = f"r{r}k{i}_{o.name}"
        objs |= new
        ds.append(dict(d=d, title=km.KIND.META["title"], y=(min(q.y for q in pts) + max(q.y for q in pts)) / 2,
                       top=max(q.z for q in pts)))
    return objs, ds


def main():
    out = km.arg("--out", os.path.join(km.ROOT, "build", "review", "together.png"))
    res_x, res_y = 2240, 700
    scene, cam = rv.scene_setup(560)
    rows = [place_row(r, names) for r, names in enumerate(ROWS)]
    # One scale for every row: the widest row, and room above the tallest head for the names.
    spans = []
    for objs, ds in rows:
        pts = [q for e in ds for q in km.visible_points([e["d"]])]
        spans.append((min(q.y for q in pts), max(q.y for q in pts)))
    width = max(hi - lo for lo, hi in spans) * 1.1
    tallest = max(e["top"] for _, ds in rows for e in ds)
    height = width * res_y / res_x
    text_size = tallest * 0.13
    mat = label_material()
    for r, (objs, ds) in enumerate(rows):
        for e in ds:
            curve = bpy.data.curves.new(f"label_{e['title']}", "FONT")
            curve.body = e["title"]
            curve.align_x = "CENTER"
            curve.size = text_size
            curve.materials.append(mat)
            t = bpy.data.objects.new(f"label_{e['title']}", curve)
            scene.collection.objects.link(t)
            t.location = (r * ROW_DEPTH - 2.0, e["y"], tallest * 1.16)
            t.rotation_euler = (math.radians(90), 0.0, math.radians(-90))  # facing the camera at -X
            objs.add(t)
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = width
    cam.rotation_euler = (math.radians(90 - TILT), 0.0, math.radians(-90))  # looking along +X, a little down
    look = Vector((math.cos(math.radians(TILT)), 0.0, -math.sin(math.radians(TILT))))
    scene.render.resolution_x, scene.render.resolution_y = res_x, res_y
    floor = bpy.data.objects["floor"]
    everything = set().union(*(objs for objs, _ in rows))
    files = []
    for r, (objs, ds) in enumerate(rows):
        for o in everything:
            o.hide_render = o not in objs
        lo, hi = spans[r]
        target = Vector((r * ROW_DEPTH, (lo + hi) / 2, height * 0.45))
        cam.location = target - look * 60.0
        floor.location = (r * ROW_DEPTH + width * 0.35, (lo + hi) / 2, 0.0)
        floor.scale = (width / 10 * 1.3,) * 3
        f = os.path.splitext(out)[0] + f"_row{r}.png"
        km.render(f)
        files.append(f)
    rv.tile(out, files, 1)


main()
