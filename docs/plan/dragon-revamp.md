# The Dragon Revamp: Work Plan (D76–D77)

Status: **under way, 2026-09-25.** DR1 done (the kit, the engine's groundwork, the Pouncer);
**DR2 done** (D79: all eight base breeds and the first crossbreed, by Opus 5.5 subagents in
parallel; [R11b](../art/reviews/R11b-new-dragons.md) answered, D80: all nine kept, the
Duskwing's ears fixed, rarities and sizes right); run 15 (0.2.1) puts them on the 3DS by the
dev menu, and (0.2.2) the Blazeplume as the HOME Menu banner's hatchling (D80). **DR3 done**
(D82, 0.2.4): every dragon and egg is a kind, the save's own rolled once; the old breeds are
archived. **Next: DR4**, the commons' pairs. What What
the dragons become is in [dragons, version 2](../design/dragons-v2.md): 8 base breeds and 28
crossbreeds (36 kinds), each with three painted variants and a rare one, their own bodies,
babies, eggs and animations, stats, manners and traits, all rideable. It comes before the
rest of Beta (D76): the four bases, into the game, and their six crossbreeds; then Beta
resumes, with bases five to eight and their crossbreeds in waves alongside it (D78).

## How the work is shared
- **Me (the lead):** the shared pipeline and the engine, the checks, merging, building and
  testing in Azahar, the review sheets, the docs.
- **Subagents, Opus 5.5 only (Noah):** one per kind, in parallel, each in its own git
  worktree so they never touch each other's files. Each gets a brief: the kind's concept
  sheet, the look, the design, its body plan's clip list, the budgets, the checks to pass,
  and the renders to deliver, with a render → compare with the concept → fix loop until it
  matches. I merge each kind once it passes, and keep the shared library steady during a
  wave (changes to it go through me).

## DR1 — The pipeline and the engine's groundwork (me) ✅
Done 2026-09-25: `tools/blender/dragonkit/` (model, texture, export, review), `tools/dragons/`
(kinds, plans, clipkit, check, lore, gen_tables), per-kind eggs, `docs/tech/dragon-kit.md`; in
the game `src/core/kinds` (generated tables, colourings, part selection) and the renderer
taking kinds as look slots with each plan's clips (dev menu page 1: Next kind, Kind
colouring); `tests/test_kinds.cpp`; `tests/autotest/kinds.txt`. The Pouncer is the first kind.

- **A kit for kinds** (`tools/blender/dragonkit/`): building a skeleton from a body plan's
  description, skinning, the growth stages (baby; young, adolescent, grown), parts seated in
  the idle pose, the hand-painted texture baker, the exporter, the review renders (stages,
  variants, turntables, animation strips), and the checks (triangles, bones, every clip
  present, parts seated, texture sizes). Today's `dragon_model.py` and `export_dragon.py`
  become its first users.
- **Body plans and clips** (`tools/anim/plans/<plan>.py`): each plan makes every clip on the
  list (the 51 clips today, grown and baby), checked for coverage; the engine's clip table
  becomes per plan.
- **Files by kind:** `romfs/dragons/<kind>/` (egg, baby, grown, the rare variant's model,
  a lower detail level, four textures) and `romfs/anims/<plan>.eca`, loaded when a dragon of
  that kind is around.
- **Engine:** a table of kinds (name, body plan, size, base stats, tendencies), the models
  and clips by kind and plan, eyes whose pupils follow the mood, a rider's seat per plan.

## DR2 — The base breeds (D79: all eight, and the first crossbreed; eight subagents in parallel) ✅ built
- Common: Pouncer (Ember, the lead), Puffback (Grove), Curlstone (Stone); harder to get:
  Crestwing (Gale), Ribbontail (Tide), Flurrytail (Frost); rare: Glimmermoth (Lumen),
  Duskwing (Shade); crossbreed: Blazeplume (Pouncer x Crestwing).
- Each: its body plan (skeleton and every clip, grown and baby), its egg, baby and grown
  body through the stages, three painted variants and the rare variant's model, the checks
  passed, the renders.
- **Review R11b:** a sheet per kind (every stage and variant, a turntable, a strip of its
  key clips), then the four together and in the den.

Done 2026-09-25: nine kinds on eight body plans (the Blazeplume shares the Pouncer's), 36–40
bones each, 55–81 clips a plan, every model inside 3000 / 1200 triangles for the common and
the rare. `check.py --all` OK; PC tests 182,810 checks, 0 failures (every kind loads, fits and
matches Blender's pose within about 0.005); in Azahar every kind as a hatchling, grown and rare in
the den at 16.7–17.2 ms, no unmapped memory access (`tests/autotest/kinds_all.txt`). Stats
climb with rarity (totals: common 28, harder to get 31, rare 34, the Blazeplume 33).
Engine and kit fixes found on the way: contacts from the plan, every `tail*` bone off the
floor, ear and antenna bones in the head's care zone, eggs in display colours, duplicate part
groups merged; the rest are in [the kit's gotchas](../tech/dragon-kit.md#gotchas-learned-building-the-first-nine-dr2).

## DR3 — Into the game (me)
- Genetics v2 (two breed alleles of eight; the variant; potentials and traits) and a new
  save version; **every dragon and egg in a save becomes a random kind in a common variant**,
  fixed by who it is, keeping everything else (D77).
- Stats, manners and traits in the core with PC tests; the profile shows them as you find
  them out; the Dragondex for 144 entries; the Market's eggs; sizes in the den (±50%).
- The eggs: each kind's egg in the den and the Market, bursting into pieces when it hatches as
  the eggs do now (the review sheets' "opened" egg was only a picture, R11b).
- Tests, scripted runs in Azahar, then **run 17** on the old 3DS (a new tab on the
  checklists page).
- **Done 2026-09-26 (D82, 0.2.4).** As built: kind, colouring, five stats, manner and up to
  three traits on every dragon (12 bytes more a record; older saves migrate as they load);
  eggs from pairs, the Market and the Wanderings roll kinds; the three commons as first eggs;
  the manner is the temperament; the profile, the Dragondex (36 entries now), the eggs, glows,
  names and voices follow the kind. Genetics v2's two breed alleles wait for DR4, where the
  crossbreeds need them; the old genome still gives the sex, a +-6% size and the favourite
  food. PC tests (227,051 checks) and Azahar runs `tests/autotest/dr3.txt` (an old save
  migrating), `starter.txt`, `hatch.txt`, `profile.txt`, `dex.txt`, `market.txt`.

## DR4 — The next crossbreeds: the commons' pairs and two more (D80, D83)
- With all eight bases built (D79), 27 crossbreeds are left (the Blazeplume, Pouncer ×
  Crestwing, is the first). Next, the three pairs of the common breeds, the crossbreeds
  players meet first: **Pouncer × Puffback, Pouncer × Curlstone, Puffback × Curlstone**, and
  (D83) **two common × harder-to-find pairs** chosen for bodies unlike any yet, so five in all,
  built alongside Beta's big update.
  Concept sheets (**R12**), then subagents in parallel, each designing a kind of its own (a
  body plan that suits it), sized on the common scale (`measure.py`, `together.py`), then
  **R12b**.

## DR5 — The rest of the crossbreeds
- The remaining crossbreeds in waves of six or so (**R13** on), each concept round before
  its models, alongside Beta (D78).

## Checks for every kind
Triangles and bones in budget; every clip on the list, grown and baby; parts seated on the
body in every pose (as the spine test does now); the textures' sizes; loads and plays in
Azahar with no unmapped memory access; the den at 60 fps with three dragons of the heaviest
kinds; a review sheet.

## Past the 36
New dragons after the 36 are **second-layer crossbreeds** (a kind from two particular
crossbreeds, a recipe), never more base breeds; not every first crossbreed gets one (D78).

## Questions (answered 2026-09-25, D78)
1. **Elements: A**, each base breed one element, a crossbreed both; one-word types ("Ember",
   "Gale", "Frost"), shown as two for a crossbreed. Elements are attached now and used more as
   the game grows, up to Pokémon-style battles much later.
2. **Colours: A**, the variant's colours with a small shift per dragon.
3. **Stats: A**, with functional names: Wing, Wit, Might, **Breath** (was Spark), **Stamina**
   (was Heart).
4. **Traits: A**, up to two (three on a rare variant), about 30 in four tiers.
5. **Manners: A**, ten, each nudging one stat up and one down.
6. **Rare variant: A**, about 1 in 20, better with a rare parent.
7. **Order: A**, the four bases, run 15, their six crossbreeds, then Beta, with the rest
   planned (DR5) and made in waves alongside it; later additions as a second layer.
8. **Reviews: A**, a sheet per kind to approve or note, then the game on the 3DS.
