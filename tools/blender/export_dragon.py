"""Export the dragon forms for the 3DS: romfs/models/{hatchling,grown}.ecm and their LOD1
twins {form}_lod1.ecm (+ parity references for LOD0).

  blender -b -P tools/blender/export_dragon.py -- --out-dir romfs/models --reference-dir tests/data
  blender -b -P tools/blender/export_dragon.py -- --form hatchling --lods 0 --out-dir romfs/models

Builds each form with tools/blender/dragon_model.py, adds every Alpha 1 part variant, and
writes the .ecm format read by src/core/model.cpp (docs/tech/architecture.md section 5):

  * skeleton (body bones first = one 25-bone draw, then 12 wing bones), rest matrices;
  * growth tables: bone scales at t = 0 (runtime lerps to 1 by growth t), build multipliers
    (sturdy/sleek/long), the idle pose (Euler XYZ) and the young head lift;
  * meshes: body (1 key), wing variants (1 key), part variants baked at 4 growth keys
    (t = 0, .35, .7, 1) in armature rest space, rigidly bound to one bone each. Parts are
    scaled and snapped to the body surface per key, with the neutral build.

The hatchling form is modelled at a comfortable scale and written scaled by its
export_scale (uniform scaling commutes with the skinning, so parity is unaffected).
Vertex paint = palette indices + mix + emissive (the shader looks up per-dragon colours).
Each vertex also carries a UV into its form's skin texture (romfs/models/<form>[_lod1]_skin.t3x,
baked here by tools/blender/dragon_texture.py) and a body region for dirt (D46). The body's
UV seams become extra vertices appended after the Blender vertices, so vertex indices (and
the parity reference) stay those of the Blender mesh.
The reference file holds poses and Blender-deformed vertex positions; the PC tests check
that src/core/skeleton reproduces them.
"""
import math
import os
import random
import struct
import sys
from pathlib import Path

import subprocess

import bpy
from mathutils import Euler, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dragon_model as dm  # noqa: E402
import dragon_texture as dt  # noqa: E402

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = Path(arg("--out-dir", str(ROOT / "romfs" / "models")))
REFERENCE_DIR = arg("--reference-dir")
FORM_NAMES = [arg("--form")] if arg("--form") else ["hatchling", "grown"]
LODS = [int(x) for x in arg("--lods", "0,1").split(",")]

KIND_BODY, KIND_WINGS, KIND_PART = 0, 1, 2
GROUP = {"eyes": 0, "horns": 1, "frill": 2, "spikes": 3, "tail_tip": 4, "heart": 5, "wings": 6, "mouth": 7,
         "body": 255}
SEX_ANY, SEX_MALE, SEX_FEMALE = 0, 1, 2
# Variant ids match the genome enums in src/core/genetics.hpp. The dorsal ridge (group
# "spikes") is picked by the frill gene: 0 spikes, 1 fin sail, 3 plumes (leaf falls back to 0).
HORNS = {"nubs": 0, "swept": 1}
FRILL = {"none": 0, "fin": 1, "leaf": 2, "feather": 3}
RIDGE = {"spikes": 0, "fin": 1, "feather": 3}
WINGS = {"classic": 0, "plumed": 1, "sail": 2}
TAIL = {"plain": 0, "spade": 1, "tuft": 2, "fan": 3}
# Palette slots (src/core/model.hpp kPal*): colours are set per dragon at runtime.
PAL_BASE, PAL_ACCENT, PAL_PATTERN, PAL_HORN, PAL_MEMBRANE, PAL_IRIS, PAL_PUPIL, PAL_GLINT, PAL_GLOW, PAL_TONGUE = range(10)
PALETTE_FIELD = 32  # palette bytes per mesh in the file (src/core/model.hpp kPaletteField)
EMISSIVE_STYLE = {"v3": {"iris": 230, "membrane": 110}}  # review R5 v3: glowing eyes and wings
MATERIAL_PAINT = {  # material name -> (palette A, palette B, emissive 0..255)
    "body": (PAL_BASE, PAL_ACCENT, 0), "body_plain": (PAL_BASE, PAL_BASE, 0),
    "accent_flat": (PAL_ACCENT, PAL_ACCENT, 0), "membrane": (PAL_MEMBRANE, PAL_MEMBRANE, 0),
    "horn": (PAL_HORN, PAL_HORN, 0), "iris": (PAL_IRIS, PAL_IRIS, 0), "pupil": (PAL_PUPIL, PAL_PUPIL, 0),
    "glint": (PAL_GLINT, PAL_GLINT, 200), "heart": (PAL_GLINT, PAL_GLOW, 255),
    "tooth": (PAL_GLINT, PAL_HORN, 0), "tongue": (PAL_TONGUE, PAL_TONGUE, 0), "mouth": (PAL_PUPIL, PAL_TONGUE, 0),
}
MATERIAL_MIX = {"tooth": 60, "mouth": 110}  # fixed A -> B mix: ivory teeth, a dark rosy mouth
HEART_CORE = 0.55  # the heartglow is white-hot inside this fraction of its radius, glow-coloured at the rim
SEX_SCALE = {"horns": {SEX_MALE: 1.15, SEX_FEMALE: 1.0}, "frill": {SEX_MALE: 1.12, SEX_FEMALE: 1.0},
             "tail_tip": {SEX_MALE: 1.0, SEX_FEMALE: 1.15}}


