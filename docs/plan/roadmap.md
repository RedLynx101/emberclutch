# Roadmap

Emberclutch is built in **playable milestones**. Each one is a game you could keep on your
3DS and enjoy on its own, and each is ordered **risk first** (the 3D creature pipeline is
proven before content piles onto it).

- **Live state:** [`docs/STATUS.md`](../STATUS.md)
- **What gets built:** [content & asset inventory](content-and-assets.md)
- **What the screens are:** [screens & flow](../design/screens-and-flow.md)
- **Current work plan:** [Alpha 2 plan](alpha-2.md) (Alpha 1 done: [plan](alpha-1.md), [checklist](alpha-1-checklist.md))

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
| **Alpha 1** | *A living pet* | Phases 1–2: 3D dragon, care, growth, naming, real saves, music, CIA | ✅ Done (2026-09-24, `v0.1.0-alpha1`) |
| **Alpha 2** | *A den* | Phase 3: several dragons, breeding, eggs, Sanctuary/Vault, Market, Wanderings, map with fast travel; style review R5; first hardware runs | ✅ Done (2026-09-25, `v0.2.0-alpha2`; [plan](alpha-2.md), [checklist](alpha-2-checklist.md)) |
| **Beta** | *A trainer* | Phase 4 + ground half of Phase 5: tricks, voice, 3 ground competitions, economy | Planned · **sit-down first** |
| **1.0** | *The sky* | Phase 6 + air half of Phase 5 + Phase 7 polish: flight, riding, valley, Sky Rings, Lantern Trial, public release | Planned · **sit-down first** |
| **1.x** | *Friends* | Sky Visits (local wireless) | Later · **sit-down first** |
| **2.0** | *The meadow* | Equine line: horse, pegasus, unicorn, alicorn | Later · **sit-down first** |

**Next: the sit-down ([brief](beta-sitdown.md)).** Noah wants the places redesigned, the map, the open world, flying and challenges next (run 13, D72): the brief's first question is whether the valley comes before training.

**Every milestone after Alpha 2 opens with a design sit-down with Noah (D65), and no work
on it starts until he has passed it.** The sit-down happens when the milestone before it is
done. Ahead of it Claude writes a short brief: the open questions, the options with a
recommendation for each, and what's already decided. In it the milestone's scale,
mechanics, progression, content and writing are settled, and the outcome goes into the
decision log. Then the milestone's detailed plan is written (like [alpha-1.md](alpha-1.md)),
and Noah signs that off too. What each sit-down covers is listed under the milestone below.

Every milestone ends with: all unit tests green, a full emulator playthrough of its content,
a run on the old 3DS, the docs synced (`STATUS.md`, decisions, this file), a commit and push,
and a RedWiki update.

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
Plan: [alpha-2.md](alpha-2.md). Order: the den grows (style-independent systems below) →
the new app icon (D48) → **style review R5**: three Ember dragon variants vs the current
style, the last thing before Noah decides (D47) → the dragons update in the chosen style
(parts library, all breeds) → the run on Noah's old 3DS (D34).
- Several dragons: den of 3 + 2 nests; Sanctuary and Cold Vault screens.
- Breeding at the Nesting Stone (rules already in `src/core/breeding.cpp`), lay-egg
  sequence, egg shells hinting at elements.
- **After R5, in the chosen style:** the full parts library: builds, all
  horns/frills/wings/tails, patterns, rare traits; 6 base breeds + 15 hybrids; dirt and
  mud (D46).
- A designed emblem **app icon** (D48) and an animated **3D HOME Menu banner** with the
  baby dragon (D50).
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
**Sit-down first (after Alpha 2):** how training works (tricks, gestures, voice, skills),
the ground competitions and their cups, the economy and progression (Gleam, what's earned
and spent, how a dragon grows as a trainee), grooming (below), the Wanderings' next pass,
what Beta leaves out. Nothing below is built until it's passed.
- **Grooming that fits together** (WP12b, moved from Alpha 2): the bath washes grime and
  leaves the dragon damp, the cloth dries and polishes, the brush clears shed scales; the
  three tools under one Groom button; judged in the Shine Show.
- **The Wanderings' next pass:** several dragons out at once, each counting its own steps;
  and the trip shown **in flight** rather than walking (Noah, run 10): the dragon flaps and
  glides over the trail, with the flap loop and glide taken early from 1.0's flight set (a
  hatchling too small to fly flutters and hops).
- Trick learning (gesture then name), skill curves, 12 tricks.
- Voice: mic capture + MFCC/DTW template matching on a worker thread; cue buttons always.
- Wing / Wit / Spark stats.
- Training yard scene; arena scene.
- Ground competitions: **Command Trial**, **Fruit Catch**, **Shine Show**, with the
  Ember → Flame → Blaze → Starfire cups, ribbons and den trophies.
- Music batch 2 and the full sound-effect set.

## 1.0 — *The sky*
**Sit-down first (after Beta, Noah's ask, 2026-09-24):** the open world's scale and mechanics
(how big the valley is, what's in it, how exploring on foot, gliding, flying and riding
work), the map (how it looks and how it blends into the open world), campaign writing and
story, progression across the whole game, people (the player character, the valley's NPCs),
the air competitions, and the scope of the public release. Nothing below is built until
it's passed; the list below is the starting point for that conversation, not a plan.
- Flight animation set and flight controller; adolescent gliding.
- **Exploring with a young dragon** before it can carry you (follow it, hops and glides, D44).
- **Riding anywhere** in free roam; a rider model. **Free flight** over the valley with a
  live map: your position, landmarks discovered by flying near them, and fast travel to
  any discovered place ([map & travel](../design/world-map-and-travel.md)).
- **Skyreach Valley**: height-field terrain, fog, lake, cliffs, floating islands, points
  of interest, riding finds.
- Air competitions: **Sky Rings** and **Lantern Trial** (breath effects for all 6 elements).
- Release polish: tutorial pass, settings, balance pass, performance pass on hardware.
- Check that no AI-concept-derived asset ships (the placeholder icon is replaced in
  Alpha 1 and Alpha 2, D48).
- **Public open-source release** (repo goes public, CIA + 3DSX on GitHub Releases).

**From Noah's run-3 notes (2026-09-24), toward 1.0:** the world map needs to look better (it
works, and the heart travelling the path is liked), and the destination pictures on the top
screen are far too simple; both are redone when the valley becomes an open world to fly
over, which the map should blend into. With it, **people**: cute, Nintendo-like human
characters for the valley's NPCs and for the player, their animations, and the start of
**campaigns** (stories and quests around the places and the cups). To explore once the
valley's first pass exists: style, a rig shared with the dragons' pipeline, the triangle
budget next to the dragons.

## 1.x — *Friends*
**Sit-down first (after 1.0):** what friends do together over local wireless, what's shared
and traded, how a visit plays, what stays single-player.
- Sky Visits over UDS local wireless (based on `3ds-linkplay`'s uds-demo): visit a den,
  play together, exchange gifts, local competitions; cross-den clutch as a stretch goal.

## 2.0 — *The meadow*
**Sit-down first (after 1.x):** whether the equine line is an expansion or its own game, how
it meets the dragons, its creatures, care and progression.
- Horse → Pegasus / Unicorn → Alicorn. See [Equine Line](../future/equine-line.md).
