"""The Puffback's body plan: a round, heavy four-legged dragon with short stout legs, a short
thick neck and tail, and a pair of small leaf-like wings (too small for it: they buzz).
The classic skeleton for the body (26 bones: the same names, so the look-at, the feet and
the rider work as everywhere) and a short five-bone wing per side (arm, forearm, three
fingers): 36 bones.

Its clips are its own (the classic dragon's timings and meanings, made for this body):
slow, heavy and bouncy. It walks with a rolling waddle, runs at a trundle (a bouncing,
rocking trot), plops down onto its rump to sit with a wobble of its belly, flops onto its
belly to lie and naps with its chin on the floor; it rolls onto its side for a belly rub
(never onto its back: that is where its garden grows). In the air its little wings buzz
fast while the body stays level. The wings fold flat along the upper flank like a shut fan;
wing poses are solved from target directions (wing_pose) rather than guessed Euler keys.
The baby is a bun (head and body one round shape): bending its neck would fold the bun
over, so its "_h" versions (eating, picking up, lying, sleeping, sulking) tip the whole body
and only nod the head.
"""
import math

from dragons import clipkit
from dragons.clipkit import Clip
from eca import q_from_pyr

NAME = "puffback"

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

# The small wing: an arm and forearm, then three short fingers fanning a rounded, leaf-like
# membrane. Laid out in its own plane (u out along the span, v back along the chord), raised
# by WING_DIHEDRAL and drooping by WING_DROOP; the kind's forms use these (and a scale).
WING_LAYOUT = {"root": (0.0, 0.0), "elbow": (0.55, 0.10), "wrist": (1.0, -0.04),
               "f1": (1.78, 0.16), "f2": (1.62, 0.74), "f3": (1.12, 1.12),
               "body": (-0.05, -0.30)}
WING_DIHEDRAL = 40.0
WING_DROOP = 8.0
WING_CHAIN = [("wing_arm", "root", "elbow", "chest"), ("wing_fore", "elbow", "wrist", "wing_arm"),
              ("wing_f1", "wrist", "f1", "wing_fore"), ("wing_f2", "wrist", "f2", "wing_fore"),
              ("wing_f3", "wrist", "f3", "wing_fore")]
WING_BONES = [f"{n}_{side}" for side in ("L", "R") for n, *_ in WING_CHAIN]
BONE_ORDER = [b[0] for b in BONES] + WING_BONES
WING_BODY = ("chest",)  # the arm's stub is rooted in the chest; the membrane is free of the flank
CONTACTS = ("hand_L", "hand_R", "foot_L", "foot_R")
SEAT = ("belly", (0.0, -0.2, 1.4))  # in the moss saddle, a gap in the plates mid-back (kinds/puffback.py)
REGION = {}


# ------------------------------------------------------------------------------ wing poses
# Wing poses are solved from where each bone should point (in the dragon's space: +X its
# right, +Y back, +Z up) and which way the wing's face should look, instead of guessing
# Euler keys: a bone's key turns with its parents (eca.py conventions).
def _norm(v):
    n = math.sqrt(sum(c * c for c in v)) or 1.0
    return tuple(c / n for c in v)


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def _frame(d, n):
    """Rotation matrix (rows) whose columns are d, n x d, n (n made perpendicular to d)."""
    d = _norm(d)
    n = _norm(tuple(nc - dc * _dot(n, d) for nc, dc in zip(n, d)))
    s = _cross(n, d)
    return [[d[i], s[i], n[i]] for i in range(3)]


def _mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]


def _t(a):
    return [[a[j][i] for j in range(3)] for i in range(3)]


def _pyr(m):
    """(pitch, yaw, roll) degrees of a rotation matrix, as eca.q_from_pyr builds them."""
    pitch = -math.degrees(math.asin(max(-1.0, min(1.0, m[2][1]))))
    yaw = math.degrees(math.atan2(-m[0][1], m[1][1]))
    roll = math.degrees(math.atan2(m[2][0], m[2][2]))
    return (round(pitch, 2), round(yaw, 2), round(roll, 2))


def wing_rest():
    """The right wing's rest: {bone: (direction, face normal)} in the dragon's space."""
    th, ph = math.radians(WING_DIHEDRAL), math.radians(WING_DROOP)
    span, chord = (math.cos(th), 0.0, math.sin(th)), (0.0, math.cos(ph), -math.sin(ph))
    normal = _norm(_cross(span, chord))
    if normal[0] < 0:
        normal = tuple(-c for c in normal)

    def at(name):
        u, v = WING_LAYOUT[name]
        return tuple(s * u + c * v for s, c in zip(span, chord))

    out = {}
    for name, h, t, _ in WING_CHAIN:
        out[name] = (_norm(tuple(b - a for a, b in zip(at(h), at(t)))), normal)
    return out


def wing_pose(targets):
    """Keys for a wing pose: targets {bone: (direction, face normal)} for the right wing (the
    left mirrors). Returns {"<bone>*": (pitch, yaw, roll)} for Clip.pose()."""
    rest = wing_rest()
    world = {}
    keys = {}
    for name, h, t, parent in WING_CHAIN:
        d, n = targets.get(name, rest[name])
        rd, rn = rest[name]
        world[name] = _mul(_frame(d, n), _t(_frame(rd, rn)))
        par = world.get(parent)
        local = _mul(_t(par), world[name]) if par else world[name]
        keys[f"{name}*"] = _pyr(local)
    return keys


# Folded: the arm lies back from the shoulder, the forearm on back along the upper flank
# (under the garden) and the three fingers closed along it like a shut fan, the wing's face
# flat against the body. The targets were found by walking along the grown body's skin from
# the seated wing root (10 degrees below straight back, the fingers 16 degrees apart, a bone's
# radius off the skin); the baby's buds turn in a little more by its form's base_pose.
def _clear(d, k=0.07):
    """A folded bone's direction turned out from the body by about 4 degrees: a bone's radius
    off the skin clipped as the waddle rolled the flank under the wings (run 15)."""
    return _norm((d[0] + k, d[1], d[2]))


WINGS_FOLDED = wing_pose({
    "wing_arm": (_clear((0.426, 0.900, 0.095)), (0.816, -0.182, 0.548)),
    "wing_fore": (_clear((0.112, 0.985, -0.132)), (0.876, -0.043, 0.481)),
    "wing_f1": (_clear((-0.123, 0.985, -0.119)), (0.897, 0.049, 0.440)),
    "wing_f2": (_clear((0.014, 0.945, -0.327)), (0.897, 0.049, 0.440)),
    "wing_f3": (_clear((0.137, 0.822, -0.553)), (0.938, 0.014, 0.345)),
})


def wing_blend(pose, k):
    """The wing pose k of the way from the rest (open, k = 0) to `pose` (k = 1), bone by bone."""
    out = {}
    for bone, v in pose.items():
        w, x, y, z = q_from_pyr(*v)
        ang = 2 * math.acos(max(-1.0, min(1.0, w)))
        sn = math.sin(ang / 2)
        if sn < 1e-6:
            out[bone] = (0.0, 0.0, 0.0)
            continue
        ax = (x / sn, y / sn, z / sn)
        h = ang * k / 2
        w, x, y, z = math.cos(h), ax[0] * math.sin(h), ax[1] * math.sin(h), ax[2] * math.sin(h)
        m = [[1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y)],
             [2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x)],
             [2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)]]
        out[bone] = _pyr(m)
    return out