def bone_order(arm):
    names = [b[0] for b in dm.BONES] + dm.WING_BONES
    assert len(names) == len(arm.data.bones)
    return names


def material_names(obj):
    return [m.name.split(".")[0] for m in obj.data.materials] or ["body_plain"]


# ---------------------------------------------------------------------------------- build
def build_all(form):
    """The Ember base dragon of a form plus every other Alpha 1 part variant, all tagged."""
    d = dm.build_dragon("ember", form)
    mats, arm = d["mats"], d["arm"]
    tagged = []  # (group, variant, sex, [objects])

    tag = lambda group, variant, sex, objs: tagged.append((group, variant, sex, list(objs)))  # noqa: E731
    tag("eyes", 0, SEX_ANY, d["groups"]["eyes"])
    tag("spikes", RIDGE["spikes"], SEX_ANY, d["groups"]["spikes"])
    tag("heart", 0, SEX_ANY, d["groups"]["heart"])
    tag("mouth", 0, SEX_ANY, d["groups"]["mouth"])

    def add_part(group, variant, sex, pieces):
        """pieces: [(object, bone)]"""
        s = SEX_SCALE.get(group, {}).get(sex, 1.0)
        for o, bone in pieces:
            o.scale = Vector(o.scale) * s
            dm.attach(d, group, o, bone)
        tag(group, variant, sex, [o for o, _ in pieces])

    # The base build made Ember's horns and tail unsexed; rebuild every variant explicitly so
    # each sex has its own baked mesh.
    for o in d["groups"]["horns"] + d["groups"]["tail_tip"]:
        bpy.data.objects.remove(o, do_unlink=True)
    for g in ("horns", "tail_tip"):
        d["groups"][g].clear()
        d["snap"].get(g, []).clear()

    for sex in (SEX_MALE, SEX_FEMALE):
        for kind in ("swept", "nubs"):
            add_part("horns", HORNS[kind], sex, [(o, "head") for o in dm.build_horns(kind, mats)])
        for kind in ("fin", "feather"):
            add_part("frill", FRILL[kind], sex, [(o, "head") for o in dm.build_frill(kind, mats)])
        for kind in ("spade", "fan", "tuft"):
            add_part("tail_tip", TAIL[kind], sex, [(dm.build_tail_tip(kind, mats), "tail4")])
    for kind in ("fin", "feather"):
        add_part("spikes", RIDGE[kind], SEX_ANY, dm.build_ridge(kind, mats))

    wings = {"classic": d["wings"]}
    for kind in ("plumed", "sail"):
        objs = dm.build_wings(kind, mats)
        for o in objs:
            dm.bind_wing(o, arm)
        wings[kind] = objs
    return d, tagged, wings


# ---------------------------------------------------------------------------------- mesh data
def world_normal_matrix(m):
    return m.to_3x3().inverted_safe().transposed()


