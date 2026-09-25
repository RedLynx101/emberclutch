"""Export a kind for the 3DS: romfs/dragons/<kind>/{hatchling,grown}.ecm, their LOD1 twins
{form}_lod1.ecm, the baked skins {form}[_lod1]_skin.t3x, and (with --reference-dir) parity
references for the PC tests.

  blender -b -P tools/blender/dragonkit/export.py -- --kind pouncer [--form grown] [--lods 0,1]
          [--reference-dir C:/abs/tests/data/kinds]

The .ecm format is the classic dragon's (version 4, src/core/model.cpp; the classic
exporter tools/blender/export_dragon.py documents it): the plan's skeleton (body bones first,
then wing bones), growth tables, the kind's gentle build tables, the idle pose, then meshes:
  body (kind 0), wings (kind 1: variant 0 common, 1 the rare variant's if it has its own),
  parts (kind 2) by group, variant 0 for everyone and 1 for the rare variant only; eyes
  variant 0 has round pupils and 1 slit ones (the game shows them by mood).
"""
import math
import os
import random
import struct
import subprocess
import sys
from pathlib import Path

import bpy
from mathutils import Euler

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from dragonkit import model as km  # noqa: E402
from dragonkit import texture as tex  # noqa: E402

arg = km.arg
ROOT = Path(km.ROOT)
KIND_BODY, KIND_WINGS, KIND_PART = 0, 1, 2
GROUP = {"eyes": 0, "horns": 1, "frill": 2, "spikes": 3, "tail_tip": 4, "heart": 5, "wings": 6, "mouth": 7,
         "runes": 9, "body": 255}
SEX_ANY = 0
PAL_BASE, PAL_ACCENT, PAL_PATTERN, PAL_HORN, PAL_MEMBRANE, PAL_IRIS, PAL_PUPIL, PAL_GLINT, PAL_GLOW, PAL_TONGUE = range(10)
PALETTE_FIELD = 32
MATERIAL_PAINT = {  # material name -> (palette A, palette B, emissive 0..255)
    "body": (PAL_BASE, PAL_ACCENT, 0), "body_plain": (PAL_BASE, PAL_BASE, 0),
    "accent_flat": (PAL_ACCENT, PAL_ACCENT, 0), "pattern_flat": (PAL_PATTERN, PAL_PATTERN, 0),
    "membrane": (PAL_MEMBRANE, PAL_MEMBRANE, 0), "horn": (PAL_HORN, PAL_HORN, 0),
    "iris": (PAL_IRIS, PAL_IRIS, 0), "pupil": (PAL_PUPIL, PAL_PUPIL, 0),
    "glint": (PAL_GLINT, PAL_GLINT, 200), "heart": (PAL_GLINT, PAL_GLOW, 255),
    "tooth": (PAL_GLINT, PAL_HORN, 0), "tongue": (PAL_TONGUE, PAL_TONGUE, 0), "mouth": (PAL_PUPIL, PAL_TONGUE, 0),
    "rune": (PAL_PATTERN, PAL_PATTERN, 230), "glow_flat": (PAL_GLOW, PAL_GLOW, 230),
}
MATERIAL_MIX = {"tooth": 60, "mouth": 110}
HEART_CORE = 0.55
BUILDS = ("sturdy", "sleek", "long")
TEX3DS = Path(os.environ.get("DEVKITPRO", "C:/msys64/opt/devkitpro")) / "tools" / "bin" / "tex3ds.exe"


def material_names(obj):
    return [m.name.split(".")[0] for m in obj.data.materials] or ["body_plain"]


def world_normal_matrix(m):
    return m.to_3x3().inverted_safe().transposed()


def mesh_arrays(objs, palette_of, matrix_of, scale, region_of):
    """Merge objects into one vertex/index list (see the classic exporter). UV seams on a
    single UV-mapped object become extra vertices appended after its own."""
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
            positions.append((m @ v.co) * scale)
            normals.append((nm @ v.normal).normalized())
            skin.append(palette_of(o, v))
            if vmat[v.index] == "heart":
                mix = int(round(255 * min(1.0, v.co.length / (reach * HEART_CORE))))
            elif vmat[v.index] in MATERIAL_MIX:
                mix = MATERIAL_MIX[vmat[v.index]]
            else:
                mix = int(round(mask.data[v.index].color[1] * 255)) if (mask and pa != pb) else 0
            paint.append((pa, pb, mix, emissive))
            uvs.append(tex.CLEAN_UV)
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
                        if (v, uv) not in extra:
                            extra[(v, uv)] = len(positions)
                            for arr in (positions, normals, skin, paint, regions):
                                arr.append(arr[i])
                            uvs.append(uv)
                        i = extra[(v, uv)]
                indices.append(i)
    return positions, normals, skin, paint, indices, uvs, regions


