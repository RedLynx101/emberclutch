"""The trailer's voice samples (docs/plan/trailer-script.md): each candidate reads the same two lines, with
eleven_v4, into build/film/voices/<slug>.mp3. Nothing is used until Noah picks (docs/plan/trailer.md).

    py -3.12 tools/film/voice_samples.py
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "build", "film", "voices")
TEXT = ("In a valley above the clouds, every keeper starts with an egg. Keep it warm, and one morning... "
        "someone says hello. ... Nothing ever dies here. A forgotten dragon only sulks, until you make up. "
        "Emberclutch: Skyreach Valley. Free, for the Nintendo 3DS.")
VOICES = [  # (slug, voice id, what it is)
    ("george", "JBFqnCBsd6RMkjVDRZzb", "George: a warm, captivating storyteller (British, middle-aged)"),
    ("ana-rita", "wJqPPQ618aTW29mptyoc", "Ana-Rita: soft and young, for stories (British)"),
    ("jessica", "cgSgspJ2msm6clMCkdW9", "Jessica: playful, bright and warm (American, young)"),
    ("grandpa", "NOpBlnGInO9m6vDvFkFC", "Grandpa Spuds Oxley: gentle and old, like Rowan (American)"),
    ("lily", "pFZP5JQG7iQjIQuC4Bku", "Lily: a velvety actress (British, middle-aged)"),
    ("brian", "nPczCjzI2devNBz1zQrb", "Brian: deep, resonant and comforting (American)"),
    ("noah", "mM39KHzWgXV23xrAfH9Z", "Noah Hicks: your own cloned voice (the maker narrating)"),
]


def main() -> None:
    os.makedirs(OUT, exist_ok=True)
    eleven = os.path.join(ROOT, "tools", "film", "eleven.py")
    for slug, voice, _ in VOICES:
        out = os.path.join(OUT, slug + ".mp3")
        if os.path.exists(out) and "--again" not in sys.argv:
            continue
        subprocess.run([sys.executable, eleven, "say", voice, out, TEXT], check=True)


if __name__ == "__main__":
    main()
