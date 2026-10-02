"""The animated 3D HOME Menu banner (Alpha 2 WP10, D50) and its flat 2D fallback.

The face of the game (D80): the Blazeplume's hatchling (tools/dragons/kinds/blazeplume.py, the
dragon kit's model) peeks out of its cracked, ember-lit egg (the cap sits on its head),
the Emberclutch: Skyreach Valley wordmark above it, gold sparkles twinkling round them, over the HOME Menu's
own background, all centred. Twice in a 10 second loop (one turn of the HOME Menu's camera,
below) it tilts its head, wags its tail, blinks twice and its heart pulses (scale, and a
diffuse-colour animation); the sparkles glint in turn.

The HOME Menu rules (docs/plan/alpha-2.md WP10): the model and its animation are named
COMMON (pycgfx does that), the CGFX stays under 512 KB, and every moving thing is a rigid
piece animated by node transforms, never skinning. So the posed dragon is frozen and cut
into pieces at its joints (body, head, eyes, tail, heart), each with its pivot at the joint.
Each moving piece also carries a collar of the body round its joint, so the joint never opens
as it turns (0.1.x's baby Ember showed breaks in its skin at the neck and tail; D80).

  blender -b -P tools/blender/banner3d.py -- [--kind blazeplume|classic] [--out build/banner] [--assets assets] [--review build/review] [--debug-rig]
    [--no-fit] [--keep-glow] [--keep-backdrop] [--no-anchor] [--no-sparkles]   (banner-lab variants, tools/banner_lab.ps1)
    [--turn | --still] [--short-loop] [--spin-egg]   (holding the banner still: see below)
  py -3.12 tools/banner_cgfx.py build/banner/banner.gltf build/banner/banner.cgfx [--turn "body*:1,egg:1" | --billboard world]

The HOME Menu turns every 3D banner round, once every 10 s, clockwise seen from above (faster
while you blow on the mic): measured on pycgfx's own HOME Menu recording (10.03 s a turn), and
a Nintendo-SDK artist on polycount (2012) held a banner still with "a reverse-360 spin over 600
frames" (10 s at the CGFX's 60 frames a second). Two ways to hold ours still:
  --turn   the body (its pivot moved onto the turning axis; head, tail and heart under it) and
           the egg (the sparkles and the wordmark under it) each get a placeholder turn, which
           tools/banner_cgfx.py --turn "body*:1,egg:1" makes one whole turn the other way over
           the loop, so the dragon keeps all its motion. (Lab 7 hung everything on the egg,
           four levels deep, and froze, as U did: P, three deep, never froze. Lab 8, run 12.)
  --still  the dragon, its egg, the wordmark and the sparkles joined into one piece, "world",
           made a Y-axis billboard (--billboard world): nothing moves but the heart's glow
           (lab 6's T: no freeze, no turning, run 10).
Run 10 also found what freezes the HOME Menu: billboards that are animated (S: the sparkles
glinting as billboards) or have animated children (U: the whole scene under one).
Lab 8's checks: --short-loop keeps the old 4 s loop (96 frames); --spin-egg turns only the
egg (flat, three deep, 4 s), to tell a whole-turn rotation from the hierarchy if X freezes.

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
from mathutils import Euler, Matrix, Quaternion, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import dragon_model as dm  # noqa: E402
import dragon_texture  # noqa: E402

ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.abspath(dm.arg("--out", os.path.join(ROOT, "build", "banner")))
ASSETS = os.path.abspath(dm.arg("--assets", os.path.join(ROOT, "assets")))
REVIEW = os.path.abspath(dm.arg("--review", os.path.join(ROOT, "build", "review")))
FONT = os.path.join(ROOT, "assets", "fonts", "cinzel-decorative", "CinzelDecorative-Bold.ttf")
KIND = dm.arg("--kind", "classic")  # "classic": the old baby Ember (X, the banner that holds on the 3DS); or a kit kind (blazeplume: froze the HOME Menu in run 15, D81)
# Run 25 (Noah: "a more highly detailed banner, cleaner, and use one of our baby dragons still in
# the game"): a kit kind's hatchling at full detail (its den model's LOD 0), smooth-shaded, its skin
# at 256, its solid parts drawn one-sided (pycgfx doubles every two-sided material's geometry), and
# an ink outline round it (an inverted hull, unlit: the storybook's line) if --outline is given.
VARIANT = int(dm.arg("--variant", "0"))                          # a kit kind's colouring
DETAIL = int(dm.arg("--lod", "1" if KIND == "classic" else "0"))  # the body's level of detail
OUTLINE = float(dm.arg("--outline", "0"))                         # the ink's width, of the dragon's height (0: none)
INK = (0.13, 0.06, 0.15)                                          # the outline: the game's deep plum
COLLAR = {"head": 0.11, "tail": 0.09} if KIND == "classic" else {"head": 0.15, "tail": 0.13}  # (a kind's wider: run 25)
WAG = 24.0 if KIND == "classic" else 16.0  # the tail's wag either way, degrees (a kind's gentler: run 25)
TAIL_LIFT = (55.0, 35.0)  # a kind's tail: raised (about X) and swung to its left (about Z), degrees  # each moving piece's collar round its joint (of the dragon's height)
# A kind's parts coloured per vertex (a feather's bands, "vc" materials): each band takes its
# palette slot's colour in the colouring, as the exporter paints it in the game.
SLOT_KEY = {"body_plain": "base", "accent_flat": "accent", "pattern_flat": "pattern", "membrane": "membrane",
            "horn": "horn", "glow_flat": "glow", "rune": "pattern", "iris": "iris", "pupil": "pupil",
            "glint": "glint", "tongue": "tongue"}

FPS, FRAMES = 24, 240         # a 10 second loop: one turn of the HOME Menu's camera
CYCLE = 120                   # the dragon's motions, twice a loop (written on a 96-frame count)
if "--short-loop" in sys.argv:
    FRAMES = CYCLE = 96       # the old 4 second loop (a lab check)
SKIN = int(dm.arg("--skin", "128" if KIND == "classic" else "256"))  # the skin texture (RGBA4 in the CGFX: 32 KB at 128, 128 KB at 256)
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
if KIND != "classic":  # an ember-orange heart vanishes on the Blazeplume's flame-orange chest
    HEART_DIM, HEART_BRIGHT = (1.0, 0.80, 0.46, 1.0), (1.0, 0.98, 0.84, 1.0)
SPARKLE = (1.0, 0.86, 0.45)   # the heart's bright gold
# The sparkles, in banner units (x across, y depth, + away from you; z up): where, how big,
# and the frame their glint peaks. Round the egg and the dragon, clear of its face and of the
# wordmark; about two or three glinting at any moment, the rest faint specks.
SPARKLES = [
    ((-10.5, -2.0, -8.0), 1.0, 12), ((11.5, 0.0, -3.0), 1.0, 19), ((-7.0, -3.0, 3.5), 0.7, 26),
    ((17.0, 2.0, -7.0), 0.7, 33), ((-11.5, 1.0, -1.0), 1.2, 40), ((7.0, -2.0, 2.0), 0.8, 47),
    ((4.5, -4.0, -11.0), 0.6, 54), ((14.5, -1.0, 2.5), 0.9, 61), ((9.5, -2.0, -9.5), 0.8, 68),
    ((-16.0, 0.0, 1.5), 0.7, 75),
]
SPARKLE_REST = 0.2            # a faint speck between glints (never 0: a zero scale can't invert)
# The heart's beat on the 96-frame count: a quick swell each second (its scale; the glow follows).
HEART_BEATS = tuple(p for beat in range(4) for p in ((beat * 24, 1.0), (beat * 24 + 4, 1.22), (beat * 24 + 12, 1.0))) + ((96, 1.0),)


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
    return bake_emission(body, d["mats"]["body"], size)


def bake_emission(body, body_mat, size):
    """The body's previewed colour (before the toon light) baked through its UVs."""
    image = bpy.data.images.new("banner_skin", size, size, alpha=False)
    for slot in body.material_slots:
        nt = slot.material.node_tree
        out = next(n for n in nt.nodes if n.bl_idname == "ShaderNodeOutputMaterial")
        if slot.material is body_mat:  # colour x detail, before the toon light
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


