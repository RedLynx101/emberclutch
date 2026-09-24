# Alpha 2 — *A den*: Work Plan

Status: **In progress** (Alpha 1 closed 2026-09-24). WP1 done: the den of three dragons
and two eggs (D53), and their life together. WP2 done (the Sanctuary and Cold Vault), with
a first world map. WP3 done (breeding at the Nesting Stone). WP4 done (the Wanderings).
WP5 done (the Market, Gleam and the pouch). Goal (D31): complete through Alpha 2. Scope and assets:
[content & assets](content-and-assets.md) (A2 rows); screens: [screens & flow](../design/screens-and-flow.md).

**The order matters** (Noah, 2026-09-24): everything that doesn't depend on how the
dragons look comes first. The **style review (R5)** is the last thing built before Noah's
decision, and it blocks. The big dragons update follows in the chosen style, then the run
on Noah's old 3DS closes the milestone.

## Definition of done
Checked in Azahar, then once on Noah's old 3DS (D34):
1. Three dragons live in the den together (plus 2 egg nests) and play with each other and
   with toys; the Sanctuary and Cold Vault swap dragons and eggs in and out.
2. Breeding at the Nesting Stone (one male, one female) lays an egg that hatches a
   visibly distinct offspring with its parents' traits, in every breed.
3. Wanderings with the step counter bring back finds; the hoard grows; wild eggs appear.
4. The Market sells food, grooming items, toys and decor for Gleam, plus the daily
   sex-labeled egg (D24).
5. The world map fast-travels between all the Alpha 2 places.
6. The dragons are in the style Noah chose at R5, with the full parts library, all 21
   breeds, patterns, rare traits, and dirt and mud that grooming cleans (D46).
7. The new emblem icon (D48) and an animated **3D HOME Menu banner** with a baby dragon
   (D50), with a 2D banner as the fallback.
8. It installs as a CIA and runs on the old 3DS: the 3-dragon den within budget, audio,
   saves, touch.
9. Tests green, no build warnings, docs, STATUS and RedWiki updated, tagged `v0.2.0-alpha2`.

## Phase A — the den grows (style-independent)
Worked through alone, package by package (D49).

### WP1 — Several dragons
- A den of 3 active dragons plus 2 egg nests; pick which one the care screen is for.
  *Done (D53):* `src/core/den_roster.*` (beds, nests, who moves out, a ready egg waits for a
  bed; PC-tested), a second egg nest in the room, an actor per bed, the D-pad and the
  profile card's arrows to switch, a heart over the one you're with; dev menu "Add dragon"
  and "Add egg"; `tests/autotest/den3.txt`. Full den measured 7,740 top + 2,752 bottom
  triangles, 15 draws.
- Dragon-to-dragon life: play-chase, nuzzle, curling up together, sharing the sunbeam
  (the crowd avoidance and three beds exist). *Done:* `denSocial` in `src/core/behavior.cpp`
  pairs idle dragons for a chase (the more playful chases; a catch is a hop and a squeak)
  or a nuzzle face to face (hearts); by bright day one lies in the sunbeam and another may
  join; most nights two sleep side by side in the big nest. PC-tested; seen in Azahar
  (`tests/autotest/together.txt`, which logs everyone's activity per shot).
- LOD1 for background dragons (exists); the 3-dragon budget confirmed in the emulator.
- *Done:* an egg LOD (312 triangles, `egg_model.py --lod 1`) and a lighter room (1,975):
  three dragons and two eggs fit the 8,000 frame.

### WP2 — Sanctuary and Cold Vault
- Illustrated screens with grids; move dragons and eggs in and out (needs drain slowly
  and never below 50 in the Sanctuary; incubation pauses in the Vault, GDD §8).
  *Done:* `src/app/scene_storage.cpp` (the keepers' meadow and the icy vault on the top
  screen with the chosen one in 3D, `r3d::drawShowcase`; a paged grid; "To the den"),
  `storeAway` / `bringHome` in `core/den_roster` (the Vault holds 50; growth now really
  pauses in the Sanctuary); "Sanctuary" on the profile card and "To the Vault" on the egg
  screen (someone always stays home). Reached through a first world map (below, WP6).
  `tests/autotest/storage.txt`.

### WP3 — Breeding and eggs
- The Nesting Stone corner of the den; courtship nuzzle, settling on the nest, laying.
- Rules exist (`src/core/breeding.cpp`); the readiness hint on screen.
- Egg shells hinting at elements (all 6 patterns); lineage kept for the profile.
- *Done:* the Nesting Stone is a place on the map (`src/app/scene_stone.cpp`): pick her and
  him from the den's dragons, the readiness hint, "Nest together" (their nuzzle on the
  stone, `r3d::drawPair`); the pair is saved and their egg comes the next calendar day into
  a free nest or the Vault (`settleToNest`, `layDueEgg`; PC-tested), with its parents
  recorded. A hybrid egg's light drifts between its two elements' colours. Dev menu
  "Breed-ready"; scripted runs use a fixed seed. `tests/autotest/breeding.txt`.

