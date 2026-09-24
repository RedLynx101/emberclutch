"""The animated 3D HOME Menu banner (Alpha 2 WP10, D50) and its flat 2D fallback.

The textured baby Ember peeks out of its cracked, ember-lit egg (the cap sits on its head),
the Emberclutch wordmark behind it. Over a 4 second loop it tilts its head, wags its tail,
blinks twice and its heart pulses (scale, and a diffuse-colour animation).

The HOME Menu rules (docs/plan/alpha-2.md WP10): the model and its animation are named
COMMON (pycgfx does that), the CGFX stays under 512 KB, and every moving thing is a rigid
piece animated by node transforms, never skinning. So the posed dragon is frozen and cut
into pieces at its joints (body, head, eyes, tail, heart), each with its pivot at the joint.

  blender -b -P tools/blender/banner3d.py -- [--out build/banner] [--assets assets] [--review build/review] [--debug-rig]
    [--no-fit] [--keep-glow] [--keep-backdrop] [--anchor]   (banner-lab variants, tools/banner_lab.ps1)
  py -3.12 build/tools/pycgfx/main.py build/banner/banner.gltf build/banner/banner.cgfx

Writes <out>/banner.gltf (+ .bin, the skin texture), <assets>/banner.png (the 2D banner,
256x128) and review renders of the 3D banner through the HOME Menu's camera.
"""
import json
import math
import os
import struct
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import dragon_model as dm  # noqa: E402
import dragon_texture  # noqa: E402

ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.abspath(dm.arg("--out", os.path.join(ROOT, "build", "banner")))
ASSETS = os.path.abspath(dm.arg("--assets", os.path.join(ROOT, "assets")))
REVIEW = os.path.abspath(dm.arg("--review", os.path.join(ROOT, "build", "review")))
FONT = os.path.join(ROOT, "assets", "fonts", "cinzel-decorative", "CinzelDecorative-Bold.ttf")

FPS, FRAMES = 24, 96          # a 4 second loop
SKIN = 128                    # the skin texture (RGBA4 in the CGFX: 32 KB)
TALL = 18.0                   # the dragon and its egg, in banner units (the view is 40 x 24)
# The HOME Menu's banner camera (pycgfx banner-camera.gltf): glTF (0, 1, 44.786), looking
# down -Z, 30 degrees tall, 5:3. In Blender's Z-up axes it stands at -Y looking +Y.
CAM_AT, CAM_FOV = Vector((0.0, -44.786, 1.0)), math.radians(30.0)

HEAD_BONES = {"head", "snout", "jaw", "eyes"}
TAIL_BONES = {"tail1", "tail2", "tail3", "tail4"}

SHELL = (0.97, 0.90, 0.80)
GLOW = (1.0, 0.52, 0.18)
GOLD = (0.96, 0.72, 0.26)
PLUM = (0.16, 0.08, 0.19)
HEART_DIM, HEART_BRIGHT = (1.0, 0.45, 0.12, 1.0), (1.0, 0.86, 0.45, 1.0)


# ------------------------------------------------------------------------------ helpers
def link(obj):
    bpy.context.scene.collection.objects.link(obj)
    return obj


def principled(name, color, rough=0.6, image=None):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*color[:3], 1)
    bsdf.inputs["Roughness"].default_value = rough
    if "Metallic" in bsdf.inputs:
        bsdf.inputs["Metallic"].default_value = 0.0
    if image is not None:
        tex = m.node_tree.nodes.new("ShaderNodeTexImage")
        tex.image = image
        m.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    return m


def toon_colour(mat):
    """The flat colour of one of dragon_model's toon materials."""
    for n in mat.node_tree.nodes:
        if n.bl_idname == "ShaderNodeRGB":
            return tuple(n.outputs[0].default_value[:3])
    for n in mat.node_tree.nodes:
        if n.bl_idname == "ShaderNodeEmission":
            return tuple(n.inputs["Color"].default_value[:3])
    return (0.8, 0.8, 0.8)


def frozen(obj, dg):
    """The object as drawn now (posed, parts carried by their bones), as a mesh in world space."""
    ev = obj.evaluated_get(dg)
    me = bpy.data.meshes.new_from_object(ev, preserve_all_data_layers=True, depsgraph=dg)
    me.transform(ev.matrix_world)
    return me


def with_origin(name, me, pivot, mats):
    """A mesh object whose origin is `pivot` (world), vertices unchanged in the world."""
    me.transform(Matrix.Translation(-pivot))
    o = link(bpy.data.objects.new(name, me))
    o.location = pivot
    for i, m in enumerate(mats):  # in place: clearing the list would send every face to slot 0
        if i < len(o.data.materials):
            o.data.materials[i] = m
        else:
            o.data.materials.append(m)
    return o


