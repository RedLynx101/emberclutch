#!/usr/bin/env python3
"""Synthesize placeholder sound effects into romfs/sfx/ (22.05 kHz mono 16-bit WAV).

  python tools/audio/make_placeholder_sfx.py

These are stand-ins (D35) until the Suno Sounds set from docs/audio/suno-sfx-alpha1.md is
generated; replace a file by dropping a real WAV with the same name into romfs/sfx/.
Needs ffmpeg on PATH or FFMPEG_DIR.
"""
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "romfs" / "sfx"

# name: (duration s, aevalsrc expression, extra filter)
SOUNDS = {
    "tap": (0.06, "sin(2*PI*1400*t)*exp(-t*90)*0.6", None),
    "confirm": (0.22, "if(lt(t,0.09),sin(2*PI*784*t),sin(2*PI*1175*t))*exp(-mod(t,0.11)*18)*0.5", None),
    "back": (0.2, "if(lt(t,0.09),sin(2*PI*1175*t),sin(2*PI*784*t))*exp(-mod(t,0.1)*18)*0.45", None),
    "munch": (0.36, "(random(0)*2-1)*exp(-mod(t,0.12)*28)*0.6", "lowpass=f=1600"),
    "brush": (0.34, "(random(0)*2-1)*sin(PI*t/0.34)*0.35", "bandpass=f=3200:width_type=o:w=1.5"),
    "purr": (0.8, "sin(2*PI*95*t)*(0.55+0.45*sin(2*PI*24*t))*sin(PI*t/0.8)*0.6", "lowpass=f=900"),
    "chirp": (0.2, "sin(2*PI*(900*t+2500*t*t))*sin(PI*t/0.2)*0.5", None),
    "crack": (0.12, "(random(0)*2-1)*exp(-t*45)*0.7", "highpass=f=800"),
    "hatch_pop": (0.55, "sin(2*PI*(300*t+1200*t*t))*exp(-t*6)*0.5+gt(t,0.15)*sin(2*PI*2400*t)*exp(-mod(t,0.12)*30)*0.15", None),
    # Animation events (WP5): footsteps, lying down / landing, wing flaps, yawns.
    "step": (0.12, "sin(2*PI*90*t)*exp(-t*40)*0.45+(random(0)*2-1)*exp(-t*60)*0.12", "lowpass=f=500"),
    "thump": (0.35, "sin(2*PI*60*t)*exp(-t*14)*0.7+(random(0)*2-1)*exp(-t*30)*0.2", "lowpass=f=400"),
    "flap": (0.3, "(random(0)*2-1)*sin(PI*t/0.3)*0.5", "bandpass=f=600:width_type=o:w=1.2"),
    "yawn": (0.9, "sin(2*PI*(420*t-260*t*t))*(0.8+0.2*sin(2*PI*6*t))*sin(PI*t/0.9)*0.35", "lowpass=f=1200"),
}


def ffmpeg() -> str:
    d = os.environ.get("FFMPEG_DIR")
    if d and (Path(d) / "ffmpeg.exe").exists():
        return str(Path(d) / "ffmpeg.exe")
    f = shutil.which("ffmpeg")
    if not f:
        sys.exit("error: ffmpeg not found (set FFMPEG_DIR)")
    return f


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    exe = ffmpeg()
    for name, (dur, expr, filt) in SOUNDS.items():
        args = [exe, "-v", "error", "-y", "-f", "lavfi", "-i", f"aevalsrc='{expr}':s=22050:d={dur}"]
        af = "afade=t=out:st={:.3f}:d=0.02".format(max(0.0, dur - 0.02))
        if filt:
            af = filt + "," + af
        args += ["-af", af, "-ac", "1", "-c:a", "pcm_s16le", str(OUT / f"{name}.wav")]
        subprocess.run(args, check=True)
        print(f"{name}.wav  {(OUT / f'{name}.wav').stat().st_size} bytes")


if __name__ == "__main__":
    main()
