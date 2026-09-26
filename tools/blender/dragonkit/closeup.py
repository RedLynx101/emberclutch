"""The close-up check (run 18): every kind's open mouth, looked into as the game draws it.

  blender -b -P tools/blender/dragonkit/closeup.py -- --kinds blazeplume,pouncer   (or --all)
          [--jaws 6,12,20,34] [--render C:/abs/prefix] [--quick]

The game culls back faces, so wherever the camera can see the inside of the head (through a
gap between the lips, the mouth's pocket and its side walls) that pixel shows whatever is
behind the head: the mouth looks see-through. Run 17 found it from above and the side; run 18
still saw it at the edges of a petted Blazeplume's mouth in the bottom screen's close-up,
which looks up at the face from below and in front.

For each form (the newborn and the late hatchling, the juvenile and the adult) and each jaw
opening, rays are cast through a fine grid of pixels round the mouth, from the close-up's own
camera (framed as src/app/render3d.cpp drawCloseUp does, in the idle pose and chin up for a
scratch) and from a sweep of angles round the snout: below, in front, beside and above. A ray
whose first hit is the back of a body face is a see-through pixel. Prints a line per kind and
exits 1 if any kind has one; --render writes the close-up and the worst view, culled, on a
bright backdrop (a gap shows as the backdrop).
"""
import math
import os
import sys

import bpy
from mathutils import Matrix, Quaternion, Vector
from mathutils.bvhtree import BVHTree

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from dragonkit import model as km  # noqa: E402
from dragonkit import review as rv  # noqa: E402
from eca import q_from_pyr  # noqa: E402  (tools/anim, on the path through the kit)

argv = km.argv
arg = km.arg
FOV = math.radians(38.0)                # the close-up's vertical field of view (render3d kFovY)
CLOSEUP_DIR = Vector((-0.3, -0.95, 0.18)).normalized()   # front-left of the face (drawCloseUp)
STAGES = (("newborn", "hatchling", 0.0), ("hatchling", "hatchling", 0.75),
          ("juvenile", "grown", 0.0), ("adult", "grown", 1.0))
JAWS = (6.0, 12.0, 20.0, 34.0)          # petting 7-12, eating 20, the yawn 30-34 degrees
AZIMUTHS = (-75, -50, -25, 0, 25, 50, 75)   # round the snout, degrees (0: straight ahead)
ELEVATIONS = (-60, -40, -20, 0, 20, 45)     # below (-) to above (+)
GRID = 112                              # rays across a view (the mouth fills most of it)
BACKFACE = 0.02                         # a first hit this far past edge-on counts (not a graze)
BACKDROP = (0.95, 0.10, 0.75)           # renders: a gap shows as this magenta


# ------------------------------------------------------------------------------ geometry
SKIN, POCKET, PART = 0, 1, 2  # what a triangle is: the body's skin, the mouth's inside, the rest


def scene_tris(d):
    """Every visible triangle of the dragon in world space: a BVH, the triangles' vertices and
    what each triangle is (SKIN: the body's outside, material 0; POCKET: the mouth's roof,
    floor and walls, material 2; PART: parts, wings, and the nostrils and the mouth line, which
    are thin open shapes laid on the skin, material 1)."""
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    verts, tris, what = [], [], []
    objs = [d["body"]] + d["wings"] + d["rare_wings"] + [o for g in d["groups"].values() for o in g]
    for o in objs:
        if o.hide_render:
            continue
        ev = o.evaluated_get(dg)
        me = ev.to_mesh()
        me.calc_loop_triangles()
        base = len(verts)
        mw = ev.matrix_world
        verts += [mw @ v.co for v in me.vertices]
        for lt in me.loop_triangles:
            tris.append(tuple(base + i for i in lt.vertices))
            m = lt.material_index if o is d["body"] else -1
            what.append(SKIN if m == 0 else POCKET if m == 2 else PART)
        ev.to_mesh_clear()
    return BVHTree.FromPolygons(verts, tris, all_triangles=True), verts, tris, what


def pocket_points(d):
    """The posed mouth: the pocket's vertices (material 2) and the jaw's tip, world space."""
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    ev = d["body"].evaluated_get(dg)
    me = ev.to_mesh()
    idx = {v for p in me.polygons if p.material_index == 2 for v in p.vertices}
    pts = [ev.matrix_world @ me.vertices[i].co for i in idx]
    ev.to_mesh_clear()
    arm = d["arm"]
    return pts + [arm.matrix_world @ arm.pose.bones["jaw"].tail]


