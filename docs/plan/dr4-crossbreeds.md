# DR4: five crossbreeds (R12, D83)

The next five kinds, built in parallel (one Opus 5.5 subagent each, in its own worktree) while
Beta 1 goes on. Noah gave creative freedom (D85): the concepts and the models come to him in the
one big Beta review. Concept sheets (AI, reference only, D74): `docs/art/concept/dragons/dragon_<kind>.jpg`
(`tools/concept/make_concepts.py --dragons`). Everything is built by script with the kit
(`docs/tech/dragon-kit.md`); the Blazeplume (`tools/dragons/kinds/blazeplume.py`) is the first
crossbreed and the model to follow.

Every crossbreed: **uncommon**, two elements (the parents' own), `parents=` the two base kinds,
stat total about 33 (the Blazeplume's), four variants (three common, the last rare, a slightly
different model), its own egg, its own hatchling body, on the body plan named below (the plan's
clips come with it: no new plan unless the design truly can't live on one).

| Dex | Kind | Parents | Elements | Plan | Size |
|---|---|---|---|---|---|
| 10 | Kindlemoss | Pouncer × Puffback | Ember, Grove | pouncer | 1.15 |
| 11 | Cindershell | Pouncer × Curlstone | Ember, Stone | curlstone | 1.1 |
| 12 | Bloomstone | Puffback × Curlstone | Grove, Stone | puffback | 1.35 |
| 13 | Lilyfin | Puffback × Ribbontail | Grove, Tide | the builder's pick (a legged plan: pouncer, puffback or curlstone) | 1.0 |
| 14 | Frostcurl | Curlstone × Flurrytail | Stone, Frost | curlstone | 1.05 |

## Kindlemoss (Ember and Grove): a cosy moss-lynx
A plump, round-bellied cat dragon carpeted in soft moss: the Pouncer's cat face, short rounded
snout and body (heavier, a Puffback's softness), lynx ear tufts of little fern fronds, curled
fern fronds along the back shaped like small flames with ember-glowing tips, a few glowing
ember berries set in the moss, small stubby leaf wings (the Puffback's), and a bushy fern-frond
tail curling up at the end with glowing ember spots. **Hatchling:** a fluffy round moss kitten
with one sprout on its head whose bud glows like an ember. **Variants:** Hearth (moss green,
ember orange), Autumn (russet and gold leaves, amber glow), Ash (grey-green moss, pale blue
embers), rare **Wildfire** (brighter moss, many more glowing embers, the fronds tipped in
flame). **Egg:** mossy green with ember speckles. **Manners:** Gentle, Sleepy, Playful, Curious.
**Traits:** Warm-Blooded, Mossback, Cuddly, Sunbather, Hearty Eater, Glowheart.
Stats lean Breath and Stamina.

## Cindershell (Ember and Stone): a friendly volcano
A lava-pangolin panther on the Curlstone's plan (it rolls into a ball): a cat head with tall
ears capped in smooth stone, a body of overlapping dark obsidian plates with glowing magma
seams between them, four sturdy legs, short stone-edged wings, a tail of stacked plates ending
in a glowing coal. Rolled up it is a glowing ball. **Hatchling:** a round pebble-kitten with one
glowing crack down its back. **Variants:** Magma (black plates, orange seams), Sandstone (tan
plates, gold glow), Basalt (blue-grey plates, cyan-white glow), rare **Molten Core** (bright
bronze plates, wide glowing seams, a flame on the tail). **Egg:** black with glowing orange
cracks. **Manners:** Brave, Stubborn, Proud, Gentle. **Traits:** Ironhide, Sturdy, Warm-Blooded,
Brave Heart, Elemental, Ancient Blood. Stats lean Might and Breath.

## Bloomstone (Grove and Stone): a walking rock garden
A stout, tortoise-like dragon on the Puffback's plan (still a dragon: a short snout and a sleepy
smile): a domed stone back that is a tiny garden (moss, little flowers, pebbles, one small
sapling), stubby legs, tiny leaf wings, a short tail ending in a mossy rock club. **Hatchling:**
a little pebble with tiny legs, big eyes and one flower growing on top. **Variants:** Meadow
(grey stone, green moss, pink flowers), Desert (sandstone, little succulents, yellow flowers),
Ruin (dark stone, ivy, white flowers), rare **Crystal Garden** (pale stone growing glowing
crystals and blue flowers). **Egg:** grey speckled stone with a flower. **Manners:** Sleepy,
Gentle, Stubborn, Greedy. **Traits:** Mossback, Sturdy, Gentle Giant, Deep Sleeper, Treasure
Hunter, Ancient Blood. Stats lean Stamina and Might; slowest of all.

## Lilyfin (Grove and Tide): a marsh axolotl-dragon
A soft, long newt-like dragon on four short webbed legs: frilly gill fronds like fern leaves
round the head (the axolotl in it), a lily pad worn on the head like a hat with a pink lotus
flower, a long paddle tail with reed-like fins (the Ribbontail's), small leafy wings that
flutter, teal green with a pale belly and dappled spots. **Hatchling:** a tiny tadpole-like
newt with a little lily pad on its head. **Variants:** Lotus (teal, pink lotus), Bayou (olive
and brown, a yellow flower), Moonpond (deep blue, a white water lily, faintly glowing speckles),
rare **Spirit Lotus** (pale jade, a glowing lotus and glowing fin edges). **Egg:** pale jade with
lily-pad rings. **Manners:** Gentle, Curious, Sleepy, Playful. **Traits:** Water-Lover, Tidy,
Keen Nose, Songbird, Moonlit, Elemental. Stats lean Wit and Breath.

## Frostcurl (Stone and Frost): a glacier fox that curls into a snowball
On the Curlstone's plan: fluffy snow-white fur on the face, chest and belly and a huge plume of
a tail (the Flurrytail's), a back of translucent ice-blue crystal plates (the Curlstone's) that
it curls up under into a snowball, frosty crystal tips on fox ears, small crystal wings, a fox's
short snout. **Hatchling:** a round snowball with tiny ice-plate nubs and a fluffy tail.
**Variants:** Glacier (white fur, pale blue ice), Aurora (white fur, lilac and teal plates),
Slate (grey fur, deep blue plates), rare **Diamond Dust** (glowing prismatic plates and glints).
**Egg:** white with ice-blue crystal facets. **Manners:** Proud, Gentle, Curious, Brave.
**Traits:** Cool-Headed, Sure-Footed, Ironhide, Loyal, Starborn, Moonlit. Stats lean Wit and Might.

## What each builder hands back
`tools/dragons/kinds/<kind>.py`; `romfs/dragons/<kind>/` (both forms, both LODs, skins, the egg);
`tests/data/kinds/<kind>_*_reference.ecr`; review renders looked at and iterated on
(`build/review/<kind>`, not committed); `python tools/dragons/check.py <kind>` OK. The lead merges,
regenerates `src/core/kinds_data.inc` (`gen_tables.py`), runs the tests and the scripted runs.

## Lessons to build in (runs 15-17)
Mouths closed at the sides (the kit's mouth pocket does it; check open wide from the side,
culled); a hatchling's folded wings must not sink into a round body (turn them out with the
hatchling's `base_pose`); parts seated on the skin (no floating tufts); sized on the common
scale (`measure.py`, `together.py`); the see-through and clipping checks in the kit doc's
gotchas.

## After these five (D110, Noah in run 21)
The next crossbreeds get features of their own, not only a blend of their parents: take from
the parents' types (their elements, their manner, a feature or two), but let some look properly
new (a shape, a part, a way of moving that no base kind has). Put it in each concept's brief.
