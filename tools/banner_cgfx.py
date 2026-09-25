"""The 3D banner's glTF to CGFX (Alpha 2 WP10, D50), with pycgfx plus what it doesn't do:
- materials marked KHR_materials_unlit keep their texture's own colours. pycgfx lights every
  material (texture x diffuse light + specular); for these, those two stages pass the colour
  through instead. The wordmark needs it: lit, it came out dark on the 3DS from one side.
- --billboard <names>: those nodes become Y-axis billboards (CGFX billboard mode 5, which glTF
  can't say), always facing the camera however the HOME Menu turns the scene (run 10; see
  tools/blender/banner3d.py). Names may end in * ("sparkle_*"). A billboard that's animated,
  or has animated children, froze the HOME Menu (run 10).
- --turn <node>:<turns>: that node's placeholder rotation becomes <turns> whole turns about
  the vertical over the loop, as two keys (pycgfx takes rotations through quaternions to Euler
  angles, which can't pass 90 degrees about the vertical smoothly). --turn egg:1 undoes the
  HOME Menu's own turn (tools/blender/banner3d.py --turn).
And it refuses to write a banner the HOME Menu is known to freeze on: any skinning, or bone
indices or weights in a vertex stream (gbatemp thread 683412).

  py -3.12 tools/banner_cgfx.py [build/banner/banner.gltf] [build/banner/banner.cgfx] [--billboard a,b*] [--turn egg:1]

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
from cgfx.sobj import BillboardMode  # noqa: E402

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
    turned = set_turn(cgfx, turn[0], turn[1]) if turn else "none"
    refuse_skinning(model)
    data = pycgfx.write(cgfx)
    with open(dst, "wb") as f:
        f.write(data)
    print(f"banner_cgfx: {dst} ({len(data) // 1024} KB), unlit: {', '.join(unlit) or 'none'}; "
          f"Y-axis billboards: {', '.join(made) or 'none'}; turning: {turned}")


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
        node, turns = args[i + 1].split(":")
        turn = (node, float(turns))
        del args[i:i + 2]
    src = args[0] if args else os.path.join(ROOT, "build", "banner", "banner.gltf")
    dst = args[1] if len(args) > 1 else os.path.splitext(src)[0] + ".cgfx"
    convert(src, dst, boards, turn)
