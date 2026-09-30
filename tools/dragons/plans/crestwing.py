"""The Crestwing's body plan: a tall, long-legged dragon with a long four-bone neck, a crest
that rises and falls on its own bone, and feathered wings that fold like a bird's.
Also for crossbreeds built on a tall, feathered body.

Skeleton (39 bones; 25 in the body draw): hips, chest (no belly bone: the body is short and
deep), neck1-neck4, head, snout, crest (for the crest's feathers only: the kind hands its skin
weights to the head, so it never bends the skin), tail1-tail4, four legs, jaw, eyes; the
classic six-bone wing per side (wing_arm, wing_fore, wing_f1-f4), whose four "finger" bones
are feather rays spread across the whole fan of flight feathers.

Its clips are its own, made for a tall, proud body (the classic clips' timings and meanings,
new poses and motion): a high-stepping strut with a pigeon's head bob; preening instead of
scratching; a heron's creep for the stalk; curled up asleep with the neck wrapped round and
the tail round the other way (a hatchling lays its chin on the floor); a graceful bow with
the wings half raised for a greeting; a hop and a flourish of wings and crest for the pounce;
a display of the raised wings when happy; the crest rising with curiosity and pleasure and
lying flat when petted or sulking; long, slow wingbeats with the long legs trailing like a
crane's, and wide, shallow glides. Poses with the neck and the wings were solved from bone
directions (a tall neck bends by roll and pitch; its yaw only twists it).
"""
import math

from dragons import clipkit
from dragons.clipkit import Clip
from eca import q_from_pyr

NAME = "crestwing"

BONES = [
    ("hips", "hips", "chest", None),
    ("chest", "chest", "neck1", "hips"),
    ("neck1", "neck1", "neck2", "chest"),
    ("neck2", "neck2", "neck3", "neck1"),
    ("neck3", "neck3", "neck4", "neck2"),
    ("neck4", "neck4", "head", "neck3"),
    ("head", "head", "muzzle", "neck4"),
    ("snout", "muzzle", "snout", "head"),
    ("crest", "crest", "crest_tip", "head"),
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

WING_CHAIN = [("wing_arm", "root", "elbow", "chest"), ("wing_fore", "elbow", "wrist", "wing_arm"),
              ("wing_f1", "wrist", "f1", "wing_fore"), ("wing_f2", "wrist", "f2", "wing_fore"),
              ("wing_f3", "wrist", "f3", "wing_fore"), ("wing_f4", "wrist", "f4", "wing_fore")]
WING_BONES = [f"{n}_{side}" for side in ("L", "R") for n, *_ in WING_CHAIN]
BONE_ORDER = [b[0] for b in BONES] + WING_BONES
WING_BODY = ("chest", "hips")
CONTACTS = ("hand_L", "hand_R", "foot_L", "foot_R")
SEAT = ("hips", (0.0, -0.42, 0.5))  # forward along the body bone, just behind the wings (take 4: +0.5 sat you on the tail)
REGION = {"crest": "head"}

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


def ease(t, a, b):
    """0 before a, 1 after b, smooth between."""
    u = min(1.0, max(0.0, (t - a) / (b - a)))
    return u * u * (3 - 2 * u)


def window(t, a, b, fade=0.15):
    """1 inside [a, b], fading in and out over `fade` seconds."""
    return ease(t, a - fade, a) * (1 - ease(t, b, b + fade))


def clip(name, length, loop=False, speed=0.0):
    c = Clip(name, length, loop=loop, speed=speed)
    CLIPS.append(c)
    return c


CLIPS = []

# ------------------------------------------------------------------------------ wing poses
# Deltas on the spread rest pose (wings raised in a V). Solved from bone directions on the
# grown Crestwing (tools/blender/fold_solver.py's method, on the dragon kit; the hatchling's
# wing has the same layout, so the same keys fold it).
# Folded like a bird's, a cloak over the back: the arm up and back along the shoulder, the
# short forearm forward, the four feather rays back along the body (the first along the spine,
# the last down the flank, each turned to lie on the body's curve), the wing's upper face to
# the body; the coverts run along the top and the golden tips point to the tail.
WINGS_FOLDED = {"wing_arm*": (37, -34, 75), "wing_fore*": (-138, 24, 89), "wing_f1*": (148, -4, 99),
                "wing_f2*": (28, 144, -69), "wing_f3*": (26, 116, -70), "wing_f4*": (38, 72, -58)}


def _q_to_pyr(q):
    """Inverse of eca.q_from_pyr (q = yaw about +Z * pitch about -X * roll about -Y)."""
    w, x, y, z = q
    m01, m11 = 2 * (x * y - w * z), 1 - 2 * (x * x + z * z)
    m20, m21, m22 = 2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)
    a = math.asin(max(-1.0, min(1.0, m21)))
    return (-math.degrees(a), math.degrees(math.atan2(-m01, m11)), -math.degrees(math.atan2(-m20, m22)))


def partway(pose, f):
    """The pose a fraction f of the way from rest (the spread wings) to `pose`, each bone along
    its shortest arc: points on the path the wings take when they fold or open, so poses
    keyed from them move cleanly (a closed fan lifting, then spreading)."""
    out = {}
    for bone, v in pose.items():
        qw, qx, qy, qz = q_from_pyr(*v)
        ang = 2 * math.acos(max(-1.0, min(1.0, qw)))
        s = math.sin(ang / 2)
        if s < 1e-6:
            out[bone] = (0.0, 0.0, 0.0)
            continue
        axis = (qx / s, qy / s, qz / s)
        h = ang * f / 2
        out[bone] = tuple(round(c, 1) for c in _q_to_pyr((math.cos(h), *(a * math.sin(h) for a in axis))))
    return out


# Raised half way, the fan still closed (a bow, a happy wiggle, running, lifting it to be brushed).
WINGS_HALF = partway(WINGS_FOLDED, 0.55)
# Folded but held a touch off the body: sitting and lying, when the rump tips up under them.
WINGS_SEATED = partway(WINGS_FOLDED, 0.88)
# Showing off: the spread wings raised high and swept forward, the feather fan flat and wide.
WINGS_DISPLAY = {"wing_arm*": (0, -26, 26), "wing_fore*": (0, -6, 8)}
WINGS_OPEN = {"wing_arm*": (0, -8, 10)}  # a little wider than the rest V


# ------------------------------------------------------------------------------ body poses
SIT = {  # sits back on its haunches like a proud cat, chest high, the plumed tail wrapped round
    "hips": (48, 0, 0), "chest": (-16, 0, 0),
    "leg_up*": (90, 0, 0), "leg_lo*": (-140, 0, 0), "foot*": (60, 0, 0),
    "arm_up*": (-34, 0, 0), "arm_lo*": (4, 0, 0), "hand*": (-12, 0, 0),
    "neck1": (-14, 0, 0), "head": (-8, 0, 0),
    "tail1": (-46, -24, 0), "tail2": (-10, -34, 0), "tail3": (-6, -38, 0), "tail4": (-10, -36, 0)}
LIE = {  # all four legs folded under like a deer, the neck still up in its S
    "arm_up*": (80, 0, 0), "arm_lo*": (-130, 0, 0), "hand*": (50, 0, 0),
    "leg_up*": (70, 0, 0), "leg_lo*": (-130, 0, 0), "foot*": (62, 0, 0),
    "neck1": (-10, 0, 0), "neck2": (2, 0, 0), "head": (-10, 0, 0),
    "tail1": (-26, 0, 0), "tail2": (-12, 18, 0), "tail3": (-8, 24, 0), "tail4": (-4, 28, 0)}  # lifted: the plumes clear the floor
