"""The dragon's animation clips (content inventory section 1.4). See eca.py for conventions:
keys are (pitch, yaw, roll) degree deltas on top of the idle pose, in armature axes;
"name*" keys both sides of the body.

Build:   python tools/anim/build_anims.py            -> romfs/anims/dragon.eca
Preview: blender -b -P tools/blender/preview_anims.py -- --clips walk --out <abs prefix>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from eca import Clip  # noqa: E402

CLIPS = []


def clip(*args, **kwargs):
    c = Clip(*args, **kwargs)
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


# ------------------------------------------------------------------------------ poses
# At rest the wings are lowered and swept back along the flanks, the finger fan closed
# (the model's rest pose has them raised in a V, for spreading and flying).
# Folded like a bird's wing (a Z): the upper arm back along the shoulder with the elbow near
# the back line, the forearm down and forward, the hand back along the side, every bone
# twisted so the membrane lies flat against the flank. Solved from bone directions by
# tools/blender/fold_solver.py (--variant zfold4, grown form; the hatchling's own solve is
# within a few degrees).
WINGS_FOLDED = {"wing_arm*": (-86, 133, 6), "wing_fore*": (-46, -126, -65),
                "wing_f1*": (39, 118, -73), "wing_f2*": (43, 63, -32), "wing_f3*": (25, 24, -5),
                "wing_f4*": (-1, -1, 4)}
WINGS_HALF = {"wing_arm*": (0, 20, -25), "wing_f1*": (12, 10, 0), "wing_f4*": (-4, -3, 0)}
WINGS_OPEN = {"wing_arm*": (0, -10, 10)}  # a little wider than the rest V

SIT = merge(WINGS_FOLDED, {
    "hips": (26, 0, 0),                      # tip back onto the haunches
    "leg_up*": (62, 0, 0), "leg_lo*": (-95, 0, 0), "foot*": (45, 0, 0),
    "arm_up*": (-24, 0, 0), "arm_lo*": (-4, 0, 0), "hand*": (-6, 0, 0),
    "tail1": (-30, 0, 0), "tail2": (-6, 12, 0), "tail3": (0, 18, 0), "tail4": (0, 22, 0),
    "neck1": (-12, 0, 0), "head": (-14, 0, 0),
})
LIE = merge(WINGS_FOLDED, {
    "arm_up*": (72, 0, 0), "arm_lo*": (-110, 0, 0), "hand*": (40, 0, 0),
    "leg_up*": (62, 0, 0), "leg_lo*": (-115, 0, 0), "foot*": (55, 0, 0),
    "neck1": (-16, 0, 0), "neck2": (-6, 0, 0), "head": (-8, 0, 0),
    "tail1": (-6, 0, 0), "tail2": (0, 14, 0), "tail3": (0, 20, 0), "tail4": (0, 24, 0),
})
CURL = merge(LIE, {
    "neck1": (-6, 32, 0), "neck2": (-4, 30, 0), "neck3": (0, 22, 0), "head": (-22, 10, 12),
    "tail1": (0, 28, 0), "tail2": (0, 30, 0), "tail3": (0, 32, 0), "tail4": (0, 30, 0),
})
# Front legs crouch so the chest comes down: babies' big heads reach the food without
# tipping below their feet.
EAT = merge(WINGS_FOLDED, {
    "neck1": (-40, 0, 0), "neck2": (-24, 0, 0), "neck3": (-10, 0, 0), "head": (-14, 0, 0),
    "arm_up*": (26, 0, 8), "arm_lo*": (-44, 0, 0), "hand*": (18, 0, 0), "chest": (-6, 0, 0), "hips": (-8, 0, 0),
})
BELLY_UP = merge(WINGS_HALF, {
    "hips": (0, 0, 165), "chest": (0, 0, 12),
    "arm_up*": (55, 0, 0), "arm_lo*": (-70, 0, 0), "leg_up*": (50, 0, 0), "leg_lo*": (-70, 0, 0),
    # the neck twists back so the head stays upright, looking at you
    "neck1": (10, 0, -60), "neck2": (0, 0, -50), "neck3": (0, 0, -30), "head": (15, 0, -20),
    "tail1": (0, 20, 0), "tail2": (0, 20, 0),
})
CROUCH_BODY = {
    "hips": (-8, 0, 0), "leg_up*": (40, 0, 0), "leg_lo*": (-70, 0, 0), "foot*": (30, 0, 0),
    "arm_up*": (38, 0, 0), "arm_lo*": (-62, 0, 0), "hand*": (24, 0, 0),
    "neck1": (-16, 0, 0), "head": (6, 0, 0), "tail1": (8, 0, 0),
}
CROUCH = merge(WINGS_HALF, CROUCH_BODY)          # excited: about to spring
CROUCH_FOLDED = merge(WINGS_FOLDED, CROUCH_BODY)  # settling down
SULK = merge(LIE, {
    "neck1": (-24, -18, 0), "neck2": (-12, -12, 0), "head": (-26, -10, -8),
    "tail1": (0, -30, 0), "tail2": (0, -30, 0), "tail3": (0, -28, 0), "tail4": (0, -26, 0),
})


# ------------------------------------------------------------------------------ layers
def breathe(amount=1.0, period=3.2):
    def fn(t):
        s = sin01(t, period) * amount
        return {"chest": (1.2 * s, 0, 0), "belly": (-0.6 * s, 0, 0), "neck1": (0.8 * s, 0, 0),
                "head": (-0.8 * s, 0, 0), "wing_arm_R": (0, 0, 1.5 * s), "wing_arm_L": (0, 0, -1.5 * s)}
    return fn


def tail_sway(amount=1.0, period=3.2):
    def fn(t):
        return {f"tail{k}": (0, amount * (2 + 2 * k) * sin01(t, period, -0.12 * k), 0) for k in range(1, 5)}
    return fn


def leg_cycle(period, amp_up, amp_bend, phases, bob=2.0):
    """Walking legs: each limb swings (pitch) around its phase and bends its lower joint
    while swinging forward. phases: {"arm_up_L": phase, ...} for the four upper bones."""
    def fn(t):
        out = {}
        for upper, ph in phases.items():
            side = upper[-2:]
            front = upper.startswith("arm")
            swing = sin01(t, period, -ph)
            lift = max(0.0, math.cos(2 * math.pi * (t / period - ph)))  # forward swing half
            lower, tip = ("arm_lo", "hand") if front else ("leg_lo", "foot")
            out[upper] = (amp_up * swing, 0, 0)
            out[lower + side] = (-amp_bend * lift, 0, 0)
            out[tip + side] = ((-0.6 if front else 0.4) * amp_bend * lift, 0, 0)
        b = sin01(t, period / 2)
        out["chest"] = (bob * 0.4 * b, 0, 1.5 * sin01(t, period))
        out["hips"] = (0, 2.5 * sin01(t, period, 0.25), 0)
        out["neck1"] = (-bob * b, 0, 0)
        out["head"] = (bob * 0.6 * b, 0, 0)
        return out
    return fn


def footsteps(c, period, phases, cycles=1):
    for k in range(cycles):
        for ph in phases.values():
            c.event((ph + 0.25 + k) * period % (period * cycles), "footstep")


def chomp(amount=20.0, period=0.6, at=0.15):
    """The jaw (pitch -: the chin drops) opens before each bite and snaps shut on it, at `at`
    and every `period` after."""
    def fn(t):
        u = ((t - at) / period) % 1.0
        return {"jaw": (-amount * math.sin(math.pi * (u - 0.5) / 0.5) ** 0.7 if u > 0.5 else 0.0, 0, 0)}
    return fn


def pant(amount=7.0, period=0.28):
    """A happy open mouth that bobs with the breath."""
    return lambda t: {"jaw": (-amount - 3 * sin01(t, period), 0, 0)}


WALK_PHASES = {"leg_up_L": 0.0, "arm_up_L": 0.25, "leg_up_R": 0.5, "arm_up_R": 0.75}  # lateral 4-beat
TROT_PHASES = {"arm_up_L": 0.0, "leg_up_R": 0.0, "arm_up_R": 0.5, "leg_up_L": 0.5}    # diagonal pairs


# ------------------------------------------------------------------------------ idle & locomotion
clip("idle", 3.2, loop=True).pose(0.0, WINGS_FOLDED).wave(breathe()).wave(tail_sway())

(clip("look_around", 3.0).pose(0.0, WINGS_FOLDED)
 .key(0.0, neck2=(0, 0, 0), neck3=(0, 0, 0), head=(0, 0, 0))
 .key(0.8, neck2=(4, 16, 0), neck3=(2, 14, 0), head=(6, 12, 6))
 .key(1.5, neck2=(4, 16, 0), neck3=(2, 14, 0), head=(6, 12, 6))
 .key(2.2, neck2=(2, -18, 0), neck3=(0, -16, 0), head=(4, -14, -6))
 .key(3.0, neck2=(0, 0, 0), neck3=(0, 0, 0), head=(0, 0, 0))
 .wave(breathe()).wave(tail_sway()).event(0.8, "sniff"))


SCRATCH = merge(WINGS_FOLDED, {
    "leg_up_R": (70, -18, 0), "leg_lo_R": (-40, 0, 0), "foot_R": (20, 0, 0), "hips": (4, 0, -8),
    "neck1": (-6, -24, 0), "neck2": (0, -20, 0), "head": (-6, -10, -18)})


def scratch_wave(t):
    s = sin01(t, 0.18) if 0.5 < t < 1.6 else 0.0
    return {"leg_lo_R": (10 * s, 0, 0), "foot_R": (14 * s, 0, 0)}


(clip("scratch", 2.1).pose(0.0, WINGS_FOLDED).pose(0.4, SCRATCH).pose(1.7, SCRATCH).pose(2.1, WINGS_FOLDED)
 .wave(scratch_wave))

# Locomotion speeds: the runtime measures each dragon's stance speed from these cycles
# (core/den_actor locomotionSpeed), so the feet stay planted; `speed` is only a fallback.
walk = clip("walk", 0.9, loop=True, speed=0.55).pose(0.0, WINGS_FOLDED)
walk.wave(leg_cycle(0.9, 28, 40, WALK_PHASES)).wave(tail_sway(1.2, 0.9))
footsteps(walk, 0.9, WALK_PHASES)

trot = clip("trot", 0.56, loop=True, speed=1.8).pose(0.0, WINGS_FOLDED)
trot.wave(leg_cycle(0.56, 22, 46, TROT_PHASES, bob=3.0)).wave(tail_sway(1.4, 0.56)).wave(pant(6.0, 0.28))
footsteps(trot, 0.56, {"a": 0.0, "b": 0.5})

shuffle = clip("shuffle", 0.9, loop=True).pose(0.0, WINGS_FOLDED)
shuffle.wave(leg_cycle(0.9, 10, 22, WALK_PHASES, bob=1.0))
footsteps(shuffle, 0.9, WALK_PHASES)

(clip("carry", 0.9, loop=True, speed=0.55).pose(0.0, merge(WINGS_FOLDED, {"neck1": (12, 0, 0), "head": (-8, 0, 0)}))
 .wave(leg_cycle(0.9, 28, 40, WALK_PHASES)).wave(tail_sway(1.6, 0.45)))

# ------------------------------------------------------------------------------ rest
clip("sit", 0.8).pose(0.0, WINGS_FOLDED).pose(0.8, SIT)
clip("sit_loop", 3.2, loop=True).pose(0.0, SIT).wave(breathe()).wave(tail_sway(0.5))
clip("lie_down", 1.1).pose(0.0, WINGS_FOLDED).pose(0.55, CROUCH_FOLDED).pose(1.1, LIE).event(0.9, "thump")
clip("lie_loop", 4.0, loop=True).pose(0.0, LIE).wave(breathe(1.2, 4.0)).wave(tail_sway(0.4, 4.0))
clip("curl_up", 1.4).pose(0.0, LIE).pose(1.4, CURL)
clip("sleep", 4.8, loop=True).pose(0.0, CURL).wave(breathe(1.8, 4.8))
(clip("wake", 2.6).pose(0.0, CURL).pose(0.6, LIE)
 .pose(1.3, merge(LIE, {"arm_up*": (20, 0, 0), "arm_lo*": (-20, 0, 0), "hips": (-14, 0, 0),
                        "leg_up*": (30, 0, 0), "leg_lo*": (-50, 0, 0), "neck1": (18, 0, 0), "head": (24, 0, 0),
                        "snout": (6, 0, 0), "jaw": (-32, 0, 0), "wing_arm*": (0, 10, -10)}))
 .pose(1.9, merge(WINGS_HALF, {"arm_up*": (22, 0, 0), "hips": (-10, 0, 0), "neck1": (14, 0, 0), "head": (10, 0, 0)}))
 .pose(2.6, WINGS_FOLDED).event(1.3, "yawn"))
(clip("yawn", 1.6).pose(0.0, WINGS_FOLDED)
 .pose(0.6, merge(WINGS_FOLDED, {"neck1": (14, 0, 0), "neck2": (8, 0, 0), "head": (30, 0, 0), "snout": (6, 0, 0),
                                 "jaw": (-30, 0, 0), "chest": (4, 0, 0)}))
 .pose(1.1, merge(WINGS_FOLDED, {"neck1": (14, 0, 0), "neck2": (8, 0, 0), "head": (30, 0, 0), "snout": (6, 0, 0),
                                 "jaw": (-34, 0, 0), "chest": (4, 0, 0)}))
 .pose(1.6, WINGS_FOLDED).event(0.6, "yawn"))
clip("nap_flop", 0.7).pose(0.0, WINGS_FOLDED).pose(0.7, LIE).event(0.55, "thump")

# ------------------------------------------------------------------------------ care reactions
eat = clip("eat", 1.2, loop=True).pose(0.0, EAT)
eat.wave(lambda t: {"head": (8 * max(0.0, sin01(t, 0.6)), 0, 0), "snout": (6 * max(0.0, sin01(t, 0.6)), 0, 0),
                    "neck1": (3 * sin01(t, 1.2), 0, 0)}).wave(chomp())
eat.event(0.15, "chomp").event(0.75, "chomp")

# Hatchling variants ("<name>_h" replaces "<name>" on the baby body, core/den_actor): a big
# baby head only needs a nod to reach the bowl.
EAT_BABY = merge(WINGS_FOLDED, {
    "neck1": (-18, 0, 0), "neck2": (-10, 0, 0), "head": (-26, 0, 0),
    "arm_up*": (30, 0, 8), "arm_lo*": (-50, 0, 0), "hand*": (20, 0, 0), "chest": (-8, 0, 0), "hips": (-6, 0, 0),
})
eat_h = clip("eat_h", 1.2, loop=True).pose(0.0, EAT_BABY)
eat_h.wave(lambda t: {"head": (8 * max(0.0, sin01(t, 0.6)), 0, 0), "neck1": (3 * sin01(t, 1.2), 0, 0)}).wave(chomp(24))
eat_h.event(0.15, "chomp").event(0.75, "chomp")
(clip("fav_wiggle", 1.6).pose(0.0, WINGS_FOLDED)
 .pose(0.3, merge(WINGS_HALF, {"neck1": (10, 0, 0), "head": (12, 0, 0), "jaw": (-16, 0, 0)}))
 .pose(1.3, merge(WINGS_HALF, {"neck1": (10, 0, 0), "head": (12, 0, 0), "jaw": (-16, 0, 0)})).pose(1.6, WINGS_FOLDED)
 .wave(lambda t: {"hips": (0, 7 * sin01(t, 0.4), 0), "chest": (0, -5 * sin01(t, 0.4), 0),
                  **{f"tail{k}": (0, 14 * sin01(t, 0.3, -0.1 * k), 0) for k in range(1, 5)}})
 .root(0.0).root(0.55, up=0.0).root(0.75, up=0.25).root(0.95, up=0.0).root(1.6).event(0.95, "land"))
(clip("pet_head", 1.6, loop=True)
 .pose(0.0, merge(WINGS_FOLDED, {"neck1": (6, 0, 0), "neck3": (4, 0, 6), "head": (-10, 6, 16), "jaw": (-7, 0, 0)}))
 .wave(lambda t: {"head": (0, 4 * sin01(t, 1.6), 4 * sin01(t, 1.6)), "neck2": (0, 3 * sin01(t, 1.6, 0.1), 0)})
 .wave(tail_sway(0.8, 0.8)).event(0.4, "purr"))
(clip("pet_chin", 1.6, loop=True)
 .pose(0.0, merge(WINGS_FOLDED, {"neck1": (14, 0, 0), "neck2": (8, 0, 0), "head": (26, 0, 0), "jaw": (-9, 0, 0)}))
 .wave(lambda t: {"head": (4 * sin01(t, 1.6), 0, 0), "jaw": (-3 * sin01(t, 1.6, 0.2), 0, 0)})
 .wave(tail_sway(0.8, 0.8)).event(0.4, "purr"))
clip("roll_over", 1.1).pose(0.0, WINGS_FOLDED).pose(0.45, LIE).pose(1.1, BELLY_UP).event(0.8, "thump")
(clip("belly_rub", 1.2, loop=True).pose(0.0, BELLY_UP)
 .wave(lambda t: {"leg_up_L": (10 * sin01(t, 0.6), 0, 0), "leg_up_R": (10 * sin01(t, 0.6, 0.5), 0, 0),
                  "arm_up_L": (8 * sin01(t, 0.6, 0.25), 0, 0), "arm_up_R": (8 * sin01(t, 0.6, 0.75), 0, 0),
                  "hips": (0, 0, 6 * sin01(t, 1.2))})
 .wave(pant(10.0, 0.6)).event(0.3, "purr"))


def shake_wave(t):
    k = max(0.0, 1.0 - abs(t - 0.5) / 0.4)
    s = sin01(t, 0.12) * k
    return {"chest": (0, 0, 18 * s), "hips": (0, 0, -12 * s), "neck1": (0, 0, 14 * s), "head": (0, 0, 16 * s),
            "wing_arm_R": (0, 0, 12 * s), "wing_arm_L": (0, 0, -12 * s)}


clip("shake", 1.0).pose(0.0, WINGS_FOLDED).pose(1.0, WINGS_FOLDED).wave(shake_wave).event(0.4, "shake")
(clip("hop", 0.9).pose(0.0, WINGS_FOLDED).pose(0.25, CROUCH)
 .pose(0.45, merge(WINGS_OPEN, {"arm_up*": (30, 0, 0), "arm_lo*": (-50, 0, 0), "leg_up*": (-10, 0, 0),
                                "neck1": (10, 0, 0), "head": (10, 0, 0), "tail1": (-10, 0, 0)}))
 .pose(0.7, CROUCH).pose(0.9, WINGS_FOLDED)
 .root(0.0).root(0.25).root(0.45, up=0.55).root(0.65).root(0.9).event(0.65, "land"))

# ------------------------------------------------------------------------------ play
(clip("pounce", 1.4).pose(0.0, WINGS_FOLDED).pose(0.35, CROUCH).pose(0.65, CROUCH)
 .pose(0.85, merge(WINGS_OPEN, {"arm_up*": (40, 0, 0), "leg_up*": (-30, 0, 0), "leg_lo*": (10, 0, 0),
                                "neck1": (-6, 0, 0), "head": (6, 0, 0), "jaw": (-20, 0, 0)}))
 .pose(1.05, CROUCH).pose(1.4, WINGS_FOLDED)
 .wave(lambda t: {"hips": (0, 6 * sin01(t, 0.15), 0) if 0.35 < t < 0.65 else (0, 0, 0)})
 .root(0.0).root(0.65).root(0.85, up=0.45).root(1.05).root(1.4)  # the den behavior moves it forward
 .event(1.05, "land"))
(clip("tail_wag", 0.5, loop=True).pose(0.0, merge(WINGS_FOLDED, {"tail1": (8, 0, 0)}))
 .wave(lambda t: {f"tail{k}": (0, (8 + 5 * k) * sin01(t, 0.5, -0.08 * k), 0) for k in range(1, 5)})
 .wave(lambda t: {"hips": (0, 3 * sin01(t, 0.5, 0.5), 0)}))
(clip("wing_flutter", 1.2).pose(0.0, WINGS_FOLDED).pose(0.25, WINGS_OPEN)
 .pose(0.45, WINGS_HALF).pose(0.65, WINGS_OPEN).pose(1.2, WINGS_FOLDED)
 .event(0.25, "flap").event(0.65, "flap"))

# ------------------------------------------------------------------------------ feelings
clip("sulk", 1.5).pose(0.0, WINGS_FOLDED).pose(0.7, CROUCH_FOLDED).pose(1.5, SULK).event(1.2, "thump")
(clip("sulk_loop", 5.0, loop=True).pose(0.0, SULK)
 .wave(lambda t: {"chest": (3 * max(0.0, sin01(t, 5.0)), 0, 0), "head": (-2 * max(0.0, sin01(t, 5.0)), 0, 0)}))
(clip("nuzzle", 1.4, loop=True)
 .pose(0.0, merge(WINGS_FOLDED, {"neck1": (8, 0, 0), "neck2": (4, 0, 0), "head": (-6, 0, 0)}))
 .wave(lambda t: {"head": (0, 10 * sin01(t, 1.4), 14 * sin01(t, 1.4)), "neck3": (0, 6 * sin01(t, 1.4, 0.1), 0)})
 .wave(tail_sway(1.5, 0.7)).event(0.3, "purr"))
(clip("greet", 1.6).pose(0.0, WINGS_FOLDED).pose(0.2, CROUCH)
 .pose(0.45, merge(WINGS_OPEN, {"arm_up*": (34, 0, 0), "arm_lo*": (-50, 0, 0), "neck1": (12, 0, 0), "head": (12, 0, 0),
                                "jaw": (-22, 0, 0)}))
 .pose(0.7, CROUCH).pose(1.0, merge(WINGS_HALF, {"neck1": (8, 0, 0), "head": (-4, 10, 12), "jaw": (-12, 0, 0)}))
 .pose(1.6, WINGS_FOLDED)
 .wave(lambda t: {f"tail{k}": (0, 14 * sin01(t, 0.4, -0.08 * k), 0) for k in range(1, 5)})
 .root(0.0).root(0.2).root(0.45, up=0.5).root(0.7).root(1.6).event(0.7, "land"))
