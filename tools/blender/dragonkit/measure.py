"""How big each kind's grown model is, next to the Pouncer's, and the export_scale that brings it
to the common scale (DR2 sizing). The measure is the geometric mean of two sizes: the cube root of
the body's volume (bulk) and the length nose to tail (reach), so a round Puffback and a long thin
Ribbontail compare fairly. META size then sizes each kind in the game (0.67 to 1.5).

  blender -b -P tools/blender/dragonkit/measure.py -- [--kinds pouncer,puffback]

Prints each kind's volume, extents, measure, its export_scale now and the one it should have.
"""
import os
import sys

import bmesh
import bpy

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from dragonkit import model as km  # noqa: E402
from dragonkit import review as rv  # noqa: E402

import dragons  # noqa: E402  (on the path through the kit)


def measure(name, stage="adult"):
    """The kind's model as authored (export_scale 1): (volume, length, width, height, measure)."""
    km.use_kind(name)
    p = rv.Posed(stage, 0, textured=False)
    p.idle_pose()
    d = p.d
    dg = bpy.context.evaluated_depsgraph_get()
    bm = bmesh.new()
    bm.from_object(d["body"], dg)
    bm.transform(d["body"].evaluated_get(dg).matrix_world)
    vol = abs(bm.calc_volume(signed=False))
    bm.free()
    pts = km.visible_points([d])
    ext = [max(getattr(q, a) for q in pts) - min(getattr(q, a) for q in pts) for a in "yxz"]
    km.clear_dragons()
    return vol, *ext, (vol ** (1 / 3) * ext[0]) ** 0.5


def main():
    names = km.arg("--kinds", ",".join(k.META["name"] for k in dragons.all_kinds())).split(",")
    rv.scene_setup(200)
    ref = measure("pouncer")[-1]
    for name in names:
        vol, length, width, height, m = measure(name)
        now = dragons.kind(name).FORMS["grown"].get("export_scale", 1.0)
        print(f"[measure] {name:12s} volume {vol:7.3f}  length {length:5.2f}  width {width:5.2f}  height {height:5.2f}  "
              f"measure {m:5.3f} ({m / ref:4.2f} of the Pouncer)  export_scale {now:4.2f}, wants {ref / m:4.2f}")


main()
