"""The Glimmermoth's body plan: a slender four-legged dragon with FOUR moth wings (a forewing
and a hindwing on each side) and two feathery antennae on bones of their own.

Skeleton (40 bones): the classic dragon's 26 body bones (hips .. tail4, three neck bones, four
three-bone legs, jaw, eyes), the two antennae (antenna_L/R, children of the head: they wave,
perk up and lie back), and six wing bones a side:

  forewing  wing_arm  root -> elbow    the inner half of the leading edge (the costa)
            wing_fore elbow -> apex    the outer leading edge, to the wing's tip
            wing_f1   elbow -> tornus  across the wing to its trailing corner
  hindwing  wing_hind root -> hmid     into the rounded lobe
            wing_h1   hmid -> tailbase to where the long trailing tail begins
            wing_h2   tailbase -> tailtip  the luna-moth tail itself

Both wings of a side grow from the same seated root on the withers, each chain on the chest.
The body draw is the 25 body bones the skin uses (the antennae carry only parts); the wing
draw is the 12 wing bones.

Moth wings are stiff plates, so the wings are posed as plates: a pose names where each plate
points (its inner edge along the back, its upper face out and up) and plate_pose() turns
that into the (pitch, yaw, roll) deltas the clips key, for the wing frame every form of a
kind on this plan lays its wings out in (DIHEDRAL, DROOP, and the plates' edge angles in the
layout: FORE_EDGES, HIND_EDGES). At rest the wings fold back over the body like a moth's
roof, the hindwings under the forewings, their long tails trailing beside the tail; the idle
fans them gently; in flight both pairs beat, the hindwings a beat behind.

The body clips start from the classic dragon's (tools/anim/clips.py), which were made for
these body bones, with the classic bat wings taken out and this plan's wings, antennae and
fluttery character put in.
"""
import math

from dragons import clipkit

NAME = "glimmermoth"

# ------------------------------------------------------------------------------ skeleton
BONES = [
    ("hips", "hips", "belly", None),
    ("belly", "belly", "chest", "hips"),
    ("chest", "chest", "neck1", "belly"),
    ("neck1", "neck1", "neck2", "chest"),
    ("neck2", "neck2", "neck3", "neck1"),
    ("neck3", "neck3", "head", "neck2"),
    ("head", "head", "muzzle", "neck3"),
    ("snout", "muzzle", "snout", "head"),
    ("tail1", "hips", "tail2", "hips"),
    ("tail2", "tail2", "tail3", "tail1"),
    ("tail3", "tail3", "tail4", "tail2"),
    ("tail4", "tail4", "tail_tip", "tail3"),
]
for _side in ("L", "R"):
    BONES += [
        (f"arm_up_{_side}", f"shoulder_{_side}", f"elbow_{_side}", "chest"),
        (f"arm_lo_{_side}", f"elbow_{_side}", f"wrist_{_side}", f"arm_up_{_side}"),
        (f"hand_{_side}", f"wrist_{_side}", f"toe_f_{_side}", f"arm_lo_{_side}"),
        (f"leg_up_{_side}", f"hipj_{_side}", f"knee_{_side}", "hips"),
        (f"leg_lo_{_side}", f"knee_{_side}", f"ankle_{_side}", f"leg_up_{_side}"),
        (f"foot_{_side}", f"ankle_{_side}", f"toe_b_{_side}", f"leg_lo_{_side}"),
    ]
BONES.append(("jaw", "jaw_hinge", "jaw_tip", "snout"))
BONES.append(("eyes", "eyes_c", "eyes_tip", "head"))
BONES += [(f"antenna_{s}", f"ant_{s}", f"anttip_{s}", "head") for s in ("L", "R")]

WING_CHAIN = [("wing_arm", "root", "elbow", "chest"), ("wing_fore", "elbow", "apex", "wing_arm"),
              ("wing_f1", "elbow", "tornus", "wing_arm"),
              ("wing_hind", "root", "hmid", "chest"), ("wing_h1", "hmid", "tailbase", "wing_hind"),
              ("wing_h2", "tailbase", "tailtip", "wing_h1")]
