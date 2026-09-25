"""The Duskwing's body plan: a wyvern. It stands on two strong hind legs and on the wrists of
its folded wings, like a bat; it has no front legs (no arm_* bones).

Skeleton (38 bones): 24 body bones (hips, belly, chest, a three-bone neck, head, snout, jaw,
eyes, a six-bone whip tail, the hind legs, and two ear bones so the tall bat ears can
swivel; they carry only the ear parts, no skin) and 7 wing bones a side (arm, forearm, the
thumb it leans on, four fingers). The body draw counts 23 bones (its skin uses 21: not the
ears), the wing draw 17 (the wing bones and chest, belly, hips for the membrane's flank).
CONTACTS: the wing thumbs (wrist to thumb claw, on the ground when it stands) and the feet.
The baby (the hatchling form) stands up on its hind legs; its tiny wing-arms don't reach the
ground, so its thumbs are contacts in name only.

Poses. Every wing pose is solved in Blender from targets (where the wrist goes, by two-bone
IK, and which way each bone points and the folded membrane faces) against the kind's body in
that pose, and kept below as keys (SOLVED for the grown form, SOLVED_H for the baby's own):
  blender -b -P tools/dragons/plans/duskwing.py -- --solve [--stage hatchling] [--poses a,b]
          [--render all|a,b --views three_quarter,side --texture --variant 3 --out C:/abs/prefix]
rewrites the SOLVED block of this file (and renders the poses and a sheet).
  stand       on the wrists, the fingers folded up along the forearms: a high-collared cloak
  walk_*      the crawl-walk's stride (lift, plant, push); run_*: the bound (both wings at once)
  crouch, eat, stalk_*   low; eating with the wrists wide and the elbows up
  sit         sitting up like a cat, wrists before the feet, the folded wings round it
  lie, curl   lying with the wings folded like a bird's along the flanks; asleep with them
              drawn up over the back like a blanket
  bow, flare, stretch, rear, open   play and display: fanned low, flared, one wing spread,
              reared up to spar, spread wide
  fly, glide, dive, belly_up
The baby's table (H_POSES): sit (its tiny wings wrapped round its tummy), curl (asleep sitting
up, cloaked), lie (flopped on its tummy), eat, crouch, open, stretch, rear, bow, belly_up.

Clips: one clip builder (_clip_set) and two pose tables (_tables): the grown set, and the baby's
own versions ("<name>_h", BABY_CLIPS) of every clip that poses the body, plus its scamper.
"""
import math
import os
import sys

NAME = "duskwing"

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
    ("tail4", "tail4", "tail5", "tail3"),
    ("tail5", "tail5", "tail6", "tail4"),
    ("tail6", "tail6", "tail_tip", "tail5"),
]
for _side in ("L", "R"):
    BONES += [
        (f"leg_up_{_side}", f"hipj_{_side}", f"knee_{_side}", "hips"),
        (f"leg_lo_{_side}", f"knee_{_side}", f"ankle_{_side}", f"leg_up_{_side}"),
        (f"foot_{_side}", f"ankle_{_side}", f"toe_b_{_side}", f"leg_lo_{_side}"),
        (f"ear_{_side}", f"ear_{_side}", f"ear_tip_{_side}", "head"),
    ]
BONES.append(("jaw", "jaw_hinge", "jaw_tip", "snout"))
BONES.append(("eyes", "eyes_c", "eyes_tip", "head"))

# The wing is the foreleg: arm, forearm, then from the wrist the thumb (its "hand": a claw it
# leans on) and four fingers spanning the membrane.
WING_CHAIN = [("wing_arm", "root", "elbow", "chest"), ("wing_fore", "elbow", "wrist", "wing_arm"),
              ("wing_thumb", "wrist", "thumb", "wing_fore"),
              ("wing_f1", "wrist", "f1", "wing_fore"), ("wing_f2", "wrist", "f2", "wing_fore"),
              ("wing_f3", "wrist", "f3", "wing_fore"), ("wing_f4", "wrist", "f4", "wing_fore")]
WING_BONES = [f"{n}_{side}" for side in ("L", "R") for n, *_ in WING_CHAIN]
BONE_ORDER = [b[0] for b in BONES] + WING_BONES
WING_BODY = ("chest", "belly", "hips")
CONTACTS = ("wing_thumb_L", "wing_thumb_R", "foot_L", "foot_R")
SEAT = ("belly", (0.0, -0.1, 0.52))
REGION = {"ear": "head"}

# ------------------------------------------------------------------------------ poses
# A pose is body keys (pitch, yaw, roll on top of the idle pose; "name*" both sides) and a
# wing: targets for the right wing (the left mirrors) in the grown adult's space (+X out to
# the right wing's side, +Y back, +Z up, z = 0 the floor under the pose):
#   wrist    (x, y, z) where the wrist goes (two-bone IK on arm and forearm); pole: which
#            way the elbow bends. Or arm / fore: directions.
#   thumb, f1..f4: directions, or ("fold", (dx, dy, dz)): back along the forearm, nudged
#   normal   which way the membrane's top faces (its twist); normals: per bone overrides
#   rest     (pitch, yaw, roll): the spread wing of the rest pose, turned about the shoulder
#            in the dragon's space (the body's own turn doesn't carry it)
FOLD_NORMAL = (0.96, 0.05, 0.28)
FOLD = {"f1": ("fold", (0.0, 0.05, 0.06)), "f2": ("fold", (0.02, 0.11, 0.0)),
        "f3": ("fold", (0.03, 0.17, -0.05)), "f4": ("fold", (0.04, 0.23, -0.1))}
THUMB_DOWN = (0.05, -0.92, -0.38)


def _stand(wrist, pole=(-0.1, 0.3, 0.95), thumb=THUMB_DOWN, normal=FOLD_NORMAL, **more):
    spec = dict(wrist=wrist, pole=pole, thumb=thumb, normal=normal)
    spec.update(FOLD)
    spec.update(more)
    return spec


LIE_BODY = {"leg_up*": (64, 0, 0), "leg_lo*": (-118, 0, 0), "foot*": (56, 0, 0), "chest": (-6, 0, 0),
            "neck1": (-14, 0, 0), "neck2": (-4, 0, 0), "head": (-2, 0, 0),
            "tail1": (-8, 0, 0), "tail2": (0, 10, 0), "tail3": (0, 16, 0), "tail4": (0, 18, 0), "tail5": (0, 16, 0),
            "tail6": (0, 12, 0)}
SIT_BODY = {"hips": (40, 0, 0), "leg_up*": (70, 0, 0), "leg_lo*": (-104, 0, 0), "foot*": (-6, 0, 0),
            "tail1": (-44, 0, 0), "tail2": (-4, 12, 0), "tail3": (0, 22, 0), "tail4": (0, 26, 0),
            "tail5": (0, 26, 0), "tail6": (0, 22, 0),
            "neck1": (-8, 0, 0), "neck2": (-6, 0, 0), "neck3": (6, 0, 0), "head": (-22, 0, 0)}
REAR_BODY = {"hips": (36, 0, 0), "leg_up*": (-30, 0, 0), "leg_lo*": (-10, 0, 0), "foot*": (8, 0, 0),
             "tail1": (-34, 0, 0), "tail2": (-6, 0, 0),
             "neck1": (-14, 0, 0), "neck2": (-6, 0, 0), "head": (-12, 0, 0)}
CROUCH_BODY = {"leg_up*": (34, 0, 0), "leg_lo*": (-58, 0, 0), "foot*": (24, 0, 0), "hips": (-4, 0, 0),
               "chest": (-8, 0, 0), "neck1": (-10, 0, 0), "head": (10, 0, 0)}
FLY_BODY = {"leg_up*": (-62, 0, 0), "leg_lo*": (50, 0, 0), "foot*": (-40, 0, 0),
            "neck1": (-34, 0, 0), "neck2": (-12, 0, 0), "neck3": (-4, 0, 0), "head": (26, 0, 0),
            "tail1": (6, 0, 0), "tail2": (2, 0, 0), "ear*": (34, 8, -10)}

