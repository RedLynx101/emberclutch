"""The people: your character's two body shapes and the six villagers, built from body.py's
parts. Each builder returns a body.Person (skeleton, body mesh, eyes, hair).

Every body shares one skeleton (rig.BONES) with its own proportions. Clothes and props are in
the body mesh (one skinned draw of 17 bones); the eyes are part group 0 and the player's hair
part group 10. Props are held in hand_L (at -X: the person's own right hand).
"""
from body import (SIDES, arm, build_eyes, garment_point, head_and_face, leg, person, profile_at, shin, shoe,
                  skirt_weights, spine_y, torso_weights)
from geom import (annulus, decal, ellipsoid, lathe, lathe_arc, ribbon, rigid, slab, tube)
from hair import angle_blend, beard, fringe, player_hair, shell
from rig import add, lerp, mul, norm, sub

HEAD = rigid("head")


def hug(p, profile, z0, z1, off, mat, seg=8, w=None, cy=None):
    """A band hugging a garment's profile from z0 down to z1, `off` metres proud of it
    (belts, hem trims); cy(z) follows a stooped garment."""
    r0, r1 = profile_at(profile, z0), profile_at(profile, z1)
    c0, c1 = (cy(z0), cy(z1)) if cy else (0.0, 0.0)
    lathe(p.body, [(z0, r0[0] + off, r0[1] + off, r0[2] + off, c0), (z1, r1[0] + off, r1[1] + off, r1[2] + off, c1)],
          seg, mat, w or torso_weights(p))


def hand_c(p, side):
    wr, tip = p.joints[f"hand_{side}"]
    return lerp(wr, tip, 0.35)


def seat_under(p, z):
    p.notes["seat"] = (0.0, 0.02, z)


# ------------------------------------------------------------------------------ your character
TUNIC_A = [(0.705, 0.1, 0.085, 0.086), (0.648, 0.142, 0.108, 0.11), (0.52, 0.132, 0.112, 0.114),
           (0.392, 0.153, 0.128, 0.131)]


def player_a():
    """Body A: a belted tunic and trousers, a round scarf, boots (the concept's adventurer)."""
    p = person("player_a")
    head_and_face(p, open_top=2)
    build_eyes(p)
    tw = torso_weights(p)
    lathe(p.body, TUNIC_A, 8, "outfit", tw)
    lathe(p.body, [(0.40, 0.142, 0.118, 0.121)], 8, "leather_dark", tw, cap_bottom=0.335)
    hug(p, TUNIC_A, 0.468, 0.43, 0.006, "leather")
    lathe(p.body, [(0.782, 0.06, 0.055, 0.055), (0.724, 0.12, 0.105, 0.106), (0.672, 0.104, 0.09, 0.092)], 8,
          "trim", tw)
    for side, s in SIDES:
        arm(p, side, sleeve_mat="outfit")
        leg(p, side, "leather_dark")
        shoe(p, side, "leather")
    p.hair = player_hair(p.head)
    seat_under(p, 0.33)
    return p


DRESS_B = [(0.705, 0.095, 0.081, 0.082), (0.645, 0.13, 0.1, 0.102), (0.52, 0.122, 0.106, 0.108),
           (0.27, 0.192, 0.168, 0.172)]


def player_b():
    """Body B: an A-line tunic-dress with a trimmed hem over leggings, a round collar, ankle boots."""
    p = person("player_b", width=0.94)
    head_and_face(p, open_top=2)
    build_eyes(p)
    sw = skirt_weights(p, 0.43, 0.27)
    lathe(p.body, DRESS_B, 8, "outfit", sw)
    lathe(p.body, [(0.28, 0.176, 0.152, 0.156)], 8, "leather_dark", sw, cap_bottom=0.33)
    hug(p, DRESS_B, 0.296, 0.27, 0.005, "trim", w=sw)
    lathe(p.body, [(0.786, 0.056, 0.052, 0.052), (0.712, 0.122, 0.104, 0.104), (0.694, 0.114, 0.096, 0.097)], 8,
          "trim", torso_weights(p))
    for side, s in SIDES:
        arm(p, side, sleeve_mat="outfit", sleeve_r=0.062)
        leg(p, side, "leather_dark", r=0.052)
        shoe(p, side, "leather", size=(0.058, 0.088, 0.054))
    p.hair = player_hair(p.head)
    seat_under(p, 0.33)
    return p


