#!/usr/bin/env python3
"""Turn a Suno WAV into a small, seamlessly looping Ogg for the 3DS.

  python tools/audio/make_loop.py assets/audio/music/source/den-hearth.wav --bpm 76

What it does (see docs/audio/suno-music-brief.md):
  1. Finds the first downbeat (first non-silent audio) and where the fade-out begins.
  2. Picks a loop [start, end) on bar lines: after the intro, before the fade.
  3. Blends the seam: the last `--xfade-bars` of the loop crossfade into the audio just
     before the loop start, and the outgoing side is muffled with a low-pass as it fades.
  4. Levels to --lufs (default -16 LUFS) with a true-peak ceiling of -1 dBTP.
  5. Encodes 32 kHz Ogg Vorbis with LOOPSTART / LOOPLENGTH tags (in samples) and updates
     romfs/music/loops.json. The intro plays once; the game loops [LOOPSTART, end).

Only needs Python 3 and ffmpeg/ffprobe on PATH (or FFMPEG_DIR set).
"""
from __future__ import annotations

import argparse
import array
import json
import math
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ANALYSIS_RATE = 8000
WINDOW_S = 0.05


def find_tool(name: str) -> str:
    env_dir = os.environ.get("FFMPEG_DIR")
    if env_dir:
        candidate = Path(env_dir) / (name + (".exe" if os.name == "nt" else ""))
        if candidate.exists():
            return str(candidate)
    found = shutil.which(name)
    if not found:
        sys.exit(f"error: {name} not found. Install ffmpeg or set FFMPEG_DIR.")
    return found


FFMPEG = find_tool("ffmpeg")
FFPROBE = find_tool("ffprobe")


def run(args: list[str], capture_stderr: bool = False) -> subprocess.CompletedProcess:
    return subprocess.run(args, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE if capture_stderr else None)


def duration_of(path: Path) -> float:
    out = run([FFPROBE, "-v", "error", "-show_entries", "format=duration", "-of", "csv=p=0", str(path)]).stdout
    return float(out.decode().strip())


