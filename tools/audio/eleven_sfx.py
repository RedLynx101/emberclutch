#!/usr/bin/env python3
"""Generate sound effects with the ElevenLabs Sound Effects API into the source folder, and record
them (with their prompts) in tools/audio/sfx_manifest.json; then tools/audio/process_sfx.py makes
the game's files.

  python tools/audio/eleven_sfx.py docs/audio/sfx-batch-4.json            (every sound in the batch)
  python tools/audio/eleven_sfx.py docs/audio/sfx-batch-4.json hit plop   (just these)
  python tools/audio/eleven_sfx.py docs/audio/sfx-batch-4.json --dry      (what it would ask for)

A batch is a JSON list of sounds: {"slug", "kind" (process_sfx's kinds), "prompt", "duration"
(seconds), "takes" (how many), "influence" (0..1, how literally; default 0.45), "loop" (true for
a bed: asks for a seamless loop; "loop_cut": process_sfx's {start, length, xfade})}. Each take
becomes assets/audio/sfx/source/<slug>-<n>.wav (not in git), and the manifest's entry lists the
takes as "eleven:<slug>#<n>" with the prompt, so the review page can show what was asked.
A sound already generated is skipped unless --again. Needs ELEVENLABS_API_KEY (never printed)
and ffmpeg.
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = Path(__file__).with_name("sfx_manifest.json")
SOURCE_DIR = ROOT / "assets" / "audio" / "sfx" / "source"
URL = "https://api.elevenlabs.io/v1/sound-generation?output_format=mp3_44100_128"
STYLE = " Clean, close, no music, no voices, no background noise."  # (said with every prompt)


def generate(key: str, prompt: str, duration: float, influence: float, loop: bool) -> bytes:
    body = {"text": prompt + STYLE, "duration_seconds": duration, "prompt_influence": influence,
            "model_id": "eleven_text_to_sound_v2"}
    if loop:
        body["loop"] = True
    req = urllib.request.Request(URL, data=json.dumps(body).encode(), method="POST",
                                 headers={"xi-api-key": key, "Content-Type": "application/json",
                                          "Accept": "audio/mpeg"})
    try:
        with urllib.request.urlopen(req, timeout=120) as r:
            return r.read()
    except urllib.error.HTTPError as e:
        sys.exit(f"error: ElevenLabs said {e.code}: {e.read()[:300]!r}")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("batch", type=Path)
    ap.add_argument("slugs", nargs="*")
    ap.add_argument("--dry", action="store_true")
    ap.add_argument("--again", action="store_true", help="generate even if the sound has takes")
    a = ap.parse_args()
    key = os.environ.get("ELEVENLABS_API_KEY", "")
    if not key and not a.dry:
        sys.exit("error: ELEVENLABS_API_KEY isn't set")
    batch = json.loads(a.batch.read_text(encoding="utf-8"))
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    SOURCE_DIR.mkdir(parents=True, exist_ok=True)
    for s in batch:
        slug = s["slug"]
        if a.slugs and slug not in a.slugs:
            continue
        have = manifest.get(slug, {}).get("takes", [])
        if have and all(t.startswith("eleven:") for t in have) and not a.again:
            print(f"{slug}: has {len(have)} takes (--again to redo)")
            continue
        takes = int(s.get("takes", 1))
        print(f"{slug}: {takes} x {s['duration']} s  \"{s['prompt']}\"")
        if a.dry:
            continue
        names = []
        for n in range(1, takes + 1):
            mp3 = SOURCE_DIR / f"{slug}-{n}.mp3"
            mp3.write_bytes(generate(key, s["prompt"], float(s["duration"]), float(s.get("influence", 0.45)),
                                     bool(s.get("loop"))))
            wav = SOURCE_DIR / f"{slug}-{n}.wav"
            subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", str(mp3), str(wav)], check=True)
            mp3.unlink()
            names.append(f"eleven:{slug}#{n}")
        entry = {"kind": s["kind"], "takes": names, "prompt": s["prompt"], "duration": s["duration"]}
        for k in ("gain", "lp", "attack"):
            if k in s:
                entry[k] = s[k]
        if s.get("loop_cut"):
            entry["loop"] = s["loop_cut"]
        manifest[slug] = entry
        with open(MANIFEST, "w", encoding="utf-8", newline="\n") as f:  # (LF, as the repo keeps it)
            f.write(json.dumps(manifest, indent=1, ensure_ascii=False) + "\n")
    print("done")


if __name__ == "__main__":
    main()
