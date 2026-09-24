"""Export the dragon for the 3DS: romfs/models/dragon.ecm (+ a parity reference for the tests).

  blender -b -P tools/blender/export_dragon.py -- --out romfs/models/dragon.ecm --reference tests/data/dragon_reference.ecr

Builds the model with tools/blender/dragon_model.py, adds every Alpha 1 part variant, and
writes the .ecm format read by src/core/model.cpp (docs/tech/architecture.md section 5):

  * skeleton (body bones first = one 24-bone draw, then wing bones), rest matrices;
  * growth tables: hatchling bone scales (runtime lerps to 1 by growth t), build
    multipliers (sturdy/sleek/long), the idle pose (Euler XYZ) and the hatchling neck lift;
  * meshes: body (1 key), wing variants (1 key), part variants baked at 4 growth keys
    (t = 0, .35, .7, 1) in armature rest space, rigidly bound to one bone each. Parts are
    scaled and snapped to the body surface per key, with the neutral build.

Vertex paint = palette indices + mix + emissive (the shader looks up per-dragon colours).
The reference file holds poses and Blender-deformed vertex positions; the PC tests check
that src/core/skeleton reproduces them.
"""
import math
import os
import random
import struct
import sys
from pathlib import Path

import bpy
from mathutils import Euler, Matrix, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dragon_model as dm  # noqa: E402

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


ROOT = Path(__file__).resolve().parents[2]
OUT = Path(arg("--out", str(ROOT / "romfs" / "models" / "dragon.ecm")))
REFERENCE = arg("--reference")

KEY_TS = (0.0, 0.35, 0.70, 1.0)
KIND_BODY, KIND_WINGS, KIND_PART = 0, 1, 2
GROUP = {"eyes": 0, "horns": 1, "frill": 2, "spikes": 3, "tail_tip": 4, "heart": 5, "wings": 6, "body": 255}
SEX_ANY, SEX_MALE, SEX_FEMALE = 0, 1, 2
# Variant ids match the genome enums in src/core/genetics.hpp.
HORNS = {"nubs": 0, "swept": 1}
FRILL = {"none": 0, "fin": 1, "leaf": 2, "feather": 3}
WINGS = {"membrane": 0, "feathered": 1, "fin": 2}
TAIL = {"plain": 0, "spade": 1, "tuft": 2, "fan": 3}
# Palette slots (src/core/model.hpp kPal*): colours are set per dragon at runtime.
PAL_BASE, PAL_ACCENT, PAL_PATTERN, PAL_HORN, PAL_MEMBRANE, PAL_IRIS, PAL_PUPIL, PAL_GLINT, PAL_GLOW = range(9)
MATERIAL_PAINT = {  # material name -> (palette A, palette B, emissive 0..255)
    "body": (PAL_BASE, PAL_ACCENT, 0), "body_plain": (PAL_BASE, PAL_BASE, 0),
    "accent_flat": (PAL_ACCENT, PAL_ACCENT, 0), "membrane": (PAL_MEMBRANE, PAL_MEMBRANE, 0),
    "horn": (PAL_HORN, PAL_HORN, 0), "iris": (PAL_IRIS, PAL_IRIS, 0), "pupil": (PAL_PUPIL, PAL_PUPIL, 0),
    "glint": (PAL_GLINT, PAL_GLINT, 200), "heart": (PAL_GLOW, PAL_GLOW, 255),
}
SEX_SCALE = {"horns": {SEX_MALE: 1.15, SEX_FEMALE: 1.0}, "frill": {SEX_MALE: 1.12, SEX_FEMALE: 1.0},
             "tail_tip": {SEX_MALE: 1.0, SEX_FEMALE: 1.15}}


def bone_order(arm):
    names = [b[0] for b in dm.BONES]
    for side in ("L", "R"):
        names += [f"{n}_{side}" for n in ("wing_arm", "wing_fore", "wing_f1", "wing_f2", "wing_f3")]
    assert len(names) == len(arm.data.bones)
    return names


def base_key(name):
    return name.rsplit("_", 1)[0] if name.endswith(("_L", "_R")) else name


def mat_name(obj):
    return obj.data.materials[0].name.split(".")[0] if obj.data.materials else "body_plain"


