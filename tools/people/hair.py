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
# (Set 2, the Storybook look, D138: the swept fringe and the braid are its own two; the rest are
# sized to its smaller head, k = the head against the old standard one.)
def _k(h):
    return h.azt / 0.265


def style_swept(m, h, mat="hair", w=W):
    """0: a swept-up fringe of chunky points, the ears showing (Set 2's keeper)."""
    shell(m, h, lambda az: angle_blend(az, {0: 22, 25: 18, 60: 10, 88: 4, 100: -10, 150: -30, 180: -34}),
          lambda az, f: 0.026 + 0.04 * (1 - f) ** 1.4, n_az=12, rows=3, phase=0.5, crown=(0.025, 0.01), mat=mat, w=w,
          locks=lambda k, az: (13 if az < 40 else 8) if k % 2 == 0 else 0)


def style_bob(m, h, mat="hair", w=W):
    """1: a rounded bob to the chin, a full straight fringe."""
    edge = lambda az: angle_blend(az, {0: 19, 38: 17, 55: -62, 100: -66, 180: -62})
    thick = lambda az, f: 0.022 + 0.03 * (1 - f) ** 1.2
    locks = lambda k, az: (4 if k % 2 == 0 else 0) if abs(az) < 45 else 0
    shell(m, h, edge, thick, n_az=12, rows=4, locks=locks, curtain=-12, flare=0.12, mat=mat, w=w,
          phase=0.5, row_bias=0.8)


def style_ponytail(m, h, mat="hair", w=W, tie="trim"):
    """2: swept back into a high, bouncy ponytail, a side-swept fringe."""
    k = _k(h)
    edge = lambda az: angle_blend(az, {0: 24, 25: 18, 50: 15, 80: 7, 96: 5, 125: -22, 180: -34})
    thick = lambda az, f: 0.02 + 0.024 * (1 - f) ** 1.3
    locks = lambda n, az: (11 if (n % 2 == 0 and -40 < az < 70) else 0)
    shell(m, h, edge, thick, n_az=10, rows=3, locks=locks, mat=mat, w=w, phase=0.5, crown=(0.02, 0.0))
    knot = h.at(180, 40, 0.03 * k)
    base = h.at(180, 40, -0.03 * k)
    p1 = add(knot, (0.0, 0.075 * k, 0.0))  # out from the tie, then hanging close down the back of the head
    p2 = add(knot, (0.0, 0.1 * k, -0.27 * k))
    tube(m, [base, p1, p2], [0.05 * k, 0.068 * k, 0.056 * k], 5, mat, w, tip1=0.1 * k)
    tube(m, [add(knot, (0, -0.02 * k, -0.004)), add(knot, (0, 0.022 * k, 0.002))], [0.05 * k, 0.052 * k], 5, tie, w)


def style_buns(m, h, mat="hair", w=W):
    """3: two round buns up top, a soft fringe parted in the middle."""
    k = _k(h)
    edge = lambda az: angle_blend(az, {0: 25, 20: 18, 45: 14, 80: 6, 96: 3, 125: -24, 180: -36})
    thick = lambda az, f: 0.02 + 0.02 * (1 - f) ** 1.5
    locks = lambda n, az: (7 if n % 2 == 1 else 0) if abs(az) < 60 else 0
    shell(m, h, edge, thick, n_az=10, rows=3, locks=locks, mat=mat, w=w, phase=0.0)
    for s in (-1, 1):
        c = h.at(s * 62, 46, 0.05 * k)
        ellipsoid(m, c, (0.074 * k, 0.072 * k, 0.07 * k), 5, 3, mat, w)


def style_spiky(m, h, mat="hair", w=W):
    """4: big chunky spikes swept up and back, a hero's quiff."""
    k = _k(h)
    edge = lambda az: angle_blend(az, {0: 24, 30: 20, 70: 12, 100: -10, 150: -30, 180: -34})
    thick = lambda az, f: 0.028 + 0.032 * (1 - f) ** 1.4
    locks = lambda n, az: 15 if n % 2 == 0 else 0
    shell(m, h, edge, thick, n_az=10, rows=3, locks=locks, mat=mat, w=w, phase=0.5)
    for az, el, l, r in ((-8, 46, 0.14, 0.09), (150, 44, 0.13, 0.085), (-150, 40, 0.12, 0.08)):
        l, r = l * k, r * k
        base = h.at(az, el, -0.02 * k)
        d = norm(sub(h.at(az, el), h.c))
        up = add(mul(d, 0.6), (0.0, 0.6, 0.45))  # up and swept back
        mid = add(base, mul(norm(up), 0.055 * k))
        tube(m, [base, mid], [(r, r * 0.55), (r * 0.8, r * 0.45)], 4, mat, w, tip1=l, up_hint=d)


def style_braid(m, h, mat="hair", w=W, tie="trim"):
    """5: a long braid down the back with a ribbon near its end, a soft parted fringe and two locks
    framing the face (Set 2's keeper)."""
    k = _k(h)
    shell(m, h, lambda az: angle_blend(az, {0: 16, 20: 20, 45: 10, 70: -12, 100: -28, 150: -38, 180: -40}),
          lambda az, f: 0.018 + 0.028 * (1 - f) ** 1.3, n_az=10, rows=3, phase=0.0, mat=mat, w=w,
          locks=lambda n, az: (7 if n % 2 == 1 else 0) if abs(az) < 60 else 0)
    for s in (-1, 1):
        base = h.at(s * 60, 2, 0.018)
        tube(m, [base, add(base, (s * 0.004, -0.012, -0.14 * k / 0.8))], [(0.03, 0.02), (0.02, 0.013)], 3, mat, w,
             tip1=0.03, up_hint=(s * 1.0, 0.0, 0.0))
    nape = h.at(180, -30, 0.01)
    f = k / 0.8
    pts = [nape, add(nape, (0.0, 0.07 * f, -0.2 * f)), add(nape, (0.0, 0.07 * f, -0.42 * f))]
    tube(m, pts, [0.05 * f, 0.044 * f, 0.03 * f], 4, mat, w, tip1=0.05 * f)
    t = add(nape, (0.0, 0.072 * f, -0.35 * f))
    tube(m, [add(t, (0.0, 0.0, 0.02 * f)), add(t, (0.0, 0.0, -0.02 * f))], [0.04 * f, 0.038 * f], 3, tie, w)


STYLES = [style_swept, style_bob, style_ponytail, style_buns, style_spiky, style_braid]
STYLE_NAMES = ["Swept", "Bob", "Ponytail", "Buns", "Spiky", "Braid"]


def player_hair(head):
    out = []
    for fn in STYLES:
        m = Mesh()
        fn(m, head)
        out.append(m)
    return out