def rms_envelope(path: Path) -> list[float]:
    """Mono RMS (dBFS) per 50 ms window."""
    raw = run([FFMPEG, "-v", "error", "-i", str(path), "-ac", "1", "-ar", str(ANALYSIS_RATE), "-f", "s16le", "-"]).stdout
    samples = array.array("h")
    samples.frombytes(raw[: len(raw) // 2 * 2])
    if sys.byteorder == "big":
        samples.byteswap()
    n = int(ANALYSIS_RATE * WINDOW_S)
    env = []
    for i in range(0, len(samples) - n + 1, n):
        chunk = samples[i : i + n]
        mean_sq = sum(s * s for s in chunk) / n
        env.append(10 * math.log10(mean_sq / (32768.0**2)) if mean_sq > 0 else -120.0)
    return env


def detect_bounds(env: list[float]) -> tuple[float, float]:
    """(first sound, start of fade-out) in seconds."""
    loud = sorted(env)[len(env) // 2]  # median level of the track
    first = next((i for i, db in enumerate(env) if db > loud - 30), 0)
    # Smooth over a centred 2 s window and find the last point within 3 dB of the median.
    k = int(1.0 / WINDOW_S)
    smooth = []
    for i in range(len(env)):
        lo, hi = max(0, i - k), min(len(env), i + k + 1)
        smooth.append(sum(env[lo:hi]) / (hi - lo))
    last = max((i for i, db in enumerate(smooth) if db > loud - 3), default=len(env) - 1)
    return first * WINDOW_S, last * WINDOW_S


def measure_loudness(path: Path, filtergraph: str) -> dict:
    proc = run(
        [FFMPEG, "-hide_banner", "-nostats", "-i", str(path), "-filter_complex",
         filtergraph + ";[out]loudnorm=print_format=json[m]", "-map", "[m]", "-f", "null", "-"],
        capture_stderr=True,
    )
    match = re.search(r"\{[^{}]*\"input_i\"[^{}]*\}", proc.stderr.decode(errors="replace"))
    if not match:
        sys.exit("error: could not measure loudness")
    return json.loads(match.group(0))


def build_filtergraph(start: float, end: float, xfade: float, muffle_hz: int) -> str:
    """Loop body [0, end) whose last `xfade` seconds blend into the audio just before `start`."""
    body_end = end - xfade
    pre = start - xfade
    dry_fade = xfade * 0.6
    return (
        f"[0:a]asplit=3[a][b][c];"
        f"[a]atrim=0:{body_end:.6f},asetpts=PTS-STARTPTS[head];"
        # Outgoing tail: the dry signal gives way to a muffled (low-passed) copy.
        f"[b]atrim={body_end:.6f}:{end:.6f},asetpts=PTS-STARTPTS,asplit=2[td][tw];"
        f"[td]afade=t=out:st=0:d={dry_fade:.6f}[tdry];"
        f"[tw]lowpass=f={muffle_hz},lowpass=f={muffle_hz},afade=t=in:st=0:d={dry_fade:.6f}[twet];"
        f"[tdry][twet]amix=inputs=2:normalize=0[tail];"
        f"[head][tail]concat=n=2:v=0:a=1[body];"
        # Incoming: the audio leading into the loop start, so playback lands exactly on it.
        f"[c]atrim={pre:.6f}:{start:.6f},asetpts=PTS-STARTPTS[pre];"
        f"[body][pre]acrossfade=d={xfade:.6f}:c1=qsin:c2=qsin[out]"
    )


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input", type=Path)
    ap.add_argument("--slug", help="output name (default: input file name)")
    ap.add_argument("--bpm", type=float, required=True)
    ap.add_argument("--beats-per-bar", type=int, default=4)
    ap.add_argument("--first-beat", type=float, help="seconds of the first downbeat (default: auto)")
    ap.add_argument("--start-bar", type=int, default=4, help="loop starts this many bars in (skips the intro)")
    ap.add_argument("--end-bar", type=int, help="loop ends at this bar (default: last full bar before the fade)")
    ap.add_argument("--xfade-bars", type=float, default=1.0, help="length of the seam blend in bars (max 3 s)")
    ap.add_argument("--muffle-hz", type=int, default=1400, help="low-pass cutoff for the outgoing tail")
    ap.add_argument("--lufs", type=float, default=-16.0)
    ap.add_argument("--rate", type=int, default=32000)
    ap.add_argument("--quality", type=float, default=3.0, help="Vorbis quality (3 = about 96 kbps)")
    ap.add_argument("--mono", action="store_true", help="downmix to mono (halves the size)")
    ap.add_argument("--out-dir", type=Path, default=ROOT / "romfs" / "music")
    ap.add_argument("--preview", action="store_true", help="also write a WAV of the seam (5 s either side)")
    args = ap.parse_args()

    src: Path = args.input
    slug = args.slug or src.stem
    total = duration_of(src)
    env = rms_envelope(src)
    first_sound, fade_start = detect_bounds(env)
    first_beat = args.first_beat if args.first_beat is not None else first_sound
    bar = args.beats_per_bar * 60.0 / args.bpm

    start = first_beat + args.start_bar * bar
    # Two bars of safety before the detected fade.
    end_bar = args.end_bar if args.end_bar is not None else int((min(fade_start, total) - first_beat) / bar) - 2
    end = first_beat + end_bar * bar
    xfade = min(args.xfade_bars * bar, 3.0, start)
    if end - start < 4 * bar:
        sys.exit(f"error: loop too short ({end - start:.1f}s). Check --bpm/--start-bar/--end-bar.")

    graph = build_filtergraph(start, end, xfade, args.muffle_hz)
    loud = measure_loudness(src, graph)
    gain = args.lufs - float(loud["input_i"])
    peak_after = float(loud["input_tp"]) + gain
    if peak_after > -1.0:
        gain -= peak_after + 1.0

    loop_start = round(start * args.rate)
    loop_length = round(end * args.rate) - loop_start
    args.out_dir.mkdir(parents=True, exist_ok=True)
    out = args.out_dir / f"{slug}.ogg"
    channels = ["-ac", "1"] if args.mono else ["-ac", "2"]
    run([
        FFMPEG, "-v", "error", "-y", "-i", str(src), "-filter_complex",
        graph + f";[out]volume={gain:.2f}dB,aresample={args.rate}[final]", "-map", "[final]",
        *channels, "-c:a", "libvorbis", "-q:a", str(args.quality),
        "-metadata", f"LOOPSTART={loop_start}", "-metadata", f"LOOPLENGTH={loop_length}",
        "-metadata", f"TITLE={slug}", str(out),
    ])

    manifest_path = args.out_dir / "loops.json"
    manifest = json.loads(manifest_path.read_text()) if manifest_path.exists() else {}
    manifest[slug] = {
        "file": f"{slug}.ogg", "rate": args.rate, "loopStart": loop_start, "loopLength": loop_length,
        "bpm": args.bpm, "lufs": args.lufs,
    }
    manifest_path.write_text(json.dumps(dict(sorted(manifest.items())), indent=2) + "\n")

    if args.preview:
        preview = out.with_suffix(".seam-preview.wav")
        seam = end
        # Play 5 s before the loop end, then jump to the loop start: exactly what the game does.
        run([
            FFMPEG, "-v", "error", "-y", "-i", str(out), "-filter_complex",
            f"[0:a]asplit=2[x][y];[x]atrim={seam - 5:.6f}:{seam:.6f},asetpts=PTS-STARTPTS[p1];"
            f"[y]atrim={start:.6f}:{start + 5:.6f},asetpts=PTS-STARTPTS[p2];[p1][p2]concat=n=2:v=0:a=1[o]",
            "-map", "[o]", str(preview),
        ])
        print(f"seam preview: {preview}")

    src_mb = src.stat().st_size / 1e6
    out_mb = out.stat().st_size / 1e6
    print(f"{slug}: bar={bar:.3f}s first-beat={first_beat:.2f}s loop={start:.2f}-{end:.2f}s "
          f"({(end - start):.1f}s, xfade {xfade:.2f}s) gain={gain:+.1f}dB")
    print(f"  {src_mb:.1f} MB -> {out_mb:.2f} MB  ({out})")


if __name__ == "__main__":
    main()