def build_kind(name):
    """A kind's hatchling from the dragon kit (the den's lower detail, the eyes at full),
    textured, sitting as its own sit clip has it, peeking up out of the egg."""
    from dragonkit import model as km
    km.use_kind(name)
    form, t = km.STAGE["hatchling"]
    km.set_lod(DETAIL)
    d = km.build_dragon(form, VARIANT)
    for e in d["groups"]["eyes"]:
        bpy.data.objects.remove(e, do_unlink=True)
    d["groups"]["eyes"].clear()
    if "eyes" in d["snap"]:
        d["snap"]["eyes"].clear()
    km.set_lod(0)
    for e in km.build_eyes(d["mats"], "round"):
        km.attach(d, "eyes", e, "eyes")
    km.set_lod(DETAIL)
    km.textured(d)
    km.pose_stage(d, t)
    arm = d["arm"]
    idle = {pb.name: pb.rotation_euler.to_quaternion() for pb in arm.pose.bones}
    rest = {b.name: b.matrix_local.to_quaternion() for b in arm.data.bones}
    clips = {c.name: c for c in km.PLAN.clips()}
    clip = clips.get("sit_loop_h") or clips["sit_loop"]
    for pb in arm.pose.bones:
        q = Quaternion(clip.sample_q(pb.name, 0.0))
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = idle[pb.name] @ (rest[pb.name].conjugated() @ q @ rest[pb.name])
    for name, x in {"neck1": -8, "head": -6, "arm_up_L": -24, "arm_up_R": -24}.items():  # looking up, paws on the rim
        if name in arm.pose.bones:
            pb = arm.pose.bones[name]
            pb.rotation_quaternion = pb.rotation_quaternion @ Euler((math.radians(x), 0, 0)).to_quaternion()
    bpy.context.view_layer.update()
    d["visible_points"] = km.visible_points
    d["palette"] = km.variant_colors(VARIANT)
    return d


