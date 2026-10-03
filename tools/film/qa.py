"""A rendered cut checked frame by frame (the trailer's critique passes, docs/plan/trailer.md): frozen stretches,
black ones, and sudden jumps in brightness, against the cuts as planned.

    py -3.12 tools/film/qa.py build/film/edit/trailer.mp4 [--cut wide|tall]

Prints each finding with its time; a held frame under a fade or the end card's stillness is expected and named.
"""
import argparse
import os
import subprocess
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import edit  # noqa: E402


def frames(path, w=192, h=108):
    proc = subprocess.Popen(["ffmpeg", "-loglevel", "error", "-i", path, "-vf", f"scale={w}:{h}:flags=area", "-f", "rawvideo",
                             "-pix_fmt", "gray", "-"], stdout=subprocess.PIPE)
    out = []
    while True:
        buf = proc.stdout.read(w * h)
        if len(buf) < w * h:
            break
        out.append(np.frombuffer(buf, np.uint8).astype(np.float32) / 255.0)
    return np.array(out)


def runs(mask, min_len):
    found, start = [], None
    for i, m in enumerate(list(mask) + [False]):
        if m and start is None:
            start = i
        elif not m and start is not None:
            if i - start >= min_len:
                found.append((start, i))
            start = None
    return found


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("video")
    ap.add_argument("--cut", default="wide")
    a = ap.parse_args()
    edit.use_cut(a.cut)
    edit.resolve(edit.SHOTS)
    f = frames(a.video)
    fps = edit.FPS
    lum = f.mean(axis=1)
    diff = np.abs(np.diff(f, axis=0)).mean(axis=1)
    cuts = {round(s.t * fps) for s in edit.SHOTS}
    print(f"{a.video}: {len(f)} frames ({len(f) / fps:.2f} s)")
    # (a repeated frame decodes as all but identical; a slow, quiet picture still moves a little)
    for s0, s1 in runs(diff < 0.00006, 6):
        t0, t1 = s0 / fps, s1 / fps
        why = "(the end card)" if t0 >= edit.END_AT else ("(black)" if lum[s0] < 0.02 else "")
        print(f"  frozen  {t0:6.2f}-{t1:6.2f} s  {why or 'A REPEATED FRAME'}")
    quiet = runs(diff < 0.0008, 30)
    if quiet:
        print("  quiet   " + ", ".join(f"{a / fps:.1f}-{b / fps:.1f}" for a, b in quiet) + " s (slow pictures: look at them)")
    for s0, s1 in runs(lum < 0.02, 3):
        print(f"  black   {s0 / fps:6.2f}-{s1 / fps:6.2f} s")
    for i in np.nonzero(np.abs(np.diff(lum)) > 0.12)[0]:
        near = min(abs(i + 1 - c) for c in cuts)
        print(f"  jump    {(i + 1) / fps:6.2f} s  brightness {lum[i]:.2f} -> {lum[i + 1]:.2f}  {'(a cut)' if near <= 1 else 'NOT AT A CUT'}")
    # the biggest frame-to-frame changes that aren't cuts: a pop, a glitch
    order = np.argsort(-diff)
    shown = 0
    for i in order:
        if min(abs(i + 1 - c) for c in cuts) <= 2:
            continue
        print(f"  change  {(i + 1) / fps:6.2f} s  {diff[i]:.3f}")
        shown += 1
        if shown >= 8:
            break


if __name__ == "__main__":
    main()
