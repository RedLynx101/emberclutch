# Emberclutch — Sound Effects Brief, Batch 2 (hands-on care and Alpha 2)

For **ElevenLabs Sound Effects** (or Suno Sounds), like [batch 1](suno-sfx-alpha1.md).
Nothing waits on these: the game uses stand-ins until they arrive (D35). Every slug below
already has its own slot in the game (`src/app/audio`, Alpha 2 WP9), so a processed
`romfs/sfx/<slug>.wav` (and `<slug>-2.wav`... for takes) simply replaces its stand-in. Drop the files in
a folder and tell me; I match them to the slugs, trim, level and convert them
(`tools/audio/process_sfx.py`). 2–3 takes of the dragon sounds help (the game rotates).

**General notes** (same as batch 1): short and clean, no music, no long reverb tails;
dragons sound like real small animals (bird + cat + small reptile), never a human voice.

## Hands-on care (Alpha 1, WP7)
| Slug | Prompt | Kind | Length |
|---|---|---|---|
| `ball-roll` | Small rubber ball rolling across a stone floor, soft rumble | one-shot | 0.6–1 s |
| `ball-pickup` | Small animal picking up a rubber ball in its mouth, soft squeak of rubber | one-shot | 0.2–0.4 s |
| `dragon-grumble` | Small dragon grumble, mildly annoyed, short and cute, not angry | one-shot | 0.4–0.8 s |
| `dragon-sniff` | Small animal sniffing curiously, two quick sniffs | one-shot | 0.3–0.6 s |
| `dragon-giggle` | Ticklish baby dragon, bubbly chirping giggle | one-shot | 0.5–1 s |
| `leg-kick` | Happy small animal thumping its hind leg on the floor, three quick thumps | one-shot | 0.4–0.7 s |
| `tub-slide` | Small wooden tub sliding across a stone floor, then a knock | one-shot | 0.5–0.9 s |
| `suds` | Soapy sponge squishing, bubbly lather | one-shot | 0.4–0.8 s |
| `water-pour` | Water poured from a small ladle over an animal, splashing into a tub | one-shot | 0.6–1 s |
| `shake-spray` | Small animal shaking off water, droplets spraying | one-shot | 0.6–1 s |
| `egg-heartbeat` | A tiny heartbeat heard through an eggshell, one soft "lub-dub", muffled and warm | one-shot | 0.3–0.5 s |
| `egg-turn` | A large egg turned gently in a straw nest, a soft rustle and a hollow shell knock | one-shot | 0.4–0.7 s |
| `hatch-first-cry` | A newborn baby dragon's very first tiny chirp, wobbly and curious | one-shot | 0.4–0.8 s |

## Toys, den and places (Alpha 2)
| Slug | Prompt | Kind | Length |
|---|---|---|---|
| `rope-tug` | Rope creaking under a playful tug, with a small dragon's playful growl | one-shot | 0.6–1 s |
| `feather-flutter` | Soft feather fluttering through the air | one-shot | 0.3–0.6 s |
| `orb-rattle` | Wooden puzzle ball rolling with a treat rattling inside | one-shot | 0.5–0.9 s |
| `treat-drop` | Small treat dropping onto a stone floor, a soft tick | one-shot | 0.2–0.4 s |
| `bowl-clink` | Food tipped into a ceramic bowl, gentle clink | one-shot | 0.3–0.6 s |
| `nest-settle` | Small creature settling into a straw nest, rustle and a contented sigh | one-shot | 0.8–1.5 s |
| `egg-lay` | Egg gently placed in a straw nest, soft thud and rustle | one-shot | 0.4–0.8 s |
| `coin` | Warm magical coin chime, a small reward | one-shot | 0.2–0.5 s |
| `register` | Cozy fantasy shop bell ding, purchase done | one-shot | 0.4–0.8 s |
| `map-open` | Old paper map unfolding | one-shot | 0.4–0.8 s |
| `travel-whoosh` | Soft magical wind whoosh, a short flight | one-shot | 0.8–1.5 s |
| `trail-depart` | Small dragon's cheerful chirp and footsteps trotting away on a dirt path | one-shot | 1–1.5 s |
| `amb-market` | Cozy fantasy market ambience, gentle chatter, distant stalls, no music, seamless | loop | 10–20 s |
