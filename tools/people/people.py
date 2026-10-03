"""The people in the Storybook look (Noah, 2026-09-30: "Set 2", for everyone; D138): about three
heads tall with a neck, big irised eyes with brows, a dragon keeper's clothes. Your character's two
bodies, the six villagers and the story's own people, built from body.py's parts, their faces from
faces.py (every feeling's eyes, mouth and brows as part groups).

Every body shares one skeleton (rig.BONES) with its own proportions. Clothes and props are in the
body mesh (one skinned draw of 17 bones); the eyes are part group 0, the mouths 11, the brows 12 and
the player's hair 10. Props are held in hand_L (at -X: the person's own right hand). Garments are
cut to each body's own joint heights (heights() below), so a taller or rounder body still fits.
"""
import math

from body import SIDES, arm, garment_point, head_and_face, leg, person, profile_at, shin, shoe, skirt_weights, \
    spine_y, torso_weights
from faces import face
from geom import annulus, decal, ellipsoid, lathe, lathe_arc, ribbon, rigid, slab, tube
from hair import angle_blend, beard, fringe, player_hair, shell
from rig import add, lerp, mul, norm

HEAD = rigid("head")


# ------------------------------------------------------------------------------ the style's kit
def sb(pid, leg=0.42, torso=1.12, arm=1.12, head_scale=0.8, neck=0.04, eyes_at=(22.0, -8.0), neck_r=0.034, **kw):
    """A Storybook body: long legs, a smaller head on a neck (it sits clear of the collar)."""
    kw.setdefault("width", 1.0)
    p = person(pid, leg=leg, torso=torso, arm=arm, head_scale=head_scale, neck=neck, eyes_at=eyes_at, **kw)
    n0, n1 = p.joints["neck"]
    tube(p.body, [add(n0, (0.0, 0.0, -0.03)), add(n1, (0.0, 0.0, 0.03))], [neck_r, neck_r * 0.92], 5, "skin",
         rigid("neck"))
    return p


def heights(p):
    """The heights garments are cut to: neck, chest, spine and hips joints, the hip sockets, the knees."""
    j = p.joints
    return (j["neck"][0][2], j["chest"][0][2], j["spine"][0][2], j["hips"][0][2], j["leg_up_R"][0][2],
            j["leg_lo_R"][0][2])


def sb_face(p, eye=(0.033, 0.049), lashes=False, brows=None, mouth=None, ears=None, nose=True, blush=True,
            freckles=False, open_top=2, brow_mat="hair"):
    """The head, then every feeling's eyes, mouth and brows (faces.py)."""
    head_and_face(p, seg=10, ears=ears, nose=nose, blush=blush, mouth=False, open_top=open_top, freckles=freckles)
    face(p, eye_size=eye, lashes=lashes, brows=brows or dict(el=12.0, width=0.04, thick=0.009, arch=0.009),
         brow_mat=brow_mat, mouth=mouth)


def hug(p, profile, z0, z1, off, mat, seg=8, w=None, cy=None):
    """A band hugging a garment's profile from z0 down to z1, `off` metres proud of it."""
    r0, r1 = profile_at(profile, z0), profile_at(profile, z1)
    c0, c1 = (cy(z0), cy(z1)) if cy else (0.0, 0.0)
    lathe(p.body, [(z0, r0[0] + off, r0[1] + off, r0[2] + off, c0), (z1, r1[0] + off, r1[1] + off, r1[2] + off, c1)],
          seg, mat, w or torso_weights(p))


def hand_c(p, side):
    wr, tip = p.joints[f"hand_{side}"]
    return lerp(wr, tip, 0.35)


def seat_under(p, z):
    p.notes["seat"] = (0.0, 0.02, z)


def limbs(p, sleeve="outfit", long_sleeve=True, sleeve_r=0.052, sleeve_len=0.62, forearm_r=0.034, hand_r=0.042,
          legs="leather_dark", leg_r=0.046, shins=False, shoes="leather", shoe_size=(0.052, 0.08, 0.05), boot=None,
          seg=5):
    """Arms, legs (or just shins, under a long garment) and shoes; boot = (height, radius, material)."""
    for side, s in SIDES:
        arm(p, side, sleeve_mat=sleeve, long_sleeve=long_sleeve, sleeve_r=sleeve_r, sleeve_len=sleeve_len,
            forearm_r=forearm_r, hand_r=hand_r, seg=seg)
        if shins:
            shin(p, side, legs, r=leg_r * 0.92, seg=seg)
        else:
            leg(p, side, legs, r=leg_r, seg=seg)
        shoe(p, side, shoes, size=shoe_size, shaft=boot, seg=seg)


def hat_brim(p, z, r_in, r_out, mat, crown_h, crown_r, seg=10, tilt=0.0, cy=0.0, top_mat=None):
    """A brimmed hat on the head: a crown from z up crown_h, a flat brim from r_in out to r_out."""
    m, h = p.body, p.head
    cyh = h.c[1] + cy
    skirt = 0.012 + abs(tilt) * crown_r  # (the crown reaches under the brim: a tilted brim leaves no gap, run 22)
    lathe(m, [(z + crown_h, crown_r * 0.86, crown_r * 0.86, crown_r * 0.86, cyh), (z - skirt, crown_r, crown_r, crown_r, cyh)],
          seg, top_mat or mat, HEAD, cap_top=z + crown_h + 0.012)
    annulus(m, (0.0, cyh, z), norm((0.0, -tilt, 1.0)), (0.0, -1.0, 0.0), r_out, r_in, seg, mat, HEAD, both=True)


def finish(p, seat):
    seat_under(p, seat)
    return p


# ------------------------------------------------------------------------------ your character
def player_a():
    """Body A (Set 2's keeper): a tunic belted in leather, a capelet over the shoulders, a satchel at
    the hip, tall boots. Hair: the creator's six styles (part group 10)."""
    p = sb("player_a")
    m = p.body
    sb_face(p, brows=dict(el=12.0, width=0.04, thick=0.009, arch=0.009))
    zn, zc, zsp, zh, zhip, zk = heights(p)
    sw = skirt_weights(p, zh, zhip - 0.1, follow=0.65)
    tunic = [(zn + 0.01, 0.05, 0.046, 0.046), (zn - 0.04, 0.12, 0.094, 0.098), (zhip - 0.1, 0.15, 0.124, 0.128)]
    lathe(m, tunic, 8, "outfit", sw)
    hug(p, tunic, zh + 0.03, zh, 0.006, "leather")
    cw = rigid("chest")
    cape = [(zn + 0.03, 0.06, 0.054, 0.058), (zn - 0.035, 0.132, 0.104, 0.122), (zc - 0.06, 0.188, 0.152, 0.18)]
    lathe(m, cape, 8, "trim", cw)
    slab(m, (0.14, -0.02, zh - 0.02), (0.94, 0.34, 0.0), (-0.34, 0.94, 0.0), (0.0, 0.0, 1.0), (0.06, 0.028, 0.055),
         "leather", rigid("hips"))
    limbs(p, boot=(0.1, 0.052, "leather"))
    p.hair = player_hair(p.head, p.body)
    return finish(p, zhip + 0.012)


