"""Preview sheets and portraits for the people (tools/people): Blender renders the meshes the
pure-Python kit builds, posed with the game's own maths (tools/people/rig.py) and coloured by
their palettes, in a toon look close to the game's.

  blender -b -P tools/blender/people_model.py -- --out C:/abs/build/people
          [--sheets models,hair,clips,portraits] [--only player_a,keeper] [--res 360]
          [--portrait-dir C:/abs/assets/sprites/people]

Sheets (PNG, in --out): models.png (each person front and three-quarter), hair_<body>.png (the
six hair styles, front and three-quarter), clips_<person>.png (key frames of the main clips),
ride.png (the riding clips on a stand-in dragon's back), looks.png (the creator's colours).
Portraits: 64 x 64 transparent PNGs of each villager's head and shoulders, smiling.
"""
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
for _p in (os.path.join(ROOT, "tools", "anim"), os.path.join(ROOT, "tools", "people")):
    if _p not in sys.path:
        sys.path.insert(0, _p)
import looks  # noqa: E402
import people  # noqa: E402
import person_clips  # noqa: E402
import rig  # noqa: E402
from eca import FPS  # noqa: E402

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


OUT = arg("--out", os.path.join(ROOT, "build", "people"))
RES = int(arg("--res", "360"))
TILES = os.path.join(OUT, "tiles")
BACKDROP = (0.93, 0.87, 0.76)


# ------------------------------------------------------------------------------ scene
def toon_material():
    """The dragons' review look (dragonkit.model.toon_material) with the colour from the vertex
    paint: a stepped plum-shadow ramp, a warm rim, plus the emissive paint."""
    mat = bpy.data.materials.new("person")
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    diff = nt.nodes.new("ShaderNodeBsdfDiffuse")
    s2r = nt.nodes.new("ShaderNodeShaderToRGB")
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.interpolation = "CONSTANT"
    els = ramp.color_ramp.elements
    els[0].position, els[0].color = 0.0, (0.52, 0.46, 0.54, 1)
    els[1].position, els[1].color = 0.2, (0.83, 0.79, 0.81, 1)
    e = els.new(0.5)
    e.color = (1.0, 0.98, 0.95, 1)
    nt.links.new(diff.outputs[0], s2r.inputs[0])
    nt.links.new(s2r.outputs["Color"], ramp.inputs["Fac"])
    col = nt.nodes.new("ShaderNodeAttribute")
    col.attribute_name = "paint"
    emit = nt.nodes.new("ShaderNodeAttribute")
    emit.attribute_name = "emit"
    mul = nt.nodes.new("ShaderNodeMix")
    mul.data_type = "RGBA"
    mul.blend_type = "MULTIPLY"
    mul.inputs["Factor"].default_value = 1.0
    nt.links.new(col.outputs["Color"], mul.inputs["A"])
    nt.links.new(ramp.outputs["Color"], mul.inputs["B"])
    lw = nt.nodes.new("ShaderNodeLayerWeight")
    lw.inputs["Blend"].default_value = 0.35
    rr = nt.nodes.new("ShaderNodeValToRGB")
    rr.color_ramp.interpolation = "CONSTANT"
    rr.color_ramp.elements[0].color = (0, 0, 0, 1)
    rr.color_ramp.elements[1].position = 0.74
    rr.color_ramp.elements[1].color = (0.3, 0.22, 0.15, 1)
    nt.links.new(lw.outputs["Facing"], rr.inputs["Fac"])
    add1 = nt.nodes.new("ShaderNodeMix")
    add1.data_type = "RGBA"
    add1.blend_type = "ADD"
    add1.inputs["Factor"].default_value = 1.0
    nt.links.new(mul.outputs["Result"], add1.inputs["A"])
    nt.links.new(rr.outputs["Color"], add1.inputs["B"])
    glow = nt.nodes.new("ShaderNodeMix")  # + colour x emissive (the game's TEV adds it)
    glow.data_type = "RGBA"
    glow.blend_type = "ADD"
    nt.links.new(emit.outputs["Fac"], glow.inputs["Factor"])
    nt.links.new(add1.outputs["Result"], glow.inputs["A"])
    nt.links.new(col.outputs["Color"], glow.inputs["B"])
    em = nt.nodes.new("ShaderNodeEmission")
    nt.links.new(glow.outputs["Result"], em.inputs["Color"])
    nt.links.new(em.outputs[0], out.inputs["Surface"])
    return mat


