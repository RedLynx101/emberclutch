# Alpha 1 — *A living pet*: Work Plan

Status: **Ready to start** (2026-09-23). Goal: one 3D dragon you raise from a starter egg
to a majestic adult in a 3D den, with care, feelings, naming, music, and real saves,
installed as a CIA. Scope and assets: [content & assets](content-and-assets.md) (A1 rows).
Screens: [screens & flow](../design/screens-and-flow.md) (A1 rows).

## Definition of done
All checked **in Azahar** (hardware deferred, D28):
1. New game → name → choose any of the 3 starter eggs → rub → hatch (cinematic) → name
   the dragon (swkbd with suggestion) → rename later works.
2. With the dev time skip, the dragon grows visibly **every day** and reaches **Adult**
   through all stages, with starter-breed parts and sex differences visible.
3. Care is hands-on, *Nintendogs*-style ([care interactions](../design/care-interactions.md)):
   the tool you hold is drawn where you touch (hand, brush, cloth, sponge, food), the
   dragon leans into petting and has a sweet spot, brushing fills shine regions you can
   see, the bath has suds and a shake-off, food is hand-fed into the opening jaw, and a
   thrown ball bounces around the den while the dragon fetches it and drops it at your
   feet. Every action has an animation and sound; plus sleep at night, naps, yawns.
4. Neglect → Upset → sulk nook → make-up (treat + pet) → heartglow re-lights.
5. Music: title → den day ↔ nestsong at night (crossfade); hatching stinger; seamless loops.
6. Save survives quit/restart and a simulated power cut mid-save (A/B slots, CRC).
7. Budget overlay stays green in the den and petting scenes (see WP1 limits).
8. The CIA installs and launches in Azahar with the original icon and banner.
9. `tools/test.ps1` green; `tools/build.ps1` has no warnings; docs, STATUS and RedWiki updated.

## Art review checkpoints (D26)
- **R1 — Sculpt:** turntable renders of hatchling, adolescent, adult (untextured), for
  each starter's parts. *Waits for Noah's OK before texturing.*
- **R2 — Textured:** the same renders with masks tinted for the 3 starters, male + female.
- **R3 — Rigged & animated:** short captures of the key clips in the emulator.

While waiting on a review, work continues on the non-art packages (engine, save, audio, UI).

## Work packages (in order; ⟂ = can run in parallel)

### WP1 — Engine foundation
- Split `src/app/main.cpp` into scenes (`scene_title`, `scene_den`, …) with a small scene
  stack; move drawing helpers to `ui_draw.*`; add `strings.hpp` (string table).
- Enable **romfs** in the Makefile; asset loader for `romfs:/`.
- citro3d init alongside citro2d (3D on top, 2D UI on bottom and overlays).
- **Debug overlay** (debug builds): FPS, CPU/GPU ms (`C3D_GetProcessingTime`,
  `C3D_GetDrawingTime`), triangles, draw calls, bones per draw, linear/VRAM use. Turns red
  above limits: 30 fps target, ≤ 8k tris/frame in the den, ≤ 40 draw calls, ≤ 24 bones/draw.
- Dev menu: time skip, set needs, force stage, reset.
- *Verify:* emulator shows the overlay; unit tests still pass.

### WP2 — Dragon model pipeline ⟂ WP1
- `tools/blender/dragon_model.py`: one organic mesh via skin modifier + subdivision over a
  spine/limb graph, shaped to the growth sheet; starter parts (swept/nub horns, fin/feather
  frills, membrane/fin/feathered wings, spade/fan/tuft tails, 3 builds); egg model.
- Rig ≤ 24 bones per draw (body and wings split if needed); growth bone-scale tables for 4
  stages + shape keys; sex bone-scale offsets (D23).
- Decimate/retopo to budget; UVs; procedural bake to the mask textures.
- **R1, then R2.**
- As built so far: both body forms approved (D36–D39); the egg model
  (`tools/blender/egg_model.py`, two bones, three crack stages, 948 triangles). Left:
  texturing → R2.
