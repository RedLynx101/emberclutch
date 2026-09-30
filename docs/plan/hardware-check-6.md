# Run 21, take 4 (0.9.12)

**0.9.12, the flicker pinned to one draw.** Your 0.9.11 session gave the clearest picture yet. You
stood still by the den's door for a while, and the tracer checked the same view twelve times. Each
time, every ground tile but the first wrote its depth normally. The first tile (the big one under
you, 60% of the screen) wrote all of it in some frames and none in others, with the view unchanged.
The first tile is also the only one whose draw carries the whole batch of new settings (the texture,
the haze's table, the colour stages, the shader's numbers) after the depth setting; each later tile
carries the depth setting alone. The flashing lake is the water showing through where that tile's
depth is missing, and it came back often (11% of valley frames), sometimes every other frame. The
battle's flashing health bars and win card look like the same fault the other way round: the 2D
drawn over the 3D keeps the 3D's depth setting and is hidden behind it. This build:
- draws a small far tile first as a throwaway, so it takes that batch, and every real tile then goes
  with the depth setting alone (it's drawn again in its turn);
- does the same wherever the game switches from 3D to 2D: two invisible one-pixel draws take the
  switch's batch, so the bars, cards and panels keep the right setting;
- **the Trailhead freeze:** the tracer missed that frame, because it only started checking a new
  scene one frame late. It now checks the new scene from its first frame, so if it freezes again the
  trace will show the part. It went through the door fine in the emulator.

**0.9.11, back to what worked.** 0.9.10 froze twice for you, both times soon after you went out to
the valley. It was a freeze, not a crash: there was no crash dump on the card. The first time, the
graphics chip never finished drawing the bottom screen in the third frame outside. The second time,
it froze within a few seconds of the tracer's first check, the first time it read the chip's own
settings back. Two things were new in 0.9.10, and neither was tried on your 3DS before: re-sending the
depth setting before *every* 3D draw, and reading the chip's settings back. This build takes both
out and draws the valley exactly as 0.9.9 did after its trial, which ran about 17,500 frames without
a single freeze: the setting re-sent before each ground tile only, trace or not. There's no window
of the old way now, so no deliberate flicker. The flicker should be as it was at the end of your
0.9.9 session (much better, not perfect). The photo, music, free camera and swimming changes from
0.9.10 stay. If it still freezes, the trace will show where.

**0.9.10, the fix kept, and a tracer for the rest.** Your 0.9.9 session settled which way works. Of
the seven ways, each tried for about 13 seconds, the ground flickered in 74% of frames as drawn, 66%
without its texture (so the texture was not the cause after all), 84% and 85% with the texture set
up earlier or with smaller coordinates, 42% with a depth pass first and 75% without mipmaps. With
the depth setting sent again before each tile, it flickered in none of its 1,620 frames. Kept after
the trial, it flickered in about 1 frame in 85 (209 of 17,500), which matches what you saw: much
better everywhere, not quite perfect. So this build:
- **sends that setting again before every 3D draw**, not just the ground's tiles (the places, the
  dragons, the people, the water and the rest), whether the trace is on or not;
- **with the trace on, starts with a short window drawn the old way on purpose**, so the flicker
  shows and the tracer catches it. For up to about 40 seconds (it usually ends sooner), the ground
  will flicker a lot and stutter now and then. In a flicker frame the tracer stops the next frame
  after each part and each ground tile, and reads the graphics chip's own settings back from the
  chip, to compare with what the game sent. After the window, the fix; any flicker left is traced
  the same way;
- **the free camera** (valley, the camera button) stays within about 100 m of you and 60 m above;
- **the dragon's swimming** is quieter;
- **photos**: the picture is written to the card in small pieces with pauses between, the music
  keeps more ahead of itself (about 0.64 s instead of 0.38 s), and finding the next photo number
  reads the photos folder once instead of checking each file.

**0.9.9, the ground's texture.** Your 0.9.8 session narrowed it to one thing: in every checked frame
where the ground was drawn with its painted texture, not one tile wrote depth, from the very first.
In the checked frames drawn without the texture, every tile wrote it normally. The commands sent
to the graphics chip were exactly the same in a flicker frame as in a good one, so the chip itself
drops the depth when it textures the ground under some conditions. The turns were too short to
trust the counts (the fault seems to carry over a second or so into the next way), and it varies
with where you look, as you noticed. This build takes longer turns, about 13 seconds each, between
seven ways of drawing the ground:
- as now;
- without the texture (plainer ground);
- with the texture set up earlier in the frame;
- with smaller texture coordinates;
- with a depth-only pass first;
- without the texture's smaller copies for the distance (mipmaps: a little grainier far off);
- with the depth setting sent before each tile.
After about five minutes it keeps whichever works best, preferring a textured way if one works
nearly as well as plain. **Buildings** also stay drawn a little past the haze's distance once
they're in, so they don't blink out as it moves.

