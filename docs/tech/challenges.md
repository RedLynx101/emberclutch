# The challenges (Beta WP8-WP11; retuned for 1.0, D89) and Driftwood Cove

Three challenges, each with four cups (Ember, Flame, Blaze, Starfire), held at two places in the
valley: **Sky Rings** (ridden, D74; a race against rival dragons since 1.0) and the **Lantern
Trial** at the arena, run by Wren, and **Fruit Catch** at Honeyroot Orchard, run by Maple (it's
her quest). The rules are pure logic in `src/core/challenges.*` (PC-tested in
`tests/test_challenges.cpp`); the scene is `src/app/scene_challenge.cpp` with one file per
challenge. Run 19 (D89) found them too easy and their trophies "participation": 1.0 made them
harder, worth more once a day, spent Energy on them and kept each dragon's record. **Driftwood
Cove's fishing** (1.0, D90) is at the end of this page.

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

## Cups and what they need (`challenge::entry`, `cupNeeds`)
| | Ember | Flame | Blaze | Starfire |
|---|---|---|---|---|
| Sky Rings | a grown partner; beat 2 rivals | Ember won; beat 3 | Flame won; beat 3 | Blaze won; beat 3 |
| Lantern Trial | any hatched partner | Ember won | Flame won, juvenile or older | Blaze won, grown |
| Fruit Catch | any hatched partner; 1,100 pts | Ember won; 1,600 | Flame won, juvenile or older; 2,050 | Blaze won, grown; 2,400 |

Any cup already opened can be run again. Every run costs the partner **Energy**
(`trainer::kEnergyChallenge`, 18 of 100; sleep brings it back); too tired, it can't enter (the
picker says so in rose, and Start or Again says "Too tired! Let it rest first."). The picker
shows the cup's prize under the blurb: its first win's, today's, or "Today's prize won".

## Results and rewards (`challenge::record`)
- **Outcomes:** won (the cup), placed (close: second in a race, 70% of the goal, half the rounds),
  or not this time.
- **Once a day per cup** (D89: no farming): a cup's **first win** pays its first prize (80 / 140 /
  220 / 350 Gleam) and puts its trophy on the den's shelf (a toast says so) with its ribbon; after
  that a win pays **the day's prize** (30 / 45 / 65 / 90) the first time each day
  (`trainer::claimToday`, bit `kClaimCup + challenge * 4 + cup - 1`), and nothing again that day
  ("Today's prize for this cup is yours already"). **Placing** pays 10 / 15 / 20 / 30 while the
  day's prize is still to win (it doesn't take it; Energy limits it). Not this time: nothing.
  Giving up (X during a run) records nothing and still costs the Energy.
- **Experience** for the partner (`trainer::gainXp`): 20 / 35 / 55 / 80 with a paid win, 4 for any
  other finished run; a level gained is toasted (`LevelUp`).
- **The dragon's record:** `trainer::recordCup` (its own cups, for the profile); the card says
  "Cinder's first Flame cup!" when it's the partner's first. Your count of cups won
  (`kCountCups`).
- **The results' bottom screen** after a win says **what the next cup needs** ("Next: the Flame
  cup. Beat 3 faster rivals!", the points, the lanterns and rounds; and whether it needs an older
  partner), or that every cup is won.
- **The save** (`WorldState`, unchanged): `cups[challenge]` the highest cup won (never lowered),
  `ribbons` a bit per cup won, `best[challenge][cup]` (Sky Rings: tenths of a second; the others:
  points); the day's claims in `Progress::claims`; the record in `Dragon::cupsWon`.
- **The den:** each challenge's trophy (in its highest cup's colour, its sign on top: a ring, a
  flame, an apple) stands on the den's shelves, **grander cup by cup** (scale 0.95 / 1.05 / 1.15 /
  1.25), and every cup's rosette hangs along the shelves' edges: one mesh in den space, one draw
  (`shelfMesh`, drawn from `drawThings` in `render3d.cpp`), rebuilt only when something new is won.
- **Stingers:** a cup's first win `cup-won`; a win again `results-first`; placed `results-placed`;
  not this time `results-try-again`. The crowd cheers or aws at the arena. The music is `cup-day`,
  or `lantern-festival` for the Lantern Trial on the festival night (the campaign's last quest at
  its trial step).
- **The campaign** follows the cups (core/campaign already watches them): Fruit Catch for Market
  day (quest 2), Sky Rings for Wings (quest 6), the Lantern Trial for the festival (quest 8).

