# Roadmap

Each phase ends with something playable on a real old 3DS. Phases are ordered by
**risk first**: the 3D creature pipeline is proven before content is built on it.

## Phase 0 — Foundations ✅ (2026-09-23)
- [x] Name chosen: **Emberclutch**. Private repo created.
- [x] Design docs: GDD, breeds & genetics, theme & art direction, architecture.
- [x] Concept art (growth sheet, hatchling, adult, breed lineup, den, eggs).
- [x] Suno music brief for 5 tracks, plus the loop-processing tool.
- [x] 3DS project skeleton builds a `.3dsx` (themed title + den placeholder screens).
- [x] Portable `core/` simulation (genetics, needs, growth, clock) with PC unit tests.
- [x] Wi-Fi deploy scripts (3dslink + ftpd).
- [x] Headless Blender pipeline proven: `tools/blender/dragon_blockout.py` renders and
      exports glTF for any growth value ([v0 renders](../art/blockout/)).
- [ ] First run on hardware (waiting on 3DS access).

**Blockout v0 findings:** the hatchling reads as cute (the head and eye proportions
work), but the snout is too duck-like. The adult does not read as majestic: the body looks
inflated, the neck is beaded, and the wings are flat wedges. Both are over the triangle
budget (3.7k / 4.2k vs 1.8k / 3k) because they're built from separate primitives.

## Phase 1 — Hatchling slice (go / no-go for full 3D)
- Replace the primitive blockout with a **single organic mesh**: skin modifier +
  subdivision over a spine/limb graph, then decimate or retopologize to budget. Match the
  growth sheet silhouettes (S-curve neck, tapered tail, ribbed membrane wings).
- Headless Blender script: hatchling mesh + ≤ 24-bone rig + idle / happy / eat / sleep animations.
- glTF → `.ecm` / `.eca` converter.
- Skinned toon shader (picasso) plus the mask-based color combiner and heartglow.
- Den scene: one hatchling you can pet (touch zones) and feed. Heartglow reacts.
- Save and load on the SD card (A/B slots).
- Install `3ds-libvorbisidec` and stream `den-hearth.ogg`.
- **Exit:** a stable 30 fps on an old 3DS, and the hatchling feels cute. If full 3D can't
  hold 30 fps, fall back to pre-rendered sprites from the same Blender assets.

## Phase 2 — Care loop and growth
- Four needs, mood, bond, personality, favorite foods.
- Real-time catch-up, clock rollback safety, day/night, sleep.
- Egg → Adult with continuous growth (bone-scale curves for every stage).
- Upset state and the make-up interaction.
- Den with up to 3 dragons + 2 nests. Sanctuary and Cold Vault storage.
- **Exit:** a dragon can be raised from egg to adult over ~2 weeks of real play.

## Phase 3 — Breeding and variants
- Nesting Stone. Genetics wired to the in-game creature records.
- Parts meshes: builds, horns, frills, wings, tail tips. Pattern masks. Rare traits.
- 6 base breeds + 15 hybrids, with egg shells that hint at what's inside.
- **Exit:** breeding two dragons produces visibly distinct offspring.

## Phase 4 — Training and voice
- Trick learning by gesture, then naming. Skill curves.
- Mic capture + MFCC/DTW template matching on a worker thread. Cue-button fallback.
- Wing / Wit / Spark stats.

## Phase 5 — Competitions and economy
- Command Trial, Fruit Catch, Shine Show (grounded or hop versions first).
- Sky Rings and Lantern Trial (these need flight and breath effects).
- Ember → Flame → Blaze → Starfire cups. Gleam, the Market, den decor.

## Phase 6 — Skyreach Valley and riding
- Height-field valley map with fog, points of interest, riding on the ground and in the air.
- Riding finds and wild eggs. Wanderings (pedometer).

## Phase 7 — Sky Visits and release polish
- UDS local multiplayer: visits, gifts, local competitions, cross-den clutch (stretch).
- Full soundtrack, sound effects, banner, icon, CIA packaging.
- Replace AI concept-derived assets with original ones, then the **public open-source
  release**.

## Phase 8 — Equine line
- Horse → Pegasus / Unicorn → Alicorn. See [Equine Line](../future/equine-line.md).
