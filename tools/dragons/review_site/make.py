"""The R11b review as a page for Noah (a claude.ai artifact): every kind's sheets, its stats and
design notes, the game's own screenshots, and a verdict and notes per kind that the page keeps
(its db: collections `verdicts` and `questions`, which Claude reads back). Plain Python.

  python tools/dragons/review_site/make.py

Needs the review JPGs (tools/dragons/review_page.py) and the scripted run's shots
(tools\\autotest.ps1 tests\\autotest\\kinds_all.txt). Writes build/review/site/ (index.html,
img/<kind>/*.jpg, game/*.jpg) and prints the published file list.
"""
import json
import os
import shutil
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import dragons  # noqa: E402

SITE = os.path.join(ROOT, "build", "review", "site")
SHEETS = {  # key: (caption, rows of the strip or None)
    "variants": ("The four colourings grown, the rare one last; the hatchling plain and rare", None),
    "lineup": ("Growing up at true size: hatchling to grown", None),
    "turntable": ("All the way round", None),
    "portraits": ("Faces: calm (round pupils) and startled (slit), hatchling and grown", None),
    "eggs": ("The egg in each colouring, cracking and opening", None),
    "clips": ("Moving, grown", ["Walk", "Run", "Sit", "Sleep", "Play bow", "Pounce", "Flying", "Tail wag"]),
    "babyclips": ("Moving, hatchling", ["Walk", "Scamper", "Hop", "Sit", "Curl up", "Happy wiggle"]),
}
RARITY = {"common": "Common", "uncommon": "Harder to find", "rare": "Rare"}

