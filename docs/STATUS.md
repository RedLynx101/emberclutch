# Emberclutch — Status

*Live handoff page. Update it at the end of every work session.*

**Updated:** 2026-09-28 · **Milestone:** **the long run: Beta and 1.0 built together** ([plan](plan/v1.md), [work split](plan/v1-work.md), D92-D102). All six workstreams merged, the look picked (A with painted textures, D99), the valley's creatures merged (L), roaming trainers and duels in progress (D); the long-run review page is up (sounds to mark, music to make); then run 20 (0.9.0) with banner labs F-K and A2. Before it: Beta 1 (`v0.3.0-beta`), Alpha 2 (`v0.2.0-alpha2`).
· **Branch:** `main` (private `RedLynx101/emberclutch`)

## Now: the long run (2026-09-28)
- **Foundation** (on main first): Love and Energy apart, a trainer's record per dragon (xp, trained
  stats, moves, wear, dye, titles, wins, cups, ribbons, deepest floor), the save's progress block
  (accessories, dyes, leagues, the Hollow, the day's claims, tips, records, the cove's day), four
  new places' ground and ids, 31 sound slots, `app/valley_ext` (features that stand people in the
  valley and take it over), custom speakers in the dialogue box. Save buffers 64 KB.
- **Workstreams merged** (Opus 5.5 in worktrees, [v1-work.md](plan/v1-work.md)): A the places
  (Emberpeak Caldera, Moonpetal Glade, Driftwood Cove, Frostspire Hollow, named anchors, run 19's
  model fixes); B battles (core/battle, core/league, core/hollow, app/battle_view: turn-based 1v1,
  the Ember to Starfire league, finals on the caldera ring, the Hollow's 30 floors); C the
  challenges retuned (once-a-day prizes, trophies, Sky Rings races with rivals, burst and brake,
  Fruit Catch by stats) and fishing at the cove; P the pageant (32 accessories on every kind, 13
  dyes, themed shows, the wardrobe); U the interface (storybook Market and Wanderings, Love and
  Energy gauges, the profile's training and record pages, the Journal's tracked goal with a map
  marker, tips, settings); S sounds (31 synthesised effects, five beds, a channel pool).
- **Lead's work:** the flicker (D95), run 19's fixes (D96-D98: islands and decks stood on, graded
  and forded paths, mountainsides not climbed, X for the Journal and the Outing, travel asks,
  home at the den's door, framed photos from the free camera, the hop on and off, eggs' cracks,
  renewing finds, the creator's controls, walking together, feeding out, textured ground,
  fireflies and falling leaves, far trees, the new women's voice D92), release prep (player
  README, CREDITS, THIRD-PARTY-NOTICES, `docs/release/universal-db.json`, `tools/release/make_qr.py`;
  nothing published).
- **The look (D99):** Noah picked A in the look lab; the ground's texture now has two channels
  (grass strokes; earth, path, rock and bark speckle) mixed per vertex. C (`ground 2`) is the
  fallback if the 3DS can't carry it.
- **Since the merges:** the integration pass (tips wait only while a battle, show or race plays;
  the battle's tip at "Battle?"); the Hollow's camera (a wall across the screen: the eye by an ice
  spire; `bview::viewClear`, the Hollow's nearer camera); prizes nothing gave (the Hollow's and the
  found things to wear, a prize dye per league final); the league tracker on the next challenger;
  new music taking over as its files arrive; the ground's detail following the triangle budget
  (D102); the glade's stage 7.2 m round for three dragons (Noah); the camera behind you after a
  battle; the whole-game playthrough script and the README's screenshots.
- **Valley life (D101):** L merged (seven critters with an A moment each, the Journal's page, one
  draw, 0-136 triangles); D merged (below).
- **Workstream D (roaming trainers, duels, people's doings; D103):** `core/roamers` (eight trainers,
  3-5 out a day on the paths' network, a fair duel, a little Gleam a day each), `app/feature_roamers`,
  `core/routines` + `app/people_acts` (villagers by the hour), 17 new people's clips, battles' people
  react, Tam fishes, trainers stop to watch and clap; autotests `roamers.txt`, `people_anims.txt`. A roamer
  encounter view ~10.1k triangles with a place in sight (the trainer adds ~1.7k: their dragon on its
  light model ~1.1k, the trainer ~0.6k); talking ~9.1k; a duel ~10k (as the league's battles).
- **Sounds and music (D100):** ElevenLabs by API (`tools/audio/eleven_sfx.py`): batch 4 (24 foley
  and beds) and the critters' 9; the Suno brief batch 4 (six tracks). All on the review page:
  https://claude.ai/artifact/52Kf8yemiqCHkq3wQKnxoW (`tools/review/make_review.py`; db `sounds/<slug>`
  and `music/<slug>`).
- **Run 20 (0.9.0):** steps drafted in [hardware-check-4.md](plan/hardware-check-4.md) (duels to
  add); banner labs F-K and A2 built (`build/lab/`, [banner-labs.md](tech/banner-labs.md)).
- **Budgets:** most views under ~9.6k triangles (the ground's detail adapts); the Market ~10.6k and
  the shows up to ~12.7k are run 20's frame checks.
- **Tests:** 369,831 PC checks, 0 failures; every autotest run without unmapped accesses.

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
  daylight, particles, items and the den's props) and the dragon kinds (nine, each its own model and body plan) with PC tests (182,810 checks). `src/app` draws the dragons in 3D
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
**Beta and 1.0 in one pass** ([plan](plan/v1.md), D89-D90). In order: run 19's fixes (the flicker
first); care and progression (Love, Energy, experience, moves, per-dragon records); turn-based
battles and their league (finals at Emberpeak Caldera); training at Frostspire Hollow; accessories
and themed shows at Moonpetal Glade; the valley's life (textured ground, particles, Driftwood Cove,
random finds, sounds); the interface (Market, Wanderings, Journal, tutorial); economy; then one
large review, run 20 and a small set of banner labs.

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
- **Run 19** (0.3.0 with the big review's fixes, D88, and banner labs B-E,
  [steps](plan/hardware-check-3.md)): on the 3DS at .54 (sdmc:/cias/emberclutch.cia and
  sdmc:/cias/lab/), sent 2026-09-26. The review ([page](https://claude.ai/artifact/6k6yYxfWQjPepJj2hsHuX4),
  [R13](art/reviews/R13-beta1.md)) is answered.
- **Sounds for the challenges** (stand-ins play): a countdown tick, a missed ring, a clean glass
  chime for the crystal lanterns, a soft fizzle for a wrong lantern, fruit bouncing on grass, an
  orchard crowd ([challenges](tech/challenges.md)). Batch 2 and 3's open sounds too, on the
  [Emberclutch Checklists](https://claude.ai/artifact/TLouY2VFEjKJb7YyqFQvAE).

## How to work
- Build: `tools\build.ps1` · Tests: `tools\test.ps1` · Emulator: `tools\emu.ps1`
- 3DS: `tools\package_cia.ps1 -Banner3D -Version x.y.z` · to the 3DS (the .3dsx, and the CIAs
  in `build/cia-test/` with `-Cia`): `tools\deploy_ftp.ps1 -FtpHost <3ds-ip> -Cia` · screenshots off the 3DS:
  `tools\pull_shots.ps1 -FtpHost <3ds-ip>` (Y in the game takes them)
- Music: `python tools/audio/make_loop.py <wav> --bpm <hint> --preview`
- Blender (headless): `blender -b -P tools/blender/<script>.py -- <args>`; model export:
  `export_dragon.py -- --out-dir romfs/models --reference-dir tests/data`
- Agent notes: [`CLAUDE.md`](../CLAUDE.md)
