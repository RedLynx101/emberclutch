"""Low-poly mesh building for the people (pure Python).

A Mesh is a list of vertices (position, two bone weights, paint) and triangles. Each primitive
adds its own island of vertices, so its normals are smooth inside it and its colour edges are
crisp (vertex paint can't change colour inside a triangle). Every triangle is wound counter-
clockwise seen from outside (the game culls back faces): each primitive says which way is out.

Weights are [(bone, weight)] with at most two bones (the shader's limit), summing to 1.
Paint is a material name from looks.PAINT (two palette slots, a mix and an emissive weight).
"""
import math

from rig import add, cross, dot, length, lerp, mul, norm, sub


class Mesh:
    def __init__(self):
        self.pos, self.w, self.mat, self.tris = [], [], [], []

    def __len__(self):
        return len(self.pos)

    def vert(self, p, w, mat):
        self.pos.append(tuple(float(c) for c in p))
        self.w.append(w)
        self.mat.append(mat)
        return len(self.pos) - 1

    def tri(self, a, b, c, out):
        """A triangle facing `out` (a direction, or a callable of the centroid)."""
        pa, pb, pc = self.pos[a], self.pos[b], self.pos[c]
        n = cross(sub(pb, pa), sub(pc, pa))
        if length(n) < 1e-12:
            return  # degenerate (a pole's sliver): skip it
        o = out(mul(add(add(pa, pb), pc), 1 / 3)) if callable(out) else out
        self.tris.append((a, b, c) if dot(n, o) >= 0 else (a, c, b))

    def quad(self, a, b, c, d, out):
        self.tri(a, b, c, out)
        self.tri(a, c, d, out)

    def extend(self, other):
        base = len(self.pos)
        self.pos += other.pos
        self.w += other.w
        self.mat += other.mat
        self.tris += [(a + base, b + base, c + base) for a, b, c in other.tris]
        return self

    def normals(self):
        acc = [(0.0, 0.0, 0.0)] * len(self.pos)
        for a, b, c in self.tris:
            n = cross(sub(self.pos[b], self.pos[a]), sub(self.pos[c], self.pos[a]))  # area-weighted
            for i in (a, b, c):
                acc[i] = add(acc[i], n)
        return [norm(n) if length(n) > 1e-12 else (0.0, 0.0, 1.0) for n in acc]

    def bones(self):
        return sorted({b for w in self.w for b, wt in w})

    def compact(self):
        """Drop vertices no triangle uses (an open head's crown)."""
        used = sorted({i for t in self.tris for i in t})
        remap = {old: new for new, old in enumerate(used)}
        self.pos = [self.pos[i] for i in used]
        self.w = [self.w[i] for i in used]
        self.mat = [self.mat[i] for i in used]
        self.tris = [(remap[a], remap[b], remap[c]) for a, b, c in self.tris]
        return self


def away_from(point):
    """An `out` for convex pieces: away from a centre point."""
    return lambda c: sub(c, point)


def away_from_axis(p0, p1):
    """An `out` for pieces round an axis (a lathe, a tube): away from the nearest axis point."""
    d = sub(p1, p0)
    dd = dot(d, d) or 1.0

    def f(c):
        t = max(0.0, min(1.0, dot(sub(c, p0), d) / dd))
        return sub(c, add(p0, mul(d, t)))
    return f


# ------------------------------------------------------------------------------ weights
def rigid(bone):
    return lambda p: [(bone, 1.0)]


def blend2(a, b, wa):
    wa = max(0.0, min(1.0, wa))
    if wa >= 0.999:
        return [(a, 1.0)]
    if wa <= 0.001:
        return [(b, 1.0)]
    return [(a, wa), (b, 1.0 - wa)]


def smoothstep(e0, e1, x):
    t = max(0.0, min(1.0, (x - e0) / (e1 - e0))) if e1 != e0 else (1.0 if x >= e1 else 0.0)
    return t * t * (3 - 2 * t)


