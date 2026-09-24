# First look on the old 3DS (before WP12, D55)

**For Noah.** A short check on the real hardware before the look system (D54) keeps up to
four sets of dragon models in memory. About 15 minutes. The full hardware run (WP13, D34)
still closes Alpha 2.

**Run 1 (2026-09-24, 0.1.1):** the 3D banner played, but the game stopped before it
started ("The SD card was removed"): the CIA had no boot logo. Fixed in **0.1.2**, with the
banner, sound and icon changes Noah asked for ([alpha-2.md](alpha-2.md), WP11b).

The builds are in `build/cia-test/` (not in git; rebuild with `tools\package_cia.ps1
-Version 0.1.2`, add `-Banner3D` for the second one):
- `emberclutch-3dbanner.cia`: the game with the animated 3D banner. **Install this one.**
- `emberclutch-2d.cia`: the same game with the flat banner, if the 3D one ever misbehaves.

**Screenshots (new):** press **Y** anywhere in the game. Both screens (and the overlay, when
it's on) go to `sdmc:/3ds/emberclutch/screenshots/` with a line of numbers in `log.txt`, and
a toast says which one it was. Press it at each number below and at anything that looks
off; Claude copies them off over FTP afterwards (`tools\pull_shots.ps1`).

Both are dev builds: SELECT opens the dev menu, L/R turn its pages.

## 1. Install
Both 0.1.2 CIAs are on the SD card in `/cias/` (uploaded over FTP on 2026-09-24, sizes
checked). In **FBI**: SD → cias →
`emberclutch-3dbanner.cia` → Install CIA. It installs over 0.1.1 (same title, newer
version). Sound needs your console's own DSP firmware at `sdmc:/3ds/dspfirm.cdc` (from the
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
It ran on run 1. On 0.1.2, check the fixes:
1. The icon is square: no black corners.
2. Selected, the top screen shows the baby Ember in its cracked egg with no wall behind it,
   the gold EMBERCLUTCH readable from the front (mirrored from behind, as the banner turns),
   and nothing of the dragon through the back of the egg.
3. The sound is a bar of the title theme.
4. If the HOME Menu freezes or shows nothing: hold POWER to turn off, and install
   `emberclutch-2d.cia` with FBI (don't select Emberclutch first). Tell me what you saw.

## What to send back
- The numbers from 2.4 and 3.2: press Y there (or a photo of the top screen).
- Anything slow, glitchy or wrong on the hardware (touch accuracy in the close-up, sound).
- What the 3D banner did.