def flat_material(name, color):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    diff = nt.nodes.new("ShaderNodeBsdfDiffuse")
    s2r = nt.nodes.new("ShaderNodeShaderToRGB")
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.interpolation = "CONSTANT"
    els = ramp.color_ramp.elements
    els[0].color = tuple(c * 0.62 for c in color) + (1,)
    els[1].position, els[1].color = 0.3, tuple(color) + (1,)
    nt.links.new(diff.outputs[0], s2r.inputs[0])
    nt.links.new(s2r.outputs["Color"], ramp.inputs["Fac"])
    em = nt.nodes.new("ShaderNodeEmission")
    nt.links.new(ramp.outputs["Color"], em.inputs["Color"])
    nt.links.new(em.outputs[0], out.inputs["Surface"])
    return mat


def setup_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_EEVEE"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = scene.render.resolution_y = RES
    scene.view_settings.view_transform = "Standard"
    world = bpy.data.worlds.new("world")
    scene.world = world
    world.use_nodes = True
    nt = world.node_tree
    bg = nt.nodes["Background"]
    bg.inputs["Color"].default_value = (*BACKDROP, 1)
    # the camera sees the warm backdrop; the people get only a soft share of it as ambient,
    # so the toon ramp shows their form (the game: a plum ambient + one key light)
    amb = nt.nodes.new("ShaderNodeBackground")
    amb.inputs["Color"].default_value = (0.62, 0.56, 0.66, 1)
    amb.inputs["Strength"].default_value = 0.35
    lp = nt.nodes.new("ShaderNodeLightPath")
    mix = nt.nodes.new("ShaderNodeMixShader")
    nt.links.new(lp.outputs["Is Camera Ray"], mix.inputs["Fac"])
    nt.links.new(amb.outputs[0], mix.inputs[1])
    nt.links.new(bg.outputs[0], mix.inputs[2])
    nt.links.new(mix.outputs[0], nt.nodes["World Output"].inputs["Surface"])
    bpy.ops.object.light_add(type="SUN", rotation=(math.radians(52), math.radians(8), math.radians(-34)))
    bpy.context.object.data.energy = 3.2
    bpy.ops.mesh.primitive_circle_add(vertices=48, radius=1.0, fill_type="NGON", location=(0, 0, 0))
    floor = bpy.context.object
    floor.name = "floor"
    floor.scale = (1.4, 1.4, 1)
    floor.data.materials.append(flat_material("floor", (0.80, 0.70, 0.56)))
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    scene.collection.objects.link(cam)
    scene.camera = cam
    return scene, cam


