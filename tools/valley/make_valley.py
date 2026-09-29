"""Skyreach Valley (Beta WP3, D73-D86): the landscape as a height field with its props, islands
and places, written as romfs/valley/skyreach.evl for src/core/valley.

  python tools/valley/make_valley.py [--out romfs/valley/skyreach.evl] [--preview <png>]

About 2.3 km across (five times the test valley's ground, D81), spaced between a cozy
life-sim's town and a 3DS-era adventure's field (D86): something always in sight, room to run,
glide and fly. Plain Python (no numpy). The shape:
  * a bowl ringed by mountains, the cold heights (snow) in the north, a crag in the east;
  * the den in the west: a plateau with a sheer cliff facing east, the cave mouth in it, a
    stream running off the plateau as a waterfall into a pool, the keeper's lodge beside the
    pool and a grotto behind the falls;
  * a river from the heights down the middle, under the windmill bridge, into Mirror Lake, and
    out through the south-west;
  * the Market village and its orchard east of the river, the arena in the south-east, the
    Nesting Stone on a hill in the north-east, the Sanctuary's meadow in the south-west, the
    Wanderers' trailhead at the valley's south gap, the Cold Vault on a snowy shelf in the
    north, Starwatch Ruins on the crag, floating isles over the middle;
  * earth paths between the places, a cobbled square at the village.
The light is baked into the vertex colours (a sun from the south-east, high); the colours are
a storybook's (soft greens, warm paths, lilac rock). Props are placed by rules: round trees in
the lowlands, pines up high, fruit trees in rows in the orchard, bushes by the paths, rocks on
slopes and banks, flowers in the meadows, reeds at the water.

File (little-endian): "EVL2", u16 version 2, u16 n, f32 spacing, f32 x0, f32 y0, f32 hmin,
f32 hmax, f32 water, f32 reserved; n*n u16 heights (row j = y, column i = x); n*n RGB888
colours; u16 props (f32 x, f32 y, u8 kind, u8 size in tenths of a metre, u8 shade, u8 yaw in
256ths of a turn); u16 islands (f32 x, y, z, radius); u16 places (u8 id, f32 x, y, z, heading);
u16 paths (u16 points, then f32 x, y each: the earth paths, for the map and for walking).
"""
import json
import math
import os
import random
import struct
import sys
import zlib

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
TILES = 36
N = TILES * 16 + 1   # 577 samples a side: 36 tiles of 16 quads
SPACING = 4.0        # metres a sample: 2304 m across
X0 = Y0 = -N // 2 * SPACING
WATER = 8.0
SUN = (-0.45, -0.5, 0.74)  # towards the sun (the south-east, high)
HALF = (N - 1) * SPACING / 2

(P_DEN, P_MARKET, P_STONE, P_SANCTUARY, P_VAULT, P_TRAILHEAD, P_ARENA, P_LAKE, P_KEEPER, P_ISLES, P_ORCHARD,
 P_MILL, P_GROTTO, P_RUINS, P_CALDERA, P_GLADE, P_COVE, P_HOLLOW) = range(18)
# Prop kinds (core/valley ValleyPropKind).
K_TREE, K_PINE, K_FRUIT, K_BUSH, K_ROCK, K_FLOWERS, K_REEDS = range(7)


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


def poly_near(px, py, pts, pad):
    """poly_dist, but only if the point is inside the polyline's box grown by pad (else pad+)."""
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    if px < min(xs) - pad or px > max(xs) + pad or py < min(ys) - pad or py > max(ys) + pad:
        return pad + 1.0
    return poly_dist(px, py, pts)


# ------------------------------------------------------------------------------ the shape
CLIFF_X = -620.0              # the den's cliff face, looking east
PLATEAU = 78.0
DEN = (CLIFF_X, 60.0)
FALLS_Y = 190.0               # the waterfall off the plateau
POOL = (CLIFF_X + 34.0, FALLS_Y, 26.0)
RIVER = [(60, 1150), (40, 860), (-60, 610), (-120, 380), (-60, 160), (-5, 60), (70, -70), (220, -210)]
OUTLET = [(250, -440), (120, -600), (-40, -760), (-190, -960), (-300, -1150)]
STREAM = [(CLIFF_X - 180, FALLS_Y - 10), (CLIFF_X - 60, FALLS_Y), (CLIFF_X + 2, FALLS_Y)]
BROOK = [(POOL[0], POOL[1]), (-470, 175), (-300, 160), (-150, 165), (-80, 160)]  # the pool's brook to the river
LAKE = (300.0, -330.0, 190.0, 140.0)                # centre, radii
HEIGHTS = (100.0, 860.0)                            # the cold heights' middle
CRAG = (960.0, 700.0)
STONE_HILL = (600.0, 520.0)

