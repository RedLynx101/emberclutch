"""The people's clip library (romfs/anims/person.eca): one set for everyone, authored with the
dragons' clip tools (tools/anim/eca.py) on the people's skeleton (rig.BONES).

Keys are (pitch, yaw, roll) degree deltas on top of the rest pose (standing at ease), about the
ARMATURE's axes, as the dragons' clips are. For a person (facing -Y):
  pitch +   a hanging arm or leg swings FORWARD; an upright bone (spine, head) tips BACK
            (so a nod down or a lean forward is pitch -); a knee bends with leg_lo pitch -,
            an elbow with arm_lo pitch +; a foot's toes lift with pitch +
  yaw +     turns toward +X (the person's own left)
  roll +    an upright bone's top leans toward -X; a hanging limb's tip swings toward +X, so
            for "_R" limbs (at +X) roll + lifts the arm out sideways; "name*" mirrors both sides
The free hand (waving, petting, talking, picking up) is hand_R; villagers hold their props in hand_L,
which every clip keeps level in the world (SteadyClip): its delta is the exact inverse of the chain
above it, so a staff, a lantern pole, a bucket or a clipboard stays upright while the arm carries it
(and a player's round hand looks the same either way).
Root tracks are (forward, up) in metres; walk and run carry footstep markers at each heel strike.
"""
import math
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "anim"))
from eca import Clip, q_mul  # noqa: E402

LEG = 0.315  # hip joint height of the standard (player) body: speeds are for it
CLIPS = []
STEADY_CHAIN = ("hips", "spine", "chest", "arm_up_L", "arm_lo_L")


class SteadyClip(Clip):
    """A clip whose hand_L keeps its rest orientation in the world: world rotation of a bone =
    the product of the armature-axis deltas down its chain (then its rest), so hand_L's delta is
    the inverse of hips * spine * chest * arm_up_L * arm_lo_L at every frame."""

    def sample_q(self, bone, t):
        if bone != "hand_L":
            return super().sample_q(bone, t)
        q = (1.0, 0.0, 0.0, 0.0)
        for b in STEADY_CHAIN:
            q = q_mul(q, super().sample_q(b, t))
        return (q[0], -q[1], -q[2], -q[3])


def clip(*args, **kwargs):
    c = SteadyClip(*args, **kwargs)
    CLIPS.append(c)
    return c


def merge(*poses):
    out = {}
    for p in poses:
        for bone, v in p.items():
            a = out.get(bone, (0.0, 0.0, 0.0))
            out[bone] = tuple(x + y for x, y in zip(a, v))
    return out


def sin01(t, period, phase=0.0):
    return math.sin(2 * math.pi * (t / period + phase))


def ease(x):
    x = max(0.0, min(1.0, x))
    return x * x * (3 - 2 * x)


# ------------------------------------------------------------------------------ poses
STAND = {"arm_lo*": (8, 0, 0), "arm_up*": (0, 0, 2)}
# Seated at knee height: the free hand on the lap, the prop hand resting on the seat beside the hip.
SIT = {"leg_up*": (86, 0, 3), "leg_lo*": (-86, 0, 0), "foot*": (2, 0, 0), "spine": (2, 0, 0),
       "arm_up_R": (26, 0, -8), "arm_lo_R": (42, 0, 0), "hand_R": (-10, 0, 0),
       "arm_up_L": (4, 0, -14), "arm_lo_L": (14, 0, 0), "head": (-3, 0, 0)}
# Astride: thighs out and forward over the back, shins hanging down its flanks (solved against
# target directions: thigh (0.84, -0.44, -0.33), shin (0.57, 0.18, -0.8) for the _R side); the
# shins hang clear of a back up to about 0.55 m across at the seat (wider: seat the rider higher).
RIDE = {"leg_up*": (50, 0, 55), "leg_lo*": (-45, 0, 0), "foot*": (30, 0, 0),
        "hips": (0, 0, 0), "spine": (-9, 0, 0), "chest": (-4, 0, 0), "neck": (2, 0, 0), "head": (9, 0, 0),
        "arm_up*": (58, 0, -10), "arm_lo*": (38, 0, 0), "hand*": (-12, 0, 0)}