# ------------------------------------------------------------------------------ people in Blender
class Shown:
    """A person's meshes in the scene (body + eyes + one hair style), posable."""

    def __init__(self, p, palette, hair=0, eyes=0):
        self.p = p
        self.parts = [p.body, p.eyes[eyes]] + ([p.hair[hair]] if p.hair else [])
        self.objs = [make_object(f"{p.id}_{i}", m, palette) for i, m in enumerate(self.parts)]
        self.offset = Vector((0, 0, 0))
        self.yaw = 0.0

    def pose(self, clip=None, t=0.0, floor=True, seat=None):
        """Pose with a clip at time t; stand the feet on the floor (or put the seat point at
        `seat`), add the clip's root track."""
        local = self.p.skel.clip_pose(clip, t)
        skin, _ = self.p.skel.pose(local)
        posed = [rig.skin_points(skin, m.pos, m.w) for m in self.parts]
        fwd, up = clip.sample_root(t) if clip is not None else (0.0, 0.0)
        dz = 0.0
        if seat is not None:
            s = seat_point(self.p, skin)
            dz = seat[2] - s[2]
            shift = Vector((seat[0] - s[0], seat[1] - s[1], dz + up))
        else:
            if floor:  # the legs and feet meet the floor (props such as a staff don't)
                legs = [q for q, w in zip(posed[0], self.parts[0].w) if any(b.startswith(("foot", "leg")) for b, _ in w)]
                dz = -min(q[2] for q in (legs or posed[0]))
            shift = Vector((0.0, -fwd, dz + up))
        cy, sy = math.cos(self.yaw), math.sin(self.yaw)
        for o, pts in zip(self.objs, posed):
            flat = []
            for q in pts:
                x, y, z = q[0] + shift.x, q[1] + shift.y, q[2] + shift.z
                flat += [x * cy - y * sy + self.offset.x, x * sy + y * cy + self.offset.y, z + self.offset.z]
            o.data.vertices.foreach_set("co", flat)
            o.data.update()

    def points(self):
        pts = []
        for o in self.objs:
            pts += [v.co.copy() for v in o.data.vertices]
        return pts

    def remove(self):
        for o in self.objs:
            me = o.data
            bpy.data.objects.remove(o, do_unlink=True)
            bpy.data.meshes.remove(me)


MATERIAL = None


def make_object(name, m, palette):
    global MATERIAL
    if MATERIAL is None:
        MATERIAL = toon_material()
    me = bpy.data.meshes.new(name)
    me.from_pydata([tuple(p) for p in m.pos], [], [tuple(t) for t in m.tris])
    me.update()
    for poly in me.polygons:
        poly.use_smooth = True
    lin = [looks.linear(c) for c in looks.palette_list(palette)]
    col = me.color_attributes.new("paint", "FLOAT_COLOR", "POINT")
    em = me.attributes.new("emit", "FLOAT", "POINT")
    cols, emits = [], []
    for mat in m.mat:
        a, b, mix, e = looks.PAINT[mat]
        f = mix / 255.0
        c = [lin[a][k] + (lin[b][k] - lin[a][k]) * f for k in range(3)]
        cols += c + [1.0]
        emits.append(e / 255.0 * 1.4)
    col.data.foreach_set("color", cols)
    em.data.foreach_set("value", emits)
    me.materials.append(MATERIAL)
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def seat_point(p, skin):
    """Where the person sits: the lowest point of the seat under the hips (see people-kit.md)."""
    s = rig.m_point(skin[rig.INDEX["hips"]], p.notes["seat"])
    return s


# ------------------------------------------------------------------------------ cameras & renders
VIEWS = {"front": Vector((0.0, -1.0, 0.16)), "three_quarter": Vector((-0.72, -0.95, 0.3)),
         "side": Vector((-1.0, 0.0, 0.12)), "back": Vector((0.3, 1.0, 0.25)),
         "portrait": Vector((-0.42, -1.0, 0.08)), "high": Vector((-0.5, -0.9, 0.75)),
         "chase": Vector((0.3, 1.0, 0.6))}


def frame(cam, center, height, view, lens=70.0, aspect=1.0):
    d = VIEWS[view].normalized()
    cam.data.lens = lens
    fov = 2 * math.atan(18.0 / lens)  # sensor 36 mm
    dist = (height * 0.5) / math.tan(fov / 2)
    cam.location = Vector(center) + d * dist
    cam.rotation_euler = (Vector(center) - cam.location).to_track_quat("-Z", "Y").to_euler()


def render(path):
    bpy.context.scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    return path


