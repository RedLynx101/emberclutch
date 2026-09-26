"""Banner labs: X plus one change from another banner, glTF to glTF (D85 (6), docs/tech/banner-labs.md).

X (the game's banner since run 12) holds on the HOME Menu; lab A (the Blazeplume within X's 16
materials and 32 pieces, runs 16-17) froze it. Converted by pycgfx they are the same model: the
same nodes and nesting, meshes per node, vertex layouts, animated channels and key times. What
differs falls into four parts, and each lab grafts one of them from the donor onto X:
  names     the donor's names: its material names with ".001" in them (X's horn, membrane and
            mouth take b_horn.001, b_membrane.001 and b_accent_flat.001, so X's longest string
            grows from 72 characters to lab A's 74), its node names (body, heart) and mesh names
  geometry  the donor's dragon: every mesh's vertices and triangles, moved from where the
            donor's node holds it into X's node, so it sits as in the donor; X's materials,
            names, nodes and animation stay (each donor piece takes X's material of the same slot)
  tail      the donor's tail swing: its rest rotation and rotation keys (lifted behind the left
            shoulder and swung, the wag on top, instead of X's wag about the vertical alone)
  paint     the donor's materials: its 14 in its order (names without the ".001", which is
            names' part), their colours, its skin and wordmark textures and the heart's glow
            colours; X's mouth, teeth and tongue take the membrane, accent and membrane colours
  none      X written back as it is (a check: it must convert byte for byte to X's CGFX)
Changes join with commas ("geometry,tail") for later rounds. The output converts with
tools/banner_cgfx.py exactly as X does (--turn "body*:1,egg:1").

  py -3.12 tools/banner_graft.py <change>[,<change>] [--base assets/banner3d/x/banner.gltf]
      [--donor assets/banner3d/lab-a/banner.gltf] [--out build/banner_labs/<change>]
"""
import copy
import json
import math
import os
import re
import shutil
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
CHANGES = ("names", "geometry", "tail", "paint", "none")
# The donor's three dotted material names; X's mouth (which the Blazeplume's banner leaves out)
# takes the longest, so the names' lengths come across too (after paint, its accent takes it).
MATERIAL_NAMES = {"b_horn": "b_horn.001", "b_membrane": "b_membrane.001", "b_mouth": "b_accent_flat.001",
                  "b_accent_flat": "b_accent_flat.001"}
# paint: X's materials onto the donor's (by name without ".001"); every donor material stays in use.
PAINT = {"b_mouth": "b_membrane", "b_tooth": "b_accent_flat", "b_tongue": "b_membrane"}
FMT = {5126: "f", 5121: "B", 5123: "H", 5125: "I", 5120: "b", 5122: "h"}
WIDTH = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}


def plain(name):
    """A name without Blender's ".001" suffix."""
    return re.sub(r"\.\d{3}$", "", name)


class Gltf:
    def __init__(self, path):
        self.path = os.path.abspath(path)
        self.dir = os.path.dirname(self.path)
        with open(self.path, encoding="utf-8") as f:
            self.g = json.load(f)
        self.bufs = [open(os.path.join(self.dir, b["uri"]), "rb").read() for b in self.g["buffers"]]

    def read(self, ai):
        a = self.g["accessors"][ai]
        bv = self.g["bufferViews"][a["bufferView"]]
        n, fmt = WIDTH[a["type"]], FMT[a["componentType"]]
        size = struct.calcsize(fmt)
        stride = bv.get("byteStride", size * n)
        at = bv.get("byteOffset", 0) + a.get("byteOffset", 0)
        b = self.bufs[bv["buffer"]]
        return [struct.unpack_from("<" + fmt * n, b, at + i * stride) for i in range(a["count"])]

    def node(self, name):
        hits = [i for i, n in enumerate(self.g["nodes"]) if plain(n["name"]) == plain(name)]
        if len(hits) != 1:
            sys.exit(f"banner_graft: node {name} matches {len(hits)} nodes in {self.path}")
        return hits[0]


