# Emberclutch — Status

*Live handoff page. Update it at the end of every work session.*

**Updated:** 2026-09-24 · **Milestone:** **Alpha 2 in progress** (Alpha 1 done and tagged `v0.1.0-alpha1`; [checklist](plan/alpha-1-checklist.md)) · WP1 ✅ (a den of three dragons and two eggs, living together) · WP2 ✅ (Sanctuary and Cold Vault, a first world map) · WP3 ✅ (breeding at the Nesting Stone)
· **Branch:** `main` (private `RedLynx101/emberclutch`)

## Where things stand
- **Design** complete for v1: [GDD](design/game-design.md), [breeds & genetics](design/breeds-and-genetics.md),
  [theme](design/theme-and-art-direction.md), [screens & flow](design/screens-and-flow.md).
  Decisions D1–D53 recorded ([log](plan/decisions.md)); nothing open.
- **Plan:** [roadmap](plan/roadmap.md) (milestones A1 → 1.0 → 2.0), [content & assets](plan/content-and-assets.md),
  [Alpha 1 plan](plan/alpha-1.md), [Alpha 2 plan](plan/alpha-2.md) (draft: the style review
  R5, the emblem icon and a 3D HOME Menu banner, D47–D50). New specs (2026-09-24): [hands-on care](design/care-interactions.md)
  (the *Nintendogs*-style polish for WP7) and [world map & travel](design/world-map-and-travel.md)
  (fast travel in Alpha 2, free flight in 1.0).
- **Code:** `src/core` (genetics, needs, mood, growth, eggs, clock, breeding, save, model
  format, skeleton/rig, per-dragon mesh assembly, animation, den behavior, den room,
  daylight, particles) with PC tests (91,750 checks). `src/app` draws the dragons in 3D
  (skinned toon shader) in a 3D den room lit for the time of day, inside themed citro2d
  screens. Runs in Azahar at 60 fps in the den (2026-09-24), sounds and all.
- **Art:** two dragon forms built by script (`tools/blender/dragon_model.py`): a metaball
  hatchling and the skin-modifier grown body, classic wings, part variants, an opening
  mouth with teeth and a tongue (D41), blinking eyes (D42). Exported to `romfs/models/{hatchling,grown}.ecm`.
- **Audio:** music batch 1 processed into `romfs/music/` (6 loops); batch 2 (the hatching and
  Wanderings stingers, the Market loop) and the full Alpha 1 sound-effect set (ElevenLabs,
  2–4 takes per sound) processed on 2026-09-24. The den has hearth and night beds and an
  egg hum under the music.
- **Hardware:** never run on a real 3DS yet (deferred, D28).

## Alpha 1 progress
- ✅ **WP1 engine foundation:** scenes split into `src/app/scene_*.cpp`, string table,
  romfs enabled (music packed in, 7 MB `.3dsx`), budget overlay (frame/CPU/GPU ms,
  command buffer, triangles, draw calls, bones, memory, romfs check), dev menu on
  SELECT (time skip, needs, hatch, next stage, save, reset). Verified in Azahar.
- ⏸ **WP2 dragon model:** sculpt **approved** after R1 → R1b → R1c (D36–D39: baby body,
  classic wings, strong breed shapes, nostrils and mouth, bigger late hatchling). **The egg
  is done:** a 3D egg (948 triangles) in the egg nest and up close on the bottom screen; it
  rocks when rubbed, the dragon inside knocks near hatching, the light inside brightens
  with warmth, and three stages of glowing cracks appear in the last stretch (sounds on
  each). **Cuteness pass** (Noah, 2026-09-24): bird-like folded wings with membranes that
  follow the fingers, the chest heart clear of the body, an opening mouth with teeth and a
  tongue (D41), a centred puppy tail wag, blinking eyes that shut in sleep (D42), the
  walking limp fixed. **Textured** (review [R2](art/reviews/R2-textures.md) sent, D51): a baked
  skin per body form with scale detail and the Pattern gene, and visible dust per body region
  (D46). In-game texture check in Azahar still to do.
- ✅ **WP4 renderer:** `dragon.v.pica` (2-bone skinning, palette colours, fragment-light
  outputs) and `render3d` (toon ramp + rim + emissive heartglow with a white-hot core;
  citro3d inside citro2d scenes). Den camera, a bottom-screen petting close-up, per-dragon
  caches, LOD1 for background dragons. Verified in Azahar: hatchling, juvenile, adult;
  three adults at LOD0 were 8,368 triangles, so LOD1 was added (~4,850 expected;
  **confirm in the emulator**). The static-mesh path came with WP6.
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

