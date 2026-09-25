"""Helpers for a body plan's clip set (pure Python).

A plan builds its clips however suits its body: from scratch with eca.Clip, or starting
from the classic dragon's 55 clips (tools/anim/clips.py) and reshaping them. The helpers:

  classic()                     fresh copies of the classic clips
  by_name(clips)                {name: clip}
  retarget(clip, plan, ...)     rename bones, spread a chain over more or fewer bones, drop
                                tracks for bones the plan doesn't have
  scaled(clip, k, bones=None)   bigger or smaller motion (keys and waves)
  timed(clip, k)                slower (k > 1) or quicker; root speed follows
  layered(clip, fn)             add a procedural layer fn(t) -> {bone: (pitch, yaw, roll)}
  replace(clips, new)           swap clips by name
  check(clips, bone_order)      every required clip present, nothing on unknown bones

Conventions are eca.py's: (pitch, yaw, roll) degree deltas in armature axes on top of the
idle pose; "name*" in a pose keys both sides.
"""
import copy
import importlib
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ANIM = os.path.join(HERE, "..", "anim")
if ANIM not in sys.path:
    sys.path.insert(0, ANIM)
from eca import Clip  # noqa: E402,F401  (re-exported for plans)

from dragons.clip_names import REQUIRED  # noqa: E402


def classic():
    """Fresh copies of the classic clips (the Alpha dragon's), safe to reshape."""
    mod = importlib.import_module("clips")
    return [copy.deepcopy(c) for c in mod.CLIPS]


def by_name(clips):
    return {c.name: c for c in clips}


def _map_pose(pose, fn):
    """Apply fn(bone, value) -> [(bone, value), ...] to a {bone: value} dict, summing clashes."""
    out = {}
    for bone, v in pose.items():
        for nb, nv in fn(bone, v):
            a = out.get(nb, (0.0, 0.0, 0.0))
            out[nb] = tuple(x + y for x, y in zip(a, nv))
    return out


def _remap(clip, fn):
    """Rewrite every bone key and wave output of a clip through fn(bone, value)."""
    keys = clip.filled_keys()
    new = {}
    times = sorted({t for k in keys.values() for t in k})
    for t in times:
        pose = {b: k[t] for b, k in keys.items() if t in k}
        for b, v in _map_pose(pose, fn).items():
            new.setdefault(b, {})[t] = v
    clip.keys = new
    clip.pose_times, clip.pose_bones = set(), set()
    clip._filled = None
    clip.waves = [(lambda t, w=w: _map_pose(w(t), fn)) for w in clip.waves]
    return clip


def retarget(clip, bone_order, rename=None, chains=None, default=None):
    """Fit a clip to a plan's skeleton.
      rename:  {old bone: new bone or None (drop)}
      chains:  [(old names, new names)]: the motion along an old chain (e.g. tail1..tail4)
               spread over a new one (tail1..tail7), each new bone taking the old chain's
               rotation at its place along it, scaled so the whole bend stays the same.
      default: what to do with a bone the plan lacks after renaming: None drops it.
    Returns the clip (changed in place)."""
    rename = rename or {}
    have = set(bone_order)
    spread = {}
    for old, new in chains or []:
        k = len(old) / len(new)
        for i, nb in enumerate(new):
            f = (i + 0.5) / len(new) * len(old) - 0.5  # position along the old chain
            lo = max(0, min(len(old) - 1, int(f)))
            hi = min(len(old) - 1, lo + 1)
            u = min(1.0, max(0.0, f - lo))
            spread.setdefault(old[lo], []).append((nb, (1 - u) * k))
            if hi != lo and u > 0:
                spread.setdefault(old[hi], []).append((nb, u * k))

    def fn(bone, v):
        if bone in spread:
            return [(nb, tuple(x * w for x in v)) for nb, w in spread[bone]]
        nb = rename.get(bone, bone)
        if nb is None or nb not in have:
            return [] if default is None else [(default, v)]
        return [(nb, v)]

    return _remap(clip, fn)


def scaled(clip, k, bones=None):
    """Motion k times as big (on the given bones, or all)."""
    def fn(bone, v):
        return [(bone, tuple(x * k for x in v) if bones is None or bone in bones else v)]
    return _remap(clip, fn)


def timed(clip, k):
    """The clip k times as long (k > 1 slower); root speed and events follow."""
    clip.keys = {b: {t * k: v for t, v in ks.items()} for b, ks in clip.filled_keys().items()}
    clip.pose_times, clip.pose_bones = set(), set()
    clip._filled = None
    clip.waves = [(lambda t, w=w: w(t / k)) for w in clip.waves]
    clip.root_keys = {t * k: v for t, v in clip.root_keys.items()}
    clip.events = [(t * k, e) for t, e in clip.events]
    clip.length *= k
    clip.speed /= k
    return clip


def layered(clip, fn):
    clip.waves.append(fn)
    return clip


def replace(clips, new):
    """The list with clips of the same name swapped for the new ones (new names appended)."""
    new_by = by_name(new)
    out = [new_by.pop(c.name, c) for c in clips]
    return out + list(new_by.values())


def check(clips, bone_order, plan_name="plan"):
    """Raises if a required clip is missing or a clip keys a bone the plan doesn't have.
    Returns a short report string."""
    names = [c.name for c in clips]
    assert len(names) == len(set(names)), f"{plan_name}: duplicate clip names"
    missing = [n for n in REQUIRED if n not in names]
    assert not missing, f"{plan_name}: missing clips {missing}"
    have = set(bone_order)
    for c in clips:
        unknown = set(c.filled_keys()) - have
        assert not unknown, f"{plan_name}/{c.name}: unknown bones {sorted(unknown)}"
        for t in (0.0, c.length * 0.5):
            for w in c.waves:
                stray = set(w(t)) - have
                assert not stray, f"{plan_name}/{c.name}: a wave moves unknown bones {sorted(stray)}"
    babies = [n for n in names if n.endswith("_h")]
    return f"{plan_name}: {len(clips)} clips ({len(REQUIRED)} required, {len(babies)} baby versions)"
