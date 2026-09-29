# Emberclutch — Status

*Live handoff page. Update it at the end of every work session.*

**Updated:** 2026-09-29 · **Milestone:** **the long run: Beta and 1.0, run 21 take 4 on the 3DS** ([plan](plan/v1.md), D89-D110). Take 3 (0.9.3) held in the valley; its notes all dealt with in 0.9.4 (D109): the flicker traced to stale GPU data (frame flushes), Noah's haze, riding, walking, the camera, the lantern and more. Run 21's review page: https://claude.ai/artifact/NK9fcBRD7NpQepnUfMfcJZ (db `run21d/<section id>` for take 4; take 3's notes stay under `run21`). Before it: Beta 1 (`v0.3.0-beta`), Alpha 2 (`v0.2.0-alpha2`).
· **Branch:** `main` (private `RedLynx101/emberclutch`)

## Now: run 21, take 4 (2026-09-29)
- **Take 3 (0.9.3)** held in the valley (the ground's shader rewrite); one freeze in 29 minutes, on the
  bottom screen's checkpoint (hangs.txt: "bottom"; no fallback for it). Noah's notes (db `run21`, s0)
  and ~80 screenshots, 6 photos (pulled read-only to the session's scratchpad).
- **The flicker, found:** frames where every citro2d solid shape is gone or black on both screens while
  text and images stay (run 19's screenshots too), with 3D dropping out: stale GPU data. 0.9.4 flushes
  each frame's command list and 2D buffers, resets the 2D state per screen, reports the heap flush and
  the 2D buffer on the overlay, guards citro2d's text. Noah's haze: a fog pull with the load (D109).
- **Everything else in the notes** fixed (D109; new autotests: take4, getoff, lantern, keeper, crestlie,
  flicker2d). Passed in take 3: Love/Energy, profile, tips, painted ground, fireflies, Journal and
  tracking, pins, the fox, Tamsin, voices, photos, the den's speed.
- **Take 4** = hardware-check-6.md; sent to .51 with trace.on kept, the old trace and hangs list off.
- **Next:** Noah's take 4 results (db `run21d`). If the flicker is gone, the flushes were it; if the
  overlay says `flush ERR`, citro3d's heap flush fails on the hardware.

## Run 21, take 3 (2026-09-29)
- **0.9.2 held in the den** (Continue with an egg, hatching, care); the valley froze on its first
  frame, from Map and from heading out (trace: "gpu: top scene sent", never drawn). The title's and
  the den's slowness was the trace itself (per-frame lines for 10 s after each change of view).
- **0.9.3** (D108): the ground's shader writes its texture output whole (one `mov`, as the shaders
  that run on the 3DS do); the valley's parts are GPU checkpoints; a part a session froze in goes
  in sdmc:/3ds/emberclutch/hangs.txt at the next start, and the ground then draws plain (checked in
  the emulator with a made-up trail); the trace keeps marks for 3 frames a view, a line every 5 s,
  and the session before as trace-prev.txt. Noah's notes fixed: X with only an egg, the sleeping
  dragon's word, the Dragondex's young entries (core `dexStage`, tested), the Journal's goals.
  Sent to .51 (sdmc:/cias/emberclutch.cia), trace.on kept, the old trace and hangs list removed.
- **Next:** pull trace.txt, trace-prev.txt and hangs.txt (read-only) after Noah's try. If the
  ground hung again, the fallback worked around it: find another way to paint the ground (UVs in
  the vertex, or none). Once the hardware holds, delete hangs.txt and trace.on on the card.
- **Azahar 2126.1.2**: the checked installer is in Noah's Downloads (winget still has 2126.1.1;
  admin rights needed, so Noah runs it); afterwards check qt-config.ini's old-3DS lines.

## Run 21, take 2 (2026-09-29)
- **Run 21 (0.9.1) froze again** at Continue, and the title crawled (the trace wrote a line to the
  card per mark). Its trace (pulled read-only): the den's first frame was all sent (room, particles,
  the egg, overlays, bottom), then the next frame's begin never returned: the GPU hung on it.
- **0.9.2** (D107): the static program is run 19's again (no texture output); the valley ground has
  its own program (`ground.v.pica`). With trace.on, GPU checkpoints for each scene's first 3 frames
  name the part that hangs ("gpu: <part> sent" with no "drawn"); the trace writes in batches.
  Checked in the emulator (old 3DS): Continue into the den with the checkpoints on, and the valley's
  ground still painted (ground.txt). Sent to .53 (sdmc:/cias/emberclutch.cia), trace.on kept.
- **If it freezes again:** pull trace.txt (read-only FTP) and read the last "gpu:" line; the part
  named is what hangs. If it holds: run 21's steps go on as written (hardware-check-5.md).
- Azahar to be updated (Noah, 2026-09-29), then set to an old 3DS again.

## Run 21 (2026-09-29)
- **Run 20's results** (the long-run review page, db `run`/`labs`): the game froze at Continue and after
  a new game's egg (D106, fixed); nothing else could be tried. Banner labs: H (names + paint) and A2
  (lab A, fresh ID) froze, F, G, I, J, K held ([banner-labs.md](tech/banner-labs.md): the next round
  when Noah asks). Sound picks applied (the rabbit's hop take 2, the flock take 1).
- **Run 21** (0.9.1) = run 20's steps again ([hardware-check-5.md](plan/hardware-check-5.md)); sent to
  .53 (sdmc:/cias/emberclutch.cia) with sdmc:/3ds/emberclutch/trace.on; run 20's labs removed from
  the card. If it freezes: pull trace.txt (read-only FTP) and read where it stopped.
- **The emulator** now runs as an old 3DS (Azahar's `is_new_3ds\default=false`, `is_new_3ds=false`
  in qt-config.ini; the old settings are backed up in the session's scratchpad).

## Run 20 (2026-09-29)
- **For Noah:** the review page above: run 20's steps (tick as you go, notes per section, the banner labs
  Held/Froze), the sounds as kept (round 2: mark only what's still off, and the two made at his note), the six
  loops' seams to hear. Build: `emberclutch.cia` (0.9.0, 73 MB) and `build/lab/banner-lab-{f..k,a2}.cia`,
  **sent to the 3DS at .53 on 2026-09-29** (sdmc:/cias/emberclutch.cia, sdmc:/cias/lab/; run 19's tested labs
  B-E removed from the card).
- **Since the last update:** the chest heart laid on every kind's chest (D104, the kit's `conform_part`, all 14
  kinds re-exported); the camera kept clear of the places and the ground whatever sets it (core/occluders, the
  grotto); the grotto's chest; the stage 7.2 m; the Hollow's camera and brazier; seen while fishing; the foe's
  bars at the top; D merged; the critters' and duels' sounds; the sound review applied (D105); the music in
  and the Performance on show-stage's beat; a whole-game playthrough script; README screenshots.
- **Tests:** 374,858 PC checks, 0 failures; every autotest (playthrough, battle, glade, critters, roamers, cove,
  Hollow, pages, valleyperf, people_anims, cavecam) without unmapped accesses.
- **After run 20:** fixes from Noah's notes, then `v1.0.0` and the public release only on his word (the GitHub
  release, the Universal-DB entry in `docs/release/`).

## The long run (2026-09-28)
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
