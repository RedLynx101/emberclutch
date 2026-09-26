"""Dark faces: every kind's baked skins, sampled per face as the 3DS samples them, for faces
that come out near-black where they shouldn't (plain Python; a Kindlemoss builder's idea, run 18).

  python tools/dragons/dark_faces.py pouncer [crestwing ...]      (or --all)  [--verbose]

The skin's alpha is the painted value that multiplies the body's colour (dragonkit/texture.py:
soft light x grain x occlusion, about 0.6 to 1 on skin). A texel the bake never reached is 0,
so a face whose UVs land on unbaked texels draws black. For each kind, form and LOD, the body
mesh's UVs and triangles are read from romfs/dragons/<kind>/<form>[_lod1].ecm and the skin
from its .t3x (RGBA8 with mipmaps); each skin face (every face not pointing at the clean
corner) is sampled bilinearly at points across its UV triangle, at mip level 0 (the close-up)
and levels 1 and 2 (the den, a little way off; the game filters trilinearly), and flagged when
its mean value falls below DARK (or a sample below SPOT at level 0). Flagged faces are grouped
into UV islands. Exit code 1 if any kind has one.
"""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))

DARK = 0.35        # a face's mean painted value below this reads as black (skin is ~0.6 to 1)
SPOT = 0.15        # any single sample this dark at full size is a black speck
CLEAN_FROM = 0.93  # dragonkit/texture.py: UVs past this in both axes are the clean corner
LEVELS = (0, 1, 2)
# Barycentric points across a triangle: the middle, toward each corner, and each edge's middle.
POINTS = [(1 / 3, 1 / 3, 1 / 3), (0.6, 0.2, 0.2), (0.2, 0.6, 0.2), (0.2, 0.2, 0.6),
          (0.45, 0.45, 0.1), (0.1, 0.45, 0.45), (0.45, 0.1, 0.45), (0.8, 0.1, 0.1), (0.1, 0.8, 0.1),
          (0.1, 0.1, 0.8)]


# ------------------------------------------------------------------------------ files
def read_body(path):
    """The body mesh of an .ecm (see check.read_ecm for the layout): (uvs, triangles)."""
    b = open(path, "rb").read()
    at = 0

    def take(fmt):
        nonlocal at
        v = struct.unpack_from(fmt, b, at)
        at += struct.calcsize(fmt)
        return v

    magic, version, nb = take("<4sHH")
    assert magic == b"ECM1" and version == 4, f"{path}: not an ECM v4"
    at += nb * (20 + 48) + nb * 12 + 3 * nb * 8 + nb * 12 + nb * 4
    (nm,) = take("<H")
    for _ in range(nm):
        name, kind, group, variant, sex, npal, pal, nkeys, t0, t1, t2, t3, nv, ni = take("<16sBBBBB32sB3x4fHH")
        at += nkeys * nv * 24 + nv * 8
        uv = take(f"<{nv * 2}f")
        at += nv
        idx = take(f"<{ni}H")
        (pieces,) = take("<B")
        if pieces:
            at += nv + nkeys * 3 * pieces * 12
        if kind == 0:
            uvs = [(uv[2 * i], uv[2 * i + 1]) for i in range(nv)]
            return uvs, [idx[k:k + 3] for k in range(0, ni, 3)]
    raise ValueError(f"{path}: no body mesh")


def _morton(i):
    """Position (x, y) of the i-th texel of an 8x8 3DS tile (Z order, x in the low bit)."""
    x = (i & 1) | ((i >> 1) & 2) | ((i >> 2) & 4)
    y = ((i >> 1) & 1) | ((i >> 2) & 2) | ((i >> 3) & 4)
    return x, y


MORTON = [_morton(i) for i in range(64)]