def player_b():
    """Body B (Set 2's keeper): a pinafore over a puff-sleeved blouse, an egg peeking from a leather
    pouch at the hip, ankle boots. Hair: the creator's six styles."""
    p = sb("player_b", width=0.94)
    m = p.body
    sb_face(p, lashes=True, eye=(0.034, 0.051), brows=dict(el=12.5, width=0.038, thick=0.007, arch=0.01))
    zn, zc, zsp, zh, zhip, zk = heights(p)
    tw = torso_weights(p)
    lathe(m, [(zn + 0.01, 0.048, 0.044, 0.044), (zn - 0.045, 0.112, 0.088, 0.092), (zc - 0.01, 0.106, 0.09, 0.092)], 7,
          "white", tw)
    sw = skirt_weights(p, zsp, zhip - 0.12, follow=0.7)
    pin = [(zc - 0.005, 0.1, 0.086, 0.088), (zh, 0.128, 0.11, 0.114), (zhip - 0.13, 0.19, 0.166, 0.172)]
    lathe(m, pin, 8, "outfit", sw)
    lathe(m, [(zhip - 0.128, 0.182, 0.158, 0.164)], 8, "outfit_shade", sw, cap_bottom=zhip - 0.06)
    hug(p, pin, zhip - 0.105, zhip - 0.13, 0.004, "trim", w=sw)
    for s in (-1, 1):
        ribbon(m, [(s * 0.055, -0.094, zc - 0.004), (s * 0.078, 0.0, zn - 0.012), (s * 0.055, 0.094, zc - 0.004)], 0.024,
               lambda q: norm((q[0], q[1], 0.25)), "outfit", tw)
    hw = rigid("hips")
    pc = (0.15, -0.03, zh - 0.05)
    lathe(m, [(pc[2] + 0.03, 0.05, 0.05, 0.05, pc[1]), (pc[2] - 0.04, 0.04, 0.04, 0.04, pc[1])], 5, "leather", hw,
          cap_bottom=pc[2] - 0.05, cx=pc[0])
    ellipsoid(m, (pc[0], pc[1], pc[2] + 0.05), (0.04, 0.04, 0.052), 5, 3, "extra", hw)
    for side, s in SIDES:
        arm(p, side, sleeve_mat="white", sleeve_r=0.064, sleeve_len=0.5, forearm_r=0.034, hand_r=0.041, seg=5)
        shin(p, side, "leather_dark", r=0.042, seg=5)
        shoe(p, side, "leather", size=(0.05, 0.076, 0.048), seg=5)
    p.hair = player_hair(p.head, p.body)
    return finish(p, zhip + 0.012)


# ------------------------------------------------------------------------------ the villagers
def keeper():
    """Old Rowan: a little stooped, white hair swept back in tufts, bushy brows, a big white beard,
    round brass spectacles, a long teal coat with gold hems, a crooked cane."""
    p = sb("keeper", leg=0.38, torso=1.04, stoop=0.05, head_scale=0.82, eyes_at=(21.0, -8.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.029, 0.04), nose=False, brows=dict(el=13.0, width=0.054, thick=0.016, arch=0.01, n=3),
            mouth=dict(el=-23.5, width=0.03, curve=0.008), open_top=1)  # (run 22: his forehead was see-through)
    decal(m, h, 0, -17, 0.021, 0.017, 0.016, 6, "nose", HEAD)
    shell(m, h, lambda az: angle_blend(az, {0: 58, 40: 46, 80: 8, 100: -8, 150: -34, 180: -40}),
          lambda az, f: 0.014 + 0.014 * (1 - f) + (0.034 if 70 < abs(az) < 140 else 0.01) * f,
          n_az=10, rows=2, locks=lambda k, az: 12 if (k % 2 == 0 and abs(az) > 60) else 0)
    beard(m, h, lambda az: angle_blend(az, {0: -31, 25: -28, 60: -22, 100: -24}),
          lambda az, f: 0.024 + 0.024 * (1 - f), 0.11, n=7, rows=3)
    for s in (-1, 1):  # spectacles, standing off the face
        c = h.at(s * 21, -8)
        n = h.normal(c)
        annulus(m, add(c, mul(n, 0.019)), n, (0.0, 0.0, 1.0), 0.041, 0.034, 6, "brass", HEAD)
    cl = add(h.at(-21, -8), mul(h.normal(h.at(-21, -8)), 0.021))
    cr = add(h.at(21, -8), mul(h.normal(h.at(21, -8)), 0.021))
    ribbon(m, [add(cl, (0.038, 0.0, 0.006)), add(cr, (-0.038, 0.0, 0.006))], 0.007, lambda q: (0.0, -1.0, 0.0), "brass",
           HEAD)
    zn, zc, zsp, zh, zhip, zk = heights(p)
    cy = lambda z: spine_y(p, z)  # noqa: E731 (the coat leans with the stoop)
    coat = [(zn + 0.03, 0.06, 0.054, 0.054), (zn - 0.04, 0.13, 0.104, 0.11), (zsp, 0.136, 0.114, 0.12),
            (zhip - 0.02, 0.152, 0.13, 0.136), (0.13, 0.19, 0.17, 0.176)]
    sw = skirt_weights(p, zhip + 0.04, 0.13)
    lathe(m, [r + (cy(r[0]),) for r in coat], 8, "outfit", sw)
    hug(p, coat, 0.16, 0.13, 0.006, "trim", w=sw, cy=cy)
    hug(p, coat, zh + 0.02, zh - 0.01, 0.006, "leather", cy=cy)
    limbs(p, shins=True, legs="leather_dark", leg_r=0.048, shoe_size=(0.054, 0.084, 0.05))
    c = hand_c(p, "L")  # the cane, its crook just above his hand
    x, y, top = c[0] - 0.004, c[1] - 0.006, c[2] + 0.18
    tube(m, [(x, y, 0.06), (x + 0.006, y, top - 0.1), (x, y - 0.014, top), (x, y - 0.058, top + 0.045),
             (x, y - 0.1, top - 0.01)], [0.018, 0.02, 0.022, 0.022, 0.018], 4, "leather", rigid("hand_L"), tip1=0.018)
    return finish(p, zhip + 0.01)


def market():
    """Maple: round and cheerful, a marigold dress with puffed sleeves, a cream apron, a teal
    headscarf knotted at the back, her fringe peeking out."""
    p = sb("market", leg=0.37, torso=1.06, width=1.14, hip_width=1.1, eyes_at=(21.0, -7.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.034, 0.048), lashes=True, brows=dict(el=11.5, width=0.038, thick=0.008, arch=0.008))
    fringe(m, h, -60, 60, 6, 30, lambda az: angle_blend(az, {0: 17, 30: 14, 60: 6}), 0.024,
           locks=lambda k, az: 5 if k % 2 else 0)
    shell(m, h, lambda az: angle_blend(az, {0: 30, 40: 25, 75: 6, 100: -22, 140: -44, 180: -50}),
          lambda az, f: 0.036 + 0.016 * (1 - f), n_az=12, rows=3, mat="leather")
    ellipsoid(m, h.at(180, -32, 0.055), (0.048, 0.036, 0.04), 5, 3, "leather", HEAD)  # the knot
    zn, zc, zsp, zh, zhip, zk = heights(p)
    dress = [(zn + 0.012, 0.05, 0.046, 0.046), (zn - 0.045, 0.15, 0.118, 0.118), (zsp, 0.17, 0.15, 0.138),
             (zhip, 0.176, 0.152, 0.146), (zk - 0.04, 0.198, 0.176, 0.172)]
    sw = skirt_weights(p, zhip + 0.03, zk - 0.04, follow=0.6)
    lathe(m, dress, 8, "outfit", sw)
    lathe(m, [(zk - 0.038, 0.19, 0.168, 0.164)], 8, "outfit_shade", sw, cap_bottom=zhip)
    apron = [(z,) + tuple(v + 0.007 for v in profile_at(dress, z)) for z in (zc - 0.03, zsp - 0.04, zhip, zk - 0.02)]
    lathe_arc(m, apron, -45, 45, 2, "trim", sw)
    limbs(p, long_sleeve=False, sleeve_r=0.062, sleeve_len=0.5, forearm_r=0.038, hand_r=0.045, shins=True,
          legs="extra_dark", leg_r=0.048, shoes="extra", shoe_size=(0.054, 0.082, 0.048))
    return finish(p, zhip + 0.008)