def add_outline(o, width, mat, outer_only=False):
    """An ink line round a piece: its faces again, welded, pushed out along their normals by
    `width` and turned inside out, in the unlit ink. Drawn one-sided, only its far side shows,
    just past the piece's silhouette (an inverted hull: no shader needed). `outer_only`: an open
    shell (the egg) keeps the faces whose normals point away from its middle."""
    bm = bmesh.new()
    bm.from_mesh(o.data)
    bm.faces.ensure_lookup_table()
    centre = sum((v.co for v in bm.verts), Vector()) / max(1, len(bm.verts))
    faces = [f for f in bm.faces if not outer_only or f.normal.dot(f.calc_center_median() - centre) > 0]
    dup = bmesh.ops.duplicate(bm, geom=faces)
    new_faces = [g for g in dup["geom"] if isinstance(g, bmesh.types.BMFace)]
    new_verts = [g for g in dup["geom"] if isinstance(g, bmesh.types.BMVert)]
    bmesh.ops.remove_doubles(bm, verts=new_verts, dist=1e-4 * TALL)
    new_faces = [f for f in new_faces if f.is_valid]
    slot = len(o.data.materials)
    o.data.materials.append(mat)
    for f in new_faces:
        f.material_index = slot
        f.smooth = True
    bm.normal_update()
    moved = {v for f in new_faces for v in f.verts}
    normals = {v: v.normal.copy() for v in moved}
    for v in moved:
        v.co += normals[v] * width
    bmesh.ops.reverse_faces(bm, faces=new_faces)
    bm.to_mesh(o.data)
    bm.free()
    print(f"[banner] {o.name}: an outline of {len(new_faces)} faces, {width:.2f} wide")


def tidy_gltf(path):
    """The names without dots (the HOME Menu froze on lab A's dotted names with its paint, run
    20) and the dragon's solid parts one-sided: pycgfx doubles a two-sided material's every
    vertex. The wings' membranes, the shell, the wordmark and the sparkles stay two-sided."""
    with open(path, encoding="utf-8") as f:
        g = json.load(f)
    seen = set()
    for key in ("nodes", "meshes", "materials", "images", "textures"):
        for item in g.get(key, []):
            if "name" not in item:
                continue
            name = item["name"].replace(".", "_").replace("-", "m")
            while (key, name) in seen:
                name += "x"
            seen.add((key, name))
            item["name"] = name
    one = []
    for m in g.get("materials", []):
        # (the egg's shell is two surfaces, cream outside and glowing inside: one-sided too)
        if (m["name"].startswith("b_") and "membrane" not in m["name"]) or m["name"] in ("heart_glow", "ink", "shell", "shell_glow"):
            m["doubleSided"] = False
            one.append(m["name"])
    # Texture coordinates only where a texture reads them (the skin, the wordmark): 8 bytes a
    # vertex fewer on everything else.
    stripped = 0
    for me in g.get("meshes", []):
        for prim in me["primitives"]:
            mat = g["materials"][prim["material"]] if "material" in prim else {}
            if "baseColorTexture" not in mat.get("pbrMetallicRoughness", {}) and prim["attributes"].pop("TEXCOORD_0", None) is not None:
                stripped += 1
    with open(path, "w", encoding="utf-8") as f:
        json.dump(g, f, indent=1)
    print(f"[banner] names tidied; one-sided: {', '.join(one) or 'none'}; texture coordinates off {stripped} primitives")


def joint(d, bone):
    arm = d["arm"]
    return arm.matrix_world @ arm.pose.bones[bone].head


def dragon_pieces(d, skin):
    """Freeze the posed dragon and cut it into rigid pieces with their pivots at the joints."""
    dg = bpy.context.evaluated_depsgraph_get()
    body = d["body"]
    arm = d["arm"]
    mats = {}

    def colour_of(m):
        painted = any(n.bl_idname == "ShaderNodeAttribute" and n.attribute_name == "vc" for n in m.node_tree.nodes)
        key = SLOT_KEY.get(m.name.split(".")[0])
        if key == "glow":  # the glowing tips in the flames' red: one material and piece fewer (X's 32)
            key = "membrane"
        if painted and "palette" in d and key:
            return tuple(d["palette"][key][:3])
        return toon_colour(m)

    def mat_for(m):
        # A kind's materials of one colour share one (a feather's bands and the plain parts had
        # twins): the HOME Menu held X's 16 materials and froze on the Blazeplume's 20 (run 15).
        key = m.name
        if KIND != "classic" and m is not d["mats"]["body"]:
            key = tuple(round(c, 3) for c in colour_of(m))
        if key not in mats:
            mats[key] = principled("b_" + m.name, (1, 1, 1), 0.7, skin) if m is d["mats"]["body"] else \
                principled("b_" + m.name, colour_of(m), 0.5)
        return mats[key]

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
    # The head and the tail each take a collar of the body's faces round their joint as well:
    # turning, it covers the seam instead of opening a gap in the skin.
    zs = [v.co.z for v in me.vertices]
    tall = max(zs) - min(zs)
    centres = [f.center.copy() for f in me.polygons]

    # A kind's mouth stays shut on the banner: its inside, teeth and tongue are left out (three
    # materials and pieces fewer).
    shut = [KIND != "classic" and body.material_slots[f.material_index].material is d["mats"]["mouth"]
            for f in me.polygons]

    def keep(fi, p):
        if shut[fi]:
            return False
        if face_owner[fi] == p:
            return True
        return p in COLLAR and face_owner[fi] == "body" and (centres[fi] - pivots[p]).length < COLLAR[p] * tall

    for piece in ("body", "head", "tail"):
        part = keep_faces(me, lambda fi, p=piece: keep(fi, p))
        pieces[piece] = [with_origin(f"{piece}_skin", part, pivots[piece], body_mats)]
    # Wings and the parts: each goes with its bone's piece; the eyes and the heart move alone.
    for w in d["wings"]:
        if w.hide_render:
            continue
        pieces["body"].append(with_origin(w.name, frozen(w, dg), pivots["body"], [mat_for(s.material) for s in w.material_slots]))
    eye_meshes, heart_meshes = [], []
    for group, objs in d["groups"].items():
        for o in objs:
            if o.hide_render or (group == "mouth" and KIND != "classic"):  # rare parts not shown; the shut mouth's
                continue
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
    for o in [body, arm] + d["wings"] + d.get("rare_wings", []) + [p for objs in d["groups"].values() for p in objs]:
        bpy.data.objects.remove(o, do_unlink=True)
    for piece, o in merged.items():  # their names back, now the kit's own "body" is gone
        o.name = piece
        if KIND != "classic":  # smooth, as the game shades it (run 25): creases only where it's sharp
            o.data.shade_smooth()
            o.data.set_sharp_from_angle(angle=math.radians(55))
    if KIND != "classic":  # (run 25: turning, a piece opened onto its hollow inside: closed with skin)
        bpy.context.view_layer.update()
        skin_mat = mats.get(d["mats"]["body"].name)
        for piece in ("body", "head", "tail"):
            if piece in merged:
                cap_openings(merged[piece], [pivots["head"], pivots["tail"]], 0.3 * tall, skin_mat)
    return merged, heart_mat