_LIE_BODY = {k: v for k, v in LIE.items() if not k.startswith(("neck", "head", "tail"))}
# Curled up asleep: the long neck curves down and round the near side so the head rests by the
# chest, facing the tail, and the plumed tail wraps round the other way (solved from bone
# directions). A hatchling just lays its chin on the floor. Asleep, the grown one's forelegs
# fold right under its chest: the lie's forearms stand down and back from the elbow, and on
# these long legs that held the curled-up body up off the floor, standing (run 18).
_CURL_LEGS = dict(_LIE_BODY, **{"arm_up*": (75, 0, 0), "arm_lo*": (-165, 0, 0), "hand*": (0, 0, 0)})
CURL = merge(_CURL_LEGS, {
    "neck1": (-121, -46, -13), "neck2": (3, -7, 42), "neck3": (4, -7, 55), "neck4": (11, -11, 29),
    "head": (62, 13, -5), "crest": (16, 0, 0),
    "tail1": (-12, -26, 0), "tail2": (-8, -34, 0), "tail3": (-12, -38, 0), "tail4": (-28, -34, 0)})
CURL_BABY = merge(_LIE_BODY, {
    "neck1": (-102, 0, 0), "neck2": (4, -3, 5), "neck3": (1, -1, 3), "neck4": (5, -1, 3), "head": (99, 24, -33),
    "crest": (20, 0, 0), "tail1": (4, -30, 0), "tail2": (0, -40, 0), "tail3": (0, -44, 0), "tail4": (0, -40, 0)})
CROUCH_BODY = {
    "hips": (-6, 0, 0), "leg_up*": (40, 0, 0), "leg_lo*": (-72, 0, 0), "foot*": (32, 0, 0),
    "arm_up*": (36, 0, 0), "arm_lo*": (-64, 0, 0), "hand*": (26, 0, 0),
    "neck1": (-18, 0, 0), "neck2": (-6, 0, 0), "head": (8, 0, 0), "tail1": (-8, 0, 0), "tail2": (-6, 0, 0),
    "tail3": (-4, 0, 0)}
CROUCH = merge(WINGS_HALF, CROUCH_BODY)
CROUCH_FOLDED = merge(WINGS_FOLDED, CROUCH_BODY)
# The long neck reaches the floor, the front legs spread and bend a little.
EAT = {"neck1": (-50, 0, 0), "neck2": (-36, 0, 0), "neck3": (-24, 0, 0), "neck4": (-14, 0, 0), "head": (-20, 0, 0),
       "arm_up*": (22, 0, 10), "arm_lo*": (-34, 0, 0), "hand*": (14, 0, 0), "chest": (-8, 0, 0), "hips": (-6, 0, 0),
       "crest": (10, 0, 0)}
EAT_BABY = {"neck1": (-18, 0, 0), "neck2": (-10, 0, 0), "head": (-26, 0, 0),
            "arm_up*": (30, 0, 8), "arm_lo*": (-50, 0, 0), "hand*": (20, 0, 0), "chest": (-8, 0, 0), "hips": (-6, 0, 0)}
BELLY_UP = merge(WINGS_FOLDED, {
    "hips": (0, 0, 160), "chest": (0, 0, 14),
    "arm_up*": (60, 0, 0), "arm_lo*": (-80, 0, 0), "leg_up*": (55, 0, 0), "leg_lo*": (-80, 0, 0),
    # the long neck curves up off the floor to one side, the head upright, looking at you (solved)
    "neck1": (-112, -55, -25), "neck2": (-31, 2, 16), "neck3": (-8, 0, 6), "neck4": (20, 4, -9),
    "head": (62, 58, 79),
    "tail1": (2, 36, 0), "tail2": (2, 30, 0), "tail3": (4, 20, 0), "tail4": (6, 10, 0)})
# Half way over: on its side, the tail held up off the floor while the body turns.
_ROLL_BODY = merge(WINGS_FOLDED, {k: v for k, v in LIE.items() if not k.startswith(("tail", "neck", "head"))}, {
    "hips": (0, 0, 80), "chest": (0, 0, 6),
    "tail1": (-10, -30, 0), "tail2": (-6, -20, 0), "tail3": (0, -10, 0), "tail4": (0, 0, 0)})
ROLL_MID = merge(_ROLL_BODY, {"neck1": (-114, 98, 47), "neck2": (-2, 1, -12), "neck3": (26, 2, -6), "neck4": (32, -4, 7),
                              "head": (43, -32, -27)})
# The hatchling's short neck: its own solves (belly up, looking at you; half way over).
_BELLY_BODY = {k: v for k, v in BELLY_UP.items() if not k.startswith(("neck", "head"))}
BELLY_UP_BABY = merge(_BELLY_BODY, {"neck1": (-82, 19, 39), "neck2": (-20, -1, 2), "neck3": (-15, 1, -3),
                                    "neck4": (-4, 2, -5), "head": (80, 74, 94), "tail2": (10, 0, 0), "tail3": (10, 0, 0)})
ROLL_MID_BABY = merge(_ROLL_BODY, {"neck1": (-67, 0, -40), "neck2": (2, 8, -15), "neck3": (3, 4, -9), "neck4": (3, -2, 4),
                                   "head": (59, -52, -29)})
SULK = merge(LIE, {
    "neck1": (-30, -24, 0), "neck2": (-14, -18, 0), "neck3": (-6, -8, 0), "head": (-24, -10, -8),
    "crest": (18, 0, 0),
    "tail1": (-16, -30, 0), "tail2": (-8, -30, 0), "tail3": (-4, -28, 0), "tail4": (0, -26, 0)})


# ------------------------------------------------------------------------------ layers
def breathe(amount=1.0, period=3.4, wings=True):
    def fn(t):
        s = sin01(t, period) * amount
        out = {"chest": (1.0 * s, 0, 0), "hips": (-0.4 * s, 0, 0), "neck1": (0.6 * s, 0, 0),
               "head": (-0.6 * s, 0, 0), "crest": (1.5 * sin01(t, period, -0.1) * amount, 0, 0)}
        if wings:
            out.update({"wing_arm_R": (0, 0, 1.2 * s), "wing_arm_L": (0, 0, -1.2 * s)})
        return out
    return fn


def tail_sway(amount=1.0, period=3.4):
    """A slow, flowing sway: each bone lags the one before, so the plumes float."""
    def fn(t):
        return {f"tail{k}": (1.2 * amount * sin01(t, period * 1.5, -0.1 * k),
                             amount * (1.5 + 2.2 * k) * sin01(t, period, -0.14 * k), 0) for k in range(1, 5)}
    return fn


def crest_flick(times, amount=-22.0, width=0.35):
    """The crest rises and settles (curious, pleased, startled) at the given times (the crest
    bone's pitch: - raises the swept-back crest upright, + lays it flat)."""
    def fn(t):
        k = max((max(0.0, 1 - abs(t - t0) / width) for t0 in times), default=0.0)
        return {"crest": (amount * k * k * (3 - 2 * k), 0, 0)}
    return fn


