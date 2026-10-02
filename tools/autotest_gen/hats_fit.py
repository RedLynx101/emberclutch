"""Writes tests/autotest/hatsfit_h.txt and hatsfit_g.txt: every kind as a hatchling and grown in
the wardrobe, its head close up in four head things (the straw sunhat, the little top hat, the
ember crown, the party hat), to check how each sits on the skull (run 23: "many are floating
quite far off of their heads"), from the side and as the
den's camera sees it.

    python tools/autotest_gen/hats_fit.py
"""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
KINDS = ["pouncer", "puffback", "curlstone", "crestwing", "ribbontail", "flurrytail", "glimmermoth", "duskwing",
         "blazeplume", "kindlemoss", "cindershell", "bloomstone", "lilyfin", "frostcurl"]
HATS = [(0, "sunhat"), (1, "tophat"), (4, "crown"), (6, "party")]

HEAD = """overlay off
wait 2.5
name Noah
tap 160 107
wait 1.5
tap 160 120
tap 160 120
wait 2
key SELECT
wait 0.3
tap 238 81
key SELECT
name Rill
wait 12
pg give
pg {form}
pg bare
pg wardrobe
wait 2
pg spin 0.9
pg tab 0
"""


def script(form):
    tag = "h" if form == "hatchling" else "g"
    lines = [f"# Head things on every kind, {form} (run 23's floating hats): written by tools/autotest_gen/hats_fit.py",
             HEAD.format(form=form).rstrip()]
    for k, name in enumerate(KINDS):
        lines.append(f"pg kind {k} 0")
        lines.append("wait 1.6")
        for hid, hname in HATS:
            lines.append(f"pg wear {hid}")
            lines.append("pg spin 0.9")
            lines.append("wait 0.7")
            lines.append(f"shot {tag}{k + 1:02d}-{name}-{hname}")
            lines.append("pg spin 0.15")  # (and as the den's camera sees it, from the front)
            lines.append("wait 0.5")
            lines.append(f"shot {tag}{k + 1:02d}-{name}-{hname}F")
    lines.append("quit")
    return "\n".join(lines) + "\n"


for form, tag in (("hatchling", "h"), ("grown", "g")):
    path = os.path.join(ROOT, "tests", "autotest", f"hatsfit_{tag}.txt")
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(script(form))
    print(path)
