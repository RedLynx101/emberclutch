# Developing Emberclutch

How to build, run and test the game, and where the design and technical docs are. The
player's README is [the repository's front page](../README.md).

## Docs

| Doc | What's in it |
|---|---|
| [Game design](design/game-design.md) | Pillars, loops, stages, needs, mood, training, competitions, riding, den |
| [Breeds & genetics](design/breeds-and-genetics.md) | Elements, hybrid table, traits, inheritance rules |
| [Theme & art direction](design/theme-and-art-direction.md) | Palette, heartglow, shape language, UI, audio |
| [Architecture](tech/architecture.md) | Layers, rendering budget, asset pipeline, save format |
| [Screens & flow](design/screens-and-flow.md) | Every screen and how they connect |
| **[Status](STATUS.md)** | Live state and next actions |
| [Roadmap](plan/roadmap.md) · [Decisions](plan/decisions.md) | Playable milestones and the decision log |
| [Content & assets](plan/content-and-assets.md) | Models, animations, scenes, effects, UI, audio, items |
| [Alpha 1 plan](plan/alpha-1.md) | Current work plan and definition of done |
| [Suno music brief](audio/suno-music-brief.md) | Prompts and settings for the soundtrack |
| [Equine line](future/equine-line.md) | Future horses, pegasi, unicorns and alicorns |

## Building

Requirements: [devkitPro](https://devkitpro.org/wiki/Getting_Started) with the `3ds-dev`
group (MSYS2 on Windows). For the PC unit tests, a desktop g++ (MSYS2 `ucrt64`).

```powershell
powershell -ExecutionPolicy Bypass -File tools\build.ps1   # emberclutch.3dsx + .smdh
powershell -ExecutionPolicy Bypass -File tools\test.ps1    # core unit tests on the PC
```

Or from an MSYS2 shell: `source /etc/profile.d/devkit-env.sh && make`.

## Running in an emulator

`tools\emu.ps1` builds and opens the game in [Azahar](https://azahar-emu.org/)
(`winget install AzaharEmu.Azahar`). The mouse is the stylus; R = `W`, X = `Z`,
A = `A`, START = `M`. Add `-ResetSave` to start fresh. The emulator is fine for checking
logic and UI, but frame rate must be judged on a real old 3DS.

## Running on a 3DS (over Wi-Fi)

- **Quick test:** open the Homebrew Launcher, press **Y** (netloader), then run
  `tools\run.ps1 [-Address <3ds-ip>]`.
- **Install to the SD card:** start **ftpd** on the 3DS, then run
  `tools\deploy_ftp.ps1 -FtpHost <3ds-ip>`. It uploads `emberclutch.3dsx` to
  `sdmc:/3ds/emberclutch/`, to run from the Homebrew Launcher; with `-Cia` also every CIA in
  `build/cia-test/` to `sdmc:/cias/`. Each file's size is checked on the 3DS.
- **Install on the HOME Menu (CIA):** `tools\package_cia.ps1` builds `emberclutch.cia`
  (icon, banner and sound included; makerom and bannertool are taken from the 3D-Claw
  project next to this one, or PATH). Copy it to the SD card and install it with **FBI**
  (Luma3DS). The animated 3D banner is the default (`-Banner2D` packs the flat one; build the
  3D one with `tools\make_banner.ps1`, which needs pycgfx in `build\tools\pycgfx`).
- **Checked before it goes:** `py -3.12 tools\check_3ds.py <files>` checks a CIA, .3dsx,
  banner, SMDH, CGFX, glTF or WAV against what the 3DS accepts (hashes, the boot logo, the
  banner's model and sound format, and more; see the script). The packing and upload scripts
  run it and stop on a failure.

Controls: Continue or New game (your name, then pick an egg: tap twice), rub the egg warm,
turn it and listen to it, watch it hatch and name it, then care for the hatchling with the
tool tray. **START** opens the system menu (settings, save & quit); **SELECT** the dev menu
in dev builds. **Y** saves a screenshot of both screens anywhere, to
`sdmc:/3ds/emberclutch/screenshots/` with a line of numbers in its `log.txt`;
`tools\pull_shots.ps1 -FtpHost <3ds-ip>` copies them off as PNGs. Dev shortcuts: **R+A**
skips 1 hour, **R+X** skips 1 day.

Unattended checks: `tools\autotest.ps1 tests\autotest\tour.txt -ResetSave` plays a scripted
session in Azahar and saves screenshots of each step to `build/autotest/`.

## Layout

```
src/core/    portable simulation (no libctru) — genetics, needs, growth, time
src/app/     3DS app (citro2d prototype for now)
tests/       PC unit tests for src/core
tools/       build, test, deploy, audio (make_loop.py), and later Blender/asset scripts
docs/        design, tech, plans, concept art
assets/      source assets (icon; music sources are git-ignored)
romfs/       files packed into the app (processed music, models, textures)
```

