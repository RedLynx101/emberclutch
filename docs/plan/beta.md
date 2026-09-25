# Beta — *The valley*: Work Plan

Status: **Draft for Noah's sign-off** (D65). Alpha 2 is done (`v0.2.0-alpha2`, D72). The
sit-down (D73, [brief](beta-sitdown.md)) put the valley first: an open world to walk and fly
in, the places rebuilt inside it, a new map, flying, the first challenges, people and a
first campaign. Training, voice and the ground cups move to 1.0 ([roadmap](roadmap.md)).
Nothing here is built until Noah has signed this plan off; the open points just below are
settled first.

## Open points for Noah (before work starts)
Each has a recommendation; they're repeated in chat.
1. **On foot.** With a player character (7C) and walking beside your young dragon (5A),
   you'd be seen on foot in the valley, where D44 and the design doc had you unseen (the
   camera following your dragon) and only ever shown as a rider. **Recommended:** a
   third-person player walking with the dragon at their side (it follows, hops and glides,
   and you can send it to sniff things out), riding once it's grown.
2. **Sky Rings: ridden or cued?** The design has every competition unmounted (you cue the
   dragon from the ground). **Recommended:** ride Sky Rings yourself (flying skill, the
   dragon's Wing stat sets its speed and turning); Lantern Trial and Fruit Catch stay cued
   from the ground.
3. **The first campaign.** Options: **A** *The Lantern Festival*: the valley's festival
   lanterns have gone dark; relight them at each place, ending at the festival with the
   Lantern Trial. **B** *The keeper's apprentice*: the valley's old dragon keeper takes you
   on, and you earn the title through the cups and the villagers' favours. **C** *The first
   Starfire*: sightings of a star-born dragon over the floating islands lead to a rare egg.
   **Recommended:** A, with the old keeper from B as your guide; 6–8 quests, 2–3 hours.
4. **Concept images (R7):** made with AI image generation as reference only, like the
   Phase 0 concepts, then built in Blender by script as always. **Recommended:** yes.
5. **The player creator:** a name (as now), two body shapes, six hair styles and colours,
   five skin tones, three outfit colours. **Recommended:** as listed; more later.
6. **The Wanderings in the world** (your "Sure" on 9, read as): the trailheads become
   places in the valley; a pedometer trip is shown as your dragon flying over the valley;
   what it finds comes from real spots it passes, which also mark themselves on your map.

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
4. **Getting about** (5A): on foot with a young dragon; adolescents glide from heights;
   grown dragons fly with you riding (take off, flap, glide, dive, bank, land anywhere flat,
   stamina); the camera follows.
5. **Challenges** (6A): the arena; **Sky Rings**, **Lantern Trial** (breath for all six
   elements) and **Fruit Catch** (a hop version for juveniles); the Ember → Flame → Blaze →
   Starfire cups; ribbons and trophies in the den; Gleam.
6. **People** (7C): the player character with a simple creator, riding and walking; a
   handful of villagers who run the places, with portraits and dialogue; **a first
   campaign** of 6–8 quests with a quest log.
7. **The Wanderings in the world** (9): trailheads as places; trips shown flying over the
   valley; finds from real spots.
8. **Saves carry over:** every dragon, egg, item and the Dragondex from Alpha 2; the new
   world state (position, discoveries, cups, quests, your look) added.
9. **Reviews passed:** R7 concepts, R8 the valley blockout, R9 people, R10 the campaign
   outline (below).
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
### WP1 — A flyable valley on the 3DS (technical test, run 14)
- A placeholder height field (a few tiles, fog, a sky dome), one place standing in it (the
  den's cliff as a block), the current grown dragon flying it with the arcade controls, a
  follow camera, and the budget overlay. Tile streaming from romfs, near/far detail.
- On the old 3DS: frame time flying low and high, over the busiest tile, with the 3D slider
  up; memory. The numbers set the real budgets above before any art is made.
- Also measures the cost of the terrain shader (vertex colours + fog; one tiling detail
  texture if it fits).

### WP2 — Concept images (review R7, blocks the world's art)
- Reference images (AI, reference only, open point 4): the valley from above, the den's cliff
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
- On foot (open point 1): the player walks and runs, the young dragon follows, hops and
  sniffs; A calls it, the stylus points it at things.
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
- The Wanderings (open point 6): the trailheads in the valley; the pedometer trip shown as the
  dragon flying over the valley; its finds from the spots it passes.

## Phase C — challenges
### WP8 — The arena and the cups
- The arena's scene, entering a challenge, the four cups per challenge with their
  requirements, scoring, results, ribbons (den decor) and trophies, Gleam; the cup day music
  (batch 1 has it) and results stingers.
### WP9 — Sky Rings (ridden, open point 2)
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
  triangles), walk, run, idle, wave, mount and ride poses; the creator (open point 5) after
  the naming at a new game (and from the settings for existing saves).
### WP13 — Villagers
- Five or six: the old keeper (your guide), the Market's keeper, the Sanctuary's keeper, the
  arena's steward, a child who follows the dragons about, a traveller at the trailhead. Low
  poly, idle and talk animations, a portrait each; a dialogue box on the bottom screen with
  choices.
### WP14 — The first campaign (review R10: the outline)
- The premise (open point 3), 6–8 quests through the places and the challenges, a quest log,
  rewards (Gleam, decor, a rare egg at the end), all dialogue written for the game's warm
  tone. R10 reviews the outline before the dialogue is written.

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
- Run 14 (WP1, the technical test), run 15 (after WP5–6: flying, the map, a few places),
  run 16 (the full milestone). Bundled, with the steps written up as before.
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
WP1 → (run 14, R7) → WP3 → WP5 → WP6 → WP4 → WP7 → (run 15) → WP8 → WP9 → WP10 → WP11 →
WP12 → WP13 → WP14 → WP15 (briefs early, so the music can arrive in time) → WP16 → run 16 →
WP18. Work goes package by package, committed, with STATUS and RedWiki kept up.

## After Beta
**1.0 — *A trainer*** (sit-down first): tricks, voice commands, the training yard, the
Command Trial and Shine Show, the grooming redesign, several wanderers at once, more of the
campaign, polish and the public release.
