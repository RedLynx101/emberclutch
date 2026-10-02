# The HOME Menu banner labs (D85 (6))

The game's 3D banner is **X** (lab 8, run 12, D71): the classic hatchling in its egg, ten gold
sparkles, the wordmark and the hidden triangle, still against the HOME Menu's turn. The
Blazeplume's banner froze the HOME Menu (run 15, D81), and so did **lab A**, the same banner cut
to X's size (runs 16-17, D83). This batch of four (**B, C, D, E**) takes X and changes one thing
each toward lab A, so whichever freezes says what to fix before the banner can show a new kind.

## What was already known
- No skinning or bone weights (`tools/banner_cgfx.py` refuses them); at most three levels deep
  (V and W, four deep, froze: D70); no animated billboards, nor billboards over animated nodes
  (S, U: D69); one mesh more than the dragon, egg and wordmark (the hidden triangle: D63).
- The sound: stereo 16-bit PCM at 32 or 44.1 kHz, at most 3 s (D68). The CGFX under 512 KB.
- Z froze (a childless egg turning whole turns every 4 s) and nobody knows why (D71).
- Lab A froze at 14 materials and 32 pieces (X has 16 and 32): the size isn't it (D83).

## How a lab is made
`tools/blender/banner3d.py` builds the scene and writes a glTF; `tools/banner_cgfx.py` converts
it with pycgfx (plus the unlit materials and the counter-turn, `--turn "body*:1,egg:1"`); and
`tools/banner_lab.ps1` packs each CGFX with a sound into a title of its own ("Banner lab B", no
romfs) and checks every CIA with `tools/check_3ds.py`. Two tools came with this batch:
- `tools/banner_graft.py`: X plus one part of another banner, glTF to glTF (names, geometry,
  tail, paint; joined with commas for later rounds; `none` writes X back as a check).
- `tools/banner_diff.py`: converts two glTFs as `banner_cgfx.py` does and lists every field of
  pycgfx's object tree that differs. Each lab was checked with it to be X plus its one part.

