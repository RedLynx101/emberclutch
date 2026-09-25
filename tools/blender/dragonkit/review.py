"""Review renders for a kind (R11b): what Noah sees before a kind goes into the game.

  blender -b -P tools/blender/dragonkit/review.py -- --kind pouncer --out C:/abs/build/review/pouncer
  ... --only lineup,variants          (some of: lineup, variants, turntable, portraits, clips, babyclips)
  ... --quick                         (smaller, faster: for iterating on a kind)

Everything is posed as the game shows it: the idle clip's first frame on the idle pose (wings
folded), the skin baked and coloured per variant, the dragon standing on the floor. Writes
PNGs into --out and tiles the sheets:
  lineup.png      egg-to-adult growth at true size (variant 0), side view
  variants.png    the four variants, grown, three-quarter (the rare one last)
  turntable.png   the adult turning round (8 views)
  portraits.png   faces: the hatchling and the adult, calm (round pupils) and startled (slit)
  clips.png       the adult's key clips, 4 frames each
  babyclips.png   the hatchling's key clips
"""
import math
import os
import subprocess
import sys

import bpy
from mathutils import Quaternion

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from dragonkit import model as km  # noqa: E402
from dragonkit import texture as tex  # noqa: E402

argv = km.argv
arg = km.arg
FPS = 30
BACKDROP = (0.40, 0.34, 0.42)   # a soft plum, like the den: pastel dragons stand out on it
FLOOR = (0.66, 0.56, 0.50)
ADULT_CLIPS = ["walk", "gallop", "sit_loop", "sleep", "play_bow", "pounce", "fly_flap", "tail_wag"]
BABY_CLIPS = ["walk", "scamper", "hop", "sit_loop", "curl_up", "fav_wiggle"]


def scene_setup(res):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene, cam = km.setup_scene(res, BACKDROP)
    floor = bpy.data.objects["floor"]
    floor.data.materials.clear()
    floor.data.materials.append(km.toon_material("floor", FLOOR))
    sun = next(o for o in bpy.data.objects if o.type == "LIGHT")
    sun.data.energy = 3.0
    return scene, cam


class Posed:
    """A built dragon at a growth stage, posed by clips as the game does it."""

    def __init__(self, stage, variant, textured=True):
        form, self.t = km.STAGE[stage]
        self.d = km.build_dragon(form, variant)
        if textured:
            km.textured(self.d)
        km.pose_stage(self.d, self.t)
        arm = self.d["arm"]
        self.idle = {pb.name: pb.rotation_euler.to_quaternion() for pb in arm.pose.bones}
        self.rest = {b.name: b.matrix_local.to_quaternion() for b in arm.data.bones}
        self.scale = km.F["export_scale"] * (0.62 + 0.38 * self.t if form == "hatchling" else 0.5 + 0.5 * self.t)
        self.clips = {c.name: c for c in km.PLAN.clips()}
        self.baby = form == "hatchling"

    def clip(self, name):
        if self.baby and f"{name}_h" in self.clips:
            return self.clips[f"{name}_h"]
        return self.clips[name]

    def frame(self, name, t):
        clip = self.clip(name)
        arm = self.d["arm"]
        for pb in arm.pose.bones:
            q = Quaternion(clip.sample_q(pb.name, t))
            local = self.rest[pb.name].conjugated() @ q @ self.rest[pb.name]
            pb.rotation_mode = "QUATERNION"
            pb.rotation_quaternion = self.idle[pb.name] @ local
        fwd, up = clip.sample_root(t)
        arm.location = (0, 0, 0)
        bpy.context.view_layer.update()
        dg = bpy.context.evaluated_depsgraph_get()
        ev = self.d["body"].evaluated_get(dg)
        me = ev.to_mesh()
        low = min((ev.matrix_world @ v.co).z for v in me.vertices)
        ev.to_mesh_clear()
        arm.location = (arm.location.x, -fwd * self.scale, -low + up * self.scale)
        bpy.context.view_layer.update()

    def idle_pose(self):
        self.frame("idle", 0.0)


def tile(out, files, cols):
    """Tile PNGs with the repo's sheet tool (a separate Blender run keeps this scene clean)."""
    blender = bpy.app.binary_path
    subprocess.run([blender, "-b", "-P", os.path.join(os.path.dirname(HERE), "sheet.py"), "--", "--out", out,
                    "--cols", str(cols), *files], check=True, capture_output=True)
    print(f"[review] {out}")


def lineup(out, res):
    ds, front = [], 0.0
    k = km.KIND.META.get("size", 1.0)
    for stage in ("newborn", "hatchling", "juvenile", "adolescent", "adult"):
        p = Posed(stage, 0)
        p.idle_pose()
        d = p.d
        s = km.F["export_scale"] * k
        d["arm"].scale = (s, s, s)
        d["arm"].location.z *= s
        bpy.context.view_layer.update()
        ys = [q.y for q in km.visible_points([d])]
        d["arm"].location.y += front - 0.35 - max(ys)
        bpy.context.view_layer.update()
        front = min(q.y for q in km.visible_points([d]))
        ds.append(d)
    km.frame_camera(bpy.context.scene.camera, ds, "side", lens=50, margin=1.4)
    bpy.context.scene.render.resolution_x, bpy.context.scene.render.resolution_y = int(res * 2.5), res
    km.render(os.path.join(out, "lineup.png"))
    bpy.context.scene.render.resolution_x = bpy.context.scene.render.resolution_y = res
    km.clear_dragons()


