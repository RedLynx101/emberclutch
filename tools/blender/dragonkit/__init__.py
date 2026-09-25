"""The dragon kit (D77-D78): builds any kind of dragon from its data in tools/dragons/kinds/
and its body plan in tools/dragons/plans/, in Blender, headless.

  model.py    the builder: two body forms, rig, parts, wings, mouth, eyes, growth, previews
  texture.py  the painted skin texture (pattern masks + painted shading), regions, UVs
  export.py   the game's files: romfs/dragons/<kind>/{hatchling,grown}[_lod1].ecm and skins
  review.py   the review renders and sheets for a kind (stages, variants, turntable, clips)
  check.py    budgets and coverage (plain Python, reads the exported files)

The classic dragon's own scripts (tools/blender/dragon_model.py, export_dragon.py) stay as
they are until the revamp replaces the old dragons in the game (DR3).
Guide: docs/tech/dragon-kit.md.
"""