def chain(skel, bones, blend=0.03):
    """Weights along a chain of consecutive bones (hips-spine-chest-neck, arm_up-arm_lo-hand):
    each vertex goes to the bone whose stretch of the chain is nearest along it, blending over
    `blend` metres either side of each joint."""
    pts = [skel.joints[bones[0]][0]] + [skel.joints[b][1] for b in bones]
    seg_len = [length(sub(pts[i + 1], pts[i])) for i in range(len(bones))]
    starts = [sum(seg_len[:i]) for i in range(len(bones))]

    def f(p):
        s, best = 0.0, None  # where along the chain the vertex is (its nearest segment)
        for i in range(len(bones)):
            a, b = pts[i], pts[i + 1]
            d = sub(b, a)
            t = max(0.0, min(1.0, dot(sub(p, a), d) / (dot(d, d) or 1.0)))
            gap = length(sub(p, add(a, mul(d, t))))
            if best is None or gap < best - 1e-9:
                best, s = gap, starts[i] + t * seg_len[i]
        for j in range(1, len(bones)):  # the first joint we are not past decides
            s0 = starts[j]
            if s <= s0 + blend:
                if s < s0 - blend:
                    return [(bones[j - 1], 1.0)]
                return blend2(bones[j - 1], bones[j], 1.0 - smoothstep(s0 - blend, s0 + blend, s))
        return [(bones[-1], 1.0)]
    return f


# ------------------------------------------------------------------------------ primitives
def ellipsoid(m, c, r, seg, rings, mat, wfn, frame=None):
    """A smooth ellipsoid: radii r = (rx, ry, rz) along the frame's axes (default world X, Y, Z),
    poles along the frame's Z. seg around, rings from pole to pole (2*seg*(rings-1) tris)."""
    fx, fy, fz = frame or ((1, 0, 0), (0, 1, 0), (0, 0, 1))

    def P(x, y, z):
        return add(c, add(add(mul(fx, x * r[0]), mul(fy, y * r[1])), mul(fz, z * r[2])))

    top = m.vert(P(0, 0, 1), wfn(P(0, 0, 1)), mat)
    rows = []
    for j in range(1, rings):
        el = math.pi / 2 - math.pi * j / rings
        row = []
        for k in range(seg):
            a = 2 * math.pi * k / seg
            p = P(math.sin(a) * math.cos(el), -math.cos(a) * math.cos(el), math.sin(el))
            row.append(m.vert(p, wfn(p), mat))
        rows.append(row)
    bot = m.vert(P(0, 0, -1), wfn(P(0, 0, -1)), mat)
    out = away_from(c)
    for k in range(seg):
        k1 = (k + 1) % seg
        m.tri(top, rows[0][k], rows[0][k1], out)
        m.tri(bot, rows[-1][k1], rows[-1][k], out)
        for a, b in zip(rows, rows[1:]):
            m.quad(a[k], b[k], b[k1], a[k1], out)
    return m


def ring_points(center, rx, ryf, ryb, seg, phase=0.0):
    """A horizontal ring (front = -Y first, going toward +X): separate front/back depths."""
    pts = []
    for k in range(seg):
        a = 2 * math.pi * (k + phase) / seg
        ca = math.cos(a)
        ry = ryf if ca > 0 else ryb
        pts.append((center[0] + rx * math.sin(a), center[1] - ry * ca, center[2]))
    return pts