# ------------------------------------------------------------------------------ the villagers
COAT_K = [(0.72, 0.085, 0.075, 0.075), (0.62, 0.15, 0.118, 0.124), (0.48, 0.152, 0.128, 0.134),
          (0.3, 0.172, 0.15, 0.156), (0.1, 0.212, 0.19, 0.198)]


def keeper():
    """The old dragon keeper, your guide: a little stooped, white hair swept back, bushy brows,
    a big white beard, round brass spectacles, a long teal coat with gold hems, a crooked staff."""
    p = person("keeper", leg=0.29, torso=0.95, stoop=0.05, eyes_at=(21.0, -8.0))
    h, m = p.head, p.body
    head_and_face(p, open_top=1, mouth=False, nose=False, ears=None,
                  brows=dict(el=13.0, width=0.066, thick=0.018, arch=0.012, n=3))
    decal(m, h, 0, -20, 0.026, 0.021, 0.02, 6, "nose", HEAD)  # a round, kindly nose
    build_eyes(p, size=(0.034, 0.045))
    # a receding hairline, white tufts fluffing out over the ears (they hide them)
    shell(m, h, lambda az: angle_blend(az, {0: 58, 40: 46, 80: 8, 100: -8, 150: -34, 180: -40}),
          lambda az, f: 0.016 + 0.018 * (1 - f) + (0.04 if 70 < abs(az) < 140 else 0.012) * f,
          n_az=10, rows=2, locks=lambda k, az: 12 if (k % 2 == 0 and abs(az) > 60) else 0)
    beard(m, h, lambda az: angle_blend(az, {0: -23, 30: -24, 60: -24, 100: -26}),
          lambda az, f: 0.03 + 0.03 * (1 - f), 0.12, n=7, rows=3)
    for s in (-1, 1):  # spectacles, standing off the face
        c = h.at(s * 21, -8)
        n = h.normal(c)
        annulus(m, add(c, mul(n, 0.022)), n, (0.0, 0.0, 1.0), 0.05, 0.041, 7, "brass", HEAD)
    cl, cr = add(h.at(-21, -8), mul(h.normal(h.at(-21, -8)), 0.024)), add(h.at(21, -8), mul(h.normal(h.at(21, -8)), 0.024))
    ribbon(m, [add(cl, (0.046, 0.0, 0.008)), add(cr, (-0.046, 0.0, 0.008))], 0.008, lambda q: (0.0, -1.0, 0.0),
           "brass", HEAD)
    cy = lambda z: spine_y(p, z)  # noqa: E731 (the coat leans with the stoop)
    lathe(m, [r + (cy(r[0]),) for r in COAT_K], 8, "outfit", skirt_weights(p, 0.36, 0.1))
    hug(p, COAT_K, 0.128, 0.1, 0.006, "trim", w=skirt_weights(p, 0.36, 0.1), cy=cy)
    for side, s in SIDES:
        arm(p, side, sleeve_mat="outfit", long_sleeve=True, sleeve_r=0.058)
        shin(p, side, "leather_dark", r=0.048)
        shoe(p, side, "leather", size=(0.06, 0.09, 0.052))
    c = hand_c(p, "L")
    x, y = c[0] - 0.004, c[1] - 0.006
    tube(m, [(x, y, 0.07), (x + 0.012, y, 0.75), (x, y - 0.02, 1.26), (x, y - 0.11, 1.34), (x, y - 0.16, 1.25)],
         [0.02, 0.022, 0.025, 0.026, 0.022], 4, "leather", rigid("hand_L"), tip1=0.02)
    seat_under(p, 0.305)
    return p


