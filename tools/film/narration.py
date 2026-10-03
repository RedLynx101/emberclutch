"""The trailer's narration (docs/plan/trailer-script.md) in Lily's voice (Noah's pick, 2026-10-02), eleven_v4.

    py -3.12 tools/film/narration.py [--takes 3] [--again]

Each passage is read whole (a sentence alone comes out flat, and passages keep one performance), N takes
of each, with character timings; each take is cut into its pieces in the silences between sentences
(build/film/narration/<piece>_t<take>.wav, 48 kHz mono). Every take is heard back (speech to text) and
measured (length, loudness, pitch); takes.json has it all, and the pick for each passage: the words right,
then the take nearest the whole read's typical pitch and pace, so the pieces sound like one sitting.
"""
import json
import os
import re
import subprocess
import sys

import numpy as np
from scipy import signal

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "build", "film", "narration")
LILY = "pFZP5JQG7iQjIQuC4Bku"
RATE = 48000
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import eleven  # noqa: E402

# (passage, [(piece, its words exactly as in the passage)])
PASSAGES = [
    ("p1", [("n01", "In a valley above the clouds, every keeper starts with an egg."),
            ("n02a", "Keep it warm, and one morning..."), ("n02b", "someone says hello.")]),
    ("p2", [("n03a", "Pet it."), ("n03b", "Feed it."), ("n03c", "Brush it."), ("n03d", "Bathe it."),
            ("n04", "Day by day, in real time, it grows.")]),
    ("p3", [("n05", "Then the door opens on Skyreach Valley."), ("n06a", "Walk it on its lead."),
            ("n06b", "Meet the villagers."), ("n07", "And when it's grown..."), ("n08", "climb on.")]),
    ("p4", [("n09", "Battle in the league."), ("n10", "Shine on the show stage."), ("n11", "Race the Sky Rings."),
            ("n12", "Or just go fishing.")]),
    ("p5", [("n13", "And when the lanterns are lit, the whole valley glows."), ("n14a", "Nothing ever dies here."),
            ("n14b", "A forgotten dragon only sulks, until you make up.")]),
    ("p6", [("n15a", "Emberclutch: Skyreach Valley."), ("n15b", "Free, for the Nintendo 3DS.")]),
]


def decode(path):
    raw = subprocess.run(["ffmpeg", "-loglevel", "error", "-i", path, "-f", "f32le", "-ac", "1", "-ar", str(RATE), "-"],
                         capture_output=True, check=True).stdout
    return np.frombuffer(raw, np.float32).copy()


def write_wav(path, a):
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-f", "f32le", "-ar", str(RATE), "-ac", "1", "-i", "-",
                    "-c:a", "pcm_s24le", path], input=a.astype(np.float32).tobytes(), check=True)


