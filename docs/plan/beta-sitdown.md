# The next milestone — sit-down brief (D65)

**For Noah, 2026-09-25.** Alpha 2 is done (run 13, D72; tagged `v0.2.0-alpha2`). Before any
more work, this sit-down settles what comes next. You said what you want most: **the places
redesigned, the map, the open world, flying and challenges.** On the roadmap those are
mostly 1.0 (*The sky*), with Beta (*A trainer*: tricks, voice, ground competitions) first.
So the first question is the order. Each question below has options and a recommendation;
answer any way you like ("1B, 2A…", or in your own words). Nothing is built until you've
passed it; then I write the detailed plan for you to sign off, as with Alpha 2.

## Already decided (for reference)
- Riding anywhere in free roam; competitions unmounted, some in the air (early decisions).
- Exploring with a young dragon before it can carry you: follow it, hops and glides (D44).
- Free flight over the valley on an adult, with a live map: your position, places found by
  flying near them, fast travel to any found place (D45).
- The map should blend into the open world; its pictures are redone then (run 3, D61).
- People: cute, Nintendo-like characters for the player and the valley's folk; campaigns
  (stories and quests round the places and the cups) (run 3, D61).
- The Wanderings shown in flight (run 10); several dragons out at once (D64).
- The grooming redesign (bath, cloth, brush each with a job, one Groom button) waits (D62).
- The Dragondex's pictures, the breed banners and the fast-travel map get replaced when
  the places move into the world (run 13, D72).
- Old 3DS first: 30 fps in 3D is fine, 60 without (D62).

## 1. The order
- **A. The valley first** (recommended). The next milestone becomes *The valley*: a first
  open world to walk and fly in, the places rebuilt inside it, the new map, flying on an
  adult, gliding for adolescents, and the first challenges (air ones, since flying is the
  point). Training, voice and the ground cups follow in the milestone after. Why: it's what
  you want to play next, and it's the riskiest thing left on an old 3DS (a big 3D space,
  streaming, draw distance, frame time), so doing it early follows the project's rule of
  risk first.
- **B. As planned.** Beta (training, voice, three ground competitions, economy) first, then
  1.0 with the valley and flight.
- **C. Both at once**, one long milestone. Not recommended: months before anything playable.

## 2. How big is the valley, and what's in it?
- **A. One valley, a few minutes' flight across** (recommended for the first pass): a
  height-field landscape about 1 km across at game scale (a grown dragon crosses it in 2–3
  minutes of flight), cut into tiles that load as you go, with fog to hide the edge. Six to
  eight places: the den (in a cliff by a waterfall), the Market (a village), the Nesting
  Stone (a hilltop), the Sanctuary (a meadow with keepers) and Cold Vault (a cave in the
  cold heights), the trailheads of the Wanderings, a lake, floating islands, and the arena
  for challenges. Room to add regions later.
- **B. Small and dense**: a single bowl you can see across, every place in view; cheaper,
  less sense of travel.
- **C. Several valleys** joined by flight routes, each loaded on its own; the most room,
  and the most to build before it's fun.

## 3. The places: inside the world, or scenes you enter?
- **A. Both** (recommended): each place stands in the world (you see the Market's roofs from
  the air and land in its square), and stepping into it opens its own close-up scene (the den
  as today, the Market's stalls), so the care and shopping screens keep their detail and the
  world stays within budget.
- **B. Fully in the world**: shops, den and all rendered in the open world; the most
  seamless, the hardest on the old 3DS.
- **C. Scenes only**, reached from the new map; the world is for flying only.

## 4. The map
- **A. A painted map of the valley** (recommended) on the bottom screen while you fly or
  walk: your position, places found (fog over the rest until you've flown near), a tap to
  fast travel to any found place, with the heart travelling the path you liked; opened full
  screen with X as today.
- **B. The top-down world itself** as the map (a camera above the valley); cheaper art,
  less charm.

## 5. Flying: how it controls
- **A. Arcade** (recommended): the circle pad steers, A flaps (climb), B dives, L/R bank for
  tighter turns, releasing everything glides; the camera follows behind. Stamina so a young
  adult can't fly forever; landing anywhere flat. Adolescents glide from a height and hop.
- **B. Sim-like**: pitch and roll directly; more skill, harder to enjoy on a circle pad.
- **Riding or following?** Recommended: you ride a grown dragon (a small rider on its back);
  before that you walk with your young dragon, which hops and glides beside you (D44).

## 6. Challenges: which first?
- **A. Two in the air, one on the ground** (recommended): **Sky Rings** (fly through rings
  against the clock), **Lantern Trial** (light lanterns with your dragon's breath, each
  element its own effect), and **Fruit Catch** (a ground one that needs no training).
  Cups Ember → Flame → Blaze → Starfire, ribbons and trophies for the den. The Command
  Trial and the Shine Show wait for training and grooming.
- **B. Only the air ones** now.
- **C. The roadmap's ground three** (Command Trial, Fruit Catch, Shine Show) now; air later.

## 7. People
- **A. The player character now, a few folk later in this milestone** (recommended): you
  appear in the world (walking, and riding), with a simple creator (a few hair, skin and
  outfit choices); then a handful of villagers who run the Market, the Sanctuary and the
  arena, with short lines and a first small quest or two. The campaigns proper wait.
- **B. No people yet**: only dragons in the world.
- **C. People and a first campaign** in this milestone.

## 8. Art for the world
- **A. The dragons' look carried over** (recommended): low-poly, toon-lit, soft colours; the
  landscape a height field with painted vertex colours and a few tiling textures, trees and
  rocks as low-poly props, fog and a sky dome with the den's time of day. Concept images first
  (as with R5) for you to choose from before anything is built.
- **B.** Something new for the world (tell me what you picture).

## 9. What waits
Recommended to wait for the milestone after: tricks, the voice commands, the training yard,
the Command Trial and Shine Show, the grooming redesign, several wanderers at once. The
Wanderings could fold into the world (a walk you actually take with your dragon) — worth a
word from you.

## What happens next
You answer here or in chat; I record it in the decision log, reorder the roadmap if the
order changes, then write the detailed plan (work packages, reviews and hardware runs, as in
Alpha 2) for you to sign off. The first package would likely be concept images of the valley
and a technical test on the 3DS (a flyable height field with fog and one place in it) to find
the frame budget before anything else is built.
