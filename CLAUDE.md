# Emberclutch — agent notes

Dragon-raising homebrew for Nintendo 3DS. **The old 3DS is the performance floor.**
Read `docs/plan/decisions.md` before changing design; approved decisions are binding.

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
- Push to 3DS: `tools\run.ps1 -Address <ip>` (Homebrew Launcher, press Y) or
  `tools\deploy_ftp.ps1 -FtpHost <ip>` (ftpd, port 5000)
- Music: `python tools/audio/make_loop.py assets/audio/music/source/<slug>.wav --bpm <bpm> --preview`
  (set `FFMPEG_DIR` if ffmpeg isn't on PATH)

## Conventions
- Match the existing style: 4-space indent, `ec` namespace, `kConstant` names, short
  comments that explain *why*.
- Balance numbers (drain rates, star thresholds, stage gates) live in `src/core/dragon.cpp`,
  and the design doc must stay in sync when they change.
- Concept art in `docs/art/concept/` is AI-generated reference only. Never ship it.