def sheet(files, cols, out, labels=None):
    """Tile PNGs into one image (Blender's numpy; rows top to bottom)."""
    tiles = []
    for f in files:
        img = bpy.data.images.load(f)
        w, h = img.size
        tiles.append(np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4))
        bpy.data.images.remove(img)
    th, tw = tiles[0].shape[:2]
    rows = (len(tiles) + cols - 1) // cols
    px = np.ones((rows * th, cols * tw, 4), dtype=np.float32)
    px[..., :3] = BACKDROP
    for i, t in enumerate(tiles):
        r, c = divmod(i, cols)
        y0 = (rows - 1 - r) * th
        px[y0:y0 + th, c * tw:(c + 1) * tw] = t
    img = bpy.data.images.new("sheet", cols * tw, rows * th, alpha=False)
    img.pixels = px.ravel()
    img.filepath_raw = out
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)
    print(f"[people] {out}: {len(files)} tiles")


def palette_for(pid, look=(0, 0, 0)):
    if pid in people.PLAYERS:
        return looks.player_palette(*look)
    return looks.VILLAGERS[pid]


# ------------------------------------------------------------------------------ the sheets
def sheet_models(cam, ids):
    files = []
    for pid in ids:
        p = people.build(pid)
        sh = Shown(p, palette_for(pid), hair=0)
        sh.pose(person_clips.by_name("idle"), 0.0)
        for view in ("front", "three_quarter"):
            frame(cam, (0, 0, 0.74), 1.7, view)
            files.append(render(os.path.join(TILES, f"model_{pid}_{view}.png")))
        sh.remove()
    sheet(files, 4, os.path.join(OUT, "models.png"))


def sheet_faces(cam, ids):
    """Close-ups: each face front and three-quarter (the eyes, the smile, the hairline)."""
    files = []
    for pid in ids:
        p = people.build(pid)
        sh = Shown(p, palette_for(pid), hair=0)
        sh.pose(None)
        z = p.head.c[2] - p.joints["hips"][0][2] + 0.0
        for view in ("front", "three_quarter"):
            frame(cam, (0, p.head.c[1], p.head.c[2] + 0.02 - (0 if pid in people.PLAYERS else 0)), 0.72, view)
            files.append(render(os.path.join(TILES, f"face_{pid}_{view}.png")))
        sh.remove()
    sheet(files, 4, os.path.join(OUT, "faces.png"))


def sheet_hair(cam, body):
    p = people.build(body)
    files = []
    looks_for = [(1, 0, 0), (1, 1, 1), (1, 3, 2), (0, 5, 1), (2, 2, 0), (3, 4, 2)]
    for view in ("front", "three_quarter", "back"):
        for k in range(6):
            sh = Shown(p, looks.player_palette(*looks_for[k]), hair=k)
            sh.pose(None)
            frame(cam, (0, 0, 0.93), 0.95, view)
            files.append(render(os.path.join(TILES, f"hair_{body}_{k}_{view}.png")))
            sh.remove()
    sheet(files, 6, os.path.join(OUT, f"hair_{body}.png"))


CLIP_PAGES = [  # (clip, view): the gaits from the side, gestures from the front or three-quarter
    [("idle", "three_quarter"), ("walk", "side"), ("run", "side"), ("wave", "front"), ("talk", "three_quarter"),
     ("nod", "side")],
    [("cheer", "front"), ("crouch_pet", "side"), ("sit", "side"), ("surprised", "three_quarter"),
     ("pick_up", "side"), ("look_around", "front")],
    # settings (workstream D)
    [("clap", "three_quarter"), ("sit_clap", "side"), ("sit_ground", "side"), ("doze", "side"),
     ("doze_stand", "side"), ("stretch", "front")],
    [("fist_pump", "three_quarter"), ("point", "side"), ("worried", "front"), ("slump", "side"), ("bow", "side"),
     ("fish", "side")],
    [("cast", "side"), ("scatter", "three_quarter"), ("write", "three_quarter"), ("tidy", "three_quarter"),
     ("fly_toy", "front"), ("clap", "front")],
]