def strut(period, lift=1.0, amp_up=26, amp_bend=46, phases=None, bob=1.0, hind=1.0):
    """The Crestwing's walk: a proud high step (the front legs lift high, knee first, and
    reach), the hind legs long and springy, and a pigeon's head bob: the head holds still in
    space, then thrusts forward with each front footfall."""
    phases = phases or {"leg_up_L": 0.0, "arm_up_L": 0.25, "leg_up_R": 0.5, "arm_up_R": 0.75}

    def fn(t):
        out = {}
        for upper, ph in phases.items():
            side = upper[-2:]
            front = upper.startswith("arm")
            u = (t / period - ph) % 1.0
            swing = math.sin(2 * math.pi * (t / period - ph))
            fwd = max(0.0, math.cos(2 * math.pi * (t / period - ph)))  # the forward swing half
            if front:
                high = math.sin(math.pi * min(1.0, u / 0.5)) if u < 0.5 else 0.0  # lifted, knee high
                out[upper] = (amp_up * swing + 22 * lift * high, 0, 0)
                out["arm_lo" + side] = (-(amp_bend + 34 * lift) * high, 0, 0)
                out["hand" + side] = (18 * lift * high - 8 * fwd, 0, 0)
            else:
                out[upper] = (amp_up * hind * swing, 0, 0)
                out["leg_lo" + side] = (-amp_bend * hind * fwd, 0, 0)
                out["foot" + side] = (0.5 * amp_bend * hind * fwd, 0, 0)
        b = sin01(t, period / 2)
        thrust = sin01(t, period / 2, 0.1)
        out["chest"] = (1.2 * b, 0, 1.4 * sin01(t, period))
        out["hips"] = (0, 2.0 * sin01(t, period, 0.25), -1.0 * sin01(t, period))
        out["neck1"] = (3.0 * bob * thrust, -1.2 * sin01(t, period, 0.1), 0)
        out["neck3"] = (-4.0 * bob * thrust, 0, 0)
        out["neck4"] = (2.0 * bob * thrust, 0, 0)
        out["head"] = (-1.5 * bob * b, 1.0 * sin01(t, period, 0.1), 0)
        out["crest"] = (3.0 * bob * sin01(t, period / 2, 0.25), 0, 0)
        return out
    return fn


def run_legs(period, amp_up, amp_bend, phases, bob=2.0):
    """Running legs (the classic leg cycle) with this body's neck."""
    def fn(t):
        out = {}
        for upper, ph in phases.items():
            side = upper[-2:]
            front = upper.startswith("arm")
            swing = sin01(t, period, -ph)
            fwd = max(0.0, math.cos(2 * math.pi * (t / period - ph)))
            lower, tip = ("arm_lo", "hand") if front else ("leg_lo", "foot")
            out[upper] = (amp_up * swing, 0, 0)
            out[lower + side] = (-amp_bend * fwd, 0, 0)
            out[tip + side] = ((-0.6 if front else 0.45) * amp_bend * fwd, 0, 0)
        b = sin01(t, period / 2)
        out["chest"] = (bob * 0.4 * b, 0, 1.2 * sin01(t, period))
        out["hips"] = (0, 2.0 * sin01(t, period, 0.25), 0)
        out["neck1"] = (-bob * b, 0, 0)
        out["neck3"] = (bob * 0.4 * b, 0, 0)
        out["head"] = (bob * 0.5 * b, 0, 0)
        return out
    return fn


def footsteps(c, period, phases, cycles=1):
    for k in range(cycles):
        for ph in phases.values():
            c.event((ph + 0.25 + k) * period % (period * cycles), "footstep")


def chomp(amount=20.0, period=0.6, at=0.15):
    def fn(t):
        u = ((t - at) / period) % 1.0
        return {"jaw": (-amount * math.sin(math.pi * (u - 0.5) / 0.5) ** 0.7 if u > 0.5 else 0.0, 0, 0)}
    return fn


def pant(amount=6.0, period=0.28):
    return lambda t: {"jaw": (-amount - 3 * sin01(t, period), 0, 0)}


def spine_flex(period, chest=6.0, hips=5.0, phase=0.0):
    def fn(t):
        s = sin01(t, period, phase)
        return {"chest": (chest * s, 0, 0), "hips": (-hips * s, 0, 0), "tail1": (4 * s, 0, 0)}
    return fn


def wing_bounce(period, amount=4.0, phase=0.0):
    def fn(t):
        s = sin01(t, period, phase) * amount
        return {"wing_arm_R": (s, 0, -0.5 * s), "wing_arm_L": (s, 0, 0.5 * s)}
    return fn


def wingbeat(period, amount, phase=0.0, hand=0.4):
    """A slow, full wingbeat: the arms sweep down and up, the hands and the feather rays follow
    a little late, so the feathers ripple."""
    def fn(t):
        s = sin01(t, period, phase)
        h = sin01(t, period, phase - 0.12)
        f = sin01(t, period, phase - 0.2)
        out = {"wing_arm_R": (0, 0, amount * s), "wing_arm_L": (0, 0, -amount * s),
               "wing_fore_R": (0, 0, hand * amount * h), "wing_fore_L": (0, 0, -hand * amount * h),
               "chest": (2.0 * s, 0, 0), "neck1": (-1.5 * s, 0, 0), "head": (1.5 * s, 0, 0)}
        for k in (1, 2, 3, 4):
            out[f"wing_f{k}_R"] = (0, 0, 0.12 * amount * f * (5 - k) / 4)
            out[f"wing_f{k}_L"] = (0, 0, -0.12 * amount * f * (5 - k) / 4)
        return out
    return fn


def fan_tail(amount=1.0):
    """The tail lifted a little, so the plumes fan out behind (showing off)."""
    return {"tail1": (-10 * amount, 0, 0), "tail2": (-8 * amount, 0, 0), "tail3": (-6 * amount, 0, 0),
            "tail4": (-8 * amount, 0, 0)}


# ------------------------------------------------------------------------------ idle & locomotion
STAND = dict(WINGS_FOLDED)  # the form's base pose is the stance: the neck in a proud S
clip("idle", 3.4, loop=True).pose(0.0, STAND).wave(breathe()).wave(tail_sway())

# A bird's look: quick turns of the head, a pause, the crest lifting with interest.
(clip("look_around", 3.2).pose(0.0, STAND)
 .key(0.0, neck3=(-3, 0, 0), neck4=(0, 0, 0), head=(-4, 0, 0))
 .key(0.35, neck3=(-1, 12, 0), neck4=(2, 16, 0), head=(2, 22, 8))
 .key(1.2, neck3=(-1, 12, 0), neck4=(2, 16, 0), head=(4, 24, 10))
 .key(1.5, neck3=(-4, -2, 0), neck4=(0, -4, 0), head=(-2, -6, -2))
 .key(1.75, neck3=(-1, -14, 0), neck4=(2, -18, 0), head=(2, -24, -8))
 .key(2.6, neck3=(-1, -14, 0), neck4=(2, -18, 0), head=(4, -26, -10))
 .key(3.2, neck3=(-3, 0, 0), neck4=(0, 0, 0), head=(-4, 0, 0))
 .wave(crest_flick((0.5, 1.9), -20)).wave(breathe()).wave(tail_sway()).event(0.4, "sniff"))

# Scratching an itch, the Crestwing's way: it preens. The neck curls round, the near wing lifts
# a little, and the snout nibbles through the coverts.
PREEN = merge(WINGS_FOLDED, {k[:-1] + "_L": (v[0], -v[1], -v[2]) for k, v in partway(WINGS_FOLDED, 0.8).items()}, {
    "neck1": (3, -10, 25), "neck2": (9, -4, 19), "neck3": (37, -10, 24), "neck4": (48, -25, 38),
    "head": (103, 38, -53), "crest": (-30, 0, 0), "chest": (0, 0, -4), "tail1": (0, -14, 0), "tail2": (0, -10, 0)})


