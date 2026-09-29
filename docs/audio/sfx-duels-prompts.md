# Emberclutch — Sound Effects Brief: roaming trainers, duels and the people's doings

For **ElevenLabs Sound Effects** (workstream D, 2026-09-28). Each slug already has its slot in the
game (`src/app/audio.hpp`: `Greet`, `Clap`, `Snore`) and a synthesised stand-in in `romfs/sfx/`
(`tools/audio/make_synth_sfx.py roamer-hello hands-clap soft-snore`), so a processed
`romfs/sfx/<slug>.wav` (and `<slug>-2.wav` ... for takes) simply replaces it
(`tools/audio/process_sfx.py`, after adding the download folder to `tools/audio/sfx_manifest.json`).
2–3 takes of each help (the game rotates them).

**General notes** (as the other briefs): short and clean, no music, no long reverb; cozy and
storybook, never harsh. People's sounds are small and friendly (the valley's people are chibi).

| Slug | Where it plays | ElevenLabs prompt | Length | Takes |
|---|---|---|---|---|
| `roamer-hello` | A roaming trainer waves as you come by (a bubble with their hello) | A cheerful friendly two-note whistle greeting, rising then falling, like a hiker calling hello across a meadow, light and breathy, no words | 0.5 s | 2 |
| `hands-clap` | A pageant show's audience applauding the results; trainers watching your battle clap | A small group of three or four people clapping warmly for a short moment outdoors, light applause, no cheering, no crowd murmur | 1.2 s | 3 |
| `soft-snore` | A villager dozing at night when you walk by (Old Rowan, Bram, Pip, Sable, Maple on her feet) | One soft cute snore from someone napping, a gentle slow breath in with a light rattle and a tiny whistle out, sleepy and sweet, not loud | 1.5 s | 2 |

The duels themselves reuse the battles' sounds (`battle-start`, `swipe`, `hit`, `victory`,
`defeat` ...); the trainers' voices are the dialogue's letter blips (voice 0 men, voice 1 women
and children).

## Music (a suggestion for Noah's Suno queue)
Duels play the battles' loop (`cup-day`) today. If they get their own, a lighter, friendlier
take than the league's:

> **Suno prompt:** "Cozy storybook adventure game, a friendly duel between two travellers on a
> sunny meadow path: bouncy pizzicato strings and marimba, light hand percussion and tambourine,
> a whistled lead melody, playful rather than epic, 112 bpm, loopable, instrumental, no vocals"

Slug suggestion `duel-day` (loop; `tools/audio/make_loop.py ... --bpm 112`), played by
`battleMusic()` while a roaming trainer's duel is on.
