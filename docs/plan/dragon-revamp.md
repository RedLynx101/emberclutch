# The Dragon Revamp: Work Plan (D76–D77)

Status: **settled, 2026-09-25** (D78: the questions below answered); ready to start with DR1. What
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

## DR1 — The pipeline and the engine's groundwork (me)
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

## DR2 — The four base breeds (four subagents in parallel)
- Each: its body plan (skeleton and every clip, grown and baby), its egg, baby and grown
  body through the stages, three painted variants and the rare variant's model, the checks
  passed, the renders.
- **Review R11b:** a sheet per kind (every stage and variant, a turntable, a strip of its
  key clips), then the four together and in the den.

## DR3 — Into the game (me)
- Genetics v2 (two breed alleles of eight; the variant; potentials and traits) and a new
  save version; **every dragon and egg in a save becomes a random kind in a common variant**,
  fixed by who it is, keeping everything else (D77).
- Stats, manners and traits in the core with PC tests; the profile shows them as you find
  them out; the Dragondex for 144 entries; the Market's eggs; sizes in the den (±50%).
- Tests, scripted runs in Azahar, then **run 15** on the old 3DS (a new tab on the
  checklists page).

## DR4 — The first six crossbreeds
- Pouncer × Puffback, Pouncer × Crestwing, Pouncer × Ribbontail, Puffback × Crestwing,
  Puffback × Ribbontail, Crestwing × Ribbontail: concept sheets (**R12**), then six
  subagents in parallel, each designing a kind of its own (a body plan that suits it), then
  **R12b**.

## DR5 — Base breeds five to eight, and their crossbreeds
- Concepts for four more bases (**R13**), built as in DR2 (**R13b**); then their 22
  crossbreeds in waves of six or so (**R14** on), each concept round before its models.

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
