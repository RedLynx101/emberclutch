# Beta — *The valley*: Work Plan

Status: **Settled** (D73, D74; 2026-09-25): the sit-down's answers and the plan's open points
are decided; work starts with WP1. Alpha 2 is done (`v0.2.0-alpha2`, D72). The
sit-down (D73, [brief](beta-sitdown.md)) put the valley first: an open world to walk and fly
in, the places rebuilt inside it, a new map, flying, the first challenges, people and a
first campaign. Training, voice and the ground cups move to 1.0 ([roadmap](roadmap.md)).
**No hardware pushes for now** (D74): the runs below wait until Noah asks; budgets are
measured in the emulator against the den's hardware numbers meanwhile.

## Decided at the sign-off (D74)
1. **On foot:** you're seen, in the third person, walking with your dragon at your side (it
   follows, hops and glides, and you can send it to sniff things out); you ride it once it's
   grown. This replaces D44's unseen follow camera.
2. **Sky Rings is ridden:** you fly the course yourself; the dragon's Wing sets its speed and
   turning. Lantern Trial and Fruit Catch stay cued from the ground.
3. **The first campaign weaves all three premises** (outline below, reviewed as R10).
4. **Concept images** by AI image generation, reference only; everything built stays
   scripted Blender.
5. **The creator:** a name (as now), two body shapes, six hair styles and colours, five skin
   tones, three outfit colours.
6. **The Wanderings in the world:** the trailheads are places in the valley; a pedometer
   trip is shown as your dragon flying over the valley; its finds come from real spots it
   passes, which mark themselves on your map.

## The first campaign: *The Lantern Festival* (outline for R10)
The valley's old dragon keeper takes you on as an apprentice (B). The festival's lanterns,
one at each place, have gone dark; relighting them is your apprenticeship (A). As they come
back, a star-born dragon is seen over the floating islands, and at the festival it leaves a
rare egg (C). Eight quests, about 2–3 hours, each teaching a part of the valley:
1. **The keeper's apprentice.** Out of the den with your young dragon; meet the old keeper at
   the waterfall; walk the valley's first path; the first lantern, by the den.
2. **Market day.** The Market village's lantern; help its keeper with a Fruit Catch (the
   first challenge, Ember cup).
3. **The hilltop.** The Nesting Stone's lantern; the keeper's story of the festival and of a
   dragon that fell from the stars.
4. **The meadow.** The Sanctuary's lantern; find a stray dragon in the meadow with your
   dragon sniffing it out.
5. **The cold heights.** The Cold Vault's lantern, up a path a young dragon can glide from;
   first glide.
6. **Wings.** Your dragon is grown: the keeper teaches you to ride; Sky Rings to the high
   lantern on the floating islands; the first sighting of the star-born dragon.
7. **The trailhead.** The traveller's lantern at the trailhead; a Wanderings trip that brings
   back a star shard.
8. **The Lantern Festival.** The Lantern Trial at the arena, every lantern lit; the
   star-born dragon comes down and leaves an egg; the keeper names you the valley's keeper.
The star-born egg (its breed or look, and what it hatches into) is settled at R10.

## Definition of done
Checked in Azahar, then on Noah's old 3DS (D34):
1. **Skyreach Valley** (2A): about 1 km across (a grown dragon crosses in 2–3 minutes of
   flight), a height-field landscape in tiles that load as you go, fog hiding the edge, a sky
   with the den's time of day, water, cliffs, floating islands; 30 fps on the old 3DS while
   flying, the budget overlay green.
2. **The places in the world** (3A): the den in a cliff by a waterfall, the Market village,
   the Nesting Stone on a hilltop, the Sanctuary meadow and the Cold Vault cave, the
   Wanderings' trailheads, the arena, the lake and the islands. Each seen from the air and
   entered on foot or by landing, which opens its own scene (the den and its care screens
   as they are).
3. **The map** (4A): a painted valley map on the bottom screen while exploring, fogged until
   explored; your position and heading; places found by coming near; fast travel to any
   found place with the travelling heart; full screen with X.
4. **Getting about** (5A, D74): you on foot in the third person with your dragon at your
   side; adolescents glide from heights; grown dragons fly with you riding (take off, flap,
   glide, dive, bank, land anywhere flat, stamina); the camera follows.
5. **Challenges** (6A): the arena; **Sky Rings**, **Lantern Trial** (breath for all six
   elements) and **Fruit Catch** (a hop version for juveniles); the Ember → Flame → Blaze →
   Starfire cups; ribbons and trophies in the den; Gleam.