POSES = {
    # standing on the wrists (the idle), and the walk's moments (the right wing)
    "stand": dict(body={}, wing=_stand((0.5, -0.95, 0.08))),
    "walk_lift": dict(body={}, wing=_stand((0.55, -0.98, 0.34), thumb=(0.05, -0.85, -0.52))),
    "walk_plant": dict(body={}, wing=_stand((0.52, -1.30, 0.08))),
    "walk_push": dict(body={}, wing=_stand((0.52, -0.62, 0.10), thumb=(0.05, -0.6, -0.8))),
    # the run's bound (both wings together)
    "run_lift": dict(body={}, wing=_stand((0.58, -1.05, 0.58), thumb=(0.05, -0.8, -0.6))),
    "run_plant": dict(body={}, wing=_stand((0.54, -1.55, 0.10))),
    "run_push": dict(body={}, wing=_stand((0.50, -0.50, 0.14), thumb=(0.05, -0.5, -0.86))),
    # low, about to spring; eating from the floor (wrists wide, elbows up)
    "crouch": dict(body=CROUCH_BODY, wing=_stand((0.62, -1.15, 0.08), pole=(0.1, 0.3, 0.95))),
    # the stalk's creeping stride, low on the crouched body
    "stalk_lift": dict(body=CROUCH_BODY, wing=_stand((0.64, -1.18, 0.26), pole=(0.1, 0.3, 0.95),
                                                     thumb=(0.05, -0.85, -0.52))),
    "stalk_plant": dict(body=CROUCH_BODY, wing=_stand((0.62, -1.42, 0.08), pole=(0.1, 0.3, 0.95))),
    "stalk_push": dict(body=CROUCH_BODY, wing=_stand((0.62, -0.90, 0.10), pole=(0.1, 0.3, 0.95),
                                                     thumb=(0.05, -0.6, -0.8))),
    "eat": dict(body={"chest": (-12, 0, 0), "neck1": (-44, 0, 0), "neck2": (-26, 0, 0), "neck3": (-12, 0, 0),
                      "head": (-8, 0, 0), "leg_up*": (12, 0, 0), "leg_lo*": (-18, 0, 0)},
                wing=_stand((0.80, -1.10, 0.08), pole=(0.35, 0.3, 0.9))),
    # sitting up like a cat, wrists planted before its feet, the folded wings standing round
    # it like a high-collared cloak
    "sit": dict(body=SIT_BODY, wing=_stand((0.42, -0.02, 0.08), pole=(0.25, 0.45, 0.85), thumb=(0.05, -0.95, -0.3),
                                         normal=(0.85, -0.35, 0.3))),
    # lying down: the wings folded like a bird's along the flanks (elbow low in front, the
    # forearm back along the side, the fingers folded forward over it)
    "lie": dict(body=LIE_BODY, wing=dict(wrist=(0.66, 1.18, 0.28), pole=(0.35, -0.85, -0.3), thumb=(0.1, 0.3, -0.95),
                                         f1=("fold", (0.0, 0.0, 0.12)), f2=("fold", (0.02, 0.0, 0.04)),
                                         f3=("fold", (0.03, 0.0, -0.04)), f4=("fold", (0.04, 0.0, -0.12)),
                                         normal=(0.8, 0.0, 0.6))),
    # asleep, cloaked: curled up, the wings drawn up over the back like a blanket
    "curl": dict(body=dict(LIE_BODY, **{"neck1": (-28, 38, 0), "neck2": (-10, 34, 0), "neck3": (-6, 26, 0),
                                        "head": (-12, 18, 22), "tail1": (-8, 26, 0), "tail2": (0, 30, 0),
                                        "tail3": (0, 32, 0), "tail4": (0, 30, 0), "tail5": (0, 28, 0),
                                        "tail6": (0, 24, 0), "ear*": (44, 10, -14)}),
                 wing=dict(wrist=(0.62, 1.12, 0.42), pole=(0.35, -0.85, -0.1), thumb=(0.1, 0.3, -0.95),
                           f1=("fold", (-0.3, 0.0, 0.45)), f2=("fold", (-0.2, 0.0, 0.3)),
                           f3=("fold", (-0.1, 0.0, 0.15)), f4=("fold", (0.0, 0.0, 0.0)),
                           normal=(0.45, 0.0, 0.9))),
    # the play bow: wrists far forward, the wings fanned out low, rump up
    "bow": dict(body={"hips": (-16, 0, 0), "chest": (-10, 0, 0), "leg_up*": (16, 0, 0), "leg_lo*": (-4, 0, 0),
                      "neck1": (22, 0, 0), "neck2": (8, 0, 0), "head": (-8, 0, 0), "tail1": (28, 0, 0),
                      "tail2": (10, 0, 0)},
                wing=dict(wrist=(0.78, -1.55, 0.06), pole=(0.25, 0.4, 0.9), thumb=(0.1, -0.9, -0.4),
                          f1=(0.85, 0.25, 0.45), f2=(0.62, 0.62, 0.35), f3=(0.38, 0.86, 0.25), f4=(0.12, 0.97, 0.15),
                          normal=(0.1, 0.0, 1.0))),
    # waking: a bat's stretch, front down and one wing spread wide (the right; the left stays bowed)
    "stretch": dict(body={"hips": (-16, 0, 0), "chest": (-10, 0, 0), "leg_up*": (16, 0, 0), "leg_lo*": (-4, 0, 0),
                          "neck1": (22, 0, 0), "neck2": (8, 0, 0), "head": (-8, 0, 0), "tail1": (28, 0, 0),
                          "tail2": (10, 0, 0)},
                    wing=dict(rest=(-8, 14, 12))),
    # standing, the fingers flared up and back in a fan: the stars show (happy, excited)
    "flare": dict(body={}, wing=_stand((0.54, -0.98, 0.08), f1=("fold", (0.0, 0.0, 0.1)),
                                       f2=("fold", (0.1, 0.45, 0.1)), f3=("fold", (0.2, 0.95, 0.0)),
                                       f4=("fold", (0.25, 1.4, -0.2)))),
    # reared up on the hind legs: wrists up in front (sparring), or the wings spread wide
    "rear": dict(body=REAR_BODY, wing=_stand((0.42, -1.25, 1.35), pole=(0.3, 0.6, -0.4), thumb=(0.0, -0.8, 0.6),
                                             normal=(0.9, -0.3, 0.2))),
    "open": dict(body=REAR_BODY, wing=dict(rest=(-12, -8, 18))),
    # flight
    "fly": dict(body=FLY_BODY, wing=dict(rest=(0, 0, 0))),
    "glide": dict(body=FLY_BODY, wing=dict(rest=(0, -4, -6))),
    "dive": dict(body=dict(FLY_BODY, neck1=(-8, 0, 0), head=(30, 0, 0)),
                 wing=dict(arm=(0.8, 0.45, 0.2), fore=(0.55, 0.8, -0.1), thumb=(0.3, -0.8, 0.3),
                           f1=(0.2, 0.97, -0.05), f2=(0.15, 0.98, -0.1), f3=(0.1, 0.98, -0.15), f4=(0.05, 0.97, -0.2),
                           normal=(0.2, 0.0, 1.0))),
    # on its back, the wings spread on the floor
    "belly_up": dict(body={"hips": (0, 0, 165), "chest": (0, 0, 12), "leg_up*": (50, 0, 0), "leg_lo*": (-70, 0, 0),
                           "neck1": (10, 0, -60), "neck2": (0, 0, -50), "neck3": (0, 0, -30), "head": (15, 0, -20),
                           "tail1": (0, 20, 0), "tail2": (0, 20, 0)},
                     wing=dict(arm=(0.9, -0.1, -0.15), fore=(0.95, -0.2, -0.1), thumb=(0.4, -0.9, 0.0),
                               f1=(0.95, 0.1, 0.0), f2=(0.8, 0.55, 0.0), f3=(0.5, 0.85, 0.0), f4=(0.15, 0.98, 0.0),
                               normal=(0.0, 0.0, -1.0))),
}

# The baby's own poses (solved on the hatchling form, in its units): it stands up like a bat
# pup, so it sits on its round bottom, sleeps sitting up wrapped in its tiny wings, flops on
# its tummy to lie down and tips back onto its back to have it rubbed.
H_SIT_BODY = {"leg_up*": (52, 0, 0), "leg_lo*": (-46, 0, 0), "foot*": (14, 0, 0), "hips": (8, 0, 0), "head": (-6, 0, 0),
              "tail1": (-10, 0, 0), "tail2": (0, 20, 0), "tail3": (0, 26, 0), "tail4": (0, 28, 0), "tail5": (0, 26, 0),
              "tail6": (0, 20, 0)}
H_LIE_BODY = {"hips": (-62, 0, 0), "leg_up*": (-12, 0, 0), "leg_lo*": (-24, 0, 0), "foot*": (10, 0, 0),
              "neck1": (20, 0, 0), "neck3": (14, 0, 0), "head": (24, 0, 0), "tail1": (20, 0, 0), "tail2": (0, 14, 0),
              "tail3": (0, 18, 0), "tail4": (0, 18, 0)}
H_CROUCH_BODY = {"leg_up*": (40, 0, 0), "leg_lo*": (-60, 0, 0), "foot*": (22, 0, 0), "hips": (-6, 0, 0),
                 "head": (4, 0, 0)}
H_POSES = {
    "sit": dict(body=H_SIT_BODY, wing=_stand((0.12, -0.36, 0.26), pole=(0.6, 0.5, -0.1), thumb=(-0.3, -0.85, 0.3),
                                           normal=(0.6, -0.8, 0.1))),
    "curl": dict(body=dict(H_SIT_BODY, **{"hips": (4, 0, 0), "neck1": (-10, 0, 0), "neck3": (-10, 0, 0),
                                          "head": (-18, 0, 10), "ear*": (46, 24, -30)}),
                 wing=_stand((0.07, -0.36, 0.2), pole=(0.6, 0.6, 0.0), thumb=(-0.3, -0.85, 0.3),
                             normal=(0.5, -0.85, 0.2),
                             f1=("fold", (-0.1, 0.0, 0.25)), f2=("fold", (-0.05, 0.1, 0.2)))),
    "lie": dict(body=H_LIE_BODY, wing=_stand((0.34, -0.62, 0.04), pole=(0.3, 0.7, 0.6), thumb=(0.0, -0.95, -0.3),
                                           normal=(0.6, 0.0, 0.8))),
    "eat": dict(body={"hips": (-26, 0, 0), "neck1": (-6, 0, 0), "head": (-18, 0, 0), "leg_up*": (10, 0, 0),
                      "leg_lo*": (-12, 0, 0)},
                wing=_stand((0.34, -0.58, 0.04), pole=(0.2, 0.5, 0.8))),
    "crouch": dict(body=H_CROUCH_BODY, wing=_stand((0.36, -0.4, 0.12), pole=(0.2, 0.6, 0.8))),
    "open": dict(body={}, wing=dict(rest=(-6, -6, 24))),
    "stretch": dict(body={"hips": (-24, 0, 0), "leg_up*": (24, 0, 0), "leg_lo*": (-16, 0, 0), "neck1": (14, 0, 0),
                          "head": (14, 0, 0), "tail1": (24, 0, 0)},
                    wing=dict(rest=(-4, 16, 8))),
    "rear": dict(body={"hips": (6, 0, 0), "head": (-4, 0, 0)},
                 wing=_stand((0.16, -0.5, 0.62), pole=(0.5, 0.4, -0.5), thumb=(0.0, -0.7, 0.7),
                             normal=(0.8, -0.5, 0.2))),
    "bow": dict(body={"hips": (-40, 0, 0), "leg_up*": (30, 0, 0), "leg_lo*": (-20, 0, 0), "neck1": (18, 0, 0),
                      "neck3": (10, 0, 0), "head": (16, 0, 0), "tail1": (30, 0, 0), "tail2": (16, 0, 0)},
                wing=dict(wrist=(0.34, -0.62, 0.03), pole=(0.3, 0.5, 0.8), thumb=(0.1, -0.9, -0.4),
                          f1=(0.85, 0.25, 0.45), f2=(0.62, 0.62, 0.35), f3=(0.38, 0.86, 0.25), f4=(0.12, 0.97, 0.15),
                          normal=(0.1, 0.0, 1.0))),
    "belly_up": dict(body={"hips": (84, 0, 0), "leg_up*": (20, 0, 0), "leg_lo*": (-30, 0, 0), "neck1": (-20, 0, 0),
                           "head": (-20, 0, 0), "tail1": (-30, 0, 0)},
                     wing=dict(arm=(0.9, 0.0, 0.2), fore=(0.9, -0.3, 0.1), thumb=(0.3, -0.9, 0.2),
                               f1=(0.95, 0.1, 0.05), f2=(0.8, 0.55, 0.05), f3=(0.5, 0.85, 0.05), f4=(0.15, 0.98, 0.05),
                               normal=(0.0, 0.0, -1.0))),
}

