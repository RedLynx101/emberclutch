"""Skyreach Valley's placeholder landscape for Beta's technical test (WP1): a height field
about 1 km across with its places, written as romfs/valley/skyreach.evl for src/core/valley.

  python tools/valley/make_valley.py [--out romfs/valley/skyreach.evl] [--preview <png>]

Plain Python (no numpy). The shape follows the Beta plan (docs/plan/beta.md): a bowl ringed by
mountains, the cold heights in the north, a river from them into the lake, the den in a cliff
on the west side with a waterfall off its plateau, the Nesting Stone on a hilltop, the Market
village and the arena on flattened ground, the Sanctuary's meadow, a trailhead in the south,
floating islands. The lighting is baked into the vertex colours (the terrain program has
none); trees are scattered by rules. WP3 replaces all of this with the real landscape.

File (little-endian): "EVL1", u16 version 1, u16 n, f32 spacing, f32 x0, f32 y0, f32 hmin,
f32 hmax, f32 water, f32 reserved; n*n u16 heights (row j = y, column i = x); n*n RGB888 colours; u16 trees
(f32 x, f32 y, u8 height m, u8 shade); u16 islands (f32 x, y, z, radius); u16 places (u8 id,
f32 x, y, z, heading).
"""
import math
import os
import random
import struct
import sys
import zlib

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
N = 257            # samples per side: 16 tiles of 16 quads
SPACING = 4.0      # metres per sample: 1024 m across
X0 = Y0 = -512.0
WATER = 8.0
SUN = (-0.45, -0.5, 0.74)  # towards the sun (the south-east, high)

PLACE_DEN, PLACE_MARKET, PLACE_STONE, PLACE_SANCTUARY, PLACE_VAULT, PLACE_TRAILHEAD, PLACE_ARENA, PLACE_LAKE = range(8)


# ------------------------------------------------------------------------------ noise
def _hash(ix, iy, seed):
    h = (ix * 374761393 + iy * 668265263 + seed * 1442695041) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    return ((h ^ (h >> 16)) & 0xFFFF) / 65535.0


def _smooth(t):
    return t * t * (3 - 2 * t)


def noise(x, y, seed=0):
    ix, iy = math.floor(x), math.floor(y)
    fx, fy = _smooth(x - ix), _smooth(y - iy)
    a, b = _hash(ix, iy, seed), _hash(ix + 1, iy, seed)
    c, d = _hash(ix, iy + 1, seed), _hash(ix + 1, iy + 1, seed)
    return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy


def fbm(x, y, octaves=5, seed=0):
    s, amp, norm = 0.0, 1.0, 0.0
    for o in range(octaves):
        s += amp * noise(x, y, seed + o * 17)
        norm += amp
        x, y, amp = x * 2.03, y * 2.03, amp * 0.5
    return s / norm