DRESS_M = [(0.74, 0.06, 0.055, 0.055), (0.615, 0.165, 0.13, 0.13), (0.46, 0.188, 0.165, 0.15),
           (0.31, 0.19, 0.162, 0.158), (0.13, 0.205, 0.18, 0.178)]


def market():
    """The Market's keeper: round and cheerful, a marigold dress, a cream apron, a teal headscarf
    knotted at the back, her fringe peeking out."""
    p = person("market", leg=0.28, torso=0.98, width=1.16, hip_width=1.1, eyes_at=(21.0, -7.0))
    h, m = p.head, p.body
    head_and_face(p, open_top=2, brows=dict(el=11.0, width=0.044, thick=0.009, arch=0.008))
    build_eyes(p, size=(0.037, 0.05))
    fringe(m, h, -60, 60, 6, 30, lambda az: angle_blend(az, {0: 17, 30: 14, 60: 6}), 0.03,
           locks=lambda k, az: 5 if k % 2 else 0)
    shell(m, h, lambda az: angle_blend(az, {0: 30, 40: 25, 75: 6, 100: -22, 140: -44, 180: -50}),
          lambda az, f: 0.044 + 0.02 * (1 - f), n_az=12, rows=3, mat="leather")
    ellipsoid(m, h.at(180, -34, 0.07), (0.058, 0.045, 0.05), 5, 3, "leather", HEAD)  # the knot
    sw = skirt_weights(p, 0.36, 0.13, follow=0.6)
    lathe(m, DRESS_M, 8, "outfit", sw)
    lathe(m, [(0.14, 0.198, 0.172, 0.17)], 8, "outfit_shade", sw, cap_bottom=0.3)
    apron = [(z,) + tuple(v + 0.008 for v in profile_at(DRESS_M, z)) for z in (0.56, 0.46, 0.31, 0.16)]
    lathe_arc(m, apron, -45, 45, 2, "trim", sw)
    for side, s in SIDES:
        arm(p, side, sleeve_mat="outfit", sleeve_r=0.066, forearm_r=0.042, hand_r=0.05)
        shin(p, side, "extra_dark", r=0.05)
        shoe(p, side, "extra", size=(0.06, 0.088, 0.05))
    seat_under(p, 0.29)
    return p


TORSO_S = [(0.8, 0.058, 0.055, 0.055), (0.665, 0.148, 0.114, 0.116), (0.57, 0.14, 0.118, 0.12),
           (0.39, 0.152, 0.13, 0.132)]


def sanctuary():
    """The Sanctuary's keeper: an elf-eared farmhand in denim overalls over a wheat shirt, a wide
    straw hat, a tin bucket of feed."""
    p = person("sanctuary", leg=0.32, torso=1.02, eyes_at=(21.0, -7.0))
    h, m = p.head, p.body
    head_and_face(p, open_top=2, ears="pointed", brows=dict(el=11.0, width=0.046, thick=0.01, arch=0.006))
    build_eyes(p, size=(0.038, 0.052))
    shell(m, h, lambda az: angle_blend(az, {0: 20, 40: 16, 80: 4, 110: -14, 150: -34, 180: -38}),
          lambda az, f: 0.024 + 0.02 * (1 - f), n_az=10, rows=1, locks=lambda k, az: 9 if k % 2 == 0 else 0)
    top, cy = h.c[2] + h.azt, h.c[1]
    lathe(m, [(top + 0.05, 0.13, 0.13, 0.13, cy), (top - 0.07, 0.226, 0.226, 0.226, cy),
              (top - 0.09, 0.4, 0.4, 0.4, cy), (top - 0.1, 0.23, 0.23, 0.23, cy)], 8, "leather", HEAD,
          cap_top=top + 0.07)
    tw = torso_weights(p)
    lathe(m, TORSO_S, 8, "outfit", tw, mats=["trim", "trim", "outfit"])
    for s in (-1, 1):  # the overall's straps, over the shoulders
        pts = [garment_point(TORSO_S, 0.575, s * 24, 0.006), garment_point(TORSO_S, 0.68, s * 45, 0.008),
               garment_point(TORSO_S, 0.68, s * 135, 0.008), garment_point(TORSO_S, 0.56, s * 156, 0.006)]
        ribbon(m, pts, 0.028, lambda q: norm((q[0], q[1], 0.12)), "outfit", tw)
    for side, s in SIDES:
        arm(p, side, sleeve_mat="trim", sleeve_r=0.058, sleeve_len=0.7)
        leg(p, side, "outfit", r=0.056)
        shoe(p, side, "extra", size=(0.064, 0.094, 0.058))
    c = hand_c(p, "L")  # the bucket hangs from the right hand
    bx, by, rim, bot = c[0] - 0.03, c[1], c[2] - 0.09, c[2] - 0.2
    hw = rigid("hand_L")
    lathe(m, [(rim, 0.07, 0.07, 0.07, by), (bot, 0.058, 0.058, 0.058, by)], 8, "metal", hw, cap_bottom=bot, cx=bx)
    lathe(m, [(rim - 0.008, 0.066, 0.066, 0.066, by)], 8, "sole", hw, cap_top=rim - 0.012, cx=bx)
    tube(m, [(bx - 0.07, by, rim + 0.004), (bx, by, c[2] + 0.01), (bx + 0.07, by, rim + 0.004)], [0.007] * 3, 3,
         "metal", hw)
    seat_under(p, 0.335)
    return p


