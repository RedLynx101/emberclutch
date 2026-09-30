# The Living Valley pass (plan)

*Planned 2026-09-30 from Noah's notes; nothing built yet. The story itself (lore, cast, quests,
letters, feelings) is in `docs/design/story.md`, and the storyboard page takes Noah's Keep / Change
marks. Once he signs off, the approved parts become decisions (D135 on) and this pass builds them in
one go; then run 22 on the 3DS with a rewritten review page.*

Noah's aim: "I want the world to feel better, the people more alive, the game more fun, and give us
something to more easily build off of later."

## The workstreams

### 1. The player build (a separate version for everyone else)
Noah: "a version of the updated game without the debug stuff or Y screenshots ... that's the version
that we'll point other players to in the repo, and in releases."

- One codebase and two build flavours. `make DEV=0` already drops the dev menu, the budget overlay,
  the stylus crosshair and the autotests. The player build also drops:
  - Y screenshots (today they work in every build);
  - the tracer (`trace.on`, `trace.txt`, `hangs.txt`), compiled out;
  - the watchdog (or it stays as a quiet crash note for bug reports: Noah's call; default out);
  - the "1.0 preview" label (the title shows the version).
  - Photo mode stays: it's a feature, not a debug tool.
- Separate build folders (`build/` for dev, `build-player/`), so switching flavours never needs a clean build.
- `tools\package_cia.ps1 -Player` builds the player flavour into `dist/player/`: `emberclutch.cia`,
  `emberclutch.3dsx` and a zip for a release.
- Same title ID and save folder as the dev build, so a player's save and Noah's move freely between them.
- The README's download section points players to Releases (the player build). Publishing a GitHub
  release waits for Noah's word each time.
- Every checkpoint builds both. The player build gets a boot check in the emulator (it starts, reaches
  the title and runs for a minute without a crash); the autotests run on the dev build.
- The dev build stays what goes to Noah's 3DS for review.

### 2. Growing up faster
Noah: "change 2 weeks of active management for fully grown dragon to 5.5 days (assuming optimal
growth). And 1.6 days per egg?"

| | Today | Proposed |
|---|---|---|
| Egg (warm time) | 24 h | 1.6 days (38.4 h) |
| Juvenile | day 4, 6 stars | 1.5 days, 3 stars |
| Adolescent | day 8, 14 stars | 3.25 days, 7 stars |
| Adult | day 14, 26 stars | 5.5 days, 12 stars |

- Stars still come a day at a time (0-3 each). About five days close in 5.5, so 12 stars means
  near-perfect care; a dragon cared for less grows up later, never earlier.
- The day gates become hours (`stageMinHours`), so half days work; `stageProgress` and `bodyScale`
  follow the new spans.
- The design doc (`game-design.md`: "about two weeks") and D5 change with it; the tests' growth checks move.
- To confirm: 5.5 days counts from hatching (a new egg to grown is then about 7.1 days).

### 3. The looks: the chosen set on everyone
Noah: "Let's go with set for the looks for now. Please implement that for all characters and new ones."
(Which set is still to confirm; see the questions at the end.)

- Rebuild all eight bodies in the chosen style in `tools/people/people.py` (players A and B, Rowan,
  Maple, Bram, Wren, Pip, Sable), keeping each one's look and clothes, and remake the creator's six hair
  styles and colours for it. The challengers and roamers use the player bodies with their own colours,
  so they follow.
- New bodies: Fig, Celestine, Linnet, Tam, Tove, and the four champions (Marigold; Captain Rook with his
  coat and eyepatch; Seraphine; Solenne). Primrose too, if the rival is in.
- Everyone stays under 600 triangles for body, face and hair.
- New portraits for everyone, one per feeling family (calm, happy, sad, angry, surprised), rendered from
  the models (`people_model.py --portraits`).
- `tools/people/candidates.py` becomes the style's source, and the candidate that isn't chosen is removed.

### 4. The feelings kit (faces, bodies, emotes, voices, sounds)
Noah: "more animations for bodies/faces to show emotion, synthesize sound effects for them (all a lot
like animal crossing) and have different eye animations based on emotion".

- **Eyes**: the eyes part group grows from 2 variants (calm, surprised) to about 10: calm, smiling
  arcs, shut, wide, droopy (sad), slanted (angry), half shut (sleepy), hearts, sparkles, spirals.
  Only one variant is drawn at a time, so it costs no triangles on screen. Eye animations per feeling:
  a happy squint, a surprised pop (scale up and settle), slow sleepy blinks, dizzy spirals turning,
  hearts pulsing, an angry twitch.
