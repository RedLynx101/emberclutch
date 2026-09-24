"""The dragon skeleton layout shared by the model (dragon_model.py) and the animation tools
(tools/anim). Pure Python: no bpy, so the animation exporter runs without Blender.

Bones: (name, head node, tail node, parent). 26 body bones come first, then 12 wing bones
(the wing draw). The body mesh uses 25 of them (one 25-bone draw); the 26th, "eyes", only
carries the eye parts. Both body forms (hatchling, grown) use this list with their own node
positions. The jaw and eyes are not body nodes (the skin modifier would turn them into
flesh), see dragon_model.extra_points:
  * the jaw hinges at the form's "jaw_hinge" point, parallel to the snout and parented to
    it, and grows exactly like the snout, so the closed lips meet at every stage and build;
  * the eyes bone sits between the eyes, parallel to the head and growing like it, so it
    moves the eyes exactly as the head did; the runtime squashes it vertically to blink.
"""

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
BONES.append(("jaw", "jaw_hinge", "jaw_tip", "snout"))  # the lower jaw: opens the mouth
BONES.append(("eyes", "eyes_c", "eyes_tip", "head"))    # both eyes: squashed to blink

# Wing bones (second draw): arm, forearm and four fingers per side.
WING_CHAIN = [("wing_arm", "root", "elbow", "chest"), ("wing_fore", "elbow", "wrist", "wing_arm"),
              ("wing_f1", "wrist", "f1", "wing_fore"), ("wing_f2", "wrist", "f2", "wing_fore"),
              ("wing_f3", "wrist", "f3", "wing_fore"), ("wing_f4", "wrist", "f4", "wing_fore")]
WING_BONES = [f"{n}_{side}" for side in ("L", "R") for n, *_ in WING_CHAIN]

# Every bone in export order (the .ecm skeleton order).
BONE_ORDER = [b[0] for b in BONES] + WING_BONES
