"""A page for Noah to play the takes of a sound and mark each keep or cut.

  python tools/audio/make_picker.py      -> build/picker/index.html + build/picker/sfx/*.wav

The takes are the game's own files (romfs/sfx), in the order the game plays them. The page is
published as an Artifact with the `db` capability and the wavs as its files; the picks land in
its database (collection `picks`, a document per sound: {takes: {"1": "keep"|"cut"}, note}),
where Claude reads them back. Add a sound to SOUNDS to ask about it next time.
"""
import html
import json
import os
import shutil
import wave

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "build", "picker")

# slug, what it is, where it plays, the takes (romfs/sfx files in the order the game plays them)
SOUNDS = [
    ("dragon-step-grass", "A grown dragon's step on grass",
     "The valley, walking and running on land. The game plays the takes in turn: 1, 2, 3, 1, 2, 3...",
     ["dragon-step-grass.wav", "dragon-step-grass-2.wav", "dragon-step-grass-3.wav"]),
]


def main():
    shutil.rmtree(OUT, ignore_errors=True)
    os.makedirs(os.path.join(OUT, "sfx"))
    sounds = []
    for slug, title, where, takes in SOUNDS:
        items = []
        for i, f in enumerate(takes):
            src = os.path.join(ROOT, "romfs", "sfx", f)
            shutil.copy(src, os.path.join(OUT, "sfx", f))
            with wave.open(src) as w:
                secs = w.getnframes() / w.getframerate()
            items.append(dict(n=i + 1, file="sfx/" + f, secs=round(secs, 2)))
        sounds.append(dict(slug=slug, title=title, where=where, takes=items))
    page = open(os.path.join(os.path.dirname(__file__), "picker_template.html"), encoding="utf-8").read()
    page = page.replace("/*DATA*/", json.dumps(sounds).replace("</", "<\\/"))
    open(os.path.join(OUT, "index.html"), "w", encoding="utf-8", newline="\n").write(page)
    print(f"[picker] {OUT}: {len(sounds)} sound(s), {sum(len(s['takes']) for s in sounds)} takes")


if __name__ == "__main__":
    main()
