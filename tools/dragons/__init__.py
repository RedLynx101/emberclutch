"""Dragons, version 2 (D77-D78): the kinds of dragon and their body plans, as data shared by
the Blender kit (tools/blender/dragonkit), the animation builder (tools/anim) and the game's
generated tables. Pure Python: nothing here imports bpy.

  tools/dragons/plans/<plan>.py   a body plan: its skeleton and its full set of clips
  tools/dragons/kinds/<kind>.py   a kind: who it is (element, rarity, size, stats), its two
                                  body forms, parts, wings, texture, egg and colour variants
  tools/dragons/clip_names.py     the clips every plan must have (the game's ClipId list)

See docs/tech/dragon-kit.md for the whole pipeline and the contract a kind fulfils.
"""
import importlib
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
for _p in (os.path.join(ROOT, "tools"), os.path.join(ROOT, "tools", "anim")):
    if _p not in sys.path:
        sys.path.insert(0, _p)


def plan(name):
    """The body plan module tools/dragons/plans/<name>.py."""
    return importlib.import_module(f"dragons.plans.{name}")


def kind(name):
    """The kind module tools/dragons/kinds/<name>.py."""
    return importlib.import_module(f"dragons.kinds.{name}")


def all_kinds():
    """Every kind with a module, in the Dragondex order of their META["dex"]."""
    names = [f[:-3] for f in os.listdir(os.path.join(HERE, "kinds")) if f.endswith(".py") and not f.startswith("_")]
    mods = [kind(n) for n in names]
    return sorted(mods, key=lambda m: m.META["dex"])