def keep_faces(me, keep):
    """A copy of `me` with only the faces for which keep(face index) holds."""
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.faces.ensure_lookup_table()
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if not keep(f.index)], context="FACES")
    out = bpy.data.meshes.new(me.name + "_part")
    bm.to_mesh(out)
    bm.free()
    return out


# ------------------------------------------------------------------------------ the dragon
def bake_colour(d, size):
    """The skin as the game shows it (base and accent, pattern, scale detail), baked through
    the body's UVs into one colour texture: the CGFX has no palette shader."""
    b = d["breed"]
    body = d["body"]
    rgba = dragon_texture.bake_skin(body, d["form"], size)
    dragon_texture.preview_material(d["mats"]["body"], rgba, b["pattern"], b["pattern_color"], 0.0)
    image = bpy.data.images.new("banner_skin", size, size, alpha=False)
    for slot in body.material_slots:
        nt = slot.material.node_tree
        out = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeOutputMaterial")
        if slot.material is d["mats"]["body"]:  # colour x detail, before the toon light
            mul = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeMix" and n.blend_type == "MULTIPLY")
            src = mul.inputs["A"].links[0].from_socket
            em = nt.nodes.new("ShaderNodeEmission")
            nt.links.new(src, em.inputs["Color"])
            nt.links.new(em.outputs[0], out.inputs["Surface"])
        tex = nt.nodes.new("ShaderNodeTexImage")
        tex.image = image
        nt.nodes.active = tex
    scene = bpy.context.scene
    engine = scene.render.engine
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 4
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    body.select_set(True)
    bpy.context.view_layer.objects.active = body
    bpy.ops.object.bake(type="EMIT", margin=4)
    scene.render.engine = engine
    os.makedirs(OUT, exist_ok=True)
    image.filepath_raw = os.path.join(OUT, "banner_skin.png")
    image.file_format = "PNG"
    image.save()
    return image


def peek_pose(d):
    """Sitting in the egg, looking up at you: the sit pose, head raised, front paws forward
    onto the rim."""
    pb = d["arm"].pose.bones
    for name, (x, y, z) in {"neck1": (-12, 0, 0), "head": (-8, 0, 0), "arm_up_L": (-30, 0, 0), "arm_up_R": (-30, 0, 0),
                            "tail1": (48, 0, 0), "tail2": (22, 0, 0), "tail3": (10, 0, 0)}.items():
        if name in pb:
            pb[name].rotation_mode = "XYZ"
            pb[name].rotation_euler.x += math.radians(x)
            pb[name].rotation_euler.z += math.radians(z)
    bpy.context.view_layer.update()


def joint(d, bone):
    arm = d["arm"]
    return arm.matrix_world @ arm.pose.bones[bone].head