FOREWING = ("wing_arm", "wing_fore", "wing_f1")
HINDWING = ("wing_hind", "wing_h1", "wing_h2")
WING_BONES = [f"{n}_{side}" for side in ("L", "R") for n, *_ in WING_CHAIN]
BONE_ORDER = [b[0] for b in BONES] + WING_BONES
WING_BODY = ("chest",)
CONTACTS = ("hand_L", "hand_R", "foot_L", "foot_R")
SEAT = ("belly", (0.0, 0.15, 0.5))
REGION = {"antenna": "head"}

# ------------------------------------------------------------------------------ the wing frame
# The rest (spread) wing plane, as the kit's wing_points lays it out: the span axis (the
# layout's u) rises by DIHEDRAL, the chord axis (v, back along the body) droops by DROOP.
DIHEDRAL, DROOP = 14.0, 4.0
# Each plate is a fan from the root: its leading edge (costa) and its inner edge at these
# angles in the layout (degrees from +u, the span, toward +v, the chord: 90 points back).
FORE_EDGES = (-12.0, 34.0)
HIND_EDGES = (14.0, 96.0)


def _v(*a):
    return list(a)


def _add(a, b):
    return [x + y for x, y in zip(a, b)]


def _mul(a, k):
    return [x * k for x in a]


def _dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def _cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def _unit(a):
    n = math.sqrt(_dot(a, a)) or 1.0
    return [x / n for x in a]


def _rot(axis, deg):
    """Rotation matrix (rows) about a unit axis."""
    x, y, z = _unit(axis)
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    t = 1 - c
    return [[t * x * x + c, t * x * y - s * z, t * x * z + s * y],
            [t * x * y + s * z, t * y * y + c, t * y * z - s * x],
            [t * x * z - s * y, t * y * z + s * x, t * z * z + c]]


def _mm(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]


def _mv(m, v):
    return [sum(m[i][k] * v[k] for k in range(3)) for i in range(3)]


def _cols(a, b, c):
    return [[a[i], b[i], c[i]] for i in range(3)]


def _transpose(m):
    return [[m[j][i] for j in range(3)] for i in range(3)]


def rest_axes():
    """The right wing's span and chord axes and its upper-face normal, at rest."""
    th, ph = math.radians(DIHEDRAL), math.radians(DROOP)
    span = [math.cos(th), 0.0, math.sin(th)]
    chord = [0.0, math.cos(ph), -math.sin(ph)]
    return span, chord, _unit(_cross(span, chord))


def rest_dir(theta):
    """A direction in the right wing's rest plane, at angle theta in the layout."""
    span, chord, _ = rest_axes()
    t = math.radians(theta)
    return _unit(_add(_mul(span, math.cos(t)), _mul(chord, math.sin(t))))


def pyr(m):
    """(pitch, yaw, roll) of a rotation matrix, as eca.q_from_pyr builds them:
    M = Rz(yaw) Rx(-pitch) Ry(-roll)."""
    a = math.asin(max(-1.0, min(1.0, m[2][1])))
    b = math.atan2(-m[2][0], m[2][2])
    c = math.atan2(-m[0][1], m[1][1])
    return (round(-math.degrees(a), 2), round(math.degrees(c), 2), round(-math.degrees(b), 2))


def from_pyr(p, y, r):
    return _mm(_rot((0, 0, 1), y), _mm(_rot((-1, 0, 0), p), _rot((0, -1, 0), r)))


def plate_rotation(edge_theta, direction, normal):
    """The rotation taking the right wing's rest plate so that its edge at edge_theta points
    along `direction` and its upper face faces `normal` (made square to the direction)."""
    _, _, n0 = rest_axes()
    e0 = rest_dir(edge_theta)
    e1 = _unit(direction)
    n1 = _unit(_add(normal, _mul(e1, -_dot(normal, e1))))
    return _mm(_cols(e1, n1, _cross(e1, n1)), _transpose(_cols(e0, n0, _cross(e0, n0))))


def plate_pose(edge_theta, direction, normal, then=None):
    """Plate pose as (pitch, yaw, roll); `then`: an extra armature-space rotation matrix after."""
    m = plate_rotation(edge_theta, direction, normal)
    if then is not None:
        m = _mm(then, m)
    return pyr(m)


# ------------------------------------------------------------------------------ wing poses
# Poses name the right side ("name*" keys both sides; eca mirrors the left).
ROOF_SLOPE, BACK_FALL, BACK_TOE, HIND_DROP, TAIL_DROOP = 50.0, 7.0, 4.0, 5.0, -24.0