# --- solved poses (generated by --solve; do not edit by hand) ---
SOLVED = {
    "stand": {"wing_arm": (-117.1, 33.2, -27.2), "wing_fore": (-6.2, -114.2, 9.7), "wing_thumb": (14.3, 2.9, 19.1),
        "wing_f1": (-34.7, 139.6, -54.5), "wing_f2": (-13.1, 114.5, -58.3), "wing_f3": (7.5, 91.9, -49.4),
        "wing_f4": (19.2, 63.8, -29.6)},
    "walk_lift": {"wing_arm": (-19.8, -150.3, 95.4), "wing_fore": (-53.0, -163.0, -8.2),
        "wing_thumb": (12.1, 6.2, 8.9), "wing_f1": (-34.9, 139.4, -54.0), "wing_f2": (-13.7, 113.9, -58.6),
        "wing_f3": (6.8, 91.3, -50.5), "wing_f4": (19.1, 63.4, -31.5)},
    "walk_plant": {"wing_arm": (33.0, -123.8, 46.7), "wing_fore": (-38.9, 99.3, -63.2),
        "wing_thumb": (7.8, -5.9, 17.6), "wing_f1": (-34.6, 139.1, -54.1), "wing_f2": (-13.5, 113.8, -58.4),
        "wing_f3": (6.8, 91.3, -50.1), "wing_f4": (18.8, 63.3, -31.0)},
    "walk_push": {"wing_arm": (-107.7, 49.4, -51.2), "wing_fore": (-37.1, -102.7, 1.5),
        "wing_thumb": (15.1, 26.6, -5.7), "wing_f1": (-34.9, 139.4, -54.0), "wing_f2": (-13.8, 113.8, -58.6),
        "wing_f3": (6.8, 91.3, -50.6), "wing_f4": (19.2, 63.4, -31.7)},
    "run_lift": {"wing_arm": (-135.3, 20.4, -112.5), "wing_fore": (-53.0, -170.9, -36.9),
        "wing_thumb": (10.3, 15.8, -6.2), "wing_f1": (-35.6, 139.8, -53.4), "wing_f2": (-14.6, 113.5, -59.2),
        "wing_f3": (6.2, 91.0, -52.2), "wing_f4": (19.6, 63.5, -34.1)},
    "run_plant": {"wing_arm": (33.5, -126.0, 36.9), "wing_fore": (-25.7, 96.6, -60.2),
        "wing_thumb": (9.9, -1.8, 12.9), "wing_f1": (-35.0, 139.4, -54.0), "wing_f2": (-13.8, 113.8, -58.7),
        "wing_f3": (6.8, 91.3, -50.7), "wing_f4": (19.2, 63.4, -31.7)},
    "run_push": {"wing_arm": (-113.5, 35.4, -74.2), "wing_fore": (-43.9, -108.6, -1.5),
        "wing_thumb": (14.1, 28.6, -8.4), "wing_f1": (-34.8, 139.2, -53.9), "wing_f2": (-13.8, 113.6, -58.6),
        "wing_f3": (6.6, 91.1, -50.7), "wing_f4": (19.0, 63.2, -31.9)},
    "crouch": {"wing_arm": (-121.0, 25.9, -105.6), "wing_fore": (-53.2, -169.6, -35.8),
        "wing_thumb": (9.7, 3.8, -0.9), "wing_f1": (-35.7, 140.0, -53.3), "wing_f2": (-14.8, 113.4, -59.3),
        "wing_f3": (6.1, 91.0, -52.5), "wing_f4": (19.6, 63.6, -34.7)},
    "stalk_lift": {"wing_arm": (-115.9, 25.2, -107.5), "wing_fore": (-53.0, -170.0, -36.1),
        "wing_thumb": (8.8, 15.9, -10.4), "wing_f1": (-36.0, 140.3, -53.0), "wing_f2": (-15.2, 113.4, -59.5),
        "wing_f3": (5.7, 91.0, -53.0), "wing_f4": (19.5, 63.8, -35.5)},
    "stalk_plant": {"wing_arm": (-114.1, 29.1, -99.6), "wing_fore": (-53.0, -170.9, -36.9),
        "wing_thumb": (11.6, 12.1, -4.2), "wing_f1": (-36.0, 140.5, -53.2), "wing_f2": (-15.0, 113.9, -59.4),
        "wing_f3": (5.9, 91.5, -52.6), "wing_f4": (19.5, 64.3, -34.8)},
    "stalk_push": {"wing_arm": (-128.0, 24.1, -110.8), "wing_fore": (-53.5, -167.9, -34.7),
        "wing_thumb": (7.5, 23.9, -14.3), "wing_f1": (-35.5, 139.4, -53.3), "wing_f2": (-14.7, 113.0, -59.4),
        "wing_f3": (6.2, 90.6, -52.5), "wing_f4": (19.7, 63.2, -34.7)},
    "eat": {"wing_arm": (-155.7, 29.4, -47.6), "wing_fore": (-38.8, -141.5, 32.4), "wing_thumb": (10.7, -0.4, 12.7),
        "wing_f1": (-35.0, 139.5, -54.0), "wing_f2": (-13.7, 113.9, -58.7), "wing_f3": (6.9, 91.4, -50.6),
        "wing_f4": (19.2, 63.5, -31.6)},
    "sit": {"wing_arm": (8.9, 177.0, 67.4), "wing_fore": (-54.0, -165.1, -35.0), "wing_thumb": (23.6, -18.1, 27.0),
        "wing_f1": (-34.5, 138.7, -54.9), "wing_f2": (-12.2, 113.8, -60.6), "wing_f3": (10.0, 92.6, -53.1),
        "wing_f4": (24.8, 65.3, -33.3)},
    "lie": {"wing_arm": (27.8, -72.7, -31.4), "wing_fore": (-31.8, 132.9, -54.3), "wing_thumb": (-13.7, -6.6, -4.4),
        "wing_f1": (-34.6, 137.0, -50.7), "wing_f2": (-17.4, 117.1, -59.0), "wing_f3": (1.7, 102.6, -55.8),
        "wing_f4": (16.3, 85.7, -43.1)},
    "curl": {"wing_arm": (3.5, -83.7, -20.3), "wing_fore": (-36.4, 143.3, -62.3), "wing_thumb": (-30.1, 1.2, -14.6),
        "wing_f1": (-26.1, 125.5, -42.2), "wing_f2": (-13.2, 105.4, -51.1), "wing_f3": (3.1, 91.6, -50.2),
        "wing_f4": (16.7, 78.2, -40.3)},
    "bow": {"wing_arm": (-9.2, 26.8, 17.9), "wing_fore": (6.1, -102.5, -40.2), "wing_thumb": (39.8, 33.7, 16.7),
        "wing_f1": (49.5, 94.7, -15.1), "wing_f2": (45.3, 85.3, -25.1), "wing_f3": (55.1, 70.1, -21.1),
        "wing_f4": (62.7, 45.3, 4.4)},
    "stretch": {"wing_arm": (17.2, 14.5, 5.6), "wing_fore": (-0.0, -0.0, 0.0), "wing_thumb": (-0.0, -0.0, 0.0),
        "wing_f1": (0.0, 0.0, -0.0), "wing_f2": (-0.0, -0.0, 0.0), "wing_f3": (-0.0, -0.0, 0.0),
        "wing_f4": (-0.0, -0.0, 0.0)},
    "flare": {"wing_arm": (-138.2, 22.4, -34.6), "wing_fore": (-10.7, -127.4, 22.0), "wing_thumb": (12.9, 0.8, 20.4),
        "wing_f1": (-37.0, 141.8, -54.5), "wing_f2": (-6.4, 106.0, -52.2), "wing_f3": (15.5, 69.4, -32.3),
        "wing_f4": (18.7, 29.6, -3.5)},
    "rear": {"wing_arm": (-111.3, 62.0, -38.5), "wing_fore": (-56.2, -98.7, 13.5), "wing_thumb": (26.4, -26.1, 11.9),
        "wing_f1": (-36.5, 142.6, -53.0), "wing_f2": (-15.7, 115.5, -60.7), "wing_f3": (5.6, 94.2, -55.3),
        "wing_f4": (20.6, 68.7, -38.8)},
    "open": {"wing_arm": (-47.5, -11.6, 11.0), "wing_fore": (0.0, 0.0, -0.0), "wing_thumb": (-0.0, -0.0, 0.0),
        "wing_f1": (-0.0, 0.0, -0.0), "wing_f2": (-0.0, -0.0, 0.0), "wing_f3": (-0.0, 0.0, 0.0),
        "wing_f4": (-0.0, -0.0, 0.0)},
    "fly": {"wing_arm": (-0.0, -0.0, -0.0), "wing_fore": (0.0, 0.0, -0.0), "wing_thumb": (-0.0, -0.0, 0.0),
        "wing_f1": (-0.0, -0.0, 0.0), "wing_f2": (-0.0, -0.0, 0.0), "wing_f3": (-0.0, 0.0, 0.0),
        "wing_f4": (-0.0, -0.0, 0.0)},
    "glide": {"wing_arm": (-0.0, -4.0, -6.0), "wing_fore": (0.0, 0.0, 0.0), "wing_thumb": (-0.0, -0.0, -0.0),
        "wing_f1": (-0.0, 0.0, -0.0), "wing_f2": (-0.0, -0.0, 0.0), "wing_f3": (-0.0, -0.0, 0.0),
        "wing_f4": (-0.0, -0.0, 0.0)},
    "dive": {"wing_arm": (-31.8, 14.1, -15.6), "wing_fore": (0.0, 39.1, -33.3), "wing_thumb": (-3.7, -84.3, 26.0),
        "wing_f1": (0.6, 1.9, -8.8), "wing_f2": (-0.1, -25.1, -8.7), "wing_f3": (1.0, -51.1, -3.2),
        "wing_f4": (-2.8, -75.1, 6.8)},
    "belly_up": {"wing_arm": (-24.8, 168.0, -20.1), "wing_fore": (7.6, 29.8, -10.0),
        "wing_thumb": (-2.7, 96.1, -35.6), "wing_f1": (-12.4, -35.5, -0.4), "wing_f2": (-29.7, -90.6, -6.8),
        "wing_f3": (-33.4, -150.1, -26.3), "wing_f4": (-22.6, 157.5, -38.6)},
}
# --- end of solved poses ---
# --- solved baby poses (generated by --solve --stage hatchling) ---
SOLVED_H = {
    "sit": {"wing_arm": (-31.6, -80.5, -120.2), "wing_fore": (-20.4, -62.5, 28.7), "wing_thumb": (35.6, -15.6, 8.8),
        "wing_f1": (-30.1, 147.0, -50.7), "wing_f2": (-10.4, 119.8, -58.0), "wing_f3": (10.3, 97.7, -52.9),
        "wing_f4": (25.3, 71.3, -36.7)},
    "curl": {"wing_arm": (-15.6, -76.4, -114.7), "wing_fore": (-23.6, -38.5, 31.7), "wing_thumb": (41.3, -22.0, 20.2),
        "wing_f1": (-32.9, 159.8, -45.0), "wing_f2": (-13.9, 129.9, -55.6), "wing_f3": (11.4, 99.0, -53.2),
        "wing_f4": (26.9, 72.9, -36.4)},
    "lie": {"wing_arm": (42.9, -20.9, -33.3), "wing_fore": (-18.9, -51.2, -30.3), "wing_thumb": (26.2, 23.0, 18.3),
        "wing_f1": (-30.0, 145.2, -50.5), "wing_f2": (-7.6, 120.1, -54.7), "wing_f3": (13.2, 97.6, -45.9),
        "wing_f4": (25.2, 69.4, -25.3)},
    "eat": {"wing_arm": (55.7, -61.7, -43.7), "wing_fore": (6.4, 22.8, -10.9), "wing_thumb": (12.1, 11.3, 3.0),
        "wing_f1": (-29.9, 144.8, -50.0), "wing_f2": (-10.6, 117.6, -54.1), "wing_f3": (7.8, 93.1, -46.8),
        "wing_f4": (18.7, 64.5, -29.6)},
    "crouch": {"wing_arm": (3.2, -24.9, -37.2), "wing_fore": (16.1, -19.6, -51.2), "wing_thumb": (16.1, 14.8, 6.9),
        "wing_f1": (-29.8, 144.9, -50.4), "wing_f2": (-10.1, 118.1, -54.1), "wing_f3": (8.4, 93.7, -46.1),
        "wing_f4": (18.9, 65.0, -28.3)},
    "open": {"wing_arm": (-6.0, -6.0, 24.0), "wing_fore": (-0.0, -0.0, -0.0), "wing_thumb": (-0.0, -0.0, 0.0),
        "wing_f1": (0.0, -0.0, 0.0), "wing_f2": (0.0, -0.0, 0.0), "wing_f3": (0.0, -0.0, 0.0),
        "wing_f4": (-0.0, 0.0, 0.0)},
    "stretch": {"wing_arm": (19.0, 16.9, 1.2), "wing_fore": (-0.0, -0.0, -0.0), "wing_thumb": (-0.0, -0.0, 0.0),
        "wing_f1": (-0.0, -0.0, 0.0), "wing_f2": (-0.0, -0.0, 0.0), "wing_f3": (-0.0, -0.0, 0.0),
        "wing_f4": (-0.0, -0.0, 0.0)},
    "rear": {"wing_arm": (-19.9, -56.0, -95.2), "wing_fore": (-21.1, -81.9, 25.0), "wing_thumb": (34.0, -4.0, 6.1),
        "wing_f1": (-30.6, 149.2, -49.5), "wing_f2": (-12.4, 122.4, -56.0), "wing_f3": (6.4, 100.5, -51.1),
        "wing_f4": (19.4, 75.3, -36.6)},
    "bow": {"wing_arm": (17.5, -60.5, -18.3), "wing_fore": (-31.8, -14.3, -22.5), "wing_thumb": (42.8, 43.1, 4.6),
        "wing_f1": (46.8, 104.3, -23.3), "wing_f2": (42.4, 95.6, -33.1), "wing_f3": (52.4, 84.1, -33.0),
        "wing_f4": (62.4, 64.8, -14.1)},
    "belly_up": {"wing_arm": (65.0, -68.7, 35.6), "wing_fore": (15.8, 44.3, -8.7), "wing_thumb": (1.7, 94.7, -44.1),
        "wing_f1": (-18.9, -36.3, 7.2), "wing_f2": (-39.5, -95.4, -11.6), "wing_f3": (-34.1, -159.5, -41.7),
        "wing_f4": (-12.3, 154.1, -50.4)},
}
# --- end of solved baby poses ---


