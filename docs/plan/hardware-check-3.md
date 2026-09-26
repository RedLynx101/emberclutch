# The Beta runs on the old 3DS (WP17, D34)

Runs 1–12 are in [hardware-check-1.md](hardware-check-1.md), run 13 (Alpha 2) in
[hardware-check-2.md](hardware-check-2.md). Beta's runs: run 14 (the flyable valley, WP1),
run 15 (the nine new dragons of the revamp and walking in the valley, 0.2.2), run 16
(run 15's fixes, the new music and sounds, swimming, 0.2.3), then run 17 (the new dragons for
real, DR3), run 18 (after flying, the map and a few places) and run 19 (the whole milestone).

# Run 14 (0.1.15): the flyable valley

**For Noah.** Beta's technical test: can the old 3DS fly a dragon over an open valley at
30 fps? The valley is a placeholder (a height field, cone trees, a lake, a block for the
den's cliff, fog, the day's sky); the real landscape comes in step 2 with the look you pick
at R7. 0.1.15 also carries 0.1.14 (run 13's fixes), which hasn't been on the 3DS yet.
About 20–25 minutes; the checklists page (its link is in STATUS) has these steps with boxes to
tick and room for the numbers. Press **Y** wherever a step says so, or whenever something looks off:
each Y saves both screens and writes the frame time, the triangles and memory to the log,
and I copy them off afterwards (`tools\pull_shots.ps1`). It's a dev build: SELECT opens the
dev menu, L/R turn its pages.

**What the emulator says** (to compare): flying anywhere in the valley draws 6,100–7,100
triangles on the top screen (the ground 3,300–4,600 of them, the rest the dragon and
trees), 14–23 draws, 17.5 MB of linear memory free; the den's full count (about 9,800
triangles) ran 17–18 ms on this 3DS in run 13. The emulator can't say how long the CPU
takes to build the ground as you fly (up to 2 tiles a frame); that's the main question.

## 0. Install
The files are on the SD card (sent 2026-09-25 to the 3DS at .51): in **FBI**, SD → cias →
`emberclutch.cia` → Install CIA, over the game (the save stays). The `.3dsx` is at
`sdmc:/3ds/emberclutch/emberclutch.3dsx` too.

## 1. Run 13's fixes (0.1.14), quickly
1. **3D:** slider up, open a dragon's profile (tap the heartglow): the platform should sit
   under the dragon now, not stand out in front of the screen. In the den, the heart over the
   chosen dragon and the particles should sit with the dragons.
2. **A baby's walk:** a hatchling toddles with quick little steps, much faster than before.
3. **Tug for the ball:** when a dragon has the ball in its mouth, take hold near its mouth
   with the ball tool and pull.
4. **More games:** dev menu page 2, **Next game** (a play bow and sparring, stalking and a
   pounce, a tail chase).
5. **Spines:** a Tallneck juvenile (or any): no spines floating off the neck or back.

## 2. Into the valley
1. Dev menu page 1: **Overlay** on. Page 2: **Valley test**. You start on the grass in front
   of the den's cliff with your dragon, grown (a stand-in of its breed and look if it's
   young yet). The music is batch 1's `skyreach`.
2. Standing there: press **Y** (the number at rest).
3. The bottom screen shows the valley from above (the heart is you, the line the way you
   face), your height and speed, stamina, and the frame time: the smoothed ms, the **worst**
   frame of the last second, and how many frames of that second were **slow** (under
   30 fps).

## 3. Flying, with the numbers
The controls: **A** flaps (hold to climb; from the ground it takes off), let go to glide,
**B** dives, the **circle pad** (or D-pad) steers and pitches (pushed up: nose down),
**L/R** bank. Land by gliding slowly onto flat ground. **X** goes home.
1. **Low over the forest** by the den's cliff (the busiest spot: the nearest ground in full
   detail, and the trees): skim along it for a while, press **Y**.
2. **High over the middle** of the valley, the whole of it out to the fog: **Y**.
3. **Fast and low**, a long dive and then straight across the valley (new ground is built
   as you go): watch **worst** and **slow**; press **Y** if they jump.