def _back(fall=BACK_FALL, toe=BACK_TOE):
    """The back line the folded wings' inner edges lie along (right side)."""
    return _unit([-math.sin(math.radians(toe)), math.cos(math.radians(fall)), -math.sin(math.radians(fall))])


def _slope_normal(slope):
    s = math.radians(slope)
    return [math.sin(s), 0.0, math.cos(s)]


def roof(lift=0.0, hind_lift=None, fall=BACK_FALL, toe=BACK_TOE, hind_drop=HIND_DROP, droop=TAIL_DROOP):
    """Wings folded back over the body like a moth's roof: each plate's inner edge runs back
    along the back (falling `fall` degrees, toed in `toe`), its upper face out and up at
    ROOF_SLOPE from level; `lift` opens the roof about the back line (46: flat out like a
    delta, showing the eyespots; 100: raised in a V). The hindwing's inner edge sits
    `hind_drop` degrees lower in the roof, tucked under the forewing, and its long tail
    droops `droop` degrees down the roof."""
    back = _back(fall, toe)
    fore = plate_rotation(FORE_EDGES[1], back, _slope_normal(ROOF_SLOPE - lift))
    n = _slope_normal(ROOF_SLOPE - (lift if hind_lift is None else hind_lift))
    hind = plate_rotation(HIND_EDGES[1], _mv(_rot(n, -hind_drop), back), n)
    return {"wing_arm*": pyr(fore), "wing_hind*": pyr(hind), "wing_h2*": pyr(_rot(rest_axes()[2], -droop))}


def raised(fore=20.0, hind=12.0, sweep=0.0, hind_sweep=None):
    """Spread wings raised by a roll (degrees; negative lowers them), swept back by `sweep`."""
    hs = sweep if hind_sweep is None else hind_sweep
    return {"wing_arm*": (0.0, sweep, fore), "wing_hind*": (0.0, hs, hind)}


def _both(bone, p, y, r):
    return {f"{bone}_R": (p, y, r), f"{bone}_L": (p, -y, -r)}


def _sides(pose, side_only=None):
    """A pose with its "name*" keys written out per side (optionally one side only)."""
    out = {}
    for k, v in pose.items():
        if k.endswith("*"):
            for s, val in (("R", v), ("L", (v[0], -v[1], -v[2]))):
                if side_only in (None, s):
                    out[f"{k[:-1]}_{s}"] = val
        else:
            out[k] = v
    return out


def fan(delta, hind_delta=None):
    """The folded wings opened `delta` degrees further about the back line (a wave's extra
    rotation, applied on top of the roof)."""
    back = _back()
    out = _both("wing_arm", *pyr(_rot(back, -delta)))
    out.update(_both("wing_hind", *pyr(_rot(back, -(delta if hind_delta is None else hind_delta)))))
    return out


WINGS_FOLDED = roof()
WINGS_SIT = roof(fall=BACK_FALL - 24)              # sitting tips the body back: the roof stays level
WINGS_FAN = roof(lift=14, hind_lift=9)             # opened a little
WINGS_HALF = roof(lift=46, hind_lift=40)           # flat out over the back: the eyespots shown
WINGS_UP = roof(lift=104, hind_lift=96)            # raised in a V over the back
WINGS_OPEN = raised(22, 14)                        # spread and raised: jumps, flutters
WINGS_STRETCH = raised(34, 26, sweep=-6)           # the morning stretch
WINGS_BELLY = raised(-26, -30)                     # spread flat on the floor (lying on its back)

ANT_REST = {"antenna*": (0.0, 0.0, 0.0)}
ANT_PERK = {"antenna*": (-14.0, 0.0, 4.0)}          # forward and up: curious, eager
ANT_BACK = {"antenna*": (26.0, 0.0, -6.0)}          # laid back: content, or wary
ANT_SLEEP = {"antenna*": (40.0, 0.0, -14.0)}        # flat back along the neck
ANT_DROOP = {"antenna*": (18.0, 0.0, -26.0)}        # drooping out to the sides: sad


# ------------------------------------------------------------------------------ clips
def _classic():
    import importlib
    return importlib.import_module("clips")


