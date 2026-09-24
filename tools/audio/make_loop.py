#!/usr/bin/env python3
"""Turn a Suno WAV into a small, seamlessly looping Ogg for the 3DS.

  python tools/audio/make_loop.py assets/audio/music/source/den-hearth.wav --bpm 76 --preview

What it does (see docs/audio/suno-music-brief.md):
  1. Estimates the tempo and downbeat (the --bpm you asked Suno for is a hint) to place
     the loop start on a bar line after the intro.
  2. Picks the loop start a few bars in (skips the intro), then searches every possible
     loop end before the fade for the spot where the music truly repeats the start:
     harmony (12 pitch classes), timbre (octave bands) and rhythm (onsets) must all
     match. This is tempo-free, so Suno's tempo drift can't knock the seam off the beat.
  3. Blends the seam: the last --xfade seconds of the loop crossfade into the audio just
     before the loop start, and the outgoing side is muffled with a low-pass as it fades.
  4. Levels to --lufs (default -16 LUFS) with a true-peak ceiling of -1 dBTP.
  5. Encodes 32 kHz Ogg Vorbis with LOOPSTART / LOOPLENGTH tags (in samples) and updates
     romfs/music/loops.json. The intro plays once; the game loops [LOOPSTART, end).

Only needs Python 3 and ffmpeg/ffprobe on PATH (or FFMPEG_DIR set).
"""
from __future__ import annotations

import argparse
import array
import itertools
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
ONSET_FPS = 100
BAND_FPS = 50
BAND_CENTRES = (60, 120, 250, 500, 1000, 2000, 4000, 8000)


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


# ---------------------------------------------------------------------------- analysis


