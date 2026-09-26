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
| next free | | 0xEC161 | |

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