def _body(pose):
    """A classic body pose without the classic bat wings."""
    return {k: v for k, v in pose.items() if not k.startswith("wing")}


def merge(*poses):
    out = {}
    for p in poses:
        for bone, v in p.items():
            a = out.get(bone, (0.0, 0.0, 0.0))
            out[bone] = tuple(x + y for x, y in zip(a, v))
    return out


def sin01(t, period, phase=0.0):
    return math.sin(2 * math.pi * (t / period + phase))


def breathe(amount=1.0, period=3.2):
    def fn(t):
        s = sin01(t, period) * amount
        return {"chest": (1.2 * s, 0, 0), "belly": (-0.6 * s, 0, 0), "neck1": (0.8 * s, 0, 0), "head": (-0.8 * s, 0, 0)}
    return fn


def fanning(period, amount=10.0, lag=0.1, phase=0.0):
    """The idle's slow fanning: the folded wings lift open a little and settle, once a period,
    the hindwings a moment behind."""
    def fn(t):
        a = amount * (0.5 - 0.5 * math.cos(2 * math.pi * (t / period + phase)))
        b = 0.8 * amount * (0.5 - 0.5 * math.cos(2 * math.pi * (t / period + phase - lag)))
        return fan(a, b)
    return fn


def flutter(t0, t1, amp=16.0, period=0.18, lag=0.14):
    """Quick little wingbeats between t0 and t1 (fading in and out), hindwings behind."""
    def fn(t):
        if not t0 < t < t1:
            return {}
        k = math.sin(math.pi * (t - t0) / (t1 - t0)) ** 0.5
        out = _both("wing_arm", 0, 0, amp * k * sin01(t - t0, period))
        out.update(_both("wing_hind", 0, 0, 0.85 * amp * k * sin01(t - t0, period, -lag)))
        out.update(_both("wing_h2", 0, 0, 0.6 * amp * k * sin01(t - t0, period, -lag - 0.25)))
        return out
    return fn


def wingbeat(period, amp, lag=0.12, hind_amp=None, sweep=7.0, flex=14.0, tail=16.0, hind_low=6.0, bob=2.5):
    """Flying: both pairs beat (roll), sweeping forward on the downstroke and back on the
    upstroke, the outer wing flexing behind, the hindwings `lag` of a beat behind the
    forewings and a little lower, their long tails streaming."""
    ha = amp * 0.9 if hind_amp is None else hind_amp

    def fn(t):
        s, c = sin01(t, period), sin01(t, period, 0.25)
        sh, ch = sin01(t, period, -lag), sin01(t, period, 0.25 - lag)
        f, fh = sin01(t, period, -0.16), sin01(t, period, -lag - 0.16)
        out = _both("wing_arm", 0, sweep * c, amp * s)
        out.update(_both("wing_fore", 0, 0, flex * f))
        out.update(_both("wing_f1", 0, 0, 0.7 * flex * f))
        out.update(_both("wing_hind", 0, sweep * ch, ha * sh - hind_low))
        out.update(_both("wing_h1", 0, 0, 0.8 * flex * fh))
        out.update(_both("wing_h2", 0, 0, tail * sin01(t, period, -lag - 0.3)))
        out["chest"] = (bob * s, 0, 0)
        out["neck1"] = (-0.8 * bob * s, 0, 0)
        return out
    return fn


def antenna_sway(amount=3.0, period=3.2, phase=0.0):
    """The antennae drifting gently, each on its own beat."""
    def fn(t):
        return {"antenna_R": (amount * sin01(t, period, phase), 0, 0.6 * amount * sin01(t, period, phase + 0.3)),
                "antenna_L": (amount * sin01(t, period, phase + 0.17), 0, -0.6 * amount * sin01(t, period, phase + 0.5))}
    return fn


def antenna_wave(t0, t1, amount=12.0, period=0.34):
    """The antennae waving (tasting the air, drawn to a light) between t0 and t1."""
    def fn(t):
        if not t0 < t < t1:
            return {}
        k = math.sin(math.pi * (t - t0) / (t1 - t0))
        return {"antenna_R": (amount * k * sin01(t, period), 0, 0.5 * amount * k * sin01(t, period, 0.25)),
                "antenna_L": (amount * k * sin01(t, period, 0.5), 0, -0.5 * amount * k * sin01(t, period, 0.75))}
    return fn


