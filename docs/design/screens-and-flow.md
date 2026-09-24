# Screens & Flow

Status: **Draft v0.1** (2026-09-23). Milestone tags: **A1** · **A2** · **B** · **1.0**.
Visual style: [theme & art direction §6](theme-and-art-direction.md).

## Conventions
- **Top screen** (400×240): the 3D world. **Bottom screen** (320×240): touch UI.
- Touch first. Buttons mirror it: **A** confirm, **B** back, **START** opens the system
  menu (save & quit, settings), **L/R** switch tabs. Touch targets ≥ 32×32 px.
- Every screen works without the mic; voice is always optional.
- The game **autosaves** after meaningful events (hatch, feed, naming, purchases, results)
  and on quit. A small ember icon flickers in the corner while saving.

## Flow

```
Boot ─▶ Title ─┬─ Continue ─────────────────────────────▶ Den (hub)
               └─ New game ─▶ Your name ─▶ Choose egg ─▶ Den (egg in nest)

Den (hub) ─┬─ Care tab ─▶ Petting close-up, Feed, Groom, Play
           ├─ Nest tab ─▶ Egg care ─▶ Hatching ─▶ Name your dragon
           ├─ Dragons tab ─▶ Profile ─▶ Rename / Move to Sanctuary        (A2)
           ├─ Pouch tab ─▶ items                                          (A2)
           └─ Map tab ─┬─ Market                                          (A2)
                       ├─ Sanctuary / Cold Vault                          (A2)
                       ├─ Nesting Stone (breeding)                        (A2)
                       ├─ Wanderings                                      (A2)
                       ├─ Training yard                                   (B)
                       ├─ Arena ─▶ Event ─▶ Results                       (B/1.0)
                       └─ Skyreach Valley (ride / fly)                    (1.0)
START ─▶ System menu: Save & quit · Settings · (Dev menu in debug builds)
```

## Screens

| Screen | Top | Bottom | Milestone |
|---|---|---|---|
| **Title** | Wordmark, glowing egg, embers | Continue / New game | A1 |
| **Your name** | Den silhouette | 3DS software keyboard (swkbd) | A1 |
| **Choose egg** | Selected egg, blurb | Ember / Tide / Gale eggs (tap twice) | A1 |
| **Den — Care tab** | 3D den, dragons wander, day/night | 4 ember gauges, heartglow orb, Feed / Groom / Play, "Pet" enters close-up | A1 |
| **Petting close-up** | Den view from behind the player | The dragon rendered close-up; touch zones (head, chin, back, belly) follow its bones | A1 |
| **Feed / Groom / Play** | Dragon reacts | Drag food to the mouth; brush strokes; flick the ball | A1 |
| **Nest tab / Egg care** | Egg in its nest, glow = warmth | Rub to warm, turn, tap to listen; incubation progress | A1 |
| **Hatching** | Crack → emerge → first blink (cinematic, skippable after first time) | "It's hatching!" | A1 |
| **Name your dragon** | The new hatchling looking at you | swkbd with a random name suggestion pre-filled (D27) | A1 |
| **Upset / make-up** | Dragon in the sulk nook | Hint text; hold still, then offer a treat, then pet | A1 |
| **System menu** | Dimmed game | Save & quit, Settings | A1 |
| **Settings** | — | Music, SFX volume; voice on/off; 3D effect on/off; clock notes; delete save (two-step confirm) | A1 (voice toggle B) |
| **Dev menu** (debug builds only) | — | Time skip, set needs, force stage, spawn egg, budget overlay toggle | A1 |
| **Dragons tab / Profile** | Selected dragon posing | Name, breed, sex, stage, personality, stats, parents, rename, move | A2 |
| **Sanctuary** | Keepers' meadow illustration | Grid of stored dragons; move to/from the den | A2 |
| **Cold Vault** | Frosty cave illustration | Grid of eggs; move to a nest | A2 |
| **Nesting Stone** | The pair on the stone | Pick a male and a female; readiness hint (`breedBlockHint`) | A2 |
| **Market** | Stall and keeper | Buy / sell tabs; daily egg (sex-labeled); Gleam | A2 |
| **Wanderings** | Trail map | Pick a dragon, start; on return: steps → finds | A2 |
| **Pouch** | — | Item grid by category | A2 |
| **Training yard** | Dragon + trick demo | Trick list, practice, record a voice command | B |
| **Arena / Event / Results** | Event in 3D | Cues (buttons or map taps); score; cup and ribbon | B, 1.0 |
| **Skyreach Valley** | Riding/flying view | Minimap, compass, dismount | 1.0 |

## First-time experience (A1)
1. Title → New game → type your name.
2. Choose an egg. It appears in the den nest, glowing faintly.
3. Tutorial prompts (one line each, never blocking): rub the egg; come back later — eggs
   take about a day; the heartglow shows how it feels.
4. Hatching → name your dragon → first feed → first pet.
5. The prompts end. From here the den is home.
