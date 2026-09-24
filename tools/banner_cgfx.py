"""The 3D banner's glTF to CGFX (Alpha 2 WP10, D50), with pycgfx plus one thing it doesn't do:
materials marked KHR_materials_unlit keep their texture's own colours. pycgfx lights every
material (texture x diffuse light + specular); for these, those two stages pass the colour
through instead. The wordmark needs it: lit, it came out dark on the 3DS from one side.

  py -3.12 tools/banner_cgfx.py [build/banner/banner.gltf] [build/banner/banner.cgfx]

pycgfx is a build tool, never committed (see tools/make_banner.ps1): build/tools/pycgfx.
"""
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
PYCGFX = os.path.join(ROOT, "build", "tools", "pycgfx")
sys.path.insert(0, PYCGFX)

import gltflib  # noqa: E402
import main as pycgfx  # noqa: E402  (pycgfx's main.py: convert_gltf, write)

UNLIT = "KHR_materials_unlit"
PREVIOUS, REPLACE = 0xFFF, 0  # a combiner stage that hands on the stage before it unchanged


def convert(src, dst):
    gltf = gltflib.GLTF.load(src, load_file_resources=True)
    cgfx = pycgfx.convert_gltf(gltf)
    model = cgfx.data.models["COMMON"]
    unlit = [m.name for m in gltf.model.materials or [] if m.extensions and UNLIT in m.extensions]
    for name in unlit:
        stages = model.materials[name].fragment_shader.texture_combiners
        for i in (1, 2):  # 1: x diffuse light, 2: + specular
            stages[i].src_rgb, stages[i].combine_rgb = PREVIOUS, REPLACE
    data = pycgfx.write(cgfx)
    with open(dst, "wb") as f:
        f.write(data)
    print(f"banner_cgfx: {dst} ({len(data) // 1024} KB), unlit: {', '.join(unlit) or 'none'}")


if __name__ == "__main__":
    args = sys.argv[1:]
    src = args[0] if args else os.path.join(ROOT, "build", "banner", "banner.gltf")
    dst = args[1] if len(args) > 1 else os.path.splitext(src)[0] + ".cgfx"
    convert(src, dst)