def tail_sway(amount=1.0, period=3.2):
    def fn(t):
        return {f"tail{k}": (0, amount * (2 + 2 * k) * sin01(t, period, -0.12 * k), 0) for k in range(1, 5)}
    return fn


# How each classic clip's wings and antennae go on this body: [(time, wing pose)], an antenna
# mood and whether the clip comes back to the idle's antennae at its end, and fluttering.
CLASSIC_WINGS = {
    "wake": [(0.0, WINGS_FOLDED), (0.6, WINGS_FOLDED), (1.3, WINGS_STRETCH), (1.9, WINGS_HALF), (2.6, WINGS_FOLDED)],
    "fav_wiggle": [(0.0, WINGS_FOLDED), (0.3, WINGS_HALF), (1.3, WINGS_HALF), (1.6, WINGS_FOLDED)],
    "roll_over": [(0.0, WINGS_FOLDED), (0.45, WINGS_FOLDED), (1.1, WINGS_BELLY)],
    "belly_rub": [(0.0, WINGS_BELLY)],
    "hop": [(0.0, WINGS_FOLDED), (0.25, WINGS_HALF), (0.45, WINGS_OPEN), (0.7, WINGS_HALF), (0.9, WINGS_FOLDED)],
    "pounce": [(0.0, WINGS_FOLDED), (0.35, WINGS_HALF), (0.65, WINGS_HALF), (0.85, WINGS_OPEN), (1.05, WINGS_HALF),
               (1.4, WINGS_FOLDED)],
    "play_bow": [(0.0, WINGS_FOLDED), (0.3, WINGS_HALF), (0.9, WINGS_HALF), (1.2, WINGS_FOLDED)],
    "spar": [(0.0, roof(lift=46, hind_lift=40, fall=BACK_FALL - 22))],
    "sit": [(0.0, WINGS_FOLDED), (0.8, WINGS_SIT)],
    "sit_loop": [(0.0, WINGS_SIT)],
    "leg_kick": [(0.0, WINGS_FOLDED), (0.25, WINGS_SIT), (1.15, WINGS_SIT), (1.4, WINGS_FOLDED)],
    "drop_wait": [(0.0, WINGS_FOLDED), (0.5, WINGS_FOLDED), (0.9, WINGS_SIT), (1.3, WINGS_SIT)],
    "drop_wait_h": [(0.0, WINGS_FOLDED), (0.5, WINGS_FOLDED), (0.9, WINGS_SIT), (1.3, WINGS_SIT)],
    "tail_chase": [(0.0, WINGS_FAN)],
    "greet": [(0.0, WINGS_FOLDED), (0.2, WINGS_HALF), (0.45, WINGS_OPEN), (0.7, WINGS_HALF), (1.0, WINGS_HALF),
              (1.6, WINGS_FOLDED)],
    "leap_catch": [(0.0, WINGS_FOLDED), (0.15, WINGS_HALF), (0.32, WINGS_OPEN), (0.45, WINGS_OPEN), (0.7, WINGS_HALF),
                   (0.9, WINGS_FOLDED)],
}
FLUTTERS = {"hop": [(0.3, 0.72, 18.0)], "pounce": [(0.7, 1.08, 18.0)], "greet": [(0.3, 1.05, 14.0)],
            "leap_catch": [(0.18, 0.66, 20.0)], "fav_wiggle": [(0.35, 1.3, 10.0)], "spar": [(0.0, 1.0, 8.0)],
            "play_bow": [(0.35, 0.85, 6.0)]}