def smoothstep(a, b, x):
    t = max(0.0, min(1.0, (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


def seg_dist(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy)))
    return math.hypot(px - (ax + dx * t), py - (ay + dy * t))


def poly_dist(px, py, pts):
    return min(seg_dist(px, py, *pts[k], *pts[k + 1]) for k in range(len(pts) - 1))


# ------------------------------------------------------------------------------ the shape
RIVER = [(20, 500), (12, 390), (-40, 285), (-62, 185), (-22, 85), (28, -20), (44, -95)]
OUTLET = [(-20, -210), (-110, -300), (-190, -400), (-230, -520)]
STREAM = [(-470, 120), (-380, 118), (-306, 120)]  # on the den's plateau, to the waterfall
LAKE = (60.0, -150.0, 120.0, 95.0)                # centre, radii
FLATS = {  # place: (x, y, radius, height)
    PLACE_MARKET: (130.0, -10.0, 80.0, 13.0),
    PLACE_ARENA: (270.0, -230.0, 55.0, 11.0),
    PLACE_SANCTUARY: (-160.0, -250.0, 110.0, 12.0),
    PLACE_TRAILHEAD: (-60.0, -390.0, 40.0, 16.0),
}
STONE = (230.0, 170.0)
CLIFF_X = -300.0      # the den's cliff face, looking east
PLATEAU = 62.0


def height(x, y):
    r = math.hypot(x, y) / 512.0
    h = 11.0 + 9.0 * (fbm(x / 170.0, y / 170.0, 5, 1) - 0.5) * 2
    # The ring of mountains, and the cold heights in the north.
    h += 200.0 * smoothstep(0.64, 1.05, r) * (0.65 + 0.6 * fbm(x / 110.0 + 7, y / 110.0 + 3, 5, 2))
    h += 120.0 * math.exp(-(((x - 10) / 230.0) ** 2 + ((y - 380) / 130.0) ** 2))
    # The den's plateau with its sheer cliff (a few metres of slope at the face).
    band = smoothstep(-160.0, -90.0, y) * (1.0 - smoothstep(210.0, 290.0, y))
    plateau = PLATEAU + 6.0 * (fbm(x / 60.0, y / 60.0, 3, 5) - 0.5)
    step = 1.0 / (1.0 + math.exp((x - CLIFF_X) / 3.5))
    h = h + (max(h, plateau) - h) * step * band
    # The Nesting Stone's hill, flat on top.
    d = math.hypot(x - STONE[0], y - STONE[1])
    h = max(h, min(52.0, 12.0 + 48.0 * math.exp(-(d / 75.0) ** 2)))
    # Flattened ground for the village, the arena, the meadow and the trailhead.
    for (fx, fy, fr, fh) in FLATS.values():
        w = 1.0 - smoothstep(fr * 0.6, fr, math.hypot(x - fx, y - fy))
        h = h + (fh + 1.5 * (fbm(x / 40.0, y / 40.0, 2, 9) - 0.5) - h) * w
    # The river, its outlet and the stream on the plateau, cut in.
    dr = min(poly_dist(x, y, RIVER), poly_dist(x, y, OUTLET))
    h -= 7.5 * math.exp(-(dr / 13.0) ** 2) + 3.0 * math.exp(-(dr / 55.0) ** 2)
    ds = poly_dist(x, y, STREAM)
    h -= 3.5 * math.exp(-(ds / 7.0) ** 2) * (1.0 if x < CLIFF_X + 4 else 0.0)
    # The lake's bowl.
    lx, ly, ra, rb = LAKE
    e = math.hypot((x - lx) / ra, (y - ly) / rb)
    if e < 1.25:
        bowl = WATER - 7.0 * (1.0 - min(1.0, e) ** 2) - 0.5
        h = h + (min(h, bowl) - h) * (1.0 - smoothstep(0.85, 1.25, e))
    return h


def region_colour(x, y, h, slope, n):
    """Base colour (before light) by what the ground is."""
    g = fbm(x / 35.0, y / 35.0, 3, 21)
    grass = (int(86 + 42 * g), int(140 + 36 * g), int(64 + 20 * g))
    if math.hypot(x - FLATS[PLACE_SANCTUARY][0], y - FLATS[PLACE_SANCTUARY][1]) < 100:
        grass = (int(120 + 40 * g), int(172 + 20 * g), int(92 + 30 * g))  # the meadow, lighter
    c = grass
    if h > 165 and slope < 0.95:
        c = (236, 240, 246)  # snow
    elif slope > 0.72:
        k = fbm(x / 20.0, y / 20.0, 2, 31)
        c = (int(116 + 30 * k), int(108 + 26 * k), int(104 + 24 * k))  # rock
    elif h < WATER + 1.6:
        c = (206, 190, 142) if h > WATER - 0.4 else (86, 118, 108)  # sand, and under the water
    for pid in (PLACE_MARKET, PLACE_ARENA, PLACE_TRAILHEAD):  # trodden ground at the places
        fx, fy, fr, _ = FLATS[pid]
        w = 1.0 - smoothstep(fr * 0.25, fr * 0.7, math.hypot(x - fx, y - fy))
        c = tuple(int(c[k] + ((168, 146, 108)[k] - c[k]) * w * 0.8) for k in range(3))
    return c


def build():
    hs = [[height(X0 + i * SPACING, Y0 + j * SPACING) for i in range(N)] for j in range(N)]
    cols = [[None] * N for _ in range(N)]
    sl = math.sqrt(sum(v * v for v in SUN))
    sun = tuple(v / sl for v in SUN)
    for j in range(N):
        for i in range(N):
            x, y, h = X0 + i * SPACING, Y0 + j * SPACING, hs[j][i]
            dx = (hs[j][min(i + 1, N - 1)] - hs[j][max(i - 1, 0)]) / (2 * SPACING)
            dy = (hs[min(j + 1, N - 1)][i] - hs[max(j - 1, 0)][i]) / (2 * SPACING)
            nl = math.sqrt(dx * dx + dy * dy + 1)
            nrm = (-dx / nl, -dy / nl, 1 / nl)
            slope = math.hypot(dx, dy)
            base = region_colour(x, y, h, slope, nrm)
            lit = 0.58 + 0.62 * max(0.0, sum(nrm[k] * sun[k] for k in range(3)))
            cols[j][i] = tuple(max(0, min(255, int(v * lit))) for v in base)
    return hs, cols


def scatter_trees(hs):
    rng = random.Random(7)
    trees = []
    for j in range(0, 1024, 9):
        for i in range(0, 1024, 9):
            x, y = X0 + i + rng.uniform(0, 9), Y0 + j + rng.uniform(0, 9)
            if fbm(x / 90.0, y / 90.0, 3, 41) < 0.54:
                continue  # forest patches, open ground between
            gi, gj = int((x - X0) / SPACING), int((y - Y0) / SPACING)
            if not (1 <= gi < N - 1 and 1 <= gj < N - 1):
                continue
            h = hs[gj][gi]
            slope = math.hypot(hs[gj][gi + 1] - hs[gj][gi - 1], hs[gj + 1][gi] - hs[gj - 1][gi]) / (2 * SPACING)
            if h < WATER + 2.0 or h > 105 or slope > 0.45:
                continue
            if any(math.hypot(x - fx, y - fy) < fr for (fx, fy, fr, _) in FLATS.values()):
                continue
            if math.hypot(x - STONE[0], y - STONE[1]) < 40 or abs(x - CLIFF_X) < 12:
                continue
            if poly_dist(x, y, RIVER) < 16 or poly_dist(x, y, OUTLET) < 16:
                continue
            trees.append((x, y, int(rng.uniform(6, 12)), rng.randrange(256)))
    return trees


ISLANDS = [(60.0, -150.0, 140.0, 26.0), (330.0, 60.0, 170.0, 20.0), (-140.0, 120.0, 190.0, 30.0),
           (200.0, 330.0, 215.0, 22.0)]


def places(hs):
    def ground(x, y):
        i, j = int(round((x - X0) / SPACING)), int(round((y - Y0) / SPACING))
        return hs[max(0, min(N - 1, j))][max(0, min(N - 1, i))]
    out = [(PLACE_DEN, CLIFF_X + 1.0, 60.0, ground(CLIFF_X + 14.0, 60.0), math.pi / 2)]  # the cave mouth, facing east
    for pid in (PLACE_MARKET, PLACE_ARENA, PLACE_SANCTUARY, PLACE_TRAILHEAD):
        fx, fy, _, _ = FLATS[pid]
        out.append((pid, fx, fy, ground(fx, fy), 0.0))
    out.append((PLACE_STONE, STONE[0], STONE[1], ground(*STONE), 0.0))
    out.append((PLACE_VAULT, 40.0, 318.0, ground(40.0, 318.0), 0.0))
    out.append((PLACE_LAKE, LAKE[0], LAKE[1], WATER, 0.0))
    return out


def write(path, hs, cols, trees, plc):
    flat = [h for row in hs for h in row]
    hmin, hmax = min(flat), max(flat)
    out = bytearray(b"EVL1") + struct.pack("<HHfffffff", 1, N, SPACING, X0, Y0, hmin, hmax, WATER, 0.0)
    out += struct.pack(f"<{N * N}H", *[int(round((h - hmin) / (hmax - hmin) * 65535)) for h in flat])
    out += bytes(v for row in cols for c in row for v in c)
    out += struct.pack("<H", len(trees))
    for x, y, s, sh in trees:
        out += struct.pack("<ffBB", x, y, s, sh)
    out += struct.pack("<H", len(ISLANDS))
    for isl in ISLANDS:
        out += struct.pack("<ffff", *isl)
    out += struct.pack("<H", len(plc))
    for pid, x, y, z, hd in plc:
        out += struct.pack("<Bffff", pid, x, y, z, hd)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, "wb").write(out)
    return len(out), hmin, hmax