def sanctuary():
    """Bram: a gentle giant with pointed ears, denim overalls over a wheat shirt, a wide straw hat,
    a tin bucket of feed."""
    p = sb("sanctuary", leg=0.46, torso=1.2, width=1.14, arm=1.16, head_scale=0.8, eyes_at=(21.0, -7.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.032, 0.045), ears="pointed", brows=dict(el=11.0, width=0.04, thick=0.01, arch=0.006),
            open_top=0)  # (run 22: you saw through his head between the hat and his hair)
    # (1.0.1: two rows, the upper one under the hat's brim. One row is a flat face from the crown to the nape,
    # and with the scalp kept under it the back of his head was cut off at a slant. He's at his triangle budget:
    # the hair has eight sides for it, the hat seven and the bucket five.)
    shell(m, h, lambda az: angle_blend(az, {0: 20, 40: 16, 80: 4, 110: -14, 150: -34, 180: -38}),
          lambda az, f: 0.02 + 0.016 * (1 - f), n_az=8, rows=2, row_bias=0.77,
          locks=lambda k, az: 9 if k % 2 == 0 else 0)
    top, cyh = h.c[2] + h.azt, h.c[1]
    lathe(m, [(top + 0.04, 0.105, 0.105, 0.105, cyh), (top - 0.055, 0.182, 0.182, 0.182, cyh),
              (top - 0.072, 0.33, 0.33, 0.33, cyh), (top - 0.08, 0.19, 0.19, 0.19, cyh)], 7, "leather", HEAD,
          cap_top=top + 0.055)
    zn, zc, zsp, zh, zhip, zk = heights(p)
    tw = torso_weights(p)
    torso = [(zn + 0.012, 0.052, 0.048, 0.048), (zn - 0.05, 0.14, 0.108, 0.11), (zsp, 0.136, 0.114, 0.116),
             (zhip - 0.02, 0.15, 0.128, 0.13)]
    lathe(m, torso, 8, "outfit", tw, mats=["trim", "trim", "outfit"])
    for s in (-1, 1):  # the overall's straps
        pts = [garment_point(torso, zsp + 0.02, s * 24, 0.006), garment_point(torso, zn - 0.045, s * 45, 0.008),
               garment_point(torso, zn - 0.045, s * 135, 0.008), garment_point(torso, zsp, s * 156, 0.006)]
        ribbon(m, pts, 0.026, lambda q: norm((q[0], q[1], 0.12)), "outfit", tw)
    limbs(p, sleeve="trim", long_sleeve=False, sleeve_r=0.056, sleeve_len=0.75, forearm_r=0.038, hand_r=0.046,
          legs="outfit", leg_r=0.054, shoes="extra", shoe_size=(0.058, 0.088, 0.054))
    c = hand_c(p, "L")  # the bucket hangs from the right hand
    bx, by, rim, bot = c[0] - 0.028, c[1], c[2] - 0.085, c[2] - 0.19
    hw = rigid("hand_L")
    lathe(m, [(rim, 0.064, 0.064, 0.064, by), (bot, 0.054, 0.054, 0.054, by)], 5, "metal", hw, cap_bottom=bot, cx=bx)
    lathe(m, [(rim - 0.008, 0.06, 0.06, 0.06, by)], 5, "sole", hw, cap_top=rim - 0.012, cx=bx)
    tube(m, [(bx - 0.064, by, rim + 0.004), (bx, by, c[2] + 0.01), (bx + 0.064, by, rim + 0.004)], [0.006] * 3, 3,
         "metal", hw)
    return finish(p, zhip + 0.012)


def steward():
    """Wren: fox-tailed and neat, a red tabard with gold hems and a badge over a cream shirt, a brass
    whistle on a cord, a clipboard."""
    p = sb("steward", eyes_at=(21.0, -7.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.033, 0.048), lashes=True, brows=dict(el=12.0, width=0.038, thick=0.008, arch=0.004, tilt=0.003))

    def side_swept(az):
        if az >= 0:
            return angle_blend(az, {0: 20, 30: 18, 60: 12, 85: 8, 96: 6, 120: -20, 180: -36})
        return angle_blend(az, {0: 20, 25: 12, 55: 8, 85: 6, 96: 5, 120: -20, 180: -36})
    shell(m, h, side_swept, lambda az, f: 0.018 + 0.018 * (1 - f), n_az=12, rows=2,
          locks=lambda k, az: (11 if az < 50 else 7) if k % 2 == 0 else 0, crown=(0.028, 0.0), phase=0.5)
    zn, zc, zsp, zh, zhip, zk = heights(p)
    tabard = [(zn + 0.012, 0.05, 0.046, 0.046), (zn - 0.03, 0.09, 0.076, 0.078), (zn - 0.07, 0.13, 0.1, 0.102),
              (zsp - 0.02, 0.122, 0.102, 0.104), (zhip - 0.12, 0.15, 0.128, 0.132)]
    sw = skirt_weights(p, zh, zhip - 0.12, follow=0.6)
    lathe(m, tabard, 8, "outfit", sw, mats=["extra", "outfit", "outfit", "outfit"])
    hug(p, tabard, zhip - 0.1, zhip - 0.12, 0.005, "trim", w=sw)
    tw = torso_weights(p)
    badge = [(z,) + tuple(v + 0.004 for v in profile_at(tabard, z)) for z in (zc - 0.02, zc - 0.075)]
    lathe_arc(m, badge, -16, 16, 1, "trim", tw)
    front = garment_point(tabard, zc - 0.015, 0, 0.012)
    for s in (-1, 1):
        ribbon(m, [garment_point(tabard, zn - 0.025, s * 42, 0.005), front], 0.007, lambda q: norm((q[0], q[1], 0.3)),
               "trim", tw)
    tube(m, [add(front, (-0.004, -0.004, -0.004)), add(front, (0.032, -0.004, -0.01))], [0.011, 0.01], 4, "brass", tw,
         cap1=True)
    limbs(p, sleeve="extra", leg_r=0.044)
    c = hand_c(p, "L")  # the clipboard, held by its top edge
    bc = add(c, (-0.016, -0.012, -0.07))
    hw = rigid("hand_L")
    slab(m, bc, (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0), (0.006, 0.056, 0.074), "leather", hw)
    pw = [m.vert(add(bc, (-0.0072, dy, dz)), hw(bc), "white") for dy, dz in
          ((-0.045, -0.062), (0.045, -0.062), (0.045, 0.054), (-0.045, 0.054))]
    m.quad(*pw, (-1.0, 0.0, 0.0))
    tz = zh - 0.06
    tube(m, [(0.0, 0.07, tz), (0.0, 0.22, tz - 0.07), (0.0, 0.33, tz + 0.1)], [0.032, 0.07, 0.06], 5, "hair",
         rigid("hips"))
    tube(m, [(0.0, 0.328, tz + 0.092), (0.0, 0.33, tz + 0.155)], [0.061, 0.05], 5, "white", rigid("hips"), tip1=0.06)
    return finish(p, zhip + 0.012)


