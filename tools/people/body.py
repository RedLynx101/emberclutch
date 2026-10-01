"""The people's shared body parts: skeleton layout, head and face, eyes, arms, legs, shoes,
garments. People (people.py) put these together with their own clothes and props.

Everything is in metres, Z up, facing -Y; x > 0 is the "_R" side (see rig.py).
"""
import math

from geom import (Ellipsoid, Mesh, blend2, chain, decal, ellipsoid, lathe, rigid, smoothstep, strip,
                  surface_mesh, tube)
from rig import Skeleton, add, lerp, mul, norm, sub

SIDES = (("L", -1.0), ("R", 1.0))


class Person:
    """A built person: skeleton, the body mesh (one skinned draw), the face's variants (faces.py:
    eyes part group 0, mouths 11, brows 12) and, for the player, six hair meshes (part group 10)."""

    def __init__(self, pid, joints, head, head_k=1.0):
        self.id = pid
        self.joints = joints
        self.skel = Skeleton(joints)
        self.head = head
        self.head_k = head_k  # the head's size against the old standard one (faces scale with it)
        self.body = Mesh()
        self.eyes = [Mesh() for _ in range(10)]
        self.mouths = [Mesh() for _ in range(10)]
        self.brows = [Mesh() for _ in range(4)]
        self.hair = []
        self.notes = {}


# ------------------------------------------------------------------------------ skeleton
def layout(leg=0.315, torso=1.0, width=1.0, arm=1.0, stoop=0.0, head_scale=1.0, head_shape=None,
           neck=0.085, hip_width=1.0):
    """Joints for a person standing at ease (arms hanging a little out). leg = the hip joints'
    height; torso, width, arm scale those parts; stoop leans the upper body forward (metres at
    the head); head_shape = (ax_top, ax_bot, ay_front, ay_back, az_top, az_bot) before head_scale."""
    hz = leg
    z_hips = hz + 0.045 * torso
    z_spine = z_hips + 0.11 * torso
    z_chest = z_spine + 0.115 * torso
    z_neck = z_chest + 0.115 * torso
    z_headb = z_neck + neck * torso

    def lean(z):  # how far forward (-Y) the stoop carries a point at height z
        if stoop <= 0 or z <= z_spine:
            return 0.0
        f = min(1.0, (z - z_spine) / (z_headb - z_spine))
        return -stoop * f * f

    def P(x, y, z):
        return (x, y + lean(z), z)

    j = {}
    j["hips"] = (P(0, 0, z_hips), P(0, 0, z_spine))
    j["spine"] = (P(0, 0, z_spine), P(0, 0, z_chest))
    j["chest"] = (P(0, 0, z_chest), P(0, 0, z_neck))
    j["neck"] = (P(0, 0, z_neck), P(0, 0.005, z_headb))
    sx, sz = 0.105 * width, z_chest + 0.07 * torso
    for side, s in SIDES:
        sh = P(s * sx, 0.0, sz)
        el = (sh[0] + s * 0.05 * arm * width ** 0.5, sh[1] + 0.006, sh[2] - 0.115 * arm)
        wr = (el[0] + s * 0.022 * arm, el[1] - 0.016, el[2] - 0.104 * arm)
        tip = (wr[0] + s * 0.008, wr[1] - 0.012, wr[2] - 0.055 * arm)
        j[f"arm_up_{side}"] = (sh, el)
        j[f"arm_lo_{side}"] = (el, wr)
        j[f"hand_{side}"] = (wr, tip)
        hx = 0.064 * hip_width
        hip = (s * hx, 0.0, hz)
        knee = (s * (hx + 0.004), -0.004, 0.075 + (hz - 0.075) * 0.48)
        ankle = (s * (hx + 0.006), 0.0, 0.075)
        toe = (s * (hx + 0.006), -0.075, 0.025)
        j[f"leg_up_{side}"] = (hip, knee)
        j[f"leg_lo_{side}"] = (knee, ankle)
        j[f"foot_{side}"] = (ankle, toe)
    ax_t, ax_b, ayf, ayb, azt, azb = [v * head_scale for v in (head_shape or (0.262, 0.27, 0.232, 0.25, 0.265, 0.245))]
    hb = j["neck"][1]
    head = Ellipsoid((0.0, hb[1] + 0.0, z_headb + azb - 0.02), ax_t, ax_b, ayf, ayb, azt, azb)
    j["head"] = (hb, (hb[0], hb[1], z_headb + azb + azt))
    return j, head