- **Mouth and brows**: out of the body mesh into part groups of their own: mouths (smile, open smile,
  wide open, "o", frown, flat, pout, smirk, wobble, yawn) and brows (calm, raised, angry, worried).
- **Bodies**: about 16 new person clips in `tools/anim/person_clips.py`: laugh, giggle, gasp, sigh,
  pout, stomp, shy sway, think, shrug, facepalm, bounce, yawn, sniffle, happy dance, a dramatic swoon
  (Celestine), a cool lean (Rook).
- **Emotes**: little icons that pop over a person's head (! ? hearts, notes, sweat drops, an anger
  mark, sparkles, zzz, a gloom cloud, a light bulb, "...", tears), drawn in the 3D view with depth.
- **Voices**: each line's feeling tunes the voiced blips (pitch, speed, loudness and wobble: higher and
  quicker when happy, lower and slower when sad, a shake when scared).
- **Sounds**: about 20 short emote sounds made by a small synthesizer (`tools/audio/synth_emotes.py`,
  plain Python): pops, chimes, giggles, droops, puffs, sparkles, gulps, a yawn whistle, a "hm?". Soft and
  round, in the Animal Crossing spirit. No ElevenLabs credits needed.
- **Dialogue**: lines carry a feeling tag (`[happy] ...`); the box strips it and drives the face, clip,
  emote, sound and voice, and the portrait changes with it.
- **Living idles**: people turn and wave as you pass, react to what happens near them (Celestine
  gasps at a muddy dragon, Pip cheers a grown one), and each has an idle habit (story.md, section 8).

### 5. The story engine (quests, talks and letters as data)
The piece that makes the valley easy to build on.

- **Story scripts**: quests, talks and letters are written in plain text files in `story/`, and
  `tools/story/build_story.py` compiles them into `src/core/story_data.inc` (the way the dragons'
  tables are generated today). A sketch:
  ```
  quest market_day "Market day"
    line main
    giver maple
    after keepers_apprentice
    start talk maple
    step "Find the Market village and meet Maple"  place market
    step "Find Fig in the Whisperwood"              talk fig at whisperwood
    step "Win a Fruit Catch at the orchard"          cup fruit_catch
    step "Light the Market's lantern with Fig"      lantern market
    finish talk maple
    reward gleam 80, friend fig

  talk maple
    when market_day.step 2
      [worried] Still no Fig? Try the Whisperwood, past the old road.
    when market_day.done
      [laugh] Asleep in the woods! Of course he was.

  letter rowan_hello from rowan
    when hatched_first and not met_rowan
    [happy] Dear new keeper, word travels fast in a small valley...
    starts keepers_apprentice
  ```
- **Quests**: any number of steps (up to 6), started by a talk, a letter or an event (never silently),
  finished by going back to someone, with rewards (Gleam, items, things to wear, letters, friends).
  Step kinds: talk to someone (anywhere, or at a place), reach a place, light a lantern, win a cup, win
  a show or a league title, beat a challenger or champion, reach a Hollow floor, catch a fish (a kind, or
  any), pick up things (map pages, a shell, honey), give an item, breed, have a grown dragon, a flag,
  real days since something.
- **Talks**: each person's lines are rules (first match wins, most specific first) that can move a
  quest, set a flag or send a letter.
