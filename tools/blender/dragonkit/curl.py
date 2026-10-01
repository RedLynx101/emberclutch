"""The curl-up check (run 18): every kind curled up and asleep, as the game poses and stands it.

  blender -b -P tools/blender/dragonkit/curl.py -- --kinds pouncer,blazeplume   (or --plan pouncer, --all)
          [--render C:/abs/prefix] [--texture] [--baseline]

For each kind's hatchling and adult: the curl_up clip halfway and at its end, and sleep at the
top of a breath, each posed on the idle pose as the game does it and stood on the floor the way
the game stands it (render3d animatedGround: the lowest body vertex that isn't mostly tail;
the clip's root lift on top). Then it measures what reads wrong in a curled-up dragon:

  floor   wings, parts or the tail below the floor (the game stands it on its skin only),
          past what the idle pose has
  wings   wing vertices past the wing's first bone inside the body (a wing through the back
          or the flank), past what the idle pose has (a folded wing already hugs the flank)
  pokes   a leg's lower half or foot, the tail (past its first bone) or the head inside
          another part of the body: the torso, a leg, the tail, the head
  head    air under the head's lowest point, down to the floor or the rest of the body (a
          sleeping head rests on something: the floor, its paws, its tail, its flank); not
          halfway through curl_up, when it's still on its way down

Depths are in body lengths (hips to snout tip standing, at the stage's growth), so the limits
read the same for every kind. Prints a line per kind and form with the worst frame of each
(--baseline: the idle pose's numbers too), and exits 1 if anything is past its limit. --render
writes each frame from four views, culled, and tiles them into <prefix>_<kind>.png (a row per
frame: hatchling, then adult).
"""
import os
import sys

import bpy
from mathutils import Quaternion, Vector
from mathutils.bvhtree import BVHTree

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from dragonkit import model as km  # noqa: E402
from dragonkit import review as rv  # noqa: E402

argv = km.argv
arg = km.arg
STAGES = (("hatchling", "hatchling", 0.75), ("adult", "grown", 1.0))
VIEWS = tuple(arg("--views", "three_quarter,top,back_quarter,front_low").split(","))  # (--views side,...: any of km.VIEWS)
km.VIEWS["front_low"] = Vector((0.35, -1.0, 0.15))
FLOOR_LIMIT = 0.02    # body lengths below the floor, past the idle pose's
WING_LIMIT = 0.04     # body lengths inside the body, past the idle pose's
POKE_LIMIT = 0.1      # body lengths inside another part of the body (a tucked paw sinks a little)
HEAD_LIMIT = 0.12     # body lengths of air under a sleeping head
TORSO = ("hips", "belly", "chest", "loin")


def frames_for(clips, baby):
    """(label, clip, time): curl_up halfway and at its end, sleep at the top of a breath."""
    def pick(name):
        return (clips.get(name + "_h") if baby else None) or clips[name]
    curl, sleep = pick("curl_up"), pick("sleep")
    return [("curl_up half", curl, curl.length * 0.5), ("curl_up end", curl, curl.length),
            ("sleep breath", sleep, sleep.length * 0.25)]


# ------------------------------------------------------------------------------ posing
class Pose:
    """A built dragon posed by clips on its idle pose and stood on the floor as the game does."""

    def __init__(self, d, t):
        self.d, self.t = d, t
        km.pose_stage(d, t)
        arm = d["arm"]
        self.idle = {pb.name: pb.rotation_euler.to_quaternion() for pb in arm.pose.bones}
        self.rest = {b.name: b.matrix_local.to_quaternion() for b in arm.data.bones}
        form = d["form"]
        self.scale = km.F["export_scale"] * (0.62 + 0.38 * t if form == "hatchling" else 0.5 + 0.5 * t)
        names = {g.index: g.name for g in d["body"].vertex_groups}
        self.tail = []  # per body vertex: mostly tail (the game leaves those out of the floor contact)
        for v in d["body"].data.vertices:
            best = max(v.groups, key=lambda g: g.weight, default=None)
            total = sum(g.weight for g in v.groups) or 1.0
            self.tail.append(bool(best) and names[best.group].startswith("tail") and best.weight / total >= 0.5)

    def frame(self, clip, ct):
        arm = self.d["arm"]
        for pb in arm.pose.bones:
            q = Quaternion(clip.sample_q(pb.name, ct))
            pb.rotation_mode = "QUATERNION"
            pb.rotation_quaternion = self.idle[pb.name] @ (self.rest[pb.name].conjugated() @ q @ self.rest[pb.name])
        _, up = clip.sample_root(ct)
        arm.location = (0, 0, 0)
        bpy.context.view_layer.update()
        dg = bpy.context.evaluated_depsgraph_get()
        ev = self.d["body"].evaluated_get(dg)
        me = ev.to_mesh()
        low = min((ev.matrix_world @ v.co).z for v in me.vertices if not self.tail[v.index])
        ev.to_mesh_clear()
        arm.location = (0, 0, -low + up * self.scale)
        bpy.context.view_layer.update()


