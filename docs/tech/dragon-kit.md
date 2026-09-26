# The Dragon Kit: Building a Kind of Dragon (D76–D78)

How a new kind of dragon is made, from its concept sheet to the game's files, and what "done"
means. Written for whoever builds a kind (a person or an agent). The design is in
[dragons, version 2](../design/dragons-v2.md); the plan in [the revamp](../plan/dragon-revamp.md);
the look in [look and feel](../design/look-and-feel.md).

## What a kind is
A kind is two files of plain Python and what they produce:

| File | What it holds |
|---|---|
| `tools/dragons/kinds/<kind>.py` | who it is (META), its four colourings (VARIANTS), its egg (EGG), its two body forms (FORMS: the baby and the grown body) and hooks that build its parts, wings, markings and texture |
| `tools/dragons/plans/<plan>.py` | its body plan: the skeleton (BONES, WING_CHAIN, ...) and its full set of animation clips. Each base breed has its own plan; a crossbreed uses the plan that suits it |

The contracts are in the docstrings of `tools/dragons/kinds/__init__.py` and
`tools/dragons/plans/__init__.py`. **The Pouncer is the worked example**
(`kinds/pouncer.py`, `plans/pouncer.py`); the classic dragon's `tools/blender/dragon_model.py`
(GROWN, HATCH, BREEDS) shows more of the same machinery in use.

## The commands
Blender: `"C:\Program Files (x86)\Steam\steamapps\common\Blender\blender.exe"` (5.2), always
headless (`-b`) with **absolute** output paths. From the repo root:

```
# quick looks while shaping (stages: newborn, hatchling, juvenile, adolescent, adult;
# views: three_quarter, side, front, back, top, portrait)
blender -b -P tools/blender/dragonkit/model.py -- --kind <kind> --stages hatchling,adult --views three_quarter,side --out C:/abs/build/kit/<kind>/try
blender -b -P tools/blender/dragonkit/model.py -- --kind <kind> --lineup --texture --out C:/abs/build/kit/<kind>/try

# the clips (pure Python) and the game files
python tools/anim/build_anims.py --plan <plan>
blender -b -P tools/blender/dragonkit/export.py -- --kind <kind> --reference-dir C:/abs/tests/data/kinds
python tools/blender/egg_model.py --kind <kind>
python tools/blender/egg_model.py --kind <kind> --lod 1

# the checks and the review sheets
python tools/dragons/check.py <kind>
blender -b -P tools/blender/dragonkit/review.py -- --kind <kind> --out C:/abs/build/review/<kind> [--quick] [--only lineup,variants]
blender -b -P tools/blender/dragonkit/measure.py -- [--kinds <kind>,pouncer]      # size vs the Pouncer, export_scale wanted
blender -b -P tools/blender/dragonkit/together.py -- --out C:/abs/build/review/together.png   # every kind at true size
```
Look at the renders (they are PNGs: open them) after every change. `review.py` writes
`lineup.png`, `eggs.png`, `variants.png`, `turntable.png`, `portraits.png`, `clips.png` and
`babyclips.png`, posed as the game shows them (the idle clip on, wings folded, textured).

## How the pieces fit
- **Two forms.** The hatchling is its own baby body (usually metaballs: soft and round); the
  grown form (usually a skin-modifier graph of nodes and radii) runs from juvenile to
  adult. Within a form, growth is bone scales: the form's `young.bones` table is the scale
  at the start of the form (t = 0), lerping to 1 (adult) by t = 1. `STAGE` in model.py maps
  review stages to (form, t). The two forms share the plan's skeleton (same bone names).
