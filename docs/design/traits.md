# Traits: what each one does (D150)

Until 1.0.0's last build the 34 traits were rolled at hatching and shown on the profile, and did nothing:
no part of the game read them. Noah (run 30): *"Make a quick proposal to map unused traits to real in-game
things"*. Each trait now has one effect, on a system already in the game, small enough that no trait is a
must-have and none is useless. The rarer the tier, the stronger or rarer the effect. The numbers live in
the code beside the systems they touch (`hasTrait`, core/kinds), and the guide's almanac lists them
(`tools/guide/check_guide.py` keeps it honest).

A dragon has one or two traits (three on the rare colouring), from those its kind leans to; a rare kind
reaches one tier further, the rare colouring the legendary tier.

## Common

| Trait | What it does | Where |
|---|---|---|
| Swift | Flies 6% faster (in the valley and in Sky Rings) | flight, the race |
| Sturdy | Games, challenges, battles and shows cost a quarter less Energy | trainer energy |
| Keen Nose | A quarter more chances to find something on the Wanderings | Wanderings |
| Tidy | Clean fades, and dust settles, 40% slower | needs |
| Hearty Eater | Food fills its Belly a quarter more | feeding |
| Early Riser | From 7 am to noon, Play and Love fade half as fast | needs |
| Night Owl | From 5 pm to 10 pm, Play and Love fade half as fast | needs |
| Sure-Footed | One extra heart in the Lantern Trial | the trial |
| Cuddly | Petting and brushing build bond faster (+1 a stroke) | care |
| Sunbather | From 10 am to 4 pm, Love fades half as fast | needs |
| Water-Lover | A bath also fills Play (+20) and Love (+10) | care |
| Chatty | Walks together build bond twice as fast | walking |

## Uncommon

| Trait | What it does | Where |
|---|---|---|
| Quick Learner | A fifth more experience from everything | levels |
| Strong Wings | Wingbeats and bursts cost a quarter less stamina (its race meter lasts longer) | flight, the race |
| Treasure Hunter | Half again as much Gleam from the Wanderings | Wanderings |
| Brave Heart | Moves that lower its stats fail half the time | battles |
| Gentle Giant | +6 Poise in shows | pageants |
| Warm-Blooded | Frost moves do a quarter less damage to it | battles |
| Cool-Headed | Ember moves do a quarter less damage to it | battles |
| Deep Sleeper | Sleep and naps bring Energy back 30% faster | needs |
| Showoff | +6 Look in shows | pageants |
| Loyal | Never upset by your being away (three days away won't do it) | moods |

## Rare

| Trait | What it does | Where |
|---|---|---|
| Skydancer | Turns 15% tighter and glides further (sinks 20% slower) | flight, the race |
| Ironhide | Takes a tenth less damage in battles | battles |
| Lucky | Rare finds on the Wanderings (a little hoard, a wild egg) twice as likely | Wanderings |
| Songbird | The show's Performance timing window is a fifth wider | pageants |
| Elemental | Breath moves of its own element hit 15% harder | battles |
| Glowheart | Its mood counts 5 points higher (it stays joyful more easily) | moods |
| Mossback | Shows that favour Grove count it as a Grove dragon | pageants |

## Legendary

| Trait | What it does | Where |
|---|---|---|
| Starborn | One more point in every stat | stats |
| Ancient Blood | Its eggs come out in the rare colouring twice as often | breeding |
| Phoenix Heart | A sulk never turns into being upset | moods |
| Moonlit | Every battle stat 10% higher from 8 pm to 6 am | battles |
| Sunkissed | Every battle stat 10% higher from 9 am to 5 pm | battles |

## Notes
- Effects stack with manners (a Shy, Tidy dragon has both). Wild dragons and challengers have traits too.
- Nothing is saved differently: traits were already in the save, so every dragon's traits start working.
- What stays for later: showing each trait's effect on the profile itself (the guide carries it for now).
