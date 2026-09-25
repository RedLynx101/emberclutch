"""The Ribbontail's body plan: a long, low serpent on four short legs, with a pair of fan-like
fin-wings at the shoulders. 26 body bones (25 in the body draw, its whole budget: eyes is
never skinned) and 10 wing bones, 36 in all.

  the spine    hips (the root) -> belly -> chest -> neck1..3 -> head -> snout (+ jaw), and
               tail1..tail8 behind the hips: the long ribbon of a body that waves
  the legs     two bones each: arm_up / leg_up (shoulder or hip down to the wrist or ankle,
               a short straight leg) and arm_lo / leg_lo (the paw, wrist to toe). The paws
               are the CONTACTS, so the contact joints sit at the wrists and ankles, just
               above the floor, as the classic hands and feet do
  the fins     wing_arm (the fin's short fleshy base, root to knuckle: the layout calls the
               knuckle "wrist", the point the kit's wing_arm branches rays from) and four fin
               rays wing_r1..r4 fanning from it (r1 the long leading ray). At rest the fan
               half closes and stands swept back at the shoulders like a pair of sails; lying
               down it tucks tighter (the baby drapes its little fins over its flanks); in
               flight it spreads and ripples ray after ray (fin_pose() solves these)

Its clips are its own. The body moves as a serpent's does, by bending along its length:
bend() curls it sideways or arches it, body_wave() runs a wave down it from head to tail (the
walk, the run, swimming through the air, the happy wiggle), and legs step in diagonal pairs
with the wave as a salamander's do. It sits up on its haunches like an otter with its tail
curled round in front, sleeps coiled in a ring, lopes like an otter when it runs, scratches an
itch with the tip of its tail and shimmies the water off from head to tail. The baby has its
own curl, sulk and scratch (its head is too big to tuck into a ring). Nothing is the classic
dragon's (tools/anim/clips.py) except the timings and events the game's behaviour expects.
"""
import math

from dragons import clipkit
from eca import Clip, q_from_pyr

NAME = "ribbontail"

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
]
for _k in range(2, 9):
    BONES.append((f"tail{_k}", f"tail{_k}", f"tail{_k + 1}" if _k < 8 else "tail_tip", f"tail{_k - 1}"))
for _side in ("L", "R"):
    BONES += [
        (f"arm_up_{_side}", f"shoulder_{_side}", f"wrist_{_side}", "chest"),
        (f"arm_lo_{_side}", f"wrist_{_side}", f"toe_f_{_side}", f"arm_up_{_side}"),
        (f"leg_up_{_side}", f"hipj_{_side}", f"ankle_{_side}", "hips"),
        (f"leg_lo_{_side}", f"ankle_{_side}", f"toe_b_{_side}", f"leg_up_{_side}"),
    ]
BONES.append(("jaw", "jaw_hinge", "jaw_tip", "snout"))
BONES.append(("eyes", "eyes_c", "eyes_tip", "head"))

# ------------------------------------------------------------------------------ the fin-wing
# The fin's layout in its own plane (u out along the span, v back along the chord), as the kit's
# wing layouts are: the root on the shoulder, a short fleshy base out to the knuckle, then four
# rays fanning back from it; "body" is where the fin's trailing edge meets the back.
FIN_LAYOUT = {"root": (0.0, 0.0), "wrist": (0.30, 0.10),
              "t1": (1.78, 0.30), "t2": (1.72, 0.98), "t3": (1.34, 1.52), "t4": (0.74, 1.78),
              "body": (0.0, 0.55)}
FIN_DIHEDRAL, FIN_DROOP = 24.0, 12.0   # the rest plane: raised a little, chord drooping back
WING_CHAIN = [("wing_arm", "root", "wrist", "chest")] + \
    [(f"wing_r{i}", "wrist", f"t{i}", "wing_arm") for i in range(1, 5)]
WING_BONES = [f"{n}_{side}" for side in ("L", "R") for n, *_ in WING_CHAIN]
BONE_ORDER = [b[0] for b in BONES] + WING_BONES
WING_BODY = ("chest",)
CONTACTS = ("arm_lo_L", "arm_lo_R", "leg_lo_L", "leg_lo_R")
SEAT = ("belly", (0.0, 0.0, 0.43))
REGION = {}


# ------------------------------------------------------------------------------ rotation maths
def _norm(v):
    n = math.sqrt(sum(c * c for c in v)) or 1.0
    return tuple(c / n for c in v)


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def _sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def _mat_from_q(q):
    w, x, y, z = q
    return ((1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y)),
            (2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x)),
            (2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)))


def pyr_from_matrix(m):
    """(pitch, yaw, roll) degrees for an armature-space rotation matrix: the inverse of
    eca.q_from_pyr (R = Rz(yaw) Rx(-pitch) Ry(-roll))."""
    b = math.asin(max(-1.0, min(1.0, m[2][1])))
    a = math.atan2(-m[0][1], m[1][1])
    c = math.atan2(-m[2][0], m[2][2])
    return (-math.degrees(b), math.degrees(a), -math.degrees(c))


def _frame_matrix(frm, to):
    """The rotation taking the orthonormal frame frm (three axes) onto to: sum to_i frm_i^T."""
    return tuple(tuple(sum(to[k][r] * frm[k][c] for k in range(3)) for c in range(3)) for r in range(3))


def _axis_angle_matrix(axis, deg):
    h = math.radians(deg) / 2
    s = math.sin(h)
    return _mat_from_q((math.cos(h), axis[0] * s, axis[1] * s, axis[2] * s))


def fin_frame():
    """The right fin's rest axes (span, chord, normal) in armature space: span out and up by
    the dihedral, chord back and down by the droop (made square to the span)."""
    th, ph = math.radians(FIN_DIHEDRAL), math.radians(FIN_DROOP)
    span = (math.cos(th), 0.0, math.sin(th))
    chord = (0.0, math.cos(ph), -math.sin(ph))
    chord = _norm(_sub(chord, tuple(c * _dot(chord, span) for c in span)))
    return span, chord, _cross(span, chord)


def fin_point(name):
    """A layout point of the right fin (relative to its root, unscaled) in armature axes."""
    span, chord, _ = fin_frame()
    u, v = FIN_LAYOUT[name]
    return tuple(span[i] * u + chord[i] * v for i in range(3))


def fin_pose(lead, facing, close, spread=None):
    """A whole fin's pose as clip keys for both sides: the fin turned so its leading ray points
    along `lead` and its face (the upper side at rest) along `facing` (the right fin's; the left
    mirrors), its fan closed by `close` (0 open as at rest .. 1 every ray on the leading one).
    spread: extra per-ray fan angles (degrees, + opens) for ripples. Returns {bone*: (p, y, r)}."""
    normal = fin_frame()[2]
    k = fin_point("wrist")
    d1 = _norm(_sub(fin_point("t1"), k))
    # the fan's plane at rest: d1 and the face normal
    rest = (d1, _norm(_cross(normal, d1)), normal)
    lead = _norm(lead)
    facing = _norm(_sub(facing, tuple(c * _dot(facing, lead) for c in lead)))
    to = (lead, _cross(facing, lead), facing)
    qa = pyr_from_matrix(_frame_matrix(rest, to))
    out = {"wing_arm*": qa}
    for i in range(2, 5):
        di = _norm(_sub(fin_point(f"t{i}"), k))
        ang = math.degrees(math.atan2(_dot(_cross(d1, di), normal), _dot(d1, di)))
        turn = -ang * close + (spread[i - 2] if spread else 0.0) * (1 if ang > 0 else -1)
        out[f"wing_r{i}*"] = pyr_from_matrix(_axis_angle_matrix(normal, turn))
    if spread:
        out["wing_r1*"] = pyr_from_matrix(_axis_angle_matrix(normal, -spread[0] * 0.3))
    return out


