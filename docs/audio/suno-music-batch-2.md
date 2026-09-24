# Emberclutch — Suno Music Brief, Batch 2 (3 cues)

Same setup as [batch 1](suno-music-brief.md): **Suno v5.5, paid plan, Custom Mode,
Instrumental ON**, Weirdness ~30%, Style Influence ~75%, the same Exclude Styles list.
Download **WAV** and drop the files in `assets/audio/music/source/` with the slugs below.

Batch 1 lesson: Suno doesn't hold the requested tempo exactly. That's fine: the loop tool
finds the seam by ear-like matching. For **stingers** (one-shots), a strong, clean
*ending* matters more than tempo.

| # | Slug | Kind | Needed by |
|---|---|---|---|
| 1 | `hatching` | Stinger (plays once, ~8–15 s) | Alpha 1 |
| 2 | `market-bustle` | Loop | Alpha 2 |
| 3 | `wanderings-return` | Stinger (~5–8 s) | Alpha 2 |

Until these exist, the game stays quiet at those moments (the code is already wired up).

---

## 1. "Hatching" — the egg cracks open
**Slug:** `hatching` · **Plays:** once, over the hatching moment, while the den loop ducks
underneath · **Mood:** wonder → joy, a tiny miracle

**Style**
```
Magical orchestral fanfare, short, wonder and joy, music box twinkle rising into warm strings and a gentle French horn swell, harp glissando, soft chimes, ends on a bright sustained major chord, cinematic handheld game cue, instrumental, no vocals
```

**Lyrics**
```
[Instrumental]
[Intro: music box twinkles, hushed]
[Build: harp glissando and strings rise]
[Climax: warm French horn swell, bright major chord]
[End: chord rings out and fades]
[End]
```

**Keep a take that:** is short (under ~15 s is ideal; I trim the lead-in and keep the
natural ending) and **ends on a clean, ringing chord** rather than cutting off.

---

## 2. "Market Bustle" — the Market
**Slug:** `market-bustle` · **Plays:** loop at the Market · **Mood:** friendly, busy, a
little mischievous

**Style**
```
Cheerful fantasy market town music, 104 BPM, F major, bouncy acoustic guitar and mandolin, playful clarinet melody, hand drums and tambourine, pizzicato strings, friendly and bustling, cozy game shop loop, steady groove, instrumental, no vocals
```

**Lyrics**
```
[Instrumental]
[Groove: acoustic guitar and mandolin, bouncy]
[Melody: playful clarinet]
[Groove: hand drums and tambourine]
[Melody: pizzicato strings answer the clarinet]
[Groove: full band, steady]
[Instrumental Break]
```

**Keep a take that:** grooves steadily without big dramatic changes (it's a shop).

---

## 3. "Wanderings Return" — back from a walk with treasures
**Slug:** `wanderings-return` · **Plays:** once, as the finds are revealed · **Mood:**
cheerful "look what I found!"

**Style**
```
Short cheerful discovery jingle, kalimba and glockenspiel melody over light strings, playful hand percussion, rising to a sparkling finish, treasure found, handheld game reward cue, instrumental, no vocals
```

**Lyrics**
```
[Instrumental]
[Intro: kalimba pickup]
[Melody: glockenspiel and kalimba, bouncy]
[Finish: sparkling rise, bright final note]
[End]
```

**Keep a take that:** finishes clearly on a bright final note in under ~8 s.

---

## Processing (what I do)
- Stingers: `make_loop.py <wav> --no-loop` trims the lead-in, keeps the natural ending,
  matches loudness, and encodes Ogg (no loop tags).
- Loops: the normal `make_loop.py` seam search, as in batch 1.
