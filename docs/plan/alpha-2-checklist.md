# Alpha 2 — Checklist (WP13–14)

The Definition of done from the [Alpha 2 plan](alpha-2.md), checked on 2026-09-24 against
the review build **0.1.13** (run 13 on Noah's old 3DS, passed, D72) and its fix build **0.1.14**. Every screen check is a scripted run in Azahar with nobody at the
controls (`tools/autotest.ps1`, scripts in `tests/autotest/`), repeatable any time; the
contact sheets are in the reviews ([R5](../art/reviews/R5-style.md),
[R6](../art/reviews/R6-breeds.md)). The hardware column is Noah's run on his old 3DS
([hardware-check-2.md](hardware-check-2.md), run 13: passed); tagged `v0.2.0-alpha2`.

| # | Done when… | Emulator | Hardware | How it was checked |
|---|---|---|---|---|
| 1 | Three dragons live in the den together (plus 2 egg nests) and play with each other and with toys; the Sanctuary and Cold Vault swap dragons and eggs in and out | ✅ | ✅ runs 3–13 | `den3.txt`, `together.txt` (chase, nuzzle, sunbeam, snuggling), `toyden.txt`, `toys.txt`, `storage.txt`; PC tests for the roster, the life together and the toys. |
| 2 | Breeding at the Nesting Stone lays an egg that hatches a visibly distinct offspring with its parents' traits, in every breed | ✅ | ✅ run 13 | `breeding.txt`; PC tests for breeding (one male and one female, the requirements, inheritance by seed) and for looks passed on with surprises. |
| 3 | Wanderings with the step counter bring back finds; the hoard grows; wild eggs appear; they come home muddy | ✅ | ✅ run 13 | `wander.txt`, `mud.txt`; the PC tests `the_wanderings` and `mud_brushes_out_and_washes_off`. |
| 4 | The Market sells food, grooming items, toys and decor for Gleam, plus the daily sex-labelled egg | ✅ | ✅ run 13 | `market.txt`, `things.txt`; the PC test `the_market`. |
| 5 | The world map fast-travels between all the Alpha 2 places | ✅ | ✅ run 13 | `maptrip.txt`, `tour.txt`. |
| 6 | The dragons in the chosen style (every look, D54), with the full parts library, all 21 breeds, patterns, rare traits, and dirt and mud that grooming cleans | ✅ | ✅ run 13 | `looks.txt`, `breeds.txt`, `breeds_face.txt`, `rares.txt`, `mud.txt`, `wings.txt`; [R6](../art/reviews/R6-breeds.md); PC tests for every part and variant. |
| 7 | The emblem icon and an animated 3D HOME Menu banner with a baby dragon, a 2D banner as the fallback | ✅ | run 12 ✅ (lab 8's X) | `tools/make_banner.ps1`, `tools/check_3ds.py` on every file. |
| 8 | It installs as a CIA and runs on the old 3DS: the 3-dragon den within budget, audio, saves, touch | ✅ | ✅ runs 3–13 | `perf.txt`, `perfmix.txt` (6,814 + 2,954 triangles, 13 draws, 17.8 MB free); run 13 on the hardware: a full den of three looks. |
| 9 | Tests green, no build warnings, docs, STATUS and RedWiki updated, tagged `v0.2.0-alpha2` | ✅ | ✅ | 103,853 PC checks, 0 failures; 0 warnings; docs synced; tagged after run 13's fixes. |

## Run 13's notes, fixed in 0.1.14
2D at its own depth in 3D (the profile's platform, the selection heart, particles), a baby
toddle, a tug for the ball, more games between dragons (sparring, stalk and pounce, tail
chasing), and spines seated on the body for every look, build and stage
([details](hardware-check-2.md)). New scripts: `play.txt`, `ridge.txt`, `stereo.txt` (redone).

## Built beyond the first plan
- **The hatching reworked** (WP12a, D60): the egg bursts into bits, the hatchling grows out
  of a white blob. `hatch.txt`.
- **The Dragondex** (D55, D66): 84 entries and the rare traits, 150 Gleam and a breed-coloured
  banner per completed breed. `dex.txt`.
- **Photo mode** (D66): the camera in the den, framed pictures on the SD card. `photo.txt`.
- **Running** (WP12c): a scamper and a gallop; chases, far throws, toy runs and zoomies.
  `running.txt`.
- **The 3D slider** (WP11e): the top screen per eye. `stereo.txt` (the right eye, previewed).
- **The splash** (D68) and **checks before anything goes on the 3DS** (`tools/check_3ds.py`).
- **The wings' roots** seated on every build, look and clip (`tools/blender/wing_gap.py`).

## The regression sweep
Every script in `tests/autotest/` run in turn on 0.1.14 with a fresh save, each checked for
unmapped memory reads (the kind the emulator hides and the 3DS crashes on, D59):

All 36 clean (2026-09-25, 30 minutes):

| Script | Screenshots | Memory |
|---|---|---|
| `alpha1-a` | 22 | no unmapped accesses |
| `alpha1-b` | 8 | no unmapped accesses |
| `breeding` | 12 | no unmapped accesses |
| `breeds` | 42 | no unmapped accesses |
| `breeds_face` | 12 | no unmapped accesses |
| `closeups` | 18 | no unmapped accesses |
| `den` | 32 | no unmapped accesses |
| `den3` | 20 | no unmapped accesses |
| `dex` | 22 | no unmapped accesses |
| `hatch` | 28 | no unmapped accesses |
| `looks` | 20 | no unmapped accesses |
| `maptrip` | 16 | no unmapped accesses |
| `market` | 22 | no unmapped accesses |
| `mud` | 10 | no unmapped accesses |
| `perf` | 2 | no unmapped accesses |
| `perfmix` | 4 | no unmapped accesses |
| `photo` | 16 | no unmapped accesses |
| `play` | 22 | no unmapped accesses |
| `probe` | 4 | no unmapped accesses |
| `profile` | 10 | no unmapped accesses |
| `rares` | 12 | no unmapped accesses |
| `ridge` | 8 | no unmapped accesses |
| `running` | 14 | no unmapped accesses |
| `screenshot` | 4 | no unmapped accesses |
| `smoke` | 6 | no unmapped accesses |
| `splash` | 10 | no unmapped accesses |
| `stereo` | 8 | no unmapped accesses |
| `storage` | 24 | no unmapped accesses |
| `styles` | 10 | no unmapped accesses |
| `things` | 16 | no unmapped accesses |
| `together` | 24 | no unmapped accesses |
| `tour` | 52 | no unmapped accesses |
| `toyden` | 24 | no unmapped accesses |
| `toys` | 20 | no unmapped accesses |
| `wander` | 10 | no unmapped accesses |
| `wings` | 24 | no unmapped accesses |

## Moved past Alpha 2
The grooming redesign and its Groom button (Beta, with the Shine Show), several dragons on
the Wanderings at once (Beta), the Wanderings showing dragons flying (Beta), lower detail for
far dragons (not needed: the den won't hold more), and the map's new look with the open
world, people and campaigns (1.0). The Beta sit-down with Noah comes first (D65).