def mesh_arrays(objs, palette_of, matrix_of, scale, region_of):
    """Merge objects into one vertex/index list. palette_of(obj, vertex) -> (b0, b1, w0, w1);
    region_of(obj, vertex) -> body region. Paint follows each face's material (eyes are one
    object with iris/pupil/glint faces). An object with a UV map gets a vertex copy for every
    extra UV a vertex has (its UV seams), appended after its own vertices."""
    positions, normals, skin, paint, indices, uvs, regions = [], [], [], [], [], [], []
    for o in objs:
        me = o.data
        me.calc_loop_triangles()
        m = matrix_of(o)
        nm = world_normal_matrix(m)
        names = material_names(o)
        vmat = [names[0]] * len(me.vertices)
        for p in me.polygons:
            for v in p.vertices:
                vmat[v] = names[min(p.material_index, len(names) - 1)]
        mask = me.color_attributes.get("mask")
        reach = max((v.co.length for v in me.vertices), default=1.0) or 1.0
        base = len(positions)
        for v in me.vertices:
            pa, pb, emissive = MATERIAL_PAINT[vmat[v.index]]
            emissive = EMISSIVE_STYLE.get(dm.STYLE, {}).get(vmat[v.index], emissive)
            positions.append((m @ v.co) * scale)
            normals.append((nm @ v.normal).normalized())
            skin.append(palette_of(o, v))
            if vmat[v.index] == "heart":  # radial white-hot core (the object origin is the heart's centre)
                mix = int(round(255 * min(1.0, v.co.length / (reach * HEART_CORE))))
            elif vmat[v.index] in MATERIAL_MIX:
                mix = MATERIAL_MIX[vmat[v.index]]
            else:
                mix = int(round(mask.data[v.index].color[1] * 255)) if (mask and pa != pb) else 0
            paint.append((pa, pb, mix, emissive))
            uvs.append(dt.CLEAN_UV)
            regions.append(region_of(o, v))
        uvl = me.uv_layers.get("UVMap")
        if uvl:
            assert len(objs) == 1, "UV seams would shift the vertices of later objects"
        first, extra = {}, {}
        for tri in me.loop_triangles:
            for v, li in zip(tri.vertices, tri.loops):
                i = base + v
                if uvl:
                    uv = (round(uvl.data[li].uv[0], 5), round(uvl.data[li].uv[1], 5))
                    if v not in first:
                        first[v] = uv
                        uvs[i] = uv
                    elif first[v] != uv:
                        if (v, uv) not in extra:  # a seam: the same point with another UV
                            extra[(v, uv)] = len(positions)
                            for arr in (positions, normals, skin, paint, regions):
                                arr.append(arr[i])
                            uvs.append(uv)
                        i = extra[(v, uv)]
                indices.append(i)
    return positions, normals, skin, paint, indices, uvs, regions


def body_region(o, v):
    attr = o.data.attributes.get("region")
    return attr.data[v.index].value if attr else dt.REGION_CLEAN


def quantize_weights(pairs):
    """Top two (bone, weight) pairs -> (b0, b1, w0, w1) with w0 + w1 == 255."""
    pairs = sorted(pairs, key=lambda p: -p[1])[:2]
    assert pairs, "vertex without bone weights"
    if len(pairs) == 1:
        pairs.append((pairs[0][0], 0.0))
    total = pairs[0][1] + pairs[1][1] or 1.0
    w0 = int(round(255 * pairs[0][1] / total))
    return pairs[0][0], pairs[1][0], w0, 255 - w0


class Mesh:
    def __init__(self, name, kind, group, variant, sex, palette):
        self.name, self.kind, self.group, self.variant, self.sex = name, kind, group, variant, sex
        self.palette = palette  # skeleton bone indices
        self.keys = []          # [(positions, normals)] per key
        self.key_ts = []
        self.skin = self.paint = self.indices = self.uvs = self.regions = None


def skinned_mesh(name, kind, group, variant, objs, bone_index, allowed, scale, region_of):
    """Body / wings: deformed by up to two bones from the mesh's palette."""
    used = sorted({bone_index[vg.name] for o in objs for vg in o.vertex_groups if allowed(vg.name)})
    local = {b: i for i, b in enumerate(used)}

    def palette_of(o, v):
        names = {vg.index: vg.name for vg in o.vertex_groups}
        pairs = [(local[bone_index[names[g.group]]], g.weight) for g in v.groups
                 if g.weight > 0 and allowed(names[g.group])]
        return quantize_weights(pairs)

    pos, nrm, skin, paint, idx, uvs, regions = mesh_arrays(objs, palette_of, lambda o: o.matrix_world.copy(), scale,
                                                          region_of)
    m = Mesh(name, kind, group, variant, SEX_ANY, used)
    m.keys, m.key_ts = [(pos, nrm)], [1.0]
    m.skin, m.paint, m.indices, m.uvs, m.regions = skin, paint, idx, uvs, regions
    return m


def part_meshes(d, tagged, bone_index, scale):
    """Rigid parts: one bone each; positions baked at every growth key (neutral build), in
    armature rest space (the Child-Of basis), so runtime = key lerp + rigid bone skinning."""
    meshes = []
    for group, variant, sex, objs in tagged:
        bones = [o.constraints[0].subtarget for o in objs]
        used = sorted({bone_index[b] for b in bones})
        local = {b: i for i, b in enumerate(used)}
        m = Mesh(f"{group}_{variant}_{sex}", KIND_PART, GROUP[group], variant, sex, used)
        m.bone_of = {o.name: local[bone_index[b]] for o, b in zip(objs, bones)}
        m.objs = objs
        meshes.append(m)
    for t in dm.F["key_ts"]:
        dm.apply_t(d, t, "neutral")  # scales + surface snap for every part at once
        for m in meshes:
            pos, nrm, skin, paint, idx, uvs, regions = mesh_arrays(
                m.objs, lambda o, v, m=m: (m.bone_of[o.name], m.bone_of[o.name], 255, 0),
                lambda o: o.matrix_basis.copy(), scale, lambda o, v: dt.REGION_CLEAN)
            m.keys.append((pos, nrm))
            m.key_ts.append(t)
            m.skin, m.paint, m.indices, m.uvs, m.regions = skin, paint, idx, uvs, regions
    return meshes