def dragon_pieces(d, skin):
    """Freeze the posed dragon and cut it into rigid pieces with their pivots at the joints."""
    dg = bpy.context.evaluated_depsgraph_get()
    body = d["body"]
    arm = d["arm"]
    mats = {}

    def mat_for(m):
        if m.name not in mats:
            mats[m.name] = principled("b_" + m.name, (1, 1, 1), 0.7, skin) if m is d["mats"]["body"] else \
                principled("b_" + m.name, toon_colour(m), 0.5)
        return mats[m.name]

    # The body mesh: each face goes with the bone most of its corners follow.
    me = frozen(body, dg)
    names = {g.index: g.name for g in body.vertex_groups}
    owner = []
    same = len(me.vertices) == len(body.data.vertices)
    segs = [(b.name, arm.matrix_world @ b.head, arm.matrix_world @ b.tail) for b in arm.pose.bones]
    for i, v in enumerate(me.vertices):
        name = None
        if same:
            ws = [(g.weight, names.get(g.group, "")) for g in body.data.vertices[i].groups]
            ws = [w for w in ws if w[1] in arm.pose.bones]
            if ws:
                name = max(ws)[1]
        if name is None:  # nearest bone segment
            best = 1e9
            for n, a, b in segs:
                ab = b - a
                t = max(0.0, min(1.0, (v.co - a).dot(ab) / max(1e-9, ab.dot(ab))))
                dist = (a + ab * t - v.co).length
                if dist < best:
                    best, name = dist, n
        owner.append("head" if name in HEAD_BONES else "tail" if name in TAIL_BONES else "body")
    face_owner = []
    for f in me.polygons:
        votes = {}
        for vi in f.vertices:
            votes[owner[vi]] = votes.get(owner[vi], 0) + 1
        face_owner.append(max(votes, key=votes.get))
    body_mats = [mat_for(s.material) for s in body.material_slots]
    pieces = {}
    pivots = {"body": joint(d, "hips"), "head": joint(d, "head"), "tail": joint(d, "tail1")}
    for piece in ("body", "head", "tail"):
        part = keep_faces(me, lambda fi, p=piece: face_owner[fi] == p)
        pieces[piece] = [with_origin(f"{piece}_skin", part, pivots[piece], body_mats)]
    # Wings and the parts: each goes with its bone's piece; the eyes and the heart move alone.
    for w in d["wings"]:
        pieces["body"].append(with_origin(w.name, frozen(w, dg), pivots["body"], [mat_for(s.material) for s in w.material_slots]))
    eye_meshes, heart_meshes = [], []
    for group, objs in d["groups"].items():
        for o in objs:
            bone = o.constraints[0].subtarget if o.constraints else "chest"
            ms = [mat_for(s.material) for s in o.material_slots]
            if group == "eyes":
                eye_meshes.append((frozen(o, dg), ms))
                continue
            if group == "heart":
                heart_meshes.append((frozen(o, dg), ms))
                continue
            piece = "head" if bone in HEAD_BONES else "tail" if bone in TAIL_BONES else "body"
            pieces[piece].append(with_origin(o.name, frozen(o, dg), pivots[piece], ms))
    eyes_at = joint(d, "eyes")
    pieces["eyes"] = [with_origin(f"eye{k}", m, eyes_at, ms) for k, (m, ms) in enumerate(eye_meshes)]
    heart_at = sum((sum((v.co for v in m.vertices), Vector()) / len(m.vertices) for m, _ in heart_meshes), Vector())
    heart_at /= max(1, len(heart_meshes))
    heart_mat = principled("heart_glow", HEART_DIM, 0.35)
    pieces["heart"] = [with_origin(f"heart{k}", m, heart_at, [heart_mat]) for k, (m, _) in enumerate(heart_meshes)]
    for hp in pieces["heart"]:  # a little bigger than in the den, to show at banner size
        hp.scale = (1.3, 1.3, 1.3)
    # One object per piece (the eyes and the heart keep their pivot).
    merged = {}
    for piece, objs in pieces.items():
        if not objs:
            continue
        for o in bpy.context.view_layer.objects:
            o.select_set(False)
        for o in objs:
            o.select_set(True)
        bpy.context.view_layer.objects.active = objs[0]
        bpy.ops.object.join()
        objs[0].name = piece
        merged[piece] = objs[0]
    # The original rig and meshes are done with.
    for o in [body, arm] + d["wings"] + [p for objs in d["groups"].values() for p in objs]:
        bpy.data.objects.remove(o, do_unlink=True)
    return merged, heart_mat


# ------------------------------------------------------------------------------ the egg
EGG_TEETH, EGG_JAG = 8, 0.1  # the crack's zigzag: teeth round the egg, and their height (of the egg's)
EGG_WALL = 0.035             # the shell's thickness (of the egg's height)


def egg_taper(zn):
    """The egg's width at zn (-1 its bottom .. 1 its top), as a share of its widest."""
    return 1.0 - 0.18 * max(0.0, zn)


def crack_height(a, height, cut):
    """How high the crack runs at angle a round the egg."""
    saw = abs(((a / (2 * math.pi) * EGG_TEETH) % 1.0) - 0.5) * 2  # 0..1 triangle wave
    return cut * height + (saw - 0.5) * EGG_JAG * height


def egg_halves(height, width, cut):
    """The egg split along a zigzag crack: the bottom cup (cream outside, glowing inside) and
    the cap. Both open at the crack."""
    shell = principled("shell", SHELL, 0.45)
    inner = principled("shell_glow", GLOW, 0.8)

    def half(name, top):
        bm = bmesh.new()
        bmesh.ops.create_uvsphere(bm, u_segments=24, v_segments=16, radius=1.0)
        for v in bm.verts:
            z = v.co.z
            v.co.x *= width * egg_taper(z)
            v.co.y *= width * egg_taper(z)
            v.co.z = (z + 1.0) * height * 0.5  # the bottom on the floor
        def crack(v):
            return crack_height(math.atan2(v.co.y, v.co.x), height, cut)
        bmesh.ops.delete(bm, geom=[v for v in bm.verts if (v.co.z > crack(v)) != top], context="VERTS")
        for v in bm.verts:  # the open edge follows the crack exactly
            if v.is_boundary:
                v.co.z = crack(v)
        me = bpy.data.meshes.new(name)
        bm.to_mesh(me)
        bm.free()
        o = link(bpy.data.objects.new(name, me))
        for p in o.data.polygons:
            p.use_smooth = True
        o.data.materials.append(shell)
        o.data.materials.append(inner)
        sol = o.modifiers.new("thick", "SOLIDIFY")
        sol.thickness = EGG_WALL * height
        sol.material_offset = 1  # the inside of the shell glows
        sol.use_rim = True
        return o

    return half("egg", False), half("cap", True)


