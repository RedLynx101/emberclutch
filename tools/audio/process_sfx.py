#!/usr/bin/env python3
"""Turn the generated sound effects into the game's small WAVs: romfs/sfx/<slug>[-N].wav.

  python tools/audio/process_sfx.py --import C:/Users/me/Downloads/batch [slug ...]   (copy + rename the takes)
  python tools/audio/process_sfx.py                                         (process every slug)
  python tools/audio/process_sfx.py dragon-chirp ui-tap                     (just these)

Sources live in assets/audio/sfx/source/<slug>-<take>.wav (not in git). The manifest,
tools/audio/sfx_manifest.json, records which generated file became which take, so
--import can rebuild the source folder from a download folder.

Per take (docs/audio/suno-sfx-alpha1.md):
  1. mono, EQ for the 3DS's small speakers: a high-pass by kind (voices lose rumble they
     can't play; the grown-up rumble keeps its lows) and a gentle low-shelf cut;
  2. trimmed to the sound: the generator pads every clip to a fixed length, so leading and
     trailing silence goes (a short fade at each cut);
  3. levelled by kind: the loudest 100 ms reaches the kind's target, so takes and sounds
     of a kind match, with peaks under -1 dBFS. Sharp sounds (knocks, cracks, steps) would
     hit that ceiling first, so their peaks go through a soft limiter (up to `limit` dB);
  4. written as 22,050 Hz 16-bit mono.
Loops (den ambience, the egg's hum) become seamless: the audio just past the loop's end
crossfades (equal power) into its start, and they are levelled by overall RMS.
Needs Python 3 and ffmpeg.
"""
from __future__ import annotations

import argparse
import array
import json
import math
import shutil
import subprocess
import sys
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = Path(__file__).with_name("sfx_manifest.json")
SOURCE_DIR = ROOT / "assets" / "audio" / "sfx" / "source"
OUT_DIR = ROOT / "romfs" / "sfx"
RATE = 22050

# hp: high-pass Hz; shelf: low-shelf gain (dB) below 200 Hz; target: loudest-100 ms RMS
# (dBFS); floor: where a sound ends, relative to its peak (dB); tail: kept after that (s);
# limit: how far (dB) peaks may be pushed into the soft limiter. A manifest entry may
# override floor / tail and cap the length with "max" (s), and soften a sound: "lp" (a low-pass,
# Hz: takes the click off a step), "gain" (dB against the kind's target) and "attack" (s: a
# fade-in over the first hit).
KINDS = {
    "voice": dict(hp=110, shelf=-3.0, target=-14.0, floor=-40.0, tail=0.06, limit=0.0),
    "low-voice": dict(hp=50, shelf=0.0, target=-14.0, floor=-40.0, tail=0.08, limit=0.0),
    "body": dict(hp=70, shelf=-2.0, target=-17.0, floor=-38.0, tail=0.04, limit=6.0),
    "egg": dict(hp=80, shelf=-2.0, target=-15.0, floor=-40.0, tail=0.06, limit=6.0),
    "care": dict(hp=90, shelf=-2.0, target=-16.0, floor=-40.0, tail=0.06, limit=6.0),
    "ui": dict(hp=150, shelf=-3.0, target=-19.0, floor=-48.0, tail=0.08, limit=0.0),
    "loop": dict(hp=50, shelf=0.0, rms=-26.0, limit=0.0),
}
PEAK_CEILING = -1.0
KNEE = 0.5  # the soft limiter starts at -6 dBFS and approaches the ceiling


def manifest() -> dict:
    return {k: v for k, v in json.loads(MANIFEST.read_text(encoding="utf-8")).items() if not k.startswith("_")}


def import_takes(src_dir: Path, slugs: dict) -> None:
    SOURCE_DIR.mkdir(parents=True, exist_ok=True)
    for slug, entry in slugs.items():
        for i, name in enumerate(entry["takes"], 1):
            src = src_dir / name
            if not src.exists():
                sys.exit(f"error: {src} not found")
            shutil.copyfile(src, SOURCE_DIR / f"{slug}-{i}.wav")
    print(f"[sfx] imported {sum(len(e['takes']) for e in slugs.values())} takes into {SOURCE_DIR}")


def decode(path: Path, kind: dict, lp: float | None = None) -> array.array:
    chain = [f"pan=mono|c0=0.5*c0+0.5*c1", f"highpass=f={kind['hp']}:poles=2"]
    if kind["shelf"]:
        chain.append(f"lowshelf=f=200:g={kind['shelf']}")
    if lp:
        chain.append(f"lowpass=f={lp}:poles=2")
    chain.append(f"aresample={RATE}")
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", str(path), "-af", ",".join(chain), "-f", "f32le",
                          "-acodec", "pcm_f32le", "-"], check=True, stdout=subprocess.PIPE).stdout
    return array.array("f", raw)


def db(x: float) -> float:
    return 20 * math.log10(max(x, 1e-9))