# The windmill's bridge crosses the river where the path does: the river runs along its local Y.
MILL_HEADING = math.atan2(0.49, 0.87)
MILL_WATER = 1.3        # the mill's anchor above the water (places.json water_z)
GROTTO_FLOOR = WATER + 1.0
# 1.0 (D90): Emberpeak Caldera, a volcano in the east whose crater floor is the battle league's
# stage, open west through a gap in its rim toward the arena; Frostspire Hollow, a bowl ringed
# with ice cut into the cold heights, open east toward the Vault's trail.
CALDERA = (900.0, -120.0)
CALDERA_FLOOR = 60.0
CALDERA_GAP = math.atan2(-210.0, -240.0)   # the rim's gap, toward the arena
HOLLOW = (-20.0, 800.0)
HOLLOW_FLOOR = 150.0
COVE_GROUND = WATER + 1.6
# place: (x, y, flat radius, ground height or None: its own ground before flattening), heading
PLACES = {
    P_DEN: (DEN[0] + 4.0, DEN[1], 0.0, None, math.pi / 2),        # in a notch in the cliff (den_notch)
    P_MARKET: (330.0, 120.0, 85.0, 14.0, 0.0),
    P_STONE: (STONE_HILL[0], STONE_HILL[1], 30.0, 72.0, math.pi),
    P_SANCTUARY: (-420.0, -470.0, 110.0, 16.0, 0.3),
    P_VAULT: (230.0, 720.0, 30.0, 152.0, 0.0),                     # its mouth toward its path (run 19)
    P_TRAILHEAD: (60.0, -880.0, 35.0, 22.0, math.pi),
    P_ARENA: (660.0, -330.0, 62.0, 15.0, -0.6),
    P_LAKE: (300.0, -178.0, 0.0, None, math.pi),
    P_KEEPER: (CLIFF_X + 70.0, FALLS_Y + 52.0, 22.0, 12.5, 2.4),
    P_ISLES: (120.0, 200.0, 0.0, None, 0.0),
    P_ORCHARD: (560.0, 40.0, 50.0, 17.0, -1.2),
    P_MILL: (-6.0, 60.0, 0.0, None, MILL_HEADING),               # its bridge across the river (mill_banks)
    P_GROTTO: (CLIFF_X - 6.0, FALLS_Y, 0.0, None, -math.pi / 2),  # in the cliff behind the falls, open east
    P_RUINS: (CRAG[0], CRAG[1], 16.0, None, math.pi * 1.2),      # flattened at the crag's own top
    # 1.0: the caldera's crater (caldera()), front toward the gap; the glade in the west woods; the
    # cove's beach on the lake's south shore, facing the water; the hollow's bowl (hollow()), open east.
    P_CALDERA: (CALDERA[0], CALDERA[1], 0.0, None, math.atan2(math.cos(CALDERA_GAP), -math.sin(CALDERA_GAP))),
    P_GLADE: (-330.0, 430.0, 46.0, None, math.atan2(-0.768, 0.640)),
    P_COVE: (320.0, -508.0, 26.0, COVE_GROUND, math.pi),
    P_HOLLOW: (HOLLOW[0], HOLLOW[1], 0.0, None, math.pi / 2),
}
ISLANDS = [(120.0, 200.0, 190.0, 55.0), (300.0, -330.0, 150.0, 40.0), (-250.0, 480.0, 172.0, 35.0),
           (480.0, 300.0, 212.0, 30.0), (-150.0, -250.0, 160.0, 28.0), (700.0, 120.0, 185.0, 26.0)]
PATHS = [
    # (over the mill's bridge along its deck: its ends at 9 m either side of its middle)
    [(DEN[0] + 14, DEN[1]), (-470, 70), (-300, 62), (-120, 60), (-24, 50), (-13.8, 55.6), (1.8, 64.4), (12, 70), (130, 80),
     (260, 110), (330, 120)],
    [(330, 120), (440, 80), (560, 40)],                                  # the orchard
    [(330, 120), (400, -40), (520, -200), (620, -300), (660, -330)],     # the arena
    [(330, 120), (430, 280), (520, 420), (575, 490), (600, 520)],        # up the hill to the Stone
    [(330, 120), (320, 260), (280, 420), (190, 540), (268, 582), (172, 622), (262, 656), (190, 690), (230, 720)],  # up to the Vault, winding up its shoulder
    [(-24, 50), (-140, -130), (-300, -330), (-420, -470)],               # the meadow
    [(-420, -470), (-260, -640), (-80, -790), (60, -880)],               # the trailhead
    [(330, 120), (320, -40), (300, -178)],                               # the lake's jetty
    [(DEN[0] + 14, DEN[1]), (-560, 110), (-522, 165), (-524, 205), (CLIFF_X + 70, FALLS_Y + 52)],  # the lodge, round the pool
    [(-300, -330), (-160, -520), (60, -600), (200, -562), (300, -522)],  # round the lake's south to the cove
    # 1.0: up the caldera's flank from the arena, through the gap in its rim
    [(660, -330), (727, -272), (787, -219), (840, -173), (CALDERA[0] - 22, CALDERA[1] - 19)],
    [(CLIFF_X + 70, FALLS_Y + 52), (-470, 318), (-400, 380), (-352, 412)],  # the lodge to the glade
    [(230, 720), (150, 760), (70, 792), (8, 800)],                       # along the heights to the hollow
]