def fit_in_egg(pieces, height, width, cut, floor_z, wag=24.0):
    """How far to move the dragon along Y (forward, toward the camera, is -Y) so nothing of it
    below the crack reaches through the shell (the tail through its whole wag). Noah saw its
    rump poke out of the back of the egg on the 3DS (2026-09-24): measured, not eyeballed."""
    bpy.context.view_layer.update()
    pts = []
    for name, o in pieces.items():
        if name in ("heart", "eyes"):
            continue
        for yaw in ((-wag, 0.0, wag) if name == "tail" else (0.0,)):
            rot = o.rotation_euler.copy()
            rot.z += math.radians(yaw)
            m = Matrix.LocRotScale(o.location, rot, o.scale)
            pts += [m @ v.co for v in o.data.vertices]
    top = floor_z + (cut + EGG_JAG) * height
    pts = [p for p in pts if p.z < top]

    def worst(dy):
        """How far the worst point reaches past the shell's inside wall (negative: inside)."""
        w = -1e9
        for p in pts:
            x, y, z = p.x, p.y + dy, p.z - floor_z
            if z > crack_height(math.atan2(y, x), height, cut):
                continue
            zn = min(1.0, max(-1.0, z / (0.5 * height) - 1.0))
            inside = width * egg_taper(zn) * math.sqrt(1.0 - zn * zn) - EGG_WALL * height
            w = max(w, math.hypot(x, y) - inside)
        return w

    steps = [-0.005 * height * k for k in range(61)]  # up to 0.3 of the egg's height forward
    fits = [dy for dy in steps if worst(dy) <= -0.01 * height]
    dy = fits[0] if fits else min(steps, key=worst)
    print(f"[banner] fit in the egg: worst {worst(0.0) / height:+.3f} where it was, "
          f"{worst(dy) / height:+.3f} moved {-dy / height:.3f} forward (of the egg's height)")
    return dy


def apply_modifiers(o):
    for o2 in bpy.context.view_layer.objects:
        o2.select_set(False)
    o.select_set(True)
    bpy.context.view_layer.objects.active = o
    for m in list(o.modifiers):
        bpy.ops.object.modifier_apply(modifier=m.name)


def wordmark_texture(path, width=256, height=64):
    """The wordmark rendered on its own (gold letters, a dark edge, clear around them): the
    banner shows it on a single quad, far lighter than the letters as geometry."""
    main = bpy.context.window.scene if bpy.context.window else bpy.context.scene
    scene = bpy.data.scenes.new("wordmark")
    try:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE"
    scene.render.film_transparent = True
    scene.view_settings.view_transform = "Standard"
    scene.render.resolution_x, scene.render.resolution_y = width, height
    curve = bpy.data.curves.new("wordmark_text", "FONT")
    curve.body = "Emberclutch"
    if os.path.exists(FONT):
        curve.font = bpy.data.fonts.load(FONT)
    curve.align_x, curve.align_y = "CENTER", "CENTER"
    curve.size = 1.0
    curve.offset = 0.035  # a dark edge (the letters grown a little, behind the gold)
    text = bpy.data.objects.new("wordmark_text", curve)
    scene.collection.objects.link(text)
    edge = bpy.data.materials.new("edge")
    edge.use_nodes = True
    edge.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.14, 0.05, 0.10, 1)
    curve.materials.append(edge)
    gold_curve = curve.copy()
    gold_curve.offset = 0.0
    gold = bpy.data.materials.new("gold_emit")
    gold.use_nodes = True
    nt = gold.node_tree
    nt.nodes.clear()
    em = nt.nodes.new("ShaderNodeEmission")
    em.inputs["Color"].default_value = (*GOLD, 1)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(em.outputs[0], out.inputs["Surface"])
    gold_curve.materials.clear()
    gold_curve.materials.append(gold)
    top = bpy.data.objects.new("wordmark_gold", gold_curve)
    top.location.z = 0.01
    scene.collection.objects.link(top)
    ecur = edge.node_tree.nodes["Principled BSDF"]
    ecur.inputs["Roughness"].default_value = 1.0
    cam = bpy.data.objects.new("wordmark_cam", bpy.data.cameras.new("wordmark_cam"))
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = 7.6
    cam.location = (0, 0, 5)
    scene.collection.objects.link(cam)
    scene.camera = cam
    world = bpy.data.worlds.new("wordmark_world")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 1.0
    scene.world = world
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True, scene=scene.name)
    for o in (text, top, cam):  # none of it may reach the banner's glTF (it did, in WP10)
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.data.scenes.remove(scene, do_unlink=True)
    img = bpy.data.images.load(path)
    return img