def child():
    """Pip: a small child who follows the dragons about: a big red cap on messy hair, a sunny tee,
    shorts, trainers, freckles, and Sir Flaps, a plush dragon, dangling from one hand."""
    p = sb("child", leg=0.27, torso=0.84, width=0.86, arm=0.86, head_scale=0.78, neck=0.03, hip_width=0.9,
           eyes_at=(21.0, -8.0), neck_r=0.03)
    h, m = p.head, p.body
    sb_face(p, eye=(0.036, 0.051), freckles=True, brows=dict(el=12.5, width=0.034, thick=0.008, arch=0.008))
    shell(m, h, lambda az: angle_blend(az, {0: 14, 60: 8, 90: -6, 130: -24, 180: -28}),
          lambda az, f: 0.024, n_az=10, rows=1, locks=lambda k, az: 11 if k % 2 == 0 else 0, phase=0.5)
    cap_edge = lambda az: angle_blend(az, {0: 18, 60: 13, 90: 4, 130: -10, 180: -14})  # noqa: E731
    shell(m, h, cap_edge, lambda az, f: 0.03 + 0.012 * (1 - f), n_az=10, rows=2, mat="outfit")
    inner, outer = [], []
    for az in (-46, -23, 0, 23, 46):  # the visor
        e = h.at(az, cap_edge(az) + 1.5, 0.029)
        rad = norm((e[0] - h.c[0], e[1] - h.c[1], 0.0))
        inner.append(e)
        outer.append(add(e, (rad[0] * 0.11, rad[1] * 0.11, -0.018)))
    for mat, up in (("outfit", 1.0), ("outfit_shade", -1.0)):
        ids_i = [m.vert(q, HEAD(q), mat) for q in inner]
        ids_o = [m.vert(q, HEAD(q), mat) for q in outer]
        for k in range(4):
            m.quad(ids_i[k], ids_o[k], ids_o[k + 1], ids_i[k + 1], (0.0, -0.15, up))
    zn, zc, zsp, zh, zhip, zk = heights(p)
    tee = [(zn + 0.01, 0.044, 0.04, 0.04), (zn - 0.04, 0.106, 0.084, 0.086), (zsp, 0.104, 0.088, 0.09),
           (zhip + 0.02, 0.112, 0.092, 0.094)]
    lathe(m, tee, 8, "trim", torso_weights(p))
    sw = skirt_weights(p, zhip + 0.01, zhip - 0.07, follow=0.8)
    lathe(m, [(zhip + 0.028, 0.108, 0.088, 0.09), (zhip - 0.07, 0.114, 0.094, 0.096)], 8, "leather", sw,
          cap_bottom=zhip - 0.055)
    limbs(p, sleeve="trim", long_sleeve=False, sleeve_r=0.045, sleeve_len=0.62, forearm_r=0.029, hand_r=0.037,
          legs="skin", leg_r=0.037, shoes="extra", shoe_size=(0.046, 0.07, 0.042))
    c = hand_c(p, "L")  # Sir Flaps, held by the neck
    hw = rigid("hand_L")
    body_c = add(c, (-0.012, -0.028, -0.068))
    ellipsoid(m, body_c, (0.04, 0.038, 0.05), 5, 3, "plush", hw)
    ellipsoid(m, add(c, (-0.01, -0.048, -0.01)), (0.036, 0.042, 0.033), 5, 3, "plush", hw)
    for s in (-1, 1):  # his little felt wings
        tube(m, [add(body_c, (s * 0.03, 0.01, 0.012)), add(body_c, (s * 0.07, 0.03, 0.04))], [(0.02, 0.006), (0.008, 0.003)],
             3, "plush", hw)
    return finish(p, zhip + 0.005)


def traveller():
    """Sable: a cloaked traveller from beyond the gap: elf ears, a short beard, a forest-green
    cloak, a great leather pack with a bedroll, a lantern hanging from a tall pole."""
    p = sb("traveller", leg=0.46, torso=1.16, width=1.04, eyes_at=(21.0, -7.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.031, 0.043), ears="pointed", brows=dict(el=11.0, width=0.042, thick=0.01, arch=0.005, tilt=0.003),
            mouth=dict(el=-27.5))
    shell(m, h, lambda az: angle_blend(az, {0: 24, 30: 18, 70: 8, 100: -12, 150: -34, 180: -38}),
          lambda az, f: 0.02 + 0.022 * (1 - f), n_az=10, rows=2, locks=lambda k, az: 9 if k % 2 == 0 else 0)
    beard(m, h, lambda az: angle_blend(az, {0: -36, 40: -26, 80: -8, 100: -4}), lambda az, f: 0.018 + 0.018 * (1 - f),
          0.04, n=6, rows=2)
    zn, zc, zsp, zh, zhip, zk = heights(p)
    cloak = [(zn + 0.03, 0.054, 0.05, 0.05), (zn - 0.04, 0.15, 0.116, 0.124), (zsp - 0.02, 0.154, 0.128, 0.138),
             (0.15, 0.19, 0.17, 0.18)]
    lathe(m, cloak, 8, "outfit", skirt_weights(p, zhip + 0.04, 0.15, follow=0.6))
    limbs(p, shins=True, leg_r=0.05, shoe_size=(0.056, 0.086, 0.054))
    cw = rigid("chest")
    ellipsoid(m, (0.0, 0.26, zc + 0.02), (0.17, 0.12, 0.22), 6, 3, "leather", cw)  # the pack
    tube(m, [(-0.22, 0.26, zc + 0.27), (0.22, 0.26, zc + 0.27)], [0.06, 0.06], 5, "trim", cw, cap0=True, cap1=True)
    c = hand_c(p, "L")  # the lantern pole
    x, y = c[0] - 0.004, c[1] - 0.004
    hw = rigid("hand_L")
    tz = zn + 0.75
    tube(m, [(x, y, 0.06), (x, y, tz), (x, y - 0.14, tz + 0.03)], [0.016, 0.018, 0.015], 4, "leather", hw, cap0=True,
         cap1=True)
    lx, ly = x, y - 0.13
    lathe(m, [(tz - 0.06, 0.045, 0.045, 0.045, ly)], 6, "metal", hw, cap_top=tz + 0.01, cx=lx)
    lathe(m, [(tz - 0.055, 0.038, 0.038, 0.038, ly), (tz - 0.135, 0.038, 0.038, 0.038, ly)], 6, "flame", hw, cx=lx)
    lathe(m, [(tz - 0.13, 0.045, 0.045, 0.045, ly)], 6, "metal", hw, cap_bottom=tz - 0.15, cx=lx)
    return finish(p, zhip + 0.014)