- **Cuteness pass** (Noah, 2026-09-24, after seeing the den in the emulator):
  - ✅ Folded wings tuck like a bird's (a Z: upper arm up, forearm down and forward, hand
    back), clear of the spine; solved from bone directions (`tools/blender/fold_solver.py`).
  - ✅ An animated mouth (D41): a jaw that opens for eating (it chomps on each bite),
    yawning, greeting, petting, the favourite-food wiggle, belly rubs, pounces and a
    little pant when trotting. Little teeth line both lips (two tiny fangs peek out when
    it's closed), with a pink tongue and a dark rosy mouth. Previews:
    `dragon_model.py -- --views mouth --jaw 28`.
  - ✅ Redo the **tail wag** (Noah): it pivoted on the hips (the root), swinging the whole
    dragon from its back end. Now the rear and front sway against each other around the
    middle of the body (the head's side-to-side travel went from 0.28 to 0.03 on an adult),
    the raised tail wags hard, the head tilts, the front paws do alternating tippy-taps
    and the hind legs counter the sway so the feet stay planted.
  - ✅ Fixed the **walking limp** (Noah, 2026-09-24): not the feet after all. Decimation
    left one loose vertex under the grown body (no faces, so never drawn), skinned to the
    left foreleg; it swung below the floor every stride and the floor contact lifted the
    whole dragon by ~0.27. Loose vertices are now removed; the walk's contact moves 0.025
    (was 0.295). PC tests guard both (no loose vertices; walk and trot stay level).
  - "A bit more cute" in general: ✅ blinking (D42): a blink every few seconds, eyes shut
    in sleep, a content squint while petted, a sleepy one while yawning, and a smile in
    the tail wag. Still open: softer expressions if R3 asks for them.
  - ✅ Texturing ([R2](../art/reviews/R2-textures.md), D51): a baked skin per body form
    (scale detail, occlusion, the Pattern gene's stripes / spots / dapple) and **visible
    dust** per body region that grooming cleans (D46). In-game check in Azahar pending
    (a full-screen overlay blocked screenshots on 2026-09-24).
- *Verify:* script prints tris/bones per part vs budget; renders saved to `docs/art/`.

### WP3 — Converter and formats ⟂ WP2 ✅
- As built: `tools/blender/export_dragon.py` writes `.ecm` directly from the Blender scene
  (no glTF step; growth tables, snapped part keys and variants have no glTF equivalent),
  one file per body form (D36). `.eca` (int16 quaternion tracks at 30 Hz + events) comes
  with WP5; textures via tex3ds after R2.
- *Verify:* PC parity test (the C++ rig reproduces Blender's deformation), triangle-budget
  test; `.ecm` sizes logged by the exporter.

### WP4 — Renderer ✅ (LOD1 triangle count to confirm in the emulator)
- `shaders/skinned.v.pica` (2 weights, ≤ 24 bones); mask-color TEV setup
  (architecture §4); toon LUT + rim; heartglow emissive with white-hot core and pulse;
  static-mesh path for the den and props.
- Camera: den overview, petting close-up (bottom screen renders the close-up directly).
- *Verify:* test scene with 3 dragons in the den stays in budget.

### WP5 — Animation and behavior ✅ (review R3 sent)
- Clip playback, crossfades, additive head look-at, event markers.
- Behavior state machine: wander, idle variants, sit/lie, nap, sleep at night, eat,
  react to pets, play, sulk in the nook, make-up, greet, look at the player.
- ~25 A1 clips authored in Blender scripts (content inventory §1.4). **R3.**
- *Verify:* every state reachable from the dev menu; no foot-sliding in walk/trot.

### WP6 — Den scene ⟂ WP4 ✅ (review R4 sent; emulator check queued)
- Den model (Blender script): cave room, sky opening, 2 nests, rug, hearth, hoard pile,
  shelves, sulk nook; vertex-color lighting sets for day / evening / night.
- Particles: embers, sparkles, hearts, crumbs, Zzz, dust.

### WP7 — Interactions (the heart of Alpha 1)
Spec: [care interactions](../design/care-interactions.md) (Noah, 2026-09-24: "more like
Nintendogs", a full polish of the at-home care). In order:
- **Touch to dragon:** stylus → ray through the close-up camera → capsules around the posed
  bones (head, cheeks, chin, neck, back, belly, tail, wings, heartglow); stroke speed,
  direction, scratch circles, pokes, roughness.
- **Tool tray and cursors:** hand, food, brush, cloth, sponge, ball; each drawn at the
  stylus and turned along the stroke.
- **Procedural layers** on the clips: lean toward the touch, jaw open by food distance
  (the look-at and eyelids exist).
- **Petting:** zone reactions, eyes easing shut, rising purr, the per-dragon sweet spot
  (leg kick), tickles, nose pokes, pulling away when rough, petting a sleeping dragon.
- **Brushing:** 8 shine regions filled by strokes with the grain, dust puffs on dirty
  regions, gloss that shows progress, the dragon turning, lifting a wing, sitting up;
  then the polish cloth and the "gleaming" moment.
- **Bath:** tub, suds from the sponge, the ladle rinse, the shake-off (element
  preferences).
- **Feeding:** hand-feeding into the jaw, one to three bites, favourites and dislikes.
- **Fetch:** `src/core` prop physics (PC-tested); flick to throw or drag to roll; the top
  screen follows; chase, pick up in the jaw, carry back, sit and drop at your feet;
  leap-catch for juveniles and up; personality variations; the ball stays in the den.
- **Calling:** tap and hold (or A) and the dragon comes to the front and sits.
- **New clips:** pick up, carry, drop and wait, leap-catch, lean-in, leg kick,
  sniff-refuse, lift wing, sit up, hop into the tub.
- **Egg:** rub (done), turn, listen; hatching cinematic; naming via swkbd; rename in the den
  (all done 2026-09-24, D52).
- *Verify:* in Azahar, every tool on every zone at every stage; a full fetch round trip
  in each personality; the budget overlay stays green while the ball is in play.

### WP8 — Save system v1 ⟂
- `src/core/save.*`: header (`EMBC`, version, CRC32, timestamp), records, migrations;
  alternate `save.a` / `save.b`; newest valid wins. Replaces the dev save.
- PC tests: round trip, corrupted slot falls back, old version migrates.
- `src/app/storage.*`: SD read/write with temp-file + rename.

### WP9 — Audio ⟂
- Ogg streaming thread (Tremor) with loop points from `loops.json`; music director (title,
  den day ↔ nestsong at night, crossfade 2 s); SFX voices with per-dragon pitch.
- Suno briefs: **music batch 2** (hatching stinger now; Market and Wanderings stinger for
  A2) and the **A1 sound-effect set** (content inventory §6.2) → Noah generates.
- `make_loop.py --no-loop` for stingers.

### WP10 — UI
- Bottom-screen HUD in the theme; fonts converted with mkbcfnt (Nunito, Cinzel Decorative,
  both OFL, license files added); title Continue/New; system menu; settings; toasts; save icon.

### WP11 — Packaging
- CIA build (makerom + bannertool, reusing the 3D-Claw pattern) as `tools/package_cia.ps1`;
  an **interim** 48×48 icon and HOME banner rendered from our own dragon model (replacing
  the AI-concept-derived placeholder), banner sound. The designed emblem comes in Alpha 2 (D48).

### WP12 — Tuning and wrap-up
- Emulator playthrough scripts (a checklist in `docs/plan/alpha-1-checklist.md`), balance
  check against the core tests, docs sync, tag `v0.1.0-alpha1`.

## Risks
| Risk | Mitigation |
|---|---|
| No hardware perf data until later (D28) | Conservative budgets + overlay counters; recommend a hardware check by end of A2 |
| Scripted models lack charm | R1/R2 reviews with Noah; fallback: image-to-3D base mesh (license-checked) or commissioned sculpt |
| 24-bone limit too tight for wings + tail | Split body/wing draws with separate bone sets (architecture §4) |
| Emulator GPU differs from hardware | Avoid exotic TEV/LUT tricks; stick to patterns in devkitPro gpu examples |
