# Emberclutch — Suno Music Brief (Batch 1: 5 tracks)

> **Status 2026-09-23: Batch 1 received and processed.** Six WAVs (two Skyreach takes)
> became six looping Oggs in `romfs/music/` (7.8 MB total, down from 172 MB).
>
> | Track | Used for | Real tempo | Loop | Size |
> |---|---|---|---|---|
> | title-theme | Title, menus | ~88–89 | 88.2 s | 1.27 MB |
> | den-hearth | Den by day | 75.0 | 140.8 s (44 bars) | 1.88 MB |
> | nestsong | Den at night, eggs | 63.0 | 61.0 s (16 bars) | 0.98 MB |
> | skyreach + skyreach-2 | Flight / riding, alternating so long rides don't repeat | ~129–130 | 66.6 s / 71.3 s | 0.93 / 0.97 MB |
> | cup-day | Arena, competitions | ~134.5 | 57.1 s (32 bars) | 0.81 MB |
>
> Suno ran most tracks 1–8% off the requested tempo (Skyreach came out ~130, not 120), so
> `make_loop.py` now finds the seam by matching harmony, timbre and rhythm, with no
> dependence on tempo. Every seam passed the click and level checks; a couple of level
> changes at the seam are the song's own dynamics. **To listen:** the 12-second seam previews
> are in `assets/audio/music/previews/` (6 s before the loop point, then the jump).

Everything needed to generate the first five music loops in Suno. Written 2026-09-23
against **Suno v5.5** (Custom Mode). Suno weights the **first** words of the Style field
most, so each Style prompt leads with genre and mood.

## Before you start

- **Plan:** use **Pro or Premier** while generating. Free-plan songs can't be used
  commercially, and a paid plan taken out later doesn't cover them. WAV download also
  needs a paid plan.
- **Mode:** Custom Mode, **Instrumental ON**.
- **Length:** aim for **2:00–3:00** per track. I cut the loop out of the middle, so a
  longer take gives me more clean material to work with.
- **Takes:** generate **2–4 takes per prompt** and keep the one with the clearest,
  steadiest groove. Consistent tempo matters more than a fancy ending.
- **Don't worry about the ending.** Suno likes to fade out. I cut before the fade and
  build the loop seam myself.
- **Download as WAV** (not MP3). Name each file exactly as the slug below.

### Settings for all five

| Setting | Value |
|---|---|
| Instrumental | ON |
| Weirdness | ~30% (conventional, loop-friendly) |
| Style Influence | ~75% (stick closely to the prompt) |
| Exclude Styles | `vocals, singing, lyrics, spoken word, rap, electric guitar, dubstep, trap drums, EDM drop` |

Copy each track's **Style** and **Lyrics** fields exactly. In instrumental mode the
Lyrics field holds only structure tags, which help keep the arrangement steady.

---

## 1. Title theme: "Emberclutch"
**Slug:** `title-theme` · **Plays on:** title screen, main menu · **Mood:** warm, majestic, a bit of wonder

**Style**
```
Orchestral fantasy main theme, warm and majestic, 88 BPM, D major, kalimba melody over soft strings, harp arpeggios, French horn swells, gentle hand drums, heartfelt cinematic handheld game soundtrack, instrumental, no vocals
```

**Lyrics**
```
[Instrumental]
[Intro: solo kalimba plays the main melody]
[Main Theme: strings and harp join, warm]
[Build: French horns and hand drums rise, majestic]
[Main Theme: full orchestra, soaring]
[Bridge: soft woodwinds and harp]
[Main Theme: kalimba and strings, steady]
[Instrumental Break]
```

**Keep a take that:** has a clear, hummable kalimba melody. This melody becomes the
game's leitmotif (see the tip under track 3).

---

## 2. Den by day: "Den Hearth"
**Slug:** `den-hearth` · **Plays on:** the den during the day (the most-heard track) · **Mood:** cozy, calm, safe

**Style**
```
Cozy acoustic folk ambient, 76 BPM, G major, kalimba and fingerpicked nylon guitar, soft flute, light shaker and hand percussion, warm and calm, relaxing game background loop, steady dynamics, no big climaxes, instrumental, no vocals
```

**Lyrics**
```
[Instrumental]
[Groove: kalimba and nylon guitar, gentle]
[Melody: soft flute enters]
[Groove: kalimba and nylon guitar]
[Melody: clarinet answers the flute]
[Groove: steady and warm]
[Instrumental Break]
```

**Keep a take that:** stays even and unobtrusive the whole way through. Players will hear
this for hours, so no dramatic swells or sudden stops.

---

## 3. Night den and eggs: "Nestsong"
**Slug:** `nestsong` · **Plays on:** the den at night, egg care, hatching lead-in · **Mood:** tender lullaby

