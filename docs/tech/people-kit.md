# The People Kit: Your Character and the Villagers (Beta 1, brief B)

Your character (two body shapes, six hair styles) and six villagers, in the dragons' formats, so
the game draws them with the dragon shader and palettes and plays them with the same animator:
**.ecm v4** models (`src/core/model.cpp`) and one **.eca** clip library (`src/core/anim.cpp`).
The look is D75's: storybook, about 2.3 heads tall, round hands, big glossy eyes, a small smile.

## Rebuilding
Everything is plain Python except the previews (no Blender needed for the game files):
```
python tools/people/build.py [--only keeper,player_a] [--anims] [--verbose]
        -> romfs/people/<id>.ecm (eight), romfs/anims/person.eca
python tools/people/check.py        # reads them back as model.cpp / anim.cpp do; budgets; exit 1 on a problem
python tools/people/check_hair.py [--only steward,fig] [--map]   # bald patches: skin outside hair anywhere round a head; exit 1 on one
blender -b -P tools/blender/people_model.py -- --out C:/abs/build/people [--only ids] [--sheets models,faces,hair,clips,ride,looks,portraits]
        -> build/people/*.png previews; portraits -> assets/sprites/people/<id>.png (64 x 64, transparent)
```
| File | What it holds |
|---|---|
| `tools/people/rig.py` | the skeleton, Blender-convention rest matrices, the game's pose and skinning maths |
| `tools/people/geom.py` | low-poly primitives (lathe, tube, ellipsoid, decals on the head, ribbons ...) |
| `tools/people/body.py` | shared parts: layout, head and face, eyes, arms, legs, shoes, skirt weights |
| `tools/people/hair.py` | hair shells, the six player styles, fringes, beards; `sink_scalp` and `lift_over` keep the scalp under every shell (1.0.1: a flat hair face across the head's curve let the skin through at the back). A shell that shows below a hat needs two rows or more: one row is flat from crown to nape and cuts the back of the head off (Tam, Bram) |
| `tools/people/people.py` | the eight people |
| `tools/people/looks.py` | palette slot use, the creator's colours, each villager's palette (sRGB bytes) |
| `tools/people/person_clips.py` | the clip library |
| `tools/people/ecm.py`, `build.py`, `check.py` | the writer, the build, the loader check |
| `tools/blender/people_model.py` | preview sheets and portraits (Blender renders the kit's meshes) |

## Conventions
- **Units and facing** as the dragons: metres (1 unit = 1 m), Z up, the person faces **-Y**, the
  origin on the ground between the feet. `_R` bones are at +X and `_L` at -X, so `_L` is the
  person's own right side (Blender's front-view naming, as the dragons' plans).
- **Heights** (top of the head or hat): your character 1.36 m (hips 0.315), keeper 1.26 (a little
  stooped; his staff reaches 1.37), market 1.30, sanctuary 1.36 (hat), steward 1.33, child 1.05
  (hips 0.21), traveller 1.36 (the lantern pole 1.65).
- **No growth, no builds:** growth scales and build multipliers are all 1, the idle pose table is
  zero (the rest pose is standing at ease), every mesh has one key (t = 1). Pass any t and build.
- **Vertex paint only:** UVs sit on the skin texture's clean corner (kCleanUv 0.97) and regions
  are kRegionClean (8), so bind the clean stand-in skin (render3d's 8 x 8 `g_cleanSkin`) and no dust.
- **Floor contact:** stand a person on the lowest vertex of the **legs and feet** (vertices weighted
  to `leg*`/`foot*`), not the whole body: the keeper's staff and the traveller's pole reach near
  the ground and dip below it when kneeling or sitting.

## The skeleton (one for everyone, 18 bones, parents first)
| # | bone | parent | # | bone | parent |
|---|---|---|---|---|---|
| 0 | hips (root) | - | 9 | leg_lo_L | leg_up_L |
| 1 | spine | hips | 10 | foot_L | leg_lo_L |
| 2 | chest | spine | 11 | arm_up_R | chest |
| 3 | neck | chest | 12 | arm_lo_R | arm_up_R |
| 4 | head | neck | 13 | hand_R | arm_lo_R |
| 5 | arm_up_L | chest | 14 | leg_up_R | hips |
| 6 | arm_lo_L | arm_up_L | 15 | leg_lo_R | leg_up_R |
| 7 | hand_L | arm_lo_L | 16 | foot_R | leg_lo_R |
| 8 | leg_up_L | hips | 17 | eyes | head |

Each person has their own bone positions (the child is small, the keeper stoops). The body draw
uses 17 bones (all but `eyes`); the parts draw uses `eyes` and `head`. model.cpp finds the contacts
hand_L, hand_R, foot_L, foot_R (7, 13, 10, 16). **Blinks** as the dragons: the `eyes` bone sits
between the eyes pointing exactly -Y, so its local Z is world up; squash its scale.z. The anim.cpp
look-at wants `neck2`/`neck3`, which people don't have: a person's look-at would turn `neck` and `head`.

## Meshes, groups and variants
| mesh | kind | group | variant | what |
|---|---|---|---|---|
| body | 0 body | 255 | 0 | the whole person with clothes and props (17 bones) |
| eyes_0 | 2 part | 0 eyes | 0 | calm eyes (big pupils), on the `eyes` bone |
| eyes_1 | 2 part | 0 eyes | 1 | surprised eyes (small pupils): use them for `surprised` |
| hair_0..hair_5 | 2 part | **10 hair** | 0-5 | players only: Tousled, Bob, Ponytail, Buns, Spiky, Long, on `head` |

`kGroupHair = 10` is new (model.hpp). **Always draw one hair style on the player:** under the
hair the player's head has no crown (it's covered by every style), so drawing none leaves a hole.

## Palette slots (vertex paint; colours in `tools/people/looks.py`)
| slot | people use it for |
|---|---|
| base | skin (creator: 5 tones) |
| accent | the outfit's main colour (creator: 3 outfits) |
| pattern | outfit trim: scarf, collar, hems, apron, cap, bedroll (creator: with the outfit) |
| horn | hair, brows, beards (creator: 6 colours) |
| membrane | leather and wood: boots, belts, staff, pack (villagers: their own; the market's headscarf, the sanctuary's straw hat) |
| iris, pupil, glint | the eyes as the dragons' (glint emissive 200); pupil also darkens the mouth and soles |
| glow | a villager's third colour (keeper unused, market shoes, sanctuary boots, steward shirt, child trainers) or the traveller's lantern flame (painted emissive 235) |
| tongue | blush (mixed into base), the nose's tint, the mouth's warmth |

Mixed paints (two slots and a fixed mix) give trousers (membrane toward pupil), grey metal (glint
toward pupil), brass (pattern toward glint), the child's pale toy dragon (accent toward glint).
looks.py values are **sRGB bytes**, as the 3DS shows them (unlike the kinds' linear VARIANTS):
`player_palette(skin, hair, outfit)` gives a creator choice's ten slots, `VILLAGERS[id]` each
villager's, `linear()` converts for Blender.

## Budgets (triangles drawn: body + eyes + the largest hair)
| person | body | eyes | hair | total | | person | body | eyes | total |
|---|---|---|---|---|---|---|---|---|---|
| player_a | 448 | 52 | 82-100 | 600 | | sanctuary | 542 | 52 | 594 |
| player_b | 448 | 52 | 82-100 | 600 | | steward | 547 | 52 | 599 |
| keeper | 546 | 52 | - | 598 | | child | 536 | 52 | 588 |
| market | 508 | 52 | - | 560 | | traveller | 542 | 52 | 594 |

## The clips (`romfs/anims/person.eca`, 35 clips, one library for everyone)
Deltas in armature axes on top of the rest pose, as the dragons' (conventions in
`person_clips.py`). **The prop hand stays level:** every clip computes `hand_L` as the inverse of
hips-spine-chest-arm_up_L-arm_lo_L, so a staff, pole, bucket or clipboard held in hand_L stays
upright however the arm moves (raised aloft in a cheer, planted when kneeling). The free hand
(`hand_R`) waves, pets, talks and picks up.

| clip | s | loop | notes |
|---|---|---|---|
| idle | 3.0 | yes | breathing, a little sway |
| look_around | 3.0 | | head left then right |
| walk | 0.8 | yes | speed 0.69 m/s; footsteps at 0.0 and 0.4 |
| run | 0.5 | yes | a bouncy run: speed 1.76 m/s, root up-bounce to 0.055 m; footsteps at 0.0 and 0.25 |
| wave | 2.0 | | hand_R, arm out to the side, three waves |
| talk | 2.4 | yes | gestures, both hands, small nods |
| nod | 1.0 | | a double nod |
| cheer | 1.6 | | arms up in a V, a 0.14 m hop; land at 0.68 |
| crouch_pet | 2.0 | yes | kneeling, stroking forward-down with hand_R |
| mount | 1.2 | | crouch, hop (root arc to 0.26 m), a leg over, ends in the ride pose; land at 0.9 |
| ride | 1.6 | yes | astride, legs apart, hands forward holding on |
| ride_lean_left | 1.6 | yes | leaning into a bank toward +X (the rider's own left) |
| ride_lean_right | 1.6 | yes | leaning toward -X (the rider's own right) |
| dismount | 1.0 | | a leg over, a hop down (root arc to 0.2 m), lands standing; land at 0.7 |
| sit | 1.0 | | sits onto a seat at knee height, ends seated |
| sit_loop | 3.0 | yes | seated, feet swinging (an extra, so a held sit doesn't freeze) |
| surprised | 1.0 | | a startled hop back, hands up (swap to eyes variant 1) |
| pick_up | 1.4 | | squats and picks up with hand_R, ends holding it up at the chest |
| clap | 0.8 | yes | clapping at chest height, a little bob; thump at each clap (workstream D, below) |
| sit_clap | 0.8 | yes | sat as `sit`, clapping, feet swinging |
| sit_ground | 4.0 | yes | sat on the ground, legs out, hands on the thighs; root down 0.21 m |
| doze | 5.0 | yes | dozing sat on the ground, chin down, a nod and a catch; root down 0.21 m |
| doze_stand | 5.0 | yes | dozing on their feet, head drooped, a slow sway |
| stretch | 2.4 | | arms up in a V, up on the toes (root 0.03 m), a yawn |
| fist_pump | 1.2 | | the free hand pumped twice, the knees bouncing (cheering a dragon on) |
| point | 1.2 | | a step and the free arm thrown forward, pointing (sending a dragon in) |
| worried | 1.4 | | hands up toward the cheeks, a lean back, a wince each way |
| slump | 1.8 | | a breath in, then shoulders down and head hung (ends slumped) |
| bow | 1.4 | | a polite bow |
| fish | 3.0 | yes | a rod held out in both hands, a gentle jig (the cove draws the rod) |
| cast | 1.2 | | the rod back over the shoulder, whipped forward, ends as `fish` |
| scatter | 2.2 | yes | a dip into the bucket in hand_L, feed flung out in an arc (Bram) |
| write | 3.0 | yes | writing on the clipboard held up in hand_L, looking up now and then (Wren) |
| tidy | 2.4 | yes | both hands busy at a counter in turn, leaning in (Maple) |
| fly_toy | 2.0 | yes | the toy in hand_L swooped up and round, bouncing (root bob), eyes on it (Pip) |

**Settings (workstream D, 2026-09-28):** the last seventeen are for the people's doings: the
villagers by the hour (`core/routines`, played by `app/people_acts`), the battle view's trainer
and you (a bow as it begins, a point as a move is chosen, a fist pump or a wince at a big hit, the
loser's slump), the roaming trainers (walk, wave, talk, sit_ground at a viewpoint, a look about or a
stretch, clapping while they watch someone else's battle), fishing at the cove. `sit_clap` waits
for a seated audience (benches at the glade: left out for now, the show's wide shots being in budget).
**Root tracks scale by the body:** the game multiplies a clip's root offsets by the body's hips /
0.315 (`render3d` drawPerson), so the child sits on the ground as the grown-ups do.
Preview sheets: `people_model.py -- --sheets clips --clip-pages 3,4,5`.

Walk and run speeds are for the standard leg (hips at 0.315 m): scale by hips / 0.315 for other
bodies (the child 0.67). Events use the dragons' ids: footstep (1), land (7). Mount and dismount
hop on the spot: move the person between the ground spot beside the dragon and the seat over the
middle of the clip (about 15% to 75%) and add the root track's arc.

## Riding: the seat point
The seat is **(0, 0.02, z)** in the person's model space, at the bottom of the pelvis: z = 0.33
for both player bodies (0.215 child, 0.29 market, 0.305 keeper, 0.33 steward, 0.335 sanctuary,
0.355 traveller). The ride clips don't turn the hips, so it stays put: place the person so the
seat point lies on the dragon's seat (the kind's `SEAT`), facing the dragon's way (-Y), with no
floor contact. The ride pose's shins hang clear of a back up to about 0.55 m across at the seat;
on broader dragons (the Pouncer is about 0.95 m) seat the rider a few centimetres higher or scale
the rider. A stand-in back is in `build/people/ride.png`.

## Previews (build/people/)
`models.png` (everyone, front and three-quarter), `faces.png`, `hair_player_a.png` and
`hair_player_b.png` (the six styles front, three-quarter and back), `clips_<id>_1.png` and
`_2.png` (key frames of the main clips), `ride.png` (mount, ride from the chase camera, the
leans, dismount), `looks.png` (the creator's colours), `portraits.png`.