def preen_wave(t):
    s = sin01(t, 0.3) if 0.5 < t < 1.8 else 0.0
    return {"head": (6 * s, 4 * s, 0), "neck4": (3 * s, 0, 0), "jaw": (-8 * max(0.0, s), 0, 0)}


(clip("scratch", 2.3).pose(0.0, STAND).pose(0.45, PREEN).pose(1.9, PREEN).pose(2.3, STAND)
 .wave(preen_wave).wave(tail_sway(0.6)).event(0.6, "sniff"))

# Locomotion speeds: the runtime measures each dragon's stance speed from these cycles.
WALK_PHASES = {"leg_up_L": 0.0, "arm_up_L": 0.25, "leg_up_R": 0.5, "arm_up_R": 0.75}
TROT_PHASES = {"arm_up_L": 0.0, "leg_up_R": 0.0, "arm_up_R": 0.5, "leg_up_L": 0.5}
walk = clip("walk", 1.0, loop=True, speed=0.6).pose(0.0, STAND)
walk.wave(strut(1.0, lift=1.0, amp_up=24, amp_bend=40)).wave(tail_sway(1.0, 1.0))
footsteps(walk, 1.0, WALK_PHASES)

trot = clip("trot", 0.6, loop=True, speed=1.7).pose(0.0, merge(STAND, {"neck1": (-6, 0, 0), "head": (4, 0, 0)}))
trot.wave(strut(0.6, lift=0.7, amp_up=24, amp_bend=44, phases=TROT_PHASES, bob=0.6))
trot.wave(tail_sway(1.2, 0.6)).wave(wing_bounce(0.6, 3.0, 0.3))
footsteps(trot, 0.6, {"a": 0.0, "b": 0.5})

shuffle = clip("shuffle", 1.0, loop=True).pose(0.0, STAND)
shuffle.wave(strut(1.0, lift=0.5, amp_up=10, amp_bend=24, bob=0.5))
footsteps(shuffle, 1.0, WALK_PHASES)

CARRY_HEAD = {"neck1": (10, 0, 0), "neck3": (-4, 0, 0), "head": (-10, 0, 0)}  # head up, the toy held proudly
(clip("carry", 1.0, loop=True, speed=0.6).pose(0.0, merge(STAND, CARRY_HEAD, {"crest": (-8, 0, 0)}))
 .wave(strut(1.0, lift=1.0, amp_up=24, amp_bend=40, bob=0.3)).wave(tail_sway(1.2, 0.5)))

# A baby toddles: quick little steps, a bob of the big head, the tail swishing.
walk_h = clip("walk_h", 0.46, loop=True, speed=0.5).pose(0.0, merge(WINGS_FOLDED, {"tail1": (6, 0, 0)}))
walk_h.wave(run_legs(0.46, 34, 46, WALK_PHASES, bob=3.5)).wave(tail_sway(1.6, 0.46))
footsteps(walk_h, 0.46, WALK_PHASES)
(clip("carry_h", 0.46, loop=True, speed=0.5).pose(0.0, merge(WINGS_FOLDED, {"neck1": (12, 0, 0), "head": (-8, 0, 0)}))
 .wave(run_legs(0.46, 34, 46, WALK_PHASES, bob=3.0)).wave(tail_sway(1.8, 0.3)))

# Running. A hatchling scampers: bounding, the hind feet pushing off together. A grown
# Crestwing runs like a crane taking off: long bounding strides, the neck reaching forward,
# the wings half open and lifting for balance.
SCAMPER_PHASES = {"leg_up_L": 0.0, "leg_up_R": 0.06, "arm_up_L": 0.5, "arm_up_R": 0.56}
GALLOP_PHASES = {"leg_up_L": 0.0, "leg_up_R": 0.12, "arm_up_R": 0.42, "arm_up_L": 0.54}
scamper = clip("scamper", 0.4, loop=True, speed=2.4).pose(
    0.0, merge(WINGS_FOLDED, {"neck1": (-6, 0, 0), "head": (8, 0, 0), "tail1": (-6, 0, 0), "tail4": (-10, 0, 0)}))
scamper.wave(run_legs(0.4, 40, 55, SCAMPER_PHASES, bob=4.0)).wave(spine_flex(0.4, 9, 7, 0.1))
scamper.wave(tail_sway(1.0, 0.4)).wave(pant(8.0, 0.2)).wave(wing_bounce(0.4, 7.0, 0.3))
scamper.root(0.0, up=0.0).root(0.14, up=0.2).root(0.28, up=0.04)
footsteps(scamper, 0.4, {"hind": 0.0, "front": 0.5})

GALLOP = merge(WINGS_HALF, {"neck1": (-26, 0, 0), "neck2": (-10, 0, 0), "neck3": (4, 0, 0), "neck4": (6, 0, 0),
                            "head": (18, 0, 0), "tail1": (-6, 0, 0), "tail2": (-4, 0, 0), "crest": (14, 0, 0)})
gallop = clip("gallop", 0.56, loop=True, speed=3.4).pose(0.0, GALLOP)
gallop.wave(run_legs(0.56, 38, 64, GALLOP_PHASES, bob=3.0)).wave(spine_flex(0.56, 6, 5, 0.2))
gallop.wave(tail_sway(0.6, 0.56)).wave(pant(3.0, 0.28))
gallop.wave(lambda t: {"wing_arm_R": (0, 0, 8 * sin01(t, 0.56, 0.3)), "wing_arm_L": (0, 0, -8 * sin01(t, 0.56, 0.3))})
gallop.root(0.0, up=0.04).root(0.14, up=0.0).root(0.33, up=0.03).root(0.45, up=0.12)
footsteps(gallop, 0.56, GALLOP_PHASES)

# Flight. The wings start from the rest pose (raised in a V, spread); the long legs trail
# straight back like a crane's, the neck reaches forward, the plumes stream out.
FLY_BODY = {
    "arm_up*": (34, 0, 0), "arm_lo*": (-118, 0, 0), "hand*": (-24, 0, 0),
    "leg_up*": (-70, 0, 0), "leg_lo*": (30, 0, 0), "foot*": (-30, 0, 0),
    "neck1": (-44, 0, 0), "neck2": (-16, 0, 0), "neck3": (-4, 0, 0), "neck4": (2, 0, 0), "head": (30, 0, 0),
    "crest": (16, 0, 0), "tail1": (6, 0, 0), "tail2": (2, 0, 0)}
fly_flap = clip("fly_flap", 0.8, loop=True).pose(0.0, merge(FLY_BODY, {"wing_arm*": (0, 0, -16)}))
fly_flap.wave(wingbeat(0.8, 44)).wave(tail_sway(0.5, 1.6))
fly_flap.event(0.1, "flap")
# A long, graceful glide: wings wide and still but for a breath of flex, the plumes floating.
(clip("fly_glide", 3.2, loop=True).pose(0.0, merge(FLY_BODY, {"wing_arm*": (0, -4, -30), "wing_fore*": (0, 0, 6)}))
 .wave(wingbeat(3.2, 4, hand=1.0)).wave(tail_sway(0.8, 3.2))
 .wave(lambda t: {"chest": (0, 0, 3 * sin01(t, 3.2, 0.25)), "hips": (0, 0, -2 * sin01(t, 3.2, 0.25))}))
FLY_DIVE = merge(FLY_BODY, {"wing_arm*": (-10, 40, -34), "wing_fore*": (0, -34, 0), "neck1": (-8, 0, 0),
                            "head": (34, 0, 0), "tail1": (4, 0, 0), "crest": (-6, 0, 0)})
(clip("fly_dive", 1.0, loop=True).pose(0.0, FLY_DIVE).wave(wingbeat(0.25, 3)).wave(tail_sway(0.4, 0.5)))