# ------------------------------------------------------------------------------ measuring
def region_of(bone):
    b = bone[:-2] if bone.endswith(("_L", "_R")) else bone
    if b in TORSO:
        return "torso"
    if b.startswith("tail"):
        return "tail1" if b == "tail1" else "tail"
    if b in ("head", "snout", "jaw", "eyes") or b.startswith(("ear", "antenna")):
        return "head"
    if b.startswith("neck"):
        return "neck"
    side = bone[-2:] if bone.endswith(("_L", "_R")) else ""
    if b.startswith(("arm_lo", "hand")):
        return "fore" + side
    if b.startswith(("leg_lo", "foot")):
        return "hind" + side
    if b.startswith(("arm", "leg")):
        return "limb_up"
    return "other"


RAYS = [Vector(v).normalized() for v in ((0.93, 0.21, 0.30), (-0.87, 0.35, 0.33), (0.12, -0.95, 0.28),
                                         (0.25, 0.90, -0.36), (-0.31, -0.40, -0.86), (0.55, -0.20, 0.81),
                                         (-0.62, 0.58, -0.53))]


def inside(tree, p):
    """Inside the surface by the crossings of rays out from p: a part of the body with the
    joining part left out is open where they met, so a ray may leave through that hole; most
    of seven rays in different directions must agree."""
    odd = 0
    for d in RAYS:
        n, at = 0, p
        for _ in range(24):
            hit, _, _, _ = tree.ray_cast(at, d)
            if hit is None:
                break
            n += 1
            at = hit + d * 1e-5
        odd += n % 2
    return odd >= 5


def measure(d, length):
    """The frame's worst numbers, in body lengths: {test: (value, where)}."""
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    body = d["body"]
    names = {g.index: g.name for g in body.vertex_groups}
    ev = body.evaluated_get(dg)
    me = ev.to_mesh()
    me.calc_loop_triangles()
    mw = ev.matrix_world
    pos = [mw @ v.co for v in me.vertices]
    region = []
    for v in body.data.vertices:  # the evaluated mesh keeps the vertex order (armature only)
        best = max(v.groups, key=lambda g: g.weight, default=None)
        region.append(region_of(names[best.group]) if best else "torso")
    tris = [tuple(lt.vertices) for lt in me.loop_triangles if lt.material_index == 0]
    ev.to_mesh_clear()
    out = {}
    # floor: parts, wings and the tail below it
    low, where = 0.0, ""
    for group, objs in [("wings", d["wings"])] + list(d["groups"].items()):
        for o in objs:
            if o.hide_render:
                continue
            e = o.evaluated_get(dg)
            m = e.to_mesh()
            z = min(((e.matrix_world @ v.co).z for v in m.vertices), default=0.0)
            e.to_mesh_clear()
            if z < low:
                low, where = z, group
    tail_z = min((p.z for p, r in zip(pos, region) if r in ("tail", "tail1")), default=0.0)
    if tail_z < low:
        low, where = tail_z, "tail"
    out["floor"] = (-low / length, where)
    # wings inside the body
    whole = BVHTree.FromPolygons(pos, tris, all_triangles=True)
    depth, where = 0.0, ""
    first = {f"{n}_{s}" for n, *_ in km.PLAN.WING_CHAIN[:1] for s in "LR"}
    for o in d["wings"]:
        if o.hide_render:
            continue
        # The arm's root and the membrane's inner edge are sunk into the body by design
        # (model.WING_SINK, WEB_SINK): only what rides on the wing past its first bone counts.
        gname = {g.index: g.name for g in o.vertex_groups}
        e = o.evaluated_get(dg)
        m = e.to_mesh()
        for v in m.vertices:
            best = max(o.data.vertices[v.index].groups, key=lambda g: g.weight, default=None)
            if best is None or not gname[best.group].startswith("wing") or gname[best.group] in first:
                continue
            p = e.matrix_world @ v.co
            _, _, _, dist = whole.find_nearest(p)
            if dist is not None and dist > depth and inside(whole, p):
                depth, where = dist, o.name
        e.to_mesh_clear()
    out["wings"] = (depth / length, where)
    # pokes: one part of the body inside another
    tri_region = [max((region[i] for i in t), key=[region[i] for i in t].count) for t in tris]
    trees = {}

    def tree_without(skip):
        key = tuple(sorted(skip))
        if key not in trees:
            keep = [t for t, r in zip(tris, tri_region) if r not in skip]
            trees[key] = BVHTree.FromPolygons(pos, keep, all_triangles=True) if keep else None
        return trees[key]

    depth, where = 0.0, ""
    movers = sorted({r for r in region if r in ("tail", "head") or r.startswith(("fore", "hind"))})
    for r in movers:
        # its own bone's neighbours are joined to it: leave them out of what it can be inside
        skip = {r, "neck"} if r == "head" else {r, "tail1"} if r == "tail" else {r, "limb_up"}
        tree = tree_without(skip)
        if tree is None:
            continue
        for p, rv_ in zip(pos, region):
            if rv_ != r:
                continue
            _, _, _, dist = tree.find_nearest(p)
            if dist is not None and dist > depth and inside(tree, p):
                depth, where = dist, r
    out["pokes"] = (depth / length, where)
    # head: air under the head's lowest point (down to the floor or the rest of the body)
    head = [p for p, r in zip(pos, region) if r == "head"]
    if head:
        chin = min(head, key=lambda p: p.z)
        rest = tree_without({"head", "neck"})
        hit = rest.ray_cast(chin + Vector((0, 0, -1e-4)), Vector((0, 0, -1))) if rest else (None,)
        gap = chin.z if hit[0] is None else min(chin.z, chin.z - hit[0].z)
        torso_low = min((p.z for p, r in zip(pos, region) if r == "torso"), default=0.0)
        out["head"] = (max(0.0, gap) / length, f"torso {torso_low / length:.2f} up")
    return out


