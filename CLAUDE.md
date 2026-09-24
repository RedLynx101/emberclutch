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
- Azahar keeps a stale touch map if its window is resized, maximized or fullscreened while
  a game runs: touches then land ~20 px low. Size the window first, then (re)start the
  game (`tools\emu.ps1` relaunches; Azahar remembers the size). Dev builds draw a green
  crosshair where the game reads the stylus (overlay on).
- Music: `python tools/audio/make_loop.py assets/audio/music/source/<slug>.wav --bpm <bpm> --preview`
  (stingers: `--no-loop`)
- Audio on the 3DS: never hand ndsp one long wave buffer (Azahar slows down quadratically
  with buffer length); `src/app/audio.cpp` queues everything in ~4,096-frame slices.
- Sound effects: `python tools/audio/process_sfx.py` (all, or name slugs) turns
  `assets/audio/sfx/source/<slug>-<take>.wav` into `romfs/sfx/`. A new download folder:
  add its files to `tools/audio/sfx_manifest.json`, then run with `--import <folder>`.
- Den room: edit `tools/blender/den_model.py` (keep its spots in sync with `DenLayout` in
  `src/core/behavior.hpp`), then
  `blender -b -P tools/blender/den_model.py -- --out <abs>/romfs/models/den.esm` and
  `tools\test.ps1`. Previews from the game camera: `-- --render <abs prefix> --dragons --shot home`
  (one shot per Blender run: building several dragons in one session is less reliable).
- Egg: `python tools/blender/egg_model.py --out romfs/models/egg.ecm` (plain Python);
  previews with `blender -b -P tools/blender/egg_model.py -- --render <abs prefix>`.
- Animations: edit `tools/anim/clips.py`, then `python tools/anim/build_anims.py` (writes
  `romfs/anims/dragon.eca`) and `tools\test.ps1`. Preview on the rig:
  `blender -b -P tools/blender/preview_anims.py -- --clips walk,sit --form grown --out <abs prefix>`
  (set `FFMPEG_DIR` if ffmpeg isn't on PATH)

## Gotchas
- The Bash tool mangles backslashes inside heredocs (`\n`, line-continuation `\`).
  Make edits that contain backslashes with the Edit/Write tools, not heredoc Python.
- Model changes: edit `tools/blender/dragon_model.py` (two forms: hatchling + grown), then re-run
  `blender -b -P tools/blender/export_dragon.py -- --out-dir romfs/models --reference-dir tests/data`
  and `tools\test.ps1` (the parity and triangle-budget tests must stay green).
- Headless Blender resolves relative output paths unpredictably: pass absolute `--out` paths.
- Heredocs in the Bash tool can also fail with "unexpected EOF" on long Python patches; write
  the patch to a script file (Write tool) and run it instead.
- Windows PowerShell 5 `Set-Content -Encoding utf8` adds a BOM; edit files with the Edit
  tool or Python instead.
- The emulator saves are `save.a`/`save.b` in `%APPDATA%\Azahar\sdmc\3ds\emberclutch`
  (`tools\emu.ps1 -ResetSave` deletes them). Dev menu (SELECT, `N` in Azahar): Hatch now,
  Next stage, Next breed, 3-dragon test.

## Conventions
- Match the existing style: 4-space indent, `ec` namespace, `kConstant` names, short
  comments that explain *why*.
- Balance numbers (drain rates, star thresholds, stage gates) live in `src/core/dragon.cpp`,
  and the design doc must stay in sync when they change.
- Concept art in `docs/art/concept/` is AI-generated reference only. Never ship it.
