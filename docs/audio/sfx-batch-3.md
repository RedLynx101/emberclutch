# Emberclutch — Sound Effects Brief, Batch 3 (Beta: *The valley*)

**For Noah, 2026-09-25** (Beta WP15, sent early). For **ElevenLabs Sound Effects** (or Suno
Sounds), like [batch 1](suno-sfx-alpha1.md) and [batch 2](sfx-batch-2.md). Drop the files in
a folder and tell me; I match them to the slugs, trim, level and convert them
(`tools/audio/process_sfx.py`). 2–3 takes of anything marked **×3** help (the game rotates
takes so repeated sounds don't grate).

**Batch 2 is still open too** (the hands-on care sounds, the toys and `amb-market`): the
game uses stand-ins for them. They can be made in the same sitting.

**General notes** (as before): short and clean, no music, no long reverb tails; dragons
sound like real animals (bird + cat + small reptile), never a human voice. Loops: 10–20 s,
steady, with nothing that stands out (a single loud gust or bird call is heard every time
the loop comes round); I make the seam.

None of these has a slot in the game yet: each gets one as the work that needs it arrives,
with a stand-in until its file does (D35). The first table is what step 2 (the valley,
getting about, the map and the places) needs first.

## 1. The valley and flying (step 2: needed first)
| Slug | Prompt | Kind | Length |
|---|---|---|---|
| `amb-wind-high` | Steady wind high in the sky, soft airy rush, no gusts or whistles | loop | 10–20 s |
| `amb-meadow` | Gentle breeze through long grass on a sunny day, soft distant birdsong, no voices | loop | 15–20 s |
| `amb-valley-night` | Quiet summer night in a valley meadow, soft crickets, a faint distant stream, very calm | loop | 15–20 s |
| `amb-waterfall` | A waterfall heard from nearby, steady roaring splash into a pool | loop | 10–15 s |
| `amb-stream` | A small river babbling over stones, bright and gentle | loop | 10–15 s |
| `amb-lake` | Calm lake water lapping softly at a pebble shore | loop | 10–15 s |
| `wingbeat` ×3 | One powerful downstroke of a large dragon's leathery wings, a deep whump of air | one-shot | 0.4–0.7 s |
| `takeoff` | A large dragon launching into the air: claws push off the ground, two strong wingbeats and a rush of air | one-shot | 1–1.5 s |
| `landing` | A large dragon landing on grass: a rush of air, a heavy soft thump, wings folding with a leathery rustle | one-shot | 0.8–1.2 s |
| `dive-whoosh` | Fast rushing wind whoosh as something dives steeply through the air, rising in pitch | one-shot | 1–1.5 s |
| `wing-flutter` | Leathery wing membranes fluttering in a strong wind while gliding | loop | 3–6 s |
| `water-skim` | Claws skimming the surface of a lake at speed, a spray of water | one-shot | 0.6–1 s |
| `splash-big` | A large animal plunging into a lake, a big splash | one-shot | 0.8–1.2 s |

**On the wind:** the game raises and lowers `amb-wind-high` and `wing-flutter` with speed
and height, so an even, featureless take is better than an exciting one.

## 2. On foot, and the places (step 2)
| Slug | Prompt | Kind | Length |
|---|---|---|---|
| `step-grass` ×3 | One footstep in short grass, a soft crunch, light boots | one-shot | 0.15–0.3 s |
| `step-stone` ×3 | One footstep on a stone path, a light boot tap | one-shot | 0.15–0.3 s |
| `step-wood` ×3 | One footstep on a wooden bridge or boardwalk, a hollow knock | one-shot | 0.15–0.3 s |
| `dragon-step-grass` ×3 | One step of a big clawed animal on grass, a soft padded thud | one-shot | 0.2–0.4 s |
| `mount` | Climbing onto a saddle: leather creaking, a buckle clink, a small grunt of effort | one-shot | 0.8–1.2 s |
| `door-wood` | A heavy wooden door creaking open, cozy village house | one-shot | 0.6–1 s |
| `village-bell` | A small village bell tower ringing twice, warm and distant | one-shot | 1.5–2.5 s |
| `amb-village` | A cozy village outdoors: a few distant voices (no clear words), a cart, chickens, a hammer on wood far off | loop | 15–20 s |
| `find-sparkle` | A small magical sparkle as a hidden treasure is found, bright and twinkling | one-shot | 0.4–0.8 s |

(Your own footsteps and the dragon's are mixed quietly under the music; `step` and `thump`
from batch 1 stay for the den.)

## 3. The villagers' voices
Animal Crossing–style babble: **gibberish syllables, no real words and no recognisable
language**, cute, the length of a short sentence. The game plays a snippet as each line of
dialogue types out, pitched a little per line. One voice per villager:

| Slug | Prompt | Kind | Length |
|---|---|---|---|
| `babble-keeper` | Kind elderly person speaking cute gibberish syllables, slow, warm, a little creaky, no real words | one-shot | 1.5–2.5 s |
| `babble-market` | Cheerful shopkeeper speaking cute gibberish syllables, quick and bubbly, no real words | one-shot | 1.5–2.5 s |
| `babble-sanctuary` | Gentle soft-spoken person speaking cute gibberish syllables, calm and soothing, no real words | one-shot | 1.5–2.5 s |
| `babble-steward` | Brisk confident announcer speaking cute gibberish syllables, clipped and upbeat, no real words | one-shot | 1.5–2.5 s |
| `babble-child` | Excited young child speaking cute gibberish syllables, high and fast, no real words | one-shot | 1.5–2.5 s |
| `babble-traveller` | Relaxed traveller speaking cute gibberish syllables, easy-going and drawling, no real words | one-shot | 1.5–2.5 s |

If the generator won't do gibberish well, **recording your own voice** saying nonsense
syllables in six styles works just as well (the game pitches and chops it); or one good
take could serve everyone, pitched per villager.

## 4. Challenges (later in Beta: WP8–WP11)
| Slug | Prompt | Kind | Length |
|---|---|---|---|
| `crowd-cheer` ×2 | A small friendly crowd at a village fair cheering and clapping | one-shot | 1.5–2.5 s |
| `crowd-aww` | A small friendly crowd going "aww", sympathetic | one-shot | 1–1.5 s |
| `whistle-start` | A referee's short whistle blast to start a game | one-shot | 0.4–0.7 s |
| `ring-pass` | Flying through a magical glowing ring, a bright rising chime with a whoosh | one-shot | 0.5–0.8 s |
| `lantern-light` | A crystal lantern lighting up, a soft magical ignition and a clear chime | one-shot | 0.5–0.9 s |
| `fruit-toss` | A piece of fruit tossed through the air, a short whoosh | one-shot | 0.3–0.5 s |
| `fruit-catch` | An animal snapping a fruit out of the air, a juicy chomp | one-shot | 0.3–0.6 s |
| `breath-flame` | A small dragon breathing a short puff of fire, a soft whoosh and crackle | one-shot | 0.8–1.2 s |
| `breath-mist` | A small dragon breathing out a cool spray of water mist, a soft hiss | one-shot | 0.8–1.2 s |
| `breath-gust` | A small dragon blowing a strong gust of wind, a rushing whoosh | one-shot | 0.8–1.2 s |
| `breath-spores` | A small dragon puffing out a cloud of glittering plant spores, a soft poof with a leafy rustle | one-shot | 0.8–1.2 s |
| `breath-frost` | A small dragon breathing frost, an icy hiss and crackling ice crystals | one-shot | 0.8–1.2 s |
| `breath-light` | A small dragon breathing a beam of warm magical light, a shimmering hum and chime | one-shot | 0.8–1.2 s |

## 5. The campaign (later in Beta: WP14)
| Slug | Prompt | Kind | Length |
|---|---|---|---|
| `lantern-relight` | A great festival lantern relighting: a deep warm whoosh of flame, then glowing chimes | one-shot | 1.5–2.5 s |
| `quest-page` | A page turned in a small leather journal, a quick pencil tick | one-shot | 0.4–0.8 s |
| `star-shimmer` | A falling star passing overhead, a long soft shimmering sparkle | one-shot | 1.5–2.5 s |

## Processing (what I do)
`process_sfx.py --import <folder>` copies and renames the takes, then trims, EQs for the
3DS's speakers, levels by kind and writes 22 kHz mono WAVs to `romfs/sfx/`; loops get their
seams made and are levelled by overall loudness.
