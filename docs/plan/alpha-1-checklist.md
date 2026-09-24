# Alpha 1 — Checklist (WP12)

The Definition of done from the [Alpha 1 plan](alpha-1.md), checked on 2026-09-24 against
the build tagged `v0.1.0-alpha1`. Every screen check is a scripted run in Azahar with nobody
at the controls (`tools/autotest.ps1`, scripts in `tests/autotest/`), so it can be repeated
any time; the contact sheets below are from those runs. Hardware stays deferred to the end
of Alpha 2 (D28, D34).

| # | Done when… | Result | How it was checked |
|---|---|---|---|
| 1 | New game → name → any of the 3 starter eggs → rub → hatch (cinematic) → name the dragon (keyboard with a suggestion) → rename later | ✅ | `tour.txt` (Ember), `alpha1-a.txt` (Tide, rename), `alpha1-b.txt` (Gale, starting over asks twice). In scripts the keyboard is stood in for (`name <text>`); the real swkbd applet is only exercised by hand. |
| 2 | With the dev time skip the dragon grows every day and reaches Adult through all stages; starter parts and sex differences visible | ✅ | `alpha1-a.txt` (hatchling → juvenile → adolescent → adult), `closeups.txt`; growth per day and the stage gates by the PC tests (`body_scale_grows_every_day`, `stage_gates`). |
| 3 | Hands-on care, *Nintendogs*-style: tools at the stylus, leaning into petting with a sweet spot, shine regions, bath with suds and shake-off, hand-feeding into the opening jaw, fetch with a bouncing ball; animation and sound for each; sleep, naps, yawns | ✅ | `tour.txt` (every tool), `alpha1-a.txt` (a whole fetch: the ball comes back in its mouth), `den.txt` (sleep at night); PC tests for picking, strokes, quirks, grooming, ball physics and every reaction. Sounds were heard in Azahar earlier; the new care sounds are stand-ins until the [sound brief 2](../audio/sfx-batch-2.md) arrives (D35). |
| 4 | Neglect → Upset → sulk nook → make-up → heartglow re-lights | ✅ | `alpha1-a.txt` (needs drained, a day alone: Upset; Make up: "Its heartglow lights up again"); PC tests for the moods. |
| 5 | Music: title → den day ↔ nestsong at night (crossfade); hatching stinger; seamless loops | ✅ | The dev overlay shows the track (`den.txt` through a day); loops and crossfades were listened to in Azahar before (WP9). |
| 6 | Save survives quit/restart and a power cut mid-save (A/B slots, CRC) | ✅ | `alpha1-a.txt` ends with Save & quit, `alpha1-b.txt` starts with Continue on that dragon; corrupt, truncated and interrupted slots by the PC save tests. |
| 7 | Budget overlay green in the den and petting scenes | ✅ | `den.txt` / `alpha1-a.txt`: one grown dragon 6,345 top + 2,906 bottom triangles (budgets 8,000 + 3,500), 8 draws, 25 bones. The dev 3-dragon test with the fresh eggshell still in the nest reaches 8,247: an egg LOD comes with Alpha 2's full den (see below). |
| 8 | The CIA installs and launches in Azahar with the original icon and banner | ✅ | `tools/package_cia.ps1`; installed with `azahar -i`, then `smoke.txt` booted from the installed title. Icon and banner are rendered from our own model (interim until the Alpha 2 emblem, D48). |
| 9 | Tests green; build without warnings; docs, STATUS and RedWiki updated | ✅ | 91,752 PC checks, 0 failures; a clean build with 0 warnings. |

## Balance check
Needs drain as the [GDD](../design/game-design.md) §3.2 sets them (Belly 6/h awake and 3 asleep,
Energy 3/h, Shine 2/h, Play 4/h): a hatchling left alone for eight hours is hungry and Sulky,
and turns Upset only after a whole day of that (or three days without a visit). That's
the *Nintendogs* rhythm of a visit or two a day; it's the first thing to tune if it feels
naggy on hardware.

## Found and fixed by the scripted runs
- The bottom-screen close-up was far too tight on a hatchling and too wide on a long-necked
  adult; feeding hid the mouth under the food row; a sleeping dragon showed its back.
- The popped egg cap floated in the air.
- The bath: a hatchling needed half a minute to reach the tub (now it's set down in front of
  it, sized to it, and small dragons step quicker, up to 2x, feet still planted).
- Dev overlay: the bone budget was stale (25 since the jaw bone), and it counted only the
  top screen.

## Left for later
- **Hardware** (D34): the first run on Noah's old 3DS closes Alpha 2.
- **An egg LOD** (~450 triangles) so three dragons and an egg in the nest stay under 8k.
- The real keyboard applet and the sounds need a person to try: both are quick to
  check by hand in Azahar (`tools/emu.ps1`).
- The 3D effect toggle waits until the top screen renders in stereo.

## Contact sheets
Each sheet shows four steps: the top screen above the bottom screen.

| | |
|---|---|
| ![](alpha-1-checklist/tour-01.jpg) | ![](alpha-1-checklist/tour-02.jpg) |
| ![](alpha-1-checklist/tour-03.jpg) | ![](alpha-1-checklist/tour-04.jpg) |
| ![](alpha-1-checklist/tour-05.jpg) | ![](alpha-1-checklist/tour-06.jpg) |
| ![](alpha-1-checklist/tour-07.jpg) | ![](alpha-1-checklist/alpha1-b-01.jpg) |
| ![](alpha-1-checklist/alpha1-a-01.jpg) | ![](alpha-1-checklist/alpha1-a-02.jpg) |
| ![](alpha-1-checklist/alpha1-a-03.jpg) | ![](alpha-1-checklist/den-01.jpg) |
| ![](alpha-1-checklist/den-02.jpg) | ![](alpha-1-checklist/den-03.jpg) |
