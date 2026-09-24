# Alpha 2 — *A den*: Work Plan

Status: **In progress** (Alpha 1 closed 2026-09-24). WP1 done: the den of three dragons
and two eggs (D53), and their life together. WP2 done (the Sanctuary and Cold Vault), with
a first world map. WP3 done (breeding at the Nesting Stone). WP4 done (the Wanderings).
WP5 done (the Market, Gleam and the pouch). WP6 done (the map's trips). WP7 done (toys and
decor). WP8 done (the profile and family tree). WP9 done (brief 2's sounds wired, stand-ins until they arrive). WP10 done (the emblem icon and the 3D banner). **R5 decided (D54): all four looks ship, per dragon.** Next: a first look on Noah's old 3DS (D55), then WP12. Goal (D31): complete through Alpha 2. Scope and assets:
[content & assets](content-and-assets.md) (A2 rows); screens: [screens & flow](../design/screens-and-flow.md).

**The order matters** (Noah, 2026-09-24): everything that doesn't depend on how the
dragons look comes first. The **style review (R5)** is the last thing built before Noah's
decision, and it blocks. The big dragons update follows in the chosen style, then the run
on Noah's old 3DS closes the milestone.

## Definition of done
Checked in Azahar, then once on Noah's old 3DS (D34):
1. Three dragons live in the den together (plus 2 egg nests) and play with each other and
   with toys; the Sanctuary and Cold Vault swap dragons and eggs in and out.
2. Breeding at the Nesting Stone (one male, one female) lays an egg that hatches a
   visibly distinct offspring with its parents' traits, in every breed.
3. Wanderings with the step counter bring back finds; the hoard grows; wild eggs appear.
4. The Market sells food, grooming items, toys and decor for Gleam, plus the daily
   sex-labeled egg (D24).
5. The world map fast-travels between all the Alpha 2 places.
6. The dragons are in the style Noah chose at R5, with the full parts library, all 21
   breeds, patterns, rare traits, and dirt and mud that grooming cleans (D46).
7. The new emblem icon (D48) and an animated **3D HOME Menu banner** with a baby dragon
   (D50), with a 2D banner as the fallback.
8. It installs as a CIA and runs on the old 3DS: the 3-dragon den within budget, audio,
   saves, touch.
9. Tests green, no build warnings, docs, STATUS and RedWiki updated, tagged `v0.2.0-alpha2`.

## Phase A — the den grows (style-independent)
Worked through alone, package by package (D49).

### WP1 — Several dragons
- A den of 3 active dragons plus 2 egg nests; pick which one the care screen is for.
  *Done (D53):* `src/core/den_roster.*` (beds, nests, who moves out, a ready egg waits for a
  bed; PC-tested), a second egg nest in the room, an actor per bed, the D-pad and the
  profile card's arrows to switch, a heart over the one you're with; dev menu "Add dragon"
  and "Add egg"; `tests/autotest/den3.txt`. Full den measured 7,740 top + 2,752 bottom
  triangles, 15 draws.
- Dragon-to-dragon life: play-chase, nuzzle, curling up together, sharing the sunbeam
  (the crowd avoidance and three beds exist). *Done:* `denSocial` in `src/core/behavior.cpp`
  pairs idle dragons for a chase (the more playful chases; a catch is a hop and a squeak)
  or a nuzzle face to face (hearts); by bright day one lies in the sunbeam and another may
  join; most nights two sleep side by side in the big nest. PC-tested; seen in Azahar
  (`tests/autotest/together.txt`, which logs everyone's activity per shot).
- LOD1 for background dragons (exists); the 3-dragon budget confirmed in the emulator.
- *Done:* an egg LOD (312 triangles, `egg_model.py --lod 1`) and a lighter room (1,975):
  three dragons and two eggs fit the 8,000 frame.

### WP2 — Sanctuary and Cold Vault
- Illustrated screens with grids; move dragons and eggs in and out (needs drain slowly
  and never below 50 in the Sanctuary; incubation pauses in the Vault, GDD §8).
  *Done:* `src/app/scene_storage.cpp` (the keepers' meadow and the icy vault on the top
  screen with the chosen one in 3D, `r3d::drawShowcase`; a paged grid; "To the den"),
  `storeAway` / `bringHome` in `core/den_roster` (the Vault holds 50; growth now really
  pauses in the Sanctuary); "Sanctuary" on the profile card and "To the Vault" on the egg
  screen (someone always stays home). Reached through a first world map (below, WP6).
  `tests/autotest/storage.txt`.

### WP3 — Breeding and eggs
- The Nesting Stone corner of the den; courtship nuzzle, settling on the nest, laying.
- Rules exist (`src/core/breeding.cpp`); the readiness hint on screen.
- Egg shells hinting at elements (all 6 patterns); lineage kept for the profile.
- *Done:* the Nesting Stone is a place on the map (`src/app/scene_stone.cpp`): pick her and
  him from the den's dragons, the readiness hint, "Nest together" (their nuzzle on the
  stone, `r3d::drawPair`); the pair is saved and their egg comes the next calendar day into
  a free nest or the Vault (`settleToNest`, `layDueEgg`; PC-tested), with its parents
  recorded. A hybrid egg's light drifts between its two elements' colours. Dev menu
  "Breed-ready"; scripted runs use a fixed seed. `tests/autotest/breeding.txt`.

### WP4 — Wanderings
- The step counter (PTMU), a trail map with progress, departure and return scenes.
- Finds (trinkets, Gleam), the hoard pile growing in the den, rare wild eggs, and mud
  spots on the dragon afterwards (cleaned in the bath, D46).
- *Done:* `src/core/wanderings.*` (one juvenile-or-older dragon at a time, keeping its bed;
  a chance of a find every 400 steps, more for adults and the curious; Gleam and six
  trinkets to the hoard; a wild egg now and then on long walks, mostly Grove, Frost and
  Lumen; muddy legs, belly and tail; PC-tested) and `src/app/scene_wander.cpp` (the
  trailhead, the trail map with your steps, the finds). The 3DS pedometer (`ptmu`); the
  dev menu's second page adds steps (the emulator's never counts). The hoard glints more as
  it grows; the showcase now plays the idle clip. `tests/autotest/wander.txt`.

