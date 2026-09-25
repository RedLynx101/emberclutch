# The Alpha 2 run on the old 3DS (WP13, D34)

**Run 13 (2026-09-25, 0.1.13): Alpha 2 passes (D72).** Noah's notes, point by point: his
save's looks good; the hatching great; the full den good; the 3D "really cool", but the
profile's platform stood out on the screen in front of the dragon and the den's selection
heart floated on the screen, far from the dragons; photo mode works; running good, and he
wants to fight for the ball back when a dragon holds it; mud works; the breed banners work
(their pictures, the Dragondex's and the fast-travel map will be redone with the open
world). Also: babies walked far too slowly; more games between dragons; spines floated on
some dragons (a Tallneck Tide juvenile). **0.1.14 fixes all of it** (below the steps). Next:
the sit-down for the places, the map, the open world, flying and challenges
([brief](beta-sitdown.md)).

**0.1.14:**
- **3D:** 2D drawn with the 3D now sits at its own depth: the profile's platform and glow,
  the Dragondex's, the Sanctuary's and Vault's plinths, the Nesting Stone, the Wanderings'
  hills and path, and in the den the heart over the chosen one and every particle. Measured
  in the emulator (dev menu page 2, Stereo preview): the right eye sees the dragon 6.6 px
  over, twice as far 10 px, and the profile's platform moves with its dragon.
- **Babies toddle:** quick little steps of their own instead of the grown walk on the baby
  body, about 3.7 times the old pace, feet planted.
- **Tug for the ball:** with the ball in its mouth, take hold near its mouth with the ball
  tool and pull; a playful one hangs on longer, a shy one lets go soon. Let go first and
  it keeps it.
- **More games:** a play bow and then sparring (reared up, batting with the front paws; one
  rolls over), stalking one that isn't looking and pouncing into a chase, and chasing its
  own tail. Dev menu page 2: Next game.
- **Spines on the body:** parts are now seated in the idle pose and for each build (a
  young dragon's raised neck, and a sturdy or long neck, lifted the first neck spine off);
  a PC test measures every spine against the posed body for every look, build and stage.

**For Noah.** Everything planned for Alpha 2 is built (runs 1–12 are in
[hardware-check-1.md](hardware-check-1.md)). This run checks the whole of it on the real
hardware before the tag. About 40 minutes. Press **Y** whenever something looks off or shows
a number worth keeping: I copy the screenshots off afterwards (`tools\pull_shots.ps1`).
It's a dev build: SELECT opens the dev menu, L/R turn its pages.

## 0. Install
Once the files are on the SD card (tell me when the 3DS is on the network): in **FBI**,
SD → cias → `emberclutch.cia` → Install CIA, over the game (the save stays; the banner is
still run 12's X). The `.3dsx` is at `sdmc:/3ds/emberclutch/emberclutch.3dsx` too.

## 1. Your own save
1. Continue. Your dragons now each have a look (D66: Classic, Pebbleback, Tallneck, or
   rarely wild), fixed by who they are. The profile (tap the heartglow) names it.
2. **The wings** (your note after run 12): walk them about, throw the ball. The start of each
   wing should sit on the back while walking and in every animation.
3. START → **Dragondex**: your hatched dragons are already in it. Tap a filled square: that
   breed in that look turns on the top screen.

## 2. A hatching
1. Dev menu page 1: **Add egg**, then with it picked (D-pad) **Hatch now**.
2. Watch: it shakes harder, stills and glows, a warm flash, the shell bursts into bits that
   fall and settle, the hatchling grows out of a white blob, blinks, cries, "It's a …!".
3. Name it. After the "Say hello" toast, the Dragondex tells you what's new.
4. Anything stutter? Press Y during the burst if it does.

## 3. A full den, three looks, with the numbers
1. Dev menu page 1: **Next stage** until grown on the new one, **Add dragon** to fill the
   beds. Page 2: **Mix looks** (Pebbleback, Tallneck and wild). Page 1: **Overlay**.
2. Let it run a minute. Press **Y** (the frame time, triangles and memory go in the log).
   The emulator says 6,814 + 2,954 triangles and 17.8 MB free; after run 12 the full den
   ran about 17–18 ms.

## 4. The 3D slider (WP11e)
1. In the den, slide the 3D slider up. The dragons should stand a little behind the
   screen, the room further back, the names and buttons on the screen.
2. Is it comfortable at the top of the slider, and halfway? (Too strong or too weak, and
   I'll change the depth.) It may run at 30 fps in 3D (you said that's fine): press Y with
   the overlay on to catch the number.
3. Try it in the Sanctuary (the dragon turning on the top screen) and in the Dragondex.

## 5. Photo mode
1. In the den, the little **camera** under the heartglow. The den holds still.
2. **X** switches between the whole den and a close framing; the arrows pick whose name goes
   on it. **A** (or the big button) snaps it: a gold frame with the name and the date.
3. They're saved in `sdmc:/3ds/emberclutch/photos/`; I'll copy them off with the screenshots.

## 6. Running
1. Dev menu page 2: **Zoomies** on a grown dragon: laps round the den at a gallop, then a
   wag. On a hatchling: a bounding scamper.
2. Throw the ball far: it runs for it. A game of chase between two dragons runs too.
3. A bath or a favourite food sometimes sets off zoomies by themselves.

## 7. Dirt and mud
1. Dev menu page 1: **Dust/mud/bath** once (dusty), again (muddy: brown blotches all over),
   again (bathed). Or send one on the Wanderings and walk a while: it comes home muddy on the
   legs, belly and tail.
2. Brushing lifts the mud slowly; the bath at once.

## 8. A breed's banner
1. Dev menu page 2: **Dex: this breed** on any dragon completes its breed in the Dragondex:
   150 Gleam and that breed's banner hangs in the den's banner spot at once.
2. START → Dragondex → pick the breed (a gold heart marks it) → **Take it down** /
   **Hang banner**.

## 9. Everything else (the Definition of done)
A quick go at each: the Market (food, goods, the egg of the day), the world map's trips,
the Sanctuary and Cold Vault swapping dragons in and out, a pair at the Nesting Stone,
the Wanderings with real steps, touch in the close-up, sound and music, Save & quit then
start again.

## What to send back
- The numbers (Y in 3 and 4), and anything slow, glitchy or wrong.
- How the 3D feels, and whether the wing roots look right now.
- Anything you'd like changed before the tag (`v0.2.0-alpha2`); then the Beta sit-down (D65).
