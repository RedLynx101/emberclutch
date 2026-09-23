# Breeds & Genetics

Status: **Draft v0.1 for review** (2026-09-23)

Goal: lots of visibly different dragons from a **small, understandable** rule set that is
cheap to render on an old 3DS. Every dragon shares one skeleton; variety comes from
swappable parts, proportion presets, and colors painted by the GPU from a mask texture.

## 1. Base breeds (elements)

| Breed | Element | Look | Heartglow | Breath | Aptitude | Favorite foods |
|---|---|---|---|---|---|---|
| **Ember** | Fire | Ember-orange, golden horn tips, sturdy | Orange | Flame | +Spark | Firepeppers |
| **Tide** | Water | Sea-teal, fin frills, webbed wing-fins, long | Aqua | Mist jet | +Wit | River fish |
| **Gale** | Wind | Sky-blue, feathered wing tips, sleek | Pale cyan | Gust | +Wing | Skyberries |
| **Grove** | Earth / plant | Moss green, leafy frill, bark horns, sturdy | Leaf green | Spore bloom | +Wit | Honeyroot |
| **Frost** | Ice | White-lavender, crystal horns, sleek | Ice violet | Frost breath | +Wing | Frostmelon |
| **Lumen** | Light | Gold-white, halo crest, radiant | Sun gold | Sunbeam | +Spark | Starfruit |

Starters: Ember, Tide, Gale. Grove, Frost and Lumen come from Wanderings and the Market.
Reserved for expansion: **Umbra** (shadow), keeping the same allele model.

## 2. Element genotype → breed

Each dragon carries **two element alleles**. The pair decides the breed:

- Same allele twice → **purebred** (e.g. Ember/Ember = Ember).
- Two different alleles → **hybrid** (e.g. Ember/Tide = Steam).

Offspring receive **one random allele from each parent**. This is plain Mendelian
inheritance, so players can reason about it: two Steam dragons (Ember/Tide) produce
25% Ember, 50% Steam, 25% Tide.

6 elements → 6 purebreds + 15 hybrids = **21 breeds**.

### Hybrid table

| × | Tide | Gale | Grove | Frost | Lumen |
|---|---|---|---|---|---|
| **Ember** | Steam | Wildfire | Cinderbloom | Solstice | Sunflare |
| **Tide** | | Squall | Lotus | Glacier | Pearl |
| **Gale** | | | Thistledown | Blizzard | Aurora |
| **Grove** | | | | Evergreen | Glowmoss |
| **Frost** | | | | | Prism |

A hybrid's **heartglow is two-toned** (both element colors swirl), and its breath is a
blend effect (Steam = hot mist, Aurora = shimmering gust). Hybrid breeds get a signature
default palette and part preference so they read as their own breed at a glance.

## 3. Traits

All traits other than elements use one simple rule so the system stays easy to explain:

> **Parts rule:** each part comes from parent A (45%), parent B (45%), or a fresh roll
> from the child's breed pool (10%).

| Trait | Variants | Notes |
|---|---|---|
| **Build** | Sturdy · Sleek · Long | Proportion presets (bone scales). Long = serpentine neck/tail. |
| **Horns** | Nubs · Swept · Crown · Crystal · Antler | Mesh parts on head bone. |
| **Frill** | None · Fin · Leaf · Feather | Mesh parts on neck/cheek. |
| **Wings** | Membrane · Feathered · Fin | Mesh part + shared wing bones. |
| **Tail tip** | Plain · Spade · Tuft · Fan | Mesh part on last tail bone. |
| **Pattern** | Solid · Stripes · Spots · Dapple · Runes | Channel of the mask texture set. |

### Colors

Each dragon stores three colors: **base**, **accent** (belly/horns/wing membrane) and
**pattern**, as hue/saturation/value bytes.

- **Base color follows element allele A; accent color follows element allele B.** A
  hybrid always shows both of its elements: a Steam dragon is either an orange body with
  teal accents or a teal body with orange accents. Two looks per hybrid, readable at a
  glance.
- Child color = the midpoint of the parents' colors, then ±8° hue jitter, then clamped
  to that allele's element range.
- 15% chance per color to copy one parent exactly (lets lines "breed true").
- Pattern color is inherited the same way but isn't clamped. It's the flair channel.

### Size

A small scale gene, 0.90–1.10 of normal, averaged from parents ± 0.03.

### Rare traits (mutations)

| Trait | Look | Rate |
|---|---|---|
| **Iridescent** | Scales shimmer through hues as it moves | 1/64 spontaneous |
| **Melanistic** | Dark near-black scales, glow looks brighter | 1/64 spontaneous |
| **Leucistic** | Pale pastel scales, pink-tinted glow | 1/64 spontaneous |
| **Starspeckle** | Tiny twinkling specks across the back | 1/128 spontaneous |

A parent with a rare trait passes it on with a 50% chance. A child can have at most two.

## 4. Breeding rules

- Both parents are **Adult**, bond ≥ 300, mood ≥ Content, and neither is Upset.
- Dragons have no sexes. Any two adults can pair, including same-breed pairs.
- Place both at the **Nesting Stone** in the den. The next calendar day there is an egg.
- One egg per pairing. Each parent then rests for **3 days** before breeding again.
- The egg needs a free nest, or it goes straight to the Cold Vault.
- The egg shell shows hints: its glow color(s) reveal the element alleles, and the speckle
  pattern hints at the pattern trait.

## 5. Variety count

21 breeds × 3 builds × 5 horns × 4 frills × 3 wings × 4 tails × 5 patterns
≈ 450,000 part combinations, before colors, size and rare traits.

## 6. Why it is cheap to render

- **One skeleton** (≤ 24 bones per draw) for every dragon. Builds are bone-scale presets.
- **Parts** are small meshes attached to fixed bones; only the selected ones are drawn.
- **Colors** cost no texture memory: a shared RGBA mask texture holds base / accent /
  pattern weights in R/G/B and the heartglow region in A. The GPU's texture combiner
  multiplies these by per-dragon constant colors (see [architecture](../tech/architecture.md)).
- Rare traits are small shader/combiner variations (e.g. an extra specular term for
  Iridescent), not new assets.

## 7. Data (save format sketch)

```
struct Genome {             // 16 bytes
  u8 elementA, elementB;    // Element enum (0..5, 6 = Umbra reserved)
  u8 build, horns, frill, wings, tailTip, pattern;
  u8 baseH, baseS, baseV;   // colours packed as HSV bytes
  u8 accentH, accentV;      // accent saturation follows base
  u8 patternH;
  u8 size;                  // 0..255 → 0.90..1.10
  u8 rareFlags;             // bitfield: iridescent, melanistic, leucistic, starspeckle
};
```

The species-level `bodyPlan` and `modules` fields live on the creature record, not the
genome, so the equine line can reuse this format (see [Equine Line](../future/equine-line.md)).