# ------------------------------------------------------------------------------ the story's people
def fig():
    """Fig Thimblewhistle, Royal Surveyor (self-appointed): lanky, a too-big feathered hat, a moss
    tunic with a gold scarf, a brass telescope at his hip, boots he hasn't grown into."""
    p = sb("fig", leg=0.48, torso=1.12, width=0.88, arm=1.2, eyes_at=(22.0, -8.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.034, 0.05), brows=dict(el=13.0, width=0.04, thick=0.008, arch=0.012), open_top=0)
    # (run 22: one ring of hair cut a jagged band across his face; two rows hug the head, a soft fringe
    # above his eyes, the tufts at his ears and nape)
    shell(m, h, lambda az: angle_blend(az, {0: 24, 40: 16, 90: -6, 130: -22, 180: -26}),
          lambda az, f: 0.016 + 0.006 * (1 - f), n_az=9, rows=2,  # (thin under the hat: none through its brim)
          locks=lambda k, az: (5 if abs(az) < 50 else 10) if k % 2 == 0 else 0, phase=0.5)
    top = h.c[2] + h.azt
    hat_brim(p, top - 0.06, 0.15, 0.3, "trim", 0.12, 0.18, seg=7, tilt=0.12)  # far too big
    q = (0.12, h.c[1] + 0.06, top + 0.02)  # the feather, swept back
    tube(m, [q, add(q, (0.05, 0.1, 0.14)), add(q, (0.03, 0.24, 0.2))], [(0.03, 0.008), (0.04, 0.008), (0.02, 0.006)], 4,
         "extra", HEAD, tip1=0.06, up_hint=(1.0, 0.0, 0.0))
    zn, zc, zsp, zh, zhip, zk = heights(p)
    tunic = [(zn + 0.012, 0.046, 0.042, 0.042), (zn - 0.04, 0.11, 0.086, 0.09), (zhip - 0.08, 0.13, 0.11, 0.114)]
    sw = skirt_weights(p, zh, zhip - 0.08, follow=0.65)
    lathe(m, tunic, 8, "outfit", sw)
    hug(p, tunic, zh + 0.02, zh - 0.01, 0.006, "leather")
    lathe(m, [(zn + 0.03, 0.066, 0.062, 0.062), (zn - 0.035, 0.1, 0.084, 0.088)], 8, "trim", torso_weights(p))
    hw = rigid("hips")
    tube(m, [(0.13, -0.02, zh + 0.02), (0.15, 0.0, zh - 0.12)], [0.02, 0.026], 4, "brass", hw, cap0=True, cap1=True)
    limbs(p, leg_r=0.042, boot=(0.12, 0.05, "leather"), shoe_size=(0.054, 0.086, 0.05))
    return finish(p, zhip + 0.012)


def tam():
    """Tam the fisher: laid back and stubbly, a yellow rain hat, a cable-knit jumper, dark waders up
    to the chest on braces."""
    p = sb("tam", leg=0.4, torso=1.1, width=1.08, eyes_at=(21.0, -8.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.031, 0.042), brows=dict(el=11.5, width=0.038, thick=0.011, arch=0.006), open_top=0)
    beard(m, h, lambda az: angle_blend(az, {0: -34, 40: -24, 80: -8, 100: -4}), lambda az, f: 0.008 + 0.006 * (1 - f),
          0.01, n=6, rows=2)
    # (run 22: a hairline across his forehead under the hat read as one long brow down to his beard;
    # his hair now shows only at the sides and back, under the sou'wester's brim)
    # (1.0.1: three rows, round the head's curve. One row is a flat face from the crown to the nape, and with
    # the scalp kept under it the back of his head was cut off at a slant.)
    shell(m, h, lambda az: angle_blend(az, {0: 52, 45: 40, 80: 6, 100: -14, 180: -30}),
          lambda az, f: 0.02, n_az=10, rows=3, locks=lambda k, az: 8 if (k % 2 == 0 and abs(az) > 70) else 0)
    top, cyh = h.c[2] + h.azt, h.c[1]  # the sou'wester: a round crown, a brim longer at the back
    lathe(m, [(top + 0.03, 0.09, 0.09, 0.09, cyh), (top - 0.05, 0.19, 0.19, 0.19, cyh)], 9, "extra", HEAD,
          cap_top=top + 0.045)
    lathe(m, [(top - 0.05, 0.19, 0.19, 0.19, cyh), (top - 0.08, 0.27, 0.25, 0.32, cyh + 0.03)], 9, "extra", HEAD)
    zn, zc, zsp, zh, zhip, zk = heights(p)
    tw = torso_weights(p)
    jumper = [(zn + 0.018, 0.058, 0.054, 0.054), (zn - 0.045, 0.14, 0.11, 0.114), (zsp, 0.14, 0.12, 0.122),
              (zhip, 0.15, 0.128, 0.13)]
    lathe(m, jumper, 8, "trim", tw)
    waders = [(zc - 0.04, 0.142, 0.118, 0.12), (zhip - 0.02, 0.156, 0.134, 0.136)]
    lathe(m, waders, 8, "outfit", tw)
    for s in (-1, 1):  # braces
        ribbon(m, [garment_point(jumper, zc - 0.04, s * 30, 0.008), garment_point(jumper, zn - 0.04, s * 48, 0.008),
                   garment_point(jumper, zc - 0.04, s * 150, 0.008)], 0.02, lambda q: norm((q[0], q[1], 0.1)), "outfit",
               tw)
    limbs(p, sleeve="trim", sleeve_r=0.056, legs="outfit", leg_r=0.054, shoes="outfit_shade",
          shoe_size=(0.058, 0.088, 0.054))
    return finish(p, zhip + 0.012)


def tove():
    """Tove, keeper of Frostspire Hollow: stoic, in a long winter coat with white fur at the collar
    and hem, a knitted hat with a pompom, mittens, a short pale bob."""
    p = sb("tove", leg=0.42, torso=1.12, width=1.02, eyes_at=(22.0, -8.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.031, 0.044), lashes=True, brows=dict(el=11.5, width=0.04, thick=0.008, arch=0.003), open_top=1)
    shell(m, h, lambda az: angle_blend(az, {0: 18, 38: 16, 55: -40, 100: -46, 180: -42}),
          lambda az, f: 0.02 + 0.02 * (1 - f) ** 1.2, n_az=12, rows=3, curtain=-12, flare=0.1, phase=0.5)
    hat = lambda az: angle_blend(az, {0: 34, 60: 26, 90: 18, 180: 12})  # noqa: E731
    shell(m, h, hat, lambda az, f: 0.05 + 0.008 * (1 - f), n_az=10, rows=2, mat="trim")  # (over her hair: run 22)
    ellipsoid(m, h.at(0, 90, 0.07), (0.05, 0.05, 0.05), 6, 2, "white", HEAD)
    zn, zc, zsp, zh, zhip, zk = heights(p)
    coat = [(zn + 0.03, 0.06, 0.056, 0.056), (zn - 0.04, 0.14, 0.108, 0.114), (zsp, 0.14, 0.118, 0.124),
            (zhip - 0.02, 0.156, 0.134, 0.14), (zk - 0.02, 0.18, 0.158, 0.164)]
    sw = skirt_weights(p, zhip + 0.03, zk - 0.02, follow=0.6)
    lathe(m, coat, 8, "outfit", sw)
    hug(p, coat, zk + 0.02, zk - 0.02, 0.012, "white", w=sw)
    lathe(m, [(zn + 0.045, 0.08, 0.074, 0.074), (zn - 0.03, 0.15, 0.12, 0.126)], 8, "white", torso_weights(p))
    for side, s in SIDES:
        arm(p, side, sleeve_mat="outfit", long_sleeve=True, sleeve_r=0.054, hand_r=0.046, hand_mat="trim", seg=5)
        shin(p, side, "leather_dark", r=0.044, seg=5)
        shoe(p, side, "leather", size=(0.054, 0.084, 0.052), shaft=(0.09, 0.05, "leather"), seg=5)
    return finish(p, zhip + 0.012)


