# Roadmap

Emberclutch is built in **playable milestones**. Each one is a game you could keep on your
3DS and enjoy on its own, and each is ordered **risk first** (the 3D creature pipeline is
proven before content piles onto it).

- **Live state:** [`docs/STATUS.md`](../STATUS.md)
- **What gets built:** [content & asset inventory](content-and-assets.md)
- **What the screens are:** [screens & flow](../design/screens-and-flow.md)
- **Current work plan:** [Alpha 1 plan](alpha-1.md)

**Testing policy (D28):** day-to-day development and checks run in the Azahar emulator
(`tools/emu.ps1`). Hardware testing on an old 3DS happens later, when Noah decides we're
far enough along. Until then, performance is protected by **budget counters** (triangles,
draw calls, bones, memory) that the debug overlay checks every frame against the limits in
[architecture §1](../tech/architecture.md). **Recommended first hardware check: by the end
of Alpha 2 at the latest**, before content volume makes a rendering change expensive.

## Milestone overview

| Milestone | Theme | Contains (old phase numbers) | Status |
|---|---|---|---|
| **Foundations** | Design, tools, proof of pipeline | Phase 0 | ✅ Done 2026-09-23 |
| **Alpha 1** | *A living pet* | Phases 1–2: 3D dragon, care, growth, naming, real saves, music, CIA | ▶ Next |
| **Alpha 2** | *A den* | Phase 3: several dragons, breeding, eggs, Sanctuary/Vault, Market, Wanderings | Planned |
| **Beta** | *A trainer* | Phase 4 + ground half of Phase 5: tricks, voice, 3 ground competitions, economy | Planned |
| **1.0** | *The sky* | Phase 6 + air half of Phase 5 + Phase 7 polish: flight, riding, valley, Sky Rings, Lantern Trial, public release | Planned |
| **1.x** | *Friends* | Sky Visits (local wireless) | Later |
| **2.0** | *The meadow* | Equine line: horse, pegasus, unicorn, alicorn | Later |

Every milestone starts by writing its own detailed plan (like [alpha-1.md](alpha-1.md))
and ends with: all unit tests green, a full emulator playthrough of its content, the docs
synced (`STATUS.md`, decisions, this file), a commit and push, and a RedWiki update.

---

## Foundations ✅ (2026-09-23)
- [x] Name chosen: **Emberclutch**. Private repo created.
- [x] Design docs: GDD, breeds & genetics, theme & art direction, architecture.
- [x] Concept art (growth sheet, hatchling, adult, breed lineup, den, eggs).
- [x] Suno music batch 1 (6 loops, processed) and the loop tool.
- [x] 3DS project skeleton builds a `.3dsx` (themed title + den placeholder screens).
- [x] Portable `core/` simulation (genetics, needs, growth, clock, breeding rules) with
      PC unit tests.
- [x] Wi-Fi deploy scripts (3dslink + ftpd).
- [x] Headless Blender pipeline proven: `tools/blender/dragon_blockout.py` renders and
      exports glTF for any growth value ([v0 renders](../art/blockout/)).
- [x] First run in the **Azahar emulator**: title → starter pick → rub egg → day skip →
      hatch → feed/pet → save → reload, at 60 fps. Fixed: the hatchling's head covered the
      heartglow; on-screen messages were too short.
- [ ] First run on hardware (deferred per D28).

**Blockout v0 findings:** the hatchling reads as cute (the head and eye proportions
work), but the snout is too duck-like. The adult does not read as majestic: the body looks
inflated, the neck is beaded, and the wings are flat wedges. Both are over the triangle
budget (3.7k / 4.2k vs 1.8k / 3k) because they're built from separate primitives.

## Alpha 1 — *A living pet*
Full plan: [alpha-1.md](alpha-1.md).
- 3D engine layer: citro3d, skinned toon renderer, mask coloring, heartglow, debug overlay.
- The dragon: one organic mesh that grows through all stages, the three starter breeds'
  parts, sex differences, procedural textures, rig, ~22 animations.
- The den in 3D with day/night, nests, the sulk nook.
- Care loop, hands-on like *Nintendogs* ([care interactions](../design/care-interactions.md)):
  pet with a hand that follows the stylus (lean-in, sweet spot), brush and polish with
  visible tools and shine regions, bath, hand-feeding into the jaw, throw the ball and
  the dragon fetches it; sleep/nap, upset → make-up.
- Egg care and a hatching sequence; naming with the 3DS keyboard; rename in the den.
- Real save system (versioned A/B + CRC + migrations).
- Music streaming (Ogg loops) and a first sound-effect set.
- Installable CIA with an original icon and HOME Menu banner.
- **Exit:** in the emulator, a starter egg is raised to an adult (with time skip), every
  care action is animated, saves survive restarts, and all budget counters stay green.

## Alpha 2 — *A den*
- Several dragons: den of 3 + 2 nests; Sanctuary and Cold Vault screens.
- Breeding at the Nesting Stone (rules already in `src/core/breeding.cpp`), lay-egg
  sequence, egg shells hinting at elements.
- The full parts library: builds, all horns/frills/wings/tails, patterns, rare traits;
  6 base breeds + 15 hybrids.
- **Wanderings** (pedometer), wild eggs, trinkets and the hoard pile.
- **Market**: food, grooming, toys, decor, and the daily sex-labeled egg (D24). Gleam.
- **The world map** with **fast travel** between the Den, Market, Nesting Stone,
  Sanctuary / Cold Vault and the Wanderings trailheads ([map & travel](../design/world-map-and-travel.md)).
- More toys (tug rope, feather wand, puzzle orb), the food bowl, toys that stay in the den
  and dragons that play with them and with each other.
- Dragon profile with parents (family tree).
- **Exit:** breeding produces visibly distinct offspring; a player can get a partner of
  the opposite sex through the Market or Wanderings.

## Beta — *A trainer*
- Trick learning (gesture then name), skill curves, 12 tricks.
- Voice: mic capture + MFCC/DTW template matching on a worker thread; cue buttons always.
- Wing / Wit / Spark stats.
- Training yard scene; arena scene.
- Ground competitions: **Command Trial**, **Fruit Catch**, **Shine Show**, with the
  Ember → Flame → Blaze → Starfire cups, ribbons and den trophies.
- Music batch 2 and the full sound-effect set.

## 1.0 — *The sky*
- Flight animation set and flight controller; adolescent gliding.
- **Exploring with a young dragon** before it can carry you (follow it, hops and glides, D44).
- **Riding anywhere** in free roam; a rider model. **Free flight** over the valley with a
  live map: your position, landmarks discovered by flying near them, and fast travel to
  any discovered place ([map & travel](../design/world-map-and-travel.md)).
- **Skyreach Valley**: height-field terrain, fog, lake, cliffs, floating islands, points
  of interest, riding finds.
- Air competitions: **Sky Rings** and **Lantern Trial** (breath effects for all 6 elements).
- Release polish: tutorial pass, settings, balance pass, performance pass on hardware.
- Replace every AI-concept-derived asset (the placeholder icon) with original work.
- **Public open-source release** (repo goes public, CIA + 3DSX on GitHub Releases).

## 1.x — *Friends*
- Sky Visits over UDS local wireless (based on `3ds-linkplay`'s uds-demo): visit a den,
  play together, exchange gifts, local competitions; cross-den clutch as a stretch goal.

## 2.0 — *The meadow*
- Horse → Pegasus / Unicorn → Alicorn. See [Equine Line](../future/equine-line.md).
