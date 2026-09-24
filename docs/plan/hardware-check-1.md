# First look on the old 3DS (before WP12, D55)

**For Noah.** A short check on the real hardware before the look system (D54) keeps up to
four sets of dragon models in memory. About 15 minutes. The full hardware run (WP13, D34)
still closes Alpha 2.

The builds are in `build/cia-test/` (not in git; rebuild with `tools\package_cia.ps1
-Version 0.1.1`, add `-Banner3D` for the second one):
- `emberclutch-2d.cia`: the game with the flat HOME Menu banner. **Install this one first.**
- `emberclutch-3dbanner.cia`: the same game with the animated 3D banner (step 4).

Both are dev builds: SELECT opens the dev menu, L/R turn its pages.

## 1. Install
Copy `emberclutch-2d.cia` to the SD card (for example `/cias/`) and install it with **FBI**
(Luma3DS). Sound needs your console's own DSP firmware at `sdmc:/3ds/dspfirm.cdc` (from the
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
The emulator can't show it, so this is its first run anywhere.
1. Install `emberclutch-3dbanner.cia` with FBI (it replaces the first install; the save stays).
2. Go to the HOME Menu and select Emberclutch. The top screen should show the baby Ember in
   its cracked egg: head tilts, a tail wag, blinks, a beating heart.
3. If the HOME Menu freezes or shows nothing: hold POWER to turn off, and reinstall
   `emberclutch-2d.cia` with FBI (don't select Emberclutch first). Tell me what you saw.

## What to send back
- The numbers from 2.4 and 3.2 (a photo of the top screen is fine).
- Anything slow, glitchy or wrong on the hardware (touch accuracy in the close-up, sound).
- What the 3D banner did.