def wing_pose(name, side=None, baby=False):
    """A solved wing pose as clip keys: both wings ("wing_arm*": ...), or one side's."""
    keys = SOLVED_H[name] if baby else SOLVED[name]
    if side is None:
        return {f"{b}*": v for b, v in keys.items()}
    return {f"{b}_{side}": (v if side == "R" else (v[0], -v[1], -v[2])) for b, v in keys.items()}


# ------------------------------------------------------------------------------ clips
def merge(*poses):
    out = {}
    for p in poses:
        for bone, v in p.items():
            a = out.get(bone, (0.0, 0.0, 0.0))
            out[bone] = tuple(x + y for x, y in zip(a, v))
    return out


def P(name, *more):
    """A whole pose (body and both wings) as clip keys, with extra keys added on top."""
    return merge(POSES[name]["body"], wing_pose(name), *more)


def sin01(t, period, phase=0.0):
    return math.sin(2 * math.pi * (t / period + phase))


def breathe(amount=1.0, period=3.4):
    """Slow breaths: the chest rises, the neck and head ride it."""
    def fn(t):
        s = sin01(t, period) * amount
        return {"chest": (1.0 * s, 0, 0), "belly": (-0.5 * s, 0, 0), "neck1": (0.8 * s, 0, 0), "head": (-0.8 * s, 0, 0)}
    return fn


def tail_sway(amount=1.0, period=3.4):
    """A whip tail's lazy S: each bone lags the one before and swings wider."""
    def fn(t):
        return {f"tail{k}": (0, amount * (1.5 + 1.4 * k) * sin01(t, period, -0.1 * k), 0) for k in range(1, 7)}
    return fn


def tail_wave(amount, period, lift=0.0):
    """A quicker wag along the whole tail (happy), optionally lifted."""
    def fn(t):
        return {f"tail{k}": (lift if k <= 2 else 0.0, amount * (0.5 + 0.35 * k) * sin01(t, period, -0.08 * k), 0)
                for k in range(1, 7)}
    return fn


def ear_flicks(period=4.4, amount=1.0):
    """Ears that swivel: slow turns out and back, now and then a quick flick of one ear."""
    def fn(t):
        s = sin01(t, period) * amount
        u = (t % period) / period
        flick = math.sin(math.pi * min(1.0, max(0.0, (u - 0.62) / 0.08))) * amount
        return {"ear_R": (6 * flick, 10 * s - 14 * flick, -4 * flick), "ear_L": (0, -10 * s, 0)}
    return fn


def ears(pitch=0.0, yaw=0.0, roll=0.0):
    """Both ears: pitch + lays them back, yaw + turns them out, roll - droops them out."""
    return {"ear*": (pitch, yaw, roll)}


def hind_legs(period, amp, bend, phases, bob=1.5):
    """Walking hind legs: each swings (pitch) round its phase and bends while it swings
    forward (at its phase it is mid-swing, lifted); the body rolls and bobs a little."""
    def fn(t):
        out = {}
        for side, ph in phases.items():
            swing = sin01(t, period, -ph)
            lift = max(0.0, math.cos(2 * math.pi * (t / period - ph)))
            out[f"leg_up_{side}"] = (amp * swing, 0, 0)
            out[f"leg_lo_{side}"] = (-bend * lift, 0, 0)
            out[f"foot_{side}"] = (0.5 * bend * lift, 0, 0)
        b = sin01(t, period / 2)
        out["chest"] = (bob * 0.4 * b, 0, 1.2 * sin01(t, period))
        out["hips"] = (0, 2.0 * sin01(t, period, 0.25), 0)
        out["neck1"] = (-bob * b, 0, 0)
        out["head"] = (bob * 0.7 * b, 0, 0)
        return out
    return fn


STRIDE = ("walk_lift", "walk_plant", "stand", "walk_push")   # mid-swing, planted forward, under, pushed back
BOUND = ("run_lift", "run_plant", "stand", "run_push")


def stride(c, period, phases, poses=STRIDE, cycles=1, extra=None):
    """Key the wings' stride: each wing goes lift, plant, under, push from its phase (the
    wrist lifted mid-swing at the phase, planted a quarter later), in step with hind_legs."""
    for side, ph in phases.items():
        for k in range(cycles):
            for f, name in zip((0.0, 0.25, 0.5, 0.75), poses):
                t = round(((ph + f + k) * period) % (period * cycles), 4)
                keys = wing_pose(name, side)
                if extra:
                    keys = merge(keys, {b: v for b, v in extra.items() if b in keys})
                c.key(t, **keys)
    return c


def footsteps(c, period, phases):
    for ph in phases.values():
        c.event(((ph + 0.25) * period) % period, "footstep")


def chomp(amount=20.0, period=0.6, at=0.15):
    """The jaw opens before each bite and snaps shut on it."""
    def fn(t):
        u = ((t - at) / period) % 1.0
        return {"jaw": (-amount * math.sin(math.pi * (u - 0.5) / 0.5) ** 0.7 if u > 0.5 else 0.0, 0, 0)}
    return fn


def pant(amount=6.0, period=0.3):
    return lambda t: {"jaw": (-amount - 3 * sin01(t, period), 0, 0)}


def wingbeat(period, amount, phase=0.0, fold=0.35):
    """A deep wingbeat: the arms sweep down and up (roll), the hands lag and fold a little on
    the upstroke; the body rises and falls against it."""
    def fn(t):
        s = sin01(t, period, phase)
        h = sin01(t, period, phase - 0.12)
        up = max(0.0, -math.cos(2 * math.pi * (t / period + phase)))  # the upstroke
        out = {"wing_arm_R": (0, 0, amount * s), "wing_arm_L": (0, 0, -amount * s),
               "wing_fore_R": (0, 0, fold * amount * h), "wing_fore_L": (0, 0, -fold * amount * h),
               "chest": (2.0 * s, 0, 0), "neck1": (-1.6 * s, 0, 0), "head": (1.2 * s, 0, 0)}
        for k, f in ((1, 0.2), (2, 0.35), (3, 0.5), (4, 0.6)):
            out[f"wing_f{k}_R"] = (0, -f * 14 * up, 0)
            out[f"wing_f{k}_L"] = (0, f * 14 * up, 0)
        return out
    return fn


WALK_PHASES_LEGS = {"L": 0.0, "R": 0.5}
WALK_PHASES_WINGS = {"L": 0.25, "R": 0.75}          # a lateral four-beat: LH, LW, RH, RW
TROT_PHASES_LEGS = {"L": 0.0, "R": 0.5}
TROT_PHASES_WINGS = {"R": 0.0, "L": 0.5}            # diagonal pairs
BOUND_LEGS = {"L": 0.5, "R": 0.55}                  # the bound: both wings, then both feet
BOUND_WINGS = {"L": 0.0, "R": 0.04}


YAWNING = {"neck1": (18, 0, 0), "head": (20, 0, 0), "snout": (6, 0, 0), "jaw": (-30, 0, 0), "ear*": (20, 0, 0)}


