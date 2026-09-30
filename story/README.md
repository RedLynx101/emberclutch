# The story scripts

Everything the valley's people say, every quest and every letter lives here, in plain text. After an
edit, run `python tools/story/build_story.py` (it checks everything and writes
`src/core/story_data.inc` and `src/core/story_ids.hpp`), then `tools\test.ps1` (the story bot plays
every line to its end). `--summary` also writes `docs/design/story-index.md`. The story bible is
`docs/design/story.md`; the engine is `src/core/story.cpp` (D137).

`ids.lock` gives every quest, flag, var, event and letter a save slot for good. Never reorder or
delete its lines; new names are added at the end by the build.

## Declarations (each starts at the beginning of a line; its body is indented)

```
person fig "Fig" "Royal Surveyor" voice 0 pitch 1.6 body fig portrait fig
```
Someone who speaks. `voice` 0 is Noah's (men), 1 the second voice (women and children).
`villager keeper` ties them to one of the six villagers (who stand at their own spots).

```
quest market_day "Market day"
  line main                  main | pageant | league | hollow | cove | errand (the Journal's groups)
  giver maple
  gleam 80                   paid when it's done
  after keepers_apprentice   quests that must be done first (any number)
  when <condition>           anything else it waits for
  rumour "Maple looks worried about someone."   (the Journal's hint while it waits to be asked about)
  step "Find Fig in the Whisperwood" talk fig where area market 60 -40 30
  step "Win a Fruit Catch at the orchard" until cup fruit where cup fruit
  reward wear surveyor_cap, letter fig_thanks
```
A quest starts only when a talk, a letter or a pickup says `start` (or `start auto`, rarely). A step
with `until <condition>` is done the moment the condition holds, so a lantern already lit or a dragon
already grown moves the quest straight on. A step with `talk <person>` waits for a talk that says
`advance <quest>` or `finish <quest>`. `where` points the map: `place P`, `lantern P`, `cup C`,
`person X`, `area P x y radius`, `spot P x y`, `unlit`, `group G` (the nearest pickup of a group).

```
talk maple
  rule once if done market_day and new meadow
    do start meadow
    [worried] Bram's feed order came in and he hasn't collected it.
    ? grown | [surprised] Goodness, {D}'s grown! You could carry it on your back.
    @fig [excited] Can I come? I'll carry the... map.
    * Maple hands you a heavy basket of feed.
  rule
    vary
      [happy] Fresh fruit today! Deal!
      --
      [thinking] Have you seen Fig? Silly question. He's asleep somewhere.
```
The first rule (for that person) whose condition holds is what they say. `once` rules say their piece
one time only. A person's `first` rules come before everyone else's, plain chatter (no condition)
after. `vary` picks one group of lines a day. Lines: `[feel] text`, `@who [feel] text` (someone else
in the same talk), `* text` (narration), `? <condition> | line` (only when it holds). `{D}` is your
dragon's name, `{P}` yours. The feelings: calm happy laugh excited surprised shock sad crying angry
huff worried scared sleepy love proud cool shy thinking wistful dizzy.

```
letter rowan_hello from rowan "Come say hello"
  when hatched and not met rowan
  do start keepers_apprentice
  Dear new keeper, word travels fast in a small valley...
  --
  (a new page)
```
A letter with a `when` arrives in the mailbox by itself once it holds; `letter <id>` in a `do`
sends one at once. Its `do` happens when it's first read.

```
spot fig market 60 -40 facing 1.2 clip doze_stand when step market_day 2
spots fig daily market 3 5 | mill 4 2 facing 0.5 | orchard 2 6 when done market_day
pickup page_mill mill 6 4 "Pick up the page" group page glint when active fig_map and not bit pages 0
  do bit pages 0, add pages_found 1
  * A page of Fig's map, caught on the bridge's rail.
sign wander_sign trailhead 2 7 "Read the sign"
  * WANDERINGS. Take a juvenile or older dragon along...
```
Where the story's people stand (the first spot that holds; a villager stands there instead of their
routine), things to pick up (a glint and a prompt) and signs to read. Positions are metres in the
place's own frame (`x` right, `y` in).

## Conditions (terms joined by `and`, each may start with `not`)

`done Q`, `begun Q`, `active Q`, `new Q`, `step Q N` (on step N), `past Q N` (step N done),
`days Q N` (N days since Q was done), `flag F`, `world W` (the game's own flags: entered_valley,
met_keeper, found_stray, glided, rode, met_traveller, wandered, festival ...), `met X`, `lantern P`,
`place P` (found), `lanterns` (all lit), `cup C [N]`, `grown` (your partner can be ridden),
`adults N`, `hatched`, `juvenile`, `partner`, `wearing A`, `event E`, `eventdays E N`,
`hour A B` (from A to B o'clock, wrapping past midnight), `daymod N I`, `var V N` (at least N),
`vareq V N`, `bit V B`, `league N` (leagues won), `beaten L I`, `shows N` (show titles),
`showwon L`, `hollow N` (the deepest floor), `count R N` (fish shells battles shows wild photos walks
cups), `mail L`, `read L`, `gleam N`, `pouch F N`, `true`.

## Effects (comma separated, after `do` or `reward`)

`start Q`, `advance Q`, `finish Q`, `flag F`, `unflag F`, `world W`, `met X`, `var V N`, `add V N`,
`bit V B`, `gleam N`, `food F N`, `take F N`, `wear A` (an accessory, by its name in snake_case),
`letter L`, `event E`, `staregg`.
