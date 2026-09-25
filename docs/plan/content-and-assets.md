# Content & Asset Inventory

Everything the game needs, tagged by milestone: **A1** Alpha 1 · **A2** Alpha 2 ·
**B** Beta · **1.0** · **1.x** · **2.0**. Counts are targets, not caps. Budgets come from
[architecture §1](../tech/architecture.md); the look comes from
[theme & art direction](../design/theme-and-art-direction.md).

**Sourcing rule:** models, rigs, animations and textures are made with scripted Blender
(headless, in `tools/blender/`), so they're original and CC BY-SA. Music and most sound
effects come from Suno (own license notice). AI concept images are reference only.

## 1. The dragon

### 1.1 Base body — A1
- One mesh topology for every dragon. Growth stages are **bone-scale tables** plus a few
  shape keys (snout length, belly roundness) blended by `growth01`.
- Stage shapes: hatchling, juvenile, adolescent, adult. (The egg is its own model.)
- LOD0 (close-up, petting) and LOD1 (den with 3 dragons, valley distance).
- Budgets: LOD0 ≤ 3,000 tris for every stage (one mesh), LOD1 ≤ 1,200; ≤ 24 bones per draw.

### 1.2 Parts library
| Part | Variants | Milestone |
|---|---|---|
| Build (bone-scale presets) | Sturdy, Sleek, Long | A1 all three (starters need them) |
| Horns | Swept, Nubs | A1 (starters) |
| Horns | Crown, Crystal, Antler | A2 |
| Frill | None, Fin, Feather | A1 (starters) |
| Frill | Leaf | A2 |
| Wings | Membrane, Fin, Feathered | A1 (starters) |
| Tail tip | Spade, Fan, Tuft | A1 (starters) |
| Tail tip | Plain | A2 |
| Pattern masks | Stripes, Dapple, Spots | A1 (starters) |
| Pattern masks | Solid, Runes | A2 |
| Rare-trait looks | Iridescent (specular LUT), Melanistic, Leucistic (constants), Starspeckle (sparkle mask) | A2 |
| **Sex differences (D23)** | Males: horn and crest bones scaled up ~15%. Females: tail-fan bones longer, accent sheen brighter. No extra meshes | A1 |
| Mood extras | Sulk: droopy wings/ears pose; Joyful: sparkles | A1 |

### 1.3 Textures — A1 (starters), A2 (rest)
- Per part family: one 128×128 **mask texture** (R base, G accent, B pattern, A heartglow),
  with the painted value (baked procedural scales + AO) in the shading.
- Eyes: one shared eye texture (iris tinted by genome).
- Egg shells: one shell mask + 6 speckle/swirl patterns (A1: the 3 starters; A2: all 6).

### 1.4 Animations
Shared by all dragons and retargeted across stages by the same rig.

| Group | Clips | Milestone |
|---|---|---|
| Idle & locomotion | idle breathe, idle look-around, idle scratch, walk, trot, turn in place | A1 |
| Rest | sit, lie down, curl up to sleep, sleep loop, wake + stretch, yawn, nap flop | A1 |
| Care reactions | eat (chomp + swallow), favorite-food wiggle, pet head (lean in), pet chin (eyes closed), belly rub (roll over), groom shake-off, happy hop | A1 |
| Play | pounce, tail wag, fetch: pick up, carry, drop and sit-wait, leap-catch | A1 |
| Hands-on care ([care interactions](../design/care-interactions.md)) | lean into petting, sweet-spot leg kick, sniff and refuse food, lift a wing (brushing), sit up (belly), hop into the tub | A1 |
| Toys | tug, paw bat (feather), nudge a ball, carry a toy to bed | A2 |
| Feelings | sulk (turn away, lie in nook), make-up nuzzle, "missed you" greeting, look at the player (name called) | A1 |
| Egg & hatching | egg wiggle, egg crack, hatch emerge, first blink | A1 |
| Den life | sniff, play-chase another dragon, nuzzle another dragon, curl around nest | A2 |
| Breeding | courtship nuzzle, settle on the nest | A2 |
| Wanderings | depart (wave-off), return with a find | A2 |
| Tricks | sit, roar, lie down, spin, beg, wing spread, tail chase, bow, breath puff | B |
| Competition | leap-catch, dive-catch, show pose ×3, victory, "aww" (lost) | B |
| Flight | takeoff, flap loop, glide, bank L/R, dive, hover, land, loop-de-loop, fly up (trick) | 1.0 |
| Breath | breath flare (one clip; effect varies by element) | 1.0 |
| Riding | carry-walk, carry-run, carry-fly variants, mount/dismount kneel | 1.0 |

About 25 clips for A1, ~40 by Beta and ~55 by 1.0. Clips are 30 Hz with event markers
(footstep, chomp, flap, breath).