### WP5 — Market, Gleam and the pouch
- The Market scene and shop UI (buy and sell), the pouch, Gleam; the daily egg.
- Items from the [catalogue](content-and-assets.md#7-items-catalogue); food stops being
  free.
- *Done:* `src/core/market.*` (prices, the pouch, selling trinkets, the egg of the day:
  labelled, the same all day, mostly Grove/Frost/Lumen, one a day; PC-tested) and
  `src/app/scene_market.cpp` (stalls under bunting; Food, Sell and Egg-of-the-day tabs; the
  egg shown on the stall). It opens on the map once a dragon is Juvenile. The care screen's
  food row shows the pouch (a food is used when eaten; none left sends you to the Market);
  a new game starts with 50 Gleam, bread, drumsticks, a candy and the starter's favourite.
  Toys and den decor for sale come with WP7. `tests/autotest/market.txt`.

### WP6 — World map and fast travel
- [Map & travel](../design/world-map-and-travel.md): the illustrated map, place pins that
  unlock, the travel transition.
  *Started with WP2:* `src/app/scene_map.cpp`, the valley painted in code (mountains,
  meadows, the river and lake, woods, dotted paths) with six pins (Den, Sanctuary and
  Vault open; the rest "not open yet"), a painted view of the place picked on the top
  screen, a short trip between places; X or the START menu opens it.
- *Done:* all six places built and open (the Market once a dragon is Juvenile, with its
  own "opens at Juvenile" note); the map remembers where you are, and the trip is a heart
  travelling the dotted path from there to where you're going, then a fade into the place
  (1.8 s; A or a tap skips it); picking where you already are goes straight in.
  `tests/autotest/maptrip.txt`.

### WP7 — Toys and den props
- Tug rope, feather wand, puzzle orb, the food bowl ([care interactions §8](../design/care-interactions.md#8-more-toys-alpha-2-from-the-market));
  toys stay where they're left and dragons play with them on their own.
- *Done:* `src/core/items.*` (22 things bought once at the Market: the four toys, a silver
  brush that shines twice as fast, bubble soap that leaves a bath's sparkle, warm stones
  that halve how fast eggs cool, and fifteen pieces of decor for five spots: rugs, lanterns,
  perches, plants, banners; saved with where the toys lie and what's in the bowl; PC-tested).
  The Market's Goods tab (icons rendered by `care_sprites.py`): buy, put up, take down. In
  3D (`src/core/prop_mesh.*`, lit like the dragons, lanterns glowing at night); the room's
  rug became one of them and the room was trimmed, so a full den with everything out still
  fits the frame (7,720 of 8,000 measured). Up close, the tray's last slot holds the toy in
  hand (tap it again for the others): the feather wand (it watches, swats, pounces when you
  let go near it), the tug rope (it bites on and tugs, then trots off proud and drops it
  where it likes), the puzzle orb (roll it about until a treat drops out); drop a food on
  the bowl to fill it. On their own they bat and pounce on the feather, shake the rope and
  carry it off (sometimes to bed), push the orb along with their noses until a treat drops
  out, eat from the bowl when hungry (while you're away too), and two may have a tug-of-war.
  New clips: paw bat, tug. `tests/autotest/things.txt`, `toys.txt`, `toyden.txt`.

### WP8 — Dragon profile and family tree
- Name, breed, sex, stage, personality, stats, the sweet spot once found, parents and
  grandparents.
- *Done:* `src/core/profile.*` (trait names, Wing / Wit / Spark from the breed's aptitudes
  and the stage until training grows them in Beta, the sweet spot as a place, the family:
  parents, grandparents, young; PC-tested). Each dragon now remembers where its egg came
  from (first egg, the Nesting Stone, a wild egg, the Market) and what you've found out
  (its sweet spot, its favourite food; the first find says where it is). The heartglow
  opens the full profile: About (identity, bond hearts, stats, looks, the discoveries) and
  Family (a three-generation tree, or the egg's story), with Rename and To the Sanctuary;
  the top screen shows the dragon posing in its heartglow's light. The Sanctuary and the
  Cold Vault open the same pages for the one picked (an egg's family, too). Dev menu page 2:
  Add family. `tests/autotest/profile.txt`.

### WP9 — Sounds
- Wire in the [sound brief 2](../audio/sfx-batch-2.md) sounds as they arrive
  (placeholders until then, D35); the Market loop and Wanderings stinger exist.
- *Done (stand-ins):* every sound in brief 2 has its own slot in `src/app/audio`, named
  after its slug and played where it belongs (the heartbeat, turning the egg, the first
  cry, the tub, suds, the rinse, the wet shake, a grumble, sniffs, the giggle at the sweet
  spot, the leg kick, the ball's roll and catch; the rope, the feather, the orb, the treat,
  the bowl, settling at the Nesting Stone, an egg laid, Gleam, the register, the map, the
  trip, setting off on a walk) and the Market's ambience bed. Until a file arrives, a
  retuned stand-in plays (what those moments played before); dropping
  `romfs/sfx/<slug>.wav` in replaces it, no code change. None of brief 2 has arrived yet.

### WP10 — The app icon and a 3D HOME Menu banner (D48, D50)
- **Icon:** a designed **emblem**, independent of the dragon style: a glowing heart inside
  an ember-lit egg with a curl of wing, in the game's palette; 48×48, readable at small
  size. It replaces the AI-concept-derived placeholder and Alpha 1's interim icon.
- **3D banner** (like retail games: a small animated 3D scene on the top screen when the
  game is selected in the HOME Menu): the **baby dragon, textured**, peeking out of its
  ember-lit egg, with the heart glowing; it blinks, tilts its head, wags its tail and the
  heart pulses; the Emberclutch wordmark behind it.
  - Built by script from our own model: Blender scene → glTF → **pycgfx** (glTF to CGFX,
    textures and animation) → `bannertool makebanner -ci` with a short banner tune
    (≤ 3 s) → the SMDH's `extendedbanner` flag → the CIA (`tools/package_cia.ps1`).
  - Rules from the homebrew community: the CGFX stays **under 512 KB**; the model and its
    animation are both named `COMMON`; animate **rigid pieces with node transforms**
    (body, head, tail, wings, heart), not skinning: skinned banners that work in
    emulators have frozen the real HOME Menu. The heart's glow uses diffuse-colour
    animation.
  - The emulator doesn't run the HOME Menu (it would need Nintendo's system files), so
    the 3D banner is proven on the old 3DS (WP13). Until then, and as the fallback, the
    same scene rendered flat makes a 2D banner.
  - Tools: bannertool and makerom are already on disk (from 3D-Claw); pycgfx and its two
    Python libraries (gltflib, pillow) are downloaded at this step (approved, D50) and
    never committed.
- Both are shown to Noah at R5 (the banner as renders and a turntable).
- *Done:* `tools/blender/emblem.py` (the emblem: a glowing heart in an ember-lit egg, a
  dragon wing curled round it, on a plum badge with a gold rim; 48x48 and a 256 review
  render) and `tools/blender/banner3d.py`: the textured baby Ember (the den's light model,
  full-detail eyes) sits in its cracked egg, the cap tipped on its head, looking up at you,
  the wordmark behind and an ember glow. The posed dragon is frozen and cut into rigid
  pieces with their pivots at the joints (body, head, eyes, tail, heart); over a 4 second
  loop the head tilts and nods, the tail wags, it blinks twice and the heart beats (scale,
  plus a diffuse-colour animation added to the glTF as `KHR_animation_pointer`). The skin
  is baked to a 128 colour texture, the wordmark is one textured quad. Exported to glTF,
  converted by pycgfx (fetched into `build/tools`, gltflib and pillow installed for Python
  3.12, D50): **284 KB** of the 512 KB limit, one `COMMON` model, skeletal and material
  animations, no skinning. `tools/make_banner.ps1` runs it all;
  `tools/package_cia.ps1 -Banner3D` packs it (with the `extendedbanner` flag) and the
  default keeps the flat banner, rendered from the same scene, until WP13 proves the 3D one
  on the old 3DS. Previews through the HOME Menu's own camera are in the R5 review.

## Phase B — the style review (R5, blocks)

### WP11 — Three Ember dragon style variants (D47)
Built last before Noah decides. The **current style, textured** (from Alpha 1), next to
three variants of the Ember dragon, fairly different from each other but all in the
game's theme (warm light inside, cute babies, majestic adults):
- **V1, surface:** today's shapes with a new surface: textures, eyes and shading.
- **V2, shape:** new proportions and shapes (head, body, limbs, wings) in a matching surface.
- **V3, bold:** a redesign of the whole look.

Each at hatchling, juvenile and adult, male and female, within the triangle and bone
budgets and animated by the same clips.
- **Review sheets and turntables** (`docs/art/reviews/R5-style.md`).
- **In the game:** a dev-menu switch shows any variant in the den and the petting
  close-up, so Noah sees them move on a 3DS screen.
- The icon and the 3D banner (WP10) are shown alongside.
- **Stop here** until Noah chooses: keep the current style, or move to a variant (or a
  mix of their best parts).
- *Built (2026-09-24), waiting on Noah:* [R5-style.md](../art/reviews/R5-style.md). V1
  surface (bold outlined scale plates, a banded belly, a darker spine, rounder pupils, softer
  three-band light), V2 shape (chubby big-headed babies; long-necked, deep-chested adults with
  big wings and horns), V3 bold (the ember-veined dragon: dark scales with glowing cracks,
  glowing eyes and wings). One `--style` switch through `dragon_model.py`,
  `dragon_texture.py` and `export_dragon.py` (models in `romfs/models/v1..v3`, all ≤ 2,910
  triangles, same rig and clips); in the game, dev menu page 2 **Next style (R5)**
  (`r3d::setStyle`). Review sheets, turntables and in-game captures (`tests/autotest/styles.txt`).

## Phase C — after the decision

### WP11b — A first look on the old 3DS (D55, before WP12)
- Noah installs a dev CIA and reads the budget overlay with a full den, with every look's
  models in memory at once (dev menu: Probe: all looks), and tries the 3D banner in the real
  HOME Menu: [hardware-check-1.md](hardware-check-1.md). Emulator: all four looks cost about
  3.4 MB of linear memory (21.4 → 18.0 MB free), the full den holds 16.7 ms.

**Run 1 (2026-09-24, the 3D-banner CIA):** the 3D banner plays in the HOME Menu, in 3D too.
The game didn't start: "An error occurred (ErrDisp). The SD card was removed." every time,
while every other title runs. Fixed in 0.1.2 (D56):
- **No boot logo.** The RSF said `Logo: None`, so the CIA had no logo region at all. Azahar
  never looks at it; the HOME Menu reads it before starting a title (3D-Claw's CIA, which
  runs, has one). Now makerom's own *homebrew* logo: the word "homebrew" and a few floating
  squares, no Nintendo branding. An Emberclutch logo of our own follows (WP11c, D58).
- **The SMDH's "extendedbanner" flag** (set with `-Banner3D`) is for a banner kept in
  extdata, not for a 3D one: dropped.
- **The wordmark showed only from behind.** pycgfx draws a two-sided material as two copies,
  the second with its normals turned away, and a blended material writes no depth, so the
  dark copy covered the gold one. Now it's unlit and alpha-tested (`make_unlit` in
  `banner3d.py`; `tools/banner_cgfx.py` runs pycgfx and passes unlit colours through):
  gold from the front and, mirrored like a sign in a window, from behind. The wordmark's own
  render scene no longer ends up in the glTF.
- **The wall is gone**, and the glow disc with it: the HOME Menu's own background shows
  round the egg (the flat 2D banner keeps both).
- **The rump through the egg.** The banner turns round as the HOME Menu swaps titles. Its
  rump was 0.165 of the egg's height outside the shell; `fit_in_egg` moves the dragon
  forward (0.21 of the egg's height) until nothing below the crack is outside, the tail
  checked through its wag. Review renders from behind and the side: `banner3d_back.png`,
  `banner3d_side.png`.
- **The hover sound:** one bar of the title theme (2.8 s from 15.25 s, the brightest early
  bar) instead of Alpha 1's knock, crack and trill. `make_banner_sound.py --candidates`
  writes the others to compare (12.55 s, the theme's first bar; 36.82 s, gentler; the old mix).
- **The icon:** square and full-bleed, the plum and ember glow filling the frame (the round
  badge left black corners).
- **Screenshots (D57):** Y anywhere saves both screens and a log line to the SD card;
  `tools\pull_shots.ps1` copies them off.

**Run 2 (2026-09-24, 0.1.2):**
- **The 3D-banner CIA froze the HOME Menu** as Noah opened the new title's present, both
  times, and never showed the banner. 0.1.2 had dropped the SMDH's *extendedbanner* flag
  (D56's reasoning was wrong: homebrew 3D banners all set it, and 0.1.1's banner showed with
  it). 0.1.3 sets it again. In case the wordmark change is at fault instead, a second
  diagnostic CIA carries the 0.1.3 banner with the wordmark as 0.1.1 had it.
- **The flat-banner CIA started:** the *homebrew* logo, the title screen, the music. A
  screenshot worked but stalled the game while it saved, so 0.1.3 writes it on a thread of
  its own. The stylus reticle is accurate.
- **It crashed after choosing Tide** (Luma: data abort, a read at 0x0000000C): the first
  frame with 3D in it. `end3D()` unbound texture unit 1 with `C3D_TexBind(1, nullptr)`, and
  citro3d 1.7.1 reads the null texture's type for units 1 and 2 before binding. Azahar
  reads 0 there and carries on; the 3DS faults. Fixed: the unit stays bound (citro2d never
  samples it).
- **Hunting the rest of that kind:** every scripted run's Azahar log is searched for
  unmapped memory accesses (`tools/autotest.ps1` now warns after each run, D59).
  All 16 scripts ran clean with the fix; the build before it logs that very read (at the
  crash's PC, 0x0014B748), so the check sees what the 3DS would.
- **Also in 0.1.3 (Noah):** the footsteps quieter and muffled, babies walking 30% faster
  with their steps unchanged (D60); the hatching plan follows his direction (WP12a).
- The screenshot showed the title at 16.6 ms, 23.0 MB of linear memory free. (Application
  memory reads 0 MB on the 3DS, because libctru gives the heap everything at start, so the
  log line no longer shows it.)

**Run 3 (2026-09-24, 0.1.3, the flat-banner CIA and the .3dsx):** the game runs on the old
3DS: past the egg pick, the hatching, care, the map, the Wanderings. 11 screenshots, the
first real numbers: one egg in the den 16.7 ms; **the full den with the close-up 22–23 ms**
(7,286 + 2,966 triangles, CPU 10.9 ms, GPU 8.3 ms), so about 45 fps; all four looks in
memory 22.9 → 19.6 MB of linear memory free.
- **Both 3D-banner CIAs still froze the HOME Menu** (the flag back, the wordmark as 0.1.1 or
  not), and the flat one played **0.1.1's sound**, though every 0.1.3 banner carries the
  theme clip (checked in the built banner). So the HOME Menu keeps a banner per title and
  showed a cached one. It no longer makes sense to test banners on the game's own title:
  `tools/banner_lab.ps1` packs **banner-lab titles** (their own IDs, no romfs, ~1 MB each)
  to try on the HOME Menu instead. Lab 1: A = 0.1.1's banner exactly (the control), B =
  0.1.1's scene with the theme clip, C = the new scene with the old sound, D = the new scene
  with 0.1.1's wordmark and the old sound, E = 0.1.3's banner exactly. Which ones freeze
  says whether it's the scene, the sound or neither.
- **Next style (R5) froze the whole 3DS** after one or two presses (CIA and .3dsx both). The
  dev menu runs while the bottom screen is drawn, after the top screen's dragons are queued
  for the GPU; the style change freed their vertex buffers and skins at once, and the GPU
  read memory already handed out again. Fixed for every such release: GPU memory is freed
  only after the next C3D_FrameBegin (which waits for the last frame), in
  `r3d::frameBegun()`. Scene updates freed buffers the same way while the last frame could
  still be drawing (decor, props); those go through it too.
- **Fixed in 0.1.4 from Noah's notes:** the need bars' empty track dark (it was the colour
  of the background); the ball bounces off the room's real wall and the rocks against it
  (it bounced off the dragons' walking circle), and past that circle the floor rises so it
  rolls back within reach; feeding no longer swings the head left and right, and petting
  under the chin no longer jitters (the look-at aimed from the head itself, so a target at
  the snout flipped it between its limits; now from a little behind it, and the gaze eases
  toward the stylus); the sponge on a rinsed dragon brings it back to the tub for another
  wash (it did nothing until the tool was picked again); L / R with the brush or cloth turns
  it to show the other flank at any time (it only listened mid-stroke); the petting close-up
  frames a grown dragon's face closer; a wagging tail no longer lifts a big dragon off the
  floor (the tail no longer counts as floor contact); a dragon interrupted while carrying the
  ball back lets it drop (it kept it in its mouth); the tub grows with the dragon; the
  Wanderings show the dragon out walking; the baby's chest heart sits lower, where its chin
  doesn't hide it when it sits (all four looks re-exported).
- **Already there, less visible than it should be:** favourite foods (one per dragon, hidden
  until fed; 1.5x belly and double bond, "Its favourite!" and hearts; the profile shows it
  once found). Planned: a small heart on the known favourite in the food row, and an eager
  sniff when an unknown favourite is offered.

**Run 4 (2026-09-24), banner lab 1:** A and B passed, C, D and E froze. So the theme clip
is fine (B: 0.1.1's scene with it) and the new scene freezes, with either wordmark (D). Every
dictionary in both CGFX files was checked offline with the runtime's Patricia lookup (all
names found; the tree isn't it). What's left between the scenes: the dragon moved forward,
the backdrop and glow disc removed, and so the skeleton's first bone (glTF lists objects by
name) became the dragon's animated body instead of the still backdrop. **Lab 2** (fresh
title IDs, `-FirstId 0xEC0E1`, the theme clip in all): F = the new scene with the backdrop
and glow back; G = the new scene with a small still "anchor" triangle as its first object,
hidden in the egg; H = the new scene with the dragon not moved; I = the new scene with the
glow disc only. First bone static: F and G pass. The move: H passes, F, G, I freeze. The flat
meshes themselves: F and I pass, G freezes.

**Lab 2:** F, G and I passed, H froze. Not the dragon's move (G and I have it), not the
first bone (I starts with the animated body). Every scene that froze (C, D, E, H) is made of
exactly the dragon, its egg and the wordmark; every one that passed has at least one mesh
more, even a small hidden triangle (G). The HOME Menu's rule isn't known, but the way round
it is proven: **the banner (D63) keeps a small still triangle hidden in the egg** and, as
Noah asked ("no background thing... some sparkles... make it nice"), **ten gold sparkles**
round the egg and the dragon: four-pointed stars, unlit, faint specks that glint in turn (two
or three at a time) and turn a quarter turn a loop. No backdrop, no glow disc: the HOME
Menu's own background shows. 340 KB of 512. **Lab 3:** J = that banner, K = the same without
the hidden triangle (whether the sparkles alone are enough).

**0.1.5** (on the 3DS with the .3dsx): 0.1.4 plus the section profiler (WP11d): the baseline
to measure the performance pass against. **0.1.6:** the new banner (a 3D CIA and a flat one)
and the look-at's head-chain evaluation.

**Run 6 (2026-09-24):** lab J (the sparkle banner) passed, K (the same without the hidden
triangle) froze: the triangle is what keeps the HOME Menu happy; the sparkles alone aren't
enough. From Noah's notes, in **0.1.7**:
- **The sparkles only glint** (no turning), everything else as J; 316 KB.
- **A new title ID, 0xEC0C2** (was 0xEC0C1): the HOME Menu kept replaying 0.1.1's banner
  sound for the old title though every build since 0.1.2 carries the theme clip, even after
  deleting and reinstalling it. The save lives on the SD card, not with the title: nothing is
  lost; the old title is deleted once in FBI. Later banner changes may meet the same cache.
- **The 3D banner is the CIA's default** (`package_cia.ps1 -Banner2D` for the flat one; D58).
- **A ready egg waited though only two dragons were in the den:** Noah's save (read off the
  SD card) showed the third bed held by a dragon out on the Wanderings. A wanderer now lends
  its bed: the hatchling takes it and the wanderer comes home to the Sanctuary (a toast
  says so); with three dragons really at home, the message says how to make room.
- **The food bowl takes any foods,** stacked, six portions (it held one food, three of it); a
  dragon eats its favourite first if it's in there, else the newest it doesn't dislike.
  Older saves keep what was in their bowl.
- **No mouth marker when feeding** (a dev-overlay dot; "humans know where their mouth is").
- **The first hardware profile** (0.1.6, full den + close-up): 18.5–21.7 ms; CPU 8.8–10.5,
  GPU 7.4–7.9; pose 2.5–3.9, draw calls 0.6, room 1.0, the rest of the top screen 3.0–4.1,
  of the bottom 1.7–3.4, update 0.3–1.1. citro3d's FrameBegin waits for the GPU to finish the
  last frame, so the CPU's and GPU's times add up. **Posing moved ahead** (WP11d).

### WP11d — Hardware performance pass (after run 3; before WP12a, Noah agreed)
The full den with the close-up runs at 22–23 ms on the old 3DS (CPU 10.9, GPU 8.3). Target:
16.7 ms with three dragons, their toys and decor, and the close-up.
- **Measure first, on the 3DS:** ✅ `src/app/perf` times the frame's sections (the scene's
  update, audio, posing dragons, their draw calls, the room and props, the rest of each
  screen), on the dev overlay's fifth line and in every screenshot's log line: a Y press on
  the hardware brings back where the milliseconds go. 0.1.5 is the baseline.
- **First cut:** ✅ the look-at evaluates only the head's bone chain instead of the whole
  skeleton, twice per dragon per frame (a PC test holds it equal to the full evaluation).
- **Posing ahead:** ✅ (0.1.7) the den's dragons are posed before C3D_FrameBegin, while the
  GPU still draws the last frame (a scene `prepare` step; `r3d::poseAhead`), and the close-up
  reuses the cared-for dragon's pose instead of posing it again. On 0.1.6 the full den spent
  2.5–3.9 ms posing inside the frame: that should come off the frame time. Next, from the
  hardware profile: the rest of the top screen (3–4 ms), then the GPU (~7.9 ms).
- **Likely CPU cuts:** evaluate each dragon's pose once a frame (the look-at evaluates the
  whole skeleton again), the ground from the feet bones instead of body vertices, dust
  streams only when dirt changes, background dragons animated at 30 Hz, fewer draw calls
  for props.
- **Likely GPU cuts:** LOD1 sooner for the dragons behind, a lighter close-up (it draws the
  whole dragon again), cheaper lighting for the room's glows.
- Checked on the 3DS with the same full den; the budget overlay's targets updated from the
  hardware numbers.

### WP11e — Stereoscopic 3D (the 3D slider; Noah, run 3)
The top screen is flat today. Rendered per eye when the slider is up: the den, the
showcases, the map and the title, with the UI at screen depth and the dragons just behind
it. It draws the top screen's 3D twice, so it waits for WP11d; in 3D it runs at 30 fps on the
old 3DS (Noah: fine), 60 without. The bottom screen can't be 3D.

### WP11c — An Emberclutch boot logo (D58)
The logo the HOME Menu plays as a title starts (the NCCH's logo region; the emulator never
shows it). Since 0.1.2 that's makerom's *homebrew* logo: a small layout (an LZ11-compressed
darc of `blyt/*.bclyt`, `anim/*.bclan` and `timg/*.bclim`: the word "homebrew" on a
256 × 64 ETC1 texture, floating squares, a mask and waves) that animates in and out.
- **Keep its layout and animation; swap the picture.** `tools/logo/make_logo.py` builds a
  CXI with `Logo: Homebrew`, reads the logo region, unpacks the darc, and replaces
  `timg/logo.bclim` with the Emberclutch wordmark (the banner's Cinzel gold with its dark
  edge, rendered by Blender at the same 225 × 40 in a 256 × 64 texture, ETC1-encoded). Then
  it repacks and recompresses to `build/logo/emberclutch.bcma.lz`. Built from makerom's
  output each time: nothing of makerom's is committed.
- **Maybe (tried, then judged on the 3DS):** the floating squares as ember specks (the
  `bubbles` texture in warm gold, or the layout's material colour tinted).
- **The limits:** the logo region stays within makerom's 0x2000 bytes (the homebrew logo's
  size), and the ETC1 texture keeps its size and format so the layout needs no changes. The
  ETC1 encoder is ours (numpy, per 4 × 4 block, the best of the 8 tables × both modes), and
  it's checked by a round trip: decoded again and compared with the source picture (PSNR),
  plus a review sheet of every texture.
- **Packing:** `package_cia.ps1` passes `-logo build/logo/emberclutch.bcma.lz` when it exists
  and falls back to `Logo: Homebrew` otherwise (a `-HomebrewLogo` switch forces the fallback).
- **Checked on the 3DS** in the next run: the logo shows and the game starts. If the HOME
  Menu rejects it, the fallback ships and the logo waits.

### WP12a — The hatching, reworked (Noah 2026-09-24; his direction after run 2)
**The problem:** the hatchling appears whole inside an intact shell. A newborn is 0.79 wide
and 1.34–1.47 long; the egg is 0.74 wide and 1.0 tall. So the body, wings and tail poke
through the shell walls, and the shell stays in the nest (for three minutes) while the
hatchling sits in it. It also "just hatches": no moment where it breaks out.

**The fix (Noah's direction):**
- **The egg bursts into bits.** At the moment of hatching the shell breaks into a couple of
  dozen small pieces (cream outside, glowing inside), flung a little up and outward. They
  fall to the nest and the floor, bounce once, settle, and fade once the hatchling is
  named. The pieces are simulated in `src/core` (small rigid bits like the ball: gravity,
  the floor and the nest's rim, a bounce, friction) and drawn as one mesh with the static
  program: one draw call for the lot.
- **The dragon grows out of a small white blob.** Where the egg was, a small glowing white
  blob swells and shapes itself into the baby dragon over about 1.5 s. It's made from the
  hatchling's own mesh: each vertex moves from its place on a small sphere round the body's
  centre to its real place (one morph value in the dragon shader, eased with a little
  overshoot), and the white fades into its colours as it takes shape. No shell is ever
  round the dragon, so nothing can clip, and no second model is needed.
- **The moment:** the cracks glow brighter and the egg wobbles harder (as now), a pause, a
  flash of warm light, the burst, sparkles as the blob takes shape. Then the first blink,
  the first cry (`hatch-first-cry`, a stand-in until it arrives) and the look's name (D54:
  "It's a Cinderveined Ember!"); a wild egg's glowing crack pattern pays off here.
- **The bottom screen follows it:** the close-up shows the burst and the blob becoming the
  hatchling, then its face for the first blink, instead of cutting away.
- **Checked, not eyeballed:** PC tests that every bit comes to rest on the floor or in the
  nest (none through it, none outside the room), that the blob starts inside where the egg
  stood, and that the morph ends exactly on the hatchling's pose; an autotest run capturing
  the sequence frame by frame.
- This replaces the first plan (the newborn curled up inside, an egg of four or five
  shards, the cap riding on its head): simpler, and closer to what Noah pictures.

### WP12b — Grooming that fits together (moved to Beta, with the Shine Show; Noah, run 3)
**The tray first:** the brush, cloth and sponge go under one **Groom** button that opens its
own row (as food does), so three tools for one need don't crowd the tray. Then the design
below, to settle with Noah when WP12b comes up.
Today the brush, the cloth and the bath each raise Shine and clear dust, so the bath does it
all and the others feel pointless. A proposal: each tool has its own job, in an order that
makes a little routine.
- **The bath** washes off mud and grime (a new "grubby" level from the Wanderings, the
  garden, rain), in a tub with **visible water** (a surface that sways, splashes, suds on
  top) sized to the dragon. It leaves the dragon damp and its scales dull.
- **The cloth** dries it (a damp sheen fading as you rub) and polishes: Shine only rises on
  dry, clean scales.
- **The brush** is for loose scales: growing dragons shed (more before a stage-up), and
  brushing them out keeps the coat even, with a little pile of shed scales to sell or
  craft with; it's also what the Shine Show (Beta) judges.
- The needs bar shows what's missing (a droplet when damp, a smudge when grubby).

### WP12c — Running (Noah, run 3)
- A **sprint / zoomies clip** for hatchlings (a scamper with bounding hops) and a gallop for
  grown dragons, used in chase games, fetch (a far throw), toy runs and a happy burst of
  zoomies after a bath or a favourite food. The walk and trot stay; run speed matches the
  new clip's stride, as the others do.

### WP12 — The dragons update: every look, every breed (D54)
- **Looks per dragon (D54):** the current look and V1–V3 all ship. A look gene per dragon
  (saved), inherited with surprises (50/50 a parent's, sometimes random; wild 8% by base
  odds, about 25% with a wild parent). Each look is drawn by its own forms and shading (the
  renderer keeps each look's forms loaded and sets the ramps and the veins' stage per draw).
  Names: Classic, Pebbleback, Tallneck + breed, and a wild name per element (Cinderveined,
  Glimmertide, Stormstreak, Mossglow, Rimelight, Starveined; hybrids follow their base
  element). Wild looks for the other five elements, each its element glowing through the
  cracks. Revealed and named in the hatching; a wild egg's shell has a faint glowing crack
  pattern. The profile shows the look; the dev menu's style switch becomes "force look".
- **The Dragondex (D55):** every breed × look (84) plus the rare traits, filled in as you
  hatch or meet them; small rewards for completing a breed.
- **Photo mode (D55):** hide the UI, freeze the den, save a framed picture to the SD card
  (with the capture behind D57's screenshots).
- Memory and frame budgets measured with mixed looks in the den (three dragons, three looks),
  against the numbers from the first hardware look.
- The full parts library: horns (Crown, Crystal, Antler), frill (Leaf), tail tip (Plain),
  patterns (Solid, Runes), rare-trait looks (Iridescent, Melanistic, Leucistic,
  Starspeckle); all 6 base breeds and 15 hybrids; egg shells for all 6 elements.
- Dirt and mud on the chosen surface (D46).
- The 3D banner keeps the current look (the classic Ember); re-exported if the looks change it.
- Review sheets of every breed (R6, not blocking).

### WP13 — The run on the old 3DS (D34)
- Install the CIA with FBI (Noah's old 3DS runs Luma3DS + FBI); check the **3D banner**
  in the real HOME Menu (select it, let it
  animate, launch; fall back to the 2D banner if it misbehaves).
- Play through the checklist on the hardware: the 3-dragon den, the petting close-up,
  fetch, Wanderings, the Market, saves, sound, touch accuracy.
- Read the budget overlay on real hardware; fix whatever the hardware finds.

### WP14 — Wrap-up
- The playthrough checklist (`docs/plan/alpha-2-checklist.md`), docs sync, tag
  `v0.2.0-alpha2`.
- The 3D banner becomes `package_cia.ps1`'s default once run 2 confirms the fixes, with
  the flat one behind a switch (agreed with Noah 2026-09-24, D58).

## What's left to close Alpha 2 (2026-09-24)
Done: WP1–WP10, R5 (WP11), the first hardware runs (WP11b: the game runs on the old 3DS, the
3D banner shows). In order:
1. **WP11d, the performance pass** (in progress): check posing-ahead on the 3DS, then the
   next cuts from the profiler until the full den holds 16.7 ms.
2. **WP11c, our own boot logo** (small).
3. **WP12a, the hatching** Noah's way: the egg bursts into bits, the dragon grows out of a
   white blob.
4. **WP12, the dragons update** (the big one): looks per dragon with their odds and names
   (D54), the Dragondex, photo mode, the parts library, all 6 breeds and 15 hybrids, egg
   shells per element, rare-trait looks; then the budgets measured again.
5. **WP12c, running clips** (a scamper for babies, a gallop for grown dragons).
6. **WP11e, the 3D slider** (after WP11d; 30 fps in 3D is fine).
7. **WP13, the full run on the old 3DS** and **WP14, the wrap-up** (the checklist, docs, the
   tag `v0.2.0-alpha2`).

Moved past Alpha 2: the grooming redesign and its Groom button (Beta, with the Shine Show),
several dragons wandering at once (Beta, the Wanderings' next pass), the map's new look and
the open world, people and campaigns (1.0).

## Review gates (D47, D49)
| Review | What | Blocks? |
|---|---|---|
| R5 — Style | The current textured style vs three variants, plus the icon and the 3D banner | Decided 2026-09-24 (D54): all four ship as per-dragon looks |
| R6 — Breeds | Every breed in the chosen style | No |
| Hardware run | WP13 on Noah's old 3DS | **Yes**: Alpha 2 ends with it (D34) |
