"""A reel at a glance (the trailer's edit, docs/plan/trailer.md): N frames evenly across it, each labelled with
its frame number, in one picture.

    py -3.12 tools/film/strip.py <reel.mkv> <out.png> [--n 8] [--cols 4] [--width 480] [--top] [--from F --to F]

--top keeps the top screen of a two-screen reel (its left 5/9).
"""
import argparse
import subprocess

import numpy as np
from PIL import Image, ImageDraw


def probe(path):
    out = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-count_packets", "-show_entries",
                          "stream=width,height,nb_read_packets", "-of", "csv=p=0", path],
                         capture_output=True, text=True, check=True).stdout.strip().split(",")
    return int(out[0]), int(out[1]), int(out[2])


def frames(path, picks, w, h, top):
    """The picked frames, as RGB arrays (one decode pass)."""
    want = set(picks)
    cw = w * 5 // 9 if top else w
    proc = subprocess.Popen(["ffmpeg", "-loglevel", "error", "-i", path, "-f", "rawvideo", "-pix_fmt", "rgb24", "-"],
                            stdout=subprocess.PIPE)
    got, n = {}, 0
    while len(got) < len(want):
        buf = proc.stdout.read(w * h * 3)
        if len(buf) < w * h * 3:
            break
        if n in want:
            got[n] = np.frombuffer(buf, np.uint8).reshape(h, w, 3)[:, :cw].copy()
        n += 1
    proc.kill()
    return got


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("reel")
    ap.add_argument("out")
    ap.add_argument("--n", type=int, default=8)
    ap.add_argument("--cols", type=int, default=4)
    ap.add_argument("--width", type=int, default=480)
    ap.add_argument("--top", action="store_true")
    ap.add_argument("--from", dest="first", type=int, default=0)
    ap.add_argument("--to", dest="last", type=int, default=-1)
    a = ap.parse_args()
    w, h, count = probe(a.reel)
    last = count - 1 if a.last < 0 else min(a.last, count - 1)
    picks = [int(round(v)) for v in np.linspace(a.first, last, a.n)]
    got = frames(a.reel, picks, w, h, a.top)
    tiles = []
    for p in picks:
        if p not in got:
            continue
        im = Image.fromarray(got[p])
        im = im.resize((a.width, int(round(a.width * im.height / im.width))), Image.LANCZOS)
        d = ImageDraw.Draw(im)
        d.rectangle([0, 0, 92, 18], fill=(0, 0, 0))
        d.text((4, 3), f"{p} ({p / 60:.2f}s)", fill=(255, 220, 120))
        tiles.append(im)
    rows = (len(tiles) + a.cols - 1) // a.cols
    th = tiles[0].height
    sheet = Image.new("RGB", (a.cols * a.width, rows * th), (20, 14, 26))
    for i, t in enumerate(tiles):
        sheet.paste(t, ((i % a.cols) * a.width, (i // a.cols) * th))
    sheet.save(a.out)
    print(f"{a.reel}: {count} frames, {w}x{h} -> {a.out}")


if __name__ == "__main__":
    main()
