"""Two candidate looks for the people (Noah, 2026-09-30: "two sets of two (boy and girl) that are
modeled and rendered in 3d for me to decide. One should be more like animal crossing, the other
is up to you. Make them clean and cute").

Both are built with the same kit and skeleton as people.py, so either set can replace player_a
and player_b (and later dress the villagers) without touching the game: the same 17 bones, the
same clips, the 600-triangle budget.

  Set 1, "Villager" (the Animal Crossing way): a big round head, about half the height, on a small
    body with short limbs; dot eyes with a glint, rosy cheeks, chunky hair. A striped tee and shorts;
    a dress with a round collar and twin tails.
  Set 2, "Storybook" (my pick): taller, about three heads, with big irised eyes, brows and (hers)
    lashes, and a dragon keeper's clothes. His: a tunic, a hooded capelet with a brass clasp, a
    satchel, boots. Hers: a long braid, a pinafore over a puff-sleeved blouse, and an egg carried in
    a pouch at her hip.

Previews: blender -b -P tools/blender/people_model.py -- --candidates --out <abs> --sheets candidates
"""
from body import (SIDES, arm, build_eyes, head_and_face, leg, person, profile_at, shin, shoe, skirt_weights,
                  smile, torso_weights)
from geom import decal, ellipsoid, lathe, ribbon, rigid, slab, tube
from hair import angle_blend, shell
from rig import add, lerp, norm

HEAD = rigid("head")


def zs(p):
    """The heights garments are cut to: the neck, chest, spine, hips joints and the hip sockets."""
    j = p.joints
    return j["neck"][0][2], j["chest"][0][2], j["spine"][0][2], j["hips"][0][2], j["leg_up_R"][0][2]


def hug(p, profile, z0, z1, off, mat, w=None, seg=8):
    r0, r1 = profile_at(profile, z0), profile_at(profile, z1)
    lathe(p.body, [(z0, r0[0] + off, r0[1] + off, r0[2] + off), (z1, r1[0] + off, r1[1] + off, r1[2] + off)],
          seg, mat, w or torso_weights(p))


def finish(p, hair_mesh_in_body=True):
    p.notes["seat"] = (0.0, 0.02, p.joints["leg_up_R"][0][2] + 0.012)
    return p


# ------------------------------------------------------------------------------ Set 1: Villager
VIL_HEAD = (0.27, 0.268, 0.245, 0.255, 0.272, 0.25)  # rounder, a full jaw


def vil_face(p):
    h, m = p.head, p.body
    head_and_face(p, seg=12, ears=None, blush=False, mouth=False, open_top=2,
                  brows=dict(az=27.0, el=13.0, width=0.04, thick=0.009, arch=0.006, n=3))
    for s in (-1, 1):  # big round rosy cheeks
        decal(m, h, s * 45, -27, 0.038, 0.028, 0.002, 6, "blush", HEAD, lift=0.0012)
    smile(m, h, HEAD, el=-34.0, width=0.044, curve=0.012)
    build_eyes(p, size=(0.036, 0.05), pupil=(0.8, 0.82))  # (the palette's iris is near black: dot eyes)


def vil_body(pid, **kw):
    kw.setdefault("width", 0.92)
    return person(pid, leg=0.2, torso=0.78, arm=0.74, head_scale=1.12, head_shape=VIL_HEAD, neck=0.04,
                  hip_width=0.84, eyes_at=(27.0, -12.0), **kw)


def vil_boy():
    """A striped tee, navy shorts, red trainers, a chunky side-swept mop with a cowlick."""
    p = vil_body("vil_boy")
    h, m = p.head, p.body
    vil_face(p)
    shell(m, h, lambda az: angle_blend(az, {0: 24, 30: 21, 60: 13, 85: 3, 100: -8, 140: -30, 180: -36}),
          lambda az, f: 0.034 + 0.05 * (1 - f) ** 1.3, n_az=12, rows=3, phase=0.5, crown=(-0.03, 0.02),
          locks=lambda k, az: (11 if k % 2 == 0 else 0) if abs(az) < 70 else (6 if k % 2 == 0 else 0))
    top = h.at(-20, 64, 0.05)
    tube(m, [top, add(top, (-0.01, 0.018, 0.04))], [(0.04, 0.026), (0.03, 0.018)], 4, "hair", HEAD, tip1=0.06)
    zn, zc, zsp, zh, zhip = zs(p)
    tw = torso_weights(p)
    tee = [(zn + 0.006, 0.05, 0.046, 0.046), (zn - 0.032, 0.104, 0.086, 0.09), (zsp, 0.104, 0.09, 0.094),
           (zh + 0.004, 0.112, 0.095, 0.099)]
    lathe(m, tee, 8, "outfit", tw)
    for z in (zc + 0.004, zsp + 0.024):  # two white stripes
        hug(p, tee, z, z - 0.022, 0.003, "trim")
    sw = skirt_weights(p, zh + 0.03, zh - 0.06, follow=0.8)
    lathe(m, [(zh + 0.014, 0.115, 0.097, 0.101), (zhip - 0.065, 0.12, 0.1, 0.104)], 8, "extra", sw,
          cap_bottom=zhip - 0.05)
    for side, s in SIDES:
        arm(p, side, sleeve_mat="outfit", sleeve_r=0.046, sleeve_len=0.6, forearm_r=0.03, hand_r=0.04)
        leg(p, side, "skin", r=0.035, top=0.03, bottom_z=0.06)
        shoe(p, side, "leather", size=(0.05, 0.074, 0.044))
    return finish(p)


