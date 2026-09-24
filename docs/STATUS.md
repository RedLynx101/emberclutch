# Emberclutch — Status

*Live handoff page. Update it at the end of every work session.*

**Updated:** 2026-09-23 · **Milestone:** **Alpha 1 in progress** (WP1 ✅, WP3 ✅, WP4 ✅, WP5 ✅, WP8 ✅, WP9 ✅; WP2 sculpt approved; WP6 next)
· **Branch:** `main` (private `RedLynx101/emberclutch`)

## Where things stand
- **Design** complete for v1: [GDD](design/game-design.md), [breeds & genetics](design/breeds-and-genetics.md),
  [theme](design/theme-and-art-direction.md), [screens & flow](design/screens-and-flow.md).
  Decisions D1–D39 recorded ([log](plan/decisions.md)); nothing open.
- **Plan:** [roadmap](plan/roadmap.md) (milestones A1 → 1.0 → 2.0), [content & assets](plan/content-and-assets.md),
  [Alpha 1 plan](plan/alpha-1.md).
- **Code:** `src/core` (genetics, needs, mood, growth, eggs, clock, breeding, save, model
  format, skeleton/rig, per-dragon mesh assembly) with PC tests (21,164 checks). `src/app`
  draws the dragon in 3D (skinned toon shader) inside themed citro2d screens; the room is
  still 2D. Runs in Azahar at 60 fps.
- **Art:** two dragon forms built by script (`tools/blender/dragon_model.py`): a metaball
  hatchling and the skin-modifier grown body, classic wings, part variants. Exported to
  `romfs/models/{hatchling,grown}.ecm`.
- **Audio:** music batch 1 processed into `romfs/music/` (6 loops); placeholder SFX until the Suno batch arrives.
- **Hardware:** never run on a real 3DS yet (deferred, D28).

## Alpha 1 progress
- ✅ **WP1 engine foundation:** scenes split into `src/app/scene_*.cpp`, string table,
  romfs enabled (music packed in, 7 MB `.3dsx`), budget overlay (frame/CPU/GPU ms,
  command buffer, triangles, draw calls, bones, memory, romfs check), dev menu on
  SELECT (time skip, needs, hatch, next stage, save, reset). Verified in Azahar.
- ⏸ **WP2 dragon model:** sculpt **approved** after R1 → R1b → R1c (D36–D39: baby body,
  classic wings, strong breed shapes, nostrils and mouth, bigger late hatchling). Still to
  do: the egg model and texturing (review R2, not blocking).
- ✅ **WP4 renderer:** `dragon.v.pica` (2-bone skinning, palette colours, fragment-light
  outputs) and `render3d` (toon ramp + rim + emissive heartglow with a white-hot core;
  citro3d inside citro2d scenes). Den camera, a bottom-screen petting close-up, per-dragon
  caches, LOD1 for background dragons. Verified in Azahar: hatchling, juvenile, adult;
  three adults at LOD0 were 8,368 triangles, so LOD1 was added (~4,850 expected;
  **confirm in the emulator**). The static-mesh path moves to WP6 with the den model.
- ✅ **WP3 model pipeline:** `export_dragon.py` writes `.ecm` (skeleton, growth/build tables,
  body, wing and part variants baked at 4 growth keys, vertex paint). `src/core/model.cpp`
  loads it; `skeleton.cpp` + `rig.cpp` reproduce Blender's deformation. PC tests: parity for
  both forms (body < 0.001, wings < 0.006, parts exact), a 3,000-triangle worst-case budget,
  and the stage → form mapping.

- ✅ **WP5 animation and behavior:** 32 clips authored in Python (`tools/anim/clips.py`,
  pitch/yaw/roll deltas in armature axes on top of the idle pose) → `romfs/anims/dragon.eca`.
  Runtime animator with crossfades and events (sounds), per-frame floor contact, head
  look-at, and a den behavior state machine (everyday life by mood/personality/energy, naps
  and night sleep at the nest, sulking in the nook, care reactions). Walking speed is
  measured from each body's stride (no skating). Dev menu "Next activity" reaches every
  state. PC tests: 57,381 checks. **R3 sent** (contact sheets). Not yet seen in the
  emulator (paused).

- ✅ **WP8 save system:** versioned A/B slots, CRC32, per-record sizes, validation,
  legacy dev-save import; 5 new PC tests (22 total, 11,383 checks). In Azahar: slots
  alternate, a corrupted newest slot falls back to the older one and is then rewritten.

- ✅ **WP9 audio:** Tremor Ogg streaming on a worker thread with sample-accurate loops
  (LOOPSTART tag), fades between tracks, stingers that duck the loop, 6-channel sound
  effects with per-dragon voice pitch; music director (title / den day / nestsong at night
  and during incubation). Placeholder SFX synthesized (D35). Verified in Azahar (fixed a
  thread race that restarted the stream forever). `make_loop.py --no-loop` for stingers.
  Briefs sent: [music batch 2](audio/suno-music-batch-2.md), [SFX](audio/suno-sfx-alpha1.md).
  Note: the emulator needs `sdmc:/3ds/dspfirm.cdc`; a local dummy file works in Azahar (never commit it).

## Next actions
1. **WP6 den scene:** the room model (Blender script), a static-mesh render path, day /
   evening / night lighting, particles. The behavior's `DenLayout` spots move into it.
2. **WP2 rest:** egg model (+ hatching clips); texturing (UVs, scale detail, the Pattern
   gene) → R2.
3. WP7 interactions, WP10 UI/fonts, WP11 CIA packaging, WP12 wrap-up.

**Emulator checks are paused** (Noah asked for no computer use until he says so). Queued
for the next session: 3-dragon test triangle count with LOD1, the petting close-up, the
heartglow core, den life (walking, sitting, sleeping, sulking, reactions, sounds, look-at),
then everything built since.

## Current goal (D31)
**Complete through Alpha 2.** Gates that stop the run:
- **R1 sculpt review blocks** until Noah approves. R2 and R3 are sent but don't block (D32).
- **Alpha 2's last step is one run on Noah's old 3DS** (D34). Everything before it uses the
  emulator plus budget counters.
- Fonts are pre-approved (D33). Missing Suno sounds use placeholders (D35).
- Effort: **high** for Alpha 1, with extra-high for WP3/WP4 (both done: Noah can switch
  back to high). Alpha 2: high for breeding/parts/Wanderings, medium for the Market,
  items and storage screens. Tell Noah when to switch.

## Waiting on Noah
- Optional, non-blocking: Suno [music batch 2](audio/suno-music-batch-2.md) and the
  [sound effects](audio/suno-sfx-alpha1.md).
- Nothing blocking. Comments welcome on R1c (faces, growth) and
  **[R3 animation](art/reviews/R3-anim.md)** (`R3-anim-grown-1/2.png`, `R3-anim-hatchling.png`).

## How to work
- Build: `tools\build.ps1` · Tests: `tools\test.ps1` · Emulator: `tools\emu.ps1`
- Music: `python tools/audio/make_loop.py <wav> --bpm <hint> --preview`
- Blender (headless): `blender -b -P tools/blender/<script>.py -- <args>`; model export:
  `export_dragon.py -- --out-dir romfs/models --reference-dir tests/data`
- Agent notes: [`CLAUDE.md`](../CLAUDE.md)
