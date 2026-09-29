"""How well each kind's heartglow sits on its chest: for every form and growth key, the heart's
vertices' distances off the body's surface (the furthest, the nearest, and the gap as a share of
the heart's size). Noah (2026-09-28): "some are straight down and not following their chestline",
a flat heart standing upright in the air before a sloping chest.

  blender -b -P tools/blender/dragonkit/heart_gap.py -- [--kinds pouncer,puffback]
"""
import sys
from pathlib import Path

import bpy
from mathutils.bvhtree import BVHTree

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from dragonkit import model as km  # noqa: E402


def arg(name, default=None):
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    return argv[argv.index(name) + 1] if name in argv else default


def kinds():
    root = Path(__file__).resolve().parents[2] / "dragons" / "kinds"
    return sorted(p.stem for p in root.glob("*.py") if not p.stem.startswith("_"))


def measure(d):
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    body = d["body"].evaluated_get(dg)
    bvh = BVHTree.FromObject(body, dg)
    inv = body.matrix_world.inverted()
    gaps = []
    for o in d["groups"].get("heart", []):
        ev = o.evaluated_get(dg)
        me = ev.to_mesh()
        to_body = inv @ ev.matrix_world
        for v in me.vertices:
            p = to_body @ v.co
            hit, nrm, _, _ = bvh.find_nearest(p)
            if hit is not None:
                gaps.append((p - hit).dot(nrm))
        ev.to_mesh_clear()
    return (max(gaps), min(gaps)) if gaps else (0.0, 0.0)


def main():
    wanted = arg("--kinds")
    for kind in (wanted.split(",") if wanted else kinds()):
        km.use_kind(kind)
        for form in ("hatchling", "grown"):
            bpy.ops.wm.read_factory_settings(use_empty=True)
            km.set_lod(0)
            d = km.build_dragon(form, 0, for_export=True)
            size = km.F["heart"]["size"]
            row = []
            for t in km.F["key_ts"]:
                km.rest_pose(d, t)
                km.apply_t(d, t, "neutral")
                far, near = measure(d)
                row.append(f"t{t:.2f} far {far:+.3f} ({far / size:+.2f}x) near {near:+.3f}")
            print(f"[heart] {kind:12s} {form:9s} size {size:.3f}: " + " | ".join(row), flush=True)


main()