# ---------------------------------------------------------------------------------- writing
def write_ecm(path, d, meshes, order, scale):
    arm = d["arm"]
    out = bytearray()
    out += b"ECM1" + struct.pack("<HH", 3, len(order))
    index = {n: i for i, n in enumerate(order)}
    for name in order:
        bone = arm.data.bones[name]
        parent = index[bone.parent.name] if bone.parent else -1
        flags = 1 if name.startswith("wing") else 0
        out += struct.pack("<16sbB2x", name.encode()[:15], parent, flags)
        ml = bone.matrix_local
        for r in range(3):
            out += struct.pack("<4f", ml[r][0], ml[r][1], ml[r][2], ml[r][3] * scale)
    # growth tables
    young = dm.young_tables()
    for name in order:
        out += struct.pack("<3f", *young[name])
    for build in ("sturdy", "sleek", "long"):
        for name in order:
            out += struct.pack("<2f", *dm.F["builds"][build].get(dm.scale_key(name), (1.0, 1.0)))
    for name in order:
        out += struct.pack("<3f", *dm.F["base_pose"].get(name, (0.0, 0.0, 0.0)))
    for name in order:
        out += struct.pack("<f", dm.F["young_pose"].get(name, 0.0))
    # meshes
    out += struct.pack("<H", len(meshes))
    for m in meshes:
        pal = list(m.palette) + [0] * (PALETTE_FIELD - len(m.palette))
        assert len(m.palette) <= 25, m.name  # the shader's bone budget (src/core/model.hpp kMaxPalette)
        n_verts = len(m.keys[0][0])
        assert n_verts < 65536 and len(m.indices) < 65536, m.name
        ts = list(m.key_ts) + [0.0] * (4 - len(m.key_ts))
        out += struct.pack(f"<16sBBBBB{PALETTE_FIELD}sB3x4fHH", m.name.encode()[:15], m.kind, m.group, m.variant, m.sex,
                           len(m.palette), bytes(pal), len(m.keys), *ts, n_verts, len(m.indices))
        for pos, nrm in m.keys:
            for p in pos:
                out += struct.pack("<3f", p.x, p.y, p.z)
            for n in nrm:
                out += struct.pack("<3f", n.x, n.y, n.z)
        for sk in m.skin:
            out += struct.pack("<4B", *sk)
        for pt in m.paint:
            out += struct.pack("<4B", *pt)
        for uv in m.uvs:  # v3: skin texture coordinates and body regions
            out += struct.pack("<2f", *uv)
        out += bytes(m.regions)
        out += struct.pack(f"<{len(m.indices)}H", *m.indices)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(bytes(out))
    return len(out)


