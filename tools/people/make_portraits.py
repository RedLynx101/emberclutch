"""Pack each story person's portraits into romfs/portraits/<portrait>.t3x (D138): the five 64 x 64
frames tools/blender/people_model.py renders (--sheets portraits) into assets/sprites/portraits/
<portrait>_<k>.png, k in faces.PORTRAIT_FEELS order (calm, happy, sad, angry, surprised; app/emotes
portraitFrame), as one tex3ds atlas the dialogue box loads as they start speaking.

  python tools/people/make_portraits.py          every portrait found
  python tools/people/make_portraits.py fig tam
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
import faces  # noqa: E402

SRC = os.path.join(ROOT, "assets", "sprites", "portraits")
OUT = os.path.join(ROOT, "romfs", "portraits")
TEX3DS = os.path.join(os.environ.get("DEVKITPRO", "C:/msys64/opt/devkitpro"), "tools", "bin", "tex3ds.exe")


def names():
    found = set()
    for f in os.listdir(SRC):
        if f.endswith("_0.png"):
            found.add(f[:-6])
    return sorted(found)


def main():
    want = sys.argv[1:] or names()
    os.makedirs(OUT, exist_ok=True)
    bad = 0
    for name in want:
        frames = [os.path.join(SRC, f"{name}_{k}.png") for k in range(len(faces.PORTRAIT_FEELS))]
        missing = [f for f in frames if not os.path.exists(f)]
        if missing:
            print(f"[portraits] {name}: missing {', '.join(os.path.basename(m) for m in missing)}")
            bad += 1
            continue
        out = os.path.join(OUT, f"{name}.t3x")
        r = subprocess.run([TEX3DS, "--atlas", "-f", "rgba8", "-z", "auto", "-o", out] + frames, capture_output=True, text=True)
        if r.returncode != 0:
            print(f"[portraits] {name}: tex3ds failed: {r.stderr.strip() or r.stdout.strip()}")
            bad += 1
            continue
        print(f"[portraits] {out}: {len(frames)} frames, {os.path.getsize(out)} bytes")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
