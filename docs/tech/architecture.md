# Technical Architecture

Status: **v0.2** (2026-09-23)

## 1. Target and constraints

- **Performance floor: old 3DS / 2DS.** ARM11 MPCore at 268 MHz, PICA200 GPU,
  64 MB application memory when installed as a CIA, and 6 MB of VRAM.
- **Screens:** top 400×240 (stereoscopic optional), bottom 320×240 touch screen.
- **Formats:** `.3dsx` for development (Homebrew Launcher / `3dslink`), `.cia` for release (FBI).
- **Toolchain:** devkitPro through MSYS2 (`C:\msys64\opt\devkitpro`): devkitARM, libctru 2.7,
  citro3d 1.7, citro2d 1.7, picasso (shader assembler), tex3ds, mkbcfnt, 3dslink.
  CIA packaging uses `makerom` and `bannertool`, the same as in the 3D-Claw project.

### Frame budget

| Item | Budget |
|---|---|
| Target frame rate | **30 fps locked** in 3D scenes, 60 fps in menus |
| Skinned dragons on screen | ≤ 3 (den), 1 up close (petting, riding) |
| Dragon triangles | Adult LOD0 ≤ 3,000 · LOD1 ≤ 1,200 · hatchling ≤ 1,800 |
| Bones per draw | ≤ 24 (vertex shader constant limit, see §4) |
| Dragon texture | One shared 128×128 ETC1A4 mask set per body part family |
| Environment | Vertex-colored, ≤ 8k visible triangles, fog-limited |
| Audio | Music streamed from romfs, sound effects preloaded, ≤ 8 voices |

## 2. Layers

```
┌────────────────────────────────────────────────────────────┐
│ game/      scenes & state machine: Title, Den, Pet, Nest,  │
│            Sanctuary, Train, Arena, Valley/Ride, Market     │
├───────────────────────────┬────────────────────────────────┤
│ render/  citro3d + citro2d│ audio/  ndsp mixer, streaming   │
│ ui/      bottom-screen UI │ input/  touch, pad, mic, gyro   │
├───────────────────────────┴────────────────────────────────┤
│ platform/  libctru wrappers: clock, fs/save, pedometer, uds │
├────────────────────────────────────────────────────────────┤
│ core/      PURE C++17 — no libctru. Genetics, needs,       │
│            growth, mood, bond, time catch-up, den/storage,  │
│            save serialization. Unit-tested on the PC.       │
└────────────────────────────────────────────────────────────┘
```

**Rule:** `src/core` must compile with a desktop g++ (MSYS2 `ucrt64`) and has no
dependency on libctru, the clock, or files. Time is passed in as plain numbers. This keeps
the whole simulation deterministic and testable without hardware (`make -C tests`).

## 3. Simulation

- **Time source:** `platform::nowUnix()` (from the RTC via `osGetTime`). Local calendar day
  is used for care stars.
- **Catch-up:** on boot or resume, elapsed time is applied in ≤ 1-hour steps, capped at
  14 days. If time moved backwards, nothing is applied (see the GDD, §10).
- **Needs:** per-hour drain rates by stage and personality; sleep hours regenerate Energy.
- **Care stars:** each local day records the average of the minimum need over the hours
  the dragon was active, plus whether the player visited. That gives 0–3 stars.
- **Growth:** `stageFor(daysSinceHatch, careStars)` decides the stage; a continuous
  `growth01` within the stage drives bone scales and proportions.
- **Randomness:** one seeded PRNG (xorshift-style) per purpose (breeding, personality,
  finds) so bugs can be reproduced from a seed.

## 4. Rendering

### Skinned dragons

- One shared skeleton for all dragons. **Builds** (Sturdy/Sleek/Long) and **growth
  stages** are per-bone scale tables blended on the CPU each frame.
- The PICA200 vertex shader has 96 float constant registers. A 3×4 bone matrix takes 3,
  so after the projection and model-view matrices there is room for about 28 bones per
  draw call. The rig targets **≤ 24 bones per draw**. The body and wings are separate
  draw calls with their own bone sets if the full rig grows past that.
- Skinning happens in a picasso vertex shader (`shaders/skinned.v.pica`), with up to 2
  bone weights per vertex.
- Parts (horns, frill, wings, tail tip) are rigid or lightly skinned meshes attached to
  fixed bones. Only the selected ones are drawn.

### Coloring (no texture per variant)

The mask texture's channels are **R = base weight, G = accent weight, B = pattern
weight, A = heartglow region**. Painted shading lives in the base value. The texture
combiner (TEV) stages:

1. `tex.r × baseColor` (constant color)
2. `+ tex.g × accentColor` (multiply-add)
3. `+ tex.b × patternColor` (multiply-add; pattern type selects which mask texture)
4. `× fragment lighting` (toon lookup table plus warm rim)
5. `+ tex.a × heartglowColor × pulse` (emissive heartglow)

Rare traits change the constants or add a specular lookup table (Iridescent).

### Environment and effects

- Vertex-colored meshes with baked lighting. Gradient sky dome, linear fog.
- Skyreach Valley: height-field chunks (32×32 quads), about 9 visible, culled against the
  view, with fog hiding the edge.
- Particles: pooled camera-facing sprites through citro2d-style batching on the top screen.
- Stereoscopic 3D: optional second eye render. Can be switched off per scene if it
  doesn't fit the budget.

### Bottom screen

