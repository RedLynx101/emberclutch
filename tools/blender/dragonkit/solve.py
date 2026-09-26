"""Solve a body pose from bone directions, print it as clip keys and render it (run 18: how
the Pouncer plan's curl was made).

  blender -b -P tools/blender/dragonkit/solve.py -- --kind pouncer [--form grown] --spec C:/abs/spec.py
          [--out C:/abs/prefix] [--res 420]

The spec is a Python file defining:
  KEYS     {bone or "name*": (pitch, yaw, roll)} clip keys set first (the legs, say)
  TARGETS  [(bone, (dx, dy, dz)[, up or None[, twist degrees]])] solved in order, parents
           first: each bone turned the short way to point along (dx, dy, dz) in the armature's
           frame (the dragon faces -Y, +X is where a forward bone's yaw + turns it), then
           rolled about itself so its local Z leans toward `up`, then by `twist`
  BASE     the clip whose first frame is under it all ("idle": the wings folded)
  VIEWS    km.VIEWS names to render (default: den, top_down, side, front_right)
Keys are armature-axis deltas on the idle pose, as the clips have them (eca.py). It stands
the dragon as the game does and prints curl.py's numbers for the pose. Solve on one kind,
then set the keys (KEYS only) on every kind on the plan: one plan's deltas land differently
on each skeleton.

The Pouncer plan's CURL came from this spec on the grown Pouncer (then tuned on all four
kinds with curl.py: the neck lower for the Blazeplume, the tail tips a little higher):
  KEYS = {"hips": (10, 0, 0), "arm_up*": (60, 0, 0), "arm_lo*": (-145, 0, 0), "hand*": (-5, 0, 0),
          "leg_up*": (62, 12, 0), "leg_lo*": (-115, 0, 0), "foot*": (55, 0, 0)}
  TARGETS = [("belly", (0.22, -0.97, -0.05)), ("chest", (0.45, -0.84, 0.25)),
             ("neck1", (0.50, -0.58, -0.64)), ("neck2", (0.62, -0.62, -0.48)),
             ("neck3", (0.70, -0.64, -0.32)), ("head", (0.75, -0.64, -0.18), None, 20),
             ("tail1", (0.55, 0.78, -0.30)), ("tail2", (0.90, -0.40, 0.0)),
             ("tail3", (0.15, -0.99, 0.0)), ("tail4", (-0.55, -0.84, 0.0))]
"""
import math
import os
import runpy
import sys

import bpy
from mathutils import Quaternion, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from dragonkit import model as km  # noqa: E402
from dragonkit import review as rv  # noqa: E402
from dragonkit import curl as cl  # noqa: E402
from eca import q_from_pyr  # noqa: E402

km.VIEWS["den"] = Vector((-0.35, -0.9, 0.55))
km.VIEWS["top_down"] = Vector((0.0, 0.02, 1.0))
km.VIEWS["front_right"] = Vector((0.7, -0.8, 0.25))


def pyr_from_q(q):
    e = q.to_matrix().to_euler("YXZ")
    return (-math.degrees(e.x), math.degrees(e.z), -math.degrees(e.y))


def expand(keys):
    out = {}
    for k, v in keys.items():
        if k.endswith("*"):
            out[k[:-1] + "_R"] = v
            out[k[:-1] + "_L"] = (v[0], -v[1], -v[2])
        else:
            out[k] = v
    return out


def solve(d, pose, targets):
    """Turn each target bone to its direction (see the docstring); {bone: (p, y, r)}."""
    arm = d["arm"]
    mw = arm.matrix_world.to_quaternion()
    keys = {}
    for item in targets:
        name, want = item[0], Vector(item[1]).normalized()
        up = Vector(item[2]).normalized() if len(item) > 2 and item[2] is not None else None
        twist = math.radians(item[3]) if len(item) > 3 else 0.0
        pb = arm.pose.bones[name]
        pb.rotation_quaternion = pose.idle[name]
        bpy.context.view_layer.update()
        world = (arm.matrix_world @ pb.matrix).to_quaternion()
        frame = world @ pose.rest[name].inverted()
        w = (mw @ want).normalized()
        turn = (world @ Vector((0, 1, 0))).normalized().rotation_difference(w)
        if up is not None:
            z_now = (turn @ world) @ Vector((0, 0, 1))
            uw = mw @ up
            a = (z_now - w * z_now.dot(w)).normalized()
            b = (uw - w * uw.dot(w)).normalized()
            turn = Quaternion(w, math.atan2(w.dot(a.cross(b)), a.dot(b))) @ turn
        if twist:
            turn = Quaternion(w, twist) @ turn
        q = frame.inverted() @ turn @ frame
        pb.rotation_quaternion = pose.idle[name] @ (pose.rest[name].inverted() @ q @ pose.rest[name])
        keys[name] = pyr_from_q(q)
        bpy.context.view_layer.update()
    return keys


def main():
    spec = runpy.run_path(km.arg("--spec"))
    km.use_kind(km.arg("--kind", "pouncer"))
    form = km.arg("--form", "grown")
    rv.scene_setup(int(km.arg("--res", "420")))
    d = km.build_dragon(form, 0)
    for m in bpy.data.materials:
        m.use_backface_culling = True
    t = km.STAGE["adult" if form == "grown" else "hatchling"][1]
    pose = cl.Pose(d, t)
    pose.frame({c.name: c for c in km.PLAN.clips()}["idle"], 0.0)
    length = cl.body_length(d, t)
    arm = d["arm"]
    base = {c.name: c for c in km.PLAN.clips()}[spec.get("BASE", "idle")]
    for pb in arm.pose.bones:
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = pose.idle[pb.name] @ (pose.rest[pb.name].conjugated()
                                                       @ Quaternion(base.sample_q(pb.name, 0.0)) @ pose.rest[pb.name])
    keys = {}
    for name, v in expand(spec.get("KEYS", {})).items():
        if name in arm.pose.bones:
            pb = arm.pose.bones[name]
            pb.rotation_quaternion = pose.idle[name] @ (pose.rest[name].conjugated() @ Quaternion(q_from_pyr(*v))
                                                        @ pose.rest[name])
            keys[name] = v
    bpy.context.view_layer.update()
    keys.update(solve(d, pose, spec.get("TARGETS", [])))
    arm.location = (0, 0, 0)  # stand it as the game does
    bpy.context.view_layer.update()
    ev = d["body"].evaluated_get(bpy.context.evaluated_depsgraph_get())
    me = ev.to_mesh()
    arm.location.z = -min((ev.matrix_world @ v.co).z for v in me.vertices if not pose.tail[v.index])
    ev.to_mesh_clear()
    bpy.context.view_layer.update()
    print("[solve] " + ", ".join(f"{k} {v:.3f} ({w})" for k, (v, w) in cl.measure(d, length).items()))
    print("[solve] POSE = {")
    for name in sorted(keys):
        if not name.endswith("_L"):
            p, y, r = keys[name]
            print(f'[solve]     "{name[:-2] + "*" if name.endswith("_R") else name}": ({p:.0f}, {y:.0f}, {r:.0f}),')
    print("[solve] }")
    out = km.arg("--out")
    if out:
        files, pts = [], km.visible_points([d])
        for view in spec.get("VIEWS", ["den", "top_down", "side", "front_right"]):
            km.frame_points(bpy.context.scene.camera, pts, view, lens=55, margin=1.6)
            files.append(f"{out}_{km.KIND.META['name']}_{form}_{view}.png")
            km.render(files[-1])
        rv.tile(f"{out}_{km.KIND.META['name']}_{form}.png", files, len(files))


if __name__ == "__main__":
    main()