def finish_eyes_bone(joints, head, at):
    """The eyes bone: from between the eyes, pointing forward (-Y) exactly, so its local Z is
    world up and the game's blink (scale z) shuts the eyes toward their middle."""
    pl, pr = head.at(-at[0], at[1]), head.at(at[0], at[1])
    c = (0.0, 0.5 * (pl[1] + pr[1]), 0.5 * (pl[2] + pr[2]))
    joints["eyes"] = (c, (0.0, c[1] - 0.1, c[2]))
    return joints


def person(pid, eyes_at=(21.0, -7.0), **kw):
    joints, head = layout(**kw)
    finish_eyes_bone(joints, head, eyes_at)
    p = Person(pid, joints, head, kw.get("head_scale", 1.0))
    p.eyes_at = eyes_at
    # where a rider sits: under the hips joint, at the bottom of the seat (set per body)
    p.notes["seat"] = (0.0, 0.02, joints["leg_up_R"][0][2] + 0.015)
    return p


# ------------------------------------------------------------------------------ head & face
HEAD_ROWS = [60, 32, 8, -16, -40, -64]  # finer over the face and cheeks, where the light falls


def head_and_face(p, mat="skin", seg=10, ears="round", nose=True, blush=True, mouth=True,
                  brows=None, brow_mat="hair", freckles=False, open_top=0, rows=None):
    """The head (a smooth egg), ears, a button nose, blush, a small smile, brows if asked.
    open_top=2 leaves out the crown (under hair or a hat that always covers it)."""
    m, h = p.body, p.head
    w = rigid("head")
    surface_mesh(m, h, seg, 0, mat, w, open_top=open_top, els=rows or HEAD_ROWS)
    if ears == "round":
        for s in (-1, 1):
            decal(m, h, s * 91, -9, 0.037, 0.052, 0.03, 7, mat, w, lift=-0.004)
    elif ears == "pointed":  # a leaf lying back along the head, its tip standing off it
        for s in (-1, 1):
            inside = h.at(s * 88, -12, -0.025)
            out = add(h.at(s * 98, 2, 0.006), (s * 0.02, 0.0, 0.0))
            tube(m, [inside, out], [(0.034, 0.012), (0.03, 0.011)], 5, mat, w, tip1=0.075,
                 up_hint=(s * 1.0, 0.0, 0.0))
    if nose:
        decal(m, h, 0, -17, 0.017, 0.013, 0.011, 6, "nose", w)
    if blush:
        for s in (-1, 1):
            decal(m, h, s * 35, -21, 0.03, 0.017, 0.002, 4, "blush", w, lift=0.0012)
    if mouth:
        smile(m, h, w)
    if brows:
        for s in (-1, 1):
            brow(m, h, s, w, brow_mat, **brows)  # n=3 points: a 4-triangle brow
    if freckles:
        for s in (-1, 1):
            for dx, dy in ((0, 0), (0.018, 0.006), (-0.012, 0.01)):
                decal(m, h, s * (33 + dx * 90), -18 + dy * 90, 0.0045, 0.0045, 0.001, 4, "nose", w, lift=0.0025)


