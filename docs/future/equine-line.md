# Future: The Equine Line (Horses, Pegasi, Unicorns, Alicorns)

Status: planning only. Target: the 2.0 milestone (“The meadow”), after the dragon game ships.

## Idea

A second creature family that reuses Emberclutch's systems: care, growth, breeding,
competitions and riding. It adds new **body plans** instead of new engine code.

| Creature | Body plan | Modules |
|---|---|---|
| Horse | `Equine` | none |
| Pegasus | `Equine` | `Wings` |
| Unicorn | `Equine` | `Horn` |
| **Alicorn** | `Equine` | `Wings` + `Horn` |

The same dragon data fields describe them: `species`, `bodyPlan`, `modules` (bitfield:
Wings, Horn, Fins, Breath). Dragons are `bodyPlan = Draconic` with
`modules = Wings | Breath`.

## Life stages

Foal → Yearling → Adult (three stages instead of five; about 10 days). Wings and horns
grow in with age, the same way dragon horns do.

Equines use the same sex rule as dragons: breeding needs a mare and a stallion.

## Getting an alicorn

Proposed (to be decided):

- **Breeding:** the `Wings` and `Horn` modules are inherited as alleles, like dragon
  elements. Pegasus × Unicorn gives a small chance of an Alicorn foal (both modules
  expressed); two Alicorns breed true more often.
- **Rarity:** alicorns should feel special, but be reachable through play, never only
  through luck.

## What carries over from dragons

- **Wing bones and flight animations** move over from the dragon rig (retargeted),
  including the gliding and flapping cycles.
- **Heartglow** becomes a glowing **mane and tail** for equines (same mask-alpha
  emissive technique).
- **Competitions:** Sky Rings (pegasus / alicorn), Command Trial and Shine Show carry over
  directly. Equines get a grounded **Jump Course** in place of Lantern Trial.
- **Riding:** the free-roam riding code is shared. Horses and unicorns ride on the
  ground only.

## Open questions

- Is it a separate game or an expansion inside Emberclutch's valley ("the Meadow")?
- Can dragons and equines share a den, or does each have its own home (Stable)?
