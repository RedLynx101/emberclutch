# Review R3: animation (not blocking)

**Sent:** 2026-09-23 · **Gate:** none (D32): comments get folded in when they arrive.
**Clips:** 32 for Alpha 1 (`tools/anim/clips.py`), shared by every breed and both body forms.

**Sheets** (each row is one clip, five moments left to right; three-quarter view, Ember):
- `R3-anim-grown-1.png`: idle, walk, trot, sit, lie down, curl up to sleep, eat, chin scratch.
- `R3-anim-grown-2.png`: roll over for a belly rub, hop, pounce, sulk, greet, wing flutter, yawn, shake.
- `R3-anim-hatchling.png`: idle, walk, sit, curl up, eat (baby version), roll over, hop, pounce.

The renders are Blender previews made with the same pose math as the game, so the 3DS
shows the same motion. The camera stays put, so a hop or a pounce moves through the frame.

## How it plays in the den
- **Dragons live on their own.** They look around, scratch, wander (walk or trot), sit, lie
  down, yawn, wag and flutter. How often each happens depends on mood, personality and
  energy: a Sleepy dragon yawns and lies down more, a Curious one sniffs about, a Joyful one
  wags and flutters.
- **Rest and feelings.** Tired dragons and all dragons at night walk to the nest, lie down, curl up and
  sleep until rested. An upset dragon walks to the sulk nook, turns its back and lies down
  until you make up (nuzzle, then a greeting).
- **Care reactions.** Stroking the head leans into your hand; stroking lower lifts the chin. Holding L
  gives a belly rub for now (WP7 maps strokes onto the body). Feeding plays eat, with a happy
  wiggle for its favourite food. Grooming ends in a shake; play is a pounce or a hop.
- **Look at the player.** Dragons turn their heads toward you when idle, sitting or greeting,
  but not while eating, sleeping or sulking.
- **Feet stay planted.** Walking speed is measured from each body's stride.
- **Sounds.** Footsteps, chomps, thumps, wing flaps, yawns and purrs play on cue (placeholder
  sounds until the Suno set).

## Please judge
1. Do the idle life and the reactions feel like a pet?
2. Folded wings at rest: do they read as folded dragon wings?
3. Anything that looks stiff or wrong (sit, lie, curl, roll over)?

## Known and planned
- Hatching (egg wiggle, crack, emerge, first blink) comes with the egg model.
- Walking legs don't bend around obstacles or slopes (flat den floor for now).

## Noah's verdict
- 2026-09-24: folded wings clipped into the back and weren't cute: redone as a bird-like
  fold with the membrane following the fingers (done). The **tail wag** pivots near the
  back of the dragon: redo it centred on the body with cute leg movement (planned, see the
  Alpha 1 plan's cuteness pass).
