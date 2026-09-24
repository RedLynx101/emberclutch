# Technical Architecture

Status: **v0.2** (2026-09-23)

## 1. Target and constraints

- **Performance floor: old 3DS / 2DS.** ARM11 MPCore at 268 MHz, PICA200 GPU,
  64 MB application memory when installed as a CIA, and 6 MB of VRAM.
- **Screens:** top 400×240 (stereoscopic optional), bottom 320×240 touch screen.
- **Formats:** `.3dsx` for development (Homebrew Launcher / `3dslink`), `.cia` for release (FBI).
- **Toolchain:** devkitPro through MSYS2 (`C:\msys64\opt\devkitpro`): devkitARM, libctru 2.7,
  citro3d 1.7, citro2d 1.7, picasso (shader assembler), tex3ds, mkbcfnt, 3dslink.
  CIA packaging uses `makerom` and `bannertool`, the same as in the 3D-Claw project.

### Frame budget

| Item | Budget |
|---|---|
| Target frame rate | **30 fps locked** in 3D scenes, 60 fps in menus |
| Skinned dragons on screen | ≤ 3 (den), 1 up close (petting, riding) |
| Dragon triangles | LOD0 ≤ 3,000 for the heaviest gene mix (grown 2,994, hatchling 2,778) · LOD1 ≤ 1,200 (1,123 / 1,047), both checked by a PC test. A full den draws the cared-for dragon at LOD0 and the others at LOD1 (~4,850 for three adults) |
| Bones per draw | ≤ 25 (the vertex shader's constants, see §4: 95 of 96 used since the jaw bone, D41) |
| Dragon colour | Per-vertex palette paint (no texture per variant); a shared scale-detail texture comes with texturing |
| Environment | Vertex-coloured. The den room is 2,251 triangles (a PC test holds it to 8,000 − 3,000 − 2 × 1,200 = 2,600, so a full den frame stays under 8k) |
| Audio | Music streamed from romfs, sound effects preloaded, ≤ 8 voices |

## 2. Layers

```
┌────────────────────────────────────────────────────────────┐
│ game/      scenes & state machine: Title, Den, Pet, Nest,  │
│            Sanctuary, Train, Arena, Valley/Ride, Market     │
├───────────────────────────┬────────────────────────────────┤
│ render/  citro3d + citro2d│ audio/  ndsp mixer, streaming   │
│ ui/      bottom-screen UI │ input/  touch, pad, mic, gyro   │
├───────────────────────────┴────────────────────────────────┤
│ platform/  libctru wrappers: clock, fs/save, pedometer, uds │
├────────────────────────────────────────────────────────────┤
│ core/      PURE C++17 — no libctru. Genetics, needs,       │
│            growth, mood, bond, time catch-up, den/storage,  │
│            save serialization. Unit-tested on the PC.       │
└────────────────────────────────────────────────────────────┘
```

**Rule:** `src/core` must compile with a desktop g++ (MSYS2 `ucrt64`) and has no
dependency on libctru, the clock, or files. Time is passed in as plain numbers. This keeps
the whole simulation deterministic and testable without hardware (`make -C tests`).

## 3. Simulation

- **Time source:** `platform::nowUnix()` (from the RTC via `osGetTime`). Local calendar day
  is used for care stars.
- **Catch-up:** on boot or resume, elapsed time is applied in ≤ 1-hour steps, capped at
  14 days. If time moved backwards, nothing is applied (see the GDD, §10).
- **Needs:** per-hour drain rates by stage and personality; sleep hours regenerate Energy.
- **Care stars:** each local day records the average of the minimum need over the hours
  the dragon was active, plus whether the player visited. That gives 0–3 stars.
- **Growth:** `stageFor(daysSinceHatch, careStars)` decides the stage; a continuous
  `growth01` within the stage drives bone scales and proportions.
- **Randomness:** one seeded PRNG (xorshift-style) per purpose (breeding, personality,
  finds) so bugs can be reproduced from a seed.

## 4. Rendering

### Skinned dragons

- **Two body forms** (D36), each a `.ecm` with the same 38-bone layout (24 body + jaw + eyes + 12 wing):
  `hatchling.ecm` for the hatchling stage and `grown.ecm` from juvenile to adult. The
  stage-up to juvenile swaps forms behind a glow (the first molt). `rig.hpp growthFor()`
  maps stage + in-stage progress to (form, growth t).
- Within a form, **builds** (Sturdy/Sleek/Long) and **growth** are per-bone scale tables
  blended on the CPU each frame.
- The PICA200 vertex shader has 96 float constant registers. A 3×4 bone matrix takes 3,
  so after the projection and model-view matrices there is room for about 28 bones per
  draw call. The body draw uses **25 bones** (the 24 of the body plus the jaw); the wings
  are a separate draw with their own bone set.
- Skinning happens in a picasso vertex shader (`src/app/dragon.v.pica`), with 2 bone
  weights per vertex (limited in Blender, so the parity test matches exactly). The shader
  reads bone rows by relative addressing (`a0` = bone index × 3); uniforms are projection
  (4) + model-view (4) + 25 bones (75) + palette (10) + 2 constants = 95 of 96.
- **The mouth** (D41): a `jaw` bone parented to the snout, parallel to it and scaled by
  the snout's growth and build tables, so the closed lips meet at every stage; clips open
  it by rotating it about a hinge behind the mouth corners. The snout is slit along the
  mouth line; the lower lip and chin are weighted to the jaw, fading back to the throat
  behind the corners. A dark pocket (roof and floor) fills the opening, and the teeth and
  tongue are a rigid part group.
- **Blinking** (D42): an `eyes` bone between the eyes, parallel to the head, parented to
  it and grown by its tables, carries the eye parts (no skin). The renderer squashes its
  vertical axis by up to 90% (`core/den_actor` `Eyelids`): a blink every 2–6 s (now and
  then a double one), shut while asleep or curling up to nap, a content 0.6 squint while
  petted and 0.5 while yawning.
- **LOD1** (`{form}_lod1.ecm`): the same skeleton, growth tables and part layout with
  fewer segments. A PC test checks it shares the LOD0 rig.
- **Pose math** (`src/core/skeleton.cpp`) copies Blender's rule for bones with scale
  inheritance off: a child's joint follows the parent's full, scaled matrix, but its
  orientation ignores the parent's scale.
- Parts (eyes, horns, frill, dorsal ridge, tail tip, heartglow, teeth and tongue) are rigid meshes attached to
  fixed bones, re-baked into one per-dragon buffer when growth changes. Only the genome's
  variants (and the dragon's sex, D23) are drawn; the ridge variant follows the Frill gene
  (D38), falling back to spikes. Three draws per dragon: body, parts, wings (the wing draw
  also uses chest/belly/hips so the membrane's flank edge follows the body).

### Animation (as built in WP5)

- **Clips** (`tools/anim/clips.py` → `romfs/anims/dragon.eca`, 32 clips, ~174 KB) store,
  per bone and 30 Hz frame, a rotation **delta** applied on top of the dragon's idle pose.
  Deltas are authored as pitch / yaw / roll about **armature axes** (they read the same
  for every bone), and `bindAnims` turns them into each bone's local frame with that
  form's rest rotation, so one clip set serves both body forms and every growth stage.
  A clip named `<name>_h` replaces `<name>` on the hatchling (a baby's big head nods
  instead of bending its neck to eat).
- Optional **root track** (lift, forward) for hops. Horizontal travel (walking, the
  pounce's leap) moves the dragon itself, never the mesh.
- **Floor contact** is recomputed every frame from the posed body (every third vertex,
  smoothed), so sitting, lying and rolling over settle onto the floor without authored
  root motion.
- **Animator** (`core/anim`): one clip plus a crossfade, event markers (footstep, chomp,
  purr, thump, flap, yawn) that play sounds. **Look-at**: the neck and head turn toward the
  den camera within limits, weighted by activity.
- **Behavior** (`core/behavior`): a state machine of activities: everyday life weighted
  by mood, personality and energy; naps and night sleep at the nest; sulking in the nook
  until made up; care reactions by petting zone. `core/den_actor` steps behavior and
  animation together. Walk/trot speeds are **measured from each body's stride**
  (`locomotionSpeed`), so feet never skate.
- `tools/blender/preview_anims.py` renders clips on the Blender rig with the same math
  (review R3). The den's spots (nest, nook, rug, hearth, hoard) and its solid obstacles live
  in `DenLayout`; dragons pick wander targets clear of the obstacles, steer around one in
  the way, and are pushed out if a leap lands them in one. Each den dragon has its own bed
  and sulking spot (`DenLayout::beds`, `sulkSpots`), and `shareCrowd` makes the others
  obstacles too (a dragon lying down or eating stays put; the moving ones make way).

### Coloring (no texture per variant) — as built in WP3/WP4

Every vertex carries **paint**: two palette slots, a mix amount and an emissive weight
(`src/core/model.hpp`, `Palette`). Each dragon uploads its own palette (base, accent,
pattern, horn, membrane, iris, pupil, glint, heartglow) as vertex-shader uniforms; the
shader mixes the two slots, so the belly/throat accent and every colour variant cost no
texture memory and no UVs. The accent weight comes from the model's painted mask.

Fragment lighting (`src/app/render3d.cpp`): primary = a plum-tinted ambient; secondary
= specular 0 through lookup table D0 on L.N (the stepped **toon ramp**); secondary alpha =
the Fresnel table on N.V (a thin **rim**). One directional light, fixed in view space.

**Skin texture and dust (R2, D46, D51).** Each body form has one baked RGBA texture
(`romfs/models/<form>[_lod1]_skin.t3x`, 256² / 128² with mipmaps, from
`tools/blender/dragon_texture.py`): R stripes, G spots, B dapple (3D procedurals baked
through automatic UVs, so they run across seams), A scale detail × occlusion. Parts, wings
and the egg sit on the texture's clean corner. Dust is per dragon: each body vertex has a
region, and the renderer keeps a per-dragon stream of 4-byte dust levels (rebuilt only when
the dirt changes) as vertex attribute 5; wings and parts use a fixed attribute value. The
shader passes the level as texture coordinate 1 into a 256-texel ramp, so the combiner
can read it.

TEV (six stages): `albedo = lerp(vertex colour, pattern colour, skin[pattern channel])`
→ `lerp(albedo, dust colour, dust)` → `× skin.a` → `× (primary + secondary)`, alpha = rim
→ `+ vertex colour × emissive` → `+ rim colour × rim`. The heartglow is emissive with a
white-hot core (the exporter paints a radial glint→glow gradient) and pulses with mood.

### Mixing citro3d with citro2d (as built in WP4)

- Screens stay citro2d scenes. The 3D pass runs inside one: `C2D_Flush()`, bind the dragon
  program, draw, then `C2D_Prepare()` to hand the GPU back.
- citro2d is set to depth test `ALWAYS` with colour-only writes (`r3d::prepare2D`), so 2D
  never writes depth: the cleared depth buffer is free for the dragons (`GREATER`), and
  2D drawn after them (text, toasts, the overlay) always lands on top.
- Den camera: in front and a little left, aimed so the feet land on the rug. Its size
  blends each dragon's framing (80%) with the adult's (20%): babies read smaller than
  adults but still fill the screen. The bottom screen draws a head-and-chest close-up
  under the pet pad (the petting view WP7 builds on).
- The den room draws first with its own program (see below); the dragons follow in the
  same pass.

### The den room (as built in WP6)

- **A cutaway diorama** (`tools/blender/den_model.py` → `romfs/models/den.esm`, D40): a
  round cave (radius 9.5) whose walls go all the way round and face inward. The den camera
  (always from the front-left, following the dragons) is often outside the wall; back-face
  culling hides the near side, and the floor runs on past the walls, fading into the
  backdrop colour, so the frame never shows an edge. Solid props stand in the back two
  thirds, where they cannot hide a dragon. A PC test checks the room against `DenLayout`.
- **Baked light, three sets.** Every vertex stores its colour for day, evening and night
  (ambient + sky fill + the pool under the skylight + the hearth, with a little occlusion).
  `core/daylight` picks two sets and a blend from the clock (dawn 05:30–08:00, dusk
  17:30–21:30); `static.v.pica` mixes them per vertex (no lighting on the GPU). Each set's
  colour array is its own linear buffer, so the blend is just two buffer pointers and a
  uniform.
- **Draw order:** room (one draw) → ambient particles (2D) → dragons → the additive glows
  (sunbeam and flames: camera-facing ribbons, depth-tested without depth writes; the flames
  flicker through a tint uniform) → care particles (2D). Three extra draws in all.
- **Dragons follow the light:** the material ambient and key colour come from
  `dragonLight(blend)` (midday = the look the dragons were designed in), scaled per channel
  by the room's floor light where the dragon stands relative to the rug's
  (`StaticScene::lightNear`): darker in the nook, warmer by the hearth, brighter in the sun.
- **The egg** (`romfs/models/egg.ecm`, `tools/blender/egg_model.py`, `core/egg`): the
  dragons' format with two bones, the shell and the cap that pops off along a zigzag seam.
  It is drawn with the dragon program; its motion is procedural (rocking about a pivot in
  the round bottom when rubbed or knocked from inside, the cap tipping back on a hinge).
  Colours are palette slots set per egg: a breed-tinted shell, speckles, the inside of
  the shell, the light inside and three crack stages. The dragon shader multiplies each
  vertex's glow by its palette slot's alpha, so a crack stays shell-coloured and dark until
  its stage, then glows, and the inner light brightens with warmth (dragons keep alpha 1).
  In the den the egg sits in the egg nest; the bottom screen shows it up close to rub.

Rare traits change the palette constants or add a lookup table (Iridescent).

### Environment and effects

- Vertex-colored meshes with baked lighting. Gradient sky dome, linear fog.
- Skyreach Valley: height-field chunks (32×32 quads), about 9 visible, culled against the
  view, with fog hiding the edge.
- Particles (as built in WP6): `core/particles` simulates a fixed pool of 96 in den space
  (embers, sunbeam motes, hoard glints, hearts, Zzz, crumbs, sparkles, dust puffs); the
  renderer projects them with the den camera and draws them with citro2d, ambient ones
  behind the dragons and care effects over them. When the pool is full, care effects
  replace ambient ones.
- Stereoscopic 3D: optional second eye render. Can be switched off per scene if it
  doesn't fit the budget.

### Bottom screen

citro2d for all UI: eggshell panels, ember gauges, heartglow orb, pouch, map. In petting
mode the bottom screen renders a close-up of the dragon directly (a second 3D pass, one
dragon only, so it fits the budget) with body-zone hitboxes projected from its bones.

## 5. Assets pipeline

```
tools/blender/dragon_model.py  (two forms: metaball hatchling + skin-modifier grown body, rig, parts, growth)
        |  imported by
tools/blender/export_dragon.py --out-dir romfs/models --reference-dir tests/data
        -> romfs/models/{hatchling,grown}.ecm + tests/data/{hatchling,grown}_reference.ecr
tools/blender/den_model.py --out romfs/models/den.esm   (the den room, baked lighting sets)
tools/blender/egg_model.py --out romfs/models/egg.ecm   (the egg: plain Python; Blender only for previews)
tools/blender/sheet.py          (tiles review renders into docs/art/reviews/*.png)
Suno WAV --tools/audio/make_loop.py (ffmpeg)--> romfs/music/*.ogg (LOOPSTART/LOOPLENGTH tags)
generated WAVs --tools/audio/process_sfx.py (ffmpeg; sfx_manifest.json)--> romfs/sfx/*.wav
```

- **Blender runs headless** (`blender -b -P ...`). Everything is a version-controlled
  script, so the model can be regenerated and reviewed.
- **Why not glTF:** the growth tables, the per-stage surface-snapped part offsets, part
  variants and sex differences live in the Blender scene and have no glTF equivalent, so
  the exporter writes the game format directly.
- **`.ecm` v3** (little-endian, read by `src/core/model.cpp`; v2 widened the bone palette
  field to 32 bytes, of which at most 25 are used; v3 added per-vertex skin UVs and body
  regions after the paint):
  header `ECM1`, version, bone count; bones (name, parent, flags, 3x4 rest matrix);
  growth tables (bone scales at the form's t = 0, build multipliers, idle pose Euler XYZ,
  young head lift); meshes (name, kind body/wings/part, group, variant, sex, bone palette,
  growth keys, then per key positions + normals, per vertex 2 bones + 2 weights and
  paint, and u16 triangle indices). Parts are baked at growth t = 0, .35, .7, 1 in
  armature rest space and bound rigidly to one bone; the runtime blends two keys.
- The hatchling is modelled at a comfortable scale and written scaled by its
  `export_scale` (uniform scaling commutes with skinning, so parity is unaffected).
- **Parity test:** the exporter also writes Blender-deformed vertex positions for three
  poses/growth/build cases per form; `tests/test_model.cpp` checks the C++ rig
  (`src/core/skeleton.cpp`, `rig.cpp`) reproduces them (body < 0.001, wings < 0.006,
  parts exact, on a ~6-unit adult).
- **`.esm` v1** (`tools/blender/den_model.py` → `src/core/static_mesh.cpp`): `ESM1`,
  version, part count, lighting-set count; a backdrop colour per set; per part a name,
  flags (additive, flicker), float3 positions, RGBA8 colours per set and u16 indices.
  Opaque parts come first so the room is one draw.
- **`.eca` v1** (`tools/anim/eca.py` → `src/core/anim.cpp`): bone names; per clip name,
  fps, frames, loop/root flags, locomotion speed; per bone a mode (none / constant /
  animated) and int16 quaternions; optional root track; event markers. Built by
  `python tools/anim/build_anims.py` (no Blender needed).
- Everything ships in **romfs**. Music is streamed and never loaded whole.

## 6. Audio

- **ndsp** (proven in 3D-Claw and asteria-ds). Channels 0–1 for music (loop, stinger),
  2–9 for sound effects (a free channel first, else the oldest), 10–12 for looping beds.
- **Music:** Ogg Vorbis via Tremor (the `3ds-libvorbisidec` package, installed
  2026-09-23) decoded on a worker thread, with sample-accurate loops from the
  `LOOPSTART`/`LOOPLENGTH` tags.
- **Sound effects:** 22 kHz mono PCM16 WAVs (`romfs/sfx/<slug>.wav`, takes `<slug>-2..4`)
  preloaded into linear memory (~3 MB); a sound's takes play in turn. Made by
  `tools/audio/process_sfx.py` from the generated sources: EQ for the small speakers,
  silence trimmed, levelled per kind (voices, body, egg, care, interface), a soft limiter
  on sharp sounds.
- **Beds:** the hearth, the night outside and an egg's hum are seamless loops; the den
  sets their levels every frame (the night bed follows the daylight blend, the hum the
  egg's warmth), they ease in and out, and a bed's channel runs only while it is heard.
- **Short wave buffers only.** Preloaded sounds are queued as slices of ~4,096 frames
  (beds as a ring of six refilled as each finishes; stingers and longer effects as a run
  of slices). Azahar's HLE DSP decodes each wave buffer whole into a vector and erases
  the played samples from its front every audio frame, so one long buffer costs the
  emulator time quadratic in its length: two 15 s beds as single buffers dropped the den
  from 100% to ~55% speed. The 3DS doesn't care; this keeps the emulator honest.
- **Dragon voices:** a small set of base samples (chirp, trill, purr, squeak, whimper,
  yawn, sneeze, rumble) pitch-shifted per dragon: up for babies, down for grown-ups, a
  little by size. Clip events trigger most of them (`call` is a trill from the young and a
  rumble from adolescents and adults).

## 7. Input

- **Touch:** petting zones, drag-and-drop (food and toys), flick gestures (throws).
- **Pad:** Circle Pad plus buttons for riding and menus.
- **Mic:** MICU at 16 kHz. Voice commands use on-device template matching: MFCC features
  plus dynamic time warping against 3 recordings per command. Runs on a worker thread.
  **Cue buttons are always available** as an alternative.
- **Gyro/accelerometer:** optional look-around while riding, shaking the treat bag.
- **Pedometer:** `PTMU_GetStepHistory` / `PTMU_GetTotalStepCount` for Wanderings.

## 8. Save data

Implemented in `src/core/save.*` (format, PC-tested) and `src/app/storage.*` (SD card).

- Location: `sdmc:/3ds/emberclutch/save.a` and `save.b`, written alternately. On load the
  valid slot with the higher sequence number wins; if it fails to decode, the other slot
  is used. An interrupted write can only ever damage the older copy.
- Format: a 32-byte header (`"EMBC"`, version, flags, sequence, payload size, CRC32,
  saved-at time) followed by the payload: player section, settings section, dragon
  count, dragon records. Every field is written explicitly in little-endian order (never a
  raw struct dump), and every section and dragon record carries its byte size. Newer
  builds can add fields (older records get defaults), and unknown trailing fields are
  skipped.
- Validation: CRC, bounds, and every enum/element checked; bad data is rejected, never
  loaded half-way.
- Capacity: 200 creatures at 134 bytes each; a full save is ~27 KB.
- Versioned migrations go in `decodeSave` (`if (info.version < N) ...`). Old saves are
  always upgraded, never rejected; a save from a *newer* build is refused, not damaged.
- The pre-WP8 dev save (`dev-save.bin`) is imported once if no slots exist.

## 9. Multiplayer (later)

UDS local wireless ("Sky Visits"), based on the `3ds-linkplay` uds-demo. The host den is
authoritative; the guest sends inputs; genomes are exchanged for cross-den clutches.

## 10. Build and deploy

| Task | Command |
|---|---|
| Build `.3dsx` | `tools/build.ps1` (runs `make` in MSYS2 with the devkitPro environment) |
| PC unit tests | `tools/test.ps1` (runs `make -C tests` with the ucrt64 g++) |
| Push + run over Wi-Fi | `tools/run.ps1 -Address <3ds-ip>` (`3dslink`, Homebrew Launcher netloader: press **Y**) |
| Upload files over Wi-Fi | `tools/deploy_ftp.ps1 -FtpHost <3ds-ip>` (ftpd on port 5000) |
| Run in the emulator | `tools/emu.ps1` (Azahar; `-ResetSave`, `-NoBuild`) |
| Package CIA | Alpha 1 (WP11): `tools/package_cia.ps1`, adapting 3D-Claw's (`makerom` + `bannertool`) |


## 11. Testing and development process

- **Unit tests (PC):** genetics distributions, stage gates, needs catch-up, clock
  rollback, breeding rules, save round-trips and migrations, model-converter round trips.
- **Emulator-first (D28):** day-to-day checks run in Azahar. It shows logic, UI and
  rendering correctness, but its speed says nothing about an old 3DS.
- **Budget counters instead of hardware timing (until the hardware check):** the debug
  overlay counts what we control and what drives old-3DS cost: triangles and draw calls
  per frame, bones per draw, particles, texture memory, linear/VRAM use, command-buffer
  use (`C3D_GetCmdBufUsage`), plus `C3D_GetProcessingTime` / `C3D_GetDrawingTime`. Any
  counter over its limit turns red. Limits: §1.
- **Hardware checks (old 3DS), when Noah is ready (recommended by end of Alpha 2):**
  frame-time overlay, memory high-water mark, a stress scene with 3 dragons plus particles.
  **Performance sign-off is on real hardware only.**
- **Stay close to known-good GPU patterns** (devkitPro `3ds-examples/graphics/gpu`) so
  the emulator and the hardware don't diverge.

## 12. Text

All player-facing strings live in one table (`src/app/strings.hpp`). v1 is English only
(D30); a translation later means adding a table, not touching code.
