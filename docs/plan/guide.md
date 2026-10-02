# The player's guide (planned for 1.0; not written yet)

Noah, 2026-10-01: *"Let's begin planning on a little PDF guide for the game when we decide 1.0 is done
for now. Don't make it yet, but we will. And don't spoil too much in the game, mostly just talk about
some of the mechanics, introduce the world and concepts, explain fighting rules and stats, and
generally some features as needed."* Part of the 1.0 plan ([release-1.0.md](release-1.0.md), R3).

## What it is
- **A little handbook**, in the game's storybook voice: warm, second person, short paragraphs, the
  game's own words for things (Belly, Gleam, the Journal, the Nesting Stone).
- **Two forms from one source:** `docs/guide/guide.md` (reads on GitHub, linked from the README) and
  `docs/guide/Emberclutch-Guide.pdf` (attached to the GitHub Release, printable).
- **About 20-24 pages, A5 portrait**, so it prints as a folded booklet and reads well on a phone.
- Working title: ***A Keeper's Handbook*** (alternatives: *The Skyreach Field Guide*, *Notes for New
  Keepers*). The cover: the release banner art and the wordmark.

## What it won't spoil
- The story past its opening: the Lantern Festival is introduced, not what happens; quests are explained
  as a system (the Journal, letters, people asking for help), not listed.
- The champions: "five champions at Emberpeak Caldera", no names, kinds or teams.
- The deep places: Frostspire Hollow is "floor by floor, wild dragons, deeper is colder", nothing about
  what waits at the bottom; secret places and the rare finds go unmentioned.
- Breeding: how it works, not what pairs make. Unknown kinds stay silhouettes in the guide's Dragondex
  page; it shows the starters only.
- Letters' contents, villagers' secrets, the rarest colourings.

## The outline (Keep/Change on the review page)
1. **Welcome to Skyreach Valley.** What the game is, who made it (Noah, for Emi), the promise that
   nothing ever dies, the 3D slider.
2. **Getting started.** Installing (Universal Updater, FBI and its QR code, by hand), the DSP firmware for
   sound, where the save lives, the controls, the two screens.
3. **Your egg.** Choosing one, keeping it warm and turned (about a day and a half), hatching, naming.
4. **Caring for a dragon.** The needs (Belly, Clean, Play, Love; Energy and sleep), the tray's tools, the
   heart glow and moods, personality and manners, sulking and making up, care stars, growing up (about
   five and a half days, and its stars).
5. **The den.** Who's out, decorating, photos, the mailbox and letters, the settings.
6. **The valley.** Walking with the lead, the map and travel, the places you can see from the start,
   day and night, the villagers and the Journal, the day's finds, Gleam and the Market.
7. **Riding and flying.** When a dragon is grown enough, getting on, flapping, diving, banking; Stamina and
   Wing; the wind.
8. **Stats and growing stronger.** Wing, Wit, Might, Breath and Stamina, what each does in and out of
   battle; levels (to 42) and experience; traits and manners; the elements.
9. **Battles: the rules.** Turn by turn, four moves each, body and breath moves, who goes first, the
   elements' strengths and weaknesses (a chart), raised and lowered stats, being "too tired" (never hurt),
   the league's ranks (Ember, Flame, Blaze, Starfire) and badges, wild dragons in Frostspire Hollow.
10. **Beauty shows.** Moonpetal Glade's themes, style, accessories and dyes, the judges, ribbons.
11. **Challenges.** Sky Rings, Fruit Catch, the Lantern Trial; cups and trophies.
12. **And more.** Fishing at Driftwood Cove, the Wanderings (steps with the 3DS closed), the Nesting Stone
    (breeding, in outline), the Dragondex.
13. **Tips and questions.** Saving, changing the 3DS clock, what happens while you're away, sound, the 3D.
14. **Credits and licences.**

## How it's made (when Noah says go)
- **Words:** written from the game itself, every number checked against the code (the balance in
  `src/core/dragon.cpp`, the battle rules in `src/core/battle.cpp`, the level cap, the egg's time),
  with a small check script that fails if the guide and the code disagree on a number.
- **Pictures:** in-game screenshots from a `tests/autotest/guide_shots.txt` run (headless, 400x240,
  scaled up crisply), dragon portraits rendered in Blender with their real skins, the game's own icons;
  no AI-generated art (D74).
- **Layout:** `tools/guide/build_guide.py` turns the Markdown into an HTML page with a print stylesheet
  (A5, the game's palette, Nunito and Cinzel Decorative, both OFL so they embed), then headless Chrome
  or Edge prints it to PDF (both are on this PC: nothing to install).
- **Reviews:** the outline (here, Keep/Change), then the full draft on the review page, then the PDF's
  pages as pictures; final with the 1.0 build so the screenshots match.