def variants(out, res):
    files = []
    for v in range(len(km.KIND.VARIANTS)):
        p = Posed("adult", v)
        p.idle_pose()
        km.frame_camera(bpy.context.scene.camera, [p.d], "three_quarter", margin=1.35)
        f = os.path.join(out, f"variant{v}.png")
        km.render(f)
        files.append(f)
        km.clear_dragons()
    for v in (0, len(km.KIND.VARIANTS) - 1):  # the babies of the first and the rare variant
        p = Posed("hatchling", v)
        p.idle_pose()
        km.frame_camera(bpy.context.scene.camera, [p.d], "three_quarter", margin=1.35)
        f = os.path.join(out, f"baby{v}.png")
        km.render(f)
        files.append(f)
        km.clear_dragons()
    tile(os.path.join(out, "variants.png"), files, 3)


def eggs(out):
    """The kind's egg in each variant's colours, then cracked and opening (variant 0)."""
    blender = bpy.app.binary_path
    script = os.path.join(os.path.dirname(HERE), "egg_model.py")
    files = []
    for v in range(len(km.KIND.VARIANTS)):
        prefix = os.path.join(out, f"egg{v}")
        subprocess.run([blender, "-b", "-P", script, "--", "--kind", km.KIND.META["name"], "--variant", str(v),
                        "--render", prefix], check=True, capture_output=True)
        files.append(prefix + "_warm.png")
    files += [os.path.join(out, "egg0_cracked.png"), os.path.join(out, "egg0_open.png")]
    tile(os.path.join(out, "eggs.png"), files, 3)


def turntable(out, res, n=8):
    p = Posed("adult", 0)
    p.idle_pose()
    arm = p.d["arm"]
    pts = []
    for k in range(n):
        arm.rotation_euler.z = 2 * math.pi * k / n
        bpy.context.view_layer.update()
        pts += km.visible_points([p.d])
    km.frame_points(bpy.context.scene.camera, pts, "side", lens=55, margin=0.95)
    files = []
    for k in range(n):
        arm.rotation_euler.z = 2 * math.pi * k / n
        bpy.context.view_layer.update()
        f = os.path.join(out, f"turn{k}.png")
        km.render(f)
        files.append(f)
    km.clear_dragons()
    tile(os.path.join(out, "turntable.png"), files, 4)


def portraits(out, res):
    files = []
    for stage in ("hatchling", "adult"):
        for slit in (False, True):
            if slit:
                argv.append("--slit")
            p = Posed(stage, 0)
            if slit:
                argv.remove("--slit")
            p.idle_pose()
            km.frame_camera(bpy.context.scene.camera, [p.d], "portrait")
            f = os.path.join(out, f"face_{stage}_{'slit' if slit else 'round'}.png")
            km.render(f)
            files.append(f)
            km.clear_dragons()
    tile(os.path.join(out, "portraits.png"), files, 4)


def clip_strips(out, res, stage, names, sheet, frames=4):
    """Each clip framed on its own frames (a flying clip needs more room than a sit)."""
    p = Posed(stage, 0)
    files = []
    for name in names:
        c = p.clip(name)
        times = [min(c.length, k * c.length / frames if c.loop else k * c.length / (frames - 1)) for k in range(frames)]
        pts = []
        for t in times:
            p.frame(name, t)
            pts += km.visible_points([p.d])
        km.frame_points(bpy.context.scene.camera, pts, "three_quarter", lens=55, margin=1.3)
        for k, t in enumerate(times):
            p.frame(name, t)
            f = os.path.join(out, f"{stage}_{name}_{k}.png")
            km.render(f)
            files.append(f)
    km.clear_dragons()
    tile(os.path.join(out, sheet), files, frames)


def main():
    km.use_kind(arg("--kind", "pouncer"))
    out = arg("--out", os.path.join(km.ROOT, "build", "review", km.KIND.META["name"]))
    os.makedirs(out, exist_ok=True)
    quick = "--quick" in argv
    res = 360 if quick else 560
    only = arg("--only", "lineup,eggs,variants,turntable,portraits,clips,babyclips").split(",")
    scene_setup(res)
    if "lineup" in only:
        lineup(out, res)
    if "eggs" in only:
        eggs(out)
    if "variants" in only:
        variants(out, res)
    if "turntable" in only:
        turntable(out, res)
    if "portraits" in only:
        portraits(out, res)
    if "clips" in only:
        clip_strips(out, res, "adult", ADULT_CLIPS, "clips.png")
    if "babyclips" in only:
        clip_strips(out, res, "hatchling", BABY_CLIPS, "babyclips.png")


main()
