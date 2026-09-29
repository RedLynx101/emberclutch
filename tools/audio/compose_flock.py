#!/usr/bin/env python3
"""A flock bursting up, made of the songbirds' own chirps (Noah on the review page, 2026-09-29:
"Just use overlapping and varied bird-chirp sounds"): romfs/sfx/bird-flutter.wav and -2.wav from
bird-chirp's takes, each a handful of chirps overlapping, pitched up and down a little, fading off
as the flock flies away. Levelled as process_sfx levels its "care" kind.

  python tools/audio/compose_flock.py
"""
from __future__ import annotations

import array
import math
import random
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SFX = ROOT / "romfs" / "sfx"
RATE = 22050
TARGET_DB, CEILING_DB = -16.0, -1.0


def read(path: Path) -> list[float]:
    with wave.open(str(path)) as w:
        assert w.getframerate() == RATE and w.getnchannels() == 1 and w.getsampwidth() == 2
        pcm = array.array("h", w.readframes(w.getnframes()))
    return [v / 32767.0 for v in pcm]


def write(path: Path, x: list[float]) -> None:
    pcm = array.array("h", (max(-32767, min(32767, int(round(v * 32767)))) for v in x))
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(pcm.tobytes())


def repitch(x: list[float], factor: float) -> list[float]:
    """Played `factor` times faster (higher and shorter), linear between samples."""
    n = int(len(x) / factor)
    out = []
    for i in range(n):
        p = i * factor
        j = int(p)
        f = p - j
        a = x[j]
        b = x[j + 1] if j + 1 < len(x) else 0.0
        out.append(a + (b - a) * f)
    return out


def loudest_db(x: list[float]) -> float:
    win = int(0.1 * RATE)
    best = 1e-9
    for start in range(0, max(1, len(x) - win), win // 4):
        seg = x[start:start + win]
        best = max(best, math.sqrt(sum(v * v for v in seg) / max(1, len(seg))))
    return 20 * math.log10(best)


def flock(chirps: list[list[float]], seed: int) -> list[float]:
    rng = random.Random(seed)
    length = int(1.0 * RATE)
    out = [0.0] * length
    count = rng.randint(6, 8)
    for k in range(count):
        at = int(rng.uniform(0.0, 0.5) * RATE * (0.4 + 0.6 * k / count))
        c = repitch(rng.choice(chirps), rng.uniform(0.88, 1.38))
        gain = (1.0 - 0.55 * k / count) * rng.uniform(0.7, 1.0)  # (further off as they go)
        for i, v in enumerate(c):
            if at + i < length:
                out[at + i] += v * gain
    fade = int(0.25 * RATE)
    for i in range(fade):
        out[length - fade + i] *= 1.0 - i / fade
    end = max((i for i, v in enumerate(out) if abs(v) > 1e-3), default=length - 1) + 1
    out = out[:end]
    gain_db = min(TARGET_DB - loudest_db(out), CEILING_DB - 20 * math.log10(max(abs(v) for v in out)))
    g = 10 ** (gain_db / 20)
    return [v * g for v in out]


def main() -> None:
    sources = sorted(p for p in SFX.glob("bird-chirp*.wav"))
    chirps = [read(p) for p in sources]
    for take, seed in ((1, 11),):  # (Noah kept take 1, 2026-09-29)
        x = flock(chirps, seed)
        out = SFX / ("bird-flutter.wav" if take == 1 else f"bird-flutter-{take}.wav")
        write(out, x)
        print(f"[flock] {out.name}: {len(x) / RATE:.2f} s from {len(chirps)} chirps")


if __name__ == "__main__":
    main()
