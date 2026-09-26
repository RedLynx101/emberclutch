# The valley's places (Beta WP4)

The fourteen places' exteriors (brief: `docs/plan/beta-builders.md` A), built by
`tools/blender/valley_places.py` as static meshes in the den room's format (.esm v1, read by
`src/core/static_mesh.cpp`), plus `tools/valley/places.json` with what the game needs to stand
each one in the valley.

## Rebuild
```
blender -b -P tools/blender/valley_places.py -- --out <abs>/romfs/valley/places [--only market,den]
        [--preview <abs>/build/places] [--json <abs>/tools/valley/places.json] [--draft]
```
- Writes `<out>/<id>.esm` per place (ids as `ValleyPlace` in `src/core/valley.hpp`) and reads each
  file back as the game's reader does. `--only` rebuilds some places and merges them into
  places.json. The script stops if a place is over its budget (`--draft` only reports it).
- `--preview` renders `<id>_day/_evening/_night.png`, `<id>.png` (the three side by side) and
  `contact_sheet.png` (day and night, all fourteen) from three-quarters above. The ground, water,
  cliffs, the island and the orchard's trees in them are stand-ins, not in the files.
- `--cam tx,ty,tz,radius,azimuth,elevation` renders a close-up instead of each place's view.
- Pass absolute paths (headless Blender). All fourteen take a few minutes with previews.

## Frame and parts
Each place is in its own frame: metres, Z up, the origin on the ground at its anchor, **+Y its
front**. (With `valley.cpp`'s heading, where forward is (sin h, -cos h), place-to-world is a turn
of h + pi about Z.) File order, all names exact:

| part | flags | what |
|---|---|---|
| `solid` | 0 | everything opaque. Built for back-face culling (counter-clockwise front), thin things double-sided. |
| `lantern` | 0 | the festival lantern: stone foot, crook post, round paper lantern (opaque, drawn always). |
| `sails` | 0 | the mill's sails round `hub`; turn them about `hub_axis` (local +Y). Lit without shadows so they can turn. |
| `glow` | 1 additive | windows, lamps and the camp fire (black by day, warm at dusk and night), the den's hearth glow deep in its mouth, the Vault's cold glow, the Ruins' star; the Grotto's crystals, mushrooms, pool and gold glow a little by day too (a dark cave). Drawn as the den's glows: no culling, no depth write. |
| `lantern_light` | 3 additive + flicker | the lit lantern: a shell over the paper and two crossed halo discs. Draw it only once the lantern is lit. |

Colours are baked per vertex for the three sets (day, evening, night, as `core/daylight`): albedo
x (sky ambient + sun or low evening sun or moon, with soft ray-traced shadows) x ambient occlusion,
plus the warm light of windows and lamps in the evening and at night. The sun is placed in each
place's frame (from the front and its right, high: the valley's sun on a place facing south), so
every front is lit whatever the heading. The header's backdrop colours are the sky per set.

## Budgets (triangles, every part counted)
At most 1,500 a place, the Market 2,500. Now:

| den | market | stone | sanctuary | vault | trailhead | arena | lake | keeper | isles | orchard | mill | grotto | ruins |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1,093 | 2,492 | 1,134 | 1,398 | 940 | 974 | 1,242 | 694 | 893 | 1,000 | 1,021 | 968 | 976 | 770 |

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

## Notes for the landscape
- **Den:** the rock arch stands out of a cliff face at about y = -1.9; the landscape must leave
  the mouth open (about |x| < 4.6, up to 7.6 m) for its dark tunnel (3.6 m deep) to show.
- **Vault:** a free-standing snowy outcrop (about 12 m wide, 9 m deep), its mouth at the front.
- **Grotto:** a chamber of radius 7 whose walls face inward (a cutaway, as the den room), open
  toward -Y where the falls are.
- **Isles:** dressing for the top of the biggest island, within about 6 m of the origin.
- **Orchard:** the fence and props leave room for the landscape's own fruit trees.
