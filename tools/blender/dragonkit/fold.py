"""Solve a kind's folded-wing pose from target bone directions (the classic fold_solver.py,
for any kind and plan), and render it.

  blender -b -P tools/blender/dragonkit/fold.py -- --kind pouncer --preset tight --out C:/abs/prefix
  ... --targets "wing_arm=0.15,0.9,0.41;wing_fore=0.06,-0.8,-0.6;..."   (right wing; +X out, +Y back, +Z up)

Prints the (pitch, yaw, roll) keys, in the clips' convention (armature-axis deltas applied
after the idle pose), for a plan's fold pose (e.g. plans/<plan>.py FOLD = {...}); renders the
folded dragon from the den camera, above, behind and the side.
"""
import math
import os
import sys

import bpy
from mathutils import Quaternion, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from dragonkit import model as km  # noqa: E402

arg = km.arg
PRESETS = {  # right-wing directions for the classic chain
    "zfold4": {"wing_arm": (0.15, 0.90, 0.41), "wing_fore": (0.06, -0.80, -0.60), "wing_f1": (0.10, 0.98, 0.18),
               "wing_f2": (0.10, 0.99, 0.05), "wing_f3": (0.10, 0.98, -0.08), "wing_f4": (0.10, 0.95, -0.24)},
    # tighter: the forearm lies down the flank, every finger nearly together along the back
    "tight": {"wing_arm": (0.12, 0.92, 0.37), "wing_fore": (0.05, -0.78, -0.62), "wing_f1": (0.06, 0.99, 0.10),
              "wing_f2": (0.06, 0.99, 0.04), "wing_f3": (0.06, 0.99, -0.02), "wing_f4": (0.06, 0.98, -0.10)},
}


def pyr_from_q(q):
    e = q.to_matrix().to_euler("YXZ")
    return (-math.degrees(e.x), math.degrees(e.z), -math.degrees(e.y))


def solve(d, targets, side, idle, normal=(0.97, 0.0, 0.25)):
    arm = d["arm"]
    rest = {b.name: b.matrix_local.to_quaternion() for b in arm.data.bones}
    keys = {}
    mirror = Vector((-1, 1, 1)) if side == "L" else Vector((1, 1, 1))
    w = km.wing_points(side)
    chain = [c[0] for c in km.WING_CHAIN]
    tip = km.WING_CHAIN[2][2] if len(km.WING_CHAIN) > 2 else km.WING_CHAIN[-1][2]
    n_rest = (w[tip] - w["root"]).cross(w["body"] - w["root"]).normalized()
    if n_rest.z < 0:
        n_rest = -n_rest
    n_fold = (arm.matrix_world.to_quaternion() @ (Vector(normal) * mirror)).normalized()
    for base in chain:
        name = f"{base}_{side}"
        pb = arm.pose.bones.get(name)
        if pb is None or base not in targets:
            continue
        bpy.context.view_layer.update()
        world = (arm.matrix_world @ pb.matrix).to_quaternion()
        frame = world @ rest[name].inverted()
        current = (world @ Vector((0, 1, 0))).normalized()
        want = (arm.matrix_world.to_quaternion() @ (Vector(targets[base]) * mirror)).normalized()
        turn = current.rotation_difference(want)
        n_now = (turn @ world) @ (rest[name].inverted() @ n_rest)
        a = (n_now - want * n_now.dot(want)).normalized()
        b = (n_fold - want * n_fold.dot(want)).normalized()
        turn = Quaternion(want, math.atan2(want.dot(a.cross(b)), a.dot(b))) @ turn
        q = frame.inverted() @ turn @ frame
        pb.rotation_quaternion = idle[name] @ (rest[name].inverted() @ q @ rest[name])
        keys[name] = pyr_from_q(q)
    bpy.context.view_layer.update()
    return keys


def main():
    km.use_kind(arg("--kind", "pouncer"))
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene, cam = km.setup_scene(int(arg("--res", "360")), (0.40, 0.34, 0.42))
    form = arg("--form", "grown")
    d = km.build_dragon(form, int(arg("--variant", "0")))
    km.pose_stage(d, km.STAGE["adult" if form == "grown" else "hatchling"][1])
    arm = d["arm"]
    idle = {pb.name: pb.rotation_euler.to_quaternion() for pb in arm.pose.bones}
    for pb in arm.pose.bones:
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = idle[pb.name]
    if arg("--targets"):
        targets = {}
        for item in arg("--targets").split(";"):
            k, v = item.split("=")
            targets[k.strip()] = tuple(float(x) for x in v.split(","))
    else:
        targets = PRESETS[arg("--preset", "tight")]
    keys = solve(d, targets, "R", idle)
    keys.update(solve(d, targets, "L", idle))
    km.ground(d)
    print("[fold] FOLD = {")
    for name in sorted(keys):
        if name.endswith("_R"):
            p, y, r = keys[name]
            print(f'[fold]     "{name[:-2]}*": ({p:.0f}, {y:.0f}, {r:.0f}),')
    print("[fold] }")
    out = arg("--out")
    km.VIEWS["den"] = Vector((-0.35, -0.9, 0.32))
    for view in ("den", "top", "back_quarter", "side"):
        km.frame_camera(cam, [d], view, margin=1.3)
        km.render(f"{out}_{view}.png")


main()