The sources are kept as tested: `assets/banner3d/x/` is X's glTF (it converts byte for byte to
the CGFX in the game and in lab X's CIA), `assets/banner3d/lab-a/` lab A's (byte for byte to the
CGFX in lab A's CIA). Blender's wordmark render changes a few pixels a run, so these aren't
rebuilt from Blender.

## X against lab A
Converted by pycgfx the two are the same model once lab A's names are matched to X's: the same
19 nodes and nesting, the same meshes per node, vertex layouts, animated channels and key times.
Everything that differs:

| | X (holds) | lab A (froze) |
|---|---|---|
| Nodes and depth | 19 + the scene root; body > head > eyes and cap, three deep; egg > sparkles and wordmark | the same |
| Node names | `body.001`, `heart.001` | `body`, `heart` |
| Pieces (meshes) | 32: anchor 1, cap 2, eyes 3, head 6, heart 1, tail 2, body 4, sparkles 10, wordmark 1, egg 2 | 32, the same per node |
| Materials | 16 | 14: no mouth, teeth or tongue (the mouth is shut); an accent colour |
| Material names | none with a dot | `b_horn.001`, `b_membrane.001`, `b_accent_flat.001` |
| Longest string in the CGFX | 72 characters (a material animation path) | 74 (`Materials["b_accent_flat.001"].FragmentOperation.BlendOperation.BlendColor`) |
| Dragon vertices (one side) | body 561, head 346, tail 99, eyes 172, heart 20 | body 387, head 319, tail 92, eyes 172, heart 20 |
| Dragon triangles | body 584, head 292, tail 111, eyes 244, heart 16 | body 399, head 258, tail 71, eyes 244, heart 16 |
| All vertices / indices (two-sided, doubled) | 4,134 / 16,620 | 3,718 / 15,066 |
| Egg and cap | | the same shapes (to 1e-6), their triangles listed in another order (the egg's inside its vertices too) |
| Vertex layout | position, normal (+ UV on the skin and flat-coloured parts), 32-bit floats, 16-bit indices, all two-sided | the same |
| Skinning | none | none |
| Textures | skin 128x128 and wordmark 256x64, both RGBA4 | the same sizes and format; the Blazeplume's skin |
| Animation | 241 linear keys a channel over 10 s: the body's bob and turn, the egg's turn, head tilt, blinks, heartbeat, tail wag, ten glints, the heart's glow | the same channels and keys but for three: the **tail** (lifted 55 degrees and swung 35 to its left, the wag about the lifted axis: in the CGFX a constant 0.96 rad on its X rotation where X has 0), the heart's glow colours, and the body's bob starting 2.5 units lower (its pivot) |
| Pivots | | the body, head, eyes, heart, tail and cap hang from the Blazeplume's joints |
| CGFX | 375 KB | 354 KB |
| Title tested | 0xEC141 (lab X), then the game's 0xEC0C2 | 0xEC111, **lab P's ID again** (see below) |

## The four labs
Each is X plus one part of lab A; the four together make lab A but for where its pieces hang
(checked: all four grafted at once differ from lab A only in the pivots, three pieces' colours
and the mesh names pycgfx makes from them). In the order I'd bet on:

| Lab | X plus | What it looks like | Why |
|---|---|---|---|
| **B** | lab A's **names**: X's horn, membrane and mouth materials renamed `b_horn.001`, `b_membrane.001`, `b_accent_flat.001` (the longest string grows to 74 characters), the nodes `body` and `heart`, lab A's mesh names | exactly X | Dotted material names are in both banners that froze and in none that held (every banner before the kit's named its materials without them). A dot in a name isn't enough by itself (X's bone `body.001` holds), but a material's name also goes inside the quotes of its animation paths (`Materials["b_horn.001"].MaterialColor.Diffuse`), and the HOME Menu finds everything by these names and paths. |
| **C** | lab A's **geometry**: every mesh's vertices and triangles, moved into X's nodes so each sits where lab A has it; X's names, materials and animation | the Blazeplume in the egg, patchy (X's skin through its UVs) and oddly coloured (each piece wears X's material in the same slot) | After the names the biggest difference: the kit's meshes, a sixth fewer vertices (the body a third), other shapes, the egg's triangles in another order. |
| **D** | lab A's **tail swing**: its rest rotation and rotation keys | X with its tail raised behind its left shoulder, wagging there | The one real difference in the animation, and a rotation was all Z changed from P (Z froze, P held). |
| **E** | lab A's **paint**: its 14 materials in its order (without the ".001": that's B), their colours, its skin and wordmark textures, the heart's paler glow; X's mouth and tongue take the membrane's red, its teeth the accent's gold | the classic hatchling in the Blazeplume's flame orange | Completes the split: the material count and sharing (14 against 16) and the colours and skin. The least likely. |

