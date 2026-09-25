"""The Flurrytail's body plan: a light, fox-like quadruped with a pair of medium bat wings and
a great plumed tail that curls up over its back.

Skeleton (25 body bones, the draw's limit, and 12 wing bones): the classic four legs and wing
chain, but the tail has SIX bones for the plume's curl and sway, paid for by a shorter spine: no
belly bone (a fox's torso bends little between the hips and the chest) and a two-bone neck
(neck2, neck3: the look-at's bones) under the mane.

Clips: the classic set reshaped for this body (the neck and torso retargeted, the wings folded
the Flurrytail's way), with the tail taken off every classic clip and given its own motion:
at rest a big, slow, luxurious sway (the plume leans from side to side, the tip lagging);
held high and wagging when happy; trailing low with the tip twitching when stalking;
streaming behind at a run and in flight. Its own versions of the clips that sit, lie, curl,
sulk, roll over and pounce use poses solved for this body from bone directions (like the
wing fold): a fox's sit with the plume wrapped round in front of its paws; a sphinx lie;
asleep curled round with its nose tucked into its tail (the baby lays its chin down by its
pom-pom instead: *_h); on its back with the plume out beside it; and the pounce, a fox's
mousing dive: a rump wiggle, a spring nose-up off the hind legs, a high arc, then down nose
and forepaws first, the plume arched high. A little spring in every step (root bounces) and
higher hops keep it light and bouncy. Same names, events and lengths as the classic clips
(the game times things against them), but for the idle: 4.8 s, one slow sway per loop.
The grown form carries its neck upright (the kind's base_pose); the grown clips that bring
the head down take that lift back exactly (LIFT, lowered()), their baby versions don't.

A tail bone points backward, so in the clips' armature-axis convention (made for bones that
point forward) pitch + swings it down and yaw + swings it to the right: bend() takes the
tail's poses as side-view angles and turns round to the left instead.
"""
import math

from dragons import clipkit
from dragons.clipkit import Clip

NAME = "flurrytail"