### 1.5 Behavior (not art, but content)
Den wander points, nap spots, the sulk nook, reaction priorities, and personality flavors
(Brave, Shy, Playful, Proud, Sleepy, Curious). A1 needs the single-dragon set; A2 adds
dragon-to-dragon behaviors.

## 2. Other characters and models
| Asset | Milestone |
|---|---|
| Egg model (shared, tinted) + cracked shell halves | A1 |
| Tool cursors (2D, drawn at the stylus): hand, brush, cloth, sponge, ladle, each food | A1 |
| Ball toy (3D, fits the jaw); bath tub (3D) | A1 |
| Feather wand; tug rope; puzzle orb; food bowl | A2 |
| **Player character** (D73): one cute-proportioned model and rig (~500 triangles), walk, run, ride poses; a creator (2 body shapes, 6 hair styles and colours, 5 skin tones, 3 outfit colours) | B |
| **Villagers** (D73): the old keeper, the Market's and Sanctuary's keepers, the arena's steward, a child, a traveller; low poly, idle and talk, a portrait each | B |
| Judge and crowd (competitions): 2D cutouts or very low-poly | B |
| Market keeper (2D portrait) | A2 |
| Equine base mesh and rig (reuses wing module) | 2.0 |

## 3. Environments
| Scene | Contents | Milestone |
|---|---|---|
| **Den** | Cave room, round opening to the sky, 2 nests, rug, hearth, hoard pile, shelves, sulk nook; day, evening and night lighting (vertex-color sets) | A1 |
| Nesting Stone | Corner of the den: a warm stone ring | A2 |
| Market | Stall with awnings (top screen) + shop UI (bottom) | A2 |
| Sanctuary / Cold Vault | Illustrated 2D screens, no 3D scene | A2 |
| Wanderings | 2D trail map with step progress | A2 |
| Training yard | Grassy clearing outside the den, props for tricks | B |
| Arena | One arena reused by events with different props (judge stand, fruit stand, show podium) | B |
| **Skyreach Valley** (D73) | About 1 km across: height-field tiles streamed from romfs, lake, river, cliffs, floating islands, trees and rocks, sky dome with the day's light, fog; the places standing in it (den cliff and waterfall, Market village, Nesting Stone hilltop, Sanctuary meadow, Cold Vault cave, trailheads, arena) | B |
| Sky Rings course | Ring props in the valley | B |
| Lantern Trial | Crystal lanterns (react to any element) | B |

## 4. Effects (particles and shaders)
- A1: embers (den), heartglow pulse, sparkles (joy, polish), hearts (petting), crumbs
  (eating), Zzz (sleep), dust puff (landing), egg glow and crack light.
- B: trick sparkle, ribbon burst, cup confetti.
- B (D73; were 1.0): breath effects ×6 (flame, mist, gust, spore bloom, frost, sunbeam), wind
  streaks, water splash, cloud wisps.

## 5. UI art
- A1: eggshell panels, ember need gauges (4 icons: drumstick, moon, sparkle, ball),
  heartglow orb, element icons ×6, sex icons, personality icons ×6, buttons,
  Nunito + Cinzel Decorative fonts (BCFNT), the title wordmark, an **original app icon**
  (48×48) and **HOME Menu banner** (+ banner sound).
- A2: item icons (see §7), Sanctuary grid tiles, egg icons ×6 (+ hybrid swirl),
  family-tree widget, Gleam coin icon.
- B: cup badges (4 tiers × 5 events), ribbons, trick icons ×12.
- A2: the illustrated world map with place pins ([map & travel](../design/world-map-and-travel.md)),
  the tool tray icons.
- B (D73; were 1.0): the painted valley map (top-view render, stylised, pins, fog of the
  unexplored), compass, ring and lantern markers, dialogue box and portraits, quest log.

## 6. Audio

### 6.1 Music
| Track | Use | Status / milestone |
|---|---|---|
| title-theme | Title, menus | ✅ Batch 1 |
| den-hearth | Den by day | ✅ Batch 1 |
| nestsong | Den at night, eggs | ✅ Batch 1 |
| skyreach, skyreach-2 | Flight and riding | ✅ Batch 1 |
| cup-day | Arena and competitions | ✅ Batch 1 |
| **Hatching** (stinger, no loop, ~10 s) | Egg hatches | Batch 2 — A1 |
| **Market bustle** (loop) | Market | Batch 2 — A2 |
| **Wanderings return** (stinger) | Finds revealed | Batch 2 — A2 |
| **valley-day**, **valley-night** (loops), **place-found** (stinger) | Exploring the valley | [Batch 3](../audio/suno-music-batch-3.md) — B, first |
| **village-green** (loop) | The Market village outside | Batch 3 — B |
| **results-first**, **results-placed**, **results-try-again**, **cup-won** (stingers) | The arena's results | Batch 3 — B |
| **quest-done** (stinger), **lantern-festival** (loop, optional) | The first campaign | Batch 3 — B |
| **Training yard** (loop, light) | Training | Batch 4 — 1.0 (D73) |
| **Evening den** (loop, optional) | Dusk transition | Batch 4 — 1.0 |

