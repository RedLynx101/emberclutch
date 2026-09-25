# Look and Feel: Storybook, in the Way of a Cozy Life-Sim (R7 → D75)

Status: **decided direction, 2026-09-25** (Noah at R7: "A, let's go storybook with Animal
Crossing like textures, feel, speech, player movement, etc, but with dragons and such").
This page says what that means for Emberclutch, part by part, and what each part costs on
the old 3DS. It extends [theme & art direction](theme-and-art-direction.md) (the heartglow,
the palette and the UI stay); the dragons get their own redesign (R11, D76). The
reference is *Animal Crossing* (on the 3DS, *New Leaf*), studied for how it feels; nothing
of it is copied, and no Nintendo names or assets go in the game.

## What makes that feel (research)
- **Simple on purpose.** Its designers speak of a "trigger of play" (easy to pick up) and an
  "imagination gap": the art stays simple so the player imagines the rest. Readable
  shapes, few details, lots of charm in motion.
- **Proportions.** Characters are a little over **two heads tall**: a big round head, big
  eyes, a small body, thin limbs, round ball hands.
- **Textures.** Hand-painted, soft, low-frequency: gentle gradients and simple motifs
  (spots, stripes, a patterned grass), never photographic detail. Light is soft with a
  gentle shade side; no hard outlines.
- **The curved world.** The ground rolls away from you like a log (the "rolling log"): a
  vertex shader bends everything down with its distance from the player along the view,
  so the horizon is close, the sky fills the top of the screen, and far things drop out of
  sight on their own.
- **Movement.** Walk with the circle pad (speed follows the push), run with a button held,
  quick turns with a little arc, a bouncy walk with a touch of shake, dust puffs when
  running, footprints; no jumping.
- **Speech ("Animalese").** Dialogue is written as text and voiced letter by letter: each
  letter plays a short recorded sound, very fast and pitched up, so it sounds like cute
  babble that follows the words. Each character has their own pitch and pace (a grumpy
  old one deep and slow, a peppy one high and quick). Lines type out in a rounded speech
  box with the speaker's name on a tab, with little reaction pops (!, ?, …, a heart).

## For Emberclutch
### The world (Beta WP3–WP4)
- **The rolling-log curve** in the valley's and the places' vertex shaders on foot (a
  gentle bend; flying straightens it out so the valley opens up below). It's one line of
  shader maths, and it helps the budget: far tiles fall below the horizon and aren't drawn.
- **Hand-painted textures** on the ground and props, as a few small shared textures (a
  grass motif, a path, rock, bark, water ripples) tinted by vertex colour and by the time
  of day, instead of the flat vertex colours of the valley test. On the 3DS: one tiling
  64×64 or 128×128 texture per material, blended with the vertex colour in the combiner.
- **Round, chunky props:** trees as soft rounded crowns (pines as stacked soft cones),
  bushes, flowers, rocks as pebbles, fences, lanterns, all low-poly and fat-edged.
- **The camera on foot:** one fixed angle, always looking north and tilted down about
  30–40° at you, easing to follow; the circle pad never turns it, and the valley is laid
  out to be read that way, north up like the map (to confirm: R11 question 11). In flight
  the chase camera as now.

### You and the people (WP5, WP12, WP13)
- **You:** about 2–2.5 heads tall, round hands, a simple face with big eyes; walk (analog),
  run (hold B), a bouncy step, dust puffs and footprints; your dragon trots at your side.
- **Villagers:** the same proportions; later more of the high-fantasy folk the valley can
  hold, not only humans (Noah, R7: "lean into high fantasy"), for example tree-folk,
  small fox-like or owl-like folk, a gnome, a stone giant; noted for the villagers' next
  round, not Beta's first five.

### Speech (WP13, WP14)
- **Letter by letter:** a set of letter sounds (one recorded alphabet, see the
  [sound brief](../audio/sfx-batch-3.md)) played per character of the line as it types
  out, sped up and pitched per speaker (the old keeper low and slow, the child high and
  quick); spaces and punctuation pause. Cheap: short samples on the SFX channels.
- **The speech box** on the bottom screen: rounded, cream (`shell`), the speaker's name on
  a coloured tab, the portrait beside it, A to go on, reaction pops over the speaker's
  head in the world.
- **Dragons don't talk:** they keep their chirps, trills and rumbles.

### Music and sound
- The valley's music leans toward that laid-back, playful life-sim sound (soft piano,
  marimba, acoustic guitar, pizzicato, brushed drums, a whistle or flute) with the
  game's fantasy colour (harp, kalimba, tin whistle); the batch 3 prompts were updated to
  suit. Music by the time of day stays as the den does it (day and night now; dawn and
  dusk cues could follow).

### The dragons (R11, D76)
A new look to match: four wholly new silhouettes, crossing that cozy life-sim softness
with a dragon film's lovable, readable dragons (big expressive eyes, cat- and dog-like
behaviour, a clear silhouette per kind) and a touch of awe and majesty in the grown
adults. The heartglow stays. See [R11](../art/reviews/R11-new-dragons.md).

## Sources
- [The rolling-log shader, explained (Alastair Aitchison)](https://alastaira.wordpress.com/2013/10/25/animal-crossing-curved-world-shader/);
  [a Godot version](https://github.com/lynnpepin/rollinglogshader)
- [Animalese (Nookipedia)](https://nookipedia.com/wiki/Animalese);
  [an implementation in Unity and FMOD](https://github.com/usdivad/Animalese)
- [How the art style helped the series (GC Art Column)](https://gcartcolumn.com/2021/10/09/how-the-art-style-of-nintendos-animal-crossing-series-helped-make-it-a-successful-franchise/);
  [the style, drawn (proportions)](https://corvidry.tumblr.com/post/712239060590198784/hi-i-am-working-on-an-assignment-for-school-and);
  [modelling in that style (textures)](https://www.tripo3d.ai/blog/explore/animal-crossing-3d-model)
- [Toothless's design: the head, the eyes' pupils by mood (HTTYD wiki)](https://howtotrainyourdragon.fandom.com/wiki/Toothless_(Franchise))
