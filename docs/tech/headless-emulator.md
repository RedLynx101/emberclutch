# Headless emulator checks

Autotest scripts (`tests/autotest/*.txt`, commands in `src/app/autotest.hpp`) can run in an
Azahar that nobody sees: in a WSL distro of their own, on a virtual screen, as fast as the PC
goes. Use this for checks after every change. The windowed run (`tools\autotest.ps1` without
`-Headless`) still works for watching a script play.

**What it can't tell you:** how fast the game runs on an old 3DS. No 3DS emulator models the
PICA200's timing or the ARM11's real cost (Azahar's "simulate GPU timings" is a fixed 0.3 ms
delay). Frame times, hitches and memory pressure are signed off on Noah's old 3DS (D3, D34).

## Using it

```powershell
tools\autotest.ps1 tests\autotest\tour.txt -ResetSave -Headless   # build, run, PNGs + sheets
tools\autotest.ps1 tests\autotest\tour.txt -ResetSave -Headless -NoBuild
tools\shotdiff.ps1 tour -Save    # the run becomes the baseline (build/autotest-baseline/tour/)
tools\shotdiff.ps1 tour          # later: which pictures changed (exit 1 if any)
```

- Output lands where the windowed run puts it: `build/autotest/<script>/` (PNGs, `sheet-NN.png`,
  the game's `log.txt`). The raw run (BMPs, `azahar_log.txt`, the emulator's stdout, the save,
  `result.txt` with the exit code and seconds) is in `build/autotest/.headless/<script>/`.
- The unmapped-memory check runs on the headless emulator's own log, as before.
- **Saves:** every run starts from a fresh SD card. `-ResetSave`: no save. Without it: the save a
  `-KeepSave` run left in this worktree (`build/autotest/headless-save/`), else a copy of the
  Windows dev save (only read, never written). A run without `-KeepSave` clears the kept save,
  so chains work as before (`alpha1-a -ResetSave -KeepSave`, then `alpha1-b`).
- **No lock:** each run has its own emulator, display and SD card, so runs from several
  worktrees (or several scripts at once) don't wait on each other. Three tours side by side
  took 44 s together, against about 33 s for one.
- **Options:** `-Speed 100` real time (default 0: no frame limit); `-SystemClock` the PC's clock
  instead of the fixed morning; `-New3DS` a New 3DS (the default is the old 3DS).

## What makes the pictures repeatable

Two runs of `tour.txt` give the same pictures: 13 of 52 byte-identical, the rest under 0.02% of
pixels, none by more than a faint shade (2026-09-30). A run at `-Speed 100` and one at full
speed give the same pictures too, because the game's clock is the emulated one. What it takes:

| Setting | Why |
|---|---|
| `init_clock=1`, `init_time=1780308000`, `TZ=UTC` | The 3DS clock starts at 2026-06-01 10:00 every run. Azahar shifts a fixed time by the PC's time zone, so the emulator runs in UTC |
| `async_fs_operations=false`, `async_presentation=false` | With them on, file reads and frame handoffs finish at host-timing-dependent moments: animations drifted (median 0.12% of pixels, up to 5%) |
| The game's autotest RNG seed (`scene_starter.cpp`) | Already fixed |
| `frame_limit=0`, `use_vsync=false` | Speed only; doesn't change pictures |

`tools\shotdiff.ps1` ignores channel changes up to 24 and flags a picture when over 0.1% of
its pixels changed. A changed picture gets `ref | new | diff` (changed pixels red) in
`build/autotest/<script>/diff/`. Baselines are local (`build/`), not committed: save one
before a change, compare after.

## How it's put together

| Piece | Where |
|---|---|
| The distro: `emberclutch-test`, Ubuntu 24.04, everything as root | `%LOCALAPPDATA%\wsl\emberclutch-test` |
| Install or repair (safe to rerun); `-Remove` unregisters it | `tools\wsl\install.ps1` |
| Packages (Xvfb, Mesa, libopengl0) and Azahar **2126.1.1**, the version on Windows, pinned by SHA-256 | `tools/wsl/setup.sh` → `/opt/azahar/2126.1.1/squashfs-root` (extracted: no FUSE in WSL), `/opt/azahar/current` |
| Azahar's settings for these runs | `tools/wsl/qt-config.ini` |
| One run (a `/tmp/ec-autotest.*` folder in portable mode: `user/config`, `user/sdmc`, `user/log`) | `tools/wsl/autotest.sh` |
| The PowerShell side (paths, saves, PNGs, sheets, memory check) | `tools/autotest.ps1 -Headless` |

The DSP firmware (`dspfirm.cdc`, dumped from a 3DS) is copied from Azahar's Windows SD card
into each run. Without it ndsp doesn't start and the game runs with the sound off ("AUDIO OFF"
in the overlay), so the audio code wouldn't be tested.

### Keeping it off the desktop

WSLg gives every distro a Wayland and an X display that show up as Windows windows, and Qt
prefers Wayland. `autotest.sh` unsets `WAYLAND_DISPLAY` and sets `QT_QPA_PLATFORM=xcb`, so
Azahar can only reach its own Xvfb. Each run takes a random display number from 100 to 999;
the X server's lock file settles races (`xvfb-run -a` can hand two runs the same number, and
`-displayfd` trips over WSLg's server at `:0`). Xvfb and Azahar share one process group,
stopped with SIGTERM and then SIGKILL (Azahar can ignore SIGTERM). A run whose PowerShell was
killed leaves its emulator behind; the next run stops any emulator whose run folder is gone.

To check by hand that nothing is left: `wsl -d emberclutch-test -- pgrep -fa AppRun.wrapped`.

### Graphics

OpenGL is drawn on the CPU (Mesa's llvmpipe), which handles 400x240 easily: the CPU emulation
is the bottleneck, not drawing. `autotest.sh --gpu NVIDIA` draws through WSL's GPU bridge
(Mesa d3d12 on the RTX 3070 Ti, GL 4.6) but was slower (38 s against 33 s for the tour) because
of the copies back. The Intel adapter only offers GL 4.1, below Azahar's 4.3.

## Upgrading Azahar

When Windows Azahar updates, keep the two in step: set `AZAHAR_VERSION` and `AZAHAR_SHA256`
in `tools/wsl/setup.sh` (the AppImage is the release's `azahar.AppImage`; take its hash with
`sha256sum`), run `tools\wsl\install.ps1`, then re-save the baselines: a new emulator can
shift pixels.

## Alternatives considered (2026-09-30)

- **Docker:** Docker Desktop runs inside WSL2 anyway and is slow to start cold; a plain distro
  needs nothing running. `setup.sh` carries over to a Dockerfile almost line for line if this
  ever needs a CI server.
- **The libretro core + libretro.py:** frame-exact stepping with no display server, but a build
  and harness to maintain. Worth it only if the Qt route stops being repeatable.
- **Panda3DS, Mikage:** less accurate than Azahar; Mikage isn't on PC.