TABARD = [(0.8, 0.058, 0.055, 0.055), (0.72, 0.1, 0.085, 0.086), (0.66, 0.146, 0.112, 0.114),
          (0.52, 0.136, 0.114, 0.116), (0.28, 0.168, 0.142, 0.146)]


def steward():
    """The arena's steward: fox-tailed and neat, a red tabard with gold hems and a badge over a
    cream shirt, a brass whistle on a cord, a clipboard."""
    p = person("steward", leg=0.315, torso=1.03, eyes_at=(21.0, -7.0))
    h, m = p.head, p.body
    head_and_face(p, open_top=2, brows=dict(el=11.5, width=0.046, thick=0.01, arch=0.004, tilt=0.004))
    build_eyes(p, size=(0.038, 0.052))
    def side_swept(az):  # the fringe swept toward -X, the ears showing
        if az >= 0:
            return angle_blend(az, {0: 20, 30: 18, 60: 12, 85: 8, 96: 6, 120: -20, 180: -36})
        return angle_blend(az, {0: 20, 25: 12, 55: 8, 85: 6, 96: 5, 120: -20, 180: -36})
    shell(m, h, side_swept, lambda az, f: 0.022 + 0.02 * (1 - f), n_az=12, rows=2,
          locks=lambda k, az: (11 if az < 50 else 7) if k % 2 == 0 else 0, crown=(0.035, 0.0), phase=0.5)
    sw = skirt_weights(p, 0.42, 0.28, follow=0.6)
    lathe(m, TABARD, 8, "outfit", sw, mats=["extra", "outfit", "outfit", "outfit"])
    lathe(m, [(0.29, 0.156, 0.132, 0.136)], 8, "leather_dark", sw, cap_bottom=0.33)
    hug(p, TABARD, 0.302, 0.28, 0.005, "trim", w=sw)
    tw = torso_weights(p)
    badge = [(z,) + tuple(v + 0.004 for v in profile_at(TABARD, z)) for z in (0.61, 0.545)]
    lathe_arc(m, badge, -16, 16, 1, "trim", tw)
    front = garment_point(TABARD, 0.6, 0, 0.012)
    for s in (-1, 1):  # the whistle's cord
        ribbon(m, [garment_point(TABARD, 0.71, s * 42, 0.005), front], 0.008, lambda q: norm((q[0], q[1], 0.3)),
               "trim", tw)
    tube(m, [add(front, (-0.004, -0.004, -0.004)), add(front, (0.036, -0.004, -0.01))], [0.012, 0.011], 4, "brass", tw,
         cap1=True)
    for side, s in SIDES:
        arm(p, side, sleeve_mat="extra", long_sleeve=True, sleeve_r=0.056)
        leg(p, side, "leather_dark", r=0.054)
        shoe(p, side, "leather")
    c = hand_c(p, "L")  # the clipboard, held by its top edge, hanging at her side
    bc = add(c, (-0.016, -0.012, -0.075))
    hw = rigid("hand_L")
    slab(m, bc, (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0), (0.006, 0.062, 0.082), "leather", hw)
    pw = [m.vert(add(bc, (-0.0072, dy, dz)), hw(bc), "white") for dy, dz in
          ((-0.05, -0.07), (0.05, -0.07), (0.05, 0.06), (-0.05, 0.06))]
    m.quad(*pw, (-1.0, 0.0, 0.0))
    tail = [(0.0, 0.08, 0.37), (0.0, 0.24, 0.3), (0.0, 0.36, 0.47)]
    tube(m, tail, [0.035, 0.078, 0.068], 5, "hair", rigid("hips"))
    tube(m, [(0.0, 0.358, 0.462), (0.0, 0.36, 0.53)], [0.069, 0.056], 5, "white", rigid("hips"), tip1=0.07)
    seat_under(p, 0.33)
    return p