def _tables():
    """The poses the two clip sets are built from: the grown dragon's and the baby's (it stands
    up like a bat pup, so its sitting, lying, sleeping and play poses are its own)."""
    def H(name, *more):
        return merge(H_POSES[name]["body"], wing_pose(name, baby=True), *more)

    grown = dict(STAND=P("stand"), SIT=P("sit"), LIE=P("lie"), CURL=P("curl"), CROUCH=P("crouch"), FLARE=P("flare"),
                 OPEN=P("open"), REAR=P("rear"), EAT=P("eat"), BOW=P("bow"), BELLY=P("belly_up"),
                 CROUCH_BODY=POSES["crouch"]["body"], OPEN_R=wing_pose("open", "R"), REAR_L=wing_pose("rear", "L"),
                 SULK=merge(P("lie", ears(30, 14, -26)),
                            {"neck1": (-14, -22, 0), "neck2": (-8, -16, 0), "head": (-22, -14, -10),
                             "tail1": (0, -30, 0), "tail2": (0, -36, 0), "tail3": (0, -44, 0), "tail4": (0, -44, 0),
                             "tail5": (0, -40, 0), "tail6": (0, -36, 0)}),
                 PICK={"neck1": (-36, 0, 0), "neck2": (-20, 0, 0), "neck3": (-8, 0, 0), "head": (-22, 0, 0),
                       "chest": (-8, 0, 0), "ear*": (-6, -10, 0)},
                 TUG={"hips": (6, 0, 0), "leg_up*": (20, 0, 0), "leg_lo*": (-30, 0, 0), "foot*": (10, 0, 0),
                      "chest": (-4, 0, 0), "neck1": (-26, 0, 0), "neck2": (-10, 0, 0), "head": (6, 0, 0),
                      "jaw": (-5, 0, 0), "tail1": (18, 0, 0), "ear*": (20, 0, 0)},
                 POUNCE_BODY={"hips": (-30, 0, 0), "leg_up*": (-26, 0, 0), "leg_lo*": (14, 0, 0), "tail1": (30, 0, 0)},
                 STRETCH=merge(POSES["stretch"]["body"], wing_pose("bow", "L"), wing_pose("stretch", "R"), YAWNING))
    baby = dict(STAND=P("stand"), SIT=H("sit"), LIE=H("lie"), CURL=H("curl"), CROUCH=H("crouch"), FLARE=P("flare"),
                OPEN=H("open"), REAR=H("rear"), EAT=H("eat"), BOW=H("bow"), BELLY=H("belly_up"),
                CROUCH_BODY=H_POSES["crouch"]["body"], OPEN_R=wing_pose("open", "R", baby=True),
                REAR_L=wing_pose("rear", "L", baby=True),
                SULK=merge(H("sit", ears(34, 20, -34)),  # sitting with its back half turned, head hung
                           {"hips": (0, -34, 0), "neck1": (-10, -10, 0), "head": (-24, -12, -8),
                            "tail2": (0, -40, 0), "tail3": (0, -50, 0), "tail4": (0, -50, 0)}),
                PICK={"hips": (-10, 0, 0), "neck1": (-10, 0, 0), "head": (-26, 0, 0), "ear*": (-6, -10, 0)},
                TUG={"hips": (-16, 0, 0), "leg_up*": (26, 0, 0), "leg_lo*": (-30, 0, 0), "foot*": (8, 0, 0),
                     "neck1": (-6, 0, 0), "head": (-14, 0, 0), "jaw": (-5, 0, 0), "ear*": (20, 0, 0)},
                POUNCE_BODY={"hips": (-40, 0, 0), "leg_up*": (-30, 0, 0), "leg_lo*": (10, 0, 0), "tail1": (30, 0, 0)},
                STRETCH=merge(H_POSES["stretch"]["body"], wing_pose("bow", "L", baby=True),
                              wing_pose("stretch", "R", baby=True), YAWNING))
    return grown, baby


# the clips the baby has its own version of ("<name>_h"): every one that poses its body
BABY_CLIPS = ("walk", "trot", "carry", "sit", "sit_loop", "lie_down", "lie_loop", "curl_up", "sleep", "wake",
              "nap_flop", "eat", "roll_over", "belly_rub", "hop", "pounce", "play_bow", "spar", "stalk", "sulk",
              "sulk_loop", "greet", "pick_up", "drop_wait", "leap_catch", "leg_kick", "lift_wing", "paw_bat", "tug",
              "wing_flutter")