def head_frame(d):
    """The snout's forward, the head's side and up (world space)."""
    arm = d["arm"]
    pb = arm.pose.bones
    fwd = (arm.matrix_world @ pb["snout"].tail - arm.matrix_world @ pb["head"].head).normalized()
    side = fwd.cross(Vector((0, 0, 1)))
    if side.length < 1e-3:
        side = Vector((1, 0, 0))
    side.normalize()
    return fwd, side, side.cross(fwd).normalized()


# ------------------------------------------------------------------------------ cameras
class View:
    """A pinhole camera: eye, look-at target, vertical field of view, aspect (w/h)."""

    def __init__(self, name, eye, target, fov=FOV, aspect=1.0):
        self.name, self.eye, self.target, self.fov, self.aspect = name, eye, target, fov, aspect
        f = (target - eye).normalized()
        r = f.cross(Vector((0, 0, 1)))
        if r.length < 1e-4:
            r = Vector((1, 0, 0))
        r.normalize()
        self.f, self.r, self.u = f, r, r.cross(f).normalized()

    def ray(self, sx, sy):
        """The ray through screen point (sx, sy) in -1..1 (y up)."""
        k = math.tan(self.fov * 0.5)
        return (self.f + self.r * (sx * k * self.aspect) + self.u * (sy * k)).normalized()

    def camera(self, cam, res):
        cam.data.sensor_fit = "VERTICAL"
        cam.data.angle_y = self.fov
        cam.location = self.eye
        cam.rotation_euler = Matrix((self.r, self.u, -self.f)).transposed().to_euler()
        scene = bpy.context.scene
        scene.render.resolution_y = res
        scene.render.resolution_x = int(round(res * self.aspect))


def closeup_view(d, form, joints):
    """The bottom screen's close-up of the face, framed as drawCloseUp frames it (the dragon
    faces -Y at heading 0; the camera sits front-left of the face, a touch above the target,
    which is between the head and the chest, so it looks up at a raised head)."""
    arm = d["arm"]
    pb = arm.pose.bones
    head = arm.matrix_world @ pb["head"].head
    chest = arm.matrix_world @ pb["chest"].head
    mouth = arm.matrix_world @ pb["jaw"].tail
    target = head.lerp(chest, 0.3)
    radius = (head - chest).length * 0.75
    if form == "hatchling":  # its head sits right on its chest: the front of it
        lo = Vector((min(p.x for p in joints), min(p.y for p in joints), min(p.z for p in joints)))
        hi = Vector((max(p.x for p in joints), max(p.y for p in joints), max(p.z for p in joints)))
        radius = max(radius, (hi - lo).length * 0.5 * 1.25 * 0.42)
    else:
        span = (mouth - head).length
        if radius > span * 1.6:
            radius = span * 1.6
            target = head.lerp(chest, 0.08)
    dist = radius / math.tan(FOV * 0.5)
    return View("closeup", target + CLOSEUP_DIR * dist, target, FOV, 320 / 240)


def joint_points(d):
    arm = d["arm"]
    return [arm.matrix_world @ pb.head for pb in arm.pose.bones]


def sweep_views(centre, reach, frame):
    """Cameras all round the front of the snout, each aimed at the mouth from close by."""
    fwd, side, up = frame
    dist = reach / math.tan(FOV * 0.5) / 0.8
    out = []
    for el in ELEVATIONS:
        for az in AZIMUTHS:
            a, e = math.radians(az), math.radians(el)
            to_eye = (fwd * math.cos(a) + side * math.sin(a)) * math.cos(e) + up * math.sin(e)
            out.append(View(f"az{az:+d}_el{el:+d}", centre + to_eye * dist, centre))
    return out