KNEEL = {"leg_up_R": (84, 0, 4), "leg_lo_R": (-86, 0, 0), "foot_R": (2, 0, 0),
         "leg_up_L": (-6, 0, -4), "leg_lo_L": (-96, 0, 0), "foot_L": (-6, 0, 0),
         "spine": (-8, 0, 0), "chest": (-4, 0, 0), "neck": (-2, 0, 0), "head": (-10, 0, 3),
         "arm_up_R": (62, 0, 6), "arm_lo_R": (20, 0, 0), "hand_R": (-24, 0, 0),
         "arm_up_L": (30, 0, -4), "arm_lo_L": (40, 0, 0), "hand_L": (-8, 0, 0)}
# Holding a find up in the free hand, the other at ease.
HOLD = {"arm_up_R": (24, 0, -16), "arm_lo_R": (86, 0, -8), "hand_R": (-6, 0, 0), "arm_up_L": (0, 0, -4),
        "arm_lo_L": (10, 0, 0), "head": (-9, 0, 4), "spine": (3, 0, 0)}
# Arms are short beside the big head: "up" is a V out to the sides with the forearms raised,
# so the hands stay clear of the head (straight up, they'd sink into it).
ARMS_UP = {"arm_up*": (-12, 0, 104), "arm_lo*": (0, 0, 24), "hand*": (0, 0, 8)}


# ------------------------------------------------------------------------------ layers
def breathe(amount=1.0, period=3.0):
    def fn(t):
        s = sin01(t, period) * amount
        return {"chest": (1.3 * s, 0, 0), "spine": (-0.5 * s, 0, 0), "head": (-0.9 * s, 0, 0),
                "arm_up_R": (0, 0, 1.2 * s), "arm_up_L": (0, 0, -1.2 * s)}
    return fn


def gait(period, amp, knee, arm, stance=0.5, bounce_lean=-4.0, hip_yaw=5.0, knee_stance=6.0, elbow=12.0,
         elbow_swing=8.0, head_counter=3.0, arm_out=4.0):
    """Legs with a straight-line stance (feet stay put while planted) and an eased swing; arms
    swinging against the legs; the hips turning with the stride, the chest against them."""
    def leg_angle(u):
        if u < stance:  # planted: sweeps back at a steady rate
            return amp * (1 - 2 * u / stance)
        s = (u - stance) / (1 - stance)
        return -amp * math.cos(math.pi * s)

    def knee_angle(u):
        if u < stance:
            return -knee_stance * math.sin(math.pi * u / stance)
        s = (u - stance) / (1 - stance)
        return -knee * math.sin(math.pi * s) ** 1.2

    def foot_angle(u):
        if u < stance:
            return 0.0
        s = (u - stance) / (1 - stance)
        return 12 * math.sin(math.pi * s)

    def fn(t):
        out = {}
        for side, ph in (("R", 0.0), ("L", 0.5)):
            u = (t / period + ph) % 1.0
            out[f"leg_up_{side}"] = (leg_angle(u), 0, 0)
            out[f"leg_lo_{side}"] = (knee_angle(u), 0, 0)
            out[f"foot_{side}"] = (foot_angle(u), 0, 0)
            c = math.cos(2 * math.pi * u)
            sgn = 1 if side == "R" else -1
            out[f"arm_up_{side}"] = (-arm * c, 0, sgn * arm_out)
            out[f"arm_lo_{side}"] = (elbow + elbow_swing * max(0.0, -c), 0, 0)
        cr = math.cos(2 * math.pi * t / period)
        b = math.cos(4 * math.pi * t / period)
        out["hips"] = (0, -hip_yaw * cr, 1.5 * math.sin(2 * math.pi * t / period))
        out["spine"] = (bounce_lean + 1.5 * b, 0, 0)
        out["chest"] = (0, 0.8 * hip_yaw * cr, 0)
        out["head"] = (head_counter - 1.2 * b, 0.2 * hip_yaw * cr, 0)
        return out
    return fn


