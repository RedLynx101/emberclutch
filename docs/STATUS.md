# Emberclutch — Status

*Live handoff page. Update it at the end of every work session.*

**Updated:** 2026-09-23 · **Milestone:** **Alpha 1 in progress** (WP1 ✅, WP3 ✅, WP8 ✅, WP9 ✅, WP2 sculpt → R1 answered, R1b sent; WP4 next)
· **Branch:** `main` (private `RedLynx101/emberclutch`)

## Where things stand
- **Design** complete for v1: [GDD](design/game-design.md), [breeds & genetics](design/breeds-and-genetics.md),
  [theme](design/theme-and-art-direction.md), [screens & flow](design/screens-and-flow.md).
  Decisions D1–D35 recorded ([log](plan/decisions.md)); D36–D38 (two body forms, classic
  wings, stronger breed silhouettes) proposed in R1b.
- **Plan:** [roadmap](plan/roadmap.md) (milestones A1 → 1.0 → 2.0), [content & assets](plan/content-and-assets.md),
  [Alpha 1 plan](plan/alpha-1.md).
- **Code:** `src/core` (genetics, needs, mood, growth, eggs, clock, breeding, save, model
  format, skeleton/rig) with PC tests (17,786 checks). `src/app` is a themed citro2d
  prototype with a 2D placeholder dragon (the 3D renderer is WP4); it runs in Azahar at 60 fps.
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
- ⏸ **WP2 dragon model:** R1 answered (hatchlings "super ugly", wings odd, breeds too
  alike). Reworked: a separate baby body (D36), classic wings for all (D37), strong
  build/ridge differences (D38). **R1b sent**; texturing waits for it.
- ✅ **WP3 model pipeline:** `export_dragon.py` writes `.ecm` (skeleton, growth/build tables,
  body, wing and part variants baked at 4 growth keys, vertex paint). `src/core/model.cpp`
  loads it; `skeleton.cpp` + `rig.cpp` reproduce Blender's deformation. PC tests: parity for
  both forms (body < 0.001, wings < 0.006, parts exact), a 3,000-triangle worst-case budget,
  and the stage → form mapping.

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
1. **WP4 renderer** (picasso skinning shader, per-dragon palette, three draws per dragon,
   depth with citro2d) while Noah reviews R1b. Effort: extra-high.
2. Fold in R1b feedback; texturing (R2) starts after the OK.

## Current goal (D31)
**Complete through Alpha 2.** Gates that stop the run:
- **R1 sculpt review blocks** until Noah approves. R2 and R3 are sent but don't block (D32).
- **Alpha 2's last step is one run on Noah's old 3DS** (D34). Everything before it uses the
  emulator plus budget counters.
- Fonts are pre-approved (D33). Missing Suno sounds use placeholders (D35).
- Effort: **high** for Alpha 1, and ask Noah for **extra-high** during WP3 (converter) and
  WP4 (renderer). Alpha 2: high for breeding/parts/Wanderings, medium for the Market,
  items and storage screens. Tell Noah when to switch.

## Waiting on Noah
- Optional, non-blocking: Suno [music batch 2](audio/suno-music-batch-2.md) and the
  [sound effects](audio/suno-sfx-alpha1.md).
- **Review R1b (sculpt, second pass)**, sent 2026-09-23:
  [docs/art/reviews/R1b-sculpt.md](art/reviews/R1b-sculpt.md) with `R1b-sculpt.png` and
  `R1b-growth.png`. Texturing waits for it. Everything else continues.

## How to work
- Build: `tools\build.ps1` · Tests: `tools\test.ps1` · Emulator: `tools\emu.ps1`
- Music: `python tools/audio/make_loop.py <wav> --bpm <hint> --preview`
- Blender (headless): `blender -b -P tools/blender/<script>.py -- <args>`; model export:
  `export_dragon.py -- --out-dir romfs/models --reference-dir tests/data`
- Agent notes: [`CLAUDE.md`](../CLAUDE.md)