def _check_pyr():
    for p, y, r in ((10, 20, 30), (-80, 130, 6), (45, -60, -20)):
        q = q_from_pyr(p, y, r)
        back = pyr_from_matrix(_mat_from_q(q))
        assert all(abs(a - b) < 1e-6 for a, b in zip((p, y, r), back)), (p, y, r, back)


_check_pyr()


# ------------------------------------------------------------------------------ pose helpers
def merge(*poses):
    out = {}
    for p in poses:
        for bone, v in p.items():
            a = out.get(bone, (0.0, 0.0, 0.0))
            out[bone] = tuple(x + y for x, y in zip(a, v))
    return out


def sin01(t, period, phase=0.0):
    return math.sin(2 * math.pi * (t / period + phase))


def smooth01(x):
    x = max(0.0, min(1.0, x))
    return x * x * (3 - 2 * x)


# Where each spine joint sits along the body, from the head joint back (adult units): the
# waves run along this. Front bones bend the body by +yaw / +pitch, tail bones by -yaw / -pitch
# (a tail bone points backward, so the same bend is the opposite rotation).
FRONT_S = [("head", 0.0), ("neck3", 0.3), ("neck2", 0.64), ("neck1", 1.02), ("chest", 1.5), ("belly", 2.16)]
TAIL_S = [("tail1", 2.86), ("tail2", 3.42), ("tail3", 3.92), ("tail4", 4.38), ("tail5", 4.8), ("tail6", 5.2),
          ("tail7", 5.58), ("tail8", 5.92)]
LENGTH = 6.2
# How steeply each front bone rises at rest (degrees up from pointing forward; the grown
# form's, and near enough the baby's). A clip's delta turns a bone in its rest frame, so a
# sideways bend of the upright neck is a tilt about the bone's own forward-up axis (a yaw
# there would only twist it about itself).
ELEV = {"head": -11, "neck3": 70, "neck2": 82, "neck1": 63, "chest": 41, "belly": 7}


def lateral(name, k):
    """A sideways bend of k degrees at a spine joint, toward +X, however the bone points."""
    if name.startswith("tail"):
        return (0.0, -k, 0.0)
    e = math.radians(ELEV[name])
    return pyr_from_matrix(_axis_angle_matrix((0.0, math.sin(e), math.cos(e)), k))


def bend(kappa, axis="yaw", only=None):
    """The body bent with curvature kappa(s) degrees at every spine joint: axis "yaw" curls it
    sideways toward +X (the right bones' side) for kappa > 0, "pitch" arches it concave-up."""
    out = {}
    for sign, joints in ((1, FRONT_S), (-1, TAIL_S)):
        for name, s in joints:
            if only and name not in only:
                continue
            k = kappa(s)
            out[name] = lateral(name, k) if axis == "yaw" else (sign * k, 0.0, 0.0)
    return out


def body_wave(period, amp, wavelength=LENGTH * 0.95, axis="yaw", steady=0.75, phase=0.0, only=None):
    """A wave running down the body from head to tail (the swimming motion): amp(s) degrees
    of bend at each joint. steady: how much the head holds its heading against the wave."""
    def fn(t):
        def kappa(s):
            return amp(s) * math.sin(2 * math.pi * (t / period - s / wavelength + phase))

        pose = bend(kappa, axis, only)
        if steady and "head" in pose:
            # the head holds its heading: it turns back by what the body's bends turned it
            if axis == "yaw":
                total = sum(kappa(s) * math.cos(math.radians(ELEV[n])) for n, s in FRONT_S[1:] if n in pose)
                pose["head"] = (0.0, -steady * total, 0.0)
            else:
                total = sum(pose[n][0] for n, _ in FRONT_S[1:] if n in pose)
                pose["head"] = (-steady * total, 0.0, 0.0)
        return pose
    return fn


def ramp(a, b, lo, hi):
    """amp(s): a at the head .. b at the tail tip, over s in [lo, hi] (flat outside)."""
    return lambda s: a + (b - a) * smooth01((s - lo) / (hi - lo))


def breathe(amount=1.0, period=3.2):
    def fn(t):
        s = sin01(t, period) * amount
        return {"chest": (1.0 * s, 0, 0), "belly": (-0.6 * s, 0, 0), "neck1": (0.6 * s, 0, 0),
                "head": (-0.6 * s, 0, 0), "wing_arm_R": (0, 0, 1.2 * s), "wing_arm_L": (0, 0, -1.2 * s)}
    return fn


def fin_ripple(period, amount, phase=0.0, arm=0.0):
    """The fins' ripple: each ray lifts and falls a little after the one before it (a wave
    across the fan), the base swaying with them."""
    def fn(t):
        out = {}
        for i in range(1, 5):
            s = sin01(t, period, phase - 0.09 * i) * amount * (0.6 + 0.2 * i)
            out[f"wing_r{i}_R"] = (0, 0, s)
            out[f"wing_r{i}_L"] = (0, 0, -s)
        if arm:
            s = sin01(t, period, phase) * arm
            out["wing_arm_R"] = (0, 0, s)
            out["wing_arm_L"] = (0, 0, -s)
        return out
    return fn


def tail_sway(amount=1.0, period=3.2):
    """The idle's slow wave down the tail (and a breath of it through the body)."""
    return body_wave(period, ramp(0.0, 7.0 * amount, 2.0, LENGTH), steady=1.0,
                     only=[n for n, _ in TAIL_S] + ["belly", "head"])


def tail_wave(period, amp_base, amp_tip, phase=0.0, axis="yaw", wavelength=3.2):
    """A wave down the tail alone, from its root (amp_base) to its tip (amp_tip)."""
    return body_wave(period, ramp(amp_base, amp_tip, 2.8, LENGTH), wavelength=wavelength, axis=axis, steady=0,
                     phase=phase, only=[n for n, _ in TAIL_S])


def steps(period, swing, lift_roll, paw, phase, lag=0.06, bob=2.0):
    """Legs stepping in diagonal pairs with the body's wave (as a salamander's do): the front
    leg on the side the body curls toward is back while the hind leg on that side is forward.
    phase: when the right front leg is at its forward-most point (fraction of the period)."""
    def fn(t):
        out = {}
        for limb, lo, ph, sgn in (("arm_up_R", "arm_lo_R", phase, 1), ("leg_up_L", "leg_lo_L", phase + lag, -1),
                                  ("arm_up_L", "arm_lo_L", phase + 0.5, -1), ("leg_up_R", "leg_lo_R", phase + 0.5 + lag, 1)):
            th = 2 * math.pi * (t / period - ph) + math.pi / 2
            lift = max(0.0, math.cos(th)) ** 1.3  # the forward swing half
            out[limb] = (swing * math.sin(th), 0, sgn * lift_roll * lift)
            out[lo] = (paw * lift, 0, 0)
        b = sin01(t, period / 2, phase)
        out["chest"] = (bob * 0.4 * b, 0, 2.0 * sin01(t, period, phase + 0.25))
        out["hips"] = (0, 0, -1.5 * sin01(t, period, phase + 0.25 + lag))
        out["neck1"] = (-bob * 0.6 * b, 0, 0)
        return out
    return fn