def sheet_clips(cam, pid, pages=None, frames=5, hair=0):
    """Key frames of the main clips, evenly spaced (loops: one cycle), one row per clip."""
    p = people.build(pid)
    sh = Shown(p, palette_for(pid), hair=hair)
    for page, rows in enumerate(pages or CLIP_PAGES):
        files = []
        for name, view in rows:
            c = person_clips.by_name(name)
            n = c.frame_count()
            picks = [round(k * (n - 1) / max(1, frames - 1)) for k in range(frames)] if not c.loop else \
                [round(k * n / frames) for k in range(frames)]
            for k, f in enumerate(picks):
                sh.pose(c, f / FPS)
                frame(cam, (0, 0.0, 0.7), 1.65, view)
                files.append(render(os.path.join(TILES, f"clip_{pid}_{name}_{k}.png")))
        sheet(files, frames, os.path.join(OUT, f"clips_{pid}_{page + 1}.png"))
    sh.remove()


def stand_in_back():
    """A stand-in for a grown dragon's back at the seat (the Pouncer's: about 0.95 m across,
    its top 1.3 m up; the seat is at the top of it)."""
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=1.0, depth=1.4, location=(0, 0.25, 1.0),
                                        rotation=(math.radians(90), 0, 0))
    o = bpy.context.object
    o.scale = (0.3, 0.3, 1.0)
    o.data.materials.append(flat_material("dragon", (0.45, 0.66, 0.62)))
    for poly in o.data.polygons:
        poly.use_smooth = True
    return o, (0.0, 0.0, 1.3)


def sheet_ride(cam, pid="player_a", frames=5):
    p = people.build(pid)
    sh = Shown(p, palette_for(pid), hair=2)
    back, seat = stand_in_back()
    floor = bpy.data.objects["floor"]
    floor.hide_render = True
    files = []
    for name in ("mount", "ride", "ride_lean_left", "ride_lean_right", "dismount"):
        c = person_clips.by_name(name)
        n = c.frame_count()
        picks = [round(k * (n - 1) / max(1, frames - 1)) for k in range(frames)] if not c.loop else \
            [round(k * n / frames) for k in range(frames)]
        for k, f in enumerate(picks):
            t = f / FPS
            if name in ("mount", "dismount"):  # the lead lerps ground spot <-> seat; show the path
                u = t / c.length
                u = u if name == "mount" else 1 - u
                ground = (0.75, 0.0, 0.0)
                sh.pose(c, t, seat=tuple(ground[i] + (seat[i] - ground[i]) * min(1.0, max(0.0, (u - 0.15) / 0.6))
                                         for i in range(3)))
            else:
                sh.pose(c, t, seat=seat)
            view = {"ride": "chase", "ride_lean_left": "front", "ride_lean_right": "front"}.get(name, "three_quarter")
            if name in ("mount", "dismount"):
                frame(cam, (0.35, 0.0, 0.95), 2.3, view)
            else:
                frame(cam, (0.0, 0.0, 1.55), 1.35, view)
            files.append(render(os.path.join(TILES, f"ride_{pid}_{name}_{k}.png")))
    sh.remove()
    bpy.data.objects.remove(back, do_unlink=True)
    floor.hide_render = False
    sheet(files, frames, os.path.join(OUT, "ride.png"))


def sheet_looks(cam):
    """The creator's colours on body A: skin tones, hair colours, outfits."""
    p = people.build("player_a")
    files = []
    combos = [(s, 0, 0) for s in range(5)] + [(1, h, 0) for h in range(6)] + [(2, 1, o) for o in range(3)]
    for k, look in enumerate(combos):
        sh = Shown(p, looks.player_palette(*look), hair=k % 6)
        sh.pose(None)
        frame(cam, (0, 0, 0.72), 1.6, "three_quarter")
        files.append(render(os.path.join(TILES, f"looks_{k}.png")))
        sh.remove()
    sheet(files, 7, os.path.join(OUT, "looks.png"))


