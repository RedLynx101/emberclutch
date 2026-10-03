"""The trailer's contact sheet (tools/film/capture.ps1): stills, labelled with their reel's name, four across.

    py -3.12 tools/film/contact.py <out.png> <still.png>...
"""
import os
import sys

from PIL import Image, ImageDraw

out, paths = sys.argv[1], sorted(sys.argv[2:])
cols = 4
sheet = Image.new("RGB", (cols * 400, ((len(paths) + cols - 1) // cols) * 262), (40, 28, 50))
draw = ImageDraw.Draw(sheet)
for i, p in enumerate(paths):
    x, y = (i % cols) * 400, (i // cols) * 262
    sheet.paste(Image.open(p).convert("RGB"), (x, y))
    name = os.path.splitext(os.path.basename(p))[0]
    draw.text((x + 6, y + 245), name[:-4] if name.endswith("_top") else name, fill=(245, 196, 81))
sheet.save(out)
print(f"[film] {len(paths)} reels -> {out}")