def lathe(m, rings, seg, mat, wfn, cap_top=None, cap_bottom=None, phase=0.0, mats=None, cx=0.0, cap_mat=None):
    """Rings from top to bottom: (z, rx, ry_front, ry_back[, cy]) about the vertical axis.
    cap_top / cap_bottom: None (open) or the z of a fan's centre. mats: one material per band
    (a band's own vertices, so its colour edge is crisp). cx moves the axis off x = 0 (a bucket,
    a lantern); cap_mat paints the caps."""
    rings = [tuple(r) + (0.0,) * (5 - len(r)) for r in rings]
    rows = []
    for z, rx, ryf, ryb, cy in rings:
        rows.append(ring_points((cx, cy, z), rx, ryf, ryb, seg, phase))

    def out_for(band):
        """The profile runs over the surface from top to bottom, so a band's outward normal is
        (radial * -dz + up * dr): walls face out, a brim's top faces up and its underside down."""
        z0, rx0, ryf0, ryb0, cy0 = rings[band]
        z1, rx1, ryf1, ryb1, cy1 = rings[band + 1]
        dr = (rx1 + ryf1 + ryb1 - rx0 - ryf0 - ryb0) / 3.0
        dz = z1 - z0
        cy = 0.5 * (cy0 + cy1)

        def f(c):
            rad = norm((c[0] - cx, c[1] - cy, 0.0))
            return (rad[0] * -dz, rad[1] * -dz, dr)
        return f

    for band in range(len(rows) - 1):
        bm = mats[band] if mats else mat
        a = [m.vert(p, wfn(p), bm) for p in rows[band]]
        b = [m.vert(p, wfn(p), bm) for p in rows[band + 1]]
        for k in range(seg):
            k1 = (k + 1) % seg
            m.quad(a[k], b[k], b[k1], a[k1], out_for(band))
    for cap, row, r, up in ((cap_top, rows[0], rings[0], 1), (cap_bottom, rows[-1], rings[-1], -1)):
        if cap is None:
            continue
        cm = cap_mat or ((mats[0] if up > 0 else mats[-1]) if mats else mat)
        ring = [m.vert(p, wfn(p), cm) for p in row]
        cp = (cx, r[4], cap)
        c = m.vert(cp, wfn(cp), cm)
        for k in range(seg):
            m.tri(c, ring[k], ring[(k + 1) % seg], (0, 0, up))
    return m


def lathe_arc(m, rings, a0, a1, n, mat, wfn, outward=True):
    """An open panel of a lathe between angles a0..a1 (degrees; 0 = the front, -Y; +90 = +X) in
    n columns: an apron, a visor, a cloak's front. Rings (z, rx, ry_front, ry_back[, cy]) run
    top to bottom; outward=False faces it inward (a lining)."""
    rings = [tuple(r) + (0.0,) * (5 - len(r)) for r in rings]
    rows = []
    for z, rx, ryf, ryb, cy in rings:
        row = []
        for k in range(n + 1):
            a = math.radians(a0 + (a1 - a0) * k / n)
            ry = ryf if math.cos(a) > 0 else ryb
            p = (rx * math.sin(a), cy - ry * math.cos(a), z)
            row.append(m.vert(p, wfn(p), mat))
        rows.append(row)
    for band in range(len(rows) - 1):
        z0, rx0, ryf0, ryb0, cy0 = rings[band]
        z1, rx1, ryf1, ryb1, cy1 = rings[band + 1]
        dr, dz = (rx1 + ryf1 - rx0 - ryf0) / 2.0, z1 - z0
        cy = 0.5 * (cy0 + cy1)
        sgn = 1.0 if outward else -1.0

        def f(c, dr=dr, dz=dz, cy=cy, sgn=sgn):
            rad = norm((c[0], c[1] - cy, 0.0))
            return (sgn * rad[0] * -dz, sgn * rad[1] * -dz, sgn * dr)
        for k in range(n):
            m.quad(rows[band][k], rows[band + 1][k], rows[band + 1][k + 1], rows[band][k + 1], f)
    return m