def wordmark(width):
    img = wordmark_texture(os.path.join(OUT, "wordmark.png"))
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    h = width / 4.0
    vs = [bm.verts.new(p) for p in ((-width / 2, 0, -h / 2), (width / 2, 0, -h / 2), (width / 2, 0, h / 2), (-width / 2, 0, h / 2))]
    f = bm.faces.new(vs)
    for loop, (u, v) in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
        loop[uv].uv = (u, v)
    me = bpy.data.meshes.new("wordmark")
    bm.to_mesh(me)
    bm.free()
    o = link(bpy.data.objects.new("wordmark", me))
    # Unlit and alpha-tested in the CGFX (make_unlit, tools/banner_cgfx.py): on the 3DS the lit,
    # blended quad came out dark from the front and gold only from behind, because pycgfx
    # draws a two-sided material as two copies, the second with its normals turned away, and
    # a blended one writes no depth, so that dark copy covered the gold one.
    m = bpy.data.materials.new("wordmark")
    m.use_nodes = True
    nt = m.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = 1.0
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = img
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    nt.links.new(tex.outputs["Alpha"], bsdf.inputs["Alpha"])
    if hasattr(m, "surface_render_method"):
        m.surface_render_method = "BLENDED"
    else:
        m.blend_method = "BLEND"
    o.data.materials.append(m)
    return o


def glow_disc(radius, colour, at):
    """A warm disc behind the dragon (the den's hearth glow, flat: the CGFX is lit)."""
    bm = bmesh.new()
    bmesh.ops.create_circle(bm, cap_ends=True, radius=radius, segments=32)
    me = bpy.data.meshes.new("glow")
    bm.to_mesh(me)
    bm.free()
    o = link(bpy.data.objects.new("glow", me))
    o.rotation_euler = (math.radians(90), 0, 0)
    o.location = at
    o.data.materials.append(principled("hearth_glow", colour, 1.0))
    return o


def anchor():
    """A small still triangle, first of the scene's objects (glTF lists them by name): the
    skeleton's first bone is then a static one, as 0.1.1's backdrop was. Hidden in the egg."""
    bm = bmesh.new()
    vs = [bm.verts.new(p) for p in ((-0.2, 0.0, 0.0), (0.2, 0.0, 0.0), (0.0, 0.0, 0.3))]
    bm.faces.new(vs)
    me = bpy.data.meshes.new("aaa_anchor")
    bm.to_mesh(me)
    bm.free()
    o = link(bpy.data.objects.new("aaa_anchor", me))
    o.location = (-3.0, 0.0, -10.0)  # inside the egg's cup
    o.data.materials.append(principled("anchor", SHELL, 0.9))
    return o


def backdrop(width, height, y):
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=1, y_segments=1, size=0.5)
    me = bpy.data.meshes.new("backdrop")
    bm.to_mesh(me)
    bm.free()
    o = link(bpy.data.objects.new("backdrop", me))
    o.scale = (width, height, 1)
    o.rotation_euler = (math.radians(90), 0, 0)
    o.location = (0, y, 1.0)
    o.data.materials.append(principled("dusk", PLUM, 0.9))
    return o


