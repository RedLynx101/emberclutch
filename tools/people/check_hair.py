"""Bald patches (1.0.1: the backs of Wren's and Fig's heads): everywhere round a head where the outermost surface
is skin but hair lies under it, so the scalp shows where hair was meant to be.

  python tools/people/check_hair.py [--only steward,fig] [--map]

Rays go out from the head's centre every 5 degrees; a ray whose farthest hit is skin with a hair triangle
nearer in is a bald spot (the ears aside: they stand through the hair on purpose). A flat hair face spanning a
wide band of the head's curve sags inside it: more rows, more columns or a thicker shell clears it.
--map prints each head as a chart (az across, el down; # bald, . hair on top, blank: no hair there).
Exit code 1 if anyone has a bald patch.
"""
import math
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "anim"))
import people  # noqa: E402

AZ = np.arange(-180.0, 180.0, 5.0)
EL = np.arange(85.0, -45.0, -5.0)


def hits(c, dirs, pos, tris):
    """For each ray (from c along dirs) and triangle: the distance to the hit, or inf (Moller-Trumbore)."""
    v0, v1, v2 = pos[tris[:, 0]], pos[tris[:, 1]], pos[tris[:, 2]]
    e1, e2 = v1 - v0, v2 - v0                      # (T, 3)
    d = dirs[:, None, :]                           # (R, 1, 3)
    pv = np.cross(d, e2[None])                     # (R, T, 3)
    det = (e1[None] * pv).sum(-1)
    ok = np.abs(det) > 1e-12
    inv = np.where(ok, 1.0 / np.where(ok, det, 1.0), 0.0)
    tv = (c - v0)[None]                            # (1, T, 3)
    u = (tv * pv).sum(-1) * inv
    qv = np.cross(np.broadcast_to(tv, pv.shape), e1[None])
    v = (d * qv).sum(-1) * inv
    t = (e2[None] * qv).sum(-1) * inv
    good = ok & (u >= -1e-6) & (v >= -1e-6) & (u + v <= 1 + 1e-6) & (t > 1e-4)
    return np.where(good, t, np.inf)


def bald(p, mesh):
    c = np.array(p.head.c)
    pos = np.array(mesh.pos)
    tris = np.array(mesh.tris)
    mats = np.array(mesh.mat)[tris[:, 0]]
    near = np.linalg.norm(pos[tris].mean(axis=1) - c, axis=1) < 0.6  # (the head and what's on it)
    tris, mats = tris[near], mats[near]
    az, el = np.meshgrid(np.radians(AZ), np.radians(EL))
    dirs = np.stack([np.sin(az) * np.cos(el), -np.cos(az) * np.cos(el), np.sin(el)], axis=-1).reshape(-1, 3)
    t = hits(c, dirs, pos, tris)
    far = t.copy()
    far[~np.isfinite(far)] = -1.0
    outer = far.argmax(axis=1)
    outer_t = far.max(axis=1)
    hair = np.where(mats[None, :] == "hair", t, np.inf).min(axis=1)  # the nearest hair along the ray
    spot = (outer_t > 0) & (mats[outer] == "skin") & np.isfinite(hair) & (hair < outer_t - 0.0005)
    azd, eld = np.degrees(az).reshape(-1), np.degrees(el).reshape(-1)
    spot &= ~((np.abs(np.abs(azd) - 91.0) < 24.0) & (np.abs(eld + 9.0) < 24.0))  # the ears
    depth = np.where(spot, outer_t - hair, 0.0)
    has_hair = np.isfinite(hair)
    return spot.reshape(len(EL), len(AZ)), depth.reshape(len(EL), len(AZ)), has_hair.reshape(len(EL), len(AZ))


def main():
    only = sys.argv[sys.argv.index("--only") + 1].split(",") if "--only" in sys.argv else None
    ids = only or list(people.VILLAGER_IDS) + list(people.STORY_IDS) + list(people.PLAYERS)
    bad = 0
    for pid in ids:
        p = people.build(pid)
        if p.hair:  # a player: each style over the body
            cases = []
            for k, h in enumerate(p.hair):
                both = type(p.body)()
                both.extend(p.body)
                both.extend(h)
                cases.append((f"{pid} hair {k}", both))
        else:
            cases = [(pid, p.body)]
        for name, mesh in cases:
            spot, depth, has = bald(p, mesh)
            n = int(spot.sum())
            if n:
                bad += 1
                j, i = np.unravel_index(depth.argmax(), depth.shape)
                print(f"{name:<18} BALD: {n} of {int(has.sum())} hair directions; deepest {depth.max() * 1000:.0f} mm at "
                      f"az {AZ[i]:.0f} el {EL[j]:.0f}")
            else:
                print(f"{name:<18} ok ({int(has.sum())} hair directions)")
            if "--map" in sys.argv:
                for j in range(len(EL)):
                    print("   " + "".join("#" if spot[j, i] else ("." if has[j, i] else " ") for i in range(len(AZ))) + f"  el {EL[j]:.0f}")
                print("   " + "back" + " " * 14 + "side" + " " * 14 + "face" + " " * 14 + "side" + " " * 14 + "back")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