def smile(m, h, w, el=-29.5, width=0.052, curve=0.011, thick=0.0085, mat="mouth"):
    n = 4
    pts, th = [], []
    for i in range(n):
        u = -width / 2 + width * i / (n - 1)
        f = u / (width / 2)
        pts.append((u, curve * f * f))
        th.append(thick * (1 - 0.55 * f * f))
    strip(m, h, pts, 0, el, th, mat, w, lift=0.0015, depth=0.0015)


def brow(m, h, s, w, mat, az=21.0, el=10.5, width=0.05, arch=0.008, thick=0.011, tilt=0.0, n=4):
    pts, th = [], []
    for i in range(n):
        f = -1 + 2 * i / (n - 1)
        pts.append((f * width / 2, arch * (1 - f * f) + tilt * f * s))
        th.append(thick * (1 - 0.35 * f * f))
    strip(m, h, pts, s * az, el, th, mat, w, lift=0.002, depth=0.002)


def build_eyes(p, size=(0.04, 0.055), depth=0.009, iris_seg=10, pupil=(0.72, 0.74), lashes=False):
    """Two variants: 0 calm (big pupils), 1 surprised (small pupils). Both glints sit on the
    same side (upper right as you look), the storybook way."""
    h = p.head
    rx, ry = size
    w = rigid("eyes")
    iris_lift = 0.0015

    def iris_h(u, v):
        r = math.sqrt((u / rx) ** 2 + (v / ry) ** 2)
        return depth * max(0.0, 1 - r)

    for variant, (pxs, pys) in enumerate((pupil, (0.36, 0.38))):
        m = p.eyes[variant]
        for s in (-1, 1):
            az, el = s * p.eyes_at[0], p.eyes_at[1]
            decal(m, h, az, el, rx, ry, depth, iris_seg, "iris", w, lift=iris_lift)
            prx, pry = rx * pxs, ry * pys
            dv = -0.1 * ry if variant == 0 else 0.0  # calm pupils sit a touch low
            pd = 0.0025
            _decal_at(m, h, az, el, 0.0, dv, prx, pry, pd, 7, "pupil", w,
                      base=lambda u, v: iris_h(u, v) + 0.0006, lift=iris_lift)

            def glint_base(u, v, prx=prx, pry=pry, dv=dv):
                r = math.sqrt((u / prx) ** 2 + ((v - dv) / pry) ** 2)
                return iris_h(u, v) + (pd * math.sqrt(max(0.0, 1 - r * r)) if r < 1 else 0.0) + 0.0012

            for gu, gv, gr, gs in ((0.34, 0.36, 0.3, 5), (-0.3, -0.34, 0.14, 4)):
                gw, gh = rx * gr, rx * gr * 1.05
                _decal_at(m, h, az, el, gu * rx, gv * ry, gw, gh, 0.0015, gs, "glint", w,
                          base=lambda u, v: glint_base(u, v), lift=iris_lift)
        if lashes:
            for s in (-1, 1):
                pts = [(-rx * 0.95 + rx * 1.9 * i / 4, ry * 0.9 * math.sqrt(max(0.0, 1 - ((-0.95 + 1.9 * i / 4)) ** 2)))
                       for i in range(5)]
                strip(m, h, pts, s * p.eyes_at[0], p.eyes_at[1], [0.006] * 5, "pupil", w, lift=0.003)


def _decal_at(m, h, az, el, du, dv, rx, ry, depth, seg, mat, w, base, lift):
    """A decal centred (du, dv) metres from (az, el) in its tangent plane."""
    c = h.at(az, el)
    n = h.normal(c)
    up = norm(sub((0.0, 0.0, 1.0), mul(n, n[2])))
    right = norm((up[1] * n[2] - up[2] * n[1], up[2] * n[0] - up[0] * n[2], up[0] * n[1] - up[1] * n[0]))
    q = h.project(add(c, add(mul(right, du), mul(up, dv))))
    d = sub(q, h.c)
    az2 = math.degrees(math.atan2(d[0], -d[1]))
    el2 = math.degrees(math.asin(max(-1.0, min(1.0, d[2] / math.sqrt(sum(x * x for x in d))))))
    decal(m, h, az2, el2, rx, ry, depth, seg, mat, w, lift=lift, base=lambda u, v: base(u + du, v + dv))