MOODS = {  # clip: (antenna pose, back to rest at the end, sway amount)
    "sleep": (ANT_SLEEP, False, 1.0), "curl_up": (ANT_SLEEP, False, 0.0), "lie_loop": (ANT_BACK, False, 2.0),
    "lie_down": (ANT_BACK, False, 0.0), "nap_flop": (ANT_SLEEP, False, 0.0), "wake": (ANT_PERK, True, 0.0),
    "sit": (ANT_REST, False, 0.0), "sit_loop": (ANT_REST, False, 3.0), "yawn": (ANT_BACK, True, 0.0),
    "sulk": (ANT_DROOP, False, 0.0), "sulk_loop": (ANT_DROOP, False, 1.5), "pull_away": (ANT_BACK, True, 0.0),
    "sniff_refuse": (ANT_BACK, True, 0.0), "pet_head": (ANT_BACK, False, 2.0), "pet_chin": (ANT_BACK, False, 2.0),
    "nuzzle": (ANT_BACK, False, 2.0), "belly_rub": (ANT_BACK, False, 3.0), "roll_over": (ANT_BACK, False, 0.0),
    "greet": (ANT_PERK, True, 6.0), "hop": (ANT_PERK, True, 6.0), "pounce": (ANT_PERK, True, 5.0),
    "leap_catch": (ANT_PERK, True, 6.0), "play_bow": (ANT_PERK, True, 5.0), "spar": (ANT_PERK, False, 6.0),
    "tail_chase": (ANT_PERK, False, 6.0), "fav_wiggle": (ANT_PERK, True, 6.0), "tail_wag": (ANT_PERK, False, 5.0),
    "drop_wait": (ANT_PERK, False, 4.0), "drop_wait_h": (ANT_PERK, False, 4.0), "stalk": (ANT_PERK, False, 2.0),
    "paw_bat": (ANT_PERK, False, 5.0), "tug": (ANT_BACK, False, 4.0), "eat": (ANT_PERK, False, 3.0),
    "eat_h": (ANT_PERK, False, 3.0), "walk": (ANT_REST, False, 4.0), "walk_h": (ANT_REST, False, 5.0),
    "trot": (ANT_BACK, False, 5.0), "scamper": (ANT_BACK, False, 7.0), "gallop": (ANT_SLEEP, False, 6.0),
    "carry": (ANT_REST, False, 4.0), "carry_h": (ANT_REST, False, 5.0), "shuffle": (ANT_REST, False, 3.0),
    "scratch": (ANT_BACK, True, 0.0), "leg_kick": (ANT_BACK, True, 4.0), "sneeze": (ANT_PERK, True, 0.0),
    "pick_up": (ANT_PERK, True, 0.0), "pick_up_h": (ANT_PERK, True, 0.0),
}
CLASSIC_WING_BONES = [f"{n}_{s}" for n in ("wing_arm", "wing_fore", "wing_f1", "wing_f2", "wing_f3", "wing_f4")
                      for s in ("L", "R")]


def _keys(c, t, pose):
    """Plain keys for a pose (no pose() completeness: other posed bones are left alone)."""
    c.key(t, **_sides(pose))
    return c


def _moody(c, mood):
    """Key the antennae's mood across a clip (easing in and, if it returns, out)."""
    pose, back, sway = mood
    if c.loop:
        _keys(c, 0.0, pose)
    elif not back:
        _keys(_keys(c, 0.0, ANT_REST), min(0.3, c.length / 3), pose)
    else:
        ease = min(0.3, c.length / 4)
        for t, p in ((0.0, ANT_REST), (ease, pose), (c.length - ease, pose), (c.length, ANT_REST)):
            _keys(c, t, p)
    if sway:
        c.wave(antenna_sway(sway, c.length if c.loop else 1.2))


def _reshaped(clip):
    """A classic clip on this body: the bat wings out, the moth wings, antennae and flutters in."""
    c = clipkit.retarget(clip, BONE_ORDER, rename={b: None for b in CLASSIC_WING_BONES})
    for t, pose in CLASSIC_WINGS.get(c.name, [(0.0, WINGS_FOLDED)]):
        c.pose(t, pose)
    for t0, t1, amp in FLUTTERS.get(c.name, []):
        c.wave(flutter(t0, t1, amp))
    _moody(c, MOODS.get(c.name, (ANT_REST, False, 3.0)))
    return c


