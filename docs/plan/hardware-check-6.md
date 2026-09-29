# Run 21, take 4 (0.9.4)

**For Noah.** Take 3 got you out into the valley and through a lot of it: thank you for the photos
and screenshots, they found the flicker. **Passed in take 3** (no need to try again): the den's
Love and Energy, the profile, the tip cards; the painted ground, fireflies and leaves, the Journal
and tracking, travel by pins; the fox; Tamsin's battle; the voices; photos; the den's speed. Every
note you wrote is dealt with below, the flicker first. The build is already on your 3DS (at .51):
in FBI, SD → cias → `emberclutch.cia` → Install CIA; your save carries over. The trace is still on
(the flashes as a screen began are gone: they were its checkpoints showing half a picture).

## 0. Start
1. Install `emberclutch.cia` (over the old one), **Continue**, and head out to the valley.

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
