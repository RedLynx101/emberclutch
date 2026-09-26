"""Hair: a shell over the head (the crown down to a hairline that changes round the head), with
chunky locks along its edge, plus extras (a ponytail, buns, a quiff). The player's six styles
are rigid part meshes on the head bone (group 10, variants 0-5); villagers' hair goes into
their body mesh the same way.

Angles are degrees round the head (rig: az 0 = the face, +90 = the +X side; el up from the
head's middle). Everything sits outside the head's surface, so nothing pokes through.
"""
import math

from geom import Mesh, away_from, ellipsoid, lathe, rigid, tube
from rig import add, mul, norm, sub

W = rigid("head")


def shell(m, head, edge, thick, n_az=12, rows=4, locks=None, curtain=None, flare=0.25, mat="hair", w=W,
          phase=0.0, crown=(0.0, 0.0), row_bias=1.0):
    """edge(az) -> the hairline's elevation; thick(az, f) -> how far off the scalp (f: 0 at the
    crown, 1 at the edge); locks(k, az) -> extra drop (degrees) of edge vertex k (points,
    scallops); curtain: below this elevation the hair hangs straight down (bobs, long hair),
    flaring out by `flare` of the drop. crown = (dx, dy) nudge of the crown point (a parting)."""
    cz = head.c

    def point(az, el, f):
        t = thick(az, f)
        # hair over the ears (az +-91, el -9) stands clear of them
        d_ear = math.hypot(abs(az) - 91.0, el + 9.0)
        if d_ear < 26.0:
            t = max(t, 0.04 - 0.012 * d_ear / 26.0)
        if curtain is not None and el < curtain:
            p0 = head.at(az, curtain, t)
            drop = (curtain - el) / 90.0 * head.azb * 1.2  # hanging: degrees past the curtain -> metres
            rad = norm((p0[0] - cz[0], p0[1] - cz[1], 0.0))
            return (p0[0] + rad[0] * flare * drop, p0[1] + rad[1] * flare * drop, p0[2] - drop)
        return head.at(az, el, t)

    top = add(head.at(0, 90, thick(0, 0.0)), (crown[0], crown[1], 0.0))
    ids_top = m.vert(top, w(top), mat)
    grid = []
    for j in range(1, rows + 1):
        f = (j / rows) ** row_bias
        row = []
        for k in range(n_az):
            az = 360.0 * (k + phase) / n_az
            az = (az + 180.0) % 360.0 - 180.0
            e = edge(az)
            el = 90.0 - f * (90.0 - e)
            if j == rows and locks:
                el -= locks(k, az)
            p = point(az, el, f)
            row.append(m.vert(p, w(p), mat))
        grid.append(row)
    out = away_from(cz)
    for k in range(n_az):
        k1 = (k + 1) % n_az
        m.tri(ids_top, grid[0][k], grid[0][k1], out)
        for a, b in zip(grid, grid[1:]):
            m.quad(a[k], b[k], b[k1], a[k1], out)
    return grid


def fringe(m, head, az0, az1, n, el_top, edge, thick, locks=None, mat="hair", w=W):
    """A strip of hair across the front only (bangs peeking from under a headscarf or hat):
    from el_top (hidden under it) down to edge(az), n columns between az0 and az1."""
    top, bot = [], []
    for k in range(n + 1):
        az = az0 + (az1 - az0) * k / n
        e = edge(az) - (locks(k, az) if locks else 0.0)
        a, b = head.at(az, el_top, thick), head.at(az, e, thick * 0.8)
        top.append(m.vert(a, w(a), mat))
        bot.append(m.vert(b, w(b), mat))
    out = away_from(head.c)
    for k in range(n):
        m.quad(top[k], bot[k], bot[k + 1], top[k + 1], out)
    return m


def beard(m, head, edge, thick, drop, n=8, rows=3, span=100, mat="hair", w=W):
    """A beard hugging the jaw: from under the chin up to edge(az) (under the nose at the front,
    the sideburns at the sides), `drop` metres hanging below the chin, open at the back."""
    grid = []
    for j in range(rows + 1):
        f = j / rows  # 0: the beard's bottom, 1: its top edge
        row = []
        for k in range(n + 1):
            az = -span + 2 * span * k / n
            el = -78 + f * (edge(az) + 78)
            q = head.at(az, el, thick(az, f))
            hang = drop * (1 - f) ** 1.5 * (1 - 0.6 * (abs(az) / span) ** 2)
            q = (q[0], q[1] - 0.25 * hang, q[2] - hang)
            row.append(m.vert(q, w(q), mat))
        grid.append(row)
    out = away_from(head.c)
    for j in range(rows):
        for k in range(n):
            m.quad(grid[j][k], grid[j][k + 1], grid[j + 1][k + 1], grid[j + 1][k], out)
    return m


def angle_blend(az, table):
    """Interpolate a value round the head from {az: value} (degrees, wraps; symmetric keys
    need only 0..180: negative az mirror)."""
    a = abs(az)
    keys = sorted(table)
    if a <= keys[0]:
        return table[keys[0]]
    for k0, k1 in zip(keys, keys[1:]):
        if a <= k1:
            t = (a - k0) / (k1 - k0)
            t = t * t * (3 - 2 * t)
            return table[k0] + (table[k1] - table[k0]) * t
    return table[keys[-1]]