# ---------------------------------------------------------------------------------- reference
def write_reference(path, d, meshes, order, sources, scale):
    """Poses + Blender-deformed positions of sampled vertices (armature space)."""
    arm = d["arm"]
    arm.location = (0, 0, 0)
    arm.rotation_euler = (0, 0, 0)
    rng = random.Random(7)
    cases = []
    for t, build, randomize in ((0.0, "neutral", False), (0.7, "long", True), (1.0, "sturdy", True)):
        pb = arm.pose.bones
        # Parts are seated on the surface in the rest pose with the neutral build, exactly as
        # the exported keys are; posing afterwards only moves them rigidly with their bone.
        for name in order:
            pb[name].rotation_mode = "XYZ"
            pb[name].rotation_euler = (0, 0, 0)
        dm.apply_t(d, t, "neutral")
        dm.rest_pose(d, t)
        if randomize:  # arbitrary rotations on every bone exercise the whole chain
            for name in order:
                pb[name].rotation_euler = Euler([math.radians(rng.uniform(-35, 35)) for _ in range(3)])
        # The build then only changes bone scales, exactly what the runtime does.
        bone_scales, _ = dm.scales_for_t(t, build)
        for name, sc in bone_scales.items():
            pb[name].scale = sc
        bpy.context.view_layer.update()
        dg = bpy.context.evaluated_depsgraph_get()
        poses = []
        for name in order:
            q = pb[name].rotation_euler.to_quaternion()
            s = pb[name].scale
            poses.append((q.x, q.y, q.z, q.w, s.x, s.y, s.z))
        samples = []
        for mi, m in enumerate(meshes):
            if m.kind == KIND_PART and t not in m.key_ts:
                continue
            step = max(1, len(m.keys[0][0]) // 40)
            flat = []
            for o in sources[mi]:
                ev = o.evaluated_get(dg)
                me = ev.to_mesh()
                flat += [ev.matrix_world @ v.co for v in me.vertices]
                ev.to_mesh_clear()
            for vi in range(0, len(flat), step):
                samples.append((mi, vi, flat[vi] * scale))
        cases.append((t, build, poses, samples))

    builds = {"neutral": 255, "sturdy": 0, "sleek": 1, "long": 2}
    out = bytearray(b"ECR1" + struct.pack("<H", len(cases)))
    for t, build, poses, samples in cases:
        out += struct.pack("<fB3xH", t, builds[build], len(poses))
        for p in poses:
            out += struct.pack("<7f", *p)
        out += struct.pack("<I", len(samples))
        for mi, vi, p in samples:
            out += struct.pack("<HH3f", mi, vi, p.x, p.y, p.z)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(bytes(out))
    return sum(len(c[3]) for c in cases)


# ---------------------------------------------------------------------------------- skin
TEX3DS = Path(os.environ.get("DEVKITPRO", "C:/msys64/opt/devkitpro")) / "tools" / "bin" / "tex3ds.exe"


def bake_skin(d, form, lod):
    """Bake the body's skin texture (rest pose) and convert it for the 3DS with mipmaps."""
    name = f"{form}_skin" if lod == 0 else f"{form}_lod{lod}_skin"
    png = ROOT / "build" / "textures" / ("" if dm.STYLE == "current" else dm.STYLE) / f"{name}.png"
    png.parent.mkdir(parents=True, exist_ok=True)
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    rgba = dt.bake_skin(d["body"], form, 256 if lod == 0 else 128)
    dt.save_png(rgba, png)
    out = OUT_DIR / f"{name}.t3x"
    subprocess.run([str(TEX3DS), "-f", "rgba8", "-m", "box", "-z", "none", "-o", str(out), str(png)], check=True)
    print(f"[export] {out} ({out.stat().st_size} bytes)")


# ---------------------------------------------------------------------------------- main
def export_form(form, lod):
    """LOD1 shares the skeleton, growth tables and part layout; only the mesh detail drops,
    so the LOD0 parity reference covers its rig too."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    dm.set_lod(lod)
    d, tagged, wings = build_all(form)
    scale = dm.F["export_scale"]
    arm = d["arm"]
    order = bone_order(arm)
    bone_index = {n: i for i, n in enumerate(order)}
    assert all(not n.startswith("wing") for n in order[:len(dm.BONES)]), "body bones must come first"

    bake_skin(d, form, lod)
    meshes, sources = [], []
    meshes.append(skinned_mesh("body", KIND_BODY, GROUP["body"], 0, [d["body"]], bone_index,
                               lambda n: not n.startswith("wing"), scale, body_region))
    sources.append([d["body"]])
    wings_region = dt.REGIONS.index("wings")
    for kind, objs in wings.items():
        meshes.append(skinned_mesh(f"wings_{kind}", KIND_WINGS, GROUP["wings"], WINGS[kind], objs, bone_index,
                                   dm.wing_keep, scale, lambda o, v: wings_region))
        sources.append(objs)
    for m in part_meshes(d, tagged, bone_index, scale):
        meshes.append(m)
        sources.append(m.objs)

    path = OUT_DIR / (f"{form}.ecm" if lod == 0 else f"{form}_lod{lod}.ecm")
    size = write_ecm(path, d, meshes, order, scale)
    print(f"[export] {path} {size} bytes, {len(order)} bones, {len(meshes)} meshes, scale {scale}")
    for m in meshes:
        print(f"[export]   {m.name:18s} kind {m.kind} verts {len(m.keys[0][0]):5d} tris {len(m.indices) // 3:5d} "
              f"palette {len(m.palette):2d} keys {len(m.keys)}")
    if REFERENCE_DIR and lod == 0:
        ref = Path(REFERENCE_DIR) / f"{form}_reference.ecr"
        n = write_reference(ref, d, meshes, order, sources, scale)
        print(f"[export] reference {ref}: {n} samples")


for _form in FORM_NAMES:
    for _lod in LODS:
        export_form(_form, _lod)