# ------------------------------------------------------------------------------ matrices
def trs(node):
    """A node's rest transform as a 4x4 row-major matrix (glTF's T * R * S)."""
    x, y, z, w = node.get("rotation", (0, 0, 0, 1))
    sx, sy, sz = node.get("scale", (1, 1, 1))
    tx, ty, tz = node.get("translation", (0, 0, 0))
    r = [[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
         [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
         [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]]
    return [[r[0][0] * sx, r[0][1] * sy, r[0][2] * sz, tx], [r[1][0] * sx, r[1][1] * sy, r[1][2] * sz, ty],
            [r[2][0] * sx, r[2][1] * sy, r[2][2] * sz, tz], [0, 0, 0, 1]]


def mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]


def invert(m):
    """The inverse of an affine matrix (its 3x3 by cofactors)."""
    a = [row[:3] for row in m[:3]]
    det = (a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0])
           + a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]))
    inv = [[(a[(j + 1) % 3][(i + 1) % 3] * a[(j + 2) % 3][(i + 2) % 3] - a[(j + 1) % 3][(i + 2) % 3] * a[(j + 2) % 3][(i + 1) % 3]) / det
            for j in range(3)] for i in range(3)]
    t = [-sum(inv[i][k] * m[k][3] for k in range(3)) for i in range(3)]
    return [inv[0] + [t[0]], inv[1] + [t[1]], inv[2] + [t[2]], [0, 0, 0, 1]]


def world(gl, i):
    """Node i's rest transform in the scene (its parents' applied): frame 0 of the banner's loop,
    where every animated channel starts at the node's rest pose."""
    parent = {c: p for p, n in enumerate(gl.g["nodes"]) for c in n.get("children", [])}
    m = trs(gl.g["nodes"][i])
    while i in parent:
        i = parent[i]
        m = mul(trs(gl.g["nodes"][i]), m)
    return m


def same(a, b, eps=1e-6):
    return all(abs(a[i][j] - b[i][j]) < eps for i in range(4) for j in range(4))


# ------------------------------------------------------------------------------ writing
class Out:
    """The base glTF, changed; new data goes into graft.bin beside it."""

    def __init__(self, base):
        self.base = base
        self.g = copy.deepcopy(base.g)
        self.blob = bytearray()
        self.g["buffers"].append({"uri": "graft.bin", "byteLength": 0})
        self.buf = len(self.g["buffers"]) - 1

    def accessor(self, rows, ctype, kind, target=None):
        fmt = FMT[ctype]
        while len(self.blob) % 4:
            self.blob.append(0)
        at = len(self.blob)
        for r in rows:
            self.blob += struct.pack("<" + fmt * len(r), *r)
        view = {"buffer": self.buf, "byteOffset": at, "byteLength": len(self.blob) - at}
        if target:
            view["target"] = target
        self.g["bufferViews"].append(view)
        acc = {"bufferView": len(self.g["bufferViews"]) - 1, "componentType": ctype, "count": len(rows), "type": kind}
        if kind == "VEC3" and ctype == 5126:
            acc["min"] = [min(r[k] for r in rows) for k in range(3)]
            acc["max"] = [max(r[k] for r in rows) for k in range(3)]
        if kind == "SCALAR" and ctype == 5126:
            acc["min"], acc["max"] = [min(r[0] for r in rows)], [max(r[0] for r in rows)]
        self.g["accessors"].append(acc)
        return len(self.g["accessors"]) - 1

    def write(self, out_dir, images_from):
        os.makedirs(out_dir, exist_ok=True)
        if self.blob:
            self.g["buffers"][self.buf]["byteLength"] = len(self.blob)
            with open(os.path.join(out_dir, "graft.bin"), "wb") as f:
                f.write(self.blob)
        else:  # nothing new: X's own buffers only (the "none" check)
            del self.g["buffers"][self.buf]
        for b in self.base.g["buffers"]:
            shutil.copyfile(os.path.join(self.base.dir, b["uri"]), os.path.join(out_dir, b["uri"]))
        for im in self.g.get("images", []):
            shutil.copyfile(os.path.join(images_from, im["uri"]), os.path.join(out_dir, im["uri"]))
        path = os.path.join(out_dir, "banner.gltf")
        with open(path, "w", encoding="utf-8") as f:
            json.dump(self.g, f, indent=1)
        return path