- **Letters**: a mailbox by the den's door, letters that arrive on a trigger and a day, read on the
  bottom screen (the picnic letter's card, grown up), some with a gift. "News from the valley" letters
  announce what a new version adds.
- **The Journal**: quests grouped by line (main, pageant, league, Hollow, cove, errands), each step
  saying who wants it; a Rumours list of quests waiting to be asked about ("Maple looked worried about
  someone...").
- **Checks** (the compiler and the host tests): every name resolves, every line fits the box, every
  feeling tag is known, every quest can start and finish, and a story bot plays Act 1 and each side
  line through to the end on a simulated save.
- Today's eight quests are ported first, unchanged, to prove the engine; then the new content goes in.

### 6. The save, next version
The quests outgrow today's save (8 quest bytes, 32 flags).

- `kSaveVersion` 2: 64 quest slots, 128 flags, the mailbox (letters delivered and read, a queue with
  arrival days), badges, map pages, the fish log, a few event days for timed letters ("two days after
  Q2"), friends met.
- Migration from version 1 keeps everyone's progress: finished quests stay finished (Q2 done: Fig met
  and his quest done too), flags carry over, and letters that would have come already count as read.
- A version 1 save fixture in `tests/data` proves the migration.

### 7. The world changes that go with the story
- The **Whisperwood** (a named spot in the woods near the Market, a map pin) and Fig asleep there.
- **Fig's daily spot**, and his map (places marked as you find them).
- **Custard** at the meadow: a small dog built with the critters' kit (trot, sit, wag, sleep, fetch),
  pettable.
- **Cinder** by the Lodge's hearth: a grown Blazeplume, grey at the muzzle, lying asleep, waking in
  the story's moments.
- The **champions in the world** after you beat them (Marigold at the Orchard, Rook on the Windmill
  Bridge at dusk, Seraphine at the Glade, Solenne at the Starwatch Ruins at night), with rematches.
- The **badge case** (five badges) on the profile.
- **Levels top out at 42** in Skyreach (`kMaxLevel` stays 50 for later valleys).
- The **Wanderings sign** at the trailhead.
- The pageant's **How it works** button.
- The mailbox prop by the den's door.
- **Old Whiskers** and a few named fish at the cove; the five map pages; the fox's den; the orchard's hives.
- Everyone at the arena on the **festival night**, Cinder's greeting and the end-of-act credits.

### 8. Testing and review
- Host tests for the engine, the migration, the growth numbers and the story bot. The triangle budget
  and parity tests stay green for every body.
- An autotest per line (main story, pageant, league, Hollow, cove, errands) walking its quests with the
  dev menu's help, with shots of each new scene and each feeling.
- The player build's boot check.
- **Run 22**: a new `docs/plan/hardware-check-7.md` and the review page rewritten around it: the story
  line by line, the feelings (a gallery of faces and emotes), the people, the growth, the player build,
  plus the dragons' pass or fail list carried over.

## Order of work
1. **Foundations**: the player build; growing up faster; save v2 and its migration; the story engine
   with today's eight quests ported unchanged.
2. **People**: the chosen look on the eight bodies; the feelings kit (eyes, mouths, brows, clips, emotes,
   voices, sounds, portraits).
3. **Act 1**: the mailbox, Fig, the Whisperwood, Custard, Cinder, the reworked eight quests, Two by two
   and the three errands, every return talk, the festival night.
4. **Side lines**: the pageant, the league, the Hollow, the cove; the champions in the world, the badges,
   the level cap.
5. **New people**: the new bodies and portraits; lines and feelings for everyone.
6. **Review**: the autotests, hardware-check-7 and the review page, the dev CIA to the 3DS when Noah
   says it's online, the player build kept ready.

## Budgets and risks
- **The old 3DS**: more people are about (Fig, Custard, the champions). Draw at most six people near you
  at once, the nearest first; the rest fade out past a distance. Each person stays within 600 triangles.
- **Memory**: about 17 bodies of roughly 45 KB each; the story's text in tables. Well within the budget.
- **The save**: the migration is the riskiest change; the fixture test and a copy of the old save
  written before the first v2 save guard it.
- **Scope**: this is the biggest pass yet. It lands in the order above so each stage is playable.

## Questions for Noah
1. **Which look?** You wrote "set for". Set 1 (Villager, the Animal Crossing way) or Set 2 (Storybook,
   the taller keepers)?
2. **Growing up**: 5.5 days from hatching (so about 7.1 from a new egg), or 5.5 days in all? And eggs
   going from 1 day to 1.6 days (a little longer than now): right?
3. **The friend**: Fig Thimblewhistle, the sleepy "Royal Surveyor". Keep, or rename or change him?
4. **The dog**: Custard, Bram's hopeless sheepdog, found with the stray in Q4. Good?
5. **Badges**: five (Ember, Flame, Blaze, Starfire, and the Skyreach Champion)? Is that what "one of 5
   now" meant?
6. **The Champion badge**: from beating Solenne alone, or only once the Hollow's count is done too?
7. **Level cap 42** in Skyreach?
8. **A pageant rival** (Primrose and Duchess): in now, or later?
9. **The watchdog** in the player build: drop it, or keep it as a quiet crash note for bug reports?
10. **Cinder**, Rowan's old dragon, as the reason the lanterns went dark: good?