# ------------------------------------------------------------------------------ assembly
def build():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene, _ = dm.setup_scene()
    bpy.data.objects.remove(bpy.data.objects["floor"], do_unlink=True)
    scene.render.fps = FPS
    scene.frame_start, scene.frame_end = 0, FRAMES
    form, t = dm.STAGE["hatchling"]
    dm.set_lod(1)  # the den's background detail: plenty at banner size, and the CGFX stays small
    d = dm.build_dragon("ember", form)
    for e in d["groups"]["eyes"]:  # ...but the eyes at full detail: the light ones lose their pupils
        bpy.data.objects.remove(e, do_unlink=True)
    d["groups"]["eyes"].clear()
    d["snap"]["eyes"].clear()
    dm.set_lod(0)
    for e in dm.build_eyes(d["mats"]):
        dm.attach(d, "eyes", e, "eyes")
    dm.set_lod(1)
    dm.pose_stage(d, t, d["breed"]["build"], sit=True)
    peek_pose(d)
    skin = bake_colour(d, SKIN)
    if "--debug-rig" in sys.argv:  # the posed rig as built, before it's frozen into pieces
        cam = banner_camera()
        cam.location = Vector((0, -6.5, 1.1))
        cam.data.angle_y = math.radians(25)
        cam.data.clip_start = 0.1
        try:
            scene.render.engine = "BLENDER_EEVEE_NEXT"
        except TypeError:
            scene.render.engine = "BLENDER_EEVEE"
        render(os.path.join(REVIEW, "banner_rig.png"), 600, 600, 0)
        sys.exit(0)
    head_top = max((dm.visible_points([d]) or [Vector()]), key=lambda p: p.z)
    pieces, heart_mat = dragon_pieces(d, skin)

    # Scale: the dragon's height sets the egg's; then everything to banner units. The rim of
    # the cracked egg runs just under the heart, so it shows (and beats) above the shell.
    lo = min(v.co.z + pieces["body"].location.z for v in pieces["body"].data.vertices)
    hi = head_top.z
    h = hi - lo
    if "tail" in pieces:  # swung out to its left, round beside the body: its wag shows
        pieces["tail"].rotation_euler.z = math.radians(62)
    if "heart" in pieces:  # on the front of the chest, just above the shell's rim (the sit pose lifts it to the face)
        body = pieces["body"]
        target = lo + 0.54 * h
        near = [v.co + body.location for v in body.data.vertices
                if abs(v.co.x + body.location.x) < 0.08 * h and abs(v.co.z + body.location.z - target) < 0.05 * h]
        if near:
            pieces["heart"].location = (0.0, min(p.y for p in near) - 0.015 * h, target)
    heart_z = pieces["heart"].location.z if "heart" in pieces else lo + 0.5 * h
    rim = 0.42 * h + 0.1 * h  # up to its belly (above the egg's bottom, 0.1 h below its feet)
    egg_h = 0.95 * h
    print(f"[banner] dragon {h:.2f} tall; heart at {(heart_z - lo) / h:.2f}; rim at {rim / egg_h:.2f} of the egg")
    egg, cap = egg_halves(egg_h, 0.42 * h, rim / egg_h)
    for o in (egg, cap):
        o.location.z = lo - 0.1 * h
        apply_modifiers(o)
    # The cap sits on its head, tipped back a little.
    cap_bottom = min(v.co.z for v in cap.data.vertices)
    cap.data.transform(Matrix.Translation((0, 0, -cap_bottom)))
    cap.location = (head_top.x, head_top.y + 0.08 * h, head_top.z - 0.12 * h)
    cap.rotation_euler = (math.radians(-16), math.radians(10), 0)
    cap.scale = (0.5, 0.5, 0.5)
    dy = fit_in_egg(pieces, egg_h, 0.42 * h, rim / egg_h, lo - 0.1 * h) if "--no-fit" not in sys.argv else 0.0
    for o in list(pieces.values()) + [cap]:
        o.location.y += dy

    s = TALL / (hi - lo + 0.2 * h)
    world_objs = list(pieces.values()) + [egg, cap]
    for o in world_objs:
        o.location = (o.location - Vector((0, 0, lo))) * s
        o.data.transform(Matrix.Scale(s, 4))
    # In the frame: the pair just left of centre and low; the wordmark across the top, behind.
    # Nothing else: the HOME Menu's own background shows round them (Noah took the wall out;
    # the flat 2D banner keeps its backdrop, see main()).
    shift = Vector((-3.0, 0.0, -10.8))
    for o in world_objs:
        o.location += shift
    word = wordmark(30.0)
    word.location = (2.0, 5.0, 8.4)
    # Banner-lab variants (run 3: the new scene froze the HOME Menu, 0.1.1's didn't).
    if "--keep-glow" in sys.argv:
        glow_disc(9.8, (0.55, 0.22, 0.12), (-3.0, 16.0, -1.0))
    if "--keep-backdrop" in sys.argv:
        backdrop(90, 50, 20.0)
    if "--anchor" in sys.argv:
        anchor()
    return pieces, egg, cap, heart_mat, word


def parent(child, par):
    m = child.matrix_world.copy()
    child.parent = par
    child.matrix_world = m