# ------------------------------------------------------------------------------ rest
clip("sit", 0.9).pose(0.0, STAND).pose(0.9, merge(WINGS_SEATED, SIT))
clip("sit_loop", 3.4, loop=True).pose(0.0, merge(WINGS_SEATED, SIT)).wave(breathe()).wave(tail_sway(0.4))
(clip("lie_down", 1.3).pose(0.0, STAND).pose(0.6, CROUCH_FOLDED).pose(1.3, merge(WINGS_FOLDED, LIE))
 .event(1.05, "thump"))
clip("lie_loop", 4.0, loop=True).pose(0.0, merge(WINGS_FOLDED, LIE)).wave(breathe(1.2, 4.0)).wave(tail_sway(0.4, 4.0))
clip("curl_up", 1.6).pose(0.0, merge(WINGS_FOLDED, LIE)).pose(1.6, merge(WINGS_SEATED, CURL))
clip("sleep", 5.0, loop=True).pose(0.0, merge(WINGS_SEATED, CURL)).wave(breathe(1.6, 5.0, wings=True))
clip("curl_up_h", 1.2).pose(0.0, merge(WINGS_FOLDED, LIE)).pose(1.2, merge(WINGS_FOLDED, CURL_BABY))
clip("sleep_h", 4.0, loop=True).pose(0.0, merge(WINGS_FOLDED, CURL_BABY)).wave(breathe(2.0, 4.0))
(clip("wake_h", 2.4).pose(0.0, merge(WINGS_FOLDED, CURL_BABY)).pose(0.7, merge(WINGS_FOLDED, LIE))
 .pose(1.2, merge(WINGS_HALF, LIE, {"neck1": (16, 0, 0), "head": (24, 0, 0), "jaw": (-30, 0, 0), "crest": (-24, 0, 0)}))
 .pose(1.8, merge(WINGS_HALF, {"arm_up*": (20, 0, 0), "hips": (-8, 0, 0), "head": (6, 0, 0)}))
 .pose(2.4, STAND).event(1.2, "yawn"))
# Waking: the head comes up off the wing, a big stretch of both wings and a yawn, then up.
(clip("wake", 3.0).pose(0.0, merge(WINGS_SEATED, CURL)).pose(0.6, merge(WINGS_FOLDED, LIE))
 .pose(1.3, merge(WINGS_DISPLAY, LIE, {"neck1": (20, 0, 0), "neck3": (6, 0, 0), "head": (26, 0, 0), "snout": (6, 0, 0),
                                       "jaw": (-32, 0, 0), "crest": (-24, 0, 0)}))
 .pose(1.8, merge(WINGS_HALF, {"arm_up*": (24, 0, 0), "hips": (-10, 0, 0), "neck1": (12, 0, 0), "head": (8, 0, 0)}))
 .pose(2.4, WINGS_HALF).pose(3.0, STAND).event(1.3, "yawn"))
YAWN_UP = {"neck1": (12, 0, 0), "neck2": (6, 0, 0), "neck3": (4, 0, 0), "head": (30, 0, 0), "snout": (6, 0, 0),
           "chest": (4, 0, 0), "crest": (-20, 0, 0)}
(clip("yawn", 1.7).pose(0.0, STAND)
 .pose(0.6, merge(WINGS_FOLDED, YAWN_UP, {"jaw": (-30, 0, 0)}))
 .pose(1.15, merge(WINGS_FOLDED, YAWN_UP, {"jaw": (-34, 0, 0)}))
 .pose(1.7, STAND).event(0.6, "yawn"))
clip("nap_flop", 0.8).pose(0.0, STAND).pose(0.8, merge(WINGS_FOLDED, LIE)).event(0.62, "thump")

# ------------------------------------------------------------------------------ care reactions
eat = clip("eat", 1.2, loop=True).pose(0.0, merge(WINGS_FOLDED, EAT))
eat.wave(lambda t: {"head": (8 * max(0.0, sin01(t, 0.6)), 0, 0), "snout": (5 * max(0.0, sin01(t, 0.6)), 0, 0),
                    "neck1": (2 * sin01(t, 1.2), 0, 0), "neck4": (4 * max(0.0, sin01(t, 0.6, 0.1)), 0, 0)})
eat.wave(chomp()).event(0.15, "chomp").event(0.75, "chomp")
eat_h = clip("eat_h", 1.2, loop=True).pose(0.0, merge(WINGS_FOLDED, EAT_BABY))
eat_h.wave(lambda t: {"head": (8 * max(0.0, sin01(t, 0.6)), 0, 0), "neck1": (3 * sin01(t, 1.2), 0, 0)}).wave(chomp(24))
eat_h.event(0.15, "chomp").event(0.75, "chomp")
# Its favourite food: a hop, the wings flicking half open, the crest up, the plumes fanned.
HAPPY = merge(WINGS_HALF, fan_tail(), {"neck1": (10, 0, 0), "head": (12, 0, 0), "jaw": (-16, 0, 0), "crest": (-26, 0, 0)})
(clip("fav_wiggle", 1.6).pose(0.0, STAND).pose(0.3, HAPPY).pose(1.3, HAPPY).pose(1.6, STAND)
 .wave(lambda t: {"hips": (0, 6 * sin01(t, 0.4), 0), "chest": (0, -4 * sin01(t, 0.4), 0),
                  **{f"tail{k}": (0, 12 * sin01(t, 0.3, -0.1 * k), 0) for k in range(1, 5)}})
 .root(0.0).root(0.55, up=0.0).root(0.75, up=0.22).root(0.95, up=0.0).root(1.6).event(0.95, "land")
 .event(0.3, "call"))
# A pat on the head: it leans into the hand, the crest laid flat with pleasure.
(clip("pet_head", 1.6, loop=True)
 .pose(0.0, merge(STAND, {"neck1": (4, 0, 0), "neck3": (-4, 0, 4), "neck4": (-4, 0, 6), "head": (-10, 6, 16),
                          "jaw": (-6, 0, 0), "crest": (22, 0, 0)}))
 .wave(lambda t: {"head": (0, 4 * sin01(t, 1.6), 4 * sin01(t, 1.6)), "neck3": (0, 3 * sin01(t, 1.6, 0.1), 0)})
 .wave(tail_sway(0.8, 0.8)).event(0.4, "purr"))
(clip("pet_chin", 1.6, loop=True)
 .pose(0.0, merge(STAND, {"neck1": (10, 0, 0), "neck3": (6, 0, 0), "neck4": (6, 0, 0), "head": (24, 0, 0),
                          "jaw": (-8, 0, 0), "crest": (-16, 0, 0)}))
 .wave(lambda t: {"head": (4 * sin01(t, 1.6), 0, 0), "jaw": (-3 * sin01(t, 1.6, 0.2), 0, 0)})
 .wave(tail_sway(0.8, 0.8)).event(0.4, "purr"))
for _sfx, _mid, _up in (("", ROLL_MID, BELLY_UP), ("_h", ROLL_MID_BABY, BELLY_UP_BABY)):
    (clip("roll_over" + _sfx, 1.2).pose(0.0, STAND).pose(0.45, merge(WINGS_FOLDED, LIE)).pose(0.8, _mid).pose(1.2, _up)
     .event(0.9, "thump"))
    (clip("belly_rub" + _sfx, 1.2, loop=True).pose(0.0, _up)
     .wave(lambda t: {"leg_up_L": (10 * sin01(t, 0.6), 0, 0), "leg_up_R": (10 * sin01(t, 0.6, 0.5), 0, 0),
                      "arm_up_L": (8 * sin01(t, 0.6, 0.25), 0, 0), "arm_up_R": (8 * sin01(t, 0.6, 0.75), 0, 0),
                      "hips": (0, 0, 5 * sin01(t, 1.2))})
     .wave(pant(9.0, 0.6)).event(0.3, "purr"))