def ribbon(m, pts, width, normal_fn, mat, wfn):
    """A flat strip through points, `width` wide, facing normal_fn(point) (a strap, a cord)."""
    left, right = [], []
    for i, p in enumerate(pts):
        d = norm(sub(pts[min(i + 1, len(pts) - 1)], pts[max(i - 1, 0)]))
        nrm = normal_fn(p)
        side = norm(cross(d, nrm))
        a, b = add(p, mul(side, width / 2)), sub(p, mul(side, width / 2))
        left.append(m.vert(a, wfn(a), mat))
        right.append(m.vert(b, wfn(b), mat))
    for i in range(len(pts) - 1):
        nrm = normal_fn(pts[i])
        m.quad(left[i], right[i], right[i + 1], left[i + 1], nrm)
    return m


def annulus(m, c, normal, up, r_out, r_in, seg, mat, wfn, both=False):
    """A flat ring facing `normal` (a spectacle rim); both=True adds the back face."""
    n = norm(normal)
    right = norm(cross(up, n))
    u = norm(cross(n, right))
    outer, inner = [], []
    for k in range(seg):
        a = 2 * math.pi * k / seg
        d = add(mul(right, math.cos(a)), mul(u, math.sin(a)))
        po, pi = add(c, mul(d, r_out)), add(c, mul(d, r_in))
        outer.append(m.vert(po, wfn(po), mat))
        inner.append(m.vert(pi, wfn(pi), mat))
    for k in range(seg):
        k1 = (k + 1) % seg
        m.quad(outer[k], outer[k1], inner[k1], inner[k], n)
        if both:
            m.quad(outer[k], outer[k1], inner[k1], inner[k], mul(n, -1.0))
    return m


def slab(m, c, ax, ay, az, size, mat, wfn):
    """A box centred at c with half-extents size along the axes ax, ay, az (a clipboard)."""
    sx, sy, sz = size
    corners = {}
    for i in (-1, 1):
        for j in (-1, 1):
            for k in (-1, 1):
                corners[(i, j, k)] = add(c, add(add(mul(ax, i * sx), mul(ay, j * sy)), mul(az, k * sz)))
    faces = [((1, -1, -1), (1, 1, -1), (1, 1, 1), (1, -1, 1)), ((-1, -1, -1), (-1, -1, 1), (-1, 1, 1), (-1, 1, -1)),
             ((-1, 1, -1), (-1, 1, 1), (1, 1, 1), (1, 1, -1)), ((-1, -1, -1), (1, -1, -1), (1, -1, 1), (-1, -1, 1)),
             ((-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)), ((-1, -1, -1), (-1, 1, -1), (1, 1, -1), (1, -1, -1))]
    for f in faces:  # each face its own vertices: crisp edges
        ids = [m.vert(corners[k], wfn(corners[k]), mat) for k in f]
        m.quad(*ids, away_from(c))
    return m


def frame_along(d, up_hint=(0.0, 0.0, 1.0)):
    d = norm(d)
    a = up_hint if abs(dot(d, up_hint)) < 0.95 else (1.0, 0.0, 0.0)
    s = norm(cross(d, a))
    u = norm(cross(s, d))
    return s, u


def tube(m, pts, radii, seg, mat, wfn, cap0=False, cap1=False, up_hint=(0.0, 0.0, 1.0), tip1=None,
         tip0=None, twist=0.0):
    """A tube through points with a radius (or (rx, ry)) at each; rings transported along the
    path. cap0/cap1 close an end flat; tip0/tip1 close it with a point this far past the end."""
    n = len(pts)
    rows = []
    s_prev = None
    for i, p in enumerate(pts):
        d = sub(pts[min(i + 1, n - 1)], pts[max(i - 1, 0)])
        if s_prev is None:
            s, u = frame_along(d, up_hint)
        else:  # parallel transport: keep the side vector as steady as the path allows
            dn = norm(d)
            s = norm(sub(s_prev, mul(dn, dot(s_prev, dn))))
            u = norm(cross(s, dn))
        s_prev = s
        r = radii[i]
        rx, ry = (r, r) if not isinstance(r, (tuple, list)) else r
        row = []
        for k in range(seg):
            a = 2 * math.pi * k / seg + twist
            q = add(p, add(mul(s, math.cos(a) * rx), mul(u, math.sin(a) * ry)))
            row.append(m.vert(q, wfn(q), mat))
        rows.append(row)
    for i in range(n - 1):
        out = away_from_axis(pts[i], pts[i + 1])
        for k in range(seg):
            k1 = (k + 1) % seg
            m.quad(rows[i][k], rows[i + 1][k], rows[i + 1][k1], rows[i][k1], out)
    for flag, tip, i, j in ((cap0, tip0, 0, 1), (cap1, tip1, n - 1, n - 2)):
        if not flag and tip is None:
            continue
        d = norm(sub(pts[i], pts[j]))
        cp = add(pts[i], mul(d, tip or 0.0))
        c = m.vert(cp, wfn(cp), mat)
        for k in range(seg):
            m.tri(c, rows[i][k], rows[i][(k + 1) % seg], d if tip is None else away_from(pts[i]))
    return m


