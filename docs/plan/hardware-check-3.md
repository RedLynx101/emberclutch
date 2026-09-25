# The Beta runs on the old 3DS (WP17, D34)

Runs 1–12 are in [hardware-check-1.md](hardware-check-1.md), run 13 (Alpha 2) in
[hardware-check-2.md](hardware-check-2.md). Beta plans three: run 14 (the flyable valley,
WP1), run 15 (after flying, the map and a few places), run 16 (the whole milestone).

# Run 14 (0.1.15): the flyable valley

**For Noah.** Beta's technical test: can the old 3DS fly a dragon over an open valley at
30 fps? The valley is a placeholder (a height field, cone trees, a lake, a block for the
den's cliff, fog, the day's sky); the real landscape comes in step 2 with the look you pick
at R7. 0.1.15 also carries 0.1.14 (run 13's fixes), which hasn't been on the 3DS yet.
About 20–25 minutes. Press **Y** wherever a step says so, or whenever something looks off:
each Y saves both screens and writes the frame time, the triangles and memory to the log,
and I copy them off afterwards (`tools\pull_shots.ps1`). It's a dev build: SELECT opens the
dev menu, L/R turn its pages.

**What the emulator says** (to compare): flying anywhere in the valley draws 6,100–7,100
triangles on the top screen (the ground 3,300–4,600 of them, the rest the dragon and
trees), 14–23 draws, 17.5 MB of linear memory free; the den's full count (about 9,800
triangles) ran 17–18 ms on this 3DS in run 13. The emulator can't say how long the CPU
takes to build the ground as you fly (up to 2 tiles a frame); that's the main question.

## 0. Install
Once the files are on the SD card (tell me when the 3DS is on the network): in **FBI**,
SD → cias → `emberclutch.cia` → Install CIA, over the game (the save stays). The `.3dsx`
is at `sdmc:/3ds/emberclutch/emberclutch.3dsx` too.

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