def footsteps(c, period, phases, cycles=1):
    for k in range(cycles):
        for ph in phases:
            c.event(((ph + k) * period) % (period * cycles), "footstep")


def chomp(amount=20.0, period=0.6, at=0.15):
    def fn(t):
        u = ((t - at) / period) % 1.0
        return {"jaw": (-amount * math.sin(math.pi * (u - 0.5) / 0.5) ** 0.7 if u > 0.5 else 0.0, 0, 0)}
    return fn


def pant(amount=7.0, period=0.28):
    return lambda t: {"jaw": (-amount - 3 * sin01(t, period), 0, 0)}


def window(t, a, b, fade=0.12):
    """1 inside [a, b], easing in and out over `fade` seconds."""
    return smooth01((t - a) / fade) * smooth01((b - t) / fade)


# ------------------------------------------------------------------------------ poses
# The fin-wings: folded, the fan half closes and swings back and up, so the pair stands at the
# shoulders like swept sails (the leading ray low along the back, the fan rising behind it);
# tucked flat along the back to lie down and sleep; half-raised for a flutter.
FOLD = fin_pose(lead=(0.22, 0.92, 0.3), facing=(0.95, 0.0, 0.3), close=0.22)
FOLD_TIGHT = fin_pose(lead=(0.12, 0.98, 0.14), facing=(0.98, 0.0, 0.15), close=0.65)
FOLD_DRAPED = fin_pose(lead=(0.1, 0.99, 0.04), facing=(-0.9, 0.1, -0.42), close=0.6)  # the baby asleep: over its flanks
FIN_HALF = fin_pose(lead=(0.55, 0.7, 0.45), facing=(0.45, 0.05, 0.89), close=0.15)
FIN_OPEN = fin_pose(lead=(0.9, 0.3, 0.3), facing=(-0.35, 0.2, 0.92), close=-0.08)
FIN_BACK = fin_pose(lead=(0.45, 0.88, 0.1), facing=(-0.2, 0.1, 0.97), close=0.2)   # swept back, diving

# The rest pose's tail sweeps in an S and curls up at its end (the grown form's base_pose);
# lying, coiling and flying lower the curl by some 20 degrees (the fan fin still clears the
# floor) and take the S out (the coil and the swim put their own bends in).
TAIL_LOW = {"tail5": (2, 0, 0), "tail6": (4, 0, 0), "tail7": (6, 0, 0), "tail8": (8, 0, 0)}
TAIL_UNSWAY = {"tail1": (0, -10, 0), "tail2": (0, -16, 0), "tail3": (0, 12, 0), "tail4": (0, 24, 0),
               "tail5": (0, 22, 0), "tail6": (0, 6, 0), "tail7": (0, -16, 0), "tail8": (0, -22, 0)}

STAND = FOLD
# Sitting up like an otter: the front raised off the ground on the haunches, the little
# front paws held to the chest, the hind legs folded forward, the tail curled round in front.
SIT = merge(FOLD, {
    "hips": (12, 0, 0), "belly": (16, 0, 0), "chest": (12, 0, 0),
    "neck1": (-24, 0, 0), "neck2": (-8, 0, 0), "neck3": (4, 0, 0), "head": (-10, 0, 0),
    "arm_up*": (-14, 0, 10), "arm_lo*": (30, 0, 0),
    "leg_up*": (60, 0, 6), "leg_lo*": (-20, 0, 0),
    "tail1": (-12, -22, 0), "tail2": (0, -34, 0), "tail3": (0, -32, 0), "tail4": (0, -28, 0),
    "tail5": (6, -26, 0), "tail6": (12, -28, 0), "tail7": (14, -32, 0), "tail8": (16, -34, 0),
})
# The baby's tail already curls round to its other side: its sit wraps the tail that way.
SIT_BABY = merge({k: v for k, v in SIT.items() if not k.startswith("tail")}, {
    "tail1": (-12, 14, 0), "tail2": (0, 18, 0), "tail3": (0, 16, 0), "tail4": (0, 14, 0),
    "tail5": (4, 12, 0), "tail6": (6, 12, 0), "tail7": (6, 10, 0), "tail8": (6, 10, 0),
})
# Lying: legs folded back along the belly, the neck lowered, the head up and level.
LIE = merge(FOLD, TAIL_LOW, {
    "arm_up*": (-76, 0, 6), "arm_lo*": (60, 0, 0), "leg_up*": (-78, 0, 6), "leg_lo*": (62, 0, 0),
    "neck1": (-34, 0, 0), "neck2": (-14, 0, 0), "neck3": (6, 0, 0), "head": (30, 0, 0),
})
CROUCH_BODY = {"hips": (-4, 0, 0), "arm_up*": (-26, 0, 8), "arm_lo*": (20, 0, 0), "leg_up*": (-24, 0, 8),
               "leg_lo*": (20, 0, 0), "neck1": (-16, 0, 0), "head": (10, 0, 0)}
CROUCH = merge(FIN_HALF, CROUCH_BODY)
CROUCH_FOLDED = merge(FOLD, CROUCH_BODY)
# Asleep: coiled into a ring on the floor, the head resting on the tail, fins tucked.
CURL = merge(FOLD_TIGHT, TAIL_UNSWAY, TAIL_LOW, {
    "arm_up*": (-76, 0, 6), "arm_lo*": (60, 0, 0), "leg_up*": (-78, 0, 6), "leg_lo*": (62, 0, 0),
    "neck1": (-40, 0, 0), "neck2": (-26, 0, 0), "neck3": (-6, 0, 0), "head": (56, 0, 0),
}, bend(lambda s: 28.0 if s < 2.5 else 25.0))
# The baby's head is too big to tuck into a ring: it curls up with its chin on the floor, head
# turned toward its tail, and the little tail wrapped round it (the way its tail already curls).
LEGS_TUCKED = {"arm_up*": (-76, 0, 6), "arm_lo*": (60, 0, 0), "leg_up*": (-78, 0, 6), "leg_lo*": (62, 0, 0)}
CURL_BABY = merge(FOLD_DRAPED, TAIL_LOW, LEGS_TUCKED, {
    "neck1": (-30, 0, 0), "neck2": (-14, 0, 0), "neck3": (4, 0, 0), "head": (26, 0, 0),
}, bend(lambda s: -6.0 if s < 1.2 else (-12.0 if s < 2.5 else -24.0)))
SULK_BABY = merge(FOLD_DRAPED, TAIL_LOW, LEGS_TUCKED, {
    "neck1": (-34, 0, 0), "neck2": (-14, 0, 0), "head": (16, 0, -12),
}, bend(lambda s: 10.0 if s < 2.5 else 16.0))
SCRATCH_BABY = merge(FOLD, {  # a hind foot up to scratch behind a gill, head tilted into it
    "hips": (4, 0, -10), "leg_up_R": (74, -10, 34), "leg_lo_R": (30, 0, 0),
    "neck1": (0, 0, 0), "neck3": (0, -16, 0), "head": (4, -8, -22), "tail1": (0, 10, 0)})