def cap_openings(o, near, radius, skin_mat):
    """Closes a piece's openings round its joints (the body's at the neck and tail, the head's and
    the tail's where they were cut) with faces in the skin, so when the head or tail turns and the
    joint parts a little you see skin, not the hollow inside (run 25, Noah: "any movement means their
    bodies split open"). Openings elsewhere (trimmed inside the egg) stay open."""
    me = o.data
    bm = bmesh.new()
    bm.from_mesh(me)
    uv = bm.loops.layers.uv.active
    uv_of = {}
    for f in bm.faces:
        for lp in f.loops:
            if uv is not None and lp.vert not in uv_of:
                uv_of[lp.vert] = lp[uv].uv.copy()
    rim = [e for e in bm.edges if e.is_boundary]
    made = bmesh.ops.holes_fill(bm, edges=rim, sides=0)["faces"]
    world = o.matrix_world
    far = [f for f in made if min((world @ f.calc_center_median() - p).length for p in near) > radius]
    bmesh.ops.delete(bm, geom=far, context="FACES_ONLY")
    kept = [f for f in made if f.is_valid]
    slot = next((i for i, m in enumerate(me.materials) if m is skin_mat), 0)
    for f in kept:
        f.material_index = slot
        f.smooth = True
        if uv is not None:
            for lp in f.loops:
                if lp.vert in uv_of:
                    lp[uv].uv = uv_of[lp.vert]
    bmesh.ops.triangulate(bm, faces=kept)
    bm.to_mesh(me)
    bm.free()
    print(f"[banner] {o.name}: {len(kept)} openings by its joints capped")


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


def clip_to_egg(o, height, width, cut, floor_z, margin=0.01, yaws=(0.0,)):
    """Drop the faces of a piece that reach out through the shell below its crack (a kind sits
    as its own sit clip has it, its paws forward: they would show through the egg). What's cut
    is inside the shell, so nothing seen changes."""
    bpy.context.view_layer.update()

    def outside(p):
        z = p.z - floor_z
        if z > crack_height(math.atan2(p.y, p.x), height, cut) or z < 0:
            return z < 0  # under the egg's bottom: out too
        zn = min(1.0, max(-1.0, z / (0.5 * height) - 1.0))
        inside = width * egg_taper(zn) * math.sqrt(1.0 - zn * zn) - EGG_WALL * height
        return math.hypot(p.x, p.y) - inside > -margin * height

    mats = []
    for yaw in yaws:  # the piece where it rests and at each end of its wag
        rot = o.rotation_euler.copy()
        rot.z += math.radians(yaw)
        mats.append(Matrix.LocRotScale(o.location, rot, o.scale))
    out = [any(outside(m @ v.co) for m in mats) for v in o.data.vertices]
    bm = bmesh.new()
    bm.from_mesh(o.data)
    bm.verts.ensure_lookup_table()
    gone = [f for f in bm.faces if any(out[v.index] for v in f.verts)]
    bmesh.ops.delete(bm, geom=gone, context="FACES")
    bm.to_mesh(o.data)
    bm.free()
    print(f"[banner] {o.name}: {len(gone)} faces through the shell trimmed")