def base_height(x, y):
    """The land before the places are shaped into it; also the ground without the den's cliff."""
    r = math.hypot(x, y) / HALF
    h = 12.0 + 10.0 * (fbm(x / 220.0, y / 220.0, 5, 1) - 0.5) * 2
    # Rolling ground: gentle hills between the places.
    h += 14.0 * (fbm(x / 140.0 + 3, y / 140.0 + 9, 4, 4) - 0.5)
    # The ring of mountains.
    h += 260.0 * smoothstep(0.7, 1.05, r) * (0.6 + 0.65 * fbm(x / 150.0 + 7, y / 150.0 + 3, 5, 2))
    # The cold heights in the north and the crag in the east.
    h += 170.0 * math.exp(-(((x - HEIGHTS[0]) / 330.0) ** 2 + ((y - HEIGHTS[1]) / 200.0) ** 2))
    h += 170.0 * math.exp(-(((x - CRAG[0]) / 90.0) ** 2 + ((y - CRAG[1]) / 110.0) ** 2))
    h0 = h  # the valley floor, before the den's plateau
    # The den's plateau with its sheer cliff.
    band = smoothstep(-330.0, -220.0, y) * (1.0 - smoothstep(480.0, 600.0, y))
    plateau = PLATEAU + 8.0 * (fbm(x / 80.0, y / 80.0, 3, 5) - 0.5)
    step = 1.0 / (1.0 + math.exp((x - CLIFF_X) / 4.0))
    h = h + (max(h, plateau) - h) * step * band
    # The Nesting Stone's hill.
    d = math.hypot(x - STONE_HILL[0], y - STONE_HILL[1])
    h = max(h, min(74.0, 14.0 + 64.0 * math.exp(-(d / 110.0) ** 2)))
    # The Vault's snowy shelf, cut into the heights.
    vx, vy = PLACES[P_VAULT][0], PLACES[P_VAULT][1]
    w = 1.0 - smoothstep(26.0, 190.0, math.hypot(x - vx, y - vy))  # (a gentle shoulder: its trail climbs it, run 19)
    h = h + (152.0 - h) * w
    return h, h0


PLACE_BASE = {}


PATH_GRADE = 0.55      # the steepest a path climbs (about 29 degrees: the walker manages 44)
PATH_CORE, PATH_EDGE = 6.0, 12.0  # graded fully within this of a path, blended out to this (the 4 m grid needs room)
FORD = WATER - 0.25    # a path crossing a stream wades it here (the walker wades 0.6 m)
_PROFILES = None


def path_profiles():
    """Each path's height along it, every 4 m: the land's, smoothed and held to PATH_GRADE."""
    global _PROFILES
    if _PROFILES is not None:
        return _PROFILES
    out = []
    for path in PATHS:
        pts, acc, corners = [], [0.0], []
        for k in range(len(path) - 1):
            (ax, ay), (bx, by) = path[k], path[k + 1]
            seg = math.hypot(bx - ax, by - ay)
            n = max(1, int(seg / 4.0))
            if k > 0:
                corners.append(len(pts))  # a bend: its sample
            for i in range(n):
                t = i / n
                pts.append((ax + (bx - ax) * t, ay + (by - ay) * t))
        pts.append(path[-1])
        for i in range(1, len(pts)):
            acc.append(acc[-1] + math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]))
        raw = [land(x, y) for x, y in pts]
        sm = []
        for i in range(len(raw)):  # a moving average over about 24 m either way
            lo, hi = max(0, i - 6), min(len(raw), i + 7)
            sm.append(sum(raw[lo:hi]) / (hi - lo))
        for _ in range(3):  # held to the grade, forward and back, the ends kept where they meet their places
            for c in corners:  # level through each bend (a switchback's legs meet at one height there)
                for i in range(max(1, c - 3), min(len(sm) - 1, c + 4)):
                    sm[i] = sm[c]
            sm[0], sm[-1] = raw[0], raw[-1]
            for i in range(1, len(sm)):
                step = PATH_GRADE * (acc[i] - acc[i - 1])
                sm[i] = min(max(sm[i], sm[i - 1] - step), sm[i - 1] + step)
            sm[-1] = raw[-1]
            for i in range(len(sm) - 2, -1, -1):
                step = PATH_GRADE * (acc[i + 1] - acc[i])
                sm[i] = min(max(sm[i], sm[i + 1] - step), sm[i + 1] + step)
        xs = [q[0] for q in pts]
        ys = [q[1] for q in pts]
        bends = [(pts[i][0], pts[i][1], sm[i]) for i in corners]
        out.append((pts, sm, (min(xs) - PATH_EDGE - 4, max(xs) + PATH_EDGE + 4, min(ys) - PATH_EDGE - 4, max(ys) + PATH_EDGE + 4),
                    bends))
    _PROFILES = out
    return out


