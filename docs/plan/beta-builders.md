# Beta 1: the builders' briefs (places and people)

Two asset jobs built in parallel by Opus 5.5 subagents (D85: creative freedom, one big review at
the end), each in its own worktree, merged and wired in by the lead. The look for both is D75's
(`docs/design/look-and-feel.md`): storybook, in the way of a cozy life-sim; chunky, rounded,
toy-like shapes; warm, soft, friendly colours; clean readable silhouettes; low poly for the old
3DS. Spacing (D86): between a cozy life-sim's town and a 3DS-era top-down adventure's field.
AI concept images (`docs/art/concept/`) are reference only (D74). Everything is built by script
in Blender 5.2 ("C:\Program Files (x86)\Steam\steamapps\common\Blender\blender.exe", background
mode). Units are metres; Z is up.

## A. The places (`tools/blender/valley_places.py`)
Each place's exterior as a static mesh in the den room's format, **.esm v1** (writer:
`write_esm` in `tools/blender/den_model.py`; reader: `src/core/static_mesh.cpp`): positions and
baked vertex colours for **three lighting sets, day, evening and night** (as the den's SETS), so
the game blends them by the time of day. Bake like the den does (albedo x a sun from the
south-east high by day, a warm low sun in the evening, cool moonlight and a dim sky at night, a
soft sky ambient and simple ambient occlusion), with windows and lamps dark by day and warm by
night.

Per place, in its own local frame (origin on the ground at the place's anchor, **+Y its front**,
the way it faces), written to `romfs/valley/places/<id>.esm`:
- parts named `solid` (opaque; split it if it's over 65,535 vertices or indices),
- `glow` (flag additive: windows and lamps glowing, black by day in their colours),
- for a festival lantern: `lantern` (the post and the lantern, opaque) and `lantern_light`
  (flag additive: its flame and glow, the game draws it only once the lantern is lit).
- Budget: **at most 1,500 triangles a place** (the Market village 2,500), few big shapes;
  reuse and instance-like repetition in the script is fine.

The places (ids as `ValleyPlace` in `src/core/valley.hpp`):
| id | place | what's there |
|---|---|---|
| den | Your den | dressing for the cave mouth in the cliff (the landscape makes the cliff): a rounded rock arch with moss and hanging vines, stepping stones, a little wooden sign with a heart, flowers, the festival lantern |
| market | Market Village | four or five cottages round a square (thatched and shingled roofs, round windows), three market stalls with striped awnings (one of them the egg stall with a round stand in the middle for the egg of the day, one the goods stall with **four display spots** on its counter), bunting between posts, a well, benches, flower boxes, the lantern |
| stone | The Nesting Stone | a hilltop ring of mossy standing stones round a big smooth stone with a nest of woven straw on it, wildflowers, the lantern |
| sanctuary | Sanctuary Meadow | two keepers' huts with round doors, a paddock fence, hay bales, a water trough, a little lookout platform, the lantern |
| vault | The Cold Vault | a cave mouth in snowy rock with icicles and blue ice at the edges, a heavy wooden door ajar, snow drifts, the lantern |
| trailhead | Wanderers' Trailhead | a wooden arch gate over the path, a signpost with arrows, a bench, the traveller's tent with a pennant, the lantern |
| arena | The Arena | a round arena ringed by low wooden stands and flag poles with pennants, an entrance arch, a festival stage with the great lantern (the lantern parts) |
| lake | Mirror Lake | a little jetty with a rowing boat, reeds and lily pads (no lantern) |
| keeper | The Keeper's Lodge | the old keeper's cottage by the falls: stone base, crooked chimney, a porch with a rocking chair, a vegetable patch, a dragon-sized perch post |
| isles | The Floating Isles | dressing for the top of the biggest floating island: a little shrine of stones, flowers, the lantern (the landscape makes the islands) |
| orchard | Honeyroot Orchard | a fenced orchard corner: a fruit cart with baskets, ladders, a scarecrow (the trees are the landscape's own) |
| mill | Windmill Bridge | a round stone windmill with cloth sails (the sails a separate part named `sails` round the hub so the game can turn it; give its hub position in the data) and a humped stone bridge over a 12 m wide river |
| grotto | The Hidden Grotto | a small cave chamber (open toward -Y, where the falls are) with glowing green crystals (in `glow`), a pool, a treasure chest |
| ruins | Starwatch Ruins | a broken round stone tower with an open top, fallen blocks, a telescope on a stand |

Also `tools/valley/places.json`: per place `{ "flat": radius the ground should be flattened to,
"door": [x, y] where you walk in (local; null if none), "lantern": [x, y, z] (null if none),
"solids": [[x, y, r], ...] circles to walk round, "hub": [x, y, z] (the mill only) }`.

Preview renders (`build/places/<id>.png`, three-quarter from above, day and night) looked at and
iterated until each is cute and reads at a glance, then a contact sheet of all fourteen.
Deliver: the script, the fourteen .esm files, places.json, and a short `docs/tech/places.md`
(how to rebuild, the parts, the budgets).

## B. People (`tools/people/`, `tools/blender/people_model.py`)
Your character (the creator, D74) and the villagers, in the dragons' model and animation formats
so the game draws them with the dragons' shader and palettes and plays them with the same
animator: **.ecm v4** (writer: `tools/blender/export_dragon.py` / `tools/blender/dragonkit/export.py`;
reader `src/core/model.cpp`) and **.eca** clips (`tools/anim/eca.py`, `tools/anim/clips.py`;
reader `src/core/anim.cpp`). Reuse those writers rather than new formats.

- **Proportions (D75):** cute, rounded, about 2 to 2.5 heads tall, round hands, big eyes, a
  friendly face; about **600 triangles** at most (LOD0), at most 25 bones a draw.
- **Skeleton** (one for everyone): hips, spine, chest, neck, head, arm_up/arm_lo/hand L and R,
  leg_up/leg_lo/foot L and R, and one `eyes` bone (blinks squash it, as the dragons').
- **Colours by palette:** paint vertices with the palette slots (see `src/core/model.hpp`
  Palette): base = skin, accent = outfit, pattern = outfit trim, horn = hair, iris = eyes, pupil
  and glint as the dragons', so one model takes any skin tone, hair colour and outfit colour at
  runtime.
- **Your character:** two body shapes as two files (`romfs/people/player_a.ecm`,
  `player_b.ecm`), each with **six hair styles** as part meshes (kind 2, group 10, variant 0-5:
  the lead adds `kGroupHair = 10` to model.hpp), and eyes as the dragons' (group 0). The
  creator's five skin tones, six hair colours and three outfit colours go in
  `tools/people/looks.py` as sRGB triples (the lead generates a table from it).
- **Villagers,** each its own file `romfs/people/<id>.ecm` with its own clothes and props as part
  of its body: `keeper` (the old dragon keeper, your guide: white beard, round glasses, a long
  coat, a crooked staff), `market` (the Market's keeper: apron, headscarf), `sanctuary` (the
  Sanctuary's keeper: straw hat, overalls, a bucket), `steward` (the arena's steward: a tabard,
  a whistle, a clipboard), `child` (a small child who follows the dragons: a big cap), and
  `traveller` (a cloaked traveller with a huge backpack and a lantern on a pole). Warm, varied,
  high-fantasy-folk hints are welcome (pointed ears on one or two, a tail on one).
- **Clips** (`romfs/anims/person.eca`, one library for everyone): idle, look_around, walk, run
  (a bouncy run), wave, talk (gestures), nod, cheer, crouch_pet (kneeling to pet a dragon),
  mount, ride (seated, legs apart, holding on), ride_lean_left, ride_lean_right, dismount, sit,
  surprised, pick_up. Walk and run loop with a clear footstep marker as the dragons' clips do.
- **Portraits** for the dialogue box: each villager's head and shoulders, front three-quarter,
  smiling, on a transparent background, 64 x 64, `assets/sprites/people/<id>.png`.

Deliver: the scripts, the models, the clip library, the portraits, preview sheets looked at and
iterated (`build/people/`), and a short `docs/tech/people-kit.md` (how to rebuild, the skeleton,
the clips, the palette slots, the groups).