citro2d for all UI: eggshell panels, ember gauges, heartglow orb, pouch, map. In petting
mode the bottom screen renders a close-up of the dragon directly (a second 3D pass, one
dragon only, so it fits the budget) with body-zone hitboxes projected from its bones.

## 5. Assets pipeline

```
Blender (.blend) ──headless export──▶ glTF ──tools/asset/convert_model.py──▶ .ecm (model)
                                                                         └─▶ .eca (animations)
PNG masks ──tex3ds──▶ .t3x          Fonts ──mkbcfnt──▶ .bcfnt
Suno WAV ──tools/audio/make_loop.py (ffmpeg)──▶ .ogg with LOOPSTART/LOOPLENGTH
```

- **Blender runs headless:** `blender.exe -b -P tools/blender/<script>.py`. Models,
  rigs and animations are produced by version-controlled Python scripts where practical,
  so they can be regenerated and reviewed.
- **.ecm** (Emberclutch model): little-endian binary. Header, vertex buffer
  (position as int16 ×3 with scale, UV as int16 ×2, normal as int8 ×3, bone indices as
  u8 ×2, weights as u8 ×2), index buffer (u16), and a list of submeshes, each with its own
  bone set.
- **.eca** (Emberclutch animation): per-bone quaternion (int16 ×4) plus optional
  translation at 30 Hz, looping flag, event markers (footstep, chomp, flap).
- Everything ships in **romfs**. Music is streamed and never loaded whole.

## 6. Audio

- **ndsp** (proven in 3D-Claw and asteria-ds). Channel 0–1 for music, 2–7 for sound effects.
- **Music:** Ogg Vorbis via Tremor (the `3ds-libvorbisidec` package, installed
  2026-09-23) decoded on a worker thread, with sample-accurate loops from the
  `LOOPSTART`/`LOOPLENGTH` tags.
- **Sound effects:** PCM16 or DSP-ADPCM WAVs preloaded into linear memory.
- **Dragon voices:** a small set of base samples pitch-shifted per dragon (from the genome
  seed) and per stage.

## 7. Input

- **Touch:** petting zones, drag-and-drop (food and toys), flick gestures (throws).
- **Pad:** Circle Pad plus buttons for riding and menus.
- **Mic:** MICU at 16 kHz. Voice commands use on-device template matching: MFCC features
  plus dynamic time warping against 3 recordings per command. Runs on a worker thread.
  **Cue buttons are always available** as an alternative.
- **Gyro/accelerometer:** optional look-around while riding, shaking the treat bag.
- **Pedometer:** `PTMU_GetStepHistory` / `PTMU_GetTotalStepCount` for Wanderings.

## 8. Save data

- Location: `sdmc:/3ds/emberclutch/save.a` and `save.b`, written alternately. The newest
  valid slot wins, so a save is never overwritten in place.
- Format: header (`"EMBC"`, version, CRC32, timestamp) followed by records. Every
  creature record carries a `species`, `bodyPlan` and `modules` field so the equine line
  fits without a format break.
- Capacity: 200 creatures (~128 bytes each) + 50 vault eggs + inventory ≈ 40 KB.
- Versioned migrations live in `core/save.cpp`. Old saves are always upgraded, never
  rejected.

## 9. Multiplayer (later)

UDS local wireless ("Sky Visits"), based on the `3ds-linkplay` uds-demo. The host den is
authoritative; the guest sends inputs; genomes are exchanged for cross-den clutches.

## 10. Build and deploy

| Task | Command |
|---|---|
| Build `.3dsx` | `tools/build.ps1` (runs `make` in MSYS2 with the devkitPro environment) |
| PC unit tests | `tools/test.ps1` (runs `make -C tests` with the ucrt64 g++) |
| Push + run over Wi-Fi | `tools/run.ps1 -Address <3ds-ip>` (`3dslink`, Homebrew Launcher netloader: press **Y**) |
| Upload files over Wi-Fi | `tools/deploy_ftp.ps1 -FtpHost <3ds-ip>` (ftpd on port 5000) |
| Run in the emulator | `tools/emu.ps1` (Azahar; `-ResetSave`, `-NoBuild`) |
| Package CIA | Alpha 1 (WP11): `tools/package_cia.ps1`, adapting 3D-Claw's (`makerom` + `bannertool`) |


## 11. Testing and development process

- **Unit tests (PC):** genetics distributions, stage gates, needs catch-up, clock
  rollback, breeding rules, save round-trips and migrations, model-converter round trips.
- **Emulator-first (D28):** day-to-day checks run in Azahar. It shows logic, UI and
  rendering correctness, but its speed says nothing about an old 3DS.
- **Budget counters instead of hardware timing (until the hardware check):** the debug
  overlay counts what we control and what drives old-3DS cost: triangles and draw calls
  per frame, bones per draw, particles, texture memory, linear/VRAM use, command-buffer
  use (`C3D_GetCmdBufUsage`), plus `C3D_GetProcessingTime` / `C3D_GetDrawingTime`. Any
  counter over its limit turns red. Limits: §1.
- **Hardware checks (old 3DS), when Noah is ready (recommended by end of Alpha 2):**
  frame-time overlay, memory high-water mark, a stress scene with 3 dragons plus particles.
  **Performance sign-off is on real hardware only.**
- **Stay close to known-good GPU patterns** (devkitPro `3ds-examples/graphics/gpu`) so
  the emulator and the hardware don't diverge.

## 12. Text

All player-facing strings live in one table (`src/app/strings.hpp`). v1 is English only
(D30); a translation later means adding a table, not touching code.