FOLD = WINGS_FOLDED
HALF = wing_blend(WINGS_FOLDED, 0.5)                    # lifted off the flank, half open
OPEN = {"wing_arm*": (0, -6, 6)}                        # spread (the rest), a little wider
FLY_WINGS = {"wing_arm*": (0, 0, -24)}                  # spread out flat for flying
R_HALF = {n[:-1] + "_R": v for n, v in HALF.items()}  # the right wing only (grooming)


# ------------------------------------------------------------------------------ helpers
def merge(*poses):
    out = {}
    for p in poses:
        for bone, v in p.items():
            a = out.get(bone, (0.0, 0.0, 0.0))
            out[bone] = tuple(x + y for x, y in zip(a, v))
    return out


def sin01(t, period, phase=0.0):
    return math.sin(2 * math.pi * (t / period + phase))


def window(t, t0, t1, ramp=0.15):
    """1 between t0 and t1, easing in and out over `ramp` seconds."""
    if t <= t0 or t >= t1:
        return 0.0
    return min(1.0, (t - t0) / ramp, (t1 - t) / ramp)


def breathe(amount=1.0, period=4.0):
    """Slow, deep breaths: the big chest and belly rise and fall."""
    def fn(t):
        s = sin01(t, period) * amount
        return {"chest": (1.5 * s, 0, 0), "belly": (-0.9 * s, 0, 0), "neck1": (0.5 * s, 0, 0),
                "head": (-0.7 * s, 0, 0), "wing_arm_R": (0, 0, 1.2 * s), "wing_arm_L": (0, 0, -1.2 * s)}
    return fn


def tail_sway(amount=1.0, period=4.0):
    """The short heavy tail swings slowly, its club lagging."""
    return lambda t: {f"tail{k}": (0, amount * (1.5 + 1.5 * k) * sin01(t, period, -0.1 * k), 0) for k in range(1, 5)}


def wag(amount, period, t0=None, t1=None):
    """A happy wag: the short tail and the whole rump swing, the club flicking last."""
    def fn(t):
        e = 1.0 if t0 is None else window(t, t0, t1)
        s = sin01(t, period)
        out = {"hips": (0, 3 * amount * s * e, 0), "belly": (0, -2 * amount * s * e, 0)}
        for k in range(1, 5):
            out[f"tail{k}"] = (0, amount * (6 + 3 * k) * sin01(t, period, -0.08 * k) * e, 0)
        return out
    return fn


def legs(period, swing, bend, phases):
    """Short stout legs stepping: each swings (pitch) round its phase and lifts its foot a
    little on the forward swing."""
    def fn(t):
        out = {}
        for upper, ph in phases.items():
            side = upper[-2:]
            front = upper.startswith("arm")
            s = sin01(t, period, -ph)
            lift = max(0.0, math.cos(2 * math.pi * (t / period - ph)))
            lower, tip = ("arm_lo", "hand") if front else ("leg_lo", "foot")
            out[upper] = (swing * s, 0, 0)
            out[lower + side] = (-bend * lift, 0, 0)
            out[tip + side] = ((-0.5 if front else 0.45) * bend * lift, 0, 0)
        return out
    return fn


def waddle(period, roll=4.0, bob=2.0, sway=2.0, phase=0.125):
    """The heavy body rocks onto each side in turn as it steps (the weight over the planted
    feet), the head bobbing down on every footfall and staying level."""
    def fn(t):
        r = -roll * math.cos(2 * math.pi * (t / period - phase))
        b = sin01(t, period / 2)
        y = sway * sin01(t, period, 0.25 - phase)
        return {"hips": (0, y, r), "chest": (bob * 0.3 * b, -0.5 * y, -0.35 * r),
                "belly": (-0.8 * bob * sin01(t, period / 2, -0.15), 0, 0),
                "neck1": (-bob * b, -0.3 * y, 0.4 * r), "head": (bob * 0.7 * b, -0.2 * y, 0.5 * r)}
    return fn


def buzz(period, amount, t0=None, t1=None, sweep=0.0):
    """The little wings buzzing: fast full strokes, the forearm and fingers lagging."""
    def fn(t):
        e = 1.0 if t0 is None else window(t, t0, t1, 0.08)
        s = sin01(t, period) * e
        h = sin01(t, period, -0.15) * e
        return {"wing_arm_R": (0, sweep * s, amount * s), "wing_arm_L": (0, -sweep * s, -amount * s),
                "wing_fore_R": (0, 0, 0.3 * amount * h), "wing_fore_L": (0, 0, -0.3 * amount * h),
                "wing_f1_R": (0, 0, 0.15 * amount * h), "wing_f1_L": (0, 0, -0.15 * amount * h)}
    return fn


def jiggle(t0, amount=4.0, period=0.3, decay=0.35, bone="belly"):
    """A heavy landing: the belly (and the body on it) wobbles and settles."""
    def fn(t):
        if t < t0:
            return {}
        k = math.exp(-(t - t0) / decay) * math.sin(2 * math.pi * (t - t0) / period)
        return {bone: (amount * k, 0, 0), "head": (-0.6 * amount * k, 0, 0)}
    return fn


def chomp(amount=18.0, period=0.6, at=0.15):
    def fn(t):
        u = ((t - at) / period) % 1.0
        return {"jaw": (-amount * math.sin(math.pi * (u - 0.5) / 0.5) ** 0.7 if u > 0.5 else 0.0, 0, 0)}
    return fn


def pant(amount=6.0, period=0.4):
    return lambda t: {"jaw": (-amount - 2.5 * sin01(t, period), 0, 0)}


def footsteps(c, period, phases, cycles=1):
    for k in range(cycles):
        for ph in phases.values():
            c.event((ph + 0.25 + k) * period % (period * cycles), "footstep")


WALK_PHASES = {"leg_up_L": 0.0, "arm_up_L": 0.25, "leg_up_R": 0.5, "arm_up_R": 0.75}  # lateral 4-beat
TROT_PHASES = {"arm_up_L": 0.0, "leg_up_R": 0.0, "arm_up_R": 0.5, "leg_up_L": 0.5}    # diagonal pairs
BOUND_PHASES = {"leg_up_L": 0.0, "leg_up_R": 0.04, "arm_up_L": 0.5, "arm_up_R": 0.54}  # a bouncing bun

# ------------------------------------------------------------------------------ poses
# Degree deltas on the idle pose (eca.py): pitch + lifts a forward bone's tip (the hips' pitch
# + rocks the whole body back onto its rump) and swings a leg forward; roll + on a leg splays
# its foot out.
SIT = {  # plopped back on its rump, the front legs straight, the belly out
    "hips": (30, 0, 0), "leg_up*": (46, 0, 8), "leg_lo*": (-52, 0, 0), "foot*": (10, 0, 0),
    "arm_up*": (-28, 0, 2), "arm_lo*": (2, 0, 0), "hand*": (-2, 0, 0),
    "tail1": (-16, 0, 0), "tail2": (-4, 10, 0), "tail3": (0, 14, 0), "tail4": (0, 16, 0),
    "neck1": (-12, 0, 0), "neck2": (-4, 0, 0), "head": (-10, 0, 0)}