def pitch(a):
    """Median pitch (Hz) of the voiced 40 ms frames, by autocorrelation (75-500 Hz)."""
    x = signal.resample_poly(a, 1, 3)  # 16 kHz
    sr, n = 16000, 640
    f0s = []
    rms_all = np.sqrt(np.mean(x ** 2)) + 1e-9
    for s in range(0, len(x) - n, n // 2):
        fr = x[s:s + n] * np.hanning(n)
        if np.sqrt(np.mean(fr ** 2)) < 0.5 * rms_all:
            continue
        ac = np.correlate(fr, fr, "full")[n - 1:]
        lo, hi = sr // 500, sr // 75
        k = lo + int(np.argmax(ac[lo:hi]))
        if ac[k] > 0.45 * ac[0]:
            f0s.append(sr / k)
    return float(np.median(f0s)) if f0s else 0.0


def words(s):
    return re.sub(r"[^a-z0-9 ]", "", s.lower().replace("ember clutch", "emberclutch").replace("3ds", "3 d s")
                  .replace("three d s", "3 d s")).split()


def cut(audio, align, text, pieces):
    """Each piece's audio: from just before its first letter to just after its last, cut at the quietest
    10 ms of each gap between pieces."""
    chars, starts, ends = align["characters"], align["character_start_times_seconds"], align["character_end_times_seconds"]
    joined = "".join(chars)
    spans, pos = [], 0
    for _, words_ in pieces:
        i = joined.index(words_, pos)
        spans.append((starts[i], ends[i + len(words_) - 1]))
        pos = i + len(words_)
    env = np.sqrt(np.convolve(audio ** 2, np.ones(480) / 480, "same"))
    bounds = [max(0.0, spans[0][0] - 0.12)]
    for (a0, a1), (b0, b1) in zip(spans, spans[1:]):
        lo, hi = int((a1 + 0.02) * RATE), int(max(a1 + 0.03, b0 - 0.02) * RATE)
        k = lo + int(np.argmin(env[lo:hi])) if hi > lo else (lo + hi) // 2
        bounds.append(k / RATE)
    bounds.append(min(len(audio) / RATE, spans[-1][1] + 0.45))
    out = []
    for i in range(len(pieces)):
        s0, s1 = int(bounds[i] * RATE), int(bounds[i + 1] * RATE)
        seg = audio[s0:s1].copy()
        # trim the quiet ends (below -50 dB of the piece's peak), keep 60 ms and 250 ms of air
        e = np.sqrt(np.convolve(seg ** 2, np.ones(240) / 240, "same"))
        loud = np.nonzero(e > e.max() * 10 ** (-50 / 20))[0]
        if len(loud):
            seg = seg[max(0, loud[0] - int(0.06 * RATE)):min(len(seg), loud[-1] + int(0.25 * RATE))]
        f = int(0.008 * RATE)
        seg[:f] *= np.linspace(0, 1, f)
        seg[-f:] *= np.linspace(1, 0, f)
        out.append(seg)
    return out


def main():
    takes = int(sys.argv[sys.argv.index("--takes") + 1]) if "--takes" in sys.argv else 3
    os.makedirs(OUT, exist_ok=True)
    report = {}
    for pid, pieces in PASSAGES:
        text = " ".join(w for _, w in pieces)
        report[pid] = {"text": text, "takes": []}
        for t in range(1, takes + 1):
            mp3 = os.path.join(OUT, f"{pid}_take{t}.mp3")
            meta = os.path.splitext(mp3)[0] + ".json"
            if "--again" in sys.argv or not (os.path.exists(mp3) and os.path.exists(meta)):
                subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "eleven.py"),
                                "say-timed", LILY, mp3, text], check=True)
            d = json.load(open(meta))
            if d["text"] != text:  # (the script changed: read it again)
                subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "eleven.py"),
                                "say-timed", LILY, mp3, text], check=True)
                d = json.load(open(meta))
            heard_path = os.path.splitext(mp3)[0] + ".heard.txt"
            if not os.path.exists(heard_path) or os.path.getmtime(heard_path) < os.path.getmtime(mp3):
                open(heard_path, "w", encoding="utf-8").write(eleven.hear(mp3))
            heard = open(heard_path, encoding="utf-8").read().strip()
            audio = decode(mp3)
            parts = cut(audio, d["alignment"], text, pieces)
            info = {"take": t, "seconds": round(len(audio) / RATE, 2), "heard": heard,
                    "words_ok": words(heard) == words(text), "pitch": round(pitch(audio), 1),
                    "rms_db": round(20 * np.log10(np.sqrt(np.mean(audio ** 2)) + 1e-9), 1),
                    "chars_per_s": round(len(text) / (len(audio) / RATE), 1), "pieces": {}}
            for (piece, _), seg in zip(pieces, parts):
                write_wav(os.path.join(OUT, f"{piece}_t{t}.wav"), seg)
                info["pieces"][piece] = round(len(seg) / RATE, 2)
            report[pid]["takes"].append(info)
            print(f"{pid} take {t}: {info['seconds']} s, pitch {info['pitch']} Hz, {info['rms_db']} dB, "
                  f"{info['chars_per_s']} ch/s, words {'ok' if info['words_ok'] else 'DIFFER'}: {heard}")
    # The pick: words right, then nearest the read's median pitch and pace (one sitting).
    allt = [t for p in report.values() for t in p["takes"]]
    mp = np.median([t["pitch"] for t in allt])
    for pid, p in report.items():
        mr = np.median([t["chars_per_s"] for t in p["takes"]])
        ok = [t for t in p["takes"] if t["words_ok"]] or p["takes"]
        best = min(ok, key=lambda t: abs(t["pitch"] - mp) / mp + abs(t["chars_per_s"] - mr) / mr)
        p["pick"] = best["take"]
        print(f"{pid}: take {best['take']}")
    json.dump(report, open(os.path.join(OUT, "takes.json"), "w"), indent=1)


if __name__ == "__main__":
    main()
