"""Kinds of dragon: one module per kind (tools/dragons/kinds/<kind>.py). Pure Python at import
(no bpy): the Blender kit calls the hooks with itself as `kit`.

META (who it is; the game's tables are generated from it)
  name        module name (lower case, the folder romfs/dragons/<name>/)
  title       its name in the game ("Pouncer")
  dex         Dragondex order (1..8 bases, then crossbreeds)
  element     one-word type(s): "Ember" or ("Ember", "Gale") for a crossbreed
  parents     crossbreeds: the two base kinds, e.g. ("pouncer", "crestwing"); bases: ()
  rarity      "common", "uncommon" or "rare" (bases); crossbreeds "uncommon" or "rare"
  plan        its body plan (tools/dragons/plans/<plan>.py)
  size        grown size relative to the Pouncer (0.67 .. 1.5: +-50% across the breeds)
  stats       base stats 1..10: dict(wing, wit, might, breath, stamina)
  manners     manners it's prone to (most likely first)
  traits      traits it's prone to (most likely first)
  rare_variant  the index of its rare variant in VARIANTS (3)
  rare_replaces True: a rare part group replaces the common one; False: adds to it
  blurb       one line for the Dragondex

VARIANTS: four colourings, [0..2] common, [3] rare. Each: dict(name, base, accent, pattern,
  horn, membrane, iris, glow, [pupil, glint, tongue]) as linear RGB 0..1, plus
  pattern_channel ("r", "g", "b" or None: which texture mask shows in the pattern colour) and
  glow_channel (None, or a channel that glows in the glow colour: the rare variant's marks).

FORMS: {"hatchling": {...}, "grown": {...}} in the kit's schema (dragonkit/model.py:
  FORM_DEFAULTS and the classic GROWN/HATCH dicts in tools/blender/dragon_model.py are the
  worked example): nodes, edges or meta, body ("skin" | "meta"), body_tris(_lod1),
  export_scale, young{bones, parts}, young_pose, base_pose, builds, eyes, head, heart, wing,
  mask, inset, face, jaw_hinge, mouth_detail, skin (texture sizes), [ridge], [sculpt].

Hooks (all optional; kit = tools/blender/dragonkit/model.py, tx = dragonkit/texture.py):
  parts(kit, d)        -> [(group, variant, [(object, bone)])]; group one of eyes, horns,
                          frill, spikes, tail_tip, heart, mouth, runes; variant 0 = everyone,
                          1 = the rare variant only. Eyes, heart and mouth are built by the kit.
  wings(kit, d, rare)  -> [objects] named "wingarm_<L|R>..." (bony arm) or "<name>_<L|R>..."
                          (membranes, feathers: strut weights). rare=True: the rare variant's
                          wings, or [] to keep the common ones. Default: kit.build_wings(style).
  accent(kit, body)    paint the body's "mask" attribute (G = accent: belly, muzzle, socks)
  texture(tx, nt, p, form) -> {"r", "g", "b"[, "value"]} node sockets (see texture.py)
  sculpt (in a form)   callable(kit, body): reshape the body mesh before decimation

EGG: dict(height, width, point, speckle, colors (base, accent) per variant): the kind's egg.
"""