**0.9.8, the flicker cornered.** Your 0.9.7 session's checks found the culprit part: in the flicker's
frames the ground's own tiles draw their colour but write no depth at all. The depth was still
empty straight after them even with the graphics chip finished first, while the houses, the den's
cliff and your dragon drawn next wrote theirs. The haze turned out not to matter: the three ways
flickered alike (25%, 18%, 18% of frames by the den's door). This build tests five ways of drawing
the tiles in turn:
- as they are now;
- without their painted texture (the ground looks plainer in those moments);
- with the depth setting sent again before each tile;
- with the houses drawn before them;
- with their colours read once instead of twice.
It keeps whichever clearly beats the rest. It also records the exact graphics commands sent for
the first tile in a flicker frame and in a good one, so I can compare them word for word, and
checks the depth after each tile. As before, the flicker may come and go for the first few minutes
in the valley (the den's door was the worst spot last time: stand there a while).
**Frostspire Hollow:** the river runs 33 m from it, and its broad valley had lowered the room
about 3 m under where the Hollow was built, so the cave and frost ring floated. The room is level
again and the Hollow rebuilt on it.

**0.9.7, the flicker hunted on your 3DS itself.** Your 0.9.6 trace proved me wrong: the ground's
depth still vanished (in 619 of about 16,900 valley frames), with no memory fill left in the frame.
So the clear wasn't it. What the trace does show: in those frames nothing still (ground, trees,
houses) kept any depth, while your dragon and the people did, and it happens in runs on busy
views. This build finds out on the 3DS itself, since the emulator never shows it.
- **For your first two minutes or so in the valley** it takes turns, a second at a time, between
  the old way of drawing the far haze and two new ones. The haze was the only thing in the game
  drawn with its depth test switched off, and on the 3DS that behaves differently from the
  emulator. It counts the flicker under each way, then keeps the best. You may still see the
  flicker now and then in those first minutes; after that it should be gone if one of the new ways
  is the fix.
- **After a flicker** it checks the depth part-way through the next frame, after each part (the
  ground, the houses, your dragon...), to see where it goes missing. The first few are mapped too.
Just play as usual in the valley with the trace on, then tell me: the trace has the rest.
**Your other notes:**
- The roaming trainers walked by a clock that only counted whole seconds, so they hopped once a
  second, and when they stopped for you they slid back and hopped on. Now they walk smoothly and
  stand still.
- You sit on the Crestwing just behind its neck, in front of the wings. The seat had been behind
  its hips, on the base of its tail. The Flurrytail had the same fault and is fixed too, and the
  seat's height now comes from the back's actual surface.
- After a battle the experience bar fills from where it was ("+38 exp"), with the fanfare as it
  passes a level.
- Bursting (R) beats the wings.
- Gale is seafoam now; its sky blue and Frost's ice blue looked the same.

**0.9.6, the flicker found** (your screenshots and the trace's watch caught it): the teal wasn't
the ground fogged. It was the lake's see-through water, drawn last, showing over the ground and
everything still, because the depth they had drawn was gone by then. The colour matches exactly:
the water at two thirds over the green gives 101, 151, 154, and your frames measure 101, 151, 155.
What wiped the depth was the screen's own clear. citro2d clears with a memory fill that the 3DS
runs *alongside* the drawing queued after it, not before it, so on a busy frame it could land after
the ground had drawn. The dragon and critters, drawn later, kept theirs, which is why they never
went teal. The emulator runs the two in order, so it never showed. Each screen is now cleared by
drawing over it, first, in the same list as everything else. The trace now also checks the depth
after each valley frame and counts any frame where the ground's depth is missing. **The den's
entrance:** the valley's ground is sampled every 4 m and the arch's floor spans about one sample,
so the ground there was a slope from samples that rose with the land; the samples round the door
are now held under the floor (a test checks every point of it). **L/R in flight:** R bursts
ahead (it spends stamina) and L brakes, as in Sky Rings; they used to bank, which steering
already does. The build is on your 3DS (at .51).

**0.9.5, your first take 4 notes** (the rest of take 4 goes on below as it was, your ticks kept):
the ground's flicker was the ground alone drawn heavily fogged, as if far off, in odd frames
(teal and washed out, its strokes still there); the valley now draws everything still on one
program, which ends the switch those frames had. The Crestwing's walk had been measured at nothing
(its trot too), so it crawled; every kind is measured afresh now and walks at its legs' real pace,
the stocky ones at least a steady 1.8 m/s. You sit on its back at its real height. Big grown
dragons in the den keep a little less apart, so they stop wedging each other. The build is on your
3DS (at .51), with a watch for one-frame flickers in the trace: if you see one, the trace has it.

**For Noah.** Take 3 got you out into the valley and through a lot of it: thank you for the photos
and screenshots, they found the flicker. **Passed in take 3** (no need to try again): the den's
Love and Energy, the profile, the tip cards; the painted ground, fireflies and leaves, the Journal
and tracking, travel by pins; the fox; Tamsin's battle; the voices; photos; the den's speed. Every
note you wrote is dealt with below, the flicker first. The build is already on your 3DS (at .51):
in FBI, SD → cias → `emberclutch.cia` → Install CIA; your save carries over. The trace is still on
(the flashes as a screen began are gone: they were its checkpoints showing half a picture).

## 0. Start
1. Install `emberclutch.cia` (over the old one), **Continue**, and head out to the valley.
2. (0.9.5) Walk, ride and fly about the valley, the meadow by the lake where the ground went teal:
   does the ground still flicker or wash out teal? (Only the ground did; fewer things flickered.)
3. (0.9.5) Ride your Crestwing at a walk: it walks at a proper pace now (about four times as fast),
   and you sit on its back, not in it. Your other grown dragons walk no slower than a steady pace.
4. (0.9.5) Three grown dragons in the den: they get round each other and each sleeps in its own bed.
5. (0.9.6) The flicker again: walk, ride and fly about the valley (the meadow by the lake, the
   Market village). Does the ground still go teal or blue, or does anything drop out, on either
   screen? The overlay's GPU figure: a little higher than before (it draws each screen's clear)?
6. (0.9.6) The den's entrance from the yard: no ground over the front of its floor.
7. (0.9.6) Flying: hold **R** to burst ahead (the stamina bar drains) and **L** to brake (slower,
   easier to land). The help under the map says so.
8. (0.9.7) The flicker: play in the valley as usual for five minutes or more (busy views: the
   Market village, the meadow by the lake, near the roaming trainers). After the first couple of
   minutes, does the ground still flicker or go blue?
9. (0.9.7) The roaming trainers walk smoothly and stand still when they stop for you.
10. (0.9.7) Ride your Crestwing: you sit just behind its neck. A Flurrytail too, if you have one.
11. (0.9.7) Win a battle: the experience bar fills up on the card.
12. (0.9.7) Burst (R) in flight: the wings beat. Gale's moves are seafoam, not Frost's blue.
13. (0.9.8) The flicker: stand by the den's door a minute, then play the valley as usual for five
    minutes or more (flying too). After the first few minutes, does the ground still flicker?
14. (0.9.8) Frostspire Hollow: the cave door and the frost ring sit on the floor.
15. (0.9.9) The flicker: play the valley for six minutes or more, some of it by the den's door and
    in the places it flickered before. After that, does the ground still flicker? (The ground may look
    plainer or a little grainier in some stretches: those are the ways under test.)
16. (0.9.9) Buildings no longer blink out of view as you move about.
17. (0.9.10) The flicker: go out to the valley and keep playing through the first minute, when it
    flickers on purpose (the tracer's window). After that, play five minutes or more (the den's door,
    the meadow, flying). Does the ground still flicker at all?
18. (0.9.10) The free camera: fly it away from your dragon. It stops about 100 m out and 60 m up.
19. (0.9.10) Swim with your dragon: the paddling and the water are a little quieter.
20. (0.9.10) Photo mode in the den: take several photos in a row. Does the music keep going, and
    the game run on without a pause?
21. (0.9.11) Go from the den out to the valley and back a few times, then play the valley five minutes
    or more. Does it run without freezing? (Step 17's deliberate flicker is gone. Is the flicker as it
    was at the end of 0.9.9, or better?)
22. (0.9.12) The flicker: stand by the den's door a minute (where it flashed most), then play the valley
    five minutes or more. Does the ground still flash to the lake's blue anywhere?
23. (0.9.12) A few battles: do the health bars, the move text and the win card stay steady?
24. (0.9.12) Go into the Wanderers' Trailhead. If it freezes, restart and tell me (the trace shows where).

## 1. The flicker, and your haze
Your screenshots showed it: in some frames every see-through shape vanished at once on **both**
screens (the map's fog, the buttons and panels, the pins; the tracking flag drawn black) while
text and pictures stayed, in the same frames things dropped out on the top screen. Your run 19
screenshots show the same, so it's old. The likeliest cause: the graphics chip reading a frame's
instructions before the CPU's cache had written them out to memory (the emulator has no cache,
so it never shows there). Each frame now pushes its instructions and its 2D out of the cache
before the chip starts, and each screen's 2D starts from a clean slate. The overlay's third line
(SELECT → Overlay) now ends in `2D used/16383` and `flush ok` (`ERR` would mean the 3DS refused
the flush). **Your haze is in:** when a view gets busy, the distance fog draws in over a few
seconds and anything too far to see stops drawing inside it, instead of popping; as the view
lightens, it clears.
1. Walk and ride through the Market village and over the bridge (where it flickered most): does
   the map's fog on the bottom screen stay put, and the buttons? Does anything still drop out on
   the top screen? A screenshot (Y) of anything that flickers helps.
2. In the busiest views (the Market's square, looking across the village) the haze thickens a
   little and eases off as you turn away: too much, too little, or right?
3. If anything still flickers, tell me what the overlay's third line says.

## 2. Riding and walking
1. Get on your grown dragon: you sit **on its back** now, every kind, not floating above it.
2. Grown dragons **walk faster** out in the valley (the Crestwing too), their steps quicker to
   match; running is as it was.
3. **Get off** (Down): the camera stays out, then glides round behind you (no diving in close).
4. Your own **run**: longer strides and fewer of them, at the same speed.
5. **Look** mode: L turns left and R right, as on foot.
6. The **lead** stays above the ground now.

## 3. The valley
1. The **den's arch** is closed off: walk up to it and press **A** to go in (and no grass comes up
   through its floor).
2. The **Keeper's vegetable patch** sits on the ground.
3. A **lantern**: your dragon turns to it and breathes its element's breath onto it (flame, frost,
   a gust...), then it catches.
