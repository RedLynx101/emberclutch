"""Build romfs/anims/dragon.eca from the clips in tools/anim/clips.py (pure Python).

  python tools/anim/build_anims.py [--out path]
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "blender"))
import clips  # noqa: E402
from eca import q_from_pyr, write_eca  # noqa: E402
from rig_layout import BONE_ORDER  # noqa: E402


def rotate(q, v):
    w, x, y, z = q
    # v' = q v q* (expanded)
    tx, ty, tz = 2 * (y * v[2] - z * v[1]), 2 * (z * v[0] - x * v[2]), 2 * (x * v[1] - y * v[0])
    return (v[0] + w * tx + y * tz - z * ty, v[1] + w * ty + z * tx - x * tz, v[2] + w * tz + x * ty - y * tx)


def check_conventions():
    """The documented axis conventions (eca.py) hold for the quaternions we write."""
    fwd = (0.0, -1.0, 0.0)
    up = rotate(q_from_pyr(90, 0, 0), fwd)       # pitch + : a forward bone's tip goes up
    left = rotate(q_from_pyr(0, 90, 0), fwd)     # yaw + : it turns to the dragon's left (+X)
    lean = rotate(q_from_pyr(0, 0, 90), (0, 0, 1))  # roll + : the top leans right (-X)
    assert up[2] > 0.99 and left[0] > 0.99 and lean[0] < -0.99, (up, left, lean)


def main():
    check_conventions()
    out = sys.argv[sys.argv.index("--out") + 1] if "--out" in sys.argv else \
        os.path.join(ROOT, "romfs", "anims", "dragon.eca")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    names = [c.name for c in clips.CLIPS]
    assert len(names) == len(set(names)), "duplicate clip names"
    size = write_eca(out, clips.CLIPS, BONE_ORDER)
    total = sum(c.length for c in clips.CLIPS)
    print(f"[anims] {out}: {len(clips.CLIPS)} clips, {total:.1f} s, {size} bytes")
    for c in clips.CLIPS:
        print(f"[anims]   {c.name:14s} {c.length:4.2f}s {'loop' if c.loop else '    '} "
              f"{'root' if c.root_keys else '    '} events {len(c.events)}")


if __name__ == "__main__":
    main()
