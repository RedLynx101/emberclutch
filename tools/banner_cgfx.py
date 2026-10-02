"""The 3D banner's glTF to CGFX (Alpha 2 WP10, D50), with pycgfx plus what it doesn't do:
- materials marked KHR_materials_unlit keep their texture's own colours. pycgfx lights every
  material (texture x diffuse light + specular); for these, those two stages pass the colour
  through instead. The wordmark needs it: lit, it came out dark on the 3DS from one side.
- --billboard <names>: those nodes become Y-axis billboards (CGFX billboard mode 5, which glTF
  can't say), always facing the camera however the HOME Menu turns the scene (run 10; see
  tools/blender/banner3d.py). Names may end in * ("sparkle_*"). A billboard that's animated,
  or has animated children, froze the HOME Menu (run 10).
- --turn <node>:<turns>[,...]: each node's placeholder rotation becomes <turns> whole turns
  about the vertical over the loop, as two keys (pycgfx takes rotations through quaternions to
  Euler angles, which can't pass 90 degrees about the vertical smoothly). Names may end in *.
  --turn "body*:1,egg:1" undoes the HOME Menu's own turn (tools/blender/banner3d.py --turn).
And it refuses to write a banner the HOME Menu is known to freeze on: any skinning, or bone
indices or weights in a vertex stream (gbatemp thread 683412).

  py -3.12 tools/banner_cgfx.py [build/banner/banner.gltf] [build/banner/banner.cgfx] [--billboard a,b*] [--turn "body*:1,egg:1"]

pycgfx is a build tool, never committed (see tools/make_banner.ps1): build/tools/pycgfx.
"""
import fnmatch
import math
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
PYCGFX = os.path.join(ROOT, "build", "tools", "pycgfx")
sys.path.insert(0, PYCGFX)

import gltflib  # noqa: E402
import main as pycgfx  # noqa: E402  (pycgfx's main.py: convert_gltf, write)
from cgfx.canm import FloatAnimationCurve, StepLinear64Key  # noqa: E402
from cgfx.primitives import VertexAttributeUsage  # noqa: E402
from cgfx.shared import StringTable  # noqa: E402
from cgfx.sobj import BillboardMode  # noqa: E402

# The IMAG section's blobs (the textures, the vertex streams) on 128-byte boundaries (run 27). pycgfx
# aligns them to 16; whether a banner froze the HOME Menu followed where its textures landed: every lab
# whose textures sat at 32 or 48 past a 64-byte boundary froze (25B, 25C, 26A, 27A, 27B), every one at
# 0 or 16 held (25A, 25D, 25E, 25F twice, 26B), whatever else changed. So a few vertices more or a
# renamed material could freeze a banner that held (run 20's "names + paint" too).
ALIGN = 128


class AlignedTable(StringTable):
    @staticmethod
    def correct(s):
        if isinstance(s, str):
            return s.encode() + b"\0"
        return s + b"\0" * (-len(s) % ALIGN)


def write_aligned(cgfx):
    """pycgfx's write(), with the IMAG content starting on an ALIGN boundary and each blob padded to it."""
    strings, imag = StringTable(), AlignedTable()
    offset = cgfx.prepare(0, strings, imag)
    offset = strings.prepare(offset)
    if not imag.empty():
        extra = -(offset + 8) % ALIGN  # (more padding at the strings' end: the IMAG content after its header)
        strings.padding += extra
        strings.total += extra
        offset += extra
    cgfx.data.section_size = offset - cgfx.data.offset
    if not imag.empty():
        cgfx.header.nr_blocks = 2
        offset += 8  # IMAG header
    offset = imag.prepare(offset)
    cgfx.header.file_size = offset
    data = cgfx.write(strings, imag)
    data += strings.write()
    if not imag.empty():
        data += b"IMAG" + imag.size().to_bytes(4, "little") + imag.write()
    bad = [o for o in (imag.offset + off for off in imag.table.values()) if o % ALIGN]
    if bad:
        sys.exit(f"banner_cgfx: {len(bad)} IMAG blobs off the {ALIGN}-byte boundary (first at {bad[0]})")
    textures = sorted(imag.offset + off for k, off in imag.table.items() if len(k) >= 16384)
    return data, textures

UNLIT = "KHR_materials_unlit"
PREVIOUS, REPLACE = 0xFFF, 0  # a combiner stage that hands on the stage before it unchanged