def animate(pieces, cap, heart_mat):
    """Rigid node animation only (the HOME Menu freezes on skinned banners)."""
    body, head, tail = pieces["body"], pieces["head"], pieces["tail"]
    for p in ("head", "tail", "heart"):
        if p in pieces:
            parent(pieces[p], body)
    if "eyes" in pieces:
        parent(pieces["eyes"], head)
    parent(cap, head)
    bpy.context.view_layer.update()

    bpy.context.preferences.edit.keyframe_new_interpolation_type = "LINEAR"

    def key(o, frame, rot=None, scale=None, loc=None):
        if rot is not None:
            o.rotation_euler = [math.radians(a) for a in rot]
            o.keyframe_insert("rotation_euler", frame=frame)
        if scale is not None:
            o.scale = scale
            o.keyframe_insert("scale", frame=frame)
        if loc is not None:
            o.location = loc
            o.keyframe_insert("location", frame=frame)

    # The head tilts one way, then nods and tilts the other (roll is about the facing axis, Y).
    h0 = tuple(math.degrees(a) for a in head.rotation_euler)
    for f, (x, y, z) in ((0, (0, 0, 0)), (18, (0, 12, 0)), (36, (0, 0, 0)), (54, (-6, -10, 0)), (72, (0, 0, 0)),
                         (96, (0, 0, 0))):
        key(head, f, rot=(h0[0] + x, h0[1] + y, h0[2] + z))
    # The tail wags, then rests.
    t0 = tuple(math.degrees(a) for a in tail.rotation_euler)
    for f, yaw in ((0, 0), (8, 24), (16, -24), (24, 24), (32, -24), (40, 24), (48, 0), (96, 0)):
        key(tail, f, rot=(t0[0], t0[1], t0[2] + yaw))
    # Two blinks: the eyes squash flat and open again.
    if "eyes" in pieces:
        e = pieces["eyes"]
        for f, sz in ((0, 1), (36, 1), (38, 0.08), (41, 1), (76, 1), (78, 0.08), (81, 1), (96, 1)):
            key(e, f, scale=(1, 1, sz))
    # The heart beats: a quick swell each second.
    if "heart" in pieces:
        hp = pieces["heart"]
        for beat in range(4):
            f = beat * FPS
            key(hp, f, scale=(1, 1, 1))
            key(hp, f + 4, scale=(1.22, 1.22, 1.22))
            key(hp, f + 12, scale=(1, 1, 1))
        key(hp, FRAMES, scale=(1, 1, 1))
    # A gentle bob.
    b0 = body.location.copy()
    for f, dz in ((0, 0.0), (48, 0.35), (96, 0.0)):
        key(body, f, loc=b0 + Vector((0, 0, dz)))
    for o in bpy.data.objects:  # straight lines between keys (pycgfx: no spline rotations)
        ad = o.animation_data
        if ad and ad.action:
            for fc in getattr(ad.action, "fcurves", []):
                for kp in fc.keyframe_points:
                    kp.interpolation = "LINEAR"


# ------------------------------------------------------------------------------ export
def export(path):
    op = bpy.ops.export_scene.gltf
    props = set(op.get_rna_type().properties.keys())
    want = dict(filepath=path, export_format="GLTF_SEPARATE", export_animations=True, export_animation_mode="SCENE",
                export_force_sampling=False, export_frame_range=True, export_yup=True, export_apply=True,
                export_texcoords=True, export_normals=True, export_materials="EXPORT", export_cameras=False,
                export_lights=False, export_skins=False, export_morph=False, export_vertex_color="NONE",
                export_colors=False, export_optimize_animation_size=False, export_image_format="AUTO",
                use_active_scene=True)
    op(**{k: v for k, v in want.items() if k in props})


def add_heart_colour(path, heart_name="heart_glow"):
    """The heart's glow as a diffuse-colour animation (KHR_animation_pointer on its base
    colour; pycgfx turns it into a CGFX material animation), in step with the beat."""
    with open(path, encoding="utf-8") as f:
        g = json.load(f)
    mi = next((i for i, m in enumerate(g.get("materials", [])) if m.get("name") == heart_name), None)
    if mi is None or not g.get("animations"):
        print("[banner] no heart material or animation: colour pulse skipped")
        return
    times, colours = [], []
    for beat in range(4):
        for f, c in ((0, HEART_DIM), (4, HEART_BRIGHT), (12, HEART_DIM)):
            times.append((beat * FPS + f) / FPS)
            colours.append(c)
    times.append(FRAMES / FPS)
    colours.append(HEART_DIM)
    blob = struct.pack(f"<{len(times)}f", *times) + b"".join(struct.pack("<4f", *c) for c in colours)
    bin_name = "heart_colour.bin"
    with open(os.path.join(os.path.dirname(path), bin_name), "wb") as f:
        f.write(blob)
    g["buffers"].append({"uri": bin_name, "byteLength": len(blob)})
    b = len(g["buffers"]) - 1
    g["bufferViews"] += [{"buffer": b, "byteOffset": 0, "byteLength": 4 * len(times)},
                         {"buffer": b, "byteOffset": 4 * len(times), "byteLength": 16 * len(colours)}]
    v = len(g["bufferViews"]) - 2
    g["accessors"] += [{"bufferView": v, "componentType": 5126, "count": len(times), "type": "SCALAR",
                        "min": [min(times)], "max": [max(times)]},
                       {"bufferView": v + 1, "componentType": 5126, "count": len(colours), "type": "VEC4"}]
    a = len(g["accessors"]) - 2
    anim = g["animations"][0]
    anim["samplers"].append({"input": a, "output": a + 1, "interpolation": "LINEAR"})
    anim["channels"].append({"sampler": len(anim["samplers"]) - 1,
                             "target": {"path": "pointer", "extensions": {"KHR_animation_pointer": {
                                 "pointer": f"/materials/{mi}/pbrMetallicRoughness/baseColorFactor"}}}})
    used = g.setdefault("extensionsUsed", [])
    if "KHR_animation_pointer" not in used:
        used.append("KHR_animation_pointer")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(g, f, indent=1)
    print(f"[banner] heart colour pulse on material {mi}")


