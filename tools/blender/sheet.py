"""Tile rendered PNGs into one review sheet (no extra Python packages: Blender's numpy).

  blender -b -P tools/blender/sheet.py -- --out sheet.png --cols 3 a.png b.png c.png ...

Every tile is scaled to the first image's size. A missing file leaves its tile empty.
"""
import os
import sys

import bpy
import numpy as np

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
out = argv[argv.index("--out") + 1]
cols = int(argv[argv.index("--cols") + 1]) if "--cols" in argv else 3
files = [a for i, a in enumerate(argv) if a.lower().endswith(".png") and argv[i - 1] not in ("--out",)]


def load(path):
    img = bpy.data.images.load(path)
    w, h = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)
    bpy.data.images.remove(img)
    return px


tiles = [load(f) if os.path.exists(f) else None for f in files]
first = next(t for t in tiles if t is not None)
th, tw = first.shape[:2]
rows = (len(tiles) + cols - 1) // cols
sheet = np.zeros((rows * th, cols * tw, 4), dtype=np.float32)
sheet[..., 3] = 1.0
for i, t in enumerate(tiles):
    if t is None:
        continue
    if t.shape[:2] != (th, tw):  # nearest-neighbour resize
        ys = (np.arange(th) * t.shape[0] / th).astype(int)
        xs = (np.arange(tw) * t.shape[1] / tw).astype(int)
        t = t[ys][:, xs]
    r, c = divmod(i, cols)
    y0 = (rows - 1 - r) * th  # Blender images are stored bottom row first
    sheet[y0:y0 + th, c * tw:(c + 1) * tw] = t

img = bpy.data.images.new("sheet", cols * tw, rows * th, alpha=False)
img.pixels = sheet.ravel()
img.filepath_raw = out
img.file_format = "PNG"
img.save()
print(f"[sheet] {out}: {len(files)} tiles, {cols}x{rows}")
