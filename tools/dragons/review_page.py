"""R11b, Noah's review of the new dragons: turns each kind's review renders into small JPGs and
writes the review page (plain Python; the JPGs through tools/concept/to_jpg.ps1).

  python tools/dragons/review_page.py [kind ...]       (default: every kind)

Reads build/review/<kind>/{lineup,eggs,variants,turntable,portraits,clips,babyclips}.png
(tools/blender/dragonkit/review.py), writes docs/art/reviews/R11b/<kind>/*.jpg and
docs/art/reviews/R11b-new-dragons.md.
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import dragons  # noqa: E402
from dragons import lore  # noqa: E402

SHEETS = [("lineup", "Growing up, at true size", 1400), ("eggs", "The egg in each colouring; cracking; opening", 960),
          ("variants", "The four colourings grown (the rare one last), and the hatchling plain and rare", 960),
          ("turntable", "All the way round", 1200), ("portraits", "Faces: calm (round pupils) and startled (slit)", 960),
          ("clips", "Moving: walk, run, sit, sleep, play bow, pounce, flying, tail wag", 960),
          ("babyclips", "The hatchling: walk, scamper, hop, sit, curl up, a happy wiggle", 960)]
PAGE = os.path.join(ROOT, "docs", "art", "reviews", "R11b-new-dragons.md")


def to_jpg(kind, name, width):
    src = os.path.join("build", "review", kind)
    if not os.path.exists(os.path.join(ROOT, src, name + ".png")):
        return False
    subprocess.run(["powershell", "-ExecutionPolicy", "Bypass", "-File",
                    os.path.join(ROOT, "tools", "concept", "to_jpg.ps1"), "-Names", name, "-OutDir",
                    os.path.join("docs", "art", "reviews", "R11b", kind), "-SrcDir", src, "-Width", str(width)],
                   check=True, capture_output=True)
    return True


def stat_bar(v):
    return "●" * v + "○" * (10 - v)


def section(k):
    m = k.META
    els = m["element"] if isinstance(m["element"], (tuple, list)) else (m["element"],)
    doc = (k.__doc__ or "").strip().split("\n\n")[0].replace("\n", " ")
    lines = [f"## {m['dex']}. {m['title']}: {' · '.join(els)}, {m['rarity']}"]
    if m.get("parents"):
        pa = " x ".join(dragons.kind(p).META["title"] for p in m["parents"])
        lines.append(f"*A crossbreed of the {pa}.*")
    lines += ["", doc, "", f"> {m['blurb']}", ""]
    lines.append("| Stat | |")
    lines.append("|---|---|")
    for s in lore.STATS:
        lines.append(f"| {s.capitalize()} | {stat_bar(m['stats'][s])} {m['stats'][s]} |")
    lines.append("")
    lines.append(f"**Size grown:** {m.get('size', 1.0):.2f} of a Pouncer. **Manners it leans to:** "
                 f"{', '.join(m['manners'])}. **Traits it leans to:** {', '.join(m['traits'])}.")
    lines.append(f"**Colourings:** {', '.join(v['name'] for v in k.VARIANTS[:3])}; rare: **{k.VARIANTS[3]['name']}**.")
    lines.append("")
    for name, caption, width in SHEETS:
        if to_jpg(m["name"], name, width):
            lines.append(f"{caption}:")
            lines.append(f"![](R11b/{m['name']}/{name}.jpg)")
            lines.append("")
    return lines


def main():
    names = [a for a in sys.argv[1:] if not a.startswith("--")]
    kinds = [k for k in dragons.all_kinds() if not names or k.META["name"] in names]
    out = ["# Review R11b — The new dragons (D77–D79)", "",
           "**For Noah.** The first nine kinds of the dragon revamp, built with the dragon kit in the",
           "storybook look (D75): the eight base breeds (three common, three harder to get, two rare) and",
           "the first crossbreed. Every image is the game's own model, posed by its own animations, in the",
           "colours and textures the 3DS shows (Blender renders; in the game: dev menu page 1, Next kind",
           "and Kind colouring). Nothing here is from the concept images, which were only reference.", "",
           "| # | Kind | Element | Rarity | Size | Wing | Wit | Might | Breath | Stamina |",
           "|---|---|---|---|---|---|---|---|---|---|"]
    for k in kinds:
        m = k.META
        els = m["element"] if isinstance(m["element"], (tuple, list)) else (m["element"],)
        s = m["stats"]
        out.append(f"| {m['dex']} | {m['title']} | {' · '.join(els)} | {m['rarity']} | {m.get('size', 1.0):.2f} | "
                   f"{s['wing']} | {s['wit']} | {s['might']} | {s['breath']} | {s['stamina']} |")
    out.append("")
    for k in kinds:
        out += section(k)
    open(PAGE, "w", encoding="utf-8", newline="\n").write("\n".join(out) + "\n")
    print(f"[review] {PAGE}: {len(kinds)} kinds")


if __name__ == "__main__":
    main()