def shake_wave(t):
    """A feather shake: a ripple from the head down to the tail, the wings loose, the crest up."""
    out = {}
    chain = [("head", 16, 0.0), ("neck4", 10, 0.03), ("neck3", 10, 0.06), ("neck2", 10, 0.09), ("neck1", 12, 0.12),
             ("chest", 14, 0.18), ("hips", -10, 0.26), ("tail1", 12, 0.32), ("tail2", 12, 0.36), ("tail3", 12, 0.4)]
    for bone, amp, lag in chain:
        k = max(0.0, 1.0 - abs(t - 0.45 - lag) / 0.32)
        out[bone] = (0, 0, amp * sin01(t, 0.12) * k)
    k = max(0.0, 1.0 - abs(t - 0.6) / 0.35)
    out["wing_arm_R"] = (0, 0, 14 * sin01(t, 0.12) * k)
    out["wing_arm_L"] = (0, 0, -14 * sin01(t, 0.12) * k)
    out["crest"] = (-26 * max(0.0, 1.0 - abs(t - 0.55) / 0.45), 0, 0)
    return out


clip("shake", 1.2).pose(0.0, STAND).pose(1.2, STAND).wave(shake_wave).event(0.45, "shake")
HOP_AIR = merge(WINGS_OPEN, {"arm_up*": (34, 0, 0), "arm_lo*": (-60, 0, 0), "leg_up*": (-12, 0, 0), "leg_lo*": (6, 0, 0),
                             "neck1": (10, 0, 0), "head": (8, 0, 0), "tail1": (-10, 0, 0), "crest": (-24, 0, 0)})
(clip("hop", 0.9).pose(0.0, STAND).pose(0.25, CROUCH).pose(0.45, HOP_AIR).pose(0.7, CROUCH).pose(0.9, STAND)
 .root(0.0).root(0.25).root(0.45, up=0.5).root(0.65).root(0.9).event(0.65, "land").event(0.35, "squeak"))

# ------------------------------------------------------------------------------ play
# The Crestwing's pounce is a hop and a flourish: it springs forward with the wings flung
# open, lands with them spread and the crest up, then folds them with a shake.
FLOURISH = merge(WINGS_DISPLAY, fan_tail(1.2), {"neck1": (6, 0, 0), "neck3": (-4, 0, 0), "head": (6, 0, 0),
                                                 "crest": (-30, 0, 0), "jaw": (-10, 0, 0)})
(clip("pounce", 1.6).pose(0.0, STAND).pose(0.35, CROUCH).pose(0.6, CROUCH)
 .pose(0.82, merge(WINGS_OPEN, {"arm_up*": (40, 0, 0), "arm_lo*": (-40, 0, 0), "leg_up*": (-30, 0, 0),
                                "leg_lo*": (10, 0, 0), "neck1": (-10, 0, 0), "head": (8, 0, 0), "jaw": (-18, 0, 0),
                                "crest": (-20, 0, 0)}))
 .pose(1.02, merge(FLOURISH, CROUCH_BODY)).pose(1.3, FLOURISH).pose(1.6, STAND)
 .wave(lambda t: {"hips": (0, 6 * sin01(t, 0.15), 0) if 0.35 < t < 0.6 else (0, 0, 0)})
 .root(0.0).root(0.6).root(0.82, up=0.45).root(1.02).root(1.6)
 .event(0.75, "squeak").event(1.02, "land"))
# Happy: a little prance on the spot, the crest up and the plumed tail swishing in a fan.
(clip("tail_wag", 1.0, loop=True).pose(0.0, merge(WINGS_FOLDED, fan_tail(0.8), {"neck1": (4, 0, 0), "head": (6, 0, 0),
                                                                               "crest": (-18, 0, 0), "jaw": (-8, 0, 0)}))
 .wave(lambda t: {"hips": (0, 4 * sin01(t, 0.5), 2 * sin01(t, 0.5)), "chest": (1.5 * sin01(t, 0.25), 3 * sin01(t, 0.5), -2 * sin01(t, 0.5)),
                  "neck3": (0, -2 * sin01(t, 0.5), 0), "head": (0, 2 * sin01(t, 0.5), 7 * sin01(t, 1.0, 0.25)),
                  **{f"tail{k}": (0, (10 + 5 * k) * sin01(t, 0.5, -0.08 * k), 0) for k in range(1, 5)},
                  **{f"arm_up_{s}": (16 * max(0.0, sin01(t, 1.0, -ph)) ** 1.5, 0, 0) for s, ph in (("L", 0.0), ("R", 0.5))},
                  **{f"arm_lo_{s}": (-40 * max(0.0, sin01(t, 1.0, -ph)) ** 1.5, 0, 0) for s, ph in (("L", 0.0), ("R", 0.5))},
                  **{f"leg_up_{s}": (0, -4 * sin01(t, 0.5), -2 * sin01(t, 0.5)) for s in ("L", "R")}}))
# Showing off its wings: they sweep up into the display, shiver twice, and fold.
(clip("wing_flutter", 1.5).pose(0.0, STAND).pose(0.2, merge(WINGS_HALF, {"crest": (-12, 0, 0)}))
 .pose(0.45, merge(WINGS_DISPLAY, fan_tail(), {"crest": (-28, 0, 0), "neck1": (6, 0, 0), "head": (6, 0, 0)}))
 .pose(1.0, merge(WINGS_DISPLAY, fan_tail(), {"crest": (-28, 0, 0), "neck1": (6, 0, 0), "head": (6, 0, 0)}))
 .pose(1.25, merge(WINGS_HALF, {"crest": (-10, 0, 0)})).pose(1.5, STAND)
 .wave(lambda t: {"wing_fore_R": (0, 0, 10 * sin01(t, 0.14) * window(t, 0.5, 0.95, 0.1)),
                  "wing_fore_L": (0, 0, -10 * sin01(t, 0.14) * window(t, 0.5, 0.95, 0.1))})
 .event(0.45, "flap").event(0.7, "flap"))

# ------------------------------------------------------------------------------ feelings
clip("sulk", 1.6).pose(0.0, STAND).pose(0.7, CROUCH_FOLDED).pose(1.6, merge(WINGS_SEATED, SULK)).event(0.5, "whimper").event(1.3, "thump")
(clip("sulk_loop", 5.0, loop=True).pose(0.0, merge(WINGS_SEATED, SULK))
 .wave(lambda t: {"chest": (3 * max(0.0, sin01(t, 5.0)), 0, 0), "head": (-2 * max(0.0, sin01(t, 5.0)), 0, 0)}))
# Nuzzling: the long neck curves round to twine with the other's, the cheek rubbing.
(clip("nuzzle", 1.6, loop=True)
 .pose(0.0, merge(STAND, {"neck1": (6, 0, 0), "neck2": (2, 8, 0), "neck3": (0, 10, 0), "neck4": (-4, 6, 0),
                          "head": (-8, 0, 12), "crest": (12, 0, 0)}))
 .wave(lambda t: {"head": (0, 10 * sin01(t, 1.6), 14 * sin01(t, 1.6)), "neck3": (0, 6 * sin01(t, 1.6, 0.1), 0),
                  "neck2": (0, 4 * sin01(t, 1.6, 0.2), 0)})
 .wave(tail_sway(1.5, 0.8)).event(0.3, "purr"))