def trim(x: array.array, floor_db: float, tail: float, longest: float | None = None) -> array.array:
    block = RATE // 400  # 2.5 ms
    env = [max(abs(v) for v in x[i:i + block]) for i in range(0, len(x), block)]
    peak = max(env)
    on = [i for i, e in enumerate(env) if e > peak * 10 ** (floor_db / 20)]
    start = max(0, on[0] * block - int(0.008 * RATE))
    end = min(len(x), (on[-1] + 1) * block + int(tail * RATE))
    if longest:
        end = min(end, start + int(longest * RATE))
    y = array.array("f", x[start:end])
    fade_in = int(0.004 * RATE) if start > 0 else 0
    for i in range(fade_in):
        y[i] *= i / fade_in
    fade_out = min(int(0.04 * RATE), len(y) // 4)
    for i in range(fade_out):
        y[len(y) - 1 - i] *= 0.5 - 0.5 * math.cos(math.pi * i / fade_out)
    return y


def loop_cut(x: array.array, start: float, length: float, xfade: float) -> array.array:
    """x[a, a+L) where the first xf samples crossfade from x[a+L...] (the audio that follows
    the loop's end) into x[a...], so the end runs straight into the start."""
    a, n, xf = int(start * RATE), int(length * RATE), int(xfade * RATE)
    if a + n + xf > len(x):
        sys.exit(f"error: loop {start}+{length}+{xfade} s runs past the source ({len(x) / RATE:.2f} s)")
    y = array.array("f", x[a:a + n])
    for i in range(xf):
        u = i / xf
        y[i] = x[a + i] * math.sqrt(u) + x[a + n + i] * math.sqrt(1 - u)
    return y


def loudest_rms(x: array.array) -> float:
    win = int(0.1 * RATE)
    sq = [0.0]
    for v in x:
        sq.append(sq[-1] + v * v)
    if len(x) <= win:
        return math.sqrt(sq[-1] / max(1, len(x)))
    return math.sqrt(max(sq[i + win] - sq[i] for i in range(0, len(x) - win, win // 4)) / win)


def rms(x: array.array) -> float:
    return math.sqrt(sum(v * v for v in x) / max(1, len(x)))


def soft_limit(x: array.array, gain: float) -> array.array:
    """Scale by gain; peaks above the knee bend smoothly toward the ceiling."""
    c = 10 ** (PEAK_CEILING / 20)
    y = array.array("f", x)
    for i, v in enumerate(y):
        a = abs(v * gain)
        if a > KNEE:
            a = KNEE + (c - KNEE) * math.tanh((a - KNEE) / (c - KNEE))
        y[i] = math.copysign(a, v)
    return y


def write_wav(path: Path, x: array.array) -> None:
    pcm = array.array("h", (max(-32767, min(32767, int(round(v * 32767)))) for v in x))
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(pcm.tobytes())


def process(slug: str, entry: dict) -> list[str]:
    kind = KINDS[entry["kind"]]
    lines = []
    for i in range(1, len(entry["takes"]) + 1):
        src = SOURCE_DIR / f"{slug}-{i}.wav"
        if not src.exists():
            sys.exit(f"error: {src} missing (run with --import <download folder>)")
        x = decode(src, kind, entry.get("lp"))
        if entry["kind"] == "loop":
            y = loop_cut(x, **entry["loop"])
            level, target = rms(y), kind["rms"]
        else:
            y = trim(x, entry.get("floor", kind["floor"]), entry.get("tail", kind["tail"]), entry.get("max"))
            level, target = loudest_rms(y), kind["target"] + entry.get("gain", 0.0)
            if entry.get("attack"):  # soften the first hit
                n = max(1, int(entry["attack"] * RATE))
                for k in range(min(n, len(y))):
                    y[k] *= (k / n) ** 0.5
        peak = max(abs(v) for v in y)
        gain_db = min(target - db(level), PEAK_CEILING + kind["limit"] - db(peak))
        if kind["limit"]:
            y = soft_limit(y, 10 ** (gain_db / 20))
        else:
            y = array.array("f", (v * 10 ** (gain_db / 20) for v in y))
        out = OUT_DIR / (f"{slug}.wav" if i == 1 else f"{slug}-{i}.wav")
        write_wav(out, y)
        measure = rms(y) if entry["kind"] == "loop" else loudest_rms(y)
        lines.append(f"[sfx] {out.name:22s} {len(y) / RATE:5.2f} s  level {db(measure):6.1f} dB"
                     f"  peak {db(max(abs(v) for v in y)):5.1f} dBFS  gain {gain_db:+5.1f} dB")
    return lines


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("slugs", nargs="*", help="only these (default: all)")
    ap.add_argument("--import", dest="import_dir", type=Path, help="copy the manifest's takes from this folder first")
    args = ap.parse_args()
    slugs = manifest()
    if args.import_dir:  # just the named slugs' takes, if any are named (a batch's own folder)
        import_takes(args.import_dir, {k: v for k, v in slugs.items() if not args.slugs or k in args.slugs})
    wanted = args.slugs or list(slugs)
    unknown = [s for s in wanted if s not in slugs]
    if unknown:
        sys.exit(f"error: unknown slug(s) {unknown}")
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for slug in wanted:
        for line in process(slug, slugs[slug]):
            print(line)
    total = sum(p.stat().st_size for p in OUT_DIR.glob("*.wav"))
    print(f"[sfx] romfs/sfx: {total / 1024:.0f} KB")


if __name__ == "__main__":
    main()