BONES = [
    ("hips", "hips", "chest", None),
    ("chest", "chest", "neck2", "hips"),
    ("neck2", "neck2", "neck3", "chest"),
    ("neck3", "neck3", "head", "neck2"),
    ("head", "head", "muzzle", "neck3"),
    ("snout", "muzzle", "snout", "head"),
    ("tail1", "hips", "tail2", "hips"),
    ("tail2", "tail2", "tail3", "tail1"),
    ("tail3", "tail3", "tail4", "tail2"),
    ("tail4", "tail4", "tail5", "tail3"),
    ("tail5", "tail5", "tail6", "tail4"),
    ("tail6", "tail6", "tail_tip", "tail5"),
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

WING_CHAIN = [("wing_arm", "root", "elbow", "chest"), ("wing_fore", "elbow", "wrist", "wing_arm"),
              ("wing_f1", "wrist", "f1", "wing_fore"), ("wing_f2", "wrist", "f2", "wing_fore"),
              ("wing_f3", "wrist", "f3", "wing_fore"), ("wing_f4", "wrist", "f4", "wing_fore")]
WING_BONES = [f"{n}_{side}" for side in ("L", "R") for n, *_ in WING_CHAIN]
BONE_ORDER = [b[0] for b in BONES] + WING_BONES
WING_BODY = ("chest", "hips")
CONTACTS = ("hand_L", "hand_R", "foot_L", "foot_R")
SEAT = ("hips", (0.0, 0.62, 0.5))
REGION = {}

# ------------------------------------------------------------------------------ helpers
# The classic folded wings (tools/anim/clips.py WINGS_FOLDED) and the Flurrytail's own fold,
# solved for its medium wings from bone directions (after tools/blender/fold_solver.py): the
# upper arm back along the shoulder, the forearm down and forward, the short hand's fingers
# held close together back along the flank, so the frosted wing lies flat like a cape and
# ends at the rump (the kind's wing layout: a long arm and forearm, a short hand).
CLASSIC_FOLDED = {"wing_arm": (-86, 133, 6), "wing_fore": (-46, -126, -65), "wing_f1": (39, 118, -73),
                  "wing_f2": (43, 63, -32), "wing_f3": (25, 24, -5), "wing_f4": (-1, -1, 4)}
FOLDED = {"wing_arm": (-86, 146, 21), "wing_fore": (-44, -127, -62), "wing_f1": (41, 107, -67),
          "wing_f2": (42, 61, -32), "wing_f3": (30, 32, -12), "wing_f4": (12, 11, -1)}
WINGS_FOLDED = {f"{b}*": v for b, v in FOLDED.items()}
WINGS_HALF = {"wing_arm*": (0, 20, -25), "wing_f1*": (12, 10, 0), "wing_f4*": (-4, -3, 0)}
WINGS_OPEN = {"wing_arm*": (0, -10, 10)}


def merge(*poses):
    out = {}
    for p in poses:
        for bone, v in p.items():
            a = out.get(bone, (0.0, 0.0, 0.0))
            out[bone] = tuple(x + y for x, y in zip(a, v))
    return out


def sin01(t, period, phase=0.0):
    return math.sin(2 * math.pi * (t / period + phase))


def refold(clip):
    """Swap the classic folded-wing keys for this body's fold (only exact classic keys: a
    wing lifted or spread in a clip keeps its own)."""
    keys = clip.filled_keys()
    for base, old in CLASSIC_FOLDED.items():
        for side, sgn in (("R", 1), ("L", -1)):
            want = (old[0], sgn * old[1], sgn * old[2])
            new = FOLDED[base]
            new = (new[0], sgn * new[1], sgn * new[2])
            for t, v in keys.get(f"{base}_{side}", {}).items():
                if all(abs(a - b) < 1e-6 for a, b in zip(v, want)):
                    keys[f"{base}_{side}"][t] = new
    clip.keys, clip.pose_times, clip.pose_bones, clip._filled = keys, set(), set(), None
    return clip


# ------------------------------------------------------------------------------ the tail
# The plume's bones at rest, as angles in the side view (degrees up from straight back; 90 is
# straight up, 180 forward): it rises from the rump and curls forward over the back.
REST = (35, 51, 74, 102, 137, 177)


def bend(angles, round_=(0, 0, 0, 0, 0, 0), lean=(0, 0, 0, 0, 0, 0)):
    """The tail posed so each bone points at the given side-view angle: the per-bone pitch
    deltas that get it there. round_: degrees each bone swings round toward the dragon's
    left (+X); lean: degrees each leans over to its left. (A tail bone points backward, so in
    the clips' armature-axis convention, made for bones pointing forward, pitch + swings it
    DOWN and yaw + swings it to the right: hence the signs.)"""
    out, before = {}, 0.0
    for k, (a, r) in enumerate(zip(angles, REST)):
        out[f"tail{k + 1}"] = (-((a - r) - before), -round_[k], -lean[k])
        before = a - r
    return out


TAIL_REST = bend(REST)
TAIL_HIGH = bend((52, 70, 94, 124, 160, 200))      # happy, proud: up and curled tight
TAIL_STREAM = bend((12, 14, 18, 26, 36, 48))       # at a run, flying: streaming behind
TAIL_TRAIL = bend((4, -2, -6, 2, 14, 30))          # low and level behind, the tip up


def sway(amount=1.0, period=4.0, phase=0.0):
    """The big, slow, luxurious sway: the plume leans from side to side from its root (roll)
    and swings a little (yaw), each bone a beat behind the last, so it flows like a scarf.
    Periodic in `period` (a loop's length must be a multiple of it)."""
    def fn(t):
        out = {}
        for k in range(6):
            lag = -0.07 * k
            out[f"tail{k + 1}"] = (1.2 * amount * sin01(t, period / 2, phase + lag * 2),
                                   amount * (3.0 + 1.0 * k) * sin01(t, period, phase + lag),
                                   amount * (7.0 - 0.6 * k) * sin01(t, period, phase + lag + 0.05))
        return out
    return fn


def wag(amount=1.0, period=0.5):
    """Happy: quicker and bigger, the tip flicking round."""
    def fn(t):
        return {f"tail{k + 1}": (0, amount * (5 + 3 * k) * sin01(t, period, -0.08 * k),
                                 amount * (9 - k) * sin01(t, period, -0.08 * k + 0.1)) for k in range(6)}
    return fn


def flick(amount=1.0, period=0.3, first=3):
    """Only the tip twitching (stalking, curious)."""
    def fn(t):
        return {f"tail{k + 1}": (0, amount * 10 * (k - first + 1) * sin01(t, period, -0.1 * k), 0)
                for k in range(first, 6)}
    return fn


def stream(amount=1.0, period=0.5):
    """Streaming behind: a ripple running down it, as through air."""
    def fn(t):
        return {f"tail{k + 1}": (amount * 3 * sin01(t, period, -0.1 * k), amount * (2 + k) * sin01(t, period, -0.13 * k),
                                 0) for k in range(6)}
    return fn


def breathe(amount=1.0, period=3.2):
    def fn(t):
        s = sin01(t, period) * amount
        return {"chest": (1.2 * s, 0, 0), "neck2": (0.8 * s, 0, 0), "head": (-0.8 * s, 0, 0),
                "wing_arm_R": (0, 0, 1.5 * s), "wing_arm_L": (0, 0, -1.5 * s)}
    return fn


def tail(clip, keys, *waves):
    """Give a classic clip its tail: static poses at times [(t, pose)] and procedural waves."""
    for t, pose in keys:
        clip.key(min(t, clip.length - 1e-3) if clip.loop else t, **pose)
    for w in waves:
        clip.wave(w)
    return clip


# ------------------------------------------------------------------------------ poses
# Solved for this body from bone directions (like the fold), on the adult; the hatchling's
# short legs fold the same way. Every pose carries its tail.
STAND = dict(WINGS_FOLDED)
LIE_LEGS = {"arm_up*": (-81, 31, 30), "arm_lo*": (165, -9, 0), "hand*": (-61, 0, 0),   # a sphinx: forearms
            "leg_up*": (40, -4, 8), "leg_lo*": (-75, -1, 2), "foot*": (69, 0, 0)}     # forward, hocks under
def mirrored(pose):
    """The same pose to the other side (yaw and roll flip; sided keys swap)."""
    out = {}
    for bone, (p, y, r) in pose.items():
        if bone.endswith(("_L", "_R")):
            bone = bone[:-2] + ("_R" if bone.endswith("_L") else "_L")
        out[bone] = (p, -y, -r)
    return out


TAIL_LIE = mirrored({"tail1": (66, -19, -2), "tail2": (2, -18, -23), "tail3": (21, -5, -46), "tail4": (14, 19, -48),
                     "tail5": (25, 26, -22), "tail6": (33, 27, -4)})       # round its right side
LIE = merge(WINGS_FOLDED, LIE_LEGS, TAIL_LIE, {"neck2": (-6, 0, 0), "head": (1, 0, 0)})
# Asleep: curled to its right, the plume round its flank and front, the nose tucked into it
# (solved curling left, mirrored: the den and review cameras look from its right).
CURL = merge(WINGS_FOLDED, LIE_LEGS, mirrored({
    "chest": (0, 22, 0), "hips": (0, -10, 0), "neck2": (-60, 8, -59), "neck3": (-21, 23, -56), "head": (57, 47, -5),
    "tail1": (69, -15, -1), "tail2": (7, -25, -32), "tail3": (27, -3, -65), "tail4": (22, 19, -41),
    "tail5": (22, 34, -29), "tail6": (30, 40, -5)}))
# The baby's big head can't turn back like that: it lays its chin on the floor, turned a little
# toward the pom-pom tucked round beside its cheek.
CURL_BABY = merge(WINGS_FOLDED, LIE_LEGS, {
    "chest": (0, -12, 0), "hips": (0, 8, 0), "neck2": (-39, -3, 12), "neck3": (-42, -1, 8), "head": (57, -17, 5),
    "tail1": (58, 24, -5), "tail2": (17, 24, 29), "tail3": (34, 5, 53), "tail4": (32, -18, 44),
    "tail5": (28, -25, 32), "tail6": (22, -42, 27)})
# A fox's sit: forelegs straight, the plume wrapped round in front of its paws.
SIT = merge(WINGS_FOLDED, {
    "hips": (37, 0, 0), "chest": (-20, 0, 0), "arm_up*": (-20, 1, -1), "arm_lo*": (1, 0, 1), "hand*": (22, -1, 0),
    "leg_up*": (11, 1, 4), "leg_lo*": (-65, -2, 3), "foot*": (54, -1, -1),
    "neck2": (-11, 0, 0), "neck3": (6, 0, 0), "head": (-16, 0, 0),
    "tail1": (58, -19, -5), "tail2": (1, -29, -36), "tail3": (46, 15, -76), "tail4": (35, 25, -44),
    "tail5": (32, 40, -35), "tail6": (39, 43, -10)})
SIT_UP = merge(SIT, {"neck2": (8, 0, 0), "head": (12, 0, 0)})  # sitting, looking up at you
# Sulking: flat on the floor, chin down and turned away, the plume swung round the other way.
SULK = merge(WINGS_FOLDED, LIE_LEGS, {
    "neck2": (-82, 22, 41), "neck3": (-9, -2, 3), "head": (88, 19, -17),
    "tail1": (65, 9, 1), "tail2": (-1, 11, 13), "tail3": (17, 2, 13), "tail4": (26, -7, 14),
    "tail5": (34, -17, 15), "tail6": (41, -21, 5)})
# Eating from the floor: down on its elbows, the rump up.
EAT = merge(WINGS_FOLDED, {
    "hips": (-10, 0, 0), "chest": (-43, 0, 0), "arm_up*": (75, -20, 22), "arm_lo*": (-49, -3, -6), "hand*": (47, 4, 0),
    "leg_up*": (3, -1, -2), "leg_lo*": (24, -1, 2), "foot*": (17, 2, 1),
    "neck2": (-56, 0, 0), "neck3": (-26, 0, 0), "head": (114, 0, 0)})
# On its back, paws up, head turned to look at you, the plume out on the floor beside it.
BELLY_UP = merge(WINGS_HALF, {
    "hips": (0, 0, 165), "chest": (0, 0, 12), "arm_up*": (55, 0, 0), "arm_lo*": (-70, 0, 0),
    "leg_up*": (50, 0, 0), "leg_lo*": (-70, 0, 0), "neck2": (8, 0, -70), "neck3": (4, 0, -55), "head": (15, 0, -20),
    "tail1": (19, 15, 9), "tail2": (33, 8, 12), "tail3": (28, 1, 20), "tail4": (28, -11, 23),
    "tail5": (28, -26, 22), "tail6": (33, -33, 5)})
CROUCH_BODY = {"hips": (-8, 0, 0), "leg_up*": (40, 0, 0), "leg_lo*": (-70, 0, 0), "foot*": (30, 0, 0),
               "arm_up*": (38, 0, 0), "arm_lo*": (-62, 0, 0), "hand*": (24, 0, 0), "neck2": (-16, 0, 0), "head": (6, 0, 0)}
CROUCH_FOLDED = merge(WINGS_FOLDED, CROUCH_BODY)
# The mousing pounce: a spring nose-up off the hind legs, then a dive nose and forepaws first,
# the plume arched high behind (the den carries it forward between 0.65 and 1.05 s).
POUNCE_CROUCH = merge(WINGS_FOLDED, CROUCH_BODY, {"hips": (-6, 0, 0), "neck2": (-10, 0, 0), "head": (10, 0, 0)},
                      TAIL_TRAIL)
POUNCE_READY = merge(WINGS_HALF, CROUCH_BODY, {"hips": (-10, 0, 0), "neck2": (-12, 0, 0), "head": (12, 0, 0),
                                              "leg_up*": (8, 0, 0), "leg_lo*": (-10, 0, 0)}, TAIL_TRAIL)
LAUNCH = merge(WINGS_OPEN, {
    "hips": (24, 0, 0), "chest": (-19, 0, 0), "leg_up*": (-86, 89, 83), "leg_lo*": (57, -10, 10), "foot*": (-86, -5, -5),
    "arm_up*": (43, -3, 2), "arm_lo*": (-72, -5, -6), "hand*": (-34, 1, 1), "neck2": (-6, 0, 0), "neck3": (-8, 0, 0),
    "head": (12, 0, 0), "tail1": (-15, 0, 0), "tail2": (20, 0, 0), "tail3": (19, 0, 0), "tail4": (20, 0, 0),
    "tail5": (14, 0, 0), "tail6": (16, 0, 0)})
DIVE = merge(WINGS_OPEN, {
    "hips": (-52, 0, 0), "chest": (-54, 0, 0), "leg_up*": (-85, 43, 37), "leg_lo*": (109, -5, 5),
    "foot*": (-124, -14, -15), "arm_up*": (124, 9, -9), "arm_lo*": (-2, 0, -1), "hand*": (-45, 6, 4),
    "neck2": (33, 0, 0), "neck3": (-30, 0, 0), "head": (64, 0, 0), "jaw": (-18, 0, 0),
    "tail1": (15, 0, 0), "tail2": (-1, 0, 0), "tail3": (5, 0, 0), "tail4": (15, 0, 0), "tail5": (18, 0, 0),
    "tail6": (27, 0, 0)})
PIN = merge(WINGS_HALF, {  # landed: forepaws pinning, nose down to them, rump and plume up
    "hips": (-18, 0, 0), "chest": (-10, 0, 0), "arm_up*": (44, 0, 10), "arm_lo*": (-64, 0, 0), "hand*": (34, 0, 0),
    "leg_up*": (18, 0, 0), "leg_lo*": (-6, 0, 0), "neck2": (-24, 0, 0), "neck3": (-14, 0, 0), "head": (-16, 0, 0),
    "jaw": (-6, 0, 0)}, TAIL_HIGH)
# Chasing its own tail: curled round to its left, the plume swung round to meet its nose.
TAIL_CHASE = merge(WINGS_HALF, {
    "neck2": (0, 44, 0), "neck3": (0, 30, 0), "head": (-6, 18, 10), "chest": (0, 14, 0), "hips": (0, 10, 0),
    "jaw": (-16, 0, 0)}, bend((20, 26, 40, 60, 80, 100), (24, 30, 32, 30, 26, 20)))


def envelope(t, t0, t1, ramp=0.12):
    """0 outside [t0, t1], easing in and out over `ramp` seconds."""
    return max(0.0, min(1.0, (t - t0) / ramp, (t1 - t) / ramp))


# The grown Flurrytail carries its neck upright and its head high (the kind's base_pose, the
# idle every clip rides on): LIFT is that lift in the clips' terms (pitch up). The baby has no
# lift. Grown clips that must bring the head down to where the poses were solved (eating,
# sleeping, sulking, picking up, the pounce's dive) take it back with lowered(): a wave, which
# the runtime applies in armature axes after the keys, so it undoes the lift exactly even on
# a twisted neck. Their baby versions (*_h) do without.
LIFT = {"neck2": 12.0, "neck3": 6.0, "head": -20.0}  # the head levels itself on the upright neck


def lowered(keys=((0.0, 1.0),)):
    """A wave taking the grown neck lift back, eased in and out by [(t, amount 0..1)]."""
    keys = sorted(keys)

    def amount(t):
        if t <= keys[0][0]:
            return keys[0][1]
        for (t0, a0), (t1, a1) in zip(keys, keys[1:]):
            if t <= t1:
                u = (t - t0) / (t1 - t0)
                return a0 + (a1 - a0) * u * u * (3 - 2 * u)
        return keys[-1][1]

    return lambda t: {b: (-v * amount(t), 0.0, 0.0) for b, v in LIFT.items()}


def pounce_wiggle(t):
    """The fox's rump wiggle before it springs."""
    k = envelope(t, 0.28, 0.62, 0.08)
    return {"hips": (0, 6 * k * sin01(t, 0.16), 0), "tail5": (0, 14 * k * sin01(t, 0.16, 0.2), 0),
            "tail6": (0, 18 * k * sin01(t, 0.16, 0.3), 0)}


# ------------------------------------------------------------------------------ clips
def fit(fn):
    """A classic wave on this skeleton: neck1's motion goes to neck2, the belly's to the chest."""
    names = {"neck1": "neck2", "belly": "chest"}

    def out(t):
        res = {}
        for bone, v in fn(t).items():
            b = names.get(bone, bone)
            res[b] = tuple(x + y for x, y in zip(res.get(b, (0.0, 0.0, 0.0)), v))
        return res
    return out


def own_clips():
    """This body's own versions of the clips that lie, sit, curl or pounce (same names,
    lengths and events as the classic ones: the game times things against them)."""
    import clips as classic  # tools/anim/clips.py, for its wave helpers
    out = []

    def clip(*a, **k):
        c = Clip(*a, **k)
        out.append(c)
        return c

    clip("sit", 0.8).pose(0.0, STAND).pose(0.8, SIT)
    clip("sit_loop", 3.2, loop=True).pose(0.0, SIT).wave(breathe()).wave(sway(0.25, 3.2))
    clip("lie_down", 1.1).pose(0.0, STAND).pose(0.55, CROUCH_FOLDED).pose(1.1, LIE).event(0.9, "thump")
    clip("lie_loop", 4.0, loop=True).pose(0.0, LIE).wave(breathe(1.2, 4.0)).wave(sway(0.25, 4.0))
    stretch = merge(LIE, {"hips": (-14, 0, 0), "neck2": (18, 0, 0), "neck3": (8, 0, 0), "head": (22, 0, 0),
                          "snout": (6, 0, 0), "jaw": (-32, 0, 0), "wing_arm*": (0, 10, -10)})
    half_up = merge(WINGS_HALF, {"arm_up*": (22, 0, 0), "hips": (-10, 0, 0), "neck2": (14, 0, 0), "head": (10, 0, 0)},
                    TAIL_HIGH)
    for sfx, curl in (("", CURL), ("_h", CURL_BABY)):
        grown = not sfx
        c = clip("curl_up" + sfx, 1.4).pose(0.0, LIE).pose(1.4, curl)
        if grown:
            c.wave(lowered([(0.0, 0.0), (1.4, 1.0)]))
        c = clip("sleep" + sfx, 4.8, loop=True).pose(0.0, curl).wave(breathe(1.8, 4.8)).wave(sway(0.12, 4.8))
        if grown:
            c.wave(lowered())
        c = (clip("wake" + sfx, 2.6).pose(0.0, curl).pose(0.6, LIE).pose(1.3, stretch).pose(1.9, half_up)
             .pose(2.6, STAND).event(1.3, "yawn"))
        if grown:
            c.wave(lowered([(0.0, 1.0), (0.6, 0.0)]))
    clip("nap_flop", 0.7).pose(0.0, STAND).pose(0.7, LIE).event(0.55, "thump")
    eat = clip("eat", 1.2, loop=True).pose(0.0, EAT)
    eat.wave(lambda t: {"head": (8 * max(0.0, sin01(t, 0.6)), 0, 0), "snout": (6 * max(0.0, sin01(t, 0.6)), 0, 0),
                        "neck2": (3 * sin01(t, 1.2), 0, 0)}).wave(classic.chomp()).wave(sway(0.5, 1.2))
    eat.wave(lowered())
    eat.event(0.15, "chomp").event(0.75, "chomp")
    clip("roll_over", 1.1).pose(0.0, STAND).pose(0.45, LIE).pose(1.1, BELLY_UP).event(0.8, "thump")
    (clip("belly_rub", 1.2, loop=True).pose(0.0, BELLY_UP)
     .wave(lambda t: {"leg_up_L": (10 * sin01(t, 0.6), 0, 0), "leg_up_R": (10 * sin01(t, 0.6, 0.5), 0, 0),
                      "arm_up_L": (8 * sin01(t, 0.6, 0.25), 0, 0), "arm_up_R": (8 * sin01(t, 0.6, 0.75), 0, 0),
                      "hips": (0, 0, 6 * sin01(t, 1.2)), "tail4": (0, 8 * sin01(t, 0.6), 0),
                      "tail5": (0, 12 * sin01(t, 0.6, -0.1), 0), "tail6": (0, 16 * sin01(t, 0.6, -0.2), 0)})
     .wave(classic.pant(10.0, 0.6)).event(0.3, "purr"))
    for sfx in ("", "_h"):
        c = (clip("sulk" + sfx, 1.5).pose(0.0, STAND).pose(0.7, CROUCH_FOLDED).pose(1.5, SULK)
             .event(0.5, "whimper").event(1.2, "thump"))
        if not sfx:
            c.wave(lowered([(0.0, 0.0), (0.7, 0.4), (1.5, 1.0)]))
        c = (clip("sulk_loop" + sfx, 5.0, loop=True).pose(0.0, SULK)
             .wave(lambda t: {"chest": (3 * max(0.0, sin01(t, 5.0)), 0, 0),
                              "head": (-2 * max(0.0, sin01(t, 5.0)), 0, 0)})
             .wave(sway(0.15, 5.0)))
        if not sfx:
            c.wave(lowered())
    for name, down in (("drop_wait", merge(WINGS_FOLDED, {"neck2": (-14, 0, 0), "head": (-16, 0, 0)})),
                       ("drop_wait_h", merge(WINGS_FOLDED, {"neck2": (-6, 0, 0), "head": (-18, 0, 0)}))):
        carry = merge(WINGS_FOLDED, {"neck2": (12, 0, 0), "head": (-8, 0, 0)})
        c = (clip(name, 1.3).pose(0.0, carry)
             .pose(0.35, merge(down, {"jaw": (-4, 0, 0)})).pose(0.5, merge(down, {"jaw": (-22, 0, 0)}))
             .pose(0.9, merge(SIT_UP, {"jaw": (-8, 0, 0)})).pose(1.3, merge(SIT_UP, {"jaw": (-6, 0, 0)}))
             .wave(lambda t: {f"tail{k}": (0, (4 + 3 * k) * sin01(t, 0.4, -0.08 * k) *
                                           min(1.0, max(0.0, (t - 0.9) * 3)), 0) for k in range(4, 7)}))
        if name == "drop_wait":
            c.wave(lowered([(0.0, 0.0), (0.35, 1.0), (0.5, 1.0), (0.9, 0.0)]))
    kick = merge(SIT, {"neck2": (4, 0, 0), "head": (6, 8, 14), "leg_up_R": (60, 0, 0), "leg_lo_R": (-40, 0, 0)})
    (clip("leg_kick", 1.4).pose(0.0, STAND).pose(0.25, kick).pose(1.15, kick).pose(1.4, STAND)
     .wave(lambda t: {"leg_up_R": (22 * sin01(t, 0.18) * envelope(t, 0.25, 1.15), 0, 0),
                      **{f"tail{k}": (0, (5 + 3 * k) * sin01(t, 0.35, -0.08 * k) * envelope(t, 0.25, 1.15), 0)
                         for k in range(4, 7)}})
     .event(0.3, "thump").event(0.48, "thump").event(0.66, "thump").event(0.84, "thump"))
    for sfx in ("", "_h"):
        c = (clip("pounce" + sfx, 1.4).pose(0.0, STAND).pose(0.3, POUNCE_CROUCH).pose(0.46, POUNCE_CROUCH)
             .pose(0.6, POUNCE_READY).pose(0.76, LAUNCH).pose(0.95, DIVE).pose(1.07, PIN).pose(1.25, PIN)
             .pose(1.4, STAND).wave(pounce_wiggle)
             .root(0.0).root(0.62).root(0.76, up=0.4).root(0.86, up=0.95).root(0.96, up=0.58).root(1.07).root(1.4)
             .event(0.7, "squeak").event(1.07, "land"))
        if not sfx:
            c.wave(lowered([(0.76, 0.0), (0.95, 1.0), (1.25, 1.0), (1.4, 0.0)]))
    tail_chase = clip("tail_chase", 0.6, loop=True).pose(0.0, TAIL_CHASE)
    tail_chase.wave(fit(classic.leg_cycle(0.6, 22, 36, classic.TROT_PHASES, bob=2.5))).wave(classic.pant(6.0, 0.3))
    tail_chase.wave(wag(0.5, 0.3))
    tail_chase.root(0.0, up=0.0).root(0.15, up=0.1).root(0.3, up=0.0).root(0.45, up=0.1)
    classic.footsteps(tail_chase, 0.6, {"a": 0.0, "b": 0.5})
    idle = clip("idle", 4.8, loop=True).pose(0.0, STAND).wave(breathe(1.0, 2.4)).wave(sway(1.0, 4.8))
    idle.wave(lambda t: {"head": (0, 0, 3 * sin01(t, 4.8, 0.3)), "neck3": (0, 2 * sin01(t, 4.8, 0.1), 0)})
    return out


def clips():
    out = clipkit.classic()
    for c in out:
        clipkit.retarget(c, BONE_ORDER, rename={"belly": "chest", "tail1": None, "tail2": None, "tail3": None,
                                                "tail4": None},
                         chains=[(("neck1", "neck2", "neck3"), ("neck2", "neck3"))])
        refold(c)
    by = clipkit.by_name(out)
    L = {c.name: c.length for c in out}
    rest = [(0.0, TAIL_REST)]
    # On its feet: the slow, luxurious sway (loops sway once per loop).
    for name in ("look_around", "scratch", "shake", "wing_flutter", "lift_wing", "sneeze", "pull_away",
                 "sniff_refuse", "yawn", "pick_up", "pick_up_h", "eat_h", "shuffle"):
        tail(by[name], rest, sway(0.8 if by[name].loop else 1.0, L[name] if by[name].loop else 4.0))
    for name in ("walk", "walk_h", "carry", "carry_h"):
        tail(by[name], [(0.0, TAIL_HIGH)], sway(0.6, L[name]))
    tail(by["trot"], [(0.0, TAIL_TRAIL)], stream(0.8, L["trot"]))
    for name in ("gallop", "scamper"):
        tail(by[name], [(0.0, TAIL_STREAM)], stream(1.0, L[name]))
    for name in ("fly_flap", "fly_glide", "fly_dive"):
        tail(by[name], [(0.0, TAIL_STREAM)], stream(0.7, L[name]))
    # Happy: held high and wagging.
    for name, period in (("tail_wag", 0.5), ("greet", 0.4), ("fav_wiggle", 0.4), ("play_bow", 0.25), ("spar", 0.5),
                         ("paw_bat", 0.375), ("tug", 0.45), ("pet_head", 0.8), ("pet_chin", 0.8), ("nuzzle", 0.7)):
        tail(by[name], [(0.0, TAIL_HIGH)], wag(1.0 if name == "tail_wag" else 0.7, period))
    for name in ("hop", "leap_catch"):
        tail(by[name], [(0.0, TAIL_HIGH)], sway(0.6, 2.0))
    tail(by["stalk"], [(0.0, TAIL_TRAIL)], flick(1.0, 0.3))
    by["pick_up"].wave(lowered([(0.0, 0.0), (0.25, 1.0), (0.4, 1.0), (0.8, 0.0)]))
    # Light on its feet: a little spring in every step, higher hops and leaps.
    for name, bounce, steps in (("walk", 0.035, 4), ("walk_h", 0.03, 4), ("carry", 0.025, 4), ("carry_h", 0.02, 4),
                                ("trot", 0.06, 2)):
        c = by[name]
        for k in range(steps):
            c.root(c.length * k / steps, up=0.0).root(c.length * (k + 0.5) / steps, up=bounce)
    for name, k in (("hop", 1.35), ("leap_catch", 1.2), ("greet", 1.15)):
        by[name].root_keys = {t: (f, u * k) for t, (f, u) in by[name].root_keys.items()}
    out = clipkit.replace(out, own_clips())
    clipkit.check(out, BONE_ORDER, NAME)
    missing = [c.name for c in out if not any(b.startswith("tail") for b in c.filled_keys())
               and not any(b.startswith("tail") for w in c.waves for b in w(0.1))]
    assert not missing, f"clips without a tail: {missing}"
    return out