**Style**
```
Gentle fantasy lullaby, 64 BPM, F major, music box and harp, felt piano, soft warm string pad, slow and tender, sleepy nighttime, very soft dynamics, instrumental, no vocals
```

**Lyrics**
```
[Instrumental]
[Intro: music box alone, soft]
[Lullaby: harp and felt piano join]
[Lullaby: warm string pad underneath]
[Lullaby: music box melody returns]
[Instrumental Break]
```

**Optional leitmotif tip:** once track 1 is done, you can instead make this track with
**Cover** on the title theme: use this Style prompt with Audio Influence around 40–50%.
That gives a lullaby version of the main melody, which ties the soundtrack together.

---

## 4. Flying / riding the valley: "Skyreach"
**Slug:** `skyreach` · **Plays on:** riding and free flight in Skyreach Valley · **Mood:** soaring, free, majestic

**Style**
```
Soaring orchestral adventure, 120 BPM, A major, sweeping strings, bold French horns, driving taiko and frame drums, bright flute runs, airy string pads, open and majestic, flying game soundtrack, instrumental, no vocals
```

**Lyrics**
```
[Instrumental]
[Intro: rising strings and flute]
[Main Theme: bold horns over driving drums]
[Soar: sweeping strings, open and bright]
[Main Theme: full orchestra, majestic]
[Glide: lighter, flute and strings over soft drums]
[Main Theme: horns return, driving]
[Instrumental Break]
```

**Keep a take that:** has a steady drum pulse. It loops much more cleanly than a take
that speeds up or drops out.

---

## 5. Competitions: "Cup Day"
**Slug:** `cup-day` · **Plays on:** the arena and competition events · **Mood:** upbeat, playful, friendly rivalry

**Style**
```
Upbeat playful festival march, 132 BPM, C major, pizzicato strings, marimba, brass fanfare stabs, snare and hand claps, tambourine, bouncy and competitive but friendly, sports minigame soundtrack, instrumental, no vocals
```

**Lyrics**
```
[Instrumental]
[Intro: brass fanfare]
[Groove: pizzicato strings and marimba, bouncy]
[Hook: brass and claps, catchy]
[Groove: marimba lead]
[Hook: full band, energetic]
[Instrumental Break]
```

**Keep a take that:** has a catchy hook and a steady beat. The fanfare intro plays only
once and is not part of the loop.

---

## When you're done

1. Save the chosen WAVs to `assets/audio/music/source/` using the slugs:
   `title-theme.wav`, `den-hearth.wav`, `nestsong.wav`, `skyreach.wav`, `cup-day.wav`.
   (That folder is git-ignored. Only processed files are committed.)
2. Tell me which ones are ready. If a take clearly drifted from its BPM, mention it.

## What I do with them

Using `tools/audio/make_loop.py` (ffmpeg):

1. **Pick loop points on bar lines**, after the intro and before any fade.
2. **Blend the seam:** the end of the loop crossfades into the audio just before the loop
   start, and the outgoing side is **muffled with a low-pass filter** as it fades. The
   jump back becomes a soft blend instead of a click.
3. **Level everything the same** (about −16 LUFS, suited to handheld speakers) with a
   true-peak limit.
4. **Shrink it:** 32 kHz Ogg Vorbis at about 96 kbps. A ~30 MB WAV becomes roughly
   1–2 MB. A `--mono` option halves that for tracks that don't need stereo. The intro plays once and the loop point is stored in the file
   (`LOOPSTART` / `LOOPLENGTH` tags) so the game loops sample-accurately.
5. Output goes to `romfs/music/<slug>.ogg` and is streamed from the game's read-only
   filesystem, never loaded whole into memory.

## Licensing note

On a paid plan Suno grants commercial-use rights, but its current terms say users are
generally **not** the owners of generated songs. So the music cannot go under the game's
CC BY-SA art license. It will ship with its own notice
(`assets/audio/music/LICENSE-MUSIC.md`), and forks may need to replace it. Worth
re-checking Suno's terms before the public release. Suno has said it will deprecate its
current models when new licensed ones launch.

## Sources

- [Suno guide: tags, meta tags & prompts (v5.5)](https://blakecrosley.com/guides/suno)
- [Suno meta tags guide 2026](https://jackrighteous.com/en-us/pages/suno-ai-meta-tags-guide)
- [Suno commercial use: rights & ownership 2026](https://techjacksolutions.com/ai-tools/suno/suno-commercial-use/)
- [Suno previews 2026 changes under the Warner deal](https://www.digitalmusicnews.com/2025/12/22/suno-warner-music-deal-changes/)