# Greeting: a graceful bow, the neck sweeping down and the wings half raised, then up with the
# crest flared and a trill.
BOW = merge(WINGS_HALF, {"neck1": (-34, 0, 0), "neck2": (-16, 0, 0), "neck3": (-6, 0, 0), "head": (-12, 0, 0),
                         "arm_up*": (18, 0, 0), "arm_lo*": (-26, 0, 0), "chest": (-6, 0, 0), "crest": (10, 0, 0)})
(clip("greet", 1.8).pose(0.0, STAND).pose(0.4, BOW).pose(0.75, BOW)
 .pose(1.1, merge(WINGS_DISPLAY, fan_tail(), {"neck1": (8, 0, 0), "head": (6, 8, 10), "jaw": (-16, 0, 0), "crest": (-30, 0, 0)}))
 .pose(1.45, merge(WINGS_HALF, {"neck1": (6, 0, 0), "head": (-4, 6, 8), "jaw": (-8, 0, 0), "crest": (-16, 0, 0)}))
 .pose(1.8, STAND)
 .wave(lambda t: {f"tail{k}": (0, 12 * sin01(t, 0.45, -0.08 * k), 0) for k in range(1, 5)})
 .event(0.5, "call").event(1.1, "call"))

# ------------------------------------------------------------------------------ hands-on care
# docs/design/care-interactions.md: the jaw closes on the ball 0.35 s into pick_up / leap_catch
# and opens 0.45 s into drop_wait.
PICK = merge(WINGS_FOLDED, EAT, {"crest": (0, 0, 0)})
PICK_BABY = merge(WINGS_FOLDED, {"neck1": (-16, 0, 0), "neck2": (-8, 0, 0), "head": (-26, 0, 0),
                                 "arm_up*": (22, 0, 8), "arm_lo*": (-40, 0, 0), "chest": (-6, 0, 0)})
for _name, _down in (("pick_up", PICK), ("pick_up_h", PICK_BABY)):
    (clip(_name, 0.8).pose(0.0, STAND)
     .pose(0.25, merge(_down, {"jaw": (-22, 0, 0)})).pose(0.4, merge(_down, {"jaw": (-3, 0, 0)}))
     .pose(0.8, merge(STAND, CARRY_HEAD)))
SIT_UP = merge(WINGS_SEATED, SIT, {"neck1": (8, 0, 0), "head": (12, 0, 0), "crest": (-10, 0, 0)})
for _name, _down in (("drop_wait", merge(STAND, {"neck1": (-24, 0, 0), "neck2": (-12, 0, 0), "head": (-16, 0, 0)})),
                     ("drop_wait_h", merge(STAND, {"neck1": (-6, 0, 0), "head": (-18, 0, 0)}))):
    (clip(_name, 1.3).pose(0.0, merge(STAND, CARRY_HEAD))
     .pose(0.35, merge(_down, {"jaw": (-4, 0, 0)})).pose(0.5, merge(_down, {"jaw": (-22, 0, 0)}))
     .pose(0.9, merge(SIT_UP, {"jaw": (-8, 0, 0)})).pose(1.3, merge(SIT_UP, {"jaw": (-6, 0, 0)}))
     .wave(lambda t: {f"tail{k}": (0, (6 + 4 * k) * sin01(t, 0.4, -0.08 * k) * min(1.0, max(0.0, (t - 0.7) * 3)), 0)
                      for k in range(1, 5)}))
LEAP = merge(WINGS_OPEN, {"arm_up*": (30, 0, 0), "arm_lo*": (-56, 0, 0), "leg_up*": (-16, 0, 0),
                          "neck1": (16, 0, 0), "neck3": (4, 0, 0), "head": (14, 0, 0), "crest": (-20, 0, 0)})
(clip("leap_catch", 0.9).pose(0.0, STAND).pose(0.15, CROUCH)
 .pose(0.32, merge(LEAP, {"jaw": (-24, 0, 0)})).pose(0.45, merge(LEAP, {"jaw": (-3, 0, 0), "head": (-6, 0, 0)}))
 .pose(0.7, CROUCH).pose(0.9, merge(STAND, CARRY_HEAD))
 .root(0.0).root(0.15).root(0.4, up=0.55).root(0.7).root(0.9).event(0.7, "land"))
KICK = merge(WINGS_SEATED, {k[:-1] + "_R": v for k, v in partway(WINGS_FOLDED, 0.72).items()}, SIT, {"neck1": (4, 0, 0), "head": (6, 8, 14), "leg_up_R": (84, 0, 0), "leg_lo_R": (-80, 0, 0),
                                 "crest": (-14, 0, 0)})
(clip("leg_kick", 1.4).pose(0.0, STAND).pose(0.25, KICK).pose(1.15, KICK).pose(1.4, STAND)
 .wave(lambda t: {"leg_up_R": (22 * sin01(t, 0.18) if 0.25 < t < 1.15 else 0.0, 0, 0),
                  **{f"tail{k}": (0, (8 + 5 * k) * sin01(t, 0.35, -0.08 * k), 0) for k in range(1, 5)}})
 .event(0.3, "thump").event(0.48, "thump").event(0.66, "thump").event(0.84, "thump"))
# Food it doesn't like: a sniff, then the nose goes up and away, haughtily.
(clip("sniff_refuse", 1.4).pose(0.0, STAND)
 .pose(0.35, merge(STAND, {"neck1": (-16, 0, 0), "neck2": (-8, 0, 0), "head": (-16, 0, 0)}))
 .pose(0.8, merge(STAND, {"neck1": (8, 0, 0), "neck3": (4, 20, 0), "neck4": (2, 16, 0), "head": (22, 26, -10),
                          "crest": (16, 0, 0)}))
 .pose(1.4, STAND).event(0.35, "sniff").event(0.85, "whimper"))
# Brushing under a wing: the right one lifts half open, then settles.
WING_R_UP = merge(WINGS_FOLDED, {n[:-1] + "_R": v for n, v in WINGS_HALF.items()})  # "_R" keys win over "*"
(clip("lift_wing", 1.4).pose(0.0, STAND).pose(0.4, merge(WING_R_UP, {"chest": (0, 0, 6), "neck3": (0, -10, 0),
                                                                       "head": (0, -12, 0)}))
 .pose(1.05, merge(WING_R_UP, {"chest": (0, 0, 6), "neck3": (0, -10, 0), "head": (0, -12, 0)})).pose(1.4, STAND)
 .event(0.35, "flap"))
(clip("sneeze", 0.8).pose(0.0, STAND)
 .pose(0.28, merge(STAND, {"neck1": (6, 0, 0), "head": (14, 0, 0), "jaw": (-8, 0, 0), "crest": (-10, 0, 0)}))
 .pose(0.42, merge(STAND, {"neck1": (-8, 0, 0), "neck3": (-4, 0, 0), "head": (-16, 0, 0), "jaw": (-4, 0, 0),
                           "crest": (-24, 0, 0)}))
 .pose(0.8, STAND).event(0.4, "sneeze"))
(clip("pull_away", 0.9).pose(0.0, STAND)
 .pose(0.3, merge(STAND, {"neck1": (16, 0, 0), "neck2": (8, 0, 0), "neck3": (6, 0, 0), "head": (10, 0, 0),
                          "chest": (6, 0, 0), "hips": (-4, 0, 0), "crest": (20, 0, 0)}))
 .pose(0.9, STAND)
 .wave(lambda t: {"head": (0, 16 * sin01(t, 0.16) * max(0.0, 1 - abs(t - 0.45) / 0.25), 0)})
 .event(0.3, "whimper"))

