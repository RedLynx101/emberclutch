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
| **Beta** | *The valley* | Phase 6 + air half of Phase 5 (moved up, D73): the open valley, the places in it, the painted map, flying and riding, Sky Rings, Lantern Trial, Fruit Catch, the player character, villagers and a first campaign | Settled (D73–D74) · [plan](beta.md) · **next** |
| **1.0** | *A trainer* | Phase 4 + ground half of Phase 5 + Phase 7 polish: tricks, voice, the training yard, Command Trial, Shine Show, grooming, economy, more campaign, public release | Planned · **sit-down first** |
| **1.x** | *Friends* | Sky Visits (local wireless) | Later · **sit-down first** |
| **2.0** | *The meadow* | Equine line: horse, pegasus, unicorn, alicorn | Later · **sit-down first** |

## The road from here (2026-09-25)
Alpha 1 and Alpha 2 are done. What's left, in order, with the gates that hold each step:

**Beta — *The valley*** ([plan](beta.md), settled D73–D74). Tag `v0.3.0-beta`.
1. **Prove it:** a flyable valley in the emulator (streamed height-field tiles, fog, sky,
   one place, the arcade flight controller, the follow camera) measured against the den's
   hardware numbers; concept images (AI, reference only) → **R7** (blocks the world's art);
   the music and sound briefs sent early. ✅ **Done 2026-09-25**; run 14 (the valley on the
   old 3DS) is ready.
   **Then the dragon revamp** (D76–D77, [plan](dragon-revamp.md)): 8 base breeds and 28
   crossbreeds in the new look, each with variants, stats, manners and traits, all rideable;
   the four bases first.
2. **The valley:** the landscape → **R8** (the blockout) → getting about (you on foot with
   your dragon, gliding, flying, riding) → the painted map with fog and fast travel → the
   places standing in the valley, entered to their scenes → discovery, finds and the
   Wanderings in the world.
3. **Challenges:** the arena and the Ember → Starfire cups → Sky Rings (ridden) → Lantern
   Trial (six breaths) → Fruit Catch.
4. **People and the campaign:** you and the creator → **R9** → villagers with dialogue →
   *The Lantern Festival* → **R10** (the outline) → its eight quests.
5. **Close:** saves carry over; the checklist; hardware runs whenever Noah asks (held for
   now, D74); the tag.

**1.0 — *A trainer*** (sit-down first, after Beta). Tag `v1.0.0`, the public release.
- Tricks (gesture, then the name), voice commands, the training yard, the five stats (Wing, Wit, Might,
  Breath, Stamina; D78); the Command Trial and Shine Show with their cups; grooming that fits together;
  several wanderers at once; more of the campaign; the economy and progression across the
  whole game.
- Release polish: tutorial, settings, balance, a performance pass on the hardware, no
  AI-derived asset shipped; the repo goes public with the CIA and 3DSX on GitHub Releases.

**1.x — *Friends*** (sit-down first): Sky Visits over local wireless (visit a den, play
together, gifts, local competitions; a cross-den clutch as a stretch).

**Later — battles** (sit-down first, much later; D78): Pokémon-style battles between
dragons, built on the elements as types (a chart of strengths and weaknesses), the five
stats, manners and traits that the dragon revamp puts in place.

**2.0 — *The meadow*** (sit-down first): the equine line (horse, pegasus, unicorn,
alicorn), as an expansion or its own game.

**Now:** Beta's first step is done (WP1, WP2, the briefs). Waiting on Noah: run 14 on the old
3DS and the batch 2–3 music and sounds; R7 is decided (D75). Now: the dragon revamp (D77), then step 2, the valley.

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

## Beta — *The valley*
**Sit-down done and plan settled (D73–D74), 2026-09-25; detailed plan: [beta.md](beta.md).** Moved ahead of the
trainer content because it's what Noah wants to play next and it's the riskiest thing left
on an old 3DS (a big 3D space, streaming, draw distance).
- **Skyreach Valley:** about 1 km across, a height field in streamed tiles with fog, water,
  cliffs, floating islands, the day's light; 30 fps while flying.
- **The places in the world**, each entered to its own scene: the den in a cliff by a
  waterfall, the Market village, the Nesting Stone hilltop, the Sanctuary meadow and Cold
  Vault cave, the trailheads, the arena, the lake.
- **The painted map:** fogged until explored, live position, fast travel to found places.
- **Getting about:** on foot with a young dragon, adolescents gliding, grown dragons flown
  with you riding (arcade controls, stamina).
- **Challenges:** Sky Rings, Lantern Trial (breath for all six elements), Fruit Catch; the
  Ember → Starfire cups, ribbons, trophies.
- **People:** the player character and a creator, villagers who run the places, **a first
  campaign** of 6–8 quests.
- **The Wanderings in the world:** trailheads as places, trips shown in flight, finds from
  real spots.
- Reviews R7 (concepts), R8 (the valley blockout), R9 (people), R10 (the campaign outline);
  hardware runs 14 (a technical test first), 15 and 16.

## 1.0 — *A trainer*
**Sit-down first (after Beta):** how training works (tricks, gestures, voice, skills), the
ground competitions and their cups, the economy and progression across the whole game, the
grooming redesign, the Wanderings' next pass, more of the campaign, and the public
release's scope. Nothing below is built until it's passed.
- Trick learning (gesture then name), skill curves, 12 tricks; the five stats (Wing, Wit, Might, Breath, Stamina; D78).
- Voice: mic capture + MFCC/DTW template matching on a worker thread; cue buttons always.
- The training yard; the ground competitions **Command Trial** and **Shine Show** with their
  cups (Fruit Catch comes with Beta).
- **Grooming that fits together** (WP12b, from Alpha 2): the bath washes grime and leaves
  the dragon damp, the cloth dries and polishes, the brush clears shed scales; the three
  tools under one Groom button; judged in the Shine Show.
- **The Wanderings' next pass:** several dragons out at once, each counting its own steps.
- More of the campaign (stories round the cups and the places), people's second pass.
- Release polish: tutorial pass, settings, balance pass, performance pass on hardware.
- Check that no AI-concept-derived asset ships (concept images stay reference only).
- **Public open-source release** (repo goes public, CIA + 3DSX on GitHub Releases).

## 1.x — *Friends*
**Sit-down first (after 1.0):** what friends do together over local wireless, what's shared
and traded, how a visit plays, what stays single-player.
- Sky Visits over UDS local wireless (based on `3ds-linkplay`'s uds-demo): visit a den,
  play together, exchange gifts, local competitions; cross-den clutch as a stretch goal.

## 2.0 — *The meadow*
**Sit-down first (after 1.x):** whether the equine line is an expansion or its own game, how
it meets the dragons, its creatures, care and progression.
- Horse → Pegasus / Unicorn → Alicorn. See [Equine Line](../future/equine-line.md).