def drop_hidden(o, height, cut, floor_z, depth=0.12):
    """Drop the faces of a piece wholly inside the egg, deeper than `depth` (of the egg's height)
    under the crack's lowest point: the HOME Menu's camera looks only a little down into the egg,
    so its feet and belly never show (run 25: room in the 512 KB for the full-detail head)."""
    bpy.context.view_layer.update()
    low = min(crack_height(2 * math.pi * k / 64, height, cut) for k in range(64)) - depth * height
    m = Matrix.LocRotScale(o.location, o.rotation_euler, o.scale)
    deep = [(m @ v.co).z - floor_z < low for v in o.data.vertices]
    bm = bmesh.new()
    bm.from_mesh(o.data)
    bm.verts.ensure_lookup_table()
    gone = [f for f in bm.faces if all(deep[v.index] for v in f.verts)]
    bmesh.ops.delete(bm, geom=gone, context="FACES")
    bm.to_mesh(o.data)
    bm.free()
    print(f"[banner] {o.name}: {len(gone)} faces hidden deep in the egg dropped")


def apply_modifiers(o):
    for o2 in bpy.context.view_layer.objects:
        o2.select_set(False)
    o.select_set(True)
    bpy.context.view_layer.objects.active = o
    for m in list(o.modifiers):
        bpy.ops.object.modifier_apply(modifier=m.name)


def wordmark_texture(path, width=256, height=128):
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
    edge = bpy.data.materials.new("edge")
    edge.use_nodes = True
    edge.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.14, 0.05, 0.10, 1)
    gold = bpy.data.materials.new("gold_emit")
    gold.use_nodes = True
    nt = gold.node_tree
    nt.nodes.clear()
    em = nt.nodes.new("ShaderNodeEmission")
    em.inputs["Color"].default_value = (*GOLD, 1)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(em.outputs[0], out.inputs["Surface"])
    font = bpy.data.fonts.load(FONT) if os.path.exists(FONT) else None
    made = []

    def line(body, size, y, name):
        """One line of the wordmark: its dark edge (the letters grown a little), the gold over it."""
        curve = bpy.data.curves.new(name, "FONT")
        curve.body = body
        if font:
            curve.font = font
        curve.align_x, curve.align_y = "CENTER", "CENTER"
        curve.size = size
        curve.offset = 0.035 * size
        curve.materials.append(edge)
        o = bpy.data.objects.new(name, curve)
        o.location.y = y
        scene.collection.objects.link(o)
        gold_curve = curve.copy()
        gold_curve.offset = 0.0
        gold_curve.materials.clear()
        gold_curve.materials.append(gold)
        g = bpy.data.objects.new(name + "_gold", gold_curve)
        g.location = (0, y, 0.01)
        scene.collection.objects.link(g)
        made.extend((o, g))

    # 1.0 (D120): Emberclutch in the top half exactly as it was alone (run 22, Noah: 0.9.13 squeezed
    # it up into the HOME Menu's rounded corners), Skyreach Valley small under it in the lower half
    # (the hatchling and its egg a little smaller below them, build()).
    line("Emberclutch", 1.0, 0.95, "wordmark_text")
    line("Skyreach Valley", 0.42, 0.1, "wordmark_sub")
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
    for o in (*made, cam):  # none of it may reach the banner's glTF (it did, in WP10)
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.data.scenes.remove(scene, do_unlink=True)
    img = bpy.data.images.load(path)
    return img


def wordmark(width):
    img = wordmark_texture(os.path.join(OUT, "wordmark.png"))
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    h = width / 2.0  # (the texture's 256 x 128: Emberclutch on its top half, as the old 256 x 64 one)
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
    """A small still triangle hidden in the egg, first of the scene's objects (glTF lists them
    by name). On the 3DS the HOME Menu froze on every scene made of just the dragon, its egg
    and the wordmark, and showed every one with one more mesh (banner lab 2, 2026-09-24: a
    backdrop, the glow disc or this triangle). The rule inside it isn't known; this keeps the
    scene on the side that works whatever else changes."""
    bm = bmesh.new()
    vs = [bm.verts.new(p) for p in ((-0.2, 0.0, 0.0), (0.2, 0.0, 0.0), (0.0, 0.0, 0.3))]
    bm.faces.new(vs)
    me = bpy.data.meshes.new("aaa_anchor")
    bm.to_mesh(me)
    bm.free()
    o = link(bpy.data.objects.new("aaa_anchor", me))
    o.location = (0.0, 0.0, -10.0)  # inside the egg's cup
    o.data.materials.append(principled("anchor", SHELL, 0.9))
    return o


def star_mesh(name, size):
    """A four-pointed star (long points up, down and to the sides, short ones between), flat,
    facing the camera."""
    bm = bmesh.new()
    centre = bm.verts.new((0, 0, 0))
    ring = []
    for k in range(8):
        a = math.pi / 4 * k
        r = size if k % 2 == 0 else size * 0.32
        ring.append(bm.verts.new((r * math.sin(a), 0, r * math.cos(a))))
    for k in range(8):
        bm.faces.new((centre, ring[k], ring[(k + 1) % 8]))
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    return me