def graded(x, y, h):
    """The land pulled to the paths' graded profiles within their corridors."""
    for path, (pts, prof, (x0, x1, y0, y1), bends) in zip(PATHS, path_profiles()):
        if x < x0 or x > x1 or y < y0 or y > y1 or poly_near(x, y, path, PATH_EDGE + 4) >= PATH_EDGE + 4:
            continue
        for bx, by, bz in bends:  # each bend a flat landing to turn on (a switchback's corner)
            db = math.hypot(x - bx, y - by)
            if db < PATH_EDGE + 4:
                h = h + (bz - h) * (1.0 - smoothstep(PATH_CORE + 3, PATH_EDGE + 4, db))
        best, at = PATH_EDGE, None
        for i in range(len(pts) - 1):
            ax, ay = pts[i]
            bx, by = pts[i + 1]
            dx, dy = bx - ax, by - ay
            L = dx * dx + dy * dy
            t = 0.0 if L == 0 else max(0.0, min(1.0, ((x - ax) * dx + (y - ay) * dy) / L))
            d = math.hypot(x - ax - dx * t, y - ay - dy * t)
            if d < best:
                best, at = d, prof[i] + (prof[i + 1] - prof[i]) * t
        if at is not None:
            w = 1.0 - smoothstep(PATH_CORE, PATH_EDGE, best)
            h = h + (at - h) * w
    return h


def near_any_path(x, y, pad):
    for path in PATHS:
        if poly_near(x, y, path, pad) < pad:
            return poly_near(x, y, path, pad)
    return pad + 1.0


def height(x, y):
    return den_pad(x, y, height_raw(x, y))


_DEN_Z = []  # the ground at the den's anchor, before den_pad (worked out once)


def den_pad(x, y, h):
    """The ground held under the den's tunnel floor (run 21 take 4: "a bit of the ground over the
    floor entrance"). The height samples are 4 m apart and the arch's floor (at its anchor's height
    and 3 cm, over x +-3.6 and y -3.6 .. 0.35 in its frame) spans little more than one of their
    cells, so the ground over it is a plane between samples: those in the anchor's row and in front
    of it are held level with the anchor (it sits on a sample), those behind it 0.3 m under."""
    u, f = local(x, y, P_DEN)
    if f < -10.0 or f > 10.0 or abs(u) > 9.0:
        return h
    if not _DEN_Z:
        _DEN_Z.append(height_raw(PLACES[P_DEN][0], PLACES[P_DEN][1]))
    z = _DEN_Z[0]
    side = 1.0 - smoothstep(5.5, 8.5, abs(u))
    level = z - 0.3 * (1.0 - smoothstep(-2.5, -1.0, f))  # under the floor behind, level in front
    if f < -1.0:
        return h + (min(h, level) - h) * side * smoothstep(-10.0, -7.0, f)
    return h + (level - h) * side * (1.0 - smoothstep(5.0, 9.0, f))


def height_raw(x, y):
    h = graded(x, y, land(x, y))
    # The Keeper's yard kept level after the paths' grading (run 21: the path up to the door
    # raised the ground round the cottage 0.4 m and buried its vegetable patch).
    kx, ky = PLACES[P_KEEPER][0], PLACES[P_KEEPER][1]
    dk = math.hypot(x - kx, y - ky)
    if dk < 16.0:
        yard = graded(kx, ky, land(kx, ky))
        h = h + (yard - h) * (1.0 - smoothstep(12.0, 16.0, dk))
    # The river, its outlet, the brook and the stream on the plateau, cut in.
    dr = min(poly_near(x, y, RIVER, 120.0), poly_near(x, y, OUTLET, 120.0))
    h -= 8.0 * math.exp(-(dr / 12.0) ** 2) + 4.0 * math.exp(-(dr / 60.0) ** 2)
    db = poly_near(x, y, BROOK, 40.0)
    h -= 7.0 * math.exp(-(db / 6.0) ** 2)
    ds = poly_near(x, y, STREAM, 40.0)
    h -= 3.5 * math.exp(-(ds / 7.0) ** 2) * (1.0 if x < CLIFF_X + 4 else 0.0)
    # Where a path crosses them: a shallow ford you wade (the mill's bridge keeps its channel below).
    if h < FORD:
        dp = near_any_path(x, y, PATH_EDGE)
        if dp < PATH_EDGE:
            h = h + (FORD - h) * (1.0 - smoothstep(PATH_CORE, PATH_EDGE, dp))
    # The falls' plunge pool.
    dpool = math.hypot(x - POOL[0], y - POOL[1])
    if dpool < POOL[2] * 1.6:
        h = h + (min(h, WATER - 2.0) - h) * (1.0 - smoothstep(POOL[2] * 0.6, POOL[2] * 1.6, dpool))
    # The lake's bowl.
    lx, ly, ra, rb = LAKE
    e = math.hypot((x - lx) / ra, (y - ly) / rb)
    if e < 1.3:
        bowl = WATER - 8.0 * (1.0 - min(1.0, e) ** 2) - 0.5
        h = h + (min(h, bowl) - h) * (1.0 - smoothstep(0.85, 1.3, e))
    h = mill_banks(x, y, h)
    h = grotto_notch(x, y, h)
    return h


