"""Render dragon animation clips (tools/anim/clips.py) on the real rig, for review R3.

  blender -b -P tools/blender/preview_anims.py -- --clips idle,walk --form grown --breed ember \\
      --frames 6 --view side --out C:/abs/path/prefix

For each clip, renders --frames evenly spaced frames (<prefix>_<clip>_<k>.png). The pose math
matches the runtime (src/core/anim.cpp): the idle pose, then each clip delta converted from
armature axes into the bone's local frame with the rest rotation (q_rest^-1 * q * q_rest),
the root track, and the lowest body vertex placed on the floor every frame.
"""
import os
import sys

import bpy
from mathutils import Quaternion, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "anim"))
import dragon_model as dm  # noqa: E402
import clips as clip_lib  # noqa: E402
from eca import FPS  # noqa: E402

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


def lowest_body_z(d):
    dg = bpy.context.evaluated_depsgraph_get()
    ev = d["body"].evaluated_get(dg)
    me = ev.to_mesh()
    low = min((ev.matrix_world @ v.co).z for v in me.vertices)
    ev.to_mesh_clear()
    return low


def apply_frame(d, clip, t, idle, rest, scale):
    arm = d["arm"]
    for pb in arm.pose.bones:
        q = Quaternion(clip.sample_q(pb.name, t))
        local = rest[pb.name].conjugated() @ q @ rest[pb.name]
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = idle[pb.name] @ local
    fwd, up = clip.sample_root(t)
    arm.location = (0, 0, 0)
    bpy.context.view_layer.update()
    low = lowest_body_z(d)
    arm.location = (0, -fwd * scale, -low + up * scale)
    bpy.context.view_layer.update()


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene, cam = dm.setup_scene()
    res = int(arg("--res", "320"))
    scene.render.resolution_x = scene.render.resolution_y = res
    form = arg("--form", "grown")
    stage = arg("--stage", "adult" if form == "grown" else "hatchling")
    d = dm.build_dragon(arg("--breed", "ember"), form)
    _, t_growth = dm.STAGE[stage]
    dm.pose_stage(d, t_growth, d["breed"]["build"])
    arm = d["arm"]
    idle = {pb.name: pb.rotation_euler.to_quaternion() for pb in arm.pose.bones}
    rest = {b.name: b.matrix_local.to_quaternion() for b in arm.data.bones}
    # Root offsets are authored in adult units; smaller dragons move proportionally less.
    scale = dm.F["export_scale"] * (0.62 + 0.38 * t_growth if form == "hatchling" else 0.5 + 0.5 * t_growth)
    names = arg("--clips", "all")
    wanted = [c for c in clip_lib.CLIPS if names == "all" or c.name in names.split(",")]
    frames = int(arg("--frames", "6"))
    view = arg("--view", "side")
    out = arg("--out")
    # Frame the camera once, on the idle clip (wings folded), with room for hops and pounces.
    idle_clip = next(c for c in clip_lib.CLIPS if c.name == "idle")
    apply_frame(d, idle_clip, 0.0, idle, rest, scale)
    dm.frame_camera(cam, [d], view, margin=float(arg("--margin", "1.25")))
    for clip in wanted:
        n = clip.frame_count()
        picks = [round(k * (n - 1) / max(1, frames - 1)) for k in range(frames)]
        for k, f in enumerate(picks):
            apply_frame(d, clip, f / FPS, idle, rest, scale)
            scene.render.filepath = f"{out}_{clip.name}_{k}.png"
            bpy.ops.render.render(write_still=True)
        print(f"[preview] {clip.name}: {frames} frames")


main()