LIMITS = {"floor": FLOOR_LIMIT, "wings": WING_LIMIT, "pokes": POKE_LIMIT, "head": HEAD_LIMIT}


def body_length(d, t):
    """Hips to snout tip in the grown adult pose, scaled by growth (hatchlings: their own)."""
    arm = d["arm"]
    pb = arm.pose.bones
    return (arm.matrix_world @ pb["snout"].tail - arm.matrix_world @ pb[km.PLAN.BONES[0][0]].head).length


# ------------------------------------------------------------------------------ main
def check_kind(kind, render_prefix, texture):
    km.use_kind(kind)
    clips = {c.name: c for c in km.PLAN.clips()}
    ok, files, lines = True, [], []
    rv.scene_setup(300)
    for stage, form, t in STAGES:
        d = km.build_dragon(form, 0)
        if texture:
            km.textured(d)
        for m in bpy.data.materials:
            m.use_backface_culling = True
        pose = Pose(d, t)
        pose.frame(clips["idle"], 0.0)
        length = body_length(d, t)
        # Standing at rest a folded wing already hugs the flank (its arm and the leaves' roots a
        # little into it) and a low frill may dip a hair under the floor: those two count only
        # past what the idle pose has.
        base = measure(d, length)
        if "--baseline" in argv:
            print(f"[curl]   {stage:9s} idle: " + ", ".join(f"{k} {v:.3f} ({w})" for k, (v, w) in base.items()))
        worst = {k: (0.0, "", "") for k in LIMITS}
        for label, clip, ct in frames_for(clips, form == "hatchling"):
            pose.frame(clip, ct)
            for k, (v, where) in measure(d, length).items():
                if k == "head" and "half" in label:  # on its way down the head is still up
                    continue
                if k in ("wings", "floor"):
                    v = max(0.0, v - base[k][0])
                if v > worst[k][0]:
                    worst[k] = (v, where, label)
            if render_prefix:
                pts = km.visible_points([d])
                for view in VIEWS:
                    km.frame_points(bpy.context.scene.camera, pts, view, lens=55, margin=1.6)
                    f = f"{render_prefix}_{kind}_{stage}_{label.replace(' ', '_')}_{view}.png"
                    km.render(f)
                    files.append(f)
        bad = [k for k, (v, _, _) in worst.items() if v > LIMITS[k]]
        ok &= not bad
        lines.append(f"[curl]   {stage:9s} " + ", ".join(
            f"{k} {v:.3f}{'!' if v > LIMITS[k] else ''}" + (f" ({w}, {lab})" if v > 0.3 * LIMITS[k] else "")
            for k, (v, w, lab) in worst.items()))
        km.clear_dragons()
    print(f"[curl] {kind} ({km.PLAN.NAME}): {'OK' if ok else 'PROBLEMS'}")
    for ln in lines:
        print(ln)
    if render_prefix and files:
        rv.tile(f"{render_prefix}_{kind}.png", files, len(VIEWS))
    return ok


def main():
    import dragons
    kinds = dragons.all_kinds()
    if "--all" in argv:
        names = [m.META["name"] for m in kinds]
    elif arg("--plan"):
        names = [m.META["name"] for m in kinds if m.META["plan"] in arg("--plan").split(",")]
    else:
        names = arg("--kinds", "pouncer").split(",")
    ok = True
    for name in names:
        ok &= check_kind(name, arg("--render"), "--texture" in argv)
    print(f"[curl] {'all curled up nicely' if ok else 'poses to look at'}")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