def vil_girl():
    """A round-collared dress with a bow, twin tails tied with ribbons, a soft fringe, Mary Janes."""
    p = vil_body("vil_girl", width=0.9)
    h, m = p.head, p.body
    vil_face(p)
    shell(m, h, lambda az: angle_blend(az, {0: 19, 35: 17, 60: 8, 90: -10, 140: -30, 180: -34}),
          lambda az, f: 0.03 + 0.044 * (1 - f) ** 1.3, n_az=12, rows=3, phase=0.5,
          locks=lambda k, az: (5 if k % 2 == 0 else 0) if abs(az) < 50 else 0)
    for s in (-1, 1):  # twin tails, low at the sides, and their ribbons
        base = h.at(s * 104, -12, 0.02)
        tube(m, [base, add(base, (s * 0.07, 0.012, -0.05)), add(base, (s * 0.1, 0.03, -0.21))],
             [0.046, 0.062, 0.034], 4, "hair", HEAD, tip1=0.06)
        ellipsoid(m, h.at(s * 104, -12, 0.058), (0.032, 0.028, 0.03), 4, 2, "trim", HEAD)
    zn, zc, zsp, zh, zhip = zs(p)
    sw = skirt_weights(p, zsp, zhip - 0.07, follow=0.7)
    dress = [(zn + 0.006, 0.05, 0.045, 0.045), (zn - 0.032, 0.098, 0.082, 0.086), (zc - 0.03, 0.098, 0.086, 0.09),
             (zhip - 0.07, 0.158, 0.138, 0.143)]
    lathe(m, dress, 8, "outfit", sw)
    lathe(m, [(zhip - 0.068, 0.15, 0.13, 0.135)], 8, "outfit_shade", sw, cap_bottom=zhip - 0.02)
    hug(p, dress, zhip - 0.05, zhip - 0.07, 0.004, "trim", w=sw)
    tw = torso_weights(p)
    lathe(m, [(zn + 0.002, 0.058, 0.054, 0.054), (zn - 0.028, 0.104, 0.088, 0.092)], 8, "white", tw)
    ellipsoid(m, (0.0, -0.094, zn - 0.03), (0.042, 0.012, 0.016), 4, 2, "trim", tw)  # the bow under the collar
    for side, s in SIDES:
        arm(p, side, sleeve_mat="outfit", sleeve_r=0.05, sleeve_len=0.55, forearm_r=0.03, hand_r=0.04, seg=5)
        leg(p, side, "skin", r=0.034, top=0.02, bottom_z=0.06, seg=5)
        shoe(p, side, "leather", size=(0.047, 0.07, 0.042), seg=5)
    return finish(p)


# ------------------------------------------------------------------------------ Set 2: Storybook
def story_body(pid, **kw):
    kw.setdefault("width", 1.0)
    p = person(pid, leg=0.42, torso=1.12, arm=1.12, head_scale=0.8, neck=0.04, eyes_at=(22.0, -8.0), **kw)
    zn = p.joints["neck"][0][2]  # a neck (the smaller head sits clear of the collar)
    tube(p.body, [(0.0, 0.0, zn - 0.03), (0.0, 0.006, zn + 0.075)], [0.036, 0.033], 5, "skin", rigid("neck"))
    return p