def land(x, y):
    """The land with its places shaped in, before the paths are graded and the water cut."""
    h, h0 = base_height(x, y)
    # Flattened ground at the places (with a soft rim).
    for pid, (px, py, fr, fh, _) in PLACES.items():
        if fr <= 0:
            continue
        if fh is None:  # its own ground, flattened
            if pid not in PLACE_BASE:
                PLACE_BASE[pid] = base_height(px, py)[0]
            fh = PLACE_BASE[pid]
        d = math.hypot(x - px, y - py)
        w = 1.0 - smoothstep(fr * 0.7, fr * 1.25, d)
        rim = smoothstep(fr * 0.7, fr * 1.05, d)  # exactly flat inside, a little unevenness at the rim (models sit true)
        h = h + (fh + 1.0 * (fbm(x / 40.0, y / 40.0, 2, 9) - 0.5) * rim - h) * w
    h = den_notch(x, y, h, h0)
    h = caldera(x, y, h)
    h = hollow(x, y, h)
    return h


def local(x, y, pid):
    """(right, forward) of a point in a place's frame (forward = (sin h, -cos h))."""
    px, py, _, _, hd = PLACES[pid]
    fx, fy = math.sin(hd), -math.cos(hd)
    dx, dy = x - px, y - py
    return dx * fy - dy * fx, dx * fx + dy * fy


def den_notch(x, y, h, h0):
    """The den's yard cut into the cliff's foot, and the cave's mouth deeper still: its arch
    model (places.json) stands out of the cliff face at local y -1.9, its tunnel 3.6 m deep."""
    u, f = local(x, y, P_DEN)
    if f < -12.0 or f > 26.0 or abs(u) > 22.0:
        return h
    yard = smoothstep(-5.0, -1.0, f) * (1.0 - smoothstep(11.0, 17.0, abs(u))) * (1.0 - smoothstep(18.0, 26.0, f))
    mouth = smoothstep(-10.0, -6.5, f) * (1.0 - smoothstep(4.5, 7.5, abs(u)))
    w = max(yard, mouth)
    h = h + (min(h, h0) - h) * w
    # Inside the arch the ground sits under the model's floor (run 21: level with it, the grass
    # came up through the tunnel's floor).
    inside = (1.0 - smoothstep(-2.4, -1.4, f)) * smoothstep(-9.0, -7.0, f) * (1.0 - smoothstep(3.2, 4.4, abs(u)))
    return h - 0.35 * inside


def mill_banks(x, y, h):
    """The windmill's banks at its bridge's ends, the river kept cut between them."""
    px, py = PLACES[P_MILL][0], PLACES[P_MILL][1]
    d = math.hypot(x - px, y - py)
    if d > 30.0:
        return h
    u, f = local(x, y, P_MILL)
    bank = WATER + MILL_WATER + 0.25  # (a touch above the bridge's ends, so they sit in it: Beta 1 review)
    w = 1.0 - smoothstep(18.0, 28.0, d)
    if abs(u) < 4.6:  # the channel under the bridge's arch
        return h + (min(h, WATER - 2.2) - h) * w
    return h + (bank - h) * w * smoothstep(4.6, 5.6, abs(u))


def grotto_notch(x, y, h):
    """The grotto's chamber cut into the cliff behind the falls, its mouth open east to the pool."""
    u, f = local(x, y, P_GROTTO)
    d = math.hypot(u, f)
    if d > 22.0:
        return h
    room = 1.0 - smoothstep(7.5, 10.0, d)
    mouth = (1.0 - smoothstep(3.0, 6.0, abs(u))) * smoothstep(0.0, 2.0, -f) * (1.0 - smoothstep(12.0, 20.0, -f))
    w = max(room, mouth)
    return h + (GROTTO_FLOOR - h) * w


def caldera(x, y, h):
    """Emberpeak: a cone on the valley's east side, its crater's floor flat (the league's stage),
    its inner walls steep, a gap in the rim toward the arena with a ramp down the flank."""
    cx, cy = CALDERA
    d = math.hypot(x - cx, y - cy)
    if d > 320.0:
        return h
    rim = CALDERA_FLOOR + 24.0 + 4.0 * (fbm(x / 30.0, y / 30.0, 2, 57) - 0.5)
    if d < 50.0:
        cone = CALDERA_FLOOR + (rim - CALDERA_FLOOR) * smoothstep(33.0, 50.0, d) ** 1.4
        hc = cone
    else:
        cone = rim - 0.31 * (d - 50.0) - 6.0 * smoothstep(50.0, 70.0, d) * 0.0
        hc = max(h, cone)
    a = math.atan2(y - cy, x - cx) - CALDERA_GAP
    a = (a + math.pi) % (2 * math.pi) - math.pi
    gap = 1.0 - smoothstep(0.10, 0.24, abs(a))
    ramp = CALDERA_FLOOR if d < 33.0 else max(h, CALDERA_FLOOR - 0.19 * (d - 33.0))
    return hc + (ramp - hc) * gap * (1.0 - smoothstep(260.0, 320.0, d))


def hollow(x, y, h):
    """Frostspire Hollow: a flat bowl cut into the heights, ringed by a raised rim of rock and ice,
    with a corridor out through the rim at its front (east) to the trail."""
    hx, hy = HOLLOW
    d = math.hypot(x - hx, y - hy)
    if d > 80.0:
        return h
    rim = HOLLOW_FLOOR + 16.0 * (1.0 - smoothstep(24.0, 70.0, d)) + 3.0 * (fbm(x / 12.0, y / 12.0, 2, 63) - 0.5)
    hr = max(h, rim) if d > 15.0 else h
    u, f = local(x, y, P_HOLLOW)
    room = 1.0 - smoothstep(15.0, 19.0, d)
    mouth = (1.0 - smoothstep(3.5, 6.5, abs(u))) * smoothstep(6.0, 10.0, f) * (1.0 - smoothstep(48.0, 64.0, f))
    return hr + (HOLLOW_FLOOR - hr) * max(room, mouth)