def refuse_skinning(model):
    for shape in model.shapes.data.contents:
        for ps in shape.primitive_sets.data.contents:
            if ps.skinning_mode != 0:
                sys.exit(f"banner_cgfx: {shape.name} is skinned (mode {ps.skinning_mode}): the HOME Menu freezes on it")
        for attr in shape.vertex_attributes.data.contents:
            streams = attr.vertex_streams.data.contents if hasattr(attr, "vertex_streams") else (attr,)
            for s in streams:
                if s.usage in (VertexAttributeUsage.BoneIndex, VertexAttributeUsage.BoneWeight):
                    sys.exit(f"banner_cgfx: {shape.name} carries bone indices or weights")


def set_turns(cgfx, spec):
    anim = cgfx.data.skeletal_animations["COMMON"]
    names = list(anim.member_animations_data) if anim is not None else []
    done = []
    for item in spec:
        pattern, turns = item.rsplit(":", 1)
        hits = [n for n in names if fnmatch.fnmatchcase(n, pattern)]
        if len(hits) != 1:
            sys.exit(f"banner_cgfx: --turn {pattern} matches {hits or 'no animated node'}")
        done.append(set_turn(cgfx, hits[0], float(turns)))
    return "; ".join(done)


def set_turn(cgfx, node, turns):
    anim = cgfx.data.skeletal_animations["COMMON"]
    member = anim.member_animations_data[node] if anim is not None else None
    if member is None or not isinstance(member.rot_y, FloatAnimationCurve):
        sys.exit(f"banner_cgfx: --turn {node}: it has no rotation animation to replace")
    seg = member.rot_y.segments[0]
    first, last = seg.keys[0].frame, seg.keys[-1].frame
    seg.keys = [StepLinear64Key(first, 0.0), StepLinear64Key(last, turns * 2 * math.pi)]
    for curve in (member.rot_x, member.rot_z):
        if isinstance(curve, FloatAnimationCurve):
            for k in curve.segments[0].keys:
                if abs(k.value) > 1e-4:
                    sys.exit(f"banner_cgfx: --turn {node}: it also turns about another axis")
    return f"{node} {turns:+g} turn over frames {first:g}-{last:g} ({(last - first) / 60:g} s)"


def convert(src, dst, billboards=(), turn=None):
    gltf = gltflib.GLTF.load(src, load_file_resources=True)
    cgfx = pycgfx.convert_gltf(gltf)
    model = cgfx.data.models["COMMON"]
    unlit = [m.name for m in gltf.model.materials or [] if m.extensions and UNLIT in m.extensions]
    for name in unlit:
        stages = model.materials[name].fragment_shader.texture_combiners
        for i in (1, 2):  # 1: x diffuse light, 2: + specular
            stages[i].src_rgb, stages[i].combine_rgb = PREVIOUS, REPLACE
    made = []
    for pattern in billboards:
        hits = [n for n in model.skeleton.bones if fnmatch.fnmatchcase(n, pattern)]
        if not hits:
            sys.exit(f"banner_cgfx: no node matches --billboard {pattern}")
        for n in hits:
            model.skeleton.bones[n].billboard_mode = BillboardMode.YAxial
            made.append(n)
    turned = set_turns(cgfx, turn) if turn else "none"
    refuse_skinning(model)
    data, textures = write_aligned(cgfx)
    if len(data) > 0x80000:
        sys.exit(f"banner_cgfx: {len(data)} bytes: the HOME Menu takes at most 512 KB")
    with open(dst, "wb") as f:
        f.write(data)
    print(f"banner_cgfx: {dst} ({len(data) // 1024} KB), unlit: {', '.join(unlit) or 'none'}; "
          f"Y-axis billboards: {', '.join(made) or 'none'}; turning: {turned}; "
          f"textures at {', '.join(str(t) for t in textures)} (all on {ALIGN}-byte boundaries)")


if __name__ == "__main__":
    args = sys.argv[1:]
    boards = []
    if "--billboard" in args:
        i = args.index("--billboard")
        boards = [p for p in args[i + 1].split(",") if p]
        del args[i:i + 2]
    turn = None
    if "--turn" in args:
        i = args.index("--turn")
        turn = [t for t in args[i + 1].split(",") if t]
        del args[i:i + 2]
    src = args[0] if args else os.path.join(ROOT, "build", "banner", "banner.gltf")
    dst = args[1] if len(args) > 1 else os.path.splitext(src)[0] + ".cgfx"
    convert(src, dst, boards, turn)
