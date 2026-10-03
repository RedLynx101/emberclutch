# The trailer: script and captions (V2, docs/plan/trailer.md)

About 75 seconds, cut to the game's own music. The narration is short and warm, in the storybook voice of
the game and the guide; the captions carry the beats on screen. Every picture is the game itself at the
3DS's own 400×240, shown as it is: sharp pixels, scaled four times.

| # | ~s | Picture (reel) | Narration | On screen |
|---|---|---|---|---|
| 1 | 0-4 | The egg in the den's nest by firelight, rocking under the stylus (`s01_egg`) | *In a valley above the clouds, every keeper starts with an egg.* | |
| 2 | 4-10 | It shakes, cracks, and the hatchling blinks at you (`s02_hatch`) | *Keep it warm, and one morning... someone says hello.* | **EMBERCLUTCH**, *Skyreach Valley* |
| 3 | 10-14 | Petting with the stylus, both screens (`s03_pet`) | *Pet it. Feed it. Brush it. Bathe it.* | |
| 4 | 14-18 | Feeding, brushing, the bath's bubbles: three quick cuts (`s04a-c`) | | |
| 5 | 18-24 | The den from day to night to morning, a little bigger each time (`s05a-d`) | *Day by day, in real time, it grows.* | |
| 6 | 24-30 | Out of the den: Skyreach Valley opens below (`s06_valley`) | *Then the door opens on Skyreach Valley.* | *A valley to explore* |
| 7 | 30-34 | Walking the lead through Market Village (`s07_market`) | *Walk it on its lead. Meet the villagers.* | |
| 8 | 34-37 | Climbing on; the take-off (`s08_takeoff`) | *And when it's grown...* | |
| 9 | 37-44 | Flying past the floating isles at golden hour (`s09_flight`) | *climb on.* | *Raise it. Ride it.* |
| 10 | 44-48 | A battle at the Market (`s10_battle`) | *Battle in the league.* | |
| 11 | 48-51 | A show on Moonpetal Glade's stage (`s11_show`) | *Shine on the show stage.* | |
| 12 | 51-54 | The Sky Rings race (`s12_rings`) | *Race the Sky Rings.* | |
| 13 | 54-57 | Fishing at Driftwood Cove (`s13_fish`) | *Or just go fishing.* | |
| 14 | 57-62 | The valley's lanterns lit at night (`s14_lanterns`) | *And when the lanterns are lit, the whole valley glows.* | |
| 15 | 62-64 | The Dragondex (`s15_dex`) | | *Fourteen kinds of dragon* |
| 16 | 64-69 | Two dragons asleep by the hearth (`s16_sleep`) | *Nothing ever dies here. A forgotten dragon only sulks, until you make up.* | |
| 17 | 69-76 | The end card: the wordmark, the hatchling in its egg | *Emberclutch: Skyreach Valley. Free, for the Nintendo 3DS.* | *Free homebrew for the Nintendo 3DS family* · *Runs on the original 2011 3DS* · the GitHub link (the Universal Updater line once it's listed) |

About 860 characters of narration in all.

## As cut (V5, `tools/film/edit.py`)
1:27 in all. The pictures are 6x renders now (not the 400x240 above); the words are as written, line 2 split
at its pause.

| At | Picture | Music | Lily |
|---|---|---|---|
| 0:00.8 | The egg by firelight, a slow push | title-theme (from 0.62 s in) | *In a valley above the clouds...* |
| 0:06.3 | The hatching: the burst at 0:09.2 on bar 4, the first cry at 0:11.3 | | *Keep it warm, and one morning...* (the burst) *someone says hello.* |
| 0:11.9 | The title over the hatchling | the strings' entry (bar 5) | |
| 0:14.6 | Pet, feed (a bar each), brush, bathe (a half bar): both screens | | *Pet it. Feed it. Brush it. Bathe it.* |
| 0:22.7 | Day, evening, night, morning, dissolving | | *Day by day, in real time, it grows.* |
| 0:28.1 | Through a white bloom: the crane past the waterfall | | *Then the door opens on Skyreach Valley.* (*A valley to explore*) |
| 0:33.5 | The Market on its lead | (title-theme dropping away at 0:37.8) | *Walk it on its lead. Meet the villagers. And when it's grown...* |
| 0:39.6 | The hop on; the take-off | skyreach's bar-4 build, its drums at 0:40.9 | *climb on.* |
| 0:42.7 | The flight; the isles at golden hour | | (*Raise it. Ride it.*) |
| 0:50.1 | Battle, show, the Dragondex, rings, the catch | a shot every bar or bar and a half | *Battle in the league. Shine on the show stage.* (*Fourteen kinds of dragon*) *Race the Sky Rings. Or just go fishing.* |
| 1:03.1 | The village's lanterns at night | a hush (the last downbeat ringing on) | *And when the lanterns are lit, the whole valley glows.* |
| 1:05.8 | Home's lantern; asleep by the hearth | title-theme's close (bar 68) | *Nothing ever dies here. A forgotten dragon only sulks, until you make up.* |
| 1:13.9 | The end card: the wordmark, the egg aglow, the lines | its last chord (bar 72) | *Emberclutch: Skyreach Valley. Free, for the Nintendo 3DS.* |

The vertical cut (0:33, `--cut tall`): the burst at 0:00.5, the care, the take-off on the drums at 0:10.9, the
montage a bar a shot, the end card on the last chord; its lines shown as captions.

## The voice samples
Each candidate reads the same two lines, the opening and the close:

1. *In a valley above the clouds, every keeper starts with an egg. Keep it warm, and one morning... someone
   says hello.*
2. *Nothing ever dies here. A forgotten dragon only sulks, until you make up. Emberclutch: Skyreach Valley.
   Free, for the Nintendo 3DS.*