# ------------------------------------------------------------------------------ the clips
idle = clip("idle", 3.0, loop=True).pose(0.0, STAND).wave(breathe())
idle.wave(lambda t: {"hips": (0, 0, 0.8 * sin01(t, 3.0, 0.25)), "head": (0, 0, 1.5 * sin01(t, 3.0, 0.1))})

look = clip("look_around", 3.0).pose(0.0, STAND)
look.pose(0.7, merge(STAND, {"head": (4, 34, -4), "neck": (0, 10, 0), "chest": (0, 8, 0)}))
look.pose(1.3, merge(STAND, {"head": (2, 36, -5), "neck": (0, 10, 0), "chest": (0, 8, 0)}))
look.pose(2.0, merge(STAND, {"head": (4, -34, 4), "neck": (0, -10, 0), "chest": (0, -8, 0)}))
look.pose(2.5, merge(STAND, {"head": (2, -36, 5), "neck": (0, -10, 0), "chest": (0, -8, 0)}))
look.pose(3.0, STAND).wave(breathe(0.6))

WALK_P, WALK_A = 0.8, 26.0
walk = clip("walk", WALK_P, loop=True, speed=round(4 * LEG * math.sin(math.radians(WALK_A)) / WALK_P, 3))
walk.wave(gait(WALK_P, WALK_A, 44, 20))
walk.event(0.0, "footstep").event(WALK_P / 2, "footstep")

RUN_P, RUN_A, RUN_STANCE = 0.5, 32.0, 0.36
run = clip("run", RUN_P, loop=True,
           speed=round(2 * LEG * 0.95 * math.sin(math.radians(RUN_A)) / (RUN_STANCE * RUN_P), 3))
run.wave(gait(RUN_P, RUN_A, 92, 38, stance=RUN_STANCE, bounce_lean=-12, hip_yaw=7, knee_stance=14, elbow=72,
              elbow_swing=14, head_counter=9, arm_out=9))
for _k in range(10):  # a bouncy run: up between the steps, down on each landing
    _t = _k * RUN_P / 10
    run.root(_t, 0.0, 0.055 * (0.5 - 0.5 * math.cos(4 * math.pi * (_t - 0.09) / RUN_P)))
run.event(0.0, "footstep").event(RUN_P / 2, "footstep")

WAVE_UP = {"arm_up_R": (-22, 0, 94), "arm_lo_R": (0, 0, 38), "hand_R": (0, 0, 6), "chest": (0, 0, 4),
           "head": (0, 0, -6), "arm_up_L": (2, 0, -2), "arm_lo_L": (10, 0, 0)}
wave = clip("wave", 2.0).pose(0.0, STAND)
wave.pose(0.32, WAVE_UP)
for _i, _t in enumerate((0.52, 0.74, 0.96, 1.18, 1.4)):
    _s = 1 if _i % 2 == 0 else -1
    wave.pose(_t, merge(WAVE_UP, {"arm_lo_R": (0, 0, 22 * _s), "hand_R": (0, 0, 12 * _s)}))
wave.pose(1.6, WAVE_UP).pose(2.0, STAND)

talk = clip("talk", 2.4, loop=True)
talk.pose(0.0, merge(STAND, {"arm_up_R": (20, 0, 10), "arm_lo_R": (56, 0, 6), "hand_R": (-8, 0, 0),
                             "arm_up_L": (8, 0, -3), "arm_lo_L": (26, 0, 0), "head": (-2, 4, 3), "chest": (0, 4, 0)}))
talk.pose(0.6, merge(STAND, {"arm_up_R": (28, 0, 18), "arm_lo_R": (70, 0, -4), "hand_R": (-16, 0, 6),
                             "arm_up_L": (12, 0, -6), "arm_lo_L": (34, 0, 0), "head": (-6, 2, 1), "chest": (0, 2, 0)}))
