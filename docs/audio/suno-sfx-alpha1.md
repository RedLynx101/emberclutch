# Emberclutch — Sound Effects Brief (Alpha 1)

> **Delivered 2026-09-24**, generated with ElevenLabs Sound Effects (paid plan) rather
> than Suno, 2–4 takes for most sounds. `tools/audio/sfx_manifest.json` records which
> generated file became which take; `tools/audio/process_sfx.py` makes `romfs/sfx/`. The
> placeholders (D35) are gone. Loaded but not played yet (no moment in the game for them
> until WP7 and WP10): `splash`, `ui-back`, `ui-toast`.

For **Suno Sounds** (one-shots and short loops). Drop finished files into
`assets/audio/sfx/source/` using the slugs below; I trim, level, and convert them to small
22 kHz mono WAVs in `romfs/sfx/`.

**General notes**
- Short and clean: no music under the sound, and no long reverb tails unless noted.
- Dragons: aim for a **juvenile-sized** creature (between a kitten and a big dog). The
  game pitches voices **up for babies and down for adults**, and varies them slightly
  per dragon, so one good take covers every stage.
- Cute but not cartoonish. Dragons should sound like real small animals (think bird +
  cat + small reptile), never like a human doing a voice.
- 2–3 takes of the voice sounds help: the game can rotate between them.

## Dragon voice
| Slug | Prompt | Kind | Length |
|---|---|---|---|
| `dragon-chirp` | Small baby dragon chirp, bright and curious, bird-like with a soft reptile rasp | one-shot | 0.3–0.6 s |
| `dragon-trill` | Happy baby dragon trill, rolling bubbly chirps, delighted | one-shot | 0.5–1 s |
| `dragon-purr` | Contented dragon purr, warm rumbling purr like a big cat, gentle | loopable one-shot | 1–2 s |
| `dragon-squeak` | Excited little dragon squeak, playful, short | one-shot | 0.2–0.4 s |
| `dragon-whimper` | Sad small dragon whimper, soft and sulky, not distressed | one-shot | 0.6–1 s |
| `dragon-yawn` | Sleepy small dragon yawn, soft squeaky yawn ending in a little sigh | one-shot | 1–1.5 s |
| `dragon-sneeze` | Tiny dragon sneeze with a puff of smoke, cute | one-shot | 0.3–0.6 s |
| `dragon-rumble` | Low friendly dragon rumble, grown-up and calm, like a greeting | one-shot | 0.8–1.5 s |

## Dragon body (animation events)
| Slug | Prompt | Kind | Length |
|---|---|---|---|
| `step` | Single soft padded footstep of a small clawed animal on a stone floor with straw | one-shot | 0.1–0.2 s |
| `thump` | Small animal flopping down to lie on a rug, soft body thump | one-shot | 0.3–0.5 s |
| `flap` | One leathery wing flap, a soft whoosh of air | one-shot | 0.3–0.5 s |

## Egg and hatching
| Slug | Prompt | Kind | Length |
|---|---|---|---|
| `egg-knock` | Egg wiggling in a straw nest, soft knocks and rustle from inside | one-shot | 0.5–1 s |
| `egg-crack` | Eggshell cracking, crisp small cracks | one-shot | 0.3–0.6 s |
| `egg-hatch` | Eggshell bursting open with a soft pop and scattering shell pieces | one-shot | 0.6–1 s |
| `egg-hum` | Warm magical hum of a glowing egg, soft and low | short loop | 2–4 s |

## Care
| Slug | Prompt | Kind | Length |
|---|---|---|---|
| `munch` | Small animal munching crunchy food, two or three bites | one-shot | 0.4–0.8 s |
| `gulp` | Cute swallow gulp | one-shot | 0.2–0.4 s |
| `brush` | Soft brush strokes on scales, gentle swishing | one-shot | 0.4–0.7 s |
| `polish-sparkle` | Magical sparkle shimmer, polishing something shiny | one-shot | 0.5–1 s |
| `splash` | Small playful water splash in a wooden tub | one-shot | 0.5–1 s |
| `ball-bounce` | Soft rubber ball bouncing on a rug, two bounces | one-shot | 0.4–0.8 s |

## Interface
| Slug | Prompt | Kind | Length |
|---|---|---|---|
| `ui-tap` | Soft wooden tap, gentle UI click | one-shot | < 0.1 s |
| `ui-confirm` | Warm two-note chime going up, confirm | one-shot | 0.2–0.4 s |
| `ui-back` | Warm two-note chime going down, cancel | one-shot | 0.2–0.4 s |
| `ui-error` | Soft low muted bonk, gentle "not now" | one-shot | 0.2–0.4 s |
| `ui-toast` | Tiny bright notification twinkle | one-shot | 0.2–0.4 s |
| `ui-save` | Small ember whoosh and chime, saving | one-shot | 0.4–0.8 s |

## Den ambience
| Slug | Prompt | Kind | Length |
|---|---|---|---|
| `amb-hearth` | Crackling hearth fire in a cozy cave, gentle, seamless | loop | 10–20 s |
| `amb-night` | Night crickets and soft distant wind outside a cave, calm, seamless | loop | 10–20 s |

## Licensing
Generated sounds made on a paid plan fall under the same notice as the music
(`assets/audio/music/LICENSE-MUSIC.md`, which lists them).
