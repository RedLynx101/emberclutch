# The World Map and Travel

Status: **Plan v1** (2026-09-24). Noah: "have a game map that you can fast travel in, or
fly around in freely". This page ties the Map tab ([screens & flow](screens-and-flow.md)),
Skyreach Valley ([GDD §7](game-design.md#7-riding-and-skyreach-valley)) and the places of
each milestone together.

## 1. The map
- An illustrated map of **Skyreach Valley** on the bottom screen, painted in the game's
  warm style. The top screen shows the selected place: a slow 3D flyover once the valley
  exists (1.0), a painted view before that.
- **Places:** the Den (home), the Market, the Nesting Stone, the Sanctuary (the keepers'
  meadow) and Cold Vault, the Wanderings trailheads, the Training Yard, the Arena, and in
  1.0 the valley's landmarks (the lake, cliffs, floating islands, lookout perches).
- Places appear as they unlock (the Market when your first dragon reaches Juvenile, the
  Arena in Beta); before that they're soft silhouettes with a one-line hint.
- Open it from the den's Map tab, or from the START menu anywhere outside an event.

## 2. Fast travel
- Tap a place, then "Go". A short travel scene plays (about 1.5 s, skippable after the
  first time): your dragon glides over the valley on the top screen, or, before it can
  fly, the view drifts across the painted map. Then you arrive.
- Always available, and free. It never replaces flying; it's there so errands stay quick.

## 3. Free flight (1.0)
- **Who can fly:** adult dragons carry you (riding, GDD §7); riding stays off in training
  and competitions.
- **Take off** from the den's sky opening (the den becomes the valley's first landmark).
  The valley is free roam: fly anywhere, land on any perch or clearing, walk the dragon
  on the ground.
- **The map becomes live:** your position and heading, the landmarks you've found, and
  where you've been. Flying close to a landmark **discovers** it: it becomes a fast-travel
  point and may hold a find (Gleam, a trinket, rarely a wild egg).
- Flying into a place (the Market's square, the Arena) lands you there.
- Tapping a discovered place on the live map fast-travels there; you arrive riding.
- Before you have an adult, the map offers fast travel only; a "Fly" button waits,
  greyed out, with "when your dragon is grown".

## 4. Milestones
| What | When |
|---|---|
| The map screen with fast travel between the Den, Market, Nesting Stone, Sanctuary and Cold Vault, and the Wanderings trailheads | **Alpha 2** |
| Training Yard and Arena join the map | **Beta** |
| Skyreach Valley free flight; the live map (position, discovery, finds); fast travel to discovered landmarks | **1.0** |

## 5. How it's built
- The map is one texture with place pins read from a small table (position, unlock
  condition, scene). Fast travel is a scene change with the travel transition in between.
- The valley (1.0) is the height-field terrain from the [asset inventory](../plan/content-and-assets.md)
  (32×32-quad chunks, fog, a short draw distance on the old 3DS). The live map is a
  pre-rendered top view of the same terrain with pins drawn over it.
- Discovery state is a bit per landmark in the save.