LIE = {  # flopped on its belly, front paws out in front, hind legs stretched back (a sploot)
    "arm_up*": (62, 0, 10), "arm_lo*": (-26, 0, 0), "hand*": (18, 0, 0),
    "leg_up*": (-72, 0, 12), "leg_lo*": (8, 0, 0), "foot*": (-42, 0, 0),
    "neck1": (-10, 0, 0), "neck2": (-3, 0, 0), "head": (-4, 0, 0),
    "tail1": (8, 0, 0), "tail2": (0, 8, 0), "tail3": (0, 10, 0), "tail4": (0, 12, 0)}
SLEEP = merge(LIE, {  # its chin on the floor, head turned a little, the tail curled round
    "neck1": (-22, 6, 0), "neck2": (-10, 6, 0), "neck3": (-4, 4, 0), "head": (-12, 8, 14),
    "tail1": (0, 10, 0), "tail2": (0, 14, 0), "tail3": (0, 16, 0), "tail4": (0, 18, 0)})
CROUCH = {  # settling its weight, about to push off
    "hips": (-3, 0, 0), "leg_up*": (24, 0, 0), "leg_lo*": (-42, 0, 0), "foot*": (18, 0, 0),
    "arm_up*": (24, 0, 0), "arm_lo*": (-42, 0, 0), "hand*": (18, 0, 0),
    "neck1": (-10, 0, 0), "head": (6, 0, 0), "tail1": (6, 0, 0)}
AIR = {  # up in the air: legs dangling, a little stretched
    "arm_up*": (22, 0, 4), "arm_lo*": (-24, 0, 0), "leg_up*": (-18, 0, 4), "leg_lo*": (10, 0, 0),
    "neck1": (8, 0, 0), "head": (8, 0, 0), "tail1": (-10, 0, 0)}
EAT = {  # the front crouched so its short neck reaches the bowl
    "hips": (-4, 0, 0), "chest": (-10, 0, 0), "arm_up*": (30, 0, 8), "arm_lo*": (-50, 0, 0), "hand*": (22, 0, 0),
    "leg_up*": (4, 0, 0), "neck1": (-36, 0, 0), "neck2": (-22, 0, 0), "neck3": (-12, 0, 0), "head": (-14, 0, 0)}
SIDE = {  # rolled onto its side, belly up to be rubbed, legs in the air, head kept upright
    "hips": (0, 0, 74), "chest": (0, 0, 6), "arm_up*": (30, 0, -10), "arm_lo*": (-40, 0, 0), "hand*": (20, 0, 0),
    "leg_up*": (26, 0, -10), "leg_lo*": (-40, 0, 0), "foot*": (20, 0, 0),
    "neck1": (8, 0, -26), "neck2": (0, 0, -22), "neck3": (0, 0, -12), "head": (10, 0, -10),
    "tail1": (0, 18, 0), "tail2": (0, 14, 0)}
PLAY_BOW = {  # front down on its elbows, rump and tail up
    "hips": (-16, 0, 0), "chest": (-6, 0, 0), "arm_up*": (46, 0, 10), "arm_lo*": (-70, 0, 0), "hand*": (32, 0, 0),
    "leg_up*": (14, 0, 0), "leg_lo*": (-4, 0, 0), "neck1": (24, 0, 0), "neck2": (8, 0, 0), "head": (-8, 0, 0),
    "tail1": (-26, 0, 0), "tail2": (-10, 0, 0), "jaw": (-12, 0, 0)}
LOW_BOW = {  # a gentler bow, ready to swat
    "hips": (-5, 0, 0), "chest": (-8, 0, 0), "arm_up*": (28, 0, 0), "arm_lo*": (-46, 0, 0), "hand*": (18, 0, 0),
    "neck1": (-4, 0, 0), "head": (8, 0, 0), "tail1": (-10, 0, 0)}
SPAR = {  # reared up on its haunches like a bear, front paws up
    "hips": (26, 0, 0), "chest": (6, 0, 0), "leg_up*": (-6, 0, 6), "leg_lo*": (-30, 0, 0), "foot*": (24, 0, 0),
    "neck1": (-10, 0, 0), "head": (-12, 0, 0), "tail1": (-20, 0, 0), "tail2": (-6, 0, 0)}
STALK = merge(CROUCH, {"neck1": (-14, 0, 0), "neck2": (-8, 0, 0), "head": (6, 0, 0), "hips": (-2, 0, 0)})
SULK = merge(LIE, {  # flopped, head turned away and down, tail swept round the other way
    "neck1": (-20, -16, 0), "neck2": (-10, -12, 0), "head": (-22, -12, -10),
    "tail1": (0, -18, 0), "tail2": (0, -22, 0), "tail3": (0, -22, 0), "tail4": (0, -20, 0)})
TAIL_CHASE = {  # curled round to one side after its own club
    "neck1": (0, 28, 0), "neck2": (0, 24, 0), "neck3": (0, 18, 0), "head": (-8, 16, 10),
    "chest": (0, 14, 0), "hips": (0, 12, 0),
    "tail1": (0, 30, 0), "tail2": (0, 32, 0), "tail3": (0, 30, 0), "tail4": (0, 26, 0), "jaw": (-14, 0, 0)}
TUG = {  # braced back on its hind legs, head low and pulling
    "hips": (8, 0, 0), "leg_up*": (22, 0, 0), "leg_lo*": (-34, 0, 0), "foot*": (12, 0, 0),
    "arm_up*": (-14, 0, 0), "arm_lo*": (-16, 0, 0), "hand*": (10, 0, 0),
    "chest": (-6, 0, 0), "neck1": (-26, 0, 0), "neck2": (-10, 0, 0), "head": (6, 0, 0), "jaw": (-5, 0, 0),
    "tail1": (14, 0, 0)}
PICK = {  # head down to the floor for a toy
    "chest": (-8, 0, 0), "arm_up*": (22, 0, 6), "arm_lo*": (-36, 0, 0), "hand*": (14, 0, 0),
    "neck1": (-34, 0, 0), "neck2": (-20, 0, 0), "neck3": (-10, 0, 0), "head": (-18, 0, 0)}
CARRY_HEAD = {"neck1": (12, 0, 0), "head": (-8, 0, 0)}  # head up, the toy held in front
SIT_UP = merge(SIT, {"neck1": (8, 0, 0), "head": (12, 0, 0)})  # sitting, looking up at you
EAT_H = {  # the bun tips forward over its front paws and nods into the bowl
    "hips": (-16, 0, 0), "arm_up*": (30, 0, 8), "arm_lo*": (-48, 0, 0), "hand*": (22, 0, 0),
    "leg_up*": (16, 0, 0), "foot*": (-4, 0, 0), "neck1": (-4, 0, 0), "head": (-12, 0, 0)}