# ------------------------------------------------------------------------------ the rays
def culled_ray(bvh, tris, what, verts, eye, dvec, fold):
    """Follow a ray as the game draws it, back faces culled, to the first face turned toward
    it (what the pixel shows). True if on the way it went out through the back of the skin,
    i.e. it was inside the head, and what it shows is not the mouth's inside: nothing (the
    backdrop), or skin or a part more than `fold` beyond (the neck, the body, beyond the head).
    Backs of the pocket's walls (seen from outside the mouth, by design), of parts (a tooth's
    open root) and of the nostrils' little cups are passed through; so is a patch of lip skin
    folded over by a wide-open jaw, which shows the pocket or its own other layer behind it."""
    at, out_at = eye, None
    for _ in range(16):
        loc, _, index, _ = bvh.ray_cast(at, dvec)
        if loc is None:
            return out_at is not None
        a, b, c = (verts[k] for k in tris[index])
        n = (b - a).cross(c - a)
        facing = n.dot(dvec) / max(n.length, 1e-12)
        if facing < BACKFACE:  # turned toward the ray (or edge-on): drawn, the ray stops here
            if out_at is None or what[index] == POCKET:
                return False
            return (loc - out_at).length > fold
        if what[index] == SKIN and out_at is None:
            out_at = loc
        at = loc + dvec * 1e-5
    return False


def see_through(bvh, tris, what, verts, view, centre, reach, grid=GRID):
    """Rays through a grid over the view that pass near the mouth: [where each see-through
    ray passes the mouth] (see culled_ray)."""
    bad = []
    rows = grid if view.aspect <= 1 else int(grid / view.aspect)
    cols = int(rows * view.aspect)
    for j in range(rows):
        sy = 1 - (j + 0.5) * 2 / rows
        for i in range(cols):
            sx = (i + 0.5) * 2 / cols - 1
            dvec = view.ray(sx, sy)
            to_c = centre - view.eye
            along = to_c.dot(dvec)
            if (to_c - dvec * along).length > reach:  # nowhere near the mouth
                continue
            fold = reach * 0.4
            if not culled_ray(bvh, tris, what, verts, view.eye, dvec, fold):
                continue
            # A ray exactly on an edge can slip between two triangles that share it (the
            # GPU's raster can't): a real gap lets most of its near neighbours through too.
            e = 0.25 * 2 / rows
            near = sum(culled_ray(bvh, tris, what, verts, view.eye, view.ray(sx + dx, sy + dy), fold)
                       for dx, dy in ((e, e), (-e, e), (e, -e), (-e, -e)))
            if near >= 2:
                bad.append(view.eye + dvec * along)
    return bad


def where(points, centre, frame, width):
    """Where see-through pixels are, in words: across (side), along the snout, up/down."""
    fwd, side, up = frame
    n = len(points)
    m = sum((p - centre for p in points), Vector()) / n
    x = sum(abs((p - centre).dot(side)) for p in points) / n / width
    return f"at |x| {x:.2f} of the mouth's half-width, {m.dot(fwd) / width:+.2f} along, {m.dot(up) / width:+.2f} up"


# ------------------------------------------------------------------------------ renders
def render_view(view, path, res=360):
    cam = bpy.context.scene.camera
    view.camera(cam, res)
    km.render(path)


def culled_look():
    """As the game draws it: back faces culled, a magenta backdrop behind any gap."""
    for m in bpy.data.materials:
        m.use_backface_culling = True
    nt = bpy.context.scene.world.node_tree  # the camera sees magenta; the light stays the review's
    lit = nt.nodes["Background"]
    seen = nt.nodes.new("ShaderNodeBackground")
    seen.inputs["Color"].default_value = (*BACKDROP, 1)
    path = nt.nodes.new("ShaderNodeLightPath")
    mix = nt.nodes.new("ShaderNodeMixShader")
    nt.links.new(path.outputs["Is Camera Ray"], mix.inputs["Fac"])
    nt.links.new(lit.outputs[0], mix.inputs[1])
    nt.links.new(seen.outputs[0], mix.inputs[2])
    out = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeOutputWorld")
    nt.links.new(mix.outputs[0], out.inputs["Surface"])
    floor = bpy.data.objects.get("floor")
    if floor:
        floor.hide_render = True


# ------------------------------------------------------------------------------ main
CLIPS = {}


def pose_for(d, stage_t, jaw, chin_up):
    """The idle clip at growth t (or pet_chin's lifted head: chin up for a scratch), the jaw
    opened as the game opens it, a pitch delta over the pose (render3d's jawOpen)."""
    km.pose_stage(d, stage_t)
    arm = d["arm"]
    idle = {pb.name: pb.rotation_euler.to_quaternion() for pb in arm.pose.bones}
    rest = {b.name: b.matrix_local.to_quaternion() for b in arm.data.bones}
    name = "pet_chin" if chin_up else "idle"
    clip = (CLIPS.get(name + "_h") if d["form"] == "hatchling" else None) or CLIPS[name]
    for pb in arm.pose.bones:
        q = q_from_pyr(-jaw, 0, 0) if pb.name == "jaw" else clip.sample_q(pb.name, 0.08)
        local = rest[pb.name].conjugated() @ Quaternion(q) @ rest[pb.name]
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = idle[pb.name] @ local
    bpy.context.view_layer.update()