Each batch gets a Suno brief in `docs/audio/` before its milestone starts. Stingers go
through `make_loop.py --no-loop` (trim, level, encode).

### 6.2 Sound effects (generated: ElevenLabs or Suno; CC0 fallback)
| Group | Sounds | Milestone |
|---|---|---|
| Dragon voice | chirp, trill, purr, happy squeak, sad whimper, yawn, sneeze, rumble, roar (each pitch-shifted per dragon and deepened by stage) | A1 (roar B) |
| Egg | wiggle knock, crack, hatch pop, warm hum | A1 |
| Care | munch, gulp, brush strokes, polish sparkle, splash, ball bounce (✅ 2026-09-24) | A1 |
| Hands-on care ([brief 2](../audio/sfx-batch-2.md)) | ball roll, ball pick-up, grumble, sniff, giggle, leg-kick thump, tub slide, suds, water pour, shake spray | A1 |
| Toys, den, places ([brief 2](../audio/sfx-batch-2.md)) | rope tug, feather flutter, orb rattle, treat drop, bowl clink, nest settle, egg lay, coin, register, map open, travel whoosh, trail depart, market ambience | A2 |
| UI | tap, confirm, back, error, toast, save chime, Gleam coin | A1 |
| Den ambience | hearth crackle loop, night crickets loop | A1 |
| Den life / Market | nest settle, market chatter loop, register ding (in brief 2) | A2 |
| The valley and flying ([brief 3](../audio/sfx-batch-3.md)) | high wind, meadow, valley night, waterfall, stream and lake loops; wingbeat, take-off, landing, dive whoosh, wing flutter, water skim, big splash | B, first |
| On foot and the places ([brief 3](../audio/sfx-batch-3.md)) | footsteps (grass, stone, wood; the dragon's), mounting, a wooden door, the village bell and ambience, a find's sparkle | B |
| Villagers ([brief 3](../audio/sfx-batch-3.md)) | one recorded alphabet, voiced letter by letter and pitched per speaker (D75) | B |
| Competition ([brief 3](../audio/sfx-batch-3.md)) | whistle, crowd cheer, crowd "aww", ring pass chime, lantern ignite, fruit toss and catch | B |
| Breath ([brief 3](../audio/sfx-batch-3.md)) | flame, mist, gust, spore burst, frost, light | B (Lantern Trial) |
| Campaign ([brief 3](../audio/sfx-batch-3.md)) | a festival lantern relit, the quest log's page, a falling star | B |

## 7. Items catalogue
Prices and exact effects are balanced in Beta; this is the content list.

| Category | Items | Milestone |
|---|---|---|
| Foods (breed favorites) | Firepepper (Ember), River fish (Tide), Skyberry (Gale), Honeyroot (Grove), Frostmelon (Frost), Starfruit (Lumen) | A1 (all six exist as foods; favorites matter from A1) |
| Foods (basic) | Hearth bread (cheap, fills Belly), Roast drumstick (fills more) | A1 |
| Treats | Ember candy (a make-up helper: counts as a favorite), Glimmer cookie (small bond boost, limited per day) | A1 |
| Grooming | Brush (Shine), Polish cloth (Shine + sparkle), Bath bucket (Shine; Tide loves, Ember hates) | A1 |
| Toys | Ball | A1 |
| Toys | Feather wand, Tug rope, Puzzle orb, Food bowl | A2 |
| Toys | Flying disc | 1.0 |
| Den decor | Rugs ×4, lanterns ×3, perches ×2, plants ×3, banners ×3 | A2 |
| Nest upgrades | Warm stones (egg Warmth drains slower) | A2 |
| Trophies | Ribbons and cups (earned, shown in the den) | B |
| Wanderings finds | Shiny pebble, old coin, feather, crystal shard, pearl, fossil tooth (sell for Gleam or add to the hoard); rare: wild egg | A2 |
| Eggs | Market daily egg (sex-labeled, D24) | A2 |

In A1 food is free and unlimited (no economy yet); Gleam arrives in A2.

## 8. Text
- English only for v1. All player-facing strings go through one string table
  (`src/app/strings.hpp`) so a translation can be added later without code changes.
- Name suggestions list: ~200 dragon names (A1).
- Tutorial lines (A1: egg, hatching, first care; later milestones add their own).