# The storybook palette.
GRASS = (126, 178, 84)
GRASS_DEEP = (84, 146, 72)
MEADOW = (160, 200, 104)
PATH = (214, 184, 132)
COBBLE = (198, 180, 152)
ROCK = (150, 134, 142)
SNOW = (240, 244, 252)
SAND = (228, 208, 154)
SHALLOW = (104, 164, 158)
BASALT = (84, 72, 78)
ASH = (128, 112, 110)
MOSS_NIGHT = (70, 128, 112)
ICE = (206, 226, 244)


def mix(a, b, t):
    return tuple(a[k] + (b[k] - a[k]) * t for k in range(3))


def region_colour(x, y, h, slope):
    """Base colour (before light) by what the ground is."""
    g = fbm(x / 40.0, y / 40.0, 3, 21)
    c = mix(GRASS_DEEP, GRASS, g)
    forest = fbm(x / 120.0, y / 120.0, 3, 41)
    c = mix(c, GRASS_DEEP, smoothstep(0.5, 0.65, forest) * 0.6)  # darker under the woods
    sx, sy = PLACES[P_SANCTUARY][0], PLACES[P_SANCTUARY][1]
    c = mix(c, MEADOW, 1.0 - smoothstep(90.0, 170.0, math.hypot(x - sx, y - sy)))  # the meadow, lighter
    snow_line = 150.0 + 70.0 * smoothstep(420.0, 250.0, y)  # the cold north snows lower (the Vault's shelf)
    if slope < 1.0:
        c = mix(c, SNOW, smoothstep(snow_line, snow_line + 25.0, h))
    if slope > 0.62:  # rock, in soft storybook bands (the den's cliff, the crags)
        k = fbm(x / 22.0, y / 22.0, 2, 31)
        band = 0.5 + 0.5 * math.sin(h * 0.32 + 2.0 * fbm(x / 60.0, y / 60.0, 2, 13))
        rock = mix(mix(ROCK, (170, 156, 160), k), (126, 110, 124), band * 0.45)
        c = mix(c, rock, smoothstep(0.62, 0.85, slope))
    if h < WATER + 1.8:
        c = SAND if h > WATER - 0.4 else SHALLOW
        if abs(h - FORD) < 0.12 and near_any_path(x, y, 5.0) < 5.0:
            c = COBBLE  # a ford's stones under the shallow water
    # Paths, and the village's cobbles.
    for path in PATHS:
        dp = poly_near(x, y, path, 6.0)
        if dp < 6.0:
            c = mix(c, PATH, (1.0 - smoothstep(2.0, 5.5, dp)) * 0.95)
    mx, my = PLACES[P_MARKET][0], PLACES[P_MARKET][1]
    dm = math.hypot(x - mx, y - my)
    c = mix(c, COBBLE, 1.0 - smoothstep(26.0, 40.0, dm))
    ax, ay = PLACES[P_ARENA][0], PLACES[P_ARENA][1]
    c = mix(c, PATH, (1.0 - smoothstep(28.0, 40.0, math.hypot(x - ax, y - ay))) * 0.8)
    # 1.0: the caldera's ash and basalt, the glade's deep moss, the cove's sand, the hollow's ice.
    dc = math.hypot(x - CALDERA[0], y - CALDERA[1])
    c = mix(c, mix(ASH, BASALT, 1.0 - smoothstep(40.0, 90.0, dc)), 1.0 - smoothstep(120.0, 240.0, dc))
    gx, gy = PLACES[P_GLADE][0], PLACES[P_GLADE][1]
    c = mix(c, MOSS_NIGHT, (1.0 - smoothstep(30.0, 60.0, math.hypot(x - gx, y - gy))) * 0.85)
    kx, ky = PLACES[P_COVE][0], PLACES[P_COVE][1]
    if h > WATER - 0.4:
        c = mix(c, SAND, 1.0 - smoothstep(28.0, 46.0, math.hypot(x - kx, y - ky)))
    c = mix(c, ICE, 1.0 - smoothstep(18.0, 30.0, math.hypot(x - HOLLOW[0], y - HOLLOW[1])))
    return c


def build():
    print("[valley] heights...")
    hs = [[height(X0 + i * SPACING, Y0 + j * SPACING) for i in range(N)] for j in range(N)]
    print("[valley] colours...")
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
            base = region_colour(x, y, h, slope)
            lit = 0.62 + 0.55 * max(0.0, sum(nrm[k] * sun[k] for k in range(3)))
            cols[j][i] = tuple(max(0, min(255, int(v * lit))) for v in base)
    return hs, cols