# What each kind is, for Noah (from the kind files and the builders' reports), and the rough
# edges still known, so the review can weigh them.
NOTES = {
    "pouncer": dict(
        baby="A kitten: a big round head, huge eyes, stubby legs, little sail-like wing buds and a leaf-tipped tail.",
        grown="A lithe panther of a dragon: long legs, a small round head with pointed ears and swept-back horns, "
              "big scalloped bat wings and a long tail ending in twin fins. The classic dragon of the eight.",
        moves="Quick and catlike: a prowling walk, a bounding gallop, a play bow and a pounce, a cat's tail sway at rest.",
        rare="Emberblaze: deep red with gold, a glowing ember sail down its neck and back, and glowing marks.",
        rough="Its folded wing still reads as a spiky fan from some angles."),
    "puffback": dict(
        baby="A bun: head and body one soft round shape, a two-leaf sprout on top, huge low eyes, nub legs.",
        grown="A walking hill: a broad domed back of mossy plates with flowers and ferns growing between them, "
              "stump legs, a big kind face under a calm brow, and leaf-fan wings comically too small for it. "
              "A mossy saddle mid-back for a rider.",
        moves="Slow, heavy and bouncy: a rolling waddle, a trundling run, plopping down to sit, flopping on its belly "
              "to nap; in the air the little wings buzz and the body stays level.",
        rare="Moonbloom: night-teal and mint with golden eyes, glowing moss spots and a crown of glowing moonflowers."),
    "curlstone": dict(
        baby="A little round pebble, half curled: huge eyes, a long button snout, its thick tail round its side "
             "and three amber crystal buds on its head.",
        grown="Low, long and sturdy under a high domed back of overlapping stone plates, like a pangolin. "
              "A gentle digging snout, big claws, a stone club on its tail and a crown of warm amber crystals.",
        moves="A steady waddle and trot; it tucks into a ball and rolls like a boulder to run, curls up to sleep "
              "or sulk, hides its face when shy and sits up like a pangolin.",
        rare="Geode: dusky violet stone with glowing amethyst seams and bigger amethyst crystals."),
    "crestwing": dict(
        baby="Round and big-headed with a spiky tuft of down on its crown, a collar of down, feather-bud wings "
             "and a little golden tail tuft.",
        grown="Tall and elegant, a heron and a peacock made dragon: a proud S-shaped neck, long legs, a crest that "
              "rises when it's pleased and lies flat when it sulks, feathered wings that fold like a cloak, and a "
              "fan of long tail plumes.",
        moves="A high-stepping strut with a pigeon's head bob, preening, a heron's coiled-neck stalk, a bow into "
              "a wing-and-crest display to greet, slow crane-like wingbeats and wide glides.",
        rare="Starplume: midnight violet with turquoise feathers, glowing star spots, a bigger crest and a "
             "peacock fan of seven plumes, each with a glowing eye.",
        rough="Folded coverts crumple a little at the elbow from some angles; sitting, the folded feathers dip "
              "into the rump; the baby reads as downy tufts more than truly fluffy."),
    "ribbontail": dict(
        baby="A little noodle: a big round head, huge dark eyes, an axolotl's smile and frilly fan-gills, "
             "with its finned tail curled round its side.",
        grown="Long and low, a river spirit: a swan's S of a neck, a crown of swept fins and two long curling "
              "whiskers, frilled fin-wings that stand like sails, and a ribbon of fin down the tail to a koi's fan.",
        moves="Moves like water: an S-wave walk on short legs, sails that spread and ripple as it swims through "
              "the air.",
        rare="Moonpearl: lavender and pearl with glowing aqua pearl spots, a glowing ribbon and fan, long "
             "streamers and a glowing pearl on its brow."),
    "flurrytail": dict(
        baby="A round fluffball: fox ears with coloured tips, crystal antler buds and a pom-pom tail nearly "
             "as big as its head.",
        grown="An elegant snow-fox dragon: a full ruff and mane, branching ice-crystal antlers, and a plumed tail "
              "bigger than its body curling up over its back. Scalloped wings with icicles fold into a cape.",
        moves="Light on its feet: a spring in every step, a fox sit with the plume round its paws, asleep with its "
              "nose tucked in its tail, and a mousing dive of a pounce.",
        rare="Aurora: midnight navy with violet points, glowing seafoam aurora ribbons across the plume, bigger "
             "glowing antlers and glowing icicles.",
        rough="The adult's fur is faceted up close (the triangle budget); asleep, it reads as a fluffy mound from "
              "the three-quarter view."),
    "glimmermoth": dict(
        baby="A pom-pom: a round fuzzy body, huge shiny eyes, a big fluffy collar, feathery antennae and four "
             "little wing buds.",
        grown="A fawn with moth wings: long legs, a long upright neck from a fluffy ruff, fern-frond antennae, "
              "and four broad wings with sun-ring eyespots and long luna-moth tails that fold into a moth's roof.",
        moves="Graceful and airy; at rest it fans its folded wings gently.",
        rare="Sunburst: warm amber with glowing sun bands, flame-orange wings with glowing margins and eyespots, "
             "and a glowing halo of rays behind its antennae."),
    "duskwing": dict(
        baby="A round fluffball with enormous bat ears and huge eyes that stands up like a bat pup and toddles "
             "with its wing-arms out for balance.",
        grown="A sleek wyvern of the night: tall ears, a fluffy ruff, strong hind legs and a whip tail ending in "
              "a crescent moon. Its wings are its forelegs; folded they hang like a cloak, spread they're a night "
              "sky full of stars.",
        moves="A bat's crawl-walk on wrists and feet, a bounding run, sitting wrapped in its wings, quiet glides "
              "and deep wingbeats, ears that swivel.",
        rare="Eclipse: black-violet with glowing gold stars, a golden corona on the wings, gold-rimmed ears and a "
             "crescent-moon crown."),
    "blazeplume": dict(
        baby="A round kitten-chick: fluffy cheeks and bib, flame-tipped kitten ears, a three-feather crest and a "
             "stumpy tail tipped with a little candle flame.",
        grown="A phoenix-cat: the Pouncer's lithe body and cat's face with the Crestwing's feathers, and every "
              "feather a flame, gold to scarlet to a hot orange tip. A flame crest down the nape and a tail of "
              "glowing plumes held up like a torch.",
        moves="The Pouncer's body plan, so it moves like a proud cat, with feathered wings that fold into a neat "
              "bundle along the back.",
        rare="Phoenix: white-gold with glowing flames and wing tips, a bigger glowing crest and longer, wavier "
             "flame plumes."),
}

