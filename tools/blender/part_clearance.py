"""How deep a part group sinks into the body across the animation clips.

  blender -b -P tools/blender/part_clearance.py -- --group heart --form grown [--breed ember]

For each clip (tools/anim/clips.py), poses the dragon at several moments, and measures
how far the group's vertices go inside the deformed body (signed distance along the
nearest surface normal; positive = inside). Prints the worst per clip, so a part's offset
or attachment bone can be chosen from numbers instead of eyeballing renders.
"""
import os
import sys

import bpy
from mathutils import Quaternion
from mathutils.bvhtree import BVHTree

HERE = os.path.dirname(os.path.abspath(__file__))
for p in (HERE, os.path.join(HERE, "..", "anim")):
    sys.path.insert(0, p)
import clips as clip_lib  # noqa: E402
import dragon_model as dm  # noqa: E402

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


def depth_inside(d, objs):
    """(sink, lift): the worst penetration into the posed body of objs' outward-facing
    vertices (its visible front; a part's back is meant to sit in the skin), and how far the
    part's deepest point stands off the skin (> 0: floating)."""
    dg = bpy.context.evaluated_depsgraph_get()
    body = d["body"].evaluated_get(dg)
    bvh = BVHTree.FromObject(body, dg)
    inv = body.matrix_world.inverted()
    worst, lowest = 0.0, 1e9
    for o in objs:
        ev = o.evaluated_get(dg)
        me = ev.to_mesh()
        to_body = inv @ ev.matrix_world
        for v in me.vertices:
            p = to_body @ v.co
            hit, nrm, _, dist = bvh.find_nearest(p)
            if hit is None:
                continue
            signed = (p - hit).dot(nrm)
            lowest = min(lowest, signed)
            facing = (to_body.to_3x3() @ v.normal).normalized().dot(nrm)
            if facing > 0.5 and signed < 0:
                worst = max(worst, dist)
        ev.to_mesh_clear()
    return worst, lowest


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    form = arg("--form", "grown")
    if arg("--inset"):  # try another seat depth for the group (negative: further out)
        dm.FORMS[form]["inset"][arg("--group", "heart")] = float(arg("--inset"))
    d = dm.build_dragon(arg("--breed", "ember"), form)
    _, t = dm.STAGE["adult" if form == "grown" else "hatchling"]
    dm.pose_stage(d, t, d["breed"]["build"])
    arm = d["arm"]
    idle = {pb.name: pb.rotation_euler.to_quaternion() for pb in arm.pose.bones}
    rest = {b.name: b.matrix_local.to_quaternion() for b in arm.data.bones}
    objs = d["groups"][arg("--group", "heart")]
    samples = int(arg("--samples", "6"))
    # What moves the skin under the part: the body weights nearest its centre.
    bpy.context.view_layer.update()
    centre = sum((o.matrix_world.translation for o in objs), objs[0].matrix_world.translation * 0) / len(objs)
    body = d["body"]
    near = min(body.data.vertices, key=lambda v: (body.matrix_world @ v.co - centre).length)
    names = {g.index: g.name for g in body.vertex_groups}
    print("[clearance] skin at the part:", ", ".join(f"{names[g.group]} {g.weight:.2f}" for g in near.groups))
    if arg("--bone"):  # try another bone for the part, keeping where it sits in the idle pose
        for pb in arm.pose.bones:
            pb.rotation_mode = "QUATERNION"
            pb.rotation_quaternion = idle[pb.name]
        bpy.context.view_layer.update()
        for o in objs:
            here = o.matrix_world.copy()
            c = o.constraints[0]
            c.subtarget = arg("--bone")
            bone = arm.matrix_world @ arm.pose.bones[c.subtarget].matrix
            c.inverse_matrix = bone.inverted() @ here @ o.matrix_basis.inverted()
        bpy.context.view_layer.update()
        print(f"[clearance] attached to {arg('--bone')}")
    overall, lift_max = 0.0, -1e9
    for clip in clip_lib.CLIPS:
        worst, lift = 0.0, -1e9
        for k in range(samples):
            t_clip = clip.length * k / max(1, samples - 1)
            for pb in arm.pose.bones:
                q = Quaternion(clip.sample_q(pb.name, t_clip))
                pb.rotation_mode = "QUATERNION"
                pb.rotation_quaternion = idle[pb.name] @ (rest[pb.name].conjugated() @ q @ rest[pb.name])
            bpy.context.view_layer.update()
            sink, gap = depth_inside(d, objs)
            worst, lift = max(worst, sink), max(lift, gap)
        overall, lift_max = max(overall, worst), max(lift_max, lift)
        print(f"[clearance] {clip.name:14s} sinks {worst:.3f}  lifts {max(0.0, lift):.3f}")
    print(f"[clearance] worst overall: sinks {overall:.3f}, lifts {max(0.0, lift_max):.3f}")


main()