def ground(hs, x, y):
    fi, fj = (x - X0) / SPACING, (y - Y0) / SPACING
    i, j = max(0, min(N - 2, int(fi))), max(0, min(N - 2, int(fj)))
    tx, ty = min(1.0, max(0.0, fi - i)), min(1.0, max(0.0, fj - j))
    a = hs[j][i] + (hs[j][i + 1] - hs[j][i]) * tx
    b = hs[j + 1][i] + (hs[j + 1][i + 1] - hs[j + 1][i]) * tx
    return a + (b - a) * ty


def slope_at(hs, x, y):
    return math.hypot(ground(hs, x + 2, y) - ground(hs, x - 2, y), ground(hs, x, y + 2) - ground(hs, x, y - 2)) / 4.0


def near_place(x, y, pad):
    for pid, (px, py, fr, _, _) in PLACES.items():
        if math.hypot(x - px, y - py) < max(fr, 18.0) + pad:
            return True
    return False


def near_path(x, y, pad):
    return any(poly_near(x, y, p, pad) < pad for p in PATHS)


def near_water(x, y, pad):
    return (poly_near(x, y, RIVER, pad) < pad or poly_near(x, y, OUTLET, pad) < pad or
            poly_near(x, y, BROOK, pad) < pad)


def scatter(hs):
    """Props by rules (see the module's doc). Returns [(x, y, kind, size m, shade, yaw)]."""
    rng = random.Random(7)
    props = []

    def add(x, y, kind, size):
        props.append((x, y, kind, size, rng.randrange(256), rng.randrange(256)))

    # Woods and single trees over the open ground.
    for gy in range(int(Y0), int(Y0 + 2 * HALF), 11):  # (Beta 1 review: more trees)
        for gx in range(int(X0), int(X0 + 2 * HALF), 11):
            x, y = gx + rng.uniform(0, 11), gy + rng.uniform(0, 11)
            if math.hypot(x, y) > HALF * 0.95:
                continue
            h = ground(hs, x, y)
            if h < WATER + 1.5 or slope_at(hs, x, y) > 0.5:
                continue
            forest = fbm(x / 120.0, y / 120.0, 3, 41)
            if forest < 0.52 and rng.random() > 0.08:
                continue  # open ground, now and then a lone tree
            if near_place(x, y, 10.0) or near_path(x, y, 6.0) or near_water(x, y, 10.0):
                continue
            if abs(x - CLIFF_X) < 14 or (x < CLIFF_X and h < PLATEAU - 12):
                continue
            if h > 150:
                continue  # the snow line
            if (math.hypot(x - CALDERA[0], y - CALDERA[1]) < 230.0 or math.hypot(x - HOLLOW[0], y - HOLLOW[1]) < 40.0
                    or math.hypot(x - PLACES[P_COVE][0], y - PLACES[P_COVE][1]) < 48.0):
                continue  # bare ash, the ice bowl, the beach
            kind = K_PINE if h > 70 or (forest > 0.62 and rng.random() < 0.3) else K_TREE
            add(x, y, kind, rng.uniform(7.0, 12.0) if kind == K_PINE else rng.uniform(5.5, 9.0))
    # The orchard: fruit trees in rows beside the orchard's corner.
    ox, oy = PLACES[P_ORCHARD][0], PLACES[P_ORCHARD][1]
    for r in range(5):
        for c in range(6):
            x, y = ox + 30 + c * 11.0 + rng.uniform(-1, 1), oy - 40 + r * 12.0 + rng.uniform(-1, 1)
            if not near_path(x, y, 4.0):
                add(x, y, K_FRUIT, rng.uniform(4.8, 6.0))
    # Bushes along the paths and round the places.
    for path in PATHS:
        for k in range(len(path) - 1):
            (ax, ay), (bx, by) = path[k], path[k + 1]
            length = math.hypot(bx - ax, by - ay)
            for s in range(int(length / 14)):
                t = (s + rng.random()) / max(1, int(length / 14))
                side = rng.choice((-1, 1))
                nx, ny = -(by - ay) / length, (bx - ax) / length
                x = ax + (bx - ax) * t + nx * side * rng.uniform(5.5, 9.0)
                y = ay + (by - ay) * t + ny * side * rng.uniform(5.5, 9.0)
                if rng.random() < 0.55 and ground(hs, x, y) > WATER + 1 and not near_place(x, y, 2.0):
                    add(x, y, K_BUSH if rng.random() < 0.8 else K_ROCK, rng.uniform(1.2, 2.2))
    # Rocks on the slopes and by the water; flowers in the meadows; reeds at the shores.
    for n in range(7000):
        x, y = rng.uniform(X0, X0 + 2 * HALF), rng.uniform(Y0, Y0 + 2 * HALF)
        if math.hypot(x, y) > HALF * 0.93:
            continue
        h, sl = ground(hs, x, y), slope_at(hs, x, y)
        if near_place(x, y, 4.0) or near_path(x, y, 4.5):
            continue
        if math.hypot(x - CALDERA[0], y - CALDERA[1]) < 52.0 or math.hypot(x - HOLLOW[0], y - HOLLOW[1]) < 20.0:
            continue
        if WATER - 0.5 < h < WATER + 1.2 and rng.random() < 0.6:
            add(x, y, K_REEDS, rng.uniform(1.0, 1.8))
        elif sl > 0.35 and h > WATER + 2 and rng.random() < 0.25:
            add(x, y, K_ROCK, rng.uniform(1.5, 4.5))
        elif h > WATER + 2 and h < 90 and sl < 0.25 and fbm(x / 90.0, y / 90.0, 2, 77) > 0.56 and rng.random() < 0.5:
            add(x, y, K_FLOWERS, rng.uniform(2.0, 4.0))
    # Extra flowers all over the Sanctuary's meadow.
    sx, sy = PLACES[P_SANCTUARY][0], PLACES[P_SANCTUARY][1]
    for n in range(140):
        a, r = rng.uniform(0, 2 * math.pi), 40 + rng.uniform(0, 110)
        x, y = sx + math.cos(a) * r, sy + math.sin(a) * r
        if not near_path(x, y, 4.0):
            add(x, y, K_FLOWERS, rng.uniform(2.0, 3.5))
    return props