def _own_clips():
    """The clips this body does its own way."""
    cl = _classic()
    Clip = clipkit.Clip
    F = WINGS_FOLDED
    out = []
    # Idle: breathing, the tail swaying, the folded wings fanning open a little and settling,
    # the antennae drifting.
    out.append(Clip("idle", 3.2, loop=True).pose(0.0, merge(F, ANT_REST)).wave(breathe()).wave(tail_sway())
               .wave(fanning(3.2, 11.0)).wave(antenna_sway(3.0, 3.2)))
    # Drawn to a light: a look to one side with the head tilting, the antennae perking and
    # waving at it, then the other side.
    la = Clip("look_around", 3.0).pose(0.0, merge(F, ANT_REST))
    la.key(0.0, neck2=(0, 0, 0), neck3=(0, 0, 0), head=(0, 0, 0))
    la.key(0.8, neck2=(4, 16, 0), neck3=(2, 14, 0), head=(8, 12, 16))
    la.key(1.5, neck2=(4, 16, 0), neck3=(2, 14, 0), head=(10, 12, 20))
    la.key(2.2, neck2=(2, -18, 0), neck3=(0, -16, 0), head=(6, -14, -16))
    la.key(3.0, neck2=(0, 0, 0), neck3=(0, 0, 0), head=(0, 0, 0))
    la.pose(0.5, merge(F, ANT_PERK)).pose(2.5, merge(F, ANT_PERK)).pose(3.0, merge(F, ANT_REST))
    la.wave(breathe()).wave(tail_sway()).wave(antenna_wave(0.6, 1.6)).wave(antenna_wave(1.9, 2.7, 10.0))
    la.wave(fanning(3.0, 6.0)).event(0.8, "sniff")
    out.append(la)
    # A quick flutter: the wings open out of the roof, beat, and fold again.
    wf = Clip("wing_flutter", 1.2).pose(0.0, merge(F, ANT_REST)).pose(0.22, merge(WINGS_OPEN, ANT_PERK))
    wf.pose(0.9, merge(WINGS_OPEN, ANT_PERK)).pose(1.2, merge(F, ANT_REST))
    wf.wave(flutter(0.2, 0.95, 26.0, 0.22)).event(0.3, "flap").event(0.52, "flap").event(0.74, "flap")
    out.append(wf)
    # Lifting a wing to be groomed: both wings of the right side open up together.
    up = merge(_sides(WINGS_UP, "R"), _sides(F, "L"))
    lw = Clip("lift_wing", 1.4).pose(0.0, merge(F, ANT_REST)).pose(0.35, merge(up, {"chest": (0, 0, 6)}, ANT_BACK))
    lw.pose(1.05, merge(up, {"chest": (0, 0, 6)}, ANT_BACK)).pose(1.4, merge(F, ANT_REST)).event(0.35, "flap")
    out.append(lw)
    # Shaking off water: a shiver through the body and a quick buzz of the folded wings.
    shake = Clip("shake", 1.0).pose(0.0, merge(F, ANT_REST)).pose(1.0, merge(F, ANT_REST))
    shake.wave(lambda t: {k: v for k, v in cl.shake_wave(t).items() if not k.startswith("wing")})
    shake.wave(flutter(0.15, 0.85, 9.0, 0.09)).event(0.4, "shake")
    out.append(shake)
    # Flight: both pairs beating, the hindwings a beat behind; gliding on flat wings; diving
    # with the wings swept back.
    fly = _body(cl.FLY_BODY)
    ff = Clip("fly_flap", 0.5, loop=True).pose(0.0, merge(fly, ANT_BACK))
    ff.wave(wingbeat(0.5, 40.0)).wave(tail_sway(0.6, 1.0)).wave(antenna_sway(4.0, 0.5)).event(0.1, "flap")
    out.append(ff)
    fg = Clip("fly_glide", 2.4, loop=True).pose(0.0, merge(fly, raised(-8, -12), ANT_BACK))
    fg.wave(wingbeat(2.4, 5.0, lag=0.2, sweep=2.0, flex=4.0, tail=10.0, hind_low=0.0, bob=0.6))
    fg.wave(tail_sway(0.8, 2.4)).wave(antenna_sway(3.0, 1.2))
    out.append(fg)
    dive = merge(_body(cl.FLY_DIVE), raised(-6, -8, sweep=46, hind_sweep=58), ANT_SLEEP)
    fd = Clip("fly_dive", 1.0, loop=True).pose(0.0, dive)
    fd.wave(wingbeat(0.25, 3.0, sweep=0.0, flex=3.0, tail=12.0, hind_low=0.0, bob=0.0)).wave(tail_sway(0.4, 0.5))
    out.append(fd)
    return out


def clips():
    own = _own_clips()
    names = {c.name for c in own}
    out = [_reshaped(c) for c in clipkit.classic() if c.name not in names]
    out = clipkit.replace(out, own)
    clipkit.check(out, BONE_ORDER, NAME)
    return out


