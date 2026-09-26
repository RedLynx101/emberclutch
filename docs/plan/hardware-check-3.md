# The Beta runs on the old 3DS (WP17, D34)

Runs 1–12 are in [hardware-check-1.md](hardware-check-1.md), run 13 (Alpha 2) in
[hardware-check-2.md](hardware-check-2.md). Beta's runs: run 14 (the flyable valley, WP1),
run 15 (the nine new dragons of the revamp and walking in the valley, 0.2.1), then run 16
(the new dragons for real, DR3), run 17 (after flying, the map and a few places) and run 18
(the whole milestone).

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