4. The bottom screen: your partner's **Energy** under the map; **no Home button** (tap the den's pin
   to travel home, then A at its door: the map's tip says so now).
5. **START**'s menu has no Map out in the valley (it's still there in the den: the way out alone
   when you only have an egg).
6. **Bram's quest:** take your dragon through the meadow's flowers, walking or riding along the
   ground: it catches the stray's scent as you come near (a sniff and a note), then finds her.
   His snoring fades as you walk away now (it followed you while you rode).

## 4. The den
1. **Heading out** takes your travel partner (the Outing panel says who goes; "Make ... your partner"
   to change it), and coming back your partner is still the one you chose.
2. The **Crestwing's lying pose**: grown in its sleep (or with the dev menu) it kept the hatchling's
   pose, neck down to the floor; now it takes its own. Tell me if a lying pose still looks wrong (a
   screenshot helps).
3. Two dragons **snuggling** at night keep a little apart (no more overlapping on one bed).

## 5. The camera, in close places
1. **The camera keeps clear** of walls, caves, cliffs and houses whatever you're doing (walking,
   riding, battles, fishing, shows): walk into the **Hidden Grotto** behind the falls and turn about
   (its chest is mended too). Tell me anywhere something still cuts across the view.

## 6. Creatures in the valley
Seven kinds live about the valley (never near the Market's square), each with something for **A**
when you're close. Nothing ever gets hurt. The first time you spot one, a toast; the Journal's
**Places** tab has a **Critters** page (seen, friends made, a note on each).
1. **Songbirds** peck in little flocks in the meadows (a pair circles high by day). Walk or run at
   them and they burst up and fly off. Stand near and **A: whistle to the birds**: one hops over,
   tilts its head and chirps.
2. **Rabbits** at the woods' edges (and white **snow hares** round Frostspire Hollow): **A: play
   chase**, and your dragon stalks, pounces and chases; the rabbit always escapes into a bush.
3. **Butterflies** over the flower patches by day: **A: hold still**, and one lands on your dragon's
   head for a few seconds, until it sneezes.
4. **Frogs** at the shallows (croaking more at night): **A: croak back**; it answers twice and leaps
   in with a splash.
5. **Ducks**, a mother and her ducklings on the water: **A: call the ducks**; they paddle over and
   quack.
6. Friends pay a pinch of Play, Love or bond to your partner (a few times a day) and a little Gleam
   the first time for each kind each day.

## 7. Battles: the Ember league
1. Win or lose, a card: experience (a level up shows on it and as a toast), moves learned, Gleam
   the first time (a rematch pays once a day), titles.
2. The **league board** by the arena's gate: this league's four challengers and its champion,
   **Track** one to put it on the map. The other three: **Oren** (the Trailhead), **Juniper**
   (the Sanctuary), **Fen** (the Orchard).
3. Beat all four and **Marigold**, the Ember champion, waits on **Emberpeak Caldera**'s ring (a
   new place: its pin on the map). A final won: a title for your dragon, a prize (and a prize dye),
   and the **Flame league**'s challengers arrive round the valley.

