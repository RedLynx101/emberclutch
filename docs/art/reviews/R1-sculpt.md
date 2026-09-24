# Review R1 — Sculpt (blocking)

**Sent:** 2026-09-23 · **Gate:** texturing (WP2 step 2) waits for Noah's OK (D32).
**Sheet:** [`R1-sculpt.png`](R1-sculpt.png): rows are Ember, Tide, Gale; columns are
hatchling (sitting), adolescent, adult, adult side view. The shading is only a preview:
flat breed colors with a toon ramp. No textures yet: no scale patterns, no belly-plate
ridges.

## What changed since blockout v0
- **One organic body mesh** made with Blender's skin modifier, instead of stacked shapes.
  One mesh serves every stage, as the game will do.
- **Growth is bone scaling.** The same mesh becomes the hatchling (big round head, huge
  eyes, short neck, stubby legs, round belly, tiny wings) and grows into the adult
  (S-curved neck, deep chest, long tail).
- Starter parts: Ember (swept horns, membrane wings, spade tail), Tide (nub horns, head
  fins, fin wings, fan tail, long build), Gale (swept horns, feather frill, feathered
  wings, tuft tail, sleek build).
- **Budgets:** body 2,000 triangles + parts ~1,400–1,700 + wings ~650; 24 bones for the
  body draw and 10 for the wings. The eye/part triangle count still needs trimming to
  land at ≤ 3,000 total.

## Please judge
1. **Adult silhouette.** Is it majestic enough? Is the head too small?
2. **Hatchling.** It's chubby and cute-ish, but still blob-like. How much more baby
   (rounder, bigger eyes, a clearer neck) do you want?
3. **Breed identities.** Do Ember, Tide and Gale read as different breeds?
4. **Wings.** Size and style for each breed?
5. Anything that feels off.

## Known issues I'll fix regardless
- Gale's feathered wings look like combs: the blades need to be wider and overlap.
- The hatchling's head sits like a helmet: needs a clearer neck.
- The hatchling heartglow is partly hidden; the front legs in the sitting pose need work
  (that comes with the animation pass).
- Small shading lumps: texturing and baked shading will soften them.
- Parts are over the triangle budget: fewer segments on the eyes and spikes.

## Noah's verdict
*(pending)*