def linnet():
    """Linnet of Linnet's Finery: a wide hat with a ribbon and a silk flower, a fitted jacket over a
    full skirt, a tape measure draped round her neck."""
    p = sb("linnet", leg=0.42, torso=1.1, width=0.94, eyes_at=(22.0, -8.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.034, 0.05), lashes=True, brows=dict(el=12.5, width=0.038, thick=0.007, arch=0.011))
    shell(m, h, lambda az: angle_blend(az, {0: 20, 30: 16, 70: 0, 100: -30, 180: -44}),
          lambda az, f: 0.02 + 0.022 * (1 - f), n_az=12, rows=3, curtain=-20, flare=0.18, phase=0.5,
          locks=lambda k, az: 8 if (k % 2 == 0 and abs(az) > 60) else 0)
    top = h.c[2] + h.azt
    hat_brim(p, top - 0.045, 0.14, 0.27, "outfit", 0.07, 0.16, seg=8, tilt=-0.1)
    lathe(m, [(top - 0.025, 0.162, 0.162, 0.162, h.c[1]), (top - 0.045, 0.164, 0.164, 0.164, h.c[1])], 8, "trim", HEAD)
    ellipsoid(m, (0.14, h.c[1] - 0.04, top - 0.02), (0.04, 0.03, 0.04), 5, 2, "extra", HEAD)  # the silk flower
    zn, zc, zsp, zh, zhip, zk = heights(p)
    jacket = [(zn + 0.01, 0.046, 0.042, 0.042), (zn - 0.04, 0.11, 0.086, 0.09), (zsp - 0.01, 0.096, 0.082, 0.084),
              (zh - 0.01, 0.124, 0.104, 0.108)]
    lathe(m, jacket, 8, "outfit", torso_weights(p))
    sw = skirt_weights(p, zh, zk - 0.04, follow=0.7)
    skirt = [(zh + 0.01, 0.11, 0.094, 0.098), (zk - 0.04, 0.2, 0.18, 0.186)]
    lathe(m, skirt, 8, "trim", sw)
    lathe(m, [(zk - 0.038, 0.192, 0.172, 0.178)], 8, "trim_shade", sw, cap_bottom=zh - 0.02)
    for s in (-1, 1):  # the tape measure, hanging down her front
        ribbon(m, [garment_point(jacket, zn - 0.03, s * 70, 0.006), garment_point(jacket, zn - 0.035, s * 30, 0.008),
                   garment_point(jacket, zsp, s * 18, 0.008)], 0.018, lambda q: norm((q[0], q[1], 0.0)), "extra",
               torso_weights(p))
    limbs(p, sleeve_r=0.05, shins=True, leg_r=0.042, shoe_size=(0.048, 0.076, 0.046))
    return finish(p, zhip + 0.01)


def madder():
    """Madder the dyer: curly hair under a knotted bandana, sleeves rolled up, an apron splashed in
    every colour, and hands stained purple to the wrist."""
    p = sb("madder", leg=0.42, torso=1.12, width=1.02, eyes_at=(21.0, -8.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.032, 0.046), brows=dict(el=12.0, width=0.04, thick=0.01, arch=0.008))
    shell(m, h, lambda az: angle_blend(az, {0: 20, 50: 12, 90: -4, 140: -24, 180: -30}),
          lambda az, f: 0.03 + 0.016 * (1 - f), n_az=12, rows=2, locks=lambda k, az: 10 if k % 2 == 0 else 4, phase=0.5)
    band_e = lambda az: angle_blend(az, {0: 30, 90: 22, 180: 18})  # noqa: E731
    shell(m, h, band_e, lambda az, f: 0.05 if f > 0.5 else 0.0, n_az=10, rows=1, mat="trim")
    tube(m, [h.at(165, 18, 0.05), add(h.at(170, 4, 0.06), (0.03, 0.04, -0.06))], [(0.03, 0.01), (0.022, 0.008)], 3,
         "trim", HEAD, tip1=0.03)
    zn, zc, zsp, zh, zhip, zk = heights(p)
    tw = torso_weights(p)
    shirt = [(zn + 0.012, 0.05, 0.046, 0.046), (zn - 0.045, 0.13, 0.1, 0.104), (zsp, 0.128, 0.108, 0.11),
             (zhip, 0.14, 0.118, 0.12)]
    lathe(m, shirt, 8, "white", tw)
    apron = [(z,) + tuple(v + 0.007 for v in profile_at(shirt, z)) for z in (zc - 0.02, zsp, zhip)]
    apron.append((zk + 0.02, 0.15, 0.13, 0.13))
    lathe_arc(m, apron, -55, 55, 3, "outfit", skirt_weights(p, zh, zk, follow=0.5))
    for z, az, c in ((zsp - 0.03, -20, "trim"), (zhip - 0.02, 25, "extra"), (zh, 5, "trim_shade")):  # splashes
        q = garment_point(apron, z, az, 0.003)
        ellipsoid(m, q, (0.024, 0.006, 0.02), 4, 2, c, tw)
    for side, s in SIDES:
        arm(p, side, sleeve_mat="white", sleeve_r=0.056, sleeve_len=0.85, forearm_r=0.034, hand_r=0.044,
            hand_mat="extra", seg=5)
        leg(p, side, "leather_dark", r=0.046, seg=5)
        shoe(p, side, "leather", size=(0.054, 0.084, 0.05), seg=5)
    return finish(p, zhip + 0.012)


def celestine():
    """Celestine, the pageant's host: a towering updo with a star pin, a floor-length gown with puffed
    sleeves, a feather boa, all of it sparkling."""
    p = sb("celestine", leg=0.44, torso=1.12, width=0.96, eyes_at=(22.0, -8.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.035, 0.052), lashes=True, brows=dict(el=13.0, width=0.04, thick=0.008, arch=0.014))
    shell(m, h, lambda az: angle_blend(az, {0: 24, 30: 20, 70: 6, 100: -6, 180: -14}),
          lambda az, f: 0.02 + 0.03 * (1 - f), n_az=12, rows=2, locks=lambda k, az: 6 if k % 2 == 0 else 0)
    top = h.c[2] + h.azt
    for c, r in (((0.0, h.c[1] + 0.02, top + 0.04), (0.12, 0.11, 0.08)), ((0.0, h.c[1] + 0.03, top + 0.13),
                                                                            (0.09, 0.085, 0.07))):
        ellipsoid(m, c, r, 6, 3, "hair", HEAD)  # the updo, in two great swirls
    ellipsoid(m, (0.07, h.c[1] - 0.06, top + 0.06), (0.03, 0.012, 0.03), 4, 2, "extra", HEAD)  # the star pin
    zn, zc, zsp, zh, zhip, zk = heights(p)
    bodice = [(zn - 0.02, 0.1, 0.078, 0.082), (zsp, 0.094, 0.08, 0.082), (zh, 0.11, 0.094, 0.098)]
    lathe(m, bodice, 8, "outfit", torso_weights(p))
    sw = skirt_weights(p, zh, 0.12, follow=0.55)
    gown = [(zh + 0.002, 0.11, 0.094, 0.098), (zk, 0.19, 0.172, 0.18), (0.04, 0.25, 0.23, 0.25)]
    lathe(m, gown, 9, "outfit", sw)
    lathe(m, [(0.042, 0.244, 0.224, 0.244)], 9, "outfit_shade", sw, cap_bottom=zk)
    hug(p, gown, 0.07, 0.04, 0.006, "trim", seg=9, w=sw)
    boa = []  # the boa, round her shoulders and trailing down both sides
    for k in range(5):
        a = math.radians(-120 + 240 * k / 4)
        boa.append((0.11 * math.sin(a), -0.09 * math.cos(a) + 0.02, zn - 0.02 - 0.03 * abs(math.sin(a))))
    tube(m, [(-0.12, -0.05, zc - 0.12)] + boa + [(0.12, -0.05, zc - 0.12)], [0.03] * 7, 4, "trim", torso_weights(p),
         tip0=0.02, tip1=0.02)
    for side, s in SIDES:
        arm(p, side, sleeve_mat="outfit", sleeve_r=0.068, sleeve_len=0.5, forearm_r=0.032, hand_r=0.04, seg=5)
        shoe(p, side, "extra", size=(0.044, 0.07, 0.04), seg=5)
    return finish(p, zhip + 0.012)


