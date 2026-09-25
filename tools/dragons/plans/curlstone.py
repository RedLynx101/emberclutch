"""The Curlstone's body plan: a low, long, armoured four-legged dragon that curls up into a
ball like a pangolin, with a pair of short, sturdy wings folded under the edge of its plates.

The skeleton is the classic one reshaped for curling: a longer, bendier back (four torso
bones: hips, loin, belly, chest) and a short, thick neck (neck2, neck3: the look-at bones),
so the back can arch round into a ball; a thick four-bone tail that wraps round the ball;
short digging legs; three-fingered wings. 26 body bones, 10 wing bones.

Its clips are its own (eca.py conventions: (pitch, yaw, roll) degree deltas in armature axes
on top of the idle pose; "name*" keys both sides). The character: a low, steady waddle
with a roll of the shoulders, a determined trot with the head down, and to run it tucks into
a ball and rolls like a boulder. It curls into a ball to sleep and to sulk, hides its face
in a half curl when a touch is too much, sniffs and scrapes at the ground, sits up on its
haunches propped on its tail like a pangolin, rocks on its round back for a belly rub, and
flies slowly on short, quick wings.
"""
import math

from dragons import clipkit
from dragons.clipkit import Clip

NAME = "curlstone"

BONES = [
    ("hips", "hips", "loin", None),
    ("loin", "loin", "belly", "hips"),
    ("belly", "belly", "chest", "loin"),
    ("chest", "chest", "neck2", "belly"),
    ("neck2", "neck2", "neck3", "chest"),
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

# Short, broad wings: an arm, a forearm and three fingers fanning a rounded membrane.
WING_CHAIN = [("wing_arm", "root", "elbow", "chest"), ("wing_fore", "elbow", "wrist", "wing_arm"),
              ("wing_f1", "wrist", "f1", "wing_fore"), ("wing_f2", "wrist", "f2", "wing_fore"),
              ("wing_f3", "wrist", "f3", "wing_fore")]
WING_BONES = [f"{n}_{side}" for side in ("L", "R") for n, *_ in WING_CHAIN]
BONE_ORDER = [b[0] for b in BONES] + WING_BONES
WING_BODY = ("chest", "belly", "loin")
CONTACTS = ("hand_L", "hand_R", "foot_L", "foot_R")
SEAT = ("belly", (0.0, 0.1, 0.62))
REGION = {}

PARENT = {b[0]: b[3] for b in BONES}
PARENT.update({f"{n}_{s}": (f"{p}_{s}" if p.startswith("wing") else p) for s in "LR" for n, _, _, p in WING_CHAIN})
SPINE_FRONT = ("loin", "belly", "chest", "neck2", "neck3", "head")


# ------------------------------------------------------------------------------ rotations
# A clip's deltas compose down the chain: a bone's whole turn (in armature axes, from its
# rest) is its parent's whole turn times its own delta. Poses like the ball are easier to
# think of as whole turns; these helpers turn them into deltas, and blend poses as rotations.
def q_mul(a, b):
    aw, ax, ay, az = a
    bw, bx, by, bz = b
    return (aw * bw - ax * bx - ay * by - az * bz, aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx, aw * bz + ax * by - ay * bx + az * bw)


def q_conj(q):
    return (q[0], -q[1], -q[2], -q[3])


def q_axis(axis, degrees):
    h = math.radians(degrees) * 0.5
    return (math.cos(h), axis[0] * math.sin(h), axis[1] * math.sin(h), axis[2] * math.sin(h))


def q_from_pyr(pitch, yaw, roll):
    """eca.q_from_pyr: roll about -Y, then pitch about -X, then yaw about +Z."""
    q = q_axis((0.0, -1.0, 0.0), roll)
    q = q_mul(q_axis((-1.0, 0.0, 0.0), pitch), q)
    return q_mul(q_axis((0.0, 0.0, 1.0), yaw), q)


def pyr_from_q(q):
    """The inverse of q_from_pyr (q = Rz(yaw) Rx(-pitch) Ry(-roll))."""
    w, x, y, z = q
    m21 = 2 * (y * z + w * x)
    m20 = 2 * (x * z - w * y)
    m22 = 1 - 2 * (x * x + y * y)
    m01 = 2 * (x * y - w * z)
    m11 = 1 - 2 * (x * x + z * z)
    a = math.asin(max(-1.0, min(1.0, m21)))
    b = math.atan2(-m20, m22)
    c = math.atan2(-m01, m11)
    return (-math.degrees(a), math.degrees(c), -math.degrees(b))


def q_slerp(a, b, t):
    d = sum(x * y for x, y in zip(a, b))
    if d < 0:
        b, d = tuple(-x for x in b), -d
    if d > 0.9995:
        q = tuple(x + (y - x) * t for x, y in zip(a, b))
    else:
        th = math.acos(d)
        s = math.sin(th)
        q = tuple((math.sin((1 - t) * th) * x + math.sin(t * th) * y) / s for x, y in zip(a, b))
    n = math.sqrt(sum(x * x for x in q))
    return tuple(x / n for x in q)


def expand(pose):
    """A pose with "name*" keys written out for both sides (yaw and roll mirrored)."""
    out = {}
    for bone, (p, y, r) in pose.items():
        if bone.endswith("*"):
            out[bone[:-1] + "_R"] = (p, y, r)
            out[bone[:-1] + "_L"] = (p, -y, -r)
        else:
            out[bone] = (p, y, r)
    return out


def merge(*poses):
    """Poses added bone by bone (small deltas add like this; big turns use deltas())."""
    out = {}
    for pose in poses:
        for bone, v in expand(pose).items():
            a = out.get(bone, (0.0, 0.0, 0.0))
            out[bone] = tuple(x + y for x, y in zip(a, v))
    return out


def blend(a, b, t):
    """Pose a turned a fraction t of the way to pose b (as rotations, bone by bone)."""
    a, b = expand(a), expand(b)
    out = {}
    for bone in set(a) | set(b):
        qa = q_from_pyr(*a.get(bone, (0.0, 0.0, 0.0)))
        qb = q_from_pyr(*b.get(bone, (0.0, 0.0, 0.0)))
        out[bone] = pyr_from_q(q_slerp(qa, qb, t))
    return out


def deltas(total):
    """{bone: whole turn} -> {bone: (pitch, yaw, roll) delta}. A whole turn is a (p, y, r)
    triple or a quaternion; bones not given keep their parent's turn (a zero delta)."""
    def whole(b):
        if b is None:
            return (1.0, 0.0, 0.0, 0.0)
        if b in total:
            t = total[b]
            return q_from_pyr(*t) if len(t) == 3 else t
        return whole(PARENT[b])

    def pitch_only(b):
        while b is not None and b not in total:
            b = PARENT[b]
        return 0.0 if b is None else (total[b][0] if len(total[b]) == 3 and not any(total[b][1:]) else None)

    out = {}
    for b in total:
        t, pp = total[b], pitch_only(PARENT[b])
        if len(t) == 3 and not any(t[1:]) and pp is not None:  # a pitch under a pitch: plain difference
            out[b] = (t[0] - pp, 0.0, 0.0)
        else:
            out[b] = pyr_from_q(q_mul(q_conj(whole(PARENT[b])), whole(b)))
    return out


def sin01(t, period, phase=0.0):
    return math.sin(2 * math.pi * (t / period + phase))


def bump(t, start, end, ease=0.25):
    """0 outside [start, end], rising to 1 and falling back smoothly inside it."""
    if t <= start or t >= end:
        return 0.0
    u = (t - start) / (end - start)
    e = min(1.0, u / ease, (1 - u) / ease)
    return e * e * (3 - 2 * e)


# ------------------------------------------------------------------------------ poses
# Wings folded flat along the flanks, tucked under the lowest plates: the arm back, the
# forearm forward, the fingers back (solved from bone directions on the grown form: the
# wing's rest pose is spread, for flying).
WINGS_FOLDED = expand({"wing_arm*": (-59, 7, -95), "wing_fore*": (-31, -110, -26), "wing_f1*": (16, 114, -48),
                       "wing_f2*": (24, 67, -27), "wing_f3*": (18, 28, -8)})
WINGS_OPEN = expand({"wing_arm*": (0, -8, 8), "wing_fore*": (0, 0, 0), "wing_f1*": (0, 0, 0),
                     "wing_f2*": (0, 0, 0), "wing_f3*": (0, 0, 0)})
WINGS_HALF = blend(WINGS_FOLDED, WINGS_OPEN, 0.5)


def wings_following(pitch, fold=None):
    """The folded wings turned to follow the curve of a bent back: the whole wing pitched down
    with the belly (the membrane's flank edge follows the chest, belly and loin; the arm alone
    would leave it poking out)."""
    fold = dict(fold or WINGS_FOLDED)
    for s in ("R", "L"):
        p, y, r = fold[f"wing_arm_{s}"]
        q = q_mul(q_axis((-1.0, 0.0, 0.0), pitch), q_from_pyr(p, y, r))
        fold[f"wing_arm_{s}"] = pyr_from_q(q)
    return fold


def body_pose(total, legs=None, wing=0.0, extra=None):
    """A pose from whole turns of the back, neck and tail, plus leg deltas and extras."""
    d = deltas(total)
    d.update(expand(legs or {}))
    return merge(wings_following(wing) if wing else WINGS_FOLDED, d, extra or {})


def ball_pose(hips, bends, tail, legs, wing=60):
    """A curl from whole turns: the hips tipped up by `hips`, each bone of the back and neck
    (loin, belly, chest, neck2, neck3, head) bent down by `bends` from the one behind it, the
    tail's whole turns given directly (pitch, then a yaw about the upright)."""
    tot, acc = {"hips": (hips, 0, 0)}, hips
    for n, b in zip(SPINE_FRONT, bends):
        acc -= b
        tot[n] = (acc, 0, 0)
    tot.update(tail)
    return body_pose(tot, legs, wing)


STAND = dict(WINGS_FOLDED)
TUCK_LEGS = {"arm_up*": (30, 0, 10), "arm_lo*": (-130, 0, 0), "hand*": (50, 0, 0),
             "leg_up*": (-30, 0, 10), "leg_lo*": (-140, 0, 0), "foot*": (70, 0, 0)}
BALL_TAIL = {"tail1": (-45, 75, 0), "tail2": (-20, 125, 0), "tail3": (5, 165, 0), "tail4": (20, 205, 0)}
# The ball: the back arched round, the head tucked down with its crown to the front, the legs
# folded in, the tail wrapped round its side to lie by its nose.
BALL_BENDS = (52, 54, 54, 54, 46, 38)
CURL = ball_pose(116, BALL_BENDS, BALL_TAIL, TUCK_LEGS, wing=80)
# Rolling, its paws brace round the inside of the ball (hidden by it) instead of bunching in
# the middle: the game measures a run's speed from the lowest paw (den_actor locomotionSpeed),
# so paws spread round the rim make the ball travel about as fast as it turns.
ROLL = ball_pose(116, BALL_BENDS, BALL_TAIL, dict(expand(TUCK_LEGS), **{
    "arm_up_R": (138, 0, 10), "arm_lo_R": (20, 0, 0), "arm_up_L": (84, 0, -10), "arm_lo_L": (8, 0, 0),
    "leg_up_R": (84, 0, 10), "leg_lo_R": (38, 0, 0), "leg_up_L": (-66, 0, -10), "leg_lo_L": (-58, 0, 0)}),
    wing=80)
BALL_CENTRE = (-0.306, 0.566)   # the ball's middle from the hips joint (y back, z up), grown form
BALL_LIFT = 0.1                 # the plates stand this far off the skin: the ball rests on them
# Where each paw sits round the rolling ball (degrees from straight down, rolling forward
# carries it toward the back): each paw scrabbles backward as it comes round underneath.
ROLL_PAWS = {"arm_up_R": -133, "arm_up_L": -86, "leg_up_R": 107, "leg_up_L": 6}

# Hiding its face: a half curl, the head tucked under its chest, the tail drawn round.
SHY = body_pose({"hips": (30, 0, 0), "loin": (10, 0, 0), "belly": (-12, 0, 0), "chest": (-36, 0, 0),
                 "neck2": (-70, 0, 0), "neck3": (-100, 0, 0), "head": (-120, 0, 0),
                 "tail1": (-20, 50, 0), "tail2": (-10, 100, 0), "tail3": (0, 140, 0), "tail4": (10, 170, 0)},
                {"leg_up*": (20, 0, 0), "leg_lo*": (-60, 0, 0), "foot*": (30, 0, 0),
                 "arm_up*": (30, 0, 0), "arm_lo*": (-70, 0, 0), "hand*": (30, 0, 0)}, wing=30)
# The hatchling's round little body curls with a gentler bend and its short tail kept low
# (the grown one's rising tail would stand its tiny plates on end).
BABY_TAIL = {"tail1": (-8, 70, 0), "tail2": (2, 120, 0), "tail3": (10, 160, 0), "tail4": (18, 200, 0)}
BABY_CURL = ball_pose(90, (34, 38, 38, 30, 24, 18), BABY_TAIL, TUCK_LEGS, wing=40)
BABY_SHY = body_pose({"hips": (26, 0, 0), "loin": (8, 0, 0), "belly": (-12, 0, 0), "chest": (-32, 0, 0),
                      "neck2": (-58, 0, 0), "neck3": (-80, 0, 0), "head": (-96, 0, 0),
                      "tail1": (-6, 50, 0), "tail2": (0, 95, 0), "tail3": (6, 135, 0), "tail4": (12, 165, 0)},
                     {"leg_up*": (20, 0, 0), "leg_lo*": (-60, 0, 0), "foot*": (30, 0, 0),
                      "arm_up*": (30, 0, 0), "arm_lo*": (-70, 0, 0), "hand*": (30, 0, 0)}, wing=20)
# Sitting up on its haunches like a pangolin, propped on its tail, front paws held up.
SIT = body_pose({"hips": (52, 0, 0), "loin": (43, 0, 0), "belly": (33, 0, 0), "chest": (21, 0, 0),
                 "neck2": (8, 0, 0), "neck3": (0, 0, 0), "head": (-12, 0, 0),
                 "tail1": (8, 0, 0), "tail2": (-8, 0, 0), "tail3": (-14, 0, 0), "tail4": (-14, 0, 0)},
                {"leg_up*": (35, 0, 0), "leg_lo*": (-100, 0, 0), "foot*": (10, 0, 0),
                 "arm_up*": (30, 0, 0), "arm_lo*": (-70, 0, 0), "hand*": (20, 0, 0)}, wing=-10)
LIE = merge(WINGS_FOLDED, {
    "arm_up*": (62, 0, 6), "arm_lo*": (-100, 0, 0), "hand*": (38, 0, 0),
    "leg_up*": (55, 0, 6), "leg_lo*": (-105, 0, 0), "foot*": (50, 0, 0),
    "neck2": (-12, 0, 0), "neck3": (-4, 0, 0), "head": (-2, 0, 0),
    "tail2": (0, 10, 0), "tail3": (0, 16, 0), "tail4": (0, 20, 0),
})
# On its back, rocking on its round, plated shell like a turtle, paws in the air, the neck
# twisting so the head stays upright to look at you.
BELLY_UP = merge(WINGS_FOLDED, {
    "hips": (0, 0, 172), "loin": (-8, 0, 0), "belly": (-8, 0, 0), "chest": (-6, 0, 6),
    "arm_up*": (50, 0, 0), "arm_lo*": (-70, 0, 0), "leg_up*": (45, 0, 0), "leg_lo*": (-70, 0, 0),
    "neck2": (10, 0, -60), "neck3": (0, 0, -50), "head": (15, 0, -40),
    "tail1": (0, 24, 0), "tail2": (0, 24, 0), "tail3": (0, 20, 0)})
CROUCH_BODY = {
    "hips": (-6, 0, 0), "leg_up*": (34, 0, 0), "leg_lo*": (-60, 0, 0), "foot*": (26, 0, 0),
    "arm_up*": (34, 0, 0), "arm_lo*": (-58, 0, 0), "hand*": (24, 0, 0),
    "neck2": (-12, 0, 0), "head": (6, 0, 0), "tail1": (6, 0, 0),
}
CROUCH = merge(WINGS_FOLDED, CROUCH_BODY)
EAT = merge(WINGS_FOLDED, {
    "neck2": (-30, 0, 0), "neck3": (-20, 0, 0), "head": (-18, 0, 0),
    "arm_up*": (22, 0, 6), "arm_lo*": (-38, 0, 0), "hand*": (16, 0, 0), "chest": (-6, 0, 0),
})
# Snout to the ground, sniffing (a digger's habit).
SNIFF = merge(WINGS_FOLDED, {"neck2": (-24, 0, 0), "neck3": (-14, 0, 0), "head": (-16, 0, 0),
                             "arm_up*": (12, 0, 4), "arm_lo*": (-20, 0, 0), "hand*": (8, 0, 0)})


# ------------------------------------------------------------------------------ layers
def breathe(amount=1.0, period=3.6):
    def fn(t):
        s = sin01(t, period) * amount
        return {"chest": (1.0 * s, 0, 0), "belly": (-0.8 * s, 0, 0), "loin": (0.5 * s, 0, 0),
                "neck2": (0.6 * s, 0, 0), "head": (-0.6 * s, 0, 0),
                "wing_arm_R": (0, 0, 1.0 * s), "wing_arm_L": (0, 0, -1.0 * s)}
    return fn


def tail_sway(amount=1.0, period=3.6):
    """A heavy tail: a slow, small sway that grows toward the club."""
    def fn(t):
        return {f"tail{k}": (0, amount * (1.5 + 1.5 * k) * sin01(t, period, -0.1 * k), 0) for k in range(1, 5)}
    return fn


def sniffing(amount=3.0, period=0.22, start=0.0, end=1e9):
    """The snout twitching as it sniffs."""
    def fn(t):
        if not start < t < end:
            return {}
        return {"snout": (amount * sin01(t, period), 0, 0), "head": (0.6 * amount * sin01(t, period, 0.3), 0, 0)}
    return fn


def leg_cycle(period, amp_up, amp_bend, phases, bob=2.0, roll=0.0, head=1.0):
    """Walking legs: each limb swings (pitch) and bends its lower joint while swinging forward.
    roll: the waddle, the body rocking over the planted feet."""
    def fn(t):
        out = {}
        for upper, ph in phases.items():
            side = upper[-2:]
            front = upper.startswith("arm")
            swing = sin01(t, period, -ph)
            lift = max(0.0, math.cos(2 * math.pi * (t / period - ph)))
            lower, tip = ("arm_lo", "hand") if front else ("leg_lo", "foot")
            out[upper] = (amp_up * swing, 0, 0)
            out[lower + side] = (-amp_bend * lift, 0, 0)
            out[tip + side] = ((-0.6 if front else 0.4) * amp_bend * lift, 0, 0)
        b = sin01(t, period / 2)
        w = -sin01(t, period)  # the body leans over the planted front paw
        out["chest"] = (bob * 0.4 * b, 0, roll * w)
        out["hips"] = (0, 2.0 * sin01(t, period, 0.25), -roll * 0.6 * w)
        out["neck2"] = (-bob * b * head, 0, -roll * 0.7 * w)
        out["head"] = (bob * 0.6 * b * head, 0, roll * 0.3 * w)
        return out
    return fn


def footsteps(c, period, phases, cycles=1):
    for k in range(cycles):
        for ph in phases.values():
            c.event((ph + 0.25 + k) * period % (period * cycles), "footstep")


def chomp(amount=18.0, period=0.6, at=0.15):
    def fn(t):
        u = ((t - at) / period) % 1.0
        return {"jaw": (-amount * math.sin(math.pi * (u - 0.5) / 0.5) ** 0.7 if u > 0.5 else 0.0, 0, 0)}
    return fn


def pant(amount=6.0, period=0.3):
    return lambda t: {"jaw": (-amount - 2 * sin01(t, period), 0, 0)}


def rolling(clip, period, centre=BALL_CENTRE, keys=16, paws=None, scrabble=50.0, lift=BALL_LIFT):
    """Roll the whole curled body forward once per period about the hips joint, with a root
    track that keeps the ball's middle gliding level (the game grounds the lowest point) and
    lifts it onto its plates. Each paw in `paws` sweeps backward as it passes underneath."""
    clip.wave(lambda t: {"hips": (-360.0 * t / period, 0, 0)})
    if paws:
        def fn(t):
            a = 360.0 * t / period
            return {b: (-scrabble * math.sin(math.radians(th + a)), 0, 0) for b, th in paws.items()}
        clip.wave(fn)
    dy, dz = centre
    for i in range(keys):
        t = period * i / keys
        a = 2 * math.pi * t / period
        clip.root(t, forward=(dy * math.cos(a) - dz * math.sin(a)) - dy, up=lift)
    return clip


WALK_PHASES = {"leg_up_L": 0.0, "arm_up_L": 0.25, "leg_up_R": 0.5, "arm_up_R": 0.75}
TROT_PHASES = {"arm_up_L": 0.0, "leg_up_R": 0.0, "arm_up_R": 0.5, "leg_up_L": 0.5}
BOUND_PHASES = {"leg_up_L": 0.0, "leg_up_R": 0.06, "arm_up_L": 0.5, "arm_up_R": 0.56}


# ------------------------------------------------------------------------------ clips
def build():
    clips = []

    def clip(*args, **kwargs):
        c = Clip(*args, **kwargs)
        clips.append(c)
        return c

    # ---------------------------------------------------------------- idle & getting about
    clip("idle", 3.6, loop=True).pose(0.0, STAND).wave(breathe()).wave(tail_sway())
    # A curious look round, the digger's way: the snout down sniffing left and right, then up.
    sniff_l = merge(SNIFF, {"neck2": (0, 14, 0), "neck3": (0, 10, 0), "head": (0, 8, 4)})
    sniff_r = merge(SNIFF, {"neck2": (0, -14, 0), "neck3": (0, -10, 0), "head": (0, -8, -4)})
    look_up = merge(STAND, {"neck2": (6, 10, 0), "neck3": (4, 8, 0), "head": (8, 8, 6)})
    (clip("look_around", 3.2).pose(0.0, STAND).pose(0.5, sniff_l).pose(1.1, sniff_l).pose(1.5, sniff_r)
     .pose(2.1, sniff_r).pose(2.6, look_up).pose(3.2, STAND)
     .wave(sniffing(3.0, 0.2, 0.4, 2.2)).wave(tail_sway()).event(0.6, "sniff").event(1.6, "sniff"))
    scratch = merge(STAND, {"leg_up_R": (60, -14, 0), "leg_lo_R": (-36, 0, 0), "foot_R": (20, 0, 0),
                            "hips": (4, 0, -8), "neck2": (-6, -22, 0), "neck3": (0, -16, 0), "head": (-6, -10, -18)})
    (clip("scratch", 2.1).pose(0.0, STAND).pose(0.4, scratch).pose(1.7, scratch).pose(2.1, STAND)
     .wave(lambda t: {"leg_lo_R": (10 * sin01(t, 0.2), 0, 0), "foot_R": (14 * sin01(t, 0.2), 0, 0)}
           if 0.5 < t < 1.6 else {}))
    # A low, steady waddle: short heavy steps, the shoulders rolling over each planted paw.
    walk = clip("walk", 1.2, loop=True, speed=0.8).pose(0.0, STAND)
    walk.wave(leg_cycle(1.2, 34, 42, WALK_PHASES, bob=1.6, roll=4.0)).wave(tail_sway(1.4, 1.2))
    footsteps(walk, 1.2, WALK_PHASES)
    # A hatchling toddles: quick little steps, the big head bobbing, rocking side to side.
    walk_h = clip("walk_h", 0.58, loop=True, speed=0.5).pose(0.0, STAND)
    walk_h.wave(leg_cycle(0.58, 30, 44, WALK_PHASES, bob=3.5, roll=5.0)).wave(tail_sway(1.6, 0.58))
    footsteps(walk_h, 0.58, WALK_PHASES)
    # A determined trot: head down and forward, the plates jouncing.
    trot = clip("trot", 0.7, loop=True, speed=1.5).pose(0.0, merge(STAND, {"neck2": (-10, 0, 0), "head": (6, 0, 0)}))
    trot.wave(leg_cycle(0.7, 26, 48, TROT_PHASES, bob=2.6, roll=2.0, head=0.6)).wave(tail_sway(1.2, 0.7))
    footsteps(trot, 0.7, {"a": 0.0, "b": 0.5})
    trot_h = clip("trot_h", 0.42, loop=True, speed=1.0)
    trot_h.pose(0.0, merge(STAND, {"neck2": (-6, 0, 0), "head": (4, 0, 0)}))
    trot_h.wave(leg_cycle(0.42, 32, 52, TROT_PHASES, bob=3.0, roll=2.0, head=0.6)).wave(tail_sway(1.4, 0.42))
    footsteps(trot_h, 0.42, {"a": 0.0, "b": 0.5})
    shuffle = clip("shuffle", 1.1, loop=True).pose(0.0, STAND)
    shuffle.wave(leg_cycle(1.1, 10, 22, WALK_PHASES, bob=1.0, roll=2.0))
    footsteps(shuffle, 1.1, WALK_PHASES)
    carry_pose = merge(STAND, {"neck2": (10, 0, 0), "head": (-6, 0, 0)})
    carry = clip("carry", 1.1, loop=True, speed=0.8).pose(0.0, carry_pose)
    carry.wave(leg_cycle(1.1, 30, 40, WALK_PHASES, bob=1.4, roll=4.0, head=0.3)).wave(tail_sway(1.6, 0.55))
    footsteps(carry, 1.1, WALK_PHASES)
    carry_h = clip("carry_h", 0.56, loop=True, speed=0.5).pose(0.0, carry_pose)
    carry_h.wave(leg_cycle(0.56, 36, 48, WALK_PHASES, bob=3.0, roll=4.0, head=0.3)).wave(tail_sway(1.8, 0.28))
    footsteps(carry_h, 0.56, WALK_PHASES)
    # To run, a grown Curlstone tucks into a ball and rolls like a boulder.
    gallop = rolling(clip("gallop", 1.2, loop=True, speed=4.0).pose(0.0, ROLL), 1.2, paws=ROLL_PAWS)
    gallop.event(0.05, "thump").event(0.65, "thump")
    # A hatchling scampers: a round little pebble bounding along, the hind paws pushing off
    # together, a hop in every stride, the big head bobbing.
    scamper = clip("scamper", 0.42, loop=True, speed=2.0).pose(
        0.0, merge(STAND, {"neck2": (-4, 0, 0), "head": (6, 0, 0), "tail1": (-8, 0, 0)}))
    scamper.wave(leg_cycle(0.42, 42, 55, BOUND_PHASES, bob=4.0, roll=0.0))
    scamper.wave(lambda t: {"chest": (8 * sin01(t, 0.42, 0.1), 0, 0), "hips": (-6 * sin01(t, 0.42, 0.1), 0, 0)})
    scamper.wave(tail_sway(1.6, 0.42)).wave(pant(8.0, 0.21))
    scamper.root(0.0, up=0.0).root(0.14, up=0.14).root(0.28, up=0.03)
    footsteps(scamper, 0.42, {"hind": 0.0, "front": 0.5})

    # ---------------------------------------------------------------- flight
    # Heavy and slow in the air: short wings beating fast, legs drawn up, the tail streaming.
    fly_body = merge(WINGS_OPEN, {"arm_up*": (-50, 0, 0), "arm_lo*": (80, 0, 0), "hand*": (-30, 0, 0),
                                  "leg_up*": (-62, 0, 0), "leg_lo*": (50, 0, 0), "foot*": (-40, 0, 0),
                                  "neck2": (-16, 0, 0), "neck3": (-4, 0, 0), "head": (12, 0, 0), "tail1": (-8, 0, 0)})

    def wingbeat(period, amount, phase=0.0):
        def fn(t):
            s = sin01(t, period, phase)
            h = sin01(t, period, phase - 0.12)
            return {"wing_arm_R": (0, 0, amount * s), "wing_arm_L": (0, 0, -amount * s),
                    "wing_fore_R": (0, 0, 0.35 * amount * h), "wing_fore_L": (0, 0, -0.35 * amount * h),
                    "chest": (2.0 * s, 0, 0), "neck2": (-1.5 * s, 0, 0)}
        return fn
    clip("fly_flap", 0.4, loop=True).pose(0.0, fly_body).wave(wingbeat(0.4, 44)).wave(tail_sway(0.5, 0.8))\
        .event(0.05, "flap")
    (clip("fly_glide", 2.4, loop=True).pose(0.0, merge(fly_body, {"wing_arm*": (0, 0, -6)}))
     .wave(wingbeat(2.4, 5)).wave(tail_sway(0.6, 2.4)))
    (clip("fly_dive", 1.0, loop=True).pose(0.0, merge(fly_body, {"wing_arm*": (-10, 30, -26), "head": (18, 0, 0)}))
     .wave(wingbeat(0.25, 3)).wave(tail_sway(0.4, 0.5)))

    # ---------------------------------------------------------------- rest
    clip("sit", 1.0).pose(0.0, STAND).pose(0.45, blend(STAND, SIT, 0.55)).pose(1.0, SIT)
    (clip("sit_loop", 3.6, loop=True).pose(0.0, SIT).wave(breathe())
     .wave(lambda t: {"tail4": (0, 6 * sin01(t, 1.8), 0), "hand_R": (6 * sin01(t, 3.6), 0, 0),
                      "hand_L": (6 * sin01(t, 3.6, 0.5), 0, 0)}))
    clip("lie_down", 1.2).pose(0.0, STAND).pose(0.6, CROUCH).pose(1.2, LIE).event(1.0, "thump")
    clip("lie_loop", 4.0, loop=True).pose(0.0, LIE).wave(breathe(1.2, 4.0)).wave(tail_sway(0.4, 4.0))
    yawn_lie = merge(LIE, {"neck2": (18, 0, 0), "neck3": (8, 0, 0), "head": (22, 0, 0), "jaw": (-30, 0, 0)})

    def curling(sfx, curl, shy, lift):
        """Everything done curled up, for a body (sfx "_h": the hatchling's own ball). Curled,
        it rests on its plates (the game stands it on its skin): lifted by `lift`."""
        # Curling up: the head tucks, the back rounds, the tail wraps over.
        (clip("curl_up" + sfx, 1.8).pose(0.0, LIE).pose(0.8, blend(LIE, shy, 0.8)).pose(1.8, curl)
         .root(0.0).root(0.8, up=lift * 0.3).root(1.8, up=lift).event(1.6, "thump"))
        clip("sleep" + sfx, 5.0, loop=True).pose(0.0, curl).wave(breathe(1.8, 5.0)).root(0.0, up=lift)
        (clip("wake" + sfx, 2.8).pose(0.0, curl).pose(0.8, blend(curl, shy, 0.7)).pose(1.3, LIE).pose(1.7, yawn_lie)
         .pose(2.1, LIE).pose(2.8, STAND).root(0.0, up=lift).root(0.8, up=lift * 0.5).root(1.3).event(1.7, "yawn"))
        # Rolling over: it tucks into a ball, tips onto its back and opens up, paws in the air.
        tip = merge(blend(curl, shy, 0.5), {"hips": (0, 0, 70)})
        (clip("roll_over" + sfx, 1.4).pose(0.0, STAND).pose(0.4, shy).pose(0.85, tip).pose(1.4, BELLY_UP)
         .root(0.0).root(0.85, up=lift * 0.5).root(1.4, up=lift).event(0.9, "thump"))
        # Sulking, it curls up into a stone and won't come out, only peeking now and then.
        (clip("sulk" + sfx, 1.6).pose(0.0, STAND).pose(0.6, shy).pose(1.6, curl).event(0.5, "whimper")
         .root(0.0).root(0.6, up=lift * 0.3).root(1.6, up=lift).event(1.4, "thump"))
        (clip("sulk_loop" + sfx, 5.0, loop=True).pose(0.0, curl).root(0.0, up=lift)
         .wave(lambda t: {"chest": (2 * sin01(t, 5.0), 0, 0),
                          "neck2": (18 * bump(t, 2.4, 3.8), 0, 0), "neck3": (14 * bump(t, 2.4, 3.8), 0, 0),
                          "head": (16 * bump(t, 2.5, 3.7), 0, 0)}))
        # Too much: it ducks its head under and half curls, then peeks back out.
        (clip("pull_away" + sfx, 1.1).pose(0.0, STAND).pose(0.3, blend(STAND, shy, 0.85))
         .pose(0.7, blend(STAND, shy, 0.7)).pose(1.1, STAND).event(0.2, "whimper"))

    curling("", CURL, SHY, BALL_LIFT)
    curling("_h", BABY_CURL, BABY_SHY, BALL_LIFT * 0.4)
    yawn_up = merge(STAND, {"neck2": (14, 0, 0), "neck3": (8, 0, 0), "head": (24, 0, 0), "jaw": (-32, 0, 0)})
    clip("yawn", 1.6).pose(0.0, STAND).pose(0.6, yawn_up).pose(1.1, yawn_up).pose(1.6, STAND).event(0.6, "yawn")
    clip("nap_flop", 0.8).pose(0.0, STAND).pose(0.35, CROUCH).pose(0.8, LIE).event(0.65, "thump")

    # ---------------------------------------------------------------- care
    eat = clip("eat", 1.2, loop=True).pose(0.0, EAT)
    eat.wave(lambda t: {"head": (6 * max(0.0, sin01(t, 0.6)), 0, 0), "neck2": (3 * sin01(t, 1.2), 0, 0)})
    eat.wave(chomp()).event(0.15, "chomp").event(0.75, "chomp")
    # Its favourite: a happy little stomp from paw to paw, tail wagging, then a heavy hop.
    wig = merge(STAND, {"neck2": (10, 0, 0), "head": (10, 0, 0), "jaw": (-16, 0, 0)})
    (clip("fav_wiggle", 1.6).pose(0.0, STAND).pose(0.25, wig).pose(1.3, wig).pose(1.6, STAND)
     .wave(lambda t: {"hips": (0, 6 * sin01(t, 0.4), 0), "chest": (0, -4 * sin01(t, 0.4), 3 * sin01(t, 0.4)),
                      "arm_up_L": (14 * max(0.0, sin01(t, 0.4)), 0, 0),
                      "arm_up_R": (14 * max(0.0, sin01(t, 0.4, 0.5)), 0, 0),
                      **{f"tail{k}": (0, 10 * sin01(t, 0.3, -0.1 * k), 0) for k in range(1, 5)}}
           if 0.2 < t < 1.4 else {})
     .root(0.0).root(0.6).root(0.8, up=0.18).root(1.0).root(1.6).event(1.0, "land").event(0.3, "call"))
    pat = merge(STAND, {"neck2": (6, 0, 0), "neck3": (4, 0, 6), "head": (-10, 6, 14), "jaw": (-6, 0, 0)})
    (clip("pet_head", 1.8, loop=True).pose(0.0, pat)
     .wave(lambda t: {"head": (0, 4 * sin01(t, 1.8), 4 * sin01(t, 1.8))}).wave(tail_sway(0.8, 0.9))
     .event(0.4, "purr"))
    chin = merge(STAND, {"neck2": (14, 0, 0), "neck3": (8, 0, 0), "head": (22, 0, 0), "jaw": (-8, 0, 0)})
    (clip("pet_chin", 1.8, loop=True).pose(0.0, chin)
     .wave(lambda t: {"head": (4 * sin01(t, 1.8), 0, 0)}).wave(tail_sway(0.8, 0.9)).event(0.4, "purr"))
    (clip("belly_rub", 1.6, loop=True).pose(0.0, BELLY_UP).root(0.0, up=BALL_LIFT * 0.7)
     .wave(lambda t: {"leg_up_L": (10 * sin01(t, 0.8), 0, 0), "leg_up_R": (10 * sin01(t, 0.8, 0.5), 0, 0),
                      "arm_up_L": (8 * sin01(t, 0.8, 0.25), 0, 0), "arm_up_R": (8 * sin01(t, 0.8, 0.75), 0, 0),
                      "hips": (0, 0, 7 * sin01(t, 1.6))})
     .wave(pant(8.0, 0.8)).event(0.3, "purr"))

    def shake_wave(t):
        k = max(0.0, 1.0 - abs(t - 0.5) / 0.4)
        s = sin01(t, 0.14) * k
        return {"chest": (0, 0, 14 * s), "hips": (0, 0, -10 * s), "neck2": (0, 0, 12 * s), "head": (0, 0, 14 * s),
                "tail2": (0, 10 * s, 0), "tail3": (0, 12 * s, 0)}
    clip("shake", 1.0).pose(0.0, STAND).pose(1.0, STAND).wave(shake_wave).event(0.4, "shake")
    hop_up = merge(STAND, {"arm_up*": (24, 0, 0), "arm_lo*": (-40, 0, 0), "leg_up*": (-10, 0, 0),
                           "neck2": (8, 0, 0), "head": (8, 0, 0)})
    (clip("hop", 0.9).pose(0.0, STAND).pose(0.25, CROUCH).pose(0.45, hop_up).pose(0.7, CROUCH).pose(0.9, STAND)
     .root(0.0).root(0.25).root(0.45, up=0.3).root(0.65).root(0.9).event(0.65, "land").event(0.35, "squeak"))

    # ---------------------------------------------------------------- play
    lunge = merge(STAND, {"arm_up*": (-40, 0, 0), "arm_lo*": (10, 0, 0), "hand*": (10, 0, 0), "leg_up*": (-26, 0, 0),
                          "hips": (6, 0, 0), "neck2": (4, 0, 0), "head": (6, 0, 0), "jaw": (-20, 0, 0)})
    (clip("pounce", 1.4).pose(0.0, STAND).pose(0.35, CROUCH).pose(0.65, CROUCH).pose(0.85, lunge)
     .pose(1.05, CROUCH).pose(1.4, STAND)
     .wave(lambda t: {"hips": (0, 5 * sin01(t, 0.15), 0)} if 0.35 < t < 0.65 else {})
     .root(0.0).root(0.65).root(0.85, up=0.28).root(1.05).root(1.4).event(0.75, "squeak").event(1.05, "land"))
    # The play bow of a digger: chest down, rump and club up, front claws scraping the ground.
    bow = merge(STAND, {"hips": (-10, 0, 0), "chest": (-6, 0, 0), "arm_up*": (40, 0, 10), "arm_lo*": (-60, 0, 0),
                        "hand*": (30, 0, 0), "leg_up*": (10, 0, 0), "neck2": (20, 0, 0), "head": (-8, 0, 0),
                        "tail1": (-8, 0, 0), "tail2": (-4, 0, 0), "jaw": (-12, 0, 0)})
    (clip("play_bow", 1.3).pose(0.0, STAND).pose(0.3, bow).pose(1.0, bow).pose(1.3, STAND)
     .wave(lambda t: {"arm_up_L": (12 * sin01(t, 0.35), 0, 0), "hand_L": (-14 * sin01(t, 0.35), 0, 0),
                      "arm_up_R": (12 * sin01(t, 0.35, 0.5), 0, 0), "hand_R": (-14 * sin01(t, 0.35, 0.5), 0, 0),
                      **{f"tail{k}": (0, 16 * sin01(t, 0.35, -0.1 * k), 0) for k in range(2, 5)}}
           if 0.3 < t < 1.0 else {})
     .event(0.35, "call"))
    spar = merge(STAND, {"hips": (22, 0, 0), "chest": (6, 0, 0), "leg_up*": (-4, 0, 0), "leg_lo*": (-36, 0, 0),
                         "foot*": (26, 0, 0), "neck2": (-6, 0, 0), "head": (-8, 0, 0), "tail1": (-20, 0, 0)})

    def spar_wave(t):
        s, c = sin01(t, 0.6), sin01(t, 0.6, 0.25)
        return {"arm_up_L": (40 + 26 * s, 0, 12), "arm_lo_L": (-40 - 18 * s, 0, 0),
                "arm_up_R": (40 - 26 * s, 0, -12), "arm_lo_R": (-40 + 18 * s, 0, 0),
                "head": (6 * c, 8 * s, 0), "jaw": (-12 - 6 * c, 0, 0)}
    (clip("spar", 1.2, loop=True).pose(0.0, spar).wave(spar_wave)
     .root(0.0, up=0.0).root(0.3, up=0.04).root(0.6, up=0.0).root(0.9, up=0.04).event(0.1, "squeak"))
    stalk = clip("stalk", 1.4, loop=True, speed=0.2).pose(0.0, merge(CROUCH, {"neck2": (-8, 0, 0)}))
    stalk.wave(leg_cycle(1.4, 12, 22, WALK_PHASES, bob=0.5, roll=1.5)).wave(sniffing(2.0, 0.25))
    tc = merge(STAND, {"neck2": (0, 34, 0), "neck3": (0, 24, 0), "head": (-6, 18, 10), "chest": (0, 14, 0),
                       "hips": (0, 10, 0), "tail1": (0, 30, 0), "tail2": (0, 34, 0), "tail3": (0, 34, 0),
                       "tail4": (0, 30, 0), "jaw": (-14, 0, 0)})
    tail_chase = clip("tail_chase", 0.7, loop=True).pose(0.0, tc)
    tail_chase.wave(leg_cycle(0.7, 20, 34, TROT_PHASES, bob=2.0)).wave(pant(5.0, 0.35))
    footsteps(tail_chase, 0.7, {"a": 0.0, "b": 0.5})

    def happy_wag(t):
        s = sin01(t, 0.6)
        out = {"hips": (0, 4 * s, 2 * s), "belly": (0, -6 * s, 0), "chest": (0, 3 * s, -2 * s),
               "head": (0, 1.5 * s, 6 * sin01(t, 1.2, 0.25))}
        for k in range(1, 5):
            out[f"tail{k}"] = (0, (8 + 5 * k) * sin01(t, 0.6, -0.08 * k), 0)
        return out
    (clip("tail_wag", 1.2, loop=True)
     .pose(0.0, merge(STAND, {"tail1": (-10, 0, 0), "neck2": (4, 0, 0), "head": (6, 0, 0), "jaw": (-8, 0, 0)}))
     .wave(happy_wag))
    (clip("wing_flutter", 1.2).pose(0.0, STAND).pose(0.25, WINGS_OPEN).pose(0.45, WINGS_HALF)
     .pose(0.65, WINGS_OPEN).pose(1.2, STAND).event(0.25, "flap").event(0.65, "flap"))

    # ---------------------------------------------------------------- feelings
    (clip("nuzzle", 1.4, loop=True).pose(0.0, merge(STAND, {"neck2": (8, 0, 0), "head": (-6, 0, 0)}))
     .wave(lambda t: {"head": (0, 10 * sin01(t, 1.4), 12 * sin01(t, 1.4)), "neck3": (0, 6 * sin01(t, 1.4, 0.1), 0)})
     .wave(tail_sway(1.5, 0.7)).event(0.3, "purr"))
    greet_up = merge(WINGS_HALF, {"arm_up*": (30, 0, 0), "arm_lo*": (-44, 0, 0), "neck2": (12, 0, 0),
                                  "head": (10, 0, 0), "jaw": (-20, 0, 0)})
    (clip("greet", 1.6).pose(0.0, STAND).pose(0.2, CROUCH).pose(0.45, greet_up)
     .pose(0.7, CROUCH).pose(1.0, merge(STAND, {"neck2": (8, 0, 0), "head": (-4, 10, 12), "jaw": (-12, 0, 0)}))
     .pose(1.6, STAND)
     .wave(lambda t: {f"tail{k}": (0, 12 * sin01(t, 0.4, -0.08 * k), 0) for k in range(1, 5)})
     .root(0.0).root(0.2).root(0.45, up=0.28).root(0.7).root(1.6).event(0.4, "call").event(0.7, "land"))

    # ---------------------------------------------------------------- hands-on care
    carry_head = {"neck2": (10, 0, 0), "head": (-6, 0, 0)}
    pick = merge(STAND, {"neck2": (-30, 0, 0), "neck3": (-16, 0, 0), "head": (-20, 0, 0),
                         "arm_up*": (16, 0, 0), "arm_lo*": (-26, 0, 0), "chest": (-6, 0, 0)})
    (clip("pick_up", 0.8).pose(0.0, STAND).pose(0.25, merge(pick, {"jaw": (-22, 0, 0)}))
     .pose(0.4, merge(pick, {"jaw": (-3, 0, 0)})).pose(0.8, merge(STAND, carry_head)))
    sit_up = merge(SIT, {"neck2": (6, 0, 0), "head": (8, 0, 0)})
    down = merge(STAND, {"neck2": (-14, 0, 0), "head": (-14, 0, 0)})
    (clip("drop_wait", 1.3).pose(0.0, merge(STAND, carry_head))
     .pose(0.35, merge(down, {"jaw": (-4, 0, 0)})).pose(0.5, merge(down, {"jaw": (-22, 0, 0)}))
     .pose(0.9, merge(sit_up, {"jaw": (-8, 0, 0)})).pose(1.3, merge(sit_up, {"jaw": (-6, 0, 0)})))
    leap = merge(WINGS_HALF, {"arm_up*": (26, 0, 0), "arm_lo*": (-44, 0, 0), "neck2": (14, 0, 0), "head": (12, 0, 0)})
    (clip("leap_catch", 0.9).pose(0.0, STAND).pose(0.15, CROUCH).pose(0.32, merge(leap, {"jaw": (-24, 0, 0)}))
     .pose(0.45, merge(leap, {"jaw": (-3, 0, 0)})).pose(0.7, CROUCH).pose(0.9, merge(STAND, carry_head))
     .root(0.0).root(0.15).root(0.4, up=0.32).root(0.7).root(0.9).event(0.7, "land"))
    kick = merge(SIT, {"head": (6, 8, 14), "leg_up_R": (30, 0, 0), "leg_lo_R": (20, 0, 0)})
    (clip("leg_kick", 1.4).pose(0.0, STAND).pose(0.3, kick).pose(1.15, kick).pose(1.4, STAND)
     .wave(lambda t: {"leg_up_R": (18 * sin01(t, 0.18) if 0.3 < t < 1.15 else 0.0, 0, 0)})
     .event(0.35, "thump").event(0.53, "thump").event(0.71, "thump").event(0.89, "thump"))
    (clip("sniff_refuse", 1.3).pose(0.0, STAND).pose(0.35, SNIFF)
     .pose(0.75, merge(STAND, {"neck2": (4, 20, 0), "neck3": (2, 16, 0), "head": (10, 24, -8)}))
     .pose(1.3, STAND).wave(sniffing(3.0, 0.2, 0.2, 0.6)).event(0.35, "sniff").event(0.8, "whimper"))
    wing_r_up = dict(STAND, **{k: v for k, v in WINGS_HALF.items() if k.endswith("_R")})
    (clip("lift_wing", 1.4).pose(0.0, STAND).pose(0.35, merge(wing_r_up, {"chest": (0, 0, 6)}))
     .pose(1.05, merge(wing_r_up, {"chest": (0, 0, 6)})).pose(1.4, STAND).event(0.35, "flap"))
    (clip("sneeze", 0.8).pose(0.0, STAND)
     .pose(0.28, merge(STAND, {"neck2": (6, 0, 0), "head": (14, 0, 0), "jaw": (-8, 0, 0)}))
     .pose(0.42, merge(STAND, {"neck2": (-6, 0, 0), "head": (-16, 0, 0), "jaw": (-4, 0, 0)}))
     .pose(0.8, STAND).event(0.4, "sneeze"))
    # Batting at a toy: a scrape of the big digging claws.
    bow2 = merge(STAND, {"hips": (-4, 0, 0), "arm_up*": (26, 0, 0), "arm_lo*": (-44, 0, 0), "hand*": (18, 0, 0),
                         "chest": (-8, 0, 0), "neck2": (-6, 0, 0), "head": (8, 0, 0)})
    swat = merge(bow2, {"arm_up_L": (-56, 12, 0), "arm_lo_L": (-20, 0, 0), "hand_L": (40, 0, 0), "head": (12, 8, -10),
                        "jaw": (-12, 0, 0)})
    (clip("paw_bat", 0.8).pose(0.0, bow2).pose(0.24, swat).pose(0.42, swat).pose(0.8, bow2).event(0.24, "squeak"))
    tug = merge(STAND, {"hips": (6, 0, 0), "leg_up*": (18, 0, 0), "leg_lo*": (-28, 0, 0), "foot*": (10, 0, 0),
                        "arm_up*": (-10, 0, 0), "arm_lo*": (-18, 0, 0), "chest": (-6, 0, 0),
                        "neck2": (-24, 0, 0), "head": (6, 0, 0), "jaw": (-5, 0, 0)})
    (clip("tug", 1.0, loop=True).pose(0.0, tug)
     .wave(lambda t: {"neck2": (0, 12 * sin01(t, 0.5), 0), "head": (0, 6 * sin01(t, 0.5, 0.2), 8 * sin01(t, 0.5, 0.15)),
                      **{f"tail{k}": (0, 12 * sin01(t, 0.5, -0.08 * k), 0) for k in range(1, 5)}}))
    return clips


def clips():
    out = build()
    clipkit.check(out, BONE_ORDER, NAME)
    return out