QUESTIONS = [
    dict(id="rarity", q="Are the rarities right?",
         hint="Common: Pouncer, Puffback, Curlstone. Harder to find: Crestwing, Ribbontail, Flurrytail. "
              "Rare: Glimmermoth, Duskwing. Rarer kinds start with better stats and rarer traits.",
         choices=["Right as they are", "Change them (say how)"]),
    dict(id="next", q="Which crossbreeds should come next?",
         hint="My suggestion: the pairs of the three common breeds first (Pouncer × Puffback, Pouncer × Curlstone, "
              "Puffback × Curlstone), since players meet those early. Then common with harder to find.",
         choices=["The commons' pairs first", "Different ones (say which)"]),
    dict(id="device", q="When do you want the nine on your 3DS?",
         hint="Now, as a test build (dev menu, Next kind), or after DR3, when they hatch from real eggs in "
              "your save and your current dragons become new kinds.",
         choices=["Send a test build now", "After DR3"]),
    dict(id="sizes", q="Do the sizes feel right?",
         hint="The picture under the table shows all nine grown at true size. The Puffback is the biggest (1.35 of a "
              "Pouncer), the Glimmermoth the smallest (0.85); each is one number to change.",
         choices=["Right as they are", "Change some (say which)"]),
    dict(id="other", q="Anything else?", hint="The look overall, stats, names, anything.", choices=[]),
]


def png_size(path):
    with open(path, "rb") as f:
        head = f.read(24)
    return struct.unpack(">II", head[16:24])


def to_jpg(names, src, out, width):
    subprocess.run(["powershell", "-ExecutionPolicy", "Bypass", "-File",
                    os.path.join(ROOT, "tools", "concept", "to_jpg.ps1"), "-Names", ",".join(names),
                    "-OutDir", out, "-SrcDir", src, "-Width", str(width)], check=True, capture_output=True)


def main():
    if os.path.isdir(SITE):
        shutil.rmtree(SITE)
    os.makedirs(SITE)
    shots = os.path.join("build", "autotest", "kinds_all")
    files, kinds = ["index.html"], []
    for i, k in enumerate(dragons.all_kinds()):
        m = k.META
        name = m["name"]
        els = list(m["element"]) if isinstance(m["element"], (tuple, list)) else [m["element"]]
        sheets = []
        for key, (caption, rows) in SHEETS.items():
            jpg = os.path.join(ROOT, "docs", "art", "reviews", "R11b", name, key + ".jpg")
            png = os.path.join(ROOT, "build", "review", name, key + ".png")
            if not os.path.exists(jpg):
                continue
            os.makedirs(os.path.join(SITE, "img", name), exist_ok=True)
            shutil.copy(jpg, os.path.join(SITE, "img", name, key + ".jpg"))
            w, h = png_size(png)
            sheets.append(dict(key=key, src=f"img/{name}/{key}.jpg", caption=caption, w=w, h=h, rows=rows))
            files.append(f"img/{name}/{key}.jpg")
        game = []
        for pre, caption in (("h", "Hatchling"), ("g", "Grown"), ("r", "Rare")):
            shot = f"{pre}{i + 1:02d}-{name}" + ("-rare" if pre == "r" else "") + "_top"
            if os.path.exists(os.path.join(ROOT, shots, shot + ".png")):
                to_jpg([shot], shots, os.path.join("build", "review", "site", "game"), 400)
                game.append(dict(src=f"game/{shot}.jpg", caption=caption))
                files.append(f"game/{shot}.jpg")
        kinds.append(dict(
            id=name, dex=m["dex"], title=m["title"], elements=els, rarity=m["rarity"],
            rarityLabel=RARITY[m["rarity"]], size=m.get("size", 1.0), stats=m["stats"],
            total=sum(m["stats"].values()), manners=list(m["manners"]), traits=list(m["traits"]),
            colourings=[v["name"] for v in k.VARIANTS[:3]], rare=k.VARIANTS[3]["name"], blurb=m["blurb"],
            parents=[dragons.kind(p).META["title"] for p in m.get("parents", ())],
            notes=NOTES.get(name, {}), sheets=sheets, game=game))
    together = None
    jpg = os.path.join(ROOT, "docs", "art", "reviews", "R11b", "together.jpg")
    if os.path.exists(jpg):
        shutil.copy(jpg, os.path.join(SITE, "img", "together.jpg"))
        w, h = png_size(os.path.join(ROOT, "build", "review", "together.png"))
        together = dict(src="img/together.jpg", w=w, h=h)
        files.append("img/together.jpg")
    template = open(os.path.join(HERE, "template.html"), encoding="utf-8").read()
    data = json.dumps(dict(kinds=kinds, questions=QUESTIONS, together=together), ensure_ascii=False)
    page = template.replace("/*DATA*/{}", data)
    open(os.path.join(SITE, "index.html"), "w", encoding="utf-8", newline="\n").write(page)
    print(f"[site] {SITE}: {len(kinds)} kinds, {len(files)} files")
    print(json.dumps(files))


if __name__ == "__main__":
    main()
