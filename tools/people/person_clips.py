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

RUN_P, RUN_A, RUN_STANCE = 0.7, 46.0, 0.36  # (run 21: longer strides, fewer of them, the same speed)
run = clip("run", RUN_P, loop=True,
           speed=round(2 * LEG * 0.95 * math.sin(math.radians(RUN_A)) / (RUN_STANCE * RUN_P), 3))
run.wave(gait(RUN_P, RUN_A, 104, 44, stance=RUN_STANCE, bounce_lean=-12, hip_yaw=7, knee_stance=14, elbow=72,
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


# ------------------------------------------------------------------------------ settings (workstream D)
# The valley's people at their doings: watchers clapping, a trainer cheering their dragon on in a
# duel (pointing it forward, a fist pump, hands to the face when it's hit, a slump when it loses, a
# polite bow), villagers at work by the hour (Maple tidying her stall, Bram scattering feed from his
# bucket, Wren writing on her clipboard, Pip flying his toy dragon about), fishing at the cove,
# sitting down on the ground at a viewpoint, a stretch, dozing at night.
GROUND = -0.21  # the root sat down on the ground: legs flat on it (the standard body; the game
                # scales root tracks by a body's hips)

CLAP_APART = {"arm_up*": (50, 0, 6), "arm_lo*": (88, 0, 4), "hand*": (0, 0, 0)}
CLAP_TOGETHER = {"arm_up*": (52, 0, -20), "arm_lo*": (90, 0, -44), "hand*": (0, 0, -6)}
clap = clip("clap", 0.8, loop=True)
for _t, _p in ((0.0, CLAP_APART), (0.2, CLAP_TOGETHER), (0.4, CLAP_APART), (0.6, CLAP_TOGETHER)):
    clap.pose(_t, merge(_p, {"spine": (2, 0, 0), "head": (4, 0, 0)}))
clap.wave(lambda t: {"spine": (1.5 * sin01(t, 0.4), 0, 0), "head": (-2.0 * sin01(t, 0.4, 0.1), 0, 0)})
clap.event(0.2, "thump").event(0.6, "thump")

sit_clap = clip("sit_clap", 0.8, loop=True)
_SIT_LEGS = {k: v for k, v in SIT.items() if k.startswith(("leg", "foot"))}
for _t, _p in ((0.0, CLAP_APART), (0.2, CLAP_TOGETHER), (0.4, CLAP_APART), (0.6, CLAP_TOGETHER)):
    sit_clap.pose(_t, merge(_SIT_LEGS, _p, {"spine": (4, 0, 0), "head": (6, 0, 0)}))
sit_clap.wave(lambda t: {"leg_lo_R": (5 * sin01(t, 0.8), 0, 0), "leg_lo_L": (-5 * sin01(t, 0.8), 0, 0)})

# On the ground, legs out in front, hands resting on the thighs, leaning back a touch.
SIT_GROUND = {"leg_up*": (84, 0, 6), "leg_lo*": (-12, 0, 0), "foot*": (18, 0, 0),
              "spine": (6, 0, 0), "chest": (2, 0, 0), "head": (-2, 0, 0),
              "arm_up*": (28, 0, 10), "arm_lo*": (34, 0, 0), "hand*": (-8, 0, 0)}
sit_ground = clip("sit_ground", 4.0, loop=True).pose(0.0, SIT_GROUND).wave(breathe(0.9, 4.0))
sit_ground.wave(lambda t: {"foot_R": (6 * max(0.0, sin01(t, 4.0)), 0, 0), "foot_L": (6 * max(0.0, sin01(t, 4.0, 0.5)), 0, 0),
                           "head": (0, 6 * sin01(t, 4.0, 0.2), 1.5 * sin01(t, 4.0))})
sit_ground.root(0.0, 0.0, GROUND)

# Dozing where they sit: chin on the chest, slow deep breaths, a bob as it nods further and
# catches itself.
DOZE = merge(SIT_GROUND, {"spine": (-4, 0, 0), "chest": (-4, 0, 0), "neck": (-4, 0, 0), "head": (-12, 0, 5),
                          "arm_up*": (-2, 0, -4), "arm_lo*": (6, 0, 0)})
doze = clip("doze", 5.0, loop=True)
doze.pose(0.0, DOZE).pose(2.6, merge(DOZE, {"head": (-6, 0, 2)})).pose(3.2, merge(DOZE, {"head": (-3, 0, 0)}))
doze.pose(3.6, merge(DOZE, {"head": (3, 0, -1)})).pose(4.3, DOZE)
doze.wave(breathe(1.6, 5.0))
doze.root(0.0, 0.0, GROUND)

# Dozing on their feet (a night at the stall): head drooped, a slow sway, a nod and a catch.
DOZE_STAND = merge(STAND, {"spine": (-3, 0, 0), "chest": (-2, 0, 0), "neck": (-4, 0, 0), "head": (-12, 0, 4),
                           "arm_up*": (4, 0, 0), "arm_lo*": (6, 0, 0)})
doze_stand = clip("doze_stand", 5.0, loop=True)
doze_stand.pose(0.0, DOZE_STAND).pose(2.8, merge(DOZE_STAND, {"head": (-19, 0, 5), "neck": (-6, 0, 0)}))
doze_stand.pose(3.3, merge(DOZE_STAND, {"head": (-4, 0, 0), "spine": (0, 0, 0)})).pose(4.2, DOZE_STAND)
doze_stand.wave(breathe(1.4, 5.0))
doze_stand.wave(lambda t: {"hips": (0, 0, 1.6 * sin01(t, 5.0)), "chest": (0, 0, -1.0 * sin01(t, 5.0))})

# A good stretch: arms up in a V, leaning back up on the toes, a yawn, and down again.
STRETCH_UP = merge(ARMS_UP, {"arm_up*": (12, 0, 118), "arm_lo*": (0, 0, 16), "spine": (10, 0, 0), "chest": (6, 0, 0),
                             "head": (14, 0, 0), "foot*": (-14, 0, 0), "leg_up*": (-3, 0, 0)})
stretch = clip("stretch", 2.4).pose(0.0, STAND)
stretch.pose(0.5, STRETCH_UP).pose(1.3, merge(STRETCH_UP, {"head": (20, 0, 0), "spine": (12, 0, 0)}))
stretch.pose(1.7, merge(STAND, {"arm_up*": (4, 0, 30), "arm_lo*": (20, 0, 0), "head": (4, 0, 0)}))
stretch.pose(2.4, STAND)
stretch.root(0.0).root(0.5, 0.0, 0.03).root(1.3, 0.0, 0.03).root(1.7).root(2.4)

# A trainer's fist pump for their dragon: the free hand up, pumped twice, a bounce in the knees.
PUMP_UP = {"arm_up_R": (26, 0, 62), "arm_lo_R": (20, 0, 76), "hand_R": (-10, 0, 0), "arm_up_L": (10, 0, -8),
           "arm_lo_L": (30, 0, 0), "spine": (4, 0, 0), "head": (6, 0, 0)}
PUMP_DOWN = merge(PUMP_UP, {"arm_up_R": (-6, 0, -26), "arm_lo_R": (0, 0, -30), "leg_up*": (14, 0, 0),
                            "leg_lo*": (-26, 0, 0), "foot*": (12, 0, 0), "spine": (-4, 0, 0)})
fist_pump = clip("fist_pump", 1.2).pose(0.0, STAND)
fist_pump.pose(0.2, PUMP_UP).pose(0.38, PUMP_DOWN).pose(0.56, PUMP_UP).pose(0.74, PUMP_DOWN).pose(0.92, PUMP_UP)
fist_pump.pose(1.2, STAND)
fist_pump.root(0.0).root(0.38, 0.0, -0.03).root(0.56).root(0.74, 0.0, -0.03).root(0.92).root(1.2)

# Sending their dragon in: a step, the free arm thrown forward, pointing at the foe.
POINT = {"arm_up_R": (88, 0, 6), "arm_lo_R": (6, 0, 0), "hand_R": (6, 0, 0), "arm_up_L": (-10, 0, -10),
         "arm_lo_L": (24, 0, 0), "spine": (-8, 0, 0), "chest": (0, 6, 0), "head": (6, -4, 0),
         "leg_up_R": (22, 0, 0), "leg_lo_R": (-10, 0, 0), "leg_up_L": (-12, 0, 0), "leg_lo_L": (-4, 0, 0)}
point = clip("point", 1.2).pose(0.0, STAND)
point.pose(0.18, merge(POINT, {"arm_up_R": (40, 0, 30), "arm_lo_R": (60, 0, 0)})).pose(0.34, POINT)
point.pose(0.8, POINT).pose(1.2, STAND)

# Hands to the cheeks as their dragon takes a hit: a lean back, a wince to either side.
FACE = {"arm_up*": (34, 0, 30), "arm_lo*": (140, 0, 24), "hand*": (-8, 0, 0), "spine": (6, 0, 0), "head": (6, 0, 0),
        "neck": (2, 0, 0)}
worried = clip("worried", 1.4).pose(0.0, STAND)
worried.pose(0.2, FACE).pose(0.5, merge(FACE, {"head": (4, 12, 4)})).pose(0.8, merge(FACE, {"head": (4, -12, -4)}))
worried.pose(1.05, FACE).pose(1.4, STAND)
worried.root(0.0).root(0.2, 0.02, 0.0).root(1.05, 0.02, 0.0).root(1.4)

# A loss: shoulders down, head hung, a sigh (it ends there; the scene eases them back).
SLUMP = merge(STAND, {"spine": (-8, 0, 0), "chest": (-4, 0, 0), "neck": (-4, 0, 0), "head": (-12, 0, 0),
                      "arm_up*": (6, 0, -4), "arm_lo*": (4, 0, 0), "leg_up*": (6, 0, 0), "leg_lo*": (-10, 0, 0),
                      "foot*": (4, 0, 0)})
slump = clip("slump", 1.8).pose(0.0, STAND)
slump.pose(0.3, merge(STAND, {"spine": (4, 0, 0), "chest": (4, 0, 0), "head": (8, 0, 0)}))  # a breath in
slump.pose(0.9, SLUMP).pose(1.8, merge(SLUMP, {"head": (-15, 0, 2)}))

# A polite bow to the other trainer.
BOW = merge(STAND, {"spine": (-26, 0, 0), "chest": (-10, 0, 0), "head": (-2, 0, 0), "arm_up*": (-10, 0, 0),
                    "arm_lo*": (4, 0, 0), "leg_up*": (4, 0, 0)})
bow = clip("bow", 1.4).pose(0.0, STAND).pose(0.4, BOW).pose(0.85, BOW).pose(1.4, STAND)

# Fishing: the rod held out in both hands (the cove draws it from the hands), a gentle jig.
FISH = {"arm_up_R": (44, 0, -8), "arm_lo_R": (46, 0, -10), "hand_R": (-10, 0, 0), "arm_up_L": (40, 0, 16),
        "arm_lo_L": (54, 0, 12), "spine": (-2, 0, 0), "chest": (0, -6, 0), "head": (-6, 0, 0),
        "leg_up_R": (8, 0, 0), "leg_lo_R": (-8, 0, 0)}
fish = clip("fish", 3.0, loop=True).pose(0.0, FISH).wave(breathe(0.7, 3.0))
fish.wave(lambda t: {"arm_lo_R": (5 * max(0.0, sin01(t, 1.5)) ** 3, 0, 0), "arm_lo_L": (5 * max(0.0, sin01(t, 1.5)) ** 3, 0, 0)})

cast = clip("cast", 1.2).pose(0.0, FISH)
cast.pose(0.35, {"arm_up_R": (-8, 0, 58), "arm_lo_R": (0, 0, 70), "hand_R": (0, 0, 10), "arm_up_L": (16, 0, 20),
                 "arm_lo_L": (70, 0, 10), "spine": (6, 0, 0), "chest": (0, 14, 0), "head": (4, 6, 0),
                 "leg_up_L": (10, 0, 0), "leg_lo_L": (-12, 0, 0)})
cast.pose(0.55, merge(FISH, {"arm_up_R": (84, 0, 0), "arm_lo_R": (10, 0, 0), "spine": (-10, 0, 0), "chest": (0, -10, 0),
                             "leg_up_R": (20, 0, 0), "leg_lo_R": (-12, 0, 0)}))
cast.pose(0.9, merge(FISH, {"arm_up_R": (54, 0, -6), "spine": (-4, 0, 0)})).pose(1.2, FISH)

# Bram scattering feed from the bucket in his other hand: a dip, then a fling out in an arc.
DIP = {"arm_up_R": (18, 0, -26), "arm_lo_R": (44, 0, -24), "hand_R": (-20, 0, 0), "arm_up_L": (12, 0, -10),
       "arm_lo_L": (40, 0, 0), "spine": (-8, 0, 0), "chest": (0, -12, 0), "head": (-10, -10, 0)}
FLING = {"arm_up_R": (64, 0, 34), "arm_lo_R": (14, 0, 6), "hand_R": (10, 0, 0), "arm_up_L": (10, 0, -10),
         "arm_lo_L": (40, 0, 0), "spine": (-2, 0, 0), "chest": (0, 12, 0), "head": (2, 12, 0)}
scatter = clip("scatter", 2.2, loop=True)
scatter.pose(0.0, DIP).pose(0.5, DIP).pose(0.9, FLING).pose(1.3, merge(FLING, {"arm_up_R": (52, 0, 48)}))
scatter.pose(1.8, merge(DIP, {"arm_up_R": (30, 0, 0), "arm_lo_R": (30, 0, -10)}))

# Wren writing on the clipboard held up in her other hand, looking up now and then.
BOARD = {"arm_up_L": (44, 0, 14), "arm_lo_L": (74, 0, 16), "arm_up_R": (40, 0, -18), "arm_lo_R": (82, 0, -34),
         "hand_R": (-14, 0, 0), "head": (-18, 0, 0), "neck": (-4, 0, 0), "spine": (-2, 0, 0)}
write = clip("write", 3.0, loop=True)
write.pose(0.0, BOARD).pose(1.6, BOARD).pose(1.9, merge(BOARD, {"head": (2, 10, 0), "neck": (0, 0, 0)}))
write.pose(2.5, merge(BOARD, {"head": (0, 8, 0), "neck": (0, 0, 0)})).pose(2.8, BOARD)
write.wave(lambda t: {"arm_lo_R": (0, 0, 4 * sin01(t, 0.3)), "hand_R": (3 * sin01(t, 0.15), 0, 0)} if t < 1.6 else {})

# Maple tidying her stall: both hands busy at the counter, in turn, a little lean in.
TIDY = {"arm_up*": (44, 0, -4), "arm_lo*": (50, 0, -8), "hand*": (-12, 0, 0), "spine": (-8, 0, 0), "head": (-12, 0, 0)}
tidy = clip("tidy", 2.4, loop=True).pose(0.0, TIDY)
tidy.wave(lambda t: {"arm_up_R": (8 * sin01(t, 1.2), 0, 6 * sin01(t, 1.2, 0.25)),
                     "arm_up_L": (8 * sin01(t, 1.2, 0.5), 0, -6 * sin01(t, 1.2, 0.75)),
                     "chest": (0, 5 * sin01(t, 2.4), 0), "head": (0, 6 * sin01(t, 2.4, 0.1), 0)})

# Pip flying his toy dragon about: the toy hand swooping up and round, a bounce, eyes on it.
fly_toy = clip("fly_toy", 2.0, loop=True)
for _t, _u in ((0.0, 0.0), (0.5, 1.0), (1.0, 0.3), (1.5, 1.0)):
    fly_toy.pose(_t, {"arm_up_L": (110 + 40 * _u, 0, -10 - 30 * _u), "arm_lo_L": (20 - 20 * _u, 0, 0),
                      "arm_up_R": (10, 0, 16 + 10 * _u), "arm_lo_R": (30, 0, 0),
                      "head": (8 + 10 * _u, -14 + 8 * _u, 0), "chest": (0, -8 + 6 * _u, 0), "spine": (4, 0, 0)})
fly_toy.root(0.0).root(0.25, 0.0, 0.04).root(0.5).root(0.75, 0.0, 0.04).root(1.0).root(1.25, 0.0, 0.04).root(1.5).root(1.75, 0.0, 0.04)


# Swimming (1.0, D121): a breaststroke, the body leaning into the water with the head held up and
# the legs kicking behind; and treading water, upright, the arms sculling and the legs cycling. The
# valley floats you with the water at your shoulders (core/walker: kFloat).
SWIM = {"spine": (-26, 0, 0), "chest": (-10, 0, 0), "neck": (10, 0, 0), "head": (24, 0, 0),
        "leg_up*": (-16, 0, 4), "leg_lo*": (-18, 0, 0), "foot*": (-30, 0, 0), "hand*": (-10, 0, 0)}
swim = clip("swim", 1.2, loop=True, speed=1.6)
for _t, _up, _lo in ((0.0, (82, 0, 10), (6, 0, 0)),     # reaching forward
                     (0.35, (64, 0, 56), (22, 0, 0)),   # the sweep out
                     (0.7, (30, 0, 24), (92, 0, 0)),    # the pull in to the chest
                     (1.0, (70, 0, 8), (42, 0, 0))):    # pushing forward again
    swim.pose(_t, merge(SWIM, {"arm_up*": _up, "arm_lo*": _lo}))
swim.wave(lambda t: {"leg_up_R": (14 * sin01(t, 0.6), 0, 0), "leg_up_L": (-14 * sin01(t, 0.6), 0, 0),
                     "leg_lo_R": (-8 * max(0.0, sin01(t, 0.6)), 0, 0), "leg_lo_L": (-8 * max(0.0, -sin01(t, 0.6)), 0, 0)})
swim.root(0.0).root(0.4, 0.0, -0.03).root(0.8, 0.0, 0.02)
swim.event(0.55, "footstep")  # (the pull: the valley plays a soft splash)

TREAD = {"spine": (-5, 0, 0), "head": (5, 0, 0), "hand*": (-6, 0, 0)}
tread = clip("tread", 1.6, loop=True).pose(0.0, TREAD)
tread.wave(lambda t: {"arm_up_R": (32 + 6 * sin01(t, 0.8), 0, 40 + 12 * sin01(t, 0.8)),
                      "arm_up_L": (32 + 6 * sin01(t, 0.8), 0, -(40 + 12 * sin01(t, 0.8))),
                      "arm_lo_R": (40 + 12 * sin01(t, 0.8, 0.25), 0, 0), "arm_lo_L": (40 + 12 * sin01(t, 0.8, 0.25), 0, 0),
                      "leg_up_R": (22 + 18 * sin01(t, 1.6), 0, 4), "leg_up_L": (22 - 18 * sin01(t, 1.6), 0, -4),
                      "leg_lo_R": (-34 - 18 * sin01(t, 1.6, 0.25), 0, 0), "leg_lo_L": (-34 + 18 * sin01(t, 1.6, 0.25), 0, 0)})
tread.root(0.0).root(0.8, 0.0, 0.025)


# ------------------------------------------------------------------------------ feelings (D138)
# A line's feeling in the body (app/emotes clipFor): played once as the line starts, then back to
# talking. Each starts and ends standing at ease.
def window(t0, t1, fn):
    """A layer only between t0 and t1 (eased in and out over a tenth of a second)."""
    def f(t):
        if t <= t0 or t >= t1:
            return {}
        k = min(1.0, (t - t0) / 0.1, (t1 - t) / 0.1)
        return {b: tuple(x * k for x in v) for b, v in fn(t).items()}
    return f


def gesture(name, length, keys, waves=(), roots=None, events=()):
    """A one-shot from STAND through `keys` [(t, pose)] back to STAND at `length`."""
    c = clip(name, length).pose(0.0, STAND)
    for t, pose in keys:
        c.pose(t, pose)
    c.pose(length, STAND)
    for w in waves:
        c.wave(w)
    if roots:
        for r in roots:
            c.root(*r)
    for t, e in events:
        c.event(t, e)
    return c


CROSS = {"arm_up*": (34, 0, -18), "arm_lo*": (108, 0, -62), "hand*": (0, 0, -8)}  # arms folded

LAUGH = {"spine": (8, 0, 0), "chest": (3, 0, 0), "head": (14, 0, 0), "arm_up_R": (30, 0, -18), "arm_lo_R": (84, 0, -30),
         "arm_up_L": (12, 0, -10), "arm_lo_L": (40, 0, 0)}
gesture("laugh", 1.6, [(0.2, LAUGH), (1.3, LAUGH)],
        waves=[window(0.2, 1.3, lambda t: {"chest": (3.5 * sin01(t, 0.16), 0, 0), "spine": (2.0 * sin01(t, 0.16), 0, 0),
                                           "head": (3.0 * sin01(t, 0.16, 0.2), 0, 0)})])

HUFF = merge(CROSS, {"head": (8, 26, -4), "neck": (2, 8, 0), "spine": (3, 0, 0)})
gesture("huff", 1.6, [(0.25, HUFF), (1.25, HUFF)], roots=[(0.0,), (0.25, 0.0, 0.015), (0.35,), (1.6,)])

STOMP_UP = {"leg_up_R": (34, 0, 0), "leg_lo_R": (-46, 0, 0), "arm_up*": (-12, 0, 14), "arm_lo*": (34, 0, 0),
            "spine": (-6, 0, 0), "head": (-8, 0, 0)}
STOMP_DOWN = {"leg_up_R": (2, 0, 0), "leg_lo_R": (-4, 0, 0), "arm_up*": (-16, 0, 18), "arm_lo*": (40, 0, 0),
              "spine": (-8, 0, 0), "head": (-10, 0, 0)}
gesture("stomp", 1.3, [(0.25, STOMP_UP), (0.4, STOMP_DOWN), (0.6, STOMP_UP), (0.75, STOMP_DOWN), (1.0, STOMP_DOWN)],
        events=[(0.4, "thump"), (0.75, "thump")])

THINK = {"arm_up_R": (30, 0, -20), "arm_lo_R": (124, 0, -34), "hand_R": (-14, 0, 0), "arm_up_L": (20, 0, -12),
         "arm_lo_L": (72, 0, -44), "head": (7, -8, 8), "spine": (2, 0, 0)}
gesture("think", 2.2, [(0.35, THINK), (1.8, merge(THINK, {"head": (2, 16, -2)}))],
        waves=[window(0.35, 1.8, lambda t: {"arm_lo_R": (3 * sin01(t, 0.6), 0, 0)})])

SHY = {"arm_up*": (-22, 0, -6), "arm_lo*": (34, 0, -22), "head": (-12, -6, 9), "neck": (-3, 0, 0), "spine": (-2, 0, 0),
       "foot_R": (12, 0, 0), "leg_up_R": (6, 0, -4)}
gesture("shy", 2.2, [(0.3, SHY), (1.8, SHY)],
        waves=[window(0.3, 1.8, lambda t: {"hips": (0, 0, 3.0 * sin01(t, 1.0)), "chest": (0, 0, -2.0 * sin01(t, 1.0)),
                                           "foot_R": (6 * max(0.0, sin01(t, 0.5)), 0, 0)})])

PROUD = {"arm_up*": (-6, 0, 40), "arm_lo*": (86, 0, -54), "hand*": (0, 0, -10), "spine": (7, 0, 0), "chest": (4, 0, 0),
         "head": (10, 0, 0)}
gesture("proud", 1.8, [(0.3, PROUD), (1.4, PROUD)], roots=[(0.0,), (0.3, 0.0, 0.012), (1.4, 0.0, 0.012), (1.8,)])

CRY = {"arm_up*": (42, 0, 26), "arm_lo*": (150, 0, 30), "hand*": (-10, 0, 0), "head": (-14, 0, 0), "neck": (-4, 0, 0),
       "spine": (-6, 0, 0), "chest": (-3, 0, 0)}
gesture("cry", 1.8, [(0.25, CRY), (1.45, CRY)],
        waves=[window(0.25, 1.45, lambda t: {"chest": (2.5 * sin01(t, 0.22), 0, 0), "head": (0, 0, 3 * sin01(t, 0.44))})])

LOVE = {"arm_up*": (30, 0, -14), "arm_lo*": (104, 0, -52), "hand*": (-6, 0, -10), "head": (4, 0, 10), "spine": (3, 0, 0)}
gesture("love", 2.0, [(0.25, LOVE), (1.6, LOVE)],
        waves=[window(0.25, 1.6, lambda t: {"hips": (0, 0, 4.0 * sin01(t, 0.9)), "head": (0, 0, 6.0 * sin01(t, 0.9)),
                                            "chest": (0, 0, -2.5 * sin01(t, 0.9))})],
        roots=[(0.0,), (0.15, 0.0, 0.04), (0.3,), (2.0,)])

SWOON = {"arm_up_R": (64, 0, 42), "arm_lo_R": (128, 0, 22), "hand_R": (-20, 0, 0), "arm_up_L": (6, 0, -56),
         "arm_lo_L": (22, 0, 0), "spine": (9, 0, 0), "chest": (4, 0, 0), "head": (12, 0, 8),
         "leg_up*": (10, 0, 0), "leg_lo*": (-18, 0, 0), "foot*": (8, 0, 0)}
gesture("swoon", 2.2, [(0.35, SWOON), (1.7, merge(SWOON, {"spine": (3, 0, 0), "head": (4, 0, 2)}))],
        roots=[(0.0,), (0.35, 0.03, -0.02), (1.7, 0.03, -0.02), (2.2,)])

COOL = merge(CROSS, {"hips": (0, 0, 6), "spine": (0, 0, -6), "head": (-4, -10, -6), "leg_up_L": (10, 0, -4),
                     "leg_lo_L": (-10, 0, 0)})
gesture("cool", 2.2, [(0.35, COOL), (1.8, merge(COOL, {"head": (-2, -4, -2)}))])

BOUNCE = {"arm_up*": (22, 0, 38), "arm_lo*": (112, 0, 0), "hand*": (-10, 0, 0), "spine": (4, 0, 0), "head": (6, 0, 0)}
gesture("bounce", 1.2, [(0.15, BOUNCE), (0.45, BOUNCE), (0.75, BOUNCE), (0.95, BOUNCE)],
        roots=[(0.0,), (0.15,), (0.3, 0.0, 0.06), (0.45,), (0.6, 0.0, 0.06), (0.75,), (1.2,)],
        events=[(0.45, "land"), (0.75, "land")])

SHRUG = {"arm_up*": (10, 0, 26), "arm_lo*": (72, 0, 40), "hand*": (-20, 0, 0), "head": (2, 0, 12), "chest": (3, 0, 0)}
gesture("shrug", 1.3, [(0.3, SHRUG), (0.9, SHRUG)])

gesture("sigh", 1.8, [(0.45, {"chest": (7, 0, 0), "head": (8, 0, 0), "arm_up*": (0, 0, 8)}),
                      (1.1, {"spine": (-6, 0, 0), "chest": (-4, 0, 0), "head": (-11, 0, 0), "arm_up*": (4, 0, -4)}),
                      (1.4, {"spine": (-5, 0, 0), "head": (-9, 0, 0)})])

YAWN = {"arm_up_R": (40, 0, -14), "arm_lo_R": (132, 0, -30), "hand_R": (-10, 0, 0), "arm_up_L": (-8, 0, 60),
        "arm_lo_L": (30, 0, 30), "head": (12, 0, 0), "spine": (5, 0, 0), "chest": (3, 0, 0)}
gesture("yawn", 2.0, [(0.4, YAWN), (1.4, merge(YAWN, {"head": (3, 0, 0)}))])

gesture("dizzy", 2.2, [(0.2, {"arm_up*": (6, 0, 26), "arm_lo*": (20, 0, 0)}), (2.0, {"arm_up*": (6, 0, 26), "arm_lo*": (20, 0, 0)})],
        waves=[window(0.2, 2.0, lambda t: {"spine": (3 * sin01(t, 0.9, 0.25), 0, 8 * sin01(t, 0.9)),
                                           "head": (6 * sin01(t, 0.9, 0.25), 0, 12 * sin01(t, 0.9)),
                                           "hips": (0, 0, -3 * sin01(t, 0.9))})])

FACEPALM = {"arm_up_R": (52, 0, -18), "arm_lo_R": (142, 0, -22), "hand_R": (-24, 0, 0), "head": (-14, 0, 0),
            "neck": (-4, 0, 0), "spine": (-4, 0, 0)}
gesture("facepalm", 1.6, [(0.3, FACEPALM), (1.2, merge(FACEPALM, {"head": (-2, 6, 0)}))])


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
    # settings (workstream D)
    "clap": "clapping at chest height, a little bob (loop; thump marker on each clap)",
    "sit_clap": "sat on a bench clapping, feet swinging (loop; seat as sit)",
    "sit_ground": "sat on the ground, legs out, hands on the thighs (loop; root down 0.21 m)",
    "doze": "dozing sat on the ground, chin down, a nod and a catch (loop; root down 0.21 m)",
    "doze_stand": "dozing on their feet, head drooped, a slow sway (loop)",
    "stretch": "arms up in a V, up on the toes, a yawn",
    "fist_pump": "the free hand pumped twice, knees bouncing (cheering a dragon on)",
    "point": "a step and the free arm thrown forward, pointing (sending a dragon in)",
    "worried": "hands to the cheeks, a lean back, a wince each way",
    "slump": "a breath in, then shoulders down and head hung (ends slumped)",
    "bow": "a polite bow",
    "fish": "holding a rod out in both hands, a gentle jig (loop)",
    "cast": "the rod back over the shoulder, whipped forward, ends as fish",
    "scatter": "a dip into the bucket in hand_L, a fling of feed out in an arc (loop)",
    "write": "writing on the clipboard held up in hand_L, looking up now and then (loop)",
    "tidy": "both hands busy at a counter in turn, leaning in (loop)",
    "fly_toy": "the toy in hand_L swooped up and round, bouncing, eyes on it (loop; root bob)",
    # 1.0 (D121)
    "swim": "a breaststroke, leaning into the water, head up, legs kicking (loop; footstep on the pull)",
    "tread": "treading water upright, arms sculling, legs cycling (loop; root bob)",
    # feelings (D138): once as a line with the feeling starts, then back to talking
    "laugh": "head back, a hand to the belly, shoulders shaking",
    "huff": "arms folded, chin up, head turned away",
    "stomp": "fists down, two stamps of a foot (thump markers)",
    "think": "a hand to the chin, the other under the elbow, head tilting",
    "shy": "hands behind the back, head down and tilted, a sway, a toe scuffing",
    "proud": "hands on the hips, chest out, chin up",
    "cry": "hands to the eyes, head down, shoulders shaking",
    "love": "hands clasped at the chest, a little hop, a sway",
    "swoon": "the back of a hand to the forehead, a dramatic lean back (Celestine)",
    "cool": "arms folded, weight on one hip, head tilted away (Rook)",
    "bounce": "two little hops, fists up (land markers)",
    "shrug": "arms out, palms up, head tilted",
    "sigh": "a breath in, then shoulders and head down",
    "yawn": "a hand to the mouth, the other arm stretched up, head back",
    "dizzy": "a wobbly sway in circles",
    "facepalm": "a hand to the face, head down",
}
REQUIRED = ["idle", "look_around", "walk", "run", "wave", "talk", "nod", "cheer", "crouch_pet", "mount", "ride",
            "ride_lean_left", "ride_lean_right", "dismount", "sit", "surprised", "pick_up",
            "clap", "sit_clap", "sit_ground", "doze", "doze_stand", "stretch", "fist_pump", "point", "worried", "slump",
            "bow", "fish", "cast", "scatter", "write", "tidy", "fly_toy", "swim", "tread",
            "laugh", "huff", "stomp", "think", "shy", "proud", "cry", "love", "swoon", "cool", "bounce", "shrug", "sigh",
            "yawn", "dizzy", "facepalm"]