# ---------------------------------------------------------------------------------- build
def build_all():
    """The Ember base dragon plus every other Alpha 1 part variant, all tagged for export."""
    d = dm.build_dragon("ember")
    mats, arm = d["mats"], d["arm"]
    tagged = []  # (group, variant, sex, [objects])

    def tag(group, variant, sex, objs):
        tagged.append((group, variant, sex, list(objs)))

    tag("eyes", 0, SEX_ANY, d["groups"]["eyes"])
    tag("spikes", 0, SEX_ANY, d["groups"]["spikes"])
    tag("heart", 0, SEX_ANY, d["groups"]["heart"])

    def add_part(group, variant, sex, objs, bone):
        s = SEX_SCALE.get(group, {}).get(sex, 1.0)
        for o in objs:
            o.scale = Vector(o.scale) * s
            dm.parent_to_bone(o, arm, bone)
            d["groups"][group].append(o)
            if group in d["snap"]:
                d["snap"][group].append((o, [o]))
        tag(group, variant, sex, objs)

    # The base build already made swept horns, no frill and a spade tail for Ember, but not
    # sexed; rebuild every variant explicitly so each sex has its own baked mesh.
    for o in d["groups"]["horns"] + d["groups"]["tail_tip"]:
        bpy.data.objects.remove(o, do_unlink=True)
    d["groups"]["horns"].clear()
    d["groups"]["tail_tip"].clear()
    d["snap"]["horns"].clear()

    for sex in (SEX_MALE, SEX_FEMALE):
        for kind in ("swept", "nubs"):
            add_part("horns", HORNS[kind], sex, dm.build_horns(kind, mats), "head")
        for kind in ("fin", "feather"):
            add_part("frill", FRILL[kind], sex, dm.build_frill(kind, mats), "head")
        for kind in ("spade", "fan", "tuft"):
            add_part("tail_tip", TAIL[kind], sex, [dm.build_tail_tip(kind, mats)], "tail4")

    wings = {"membrane": d["wings"]}
    for kind in ("fin", "feathered"):
        objs = dm.build_wings(kind, mats)
        for o in objs:
            dm.bind(o, arm, lambda n: n.startswith("wing") or n == "chest")
        wings[kind] = objs
    return d, tagged, wings


# ---------------------------------------------------------------------------------- mesh data
def world_normal_matrix(m):
    return m.to_3x3().inverted_safe().transposed()


def mesh_arrays(objs, palette_of, matrix_of):
    """Merge objects into one vertex/index list. palette_of(obj, vertex) -> [(bone, w), ...]."""
    positions, normals, skin, paint, indices = [], [], [], [], []
    for o in objs:
        me = o.data
        me.calc_loop_triangles()
        m = matrix_of(o)
        nm = world_normal_matrix(m)
        pa, pb, emissive = MATERIAL_PAINT[mat_name(o)]
        mask = me.color_attributes.get("mask")
        base = len(positions)
        for v in me.vertices:
            positions.append(m @ v.co)
            normals.append((nm @ v.normal).normalized())
            skin.append(palette_of(o, v))
            mix = int(round(mask.data[v.index].color[1] * 255)) if (mask and pa != pb) else 0
            paint.append((pa, pb, mix, emissive))
        for tri in me.loop_triangles:
            indices += [base + tri.vertices[0], base + tri.vertices[1], base + tri.vertices[2]]
    return positions, normals, skin, paint, indices


def quantize_weights(pairs):
    """Top two (bone, weight) pairs -> (b0, b1, w0, w1) with w0 + w1 == 255."""
    pairs = sorted(pairs, key=lambda p: -p[1])[:2] or [(0, 1.0)]
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
        self.skin = self.paint = self.indices = None


def skinned_mesh(name, kind, group, variant, objs, bone_index, allowed):
    """Body / wings: deformed by up to two bones from the mesh's palette."""
    used = sorted({bone_index[vg.name] for o in objs for vg in o.vertex_groups if allowed(vg.name)})
    local = {b: i for i, b in enumerate(used)}

    def palette_of(o, v):
        names = {vg.index: vg.name for vg in o.vertex_groups}
        pairs = [(local[bone_index[names[g.group]]], g.weight) for g in v.groups
                 if g.weight > 0 and allowed(names[g.group])]
        return quantize_weights(pairs)

    pos, nrm, skin, paint, idx = mesh_arrays(objs, palette_of, lambda o: o.matrix_world.copy())
    m = Mesh(name, kind, group, variant, SEX_ANY, used)
    m.keys, m.key_ts = [(pos, nrm)], [1.0]
    m.skin, m.paint, m.indices = skin, paint, idx
    return m


def part_meshes(d, tagged, bone_index):
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
    for t in KEY_TS:
        dm.apply_t(d, t, "neutral")  # scales + surface snap for every part at once
        for m in meshes:
            pos, nrm, skin, paint, idx = mesh_arrays(
                m.objs, lambda o, v, m=m: (m.bone_of[o.name], m.bone_of[o.name], 255, 0),
                lambda o: o.matrix_basis.copy())
            m.keys.append((pos, nrm))
            m.key_ts.append(t)
            m.skin, m.paint, m.indices = skin, paint, idx
    return meshes