## Building them
```
py -3.12 tools/banner_graft.py names    --out build/banner_labs/B
py -3.12 tools/banner_graft.py geometry --out build/banner_labs/C
py -3.12 tools/banner_graft.py tail     --out build/banner_labs/D
py -3.12 tools/banner_graft.py paint    --out build/banner_labs/E
py -3.12 tools/banner_cgfx.py build/banner_labs/B/banner.gltf build/banner_labs/B/banner.cgfx --turn "body*:1,egg:1"   (and C, D, E)
tools\banner_lab.ps1 -Variants "B=build\banner_labs\B\banner.cgfx;assets\audio\banner.wav", "C=build\banner_labs\C\banner.cgfx;assets\audio\banner.wav", "D=build\banner_labs\D\banner.cgfx;assets\audio\banner.wav", "E=build\banner_labs\E\banner.cgfx;assets\audio\banner.wav" -FirstId 0xEC151
```
The CIAs (`build/lab/banner-lab-b.cia` to `-e.cia`, about 1.3 MB each) pass `check_3ds.py`;
the grafts and the conversion are deterministic, so the commands rebuild the same four banners
byte for byte.
`py -3.12 tools/banner_graft.py none` and `banner_diff.py` against X show nothing changed.
(`build/lab/` in the main checkout still holds lab 1's old B-E CIAs under the same file names.)

## Title IDs used
Every round needs fresh IDs and product codes: `banner_lab.ps1 -FirstId` no longer has a default.

| Round | Labs | Unique IDs | Product codes |
|---|---|---|---|
| lab 1 (run 4) | A-E | 0xEC0D1-0xEC0D5 | CTR-P-EMBL, shared |
| lab 2 (run 5) | F-I | 0xEC0E1-0xEC0E4 | CTR-P-EMBL |
| lab 3 (run 6) | J, K | 0xEC0F1-0xEC0F2 | CTR-P-EMBL |
| lab 4 (run 8) | M-O | 0xEC101-0xEC103 | CTR-P-EMBL |
| lab 5 (run 9) | P-R | 0xEC111-0xEC113 | CTR-P-L111-L113 |
| lab 6 (run 10) | S-U | 0xEC121-0xEC123 | CTR-P-L121-L123 |
| lab 7 (run 11) | V, W | 0xEC131-0xEC132 | CTR-P-L131-L132 |
| lab 8 (run 12) | X-Z | 0xEC141-0xEC143 | CTR-P-L141-L143 |
| lab A (runs 16-17) | A | **0xEC111** (lab P's) | CTR-P-L111 (lab P's) |
| Beta | B-E | 0xEC151-0xEC154 | CTR-P-L151-L154 |
| 1.0 (run 20) | F-K, A2 | 0xEC161-0xEC167 | CTR-P-L161-L167 |
| next free | | 0xEC171 | |

Lab A reused lab P's title ID and product code. It most likely didn't matter (the game's own
title froze on the Blazeplume banner in run 15, and a HOME Menu holding P's banner would have
shown P's, which held), but if none of B-E freezes, lab A itself is the next to re-run, under a
fresh ID.

## For the run list
Paste as a step of the Beta run list (the CIAs go to `cias/lab/` with the build):

> **The banner labs.** Four test titles: **Banner lab B**, **C**, **D** and **E**. Each is the
> game's banner with one thing taken from the Blazeplume banner that froze (lab A), so whichever
> freezes shows what to fix, and the banner can then show one of the new kinds.
> 1. In **FBI**: SD → cias → lab → install all four CIAs (the folder's "Install all CIAs").
> 2. On the HOME Menu, move to **Banner lab B** and rest on it about 15 seconds (never start it).
>    Then **C**, **D** and **E** the same way. If one freezes, hold **POWER** to turn off, turn on
>    again, note which it was and go on with the next (don't rest on the frozen one again).
> 3. What you'll see if it holds: **B** looks exactly like the game's banner (only the names
>    inside the file changed); **C** is the Blazeplume's body in the egg, patchy and oddly
>    coloured on purpose; **D** is the game's banner with its tail raised behind its shoulder;
>    **E** is the old hatchling in the Blazeplume's flame orange.
> 4. Afterwards in FBI: Titles → "Banner lab B" → Delete Title, and the same for C, D and E.
>
> **Send back:** which of B, C, D and E froze and which held.

## What each outcome tells us

| Freezes | Means | Next |
|---|---|---|
| **B** only | the names: almost surely the dotted material names (X's own `body.001` holds), by the dot or the length | banner3d.py names a kind's materials without Blender's ".001" and the Blazeplume banner goes back in; a check lab splits the dot from the length if we want the rule |
| **C** only | something in the Blazeplume's meshes | graft piece by piece (body, head, tail, eyes, egg and cap) to find the one; rebuild it (weld, retriangulate) |
| **D** only | the tail's lifted swing | bake the lift into the tail's mesh and wag it about one axis, as X's does |
| **E** only | the materials or paint | split the count (14 against 16), the colours and the skin texture |
| more than one | each is a cause on its own | fix each |
| none | two of these only together, or the pivots, or lab A's reused ID | re-run lab A under a fresh ID, with pairs (names + geometry, geometry + tail) |

## The 1.0 round: pairs (run 20)
Every Beta lab held (run 19: B, C, D and E each alone didn't freeze), so this round grafts lab A's
parts two at a time, and lab A itself comes back under a fresh title ID (the first time it took lab
P's). Whichever freezes narrows it to two parts; if only A2 freezes, it takes three or all four; if
A2 holds, the old freeze was lab A sharing lab P's ID and the kit's banners can be tried again.

| Lab | X plus lab A's | Title ID |
|---|---|---|
| **F** | names + geometry | 0xEC161 |
| **G** | names + tail | 0xEC162 |
| **H** | names + paint | 0xEC163 |
| **I** | geometry + tail | 0xEC164 |
| **J** | geometry + paint | 0xEC165 |
| **K** | tail + paint | 0xEC166 |
| **A2** | lab A again (its glTF as tested, `assets/banner3d/lab-a/`) | 0xEC167 |

```
py -3.12 tools/banner_graft.py names,geometry --out build/banner_labs/F   (G names,tail; H names,paint; I geometry,tail; J geometry,paint; K tail,paint)
py -3.12 tools/banner_cgfx.py build/banner_labs/F/banner.gltf build/banner_labs/F/banner.cgfx --turn "body*:1,egg:1"   (and the rest)
py -3.12 tools/banner_cgfx.py assets/banner3d/lab-a/banner.gltf build/banner_labs/L/banner.cgfx --turn "body*:1,egg:1"
tools\banner_lab.ps1 -Variants "F=build\banner_labs\F\banner.cgfx;assets\audio\banner.wav", ..., "A2=build\banner_labs\L\banner.cgfx;assets\audio\banner.wav" -FirstId 0xEC161
```
All seven CIAs (`build/lab/banner-lab-f.cia` ... `-a2.cia`) pass `check_3ds.py`.


### Results (run 20, 2026-09-29)
| Lab | X plus lab A's | Result |
|---|---|---|
| F | names + geometry | held |
| G | names + tail | held |
| **H** | **names + paint** | **froze** |
| I | geometry + tail | held |
| J | geometry + paint | held |
| K | tail + paint | held |
| **A2** | lab A itself, fresh title ID | **froze** |

So the title ID wasn't it (A2 froze under its own), and neither part alone freezes (B names and E
paint held in run 19): **the names and the paint together** do. Paint renames nothing, but it brings
lab A's 14 materials in its order and colours; with lab A's names on top, the three dotted material
names (`b_horn.001`, `b_membrane.001`, `b_accent_flat.001`) land in paint's material table. The next
round, when Noah asks for labs again: X plus paint with each dotted name alone (three labs), and
paint with the names undotted, to find the one pairing. Noah's note: none of the ones that held look
good (F's tail doesn't read from the front); a new kind's banner will need its own art pass anyway.

## The new banner: kit hatchlings (run 25, 2026-10-02)
Noah: *"Make a more highly detailed banner, cleaner, and use one of our baby dragons still in the game.
Then variants of it for a banner test."* (D144.) Each lab is a kit kind's hatchling as the game has it,
built by `tools\banner_variants.ps1` (`tools/blender/banner3d.py --kind <kind> --variant <n> --outline
<w> --turn`): the den model's full detail (LOD 0), smooth-shaded, its skin baked at 256, peeking from the
cracked egg with the wordmark, under X's recipe (rigid pieces, `--turn "body*:1,egg:1"`). What's new for
the HOME Menu: **no dotted names** anywhere (every node, mesh, material and image tidied: run 20's
suspect), the solid parts **one-sided** (pycgfx doubles every two-sided material's vertices), texture
coordinates only where a texture reads them, the faces deep inside the egg dropped, and on four of them
an **ink outline** (an inverted hull in one unlit material, "ink"). 13-15 materials (X had 16).

| Lab | Kind (colouring) | Outline | CGFX | Title ID |
|---|---|---|---|---|
| **25A** | Pouncer (Ember) | ink | 494 KB | 0xEC168 |
| **25B** | Blazeplume (its first) | ink | 502 KB | 0xEC169 |
| **25C** | Crestwing (its first) | ink | 509 KB | 0xEC16A |
| **25D** | Puffback (its first) | ink | 455 KB | 0xEC16B |
| **25E** | Blazeplume | none | 448 KB | 0xEC16C |
| **25F** | Pouncer | none | 455 KB | 0xEC16D |

```
tools\banner_variants.ps1 -Variants "A=pouncer:0:0.012", "B=blazeplume:0:0.012", "C=crestwing:0:0.012", "D=puffback:0:0.012", "E=blazeplume:0:0", "F=pouncer:0:0"
tools\banner_lab.ps1 -Variants "25A=build\banner_v\A\banner.cgfx;assets\audio\banner.wav", ... "25F=..." -FirstId 0xEC168
```
B and E are the Blazeplume that froze in run 15 (lab A), now with its names undotted: if they hold, the
dotted names were the trouble. The game's own CIA keeps X until Noah picks one.

### Results (run 25, 2026-10-02)
| Lab | | CGFX | Result |
|---|---|---|---|
| 25A | Pouncer, ink | 494 KB | held |
| **25B** | Blazeplume, ink | 502 KB | **froze** (a first frame, then stuck) |
| **25C** | Crestwing, ink | 509 KB | **froze** |
| 25D | Puffback, ink | 455 KB | held ("ugly") |
| 25E | Blazeplume, no ink | 448 KB | held |
| 25F | Pouncer, no ink | 455 KB | held: **Noah's pick** ("probably the best") |

So **the size**, not the kind or the names: the two over 500 KB froze, everything at 494 KB or under held,
the Blazeplume included (25E: run 15's freeze was the dotted names, or its size then). Whatever the HOME
Menu's real ceiling is, it's under 502 KB of CGFX: `tools\banner_variants.ps1` now warns over 480 KB.
Noah also saw the joints split open as the head and tail moved (rigid pieces turning, each open where it
was cut, one-sided: the hollow inside showed).

## Round 26 (run 26): 25F with its joints closed
`banner3d.py` now closes each piece's openings by its joints with skin (`cap_openings`), the collars wider
(a kind's head 0.15 of its height, tail 0.13) and the motion gentler (the head's roll 8 degrees, its nod 4,
the tail's wag 16); the previews draw one-sided as the 3DS does, so a hole would show there (`--seams`
renders the joints mid-motion).

| Lab | | CGFX | Title ID |
|---|---|---|---|
| **26A** | Pouncer, no ink (25F, joints closed) | 456 KB | 0xEC16E |
| **26B** | Blazeplume, no ink (25E, joints closed) | 448 KB | 0xEC16F |

If 26A holds and its joints stay shut, it becomes the game's banner (`tools\make_banner.ps1` with
`--kind pouncer`).

### Results (run 26, 2026-10-02)
| Lab | | CGFX | Result |
|---|---|---|---|
| **26A** | Pouncer, joints capped | 456 KB | **froze** |
| 26B | Blazeplume, joints capped | 448 KB | held, but the head still parted from the jagged neck as it twisted |

26A froze at 456 KB where 25F (455 KB, the same banner without the caps, wider collars and gentler
motion) held, and 25A held at 494 KB: so the size alone isn't the trigger after all. Every glTF checked
(no NaN, no zero normals; 26A has no degenerate triangles, and 26B, which held, has 20), the same materials,
nodes and animation channels as 25F. What differs is only the geometry's detail and the motion's values.
Noah: "If you cannot fix this, we should stop the dragon from moving its head. But the eye animation works.
And bobbing up and down is fine." So:

## Round 27 (run 27): the head held still, and a control
`banner3d.py --still-head` joins the head and tail into the body (by hand with bmesh: `bpy.ops.object.join`
crashed Blender 5.2 on the pieces' leftover vertex groups), no collars; the eyes blink, the heart beats,
the dragon bobs; nothing turns but the body and egg against the HOME Menu's own turn. The cap sits on the
right side of the head (Noah: "the egg needs to be moved to one side of the head"). Five nodes fewer.

| Lab | | CGFX | Title ID |
|---|---|---|---|
| **27A** | Pouncer, still head | 419 KB | 0xEC170 |
| **27B** | Blazeplume, still head | 426 KB | 0xEC171 |
| **27C** | 25F again, unchanged (the control: if it freezes now, the freezes come and go) | 455 KB | 0xEC172 |

### Results (run 27, 2026-10-02)
27A (still Pouncer) **froze**, 27B (still Blazeplume) **froze**, 27C (25F unchanged) **held** again: the
freezes are the content's, every time.

## What froze them: the textures' alignment (2026-10-02)
pycgfx puts each blob in the CGFX's IMAG section (the textures, the vertex streams) on a 16-byte boundary.
Where the two textures (the 256 skin, the wordmark) landed, modulo 64, split all ten labs of runs 25-27:

| Lab | Result | Texture offset % 128 |
|---|---|---|
| 25A, 25D, 25E, 25F (twice), 26B | held | 64, 80, 0, 0, 16 |
| 25B, 25C, 26A, 27A, 27B | froze | 96, 32, 112, 48, 96 |

At 0 or 16 past a 64-byte boundary they held, at 32 or 48 they froze, whatever else was different: a few
vertices more (26A against 25F), the head joined (27A), names or paint (run 20's H). So
`tools/banner_cgfx.py` now writes the CGFX itself (`write_aligned`): the IMAG content starts on a 128-byte
boundary and every blob is padded to one; it refuses a file that isn't, and prints where the textures sit.
The size cost: about 4 KB. The size ceiling (D145) was a coincidence; 512 KB is the only limit.

## Round 28 (run 28): the looks, all aligned
The head held still (Noah's run 26 note), the cap on its side, and a choice of looks, with Noah's run 27
note ("You can add a bit of a border, that look wasn't bad"): the ink outline thin or bold.

| Lab | | CGFX | Title ID |
|---|---|---|---|
| **28A** | Pouncer, thin ink (0.008) | 481 KB | 0xEC173 |
| **28B** | Pouncer, bold ink (0.016) | 481 KB | 0xEC174 |
| **28C** | Blazeplume, thin ink | 489 KB | 0xEC175 |
| **28D** | Blazeplume, bold ink | 489 KB | 0xEC176 |
| **28E** | Pouncer, no ink (27A, aligned) | 423 KB | 0xEC177 |
| **28F** | Blazeplume, no ink (27B, aligned) | 430 KB | 0xEC178 |
| **28G** | Pouncer in its Tabby colouring, ink (0.010) | 481 KB | 0xEC179 |
| **28H** | 26A unchanged but aligned (its head moving, the joints capped) | 461 KB | 0xEC17A |

28E, 28F and 28H froze before unaligned: if they hold now, the alignment was it.

**Run 28: all eight held**, 28E, 28F and 28H too: the alignment was it (D147). Noah picked **28G**, now the
game's banner (D148): `tools\make_banner.ps1` builds its look (`--kind pouncer --variant 1 --outline 0.010
--still-head --turn`), and 0.10.5's CIA carries the exact file that held (`build\banner_v\T1\banner.cgfx`;
Blender joins the mesh with its vertices in another order each run, the same mesh in other bytes, every build
aligned all the same). The lab titles 0xEC173-0xEC17A can be deleted; their CIAs are off the SD.
