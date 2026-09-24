"""Solve a wing pose from target bone directions, and render it for review.

  blender -b -P tools/blender/fold_solver.py -- --form grown --out C:/abs/prefix

Posing a whole wing chain by hand-tuned Euler deltas is guesswork: each bone's delta turns
with its parents. Here each wing bone gets a target direction (in the dragon's space: +X
out to its right, +Y back, +Z up) and an optional "up" for its twist; the script finds the
delta rotation (the clips' convention: armature axes, q_local = rest^-1 q rest, applied after
the idle pose) that points it there, parents first, and prints the (pitch, yaw, roll) keys
to paste into tools/anim/clips.py. It renders the result from the den camera, above and
the side.
"""
import math
import os
import sys

import bpy
from mathutils import Matrix, Quaternion, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import dragon_model as dm  # noqa: E402

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


# Right-wing targets (the left wing mirrors X). Directions need not be unit length.
VARIANTS = {
    "cape": {  # elbow up, forearm down the flank, fingers trailing back and down
        "wing_arm": ((0.30, 0.30, 0.90), None), "wing_fore": ((0.16, 0.80, -0.58), None),
        "wing_f1": ((0.14, 0.92, -0.36), None), "wing_f2": ((0.13, 0.88, -0.46), None),
        "wing_f3": ((0.12, 0.83, -0.55), None), "wing_f4": ((0.11, 0.76, -0.64), None)},
    "tucked": {  # elbow straight up, forearm back along the back, fingers tight and level
        "wing_arm": ((0.22, 0.10, 0.97), None), "wing_fore": ((0.14, 0.95, -0.28), None),
        "wing_f1": ((0.12, 0.98, 0.12), None), "wing_f2": ((0.12, 0.99, 0.02), None),
        "wing_f3": ((0.11, 0.98, -0.10), None), "wing_f4": ((0.10, 0.96, -0.22), None)},
    "bird": {  # elbow up and back, forearm back, fingertips crossing over the tail base
        "wing_arm": ((0.20, 0.45, 0.87), None), "wing_fore": ((0.10, 0.92, -0.38), None),
        "wing_f1": ((-0.16, 0.96, 0.22), None), "wing_f2": ((-0.12, 0.98, 0.12), None),
        "wing_f3": ((-0.08, 0.99, 0.02), None), "wing_f4": ((-0.04, 0.98, -0.10), None)},
}
VARIANTS["zfold"] = {  # like a bird: upper arm up and back, forearm down and forward, hand back
    "wing_arm": ((0.25, 0.55, 0.80), None), "wing_fore": ((0.12, -0.55, -0.83), None),
    "wing_f1": ((0.10, 0.98, 0.15), None), "wing_f2": ((0.10, 0.99, 0.02), None),
    "wing_f3": ((0.10, 0.97, -0.12), None), "wing_f4": ((0.10, 0.94, -0.30), None)}
VARIANTS["zfold_low"] = {  # the same, the hand angled down along the flank
    "wing_arm": ((0.25, 0.50, 0.83), None), "wing_fore": ((0.14, -0.45, -0.88), None),
    "wing_f1": ((0.12, 0.96, -0.05), None), "wing_f2": ((0.12, 0.94, -0.20), None),
    "wing_f3": ((0.11, 0.90, -0.34), None), "wing_f4": ((0.10, 0.84, -0.48), None)}
FOLD = VARIANTS[arg("--variant", "cape")]
ORDER = ["wing_arm", "wing_fore", "wing_f1", "wing_f2", "wing_f3", "wing_f4"]


def pyr_from_q(q):
    """Inverse of eca.q_from_pyr: q = Rz(yaw) * R(-X)(pitch) * R(-Y)(roll)."""
    e = q.to_matrix().to_euler("YXZ")  # M = Rz(ez) Rx(ex) Ry(ey)
    return (-math.degrees(e.x), math.degrees(e.z), -math.degrees(e.y))


def to_idle(d):
    """Quaternion mode, idle pose; returns the idle rotations."""
    arm = d["arm"]
    idle = {pb.name: pb.rotation_euler.to_quaternion() for pb in arm.pose.bones}
    for pb in arm.pose.bones:
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = idle[pb.name]
    return idle


def solve(d, targets, side, idle):
    arm = d["arm"]
    rest = {b.name: b.matrix_local.to_quaternion() for b in arm.data.bones}
    keys = {}
    mirror = Vector((-1, 1, 1)) if side == "L" else Vector((1, 1, 1))
    for base in ORDER:
        name = f"{base}_{side}"
        pb = arm.pose.bones.get(name)
        if pb is None or base not in targets:
            continue
        bpy.context.view_layer.update()
        world = (arm.matrix_world @ pb.matrix).to_quaternion()  # with no delta yet
        frame = world @ rest[name].inverted()                   # where the delta's axes point
        current = (world @ Vector((0, 1, 0))).normalized()       # a bone points along its Y
        direction, _ = targets[base]
        want = Vector(direction) * mirror
        want = (arm.matrix_world.to_quaternion() @ want).normalized()
        turn = current.rotation_difference(want)
        q = frame.inverted() @ turn @ frame
        pb.rotation_quaternion = idle[name] @ (rest[name].inverted() @ q @ rest[name])
        keys[name] = pyr_from_q(q)
    bpy.context.view_layer.update()
    return keys


def ground(d):
    arm = d["arm"]
    arm.location.z = 0
    bpy.context.view_layer.update()
    ev = d["body"].evaluated_get(bpy.context.evaluated_depsgraph_get())
    me = ev.to_mesh()
    low = min((ev.matrix_world @ v.co).z for v in me.vertices)
    ev.to_mesh_clear()
    arm.location.z = -low
    bpy.context.view_layer.update()


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene, cam = dm.setup_scene()
    scene.render.resolution_x = scene.render.resolution_y = int(arg("--res", "360"))
    form = arg("--form", "grown")
    d = dm.build_dragon(arg("--breed", "ember"), form)
    _, t = dm.STAGE["adult" if form == "grown" else "hatchling"]
    dm.pose_stage(d, t, d["breed"]["build"])
    idle = to_idle(d)
    keys = solve(d, FOLD, "R", idle)
    keys.update(solve(d, FOLD, "L", idle))
    ground(d)
    print("[fold] keys (pitch, yaw, roll):")
    for name in sorted(keys):
        if name.endswith("_R"):
            p, y, r = keys[name]
            pl, yl, rl = keys[name[:-2] + "_L"]
            print(f"[fold]   {name[:-2]}*: ({p:.0f}, {y:.0f}, {r:.0f})   L check ({pl:.0f}, {-yl:.0f}, {-rl:.0f})")
    out = arg("--out")
    dm.VIEWS["den"] = Vector((-0.35, -0.9, 0.32))
    for view in ("den", "top", "back_quarter", "side"):
        dm.frame_camera(cam, [d], view, margin=1.3)
        scene.render.filepath = f"{out}_{view}.png"
        bpy.ops.render.render(write_still=True)


main()