# ------------------------------------------------------------------------------ torso
def torso_weights(p, blend=0.035):
    return chain(p.skel, ["hips", "spine", "chest", "neck"], blend)


def skirt_weights(p, top_z, bottom_z, follow=0.75, blend=0.035):
    """A skirt, coat or cloak: the torso chain above top_z; below it, blending toward the leg
    on that side (so a leg swinging forward carries the hem with it)."""
    base = torso_weights(p, blend)
    hx = abs(p.joints["leg_up_R"][0][0]) or 0.06

    def f(q):
        if q[2] >= top_z:
            return base(q)
        s = smoothstep(top_z, bottom_z, q[2])
        side = "R" if q[0] >= 0 else "L"
        lateral = min(1.0, abs(q[0]) / (hx * 1.6))
        wl = s * follow * (0.45 + 0.55 * lateral)
        return blend2("hips", f"leg_up_{side}", 1.0 - wl)
    return f


def band(p, z0, z1, rx0, ry0, rx1, ry1, mat, seg=8, cy=0.0, w=None):
    """A ring band (a belt, a hem trim) from z0 (top) to z1."""
    return lathe(p.body, [(z0, rx0, ry0, ry0, cy), (z1, rx1, ry1, ry1, cy)], seg, mat, w or torso_weights(p))


# ------------------------------------------------------------------------------ limbs
def arm(p, side, sleeve_mat="outfit", sleeve_r=0.058, sleeve_len=0.62, forearm_mat="skin",
        forearm_r=0.038, cuff=None, long_sleeve=False, hand_mat="skin", hand_r=0.048, seg=6):
    """An arm: a capsule sleeve pivoting at the shoulder, a forearm, a round hand.
    sleeve_len: how far down the upper arm the sleeve reaches (0-1+); long_sleeve runs the
    sleeve's material down to the wrist; cuff = (material, radius) adds a cuff band."""
    j = p.joints
    sh, el = j[f"arm_up_{side}"]
    wr, tip = j[f"hand_{side}"]
    m = p.body
    wa = chain(p.skel, [f"arm_up_{side}", f"arm_lo_{side}"], 0.022)
    s_end = lerp(sh, el, sleeve_len)
    # the sleeve: a round-topped capsule on the shoulder joint (it swings without a gap)
    tube(m, [sh, s_end], [sleeve_r * 0.98, sleeve_r * 1.04], seg, sleeve_mat, rigid(f"arm_up_{side}"),
         tip0=sleeve_r * 0.95)
    if long_sleeve:
        f0 = lerp(sh, el, sleeve_len - 0.12)
        tube(m, [f0, el, wr], [sleeve_r * 0.8, sleeve_r * 0.8, sleeve_r * 0.8], seg, sleeve_mat, wa)
    else:
        f0 = lerp(sh, el, max(0.2, sleeve_len - 0.2))
        tube(m, [f0, el, wr], [forearm_r, forearm_r, forearm_r * 0.92], seg, forearm_mat, wa)
    if cuff:
        cm, cr = cuff
        c0, c1 = lerp(el, wr, 0.62), lerp(el, wr, 0.86)
        tube(m, [c0, c1], [cr, cr * 1.04], seg, cm, rigid(f"arm_lo_{side}"))
    hand(p, side, hand_mat, hand_r)


def hand(p, side, mat="skin", r=0.048):
    wr, tip = p.joints[f"hand_{side}"]
    c = lerp(wr, tip, 0.35)
    ellipsoid(p.body, c, (r * 0.95, r * 0.98, r * 1.08), 5, 3, mat, rigid(f"hand_{side}"))