def primrose():
    """Primrose Pembrook: ringlets in two tails tied with bows, a big bow on top, a frilly tiered
    dress in pink and cream, little buckled shoes. Snooty, and secretly lovely."""
    p = sb("primrose", leg=0.4, torso=1.06, width=0.92, eyes_at=(22.0, -8.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.035, 0.05), lashes=True, brows=dict(el=13.0, width=0.036, thick=0.007, arch=0.012))
    shell(m, h, lambda az: angle_blend(az, {0: 18, 25: 20, 50: 12, 85: 2, 110: -18, 180: -30}),
          lambda az, f: 0.02 + 0.024 * (1 - f), n_az=12, rows=2, locks=lambda k, az: 6 if k % 2 == 1 else 0)
    for s in (-1, 1):  # the ringlets: twin tails in a corkscrew
        base = h.at(s * 105, 10, 0.02)
        pts = [base]
        for k in range(1, 6):
            a = k * 1.6
            pts.append(add(base, (s * (0.03 + 0.025 * math.cos(a)), 0.025 * math.sin(a), -0.05 * k)))
        tube(m, pts[:5], [0.036, 0.033, 0.03, 0.026, 0.02], 4, "hair", HEAD, tip1=0.03)
    bow = h.at(30, 62, 0.03)
    for s in (-1, 1):
        ellipsoid(m, add(bow, (s * 0.05, 0.0, 0.01)), (0.05, 0.02, 0.034), 5, 2, "trim", HEAD)
    ellipsoid(m, bow, (0.022, 0.022, 0.022), 4, 2, "trim_shade", HEAD)
    zn, zc, zsp, zh, zhip, zk = heights(p)
    lathe(m, [(zn + 0.01, 0.046, 0.042, 0.042), (zn - 0.04, 0.104, 0.082, 0.086), (zh, 0.104, 0.088, 0.09)], 8,
          "outfit", torso_weights(p))
    lathe(m, [(zn + 0.012, 0.062, 0.058, 0.058), (zn - 0.016, 0.09, 0.076, 0.078)], 8, "white", torso_weights(p))
    sw = skirt_weights(p, zh, zhip - 0.14, follow=0.7)
    for z0, z1, r0, r1, mat in ((zh + 0.004, zhip - 0.05, 0.11, 0.17, "outfit"), (zhip - 0.04, zhip - 0.14, 0.17, 0.2,
                                                                                 "white")):
        lathe(m, [(z0, r0, r0 * 0.86, r0 * 0.88), (z1, r1, r1 * 0.88, r1 * 0.9)], 8, mat, sw)
    lathe(m, [(zhip - 0.138, 0.195, 0.172, 0.176)], 8, "trim_shade", sw, cap_bottom=zhip - 0.06)
    for side, s in SIDES:
        arm(p, side, sleeve_mat="white", sleeve_r=0.06, sleeve_len=0.45, forearm_r=0.031, hand_r=0.039, seg=5)
        shin(p, side, "white", r=0.038, seg=5)
        shoe(p, side, "trim_shade", size=(0.046, 0.072, 0.044), seg=5)
    return finish(p, zhip + 0.01)


def marigold():
    """Marigold, the Ember champion: a gardener, ginger hair in two low bunches, a wide sun hat with a
    band of flowers, a sunny dress under a pocketed apron."""
    p = sb("marigold", leg=0.42, torso=1.1, width=0.96, eyes_at=(22.0, -8.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.034, 0.05), lashes=True, brows=dict(el=12.5, width=0.038, thick=0.008, arch=0.01))
    shell(m, h, lambda az: angle_blend(az, {0: 18, 30: 16, 70: 2, 110: -22, 180: -34}),
          lambda az, f: 0.02 + 0.02 * (1 - f), n_az=12, rows=2, locks=lambda k, az: 6 if k % 2 == 0 else 0, phase=0.5)
    for s in (-1, 1):  # low bunches
        b = h.at(s * 120, -30, 0.02)
        tube(m, [b, add(b, (s * 0.03, 0.03, -0.12))], [0.04, 0.026], 4, "hair", HEAD, tip1=0.03)
    top = h.c[2] + h.azt
    hat_brim(p, top - 0.06, 0.15, 0.29, "leather", 0.09, 0.17, seg=8, tilt=0.06)
    for k in range(4):  # flowers round the band
        a = math.radians(-60 + k * 40)
        ellipsoid(m, (0.175 * math.sin(a), h.c[1] - 0.175 * math.cos(a), top - 0.03), (0.026, 0.026, 0.02), 4, 2,
                  "extra" if k % 2 else "trim", HEAD)
    zn, zc, zsp, zh, zhip, zk = heights(p)
    dress = [(zn + 0.01, 0.046, 0.042, 0.042), (zn - 0.04, 0.11, 0.088, 0.09), (zh, 0.114, 0.098, 0.1),
             (zk + 0.02, 0.18, 0.16, 0.164)]
    sw = skirt_weights(p, zh + 0.02, zk + 0.02, follow=0.7)
    lathe(m, dress, 8, "outfit", sw)
    lathe(m, [(zk + 0.022, 0.172, 0.152, 0.156)], 8, "outfit_shade", sw, cap_bottom=zh)
    apron = [(z,) + tuple(v + 0.006 for v in profile_at(dress, z)) for z in (zc - 0.03, zh, zk + 0.04)]
    lathe_arc(m, apron, -40, 40, 2, "white", sw)
    limbs(p, long_sleeve=False, sleeve_r=0.05, sleeve_len=0.5, forearm_r=0.032, hand_r=0.04, shins=True,
          legs="leather_dark", leg_r=0.042, shoe_size=(0.05, 0.078, 0.046))
    return finish(p, zhip + 0.01)


def rook():
    """Captain Rook, the Flame champion: a brooding sky-pirate in a long dark coat with a high collar
    and flared tails, a captain's hat, and an eyepatch he doesn't need."""
    p = sb("rook", leg=0.44, torso=1.14, width=1.04, eyes_at=(21.0, -8.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.031, 0.044), brows=dict(el=11.0, width=0.042, thick=0.011, arch=0.003, tilt=0.004))
    shell(m, h, lambda az: angle_blend(az, {0: 10, 30: 14, 70: 6, 100: -16, 180: -34}),
          lambda az, f: 0.022 + 0.022 * (1 - f), n_az=12, rows=2, locks=lambda k, az: 12 if k % 2 == 0 else 0, phase=0.5,
          crown=(-0.03, 0.0))
    c = h.at(21, -8)  # the patch over his left eye (+X), standing just off the face, and its strap
    n = h.normal(c)
    decal(m, h, 21, -8, 0.04, 0.036, 0.003, 7, "pupil", HEAD, lift=0.016)
    ribbon(m, [h.at(-10, 20, 0.006), add(c, mul(n, 0.016)), h.at(95, -4, 0.008)], 0.008, lambda q: norm(
        (q[0] - h.c[0], q[1] - h.c[1], 0.0)), "pupil", HEAD)
    top = h.c[2] + h.azt  # the hat: a bicorne, crosswise
    ellipsoid(m, (0.0, h.c[1], top - 0.01), (0.2, 0.09, 0.07), 7, 3, "outfit_shade", HEAD)
    lathe(m, [(top - 0.04, 0.17, 0.17, 0.17, h.c[1]), (top - 0.06, 0.172, 0.172, 0.172, h.c[1])], 8, "brass", HEAD)
    zn, zc, zsp, zh, zhip, zk = heights(p)
    coat = [(zn + 0.06, 0.08, 0.074, 0.074), (zn - 0.04, 0.146, 0.112, 0.118), (zsp, 0.14, 0.12, 0.126),
            (zh, 0.15, 0.13, 0.136), (zk - 0.06, 0.2, 0.17, 0.21)]
    sw = skirt_weights(p, zh, zk - 0.06, follow=0.6)
    lathe(m, coat, 8, "outfit", sw, mats=["outfit_shade", "outfit", "outfit", "outfit"])
    hug(p, coat, zh + 0.02, zh - 0.01, 0.006, "leather", w=sw)
    hug(p, coat, zk - 0.04, zk - 0.06, 0.005, "brass", w=sw)
    limbs(p, leg_r=0.044, boot=(0.14, 0.052, "leather"), shoe_size=(0.054, 0.086, 0.05))
    return finish(p, zhip + 0.012)


