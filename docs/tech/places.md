# The valley's places (Beta WP4; 1.0)

The eighteen places' exteriors (Beta's fourteen, brief `docs/plan/beta-builders.md` A; 1.0's
four, brief `docs/plan/v1-work.md` A), built by `tools/blender/valley_places.py` as static
meshes in the den room's format (.esm v1, read by `src/core/static_mesh.cpp`), plus
`tools/valley/places.json` with what the game needs to stand each one in the valley, turned
into `src/core/places_data.inc` by `python tools/valley/gen_places.py` (read by
`core/place_layout`).

## Rebuild
```
blender -b -P tools/blender/valley_places.py -- --out <abs>/romfs/valley/places [--only market,den]
        [--preview <abs>/build/places] [--json <abs>/tools/valley/places.json] [--draft]
```
- Writes `<out>/<id>.esm` per place (ids as `ValleyPlace` in `src/core/valley.hpp`) and reads each
  file back as the game's reader does. `--only` rebuilds some places and merges them into
  places.json. The script stops if a place is over its budget (`--draft` only reports it).
- `--preview` renders `<id>_day/_evening/_night.png`, `<id>.png` (the three side by side) and
  `contact_sheet.png` (day and night, all eighteen) from three-quarters above. The ground, water,
  cliffs, the island and the orchard's trees in them are stand-ins, not in the files (the arena's
  and 1.0's places show the landscape's own ground and colours).
- `--cam tx,ty,tz,radius,azimuth,elevation` renders a close-up instead of each place's view.
- Pass absolute paths (headless Blender). All eighteen take a few minutes with previews.
- Then `python tools/valley/gen_places.py` (places.json to `src/core/places_data.inc`) and
  `tools\test.ps1` (every place loads and fits its budget; 1.0's anchors are where they belong).
- The script reads the landscape, `romfs/valley/skyreach.evl`: each place's anchor and heading,
  and the ground as the game draws it (4 m quads cut corner to corner), as `pl.gz(x, y)` in the
  place's frame. The arena's sand floor and 1.0's four places sit on that ground (the glade's
  stage and benches, the cove's shack, jetty posts, boat and rocks, the hollow's spires), and
  the landscape round them shades their ambient occlusion (no sun shadow: the game's ground casts
  none either). **Rebuild those places when the landscape changes**
  (`--only arena,caldera,glade,cove,hollow`, then gen_places.py).

## Frame and parts
Each place is in its own frame: metres, Z up, the origin on the ground at its anchor, **+Y its
front**. (With `valley.cpp`'s heading, where forward is (sin h, -cos h), place-to-world is a turn
of h + pi about Z.) File order, all names exact:

| part | flags | what |
|---|---|---|
| `solid` | 0 | everything opaque. Built for back-face culling (counter-clockwise front), thin things double-sided. |
| `lantern` | 0 | the festival lantern: stone foot, crook post, round paper lantern (opaque, drawn always). |
| `sails` | 0 | the mill's sails round `hub`; turn them about `hub_axis` (local +Y). Lit without shadows so they can turn. |
| `glow` | 1 additive | windows, lamps and the camp fire (black by day, warm at dusk and night), the den's hearth glow deep in its mouth, the Vault's cold glow, the Ruins' star; the Grotto's crystals, mushrooms, pool and gold glow a little by day too (a dark cave). 1.0: the caldera's lava and braziers and the cove's and hollow's fires (alight by day too, `LAVA`), the glade's moonpetals (soft by day, bright at night, `MOONPETAL`), the hollow's cave door, crystals and some spires (`FROST`). Drawn as the den's glows: no culling, no depth write. |
| `lantern_light` | 3 additive + flicker | the lit lantern: a shell over the paper and two crossed halo discs. Draw it only once the lantern is lit. |

Colours are baked per vertex for the three sets (day, evening, night, as `core/daylight`): albedo
x (sky ambient + sun or low evening sun or moon, with soft ray-traced shadows) x ambient occlusion,
plus the warm light of windows and lamps in the evening and at night. The sun is placed in each
place's frame (from the front and its right, high: the valley's sun on a place facing south), so
every front is lit whatever the heading. The header's backdrop colours are the sky per set.

## Budgets (triangles, every part counted)
At most 1,500 a place, the Market 2,500 (checked by the script and by `tests/test_places.cpp`). Now:

| den | market | stone | sanctuary | vault | trailhead | arena | lake | keeper | isles | orchard | mill | grotto | ruins |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1,111 | 2,480 | 1,134 | 1,470 | 958 | 950 | 1,322 | 694 | 893 | 1,000 | 1,021 | 956 | 976 | 770 |

| caldera | glade | cove | hollow |
|---|---|---|---|
| 1,352 | 1,478 | 1,232 | 1,402 |

## places.json
`{"places": {"<id>": {...}}}`, in the place's frame:
- `flat`: radius (m) to flatten the ground to round the origin.
- `door`: `[x, y]` where you walk in (null: none): the den's and Vault's mouths, the Market keeper's
  shop, the Stone's front, a Sanctuary hut, the trailhead's gate, the arena's entrance, the
  keeper's porch.
- `lantern`: `[x, y, z]`, the middle of the festival lantern's paper ball (null: none). Eight have
  one: den, market, stone, sanctuary, vault, trailhead, arena (the great lantern, twice the size,
  on the stage) and isles.
