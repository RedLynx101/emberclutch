"""The Pouncer's body plan: a lithe four-legged dragon with a pair of bat-like wings, the
classic dragon's skeleton (tools/blender/rig_layout.py): 26 body bones and 12 wing bones.
Also used by crossbreeds built on a cat-like body (the Blazeplume, the Kindlemoss, the Lilyfin).

Its clips are the classic dragon's, which were made for this skeleton, with a cat's
character on top: a longer, lazier tail sway, a lower stalk and a springier pounce, and a
cat's way of curling up to sleep (run 18).
"""
from dragons import clipkit
from clips import LIE, WINGS_FOLDED, WINGS_HALF, breathe, merge  # tools/anim/clips.py (the classic set)

NAME = "pouncer"

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

WING_CHAIN = [("wing_arm", "root", "elbow", "chest"), ("wing_fore", "elbow", "wrist", "wing_arm"),
              ("wing_f1", "wrist", "f1", "wing_fore"), ("wing_f2", "wrist", "f2", "wing_fore"),
              ("wing_f3", "wrist", "f3", "wing_fore"), ("wing_f4", "wrist", "f4", "wing_fore")]
WING_BONES = [f"{n}_{side}" for side in ("L", "R") for n, *_ in WING_CHAIN]
BONE_ORDER = [b[0] for b in BONES] + WING_BONES
WING_BODY = ("chest", "belly", "hips")
CONTACTS = ("hand_L", "hand_R", "foot_L", "foot_R")
SEAT = ("belly", (0.0, 0.2, 0.55))
REGION = {}


# Curled up asleep, as a cat sleeps (run 18). The classic curl was made for the classic dragon's
# long S of a neck: on these cat-like bodies it stood propped on its forearms with the head held
# up, the neck turned one way and the tail the other (an S, not a curl). Instead: down on its
# belly, the forelegs folded under the chest (bending the classic legs' way, so the lie folds
# into it without a swing) and the hind legs along the flanks, the rump let down (the hips tip
# back), the body in a soft C, the chin down by its paws with the head tilted, and the tail
# wrapped round the same side to its nose. Solved from bone directions on the Pouncer and
# checked on every kind on the plan (tools/blender/dragonkit/curl.py).
CURL = merge(WINGS_FOLDED, {
    "hips": (10, 0, 0),
    "arm_up*": (60, 0, 0), "arm_lo*": (-145, 0, 0), "hand*": (-5, 0, 0),
    "leg_up*": (62, 12, 0), "leg_lo*": (-115, 0, 0), "foot*": (55, 0, 0),
    "belly": (-18, 13, 1), "chest": (-8, 12, -7),
    "neck1": (-72, 11, -7), "neck2": (-14, 2, -2), "neck3": (18, 2, -2), "head": (68, -9, 14),
    "tail1": (4, -33, 7), "tail2": (7, -87, 9), "tail3": (14, -68, 6), "tail4": (4, -51, -6),
})
# Settling: the head going down and round, the tail starting to wrap.
_SHIFT = merge(WINGS_FOLDED, {
    "hips": (5, 0, 0), "arm_up*": (68, 0, 0), "arm_lo*": (-122, 0, 0), "hand*": (32, 0, 0),
    "leg_up*": (62, 6, 0), "leg_lo*": (-115, 0, 0), "foot*": (55, 0, 0),
    "belly": (-9, 6, 0), "chest": (-4, 6, -3), "neck1": (-40, 6, -3), "neck2": (-6, 1, -1), "head": (26, -4, 12),
    "tail1": (-1, -16, 3), "tail2": (4, -45, 4), "tail3": (10, -40, 3), "tail4": (12, -36, -3),
})


def rest_clips():
    """curl_up, sleep and wake on the cat's curl (the rest of the classic rest clips stay)."""
    curl_up = clipkit.Clip("curl_up", 1.6).pose(0.0, LIE).pose(0.75, _SHIFT).pose(1.6, CURL).event(1.35, "thump")
    sleep = clipkit.Clip("sleep", 4.8, loop=True).pose(0.0, CURL).wave(breathe(1.8, 4.8))
    wake = (clipkit.Clip("wake", 2.8).pose(0.0, CURL).pose(0.45, _SHIFT).pose(0.9, LIE)
            .pose(1.5, merge(LIE, {"arm_up*": (20, 0, 0), "arm_lo*": (-20, 0, 0), "hips": (-14, 0, 0),
                                   "leg_up*": (30, 0, 0), "leg_lo*": (-50, 0, 0), "neck1": (18, 0, 0),
                                   "head": (24, 0, 0), "snout": (6, 0, 0), "jaw": (-32, 0, 0),
                                   "wing_arm*": (0, 10, -10)}))
            .pose(2.1, merge(WINGS_HALF, {"arm_up*": (22, 0, 0), "hips": (-10, 0, 0), "neck1": (14, 0, 0),
                                          "head": (10, 0, 0)}))
            .pose(2.8, WINGS_FOLDED).event(1.5, "yawn"))
    return [curl_up, sleep, wake]


def clips():
    out = clipkit.replace(clipkit.classic(), rest_clips())
    by = clipkit.by_name(out)
    tails = {"tail1", "tail2", "tail3", "tail4"}
    # A cat's tail: a longer, lazier sway at rest.
    for name in ("idle", "sit_loop", "look_around"):
        clipkit.scaled(by[name], 1.3, bones=tails)
    # Stalking lower and pouncing further (the root tracks are in adult units).
    clipkit.scaled(by["stalk"], 1.15)
    by["pounce"].root_keys = {t: (f * 1.2, u * 1.15) for t, (f, u) in by["pounce"].root_keys.items()}
    clipkit.check(out, BONE_ORDER, NAME)
    return out