LIE_H = {  # the bun settles flat: little paws forward, hind feet tucked back (no splay: it would crumple)
    "arm_up*": (40, 0, 4), "arm_lo*": (-18, 0, 0), "hand*": (10, 0, 0),
    "leg_up*": (-44, 0, 4), "leg_lo*": (6, 0, 0), "foot*": (-24, 0, 0),
    "neck1": (-4, 0, 0), "head": (-2, 0, 0),
    "tail1": (8, 0, 0), "tail2": (0, 8, 0), "tail3": (0, 10, 0), "tail4": (0, 12, 0)}
SLEEP_H = merge(LIE_H, {  # flopped, a little nod of the head, tilted, the tail curled round
    "neck1": (-4, 4, 0), "neck2": (-2, 4, 0), "head": (-10, 6, 14),
    "tail1": (0, 10, 0), "tail2": (0, 14, 0), "tail3": (0, 16, 0), "tail4": (0, 18, 0)})
SULK_H = merge(LIE_H, {  # flopped and turned away, head down a little
    "neck1": (-4, -12, 0), "neck2": (-2, -10, 0), "head": (-12, -10, -10),
    "tail1": (0, -18, 0), "tail2": (0, -22, 0), "tail3": (0, -22, 0), "tail4": (0, -20, 0)})
FLY_BODY = {  # level in the air, the short legs hanging back, tail out behind
    "arm_up*": (-26, 0, 6), "arm_lo*": (22, 0, 0), "hand*": (-12, 0, 0),
    "leg_up*": (-34, 0, 6), "leg_lo*": (22, 0, 0), "foot*": (-24, 0, 0),
    "neck1": (-8, 0, 0), "neck2": (-2, 0, 0), "head": (8, 0, 0), "tail1": (-10, 0, 0), "tail2": (-4, 0, 0)}


