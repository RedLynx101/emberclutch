"""The trailer's YouTube thumbnails (docs/plan/trailer.md, V5): three to pick from, 1280x720, from the 6x reels.

    py -3.12 tools/film/thumbs.py

Each is a frame of the game (cropped in on its subject: the reels are 2400x1440, so there's room), the wordmark
in the title screen's gold with a dark edge that holds at a phone's thumbnail size, and a short line.
"""
import os
import subprocess

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GRAB = os.path.join(ROOT, "build", "film", "grab")
OUT = os.path.join(ROOT, "build", "film", "thumbs")
FONT_TITLE = os.path.join(ROOT, "assets", "fonts", "cinzel-decorative", "CinzelDecorative-Bold.ttf")
FONT_BODY = os.path.join(ROOT, "assets", "fonts", "nunito", "Nunito-SemiBold.ttf")
GOLD, SHELL, EMBER, TRACK = (245, 196, 81), (255, 243, 220), (232, 102, 43), (28, 17, 34)

THUMBS = [  # (name, reel, frame, crop box in the top screen (x, y, w, h), wordmark at, line)
    ("a_fly", "s08_takeoff", 104, (120, 40, 2160, 1215), "top", "Raise it. Ride it."),
    ("b_hatch", "s02_hatch", 330, (420, 330, 1600, 900), "top", "Hatch your own dragon"),
    ("c_egg", "s01_egg", 200, (300, 120, 1960, 1102), "top", "A free game for the 3DS"),
]


def frame(reel, n):
    path = os.path.join(GRAB, reel + ".mkv")
    w, h = (int(v) for v in subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
                                             "stream=width,height", "-of", "csv=p=0", path],
                                            capture_output=True, text=True, check=True).stdout.strip().split(","))
    crop = "crop=2400:1440:0:0," if w > h * 2 else ""
    raw = subprocess.run(["ffmpeg", "-loglevel", "error", "-i", path, "-vf", f"{crop}select=eq(n\\,{n})", "-frames:v", "1",
                          "-f", "rawvideo", "-pix_fmt", "rgb24", "-"], capture_output=True, check=True).stdout
    return Image.frombytes("RGB", (2400, 1440), raw)


def make(name, reel, n, box, where, line):
    x, y, w, h = box
    im = frame(reel, n).resize((1280, 720), Image.LANCZOS, box=(x, y, x + w, y + h))
    a = np.asarray(im, np.float32) / 255.0
    yy, xx = np.mgrid[0:720, 0:1280].astype(np.float32)
    d = np.sqrt(((xx - 640) / 640) ** 2 + ((yy - 360) / 360) ** 2) / np.sqrt(2)
    a *= (1 - 0.35 * np.clip((d - 0.35) / 0.65, 0, 1) ** 1.5)[..., None]
    a = np.clip((a - 0.5) * 1.08 + 0.5 + 0.02, 0, 1)  # (a touch more contrast: it's small)
    # a soft darkening behind the words, for them to stand on
    shade = np.zeros((720, 1280), np.float32)
    shade[:300] = np.linspace(0.62, 0.0, 300)[:, None] if where == "top" else 0
    a *= (1 - shade[..., None])
    im = Image.fromarray((a * 255).astype(np.uint8)).convert("RGBA")
    layer = Image.new("RGBA", im.size, (0, 0, 0, 0))
    glow = Image.new("RGBA", im.size, (0, 0, 0, 0))
    f = ImageFont.truetype(FONT_TITLE, 112)
    ImageDraw.Draw(glow).text((640, 118), "EMBERCLUTCH", font=f, fill=EMBER + (190,), anchor="mm")
    layer.alpha_composite(glow.filter(ImageFilter.GaussianBlur(16)))
    dr = ImageDraw.Draw(layer)
    dr.text((640, 118), "EMBERCLUTCH", font=f, fill=GOLD + (255,), anchor="mm", stroke_width=6, stroke_fill=TRACK + (255,))
    f2 = ImageFont.truetype(FONT_BODY, 46)
    dr.text((640, 214), line, font=f2, fill=SHELL + (255,), anchor="mm", stroke_width=4, stroke_fill=TRACK + (230,))
    # the 3DS pill, low right
    f3 = ImageFont.truetype(FONT_BODY, 34)
    label = "FREE  •  3DS"
    tw = f3.getlength(label)
    x1, y1 = 1240, 680
    dr.rounded_rectangle([x1 - tw - 44, y1 - 56, x1, y1], 28, fill=TRACK + (215,), outline=GOLD + (255,), width=3)
    dr.text((x1 - tw / 2 - 22, y1 - 28), label, font=f3, fill=GOLD + (255,), anchor="mm")
    im.alpha_composite(layer)
    out = os.path.join(OUT, name + ".png")
    im.convert("RGB").save(out)
    im.convert("RGB").save(os.path.join(OUT, name + ".jpg"), quality=92)
    print(out)


def main():
    os.makedirs(OUT, exist_ok=True)
    for t in THUMBS:
        make(*t)
    # all three side by side at a phone's size, for picking
    ims = [Image.open(os.path.join(OUT, t[0] + ".png")).resize((426, 240), Image.LANCZOS) for t in THUMBS]
    sheet = Image.new("RGB", (426 * 3 + 40, 280), (20, 14, 26))
    for i, im in enumerate(ims):
        sheet.paste(im, (10 + i * 436, 20))
    sheet.save(os.path.join(OUT, "pick.png"))


if __name__ == "__main__":
    main()