# ------------------------------------------------------------------------------ the changes
def graft_names(out, donor):
    """The donor's node and mesh names by node, and its dotted material names."""
    g = out.g
    for n in g["nodes"]:
        d = donor.g["nodes"][donor.node(n["name"])]
        if "mesh" in n and "mesh" in d:
            g["meshes"][n["mesh"]]["name"] = donor.g["meshes"][d["mesh"]]["name"]
        for a in g.get("animations", []):  # Blender names each object's action after it
            if a.get("name") == n["name"]:
                a["name"] = d["name"]
        n["name"] = d["name"]
    renamed = []
    for m in g["materials"]:
        if m["name"] in MATERIAL_NAMES:
            renamed.append(f"{m['name']} -> {MATERIAL_NAMES[m['name']]}")
            m["name"] = MATERIAL_NAMES[m["name"]]
    return renamed + ["nodes and meshes as the donor's"]


def graft_geometry(out, donor):
    """Every mesh of the donor's in X's node of the same name, slot for slot (the same number of
    primitives and the same attributes in both), moved from the donor's node into X's so it sits
    where the donor has it. A mesh the same in both, where both nodes stand alike, is left as it is."""
    g, base = out.g, out.base
    done = []
    for i, n in enumerate(g["nodes"]):
        if "mesh" not in n:
            continue
        j = donor.node(n["name"])
        mine, theirs = g["meshes"][n["mesh"]], donor.g["meshes"][donor.g["nodes"][j]["mesh"]]
        if len(mine["primitives"]) != len(theirs["primitives"]):
            sys.exit(f"banner_graft: {n['name']} has {len(mine['primitives'])} primitives, the donor's {len(theirs['primitives'])}")
        move = mul(invert(world(base, i)), world(donor, j))
        still = same(move, [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]])
        if still and all(base.read(p["attributes"]["POSITION"]) == donor.read(q["attributes"]["POSITION"])
                         and base.read(p["indices"]) == donor.read(q["indices"])
                         for p, q in zip(mine["primitives"], theirs["primitives"])):
            continue
        rot = [row[:3] for row in move[:3]]
        for p, q in zip(mine["primitives"], theirs["primitives"]):
            if set(p["attributes"]) != set(q["attributes"]):
                sys.exit(f"banner_graft: {n['name']}: attributes {sorted(p['attributes'])} against {sorted(q['attributes'])}")
            pos = [tuple(sum(move[r][k] * v[k] for k in range(3)) + move[r][3] for r in range(3))
                   for v in donor.read(q["attributes"]["POSITION"])]
            nor = []
            for v in donor.read(q["attributes"]["NORMAL"]):
                w = [sum(rot[r][k] * v[k] for k in range(3)) for r in range(3)]
                length = math.sqrt(sum(c * c for c in w)) or 1.0
                nor.append(tuple(c / length for c in w))
            p["attributes"]["POSITION"] = out.accessor(pos, 5126, "VEC3", 34962)
            p["attributes"]["NORMAL"] = out.accessor(nor, 5126, "VEC3", 34962)
            if "TEXCOORD_0" in q["attributes"]:
                p["attributes"]["TEXCOORD_0"] = out.accessor(donor.read(q["attributes"]["TEXCOORD_0"]), 5126, "VEC2", 34962)
            ia = donor.g["accessors"][q["indices"]]
            p["indices"] = out.accessor(donor.read(q["indices"]), ia["componentType"], "SCALAR", 34963)
        done.append(f"{n['name']} ({len(mine['primitives'])})")
    return done


def channel(g, test):
    """The sampler of the one animation channel `test` picks out."""
    hits = [a["samplers"][c["sampler"]] for a in g.get("animations", []) for c in a["channels"] if test(c["target"])]
    if len(hits) != 1:
        sys.exit(f"banner_graft: {len(hits)} animation channels match where one should")
    return hits[0]


def pointer(target):
    return target.get("extensions", {}).get("KHR_animation_pointer", {}).get("pointer")


