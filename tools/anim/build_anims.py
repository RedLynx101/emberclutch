"""Build the animation libraries (pure Python).

  python tools/anim/build_anims.py [--out path]          romfs/anims/dragon.eca (the classic dragon, tools/anim/clips.py)
  python tools/anim/build_anims.py --plan pouncer        romfs/anims/pouncer.eca (a body plan, tools/dragons/plans)
  python tools/anim/build_anims.py --plans               every body plan
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "blender"))
sys.path.insert(0, os.path.join(ROOT, "tools"))
from eca import q_from_pyr, write_eca  # noqa: E402


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


def build(clip_list, order, out):
    os.makedirs(os.path.dirname(out), exist_ok=True)
    names = [c.name for c in clip_list]
    assert len(names) == len(set(names)), "duplicate clip names"
    size = write_eca(out, clip_list, order)
    total = sum(c.length for c in clip_list)
    print(f"[anims] {out}: {len(clip_list)} clips, {total:.1f} s, {size} bytes, {len(order)} bones")
    if "--verbose" in sys.argv:
        for c in clip_list:
            print(f"[anims]   {c.name:14s} {c.length:4.2f}s {'loop' if c.loop else '    '} "
                  f"{'root' if c.root_keys else '    '} events {len(c.events)}")


def build_plan(name, out=None):
    import dragons
    plan = dragons.plan(name)
    build(plan.clips(), plan.BONE_ORDER, out or os.path.join(ROOT, "romfs", "anims", f"{plan.NAME}.eca"))


def main():
    check_conventions()
    out = sys.argv[sys.argv.index("--out") + 1] if "--out" in sys.argv else None
    if "--plan" in sys.argv:
        build_plan(sys.argv[sys.argv.index("--plan") + 1], out)
    elif "--plans" in sys.argv:
        for f in sorted(os.listdir(os.path.join(ROOT, "tools", "dragons", "plans"))):
            if f.endswith(".py") and not f.startswith("_"):
                build_plan(f[:-3])
    else:
        import clips
        from rig_layout import BONE_ORDER
        build(clips.CLIPS, BONE_ORDER, out or os.path.join(ROOT, "romfs", "anims", "dragon.eca"))


if __name__ == "__main__":
    main()