TSHIRT = [(0.56, 0.05, 0.046, 0.046), (0.47, 0.118, 0.092, 0.094), (0.36, 0.114, 0.096, 0.098),
          (0.275, 0.122, 0.1, 0.102)]


def child():
    """A small child who follows the dragons about: a big red cap on backwards-messy hair, a sunny
    t-shirt, shorts, trainers, freckles, and a toy dragon dangling from one hand."""
    p = person("child", leg=0.21, torso=0.78, width=0.86, arm=0.82, head_scale=0.9, neck=0.06, hip_width=0.9,
               eyes_at=(21.0, -8.0))
    h, m = p.head, p.body
    head_and_face(p, open_top=2, freckles=True)
    build_eyes(p, size=(0.041, 0.057))
    shell(m, h, lambda az: angle_blend(az, {0: 14, 60: 8, 90: -6, 130: -24, 180: -28}),
          lambda az, f: 0.03, n_az=10, rows=1, locks=lambda k, az: 11 if k % 2 == 0 else 0, phase=0.5)
    cap_edge = lambda az: angle_blend(az, {0: 18, 60: 13, 90: 4, 130: -10, 180: -14})  # noqa: E731
    shell(m, h, cap_edge, lambda az, f: 0.036 + 0.014 * (1 - f), n_az=10, rows=2, mat="outfit")
    inner, outer = [], []
    for az in (-46, -23, 0, 23, 46):  # the visor
        e = h.at(az, cap_edge(az) + 1.5, 0.035)
        rad = norm((e[0] - h.c[0], e[1] - h.c[1], 0.0))
        inner.append(e)
        outer.append(add(e, (rad[0] * 0.13, rad[1] * 0.13, -0.022)))
    for mat, up in (("outfit", 1.0), ("outfit_shade", -1.0)):
        ids_i = [m.vert(q, HEAD(q), mat) for q in inner]
        ids_o = [m.vert(add(q, (0.0, 0.0, 0.0)), HEAD(q), mat) for q in outer]
        for k in range(4):
            m.quad(ids_i[k], ids_o[k], ids_o[k + 1], ids_i[k + 1], (0.0, -0.15, up))
    sw = skirt_weights(p, 0.27, 0.2, follow=0.8)
    lathe(m, TSHIRT, 8, "trim", torso_weights(p))
    lathe(m, [(0.285, 0.118, 0.096, 0.098), (0.2, 0.124, 0.102, 0.104)], 8, "leather", sw, cap_bottom=0.215)
    for side, s in SIDES:
        arm(p, side, sleeve_mat="trim", sleeve_r=0.05, forearm_r=0.032, hand_r=0.042)
        leg(p, side, "skin", r=0.043, top=0.05, bottom_z=0.07)
        shoe(p, side, "extra", size=(0.052, 0.078, 0.046))
    c = hand_c(p, "L")  # the toy dragon, held by the neck
    hw = rigid("hand_L")
    body_c = add(c, (-0.012, -0.03, -0.075))
    ellipsoid(m, body_c, (0.045, 0.042, 0.056), 5, 3, "plush", hw)
    ellipsoid(m, add(c, (-0.01, -0.052, -0.012)), (0.04, 0.046, 0.037), 5, 3, "plush", hw)
    seat_under(p, 0.215)
    return p