def sparkles():
    """Gold stars round the egg (unlit in the CGFX: make_unlit); animate() makes them glint."""
    mat = principled("sparkle", SPARKLE, 0.5)
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    key = "Emission Color" if "Emission Color" in bsdf.inputs else "Emission"
    bsdf.inputs[key].default_value = (*SPARKLE, 1)  # bright in the review renders too
    bsdf.inputs["Emission Strength"].default_value = 0.8
    stars = []
    for i, (at, size, _) in enumerate(SPARKLES):
        o = link(bpy.data.objects.new(f"sparkle_{i}", star_mesh(f"sparkle_{i}", size * 1.6)))
        o.location = at
        o.scale = (SPARKLE_REST,) * 3
        o.data.materials.append(mat)
        stars.append(o)
    return stars


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
    if KIND == "classic":
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
        visible_points = dm.visible_points
    else:
        d = build_kind(KIND)
        skin = bake_emission(d["body"], d["mats"]["body"], SKIN)
        visible_points = d["visible_points"]
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
    head_top = max((visible_points([d]) or [Vector()]), key=lambda p: p.z)
    pieces, heart_mat = dragon_pieces(d, skin)

    # Scale: the dragon's height sets the egg's; then everything to banner units. The rim of
    # the cracked egg runs just under the heart, so it shows (and beats) above the shell.
    lo = min(v.co.z + pieces["body"].location.z for v in pieces["body"].data.vertices)
    hi = head_top.z
    h = hi - lo
    if "tail" in pieces and KIND == "classic":  # swung out to its left, round beside the body: its wag shows
        pieces["tail"].rotation_euler.z = math.radians(62)
    elif "tail" in pieces:  # a kind's short tail: raised behind its left shoulder, its tip showing over the rim
        pieces["tail"].rotation_euler = (math.radians(TAIL_LIFT[0]), 0.0, math.radians(TAIL_LIFT[1]))
    if "heart" in pieces:  # on the front of the chest, just above the shell's rim (the sit pose lifts it to the face)
        # A kind's big round head overhangs its chest: its heart sits lower, in front of the chin too.
        target = lo + (0.54 if KIND == "classic" else 0.525) * h
        fronts = [pieces["body"]] if KIND == "classic" else [pieces["body"], pieces["head"]]
        near = [v.co + o.location for o in fronts for v in o.data.vertices
                if abs(v.co.x + o.location.x) < 0.08 * h and abs(v.co.z + o.location.z - target) < 0.05 * h]
        if near:
            pieces["heart"].location = (0.0, min(p.y for p in near) - 0.015 * h, target)
    heart_z = pieces["heart"].location.z if "heart" in pieces else lo + 0.5 * h
    drop = d.get("egg_drop", 0.1) if isinstance(d, dict) else 0.1  # the egg's bottom this far (of h) below its feet
    rim = 0.42 * h + drop * h  # up to its belly
    egg_h = 0.95 * h + (drop - 0.1) * h
    print(f"[banner] dragon {h:.2f} tall; heart at {(heart_z - lo) / h:.2f}; rim at {rim / egg_h:.2f} of the egg")
    egg, cap = egg_halves(egg_h, 0.42 * h, rim / egg_h)
    for o in (egg, cap):
        o.location.z = lo - drop * h
        apply_modifiers(o)
    # The cap sits on its head, tipped back a little.
    cap_bottom = min(v.co.z for v in cap.data.vertices)
    cap.data.transform(Matrix.Translation((0, 0, -cap_bottom)))
    cap.location = (head_top.x, head_top.y + 0.08 * h, head_top.z - 0.12 * h)
    cap.rotation_euler = (math.radians(-16), math.radians(10), 0)
    cap.scale = (0.5, 0.5, 0.5)
    dy = fit_in_egg(pieces, egg_h, 0.42 * h, rim / egg_h, lo - drop * h, wag=WAG) if "--no-fit" not in sys.argv else 0.0
    for o in list(pieces.values()) + [cap]:
        o.location.y += dy
    if KIND != "classic" and "body" in pieces:  # its paws forward in the sit: trimmed where they'd show
        clip_to_egg(pieces["body"], egg_h, 0.42 * h, rim / egg_h, lo - drop * h)
    if KIND != "classic" and "tail" in pieces:  # ...and the tail through its whole wag
        clip_to_egg(pieces["tail"], egg_h, 0.42 * h, rim / egg_h, lo - drop * h, yaws=(-WAG, 0.0, WAG))
    if KIND != "classic":  # what's deep in the egg never shows
        for name in ("body", "tail"):
            if name in pieces:
                drop_hidden(pieces[name], egg_h, rim / egg_h, lo - drop * h)

    s = 0.84 * TALL / (hi - lo + 2 * drop * h)  # (run 22: a little smaller, room for the subtitle)
    world_objs = list(pieces.values()) + [egg, cap]
    for o in world_objs:
        o.location = (o.location - Vector((0, 0, lo))) * s
        o.data.transform(Matrix.Scale(s, 4))
    if OUTLINE > 0:  # (run 25: the storybook's ink round the dragon and its egg)
        ink = principled("ink", INK, 0.9)
        ink.use_backface_culling = True  # (the hull's near side culled: in the previews too)
        for name in ("body", "head", "tail"):
            if name in pieces:
                add_outline(pieces[name], OUTLINE * TALL, ink)
        add_outline(egg, 0.7 * OUTLINE * TALL, ink, outer_only=True)
        add_outline(cap, 0.7 * OUTLINE * TALL, ink, outer_only=True)
    # In the frame: the pair just left of centre and low; the wordmark across the top, behind.
    # Nothing else: the HOME Menu's own background shows round them (Noah took the wall out;
    # the flat 2D banner keeps its backdrop, see main()).
    shift = Vector((0.0, 0.0, -11.3))  # centred (run 10: the wordmark sat off centre); 1.0 a little lower
    for o in world_objs:
        o.location += shift
    word = wordmark(30.0)
    # Emberclutch where it was (run 11: its middle at 7.9, 5 px lower on the 3DS than 8.4): the quad
    # is twice as tall now, its top half the old one, so its middle sits a quarter lower.
    word.location = (0.0, 5.0, 7.9 - 30.0 / 2.0 / 4.0)
    # Banner-lab variants (run 3: the new scene froze the HOME Menu, 0.1.1's didn't).
    if "--keep-glow" in sys.argv:
        glow_disc(9.8, (0.55, 0.22, 0.12), (0.0, 16.0, -1.0))
    if "--keep-backdrop" in sys.argv:
        backdrop(90, 50, 20.0)
    if "--no-anchor" not in sys.argv:
        anchor()
    stars = sparkles() if "--no-sparkles" not in sys.argv else []
    return pieces, egg, cap, heart_mat, word, stars


