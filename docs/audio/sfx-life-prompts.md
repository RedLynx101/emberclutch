# Emberclutch — Sound Effects: the valley's critters (1.0, workstream L)

For **ElevenLabs Sound Effects** (`POST /v1/sound-generation`: `text`, `duration_seconds`,
`prompt_influence` about 0.5). Each slug already has a slot (`src/app/audio.hpp`, the critters'
block) and a synthesised stand-in in `romfs/sfx/` (`tools/audio/make_synth_sfx.py`), so the game
plays now; a generated take replaces its stand-in once processed with
`tools/audio/process_sfx.py` (add the files to `tools/audio/sfx_manifest.json`, then
`--import <folder>`). **×2 / ×3**: that many takes (the game rotates them, so repeats don't grate).

**General notes** (as the other batches): one clean sound per file, mono-friendly, no music, no
voices, no long reverb tails, soft and cute rather than realistic-harsh; they're heard small, a
few metres off, under the meadow's own bed. The critters are the storybook kind: nothing is ever
hurt or frightened for long (a rabbit giggling off into a bush, not fleeing a predator).

| Slug | Prompt | Length |
|---|---|---|
| `bird-chirp` ×3 | A small songbird's cheerful little twitter in a sunny meadow, two or three quick bright chirps, close and clean, no other birds | 0.3 s |
| `bird-flutter` | A little flock of small birds bursting up off the grass together, a soft flurry of tiny wingbeats fading as they fly away, a startled peep | 0.8 s |
| `rabbit-hop` ×2 | A rabbit bounding away over soft grass, two or three light padded thumps, quick and gentle | 0.35 s |
| `frog-croak` ×2 | One small cute pond frog's round "ribbit", a short throaty double croak, clean, no other frogs or water | 0.35 s |
| `duck-quack` ×2 | A friendly farm duck's single soft quack on a calm lake, short and gentle, not loud | 0.3 s |
| `fox-yip` | A curious young fox's soft short yip, a bright little bark-chirp, playful and small, more puppy than wolf | 0.3 s |
| `butterfly-land` | A tiny magical sparkle as a butterfly settles, three soft high chime notes floating down, very delicate | 0.6 s |
| `whistle-call` | A person whistling softly to call birds, a clear friendly two-note whistle rising, breathy, no melody beyond that | 0.6 s |
| `critter-friend` | A short warm happy jingle for making a new animal friend, three soft kalimba notes skipping up and a gentle bell | 1.0 s |
| `leaf-rustle` | A small animal diving into a leafy bush, a quick soft rustle of leaves and twigs, then still | 0.5 s |

**Where they play** (`src/app/wildlife.cpp`): the chirps, croaks and quacks now and then while
you're near (spaced out, softer with distance; a duckling's quack is the same file pitched up),
`bird-flutter` when a flock scatters, `rabbit-hop` and `leaf-rustle` for a rabbit or snow hare
bolting and diving into cover, `whistle-call` when you whistle to the birds or call the ducks,
`frog-croak` pitched low when you croak back at a frog, `fox-yip` when the fox boops noses with
your dragon, `butterfly-land` when one lands on its head, and `critter-friend` with every
befriend. A frog's leap into the water uses the existing `splash`, pitched up.