def leg(p, side, mat="leather_dark", r=0.056, top=0.06, bottom_z=0.1, seg=6, knee_r=None):
    """A leg from inside the body down into the shoe, bending at the knee."""
    hip, knee = p.joints[f"leg_up_{side}"]
    ankle = p.joints[f"leg_lo_{side}"][1]
    t = (hip[0], hip[1], hip[2] + top)
    b = lerp(knee, ankle, (knee[2] - bottom_z) / max(1e-6, knee[2] - ankle[2]))
    w = chain(p.skel, [f"leg_up_{side}", f"leg_lo_{side}"], 0.028)
    tube(p.body, [t, knee, b], [r, knee_r or r * 0.92, r * 0.86], seg, mat, w)


def shin(p, side, mat="leather_dark", r=0.05, seg=6):
    """Just the lower leg (under a long coat or dress, the thigh never shows)."""
    knee, ankle = p.joints[f"leg_lo_{side}"]
    w = chain(p.skel, [f"leg_up_{side}", f"leg_lo_{side}"], 0.02)
    tube(p.body, [lerp(knee, p.joints[f"leg_up_{side}"][0], 0.15), (ankle[0], ankle[1], 0.1)], [r, r * 0.9],
         seg, mat, w)


def spine_y(p, z):
    """The spine's y at height z (a stooped person's garments lean with it)."""
    pts = [p.joints["hips"][0], p.joints["spine"][0], p.joints["chest"][0], p.joints["neck"][0],
           p.joints["neck"][1], p.joints["head"][1]]
    if z <= pts[0][2]:
        return pts[0][1]
    for a, b in zip(pts, pts[1:]):
        if a[2] <= z <= b[2]:
            t = (z - a[2]) / max(1e-6, b[2] - a[2])
            return a[1] + (b[1] - a[1]) * t
    return pts[-1][1]


def profile_at(profile, z):
    """Interpolate a lathe profile [(z, rx, ryf, ryb), ...] (top to bottom) at height z."""
    for a, b in zip(profile, profile[1:]):
        if a[0] >= z >= b[0]:
            t = (a[0] - z) / (a[0] - b[0])
            return tuple(a[i] + (b[i] - a[i]) * t for i in range(1, 4))
    r = profile[0] if z > profile[0][0] else profile[-1]
    return r[1], r[2], r[3]


def garment_point(profile, z, az, off=0.0, cy=0.0):
    """A point on a lathe garment's ideal surface at height z, angle az (0 = the front, +90 =
    +X), `off` metres proud of it (straps, cords, badges)."""
    rx, ryf, ryb = profile_at(profile, z)
    a = math.radians(az)
    ry = ryf if math.cos(a) > 0 else ryb
    x, y = (rx + off) * math.sin(a), -(ry + off) * math.cos(a)
    return (x, cy + y, z)


def shoe(p, side, mat="leather", size=(0.062, 0.092, 0.056), shaft=None, seg=6, rings=3):
    """A rounded shoe (or a boot with shaft = (height, radius, material))."""
    ankle, toe = p.joints[f"foot_{side}"]
    rx, ry, rz = size
    c = (ankle[0], ankle[1] - 0.022, rz)
    ellipsoid(p.body, c, size, seg, rings, mat, rigid(f"foot_{side}"))
    if shaft:
        hgt, r, smat = shaft
        z0 = rz * 1.1
        b0 = (ankle[0], ankle[1] + 0.004, z0)
        b1 = (ankle[0], ankle[1] + 0.002, z0 + hgt)
        w = chain(p.skel, [f"leg_lo_{side}", f"foot_{side}"], 0.02)
        tube(p.body, [b1, b0], [r * 0.92, r], seg, smat, lambda q, side=side: blend2(
            f"leg_lo_{side}", f"foot_{side}", smoothstep(z0, z0 + hgt * 0.7, q[2])))


def rigid_to(bone):
    return rigid(bone)