def parent(child, par):
    m = child.matrix_world.copy()
    child.parent = par
    child.matrix_world = m


def cycles(pattern):
    """A motion written on a 96-frame count, (frame, value) pairs, stretched to CYCLE and
    repeated through the loop (a repeated frame dropped)."""
    out = []
    for c in range(FRAMES // CYCLE):
        for f, v in pattern:
            at = c * CYCLE + f * CYCLE / 96
            if not out or at > out[-1][0]:
                out.append((at, v))
    return out


def still_world(pieces, egg, cap, word, stars):
    """--still: the dragon, its egg, the wordmark and the sparkles (frozen mid-glint) joined
    into one rigid piece, "world", with its origin at the scene's centre and no transform of
    its own, for a Y-axis billboard."""
    for o in stars:
        o.scale = (0.6,) * 3
    join = list(pieces.values()) + [egg, cap, word] + list(stars)
    for o in join:
        o.data = o.data.copy()  # single-user, so its transform can be applied
        apply_modifiers(o)
        o.vertex_groups.clear()  # the rig's, left on the frozen pieces (joining them crashes Blender 5.2)
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    for o in join:
        o.select_set(True)
    bpy.context.view_layer.objects.active = pieces["body"]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.object.join()
    world = bpy.context.view_layer.objects.active
    world.name = world.data.name = "world"
    print(f"[banner] still: one piece of {len(join)} ({len(world.data.polygons)} faces)")


def turn_setup(pieces, egg, word, stars):
    """--turn, before animating: the body's pivot moved onto the turning axis (the scene is
    centred; only its depth is off), and the sparkles and the wordmark put under the egg,
    whose origin is on the axis. Nothing goes deeper than P's body > head > eyes."""
    body = pieces["body"]
    dy = body.location.y
    body.data.transform(Matrix.Translation((0.0, dy, 0.0)))
    body.location.y = 0.0
    bpy.context.view_layer.update()  # parent() reads matrix_world (lab 7 lost the wordmark without this)
    for o in [word] + list(stars):
        parent(o, egg)
    bpy.context.view_layer.update()
    print(f"[banner] turn: body pivot moved {dy:+.2f} onto the axis; {len(stars)} sparkles and the wordmark under the egg")


def placeholder_turn(o):
    """10 degrees about the vertical over the loop, which tools/banner_cgfx.py --turn replaces
    with whole turns: pycgfx keeps rotations as Euler angles taken from quaternions, which
    can't pass 90 degrees about the vertical smoothly."""
    o.rotation_euler = (0, 0, 0)
    o.keyframe_insert("rotation_euler", frame=0)
    o.rotation_euler = (0, 0, math.radians(10))
    o.keyframe_insert("rotation_euler", frame=FRAMES)
    o.rotation_euler = (0, 0, 0)


def animate(pieces, cap, heart_mat, stars=()):
    """Rigid node animation only (the HOME Menu freezes on skinned banners). With no pieces
    (--still) only the sparkles glint."""
    bpy.context.preferences.edit.keyframe_new_interpolation_type = "LINEAR"
    if pieces:
        animate_dragon(pieces, cap)
    for o, (_, _, peak) in zip(stars, SPARKLES):  # they glint in turn: a quick swell, a slower fade
        rest, full = (SPARKLE_REST,) * 3, (1.0, 1.0, 1.0)
        for f, s in cycles(((0, rest), (peak - 10, rest), (peak, full), (peak + 14, rest), (96, rest))):
            o.scale = s
            o.keyframe_insert("scale", frame=f)
    for o in bpy.data.objects:  # straight lines between keys (pycgfx: no spline rotations)
        ad = o.animation_data
        if ad and ad.action:
            for fc in getattr(ad.action, "fcurves", []):
                for kp in fc.keyframe_points:
                    kp.interpolation = "LINEAR"


def animate_dragon(pieces, cap):
    body, head, tail = pieces["body"], pieces["head"], pieces["tail"]
    for p in ("head", "tail", "heart"):
        if p in pieces:
            parent(pieces[p], body)
    if "eyes" in pieces:
        parent(pieces["eyes"], head)
    parent(cap, head)
    bpy.context.view_layer.update()

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
    # (a kind's a little gentler, so its joints stay closed: run 25)
    roll, nod = (12, -6) if KIND == "classic" else (8, -4)
    for f, (x, y, z) in cycles(((0, (0, 0, 0)), (18, (0, roll, 0)), (36, (0, 0, 0)), (54, (nod, -roll * 5 / 6, 0)),
                                (72, (0, 0, 0)), (96, (0, 0, 0)))):
        key(head, f, rot=(h0[0] + x, h0[1] + y, h0[2] + z))
    # The tail wags, then rests.
    t0 = tuple(math.degrees(a) for a in tail.rotation_euler)
    for f, yaw in cycles(((0, 0), (8, WAG), (16, -WAG), (24, WAG), (32, -WAG), (40, WAG), (48, 0), (96, 0))):
        key(tail, f, rot=(t0[0], t0[1], t0[2] + yaw))
    # Two blinks: the eyes squash flat and open again.
    if "eyes" in pieces:
        e = pieces["eyes"]
        for f, sz in cycles(((0, 1), (36, 1), (38, 0.08), (41, 1), (76, 1), (78, 0.08), (81, 1), (96, 1))):
            key(e, f, scale=(1, 1, sz))
    # The heart beats: four quick swells a cycle.
    if "heart" in pieces:
        hp = pieces["heart"]
        for f, s in cycles(HEART_BEATS):
            key(hp, f, scale=(s, s, s))
    # A gentle bob.
    b0 = body.location.copy()
    for f, dz in cycles(((0, 0.0), (48, 0.35), (96, 0.0))):
        key(body, f, loc=b0 + Vector((0, 0, dz)))


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
    if mi is None:
        print("[banner] no heart material: colour pulse skipped")
        return
    if not g.get("animations"):  # nothing moves (--still --join-sparkles): the pulse on its own
        g["animations"] = [{"name": "COMMON", "channels": [], "samplers": []}]
    glow = {1.0: HEART_DIM, 1.22: HEART_BRIGHT}  # in step with the beat's swell
    times = [f / FPS for f, _ in cycles(HEART_BEATS)]
    colours = [glow[s] for _, s in cycles(HEART_BEATS)]
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


def make_unlit(path, names=("wordmark", "sparkle")):
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
    pieces, egg, cap, heart_mat, word, stars = build()
    if "--still" in sys.argv:
        still_world(pieces, egg, cap, word, stars)
        pieces, stars = {}, []
    elif "--turn" in sys.argv:
        turn_setup(pieces, egg, word, stars)
    body = pieces.get("body")
    animate(pieces, cap, heart_mat, stars)
    if "--turn" in sys.argv:  # after animate(), which hangs the head, tail and heart on the body
        placeholder_turn(body)
        placeholder_turn(egg)
    elif "--spin-egg" in sys.argv:
        placeholder_turn(egg)
    tris = 0
    for o in bpy.context.scene.objects:
        if o.type == "MESH":
            tris += sum(len(p.vertices) - 2 for p in o.data.polygons)
    print(f"[banner] {len(pieces)} dragon pieces, {tris} triangles in all")
    os.makedirs(OUT, exist_ok=True)
    if KIND != "classic":  # the previews one-sided where the 3DS is (tidy_gltf), so a hole shows here too
        for m in bpy.data.materials:
            if (m.name.startswith("b_") and "membrane" not in m.name) or m.name.split(".")[0] in ("heart_glow", "ink", "shell", "shell_glow"):
                m.use_backface_culling = True
    gltf = os.path.join(OUT, "banner.gltf")
    export(gltf)
    add_heart_colour(gltf)
    make_unlit(gltf, names=("wordmark", "sparkle", "ink"))
    if KIND != "classic":
        tidy_gltf(gltf)
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
    if "--seams" in sys.argv:  # the joints mid-motion, big (run 25: they split open as the head and tail moved)
        for f in (round(8 * CYCLE / 96), round(18 * CYCLE / 96), round(54 * CYCLE / 96)):
            render(os.path.join(REVIEW, f"banner3d_seams_{f}.png"), 1400, 840, f)
    # The HOME Menu turns the banner round as it swaps titles: the back and the side must hold up too.
    cam = scene.camera
    for name, yaw in (("banner3d_back.png", 180.0), ("banner3d_side.png", 90.0)):
        a = math.radians(yaw)
        cam.location = Vector((CAM_AT.y * -math.sin(a), CAM_AT.y * math.cos(a), CAM_AT.z))
        cam.rotation_euler = (math.radians(90), 0, a)
        render(os.path.join(REVIEW, name), 500, 300, 0)
    banner_camera()
    if "--keep-glow" not in sys.argv:  # the 2D banner is a flat picture: it keeps its hearth glow
        glow_disc(9.8, (0.55, 0.22, 0.12), (0.0, 16.0, -1.0))
    if "--keep-backdrop" not in sys.argv:  # ...and its dusk wall
        backdrop(90, 50, 20.0)
    cam = scene.camera  # the 2D banner: the same scene, a little closer (its frame is wider, 2:1)
    cam.data.angle_y = math.radians(24.5)
    cam.location.z -= 0.4
    render(os.path.join(ASSETS, "banner.png"), 1024, 512, 0, final=(256, 128))


main()