def write_png(path, cols, trees, plc):
    """A top view for review (north up), trees as dark dots, places as white squares."""
    img = [[list(c) for c in row] for row in cols]
    for x, y, _, _ in trees:
        i, j = int((x - X0) / SPACING), int((y - Y0) / SPACING)
        if 0 <= i < N and 0 <= j < N:
            img[j][i] = [40, 80, 40]
    for _, x, y, _, _ in plc:
        ci, cj = int((x - X0) / SPACING), int((y - Y0) / SPACING)
        for dj in range(-2, 3):
            for di in range(-2, 3):
                if 0 <= ci + di < N and 0 <= cj + dj < N:
                    img[cj + dj][ci + di] = [255, 255, 255]
    raw = bytearray()
    for j in range(N - 1, -1, -1):  # north up
        raw.append(0)
        for i in range(N):
            raw += bytes(img[j][i])

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", N, N, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, "wb").write(png)


def main():
    argv = sys.argv[1:]
    out = argv[argv.index("--out") + 1] if "--out" in argv else os.path.join(ROOT, "romfs", "valley", "skyreach.evl")
    hs, cols = build()
    trees = scatter_trees(hs)
    plc = places(hs)
    size, hmin, hmax = write(out, hs, cols, trees, plc)
    print(f"[valley] {out}: {size} bytes, heights {hmin:.1f}..{hmax:.1f} m, {len(trees)} trees, "
          f"{len(ISLANDS)} islands, {len(plc)} places")
    if "--preview" in argv:
        write_png(argv[argv.index("--preview") + 1], cols, trees, plc)


if __name__ == "__main__":
    main()