def check_kind(kind, jaws, render_prefix, quick):
    km.use_kind(kind)
    CLIPS.clear()
    CLIPS.update({c.name: c for c in km.PLAN.clips()})
    results, worst = [], None
    stages = STAGES[1::2] if quick else STAGES
    for form in ("hatchling", "grown"):
        if not km.KIND.FORMS[form].get("mouth", True):
            continue
        rv.scene_setup(360)
        d = km.build_dragon(form, 0)
        culled_look()
        for stage, f, t in stages:
            if f != form:
                continue
            for jaw in jaws:
                for chin_up in (False, True):
                    pose_for(d, t, jaw, chin_up)
                    bvh, verts, tris, what = scene_tris(d)
                    mouth = pocket_points(d)
                    centre = sum(mouth, Vector()) / len(mouth)
                    reach = max((p - centre).length for p in mouth) * 1.15
                    frame = head_frame(d)
                    width = km.F["face"]["mouth"](1, 1.0)[0] or reach
                    views = [closeup_view(d, form, joint_points(d))]
                    if not chin_up:
                        views += sweep_views(centre, reach, frame)
                    for view in views:
                        bad = see_through(bvh, tris, what, verts, view, centre, reach)
                        if bad:
                            label = f"{stage} jaw {jaw:g}{' chin up' if chin_up else ''} {view.name}"
                            results.append((label, len(bad), where(bad, centre, frame, width)))
                            if worst is None or len(bad) > worst[0]:
                                worst = (len(bad), stage, jaw, chin_up, view.name)
        if render_prefix:
            for stage, f, t in stages:
                if f != form:
                    continue
                for chin_up in (False, True):
                    pose_for(d, t, 12.0, chin_up)
                    render_view(closeup_view(d, form, joint_points(d)),
                                f"{render_prefix}_{kind}_{stage}_closeup{'_chin' if chin_up else ''}.png")
                pose_for(d, t, 20.0, False)
                mouth = pocket_points(d)
                centre = sum(mouth, Vector()) / len(mouth)
                reach = max((p - centre).length for p in mouth) * 1.15
                for view in sweep_views(centre, reach, head_frame(d)):
                    if view.name in ("az-25_el-40", "az-50_el-20", "az+0_el-60"):
                        render_view(view, f"{render_prefix}_{kind}_{stage}_{view.name}.png")
            if worst and worst[1] in [s for s, f, _ in stages if f == form]:
                _, stage, jaw, chin_up, name = worst
                t = next(t for s, _, t in STAGES if s == stage)
                pose_for(d, t, jaw, chin_up)
                mouth = pocket_points(d)
                centre = sum(mouth, Vector()) / len(mouth)
                reach = max((p - centre).length for p in mouth) * 1.15
                views = [closeup_view(d, form, joint_points(d))] + sweep_views(centre, reach, head_frame(d))
                view = next(v for v in views if v.name == name)
                render_view(view, f"{render_prefix}_{kind}_worst_{stage}_jaw{jaw:g}_{name}.png")
        km.clear_dragons()
    total = sum(n for _, n, _ in results)
    print(f"[closeup] {kind}: {'OK' if not results else 'SEE-THROUGH'}"
          f" ({len(results)} views with gaps, {total} rays)")
    for label, n, at in sorted(results, key=lambda r: -r[1])[:12]:
        print(f"[closeup]   {label}: {n} rays, {at}")
    return not results


def main():
    import dragons
    names = [m.META["name"] for m in dragons.all_kinds()] if "--all" in argv else arg("--kinds", "blazeplume").split(",")
    jaws = [float(j) for j in arg("--jaws", ",".join(f"{j:g}" for j in JAWS)).split(",")]
    ok = True
    for name in names:
        ok &= check_kind(name, jaws, arg("--render"), "--quick" in argv)
    print(f"[closeup] {'all clear' if ok else 'see-through mouths found'}")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
