"""The HOME Menu banner's sound (Alpha 1 WP11): a knock, a crack, the hatch and a baby's
trill, mixed from the game's own processed sound effects (romfs/sfx) into a clip under three
seconds, the banner's limit. Writes assets/audio/banner.wav (16-bit mono).

  python tools/audio/make_banner_sound.py
"""
from __future__ import annotations

import array
import math
import os
import wave

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
SFX = os.path.join(ROOT, "romfs", "sfx")
OUT = os.path.join(ROOT, "assets", "audio", "banner.wav")
RATE = 22050
LENGTH = 2.8  # seconds

# (sound, start in seconds, gain, pitch)
CUES = [
    ("egg-knock.wav", 0.00, 0.8, 1.0),
    ("egg-knock-2.wav", 0.42, 0.9, 1.08),
    ("egg-crack-2.wav", 0.95, 0.9, 1.0),
    ("egg-hatch.wav", 1.25, 1.0, 1.0),
    ("dragon-trill.wav", 1.85, 0.9, 1.35),  # a baby's voice, as the game pitches it
]


def load(name: str) -> list[float]:
    with wave.open(os.path.join(SFX, name)) as w:
        assert w.getsampwidth() == 2 and w.getnchannels() == 1 and w.getframerate() == RATE, name
        data = array.array("h", w.readframes(w.getnframes()))
    return [s / 32768.0 for s in data]


def resample(x: list[float], pitch: float) -> list[float]:
    """Plays it `pitch` times faster (linear interpolation), as the 3DS does."""
    n = int(len(x) / pitch)
    out = []
    for i in range(n):
        p = i * pitch
        j = int(p)
        f = p - j
        a = x[j]
        b = x[j + 1] if j + 1 < len(x) else 0.0
        out.append(a + (b - a) * f)
    return out


def main() -> None:
    mix = [0.0] * int(LENGTH * RATE)
    for name, start, gain, pitch in CUES:
        clip = resample(load(name), pitch)
        at = int(start * RATE)
        for i, s in enumerate(clip):
            if at + i < len(mix):
                mix[at + i] += s * gain
    fade = int(0.25 * RATE)  # a soft tail
    for i in range(fade):
        mix[len(mix) - fade + i] *= 1.0 - i / fade
    peak = max(abs(s) for s in mix) or 1.0
    k = min(1.0, 0.89 / peak)  # about -1 dBFS at most
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with wave.open(OUT, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(array.array("h", (int(max(-1.0, min(1.0, s * k)) * 32767) for s in mix)).tobytes())
    print(f"wrote {OUT}: {LENGTH:.1f} s, peak {20 * math.log10(peak * k):.1f} dBFS")


if __name__ == "__main__":
    main()
