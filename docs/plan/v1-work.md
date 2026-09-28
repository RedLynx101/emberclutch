# Beta + 1.0: the work split (the long run, 2026-09-28)

The plan is `docs/plan/v1.md` (D89-D91). The foundation is on main (commit after 58d6688):
the save's new fields, the four new places' ground and ids, the 1.0 sound slots, other
dragons in the valley view, custom speakers in the dialogue box and the valley's feature hooks.
Six workstreams run at once, five of them in worktrees (Opus 5.5 agents); the lead merges.

## Rules for every workstream
- **Read first:** `CLAUDE.md`, `docs/plan/v1.md`, this file, then the code you touch. The
  existing code's style is the style: plain, commented in full sentences, no clever layers.
- **Your branch only.** Commit on your worktree's branch as you finish pieces (small, working
  commits). Don't push, don't merge into main, don't rebase main. The lead merges.
- **Build** (from your worktree root; put its path in place of `<wt>`):
  `C:\msys64\usr\bin\bash.exe -lc "source /etc/profile.d/devkit-env.sh && cd <wt as /c/...> && rm -f emberclutch.3dsx && make 2>&1"`
  Zero warnings: keep it that way.
- **PC tests:** `powershell -NoProfile -Command "& tools\test.ps1 *> $env:TEMP\ec_test_<you>.log"`
  then read the log (`tr -d '\0' < ... | grep -E "FAIL|checks,|rror"`). The runner stops at the
  first compiler warning, so a warning looks like a failure: fix it. Everything in `src/core` is
  pure logic (no libctru, no clock reads, no file IO) with tests in `tests/`.
- **The emulator:** `powershell -NoProfile -Command "& tools\autotest.ps1 tests\autotest\<script>.txt -ResetSave -NoBuild"`.
  It holds a machine-wide lock (the other workstreams wait for yours and you for theirs), so
  keep runs few and short, and don't start one while another of yours runs. Scripts are in
  `tests/autotest/` (commands in `src/app/autotest.cpp`: `name`, `tap`, `key`, `wait`, `shot`,
  `skip`, `travel <place>`, `view <place> ex ey ez tx ty tz`, `light`, `goto x y`, ...; add your
  own commands there if you need them). Shots land in `build/autotest/<script>/` with contact
  sheets `sheet-*.png` (read them with the Read tool). **Never use computer use.**
- **Shared files:** the bulk of your work goes in new files (source file names unique across
  `src/app` and `src/core`). In shared files (`scene_valley.cpp`, `render3d.cpp`, `app.hpp`,
  `strings.hpp`, `care_ui.cpp`, `villagers.cpp`, `audio.*`) keep hooks small and mark each with a
  comment naming your feature. Colours come from `app/theme.hpp`. Strings the player reads go in
  `app/strings.hpp` (append a section of your own).
- **The save:** don't change the format. `Dragon` (xp, trained, moves, wear, dye, titles, wins,
  cupsWon, ribbons, frostDeepest) and `SaveData::progress` (`core/trainer.hpp`: accessories,
  dyes, tracked goal, battle and show leagues, the Hollow, the day's claims, tips, counts) hold
  what 1.0 needs. If you truly need another field, stop and say so in your report.
- **The valley's features** (`app/valley_ext.hpp`): a feature stands people (or invisible spots)
  about the valley with `folk()`, answers A beside them with `act()`, and can take the valley over
  (`active()`, then `update()`, `view()`, `drawTop()`, `drawBottom()`): the scene stops walking,
  lends you the stage (you and your partner, their clips, the camera) and draws what you add to
  the view (`view.others` for dragons, `view.people` for people). Add your feature with one line
  in `valley_ext.cpp`'s `kFeatures`. Anyone can speak with `startSpeech()` (`app/dialogue.hpp`).
  NPCs reuse the people kit's bodies (`core/people.hpp` `Person`: the player bodies take any
  palette and hair, so challengers and hosts can look different without new models).