def decode_mono(path: Path) -> array.array:
    raw = run([FFMPEG, "-v", "error", "-i", str(path), "-ac", "1", "-ar", str(ANALYSIS_RATE), "-f", "s16le", "-"]).stdout
    samples = array.array("h")
    samples.frombytes(raw[: len(raw) // 2 * 2])
    if sys.byteorder == "big":
        samples.byteswap()
    return samples


def level_and_onsets(samples: array.array) -> tuple[list[float], list[float]]:
    """Per-frame level (dBFS) and onset strength (rectified rise in log energy), at ONSET_FPS."""
    hop = ANALYSIS_RATE // ONSET_FPS
    prefix = array.array("d", itertools.accumulate((s * s for s in samples), initial=0.0))
    level, onset = [], []
    prev = None
    for i in range(0, len(samples) - 2 * hop, hop):
        e = (prefix[i + 2 * hop] - prefix[i]) / (2 * hop)
        db = 10 * math.log10(e / 32768.0**2) if e > 0 else -120.0
        level.append(db)
        onset.append(max(0.0, db - prev) if prev is not None else 0.0)
        prev = db
    return level, onset


def filterbank(path: Path, filters: list[str], in_rate: int) -> list[list[float]]:
    """Energy envelope per filter, BAND_FPS frames: [[e_filter0, e_filter1, ...], ...]."""
    n = len(filters)
    split = "".join(f"[s{i}]" for i in range(n))
    chains = ";".join(f"[s{i}]{f},aeval=val(0)*val(0),lowpass=f=8,lowpass=f=8[b{i}]" for i, f in enumerate(filters))
    graph = (f"[0:a]aresample={in_rate},pan=mono|c0=0.5*c0+0.5*c1,asplit={n}{split};{chains};"
             + "".join(f"[b{i}]" for i in range(n)) + f"amerge=inputs={n},aresample={BAND_FPS}[out]")
    raw = run([FFMPEG, "-v", "error", "-i", str(path), "-filter_complex", graph, "-map", "[out]",
               "-f", "f32le", "-c:a", "pcm_f32le", "-"]).stdout
    vals = array.array("f")
    vals.frombytes(raw[: len(raw) // (4 * n) * 4 * n])
    if sys.byteorder == "big":
        vals.byteswap()
    return [list(vals[i : i + n]) for i in range(0, len(vals), n)]


def band_profile(path: Path) -> list[list[float]]:
    """8 octave-band log energies: a coarse 'what it sounds like' (timbre) signature."""
    frames = filterbank(path, [f"bandpass=f={f}:width_type=o:w=1" for f in BAND_CENTRES], 22050)
    return [[math.log10(max(v, 1e-12)) for v in f] for f in frames]


def chroma_profile(path: Path) -> list[list[float]]:
    """12 pitch-class energies (C..B) from semitone filters over 4 octaves: the harmony signature."""
    notes = [65.406 * 2 ** (k / 12) for k in range(48)]  # C2 .. B5
    frames = filterbank(path, [f"bandpass=f={f:.3f}:width_type=q:w=17" for f in notes], 8000)
    out = []
    for f in frames:
        pc = [sum(f[o * 12 + k] for o in range(4)) for k in range(12)]
        total = sum(pc) or 1e-12
        out.append([v / total for v in pc])
    return out


def detect_bounds(level: list[float]) -> tuple[float, float]:
    """(first sound, last point before the fade/ending) in seconds."""
    loud = sorted(level)[len(level) // 2]
    first = next((i for i, db in enumerate(level) if db > loud - 30), 0)
    k = ONSET_FPS  # centred 2 s smoothing
    prefix = list(itertools.accumulate(level, initial=0.0))
    smooth = [(prefix[min(len(level), i + k + 1)] - prefix[max(0, i - k)]) / (min(len(level), i + k + 1) - max(0, i - k))
              for i in range(len(level))]
    last = max((i for i, db in enumerate(smooth) if db > loud - 3), default=len(level) - 1)
    return first / ONSET_FPS, last / ONSET_FPS


def comb_score(onset: list[float], period: float, phase: float) -> float:
    total, t, n = 0.0, phase, 0
    while t < len(onset) - 1:
        total += onset[int(t + 0.5)]
        t += period
        n += 1
    return total / max(n, 1)


def detect_tempo(onset: list[float], hint: float | None) -> tuple[float, float]:
    """(bpm, first beat time in seconds) by autocorrelation, then a fine comb search."""
    mean = sum(onset) / len(onset)
    x = [v - mean for v in onset]
    lo_bpm, hi_bpm = (hint * 0.88, hint * 1.12) if hint else (55.0, 190.0)
    best_lag, best = None, -1e30
    for lag in range(int(60 * ONSET_FPS / hi_bpm), int(60 * ONSET_FPS / lo_bpm) + 1):
        r = sum(a * b for a, b in zip(x, x[lag:])) / (len(x) - lag)
        if r > best:
            best, best_lag = r, lag
    coarse = 60.0 * ONSET_FPS / best_lag
    best_bpm, best_phase, best = coarse, 0.0, -1.0
    steps = 31
    for i in range(-60, 61):  # +-3% in 0.05% steps
        bpm = coarse * (1 + i * 0.0005)
        period = 60.0 * ONSET_FPS / bpm
        for j in range(steps):
            phase = period * j / steps
            s = comb_score(onset, period, phase)
            if s > best:
                best, best_bpm, best_phase = s, bpm, phase
    return best_bpm, best_phase / ONSET_FPS


def pick_downbeat(onset: list[float], bpm: float, beat0: float, beats_per_bar: int) -> float:
    """Of the beats_per_bar candidate bar phases, the one with the strongest onsets."""
    beat = 60.0 / bpm
    bar_period = beat * beats_per_bar * ONSET_FPS
    scores = [comb_score(onset, bar_period, (beat0 + j * beat) * ONSET_FPS) for j in range(beats_per_bar)]
    j = max(range(beats_per_bar), key=scores.__getitem__)
    return beat0 + j * beat


def window(seq: list, centre: float, fps: int, half_s: float) -> list:
    c = int(centre * fps)
    h = int(half_s * fps)
    return seq[max(0, c - h) : c + h]


def correlation(a: list[float], b: list[float]) -> float:
    n = min(len(a), len(b))
    if n < 2:
        return 0.0
    a, b = a[:n], b[:n]
    ma, mb = sum(a) / n, sum(b) / n
    num = sum((p - ma) * (q - mb) for p, q in zip(a, b))
    den = math.sqrt(sum((p - ma) ** 2 for p in a) * sum((q - mb) ** 2 for q in b))
    return num / den if den > 0 else 0.0


class Features:
    def __init__(self, path: Path, onset: list[float]):
        flat = lambda frames: [v for f in frames for v in f]  # noqa: E731
        self.onset = onset
        self.bands = flat(band_profile(path))
        self.chroma = flat(chroma_profile(path))

    def seam(self, start: float, end: float, half_s: float = 2.0) -> float:
        """How alike the music is around the loop start and the loop end (-1..1):
        harmony (chroma), timbre (octave bands) and rhythm (onsets)."""
        def win(seq, width, t):
            c, h = int(t * BAND_FPS) * width, int(half_s * BAND_FPS) * width
            return seq[max(0, c - h) : c + h]

        harmony = correlation(win(self.chroma, 12, start), win(self.chroma, 12, end))
        timbre = correlation(win(self.bands, len(BAND_CENTRES), start), win(self.bands, len(BAND_CENTRES), end))
        rhythm = correlation(window(self.onset, start, ONSET_FPS, half_s), window(self.onset, end, ONSET_FPS, half_s))
        return 0.45 * harmony + 0.25 * timbre + 0.30 * rhythm


def find_loop_end(feat: Features, start: float, lo: float, hi: float) -> list[tuple[float, float]]:
    """Tempo-free search for where the music repeats the loop start: a coarse 0.1 s scan over
    [lo, hi], then 10 ms refinement of the best few peaks. Returns [(score, end)], best first."""
    coarse = []
    t = lo
    while t <= hi:
        coarse.append((feat.seam(start, t), t))
        t += 0.1
    peaks = []
    for s, t in sorted(coarse, reverse=True):
        if all(abs(t - p) > 2.0 for _, p in peaks):
            peaks.append((s, t))
        if len(peaks) == 6:
            break
    refined = []
    for _, t in peaks:
        best = max((feat.seam(start, t + d / 100), t + d / 100) for d in range(-12, 13))
        if best[1] <= hi:
            refined.append(best)
    return sorted(refined, reverse=True)



# ---------------------------------------------------------------------------- rendering


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
    ap.add_argument("--bpm", type=float, help="tempo hint (the BPM asked of Suno)")
    ap.add_argument("--exact-bpm", action="store_true", help="trust --bpm and skip tempo detection")
    ap.add_argument("--beats-per-bar", type=int, default=4)
    ap.add_argument("--first-beat", type=float, help="seconds of the first downbeat (default: detected)")
    ap.add_argument("--start-bar", type=int, default=4, help="loop starts this many bars after the first downbeat")
    ap.add_argument("--start", type=float, help="force the loop start (seconds)")
    ap.add_argument("--end", type=float, help="force the loop end (seconds; default: best musical match)")
    ap.add_argument("--min-loop", type=float, default=40.0, help="shortest acceptable loop in seconds")
    ap.add_argument("--xfade", type=float, default=2.0, help="length of the seam blend in seconds")
    ap.add_argument("--muffle-hz", type=int, default=1400, help="low-pass cutoff for the outgoing tail")
    ap.add_argument("--lufs", type=float, default=-16.0)
    ap.add_argument("--rate", type=int, default=32000)
    ap.add_argument("--quality", type=float, default=3.0, help="Vorbis quality (3 = about 96 kbps)")
    ap.add_argument("--mono", action="store_true", help="downmix to mono (halves the size)")
    ap.add_argument("--out-dir", type=Path, default=ROOT / "romfs" / "music")
    ap.add_argument("--preview-dir", type=Path, default=ROOT / "assets" / "audio" / "music" / "previews")
    ap.add_argument("--preview", action="store_true", help="also write a WAV of the seam (6 s either side)")
    args = ap.parse_args()

    src: Path = args.input
    slug = args.slug or src.stem
    total = duration_of(src)
    level, onset = level_and_onsets(decode_mono(src))
    first_sound, fade_start = detect_bounds(level)
    usable_end = min(fade_start, total) - 0.5

    if args.exact_bpm and args.bpm:
        bpm, beat0 = args.bpm, first_sound
    else:
        bpm, beat0 = detect_tempo(onset, args.bpm)
    beat = 60.0 / bpm
    bar = beat * args.beats_per_bar
    downbeat = args.first_beat if args.first_beat is not None else pick_downbeat(onset, bpm, beat0, args.beats_per_bar)
    downbeat %= bar  # earliest bar line
    while downbeat + bar < first_sound:
        downbeat += bar

    start = args.start if args.start is not None else downbeat + args.start_bar * bar
    xfade = min(args.xfade, start)
    feat = Features(src, onset)

    if args.end is not None:
        end, similarity, candidates = args.end, None, []
    else:
        lo, hi = start + args.min_loop, usable_end
        if lo > hi:
            sys.exit(f"error: no loop of >= {args.min_loop:.0f}s fits before {usable_end:.1f}s. "
                     "Try --start-bar 2 or --min-loop 30.")
        candidates = find_loop_end(feat, start, lo, hi)
        # Prefer the best match; among near-ties (within 0.03) prefer the longer loop.
        top = candidates[0][0]
        similarity, end = max((c for c in candidates if c[0] >= top - 0.03), key=lambda c: c[1])

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
        "bpm": round(bpm, 2), "lufs": args.lufs,
    }
    manifest_path.write_text(json.dumps(dict(sorted(manifest.items())), indent=2) + "\n")

    hint = f" (asked {args.bpm:g})" if args.bpm else ""
    print(f"{slug}: ~{bpm:.1f} BPM{hint}, loop {start:.2f}-{end:.2f}s = {end - start:.1f}s "
          f"(~{(end - start) / bar:.1f} bars), xfade {xfade:.1f}s, gain {gain:+.1f} dB")
    if similarity is not None:
        print("  seam match {:.2f} (other candidates: {})".format(
            similarity, ", ".join(f"{e - start:.1f}s={s:.2f}" for s, e in candidates[:4] if e != end)))
        if similarity < 0.6:
            print("  WARNING: weak seam match - listen to the preview.")

    if args.preview:
        args.preview_dir.mkdir(parents=True, exist_ok=True)
        preview = args.preview_dir / f"{slug}.seam-preview.wav"
        # 6 s before the loop end, then jump to the loop start: exactly what the game does.
        run([
            FFMPEG, "-v", "error", "-y", "-i", str(out), "-filter_complex",
            f"[0:a]asplit=2[x][y];[x]atrim={end - 6:.6f}:{end:.6f},asetpts=PTS-STARTPTS[p1];"
            f"[y]atrim={start:.6f}:{start + 6:.6f},asetpts=PTS-STARTPTS[p2];[p1][p2]concat=n=2:v=0:a=1[o]",
            "-map", "[o]", str(preview),
        ])
        print(f"  seam preview: {preview.relative_to(ROOT) if preview.is_relative_to(ROOT) else preview}")

    print(f"  {src.stat().st_size / 1e6:.1f} MB -> {out.stat().st_size / 1e6:.2f} MB")


if __name__ == "__main__":
    main()