def seraphine():
    """Seraphine, the Blaze champion: a dancer, poised and strict: hair in a high tight bun, a ribbon
    choker, a fitted bodice and a long wrap skirt that flows as she turns, soft dancing shoes."""
    p = sb("seraphine", leg=0.46, torso=1.12, width=0.9, eyes_at=(22.0, -8.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.033, 0.048), lashes=True, brows=dict(el=12.5, width=0.038, thick=0.007, arch=0.006, tilt=0.002))
    shell(m, h, lambda az: angle_blend(az, {0: 32, 40: 24, 80: 8, 100: -10, 180: -26}),
          lambda az, f: 0.014 + 0.01 * (1 - f), n_az=12, rows=2, crown=(0.0, 0.03))
    ellipsoid(m, h.at(180, 52, 0.05), (0.07, 0.065, 0.065), 6, 3, "hair", HEAD)  # the bun
    lathe(m, [(h.at(180, 40)[2] + 0.06, 0.04, 0.04, 0.04, h.at(180, 52)[1])], 6, "trim", HEAD,
          cap_top=h.at(180, 40)[2] + 0.07)
    zn, zc, zsp, zh, zhip, zk = heights(p)
    tw = torso_weights(p)
    lathe(m, [(zn + 0.02, 0.04, 0.036, 0.036), (zn + 0.005, 0.04, 0.036, 0.036)], 8, "trim", tw)  # the choker
    bodice = [(zn - 0.03, 0.1, 0.078, 0.08), (zsp, 0.088, 0.076, 0.078), (zh, 0.104, 0.09, 0.092)]
    lathe(m, bodice, 8, "outfit", tw)
    sw = skirt_weights(p, zh, 0.1, follow=0.65)
    skirt = [(zh + 0.002, 0.106, 0.092, 0.094), (zk, 0.17, 0.15, 0.156), (0.1, 0.22, 0.2, 0.21)]
    lathe(m, skirt, 9, "trim", sw)
    lathe(m, [(0.102, 0.214, 0.194, 0.204)], 9, "trim_shade", sw, cap_bottom=zk)
    tube(m, [garment_point(skirt, zh - 0.01, 40, 0.006), garment_point(skirt, zk + 0.06, 70, 0.006)],
         [(0.03, 0.008), (0.024, 0.006)], 3, "outfit", sw, tip1=0.03)  # the wrap's tie
    for side, s in SIDES:
        arm(p, side, sleeve_mat="outfit", long_sleeve=True, sleeve_r=0.044, hand_r=0.038, seg=5)
        shin(p, side, "skin", r=0.036, seg=5)
        shoe(p, side, "trim", size=(0.042, 0.074, 0.036), seg=5)
    return finish(p, zhip + 0.01)


def solenne():
    """Solenne, the Starfire champion: serene, an elf with long silver hair, violet eyes and a white
    flower behind her ear, a white dress under a lavender capelet trimmed in purple (run 22, Noah:
    after Emilia)."""
    p = sb("solenne", leg=0.44, torso=1.12, width=0.96, eyes_at=(22.0, -8.0))
    h, m = p.head, p.body
    sb_face(p, eye=(0.035, 0.052), lashes=True, ears="pointed",
            brows=dict(el=12.5, width=0.036, thick=0.006, arch=0.009))

    def edge(az):
        return angle_blend(az, {0: 18, 24: 12, 46: -2, 62: -80, 180: -86})
    shell(m, h, edge, lambda az, f: 0.02 + 0.026 * (1 - f) ** 1.2, n_az=12, rows=4,
          locks=lambda k, az: (18 if k % 2 == 0 else 0) if abs(az) > 45 else 0, curtain=-10, flare=-0.22, phase=0.5,
          row_bias=0.75)
    fl = h.at(-62, 30, 0.035)  # the flower behind her right ear (-X), five petals round a purple heart
    for k in range(5):
        a = k * 1.2566
        ellipsoid(m, add(fl, (0.026 * math.cos(a) * 0.5, -0.004, 0.026 * math.sin(a))), (0.016, 0.008, 0.016), 3, 2,
                  "white", HEAD)
    ellipsoid(m, add(fl, (0.0, -0.01, 0.0)), (0.01, 0.006, 0.01), 3, 2, "trim", HEAD)
    zn, zc, zsp, zh, zhip, zk = heights(p)
    sw = skirt_weights(p, zh, 0.1, follow=0.55)
    dress = [(zn + 0.012, 0.046, 0.042, 0.042), (zn - 0.04, 0.106, 0.084, 0.088), (zh, 0.104, 0.09, 0.092),
             (0.06, 0.2, 0.18, 0.19)]
    lathe(m, dress, 8, "white", sw)
    tw = torso_weights(p)
    cape = [(zn + 0.03, 0.058, 0.054, 0.054), (zn - 0.035, 0.13, 0.104, 0.116), (zc - 0.07, 0.16, 0.13, 0.15)]
    lathe(m, cape, 7, "outfit", tw)  # a lavender capelet over her shoulders
    hug(p, cape, zc - 0.05, zc - 0.07, 0.006, "trim", w=tw)  # its purple hem
    hug(p, dress, zh + 0.012, zh - 0.012, 0.006, "trim", w=sw)  # a purple sash
    for side, s in SIDES:
        arm(p, side, sleeve_mat="white", long_sleeve=True, sleeve_r=0.048, hand_r=0.04, seg=5)
        shoe(p, side, "trim", size=(0.044, 0.072, 0.04), seg=5)
    return finish(p, zhip + 0.012)


PEOPLE = {"player_a": player_a, "player_b": player_b, "keeper": keeper, "market": market, "sanctuary": sanctuary,
          "steward": steward, "child": child, "traveller": traveller, "fig": fig, "tam": tam, "tove": tove,
          "linnet": linnet, "madder": madder, "celestine": celestine, "primrose": primrose, "marigold": marigold,
          "rook": rook, "seraphine": seraphine, "solenne": solenne}
PLAYERS = ("player_a", "player_b")
VILLAGER_IDS = ("keeper", "market", "sanctuary", "steward", "child", "traveller")
STORY_IDS = ("fig", "tam", "tove", "linnet", "madder", "celestine", "primrose", "marigold", "rook", "seraphine",
             "solenne")


def build(pid):
    p = PEOPLE[pid]()
    for m in [p.body] + p.eyes + p.mouths + p.brows + p.hair:
        m.compact()
    return p
