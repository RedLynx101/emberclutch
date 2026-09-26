# The challenges (Beta WP8-WP11)

Three challenges, each with four cups (Ember, Flame, Blaze, Starfire), held at two places in the
valley: **Sky Rings** (ridden, D74) and the **Lantern Trial** at the arena, run by Wren, and
**Fruit Catch** at Honeyroot Orchard, run by Maple (it's her quest). The rules are pure logic in
`src/core/challenges.*` (PC-tested in `tests/test_challenges.cpp`); the scene is
`src/app/scene_challenge.cpp` with one file per challenge.

## Getting in
- A **notice board** stands by the arena's gate (across from Wren, arena frame (-5, 20.5)) and by
  the orchard's cart (orchard frame (-3.5, 11)). Walking up to one shows "A: the challenges"
  (`Action::Board` in `scene_valley.cpp`); A opens the challenge scene there. The first visit to
  the arena's board is also meeting Wren (her hello plays over the picker). **Talking to Wren**
  opens it too, after whatever she says (but the festival night's story); at the arena the
  Lantern Trial is offered first on the festival night, or when your dragon is too young to ride.
- **The picker** (bottom screen): the three challenges (tap, or L/R), their four cups (tap, or the
  D-pad) with a ribbon for each cup won, what the cup needs or your best, Start and Leave. The top
  screen shows the place, you and your partner at the board, the host, and the challenge and cup
  on a ribbon. Picking a challenge held at the other place runs it there.
- **The first run of each challenge** in a visit, the host explains it in the dialogue box.
- Leaving goes back out into the valley on foot at the place you're at.

## Cups and what they need (`challenge::entry`)
| | Ember | Flame | Blaze | Starfire |
|---|---|---|---|---|
| Sky Rings | a grown partner | Ember won | Flame won | Blaze won |
| Lantern Trial | any hatched partner | Ember won | Flame won, juvenile or older | Blaze won, grown |
| Fruit Catch | any hatched partner | Ember won | Flame won, juvenile or older | Blaze won, grown |

Any cup already opened can be run again.

## Results and rewards (`challenge::record`)
- **Outcomes:** won (the cup), placed (close: a smaller prize), or not this time.
- **Gleam:** a cup's first win 60 / 100 / 160 / 250; winning it again 20 / 30 / 45 / 60; placing
  10 / 15 / 20 / 30; trying 5. Giving up (X during a run) records nothing.
- **The save** (`WorldState`, the world block, appended in its size-prefixed way): `cups[challenge]`
  the highest cup won (never lowered), `ribbons` a bit per cup won (bit `challenge * 4 + cup - 1`),
  and `best[challenge][cup]` (Sky Rings: tenths of a second, lower is better; the others: points).
- **The den:** each challenge's trophy (in its highest cup's colour, its sign on top: a ring, a
  flame, an apple) stands on the den's shelves and every cup's rosette hangs along the shelves'
  edges: one mesh in den space, one draw (`shelfMesh`, drawn from `drawThings` in `render3d.cpp`),
  rebuilt only when something new is won.
- **Stingers:** a cup's first win `cup-won`; a win again `results-first`; placed `results-placed`;
  not this time `results-try-again`. The crowd cheers or aws at the arena. The music is `cup-day`,
  or `lantern-festival` for the Lantern Trial on the festival night (the campaign's last quest at
  its trial step).
- **The campaign** follows the cups (core/campaign already watches them): Fruit Catch for Market
  day (quest 2), Sky Rings for Wings (quest 6), the Lantern Trial for the festival (quest 8).

## Sky Rings (`challenge_rings.cpp`)
- **Ridden:** a countdown hovering over the arena, then the valley's own arcade flight
  (`core/flight`) with `courseTuning(wing, stamina)`: the dragon's **Wing** scales its glide, flap
  and dive speeds (up to +-12%) and its turning (+-20%); **Stamina** how much a wingbeat tires it;
  on a course wingbeats cost less than in free flight, and each ring passed gives a lift (stamina
  back, a little speed). Wing 5 and Stamina 5 fly as the free flight does.
- **Courses** (`makeCourse`): a curve through places, sampled into evenly spaced rings; each cup
  longer, with smaller rings, weaving side to side and bobbing more:
  - Ember: from the arena, a steady climb over the orchard and the Market to **the floating isles'
    high lantern**, the last ring at its flame (11 rings, 6.5 m). Finishing it has your dragon
    breathe the high lantern alight (the Wings quest's last step), and finds the isles.
  - Flame: low over Mirror Lake, past the Market, to the orchard (15 rings, 5.5 m).
  - Blaze: the other way, lower, weaving, to finish skimming the lake (19 rings, 4.8 m).
  - Starfire: up to the Nesting Stone's hill and down into the Market, weaving hard (28 rings, 4.2 m).
- **Par** is the steady pilot's time (`pilot`, `pilotTime`: it flies the course at Wing 5) times
  1.45 / 1.3 / 1.2 / 1.1, so every course is proven flyable, and it follows the valley if the valley
  changes. A missed ring (through its plane outside it) adds 3 s; one skipped wide with the next
  flown through counts as missed. Won: the total at or under par; placed within 15% over; the run
  ends at 2.5 times par.
- **The ghost:** your best run is recorded every 0.2 s and kept on the SD card
  (`sdmc:/3ds/emberclutch/ghost-rings-<cup>.bin`, written on a thread of its own); in later runs it
  flies beside you as a wisp in the cup's colour.
- **On screen:** the next ring gold and pulsing, the next three after it pale; an arrow at the edge
  when the next ring is out of sight; the clock and the rings on the top; the course over the
  valley's painted map, your stamina and the buttons on the bottom.

## The Lantern Trial (`challenge_lanterns.cpp`)
- Crystal lanterns stand in an arc round the arena's star, your dragon on it, you beside it, Wren
  judging from the festival stage. Each round the pattern lights lantern by lantern, each with its
  chime (a pentatonic step); then you tap the lanterns on the bottom screen (laid out as you see
  them) or pick with the D-pad and A, and your dragon turns and breathes each alight.
- | cup | lanterns | rounds | pattern | hearts | each lit for |
  |---|---|---|---|---|---|
  | Ember | 4 | 4 | 2 to 5 | 3 | 0.85 s |
  | Flame | 5 | 5 | 3 to 7 | 3 | 0.72 s |
  | Blaze | 6 | 5 | 4 to 8 | 2 | 0.6 s |
  | Starfire | 6 | 6 | 5 to 10 | 2 | 0.5 s |
- A wrong lantern costs a heart and shows the round again. Points: 10 a lantern, 25 x the round
  for clearing it, 40 a heart left at the end. Won: every round; placed: half of them.
- **Stats:** a strong **Breath** reaches the lantern quicker; a clever dragon (**Wit** 7 and up)
  glances at the right lantern (with a little trill) when you hesitate.
- **Breath by element** (`breathFor`, drawn as 2D puffs over the 3D): Ember flame (warm puffs
  rising), Grove spores (floating, drifting), Stone a sandy gust, Gale a gust (streaks), Tide mist
  (soft growing clouds), Frost frost (glinting crosses), Lumen light (bright glints), Shade a
  dusk-light (violet glints). A crossbreed breathes its first element.
- Winning the trial with every other festival lantern lit lights **the great lantern** on the
  arena's stage (the festival night).

## Fruit Catch (`challenge_fruit.cpp`)
- You stand in front of the orchard facing the open meadow, the basket at your feet, your dragon
  beside you, Maple watching; little flags mark the distances. On the bottom screen, touch the
  fruit on the basket and **flick it up and away**: the flick's speed is the throw's strength, its
  slant sends it left or right. Eight throws; the 4th and 8th are golden pears (double points).
- `planCatch` settles each catch as the fruit leaves your hand: the dragon watches a moment, runs,
  and catches it on its way down: waiting under a short one for an easy **leap**, catching up with
  a long one while it's still high for a **sky-high leap**, low for a snap, or a **dive** at the
  very end; too far and it's missed (it eats it anyway). Then it trots back and eats it.
- **The hop version** (a hatchling or a juvenile): softer, lobbed throws; it **hops** and
  **tumbles**. Its points per metre are higher, so its goals are like the grown ones.
- Points: the distance (10 a metre grown, 25 young) plus the style (leap 25, sky-high 45, dive 60;
  hop 15, tumble 40), golden x2. Goals (placed at 70%): grown 600 / 1,100 / 1,600 / 2,100; young
  450 / 800 / 1,200.
- **Stats:** **Stamina** adds to its running, **Might** to its leap. Every kind can win: a slow
  gallop just plays quicker.

## Files
- `src/core/challenges.hpp/.cpp`: cups, entry, rewards, courses, ring judging, the pilot, ghosts,
  the trial, the fruit's flight and the catch, breath.
- `src/core/challenge_mesh.hpp/.cpp`: rings, crystal lanterns, fruit, the basket, the boards,
  trophies, rosettes, the den's shelf mesh; their colours; the shelf spots.
- `src/core/world.hpp`, `src/core/save.cpp`: `best[][]` appended to the world block; `ribbons` as
  bits.
- `src/app/scene_challenge.cpp`: the scene (picker, runs, results), the stage (the valley round the
  place, you, your partner, the host, the camera, effects), the boards, the hooks the valley calls.
- `src/app/challenge_stage.hpp`: what the challenge files share.
- `src/app/challenge_rings.cpp`, `challenge_lanterns.cpp`, `challenge_fruit.cpp`: each challenge.
- `src/app/render3d.cpp` (its last section) / `.hpp`: `drawChallengeProps` (after `drawValley`,
  into its depth with its camera and fog) and the den's `drawShelf`.
- Hooks: `scene_valley.cpp` (the Board action, the boards drawn, `loadedValley`, `valleySky`),
  `app.hpp`/`app.cpp` (`SceneId::Challenge`), `main.cpp` (the music), `dialogue.*` (`startLines`),
  `strings.hpp`, `screenshot.cpp`/`hitch.cpp` (the scene's name), `autotest.*`.
- Autotests: `tests/autotest/ch_lantern.txt`, `ch_fruit.txt`, `ch_rings.txt`, `ch_breath.txt`, `ch_wren.txt`, `ch_starfire.txt`,
  `ch_den.txt`; commands `challenge <c> <cup>` (0 Fruit Catch, 1 Sky Rings, 2 Lantern Trial; cup 0
  opens the picker), `autoplay on|off` (the pilot flies, the lanterns are lit right, fruit thrown),
  `cups <fruit> <rings> <lantern>`.

## Sounds
All in romfs already (batch 3): `whistle-start`, `ring-pass`, `lantern-light`, `lantern-relight`,
`fruit-toss`, `fruit-catch`, `crowd-cheer`, `crowd-aww`, the six `breath-*`, and the stingers.
Stand-ins, until their own sounds come: the countdown's beeps (`ui-tap` pitched up), a missed ring
(`dragon-whimper`, soft), a crystal's chime (`lantern-light` pitched per lantern), a wrong lantern
(`dragon-grumble`), the clever dragon's hint (`dragon-trill`), a fruit bouncing (`ball-bounce`).