def _clip_set(T, baby):
    from dragons.clipkit import Clip
    out = []

    def clip(*args, **kwargs):
        c = Clip(*args, **kwargs)
        out.append(c)
        return c

    STAND, SIT, LIE, CURL, CROUCH = T["STAND"], T["SIT"], T["LIE"], T["CURL"], T["CROUCH"]
    FLARE, OPEN, REAR, EAT = T["FLARE"], T["OPEN"], T["REAR"], T["EAT"]
    BOW, BELLY, SULK = T["BOW"], T["BELLY"], T["SULK"]

    # ---------------------------------------------------------------- idle & looking
    clip("idle", 3.4, loop=True).pose(0.0, STAND).wave(breathe()).wave(tail_sway()).wave(ear_flicks())
    (clip("look_around", 3.0).pose(0.0, STAND)
     .pose(0.8, merge(STAND, {"neck2": (4, 16, 0), "neck3": (2, 14, 0), "head": (6, 12, 6), "ear_R": (0, -18, 0),
                              "ear_L": (0, 14, 0)}))
     .pose(1.5, merge(STAND, {"neck2": (4, 16, 0), "neck3": (2, 14, 0), "head": (6, 12, 6), "ear_R": (0, -10, 0),
                              "ear_L": (0, 22, 0)}))
     .pose(2.2, merge(STAND, {"neck2": (2, -18, 0), "neck3": (0, -16, 0), "head": (4, -14, -6), "ear_R": (0, 16, 0),
                              "ear_L": (0, -16, 0)}))
     .pose(3.0, STAND).wave(breathe()).wave(tail_sway()).event(0.8, "sniff"))
    SCRATCH = merge(STAND, {"leg_up_R": (72, -16, 0), "leg_lo_R": (-44, 0, 0), "foot_R": (24, 0, 0), "hips": (4, 0, -8),
                            "neck1": (-8, -26, 0), "neck2": (0, -20, 0), "head": (-6, -12, -20), "ear_R": (20, 0, -20)})
    (clip("scratch", 2.1).pose(0.0, STAND).pose(0.4, SCRATCH).pose(1.7, SCRATCH).pose(2.1, STAND)
     .wave(lambda t: {"leg_lo_R": (10 * sin01(t, 0.18), 0, 0), "foot_R": (14 * sin01(t, 0.18), 0, 0)}
           if 0.5 < t < 1.6 else {}))

    # ---------------------------------------------------------------- walking on the wrists
    # The grown dragon crawls like a bat, wrists and feet in a slow four-beat; the baby toddles
    # on its hind legs twice as fast, its wing-arms swinging and its round body rocking.
    wk = 0.56 if baby else 1.1
    waddle = (lambda t: {"hips": (0, 0, 6 * sin01(t, wk)), "chest": (0, 0, -4 * sin01(t, wk)),
                         "head": (0, 0, 3 * sin01(t, wk, 0.1)), "ear_R": (4 * sin01(t, wk / 2), 0, 0),
                         "ear_L": (4 * sin01(t, wk / 2, 0.5), 0, 0)}) if baby else (lambda t: {})
    walk = clip("walk", wk, loop=True, speed=0.5 if baby else 0.55)
    stride(walk, wk, WALK_PHASES_WINGS)
    walk.wave(hind_legs(wk, 30 if baby else 18, 40 if baby else 34, WALK_PHASES_LEGS, bob=3.0 if baby else 1.5))
    walk.wave(tail_sway(1.4 if baby else 1.1, wk)).wave(waddle)
    if not baby:
        walk.wave(ear_flicks(2.2, 0.5))
    footsteps(walk, wk, {"a": 0.0, "b": 0.5} if baby else dict(WALK_PHASES_WINGS, hL=0.0, hR=0.5))
    tk = 0.38 if baby else 0.62
    trot = clip("trot", tk, loop=True, speed=1.6 if baby else 1.8)
    stride(trot, tk, TROT_PHASES_WINGS)
    trot.wave(hind_legs(tk, 30 if baby else 20, 46 if baby else 44, TROT_PHASES_LEGS, bob=3.4 if baby else 2.4))
    trot.wave(tail_sway(1.2, tk)).wave(pant(5.0, tk / 2)).wave(waddle)
    footsteps(trot, tk, {"a": 0.0, "b": 0.5})
    if not baby:
        shuffle = clip("shuffle", 1.0, loop=True)
        stride(shuffle, 1.0, WALK_PHASES_WINGS)
        shuffle.wave(hind_legs(1.0, 8, 22, WALK_PHASES_LEGS, bob=0.8))
        footsteps(shuffle, 1.0, WALK_PHASES_WINGS)
    carry = clip("carry", wk, loop=True, speed=0.5 if baby else 0.55).pose(0.0, {"neck1": (12, 0, 0),
                                                                                  "head": (-8, 0, 0)})
    stride(carry, wk, WALK_PHASES_WINGS)
    carry.wave(hind_legs(wk, 30 if baby else 18, 40 if baby else 34, WALK_PHASES_LEGS)).wave(tail_sway(1.6, wk / 2))
    carry.wave(waddle)
    if not baby:
        # Running: a bat's bound. Both wrists plant and throw the body forward, the back arches,
        # then both hind feet land under it; the folded wings bounce, the ears stream back.
        gallop = clip("gallop", 0.56, loop=True, speed=3.4).pose(
            0.0, {"neck1": (-10, 0, 0), "neck2": (-4, 0, 0), "head": (12, 0, 0), "tail1": (8, 0, 0),
                  "ear*": (26, 0, 0)})
        stride(gallop, 0.56, BOUND_WINGS, BOUND)
        gallop.wave(hind_legs(0.56, 30, 56, BOUND_LEGS, bob=3.0))
        gallop.wave(lambda t: {"chest": (7 * sin01(t, 0.56, 0.1), 0, 0), "hips": (-6 * sin01(t, 0.56, 0.1), 0, 0),
                               "tail1": (5 * sin01(t, 0.56, 0.1), 0, 0)})
        gallop.wave(tail_sway(0.7, 0.56)).wave(pant(4.0, 0.28))
        gallop.root(0.0, up=0.02).root(0.14, up=0.16).root(0.28, up=0.03).root(0.42, up=0.1)
        footsteps(gallop, 0.56, {"w": 0.0, "h": 0.5})
        # the scamper (the baby's run) on a grown body: the bound, quicker
        scamper = clip("scamper", 0.46, loop=True, speed=3.0).pose(0.0, {"neck1": (-8, 0, 0), "head": (10, 0, 0),
                                                                         "ear*": (22, 0, 0)})
        stride(scamper, 0.46, BOUND_WINGS, BOUND)
        scamper.wave(hind_legs(0.46, 28, 50, BOUND_LEGS, bob=3.0)).wave(pant(6.0, 0.23)).wave(tail_sway(0.8, 0.46))
        scamper.root(0.0, up=0.0).root(0.15, up=0.14).root(0.3, up=0.03)
        footsteps(scamper, 0.46, {"w": 0.0, "h": 0.5})

    # ---------------------------------------------------------------- rest: the cloak
    clip("sit", 1.0).pose(0.0, STAND).pose(0.45, merge(CROUCH, {"hips": (0 if baby else 14, 0, 0)})).pose(1.0, SIT)
    (clip("sit_loop", 3.4, loop=True).pose(0.0, SIT).wave(breathe(0.9)).wave(ear_flicks(4.0))
     .wave(lambda t: {"tail5": (0, 6 * sin01(t, 3.4), 0), "tail6": (0, 12 * sin01(t, 3.4, -0.1), 0)}))
    clip("lie_down", 1.2).pose(0.0, STAND).pose(0.55, CROUCH).pose(1.2, LIE).event(1.0, "thump")
    (clip("lie_loop", 4.0, loop=True).pose(0.0, LIE).wave(breathe(1.2, 4.0)).wave(tail_sway(0.4, 4.0))
     .wave(ear_flicks(5.0, 0.6)))
    if baby:  # from its tummy it sits up and nods off, wrapped in its wings
        clip("curl_up", 1.8).pose(0.0, LIE).pose(0.8, SIT).pose(1.8, CURL)
    else:
        (clip("curl_up", 1.6).pose(0.0, LIE).pose(0.7, merge(LIE, {"neck1": (-6, 16, 0), "head": (-10, 6, 6)}))
         .pose(1.6, CURL))
    sleep = clip("sleep", 5.0, loop=True).pose(0.0, CURL).wave(breathe(1.8, 5.0))
    if baby:  # a sleepy nod
        sleep.wave(lambda t: {"head": (-4 * max(0.0, sin01(t, 5.0)), 0, 2 * sin01(t, 5.0)),
                              "ear_R": (0, 0, -3 * sin01(t, 5.0, 0.2))})
    (clip("wake", 2.8).pose(0.0, CURL).pose(0.7, SIT if baby else LIE).pose(1.4, T["STRETCH"])
     .pose(2.0, merge(FLARE, {"head": (6, 0, 0)})).pose(2.8, STAND).event(1.4, "yawn").event(2.0, "flap"))
    YAWN = merge(STAND, {"neck1": (14, 0, 0), "neck2": (8, 0, 0), "head": (30, 0, 0), "snout": (6, 0, 0),
                         "jaw": (-32, 0, 0), "chest": (4, 0, 0), "ear*": (18, 0, -8)})
    (clip("yawn", 1.6).pose(0.0, STAND).pose(0.6, YAWN).pose(1.1, merge(YAWN, {"jaw": (-4, 0, 0)})).pose(1.6, STAND)
     .event(0.6, "yawn"))
    clip("nap_flop", 0.8).pose(0.0, STAND).pose(0.8, LIE).event(0.6, "thump")

    # ---------------------------------------------------------------- care reactions
    eat = clip("eat", 1.2, loop=True).pose(0.0, EAT)
    eat.wave(lambda t: {"head": (8 * max(0.0, sin01(t, 0.6)), 0, 0), "snout": (6 * max(0.0, sin01(t, 0.6)), 0, 0),
                        "neck1": (3 * sin01(t, 1.2), 0, 0)}).wave(chomp(24 if baby else 20)).wave(ear_flicks(1.2, 0.5))
    eat.event(0.15, "chomp").event(0.75, "chomp")
    HAPPY = merge(FLARE, {"neck1": (10, 0, 0), "head": (12, 0, 0), "jaw": (-16, 0, 0), "ear*": (-6, 12, 0)})
    (clip("fav_wiggle", 1.6).pose(0.0, STAND).pose(0.3, HAPPY).pose(1.3, HAPPY).pose(1.6, STAND)
     .wave(lambda t: {"hips": (0, 6 * sin01(t, 0.4), 0), "chest": (0, -4 * sin01(t, 0.4), 0),
                      **{f"tail{k}": (0, 12 * sin01(t, 0.3, -0.08 * k), 0) for k in range(1, 7)}})
     .root(0.0).root(0.55, up=0.0).root(0.75, up=0.2).root(0.95, up=0.0).root(1.6).event(0.95, "land")
     .event(0.3, "call"))
    (clip("pet_head", 1.6, loop=True)
     .pose(0.0, merge(STAND, {"neck1": (6, 0, 0), "neck3": (4, 0, 6), "head": (-10, 6, 16), "jaw": (-6, 0, 0),
                              "ear*": (34, 10, -12)}))
     .wave(lambda t: {"head": (0, 4 * sin01(t, 1.6), 4 * sin01(t, 1.6)), "neck2": (0, 3 * sin01(t, 1.6, 0.1), 0)})
     .wave(tail_sway(0.8, 0.8)).event(0.4, "purr"))
    (clip("pet_chin", 1.6, loop=True)
     .pose(0.0, merge(STAND, {"neck1": (14, 0, 0), "neck2": (8, 0, 0), "head": (26, 0, 0), "jaw": (-8, 0, 0),
                              "ear*": (26, 6, -6)}))
     .wave(lambda t: {"head": (4 * sin01(t, 1.6), 0, 0), "jaw": (-3 * sin01(t, 1.6, 0.2), 0, 0)})
     .wave(tail_sway(0.8, 0.8)).event(0.4, "purr"))
    BELLY_UP = merge(BELLY, ears(20, 20, 0))
    clip("roll_over", 1.2).pose(0.0, STAND).pose(0.5, SIT if baby else LIE).pose(1.2, BELLY_UP).event(0.9, "thump")
    (clip("belly_rub", 1.2, loop=True).pose(0.0, BELLY_UP)
     .wave(lambda t: {"leg_up_L": (10 * sin01(t, 0.6), 0, 0), "leg_up_R": (10 * sin01(t, 0.6, 0.5), 0, 0),
                      "wing_fore_L": (0, 0, 6 * sin01(t, 0.6, 0.25)), "wing_fore_R": (0, 0, -6 * sin01(t, 0.6, 0.75)),
                      "hips": (0, 0, 5 * sin01(t, 1.2))})
     .wave(pant(9.0, 0.6)).event(0.3, "purr"))

    def shake_wave(t):
        k = max(0.0, 1.0 - abs(t - 0.55) / 0.42)
        s = sin01(t, 0.12) * k
        return {"chest": (0, 0, 16 * s), "hips": (0, 0, -10 * s), "neck1": (0, 0, 14 * s), "head": (0, 0, 18 * s),
                "ear_R": (0, 0, 22 * s), "ear_L": (0, 0, 22 * s), "tail3": (0, 14 * s, 0), "tail5": (0, -18 * s, 0),
                **{f"wing_f{j}_R": (0, 0, 10 * s) for j in range(1, 5)},
                **{f"wing_f{j}_L": (0, 0, 10 * s) for j in range(1, 5)}}

    (clip("shake", 1.1).pose(0.0, STAND).pose(0.2, FLARE).pose(0.9, FLARE).pose(1.1, STAND).wave(shake_wave)
     .event(0.4, "shake"))
    (clip("hop", 1.0).pose(0.0, STAND).pose(0.25, CROUCH)
     .pose(0.45, merge(OPEN, {"neck1": (6, 0, 0), "head": (8, 0, 0)}))
     .pose(0.72, CROUCH).pose(1.0, STAND)
     .root(0.0).root(0.25).root(0.45, up=0.55).root(0.7).root(1.0).event(0.7, "land").event(0.35, "squeak")
     .event(0.4, "flap"))

    # ---------------------------------------------------------------- play
    POUNCE_AIR = merge(OPEN, T["POUNCE_BODY"], {"neck1": (-6, 0, 0), "head": (6, 0, 0), "jaw": (-20, 0, 0),
                                                "ear*": (26, 0, 0)})
    (clip("pounce", 1.4).pose(0.0, STAND).pose(0.35, CROUCH).pose(0.65, CROUCH).pose(0.85, POUNCE_AIR)
     .pose(1.05, CROUCH).pose(1.4, STAND)
     .wave(lambda t: {"hips": (0, 6 * sin01(t, 0.15), 0), "tail6": (0, 20 * sin01(t, 0.15), 0)}
           if 0.35 < t < 0.65 else {})
     .root(0.0).root(0.65).root(0.85, up=0.45).root(1.05).root(1.4)
     .event(0.75, "squeak").event(0.8, "flap").event(1.05, "land"))
    PLAY_BOW = merge(BOW, {"jaw": (-14, 0, 0), "ear*": (-8, 14, 0)})
    (clip("play_bow", 1.2).pose(0.0, STAND).pose(0.3, PLAY_BOW).pose(0.9, PLAY_BOW).pose(1.2, STAND)
     .wave(lambda t: {f"tail{k}": (0, 20 * sin01(t, 0.25, -0.1 * k), 0) for k in range(2, 7)}
           if 0.25 < t < 0.95 else {})
     .event(0.35, "call"))

    def spar_wave(t):
        s, c = sin01(t, 0.5), sin01(t, 0.5, 0.25)
        return {"wing_arm_L": (26 * s, 0, 0), "wing_arm_R": (-26 * s, 0, 0),
                "wing_fore_L": (18 * max(0.0, s), 0, 0), "wing_fore_R": (18 * max(0.0, -s), 0, 0),
                "head": (6 * c, 8 * s, 0), "neck2": (0, 6 * s, 0), "jaw": (-14 - 6 * c, 0, 0),
                **{f"tail{k}": (0, 14 * sin01(t, 0.5, -0.1 * k), 0) for k in range(1, 7)}}

    (clip("spar", 1.0, loop=True).pose(0.0, merge(REAR, ears(16, 0, 0))).wave(spar_wave)
     .root(0.0, up=0.0).root(0.25, up=0.05).root(0.5, up=0.0).root(0.75, up=0.05)
     .event(0.1, "squeak").event(0.6, "thump"))
    sk = 0.8 if baby else 1.3
    stalk = clip("stalk", sk, loop=True, speed=0.25).pose(0.0, merge(T["CROUCH_BODY"], {"neck1": (-8, 0, 0),
                                                                                       "neck2": (-6, 0, 0),
                                                                                       "head": (6, 0, 0),
                                                                                       "ear*": (-10, -14, 0)}))
    stride(stalk, sk, WALK_PHASES_WINGS, STRIDE if baby else ("stalk_lift", "stalk_plant", "crouch", "stalk_push"))
    stalk.wave(hind_legs(sk, 10, 24, WALK_PHASES_LEGS, bob=0.5))
    stalk.wave(lambda t: {"tail6": (0, 22 * sin01(t, 0.3), 0), "tail5": (0, 8 * sin01(t, 0.3, -0.1), 0)})
    TAIL_CHASE = merge(FLARE, {"neck1": (0, 30, 0), "neck2": (0, 28, 0), "neck3": (0, 20, 0), "head": (-6, 18, 10),
                               "chest": (0, 12, 0), "hips": (0, 10, 0), "jaw": (-16, 0, 0),
                               **{f"tail{k}": (0, 24 + 2 * k, 0) for k in range(1, 7)}})
    tail_chase = clip("tail_chase", 0.6, loop=True).pose(0.0, TAIL_CHASE)
    tail_chase.wave(hind_legs(0.6, 18, 34, TROT_PHASES_LEGS, bob=2.0)).wave(pant(6.0, 0.3))
    tail_chase.root(0.0, up=0.0).root(0.15, up=0.08).root(0.3, up=0.0).root(0.45, up=0.08)
    footsteps(tail_chase, 0.6, {"a": 0.0, "b": 0.5})
    (clip("tail_wag", 1.0, loop=True)
     .pose(0.0, merge(STAND, {"neck1": (4, 0, 0), "head": (6, 0, 0), "jaw": (-8, 0, 0), "ear*": (-4, 10, 0)}))
     .wave(tail_wave(16, 0.5, 14))
     .wave(lambda t: {"hips": (0, 4 * sin01(t, 0.5), 2 * sin01(t, 0.5)), "belly": (0, -6 * sin01(t, 0.5), 0),
                      "head": (0, 1.5 * sin01(t, 0.5), 7 * sin01(t, 1.0, 0.25)),
                      "leg_up_L": (0, -4 * sin01(t, 0.5), 0), "leg_up_R": (0, -4 * sin01(t, 0.5), 0),
                      "ear_R": (0, 8 * sin01(t, 1.0), 0), "ear_L": (0, -8 * sin01(t, 1.0, 0.5), 0)}))
    (clip("wing_flutter", 1.4).pose(0.0, STAND).pose(0.3, OPEN).pose(0.5, merge(OPEN, T["CROUCH_BODY"]))
     .pose(0.7, OPEN).pose(1.0, FLARE).pose(1.4, STAND).event(0.3, "flap").event(0.7, "flap"))

    # ---------------------------------------------------------------- feelings
    (clip("sulk", 1.5).pose(0.0, STAND).pose(0.7, merge(CROUCH, ears(20, 10, -20))).pose(1.5, SULK)
     .event(0.5, "whimper").event(1.2, "thump"))
    (clip("sulk_loop", 5.0, loop=True).pose(0.0, SULK)
     .wave(lambda t: {"chest": (3 * max(0.0, sin01(t, 5.0)), 0, 0), "head": (-2 * max(0.0, sin01(t, 5.0)), 0, 0),
                      "ear_R": (0, 0, -4 * max(0.0, sin01(t, 5.0, 0.3)))}))
    (clip("nuzzle", 1.4, loop=True)
     .pose(0.0, merge(STAND, {"neck1": (8, 0, 0), "neck2": (4, 0, 0), "head": (-6, 0, 0), "ear*": (24, 8, -6)}))
     .wave(lambda t: {"head": (0, 10 * sin01(t, 1.4), 14 * sin01(t, 1.4)), "neck3": (0, 6 * sin01(t, 1.4, 0.1), 0)})
     .wave(tail_sway(1.5, 0.7)).event(0.3, "purr"))
    (clip("greet", 1.6).pose(0.0, STAND).pose(0.2, CROUCH)
     .pose(0.45, merge(OPEN, {"neck1": (12, 0, 0), "head": (12, 0, 0), "jaw": (-22, 0, 0), "ear*": (-8, 10, 0)}))
     .pose(0.75, CROUCH).pose(1.05, merge(FLARE, {"neck1": (8, 0, 0), "head": (-4, 10, 12), "jaw": (-12, 0, 0)}))
     .pose(1.6, STAND)
     .wave(lambda t: {f"tail{k}": (0, 12 * sin01(t, 0.4, -0.08 * k), 0) for k in range(1, 7)})
     .root(0.0).root(0.2).root(0.45, up=0.45).root(0.75).root(1.6).event(0.4, "call").event(0.45, "flap")
     .event(0.75, "land"))

    # ---------------------------------------------------------------- hands-on care
    CARRY_HEAD = {"neck1": (12, 0, 0), "head": (-8, 0, 0)}
    PICK = merge(STAND, T["PICK"])
    (clip("pick_up", 0.8).pose(0.0, STAND).pose(0.25, merge(PICK, {"jaw": (-22, 0, 0)}))
     .pose(0.4, merge(PICK, {"jaw": (-3, 0, 0)})).pose(0.8, merge(STAND, CARRY_HEAD)))
    SIT_UP = merge(SIT, {"neck1": (8, 0, 0), "head": (12, 0, 0), "ear*": (-6, -8, 0)})
    DROP = merge(STAND, {"neck1": (-16, 0, 0), "head": (-16, 0, 0)})
    (clip("drop_wait", 1.3).pose(0.0, merge(STAND, CARRY_HEAD))
     .pose(0.35, merge(DROP, {"jaw": (-4, 0, 0)})).pose(0.5, merge(DROP, {"jaw": (-22, 0, 0)}))
     .pose(0.9, merge(SIT_UP, {"jaw": (-8, 0, 0)})).pose(1.3, merge(SIT_UP, {"jaw": (-6, 0, 0)}))
     .wave(lambda t: {f"tail{k}": (0, (6 + 3 * k) * sin01(t, 0.4, -0.08 * k) * min(1.0, max(0.0, (t - 0.7) * 3)), 0)
                      for k in range(1, 7)}))
    (clip("leap_catch", 1.0).pose(0.0, STAND).pose(0.15, CROUCH)
     .pose(0.34, merge(OPEN, {"neck1": (16, 0, 0), "head": (14, 0, 0), "jaw": (-24, 0, 0), "ear*": (-6, 0, 0)}))
     .pose(0.48, merge(OPEN, {"neck1": (10, 0, 0), "head": (6, 0, 0), "jaw": (-3, 0, 0)}))
     .pose(0.74, CROUCH).pose(1.0, merge(STAND, CARRY_HEAD))
     .root(0.0).root(0.15).root(0.42, up=0.6).root(0.74).root(1.0).event(0.36, "flap").event(0.74, "land"))
    KICK = merge(SIT, {"neck1": (4, 0, 0), "head": (6, 8, 14), "leg_up_R": (30 if baby else 84, 0, 0),
                       "leg_lo_R": (-30 if baby else -70, 0, 0), "ear*": (28, 10, -10)})
    (clip("leg_kick", 1.4).pose(0.0, STAND).pose(0.28, KICK).pose(1.15, KICK).pose(1.4, STAND)
     .wave(lambda t: {"leg_up_R": (22 * sin01(t, 0.18) if 0.28 < t < 1.15 else 0.0, 0, 0),
                      **{f"tail{k}": (0, (6 + 4 * k) * sin01(t, 0.35, -0.08 * k), 0) for k in range(1, 7)}})
     .event(0.32, "thump").event(0.5, "thump").event(0.68, "thump").event(0.86, "thump"))
    (clip("sniff_refuse", 1.3).pose(0.0, STAND)
     .pose(0.35, merge(STAND, {"neck1": (-10, 0, 0), "head": (-14, 0, 0), "ear*": (-6, -10, 0)}))
     .pose(0.75, merge(STAND, {"neck2": (4, 22, 0), "neck3": (2, 18, 0), "head": (12, 26, -8), "ear*": (24, 12, -14)}))
     .pose(1.3, STAND).event(0.35, "sniff").event(0.8, "whimper"))
    # Grooming under a wing: it holds the right wing out and up, leaning on the left.
    LIFT = merge(wing_pose("stand", "L"), T["OPEN_R"], {"chest": (0, 0, -6), "neck1": (0, 10, 0), "head": (0, 10, 8)})
    clip("lift_wing", 1.5).pose(0.0, STAND).pose(0.4, LIFT).pose(1.1, LIFT).pose(1.5, STAND).event(0.4, "flap")
    (clip("sneeze", 0.8).pose(0.0, STAND)
     .pose(0.28, merge(STAND, {"neck1": (6, 0, 0), "head": (14, 0, 0), "jaw": (-8, 0, 0), "ear*": (-10, 0, 0)}))
     .pose(0.42, merge(STAND, {"neck1": (-6, 0, 0), "head": (-16, 0, 0), "jaw": (-4, 0, 0), "ear*": (26, 0, 0)}))
     .pose(0.8, STAND).event(0.4, "sneeze"))
    (clip("pull_away", 0.9).pose(0.0, STAND)
     .pose(0.3, merge(STAND, {"neck1": (14, 0, 0), "neck2": (10, 0, 0), "head": (10, 0, 0), "chest": (6, 0, 0),
                              "hips": (-4, 0, 0), "ear*": (40, 10, -20)}))
     .pose(0.9, STAND)
     .wave(lambda t: {"head": (0, 16 * sin01(t, 0.16) * max(0.0, 1 - abs(t - 0.45) / 0.25), 0)})
     .event(0.3, "whimper"))

    # ---------------------------------------------------------------- toys
    # A swat at the dangled feather: from the play bow, the left wrist comes up and forward.
    SWAT = merge(PLAY_BOW, {"chest": (-4, 0, 6), "head": (14, 8, -12)})
    SWAT.update(T["REAR_L"])  # the left wing alone takes the rear pose (after the both-sides key)
    (clip("paw_bat", 0.8).pose(0.0, PLAY_BOW).pose(0.24, SWAT).pose(0.42, SWAT).pose(0.8, PLAY_BOW)
     .wave(lambda t: {f"tail{k}": (0, 12 * sin01(t, 0.4, -0.08 * k), 0) for k in range(1, 7)})
     .event(0.24, "squeak"))
    (clip("tug", 0.9, loop=True).pose(0.0, merge(STAND, T["TUG"]))
     .wave(lambda t: {"neck1": (0, 14 * sin01(t, 0.45), 0), "neck2": (0, 8 * sin01(t, 0.45, 0.1), 0),
                      "head": (0, 6 * sin01(t, 0.45, 0.2), 10 * sin01(t, 0.45, 0.15)),
                      "hips": (0, 0, 3 * sin01(t, 0.9)),
                      **{f"tail{k}": (0, 16 * sin01(t, 0.45, -0.08 * k), 0) for k in range(1, 7)}}))

    if not baby:
        # ------------------------------------------------------------ flight
        # Quiet and deep: slow, big wingbeats; a long glide with the wingtips barely stirring;
        # the dive with the wings swept back. Ears laid back, legs tucked, the tail streaming.
        fly_flap = clip("fly_flap", 0.9, loop=True).pose(0.0, P("fly"))
        fly_flap.wave(wingbeat(0.9, 46)).wave(tail_sway(0.5, 1.8)).event(0.12, "flap")
        (clip("fly_glide", 3.0, loop=True).pose(0.0, P("glide")).wave(wingbeat(3.0, 4, fold=0.2))
         .wave(tail_sway(0.6, 3.0))
         .wave(lambda t: {"ear_R": (0, 3 * sin01(t, 1.5), 0), "ear_L": (0, -3 * sin01(t, 1.5), 0)}))
        (clip("fly_dive", 1.0, loop=True).pose(0.0, P("dive", ears(44, 6, -10))).wave(wingbeat(0.25, 2, fold=0.1))
         .wave(tail_sway(0.3, 0.5)))
    else:
        # the baby's run: a scamper, bouncing along on its hind legs with its wing-arms out
        scamper = clip("scamper", 0.4, loop=True, speed=2.4).pose(0.0, merge(wing_pose("open", baby=True),
                                                                             ears(18, 0, 0)))
        scamper.wave(hind_legs(0.4, 36, 50, {"L": 0.0, "R": 0.5}, bob=4.0)).wave(pant(8.0, 0.2))
        scamper.wave(tail_sway(1.2, 0.4)).wave(lambda t: {"wing_arm_R": (0, 0, 10 * sin01(t, 0.2)),
                                                          "wing_arm_L": (0, 0, -10 * sin01(t, 0.2)),
                                                          "hips": (0, 0, 5 * sin01(t, 0.4))})
        scamper.root(0.0, up=0.0).root(0.1, up=0.08).root(0.2, up=0.0).root(0.3, up=0.08)
        footsteps(scamper, 0.4, {"a": 0.0, "b": 0.5})
    return out