def read_t3x(path):
    """A tex3ds RGBA8 texture and its mipmaps: [level] -> (size, alpha rows), rows bottom-up
    (row 0 is v = 0, as the UVs have it), alpha 0..1. Only the alpha (the painted value) is
    kept; the colour channels are pattern masks, where 0 is fine."""
    b = open(path, "rb").read()
    count, dims, fmt, mips = struct.unpack_from("<HBBB", b, 0)
    assert fmt == 0, f"{path}: not RGBA8"
    w, h = 8 << (dims & 7), 8 << ((dims >> 3) & 7)
    at = 5 + 12 * count
    comp = b[at]
    size = int.from_bytes(b[at + 1:at + 4], "little")
    assert comp == 0, f"{path}: compressed (tex3ds -z none expected)"
    at += 4
    assert len(b) - at == size, f"{path}: {len(b) - at} bytes of data, header says {size}"
    levels = []
    for _ in range(mips + 1):
        rows = [[0.0] * w for _ in range(h)]
        tiles_x = w // 8
        for t in range(tiles_x * (h // 8)):
            ty, tx = divmod(t, tiles_x)
            base = at + t * 256
            for i, (x, y) in enumerate(MORTON):
                # Tile rows run from the top of the image down; a texel's bytes are A, B, G, R.
                rows[h - 1 - (ty * 8 + y)][tx * 8 + x] = b[base + 4 * i] / 255.0
        levels.append((w, rows))
        at += w * h * 4
        w, h = max(1, w // 2), max(1, h // 2)
    return levels


def sample(level, u, v):
    """Bilinear, clamped to the edge (the game's filter and wrap)."""
    size, rows = level
    x, y = u * size - 0.5, v * size - 0.5
    x0, y0 = int(x // 1), int(y // 1)
    fx, fy = x - x0, y - y0
    c = lambda k: min(size - 1, max(0, k))  # noqa: E731
    r0, r1 = rows[c(y0)], rows[c(y0 + 1)]
    top = r0[c(x0)] * (1 - fx) + r0[c(x0 + 1)] * fx
    bot = r1[c(x0)] * (1 - fx) + r1[c(x0 + 1)] * fx
    return top * (1 - fy) + bot * fy


# ------------------------------------------------------------------------------ the scan
def islands(faces, tris):
    """Group face indices by shared UV vertices (the exporter splits vertices at UV seams, so
    faces sharing a vertex index share a UV island)."""
    parent = {f: f for f in faces}

    def find(f):
        while parent[f] != f:
            parent[f] = parent[parent[f]]
            f = parent[f]
        return f

    owner = {}
    for f in faces:
        for vi in tris[f]:
            if vi in owner:
                parent[find(f)] = find(owner[vi])
            else:
                owner[vi] = f
    groups = {}
    for f in faces:
        groups.setdefault(find(f), []).append(f)
    return sorted(groups.values(), key=len, reverse=True)


def scan(ecm, t3x):
    """[(level, face, mean, lowest, uv centre)] for the dark skin faces of one form's LOD."""
    uvs, tris = read_body(ecm)
    levels = read_t3x(t3x)
    found = []
    for f, tri in enumerate(tris):
        a, b, c = (uvs[i] for i in tri)
        if min(a[0], b[0], c[0]) >= CLEAN_FROM and min(a[1], b[1], c[1]) >= CLEAN_FROM:
            continue  # not skin: the mouth, nostrils, the clean corner
        pts = [(a[0] * p + b[0] * q + c[0] * r, a[1] * p + b[1] * q + c[1] * r) for p, q, r in POINTS]
        for lv in LEVELS:
            if lv >= len(levels):
                continue
            vals = [sample(levels[lv], u, v) for u, v in pts]
            mean, low = sum(vals) / len(vals), min(vals)
            if mean < DARK or (lv == 0 and low < SPOT):
                found.append((lv, f, mean, low, pts[0]))
    return found, tris, len(tris)


def check_kind(name, verbose=False, quiet=False):
    """Scans a kind's four skins; prints a line per skin (and the dark islands); True if clean."""
    folder = os.path.join(ROOT, "romfs", "dragons", name)
    ok, lines = True, []
    for form in ("hatchling", "grown"):
        for lod in ("", "_lod1"):
            stem = form + lod
            ecm, t3x = os.path.join(folder, stem + ".ecm"), os.path.join(folder, stem + "_skin.t3x")
            if not (os.path.exists(ecm) and os.path.exists(t3x)):
                lines.append(f"  {stem:15s} missing .ecm or skin")
                ok = False
                continue
            found, tris, n = scan(ecm, t3x)
            by_level = {lv: [x for x in found if x[0] == lv] for lv in LEVELS}
            summary = ", ".join(f"mip {lv}: {len(v)}" for lv, v in by_level.items())
            lines.append(f"  {stem:15s} {n} faces; dark faces {summary}")
            if found:
                ok = False
                for lv, hits in by_level.items():
                    if not hits:
                        continue
                    groups = islands([h[1] for h in hits], tris)
                    info = {h[1]: h for h in hits}
                    for g in groups[:None if verbose else 4]:
                        worst = min(g, key=lambda f: info[f][2])
                        _, _, mean, low, (u, v) = info[worst]
                        lines.append(f"    mip {lv}: island of {len(g)} face(s) near uv ({u:.3f}, {v:.3f}), "
                                     f"value {mean:.2f} (lowest sample {low:.2f})")
                    if len(groups) > 4 and not verbose:
                        lines.append(f"    mip {lv}: ... {len(groups) - 4} more islands")
    if not quiet or not ok:
        print(f"[dark] {name}: {'OK' if ok else 'DARK FACES'}")
        for ln in lines:
            print(ln)
    return ok


def main():
    import dragons
    names = [m.META["name"] for m in dragons.all_kinds()] if "--all" in sys.argv else \
        [a for a in sys.argv[1:] if not a.startswith("--")]
    ok = all([check_kind(n, "--verbose" in sys.argv) for n in names])
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