CLOAK = [(0.85, 0.06, 0.056, 0.056), (0.71, 0.168, 0.13, 0.138), (0.48, 0.172, 0.142, 0.152),
         (0.16, 0.215, 0.19, 0.2)]


def traveller():
    """A cloaked traveller from far off: elf ears, a short beard, a forest-green cloak, a huge
    leather pack with a bedroll, a lantern hanging from a tall pole."""
    p = person("traveller", leg=0.34, torso=1.08, width=1.05, head_scale=0.95, eyes_at=(21.0, -7.0))
    h, m = p.head, p.body
    head_and_face(p, open_top=2, ears="pointed", brows=dict(el=11.0, width=0.05, thick=0.012, arch=0.005, tilt=0.004))
    build_eyes(p, size=(0.036, 0.048))
    shell(m, h, lambda az: angle_blend(az, {0: 24, 30: 18, 70: 8, 100: -12, 150: -34, 180: -38}),
          lambda az, f: 0.024 + 0.028 * (1 - f), n_az=10, rows=2, locks=lambda k, az: 9 if k % 2 == 0 else 0)
    beard(m, h, lambda az: angle_blend(az, {0: -35, 40: -24, 80: -8, 100: -4}), lambda az, f: 0.022 + 0.022 * (1 - f),
          0.05, n=6, rows=2)
    lathe(m, CLOAK, 8, "outfit", skirt_weights(p, 0.42, 0.16, follow=0.6))
    for side, s in SIDES:
        arm(p, side, sleeve_mat="outfit", long_sleeve=True, sleeve_r=0.062)
        shin(p, side, "leather_dark", r=0.052)
        shoe(p, side, "leather", size=(0.064, 0.094, 0.06))
    cw = rigid("chest")
    ellipsoid(m, (0.0, 0.29, 0.74), (0.2, 0.14, 0.26), 6, 3, "leather", cw)  # the pack
    tube(m, [(-0.26, 0.29, 1.03), (0.26, 0.29, 1.03)], [0.07, 0.07], 5, "trim", cw, cap0=True, cap1=True)
    c = hand_c(p, "L")  # the lantern pole
    x, y = c[0] - 0.004, c[1] - 0.004
    hw = rigid("hand_L")
    tube(m, [(x, y, 0.06), (x, y, 1.6), (x, y - 0.15, 1.63)], [0.018, 0.02, 0.016], 4, "leather", hw, cap0=True,
         cap1=True)
    lx, ly = x, y - 0.14
    lathe(m, [(1.535, 0.05, 0.05, 0.05, ly)], 6, "metal", hw, cap_top=1.612, cx=lx)
    lathe(m, [(1.54, 0.042, 0.042, 0.042, ly), (1.45, 0.042, 0.042, 0.042, ly)], 6, "flame", hw, cx=lx)
    lathe(m, [(1.455, 0.05, 0.05, 0.05, ly)], 6, "metal", hw, cap_bottom=1.435, cx=lx)
    seat_under(p, 0.355)
    return p


PEOPLE = {"player_a": player_a, "player_b": player_b, "keeper": keeper, "market": market, "sanctuary": sanctuary,
          "steward": steward, "child": child, "traveller": traveller}
PLAYERS = ("player_a", "player_b")
VILLAGER_IDS = ("keeper", "market", "sanctuary", "steward", "child", "traveller")


def build(pid):
    p = PEOPLE[pid]()
    for m in [p.body] + p.eyes + p.hair:
        m.compact()
    return p