## Sky Rings (`challenge_rings.cpp`)
- **A race** (D89): you and two (Ember) or three rival dragons from a countdown hovering over the
  arena, through the rings in order. **First to the last ring wins the cup** (each missed ring
  adds 3 s to its flier's time, rivals' too), second places. The run ends at 2.5 times par (time's
  up). Par is the steady pilot's time with some slack; it's the time limit's base now, not the goal.
- **The race's flight** (`raceStep`, `RaceTuning`; its own, the valley's free flight is
  untouched): **speed carries** (level flight eases back to cruising only slowly), **hard turns
  cost speed** (and a fast dragon turns wider, a slow one tighter), climbing costs speed and
  diving gives it back. **R bursts** toward top speed while the **Stamina** meter lasts; **L
  brakes** (and turns 30% tighter). A wingbeat (A) lifts and costs a little of the meter; easing
  off fills it; each ring passed tops it up (+10%) with a little speed. It never lands: it skims
  the ground or the lake.
  - The dragon's **Wing** (statLevel: its kind's points plus training, past 10 at a quarter)
    scales cruise, top and dive speeds (+-10%) and turning (+-12%); its **Stamina** sizes the
    meter (2 + 0.3 x Stamina seconds of burst: 3.5 s for an average dragon; the bar on screen is
    as long as the meter). An average dragon: cruise 14 m/s, burst 19, dive 30, braked 7.
- **The rivals** (`rivalFor`, `Pilot`): a pilot that lines up the next ring (aiming inside it
  toward the one after), flaps to climb, dives to drop; a shakier one wanders about its line and
  now and then goes wide of a ring; a cannier one bursts along more of the straights and brakes
  into hard turns. By cup (Wing / Stamina / bursting / braking / going wide):
  - Ember: Puddle 3.5/4, Sprig 4.5/4.5; no bursts; 8-10% wide.
  - Flame: Breeze 4.5/5 (never), Tumble 5/5 (15%), Cinderwisp 5.5/5.5 (25%); 6-7% wide.
  - Blaze: Gale 5.5/6, Flicker 6/6, Swoop 6/6.5; bursting half the time or more, braking; 4-5%.
  - Starfire: Starling 6/7, Tempest 6.5/7, Aurora 7/7.5; bursting at every chance, braking; 2-3%.
  Balanced by simulation (`sky_rings_rivals` prints it): the Ember field loses to a steady flight
  at an average dragon's stats; the Flame field about ties it (bursts win); the Blaze field about
  ties an expert (bursting and braking well) at average stats; **the Starfire field beats that
  expert**: it takes a strong, trained dragon (Wing/Stamina 8) flown well.
  They look like grown dragons of the kinds your den has (no kind loads on the spot), in other
  colourings (the Starfire cup's fastest in its kind's rare one), with name tags; drawn on the
  light model (`ValleyDragon::lod`); nudged out of the camera's line to you and out of each
  other's way; far ones (over 50 m) aren't drawn while the valley's ground is heavy (the budget).
