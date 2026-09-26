"""What differs between two 3D banners as the HOME Menu gets them (docs/tech/banner-labs.md).

Converts each glTF as tools/banner_cgfx.py does (pycgfx, the unlit materials, the --turn) and
walks pycgfx's whole object tree: every field of every bone, material, mesh, shape, vertex
stream, texture, look-up table and animation curve. Prints the fields found in only one, and the
fields whose values differ, grouped by kind (a data blob by its size and hash). How labs B-E
were checked to be X plus exactly one change.
  py -3.12 tools/banner_diff.py <a.gltf> <b.gltf> [--turn "body*:1,egg:1"] [--rename "body.001=body,heart.001=heart"] [--all]
--rename matches nodes renamed between the two, so their fields are compared rather than listed
as missing; --all prints every differing field, not one example of each kind.
"""
import enum
import hashlib
import os
import re
import sys
from collections import OrderedDict

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(ROOT, "build", "tools", "pycgfx"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gltflib  # noqa: E402
import main as pycgfx  # noqa: E402
import banner_cgfx  # noqa: E402
from cgfx.dict import DICT, DictInfo  # noqa: E402

LINKS = ("parent", "child", "previous_sibling", "next_sibling")  # the skeleton's links, by name only
SKIP = ("cmdl", "offset", "struct")                            # back-references and write-time state


def convert(path, turn):
    gltf = gltflib.GLTF.load(path, load_file_resources=True)
    cgfx = pycgfx.convert_gltf(gltf)
    model = cgfx.data.models["COMMON"]
    for m in gltf.model.materials or []:
        if m.extensions and banner_cgfx.UNLIT in m.extensions:
            for i in (1, 2):
                stages = model.materials[m.name].fragment_shader.texture_combiners
                stages[i].src_rgb, stages[i].combine_rgb = banner_cgfx.PREVIOUS, banner_cgfx.REPLACE
    if turn:
        banner_cgfx.set_turns(cgfx, [t for t in turn.split(",") if t])
    return cgfx


def fields(cgfx, rename):
    """Every leaf of the object tree as path -> value (floats to 4 places, blobs hashed)."""
    out, seen = OrderedDict(), {}

    def name(s):
        for a, b in rename:
            s = s.replace(a, b)
        return s

    def walk(o, path, depth=0):
        if depth > 40:
            return
        if isinstance(o, enum.Enum) or o is None or isinstance(o, (bool, int, str)):
            out[path] = name(o) if isinstance(o, str) else o
        elif isinstance(o, float):
            out[path] = round(o, 4)
        elif isinstance(o, (bytes, bytearray)):
            out[path] = f"<{len(o)} bytes {hashlib.md5(o).hexdigest()[:8]}>"
        elif isinstance(o, (list, tuple)):
            out[path] = f"[{len(o)}]"
            for i, v in enumerate(o):
                walk(v, f"{path}[{i}]", depth + 1)
        elif isinstance(o, (DictInfo, DICT)):
            d = o.dict if isinstance(o, DictInfo) else o
            out[path] = f"DICT[{d.len()}]"
            for n in d.nodes[1:]:
                walk(n.content, f"{path}{{{name(n.name)}}}", depth + 1)
        elif id(o) in seen:
            out[path] = f"-> {seen[id(o)]}"
        elif hasattr(o, "__dict__"):
            seen[id(o)] = path
            for k, v in vars(o).items():
                if k in LINKS:
                    out[f"{path}.{k}"] = name(getattr(v, "name", "")) if v is not None else None
                elif k not in SKIP:
                    walk(v, f"{path}.{k}", depth + 1)
        else:
            out[path] = repr(o)

    walk(cgfx.data, "")
    return out


def kind(path):
    """A field's kind: its path with names and list positions taken out."""
    return re.sub(r"\[\d+\]", "[]", re.sub(r"\{[^}]*\}", "{}", path))


def main():
    args = sys.argv[1:]

    def opt(flag, default):
        if flag in args:
            i = args.index(flag)
            v = args[i + 1]
            del args[i:i + 2]
            return v
        return default

    turn = opt("--turn", "body*:1,egg:1")
    rename = [tuple(p.split("=", 1)) for p in opt("--rename", "").split(",") if "=" in p]
    every = "--all" in args
    args = [a for a in args if a != "--all"]
    if len(args) != 2:
        sys.exit(__doc__)
    a, b = (fields(convert(p, turn), rename) for p in args)
    only_a = [p for p in a if p not in b]
    only_b = [p for p in b if p not in a]
    diff = [p for p in a if p in b and a[p] != b[p]]
    print(f"banner_diff: {len(a)} fields and {len(b)}; {len(only_a)} only in the first, {len(only_b)} only in the second, "
          f"{len(diff)} differ")
    def prefixes(p):  # the path cut after each dictionary entry or list item in it
        return [p[:m.end()] for m in re.finditer(r"\{[^}]*\}|\[\d+\]", p)]

    for label, paths, other in (("only in the first", only_a, b), ("only in the second", only_b, a)):
        there = {q for p in other for q in prefixes(p)}
        heads = OrderedDict()
        for p in paths:  # by the first object along the path the other hasn't got (a material's
            h = next((q for q in prefixes(p) if q not in there), p)  # animation members as one)
            h = re.sub(r'\{Materials\["([^"]*)"\][^}]*\}$', r'{Materials["\1"]...}', h)
            heads[h] = heads.get(h, 0) + 1
        for h, n in heads.items():
            print(f"  {label}: {h} ({n} fields)")
    groups = OrderedDict()
    for p in diff:
        groups.setdefault(kind(p), []).append(p)
    for k, ps in groups.items():
        print(f"  differ x{len(ps)}: {k}")
        for p in (ps if every else ps[:1]):
            print(f"      {p}: {a[p]!r} -> {b[p]!r}")


if __name__ == "__main__":
    main()