talk.pose(1.2, merge(STAND, {"arm_up_R": (16, 0, 8), "arm_lo_R": (48, 0, 10), "hand_R": (-4, 0, -4),
                             "arm_up_L": (20, 0, -12), "arm_lo_L": (52, 0, 4), "hand_L": (-10, 0, 0),
                             "head": (-1, -4, -3), "chest": (0, -4, 0)}))
talk.pose(1.8, merge(STAND, {"arm_up_R": (24, 0, 14), "arm_lo_R": (62, 0, 2), "hand_R": (-12, 0, 0),
                             "arm_up_L": (14, 0, -8), "arm_lo_L": (40, 0, 2), "head": (-5, 0, 2), "chest": (0, 0, 0)}))
talk.wave(breathe(0.7, 2.4))

nod = clip("nod", 1.0).pose(0.0, STAND)
nod.pose(0.2, merge(STAND, {"head": (-17, 0, 0), "neck": (-4, 0, 0)}))
nod.pose(0.4, merge(STAND, {"head": (3, 0, 0)}))
nod.pose(0.6, merge(STAND, {"head": (-13, 0, 0), "neck": (-3, 0, 0)}))
nod.pose(0.82, STAND).pose(1.0, STAND)

CROUCH = {"leg_up*": (30, 0, 0), "leg_lo*": (-58, 0, 0), "foot*": (26, 0, 0), "spine": (-10, 0, 0),
          "arm_up*": (-22, 0, 6), "arm_lo*": (14, 0, 0)}
cheer = clip("cheer", 1.6).pose(0.0, STAND)
cheer.pose(0.22, CROUCH)
cheer.pose(0.46, merge(ARMS_UP, {"leg_up*": (-4, 0, 0), "leg_lo*": (-12, 0, 0), "foot*": (-24, 0, 0),
                                 "spine": (7, 0, 0), "head": (12, 0, 0)}))
cheer.pose(0.7, merge(ARMS_UP, {"leg_up*": (20, 0, 0), "leg_lo*": (-38, 0, 0), "foot*": (18, 0, 0),
                                "spine": (-2, 0, 0), "head": (8, 0, 0)}))
cheer.pose(0.9, merge(ARMS_UP, {"arm_up*": (0, 0, 8), "arm_lo*": (-6, 0, 0), "head": (10, 0, 0)}))
cheer.pose(1.1, merge(ARMS_UP, {"arm_up*": (0, 0, -8), "arm_lo*": (14, 0, 0), "head": (6, 0, 0)}))
cheer.pose(1.6, STAND)
cheer.root(0.0).root(0.26).root(0.47, 0.0, 0.14).root(0.68).root(1.6)
cheer.event(0.68, "land")

crouch_pet = clip("crouch_pet", 2.0, loop=True)
for _t, _a in ((0.0, 0.0), (0.5, 1.0), (1.0, 0.0), (1.5, 1.0)):
    crouch_pet.pose(_t, merge(KNEEL, {"arm_up_R": (12 * _a, 0, 0), "arm_lo_R": (-12 * _a, 0, 0),
                                      "hand_R": (10 * _a, 0, 0), "head": (0, 0, 3 - 6 * _a)}))
crouch_pet.wave(breathe(0.6, 2.0))

mount = clip("mount", 1.2).pose(0.0, STAND)
mount.pose(0.2, merge(CROUCH, {"arm_up*": (-26, 0, 6)}))
mount.pose(0.46, {"leg_up_L": (58, 0, -62), "leg_lo_L": (-40, 0, 0), "leg_up_R": (46, 0, 8), "leg_lo_R": (-80, 0, 0),
                  "foot*": (10, 0, 0), "arm_up*": (78, 0, -6), "arm_lo*": (18, 0, 0), "spine": (-14, 0, 0),
                  "head": (10, 0, 0)})