6. **People** (7C): the player character with a simple creator, riding and walking; a
   handful of villagers who run the places, with portraits and dialogue; **the first
   campaign**, *The Lantern Festival* (eight quests, D74), with a quest log.
7. **The Wanderings in the world** (9): trailheads as places; trips shown flying over the
   valley; finds from real spots.
8. **Saves carry over:** every dragon, egg, item and the Dragondex from Alpha 2; the new
   world state (position, discoveries, cups, quests, your look) added.
9. **Reviews passed:** R7 concepts, R8 the valley blockout, R9 people, R10 the campaign
   outline (below). The hardware runs happen when Noah asks for them (D74).
10. Tests green, no build warnings, docs, STATUS and RedWiki updated, tagged `v0.3.0-beta`.

## Budgets (proposed; WP1 measures them on the 3DS)
- **30 fps in the valley** (33.3 ms), as architecture §1 sets for 3D scenes; the den and
  menus keep 60 where they have it. In 3D, the valley may drop the right eye's distant tiles.
- Top screen: terrain in view ≤ 3,000 triangles (near tiles full, far tiles coarse), props
  ≤ 1,500 (trees and rocks as low-poly clumps, instanced by draw call), the ridden dragon
  3,000 (LOD0), the rider 500, up to 3 villagers or dragons in view at LOD1 (≤ 1,200 each).
- Draw distance about 150–200 m with fog; the sky a dome with the day's gradient.
- Memory: terrain tiles streamed from romfs around you (about 9 loaded), the places'
  exteriors resident, dragons' looks as now (17.8 MB of linear memory free with all four).

## Phase A — prove it first (risk first)
### WP1 — A flyable valley (technical test; on the 3DS as run 14 when Noah asks)
- A placeholder height field (a few tiles, fog, a sky dome), one place standing in it (the
  den's cliff as a block), the current grown dragon flying it with the arcade controls, a
  follow camera, and the budget overlay. Tile streaming from romfs, near/far detail.
- In the emulator now (D74): the triangles, draws and memory flying low and high over the
  busiest tile, against the den's hardware numbers (a full den of about 9,800 triangles ran
  17–18 ms on the old 3DS). On the old 3DS when Noah asks: frame time, the 3D slider up.
- Also measures the cost of the terrain shader (vertex colours + fog; one tiling detail
  texture if it fits).

### WP2 — Concept images (review R7, blocks the world's art)
- Reference images (AI, reference only, D74): the valley from above, the den's cliff
  and waterfall, the Market village, the arena, the Nesting Stone hilltop, the floating
  islands, and the player character and three villagers; a palette sheet of the valley at
  dawn, day, dusk and night next to the dragons. Noah picks the direction.

## Phase B — the valley
### WP3 — The valley's landscape (review R8: the blockout)
- `tools/blender/valley_model.py`: the height field (a painted height map plus scripted
  shaping: the river valley, cliffs, the lake, the hilltop), vertex-colour painting by slope
  and height, cut into tiles with two detail levels, exported to romfs; props (trees,
  bushes, rocks, grass tufts, reeds) scattered by rules; water; floating islands; clouds.
- The sky dome and fog follow the den's day lighting (dawn, day, dusk, night).
- R8: flyover renders and an in-game fly-through of the blockout, before any detailing.