- ✅ **WP6 den scene:** a round cave built by `tools/blender/den_model.py` (2,251 triangles:
  rug, sleeping nest, egg nest, hearth with flames, hoard, shelves, sulk nook, skylight
  with a sunbeam) as a cutaway diorama (D40), with vertex lighting baked for day, evening
  and night and blended by the clock; the dragons' light follows the time of day and the
  room's light where they stand. `static.v.pica` + `.esm` loader; particles (embers, motes,
  glints, hearts, Zzz, crumbs, sparkles, dust). Dragons now walk around the hearth, egg nest
  and hoard. The egg sits in the egg nest. **Review R4 sent**
  ([den](art/reviews/R4-den.md)). Builds clean; **not yet seen in the emulator** (paused).

- ✅ **WP8 save system:** versioned A/B slots, CRC32, per-record sizes, validation,
  legacy dev-save import; 5 new PC tests (22 total, 11,383 checks). In Azahar: slots
  alternate, a corrupted newest slot falls back to the older one and is then rewritten.

- ✅ **WP9 audio:** Tremor Ogg streaming on a worker thread with sample-accurate loops
  (LOOPSTART tag), fades between tracks, stingers that duck the loop, 8-channel sound
  effects with takes and per-dragon voice pitch, looping den beds (all queued in short
  slices so Azahar keeps full speed); music director (title / den day / nestsong at night
  and during incubation). Placeholder SFX synthesized (D35; replaced by the real set on
  2026-09-24). Verified in Azahar (fixed a
  thread race that restarted the stream forever). `make_loop.py --no-loop` for stingers.
  Briefs sent: [music batch 2](audio/suno-music-batch-2.md), [SFX](audio/suno-sfx-alpha1.md).
  Note: the emulator needs `sdmc:/3ds/dspfirm.cdc`; a local dummy file works in Azahar (never commit it).

## Next actions
**Alpha 1 is done** (2026-09-24, tag `v0.1.0-alpha1`): every item of its Definition of done
was checked in Azahar by scripted runs, see the [checklist](plan/alpha-1-checklist.md)
with contact sheets. In its last stretch: hands-on care (WP7: the tool tray, petting with a
sweet spot, hand-feeding, brushing and polishing, the bath, fetch; egg turning and
listening, the hatching, naming and renaming, D52), the UI (WP10: fonts, title, system
menu and settings, toasts, save icon) and the CIA (WP11: an interim icon and banner from
our own model). New tool: `tools/autotest.ps1` plays a script in Azahar with nobody at the
controls and saves screenshots of every step (`tests/autotest/`).

1. **Alpha 2** ([plan](plan/alpha-2.md)), the style-independent systems first. WP1
   ✅ (2026-09-24, D53): three dragons and two eggs in the den, switching who you care for,
   the room and egg trimmed to fit the frame; and their life together: games of chase,
   nuzzles, the sunbeam, two curled up in the big nest at night. WP2 ✅: the Sanctuary and
   the Cold Vault, reached through a first world map (X in the den). WP3 ✅: breeding at
   the Nesting Stone (the pair's egg comes the next day). Next: Wanderings, the Market, the
   rest of the world map, more toys, the profile.
2. Then the emblem icon and the animated 3D HOME Menu banner (D48, D50).
3. Then **R5**, three Ember style variants for Noah (blocks, D47); the dragons update in
   the chosen style; the run on Noah's old 3DS (D34).

## Current goal (D31)
**Complete through Alpha 2.** Noah is hands-off until the style variants are ready (D49):
work continues package by package, committing, pushing and updating STATUS and RedWiki
after each. Stops:
- **R5, the style review** (D47): three Ember dragon variants next to the current textured
  style, plus the emblem icon. Blocks the dragons update.
- **The run on Noah's old 3DS** (D34), the last step of Alpha 2.
- R2, R3, R4 and R6 are sent without blocking. Fonts are pre-approved (D33). Sounds that
  haven't arrived use stand-ins (D35). Computer use only while testing in the emulator.

## Waiting on Noah
- Nothing blocking until R5.
- Whenever convenient: the [sound brief 2](audio/sfx-batch-2.md) (hands-on care and
  Alpha 2 sounds); a listen to the new sounds, the two UI chimes and the Market loop seam
  (`assets/audio/music/previews/market-bustle.seam-preview.wav`).
## How to work
- Build: `tools\build.ps1` · Tests: `tools\test.ps1` · Emulator: `tools\emu.ps1`
- Music: `python tools/audio/make_loop.py <wav> --bpm <hint> --preview`
- Blender (headless): `blender -b -P tools/blender/<script>.py -- <args>`; model export:
  `export_dragon.py -- --out-dir romfs/models --reference-dir tests/data`
- Agent notes: [`CLAUDE.md`](../CLAUDE.md)
