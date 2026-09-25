# Emberclutch — Status

*Live handoff page. Update it at the end of every work session.*

**Updated:** 2026-09-25 · **Milestone:** **Alpha 2 done** (run 13 on the old 3DS passed, D72; tagged `v0.2.0-alpha2`; [checklist](plan/alpha-2-checklist.md)) · **Beta, *The valley*: [plan](plan/beta.md) settled (D73–D74); **Phase A ("prove it") done**: WP1 ✅ the flyable valley (measured in the emulator), WP2 ✅ the concept images ([R7](art/reviews/R7-valley-concepts.md) decided: look A, storybook in the way of a cozy life-sim, D75, [look and feel](design/look-and-feel.md)), WP15's briefs sent ([music 3](audio/suno-music-batch-3.md), [sounds 3](audio/sfx-batch-3.md)); **run 14 on the 3DS** (0.1.15: the valley test and 0.1.14's fixes, [steps](plan/hardware-check-3.md)); Noah's working checklists (the sounds to make, run 14's steps): [Emberclutch Checklists](https://claude.ai/artifact/TLouY2VFEjKJb7YyqFQvAE); **now: the dragon revamp** (DR1 done: the kit, the game loads the new kinds, the Pouncer; DR2 under way, D79: all eight base breeds and the Blazeplume crossbreed by eight Opus 5.5 subagents, then R11b; D76–D77: 8 base breeds + 28 crossbreeds = 36 kinds, variants, stats, manners and traits, all rideable; [design](design/dragons-v2.md), [plan](plan/dragon-revamp.md); settled, D78: elements as one-word types, five stats Wing/Wit/Might/Breath/Stamina, ten manners, traits, battles much later, a second layer of crossbreeds past the 36), **then step 2, the valley**; the whole road ahead: [roadmap](plan/roadmap.md#the-road-from-here-2026-09-25)** · hardware pushes when Noah asks (D74) · Alpha 1 done and tagged `v0.1.0-alpha1` ([checklist](plan/alpha-1-checklist.md)) · WP1 ✅ (a den of three dragons and two eggs, living together) · WP2 ✅ (Sanctuary and Cold Vault, a first world map) · WP3 ✅ (breeding at the Nesting Stone) · WP4 ✅ (the Wanderings) · WP5 ✅ (the Market) · WP6 ✅ (the world map and its trips) · WP7 ✅ (toys and den decor) · WP8 ✅ (the profile and family tree) · WP9 ✅ (brief 2's sounds wired, stand-ins until they arrive) · WP10 ✅ (the emblem icon, the 3D banner) · **R5 decided (D54): every look ships, per dragon** · 3DS runs 1–7: the game runs on the old 3DS; the 3D banner shows (sparkles, a hidden triangle, D63); 0.1.7 plays (a new title ID, beds lent by wanderers, a stacking bowl, posing ahead, D64) · a design sit-down opens every milestone from Beta on (D65) · run 9: the banner's sound fixed (stereo), its turning is the HOME Menu's camera (billboards hold it still), a logo of our own can't be signed, so the gold wordmark is a splash in the game (D68) · every file checked before the 3DS (`tools/check_3ds.py`) · the performance pass (WP11d) under way · run 10: a still banner shows (T), the sound plays; the HOME Menu turns every banner once every 10 s, so lab 7 turns ours back (D69) · 0.1.10: the splash, T's banner centred · run 11: V and W froze (four levels deep, D70) · run 12: the banner is settled (X: still, animated, D71) · **0.1.12** on the 3DS · WP11d accepted as it is (Noah) · the wings' roots seated · WP12a ✅ (the hatching: the egg bursts, the hatchling grows out of a white blob) · **WP12 ✅** (looks per dragon, the Dragondex with breed banners, photo mode, the parts library, all 21 breeds, rare traits, egg shells, mud spots; [R6](art/reviews/R6-breeds.md) sent) · WP12c ✅ (a scamper and a gallop; chases, far throws, toy runs and zoomies run) · WP11e ✅ (the 3D slider: the top screen per eye) · run 13 (0.1.13) passed; **0.1.14** fixes its notes (2D at its own depth in 3D, a baby toddle, a tug for the ball, sparring, stalking and tail chasing, spines seated for every build)
· **Branch:** `main` (private `RedLynx101/emberclutch`)

## Where things stand
- **Design** complete for v1: [GDD](design/game-design.md), [breeds & genetics](design/breeds-and-genetics.md),
  [theme](design/theme-and-art-direction.md), [screens & flow](design/screens-and-flow.md).
  Decisions D1–D71 recorded ([log](plan/decisions.md)); open: the grooming design (Beta).
- **Plan:** [roadmap](plan/roadmap.md) (milestones A1 → 1.0 → 2.0), [content & assets](plan/content-and-assets.md),
  [Alpha 1 plan](plan/alpha-1.md), [Alpha 2 plan](plan/alpha-2.md) (draft: the style review
  R5, the emblem icon and a 3D HOME Menu banner, D47–D50). New specs (2026-09-24): [hands-on care](design/care-interactions.md)
  (the *Nintendogs*-style polish for WP7) and [world map & travel](design/world-map-and-travel.md)
  (fast travel in Alpha 2, free flight in 1.0).
- **Code:** `src/core` (genetics, needs, mood, growth, eggs, clock, breeding, save, model
  format, skeleton/rig, per-dragon mesh assembly, animation, den behavior, den room,
  daylight, particles, items and the den's props) with PC tests (103,853 checks). `src/app` draws the dragons in 3D
  (skinned toon shader) in a 3D den room lit for the time of day, inside themed citro2d
  screens. Runs in Azahar at 60 fps in the den (2026-09-24), sounds and all.
- **Art:** two dragon forms built by script (`tools/blender/dragon_model.py`): a metaball
  hatchling and the skin-modifier grown body, classic wings, part variants, an opening
  mouth with teeth and a tongue (D41), blinking eyes (D42). Exported to `romfs/models/{hatchling,grown}.ecm`.
- **Audio:** music batch 1 processed into `romfs/music/` (6 loops); batch 2 (the hatching and
  Wanderings stingers, the Market loop) and the full Alpha 1 sound-effect set (ElevenLabs,
  2–4 takes per sound) processed on 2026-09-24. The den has hearth and night beds and an
  egg hum under the music.
- **Hardware:** run 1 on Noah's old 3DS (2026-09-24, 0.1.1): the 3D banner plays; the game
  didn't start (no boot logo in the CIA). Run 2 (0.1.2): it starts, the stylus is accurate,
  screenshots work; the 3D-banner CIA froze the HOME Menu (a flag 0.1.2 dropped), and the
  game crashed at its first 3D frame (`C3D_TexBind(1, nullptr)`: a null read that Azahar
  lets pass). Both fixed in 0.1.3; every scripted run now checks for unmapped accesses (D59).
  Run 3 (0.1.3): it plays on the hardware; the full den runs 22–23 ms (a performance pass is
  planned, WP11d); Next style froze the 3DS (GPU memory freed while in use: fixed); the 3D
  banner still freezes the HOME Menu, which caches banners per title (banner-lab titles
  next). Run 4 next.

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
**Beta, *The valley*** ([plan](plan/beta.md)). Phase A is done (2026-09-25): WP1, a
flyable placeholder valley (dev menu page 2, Valley test: 1 km, tiles at three detail
levels, fog and the day's sky, the arcade flight and chase camera, the map on the bottom
screen; 6,100–7,100 top-screen triangles in the emulator); WP2, the concept images for R7
in two looks; WP15's music and sound briefs. Next:
1. **Run 14** on the old 3DS when Noah's ready ([steps](plan/hardware-check-3.md)): 0.1.15
   is built, checked and on the 3DS (sent to .51 on 2026-09-25); the steps are also on the
   [checklists page](https://claude.ai/artifact/TLouY2VFEjKJb7YyqFQvAE), whose ticks and notes Claude reads back.
2. Noah makes the batch 2 and 3 music and sounds (the same page). R7 is decided (D75).
3. **The dragon revamp (D76–D77, [plan](plan/dragon-revamp.md)):** R11 answered: 36 kinds in
   the end (8 base breeds, a crossbreed for every pair), three variants and a rare one each,
   stats, manners and traits, own bodies and animations, all rideable. Its questions are
   answered (D78); it starts on Noah's go: DR1 (the pipeline and engine), DR2 (the four
   bases, Opus 5.5 subagents in parallel, R11b), DR3 (into the game, run 15), DR4 (six
   crossbreeds), then Beta resumes with DR5 alongside.
4. Then step 2, the valley: WP3 (the landscape, R8) → WP5 (getting about) → WP6 (the map)
   → WP4 (the places) → WP7 (discovery, the Wanderings).

**Earlier:** **Alpha 1 is done** (2026-09-24, tag `v0.1.0-alpha1`): every item of its Definition of done
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
   the Nesting Stone (the pair's egg comes the next day). WP4 ✅: the Wanderings (walk with
   a dragon, the pedometer counts, it finds Gleam, trinkets and sometimes a wild egg). WP5
   ✅: the Market (food for Gleam into the pouch, selling trinkets, the egg of the day). WP6
   ✅: the world map's trips (a heart travels the path from where you are; A skips). WP7 ✅:
   toys and decor from the Market (the feather, the rope, the puzzle orb, the food bowl;
   rugs, lanterns, perches, plants, banners), played with up close and on their own,
   tug-of-war included. WP8 ✅: the profile (about it, its stats and looks, its sweet spot
   and favourite once found, a three-generation family tree), also from the Sanctuary and
   the Vault. WP9 ✅: every sound in brief 2 has its slot and plays where it belongs, with
   a retuned stand-in until its file arrives. WP10 ✅: the emblem app icon and the animated 3D
   HOME Menu banner (the baby Ember in its cracked egg; 284 KB CGFX via pycgfx, rigid pieces
   only; packed with `package_cia.ps1 -Banner3D`, the flat banner stays the default until
   the old 3DS shows the 3D one). **R5 is built** ([review](art/reviews/R5-style.md)): the
   current look next to V1 surface, V2 shape and V3 bold (the ember-veined dragon), as sheets,
   turntables and in the game (dev menu page 2: Next style). **Waiting on Noah's choice.**
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
- **Go for the dragon revamp** ([plan](plan/dragon-revamp.md)): its questions are answered
  (D78); DR1 starts when Noah says so.
- **Music and sounds, batch 3** ([music](audio/suno-music-batch-3.md),
  [sounds](audio/sfx-batch-3.md)); `valley-day`, `valley-night` and `place-found` first.
  Batch 2's sounds are still open too (stand-ins play). All of it, with copy buttons and
  boxes to tick: [Emberclutch Checklists](https://claude.ai/artifact/TLouY2VFEjKJb7YyqFQvAE) (`tools/checklists/make_checklists.py`
  rebuilds it from the briefs).
- **Run 14** (0.1.15, [steps](plan/hardware-check-3.md), and on the checklists page): on the
  3DS's SD card, to install with FBI; it carries 0.1.14 (run 13's fixes) too.
- **0.1.12 on the old 3DS** ([steps](plan/hardware-check-1.md)): the final banner for now
  (lab 8's X), the splash, the stereo sound.
  [What's left for Alpha 2](plan/alpha-2.md); after Alpha 2, the Beta sit-down (D65),
  check the banner, sound and icon fixes, read the budget with a full den and with every
  look in memory, pressing **Y** for a screenshot at each number and anything odd (pulled
  with `tools\pull_shots.ps1`). Runs 1 and 2: [what they found](plan/alpha-2.md) (WP11b). The boot logo stays makerom's (WP11c can't be done as the system logo, D68). Then the hatching rework (WP12a: the egg bursts
  into bits that fall to the floor, and the hatchling grows out of a small white blob), WP12
  (looks per dragon, the Dragondex, the parts library, all 21 breeds, photo mode), the full
  hardware run (D34) and the Alpha 2 tag.
- Sound brief 2 (`docs/audio/sfx-batch-2.md`): nothing waits on it (stand-ins play).
- Whenever convenient: the [sound brief 2](audio/sfx-batch-2.md) (hands-on care and
  Alpha 2 sounds); a listen to the new sounds, the two UI chimes and the Market loop seam
  (`assets/audio/music/previews/market-bustle.seam-preview.wav`).
## How to work
- Build: `tools\build.ps1` · Tests: `tools\test.ps1` · Emulator: `tools\emu.ps1`
- 3DS: `tools\package_cia.ps1 -Banner3D -Version x.y.z` · to the 3DS (the .3dsx, and the CIAs
  in `build/cia-test/` with `-Cia`): `tools\deploy_ftp.ps1 -FtpHost <3ds-ip> -Cia` · screenshots off the 3DS:
  `tools\pull_shots.ps1 -FtpHost <3ds-ip>` (Y in the game takes them)
- Music: `python tools/audio/make_loop.py <wav> --bpm <hint> --preview`
- Blender (headless): `blender -b -P tools/blender/<script>.py -- <args>`; model export:
  `export_dragon.py -- --out-dir romfs/models --reference-dir tests/data`
- Agent notes: [`CLAUDE.md`](../CLAUDE.md)