### WP4 — The places in the world
- Exteriors for each place (the den's cliff and waterfall entrance, the Market village's
  roofs and square, the Nesting Stone hilltop, the Sanctuary meadow with its keepers' huts,
  the Cold Vault cave mouth, the trailheads, the arena, the lake's shore).
- A doorway at each: stepping in (or landing and walking in) opens its scene; leaving puts
  you back outside. The den, the Market, the Stone, the Sanctuary and the Vault keep their
  screens; their top-screen pictures are redone to match the valley (run 13: the old ones
  get replaced).

### WP5 — Getting about: on foot, gliding, flying, riding
- On foot (D74): you walk and run in the third person; the dragon follows at your side,
  hops and glides, and sniffs things out; A calls it, the stylus points it at things.
- Adolescents glide off ledges and slopes; grown dragons take off (from the ground or a
  ledge), flap to climb (A), dive (B), bank (L/R), glide on release, land on flat ground,
  with stamina that grows with Wing and bond.
- The flight animation set (take-off, flap loop, glide, bank left/right, dive, hover,
  landing; a young dragon's flutter-hop), the rider's poses, the follow camera and its
  collisions with the ground.

### WP6 — The painted map
- A painted top view of the valley (rendered from the landscape, then stylised), pins for
  places, fog lifting where you've been; your position and heading live on the bottom screen
  while exploring; tap a found place to fast travel (the heart travels the path); full screen
  with X. Replaces Alpha 2's map (run 13).

### WP7 — Discovery, finds and the Wanderings
- Landmarks discovered by coming near (a bit each in the save), with a find at many (Gleam,
  trinkets, rarely a wild egg), some reachable only from the air.
- The Wanderings (D74): the trailheads in the valley; the pedometer trip shown as the
  dragon flying over the valley; its finds from the spots it passes.

## Phase C — challenges
### WP8 — The arena and the cups
- The arena's scene, entering a challenge, the four cups per challenge with their
  requirements, scoring, results, ribbons (den decor) and trophies, Gleam; the cup day music
  (batch 1 has it) and results stingers.
### WP9 — Sky Rings (ridden, D74)
- Ring courses over the valley per cup (longer and trickier higher up), a clock and a ghost
  of your best run; the dragon's Wing sets speed and turn.
### WP10 — Lantern Trial
- Crystal lanterns light in a pattern; cue the breath to light them in order. Breath effects
  for all six elements (flame, mist, gust, spores, frost, light).
### WP11 — Fruit Catch
- Flick fruit with the stylus; the dragon leaps or dives for it (hop version for juveniles);
  distance and style scoring.

## Phase D — people and the first campaign
### WP12 — The player character (review R9)
- One model and rig through the dragons' pipeline (cute, rounded proportions, about 500
  triangles), walk, run, idle, wave, mount and ride poses; the creator (D74) after
  the naming at a new game (and from the settings for existing saves).
### WP13 — Villagers
- Five or six: the old keeper (your guide), the Market's keeper, the Sanctuary's keeper, the
  arena's steward, a child who follows the dragons about, a traveller at the trailhead. Low
  poly, idle and talk animations, a portrait each; a dialogue box on the bottom screen with
  choices.
### WP14 — The first campaign (review R10: the outline)
- *The Lantern Festival* (outline above): eight quests through the places and the
  challenges, a quest log, rewards (Gleam, decor, the star-born egg at the end), all dialogue
  written for the game's warm tone. R10 reviews the outline before the dialogue is written.

## Phase E — sound, saves, hardware, wrap-up
### WP15 — Music and sound
- A Suno brief (batch 3): the valley by day and by night, the Market village, the arena's
  results stingers, a quest jingle; an ElevenLabs/Suno brief for wind, wingbeats, water,
  footsteps on grass and stone, villagers' voices (babble); stand-ins until they arrive
  (D35). The flight loops `skyreach`/`skyreach-2` and `cup-day` are in hand from batch 1.
### WP16 — Saves and settings
- New save sections (your look, where you are, discoveries, cups and ribbons, quests); an
  Alpha 2 save starts outside the den's cliff with everything it had.
### WP17 — Performance and the hardware runs
- Planned: run 14 (WP1, the technical test), run 15 (after WP5–6: flying, the map, a few
  places), run 16 (the full milestone), bundled, with the steps written up as before. **Held
  until Noah asks (D74)**; until then the emulator's counters keep to the budgets.
### WP18 — Wrap-up
- The checklist, docs, the tag `v0.3.0-beta`; then the 1.0 sit-down.

## Reviews
| Review | What | Blocks |
|---|---|---|
| R7 | Concept images: valley, places, people, palette | The world's art (WP3 on) |
| R8 | The valley blockout, from the air and in the game | Detailing the landscape |
| R9 | The player character and one villager | The other villagers |
| R10 | The campaign outline | Writing the dialogue |

## Order of work
1. **Now:** WP1 (the flyable valley, measured in the emulator) and WP2 (concept images for
   R7) side by side, and the sound briefs of WP15 early so the music can arrive in time.
2. **The valley:** WP3 (landscape, R8) → WP5 (getting about) → WP6 (the map) → WP4 (the
   places) → WP7 (discovery, the Wanderings).
3. **Challenges:** WP8 (the arena and cups) → WP9 (Sky Rings) → WP10 (Lantern Trial) →
   WP11 (Fruit Catch).
4. **People and the campaign:** WP12 (you, R9) → WP13 (villagers) → WP14 (the campaign, R10).
5. **Close:** WP16 (saves) → WP18 (checklist, tag `v0.3.0-beta`), with the hardware runs of
   WP17 wherever Noah asks for them.
Work goes package by package, committed, with STATUS and RedWiki kept up.

## After Beta
**1.0 — *A trainer*** (sit-down first): tricks, voice commands, the training yard, the
Command Trial and Shine Show, the grooming redesign, several wanderers at once, more of the
campaign, polish and the public release.
