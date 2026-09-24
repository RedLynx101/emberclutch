# Review R1b: sculpt, second pass (blocking)

**Sent:** 2026-09-23 · **Gate:** texturing (WP2 step 2) waits for Noah's OK (D32).
**Answers:** [R1 verdict](R1-sculpt.md#noahs-verdict). **Decisions proposed:** D36–D38 in the
[decision log](../../plan/decisions.md).

**Sheets:**
- `R1b-sculpt.png`: rows are Ember, Tide, Gale. Columns: hatchling (three-quarter), hatchling
  (front), juvenile, adult (three-quarter), adult (side).
- `R1b-growth.png`: each breed from hatch day to adult, at true relative size.

The shading is still a flat preview (toon ramp and breed colors). There is no texture, mouth
or scale detail yet; those come with texturing.

## What changed, point by point

1. **Hatchlings are redone from scratch (D36).** The hatchling is no longer the adult mesh
   squashed down. It has its own baby body, sculpted from soft round volumes:
   - a big round head with large eyes and highlights;
   - a short snout and chubby cheeks;
   - a round belly (no chest keel) and stubby legs;
   - little horn buds and tiny wings attached to the back.

   When it grows up to juvenile, it swaps to the grown body behind a glow ("the first
   molt"). From then on it grows smoothly: a lanky, big-headed juvenile, then the
   adolescent, then the adult.
2. **Classic dragon wings for every breed (D37).** Every breed now has the same wing
   skeleton:
   - arm, forearm and a thumb claw;
   - four long fingers spread across the whole membrane, which reaches back along the
     flank to the hips.

   No panel is bare anymore. The wings are much larger and held up in a V at rest. The
   wing gene now only changes the trailing edge: Ember has **Classic** (scalloped),
   Tide has **Sail** (smooth and rounded), Gale has **Plumed** (frilled).
3. **Breeds differ in shape, not just color (D38):**
   - **Ember (sturdy):** heavy chest, thick neck and legs, big swept horns, a row of
     back spikes, spade tail.
   - **Tide (long):** serpentine neck and tail, short legs, large lobed ear fins, a fin
     sail down the back, fan tail.
   - **Gale (sleek):** slender with long legs, a feather crest, feather plumes along
     the neck and back, a feather-tuft tail. As a baby, the crest is a fluffy tuft.

   The frill gene now also picks the back ridge, so bred dragons mix these looks too.
4. **Budget:** every combination is at most 3,000 triangles (a PC test checks this).
   Starters: Ember 2,828, Tide 2,648, Gale 2,910. Hatchlings are 2,400–2,600.

## Please judge
1. **Hatchlings:** cute enough? Is the head too big or too small, and are the eyes right?
2. **Wings:** is this the "generic dragon wing" you meant? Is the size right?
3. **Breeds:** do Ember, Tide and Gale now read as different dragons in silhouette?
4. **The first molt:** is the swap from baby body to juvenile at the first stage-up OK?
   (See the growth sheet.)
5. Anything else that feels off.

## Known and planned
- In the idle pose, wings stay raised for now. Folding and spreading come with the
  animation pass (WP5, review R3).
- A mouth line, belly-plate ridges and scale detail come with texturing (R2).

## Noah's verdict
*(pending)*