# ------------------------------------------------------------------------------ surfaces
class Ellipsoid:
    """An egg-ish ellipsoid surface (the head): half-axes per side:
    x (left/right, below and above the centre), y front/back, z top/bottom."""

    def __init__(self, c, ax_top, ax_bot, ay_front, ay_back, az_top, az_bot):
        self.c = c
        self.ax_top, self.ax_bot, self.ayf, self.ayb, self.azt, self.azb = ax_top, ax_bot, ay_front, ay_back, az_top, az_bot

    def axes(self, d):
        ax = self.ax_top if d[2] > 0 else self.ax_bot
        return ax, (self.ayf if d[1] < 0 else self.ayb), (self.azt if d[2] > 0 else self.azb)

    def radius(self, d):
        a, b, c = self.axes(d)
        return 1.0 / math.sqrt((d[0] / a) ** 2 + (d[1] / b) ** 2 + (d[2] / c) ** 2)

    @staticmethod
    def dir(az, el):
        """Degrees: az 0 = the front (-Y), +90 = +X; el up from the equator."""
        a, e = math.radians(az), math.radians(el)
        return (math.sin(a) * math.cos(e), -math.cos(a) * math.cos(e), math.sin(e))

    def point(self, d, extra=0.0):
        d = norm(d)
        return add(self.c, mul(d, self.radius(d) + extra))

    def at(self, az, el, extra=0.0):
        return self.point(self.dir(az, el), extra)

    def normal(self, p):
        q = sub(p, self.c)
        a, b, c = self.axes(norm(q))
        return norm((q[0] / a ** 2, q[1] / b ** 2, q[2] / c ** 2))

    def project(self, p, extra=0.0):
        return self.point(sub(p, self.c), extra)


def surface_mesh(m, surf, seg, rings, mat, wfn, extra=0.0, open_top=0, els=None):
    """The whole surface as a UV sphere (the head): rows at `els` (degrees, top to bottom) or
    evenly spaced. open_top leaves out the crown's fan and the next open_top - 1 bands (under
    the player's hair, which always covers them). The quads are split mirror-symmetrically
    about the face (az 0), so the toon ramp's edge makes a V on the face, not a slash."""
    els = els or [90.0 - 180.0 * j / rings for j in range(1, rings)]
    top = m.vert(surf.at(0, 90, extra), wfn(surf.at(0, 90)), mat)
    rows = []
    for el in els:
        rows.append([m.vert(surf.at(360.0 * k / seg, el, extra), wfn(surf.at(360.0 * k / seg, el)), mat)
                     for k in range(seg)])
    bot = m.vert(surf.at(0, -90, extra), wfn(surf.at(0, -90)), mat)
    out = away_from(surf.c)
    for k in range(seg):
        k1 = (k + 1) % seg
        if not open_top:
            m.tri(top, rows[0][k], rows[0][k1], out)
        m.tri(bot, rows[-1][k1], rows[-1][k], out)
        for j, (a, b) in enumerate(zip(rows, rows[1:])):
            if j >= open_top - 1:
                if k < seg // 2:  # the +X half
                    m.quad(a[k], b[k], b[k1], a[k1], out)
                else:             # the mirror half: split along the other diagonal
                    m.quad(a[k1], a[k], b[k], b[k1], out)
    return m