- **Courses** (`makeCourse`): a curve through places, sampled into evenly spaced rings; each cup
  longer, with smaller rings, weaving side to side and bobbing more:
  - Ember: from the arena, a steady climb over the orchard and the Market to **the floating isles'
    high lantern**, the last ring at its flame (11 rings, 6.5 m). Finishing it has your dragon
    breathe the high lantern alight (the Wings quest's last step), and finds the isles.
  - Flame: low over Mirror Lake, past the Market, to the orchard (15 rings, 5.5 m).
  - Blaze: the other way, lower, weaving, to finish skimming the lake (19 rings, 4.8 m).
  - Starfire: up to the Nesting Stone's hill and down into the Market, weaving hard (28 rings, 4.2 m).
- **Sounds:** `Burst` and `Brake` as they take hold; the wind (`Bed::WindHigh`) rushes louder the
  faster you go (a burst or a dive roars); the wings flutter in a glide.
- **The ghost:** your best run is recorded every 0.2 s and kept on the SD card
  (`sdmc:/3ds/emberclutch/ghost-rings-<cup>.bin`, written on a thread of its own); in later runs it
  flies beside you as a wisp in the cup's colour.
- **On screen:** the clock, **your place**, the Stamina meter (under the clock), the rings; the
  burst's speed lines; the next ring gold and pulsing, the next three after it pale; an arrow at
  the edge when the next ring is out of sight. The bottom screen maps the course with the rivals
  as coloured dots, the standings, the meter and the buttons. The results say your place and
  time, and who won or by how much you beat the nearest.

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
- **The breeze** (D89): from the Flame cup on, each throw has its own breeze (`windFor`: up to 0.6
  / 1.0 / 1.4 m/s^2, from any way; the arrow on the bottom screen shows it as the meadow lies before
  you): a tailwind carries the fruit further, a headwind holds it back, a crosswind drifts it. The
  dragon reads it; you have to, to keep the long throws in its reach.
- **The hop version** (a hatchling or a juvenile): softer, lobbed throws; it **hops** and
  **tumbles**. Its points per metre are higher, so its goals are like the grown ones.
- Points: the distance (10 a metre grown, 25 young) plus the style (leap 25, sky-high 45, dive 60;
  hop 15, tumble 40), golden x2. **Goals** (placed at 70%): grown 1,100 / 1,600 / 2,050 / 2,400
  (37 / 54 / 69 / 81% of a perfect round for an average dragon), young 800 / 1,150 / 1,500.
- **Stats** (D89): **Wing** sets its running (+-10%), **Wit** its reach: how soon it reads the
  throw (0.16-0.32 s), its leap (+-15%) and its dive's stretch (+-25%). A quick, clever dragon's
  perfect round is ~14% more.
- **The 3D** is focused on you (`ValleyView::focus`): before, it focused on the dragon, so when it
  ran 20-40 m down the meadow the eyes' separation grew with that distance and you, close in
  front, stood far out of the screen, split in two (run 19).

## Files
- `src/core/challenges.hpp/.cpp`: cups, entry, prizes and the record, stats, courses, the race's
  flight, pilots and racers, the rivals, ring judging, ghosts, the trial, the fruit's flight, the
  breeze and the catch, breath.
- `src/core/challenge_mesh.hpp/.cpp`: rings, crystal lanterns, fruit, the basket, the boards,
  trophies, rosettes, the den's shelf mesh; the cove's shells, bobber and fish; their colours; the
  shelf spots.
- `src/core/world.hpp`, `src/core/save.cpp`: `best[][]` appended to the world block; `ribbons` as
  bits. `src/core/trainer.*`: Energy, the day's claims, the record, experience.
- `src/app/scene_challenge.cpp`: the scene (picker, runs, results), the stage (the valley round the
  place, you, your partner, the host, the camera, effects), the boards, the hooks the valley calls.
- `src/app/challenge_stage.hpp`: what the challenge files share (with the rivals' `others` and the
  3D's `focus`).
- `src/app/challenge_rings.cpp`, `challenge_lanterns.cpp`, `challenge_fruit.cpp`: each challenge.
- `src/app/render3d.cpp` (its last section) / `.hpp`: `drawChallengeProps` (after `drawValley`,
  into its depth with its camera and fog) and the den's `drawShelf`; 1.0: `ValleyView::focus`,
  `ValleyDragon::lod`, `PropKind::Shell/Bobber/Fish`.
- Hooks: `scene_valley.cpp` (the Board action, the boards drawn, `loadedValley`, `valleySky`; the
  cove's things drawn), `valley_ext.cpp` (the cove's feature), `app.hpp`/`app.cpp`
  (`SceneId::Challenge`), `main.cpp` (the music), `dialogue.*` (`startLines`, `startSpeech`),
  `strings.hpp`, `screenshot.cpp`/`hitch.cpp` (the scene's name), `autotest.*`.
- Autotests: `tests/autotest/ch_race.txt` (Sky Rings' race: Ember with its results and today's
  prize, a tired dragon, the Starfire field), `ch_fruit_wind.txt` (the breeze, the right eye's
  picture while the dragon is far), `ch_lantern.txt`, `ch_fruit.txt`, `ch_rings.txt`,
  `ch_breath.txt`, `ch_wren.txt`, `ch_starfire.txt`, `ch_den.txt`, `cove.txt`; commands
  `challenge <c> <cup>` (0 Fruit Catch, 1 Sky Rings, 2 Lantern Trial; cup 0 opens the picker),
  `autoplay on|off` (the expert flies, the lanterns are lit right, fruit thrown, fish caught),
  `cups <fruit> <rings> <lantern>`, `energy <0..100>`, `cove <0 Tam, 1 fish, 2 a shell>`. The
  runner copies the game's log (each picture's triangles) beside the shots.

## Sounds
All in romfs already (batch 3): `whistle-start`, `ring-pass`, `lantern-light`, `lantern-relight`,
`fruit-toss`, `fruit-catch`, `crowd-cheer`, `crowd-aww`, the six `breath-*`, and the stingers.
Stand-ins, until their own sounds come: the countdown's beeps (`ui-tap` pitched up), a missed ring
(`dragon-whimper`, soft), a crystal's chime (`lantern-light` pitched per lantern), a wrong lantern
(`dragon-grumble`), the clever dragon's hint (`dragon-trill`), a fruit bouncing (`ball-bounce`).

## Driftwood Cove (`src/app/cove.cpp`, `src/core/fishing.*`; 1.0, D90)
A valley feature (`app/valley_ext`): nothing loads or opens, the valley lends it the stage.
- **Tam the fisher** stands by the water (the people kit's first body in a yellow oilskin, silver
  hair; voice 0, pitch 1.25). The first time he lends you his spare rod and explains; after that
  a tip (dawn and dusk, easing off when a fish runs, the shells) and how the fish are today.
- **Fishing:** at the water's edge ("A: fish here"; without the rod, "Talk to Tam first") you
  stand facing the lake, your partner sits beside you, the camera over your right shoulder. **A
  casts** (the bobber arcs out and plops); the bobber **twitches with nibbles** (A on one: "too
  soon", it swims off; A otherwise reels in to cast again), then **dips right under** with a "!"
  (`Bite`): **A in its moment** (0.7-1.0 s) strikes. Then **reel** on the bottom screen: hold A,
  or crank the reel round with the stylus (its speed is how hard you reel). The tension gauge's
  green band is 0.3-0.8: reeling tightens the line, the fish pulls (hard while it runs, now and
  then), letting up gives line; the fish comes in while you reel in the band (slower out of it)
  and slips back as it runs. Over 1: **snap**. Slack for 1.6 s, or 5 s without reeling: it
  slips the hook. A careful reeler lands a River Fish in about 6 s, a big one in about 11 (holding
  A flat out snaps the line within a second). B puts the rod away (not with a fish on).
- **What bites** (`rollCatch`): River Fish 58%, a big River Fish 12% (20% at dawn and dusk; it
  counts two and pulls hard), Honeyroot 7%, a Skyberry sprig 6%, a Frostmelon 5%, a shell tangled
  on the hook 10% (8 Gleam), a pearl 2% (120 Gleam). Bites come after 2.6-7 s (1.6-4.6 at dawn and
  dusk). Foods go into the pouch (a full pouch: 15 Gleam each instead); your partner has a
  **nibble** of any food (Love +6, Belly +3, hearts, `Munch`). Counted: `kCountFish`,
  `kCountShells`.
- **Not endless:** eight catches a day ("The fish are biting: 5 more today"; then "resting till
  tomorrow" and Tam says so).
- **Shells:** three to five of five spots along the wet sand have a shell each day
  (`shellsToday`: a spiral, a scallop, a cowrie, a sand dollar; a pearl now and then), drawn big
  enough to read; A beside one picks it up (5-12 Gleam, a pearl 120; `ShellPick`).
- **The day** (the stock, the shells picked, the rod) is kept in `sdmc:/3ds/emberclutch/cove.bin`
  (16 bytes, written on a thread), stamped with the game (its first dragon's id and laying time)
  so a new game starts afresh: **the save has no room for it** (1.0's progress block is spoken
  for). If the lead adds ~2 bytes to `Progress` (reset with `claimDay`), `cove.cpp`'s `today()` is
  the one place to move it. The autotest runner treats `cove.bin` as part of the save.
- **Where things stand** (`coveSpots`, the cove's frame): until workstream A's anchors, picked on
  the cove's ground: the waterline straight out (~42 m from the anchor), you 1.3 m up the beach,
  the bobber 9.5 m out, your partner 1.8 m to your left, Tam 5.2 m to your left and a little back,
  the shells at x = -18, -10.5, -5, 9, 16 on the wet sand. `coveSpots` is the one place to swap in
  `placeAnchor(kPlaceCove, "fish_spot" / "fisher" / "shells")`.
- **Sounds:** `Equip` (the rod), `Cast`, `Plop` (and soft for nibbles), `Bite`, `Reel` (clicks,
  higher as the line tightens), `Splash`/`SplashBig`, `Coin`, `ShellPick`, `Munch`, `Chirp`,
  `Whimper`/`Grumble` for a lost one; the `place-found` stinger for a pearl.
- **The view:** your partner sits on your left (a grown one further off and a little ahead), the
  camera over your right shoulder, further back and higher for a bigger partner.
- **Budget:** the fishing view is ~8,200 top-screen triangles without the cove's model (the
  ground ~4,400; your partner on its light model: the camera stands far enough back); the shells,
  bobber and fish are 11-128 triangles each.