mount.pose(0.75, merge(RIDE, {"spine": (-8, 0, 0), "arm_up*": (8, 0, 0), "leg_up*": (0, 0, 6)}))
mount.pose(0.95, merge(RIDE, {"spine": (-6, 0, 0), "chest": (-4, 0, 0), "head": (-4, 0, 0)}))
mount.pose(1.2, RIDE)
mount.root(0.0).root(0.2).root(0.5, 0.0, 0.26).root(0.9).root(1.2)
mount.event(0.9, "land")

ride = clip("ride", 1.6, loop=True).pose(0.0, RIDE)
ride.wave(lambda t: {"spine": (2.5 * sin01(t, 0.8), 0, 0), "head": (-2.0 * sin01(t, 0.8), 0, 0),
                     "arm_up_R": (1.5 * sin01(t, 0.8, 0.2), 0, 0), "arm_up_L": (1.5 * sin01(t, 0.8, 0.2), 0, 0),
                     "leg_lo_R": (2 * sin01(t, 1.6), 0, 0), "leg_lo_L": (-2 * sin01(t, 1.6), 0, 0)})


def lean(sign):
    """Leaning into a bank: sign +1 toward +X (the rider's own left), -1 toward -X."""
    inside, outside = ("R", "L") if sign > 0 else ("L", "R")
    return merge(RIDE, {"spine": (0, 0, -13 * sign), "chest": (0, 0, -10 * sign), "neck": (0, 0, 4 * sign),
                        "head": (0, 13 * sign, 7 * sign),
                        f"arm_up_{inside}": (-12, 0, 0), f"arm_lo_{inside}": (-6, 0, 0),
                        f"arm_up_{outside}": (8, 0, 0), f"arm_lo_{outside}": (-10, 0, 0),
                        f"leg_up_{outside}": (0, 0, 6 * (1 if outside == "R" else -1))})


for _name, _sign in (("ride_lean_left", 1), ("ride_lean_right", -1)):
    _c = clip(_name, 1.6, loop=True).pose(0.0, lean(_sign))
    _c.wave(lambda t: {"spine": (2.0 * sin01(t, 0.8), 0, 0), "head": (-1.6 * sin01(t, 0.8), 0, 0)})

dismount = clip("dismount", 1.0).pose(0.0, RIDE)
dismount.pose(0.25, merge(RIDE, {"leg_up_R": (24, 0, -80), "leg_lo_R": (34, 0, 0), "spine": (-4, 0, 6),
                                 "arm_up*": (-20, 0, 0), "head": (0, -10, 0)}))
dismount.pose(0.45, {"leg_up*": (26, 0, 4), "leg_lo*": (-44, 0, 0), "foot*": (-10, 0, 0), "arm_up*": (-6, 0, 48),
                     "arm_lo*": (20, 0, 0), "spine": (-4, 0, 0), "head": (6, 0, 0)})
dismount.pose(0.7, merge(CROUCH, {"arm_up*": (12, 0, 20)}))
dismount.pose(1.0, STAND)
dismount.root(0.0).root(0.2).root(0.45, 0.0, 0.2).root(0.7).root(1.0)
dismount.event(0.7, "land")

sit = clip("sit", 1.0).pose(0.0, STAND)
sit.pose(0.35, {"leg_up*": (46, 0, 0), "leg_lo*": (-58, 0, 0), "foot*": (10, 0, 0), "spine": (-14, 0, 0),
                "arm_up*": (18, 0, 4), "arm_lo*": (20, 0, 0)})
sit.pose(0.7, merge(SIT, {"spine": (-5, 0, 0)}))
sit.pose(1.0, SIT)

sit_loop = clip("sit_loop", 3.0, loop=True).pose(0.0, SIT).wave(breathe(0.8))
sit_loop.wave(lambda t: {"leg_lo_R": (6 * sin01(t, 1.5), 0, 0), "leg_lo_L": (-6 * sin01(t, 1.5), 0, 0),
                         "head": (0, 0, 2 * sin01(t, 3.0))})

