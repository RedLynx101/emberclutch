# Care Interactions: the hands-on polish

Status: **Plan v1** (2026-09-24; Noah's answers: all of it in Alpha 1, D43; visible dirt, D46). Noah: care should feel like *Nintendogs*: "actually throw
a ball or similar for the dragon, who goes to get it", "a brush when brushing that goes
around where you're brushing", "petting the same", "a full polish". This page is the spec
for [Alpha 1 WP7](../plan/alpha-1.md#wp7--interactions) and the toys that follow in Alpha 2.
It deepens [GDD §4](game-design.md#4-care-interactions-touch-first); screens are in
[screens & flow](screens-and-flow.md).

## 1. Principles
1. **The stylus is your hand.** Where you touch on the bottom screen is exactly where it
   lands on the dragon, and the tool you're holding (hand, brush, cloth, sponge, food) is
   drawn right there, moving and tilting with the stroke.
2. **The dragon notices first.** Within a tenth of a second of a touch its eyes and head
   turn toward it. Reactions then build over a few seconds: lean in, eyes ease shut, a
   purr, then a happy leg kick at its favourite spot.
3. **Props are real things in the den.** Balls and toys fly, bounce, roll and come to rest
   in the 3D room, and stay where they land. Dragons play with them on their own.
4. **Personality, mood and age colour everything.** A Shy dragon startles at a poke; a
   Playful one brings the ball back fast and sometimes plays keep-away; a tired one
   watches the ball roll by. Babies stumble after the ball, adults snatch it out of the air.
5. **Every action gets an animation, a sound, a particle effect and a small change to
   needs and bond**, with a little randomness so it never feels canned. Nothing locks you
   in: any action can be interrupted by another.
6. **Two screens, one moment.** The bottom screen is the hands-on close-up; the top screen
   shows the den, and follows a thrown toy and the dragon chasing it.

## 2. The tool tray
A row of tools along the bottom of the close-up: **hand** (pet), **food**, **brush**,
**cloth**, **sponge** (bath), **ball**; Alpha 2 adds the toys bought at the Market
(tug rope, feather wand, puzzle orb). Pick a tool, then use it on the dragon; the hand is
the default. Items that run out (food, in Alpha 2) show a count.

## 3. Petting (the hand)
- **Cursor:** a soft cartoon hand at the stylus, tilted along the stroke; the fingers curl
  while you press.
- **Where:** touches become points on the dragon via its bones (capsules around the head,
  cheeks, chin and throat, neck, back, belly, tail, folded wings and the heartglow).
- **How:** slow long strokes (gentle), quick back-and-forth (scrubbing), small fast
  circles (scratching), taps (pokes). Too fast or too long becomes rough.
- **Reactions:**
  - leans into the stylus (head and neck bend toward the touch, blended over the clip);
  - eyes ease shut with pleasure (the eyelids from D42), a purr that rises, the heartglow
    pulsing under the stylus, hearts drifting up;
  - **a sweet spot per dragon** (rolled at hatch, e.g. behind the left cheek): a hind-leg
    kick and a burst of tail wagging. Finding it the first time is a small bond bonus and a
    "sweet spot found!" note in the dragon's profile;
  - belly (when it rolls over or sits up): tickles make it wriggle; chin: it lifts its head;
  - a poke on the nose: a blink and a sneeze; a Shy dragon startles back;
  - rough or endless petting: it pulls away with a small grumble (never a punishment);
  - a sleeping dragon purrs and curls tighter.
- **Balance:** the first minute of petting each day counts most (no grinding).

## 4. Brushing and polishing
- **Cursor:** a brush drawn over the dragon at the stylus, turned along the stroke, its
  bristles bending away from the motion.
- **Shine regions:** head, neck, back, belly, left flank, right flank, tail, wings. Each
  holds 0–100 shine. Strokes with the grain (head to tail) count fully; against the grain
  count half and ruffle the scales (a small shiver).
- **Dirt you can see** (D46): each region also has a dirt level that creeps up over a day
  or two, dulling and dusting its colours; Wanderings (Alpha 2) can bring back mud spots.
  Brushing clears dust (with puffs at the brush), the bath clears everything, mud needs
  the bath.
- **Visible result:** as a region's shine rises its gloss and rim glint brighten, so you
  can see what's done.
- **The dragon helps:** it turns the brushed side toward you, lifts a wing so you can
  brush underneath, flicks its tail when you reach the tip, and sits up for the belly.
- **Polish cloth** (after brushing): small circles leave a sparkle trail and a glint sweeps
  across the region. When every region is done, a "gleaming" moment: a sparkle burst and a
  proud pose.
- **Needs:** Shine rises with how much of the dragon you've groomed, not with button presses.

## 5. Bath
- A wooden tub slides in; the dragon hops in (Tide dragons happily, Ember dragons
  grudgingly with steam puffs, the rest in between).
- **Sponge:** rub to raise suds; bubbles gather where you rub and cling to the dragon.
- **Rinse:** drag the ladle over the dragon and pour; water splashes.
- The dragon **shakes off**, and a few drops land "on the screen" of the bottom display.
- Afterwards every region is clean (the dust is gone) and Shine gets a boost.

## 6. Feeding
- **Hand-feeding:** drag a food from the tray toward the dragon. Its head follows the food,
  the jaw opens as the food comes close (driven by distance), and it takes one to three
  bites (crumbs, chomp sounds), then gulps.
- **Favourites** get the happy wiggle and a trill; **dislikes** (one or two per dragon,
  rolled at hatch) get a sniff and a turned head, and the food drops to the floor.
- **The bowl** (Alpha 2): drop food in the den's bowl and the dragon trots over to eat.
- Treats from the hand reward tricks in Beta (same code).

## 7. Play: fetch
- **Throw:** flick the ball on the bottom screen; the flick's direction and speed launch it
  from where "you" stand into the den (an arc with a little spin). A slow drag rolls it
  along the floor instead.
- **Physics:** gravity, bounces (softer on the rug), walls, the hearth, rocks and props as
  obstacles, rolling friction, coming to rest. Each bounce plays the ball sound, louder
  when faster. The top screen follows the ball and the dragon.
- **The dragon:**
  1. watches the throw (head tracks the ball);
  2. chases it around obstacles, pouncing on a rolling ball;
  3. picks it up in its mouth (the jaw opens, the ball sits in the jaw);
  4. trots back to you, sits and drops it at your feet (it rolls toward the screen), wags,
     and waits looking up at you;
  5. juveniles and older **leap and catch** a high throw in mid-air.
- **Personality:** Playful fetches fast and sometimes runs off with the ball (tap to call
  it back); Sleepy or low Energy may just watch; Shy fetches but drops the ball a little
  further away; babies wobble and sometimes lose the ball on the way.
- **Afterwards:** the ball stays where it ends up; idle dragons nudge it, chase it, or carry
  it to their bed. Each fetch raises Play and bond and costs a little Energy.

## 8. More toys (Alpha 2, from the Market)
- **Tug rope:** hold one end; the dragon grabs the other and tugs, growling playfully as
  you pull left and right. Let go and it trots off with it, proud.
- **Feather wand:** dangle the feather in the close-up; the dragon bats at it and pounces
  when it drops.
- **Puzzle orb:** roll it around; a treat drops out after enough rolling.
- **Flying disc** (1.0, outdoors, flying dragons): throw it and the dragon catches it in
  flight.

## 9. Calling and attention
- Tap and hold an empty spot on the bottom screen (or press A) to call: the dragon looks,
  comes to the front of the den and sits. Voice commands replace this in Beta, optionally.
- Without the stylus touching, the dragon keeps glancing at you (the existing look-at).

## 10. How it's built
- **Touch to dragon:** the close-up camera turns the stylus into a ray; it's tested
  against capsules around the posed bones (fast, no mesh raycasts), giving the zone and
  the point on the body where the tool is drawn and the dragon reacts.
- **Tool cursors:** citro2d sprites drawn over the 3D close-up, their angle smoothed from
  the stroke direction.
- **Procedural pose layers** on top of the clips: the head look-at (exists), a lean toward
  the touch, the jaw driven by food distance, the eyelids (exist).
- **Props:** a small physics module in `src/core` (balls, ropes' ends: position, velocity,
  radius, bounce, friction; the den floor, walls and obstacle circles from `DenLayout`),
  PC-tested like the rest of the core. A prop in the mouth is a rigid part attached to the
  jaw bone at runtime.
- **Dirt on screen** (D46): built with texturing (R2). Each body vertex knows its region,
  and a per-dragon dirt level per region darkens and desaturates its colours (a per-dragon
  copy of the paint buffer, refreshed when dirt changes, since shader uniforms are nearly
  full); mud spots are decals on a texture layer.
- **Save:** shine and dirt per region (16 bytes per dragon), toy positions in the den.
  Sweet spots and dislikes come from the genome, so they cost no save space.
- **New clips:** pick up, carry (head up, mouth closed on the toy), drop and sit-wait,
  leap-catch, lean-in (pet), leg kick (sweet spot), sniff-refuse, lift wing (brushing),
  sit up (belly), hop into the tub, tug, paw bat. The shake-off and pounce exist.
- **Sounds** already in hand: brush, polish sparkle, splash, ball bounce, munch, gulp, purr,
  trill, squeak, sneeze. Needed later: rope creak, tub wood knock, suds squish, water pour
  (added to the next sound brief).

## 11. Milestones
| What | When |
|---|---|
| Tool tray; petting with the hand (zones, lean-in, eyelids, sweet spot); brushing with shine regions and the polish cloth; bath; hand-feeding with the jaw; the ball with fetch and physics; calling | **Alpha 1 (WP7)** |
| Tug rope, feather wand, puzzle orb, the food bowl, toys that stay in the den and dragons playing with them and with each other | **Alpha 2** |
| Treats as training rewards; leap and dive catches in competitions | **Beta** |
| Flying disc outdoors | **1.0** |
