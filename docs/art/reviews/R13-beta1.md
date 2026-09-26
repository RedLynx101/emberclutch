# R13 — The big Beta 1 review (D85 (5))

**Sent 2026-09-26 with 0.3.0:** [the review page](https://claude.ai/artifact/6k6yYxfWQjPepJj2hsHuX4). One review for all of Beta 1 (R8 the valley's layout, R9 you,
R10 the campaign outline and R12's crossbreeds were made along the way with creative freedom,
D87). The review page shows each part with the emulator's pictures and takes a verdict (love it,
tweak it, rethink it) and a note per part, and five questions; the answers are kept in the page's
database and read back with ArtifactData (collections `verdicts` and `questions`).

## The parts
| Part | What's in it | Where it's built |
|---|---|---|
| The valley | the landscape round the places, the ring of mountains, the haze, the waterfall, the islands | `tools/valley/make_valley.py`, `src/core/valley.cpp`, `src/app/render3d.cpp` |
| The places | fourteen Blender models, three lighting sets, lanterns, the mill's sails | `tools/blender/valley_places.py`, `docs/tech/places.md` |
| The Market | the egg of the day on its stand, the daily goods stall, the stalls' pages | `src/core/market.cpp`, `src/core/prop_mesh.cpp` |
| You | the creator, the people kit | `src/app/scene_creator.cpp`, `tools/people/`, `docs/tech/people-kit.md` |
| Villagers | six, their talk, portraits, voices | `src/core/villagers.cpp`, `src/app/dialogue.cpp` |
| Getting about | the pace, the camera, the lead, sliding round walls | `src/core/walker.cpp`, `src/app/scene_valley.cpp` |
| Riding | the seat, flying, the tail in the wind (D84), 3D | `src/app/render3d.cpp`, `src/app/scene_valley.cpp` |
| Discovery | 22 finds, the map's fog, the Wanderings on the map and in the sky | `src/core/finds.cpp`, `src/core/wanderings.cpp` |
| The festival | eight quests, the star dragon, the star-born egg | `src/core/campaign.cpp`, `src/app/app.cpp` |
| Challenges | the arena and cups, Sky Rings, Lantern Trial, Fruit Catch | `src/core/challenges.cpp`, `docs/tech/challenges.md` |
| Den and dragons | the mouths, skins and curls fixed; the den's lighter models | `tools/blender/dragonkit/`, `docs/tech/dragon-kit.md` |
| Speed | triangle counts, the banner labs | `docs/plan/hardware-check-3.md` (run 19) |

## Questions
Walking pace; the walking camera's height; the haze; the festival's gift (a rare Starborn
Glimmermoth); what Beta 2 leads with.

## Answers (D88, 2026-09-26)
Loved: the Market, you, discovery, the festival, riding, the challenges, the den, speed. Tweaked
and done for run 19: the lead (left hand, always shown, above the ground), Rowan's cane, trees on
the islands, more and painted trees, the mill bridge, prompts only close and facing. The camera
lower and further behind; the star-born egg kept; pace after playing; Beta 2 leads with all four.