def clip_set():
    C = []

    def clip(*a, **k):
        c = Clip(*a, **k)
        C.append(c)
        return c

    # ------------------------------------------------------------------ idle & looking
    clip("idle", 4.0, loop=True).pose(0.0, FOLD).wave(breathe()).wave(tail_sway(1.0, 4.0))
    (clip("look_around", 3.4).pose(0.0, FOLD)
     .key(0.0, neck2=(0, 0, 0), neck3=(0, 0, 0), head=(0, 0, 0))
     .key(1.0, neck2=(3, 14, 0), neck3=(2, 12, 0), head=(5, 12, 7))
     .key(1.7, neck2=(3, 14, 0), neck3=(2, 12, 0), head=(5, 12, 7))
     .key(2.6, neck2=(2, -16, 0), neck3=(0, -14, 0), head=(4, -12, -7))
     .key(3.4, neck2=(0, 0, 0), neck3=(0, 0, 0), head=(0, 0, 0))
     .wave(breathe()).wave(tail_sway()).event(1.0, "sniff"))
    scratch = merge(FOLD, {"leg_up_R": (58, -14, 0), "leg_lo_R": (-30, 0, 0), "foot_R": (16, 0, 0), "hips": (6, 0, -8),
                           "neck1": (-6, -22, 0), "neck2": (0, -18, 0), "head": (-6, -10, -18)})
    (clip("scratch", 2.3).pose(0.0, FOLD).pose(0.5, scratch).pose(1.8, scratch).pose(2.3, FOLD)
     .wave(lambda t: {"leg_lo_R": (9 * sin01(t, 0.2), 0, 0), "foot_R": (12 * sin01(t, 0.2), 0, 0)}
           if 0.6 < t < 1.7 else {}))

    # ------------------------------------------------------------------ on foot
    # Slow, heavy steps with a rolling waddle; the den measures the speed from the feet.
    walk = clip("walk", 1.2, loop=True, speed=0.45).pose(0.0, FOLD)
    walk.wave(legs(1.2, 18, 24, WALK_PHASES)).wave(waddle(1.2, 4.0, 2.2, 2.5)).wave(tail_sway(1.6, 1.2))
    for k in range(8):
        walk.root(0.15 * k, up=0.015 * (k % 2))
    footsteps(walk, 1.2, WALK_PHASES)
    trot = clip("trot", 0.72, loop=True, speed=1.2).pose(0.0, FOLD)
    trot.wave(legs(0.72, 22, 34, TROT_PHASES)).wave(waddle(0.72, 5.0, 2.6, 2.0, 0.0)).wave(tail_sway(1.8, 0.72))
    trot.wave(pant(4.0, 0.36))
    for k in range(4):
        trot.root(0.18 * k, up=0.03 * (k % 2))
    footsteps(trot, 0.72, {"a": 0.0, "b": 0.5})
    # The run: a trundle, a rocking, bouncing trot with the belly bobbing and the little
    # wings lifting with excitement.
    gallop = clip("gallop", 0.52, loop=True, speed=2.4).pose(
        0.0, merge(HALF, {"neck1": (-8, 0, 0), "head": (8, 0, 0), "tail1": (-6, 0, 0)}))
    gallop.wave(legs(0.52, 28, 44, TROT_PHASES)).wave(waddle(0.52, 6.5, 3.5, 2.5, 0.0)).wave(tail_sway(1.6, 0.52))
    gallop.wave(pant(8.0, 0.26)).wave(buzz(0.13, 10))
    for k in range(4):
        gallop.root(0.13 * k, up=0.07 * (k % 2))
    footsteps(gallop, 0.52, {"a": 0.0, "b": 0.5})
    # A baby's run: a bouncing bun, hopping along on its little legs.
    scamper = clip("scamper", 0.4, loop=True, speed=2.0).pose(
        0.0, merge(FOLD, {"neck1": (-4, 0, 0), "head": (6, 0, 0), "tail1": (-8, 0, 0)}))
    scamper.wave(legs(0.4, 34, 44, BOUND_PHASES)).wave(pant(10.0, 0.2)).wave(tail_sway(1.4, 0.4))
    scamper.wave(lambda t: {"hips": (5 * sin01(t, 0.4, 0.1), 0, 0), "chest": (-4 * sin01(t, 0.4, 0.15), 0, 0)})
    scamper.root(0.0, up=0.0).root(0.12, up=0.16).root(0.24, up=0.04).root(0.32, up=0.0)
    footsteps(scamper, 0.4, {"hind": 0.0, "front": 0.5})
    shuffle = clip("shuffle", 1.2, loop=True).pose(0.0, FOLD)
    shuffle.wave(legs(1.2, 9, 18, WALK_PHASES)).wave(waddle(1.2, 2.5, 1.0, 1.0))
    footsteps(shuffle, 1.2, WALK_PHASES)
    carry = clip("carry", 1.2, loop=True, speed=0.45).pose(0.0, merge(FOLD, CARRY_HEAD))
    carry.wave(legs(1.2, 18, 24, WALK_PHASES)).wave(waddle(1.2, 3.5, 1.2, 2.0)).wave(tail_sway(2.2, 0.6))
    footsteps(carry, 1.2, WALK_PHASES)
    stalk = clip("stalk", 1.6, loop=True, speed=0.2).pose(0.0, merge(FOLD, STALK))
    stalk.wave(legs(1.6, 12, 20, WALK_PHASES)).wave(waddle(1.6, 2.0, 0.6, 1.0))
    stalk.wave(lambda t: {"tail4": (0, 16 * sin01(t, 0.4), 0), "tail3": (0, 6 * sin01(t, 0.4, -0.1), 0),
                          "hips": (0, 3 * sin01(t, 0.8), 0)})

    # ------------------------------------------------------------------ rest
    # Plopping down: a heavy drop onto the rump and a wobble of the belly as it settles.
    (clip("sit", 0.9).pose(0.0, FOLD).pose(0.3, merge(FOLD, CROUCH))
     .pose(0.55, merge(FOLD, SIT, {"hips": (6, 0, 0), "chest": (-6, 0, 0), "head": (-4, 0, 0)}))
     .pose(0.9, merge(FOLD, SIT)).wave(jiggle(0.55, 3.0, 0.22, 0.2)).event(0.55, "thump"))
    clip("sit_loop", 4.0, loop=True).pose(0.0, merge(FOLD, SIT)).wave(breathe(1.2)).wave(tail_sway(0.5))
    (clip("lie_down", 1.2).pose(0.0, FOLD).pose(0.45, merge(FOLD, CROUCH))
     .pose(0.85, merge(FOLD, LIE, {"chest": (-4, 0, 0), "head": (-6, 0, 0)})).pose(1.2, merge(FOLD, LIE))
     .wave(jiggle(0.85, 3.0, 0.24, 0.22)).event(0.85, "thump"))
    clip("lie_loop", 4.4, loop=True).pose(0.0, merge(FOLD, LIE)).wave(breathe(1.4, 4.4)).wave(tail_sway(0.4, 4.4))
    clip("curl_up", 1.6).pose(0.0, merge(FOLD, LIE)).pose(1.6, merge(FOLD, SLEEP))
    clip("sleep", 5.2, loop=True).pose(0.0, merge(FOLD, SLEEP)).wave(breathe(2.2, 5.2))
    (clip("wake", 2.8).pose(0.0, merge(FOLD, SLEEP)).pose(0.7, merge(FOLD, LIE))
     .pose(1.4, merge(FOLD, LIE, {"neck1": (16, 0, 0), "head": (24, 0, 0), "snout": (6, 0, 0), "jaw": (-34, 0, 0),
                                  "chest": (6, 0, 0)}))
     .pose(2.1, merge(HALF, CROUCH, {"neck1": (8, 0, 0)}))
     .pose(2.8, FOLD).wave(buzz(0.14, 18, 1.9, 2.4)).event(1.4, "yawn"))
    (clip("yawn", 1.8).pose(0.0, FOLD)
     .pose(0.7, merge(FOLD, {"neck1": (14, 0, 0), "neck2": (8, 0, 0), "head": (28, 0, 0), "snout": (6, 0, 0),
                             "jaw": (-34, 0, 0), "chest": (5, 0, 0)}))
     .pose(1.25, merge(FOLD, {"neck1": (14, 0, 0), "neck2": (8, 0, 0), "head": (30, 0, 0), "snout": (6, 0, 0),
                              "jaw": (-38, 0, 0), "chest": (5, 0, 0)}))
     .pose(1.8, FOLD).event(0.7, "yawn"))
    # Flopping: it just lets go and lands on its belly, wherever it is.
    (clip("nap_flop", 0.8).pose(0.0, FOLD).pose(0.3, merge(FOLD, CROUCH, {"head": (8, 0, 0)}))
     .pose(0.55, merge(FOLD, LIE, {"chest": (-6, 0, 0), "head": (-8, 0, 0)})).pose(0.8, merge(FOLD, LIE))
     .wave(jiggle(0.55, 4.0, 0.22, 0.2)).event(0.55, "thump"))

    # ------------------------------------------------------------------ care
    eat = clip("eat", 1.4, loop=True).pose(0.0, merge(FOLD, EAT))
    eat.wave(lambda t: {"head": (7 * max(0.0, sin01(t, 0.7)), 0, 0), "snout": (5 * max(0.0, sin01(t, 0.7)), 0, 0),
                        "neck1": (3 * sin01(t, 1.4), 0, 0)}).wave(chomp(18, 0.7))
    eat.event(0.15, "chomp").event(0.85, "chomp")
    (clip("fav_wiggle", 1.8).pose(0.0, FOLD)
     .pose(0.3, merge(HALF, {"neck1": (10, 0, 0), "head": (12, 0, 0), "jaw": (-16, 0, 0)}))
     .pose(1.5, merge(HALF, {"neck1": (10, 0, 0), "head": (12, 0, 0), "jaw": (-16, 0, 0)})).pose(1.8, FOLD)
     .wave(lambda t: {"hips": (0, 6 * sin01(t, 0.45), 3 * sin01(t, 0.45)), "chest": (0, -4 * sin01(t, 0.45), 0),
                      **{f"tail{k}": (0, 14 * sin01(t, 0.3, -0.1 * k), 0) for k in range(1, 5)}})
     .wave(buzz(0.14, 20, 0.35, 1.45))
     .root(0.0).root(0.6, up=0.0).root(0.8, up=0.18).root(1.0, up=0.0).root(1.8)
     .wave(jiggle(1.0, 3.0, 0.22, 0.2)).event(1.0, "land").event(0.3, "call"))
    (clip("pet_head", 1.8, loop=True)
     .pose(0.0, merge(FOLD, {"neck1": (4, 0, 0), "neck3": (4, 0, 6), "head": (-10, 6, 16), "jaw": (-6, 0, 0)}))
     .wave(lambda t: {"head": (0, 4 * sin01(t, 1.8), 4 * sin01(t, 1.8)), "neck2": (0, 3 * sin01(t, 1.8, 0.1), 0)})
     .wave(tail_sway(0.8, 0.9)).event(0.4, "purr"))
    (clip("pet_chin", 1.8, loop=True)
     .pose(0.0, merge(FOLD, {"neck1": (12, 0, 0), "neck2": (8, 0, 0), "head": (24, 0, 0), "jaw": (-8, 0, 0)}))
     .wave(lambda t: {"head": (4 * sin01(t, 1.8), 0, 0), "jaw": (-3 * sin01(t, 1.8, 0.2), 0, 0)})
     .wave(tail_sway(0.8, 0.9)).event(0.4, "purr"))
    # Rolling over: onto its side (never its back: its garden grows there), legs in the air.
    (clip("roll_over", 1.3).pose(0.0, FOLD).pose(0.45, merge(FOLD, LIE))
     .pose(1.0, merge(FOLD, SIDE, {"hips": (0, 0, 8)})).pose(1.3, merge(FOLD, SIDE))
     .wave(jiggle(1.0, 3.0, 0.24, 0.22)).event(0.95, "thump"))
    (clip("belly_rub", 1.4, loop=True).pose(0.0, merge(FOLD, SIDE))
     .wave(lambda t: {"leg_up_L": (12 * sin01(t, 0.7), 0, 0), "leg_up_R": (12 * sin01(t, 0.7, 0.5), 0, 0),
                      "arm_up_L": (10 * sin01(t, 0.7, 0.25), 0, 0), "arm_up_R": (10 * sin01(t, 0.7, 0.75), 0, 0),
                      "hips": (0, 0, 4 * sin01(t, 1.4)),
                      **{f"tail{k}": (0, 8 * sin01(t, 0.7, -0.1 * k), 0) for k in range(1, 5)}})
     .wave(pant(9.0, 0.7)).event(0.3, "purr"))

    def shake_wave(t):
        k = max(0.0, 1.0 - abs(t - 0.55) / 0.42)
        s = sin01(t, 0.14) * k
        return {"chest": (0, 0, 14 * s), "hips": (0, 0, -9 * s), "belly": (0, 0, 6 * s), "neck1": (0, 0, 12 * s),
                "head": (0, 0, 14 * s), "tail2": (0, 12 * s, 0), "wing_arm_R": (0, 0, 10 * s),
                "wing_arm_L": (0, 0, -10 * s)}

    clip("shake", 1.1).pose(0.0, FOLD).pose(1.1, FOLD).wave(shake_wave).event(0.45, "shake")
    # A heavy little hop: the feet hardly leave the ground, the wings buzz to help.
    (clip("hop", 1.0).pose(0.0, FOLD).pose(0.28, merge(FOLD, CROUCH)).pose(0.5, merge(OPEN, AIR))
     .pose(0.72, merge(HALF, CROUCH, {"chest": (-5, 0, 0)})).pose(1.0, FOLD)
     .wave(buzz(0.12, 30, 0.34, 0.7)).wave(jiggle(0.72, 3.0, 0.22, 0.2))
     .root(0.0).root(0.28).root(0.5, up=0.3).root(0.72).root(1.0).event(0.72, "land").event(0.38, "squeak"))

    # ------------------------------------------------------------------ play
    # Its pounce: a rump wiggle, a heavy hop, and a belly-flop landing.
    (clip("pounce", 1.7).pose(0.0, FOLD).pose(0.3, merge(FOLD, CROUCH)).pose(0.65, merge(FOLD, CROUCH))
     .pose(0.88, merge(OPEN, AIR, {"arm_up*": (40, 0, 6), "leg_up*": (-40, 0, 6), "jaw": (-20, 0, 0)}))
     .pose(1.08, merge(HALF, LIE, {"chest": (-6, 0, 0), "head": (-6, 0, 0), "jaw": (-10, 0, 0)}))
     .pose(1.35, merge(FOLD, LIE)).pose(1.7, FOLD)
     .wave(lambda t: {"hips": (0, 7 * sin01(t, 0.16), 0) if 0.3 < t < 0.65 else (0, 0, 0)})
     .wave(buzz(0.12, 34, 0.7, 1.05)).wave(jiggle(1.08, 4.0, 0.22, 0.22))
     .root(0.0).root(0.65).root(0.88, up=0.38).root(1.08).root(1.7)
     .event(0.78, "squeak").event(1.08, "thump"))
    (clip("play_bow", 1.3).pose(0.0, FOLD).pose(0.35, merge(HALF, PLAY_BOW)).pose(1.0, merge(HALF, PLAY_BOW))
     .pose(1.3, FOLD).wave(wag(1.2, 0.3, 0.3, 1.05)).event(0.35, "call"))

    def spar_wave(t):
        s, c = sin01(t, 0.6), sin01(t, 0.6, 0.25)
        return {"arm_up_L": (44 + 26 * s, 0, 10), "arm_lo_L": (-40 - 20 * s, 0, 0), "hand_L": (-16 * s, 0, 0),
                "arm_up_R": (44 - 26 * s, 0, -10), "arm_lo_R": (-40 + 20 * s, 0, 0), "hand_R": (16 * s, 0, 0),
                "head": (5 * c, 7 * s, 0), "neck2": (0, 5 * s, 0), "jaw": (-12 - 5 * c, 0, 0),
                **{f"tail{k}": (0, 12 * sin01(t, 0.6, -0.1 * k), 0) for k in range(1, 5)}}

    (clip("spar", 1.2, loop=True).pose(0.0, merge(HALF, SPAR)).wave(spar_wave).wave(buzz(0.15, 12))
     .root(0.0, up=0.0).root(0.3, up=0.04).root(0.6, up=0.0).root(0.9, up=0.04)
     .event(0.1, "squeak").event(0.7, "thump"))
    tail_chase = clip("tail_chase", 0.8, loop=True).pose(0.0, merge(HALF, TAIL_CHASE))
    tail_chase.wave(legs(0.8, 20, 30, TROT_PHASES)).wave(pant(6.0, 0.4)).wave(buzz(0.16, 10))
    tail_chase.root(0.0, up=0.0).root(0.2, up=0.08).root(0.4, up=0.0).root(0.6, up=0.08)
    footsteps(tail_chase, 0.8, {"a": 0.0, "b": 0.5})

    def happy_wag(t):
        """The whole rump swings with the short tail; the front paws do little heavy
        tippy-taps and the head tilts."""
        s = sin01(t, 0.6)
        out = {"hips": (0, 6 * s, 2 * s), "belly": (0, -7 * s, 0), "chest": (1.5 * sin01(t, 0.3), 3 * s, -2 * s),
               "neck1": (0, -2 * s, 0), "head": (0, 2 * s, 7 * sin01(t, 1.2, 0.25))}
        for k in range(1, 5):
            out[f"tail{k}"] = (0, (10 + 5 * k) * sin01(t, 0.6, -0.08 * k), 0)
        for side, phase in (("L", 0.0), ("R", 0.5)):
            lift = max(0.0, sin01(t, 1.2, -phase)) ** 1.5
            out[f"arm_up_{side}"] = (14 * lift, 0, 0)
            out[f"arm_lo_{side}"] = (-28 * lift, 0, 0)
            out[f"hand_{side}"] = (-14 * lift, 0, 0)
            out[f"leg_up_{side}"] = (0, -6 * s, -2 * s)
        return out

    (clip("tail_wag", 1.2, loop=True)
     .pose(0.0, merge(HALF, {"tail1": (-14, 0, 0), "tail2": (-6, 0, 0), "neck1": (4, 0, 0), "head": (6, 0, 0),
                             "jaw": (-8, 0, 0)}))
     .wave(happy_wag).wave(buzz(0.15, 8)))
    (clip("wing_flutter", 1.3).pose(0.0, FOLD).pose(0.22, OPEN).pose(1.0, OPEN).pose(1.3, FOLD)
     .wave(buzz(0.12, 38, 0.22, 1.0)).event(0.25, "flap").event(0.6, "flap"))

    # ------------------------------------------------------------------ feelings
    (clip("sulk", 1.6).pose(0.0, FOLD).pose(0.7, merge(FOLD, CROUCH))
     .pose(1.1, merge(FOLD, SULK, {"chest": (-4, 0, 0)})).pose(1.6, merge(FOLD, SULK))
     .wave(jiggle(1.1, 2.5, 0.24, 0.22)).event(0.5, "whimper").event(1.1, "thump"))
    (clip("sulk_loop", 5.0, loop=True).pose(0.0, merge(FOLD, SULK))
     .wave(lambda t: {"chest": (3 * max(0.0, sin01(t, 5.0)), 0, 0), "head": (-2 * max(0.0, sin01(t, 5.0)), 0, 0)}))
    (clip("nuzzle", 1.6, loop=True)
     .pose(0.0, merge(FOLD, {"neck1": (6, 0, 0), "neck2": (4, 0, 0), "head": (-6, 0, 0)}))
     .wave(lambda t: {"head": (0, 10 * sin01(t, 1.6), 14 * sin01(t, 1.6)), "neck3": (0, 6 * sin01(t, 1.6, 0.1), 0)})
     .wave(tail_sway(1.4, 0.8)).event(0.3, "purr"))
    (clip("greet", 1.8).pose(0.0, FOLD).pose(0.22, merge(FOLD, CROUCH))
     .pose(0.48, merge(OPEN, AIR, {"neck1": (12, 0, 0), "head": (12, 0, 0), "jaw": (-22, 0, 0)}))
     .pose(0.72, merge(HALF, CROUCH, {"chest": (-5, 0, 0)}))
     .pose(1.1, merge(HALF, {"neck1": (8, 0, 0), "head": (-4, 10, 12), "jaw": (-12, 0, 0)})).pose(1.8, FOLD)
     .wave(wag(1.0, 0.4)).wave(buzz(0.12, 30, 0.3, 1.3)).wave(jiggle(0.72, 3.0, 0.22, 0.2))
     .root(0.0).root(0.22).root(0.48, up=0.32).root(0.72).root(1.8).event(0.42, "call").event(0.72, "land"))

    # ------------------------------------------------------------------ hands-on care (the behaviour times these)
    (clip("pick_up", 0.8).pose(0.0, FOLD)
     .pose(0.25, merge(FOLD, PICK, {"jaw": (-22, 0, 0)})).pose(0.4, merge(FOLD, PICK, {"jaw": (-3, 0, 0)}))
     .pose(0.8, merge(FOLD, CARRY_HEAD)))
    (clip("drop_wait", 1.3).pose(0.0, merge(FOLD, CARRY_HEAD))
     .pose(0.35, merge(FOLD, {"neck1": (-14, 0, 0), "head": (-16, 0, 0), "jaw": (-4, 0, 0)}))
     .pose(0.5, merge(FOLD, {"neck1": (-14, 0, 0), "head": (-16, 0, 0), "jaw": (-22, 0, 0)}))
     .pose(0.9, merge(FOLD, SIT_UP, {"jaw": (-8, 0, 0)})).pose(1.3, merge(FOLD, SIT_UP, {"jaw": (-6, 0, 0)}))
     .wave(lambda t: {f"tail{k}": (0, (6 + 4 * k) * sin01(t, 0.45, -0.08 * k) * min(1.0, max(0.0, (t - 0.7) * 3)), 0)
                      for k in range(1, 5)}))
    (clip("leap_catch", 1.0).pose(0.0, FOLD).pose(0.15, merge(FOLD, CROUCH))
     .pose(0.32, merge(OPEN, AIR, {"neck1": (16, 0, 0), "head": (14, 0, 0), "jaw": (-24, 0, 0)}))
     .pose(0.45, merge(OPEN, AIR, {"neck1": (10, 0, 0), "head": (6, 0, 0), "jaw": (-3, 0, 0)}))
     .pose(0.7, merge(HALF, CROUCH, {"chest": (-5, 0, 0)})).pose(1.0, merge(FOLD, CARRY_HEAD))
     .wave(buzz(0.12, 34, 0.2, 0.68)).wave(jiggle(0.7, 3.0, 0.22, 0.2))
     .root(0.0).root(0.15).root(0.4, up=0.4).root(0.7).root(1.0).event(0.7, "land"))
    kick = merge(FOLD, SIT, {"neck1": (4, 0, 0), "head": (6, 8, 14), "leg_up_R": (70, 0, 8), "leg_lo_R": (-60, 0, 0)})
    (clip("leg_kick", 1.4).pose(0.0, FOLD).pose(0.3, kick).pose(1.15, kick).pose(1.4, FOLD)
     .wave(lambda t: {"leg_up_R": (18 * sin01(t, 0.2) if 0.3 < t < 1.15 else 0.0, 0, 0),
                      **{f"tail{k}": (0, (8 + 5 * k) * sin01(t, 0.4, -0.08 * k), 0) for k in range(1, 5)}})
     .event(0.35, "thump").event(0.55, "thump").event(0.75, "thump").event(0.95, "thump"))
    (clip("sniff_refuse", 1.4).pose(0.0, FOLD)
     .pose(0.4, merge(FOLD, {"neck1": (-10, 0, 0), "head": (-14, 0, 0)}))
     .pose(0.85, merge(FOLD, {"neck2": (4, 20, 0), "neck3": (2, 16, 0), "head": (10, 24, -8)}))
     .pose(1.4, FOLD).event(0.4, "sniff").event(0.9, "whimper"))
    (clip("lift_wing", 1.4).pose(0.0, FOLD).pose(0.35, merge(FOLD, R_HALF, {"chest": (0, 0, 5)}))
     .pose(1.05, merge(FOLD, R_HALF, {"chest": (0, 0, 5)})).pose(1.4, FOLD).event(0.35, "flap"))
    (clip("sneeze", 0.9).pose(0.0, FOLD)
     .pose(0.3, merge(FOLD, {"neck1": (6, 0, 0), "head": (14, 0, 0), "jaw": (-8, 0, 0)}))
     .pose(0.45, merge(HALF, {"neck1": (-6, 0, 0), "head": (-16, 0, 0), "jaw": (-4, 0, 0), "chest": (-3, 0, 0)}))
     .pose(0.9, FOLD).wave(jiggle(0.45, 2.5, 0.2, 0.18)).event(0.43, "sneeze"))
    (clip("pull_away", 1.0).pose(0.0, FOLD)
     .pose(0.32, merge(FOLD, {"neck1": (12, 0, 0), "neck2": (8, 0, 0), "head": (8, 0, 0), "chest": (5, 0, 0),
                              "hips": (-3, 0, 0)}))
     .pose(1.0, FOLD)
     .wave(lambda t: {"head": (0, 14 * sin01(t, 0.18) * max(0.0, 1 - abs(t - 0.5) / 0.26), 0)})
     .event(0.32, "whimper"))
    swat = merge(LOW_BOW, {"arm_up_L": (-70, 10, 0), "arm_lo_L": (-20, 0, 0), "hand_L": (30, 0, 0),
                           "chest": (-4, 0, 6), "head": (14, 8, -12), "jaw": (-14, 0, 0)})
    (clip("paw_bat", 0.8).pose(0.0, merge(FOLD, LOW_BOW)).pose(0.24, merge(FOLD, swat))
     .pose(0.42, merge(FOLD, swat)).pose(0.8, merge(FOLD, LOW_BOW))
     .wave(lambda t: {f"tail{k}": (0, 12 * sin01(t, 0.4, -0.08 * k), 0) for k in range(1, 5)})
     .event(0.24, "squeak"))
    (clip("tug", 1.0, loop=True).pose(0.0, merge(FOLD, TUG))
     .wave(lambda t: {"neck1": (0, 12 * sin01(t, 0.5), 0), "neck2": (0, 7 * sin01(t, 0.5, 0.1), 0),
                      "head": (0, 5 * sin01(t, 0.5, 0.2), 9 * sin01(t, 0.5, 0.15)), "hips": (0, 0, 3 * sin01(t, 1.0)),
                      **{f"tail{k}": (0, 14 * sin01(t, 0.5, -0.08 * k), 0) for k in range(1, 5)}}))

    # ------------------------------------------------------------------ flight
    # The little wings buzz like a bumblebee's while the heavy body stays level.
    fly = merge(FLY_WINGS, FLY_BODY)
    fly_flap = clip("fly_flap", 0.6, loop=True).pose(0.0, fly)
    fly_flap.wave(buzz(0.2, 44, sweep=8)).wave(tail_sway(0.5, 0.6))
    fly_flap.wave(lambda t: {"chest": (1.2 * sin01(t, 0.2, 0.3), 0, 0), "head": (-0.8 * sin01(t, 0.2, 0.3), 0, 0),
                             "arm_up_L": (4 * sin01(t, 0.6), 0, 0), "arm_up_R": (4 * sin01(t, 0.6, 0.5), 0, 0),
                             "leg_up_L": (3 * sin01(t, 0.6, 0.25), 0, 0), "leg_up_R": (3 * sin01(t, 0.6, 0.75), 0, 0)})
    fly_flap.event(0.05, "flap").event(0.25, "flap").event(0.45, "flap")
    (clip("fly_glide", 2.4, loop=True).pose(0.0, merge(fly, {"wing_arm*": (0, 0, -6)}))
     .wave(buzz(0.2, 12)).wave(tail_sway(0.6, 2.4))
     .wave(lambda t: {"hips": (0, 0, 2 * sin01(t, 2.4)), "head": (0, 0, -1.5 * sin01(t, 2.4))}))
    dive = merge(FLY_BODY, {"wing_arm*": (0, 34, -24), "wing_fore*": (0, -20, 0), "neck1": (-10, 0, 0),
                            "head": (-4, 0, 0), "tail1": (-4, 0, 0)})
    clip("fly_dive", 1.0, loop=True).pose(0.0, dive).wave(buzz(0.2, 8)).wave(tail_sway(0.4, 0.5))

    # ------------------------------------------------------------------ the bun (hatchling versions)
    # A baby's head and body are one round bun: bending its neck folds the bun over, so it
    # tips its whole body instead and only nods its head.
    (clip("eat_h", 1.4, loop=True).pose(0.0, merge(FOLD, EAT_H))
     .wave(lambda t: {"head": (6 * max(0.0, sin01(t, 0.7)), 0, 0), "hips": (-2 * max(0.0, sin01(t, 0.7)), 0, 0)})
     .wave(chomp(22, 0.7)).event(0.15, "chomp").event(0.85, "chomp"))
    (clip("pick_up_h", 0.8).pose(0.0, FOLD)
     .pose(0.25, merge(FOLD, EAT_H, {"jaw": (-24, 0, 0)})).pose(0.4, merge(FOLD, EAT_H, {"jaw": (-3, 0, 0)}))
     .pose(0.8, merge(FOLD, {"head": (-6, 0, 0)})))
    (clip("drop_wait_h", 1.3).pose(0.0, merge(FOLD, {"head": (-6, 0, 0)}))
     .pose(0.35, merge(FOLD, {"hips": (-6, 0, 0), "head": (-12, 0, 0), "jaw": (-4, 0, 0)}))
     .pose(0.5, merge(FOLD, {"hips": (-6, 0, 0), "head": (-12, 0, 0), "jaw": (-24, 0, 0)}))
     .pose(0.9, merge(FOLD, SIT, {"head": (8, 0, 0), "jaw": (-8, 0, 0)}))
     .pose(1.3, merge(FOLD, SIT, {"head": (8, 0, 0), "jaw": (-6, 0, 0)}))
     .wave(lambda t: {f"tail{k}": (0, (6 + 4 * k) * sin01(t, 0.45, -0.08 * k) * min(1.0, max(0.0, (t - 0.7) * 3)), 0)
                      for k in range(1, 5)}))
    (clip("lie_down_h", 1.2).pose(0.0, FOLD).pose(0.45, merge(FOLD, CROUCH))
     .pose(0.85, merge(FOLD, LIE_H, {"chest": (-4, 0, 0), "head": (-6, 0, 0)})).pose(1.2, merge(FOLD, LIE_H))
     .wave(jiggle(0.85, 3.0, 0.24, 0.22)).event(0.85, "thump"))
    clip("lie_loop_h", 4.4, loop=True).pose(0.0, merge(FOLD, LIE_H)).wave(breathe(1.6, 4.4)).wave(tail_sway(0.5, 4.4))
    (clip("nap_flop_h", 0.8).pose(0.0, FOLD).pose(0.3, merge(FOLD, CROUCH, {"head": (8, 0, 0)}))
     .pose(0.55, merge(FOLD, LIE_H, {"chest": (-6, 0, 0), "head": (-8, 0, 0)})).pose(0.8, merge(FOLD, LIE_H))
     .wave(jiggle(0.55, 4.0, 0.22, 0.2)).event(0.55, "thump"))
    clip("curl_up_h", 1.6).pose(0.0, merge(FOLD, LIE_H)).pose(1.6, merge(FOLD, SLEEP_H))
    clip("sleep_h", 5.2, loop=True).pose(0.0, merge(FOLD, SLEEP_H)).wave(breathe(2.4, 5.2))
    (clip("wake_h", 2.8).pose(0.0, merge(FOLD, SLEEP_H)).pose(0.7, merge(FOLD, LIE_H))
     .pose(1.4, merge(FOLD, LIE_H, {"head": (18, 0, 0), "snout": (6, 0, 0), "jaw": (-34, 0, 0), "chest": (6, 0, 0)}))
     .pose(2.1, merge(HALF, CROUCH)).pose(2.8, FOLD).wave(buzz(0.12, 22, 1.9, 2.4)).event(1.4, "yawn"))
    (clip("sulk_h", 1.6).pose(0.0, FOLD).pose(0.7, merge(FOLD, CROUCH))
     .pose(1.1, merge(FOLD, SULK_H, {"chest": (-4, 0, 0)})).pose(1.6, merge(FOLD, SULK_H))
     .wave(jiggle(1.1, 2.5, 0.24, 0.22)).event(0.5, "whimper").event(1.1, "thump"))
    (clip("sulk_loop_h", 5.0, loop=True).pose(0.0, merge(FOLD, SULK_H))
     .wave(lambda t: {"chest": (3 * max(0.0, sin01(t, 5.0)), 0, 0), "head": (-2 * max(0.0, sin01(t, 5.0)), 0, 0)}))
    return C


def clips():
    out = clip_set()
    clipkit.check(out, BONE_ORDER, NAME)
    return out