def body_region(o, v):
    attr = o.data.attributes.get("region")
    return attr.data[v.index].value if attr else tex.REGION_CLEAN


def quantize_weights(pairs):
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
        self.palette = palette
        self.keys, self.key_ts, self.shifts, self.objs = [], [], [], []
        self.skin = self.paint = self.indices = self.uvs = self.regions = None


def skinned_mesh(name, kind, group, variant, objs, bone_index, allowed, scale, region_of):
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
    """Rigid parts, baked at every growth key in the idle pose, with per-build seat shifts.
    Entries of the same group and variant become one mesh (the game finds one per pair)."""
    merged = {}
    for group, variant, objs in tagged:
        merged.setdefault((group, variant), []).extend(objs)
    meshes = []
    for (group, variant), objs in merged.items():
        if not objs:
            continue
        bones = [o.constraints[0].subtarget for o in objs]
        used = sorted({bone_index[b] for b in bones})
        local = {b: i for i, b in enumerate(used)}
        m = Mesh(f"{group}_{variant}", KIND_PART, GROUP[group], variant, SEX_ANY, used)
        m.bone_of = {o.name: local[bone_index[b]] for o, b in zip(objs, bones)}
        m.objs = objs
        m.snapped = group in d["snap"]
        meshes.append(m)
    for t in km.F["key_ts"]:
        km.rest_pose(d, t)
        km.apply_t(d, t, "neutral")
        for m in meshes:
            pos, nrm, skin, paint, idx, uvs, regions = mesh_arrays(
                m.objs, lambda o, v, m=m: (m.bone_of[o.name], m.bone_of[o.name], 255, 0),
                lambda o: o.matrix_basis.copy(), scale, lambda o, v: tex.REGION_CLEAN)
            m.keys.append((pos, nrm))
            m.key_ts.append(t)
            m.skin, m.paint, m.indices, m.uvs, m.regions = skin, paint, idx, uvs, regions
        seated = [m for m in meshes if m.snapped]
        neutral = {o.name: o.matrix_basis.translation.copy() for m in seated for o in m.objs}
        per_build = []
        for build in BUILDS:
            km.apply_t(d, t, build)
            per_build.append({o.name: (o.matrix_basis.translation - neutral[o.name]) * scale
                              for m in seated for o in m.objs})
        km.apply_t(d, t, "neutral")
        for m in seated:
            m.shifts.append([[moved[o.name] for o in m.objs] for moved in per_build])
    return meshes


def write_ecm(path, d, meshes, order, scale):
    arm = d["arm"]
    out = bytearray()
    out += b"ECM1" + struct.pack("<HH", 4, len(order))
    index = {n: i for i, n in enumerate(order)}
    for name in order:
        bone = arm.data.bones[name]
        parent = index[bone.parent.name] if bone.parent else -1
        flags = 1 if name.startswith("wing") else 0
        out += struct.pack("<16sbB2x", name.encode()[:15], parent, flags)
        ml = bone.matrix_local
        for r in range(3):
            out += struct.pack("<4f", ml[r][0], ml[r][1], ml[r][2], ml[r][3] * scale)
    young = km.young_tables()
    for name in order:
        out += struct.pack("<3f", *young[name])
    for build in BUILDS:
        for name in order:
            out += struct.pack("<2f", *km.F["builds"][build].get(km.scale_key(name), (1.0, 1.0)))
    for name in order:
        out += struct.pack("<3f", *km.F["base_pose"].get(name, (0.0, 0.0, 0.0)))
    for name in order:
        out += struct.pack("<f", km.F["young_pose"].get(name, 0.0))
    out += struct.pack("<H", len(meshes))
    for m in meshes:
        pal = list(m.palette) + [0] * (PALETTE_FIELD - len(m.palette))
        assert len(m.palette) <= 25, f"{m.name}: {len(m.palette)} bones in one draw (the shader's budget is 25)"
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
        for uv in m.uvs:
            out += struct.pack("<2f", *uv)
        out += bytes(m.regions)
        out += struct.pack(f"<{len(m.indices)}H", *m.indices)
        out += struct.pack("<B", len(m.objs) if m.shifts else 0)
        if m.shifts:
            assert len(m.objs) < 256, m.name
            piece = [i for i, o in enumerate(m.objs) for _ in o.data.vertices]
            assert len(piece) == n_verts, m.name
            out += bytes(piece)
            for key in m.shifts:
                for pieces in key:
                    for s in pieces:
                        out += struct.pack("<3f", s.x, s.y, s.z)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(bytes(out))
    return len(out)