### WP4 — Wanderings
- The step counter (PTMU), a trail map with progress, departure and return scenes.
- Finds (trinkets, Gleam), the hoard pile growing in the den, rare wild eggs, and mud
  spots on the dragon afterwards (cleaned in the bath, D46).
- *Done:* `src/core/wanderings.*` (one juvenile-or-older dragon at a time, keeping its bed;
  a chance of a find every 400 steps, more for adults and the curious; Gleam and six
  trinkets to the hoard; a wild egg now and then on long walks, mostly Grove, Frost and
  Lumen; muddy legs, belly and tail; PC-tested) and `src/app/scene_wander.cpp` (the
  trailhead, the trail map with your steps, the finds). The 3DS pedometer (`ptmu`); the
  dev menu's second page adds steps (the emulator's never counts). The hoard glints more as
  it grows; the showcase now plays the idle clip. `tests/autotest/wander.txt`.

### WP5 — Market, Gleam and the pouch
- The Market scene and shop UI (buy and sell), the pouch, Gleam; the daily egg.
- Items from the [catalogue](content-and-assets.md#7-items-catalogue); food stops being
  free.
- *Done:* `src/core/market.*` (prices, the pouch, selling trinkets, the egg of the day:
  labelled, the same all day, mostly Grove/Frost/Lumen, one a day; PC-tested) and
  `src/app/scene_market.cpp` (stalls under bunting; Food, Sell and Egg-of-the-day tabs; the
  egg shown on the stall). It opens on the map once a dragon is Juvenile. The care screen's
  food row shows the pouch (a food is used when eaten; none left sends you to the Market);
  a new game starts with 50 Gleam, bread, drumsticks, a candy and the starter's favourite.
  Toys and den decor for sale come with WP7. `tests/autotest/market.txt`.

### WP6 — World map and fast travel
- [Map & travel](../design/world-map-and-travel.md): the illustrated map, place pins that
  unlock, the travel transition.
  *Started with WP2:* `src/app/scene_map.cpp`, the valley painted in code (mountains,
  meadows, the river and lake, woods, dotted paths) with six pins (Den, Sanctuary and
  Vault open; the rest "not open yet"), a painted view of the place picked on the top
  screen, a short trip between places; X or the START menu opens it.
- *Done:* all six places built and open (the Market once a dragon is Juvenile, with its
  own "opens at Juvenile" note); the map remembers where you are, and the trip is a heart
  travelling the dotted path from there to where you're going, then a fade into the place
  (1.8 s; A or a tap skips it); picking where you already are goes straight in.
  `tests/autotest/maptrip.txt`.

### WP7 — Toys and den props
- Tug rope, feather wand, puzzle orb, the food bowl ([care interactions §8](../design/care-interactions.md#8-more-toys-alpha-2-from-the-market));
  toys stay where they're left and dragons play with them on their own.

### WP8 — Dragon profile and family tree
- Name, breed, sex, stage, personality, stats, the sweet spot once found, parents and
  grandparents.

### WP9 — Sounds
- Wire in the [sound brief 2](../audio/sfx-batch-2.md) sounds as they arrive
  (placeholders until then, D35); the Market loop and Wanderings stinger exist.

### WP10 — The app icon and a 3D HOME Menu banner (D48, D50)
- **Icon:** a designed **emblem**, independent of the dragon style: a glowing heart inside
  an ember-lit egg with a curl of wing, in the game's palette; 48×48, readable at small
  size. It replaces the AI-concept-derived placeholder and Alpha 1's interim icon.
- **3D banner** (like retail games: a small animated 3D scene on the top screen when the
  game is selected in the HOME Menu): the **baby dragon, textured**, peeking out of its
  ember-lit egg, with the heart glowing; it blinks, tilts its head, wags its tail and the
  heart pulses; the Emberclutch wordmark behind it.
  - Built by script from our own model: Blender scene → glTF → **pycgfx** (glTF to CGFX,
    textures and animation) → `bannertool makebanner -ci` with a short banner tune
    (≤ 3 s) → the SMDH's `extendedbanner` flag → the CIA (`tools/package_cia.ps1`).
  - Rules from the homebrew community: the CGFX stays **under 512 KB**; the model and its
    animation are both named `COMMON`; animate **rigid pieces with node transforms**
    (body, head, tail, wings, heart), not skinning: skinned banners that work in
    emulators have frozen the real HOME Menu. The heart's glow uses diffuse-colour
    animation.
  - The emulator doesn't run the HOME Menu (it would need Nintendo's system files), so
    the 3D banner is proven on the old 3DS (WP13). Until then, and as the fallback, the
    same scene rendered flat makes a 2D banner.
  - Tools: bannertool and makerom are already on disk (from 3D-Claw); pycgfx and its two
    Python libraries (gltflib, pillow) are downloaded at this step (approved, D50) and
    never committed.
- Both are shown to Noah at R5 (the banner as renders and a turntable).

## Phase B — the style review (R5, blocks)

### WP11 — Three Ember dragon style variants (D47)
Built last before Noah decides. The **current style, textured** (from Alpha 1), next to
three variants of the Ember dragon, fairly different from each other but all in the
game's theme (warm light inside, cute babies, majestic adults):
- **V1, surface:** today's shapes with a new surface: textures, eyes and shading.
- **V2, shape:** new proportions and shapes (head, body, limbs, wings) in a matching surface.
- **V3, bold:** a redesign of the whole look.

Each at hatchling, juvenile and adult, male and female, within the triangle and bone
budgets and animated by the same clips.
- **Review sheets and turntables** (`docs/art/reviews/R5-style.md`).
- **In the game:** a dev-menu switch shows any variant in the den and the petting
  close-up, so Noah sees them move on a 3DS screen.
- The icon and the 3D banner (WP10) are shown alongside.
- **Stop here** until Noah chooses: keep the current style, or move to a variant (or a
  mix of their best parts).

## Phase C — after the decision

### WP12 — The dragons update, in the chosen style
- Apply the chosen style to both body forms and all parts; remove the variants not chosen.
- The full parts library: horns (Crown, Crystal, Antler), frill (Leaf), tail tip (Plain),
  patterns (Solid, Runes), rare-trait looks (Iridescent, Melanistic, Leucistic,
  Starspeckle); all 6 base breeds and 15 hybrids; egg shells for all 6 elements.
- Dirt and mud on the chosen surface (D46).
- Re-export the 3D banner's baby dragon in the chosen style (the banner is scripted).
- Review sheets of every breed (R6, not blocking).

### WP13 — The run on the old 3DS (D34)
- Install the CIA with FBI (Noah's old 3DS runs Luma3DS + FBI); check the **3D banner**
  in the real HOME Menu (select it, let it
  animate, launch; fall back to the 2D banner if it misbehaves).
- Play through the checklist on the hardware: the 3-dragon den, the petting close-up,
  fetch, Wanderings, the Market, saves, sound, touch accuracy.
- Read the budget overlay on real hardware; fix whatever the hardware finds.

### WP14 — Wrap-up
- The playthrough checklist (`docs/plan/alpha-2-checklist.md`), docs sync, tag
  `v0.2.0-alpha2`.

## Review gates (D47, D49)
| Review | What | Blocks? |
|---|---|---|
| R5 — Style | The current textured style vs three variants, plus the icon and the 3D banner | **Yes**: Phase C waits for Noah's choice |
| R6 — Breeds | Every breed in the chosen style | No |
| Hardware run | WP13 on Noah's old 3DS | **Yes**: Alpha 2 ends with it (D34) |
