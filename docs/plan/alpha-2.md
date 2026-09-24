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

### WP12a — The hatching, reworked (proposed; Noah 2026-09-24)
**The problem:** the hatchling appears whole inside an intact shell. A newborn is 0.79 wide
and 1.34–1.47 long; the egg is 0.74 wide and 1.0 tall. So the body, wings and tail poke
through the shell walls, and the shell stays in the nest (for three minutes) while the
hatchling sits in it. It also "just hatches": no moment where it breaks out.

**The fix:**
- **It starts curled up inside.** A new clip, `hatch_emerge`: from the curl pose (head
  tucked, tail wrapped round) it pushes up, the snout breaks through first, then the head
  comes out wearing the cap, then it stands and stretches. An egg sized to hold the curled
  newborn (about 1.2 tall instead of 1.0; the nests have room).
- **The shell breaks apart instead of staying whole.** A shattering egg model: the cap and
  four or five lower shards, each a rigid piece on its own bone. As the hatchling stands,
  the shards tip outward about their bottom edges and fall onto the straw with a small
  bounce, settling in a ring clear of its body; the cap rides on its head until it shakes
  it off. The shards fade once it's named and walks out.
- **The moment:** the cracks glow brighter and the egg wobbles harder (as now), a pause, a
  flash of warm light from inside, then the break. Shell crumbs and dust, the first cry
  (`hatch-first-cry`, stand-in until it arrives), the first blink, and the look's name
  (D54: "It's a Cinderveined Ember!"); a wild egg's glowing crack pattern pays off here.
- **The bottom screen follows it:** the close-up shows the egg breaking and then the
  hatchling's face for the first blink, instead of cutting away.
- **Checked, not eyeballed:** a PC test that the curled newborn fits inside the egg at the
  start (every body vertex inside the shell's inner surface), that no shard passes through
  the hatchling's body capsules at sampled moments of the sequence, and that the shards'
  resting ring clears its footprint; an autotest run capturing the sequence frame by frame.

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
- **Photo mode (D55):** hide the UI, freeze the den, save a framed picture to the SD card.
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

## Review gates (D47, D49)
| Review | What | Blocks? |
|---|---|---|
| R5 — Style | The current textured style vs three variants, plus the icon and the 3D banner | Decided 2026-09-24 (D54): all four ship as per-dragon looks |
| R6 — Breeds | Every breed in the chosen style | No |
| Hardware run | WP13 on Noah's old 3DS | **Yes**: Alpha 2 ends with it (D34) |