def write_reference(path, d, meshes, order, sources, scale):
    """Poses + Blender-deformed positions of sampled vertices (armature space), for the PC
    parity test (tests/test_kinds.cpp)."""
    arm = d["arm"]
    arm.location = (0, 0, 0)
    arm.rotation_euler = (0, 0, 0)
    rng = random.Random(7)
    cases = []
    for t, build, randomize in ((0.0, "neutral", False), (0.7, "long", True), (1.0, "sturdy", True)):
        pb = arm.pose.bones
        for name in order:
            pb[name].rotation_mode = "XYZ"
            pb[name].rotation_euler = (0, 0, 0)
        km.rest_pose(d, t)
        km.apply_t(d, t, build)
        km.rest_pose(d, t)
        if randomize:
            for name in order:
                pb[name].rotation_euler = Euler([math.radians(rng.uniform(-35, 35)) for _ in range(3)])
        bone_scales, _ = km.scales_for_t(t, build)
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


def bake_skin(d, form, lod, out_dir):
    name = f"{form}_skin" if lod == 0 else f"{form}_lod{lod}_skin"
    png = ROOT / "build" / "textures" / "kinds" / km.KIND.META["name"] / f"{name}.png"
    png.parent.mkdir(parents=True, exist_ok=True)
    rgba = tex.bake_skin(d["body"], form, 256 if lod == 0 else 128, km.KIND)
    tex.save_png(rgba, png)
    out = out_dir / f"{name}.t3x"
    subprocess.run([str(TEX3DS), "-f", "rgba8", "-m", "box", "-z", "none", "-o", str(out), str(png)], check=True)
    return out


def export_form(form, lod, out_dir, reference_dir):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    km.set_lod(lod)
    d = km.build_dragon(form, 0, for_export=True)
    scale = km.F["export_scale"]
    order = list(km.PLAN.BONE_ORDER)
    assert len(order) == len(d["arm"].data.bones) and len(order) <= 40, "the plan's bones don't match the rig"
    nbody = len(km.PLAN.BONES)
    assert all(not n.startswith("wing") for n in order[:nbody]) and all(n.startswith("wing") for n in order[nbody:])
    bone_index = {n: i for i, n in enumerate(order)}
    out_dir.mkdir(parents=True, exist_ok=True)
    skin = bake_skin(d, form, lod, out_dir)
    meshes, sources = [], []
    meshes.append(skinned_mesh("body", KIND_BODY, GROUP["body"], 0, [d["body"]], bone_index,
                               lambda n: not n.startswith("wing"), scale, body_region))
    sources.append([d["body"]])
    wings_region = tex.REGIONS.index("wings")
    for v, objs in ((0, d["wings"]), (1, d["rare_wings"])):
        if objs:
            meshes.append(skinned_mesh(f"wings_{v}", KIND_WINGS, GROUP["wings"], v, objs, bone_index, km.wing_keep,
                                       scale, lambda o, vv: wings_region))
            sources.append(objs)
    for m in part_meshes(d, d["tagged"], bone_index, scale):
        meshes.append(m)
        sources.append(m.objs)
    path = out_dir / (f"{form}.ecm" if lod == 0 else f"{form}_lod{lod}.ecm")
    size = write_ecm(path, d, meshes, order, scale)
    total = 0
    print(f"[export] {path} {size} bytes, {len(order)} bones, {len(meshes)} meshes; skin {skin.name}")
    for m in meshes:
        tris = len(m.indices) // 3
        total += tris
        print(f"[export]   {m.name:16s} kind {m.kind} var {m.variant} verts {len(m.keys[0][0]):5d} tris {tris:5d} "
              f"palette {len(m.palette):2d} keys {len(m.keys)}")
    if reference_dir and lod == 0:
        ref = Path(reference_dir) / f"{km.KIND.META['name']}_{form}_reference.ecr"
        n = write_reference(ref, d, meshes, order, sources, scale)
        print(f"[export] reference {ref}: {n} samples")


def main():
    km.use_kind(arg("--kind", "pouncer"))
    out_dir = Path(arg("--out-dir", str(ROOT / "romfs" / "dragons" / km.KIND.META["name"])))
    forms = [arg("--form")] if arg("--form") else ["hatchling", "grown"]
    lods = [int(x) for x in arg("--lods", "0,1").split(",")]
    for form in forms:
        for lod in lods:
            export_form(form, lod, out_dir, arg("--reference-dir"))


main()
