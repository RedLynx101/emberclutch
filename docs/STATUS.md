# Emberclutch — Status

*Live handoff page. Update it at the end of every work session.*

**Updated:** 2026-09-23 · **Milestone:** **Alpha 1 in progress** (WP1 ✅, WP2 sculpt → R1 sent; WP8 next)
· **Branch:** `main` (private `RedLynx101/emberclutch`)

## Where things stand
- **Design** complete for v1: [GDD](design/game-design.md), [breeds & genetics](design/breeds-and-genetics.md),
  [theme](design/theme-and-art-direction.md), [screens & flow](design/screens-and-flow.md).
  Decisions D1–D35 recorded ([log](plan/decisions.md)); nothing open.
- **Plan:** [roadmap](plan/roadmap.md) (milestones A1 → 1.0 → 2.0), [content & assets](plan/content-and-assets.md),
  [Alpha 1 plan](plan/alpha-1.md).
- **Code:** `src/core` (genetics, needs, mood, growth, eggs, clock, breeding) with 17 PC
  tests (11,356 checks). `src/app` is a themed citro2d **prototype** with a 2D placeholder
  dragon; it runs in Azahar at 60 fps.
- **Art:** concept images (reference only); Blender blockout v0 (hatchling cute, adult
  not yet majestic).
- **Audio:** music batch 1 processed into `romfs/music/` (6 loops). No SFX yet.
- **Hardware:** never run on a real 3DS yet (deferred, D28).

## Alpha 1 progress
- ✅ **WP1 engine foundation:** scenes split into `src/app/scene_*.cpp`, string table,
  romfs enabled (music packed in, 7 MB `.3dsx`), budget overlay (frame/CPU/GPU ms,
  command buffer, triangles, draw calls, bones, memory, romfs check), dev menu on
  SELECT (time skip, needs, hatch, next stage, save, reset). Verified in Azahar.
- ⏸ **WP2 dragon model:** organic mesh, rig and growth stages done
  (`tools/blender/dragon_model.py`); **R1 sent**, texturing waits for it.

## Next actions
1. WP8 save system (versioned A/B + CRC + migrations), with PC tests.
2. WP9 audio: Ogg streaming with loop points; Suno briefs for the hatching stinger and
   the A1 sound-effect set.
3. WP3 converter + WP4 renderer (**ask Noah for extra-high effort**).

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
- **Review R1 (sculpt)**, sent 2026-09-23: [docs/art/reviews/R1-sculpt.md](art/reviews/R1-sculpt.md).
  Texturing waits for it. Everything else continues.

## How to work
- Build: `tools\build.ps1` · Tests: `tools\test.ps1` · Emulator: `tools\emu.ps1`
- Music: `python tools/audio/make_loop.py <wav> --bpm <hint> --preview`
- Blender (headless): `blender -b -P tools/blender/<script>.py -- <args>`
- Agent notes: [`CLAUDE.md`](../CLAUDE.md)