def portraits(cam, ids, out_dir):
    """64 x 64 transparent: head and shoulders, front three-quarter, smiling (the idle pose)."""
    scene = bpy.context.scene
    floor = bpy.data.objects["floor"]
    floor.hide_render = True
    scene.render.film_transparent = True
    scene.render.resolution_x = scene.render.resolution_y = 256
    os.makedirs(out_dir, exist_ok=True)
    files = []
    for pid in ids:
        p = people.build(pid)
        sh = Shown(p, palette_for(pid))
        sh.pose(None)
        # frame the head (hair, hat, beard: everything on the head bone) down to the chest; props
        # such as a staff or a lantern pole may run out of the picture
        head = [q for q, w in zip(p.body.pos, p.body.w) if w[0][0] == "head"]
        top = max(q[2] for q in head)
        width = max(q[0] for q in head) - min(q[0] for q in head)
        low = p.joints["chest"][0][2] + 0.02
        span = max(top - low, 0.8 * width) * 1.06
        frame(cam, (0.0, p.head.c[1], top - span / 2 + 0.01), span, "portrait", lens=85)
        big = render(os.path.join(TILES, f"portrait_{pid}_256.png"))
        small = os.path.join(out_dir, f"{pid}.png")
        downsample(big, small, 4)
        files.append(big)
        sh.remove()
    scene.render.film_transparent = False
    scene.render.resolution_x = scene.render.resolution_y = RES
    floor.hide_render = False
    return files


def downsample(src, dst, k):
    """Box-filter a transparent render down by k (premultiplied, so edges stay clean)."""
    img = bpy.data.images.load(src)
    w, h = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)
    bpy.data.images.remove(img)
    rgb, a = px[..., :3] * px[..., 3:4], px[..., 3:4]
    rgb = rgb.reshape(h // k, k, w // k, k, 3).mean(axis=(1, 3))
    a = a.reshape(h // k, k, w // k, k, 1).mean(axis=(1, 3))
    out = np.concatenate([np.where(a > 1e-4, rgb / np.maximum(a, 1e-4), 0.0), a], axis=2)
    small = bpy.data.images.new("small", w // k, h // k, alpha=True)
    small.pixels = out.ravel()
    small.filepath_raw = dst
    small.file_format = "PNG"
    small.save()
    bpy.data.images.remove(small)
    print(f"[people] portrait {dst}")


def main():
    os.makedirs(TILES, exist_ok=True)
    scene, cam = setup_scene()
    only = arg("--only")
    ids = only.split(",") if only else list(people.PLAYERS) + list(people.VILLAGER_IDS)
    ids = [i for i in ids if i in people.PEOPLE]
    sheets = arg("--sheets", "models,hair,clips,ride,looks,portraits").split(",")
    if "models" in sheets:
        sheet_models(cam, ids)
    if "faces" in sheets:
        sheet_faces(cam, ids)
    if "hair" in sheets:
        for body in [i for i in ids if i in people.PLAYERS]:
            sheet_hair(cam, body)
    if "clips" in sheets:
        pages = arg("--clip-pages")  # (1-based page numbers, e.g. 3,4,5: just those)
        chosen = [CLIP_PAGES[int(k) - 1] for k in pages.split(",")] if pages else None
        for pid in ids:
            sheet_clips(cam, pid, pages=chosen, hair=2)
    if "ride" in sheets:
        sheet_ride(cam)
    if "looks" in sheets:
        sheet_looks(cam)
    if "portraits" in sheets:
        vill = [i for i in ids if i in people.VILLAGER_IDS]
        files = portraits(cam, vill, arg("--portrait-dir", os.path.join(ROOT, "assets", "sprites", "people")))
        if files:
            sheet(files, len(files), os.path.join(OUT, "portraits.png"))


main()
