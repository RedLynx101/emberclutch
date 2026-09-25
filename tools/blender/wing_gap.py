"""How far the wings' roots stand off the body across the animation clips (Noah, 2026-09-24:
"the very beginning of the wing meant to come from their back isn't attached correctly" on
many adults, walking and in other animations).

  blender -b -P tools/blender/wing_gap.py -- [--builds neutral,sturdy,sleek,long] [--wings classic] [--stage adult] [--samples 6]

For each build (the classic Ember otherwise), poses the dragon through every clip
(tools/anim/clips.py) with the runtime's pose math (like part_clearance.py), and measures on
both sides, in body units (the adult is about 4.5 long):
  root  how far the nearest point of the wing arm's root stands off the posed body surface
        (0: touching or sunk in; > 0: a gap you can see through)
  web   how far the membrane's inner edge (sampled from its root corner to its flank corner,
        on both sheets) stands off the body
Prints the worst per clip and overall, so a fix can be judged by numbers, not renders.
"""
import os
import sys

import bpy
from mathutils import Quaternion, Vector
from mathutils.bvhtree import BVHTree

HERE = os.path.dirname(os.path.abspath(__file__))
for p in (HERE, os.path.join(HERE, "..", "anim")):
    sys.path.insert(0, p)
import clips as clip_lib  # noqa: E402
import dragon_model as dm  # noqa: E402

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


def posed_points(obj, dg):
    ev = obj.evaluated_get(dg)
    me = ev.to_mesh()
    pts = [ev.matrix_world @ v.co for v in me.vertices]
    ev.to_mesh_clear()
    return pts


def gaps(d, side, near_root, web_ids):
    """(root gap, web gap) for one side in the current pose."""
    dg = bpy.context.evaluated_depsgraph_get()
    body = d["body"].evaluated_get(dg)
    bvh = BVHTree.FromObject(body, dg)
    inv = body.matrix_world.inverted()
    arm = d["arm"]
    joint = arm.matrix_world @ arm.pose.bones[f"wing_arm_{side}"].head

    def crossings(q, direction):
        n, start = 0, q
        for _ in range(64):
            hit, _, _, _ = bvh.ray_cast(start, direction)
            if hit is None:
                return n
            n += 1
            start = hit + direction * 1e-4
        return n

    def outside(p):
        """Distance to the skin, > 0 outside. The nearest face's normal alone misreads a point
        pressed between a folded leg and the flank (the leg's inner face is nearest), so a point
        the normal calls outside counts only if rays cross the skin an even number of times."""
        q = inv @ p
        hit, nrm, _, dist = bvh.find_nearest(q)
        if hit is None:
            return 0.0
        signed = (q - hit).dot(nrm)
        if signed > 0:
            votes = sum(crossings(q, d) % 2 for d in (Vector((0.3, 0.2, 1)).normalized(),
                                                       Vector((-0.4, 1, 0.1)).normalized(),
                                                       Vector((1, -0.3, -0.2)).normalized()))
            if votes >= 2:  # inside by the rays
                return -dist
        return signed

    wingarm = next(o for o in d["wings"] if o.name.startswith(f"wingarm_{side}"))
    root = [p for p in posed_points(wingarm, dg) if (p - joint).length < near_root]
    membrane = next(o for o in d["wings"] if o.name.startswith(f"membrane_{side}"))
    mem = posed_points(membrane, dg)
    root_gap = max(0.0, min(outside(p) for p in root)) if root else float("nan")
    web_gap = 0.0  # along the inner edge itself: each sheet's root corner to its flank corner
    for a, b in web_ids:
        profile = [outside(mem[a].lerp(mem[b], k / 10)) for k in range(11)]
        web_gap = max(web_gap, max(profile))
        if "--profile" in argv:
            print(f"[winggap]   {side} edge, root to flank: " + " ".join(f"{x:+.3f}" for x in profile))
    return root_gap, web_gap


def web_vertices(d, side):
    """The membrane's inner edge: (root corner, flank corner) vertex pairs, one per sheet."""
    w = dm.wing_points(side)
    membrane = next(o for o in d["wings"] if o.name.startswith(f"membrane_{side}"))
    pts = [(v.index, membrane.matrix_world @ v.co) for v in membrane.data.vertices]
    roots = sorted(pts, key=lambda p: (p[1] - w["root"]).length)[:2]
    flanks = sorted(pts, key=lambda p: (p[1] - w["body"]).length)[:2]
    pairs = []
    for i, p in roots:  # pair each root corner with the flank corner on its own sheet
        j = min(flanks, key=lambda q: ((q[1] - w["body"]) - (p - w["root"])).length)[0]
        pairs.append((i, j))
    return pairs


def measure(build, stage, samples):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    breed = dict(dm.BREEDS["ember"], build=build, wings=arg("--wings", "classic"))
    dm.BREEDS["_probe"] = breed
    form, t = dm.STAGE[stage]
    d = dm.build_dragon("_probe", form)
    dm.pose_stage(d, t, build)
    arm = d["arm"]
    idle = {pb.name: pb.rotation_euler.to_quaternion() for pb in arm.pose.bones}
    rest = {b.name: b.matrix_local.to_quaternion() for b in arm.data.bones}
    near_root = 2.2 * dm.F["wing"]["radii"]["root"] * dm.F["wing"]["scale"]
    webs = {s: web_vertices(d, s) for s in ("L", "R")}
    bpy.context.view_layer.update()
    rest_gaps = [gaps(d, s, near_root, webs[s]) for s in ("L", "R")]
    print(f"[winggap] {build:8s} {'(rest pose)':14s} root {max(g[0] for g in rest_gaps):.3f}  web {max(g[1] for g in rest_gaps):.3f}")
    worst_root = worst_web = 0.0
    for clip in clip_lib.CLIPS:
        r_clip = w_clip = 0.0
        for k in range(samples):
            t_clip = clip.length * k / max(1, samples - 1)
            for pb in arm.pose.bones:
                q = Quaternion(clip.sample_q(pb.name, t_clip))
                pb.rotation_mode = "QUATERNION"
                pb.rotation_quaternion = idle[pb.name] @ (rest[pb.name].conjugated() @ q @ rest[pb.name])
            bpy.context.view_layer.update()
            for s in ("L", "R"):
                rg, wg = gaps(d, s, near_root, webs[s])
                r_clip, w_clip = max(r_clip, rg), max(w_clip, wg)
        worst_root, worst_web = max(worst_root, r_clip), max(worst_web, w_clip)
        print(f"[winggap] {build:8s} {clip.name:14s} root {r_clip:.3f}  web {w_clip:.3f}")
    print(f"[winggap] {build:8s} WORST          root {worst_root:.3f}  web {worst_web:.3f}")
    return worst_root, worst_web


def main():
    builds = arg("--builds", "neutral,sturdy,sleek,long").split(",")
    stage = arg("--stage", "adult")
    samples = int(arg("--samples", "6"))
    results = {b: measure(b, stage, samples) for b in builds}
    for b, (r, w) in results.items():
        print(f"[winggap] summary {b:8s} root {r:.3f}  web {w:.3f}")


main()