- `solids`: `[[x, y, r], ...]` circles to walk round (buildings, stalls, stones, the arena's wall
  and stands, fences, props).
- Market: `egg_stand` `[x, y, z]`, the top of the straw ring on the egg stall's round stand (the
  egg of the day's bottom); `goods_spots`, four `[x, y, z]`, the tops of the goods stall's mats.
- Mill: `hub` `[x, y, z]` and `hub_axis` `[0, 1, 0]`; `river` `{axis: "y", half_width: 6}` (a 12 m
  river along local Y under the bridge: keep it cut through the flattened ground); `water_z`.
- Lake: `water_z` (-0.6). The jetty runs from the origin on the shore out over the water toward -Y.
- `anchors` (1.0): named spots, `{"name": point or [points]}`, each point `[x, y]` or `[x, y, z]`.
  The game reads them with `placeAnchor(place, "name")` (`core/place_layout`: `.count`, `.at(i)`
  gives a Vec3; empty, count 0, for a name the place hasn't). A third value is a **height in the
  place's frame** (above its anchor, as the model is drawn: put it in the valley with
  `placeFrameToWorld`), or for the glade's `stalls` a **facing** (radians, 0 = the place's +Y,
  counter-clockwise from above: its valley heading is the place's heading plus it); `[x, y]`
  spots are on the ground (`placeToWorld3` with z 0).

| place | anchor | what |
|---|---|---|
| caldera | `ring` [x, y, z] | the battle ring's middle, z its top (a 0.25 m step up; 14 m across) |
| caldera | `sides` 2 x [x, y] | where the two trainers stand, just off the ring's two ends (on the floor, 17.8 m apart) |
| caldera | `board` [x, y] | the league's board (modelled, so a spot, not a person) beside the way in |
| glade | `stage` [x, y, z] | the stage's middle, z its boards (0.55 m up) |
| glade | `rivals` 4 x [x, y] | four spots in a gentle arc across the stage (stand them at the stage's z) |
| glade | `judges` [x, y] | the judges' table (modelled, turned to the stage; three stools behind it) |
| glade | `stalls` 2 x [x, y, facing] | accessories (left, -X), dyes (right): 1.2 m in front of each counter's middle, and the way it faces (its keeper stands behind the counter) |
| glade | `board` [x, y] | the pageant's board (modelled) by the way in |
| cove | `fish_spot` [x, y, z] | where you stand at the jetty's end, z its deck (0.9 m over the water; the lake's bed ~2.5 m under it) |
| cove | `jetty` [x, y] | the jetty's foot on the beach (walls keep walkers off the jetty: they'd pass under its deck) |
| cove | `fisher` [x, y] | before the fisher's shack |
| cove | `shells` 5 x [x, y] | along the water's edge, where shells wash up |
| hollow | `arena` [x, y, z] | where the training bouts are fought (a ring of frost on the floor, radius ~6.4) |
| hollow | `wild_door` [x, y] | just out of the cave door in the back wall, where wild dragons come out |
| hollow | `keeper` [x, y] | the keeper's spot at the camp by the corridor |

## Notes for the landscape
- **No modelled roads** (run 19): the landscape draws the earth paths (`make_valley.py` PATHS);
  the Market's cobbled road out of the square, the Trailhead's road through its gate, the
  Sanctuary's walk and the mill's road to its door doubled them and are gone (the Sanctuary has a
  few stepping stones instead). The glade's stepping stones start where its path ends.
- **Arena:** its ground is flattened to 15 m with a hand's width of noise (about +-0.2 m across the
  floor), so a flat floor at the anchor's height was covered at its front (run 19). The floor now
  follows the ground; if the landscape's flat places become exactly flat, rebuild the arena.
- **Caldera:** on the crater's flat floor (flat to ~29 m; the walls rise to ~23 m by ~45 m out);
  the terraces reach from 21 to 29.5 m at the back (-Y); the way in (+Y) is the rim's gap.
- **Glade:** its ground is flattened to ~30 m (+-0.2 m; the model follows it).
- **Cove:** the water's edge is ~43 m out at x = 0 (not 18-30 m as planned), past a low dune
  (+0.5 m) at ~28-32 m, so the jetty runs from 33.5 m (the dune's far side) to 53 m and the camp
  (shack, campfire, boat) sits at 14-29 m. A shore nearer the anchor would need the cove's flat
  radius and the lake's bowl changed in `make_valley.py` (then rebuild the cove).
- **Hollow:** flat to ~12-15 m; the cave door's arch stands out of the back wall at y -10.4 (its
  short mouth on the flat floor), the rock behind it sunk into the wall.
- **Den:** the rock arch stands out of a cliff face at about y = -1.9; the landscape must leave
  the mouth open (about |x| < 4.6, up to 7.6 m) for its dark tunnel (3.6 m deep) to show.
- **Vault:** a free-standing snowy outcrop (about 12 m wide, 9 m deep), its mouth at the front.
- **Grotto:** a chamber of radius 7 whose walls face inward (a cutaway, as the den room), open
  toward -Y where the falls are.
- **Isles:** dressing for the top of the biggest island, within about 6 m of the origin.
- **Orchard:** the fence and props leave room for the landscape's own fruit trees.
