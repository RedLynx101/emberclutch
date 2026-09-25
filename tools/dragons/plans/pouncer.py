"""The Pouncer's body plan: a lithe four-legged dragon with a pair of bat-like wings, the
classic dragon's skeleton (tools/blender/rig_layout.py): 26 body bones and 12 wing bones.
Also used by crossbreeds built on a cat-like body (the Blazeplume).

Its clips are the classic dragon's, which were made for this skeleton, with a cat's
character on top: a longer, lazier tail sway, a lower stalk and a springier pounce.
"""
from dragons import clipkit

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


def clips():
    out = clipkit.classic()
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