# ---------------------------------------------------------------------------------- writing
def write_ecm(path, d, meshes, order):
    arm = d["arm"]
    out = bytearray()
    out += b"ECM1" + struct.pack("<HH", 1, len(order))
    index = {n: i for i, n in enumerate(order)}
    for name in order:
        bone = arm.data.bones[name]
        parent = index[bone.parent.name] if bone.parent else -1
        flags = 1 if name.startswith("wing") else 0
        out += struct.pack("<16sbB2x", name.encode()[:15], parent, flags)
        ml = bone.matrix_local
        for r in range(3):
            out += struct.pack("<4f", *[ml[r][c] for c in range(4)])
    # growth tables
    wing_hatch = dm.HATCHLING["parts"]["wings"]
    for name in order:
        if name.startswith("wing"):
            out += struct.pack("<3f", wing_hatch, wing_hatch, wing_hatch)
        else:
            h = dm.HATCHLING["bones"].get(base_key(name), (1.0, 1.0))
            gx, l, gz = (h[0], h[1], h[0]) if len(h) == 2 else h
            out += struct.pack("<3f", gx, l, gz)
    for build in ("sturdy", "sleek", "long"):
        for name in order:
            out += struct.pack("<2f", *dm.BUILDS[build].get(base_key(name), (1.0, 1.0)))
    for name in order:
        out += struct.pack("<3f", *dm.BASE_POSE.get(name, (0.0, 0.0, 0.0)))
    for name in order:
        out += struct.pack("<f", dm.HATCH_POSE.get(name, 0.0))
    # meshes
    out += struct.pack("<H", len(meshes))
    for m in meshes:
        pal = list(m.palette) + [0] * (24 - len(m.palette))
        assert len(m.palette) <= 24, m.name
        n_verts = len(m.keys[0][0])
        assert n_verts < 65536 and len(m.indices) < 65536, m.name
        ts = list(m.key_ts) + [0.0] * (4 - len(m.key_ts))
        out += struct.pack("<16sBBBBB24sB3x4fHH", m.name.encode()[:15], m.kind, m.group, m.variant, m.sex,
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
        out += struct.pack(f"<{len(m.indices)}H", *m.indices)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(bytes(out))
    return len(out)


# ---------------------------------------------------------------------------------- reference
def write_reference(path, d, meshes, order, sources):
    """Poses + Blender-deformed positions of sampled vertices (armature space)."""
    arm = d["arm"]
    arm.location = (0, 0, 0)
    arm.rotation_euler = (0, 0, 0)
    rng = random.Random(7)
    cases = []
    for t, build, randomize in ((0.0, "neutral", False), (0.7, "long", True), (1.0, "sturdy", True)):
        pb = arm.pose.bones
        for name in order:  # clear the previous case
            pb[name].rotation_mode = "XYZ"
            pb[name].rotation_euler = (0, 0, 0)
        # Parts are seated on the surface in the rest pose with the neutral build, exactly as
        # the exported keys are; posing afterwards only moves them rigidly with their bone.
        dm.apply_t(d, t, "neutral")
        dm.rest_pose(d, t=t)
        if randomize:  # arbitrary rotations on every bone exercise the whole chain
            for name in order:
                b = pb[name]
                b.rotation_mode = "XYZ"
                b.rotation_euler = Euler([math.radians(rng.uniform(-35, 35)) for _ in range(3)])
        # The build then only changes bone scales — exactly what the runtime does.
        bone_scales, parts = dm.scales_for_t(t, build)
        for name, sc in bone_scales.items():
            pb[name].scale = sc
        for name in order:
            if name.startswith("wing"):
                pb[name].scale = (parts["wings"],) * 3
        bpy.context.view_layer.update()
        dg = bpy.context.evaluated_depsgraph_get()
        poses = []
        for name in order:
            b = pb[name]
            q = b.rotation_euler.to_quaternion() if b.rotation_mode == "XYZ" else b.rotation_quaternion
            poses.append((q.x, q.y, q.z, q.w, b.scale.x, b.scale.y, b.scale.z))
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
                samples.append((mi, vi, flat[vi]))
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


# ---------------------------------------------------------------------------------- main
def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    d, tagged, wings = build_all()
    arm = d["arm"]
    order = bone_order(arm)
    bone_index = {n: i for i, n in enumerate(order)}
    assert all(not n.startswith("wing") for n in order[:24]), "body bones must come first"

    meshes, sources = [], []
    body = skinned_mesh("body", KIND_BODY, GROUP["body"], 0, [d["body"]], bone_index,
                        lambda n: not n.startswith("wing"))
    meshes.append(body)
    sources.append([d["body"]])
    for kind, objs in wings.items():
        meshes.append(skinned_mesh(f"wings_{kind}", KIND_WINGS, GROUP["wings"], WINGS[kind], objs, bone_index,
                                   lambda n: n.startswith("wing") or n == "chest"))
        sources.append(objs)
    for m in part_meshes(d, tagged, bone_index):
        meshes.append(m)
        sources.append(m.objs)

    size = write_ecm(OUT, d, meshes, order)
    tris = {m.name: len(m.indices) // 3 for m in meshes}
    print(f"[export] {OUT} {size} bytes, {len(order)} bones, {len(meshes)} meshes")
    for m in meshes:
        print(f"[export]   {m.name:18s} kind {m.kind} verts {len(m.keys[0][0]):5d} tris {tris[m.name]:5d} "
              f"palette {len(m.palette):2d} keys {len(m.keys)}")
    if REFERENCE:
        n = write_reference(Path(REFERENCE), d, meshes, order, sources)
        print(f"[export] reference {REFERENCE}: {n} samples")


main()