def story_boy():
    """A young dragon keeper: a cream tunic belted in leather, a teal hooded capelet with a brass clasp,
    a satchel at his hip, tall boots, a swept-up fringe."""
    p = story_body("story_boy")
    h, m = p.head, p.body
    head_and_face(p, seg=10, ears=None, open_top=2, brows=dict(el=12.0, width=0.04, thick=0.009, arch=0.009))
    build_eyes(p, size=(0.033, 0.049))
    shell(m, h, lambda az: angle_blend(az, {0: 22, 25: 18, 60: 10, 88: 4, 100: -10, 150: -30, 180: -34}),
          lambda az, f: 0.026 + 0.04 * (1 - f) ** 1.4, n_az=12, rows=3, phase=0.5, crown=(0.025, 0.01),
          locks=lambda k, az: (13 if az < 40 else 8) if k % 2 == 0 else 0)
    zn, zc, zsp, zh, zhip = zs(p)
    sw = skirt_weights(p, zh, zhip - 0.1, follow=0.65)
    tunic = [(zn + 0.01, 0.05, 0.046, 0.046), (zn - 0.04, 0.12, 0.094, 0.098), (zhip - 0.1, 0.15, 0.124, 0.128)]
    lathe(m, tunic, 8, "outfit", sw)
    hug(p, tunic, zh + 0.03, zh - 0.0, 0.006, "leather", w=torso_weights(p))
    # the capelet over the shoulders, and its hood lying down at the back
    cw = rigid("chest")
    cape = [(zn + 0.03, 0.06, 0.054, 0.058), (zn - 0.035, 0.132, 0.104, 0.122), (zc + 0.01, 0.172, 0.138, 0.164),
            (zc - 0.06, 0.188, 0.152, 0.18)]
    lathe(m, cape, 8, "trim", cw)
    lathe(m, [(zc - 0.058, 0.182, 0.146, 0.174)], 8, "trim_shade", cw, cap_bottom=zc - 0.01)
    ellipsoid(m, (0.0, 0.128, zn - 0.035), (0.11, 0.045, 0.078), 5, 2, "trim", cw)  # the hood, lying down
    ellipsoid(m, (0.0, -0.073, zn - 0.01), (0.02, 0.012, 0.02), 4, 2, "brass", cw)
    # the satchel on his left hip (+X), its strap from the right shoulder
    hw = rigid("hips")
    slab(m, (0.14, -0.02, zh - 0.02), (0.94, 0.34, 0.0), (-0.34, 0.94, 0.0), (0.0, 0.0, 1.0), (0.06, 0.028, 0.055),
         "leather", hw)
    for side, s in SIDES:
        arm(p, side, sleeve_mat="outfit", long_sleeve=True, sleeve_r=0.052, hand_r=0.042, seg=5)
        leg(p, side, "leather_dark", r=0.046)
        shoe(p, side, "leather", size=(0.052, 0.08, 0.05), shaft=(0.1, 0.052, "leather"), seg=5)
    return finish(p)