# ------------------------------------------------------------------------------ the styles
def style_tousled(m, h, mat="hair", w=W):
    """0: short and tousled, a fringe of chunky points swept to one side, a cowlick on top,
    the ears showing (the concept's)."""
    edge = lambda az: angle_blend(az, {0: 21, 30: 19, 60: 12, 80: 8, 96: 6, 118: -20, 150: -36, 180: -40})
    thick = lambda az, f: 0.026 + 0.036 * (1 - f) ** 1.4
    locks = lambda k, az: (12 if k % 2 == 0 else 0) if abs(az) < 75 else (8 if k % 2 == 0 else 0)
    shell(m, h, edge, thick, n_az=14, rows=3, locks=locks, mat=mat, w=w, phase=0.5, crown=(-0.02, 0.02))
    top = h.at(-24, 60, 0.035)
    tube(m, [top, add(top, (-0.006, 0.014, 0.03))], [(0.034, 0.022), (0.026, 0.016)], 4, mat, w, tip1=0.055)


def style_bob(m, h, mat="hair", w=W):
    """1: a rounded bob to the chin, a full straight fringe."""
    edge = lambda az: angle_blend(az, {0: 19, 38: 17, 55: -62, 100: -66, 180: -62})
    thick = lambda az, f: 0.024 + 0.034 * (1 - f) ** 1.2
    locks = lambda k, az: (4 if k % 2 == 0 else 0) if abs(az) < 45 else 0
    shell(m, h, edge, thick, n_az=12, rows=4, locks=locks, curtain=-12, flare=0.12, mat=mat, w=w,
          phase=0.5, row_bias=0.8)


def style_ponytail(m, h, mat="hair", w=W, tie="trim"):
    """2: swept back into a high, bouncy ponytail, a side-swept fringe."""
    edge = lambda az: angle_blend(az, {0: 24, 25: 18, 50: 15, 80: 7, 96: 5, 125: -22, 180: -34})
    thick = lambda az, f: 0.02 + 0.026 * (1 - f) ** 1.3
    locks = lambda k, az: (11 if (k % 2 == 0 and -40 < az < 70) else 0)
    shell(m, h, edge, thick, n_az=12, rows=3, locks=locks, mat=mat, w=w, phase=0.5, crown=(0.02, 0.0))
    knot = h.at(180, 40, 0.03)
    base = h.at(180, 40, -0.03)
    p1 = add(knot, (0.0, 0.075, 0.0))  # out from the tie, then hanging close down the back of the head,
    p2 = add(knot, (0.0, 0.1, -0.27))  # its end over the nape (it must read from behind: the camera's view)
    tube(m, [base, p1, p2], [0.05, 0.068, 0.056], 5, mat, w, tip1=0.1)
    tube(m, [add(knot, (0, -0.02, -0.004)), add(knot, (0, 0.022, 0.002))], [0.05, 0.052], 5, tie, w)


def style_buns(m, h, mat="hair", w=W):
    """3: two round buns up top, a soft fringe parted in the middle."""
    edge = lambda az: angle_blend(az, {0: 25, 20: 18, 45: 14, 80: 6, 96: 3, 125: -24, 180: -36})
    thick = lambda az, f: 0.02 + 0.022 * (1 - f) ** 1.5
    locks = lambda k, az: (7 if k % 2 == 1 else 0) if abs(az) < 60 else 0
    shell(m, h, edge, thick, n_az=12, rows=3, locks=locks, mat=mat, w=w, phase=0.0)
    for s in (-1, 1):
        c = h.at(s * 62, 46, 0.055)
        ellipsoid(m, c, (0.074, 0.072, 0.07), 5, 3, mat, w)


def style_spiky(m, h, mat="hair", w=W):
    """4: big chunky spikes swept up and back, a hero's quiff."""
    edge = lambda az: angle_blend(az, {0: 24, 30: 20, 70: 12, 100: -10, 150: -30, 180: -34})
    thick = lambda az, f: 0.03 + 0.036 * (1 - f) ** 1.4
    locks = lambda k, az: 15 if k % 2 == 0 else 0
    shell(m, h, edge, thick, n_az=12, rows=3, locks=locks, mat=mat, w=w, phase=0.5)
    for az, el, l, r in ((-8, 46, 0.14, 0.09), (150, 44, 0.13, 0.085), (-150, 40, 0.12, 0.08)):
        base = h.at(az, el, -0.02)
        d = norm(sub(h.at(az, el), h.c))
        up = add(mul(d, 0.6), (0.0, 0.6, 0.45))  # up and swept back
        mid = add(base, mul(norm(up), 0.055))
        tube(m, [base, mid], [(r, r * 0.55), (r * 0.8, r * 0.45)], 4, mat, w, tip1=l, up_hint=d)


def style_long(m, h, mat="hair", w=W):
    """5: long and flowing to the shoulders, parted to one side, the ends in soft points; it
    falls in behind the shoulders instead of standing out like a hood."""
    def edge(az):  # a side-swept fringe: longer over the +X brow
        if az >= 0:
            return angle_blend(az, {0: 17, 28: 9, 46: 0, 60: -78, 180: -84})
        return angle_blend(az, {0: 17, 18: 22, 44: 8, 62: -78, 180: -84})
    thick = lambda az, f: 0.022 + 0.03 * (1 - f) ** 1.2
    locks = lambda k, az: (20 if k % 2 == 0 else 0) if abs(az) > 45 else (7 if k % 2 == 0 else 0)
    shell(m, h, edge, thick, n_az=12, rows=4, locks=locks, curtain=-10, flare=-0.22, mat=mat, w=w,
          phase=0.5, crown=(-0.04, 0.0), row_bias=0.75)


STYLES = [style_tousled, style_bob, style_ponytail, style_buns, style_spiky, style_long]
STYLE_NAMES = ["Tousled", "Bob", "Ponytail", "Buns", "Spiky", "Long"]


def player_hair(head):
    out = []
    for fn in STYLES:
        m = Mesh()
        fn(m, head)
        out.append(m)
    return out