4. **Over the lake** and along the river: **Y**.
5. Anything that hitches, pops in badly, or leaves gaps in the ground: **Y** there.

## 4. The 3D slider in the valley
1. Slide it up while flying low over the forest: **Y**. (In 3D the top screen is drawn
   twice, so it may be slower; the valley may drop the right eye's far tiles later if needed.)
2. Is the depth comfortable? The dragon should sit a little behind the screen, the valley
   going away into the fog.

## 5. Evening and night
1. SELECT, page 1: **+1 hour** until the sky turns (the sky and fog follow the den's clock:
   evening, then night). SELECT again to close and fly on.
2. A **Y** at dusk and one at night.

## 6. Home and back
1. **X** (or Home) takes you back to the den; the valley gives its memory back. Go in and out
   two or three times, then press **Y** in the den with the overlay on (LIN should be about
   what it was before the first trip).

## What to send back
- The numbers (the Ys in 2–5); whether it felt smooth, and where it didn't.
- How flying feels: the controls on the circle pad, the speed (the plan is 2–3 minutes to
  cross the valley), the camera, landing.
- The fog and draw distance, and the 3D.
- Anything else before the valley's real landscape starts (step 2).

## Results (2026-09-25)
Noah flew it on the old 3DS (ten Ys): **the flying is pretty good**. The valley ran
16.5–18.3 ms (the worst frame of a second 28–49 ms, at most one slow frame), 6,500–8,600
triangles, 19 MB of linear memory free. His note: on the ground the dragon stood stuck; he
should be able to walk with it. Done in 0.2.1 (run 15): the pad walks it, B runs. (Builds after Alpha 2 are 0.2.x: a CIA's last number stops at 15.)

# Run 15 (0.2.2): the nine new dragons, walking in the valley, the Blazeplume's banner

**For Noah.** The first nine kinds of the dragon revamp on the real 3DS (R11b: all nine
approved, the Duskwing's ears now grow out of its head), and walking on the ground in the
valley (run 14's note). Your save doesn't change: on the dev menu, **Next kind** shows every
dragon in the den as one of the new kinds in turn, for looking only; your dragons become
new kinds for real in DR3, with their eggs. About 20 minutes; the checklists page has these
steps (the Run 15 tab). **Y** saves both screens and the numbers, as before. And the HOME
Menu's 3D banner is now the Blazeplume's hatchling peeking out of the egg (D80): built exactly
as X was (the same pieces, nesting and turning), with no breaks at its neck or tail.

**What the emulator says** (to compare): one dragon of a new kind in the den ran 16.6–17.4 ms
in Azahar, each kind drawing 2,600–3,000 triangles (the rare colourings the most), no memory
faults; three dragons of one kind are heavier, which is one of the questions here. On foot in
the valley: 5,900–8,600 triangles, 16.7–17.4 ms.

## 0. Install
The files are on the SD card (sent 2026-09-26 to the 3DS at .61): in **FBI**, SD → cias →
`emberclutch.cia` → Install CIA, over the game (the save stays). It's 0.2.2: the game is
0.2.1's, with the new banner. `emberclutch-oldbanner.cia` beside it is the same game with X's
banner, in case the new one misbehaves (step 4).

## 1. The nine in the den
1. SELECT, page 1: **Overlay** on, then **Next kind**: every dragon in the den becomes a
   **Pouncer** (a toast names the kind). SELECT to close; watch them a while (walking,
   sitting, playing, sleeping), then press **Y**.
2. Again for each of the nine (Puffback, Curlstone, Crestwing, Ribbontail, Flurrytail,
   Glimmermoth, Duskwing, Blazeplume): SELECT, **Next kind**, SELECT, watch, **Y**. After the
   ninth, Next kind gives them their own looks back.
3. On any you like: **Kind colouring** steps through its four colourings (the fourth is the
   rare one): **Y** on the rare.
4. They show at the stage your dragons are; **Next stage** grows the chosen one along.
5. The **Duskwing**: its ears should join its head from the side and from behind now.
6. Which kinds are heaviest with three in the den? Note the overlay's ms (and **Y**) on the
   slowest.

## 2. Walking in the valley
1. Choose a kind first (page 1, **Next kind**), then page 2: **Valley test**. You start on
   the grass before the den's cliff, as that kind, grown.
2. **Circle pad up** walks; left and right turn it as it goes. Hold **B** as well to run
   (a trot, then a gallop; the Curlstone tucks into its rolling ball). Let go and it stops.
   **Y** walking and running.
3. It stops at deep water and at slopes too steep to climb; walk off a high edge and it
   glides.
4. **A** takes off as before; land slowly on flat ground and walk on.
5. Try two or three kinds, flying and walking each (each has its own wings and gait): **Y**.

## 3. Home
1. **X** goes back to the den. Page 1, **Next kind** until the toast says their own looks
   are back (or leave them as a kind; it isn't saved).

## 4. The HOME Menu banner
1. Close the game and rest the cursor on Emberclutch in the HOME Menu: the Blazeplume's
   hatchling should peek out of its egg, tilt its head, blink, its cream heart pulsing, the
   sparkles glinting, and hold still while the HOME Menu turns (as X did). Its neck and tail
   shouldn't show breaks as it moves.
2. If the HOME Menu freezes: hold **POWER** to turn off, then in FBI install
   `emberclutch-oldbanner.cia` instead (X's banner, the same game) and tell me.

## What to send back
- Which kinds look best and worst on the 3DS's screens, and anything wrong (floating,
  clipping, a colouring that doesn't read).
- The den's frame time with three of the heaviest kinds.
- How walking feels: the speed, the turning, running, the camera on foot.
- The banner: does it look right on the HOME Menu, and does it move cleanly?
- Anything else before DR3 (your dragons and eggs moving over to the new kinds).

## Results (2026-09-26)
The Blazeplume banner **froze the HOME Menu**; Noah went back to `emberclutch-oldbanner.cia`
(X's banner) and carried on. His notes (D81): swimming instead of stopping at the shore, landing
a little faster, running at least 3x as fast, a shadow as a height tell, the real valley at
least 5x the size; the Puffback's wings clipping as it moves; the Duskwing baby's neck tufts
hovering and grown Duskwings' heads meeting in their games; the mouths seen through at the
sides, a smaller bite and chewing; dragons stuck on the den's walls; the dragons' voices pitched
further apart. All of it is in 0.2.3 (run 16) but the voices, which come with DR3.

# Run 16 (0.2.3): run 15's fixes, the new music and sounds, swimming, a banner lab

**Not run:** folded into run 17 (0.2.4), which has all of it (Noah, 2026-09-26).

**For Noah.** Everything from your run 15 notes except the voices (with DR3, next): and your
batch 2 and 3 music and sounds are in. Your dragons are still their old selves; the new kinds
show by the dev menu as in run 15. About 15 minutes; the checklists page has these steps (the
Run 16 tab). **Y** saves both screens and the numbers.

## 0. Install
The files are on the SD card (sent 2026-09-26 to the 3DS at .61): in **FBI**, SD → cias →
`emberclutch.cia` → Install CIA, over the game (the save stays). It has X's banner, so the
old-banner CIA is gone from the card. And `cias/lab/banner-lab-a.cia`: install it too (step 5).

## 1. Sounds
1. In the den the care and toy sounds are the real ones now (brushing, suds, the tub, the
   bowl, the ball, the rope, the orb, the egg's turn and heartbeat, a sniff, a giggle, a
   grumble); the Market's murmur plays at the stalls. Anything too loud, too quiet or wrong?

## 2. The valley
1. Page 2, **Valley test** (choose a kind on page 1 first if you like). The music is **Valley
   Day** by day and **Valley Night** at night; the meadow's breeze or the night's crickets
   underneath, wind as you climb, the wings fluttering in a glide, wingbeats, the take-off
   and the landing, a whoosh as you dive, the lake lapping near the water.
2. **Run** with B: at least three times as fast as before.
3. **Swim:** walk into the lake: it splashes in and swims, bobbing; B paddles faster; walk out
   on the far shore, or take off from the water with A. Gliding down slowly onto the lake it
   splashes in; skimming low and fast over it throws spray.
4. **Landing** works at a higher speed now.
5. **The shadow:** coming down to land, its shadow on the ground grows and darkens under it.
   **Y** close to the ground.

## 3. The dragons
1. Dev menu page 1, **Next kind**: the **Puffback**'s little wings shouldn't sink into its
   flank as it walks and runs; the **Duskwing** baby has no tufts hovering under its chin now.
2. Grown Duskwings (or any long neck) playing together: their heads shouldn't meet in the
   middle any more.
3. Every kind's **mouth** (yawning, chomping, eating from your hand) should be dark inside
   right to the corners, never see-through.
4. **Feeding by hand:** a smaller bite, then three little chews before the next.

## 4. The den
1. Watch a while with two or three dragons: they shouldn't walk into the walls or props for
   long, or get stuck on each other.

## 5. The banner lab
1. On the HOME Menu, move to **Banner lab A** and rest on it (never start it): it's the
   Blazeplume banner cut down to X's size (14 materials, 32 pieces: the frozen one had 20 and
   37). If it freezes, hold **POWER** and carry on. If it holds, the Blazeplume can be the
   game's banner in the next build.
2. Delete it afterwards in FBI: Titles, "Banner lab A", Delete Title.

## What to send back
- The sounds and the music: anything to change?
- Swimming, running, landing and the shadow: how they feel.
- Whether the lab banner held, and anything still wrong with the dragons or the den.

# Run 17 (0.2.4): your dragons become the new kinds, and everything from run 16

**For Noah.** This is the only run to do now (run 16 is folded in). 0.2.4 has all of run 16
(your run 15 fixes, batch 2 and 3's music and sounds, swimming, the banner lab) and DR3:
**every dragon and egg in your save becomes one of the nine new kinds** the first time 0.2.4
opens it, rolled once and kept from then on (names, ages, bonds and families stay), and voices
pitched by kind and stage. About 25 minutes. **Y** saves both screens and the numbers.

## 0. Install
In **FBI**: SD → cias → `emberclutch.cia` → Install CIA, over the game (the save stays, and moves over to the new kinds as it loads). Then SD → cias → lab → `banner-lab-a.cia` → Install CIA too (for step 7).

## 1. Your dragons
1. Start the game. In the den each dragon is now a kind in a common colouring: the top line
   reads "Name - Colouring Kind Stage", then its mood and its manner (Brave, Shy, Playful,
   Proud, Sleepy, Curious, Gentle, Mischievous, Greedy or Stubborn), which is how it behaves too.
   Switch between them with < >.
2. **The profile** (the heart, top right of the bottom screen), About: the five stats (Wing,
   Wit, Might, Breath, Stamina, out of 10), its elements and how rare its kind is, and its
   traits (the rarer ones in gold). Family shows each one's kind.
3. **Voices:** a hatchling squeaks well above a grown one, and a big kind sounds deeper than a
   small one.

## 2. Eggs, the Market and the Dragondex
1. Your eggs wear their kind's shell and markings, glowing in their element's colour. When one
   hatches it says what it is ("It's a Moss Puffback!").
2. The Market's **egg of the day** is a kind: 150 Gleam common, 250 harder to find, 400 rare.
3. **START → Dragondex:** nine kinds over two pages, four colourings each (the last, "*", is the
   rare one); your hatched dragons are in it. Tap a cell to see that one turning up top.

## 3. Sounds
1. In the den the care and toy sounds are the real ones now (brushing, suds, the tub, the bowl,
   the ball, the rope, the orb, the egg's turn and heartbeat, a sniff, a giggle, a grumble);
   the Market's murmur plays at the stalls. Anything too loud, too quiet or wrong?

## 4. The valley
1. SELECT (dev menu), page 2, **Valley test**: you fly your own dragon, as its new kind. The
   music is Valley Day by day and Valley Night at night; the breeze or the crickets underneath,
   wind as you climb, wingbeats, the take-off and landing, a whoosh as you dive, the lake lapping.
2. **Run** with B on the ground: at least three times as fast as before.
3. **Swim:** walk into the lake: it splashes in and swims, bobbing; B paddles faster; walk out on
   the far shore, or take off from the water with A. Gliding down slowly onto the lake it
   splashes in; skimming low and fast over it throws spray.
4. **Landing** works at a higher speed now.
5. **The shadow:** coming down, its shadow on the ground grows and darkens under it. **Y** close
   to the ground.

## 5. The dragons
1. Dev menu page 1, **Next kind** shows every dragon as each kind in turn (not saved; press
   until the toast says their own looks are back). The **Puffback**'s little wings shouldn't sink
   into its flank as it walks and runs; the **Duskwing** baby has no tufts hovering under its chin.
2. Grown dragons playing together: their heads shouldn't meet in the middle any more.
3. Every kind's **mouth** (yawning, chomping, eating from your hand) is dark inside right to the
   corners, never see-through.
4. **Feeding by hand:** a smaller bite, then three little chews before the next.

## 6. The den
1. Watch a while with two or three dragons: they shouldn't walk into the walls or props for
   long, or get stuck on each other.

## 7. The banner lab
1. On the HOME Menu, move to **Banner lab A** and rest on it (never start it): the Blazeplume
   banner cut down to the working banner's size. If it freezes, hold **POWER** and carry on. If it
   holds, the Blazeplume becomes the game's banner in the next build.
2. Delete it afterwards in FBI: Titles, "Banner lab A", Delete Title.

## What to send back
- Which kinds your dragons became, and whether their manners and stats feel right.
- The voices: far enough apart now?
- The sounds and the music: anything to change?
- Swimming, running, landing and the shadow: how they feel.
- Whether the lab banner held, and anything still wrong with the dragons or the den.
- Note: on the dev menu, **Change kind** (page 1) and **Change colour** (page 2) change a dragon
  for good; **Next kind** and **Kind colouring** only show.

## Results (2026-09-26)
All 20 steps done. Noah's notes (D83):
- **Mouths:** the Blazeplume's open mouth is still see-through **from above** (others may be
  too); the see-through issues show at least on the Blazeplume.
- **A stall:** the game froze in the den once for a few seconds, then carried on by itself.
- **Sounds:** one of the dragon's grass steps sounds bad, one good: to pick by ear
  ([the takes page](https://claude.ai/artifact/BP8VgPaAL3MnsN59jW7jxE)).
- **The valley:** the 3D slider does nothing there (fine for the test valley; the real one needs it).
- **Banner lab A froze** the HOME Menu: the size (materials and pieces) isn't what freezes the
  Blazeplume banner. X stays.
- **Grooming, simpler:** cleanliness instead of shine, the polishing cloth out, the brush a
  slightly faster stroke than the hand (for attention and play), petting and brushing both
  turning the dragon with L/R and reaching its back, sides, head, neck and chin; brushes of
  different kinds from shops later.
- Everything else passed: the dragons moved over to their new kinds, the profile, voices,
  eggs, the Market, the Dragondex, the sounds, swimming, running, landing, the shadow, the
  Puffback's wings, the Duskwing baby, the heads apart, feeding, the den.

# Run 18 (0.2.5): grooming made simpler, run 17's fixes

**For Noah.** Run 17's notes (D83): grooming the way you described it, the mouths closed at
the sides, the babies' wings, one soft grass step, the Dragondex's typing, and the den's stall
chased two ways (saving no longer waits for the SD card; your dragons' kinds load ahead
instead of on the spot). About 10 minutes. **Y** saves both screens and the numbers.

## 0. Install
In **FBI**: SD → cias → `emberclutch.cia` → Install CIA, over the game (the save stays).

## 1. Grooming
1. The top bar reads **Clean** where Shine was, and the tray has no cloth: hand, food, brush,
   sponge, toy.
2. **The hand:** press **R**: it turns its right flank to you, and the close-up shows its body;
   again: its back; again: its left flank; again: facing you (**L** goes the other way). Stroke
   its back, sides, neck, head and chin; one place it likes more (the hearts come faster).
3. **The brush:** a little faster than the hand and it pleases more (Play fills quicker); L / R
   turn it the same way. It no longer cleans.
4. **The bath:** rub the suds in and rinse: "Clean and gleaming, nose to tail!", a burst of
   sparkles, and Clean fills right up. Only the bath cleans now; muddy trips knock Clean down.

## 2. The dragons
1. **Mouths:** watch a Blazeplume (and others) yawn, from the side and from above: dark inside
   right across, never see-through.
2. **Babies' wings:** a Pouncer and a Blazeplume hatchling show their little wings on their
   backs instead of sinking into them.

## 3. The valley and the Dragondex
1. SELECT, page 2, **Valley test**: walking on grass has one soft step now, a little different
   each time.
2. **START → Dragondex:** every kind shows its typing (a dot per element on the list, chips up
   top); one you haven't met says what to pair ("A crossbreed: pair an Ember kind with a Gale
   kind").

## 4. The den
1. Play in the den a good while. If it ever freezes for a moment, carry on and close the game
   normally afterwards: it writes what happened to `hitches.txt` on the SD card, and I read it
   next time.

## What to send back
- How the grooming feels (the turning, the brush against the hand, the bath).
- Any see-through mouths or clipping wings left.
- Whether the den froze again.

## Results (2026-09-26)
All 10 steps done. Noah's notes (D85), with the 3DS's screenshots 85-91 and its hitch log
(`build/run18`, not in git):
- **Mouths:** a grown Blazeplume still shows empty space at the edges of its mouth while it's
  petted; still see-through on the bottom screen (the close-up looks up at the face).
- **The Flurrytail hatchling** has an obvious rift in its texture down the middle.
- **The grass step** is good but now a little too quiet.
- **The tray:** the brush goes under the hand's button as a pop-up, as the toys do (the hand
  and the brush do the same job now), which frees two places: one for the **Den** (den
  customisation) and one more that suits the game's plans.
- **Cameras:** a free-fly camera in the open world; in places like the den, let the player
  turn the view a little (each place keeps one fixed view, with or without your character).
- **The den:** dragons fly in it a little (classy, cute, careful), and it's a little more alive.
- **The hitch log:** no stall in the den this time (saving on its own thread). The title
  screen stuttered while the save's kinds loaded ahead: one piece of a kind takes 0.3-0.9 s to
  read on the 3DS (once 2.3 s), so the run 17 stall was most likely a kind read on the spot.
  One 0.56 s frame in the valley with nothing marked. The den ran 20-26 ms with three grown
  Blazeplume-sized kinds (shots 90-91): over the 16.7 ms budget.

# Run 19 (0.3.0, Beta 1): the valley, its places and people, the challenges, the festival

**For Noah.** Beta 1, all of it at once (D85-D87, [what's in it](beta.md#beta-1-whats-built-2026-09-26-with-creative-freedom-d85-d87)):
the real valley with its fourteen places, you and the villagers, riding with you on your
dragon's back, the lead, finds and the map's fog, the Wanderings seen in the world, the
challenges and their cups, the Lantern Festival with the star dragon, and the fixes (mouths,
sleeping, skins, the den's frame time). Your save carries over. Take it in one or two
sittings; **Y** saves both screens and the numbers wherever you are. Sent to the 3DS at .54 on
2026-09-26 with the big review's fixes (D88): the lead in your left hand and always there, Rowan's
cane, trees on the floating islands and thicker woods, the mill bridge's ends on the ground, a
lower camera, and what A does shown only close by and facing it.

## 0. Install
1. In **FBI**: SD → cias → `emberclutch.cia` → Install CIA, over the game (the save stays).
2. The banner labs: SD → cias → lab → **Install all CIAs** (four titles, Banner lab B to E;
   step 9).

## 1. Your look
1. **Continue:** your character's creator comes first (once, for a save from before Beta): a
   row each for clothes, hair, hair colour, skin, outfit and eyes; the circle pad turns you;
   **Done** (a cheer), then the den.
2. Later: START → Settings → **Your look** changes it any time.

## 2. Out into the valley
1. From the den, **X** (or the tray's **Outing**) takes you and your partner out by the den's
   arch in its cliff. A hatchling or juvenile walks on its **lead**; a grown one at your side.
2. Walk (circle pad), run (**B**), turn the view (**L/R**); walk round the well and the
   villagers (you slide round them). Try the **3D slider** here.
3. **The map** (bottom screen): fogged where you haven't been, clearing round you; tap a pin to
   go there. Far off, a **ring of mountains** stands round the valley.

## 3. The places
Tap each pin in turn and look round (A at a door goes in, as before):
1. **The Market:** the **egg of the day** on its stand, and the day's goods on the goods
   stall (a crate where one's sold out); walk up to either and press **A**: its page of the
   shop. Buy the egg: it's gone from the stand when you come back out.
2. The Nesting Stone, the Sanctuary, the Cold Vault, the Trailhead, the Arena, Mirror Lake,
   the Keeper's Lodge, the Orchard, the **Windmill** (its sails turn), the **Hidden Grotto**
   (behind the falls), the Floating Isles and Starwatch Ruins (from the air).
3. At dusk and night: windows glow, and every festival lantern you've lit burns.

## 4. People and the Lantern Festival
1. Talk to **Rowan** by the falls (**A** near him): the festival's first quest. Each villager
   turns to you, waves, talks (voiced, a letter at a time, with a portrait) and nods.
2. Light the den's lantern: stand by it and press **A** (your dragon's breath).
3. Follow the quests (the **Journal** in the tray shows them, your places and finds): Maple's
   Fruit Catch, Bram's stray in the meadow (walk your dragon through the flowers), Sable's
   Wandering, the rest. Once your dragon can fly, watch the sky over the floating isles.

## 5. Riding and flying
1. With a grown partner, turn to face it (it waits while you stand still) and press **A**: you
   climb on and sit on its back. **A** flaps,
   **B** dives, **L/R** bank, the circle pad steers.
2. **Its tail in the wind:** turn right and the tail swings right; dive and it streams out
   straight behind.
3. Land on flat ground; **D-pad down** gets off.

## 6. Finds and the Wanderings
1. **Gold stars** glint about the valley (22, ten only from the air): walk or fly to one for
   Gleam, a trinket, or (twice) a wild egg.
2. Send a dragon on the **Wanderings** (the Trailhead's door): its pin goes round a loop on the
   map; walk a while with the 3DS closed and it moves on. A grown one flies circles over its
   spot.

## 7. The challenges
1. The notice board by the **arena's gate** (or talking to **Wren**): the Lantern Trial,
   Ember cup. Watch the crystal lanterns, then tap them in order: your dragon breathes each
   alight in its element.
2. The board by the **orchard's cart** (Maple): Fruit Catch, Ember. Flick fruit up from the
   basket; your dragon leaps, snaps or dives.
3. With a grown partner: **Sky Rings**, Ember. Fly the gold rings to the floating isles; the
   last lights their lantern. Try the Flame cup too: your best run flies beside you as a wisp.
4. In the den: the trophies and rosettes you've won on the shelves.

## 8. The den
1. With three or more grown dragons out: the frame time (overlay) and **Y**.
2. At night (or after a nap): the Pouncer family (Pouncer, Blazeplume, Kindlemoss, Lilyfin)
   curl up like cats to sleep; the Crestwing and Glimmermoth lie down.
3. Pet a grown Blazeplume's chin and feed it: the mouth's corners in the close-up, never
   see-through.

## 9. The banner labs
Each is the game's banner with one thing from the Blazeplume banner that froze (lab A).
1. On the HOME Menu, rest on **Banner lab B** about 15 seconds (never start it), then **C**,
   **D** and **E**. If one freezes, hold POWER to turn off, turn on again, note which, and go on
   with the next.
2. If they hold: **B** looks just like the game's banner (only names inside changed), **C** is
   the Blazeplume's body in the egg (patchy on purpose), **D** has its tail raised behind its
   shoulder, **E** is the old hatchling in the Blazeplume's orange.
3. Afterwards in FBI: Titles → each "Banner lab" → Delete Title.

## 10. Performance
**Y** in: the Market's square, flying high over the valley, the arena during a challenge, and
the den with three grown dragons.

## What to send back
- How it feels: the valley's look and spacing, getting about, the camera, the people and
  their talk, riding.
- The challenges: fun? too easy or hard (Sky Rings' Flame and Starfire especially)?
- Anything broken, stuck or confusing.
- Which banner labs froze and which held.