- **The skeleton** comes from the plan's BONES (node names from the form's `nodes`) and
  WING_CHAIN (points from the form's wing `layout`). The jaw and eyes bones are added by the
  kit (`jaw_hinge`, `eyes.at`).
- **The body** is bound by heat weights (two bones per vertex, the shader's limit); the
  mouth is slit and opened on the jaw; nostrils and the mouth line are added; UVs and dirt
  regions are made. The accent mask (`accent` hook or the form's `mask`) marks where the
  accent colour goes (belly, muzzle, socks).
- **Parts** (horns, ears, frills, crests, plates, fins, tail tips, glowing marks) are rigid
  meshes on one bone each, seated on the skin at every growth key (a ray from the bone's
  joint through the part, stopping at the outermost skin, less the group's `inset`). Build
  them with the kit's helpers: `horn_mesh`, `blade`, `lobed_fin`, `flat_fan`, `blob`,
  `tube`, `add_dome`, `build_horns`, `build_ridge` (spikes, fin, feather, plates, tufts),
  `glow_marks`, `build_frill`, `build_tail_tip`; place them with `head_point`, `node`,
  `mirror`. Groups: eyes, horns, frill, spikes, tail_tip, heart, mouth, runes. Variant 0
  parts are on everyone; variant 1 parts only on the rare variant (META `rare_replaces`:
  whether a rare group replaces the common one or adds to it).
- **Wings** are the second skinned draw: bony arms (`wing_arm` or the classic
  `build_wings`) and membranes, feathers or fins weighted to the nearest two struts (the
  plan's wing bones and WING_BODY). Name objects `wingarm_<L|R>` (arms) or
  `<anything>_<L|R>...` (surfaces).
- **The texture** is baked per form: R, G, B pattern masks from the `texture` hook and a
  painted value (soft light from above, a faint brush grain, occlusion). Each variant says
  which channel shows in its pattern colour, and the rare one which channel glows.
- **Eyes** come in two pupils: round (calm, happy) and slit (startled, cross); the game
  swaps them by mood. **The heartglow** is on every dragon's chest; keep it unoccluded.

## Budgets and rules (the checker enforces them)
- At most **40 bones**; the body draw (every non-wing bone but `eyes`) at most **25**; the
  wing draw (wing bones + WING_BODY) at most 25; all parts in one draw at most 24 bones.
- Triangles, common and rare variant: **LOD0 3,000** (grown and hatchling), **LOD1 1,200**.
  Aim for the body about 1,600-1,900, wings 300-500, parts the rest.
- The names the game looks up: `head`, `snout`, `jaw`, `eyes`, `chest` in every plan;
  `neck2`, `neck3` if it has a neck (the look-at); tail bones start with `tail`, wing bones
  with `wing`, limbs with `arm`, `leg`, `hand`, `foot`. CONTACTS name the four bones that
  meet the ground (a wyvern's front pair can be wing bones).
- Every plan has **every clip** in `tools/dragons/clip_names.py` (grown; a baby version
  `<name>_h` is optional), with the same meaning, made for that body. Starting from the
  classic clips (`clipkit.classic()`) and reshaping them is fine where the body is close;
  a body unlike it (a serpent, a wyvern, four wings) needs its own versions of the clips
  that move differently (walk, run, sit, lie, sleep, curl up, fly, fold the wings...).
  Loops must loop cleanly; root tracks are in adult units.
- Sizes: model freely, then set the grown form's `export_scale` so the model comes out at
  the **common scale**: the Pouncer's bulk and length (`tools/blender/dragonkit/measure.py`
  prints each kind's measure, the geometric mean of the cube root of the body's volume and
  its length, and the `export_scale` it wants). The kind's META `size` (0.67 to 1.5 of the
  Pouncer) then sizes it in the game, so never bake the size into the model. Length alone
  misleads: a round Puffback 6 units long is more than twice a Pouncer's bulk. Hatchlings
  all come out about the same size and keep `export_scale` 1. `together.py` renders every
  kind side by side at true size to check.
- Don't copy anything from the concept images into the game: they are reference only (D74).

## The look (what Noah asked for)
Storybook, in the way of a cozy life-sim, crossed with a dragon film's lovable dragons,
with a touch of awe and majesty in the grown adults (D75–D77):
- **Silhouette first.** Each kind reads from its outline alone, and unlike every other kind.
- **Babies are round and big-headed** (about a third of their length is head), huge eyes,
  stubby limbs, tiny wing buds; soft and squashy. **Adults are cool, majestic, graceful**,
  still friendly: kind eyes, calm brows, a proud pose; never menacing.
- **Big glossy eyes** with a clear glint; the pupils change with mood.
- **Chunky, soft shapes**: rounded edges, no spiky clutter, few but bold features.
- **Hand-painted colour**: two or three colours per variant (base, belly accent, pattern),
  soft patterns (stripes, spots, patches, bands), gentle shading; no fine detail.
- **The variants**: three common colourings that each look lovely (natural, a surprise, a
  subtle one), and one rare variant that is special: its own colours, a glowing pattern
  and a slightly different model (a bigger crest, extra fins, glowing marks, new horns).

## Done means
1. `python tools/anim/build_anims.py --plan <plan>` and the export and eggs run cleanly.
2. `python tools/dragons/check.py <kind>` says OK.
3. The review sheets look right from every angle and at every stage: nothing floating or
   sunk, no holes, no parts through the body in any clip, wings folding neatly, the face
   cute, the adult majestic, matching the concept's spirit.
4. The kind file and the plan read clearly (a short docstring saying what the kind is).

## Gotchas (learned building the first nine, DR2)
Things that tripped the builders; the kit may fix some later, until then work with them:
- **EGG colours are display (sRGB) values**; VARIANTS colours are linear. Pick egg colours
  as you'd see them in a paint program.
- **WING_BODY must include the parent of the first wing bone** (usually the chest), or
  `weight_membrane` divides by zero; a kind with no body-joined membrane still needs one.
- **`base_pose` values are always (x, y, z) tuples** in degrees, never a bare number, and
  they are **bone-local** Euler angles (each pose bone's own XYZ): on an upright neck bone +X
  tips it forward, on the head +X is nose-down. Clip keys are different: deltas on top of
  the idle pose in the **armature's** axes (pitch, yaw, roll), the convention `fold.py` prints.
- **No fangs:** `fangs=[]` in a form turns them off (the storybook look has none).
- **Two groups with the same (group, variant)** are merged by the exporter; the game finds a
  part group by name, so one mesh per group and variant is what it sees.
- **Contacts are the plan's** (`CONTACTS`: the four bones that touch the floor in a walk);
  a wyvern's are its wing thumbs, a serpent's its lower legs. Every `tail*` bone is kept off
  the floor contact, so a long tail may drag without lifting the body.
- **Care zones:** bones named `ear*` and `antenna*` count as the head for stroking.
- **Known, not fixed yet** (work around them): `snap_parts` takes the outermost hit when a
  ray leaves and re-enters the body (seat parts on convex spots, or nudge them by hand), and
  a ray exactly on the x = 0 plane can slip through the midline seam and leave a part (a
  heart) unseated without a warning (author midline parts 2 mm off the midline);
  `glow_marks` faces a mark away from its bone's node, which tilts limb marks into the skin
  (pass the joint's node);
  `weight_membrane` has one strut list for all membranes (a four-winged kind weights each
  pair itself); `wing_arm` branches its fingers only from a point named `wrist`; the UV
  unwrap can leave tiny islands on thin parts (give them a flat material); bones used only
  by parts still get skin weights (keep them away from the body); there's no hook after
  decimation; `review.py`'s clip strips are a fixed list; `glow_flat` previews flat in
  Blender but glows in the game.
- **Check an open mouth from the side, culled (run 17).** The mouth pocket's roof and floor
  meet at a back line, but a wide open mouth seen from the side looked straight through it
  (in at one side, out at the other) until `build_mouth_pocket` grew a dark wall down each side
  from the upper lip to the lower (the lips' outer 45% across). `build/tools/mouth_look.py
  --jaw 45 --views mouth,mouth_top,mouth_side --cull 1` renders it the way the game culls.
- **A baby's folded wings can sink into a round body (run 17).** The plan's fold suits the
  grown body; a chubby hatchling's flank bulges past its little wings. Turn them out with the
  hatchling form's `base_pose` (the Pouncer's and the Blazeplume's: `wing_arm_R` (-20, 0, -55),
  mirrored for L; the third angle, negative for the right wing, lifts it off the body), and
  look from behind and above, culled (`build/tools/wing_look.py --close 1`, `wing_try.py`).
