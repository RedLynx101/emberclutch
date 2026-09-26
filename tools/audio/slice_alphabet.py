#!/usr/bin/env python3
"""Slice the villagers' voice alphabets (sound brief 3, D75: lines voiced letter by letter, in the
way of a cozy life-sim) into one short clip a letter: romfs/voice/v<N>/<letter>.wav.

  python tools/audio/slice_alphabet.py

Sources: assets/audio/voice/source/voice-alphabet.mp3 (v1) and voice-alphabet-2.mp3 (v2), the
26 letters A to Z said one at a time (not in git: Noah's recordings). The loudness envelope is
cut at the quiet between letters, the threshold swept until exactly 26 letters stand out (the
first voice speaks fast, with little quiet between). Each letter is trimmed, faded, levelled and
written as 22,050 Hz 16-bit mono; the game plays them quick and high-pitched, one per letter
shown, pitched per speaker. Needs Python 3 and ffmpeg.
"""
import array
import math
import os
import subprocess
import sys
import wave

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(ROOT, "assets", "audio", "voice", "source")
OUT = os.path.join(ROOT, "romfs", "voice")
RATE = 22050
VOICES = [("v1", "voice-alphabet.mp3"), ("v2", "voice-alphabet-2.mp3")]


def decode(path):
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-af", f"highpass=f=90,aresample={RATE}", "-ac", "1",
                          "-f", "f32le", "-acodec", "pcm_f32le", "-"], check=True, stdout=subprocess.PIPE).stdout
    return array.array("f", raw)


def envelope(x, win):
    return [math.sqrt(sum(v * v for v in x[i:i + win]) / win) for i in range(0, len(x) - win, win)]


def regions(env, thresh, merge):
    out, start = [], None
    for i, e in enumerate(env):
        if e > thresh and start is None:
            start = i
        elif e <= thresh and start is not None:
            out.append([start, i])
            start = None
    if start is not None:
        out.append([start, len(env)])
    merged = []
    for r in out:  # a dip inside a letter ("W") isn't the gap between two
        if merged and r[0] - merged[-1][1] < merge:
            merged[-1][1] = r[1]
        else:
            merged.append(r)
    return [r for r in merged if r[1] - r[0] >= 3]  # not clicks


def slice_voice(x):
    win = RATE // 100  # 10 ms
    env = envelope(x, win)
    peak = max(env)
    for merge in (6, 4, 3, 2):
        for k in range(200):
            t = peak * (0.5 - 0.0024 * k)
            if t <= 0:
                break
            r = regions(env, t, merge)
            if len(r) == 26:
                return [(a * win, b * win) for a, b in r]
    sys.exit("error: couldn't find 26 letters")


def write(path, y):
    pcm = array.array("h", (max(-32767, min(32767, int(v * 32767))) for v in y))
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(pcm.tobytes())


def main():
    for name, src in VOICES:
        path = os.path.join(SRC, src)
        if not os.path.exists(path):
            print(f"[voice] {src} missing: skipped")
            continue
        x = decode(path)
        cuts = slice_voice(x)
        os.makedirs(os.path.join(OUT, name), exist_ok=True)
        lengths = []
        for i, (a, b) in enumerate(cuts):
            pad = RATE // 100
            y = x[max(0, a - pad):min(len(x), b + 2 * pad)]
            y = y[:int(RATE * 0.42)]  # a letter's worth: the game plays only the start, sped up
            fade = min(len(y) // 4, RATE // 80)
            for k in range(fade):
                y[k] *= k / fade
                y[-1 - k] *= k / fade
            peak = max(abs(v) for v in y) or 1.0
            y = array.array("f", (v * 0.7 / peak for v in y))
            write(os.path.join(OUT, name, f"{chr(97 + i)}.wav"), y)
            lengths.append(len(y) / RATE)
        print(f"[voice] {name}: 26 letters, {min(lengths):.2f}-{max(lengths):.2f} s")


if __name__ == "__main__":
    main()