def story_girl():
    """A young dragon keeper: a berry pinafore over a white puff-sleeved blouse, a long braid with a
    ribbon, a soft parted fringe, and an egg in a leather pouch at her hip."""
    p = story_body("story_girl", width=0.94)
    h, m = p.head, p.body
    head_and_face(p, seg=10, ears=None, open_top=2, brows=dict(el=12.5, width=0.038, thick=0.007, arch=0.01))
    build_eyes(p, size=(0.034, 0.051), lashes=True)
    shell(m, h, lambda az: angle_blend(az, {0: 16, 20: 20, 45: 10, 70: -12, 100: -28, 150: -38, 180: -40}),
          lambda az, f: 0.018 + 0.028 * (1 - f) ** 1.3, n_az=10, rows=3, phase=0.0,
          locks=lambda k, az: (7 if k % 2 == 1 else 0) if abs(az) < 60 else 0)
    for s in (-1, 1):  # two locks framing her face
        base = h.at(s * 60, 2, 0.018)
        tube(m, [base, add(base, (s * 0.004, -0.012, -0.14))], [(0.03, 0.02), (0.02, 0.013)], 3, "hair", HEAD,
             tip1=0.03, up_hint=(s * 1.0, 0.0, 0.0))
    nape = h.at(180, -30, 0.01)  # the braid: plaits down her back to the waist, a ribbon near the end
    pts = [nape, add(nape, (0.0, 0.05, -0.1)), add(nape, (0.0, 0.074, -0.28)), add(nape, (0.0, 0.07, -0.42))]
    tube(m, pts, [0.05, 0.046, 0.038, 0.03], 4, "hair", HEAD, tip1=0.05)
    tie = add(nape, (0.0, 0.072, -0.35))
    tube(m, [add(tie, (0.0, 0.0, 0.02)), add(tie, (0.0, 0.0, -0.02))], [0.04, 0.038], 3, "trim", HEAD)
    zn, zc, zsp, zh, zhip = zs(p)
    tw = torso_weights(p)
    lathe(m, [(zn + 0.01, 0.048, 0.044, 0.044), (zn - 0.045, 0.112, 0.088, 0.092), (zc - 0.01, 0.106, 0.09, 0.092)], 7,
          "white", tw)
    sw = skirt_weights(p, zsp, zhip - 0.12, follow=0.7)
    pinafore = [(zc - 0.005, 0.1, 0.086, 0.088), (zh, 0.128, 0.11, 0.114), (zhip - 0.13, 0.19, 0.166, 0.172)]
    lathe(m, pinafore, 8, "outfit", sw)
    lathe(m, [(zhip - 0.128, 0.182, 0.158, 0.164)], 8, "outfit_shade", sw, cap_bottom=zhip - 0.06)
    hug(p, pinafore, zhip - 0.105, zhip - 0.13, 0.004, "trim", w=sw)
    for s in (-1, 1):  # the pinafore's straps over the shoulders
        ribbon(m, [(s * 0.055, -0.094, zc - 0.004), (s * 0.078, 0.0, zn - 0.012), (s * 0.055, 0.094, zc - 0.004)], 0.024, lambda q: norm((q[0], q[1], 0.25)), "outfit", tw)
    # the egg pouch on her left hip (+X): a leather cup, a cream speckled egg peeking out
    hw = rigid("hips")
    pc = (0.15, -0.03, zh - 0.05)
    lathe(m, [(pc[2] + 0.03, 0.05, 0.05, 0.05, pc[1]), (pc[2] - 0.04, 0.04, 0.04, 0.04, pc[1])], 5, "leather", hw,
          cap_bottom=pc[2] - 0.05, cx=pc[0])
    ellipsoid(m, (pc[0], pc[1], pc[2] + 0.05), (0.04, 0.04, 0.052), 5, 3, "extra", hw)
    for side, s in SIDES:
        arm(p, side, sleeve_mat="white", sleeve_r=0.064, sleeve_len=0.5, forearm_r=0.034, hand_r=0.041, seg=5)
        shin(p, side, "leather_dark", r=0.042, seg=5)
        shoe(p, side, "leather", size=(0.05, 0.076, 0.048), seg=5)
    return finish(p)


PEOPLE = {"vil_boy": vil_boy, "vil_girl": vil_girl, "story_boy": story_boy, "story_girl": story_girl}
SETS = [("Set 1: Villager", ("vil_boy", "vil_girl")), ("Set 2: Storybook", ("story_boy", "story_girl"))]

_FIXED = {"glint": (255, 255, 255), "pupil": (28, 20, 26), "tongue": (236, 124, 124)}
PALETTES = {  # slot -> sRGB (see looks.py: base skin, accent outfit, pattern trim, horn hair, membrane
    #            leather, iris, glow extra)
    "vil_boy": dict(_FIXED, base=(255, 222, 196), accent=(96, 160, 214), pattern=(250, 246, 236), horn=(132, 78, 44),
                    membrane=(222, 84, 72), iris=(40, 30, 38), glow=(62, 80, 138)),
    "vil_girl": dict(_FIXED, base=(240, 192, 152), accent=(246, 168, 186), pattern=(214, 70, 96), horn=(82, 54, 40),
                     membrane=(196, 58, 70), iris=(40, 30, 38), glow=(255, 206, 120)),
    "story_boy": dict(_FIXED, base=(240, 196, 160), accent=(236, 224, 196), pattern=(63, 150, 152), horn=(150, 86, 46),
                      membrane=(126, 84, 54), iris=(186, 118, 48), glow=(255, 206, 120)),
    "story_girl": dict(_FIXED, base=(255, 222, 196), accent=(176, 62, 82), pattern=(246, 200, 110), horn=(78, 52, 46),
                       membrane=(126, 84, 54), iris=(76, 128, 78), glow=(250, 238, 210)),
}


def build(pid):
    p = PEOPLE[pid]()
    for mm in [p.body] + p.eyes + p.hair:
        mm.compact()
    return p


def triangles(p):
    return len(p.body.tris) + len(p.eyes[0].tris) + (max(len(x.tris) for x in p.hair) if p.hair else 0)


if __name__ == "__main__":
    for pid in PEOPLE:
        p = build(pid)
        top = max(q[2] for q in p.body.pos)
        print(f"{pid}: {triangles(p)} triangles (body {len(p.body.tris)}, eyes {len(p.eyes[0].tris)}), "
              f"height {top:.3f} m, head {p.head.c[2] - p.head.azb:.3f}-{p.head.c[2] + p.head.azt:.3f} m")