SHOCK = {"arm_up*": (26, 0, 52), "arm_lo*": (64, 0, 0), "hand*": (-10, 0, 0), "spine": (9, 0, 0),
         "chest": (3, 0, 0), "head": (11, 0, 0), "leg_up*": (-6, 0, 3), "leg_lo*": (-8, 0, 0)}
surprised = clip("surprised", 1.0).pose(0.0, STAND)
surprised.pose(0.12, SHOCK)
surprised.pose(0.3, merge(SHOCK, {"leg_up*": (6, 0, 0), "leg_lo*": (-6, 0, 0)}))
surprised.pose(0.6, merge(SHOCK, {"arm_up*": (-6, 0, -10), "spine": (-3, 0, 0), "head": (-2, 0, 0),
                                  "leg_up*": (6, 0, 0), "leg_lo*": (-6, 0, 0)}))
surprised.pose(1.0, STAND)
surprised.root(0.0).root(0.12, 0.0, 0.06).root(0.3).root(1.0)

pick_up = clip("pick_up", 1.4).pose(0.0, STAND)
REACH = {"leg_up*": (78, 0, 10), "leg_lo*": (-120, 0, 0), "foot*": (42, 0, 0), "spine": (-18, 0, 0),
         "chest": (-6, 0, 2), "head": (-6, 0, 0), "arm_up_R": (60, 0, -6), "arm_lo_R": (12, 0, 0),
         "arm_up_L": (-8, 0, -16), "arm_lo_L": (22, 0, 0)}
pick_up.pose(0.4, REACH)
pick_up.pose(0.6, merge(REACH, {"arm_up_R": (-4, 0, -4), "arm_lo_R": (6, 0, 0)}))
pick_up.pose(0.95, {"leg_up*": (22, 0, 2), "leg_lo*": (-36, 0, 0), "foot*": (12, 0, 0), "spine": (-8, 0, 0),
                    "arm_up_R": (28, 0, -12), "arm_lo_R": (70, 0, -4), "arm_up_L": (-2, 0, -8),
                    "arm_lo_L": (14, 0, 0), "head": (-8, 0, 0)})
pick_up.pose(1.2, HOLD).pose(1.4, HOLD)


def by_name(name):
    for c in CLIPS:
        if c.name == name:
            return c
    raise KeyError(name)


# What each clip is for (people-kit.md), in the brief's order; sit_loop is an extra.
PURPOSE = {
    "idle": "standing at ease, breathing (loop)",
    "look_around": "a curious look left and right",
    "walk": "walking (loop; footsteps; speed for the standard body)",
    "run": "a bouncy run (loop; footsteps; a small up-bounce root track)",
    "wave": "waving hello with the free hand (hand_R)",
    "talk": "talking with gestures (loop)",
    "nod": "a double nod",
    "cheer": "arms up and a little hop (root up; land marker)",
    "crouch_pet": "kneeling to pet a dragon, stroking with hand_R (loop)",
    "mount": "climbing on: crouch, hop, a leg over, ending in the ride pose (hop root arc; land)",
    "ride": "seated astride, legs apart, holding on (loop)",
    "ride_lean_left": "riding, leaning into a bank toward +X, the rider's own left (loop)",
    "ride_lean_right": "riding, leaning into a bank toward -X, the rider's own right (loop)",
    "dismount": "a leg over and a hop down, landing to stand (hop root arc; land)",
    "sit": "sitting down onto a seat at knee height (ends seated; hand_R on the lap, hand_L beside the hip)",
    "sit_loop": "sitting (loop, feet swinging)",
    "surprised": "a startled jump back, hands up (use eyes variant 1)",
    "pick_up": "squatting to pick something up with hand_R, ending holding it up at the chest",
}
REQUIRED = ["idle", "look_around", "walk", "run", "wave", "talk", "nod", "cheer", "crouch_pet", "mount", "ride",
            "ride_lean_left", "ride_lean_right", "dismount", "sit", "surprised", "pick_up"]
