# Emberclutch — agent notes

Dragon-raising homebrew for Nintendo 3DS. **The old 3DS is the performance floor.**
Start every session with `docs/STATUS.md` (live state, next actions), then the current
milestone plan (`docs/plan/alpha-1.md`). Read `docs/plan/decisions.md` before changing
design; approved decisions are binding. Update `docs/STATUS.md` at the end of every
session, and add new decisions to the log.

## Layout rules
- `src/core/` is pure C++17: **no libctru, no clock reads, no file I/O**. Time comes in as
  "local unix seconds". Everything in it must compile with the host g++ and be unit-tested
  in `tests/test_main.cpp`.
- `src/app/` is the 3DS layer (citro2d/citro3d). Source file names must be unique across
  `src/app` and `src/core` (the devkitPro Makefile flattens object names).
- UI colors come only from `src/app/theme.hpp` (mirrors the palette in
  `docs/design/theme-and-art-direction.md`).
- The `Genome` struct (16 bytes) and creature records are part of the save format. Change
  them only with a version bump and a migration.

## Commands (PowerShell)
- Build: `powershell -ExecutionPolicy Bypass -File tools\build.ps1` (`-Clean` for a clean build)
- Tests: `powershell -ExecutionPolicy Bypass -File tools\test.ps1`
  (run from PowerShell; MSYS2 bash-launched g++ can't find a temp dir in this sandbox)
- Emulator: `powershell -ExecutionPolicy Bypass -File tools\emu.ps1` (Azahar; `-ResetSave`, `-NoBuild`).
  When driving it with synthetic input, hold taps ~0.2 s and hold key chords ~0.3 s, because
  instant clicks can fall between frames. Dismiss Azahar's update prompt with "Ignore".
- Push to 3DS: `tools\run.ps1 -Address <ip>` (Homebrew Launcher, press Y) or
  `tools\deploy_ftp.ps1 -FtpHost <ip>` (ftpd, port 5000)
- Music: `python tools/audio/make_loop.py assets/audio/music/source/<slug>.wav --bpm <bpm> --preview`
  (set `FFMPEG_DIR` if ffmpeg isn't on PATH)

## Gotchas
- The Bash tool mangles backslashes inside heredocs (`\n`, line-continuation `\`).
  Make edits that contain backslashes with the Edit/Write tools, not heredoc Python.
- Model changes: edit `tools/blender/dragon_model.py`, then re-run
  `blender -b -P tools/blender/export_dragon.py -- --out romfs/models/dragon.ecm --reference tests/data/dragon_reference.ecr`
  and `tools\test.ps1` (the parity test must stay green).

## Conventions
- Match the existing style: 4-space indent, `ec` namespace, `kConstant` names, short
  comments that explain *why*.
- Balance numbers (drain rates, star thresholds, stage gates) live in `src/core/dragon.cpp`,
  and the design doc must stay in sync when they change.
- Concept art in `docs/art/concept/` is AI-generated reference only. Never ship it.