- **Budgets (old 3DS):** a valley view at most ~9,000 top-screen triangles including what you
  add; free GPU memory only through `render3d`'s deferred `retire()`; nothing allocated per frame
  in hot paths; mind linear memory.
- **Never:** download or install anything, touch the network, push, commit third-party binaries
  or AI concept art, print secrets, or edit files outside your worktree (the emulator's SD card
  via the autotest runner is fine). Commit messages end with
  `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- **Your report** (the last message): what you built and where, the hooks in shared files, how
  to see it (autotest scripts and the shots worth looking at), what's left or rough, and anything
  the lead must wire after merging.

## A. The places (models)
Blender (`C:\Program Files (x86)\Steam\steamapps\common\Blender\blender.exe`, headless, absolute
paths), `tools/blender/valley_places.py`, `tools/valley/places.json` -> `tools/valley/gen_places.py`
-> `src/core/places_data.inc` / `core/place_layout`. Read `docs/tech/places.md` first.
1. **Four new places** (<= 1,500 triangles each, three light sets baked, the same storybook look):
   - `caldera` (Emberpeak Caldera, the battle league's grand stage): stands on the crater's flat
     floor (radius ~33 m round the anchor; the landscape's inner walls rise steeply from 33 m to
     50 m out, ~24 m up; a gap in the rim toward local +Y is the way in, the path arriving at
     ~+30 m). A round stone battle ring (~14 m across, a low step up), basalt pillars with
     braziers (glow), stone terraces for a crowd against the far wall, glowing lava cracks and
     pools (glow, orange; never on the ring or the path), a banner arch at the way in, the
     champion's dais. Anchors: `ring` [x,y,z] the ring's middle, `sides` two [x,y] where the two
     trainers stand, `board` [x,y] the league board's sign.
   - `glade` (Moonpetal Glade, the pageant's hall; a night garden in the west woods, flat
     radius ~30 m): glowing moonpetal flowers (glow: soft blue, violet, pink; brighter at night),
     a round wooden stage with a flowered arch, benches for an audience, two stalls with awnings
     (accessories, dyes), strings of lanterns, a willow or two (simple), stepping stones. Anchors:
     `stage` [x,y,z], `rivals` four [x,y] on the stage, `judges` [x,y] (their table), `stalls`
     two [x,y,facing] (accessories, dyes), `board` [x,y].
   - `cove` (Driftwood Cove, fishing): a beach facing the lake (local +Y is the water, the shore
     ~18-30 m out, the ground a gentle slope into it). A wooden jetty out over the water toward
     +Y, driftwood logs, shells, a fisher's shack with nets, a rowboat, a small campfire (glow),
     tide pools. Anchors: `fish_spot` [x,y,z] where you stand at the jetty's end, `fisher` [x,y],
     `shells` five [x,y] where shells wash up.
   - `hollow` (Frostspire Hollow, the training ground): a bowl in the cold heights, flat radius
     ~15 m round the anchor, the landscape's rim rising from 15-19 m out to ~16 m up; local +Y
     (east) is the corridor out through the rim. Tall ice spires ringing the bowl (pale blue), a
     cave door in the back wall (local -Y) with a blue glow where wild dragons come out, the
     keeper's camp near the mouth (a tent, a brazier), frost crystals. Anchors: `arena`
     [x,y,z], `wild_door` [x,y], `keeper` [x,y].
   Give each its `places.json` entry (flat, door null unless it has one, lantern null, solids)
   and export the anchors so the game can read them (extend `gen_places.py` and
   `core/place_layout` with a small named-anchor table, e.g. `placeAnchor(place, "ring")`, tested).
2. **Fixes from run 19 (D89):** signs' posts in front of their faces (the den's heart sign and
   every other sign: posts to the sides or behind); the Nesting Stone's faces the wrong way round
   (its outer ring unseen: fix the winding/normals); modelled roads that double the landscape's own
   earth paths (the Trailhead's; check every place and remove them); the arena sitting partly in
   the ground (find why: model floor vs the flattened ground at 15 m; fix on the model's side, or
   tell the lead the ground height it needs).
3. Look at every place in the emulator (`view`, day with `light`), before and after; keep the
   budgets table in `docs/tech/places.md` up to date. Don't edit `tools/valley/make_valley.py` or
   `romfs/valley/skyreach.evl` (the lead's); if the ground under a model needs changing, say what.

## B. Battles and Frostspire Hollow
The core rules first (PC-tested, balanced by simulation), then the battle in the valley.
1. **`core/battle.*`**: the eight elements' matchups (`kinds_data.inc`: Ember, Grove, Stone, Gale,
   Tide, Frost, Lumen, Shade; x2 strong, x0.5 weak, a clear, learnable wheel); ~40 moves (each
   element's breath moves by level, body moves: tackle, claw, tail, wing buffet, headbutt; a few
   status moves: roar lowers Might, preen raises Wit, rest heals ...), power/accuracy/element/kind;
   what a dragon knows (its kind's elements and level) and its four equipped (`Dragon::moves`,
   `kNone` filled from the best known; `equipMove()` for the profile page); battle stats from
   `trainer::statPoints` and the level (Wing: who goes first; Might/Breath: body/breath power;
   Stamina: health; Wit: accuracy, critical hits, dodging); the turn's resolution with an `Rng`;
   an AI that picks moves by league (random-ish at Ember, smart at Starfire); experience and
   rewards (`trainer::gainXp`, `claimToday` so rematches pay once a day, the per-dragon record in
   `core/trainer`). Balance tests: equal levels ~50/50, +5 levels ~80%, no move always best.
2. **The league**: challengers with names, looks (people kit bodies, palettes, hair), lines and
   their dragons (kind, colouring, level, moves), four per league (Ember L3-8, Flame L10-16,
   Blaze L18-26, Starfire L28-38) standing about the valley at the places (their spots in place
   frames), and each league's champion at Emberpeak Caldera (a little stronger). A league board
   at the arena (and at the caldera) shows who's beaten and where the rest are; beating all four
   opens the champion; winning a final gives the dragon its title (`recordLeague`), your
   `progress.battleLeague`, Gleam and a prize, and the next league's challengers arrive.
3. **The battle in the valley** (a `vext::Feature`): A beside a challenger (after their lines)
   starts it where you stand (the caldera's ring for finals): you and your partner on one side,
   the challenger and theirs ~8 m off, the camera framing both; the bottom screen the four moves
   (name, element colour, power) and giving up; the top screen health bars, names, levels and a
   line of what happened ("Cinder used Flame Breath! It's super effective!"). Moves shown with
   the dragons' clips (a lunge for body moves: move it toward the foe and back; `Pounce`, `Spar`,
   `Hop`, `Shake`), breath effects (the challenges' breath streams in `core/challenge_mesh` and
   `drawChallengeProps` can be reused), hit flashes and shakes, the new sounds (`Hit`, `HitBig`,
   `Whiff`, `Faint`, `StatUp`, `BattleStart`, `Victory`, `Defeat`, breath sounds). Energy:
   `trainer::kEnergyBattle` (too tired: it says so and there's no battle). Afterwards: experience
   (a level-up toast and `LevelUp`, a new move learned), health back (no harm done), Gleam.
   Eggs can't battle; any hatched dragon can (small ones on their lead stand beside you).
4. **Frostspire Hollow**: the keeper at the bowl's mouth (lines, the floor you're on); floors 1-30,
   each a wild dragon (no trainer) out of the cave door, level ~2 + floor x 1.2, kinds weighted by
   depth, the rare colouring likelier deeper, a guardian every fifth floor; go on floor after
   floor until you lose or leave, checkpoints every five (`progress.hollowDeepest`, the dragon's
   `frostDeepest`); rewards: more experience than the league, now and then a trained stat point
   (the stat the wild one was strongest in: `trainer::train`), Gleam; the bowl colder and darker
   deeper (tint and fog). Energy per floor.
5. Autotest commands for starting a battle and a floor directly, and scripts that play one
   through with an autoplay (the challenges' `autoplay` is the model).

## C. The challenges and Driftwood Cove
1. **The challenges (D89)**: harder cups (retune the targets per cup; Starfire should need real
   skill); a cup's reward paid once a day (`trainer::claimToday(kClaimCup + challenge*4+cup-1)`;
   the first-ever win of a cup pays more; say so on the results screen); the per-dragon record
   (`trainer::recordCup`) and trophies that mean something (the first win of each cup puts its
   trophy on the den's shelf, a toast; a note of what the next cup needs); Energy
   (`kEnergyChallenge`, too tired: can't enter). **Sky Rings**: two or three rival dragons racing
   the course (`ValleyView::others`, a simple AI following the rings with some mistakes, faster
   by cup), L/R for a burst and a brake, the old "Flight" meter renamed **Stamina** and sized
   from the dragon's Stamina stat, momentum (speed carries, hard turns cost speed), the wind's
   sound by speed (`Bed::WindHigh`'s level, the `Burst`/`Brake` sounds). **Fruit Catch**: its
   reach and speed from the stats (Wing: speed, Wit: reach), and your figure no longer split in
   3D when the dragon is far (find why: a 2D drawn at the wrong depth for the stereo eye?).
2. **Driftwood Cove fishing** (a `vext::Feature`): the fisher (a person at the cove's `fisher`
   anchor; lines; lends you a rod the first time), and the jetty's end (`fish_spot`, an invisible
   spot): cast (A), wait for a bite (the bobber dips, `Plop`/`Bite`), strike in time, then reel
   on the bottom screen (keep the line's tension in a band, `Reel`); catches: River Fish (the
   pouch: `Food::RiverFish`), other foods now and then, shells and a rare pearl (Gleam), counted
   (`trainer::count(kCountFish)`); your partner sits beside you and gets a nibble (Love up).
   The fish are plentiful but not endless (a daily stock that renews, `progress.claims`' hollow
   range is B's: use `kClaimCup`-style bits of your own inside `counts` or ask the lead). Shells
   wash up on the beach daily at the `shells` anchors (little finds: A to pick up, `ShellPick`).
   The anchors come from workstream A; until they're merged use spots in the cove's frame you
   pick (the cove's anchor is on the beach, +Y toward the water).

## P. The pageant, accessories and dyes
1. **`core/accessories.*`**: ~30 accessories in the four wear slots (head: hats, crowns, flower
   crowns, horn rings; neck: bows, scarves, collars, bells, pendants; back: saddles, capes,
   blankets; tail: tail bows, ribbons, rings, wing charms), each with a name, slot, price, two
   colours, style tags (cute, elegant, wild, festive, frosty, fiery, floral, starry) and where it
   comes from (the glade's stall, show prizes, the Hollow, finds); ~12 dyes that tint a dragon's
   body colours toward theirs (and "natural"); owning (`trainer::ownsAccessory`/`ownsDye`),
   wearing (`Dragon::wear`, `Dragon::dye`), tested.
2. **Worn on the models**: small low-poly meshes built in code (like `core/prop_mesh.cpp`,
   ~40-150 triangles each), drawn on the dragon's bones (head, neck/chest, spine, tail) wherever
   dragons are drawn (the den, close-ups, showcases, the valley, others in the valley), fitted per
   body plan (a small table of offsets and scales per plan and slot; check every kind in both
   forms with autotest shots). Dyes through the palette (`kindPalette` and the render path).
3. **`core/pageant.*`**: themed shows (~8: Frost Ball, Harvest Fair, Starlight Gala, Ember
   Carnival, Tide Regatta, Blossom Festival, Shadow Masquerade, Sunrise Parade), each favouring
   elements, colours and style tags; three rounds: **Look** (clean, no mud, accessories suiting
   the theme and the dragon's colours, the kind's and colouring's rarity, the dye), **Poise**
   (bond, manner, mood, care stars), **Performance** (a short rhythm game: cued tricks by button
   or touch in time; bond widens the window; the dragon plays the clip); rivals picked for the
   league; judges' scores; placings. Leagues Ember -> Starfire on the pageant's board (four shows
   a league, the themes changing by day); win all four for the league title (`recordShowLeague`,
   `progress.showLeague`); ribbons per dragon (`recordShow`); rewards once a day per show.
4. **At Moonpetal Glade** (a `vext::Feature`): the host, the board, the shows on the stage (rivals
   in `view.others`, judges as people, sparkles), the accessory and dye stalls, and a
   **wardrobe** to dress your dragon (its own scene `scene_wardrobe`, the dragon turning on the
   top screen as it tries things on; opened from the glade and from the den; tell the lead how
   the den should open it). Anchors from workstream A (until merged, spots you pick in the
   glade's frame).

## U. The interface
1. **The Market's and the Wanderings' top screens** redesigned cozy (D89): warm, readable,
   storybook (the Market: the stalls and the day's things; the Wanderings: the trail, the dragon
   out walking, its progress and what it's found).
2. **The needs** (`care_ui.cpp` gauges): Belly, Clean, Play and **Love** as the four; **Energy**
   as a bar below them. Petting and brushing now fill Love (`core/dragon`).
3. **The dragon's profile and record** (the Journal's dragon tab or its own page): level and the
   experience bar, stats (the kind's and trained), its four moves (swap from what it knows, once
   workstream B's `core/battle` is in: leave a clean hook), what it wears and its dye (a button
   to the wardrobe, workstream P), its titles, ribbons, cups, wins and deepest Hollow floor.
4. **The Journal tracks a goal** (`trainer::track`: a quest, the battle board's next
   challenger, the show board, the Hollow, a place): pick one to track on the Journal's quests
   tab; a small "tracking" line wherever the Journal's quest shows; and a function the lead calls
   from the valley's map to draw the tracked goal's marker and search area
   (`drawTrackedOnMap(app, valley, mapX, mapY, mapSize)` in a file of your own).
5. **A gentle tutorial**: short tips the first time things happen (the first valley walk, the
   first ride, a first battle, a show, fishing, low Energy, Love ...; `trainer::tipSeen/markTip`),
   shown as a small card that doesn't block play; a table in core, the card in app.
6. **Settings**: a page in the system menu for the volumes, the 3D, and resetting tips.

## S. Sounds
1. Cute generated synths for the 31 new sounds (`audio.hpp` 1.0 block: `hop-on` ... `brake`),
   written by a new `tools/audio/make_synth_sfx.py` (pure Python: `wave`, `math`, `random`; no
   numpy) into `romfs/sfx/<slug>.wav` in the game's format (check `tools/audio/process_sfx.py`
   and an existing wav: rate, bits, channels, length, loudness); each short, soft and warm,
   matching the storybook sound of the existing effects; two or three takes where variety helps
   (`<slug>-2.wav`).
2. New ambient beds (loops): the cove's waves, the caldera's low rumble and crackle, the glade's
   night chimes, the Hollow's cold wind and drips, and a rushing wind for speed; add them to
   `audio.hpp`'s `Bed` and `audio.cpp` (non-resident, loaded when first wanted), with the scene
   code setting them by place (tell the lead the one-line calls for `scene_valley`).
3. Short jingles as stingers (a battle win, a level up, a ribbon) made the same way and
   encoded to `romfs/music/<slug>.ogg` with ffmpeg (on PATH), if they sound good; else skip.
4. Check every file with `tools/check_3ds.py`; keep the whole set small (romfs size).

## Lead (main)
The flicker (depth, pop-in, culling, buffers); run 19's fixes not listed above; Love/Energy
wiring; walking together; feeding in the valley; textured ground; particles; random renewing
finds; the valley's hooks for B, C, P, U and S; the merges; the whole-game playthrough; docs,
release prep, the review, run 20 and the banner labs.