# ------------------------------------------------------------------------------ toys
# A swat at the dangled feather: from a little bow, the left front foot comes up and forward.
PLAY_READY = merge(WINGS_FOLDED, {"hips": (-4, 0, 0), "arm_up*": (30, 0, 0), "arm_lo*": (-50, 0, 0), "hand*": (20, 0, 0),
                                  "chest": (-8, 0, 0), "neck1": (-14, 0, 0), "head": (10, 0, 0), "tail1": (-10, 0, 0), "tail2": (-6, 0, 0),
                                  "crest": (-16, 0, 0)})
SWAT = merge(PLAY_READY, {"arm_up_L": (-84, 12, 0), "arm_lo_L": (-24, 0, 0), "hand_L": (34, 0, 0),
                          "chest": (-4, 0, 6), "head": (14, 8, -12), "jaw": (-14, 0, 0)})
(clip("paw_bat", 0.75).pose(0.0, PLAY_READY).pose(0.22, SWAT).pose(0.4, SWAT).pose(0.75, PLAY_READY)
 .wave(lambda t: {f"tail{k}": (0, 14 * sin01(t, 0.375, -0.08 * k), 0) for k in range(1, 5)})
 .event(0.22, "squeak"))
TUG = merge(WINGS_FOLDED, {"hips": (8, 0, 0), "leg_up*": (22, 0, 0), "leg_lo*": (-34, 0, 0), "foot*": (12, 0, 0),
                           "arm_up*": (-12, 0, 0), "arm_lo*": (-20, 0, 0), "hand*": (10, 0, 0),
                           "chest": (-6, 0, 0), "neck1": (-36, 0, 0), "neck2": (-16, 0, 0), "neck3": (-6, 0, 0),
                           "head": (8, 0, 0), "jaw": (-5, 0, 0), "tail1": (-20, 0, 0), "tail2": (-8, 0, 0), "crest": (10, 0, 0)})
(clip("tug", 0.9, loop=True).pose(0.0, TUG)
 .wave(lambda t: {"neck1": (0, 12 * sin01(t, 0.45), 0), "neck2": (0, 8 * sin01(t, 0.45, 0.1), 0),
                  "head": (0, 6 * sin01(t, 0.45, 0.2), 10 * sin01(t, 0.45, 0.15)),
                  "hips": (0, 0, 3 * sin01(t, 0.9)),
                  **{f"tail{k}": (0, 16 * sin01(t, 0.45, -0.08 * k), 0) for k in range(1, 5)}}))
PLAY_BOW = merge(WINGS_HALF, fan_tail(), {
    "hips": (-16, 0, 0), "chest": (-6, 0, 0),
    "arm_up*": (46, 0, 10), "arm_lo*": (-70, 0, 0), "hand*": (36, 0, 0),
    "leg_up*": (16, 0, 0), "leg_lo*": (-6, 0, 0),
    "neck1": (-10, 0, 0), "neck3": (8, 0, 0), "head": (-6, 0, 0), "jaw": (-14, 0, 0), "crest": (-24, 0, 0)})
(clip("play_bow", 1.2).pose(0.0, STAND).pose(0.3, PLAY_BOW).pose(0.9, PLAY_BOW).pose(1.2, STAND)
 .wave(lambda t: {f"tail{k}": (0, 20 * sin01(t, 0.25, -0.1 * k), 0) for k in range(2, 5)} if 0.25 < t < 0.95 else {})
 .event(0.35, "call"))
# Sparring: reared up on the long hind legs, wings open for balance, batting with the front feet.
SPAR = merge(WINGS_OPEN, {"hips": (26, 0, 0), "chest": (6, 0, 0), "leg_up*": (-6, 0, 0), "leg_lo*": (-36, 0, 0),
                          "foot*": (26, 0, 0), "neck1": (-12, 0, 0), "neck3": (4, 0, 0), "head": (-12, 0, 0),
                          "tail1": (-34, 0, 0), "tail2": (-12, 0, 0), "tail3": (-4, 0, 0), "crest": (-26, 0, 0)})


def spar_wave(t):
    s, c = sin01(t, 0.5), sin01(t, 0.5, 0.25)
    return {"arm_up_L": (40 + 30 * s, 0, 12), "arm_lo_L": (-40 - 20 * s, 0, 0), "hand_L": (-20 * s, 0, 0),
            "arm_up_R": (40 - 30 * s, 0, -12), "arm_lo_R": (-40 + 20 * s, 0, 0), "hand_R": (20 * s, 0, 0),
            "head": (6 * c, 8 * s, 0), "neck3": (0, 6 * s, 0), "jaw": (-14 - 6 * c, 0, 0),
            "wing_arm_R": (0, 0, 6 * c), "wing_arm_L": (0, 0, -6 * c),
            **{f"tail{k}": (0, 14 * sin01(t, 0.5, -0.1 * k), 0) for k in range(1, 5)}}


(clip("spar", 1.0, loop=True).pose(0.0, SPAR).wave(spar_wave)
 .root(0.0, up=0.0).root(0.25, up=0.05).root(0.5, up=0.0).root(0.75, up=0.05)
 .event(0.1, "squeak").event(0.6, "thump"))
# Stalking like a heron: slow, high, deliberate steps, the neck drawn back into a coil and the
# head low and forward, perfectly still, the crest flat.
STALK = merge(WINGS_FOLDED, {"hips": (-4, 0, 0), "leg_up*": (16, 0, 0), "leg_lo*": (-26, 0, 0), "foot*": (10, 0, 0),
                             "arm_up*": (12, 0, 0), "arm_lo*": (-20, 0, 0),
                             "neck1": (-34, 0, 0), "neck2": (38, 0, 0), "neck3": (-17, 0, 0), "neck4": (-23, 0, 0),
                             "head": (24, 0, 0), "crest": (26, 0, 0), "tail1": (6, 0, 0)})
stalk = clip("stalk", 1.6, loop=True, speed=0.22).pose(0.0, STALK)
stalk.wave(strut(1.6, lift=1.2, amp_up=12, amp_bend=26, bob=0.0, hind=0.8))
stalk.wave(lambda t: {"tail4": (0, 16 * sin01(t, 0.4), 0), "tail3": (0, 5 * sin01(t, 0.4, -0.1), 0)})
TAIL_CHASE = merge(WINGS_HALF, {
    "neck1": (0, 30, 0), "neck2": (0, 30, 0), "neck3": (0, 24, 0), "neck4": (0, 16, 0), "head": (-10, 16, 10),
    "chest": (0, 12, 0), "hips": (0, 10, 0),
    "tail1": (0, 30, 0), "tail2": (0, 34, 0), "tail3": (0, 34, 0), "tail4": (0, 30, 0), "jaw": (-16, 0, 0),
    "crest": (-20, 0, 0)})
tail_chase = clip("tail_chase", 0.6, loop=True).pose(0.0, TAIL_CHASE)
tail_chase.wave(run_legs(0.6, 22, 36, TROT_PHASES, bob=2.5)).wave(pant(6.0, 0.3))
tail_chase.root(0.0, up=0.0).root(0.15, up=0.1).root(0.3, up=0.0).root(0.45, up=0.1)
footsteps(tail_chase, 0.6, {"a": 0.0, "b": 0.5})


def clips():
    """Fresh copies of the Crestwing's clips (every required clip, some baby versions)."""
    import copy
    out = [copy.deepcopy(c) for c in CLIPS]
    clipkit.check(out, BONE_ORDER, NAME)
    return out