def clips():
    from dragons import clipkit
    grown, baby = _tables()
    out = _clip_set(grown, False)
    for c in _clip_set(baby, True):
        if c.name in BABY_CLIPS or c.name == "scamper":
            c.name = f"{c.name}_h"
            out.append(c)
    clipkit.check(out, BONE_ORDER, NAME)
    return out




# ------------------------------------------------------------------------------ the solver
def _solve_main():
    """Blender: solve WING_TARGETS on the Duskwing's grown form, rewrite SOLVED, render."""
    import bpy
    from mathutils import Quaternion, Vector
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.abspath(os.path.join(here, "..", "..", ".."))
    for p in (os.path.join(root, "tools"), os.path.join(root, "tools", "blender"), os.path.join(root, "tools", "anim")):
        if p not in sys.path:
            sys.path.insert(0, p)
    from dragonkit import model as km
    from eca import q_from_pyr

    argv = km.argv
    km.use_kind(km.arg("--kind", "duskwing"))
    stage = km.arg("--stage", "adult")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene, cam = km.setup_scene(int(km.arg("--res", "420")), (0.40, 0.34, 0.42))
    form, t = km.STAGE[stage]
    d = km.build_dragon(form, int(km.arg("--variant", "0")))
    if "--texture" in argv:
        km.textured(d)
    km.pose_stage(d, t)
    arm = d["arm"]
    pb = arm.pose.bones
    idle = {b.name: b.rotation_euler.to_quaternion() for b in pb}
    rest = {b.name: b.matrix_local.to_quaternion() for b in arm.data.bones}
    for b in pb:
        b.rotation_mode = "QUATERNION"

    def pyr(q):
        e = q.to_matrix().to_euler("YXZ")
        return (round(-math.degrees(e.x), 1), round(math.degrees(e.z), 1), round(-math.degrees(e.y), 1))

    def expand(keys):
        out = {}
        for b, v in keys.items():
            if b.endswith("*"):
                out[b[:-1] + "_R"] = v
                out[b[:-1] + "_L"] = (v[0], -v[1], -v[2])
            else:
                out[b] = v
        return out

    def set_keys(keys):
        for b in pb:
            q = Quaternion(q_from_pyr(*keys.get(b.name, (0.0, 0.0, 0.0))))
            b.rotation_quaternion = idle[b.name] @ (rest[b.name].conjugated() @ q @ rest[b.name])
        bpy.context.view_layer.update()

    def floor():
        dg = bpy.context.evaluated_depsgraph_get()
        ev = d["body"].evaluated_get(dg)
        me = ev.to_mesh()
        low = min((ev.matrix_world @ v.co).z for v in me.vertices)
        ev.to_mesh_clear()
        return low

    def head(name):
        return arm.matrix_world @ pb[name].head

    def tail(name):
        return arm.matrix_world @ pb[name].tail

    w = km.wing_points("R")
    n_rest = (w["f1"] - w["root"]).cross(w["body"] - w["root"]).normalized()
    if n_rest.z < 0:
        n_rest = -n_rest

    def aim(name, want, n_fold, keys):
        """Turn one bone to point along `want` with the membrane facing n_fold; keep its key."""
        bpy.context.view_layer.update()
        world = (arm.matrix_world @ pb[name].matrix).to_quaternion()
        frame = world @ rest[name].conjugated()
        current = (world @ Vector((0, 1, 0))).normalized()
        want = Vector(want).normalized()
        turn = current.rotation_difference(want)
        n_now = (turn @ world) @ (rest[name].conjugated() @ n_rest)
        a = (n_now - want * n_now.dot(want)).normalized()
        nf = Vector(n_fold).normalized()
        b = (nf - want * nf.dot(want)).normalized()
        turn = Quaternion(want, math.atan2(want.dot(a.cross(b)), a.dot(b))) @ turn
        q = frame.conjugated() @ turn @ frame
        pb[name].rotation_quaternion = idle[name] @ (rest[name].conjugated() @ q @ rest[name])
        bpy.context.view_layer.update()
        keys[name[:-2]] = pyr(q)

    def solve(pose):
        spec = pose["wing"]
        set_keys(expand(pose.get("body", {})))
        z0 = floor()
        keys = {}
        normals = spec.get("normals", {})
        nf = spec.get("normal", FOLD_NORMAL)
        if "rest" in spec:  # the spread wing, turned about the shoulder
            turn = Quaternion(q_from_pyr(*spec["rest"]))
            for name, *_ in WING_CHAIN:
                bone = arm.data.bones[f"{name}_R"]
                want = turn @ (bone.matrix_local.to_quaternion() @ Vector((0, 1, 0)))
                aim(f"{name}_R", want, turn @ n_rest, keys)
            return keys
        if "wrist" in spec:
            r = head("wing_arm_R")
            la = (tail("wing_arm_R") - head("wing_arm_R")).length
            lb = (tail("wing_fore_R") - head("wing_fore_R")).length
            if isinstance(spec["wrist"][0], str):  # (bone, offset): from that bone's joint
                target = head(spec["wrist"][0]) + Vector(spec["wrist"][1])
            else:
                x, y, z = spec["wrist"]
                target = Vector((x, y, z0 + z))
            dvec = target - r
            dist = max(abs(la - lb) + 1e-3, min(la + lb - 1e-3, dvec.length))
            u = dvec.normalized()
            pole = Vector(spec.get("pole", (0.3, 0.6, 0.7)))
            p = (pole - u * pole.dot(u)).normalized()
            ca = (la * la + dist * dist - lb * lb) / (2 * la * dist)
            elbow = r + (u * ca + p * math.sqrt(max(0.0, 1 - ca * ca))) * la
            arm_dir, fore_dir = elbow - r, (r + u * dist) - elbow
        else:
            arm_dir, fore_dir = Vector(spec["arm"]), Vector(spec["fore"])
        aim("wing_arm_R", arm_dir, normals.get("arm", nf), keys)
        aim("wing_fore_R", fore_dir, normals.get("fore", nf), keys)
        fore_now = (tail("wing_fore_R") - head("wing_fore_R")).normalized()
        for bone in ("thumb", "f1", "f2", "f3", "f4"):
            want = spec.get(bone)
            if want is None:
                continue
            if want[0] == "fold":
                want = -fore_now + Vector(want[1])
            aim(f"wing_{bone}_R", want, normals.get(bone, nf), keys)
        return keys

    baby = km.arg("--table", "baby" if form == "hatchling" else "grown") == "baby"
    table, known = (H_POSES, SOLVED_H) if baby else (POSES, SOLVED)
    solved = {}
    only = km.arg("--poses")
    for name, pose in table.items():
        if only and name not in only.split(","):
            solved[name] = known.get(name, {})
            continue
        keys = solve(pose)
        solved[name] = keys
        full = dict(expand(pose.get("body", {})))
        full.update(expand({f"{b}*": v for b, v in keys.items()}))
        set_keys(full)
        z0 = floor()
        rel = lambda p: tuple(round(c, 2) for c in (p.x, p.y, p.z - z0))  # noqa: E731
        print(f"[solve] {name}: wrist R {rel(tail('wing_fore_R'))} L {rel(tail('wing_fore_L'))} "
              f"thumb tip {rel(tail('wing_thumb_R'))} elbow {rel(tail('wing_arm_R'))} "
              f"f1 tip {rel(tail('wing_f1_R'))} f4 tip {rel(tail('wing_f4_R'))}")
        print(f"[solve]   body: hips {rel(head('hips'))} chest {rel(head('chest'))} shoulder "
              f"{rel(head('wing_arm_R'))} neck1 {rel(head('neck1'))} knee {rel(head('leg_lo_R'))} "
              f"toe {rel(tail('foot_R'))}")
    if "--write" in argv or "--solve" in argv:
        path = os.path.abspath(__file__)
        text = open(path, encoding="utf-8").read()
        start, end = ("SOLVED_H = {", "# --- end of solved baby poses ---") if baby else \
            ("SOLVED = {", "# --- end of solved poses ---")
        a = text.index(start)
        b = text.index(end)
        lines = [start]
        for name, keys in solved.items():
            items = [f'"{k}": ({v[0]}, {v[1]}, {v[2]})' for k, v in keys.items()]
            line = f'    "{name}": {{'
            for i, item in enumerate(items):
                piece = item + ("}," if i == len(items) - 1 else ", ")
                if len(line) + len(piece.rstrip()) > 118:
                    lines.append(line.rstrip())
                    line = " " * 8
                line += piece
            lines.append(line)
        lines.append("}")
        text = text[:a] + "\n".join(lines) + "\n" + text[b:]
        open(path, "w", encoding="utf-8", newline="\n").write(text)
        print(f"[solve] wrote {len(solved)} poses into {path}")
    shots = km.arg("--render")
    if shots:
        out = km.arg("--out", os.path.join(root, "build", "kit", "duskwing", "pose"))
        views = km.arg("--views", "three_quarter,side,front").split(",")
        files = []
        for name in (table if shots == "all" else shots.split(",")):
            full = dict(expand(table[name].get("body", {})))
            full.update(expand({f"{b}*": v for b, v in solved.get(name, known.get(name, {})).items()}))
            set_keys(full)
            arm.location = (0, 0, 0)
            bpy.context.view_layer.update()
            arm.location.z = -floor()
            bpy.context.view_layer.update()
            for view in views:
                km.frame_camera(cam, [d], view, margin=1.3)
                files.append(f"{out}_{stage}_{name}_{view}.png")
                km.render(files[-1])
        import subprocess
        subprocess.run([bpy.app.binary_path, "-b", "-P", os.path.join(root, "tools", "blender", "sheet.py"), "--",
                        "--out", f"{out}_{stage}_sheet.png", "--cols", str(len(views) * (2 if len(views) < 3 else 1)),
                        *files], check=True, capture_output=True)


if __name__ == "__main__":
    _solve_main()
