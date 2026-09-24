# First look on the old 3DS (before WP12, D55)

**For Noah.** A short check on the real hardware before the look system (D54) keeps up to
four sets of dragon models in memory. About 15 minutes. The full hardware run (WP13, D34)
still closes Alpha 2.

**Run 1 (2026-09-24, 0.1.1):** the 3D banner played, but the game stopped before it
started ("The SD card was removed"): the CIA had no boot logo. **Run 2 (0.1.2):** the game
started, but the 3D-banner CIA froze the HOME Menu (a flag it needs was dropped) and the
game crashed at its first 3D frame (a null texture read the emulator hides). Both fixed in
**0.1.3** ([alpha-2.md](alpha-2.md), WP11b).

**Run 3 (0.1.3):** the game runs (screenshots and numbers in [alpha-2.md](alpha-2.md),
WP11b); both 3D-banner CIAs froze the HOME Menu, which also turned out to keep an old banner
per title; Next style froze the 3DS (fixed in 0.1.4).

**Run 7:** 0.1.7 plays (the waiting egg hatched); the full den averages ~19 ms. **Run 8:** lab
M, N, O in `/cias/lab/`: the banner without turning, with three cute sounds (sparkle-chirp,
chirp-chirp, hello). Pick one; tell whether the banner still turns while it sits there or
only as it comes in (the HOME Menu turns every 3D banner in).

**Run 6:** J passed, K froze (the hidden triangle matters). **Run 7 (0.1.7):** delete the old
Emberclutch title in FBI (Titles), then install `emberclutch-3dbanner.cia`: it's a new title
(0xEC0C2), so the HOME Menu has no old banner for it, and the save on the SD card carries
over. Check the banner (sparkles glinting, not turning; the theme's bar of music), the egg
that waited for a bed, the bowl with mixed foods; press Y in the full den and the close-up
for the profiler's numbers (posing now happens before the frame).

**Run 5 lab 2:** F, G, I passed, H froze: one mesh more than the dragon, egg and wordmark
is enough (D63). **Run 6:** lab 3 (J = the new banner with its sparkles, K = the same without
the hidden triangle) in `/cias/lab/`; if J passes, delete the Emberclutch title in FBI and
install `emberclutch-3dbanner.cia` (0.1.6): the game's own title with the new banner. The
flat `emberclutch-2d.cia` and the `.3dsx` are 0.1.6 too.

**Run 4 lab 1:** A and B passed, C, D and E froze: the new scene, not the sound. **Run 5:**
the banner lab's second round (F, G, H, I, in `/cias/lab/`, same steps as below), and 0.1.5
(the flat banner, with the section profiler: press Y in the full den and the close-up).

**Run 4** (0.1.4, 2026-09-24):
1. **The banner lab.** In FBI: SD → cias → lab → install the five `banner-lab-*.cia`. On the
   HOME Menu, open each `Banner lab A`…`E` present and select it; if it freezes, hold POWER
   and carry on with the next. Never start them. Tell me which froze. Then delete them in
   FBI (Titles → Banner lab … → Delete Title).
2. **The game.** In FBI, delete the Emberclutch title first (Titles → Emberclutch → Delete
   Title: the HOME Menu's cached banner goes with it; the save is on the SD card and stays),
   then install `emberclutch-2d.cia`. Or run `emberclutch.3dsx` from the Homebrew Launcher.
3. Try the fixes (alpha-2.md, WP11b run 3), then the steps below, pressing Y at anything odd.

**Screenshots (new):** press **Y** anywhere in the game. Both screens (and the overlay, when
it's on) go to `sdmc:/3ds/emberclutch/screenshots/` with a line of numbers in `log.txt`, and
a toast says which one it was. Press it at each number below and at anything that looks
off; Claude copies them off over FTP afterwards (`tools\pull_shots.ps1`).

Both are dev builds: SELECT opens the dev menu, L/R turn its pages.

## 1. Install
The 0.1.3 CIAs are on the SD card in `/cias/` (uploaded over FTP on 2026-09-24, sizes
checked). In **FBI**: SD → cias →
`emberclutch-3dbanner.cia` → Install CIA. It installs over the last one (same title, newer
version; the save stays). Sound needs your console's own DSP firmware at `sdmc:/3ds/dspfirm.cdc` (from the
DSP1 homebrew); if other homebrew has sound, it's there already. The save goes to
`sdmc:/3ds/emberclutch/`.

## 2. A full den, with the numbers
1. New game, pick an egg. Dev menu (SELECT), page 1: **Hatch now**, name it.
2. Dev menu page 1: **Next stage** twice, **Add dragon** twice, **Add egg** twice. Close.
3. Dev menu page 1: **Overlay**. The top screen shows the frame budget.
4. Let it run a minute. Note (or photograph the top screen):
   - line 1: the frame time (**ms**; 16.7 is a full 60 fps), CPU, GPU;
   - line 2: **TRI** (top/8000 + bottom/3500), DRAW;
   - line 3: **LIN** (linear memory free), VRAM, **APP** (application memory free).
5. Try the petting close-up, a ball throw, the map, the Market.

## 3. Every look in memory at once
1. Dev menu page 2 (R): **Probe: all looks**. A toast shows the linear memory before and
   after (in the emulator: 21.4 → 18.0 MB free).
2. Note the overlay's **LIN** and **APP** again, and the frame time.
3. Page 2: **Next style (R5)** a few times: V1, V2 and V3 on the real screen (the 3D slider
   too, if you like).
4. **Probe: all looks** again releases them.

## 4. The 3D HOME Menu banner
It ran on run 1 (0.1.1) and froze the HOME Menu on run 2 (0.1.2). On 0.1.3, check the fixes:
1. The icon is square: no black corners.
2. Selected, the top screen shows the baby Ember in its cracked egg with no wall behind it,
   the gold EMBERCLUTCH readable from the front (mirrored from behind, as the banner turns),
   and nothing of the dragon through the back of the egg.
3. The sound is a bar of the title theme.
4. If the HOME Menu freezes or shows nothing: hold POWER to turn off, install
   `emberclutch-3dbanner-diag.cia` with FBI (don't select Emberclutch first) and try it; if
   that freezes too, `emberclutch-2d.cia`. Tell me which froze.

## What to send back
- The numbers from 2.4 and 3.2: press Y there (or a photo of the top screen).
- Anything slow, glitchy or wrong on the hardware (touch accuracy in the close-up, sound).
- What the 3D banner did.