def decal(m, surf, az, el, rx, ry, depth, seg, mat, wfn, rings=1, lift=0.002, tilt=0.0, base=None,
          shape=None):
    """A dome lying on a surface (eyes, a nose, blush, ears): an ellipse rx x ry in the surface's
    tangent plane at (az, el), projected onto it, rising `depth` at the centre. base(u, v) -> an
    extra height under the dome (to sit on another dome); tilt rotates it in the plane (degrees);
    shape(angle) scales the rim radius (a leaf, a tear drop)."""
    c = surf.at(az, el)
    n = surf.normal(c)
    up = norm(sub((0.0, 0.0, 1.0), mul(n, n[2]))) if abs(n[2]) < 0.99 else (0.0, 1.0, 0.0)
    right = norm(cross(up, n))
    ta = math.radians(tilt)
    right, up = (add(mul(right, math.cos(ta)), mul(up, math.sin(ta))),
                 add(mul(up, math.cos(ta)), mul(right, -math.sin(ta))))
    base = base or (lambda u, v: 0.0)

    def place(u, v, h):
        q = surf.project(add(c, add(mul(right, u), mul(up, v))))
        return add(q, mul(surf.normal(q), lift + base(u, v) + h))

    rows = []
    for j in range(rings):
        f = 1.0 - j / rings  # 1 = the rim
        h = depth * math.sqrt(max(0.0, 1.0 - f * f))
        row = []
        for k in range(seg):
            a = 2 * math.pi * k / seg
            s = shape(a) if shape else 1.0
            p = place(math.cos(a) * rx * f * s, math.sin(a) * ry * f * s, h)
            row.append(m.vert(p, wfn(p), mat))
        rows.append(row)
    pc = place(0.0, 0.0, depth)
    pole = m.vert(pc, wfn(pc), mat)
    for a, b in zip(rows, rows[1:]):
        for k in range(seg):
            k1 = (k + 1) % seg
            m.quad(a[k], b[k], b[k1], a[k1], n)
    for k in range(seg):
        m.tri(rows[-1][k], rows[-1][(k + 1) % seg], pole, n)
    return c, n, right, up


def strip(m, surf, pts_uv, az, el, thick, mat, wfn, lift=0.003, depth=0.002):
    """A thin curved stroke on a surface (a smile, a brow): pts_uv are (u, v) offsets in metres in
    the tangent plane at (az, el), thick the stroke's width at each point."""
    c = surf.at(az, el)
    n = surf.normal(c)
    up = norm(sub((0.0, 0.0, 1.0), mul(n, n[2])))
    right = norm(cross(up, n))

    def place(u, v, h):
        q = surf.project(add(c, add(mul(right, u), mul(up, v))))
        return add(q, mul(surf.normal(q), lift + h))

    top, bot = [], []
    for i, (u, v) in enumerate(pts_uv):
        j0, j1 = max(i - 1, 0), min(i + 1, len(pts_uv) - 1)
        du, dv = pts_uv[j1][0] - pts_uv[j0][0], pts_uv[j1][1] - pts_uv[j0][1]
        L = math.hypot(du, dv) or 1.0
        nu, nv = -dv / L, du / L  # the stroke's normal in the plane
        t = thick[i] / 2
        p1 = place(u + nu * t, v + nv * t, depth)
        p2 = place(u - nu * t, v - nv * t, depth)
        top.append(m.vert(p1, wfn(p1), mat))
        bot.append(m.vert(p2, wfn(p2), mat))
    for i in range(len(pts_uv) - 1):
        m.quad(top[i], bot[i], bot[i + 1], top[i + 1], n)
    return m