def places(hs):
    out = []
    for pid, (x, y, fr, fh, heading) in PLACES.items():
        if pid == P_ISLES:
            isl = ISLANDS[0]
            out.append((pid, isl[0], isl[1], isl[2], heading))
        elif pid == P_LAKE:
            out.append((pid, x, y, WATER + 0.6, heading))  # places.json water_z -0.6
        elif pid == P_MILL:
            out.append((pid, x, y, WATER + MILL_WATER, heading))
        elif pid == P_GROTTO:
            out.append((pid, x, y, GROTTO_FLOOR, heading))
        elif pid == P_CALDERA:
            out.append((pid, x, y, CALDERA_FLOOR, heading))
        elif pid == P_HOLLOW:
            out.append((pid, x, y, HOLLOW_FLOOR, heading))
        else:
            out.append((pid, x, y, ground(hs, x, y), heading))
    return out


def write(path, hs, cols, props, plc):
    flat = [h for row in hs for h in row]
    hmin, hmax = min(flat), max(flat)
    out = bytearray(b"EVL2") + struct.pack("<HHfffffff", 2, N, SPACING, X0, Y0, hmin, hmax, WATER, 0.0)
    out += struct.pack(f"<{N * N}H", *[int(round((h - hmin) / (hmax - hmin) * 65535)) for h in flat])
    out += bytes(int(v) for row in cols for c in row for v in c)
    out += struct.pack("<H", len(props))
    for x, y, kind, size, shade, yaw in props:
        out += struct.pack("<ffBBBB", x, y, kind, max(1, min(255, int(round(size * 10)))), shade, yaw)
    out += struct.pack("<H", len(ISLANDS))
    for isl in ISLANDS:
        out += struct.pack("<ffff", *isl)
    out += struct.pack("<H", len(plc))
    for pid, x, y, z, hd in plc:
        out += struct.pack("<Bffff", pid, x, y, z, hd)
    out += struct.pack("<H", len(PATHS))
    for p in PATHS:
        out += struct.pack("<H", len(p))
        for x, y in p:
            out += struct.pack("<ff", x, y)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, "wb").write(out)
    return len(out), hmin, hmax


def write_png(path, cols, props, plc):
    """A top view for review (north up): props as dots in their colours, places as white squares."""
    img = [[list(c) for c in row] for row in cols]
    dot = {K_TREE: [46, 96, 50], K_PINE: [30, 70, 50], K_FRUIT: [200, 90, 60], K_BUSH: [70, 120, 60],
           K_ROCK: [120, 110, 116], K_FLOWERS: [236, 150, 190], K_REEDS: [120, 150, 80]}
    for x, y, kind, _, _, _ in props:
        i, j = int((x - X0) / SPACING), int((y - Y0) / SPACING)
        if 0 <= i < N and 0 <= j < N:
            img[j][i] = dot[kind]
    for _, x, y, _, _ in plc:
        ci, cj = int((x - X0) / SPACING), int((y - Y0) / SPACING)
        for dj in range(-3, 4):
            for di in range(-3, 4):
                if 0 <= ci + di < N and 0 <= cj + dj < N:
                    img[cj + dj][ci + di] = [255, 255, 255]
    raw = bytearray()
    for j in range(N - 1, -1, -1):  # north up
        raw.append(0)
        for i in range(N):
            raw += bytes(int(v) for v in img[j][i])

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
    props = scatter(hs)
    plc = places(hs)
    size, hmin, hmax = write(out, hs, cols, props, plc)
    kinds = [sum(1 for p in props if p[2] == k) for k in range(7)]
    print(f"[valley] {out}: {size} bytes, {2 * HALF:.0f} m across, heights {hmin:.1f}..{hmax:.1f} m, props {len(props)} "
          f"(trees {kinds[0]}, pines {kinds[1]}, fruit {kinds[2]}, bushes {kinds[3]}, rocks {kinds[4]}, flowers "
          f"{kinds[5]}, reeds {kinds[6]}), {len(ISLANDS)} islands, {len(plc)} places, {len(PATHS)} paths")
    if "--preview" in argv:
        write_png(argv[argv.index("--preview") + 1], cols, props, plc)


if __name__ == "__main__":
    main()