def make_unlit(path, names=("wordmark",)):
    """Marks materials unlit (KHR_materials_unlit, which tools/banner_cgfx.py honours: their
    texture's own colours, however they face the light) and alpha-tested rather than blended,
    so they write depth and the nearer copy of a two-sided quad wins. Seen from the back the
    wordmark reads mirrored, like a sign in a window, but gold."""
    with open(path, encoding="utf-8") as f:
        g = json.load(f)
    done = []
    for m in g.get("materials", []):
        if m.get("name") in names:
            m["alphaMode"], m["alphaCutoff"] = "MASK", 0.5
            m.setdefault("extensions", {})["KHR_materials_unlit"] = {}
            done.append(m["name"])
    if done:
        used = g.setdefault("extensionsUsed", [])
        if "KHR_materials_unlit" not in used:
            used.append("KHR_materials_unlit")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(g, f, indent=1)
    print(f"[banner] unlit, alpha-tested: {', '.join(done) or 'nothing'}")


# ------------------------------------------------------------------------------ renders
def banner_camera():
    cam = bpy.context.scene.camera
    cam.location = CAM_AT
    cam.rotation_euler = (math.radians(90), 0, 0)
    cam.data.type = "PERSP"
    cam.data.sensor_fit = "VERTICAL"
    cam.data.angle_y = CAM_FOV
    cam.data.clip_start, cam.data.clip_end = 20.0, 200.0
    return cam


def render(path, width, height, frame, final=None):
    scene = bpy.context.scene
    scene.frame_set(frame)
    scene.render.resolution_x, scene.render.resolution_y = width, height
    scene.render.filepath = path
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.render.render(write_still=True)
    if final:
        img = bpy.data.images.load(path)
        img.scale(*final)
        img.save()
    print(f"[banner] wrote {path}")


def main():
    pieces, egg, cap, heart_mat, word = build()
    animate(pieces, cap, heart_mat)
    tris = 0
    for o in bpy.context.scene.objects:
        if o.type == "MESH":
            tris += sum(len(p.vertices) - 2 for p in o.data.polygons)
    print(f"[banner] {len(pieces)} dragon pieces, {tris} triangles in all")
    os.makedirs(OUT, exist_ok=True)
    gltf = os.path.join(OUT, "banner.gltf")
    export(gltf)
    add_heart_colour(gltf)
    make_unlit(gltf)
    banner_camera()
    scene = bpy.context.scene
    for o in scene.objects:
        if o.type == "LIGHT":
            o.data.use_shadow = False
    try:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE"
    for k, f in enumerate((0, 18, 38, 54)):  # the 3D banner through the HOME Menu's camera
        render(os.path.join(REVIEW, f"banner3d_{k}.png"), 400, 240, f)
    render(os.path.join(REVIEW, "banner3d_big.png"), 1000, 600, 0)
    # The HOME Menu turns the banner round as it swaps titles: the back and the side must hold up too.
    cam = scene.camera
    for name, yaw in (("banner3d_back.png", 180.0), ("banner3d_side.png", 90.0)):
        a = math.radians(yaw)
        cam.location = Vector((CAM_AT.y * -math.sin(a), CAM_AT.y * math.cos(a), CAM_AT.z))
        cam.rotation_euler = (math.radians(90), 0, a)
        render(os.path.join(REVIEW, name), 500, 300, 0)
    banner_camera()
    if "--keep-glow" not in sys.argv:  # the 2D banner is a flat picture: it keeps its hearth glow
        glow_disc(9.8, (0.55, 0.22, 0.12), (-3.0, 16.0, -1.0))
    if "--keep-backdrop" not in sys.argv:  # ...and its dusk wall
        backdrop(90, 50, 20.0)
    cam = scene.camera  # the 2D banner: the same scene, a little closer (its frame is wider, 2:1)
    cam.data.angle_y = math.radians(24.5)
    cam.location.z -= 0.4
    render(os.path.join(ASSETS, "banner.png"), 1024, 512, 0, final=(256, 128))


main()