## 8. Roaming trainers, duels and the villagers' days
1. Between 8:00 and 20:00, three to five of eight **roaming trainers** walk the valley's paths,
   each with their dragon at their heels; they sit a while at the paths' ends and look about.
   Come near and one waves with a hello; walk up and they stop for you.
2. **A:** their lines, then "**Duel?**": a friendly battle right there, their dragon matched to
   your partner's level. Experience every time; a little Gleam for each trainer's first loss of
   the day; your **duels won** on the Record page.
3. Trainers near someone else's battle stop to watch and clap.
4. **The villagers' days:** Maple tidies her stall, Bram scatters feed, Wren writes on her
   clipboard, Pip flies his toy dragon, Rowan and Sable sit in the evening, and everyone dozes at
   night (a soft snore close by). In a battle the trainers bow, point, wince and cheer.
5. At Driftwood Cove, Tam fishes with a rod, and you hold yours as you fish.

## 9. Frostspire Hollow
1. A new place high in the cold: an icy bowl ringed by spires. **Tove** by the corridor (her brazier's legs hold its bowl now): **A**,
   her welcome, then where to start (floor 1, later every fifth floor you've reached).
2. Each floor a wild dragon comes out of the cave door; beat it and go **Deeper** or **Leave**.
   Every fifth floor is a guardian: the first time past each, a prize (food, a trinket and one of
   the Hollow's four things to wear). Thirty floors.
3. **The camera** (your note): your dragon at the lower left, the foe and the glowing door beyond;
   no wall across the screen on any floor. And the gate: you and Tove side on.

## 10. Driftwood Cove
1. A new place on the shore: a beach, a jetty and **Tam**. Walk out on the jetty and
   press **A** to fish: A casts, wait for the bobber to dip, **strike**, then keep the line in
   the band until it's landed. Eight bites a day. You're seen fishing now, even if you rode there.
2. Fish and roots go in the pouch (your dragon gets a nibble); shells and now and then a pearl
   are worth Gleam. Shells wash up on the beach too (A to pick one up).

## 11. Moonpetal Glade: shows and the wardrobe
1. A new place, a moonlit garden stage. **Celestine**, the host: pick a show (its **theme**:
   cute, elegant, fiery, frosty...). The show: the **Look** (how your dragon's things suit the
   theme), **Poise** (its manner, care and Love), your **Performance** (press each cue as it
   reaches the ring, in time with the beat), the rivals, the results.
2. **Linnet's Finery** (hats, scarves, capes, tail things) and **Madder's Dyes**: buy with Gleam.
   Try them on in the **wardrobe** (the tent, or the profile's **Dress up**). Wins give the shows'
   own prizes; a whole league won, a prize dye.
3. The stage is bigger now (room for three grown dragons), and the Performance plays your new
   **show-stage** track: the cues land on its beat.
4. What your dragon wears shows in the den, the valley and battles.

## 12. The challenges, again
1. Each cup pays once a day (the first win more); each go costs Energy. The tip cards wait while
   you race.

## What to send back
- The flicker and the haze (section 1): better, the same, or different?
- How the trainer's game feels: battles, the Hollow's climb, fishing, shows.
- Anything broken, stuck or confusing; if it froze anywhere, where (the trace says the rest).
