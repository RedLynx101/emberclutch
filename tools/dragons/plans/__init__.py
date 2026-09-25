"""Body plans: one module per plan (tools/dragons/plans/<plan>.py). The contract, which
tools/blender/dragonkit, tools/anim/build_anims.py and the game all rely on:

  NAME          the plan's name (= the module name; the game loads romfs:/anims/<NAME>.eca)
  BONES         body bones, parents first: (name, head node, tail node, parent or None).
                Node names are keys of the kind's form "nodes" (plus the extra points
                "jaw_hinge", "jaw_tip", "eyes_c", "eyes_tip" that the kit adds).
  WING_CHAIN    wing bones per side: (name, head point, tail point, parent). Points are keys
                of the form's wing layout; a parent that isn't another wing bone is a body
                bone (usually "chest"). Every name starts with "wing" (the game's wing flag).
  WING_BONES    [f"{name}_{side}" ...] for sides L, R
  BONE_ORDER    BONES then WING_BONES: the exported skeleton order (at most 40 bones)
  WING_BODY     body bones the wing draw may also use (the membrane's flank edge)
  CONTACTS      (front left, front right, back left, back right): the bones that touch the
                ground when it stands (footsteps, the floor contact, riding)
  SEAT          (bone, (x, y, z) offset in the bone's rest frame): where a rider sits
  REGION        {bone name prefix: dirt region} for body regions the kit can't guess
  clips()       the full clip list for this body (tools/dragons/clip_names.REQUIRED at least)

Names the game looks up by name: "head", "snout", "jaw", "eyes", "chest", "neck2", "neck3"
(look-at), bones starting "tail" (the tail), "wing" (wings), "arm"/"leg"/"hand"/"foot"
(limbs). Every plan has head, snout, jaw, eyes and chest; neck2/neck3 if it has a neck.
The body draw is every non-wing bone except "eyes": at most 25 (the shader's budget).
"""