def graft_tail(out, donor):
    """The donor's tail: its rest rotation and its rotation keys (at X's key times)."""
    i, j = out.base.node("tail"), donor.node("tail")
    out.g["nodes"][i]["rotation"] = donor.g["nodes"][j]["rotation"]
    mine = channel(out.g, lambda t: t.get("node") == i and t["path"] == "rotation")
    theirs = channel(donor.g, lambda t: t.get("node") == j and t["path"] == "rotation")
    if out.base.read(mine["input"]) != donor.read(theirs["input"]):
        sys.exit("banner_graft: the tails' keys fall on other frames")
    mine["output"] = out.accessor(donor.read(theirs["output"]), 5126, "VEC4")
    return ["the rest rotation and rotation keys"]


def graft_paint(out, donor):
    """The donor's materials in its order (their names without ".001"), X's primitives onto them,
    its textures, and the heart's glow colours (the KHR_animation_pointer channel)."""
    g = out.g
    names = [plain(m["name"]) for m in donor.g["materials"]]
    remap = {}
    for k, m in enumerate(g["materials"]):
        to = PAINT.get(m["name"], plain(m["name"]))
        if to not in names:
            sys.exit(f"banner_graft: paint: X's {m['name']} has no donor material ({to})")
        remap[k] = names.index(to)
    for me in g["meshes"]:
        for p in me["primitives"]:
            p["material"] = remap[p["material"]]
    used = {p["material"] for me in g["meshes"] for p in me["primitives"]}
    if used != set(range(len(names))):
        sys.exit(f"banner_graft: paint: donor materials left unused: {[names[k] for k in set(range(len(names))) - used]}")
    g["materials"] = copy.deepcopy(donor.g["materials"])
    for m in g["materials"]:
        m["name"] = plain(m["name"])
    for key in ("images", "textures", "samplers"):
        g[key] = copy.deepcopy(donor.g.get(key, []))
    # The heart's glow: the pointer follows the heart's material to its place in the donor's list,
    # and takes the donor's colours.
    target = next(c["target"] for a in g["animations"] for c in a["channels"] if pointer(c["target"]))
    ptr = target["extensions"]["KHR_animation_pointer"]
    k = int(ptr["pointer"].split("/")[2])
    ptr["pointer"] = ptr["pointer"].replace(f"/materials/{k}/", f"/materials/{remap[k]}/")
    mine = channel(g, lambda t: pointer(t) == ptr["pointer"])
    theirs = channel(donor.g, lambda t: pointer(t) is not None)
    if out.base.read(mine["input"]) != donor.read(theirs["input"]):
        sys.exit("banner_graft: the heart's glow keys fall on other frames")
    mine["output"] = out.accessor(donor.read(theirs["output"]), 5126, "VEC4")
    return [f"{len(names)} materials", "the skin and wordmark textures", f"the heart's glow ({g['materials'][remap[k]]['name']})"]


def graft(changes, base_path, donor_path, out_dir):
    base, donor = Gltf(base_path), Gltf(donor_path)
    out = Out(base)
    report = []
    # Names last: the others find X's nodes and materials by their own names.
    for change in ("paint", "geometry", "tail", "names"):
        if change in changes:
            report.append(f"{change}: " + ", ".join(globals()[f"graft_{change}"](out, donor) or ["nothing"]))
    images_from = donor.dir if "paint" in changes else base.dir
    path = out.write(out_dir, images_from)
    print(f"banner_graft: {path} = X + {', '.join(changes)} from {os.path.relpath(donor_path, ROOT)}")
    for line in report:
        print(f"  {line}")
    return path


if __name__ == "__main__":
    args = sys.argv[1:]

    def opt(name, default):
        if name in args:
            i = args.index(name)
            v = args[i + 1]
            del args[i:i + 2]
            return v
        return default

    base = opt("--base", os.path.join(ROOT, "assets", "banner3d", "x", "banner.gltf"))
    donor = opt("--donor", os.path.join(ROOT, "assets", "banner3d", "lab-a", "banner.gltf"))
    out_dir = opt("--out", "")
    if len(args) != 1 or any(c not in CHANGES for c in args[0].split(",")):
        sys.exit(__doc__)
    changes = [c for c in args[0].split(",") if c != "none"]
    graft(changes, base, donor, out_dir or os.path.join(ROOT, "build", "banner_labs", args[0].replace(",", "+")))
