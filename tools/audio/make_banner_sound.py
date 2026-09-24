"""The HOME Menu banner's sound: what plays when the Emberclutch icon is selected. Writes
assets/audio/banner.wav (16-bit mono, under three seconds, the banner's limit).

Noah asked for "a cuter sound" (first 3DS run, 2026-09-24). A bar of the title theme was tried
first (--theme); on the 3DS he still wanted a cute sound effect (run 7), so the default is a
little mix of the game's own sounds: a sparkle (the banner's sparkles), a baby's chirp and
trill, pitched up, and a soft sparkle to end ("sparkle-chirp"). The other mixes in MIXES and
the theme's bars stay for comparing.

  python tools/audio/make_banner_sound.py [--mix <name>] [--theme [--start <seconds>]] [--candidates]

--candidates also writes every mix and every bar in THEME_BARS to build/review/banner-sound/.
The theme needs ffmpeg on PATH (to decode the Ogg Vorbis).
"""
from __future__ import annotations

import array
import math
import os
import shutil
import subprocess
import sys
import wave

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
SFX = os.path.join(ROOT, "romfs", "sfx")
THEME = os.path.join(ROOT, "romfs", "music", "title-theme.ogg")
OUT = os.path.join(ROOT, "assets", "audio", "banner.wav")
REVIEW = os.path.join(ROOT, "build", "review", "banner-sound")
RATE = 22050
LENGTH = 2.8  # seconds

# The title theme's bars worth hearing (seconds; from romfs/music/loops.json: the loop starts
# at 12.55 s, a bar is 4 x 60 / 88.99 s). Picked by measuring each bar's loudness, brightness
# (1.5-6 kHz share) and how busy it is: 12.55 the theme's first bar after the intro, 15.25 the
# brightest early bar (the default), 36.82 bright but gentler.
THEME_BARS = {"loop-start": 12.55, "bright": 15.25, "gentle": 36.82}
THEME_START = THEME_BARS["bright"]

# The effect mixes: (sound, start in seconds, gain, pitch), and each one's length.
MIXES = {
    "sparkle-chirp": ([  # the default: a sparkle, a baby's hello, a soft sparkle
        ("polish-sparkle.wav", 0.00, 0.8, 1.1),
        ("dragon-chirp.wav", 0.28, 1.0, 1.35),
        ("dragon-trill.wav", 0.78, 0.8, 1.45),
        ("polish-sparkle-2.wav", 1.45, 0.45, 1.2),
    ], 2.4),
    "chirp-chirp": ([  # two quick chirps, a sparkle
        ("dragon-chirp.wav", 0.00, 1.0, 1.35),
        ("dragon-chirp-2.wav", 0.42, 0.9, 1.55),
        ("polish-sparkle.wav", 0.85, 0.55, 1.2),
    ], 1.9),
    "hello": ([  # a chime, a trill, a sparkle
        ("ui-confirm.wav", 0.00, 0.6, 1.0),
        ("dragon-trill.wav", 0.30, 1.0, 1.45),
        ("polish-sparkle-2.wav", 1.10, 0.5, 1.15),
    ], 2.0),
    "alpha1": ([  # Alpha 1's: a knock, a crack, the hatch, a baby's trill
        ("egg-knock.wav", 0.00, 0.8, 1.0),
        ("egg-knock-2.wav", 0.42, 0.9, 1.08),
        ("egg-crack-2.wav", 0.95, 0.9, 1.0),
        ("egg-hatch.wav", 1.25, 1.0, 1.0),
        ("dragon-trill.wav", 1.85, 0.9, 1.35),  # a baby's voice, as the game pitches it
    ], LENGTH),
}
DEFAULT_MIX = "sparkle-chirp"


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


def sfx_mix(name: str = DEFAULT_MIX) -> list[float]:
    cues, length = MIXES[name]
    mix = [0.0] * int(length * RATE)
    for name, start, gain, pitch in cues:
        clip = resample(load(name), pitch)
        at = int(start * RATE)
        for i, s in enumerate(clip):
            if at + i < len(mix):
                mix[at + i] += s * gain
    fade = int(0.25 * RATE)  # a soft tail
    for i in range(fade):
        mix[len(mix) - fade + i] *= 1.0 - i / fade
    return mix


def theme_bar(start: float) -> list[float]:
    """LENGTH seconds of the title theme from `start`, mono: in over 20 ms, out over the last 0.5 s."""
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        sys.exit("ffmpeg is needed on PATH to decode the title theme")
    raw = subprocess.run([ffmpeg, "-v", "error", "-ss", f"{start:.3f}", "-t", f"{LENGTH:.3f}", "-i", THEME,
                          "-ac", "1", "-ar", str(RATE), "-f", "s16le", "-"], check=True, capture_output=True).stdout
    x = [s / 32768.0 for s in array.array("h", raw)]
    fade_in, fade_out = int(0.02 * RATE), int(0.5 * RATE)
    for i in range(min(fade_in, len(x))):
        x[i] *= i / fade_in
    for i in range(min(fade_out, len(x))):
        x[len(x) - 1 - i] *= math.sin(0.5 * math.pi * i / fade_out)  # a gentle curve to silence
    return x


def write(path: str, mix: list[float]) -> None:
    peak = max(abs(s) for s in mix) or 1.0
    k = 0.89 / peak  # about -1 dBFS at the peak
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(array.array("h", (int(max(-1.0, min(1.0, s * k)) * 32767) for s in mix)).tobytes())
    print(f"wrote {path}: {len(mix) / RATE:.1f} s, peak {20 * math.log10(peak * k):.1f} dBFS")


def main() -> None:
    args = sys.argv[1:]
    if "--candidates" in args:
        for name in MIXES:
            write(os.path.join(REVIEW, f"sfx-{name}.wav"), sfx_mix(name))
        for name, t in THEME_BARS.items():
            write(os.path.join(REVIEW, f"theme-{name}-{t:.2f}s.wav"), theme_bar(t))
    if "--theme" in args:
        write(OUT, theme_bar(float(args[args.index("--start") + 1]) if "--start" in args else THEME_START))
    else:
        write(OUT, sfx_mix(args[args.index("--mix") + 1] if "--mix" in args else DEFAULT_MIX))


if __name__ == "__main__":
    main()