SULK = merge(FOLD_TIGHT, TAIL_UNSWAY, TAIL_LOW, {
    "arm_up*": (-76, 0, 6), "arm_lo*": (60, 0, 0), "leg_up*": (-78, 0, 6), "leg_lo*": (62, 0, 0),
    "neck1": (-44, 0, 0), "neck2": (-16, 0, 0), "neck3": (6, 0, 0), "head": (14, 0, -10),
}, bend(lambda s: -22.0 if s > 2.5 else -18.0))
EAT = merge(FOLD, {
    "hips": (-4, 0, 0), "chest": (-6, 0, 0), "arm_up*": (10, 0, 6),
    "neck1": (-48, 0, 0), "neck2": (-32, 0, 0), "neck3": (-10, 0, 0), "head": (-6, 0, 0),
})
EAT_BABY = merge(FOLD, {"neck1": (-18, 0, 0), "neck2": (-10, 0, 0), "head": (-24, 0, 0), "chest": (-6, 0, 0),
                        "arm_up*": (12, 0, 6)})
# On its back: the whole body rolls over, the neck and the tail twisting back so the head
# looks at you and the tail's fin stays up.
FIN_SPLAY = fin_pose(lead=(0.9, 0.35, -0.25), facing=(-0.25, 0.1, -0.96), close=0.3)  # out flat to the sides
BELLY_UP = merge(FIN_SPLAY, {
    "hips": (0, 0, 168), "chest": (0, 0, 10),
    "arm_up*": (40, 0, -10), "arm_lo*": (-20, 0, 0), "leg_up*": (36, 0, -10), "leg_lo*": (-20, 0, 0),
    "neck1": (12, 0, -60), "neck2": (0, 0, -50), "neck3": (0, 0, -32), "head": (16, 0, -20),
    "tail1": (0, 16, 0), "tail2": (0, 18, 30), "tail3": (0, 16, 30), "tail4": (0, 10, 30), "tail5": (0, 0, 30),
})
# Flying: legs tucked back, the neck stretched forward, the tail streaming out behind.
FLY = merge(FIN_OPEN, TAIL_UNSWAY, TAIL_LOW, {
    "arm_up*": (-70, 0, 10), "arm_lo*": (50, 0, 0), "leg_up*": (-80, 0, 8), "leg_lo*": (60, 0, 0),
    "neck1": (-44, 0, 0), "neck2": (-10, 0, 0), "neck3": (6, 0, 0), "head": (24, 0, 0),
    "hips": (4, 0, 0),
})
CARRY_HEAD = {"neck1": (10, 0, 0), "head": (-8, 0, 0)}
PLAY_BOW = merge(FIN_HALF, {  # the front down on outstretched paws, the rump up, the tail waving
    "hips": (-14, 0, 0), "belly": (-6, 0, 0), "chest": (-4, 0, 0),
    "arm_up*": (54, 0, 12), "arm_lo*": (-30, 0, 0), "leg_up*": (14, 0, 0),
    "neck1": (-10, 0, 0), "neck2": (8, 0, 0), "head": (16, 0, 0), "jaw": (-14, 0, 0),
    "tail1": (10, 0, 0), "tail2": (4, 0, 0), "tail6": (6, 0, 0), "tail7": (8, 0, 0), "tail8": (8, 0, 0),
})
SPAR = merge(FIN_HALF, {  # reared up on the haunches, as sitting, the paws up to bat
    "hips": (14, 0, 0), "belly": (16, 0, 0), "chest": (10, 0, 0),
    "neck1": (-20, 0, 0), "neck2": (-6, 0, 0), "head": (-8, 0, 0),
    "leg_up*": (50, 0, 6), "leg_lo*": (-16, 0, 0), "tail1": (-14, 0, 0),
})
STALK = merge(FOLD_TIGHT, {"hips": (-2, 0, 0), "arm_up*": (-30, 0, 16), "arm_lo*": (24, 0, 0),
                           "leg_up*": (-28, 0, 16), "leg_lo*": (24, 0, 0),
                           "neck1": (-36, 0, 0), "neck2": (-8, 0, 0), "neck3": (6, 0, 0), "head": (26, 0, 0)})
TAIL_CHASE = merge(FIN_HALF, bend(lambda s: 20.0 if s > 1.0 else 12.0), {"jaw": (-16, 0, 0)})
SCRATCH = merge(FOLD, bend(lambda s: 24.0 if s > 2.5 else (22.0 if s > 0.2 else 10.0)),
                {"head": (-6, 0, 16), "tail7": (-10, 0, 0), "tail8": (-14, 0, 0)})


def sided(pose, side):
    """Only one side's keys of a pose ("_R" or "_L")."""
    return {k: v for k, v in pose.items() if k.endswith(side)}


def expand(pose):
    """A pose's "name*" keys written out for both sides (so one side can be picked)."""
    out = {}
    for k, v in pose.items():
        if k.endswith("*"):
            out[k[:-1] + "_R"] = v
            out[k[:-1] + "_L"] = (v[0], -v[1], -v[2])
        else:
            out[k] = v
    return out


