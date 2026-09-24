"""Assemble review R5's pictures (docs/art/reviews/R5-style.md) from the renders in
build/review and the in-game captures in build/autotest/styles. Python 3.12 with Pillow
(a build tool, D50):

  py -3.12 tools/review_r5.py

The renders come from tools/blender/dragon_model.py (--style, --sex, --turntable),
tools/blender/emblem.py and tools/blender/banner3d.py; the captures from
tests/autotest/styles.txt.
"""
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
REV = ROOT / "build" / "review"
SHOTS = ROOT / "build" / "autotest" / "styles"
OUT = ROOT / "docs" / "art" / "reviews"
STYLES = [("current", "Current (textured)"), ("v1", "V1 surface"), ("v2", "V2 shape"), ("v3", "V3 bold")]
STAGES = ["hatchling", "juvenile", "adult"]
BG = (40, 26, 48)
INK = (250, 236, 210)
FONT = ROOT / "assets" / "fonts" / "nunito" / "Nunito-SemiBold.ttf"


def font(size):
    try:
        return ImageFont.truetype(str(FONT), size)
    except OSError:
        return ImageFont.load_default()


def grid(images, cols, cell, labels=None, title=None, pad=8):
    rows = (len(images) + cols - 1) // cols
    top = 44 if title else 0
    lab = 26 if labels else 0
    w, h = cols * (cell[0] + pad) + pad, top + rows * (cell[1] + lab + pad) + pad
    sheet = Image.new("RGB", (w, h), BG)
    d = ImageDraw.Draw(sheet)
    if title:
        d.text((pad + 4, 10), title, fill=INK, font=font(26))
    for i, im in enumerate(images):
        x = pad + (i % cols) * (cell[0] + pad)
        y = top + pad + (i // cols) * (cell[1] + lab + pad)
        if im is not None:
            sheet.paste(im.convert("RGB").resize(cell, Image.LANCZOS), (x, y + lab))
        if labels and labels[i]:
            d.text((x + 4, y + 2), labels[i], fill=INK, font=font(18))
    return sheet


def load(path):
    return Image.open(path) if Path(path).exists() else None


def save_jpg(im, name):
    OUT.mkdir(parents=True, exist_ok=True)
    im.save(OUT / name, "JPEG", quality=88)
    print("wrote", OUT / name)


def main():
    cell = (300, 300)
    # Every style side by side at three stages (neutral build).
    ims, labels = [], []
    for key, name in STYLES:
        for st in STAGES:
            ims.append(load(REV / f"r5_{key}_ember_{st}_three_quarter.png"))
            labels.append(f"{name}: {st}")
    save_jpg(grid(ims, 3, cell, labels, "R5: the Ember in four styles"), "R5-compare.jpg")
    # One sheet per style: male and female at each stage.
    for key, name in STYLES:
        ims, labels = [], []
        for sex in ("male", "female"):
            for st in STAGES:
                ims.append(load(REV / f"r5sex_{key}_{sex}_ember_{st}_three_quarter.png"))
                labels.append(f"{sex} {st}")
        save_jpg(grid(ims, 3, cell, labels, f"R5: {name}"), f"R5-{key}.jpg")
    # Turntables: the adult turning round.
    for key, _ in STYLES:
        frames = [load(REV / f"r5turn_{key}_ember_adult_turn{k:02d}.png") for k in range(12)]
        frames = [f.convert("RGB").resize((240, 240), Image.LANCZOS).quantize(colors=128) for f in frames if f]
        if frames:
            frames[0].save(OUT / f"R5-turn-{key}.gif", save_all=True, append_images=frames[1:], duration=160, loop=0)
            print("wrote", OUT / f"R5-turn-{key}.gif")
    # In the game (Azahar): the den and the close-up in each style.
    ims, labels = [], []
    for k, (key, name) in enumerate(STYLES):
        stem = ["s0-current", "s1-v1-surface", "s2-v2-shape", "s3-v3-bold"][k]
        ims += [load(SHOTS / f"{stem}_top.png"), load(SHOTS / f"{stem}_bottom.png")]
        labels += [f"{name}: the den", "up close"]
    save_jpg(grid(ims, 2, (400, 240), labels, "R5 in the game (Azahar, dev menu: Next style)"), "R5-ingame.jpg")
    # The emblem icon and the HOME Menu banners.
    icon = load(ROOT / "assets" / "icon.png")
    ims = [load(REV / "emblem_256.png"), icon.resize((192, 192), Image.NEAREST) if icon else None,
           load(ROOT / "assets" / "banner.png")]
    save_jpg(grid(ims, 3, (256, 256), ["the emblem", "the icon at 48 x 48 (x4)", "the flat banner (fallback)"],
                  "R5: the icon and the banner"), "R5-icon.jpg")
    frames = [load(REV / f"banner3d_{k}.png") for k in range(4)]
    save_jpg(grid(frames, 2, (400, 240), ["the 3D banner: rest", "head tilt", "blink", "nod"],
                  "R5: the animated 3D HOME Menu banner (through the HOME Menu's camera)"), "R5-banner3d.jpg")


main()