# ------------------------------------------------------------------------------ the clips
def clips():
    out = []

    def clip(*args, **kwargs):
        c = Clip(*args, **kwargs)
        out.append(c)
        return c

    # ---- idle & locomotion
    clip("idle", 3.2, loop=True).pose(0.0, STAND).wave(breathe()).wave(tail_sway()).wave(fin_ripple(3.2, 3.0))
    (clip("look_around", 3.0).pose(0.0, STAND)
     .key(0.0, neck2=(0, 0, 0), neck3=(0, 0, 0), head=(0, 0, 0))
     .key(0.8, neck2=(4, 16, 0), neck3=(2, 14, 0), head=(6, 12, 8))
     .key(1.5, neck2=(4, 16, 0), neck3=(2, 14, 0), head=(6, 12, 8))
     .key(2.2, neck2=(2, -18, 0), neck3=(0, -16, 0), head=(4, -14, -8))
     .key(3.0, neck2=(0, 0, 0), neck3=(0, 0, 0), head=(0, 0, 0))
     .wave(breathe()).wave(tail_sway()).event(0.8, "sniff"))
    # An itch behind the frill: it curls round and rubs it with the tip of its tail.
    (clip("scratch", 2.1).pose(0.0, STAND).pose(0.55, SCRATCH).pose(1.6, SCRATCH).pose(2.1, STAND)
     .wave(lambda t: {"tail8": (0, 16 * sin01(t, 0.2) * window(t, 0.6, 1.55), 0),
                      "tail7": (0, 7 * sin01(t, 0.2, -0.1) * window(t, 0.6, 1.55), 0),
                      "head": (0, 0, 5 * sin01(t, 0.4) * window(t, 0.6, 1.55))}))
    # the baby's short noodle can't reach round: a hind foot does it, as a kitten's would
    (clip("scratch_h", 2.1).pose(0.0, STAND).pose(0.4, SCRATCH_BABY).pose(1.7, SCRATCH_BABY).pose(2.1, STAND)
     .wave(lambda t: {"leg_up_R": (12 * sin01(t, 0.18) * window(t, 0.5, 1.6), 0, 0),
                      "leg_lo_R": (14 * sin01(t, 0.18, -0.1) * window(t, 0.5, 1.6), 0, 0)}))

    # The walk: a slow wave runs down the body, the legs stepping in diagonal pairs with it.
    walk_amp = ramp(3.0, 13.0, 1.0, LENGTH)
    walk = clip("walk", 1.2, loop=True, speed=0.5).pose(0.0, STAND)
    walk.wave(body_wave(1.2, walk_amp)).wave(steps(1.2, 22, 18, 34, 0.0)).wave(fin_ripple(1.2, 4.0))
    footsteps(walk, 1.2, (0.25, 0.31, 0.75, 0.81))
    trot = clip("trot", 0.72, loop=True, speed=1.3).pose(0.0, STAND)
    trot.wave(body_wave(0.72, ramp(4.0, 16.0, 1.0, LENGTH))).wave(steps(0.72, 26, 22, 40, 0.0, bob=3.0))
    trot.wave(pant(5.0, 0.36)).wave(fin_ripple(0.72, 5.0))
    footsteps(trot, 0.72, (0.25, 0.75))
    shuffle = clip("shuffle", 0.9, loop=True).pose(0.0, merge(STAND, bend(lambda s: 5.0)))
    shuffle.wave(steps(0.9, 10, 14, 24, 0.0, bob=1.0)).wave(tail_wave(0.9, 3.0, 10.0))
    footsteps(shuffle, 0.9, (0.25, 0.75))
    carry = clip("carry", 1.2, loop=True, speed=0.5).pose(0.0, merge(STAND, CARRY_HEAD))
    carry.wave(body_wave(1.2, walk_amp, steady=1.0)).wave(steps(1.2, 22, 18, 34, 0.0))
    footsteps(carry, 1.2, (0.25, 0.75))

    # Running: the grown one lopes like an otter, bounding, the back arching and stretching,
    # the tail streaming in a wave; the baby scampers the same way, quicker and bouncier.
    def lope(period, arch, reach, swing, tail):
        def fn(t):
            s = sin01(t, period)
            c = sin01(t, period, 0.25)
            out = {"hips": (-arch * 0.5 * s, 0, 0), "belly": (arch * s, 0, 0), "chest": (arch * 0.6 * s, 0, 0),
                   "neck1": (-arch * 0.8 * s, 0, 0), "head": (arch * 0.4 * s, 0, 0)}
            for side, off in (("R", 0.0), ("L", 0.06)):
                f = sin01(t, period, 0.1 + off)
                h = sin01(t, period, 0.6 + off)
                out[f"arm_up_{side}"] = (swing * f, 0, 0)
                out[f"arm_lo_{side}"] = (reach * max(0.0, sin01(t, period, 0.35 + off)), 0, 0)
                out[f"leg_up_{side}"] = (swing * h, 0, 0)
                out[f"leg_lo_{side}"] = (reach * max(0.0, sin01(t, period, 0.85 + off)), 0, 0)
            for k, (name, _) in enumerate(TAIL_S):
                out[name] = (tail * 0.4 * sin01(t, period, -0.1 * k + 0.2) - 2, tail * sin01(t, period * 2, -0.08 * k), 0)
            out["jaw"] = (-6 - 2 * c, 0, 0)
            return out
        return fn

    # (two strides a loop: the tail swings side to side once over both)
    gallop = clip("gallop", 1.12, loop=True, speed=3.0).pose(0.0, merge(FOLD_TIGHT, {"neck1": (-14, 0, 0),
                                                                                   "head": (12, 0, 0)}))
    gallop.wave(lope(0.56, 9.0, 34, 34, 10.0)).wave(fin_ripple(0.56, 6.0))
    for k in (0.0, 0.56):
        gallop.root(k, up=0.02).root(k + 0.14, up=0.12).root(k + 0.28, up=0.03).root(k + 0.42, up=0.08)
    footsteps(gallop, 0.56, (0.2, 0.26, 0.7, 0.76), cycles=2)
    scamper = clip("scamper", 0.8, loop=True, speed=2.2).pose(0.0, merge(FOLD, {"neck1": (-6, 0, 0),
                                                                            "head": (8, 0, 0)}))
    scamper.wave(lope(0.4, 11.0, 34, 36, 14.0)).wave(fin_ripple(0.4, 8.0))
    for k in (0.0, 0.4):
        scamper.root(k, up=0.0).root(k + 0.12, up=0.18).root(k + 0.26, up=0.03)
    footsteps(scamper, 0.4, (0.2, 0.7), cycles=2)

    # Flying: it swims through the air: a wave runs down the body (up and down, with a
    # slower one side to side) while the fin-wings stroke and ripple.
    def swim(period, vert, side, stroke, ripple):
        v = body_wave(period, ramp(2.0, vert, 1.0, LENGTH), axis="pitch", steady=0.6, wavelength=LENGTH * 1.1)
        s = body_wave(period * 2, ramp(1.0, side, 1.0, LENGTH), steady=0.8, wavelength=LENGTH * 1.2)
        f = fin_ripple(period, ripple, 0.0, stroke)
        return lambda t: merge(v(t), s(t), f(t), {"chest": (1.5 * sin01(t, period), 0, 0)})

    # (each loop two strokes long, so the slower side-to-side wave comes round too)
    fly_flap = clip("fly_flap", 2.0, loop=True).pose(0.0, FLY)
    fly_flap.wave(swim(1.0, 12.0, 7.0, 26.0, 18.0)).event(0.1, "flap").event(1.1, "flap")
    clip("fly_glide", 4.8, loop=True).pose(0.0, FLY).wave(swim(2.4, 6.0, 9.0, 5.0, 7.0))
    dive = merge({b: v for b, v in FLY.items() if not b.startswith("wing")}, FIN_BACK,
                 {"neck1": (-10, 0, 0), "head": (6, 0, 0), "hips": (-4, 0, 0)})
    clip("fly_dive", 1.0, loop=True).pose(0.0, dive).wave(swim(0.5, 5.0, 3.0, 2.0, 3.0))

    # ---- rest
    for suffix, sit in (("", SIT), ("_h", SIT_BABY)):
        clip("sit" + suffix, 1.0).pose(0.0, STAND).pose(1.0, sit)
        clip("sit_loop" + suffix, 3.2, loop=True).pose(0.0, sit).wave(breathe()).wave(
            tail_wave(3.2, 0.0, 6.0)).wave(fin_ripple(3.2, 2.0))
    clip("lie_down", 1.1).pose(0.0, STAND).pose(0.55, CROUCH_FOLDED).pose(1.1, LIE).event(0.9, "thump")
    clip("lie_loop", 4.0, loop=True).pose(0.0, LIE).wave(breathe(1.2, 4.0)).wave(tail_sway(0.6, 4.0))
    for suffix, curl in (("", CURL), ("_h", CURL_BABY)):
        clip("curl_up" + suffix, 1.6).pose(0.0, LIE).pose(1.6, curl)
        clip("sleep" + suffix, 4.8, loop=True).pose(0.0, curl).wave(breathe(1.8, 4.8)).wave(
            lambda t: {"tail8": (0, 3 * sin01(t, 4.8, 0.3), 0)})
        (clip("wake" + suffix, 2.6).pose(0.0, curl).pose(0.7, LIE)
         .pose(1.3, merge(LIE, {"arm_up*": (50, 0, 0), "arm_lo*": (-50, 0, 0), "neck1": (44, 0, 0),
                                "neck2": (10, 0, 0), "head": (-10, 0, 0), "snout": (6, 0, 0), "jaw": (-32, 0, 0),
                                "hips": (-6, 0, 0)}))
         .pose(1.9, merge(FIN_HALF, {"neck1": (10, 0, 0), "head": (6, 0, 0)}))
         .pose(2.6, STAND).event(1.3, "yawn"))
    YAWN = merge(STAND, {"neck1": (10, 0, 0), "neck2": (8, 0, 0), "head": (28, 0, 0), "snout": (6, 0, 0),
                         "jaw": (-32, 0, 0), "chest": (4, 0, 0)})
    (clip("yawn", 1.6).pose(0.0, STAND).pose(0.6, YAWN).pose(1.1, merge(YAWN, {"jaw": (-3, 0, 0)}))
     .pose(1.6, STAND).event(0.6, "yawn"))
    clip("nap_flop", 0.7).pose(0.0, STAND).pose(0.7, LIE).event(0.55, "thump")

    # ---- care reactions
    eat = clip("eat", 1.2, loop=True).pose(0.0, EAT)
    eat.wave(lambda t: {"head": (8 * max(0.0, sin01(t, 0.6)), 0, 0), "snout": (6 * max(0.0, sin01(t, 0.6)), 0, 0),
                        "neck2": (3 * sin01(t, 1.2), 0, 0)}).wave(chomp()).wave(tail_wave(1.2, 1.0, 6.0))
    eat.event(0.15, "chomp").event(0.75, "chomp")
    eat_h = clip("eat_h", 1.2, loop=True).pose(0.0, EAT_BABY)
    eat_h.wave(lambda t: {"head": (8 * max(0.0, sin01(t, 0.6)), 0, 0), "neck1": (3 * sin01(t, 1.2), 0, 0)})
    eat_h.wave(chomp(24)).wave(tail_wave(1.2, 2.0, 10.0)).event(0.15, "chomp").event(0.75, "chomp")
    # The baby toddles: quicker little steps, a wiggle down its whole noodle of a body.
    walk_h = clip("walk_h", 0.56, loop=True, speed=0.45).pose(0.0, STAND)
    walk_h.wave(body_wave(0.56, ramp(5.0, 16.0, 0.8, LENGTH), steady=0.85)).wave(steps(0.56, 30, 22, 40, 0.0, bob=4.0))
    footsteps(walk_h, 0.56, (0.25, 0.75))
    carry_h = clip("carry_h", 0.56, loop=True, speed=0.45).pose(0.0, merge(STAND, CARRY_HEAD))
    carry_h.wave(body_wave(0.56, ramp(5.0, 16.0, 0.8, LENGTH), steady=1.0)).wave(steps(0.56, 30, 22, 40, 0.0, bob=3.0))
    HAPPY = merge(FIN_HALF, {"neck1": (10, 0, 0), "head": (12, 0, 0), "jaw": (-16, 0, 0)})
    (clip("fav_wiggle", 1.6).pose(0.0, STAND).pose(0.3, HAPPY).pose(1.3, HAPPY).pose(1.6, STAND)
     .wave(lambda t: merge(body_wave(0.4, ramp(4.0, 20.0, 1.0, LENGTH), steady=0.6)(t),
                           {k: tuple(x * window(t, 0.2, 1.45, 0.2) for x in v) for k, v in
                            fin_ripple(0.3, 14.0)(t).items()}))
     .root(0.0).root(0.55, up=0.0).root(0.75, up=0.25).root(0.95, up=0.0).root(1.6)
     .event(0.95, "land").event(0.3, "call"))
    (clip("pet_head", 1.6, loop=True)
     .pose(0.0, merge(STAND, {"neck1": (4, 0, 0), "neck3": (4, 0, 6), "head": (-10, 6, 16), "jaw": (-7, 0, 0)}))
     .wave(lambda t: {"head": (0, 4 * sin01(t, 1.6), 4 * sin01(t, 1.6)), "neck2": (0, 3 * sin01(t, 1.6, 0.1), 0)})
     .wave(tail_wave(1.6, 2.0, 12.0)).event(0.4, "purr"))
    (clip("pet_chin", 1.6, loop=True)
     .pose(0.0, merge(STAND, {"neck1": (12, 0, 0), "neck2": (8, 0, 0), "head": (26, 0, 0), "jaw": (-9, 0, 0)}))
     .wave(lambda t: {"head": (4 * sin01(t, 1.6), 0, 0), "jaw": (-3 * sin01(t, 1.6, 0.2), 0, 0)})
     .wave(tail_wave(1.6, 2.0, 12.0)).event(0.4, "purr"))
    clip("roll_over", 1.1).pose(0.0, STAND).pose(0.45, LIE).pose(1.1, BELLY_UP).event(0.8, "thump")
    (clip("belly_rub", 1.2, loop=True).pose(0.0, BELLY_UP)
     .wave(lambda t: {"leg_up_L": (12 * sin01(t, 0.6), 0, 0), "leg_up_R": (12 * sin01(t, 0.6, 0.5), 0, 0),
                      "arm_up_L": (10 * sin01(t, 0.6, 0.25), 0, 0), "arm_up_R": (10 * sin01(t, 0.6, 0.75), 0, 0),
                      "hips": (0, 0, 5 * sin01(t, 1.2))})
     .wave(tail_wave(0.6, 3.0, 14.0)).wave(pant(10.0, 0.6)).event(0.3, "purr"))

    # A shake: a shimmy runs from its head to the tip of its tail, throwing off the water.
    def shake_wave(t):
        out = {}
        for name, s in FRONT_S + TAIL_S:
            u = t - 0.25 - s * 0.06
            k = max(0.0, 1.0 - abs(u - 0.2) / 0.28)
            out[name] = (0, 0, (16 if name in ("chest", "belly", "tail1") else 12) * sin01(u, 0.12) * k)
        out.update({k: (0, 0, 14 * sin01(t, 0.12) * max(0.0, 1 - abs(t - 0.5) / 0.35) * (1 if k.endswith("R") else -1))
                    for k in ("wing_arm_R", "wing_arm_L")})
        return out
    clip("shake", 1.0).pose(0.0, STAND).pose(1.0, STAND).wave(shake_wave).event(0.4, "shake")
    HOP_UP = merge(FIN_OPEN, {"arm_up*": (30, 0, 0), "leg_up*": (-20, 0, 0), "neck1": (10, 0, 0), "head": (6, 0, 0),
                              "belly": (-6, 0, 0)}, bend(lambda s: -8.0, "pitch", [n for n, _ in TAIL_S]))
    (clip("hop", 0.9).pose(0.0, STAND).pose(0.25, CROUCH).pose(0.45, HOP_UP).pose(0.7, CROUCH).pose(0.9, STAND)
     .root(0.0).root(0.25).root(0.45, up=0.5).root(0.65).root(0.9).event(0.65, "land").event(0.35, "squeak"))

    # ---- play
    # A pounce: it draws its neck back into an S, wiggles, then springs.
    COILED = merge(CROUCH, {"neck1": (-26, 0, 0), "neck2": (-14, 0, 0), "neck3": (16, 0, 0), "head": (24, 0, 0)})
    SPRING = merge(FIN_OPEN, {"arm_up*": (50, 0, 0), "arm_lo*": (-20, 0, 0), "leg_up*": (-40, 0, 0),
                              "neck1": (-20, 0, 0), "neck2": (-4, 0, 0), "head": (14, 0, 0), "jaw": (-22, 0, 0)},
                   bend(lambda s: -6.0, "pitch", [n for n, _ in TAIL_S]))
    (clip("pounce", 1.4).pose(0.0, STAND).pose(0.35, COILED).pose(0.65, COILED).pose(0.85, SPRING)
     .pose(1.05, CROUCH).pose(1.4, STAND)
     .wave(lambda t: merge({"hips": (0, 5 * sin01(t, 0.15), 0)} if 0.35 < t < 0.65 else {},
                           tail_wave(0.3, 4.0, 18.0)(t) if 0.3 < t < 0.7 else {}))
     .root(0.0).root(0.65).root(0.85, up=0.45).root(1.05).root(1.4)
     .event(0.75, "squeak").event(1.05, "land"))
    (clip("play_bow", 1.2).pose(0.0, STAND).pose(0.3, PLAY_BOW).pose(0.9, PLAY_BOW).pose(1.2, STAND)
     .wave(lambda t: {k: tuple(x * window(t, 0.25, 0.95) for x in v) for k, v in
                      tail_wave(0.3, 6.0, 24.0, wavelength=2.6)(t).items()})
     .event(0.35, "call"))

    def spar_wave(t):
        s, c = sin01(t, 0.5), sin01(t, 0.5, 0.25)
        return merge({"arm_up_L": (50 + 30 * s, 0, -12), "arm_lo_L": (-20 * s, 0, 0),
                      "arm_up_R": (50 - 30 * s, 0, 12), "arm_lo_R": (20 * s, 0, 0),
                      "head": (6 * c, 8 * s, 0), "neck2": (0, 6 * s, 0), "jaw": (-14 - 6 * c, 0, 0)},
                     tail_wave(0.5, 4.0, 16.0)(t))
    (clip("spar", 1.0, loop=True).pose(0.0, SPAR).wave(spar_wave)
     .root(0.0, up=0.0).root(0.25, up=0.05).root(0.5, up=0.0).root(0.75, up=0.05)
     .event(0.1, "squeak").event(0.6, "thump"))
    stalk = clip("stalk", 1.4, loop=True, speed=0.22).pose(0.0, STALK)
    stalk.wave(body_wave(1.4, ramp(2.0, 9.0, 1.0, LENGTH), steady=1.0)).wave(steps(1.4, 12, 12, 20, 0.0, bob=0.5))
    stalk.wave(lambda t: {"tail8": (0, 22 * sin01(t, 0.35), 0), "tail7": (0, 8 * sin01(t, 0.35, -0.1), 0)})
    tail_chase = clip("tail_chase", 0.6, loop=True).pose(0.0, TAIL_CHASE)
    tail_chase.wave(steps(0.6, 22, 18, 30, 0.0, bob=2.5)).wave(pant(6.0, 0.3)).wave(tail_wave(0.3, 2.0, 14.0))
    tail_chase.root(0.0, up=0.0).root(0.15, up=0.1).root(0.3, up=0.0).root(0.45, up=0.1)
    footsteps(tail_chase, 0.6, (0.25, 0.75))

    # The happy wiggle (tail_wag): the long tail swings in big waves, the hips sway against the
    # chest so it stays in place, the head tilts and the front paws tippy-tap.
    def happy_wag(t):
        s = sin01(t, 0.5)
        out = {"hips": (0, 4 * s, 2 * s), "belly": (0, -7 * s, 0), "chest": (1.5 * sin01(t, 0.25), 3 * s, -2 * s),
               "neck1": (0, -1.5 * s, 0), "head": (0, 1.5 * s, 7 * sin01(t, 1.0, 0.25))}
        out = merge(out, tail_wave(0.5, 8.0, 26.0, wavelength=2.4)(t))
        for side, phase in (("L", 0.0), ("R", 0.5)):
            lift = max(0.0, sin01(t, 1.0, -phase)) ** 1.5
            out[f"arm_up_{side}"] = (22 * lift, 0, (8 if side == "R" else -8) * lift)
            out[f"arm_lo_{side}"] = (30 * lift, 0, 0)
            out[f"leg_up_{side}"] = (0, -4 * s, -2 * s)
        return out
    (clip("tail_wag", 1.0, loop=True)
     .pose(0.0, merge(STAND, {"neck1": (4, 0, 0), "head": (6, 0, 0), "jaw": (-8, 0, 0)}, bend(
         lambda s: -6.0, "pitch", ["tail1", "tail2"])))
     .wave(happy_wag).wave(fin_ripple(0.5, 6.0)))
    (clip("wing_flutter", 1.2).pose(0.0, STAND).pose(0.3, FIN_OPEN).pose(0.48, FIN_HALF).pose(0.66, FIN_OPEN)
     .pose(1.2, STAND).wave(lambda t: {k: tuple(x * window(t, 0.15, 0.9) for x in v)
                                       for k, v in fin_ripple(0.4, 12.0)(t).items()})
     .event(0.3, "flap").event(0.66, "flap"))

    # ---- feelings
    for suffix, sulk in (("", SULK), ("_h", SULK_BABY)):
        (clip("sulk" + suffix, 1.5).pose(0.0, STAND).pose(0.7, CROUCH_FOLDED).pose(1.5, sulk)
         .event(0.5, "whimper").event(1.2, "thump"))
        (clip("sulk_loop" + suffix, 5.0, loop=True).pose(0.0, sulk)
         .wave(lambda t: {"chest": (3 * max(0.0, sin01(t, 5.0)), 0, 0), "head": (-2 * max(0.0, sin01(t, 5.0)), 0, 0)}))
    (clip("nuzzle", 1.4, loop=True)
     .pose(0.0, merge(STAND, {"neck1": (6, 0, 0), "neck2": (4, 0, 0), "head": (-6, 0, 0)}))
     .wave(lambda t: {"head": (0, 10 * sin01(t, 1.4), 14 * sin01(t, 1.4)), "neck3": (0, 6 * sin01(t, 1.4, 0.1), 0)})
     .wave(tail_wave(0.7, 2.0, 12.0)).event(0.3, "purr"))
    GREET_UP = merge(FIN_OPEN, {"hips": (6, 0, 0), "belly": (8, 0, 0), "arm_up*": (30, 0, 0), "neck1": (-4, 0, 0),
                                "head": (6, 0, 0), "jaw": (-22, 0, 0)})
    (clip("greet", 1.6).pose(0.0, STAND).pose(0.2, CROUCH).pose(0.45, GREET_UP)
     .pose(0.7, CROUCH).pose(1.0, merge(FIN_HALF, {"neck1": (8, 0, 0), "head": (-4, 10, 12), "jaw": (-12, 0, 0)}))
     .pose(1.6, STAND)
     .wave(tail_wave(0.4, 6.0, 20.0))
     .root(0.0).root(0.2).root(0.45, up=0.45).root(0.7).root(1.6).event(0.4, "call").event(0.7, "land"))

    # ---- hands-on care (core/behavior.cpp times the grab and the drop against these: the jaw
    # closes on the ball 0.35 s into pick_up / leap_catch and opens 0.45 s into drop_wait)
    PICK = merge(EAT, {"neck1": (6, 0, 0)})
    PICK_BABY = merge(EAT_BABY, {"head": (-4, 0, 0)})
    for name, down in (("pick_up", PICK), ("pick_up_h", PICK_BABY)):
        (clip(name, 0.8).pose(0.0, STAND)
         .pose(0.25, merge(down, {"jaw": (-22, 0, 0)})).pose(0.4, merge(down, {"jaw": (-3, 0, 0)}))
         .pose(0.8, merge(STAND, CARRY_HEAD)))
    look_up = {"neck1": (6, 0, 0), "head": (10, 0, 0)}  # sitting, looking up at you, eager
    for name, down, sit in (("drop_wait", merge(STAND, {"neck1": (-20, 0, 0), "neck2": (-10, 0, 0), "head": (-8, 0, 0)}),
                             SIT),
                            ("drop_wait_h", merge(STAND, {"neck1": (-6, 0, 0), "head": (-18, 0, 0)}), SIT_BABY)):
        sit_up = merge(sit, look_up)
        (clip(name, 1.3).pose(0.0, merge(STAND, CARRY_HEAD))
         .pose(0.35, merge(down, {"jaw": (-4, 0, 0)})).pose(0.5, merge(down, {"jaw": (-22, 0, 0)}))
         .pose(0.9, merge(sit_up, {"jaw": (-8, 0, 0)})).pose(1.3, merge(sit_up, {"jaw": (-6, 0, 0)}))
         .wave(lambda t: {k: tuple(x * smooth01((t - 0.7) * 3) for x in v)
                          for k, v in tail_wave(0.4, 2.0, 12.0)(t).items()}))
    CATCH = merge(FIN_OPEN, {"arm_up*": (30, 0, 0), "neck1": (10, 0, 0), "head": (14, 0, 0), "hips": (8, 0, 0)})
    (clip("leap_catch", 0.9).pose(0.0, STAND).pose(0.15, CROUCH)
     .pose(0.32, merge(CATCH, {"jaw": (-24, 0, 0)})).pose(0.45, merge(CATCH, {"jaw": (-3, 0, 0), "head": (-8, 0, 0)}))
     .pose(0.7, CROUCH).pose(0.9, merge(STAND, CARRY_HEAD))
     .root(0.0).root(0.15).root(0.4, up=0.55).root(0.7).root(0.9).event(0.7, "land"))
    # Scratched in just the right spot: a hind foot thumps and the tail's tip curls happily.
    KICK = merge(STAND, {"neck1": (4, 0, 0), "head": (6, 8, 14), "leg_up_R": (40, 0, 20), "hips": (0, 0, -4)})
    (clip("leg_kick", 1.4).pose(0.0, STAND).pose(0.25, KICK).pose(1.15, KICK).pose(1.4, STAND)
     .wave(lambda t: merge({"leg_up_R": (24 * sin01(t, 0.18) if 0.25 < t < 1.15 else 0.0, 0, 0)},
                           tail_wave(0.35, 3.0, 16.0)(t)))
     .event(0.3, "thump").event(0.48, "thump").event(0.66, "thump").event(0.84, "thump"))
    (clip("sniff_refuse", 1.3).pose(0.0, STAND)
     .pose(0.35, merge(STAND, {"neck1": (-14, 0, 0), "neck2": (-6, 0, 0), "head": (-12, 0, 0)}))
     .pose(0.75, merge(STAND, {"neck2": (4, 22, 0), "neck3": (2, 18, 0), "head": (14, 26, -8)}))
     .pose(1.3, STAND).event(0.35, "sniff").event(0.8, "whimper"))
    # Grooming under a fin: the right one lifts halfway, then settles.
    FIN_R_UP = merge(sided(expand(STAND), "_L"), sided(expand(FIN_HALF), "_R"))
    (clip("lift_wing", 1.4).pose(0.0, STAND).pose(0.35, merge(FIN_R_UP, {"chest": (0, 0, -6)}))
     .pose(1.05, merge(FIN_R_UP, {"chest": (0, 0, -6)})).pose(1.4, STAND).event(0.35, "flap"))
    (clip("sneeze", 0.8).pose(0.0, STAND)
     .pose(0.28, merge(STAND, {"neck1": (6, 0, 0), "head": (14, 0, 0), "jaw": (-8, 0, 0)}))
     .pose(0.42, merge(STAND, {"neck1": (-6, 0, 0), "head": (-16, 0, 0), "jaw": (-4, 0, 0)}))
     .pose(0.8, STAND).event(0.4, "sneeze"))
    (clip("pull_away", 0.9).pose(0.0, STAND)
     .pose(0.3, merge(STAND, {"neck1": (16, 0, 0), "neck2": (10, 0, 0), "head": (8, 0, 0), "chest": (4, 0, 0),
                              "hips": (-3, 0, 0)}))
     .pose(0.9, STAND)
     .wave(lambda t: {"head": (0, 16 * sin01(t, 0.16) * max(0.0, 1 - abs(t - 0.45) / 0.25), 0)})
     .event(0.3, "whimper"))

    # ---- toys
    BOW = merge(FOLD, {"hips": (-6, 0, 0), "arm_up*": (30, 0, 8), "arm_lo*": (-10, 0, 0), "chest": (-6, 0, 0),
                       "neck1": (-10, 0, 0), "head": (10, 0, 0), "tail1": (-10, 0, 0)})
    SWAT = merge(BOW, {"arm_up_L": (60, -10, -10), "arm_lo_L": (30, 0, 0), "chest": (0, 0, 6), "neck1": (4, 0, 0),
                       "head": (14, 8, -12), "jaw": (-14, 0, 0)})
    (clip("paw_bat", 0.75).pose(0.0, BOW).pose(0.22, SWAT).pose(0.4, SWAT).pose(0.75, BOW)
     .wave(tail_wave(0.375, 4.0, 16.0)).event(0.22, "squeak"))
    TUG = merge(FOLD, {"hips": (4, 0, 0), "leg_up*": (16, 0, 8), "arm_up*": (-16, 0, 8), "arm_lo*": (10, 0, 0),
                       "chest": (-4, 0, 0), "neck1": (-30, 0, 0), "neck2": (-10, 0, 0), "head": (10, 0, 0),
                       "jaw": (-5, 0, 0)})
    (clip("tug", 0.9, loop=True).pose(0.0, TUG)
     .wave(lambda t: merge({"neck1": (0, 12 * sin01(t, 0.45), 0), "neck2": (0, 8 * sin01(t, 0.45, 0.1), 0),
                            "head": (0, 6 * sin01(t, 0.45, 0.2), 10 * sin01(t, 0.45, 0.15)),
                            "hips": (0, 0, 3 * sin01(t, 0.9))}, tail_wave(0.45, 6.0, 20.0)(t))))

    clipkit.check(out, BONE_ORDER, NAME)
    return out
