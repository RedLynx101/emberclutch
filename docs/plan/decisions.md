# Decision Log

Approved decisions, newest first. "Approved" means the project owner signed off.

## 2026-09-24 — Alpha 2 order: the style review, the icon, working alone

| # | Decision | Why | Status |
|---|---|---|---|
| D47 | **Style review R5 in Alpha 2** ([Alpha 2 plan](alpha-2.md)): three variants of the Ember dragon next to the current style (textured): V1 a new surface on today's shapes, V2 new proportions and shapes, V3 a bold redesign, all in the game's theme. Shown as review sheets and turntables **and** switchable in the game (dev menu). It's the last thing built before Noah decides and **blocks** the dragons update (the chosen style, full parts library, all breeds), which comes before the run on the old 3DS | Noah wants to choose between keeping the style and moving to a newer one before the big dragon content is built on it | Approved |
| D48 | **A designed app icon and HOME Menu banner in Alpha 2**: a style-independent emblem (a glowing heart inside an ember-lit egg with a curl of wing), shown with the R5 variants. Alpha 1's CIA uses an interim icon rendered from our own dragon model, so the AI-concept-derived placeholder never ships | Noah asked for a better, thematic icon; an emblem doesn't depend on the style decision | Approved |
| D50 | **An animated 3D HOME Menu banner** (Alpha 2 WP10): the textured baby dragon peeking out of its ember-lit egg, blinking and wagging, the heart pulsing. Scripted from our Blender model through glTF and pycgfx to CGFX, packed with bannertool (`-ci`, the `extendedbanner` SMDH flag); under 512 KB, `COMMON` model and animation, rigid pieces animated by node transforms (skinned banners have frozen real HOME Menus). Proven on the old 3DS (WP13); a flat 2D banner of the same scene is the fallback. Re-exported in the chosen style after R5 | Noah asked for a 3D banner with a baby textured dragon | Approved (the pycgfx download awaits Noah's OK) |
| D49 | **Working alone until R5.** From 2026-09-24 Noah steps back: the rest of Alpha 1 and Alpha 2 Phase A are worked package by package (commit, push, STATUS and RedWiki after each). Computer use only while testing in the emulator; stand-ins for sounds that haven't arrived (D35); R2, R3, R4 and R6 are sent without blocking. Stops: R5 (the variants) and the hardware run (D34) | Owner direction | Approved |

## 2026-09-24 — Hands-on care and travel (Noah's answers)

| # | Decision | Why | Status |
|---|---|---|---|
| D43 | **The full hands-on care polish is Alpha 1** ([care interactions](../design/care-interactions.md)): tools drawn where you touch, petting with lean-in and a sweet spot, brushing and polishing shine regions, the bath, hand-feeding into the jaw, fetch with ball physics, calling. Extra toys (tug rope, feather wand, puzzle orb, bowl) come with the Market in Alpha 2 | It's the heart of "a living pet"; WP7 becomes Alpha 1's biggest package | Approved |
| D44 | **Before a dragon can carry you, you explore by following your young dragon** (1.0, with the valley): the camera follows it, the Circle Pad guides it, it trots and hops, and from Juvenile on it glides short distances from ledges. No player on foot; the player is still only seen as a rider | Noah's pick over fast-travel-only and walking together; no walking player character needed | Approved |
| D45 | **Free flight stays in 1.0**; the map with fast travel arrives in Alpha 2 ([map & travel](../design/world-map-and-travel.md)) | The valley is 1.0 work; fast travel keeps Alpha 2's places quick to reach | Approved |
| D46 | **Dragons get visibly dirty**: dust slowly dulls a region's colours over a day or two, Wanderings can bring back mud spots (Alpha 2); brushing and baths visibly clean them. Built with texturing (R2) | Grooming should show a before and after, like Nintendogs | Approved |

## 2026-09-24 — Cuteness pass

| # | Decision | Why | Status |
|---|---|---|---|
| D42 | **Dragons blink by squashing their eyes.** A 38th bone, `eyes`, sits between the eyes (parallel to the head, parented to it, grown like it) and carries the eye parts; the renderer squashes it vertically: a blink every 2–6 s, shut in sleep, a content squint while petted, a sleepy one while yawning. The closed eye is the squashed eye (a thin line), not a separate eyelid mesh | No extra triangles and no body skin on the new bone (the body draw stays at 25 bones); the animation clips stay rotation-only because the eyelids are driven by the den activity, not by clips | Default (owner can change) |
| D41 | **The dragons have an opening mouth.** A `jaw` bone hinged behind the mouth corners, parented to the snout, parallel to it and grown by the snout's tables; the snout is slit along the mouth line, with a dark pocket inside, little teeth on both lips (two small fangs show when closed) and a tongue as a new part group. The body draw grows from 24 to 25 bones (shader uniforms 95 of 96), so the model format goes to `.ecm` v2 (32-byte bone palettes). The inside of the mouth mixes the pupil colour with a new tongue palette slot; teeth mix glint white with the horn colour | Noah asked for "an animated mouth with little teeth lining them, and maybe a little tongue". Growing the jaw exactly like the snout keeps the lips together at every growth stage and build without new tables; giving the snout's skin to the head instead would have lost its growth and build shaping | Default (owner can change) |

## 2026-09-24 — Den scene (WP6)

| # | Decision | Why | Status |
|---|---|---|---|
| D40 | **The den is a round cave seen as a cutaway diorama.** Walls all the way round face inward, so back-face culling hides whichever part stands between the camera and the dragons; the floor runs on into the dark. Room radius 9.5 (the walkable circle stays 6 around the rug), props in the back two thirds, a skylight in the back wall. Lighting is baked into vertex colours for day, evening and night and blended by the clock; the dragons' light follows, picking up the room's light where they stand | The den camera follows the dragons from the front-left and often sits outside the room; three adults (nose ~2.3 ahead of their origin) need the room to be well past the walkable circle. Baked vertex colours cost nothing per frame on an old 3DS | Default (review sent, owner can change) |

## 2026-09-23 — Sculpt reviews R1 and R1b

| # | Decision | Why | Status |
|---|---|---|---|
| D36 | **Two body forms.** Hatchlings use their own baby model (metaball-sculpted: round head, big eyes, short snout, chubby body, stubby legs, tiny wings). The stage-up to juvenile swaps to the grown model behind a glow, **the first molt**. Within each form, growth is still bone scaling. Both forms share one skeleton layout, so animations carry over | R1: the hatchling made by shrinking the adult mesh was "super ugly" (floating wings, massive chest). A baby needs its own proportions and head detail | Approved (R1b) |
| D37 | **Classic dragon wings for every breed:** arm, forearm, thumb claw and four long fingers spread across the whole membrane, which reaches back along the flank (12 wing bones). The Wings gene now only picks the **trailing edge**: Classic (scalloped, was Membrane), Plumed (frilled, was Feathered), Sail (smooth and rounded, was Fin). Enum values are unchanged, so saves are unaffected | R1: "larger generic dragon wings for all", no spined top over a bare bottom | Approved (R1b) |
| D38 | **Stronger breed silhouettes:** builds change proportions by 15–55% (sturdy = heavy chest and thick limbs, sleek = slender with long legs, long = serpentine neck and tail with short legs), and the **Frill gene also picks the dorsal ridge** (none/leaf: spikes, fin: a fin sail, feather: plumes along the neck and back). Frills and tail tips are bigger | R1: breeds differed "more so in color"; shape differences were too subtle | Approved (R1b) |
| D39 | **Faces and growth after R1b:** every dragon gets nostrils and a thin mouth line, projected onto the body and joined into its mesh (so they deform with the skin). Hatchlings grow about 1.6x within their stage, so the late hatchling sits halfway between the newborn and the juvenile. Frill bases sit deeper in the skull | Noah's R1b verdict: "certainly a nose", "consider a mouth", stage 2 looked like stage 1, the blue baby's head bits floated | Approved (R1b follow-up) |

## 2026-09-23 — Design review rounds 1–3

| # | Decision | Why | Status |
|---|---|---|---|
| D17 | Growth pace stays at **~2 weeks with perfect care** (4 / 8 / 14 day gates) | Owner review | Approved |
| D18 | **Three starters** only: Ember, Tide, Gale | Owner review | Approved |
| D19 | Dragons are **male or female**, 50/50 per egg, revealed at hatch; **breeding needs one of each** | Owner review | Approved |
| D20 | Upset dragons **retreat to the sulk nook** in the den until you make up | Owner review | Approved |
| D21 | **Wanderings (pedometer) is a v1 must-have**; it moves up to Phase 3 with breeding and Market eggs, which together supply opposite-sex partners | Owner review | Approved |
| D22 | The Suno batch 1 music is processed; both Skyreach takes are kept and alternate during flight | More variety on long rides | Approved (owner handed the music over) |
| D8, D9 | Breeding/variants and the heartglow theme | "All looks good" in review | Approved |
| D23 | **Subtle sex differences:** males have bigger horns and crest; females have a longer tail fan and brighter accent sheen. Silhouettes stay close | Owner choice | Approved |
| D24 | **Egg sex:** bred and wild eggs are a hatch-day surprise; **Market eggs are labeled** so a partner can be bought on purpose | Owner choice | Approved |
| D25 | Install the **Azahar** emulator (winget) for quick local testing and **3ds-libvorbisidec** for Ogg music. Performance sign-off stays on real old-3DS hardware | Owner approved both installs | Approved |
| D26 | **Art reviews at milestones:** renders after the sculpt, after texturing, and after rigging/animation | Owner choice | Approved |
| D27 | **Naming:** name the dragon at hatch with the 3DS keyboard (a random suggestion is pre-filled) and rename it any time in the den | Owner approved the proposed default | Approved |
| D28 | **Hardware testing is deferred** until the game is much further along; development is checked in the Azahar emulator with budget counters in a debug overlay. Recommended first hardware check: by the end of Alpha 2 | Owner direction | Approved |
| D29 | Work is organized into **playable milestones**: Alpha 1 (*a living pet*), Alpha 2 (*a den*), Beta (*a trainer*), 1.0 (*the sky*), 1.x (Sky Visits), 2.0 (equine) | Keeps every stage enjoyable and reviewable | Approved |
| D31 | **Goal: complete through Alpha 2.** Work continues package by package, committing, pushing and updating STATUS/RedWiki after each, stopping only for the gates below | Owner goal | Approved |
| D32 | **Review gates during the run:** R1 (sculpt) **blocks** until Noah approves; R2 (textured) and R3 (rigged/animated) are sent but **don't block**. Feedback is folded in when it arrives | Owner choice (refines D26) | Approved |
| D33 | Pre-approved download: **Nunito** and **Cinzel Decorative** (SIL OFL) from Google Fonts' official GitHub, with license files, into `assets/fonts/` | Owner approval | Approved |
| D34 | **Alpha 2 is not complete until it has run once on Noah's old 3DS.** Everything else can finish on emulator checks; the final step waits for the hardware session | Owner choice (refines D28) | Approved |
| D35 | Missing Suno sounds never block: placeholder sounds are used until Noah delivers the briefs' tracks (the sound effects arrived on 2026-09-24, from ElevenLabs) | Keeps the run moving | Approved (default) |
| D30 | **English only for v1**, with every string in one table so translations can be added later | Scope | Default (owner can change) |

## 2026-09-23 — Project kickoff

| # | Decision | Why | Status |
|---|---|---|---|
| D1 | The first creature is **dragons** (not aquarium, horses or pegasi) | Growth stages, flight-based competitions without riding, a stylized look that suits old-3DS limits, and a wing rig that carries over to pegasi and alicorns | Approved |
| D2 | Name: **Emberclutch** | Warm (ember) + eggs and breeding (clutch). No existing game found with this name | Approved |
| D3 | **Old 3DS is the performance floor**. Native C++17, libctru, citro3d (top screen) and citro2d (bottom screen) | Same stack as 3D-Claw and asteria-ds, and the toolchain is already installed | Approved |
| D4 | **Riding allowed anywhere in free roam**, never in training or competitions. Events are split between flying and grounded | Owner direction | Approved |
| D5 | Egg → Adult in **about 2 weeks**, gated by real days (3DS clock) **and** care stars | Owner direction: growth driven by needs met over time | Approved |
| D6 | **Forgiving but upset**: no death, running away or regression. Neglect makes the dragon Upset until you make up | Owner direction | Approved |
| D7 | Own **many dragons of any breed**: den (3 active + 2 egg nests), off-map **Sanctuary**, egg **Cold Vault** | Owner direction; the 3-dragon cap in the den comes from old-3DS performance | Approved |
| D8 | **Breeding with lots of variants**: Mendelian element alleles (6 base breeds + 15 hybrids) plus inherited parts, colors and rare traits | Owner direction: creative but not overly complex | Approved (review round 1) |
| D9 | Visual theme: **warm light inside**. A heart-shaped heartglow is the signature and the mood indicator. Babies super cute, adults majestic | Owner direction ("super cute", "majestic") | Approved (review round 1) |
| D10 | Voice commands are **on-device and optional**, with cue buttons always available | Accessibility, public release, no cloud | Approved (from suggestions) |
| D11 | Species data has `bodyPlan` + `modules` fields from day one, so **the equine line and alicorns** fit later | Owner wants alicorns planned | Approved |
| D12 | **Open source**, starting private. Code MIT, original art CC BY-SA 4.0, music under its own notice | Owner direction; Suno terms don't allow relicensing the music | Approved |
| D13 | Deploy over **Wi-Fi** (3dslink / ftpd), not by pulling the SD card | Owner direction | Approved |
| D14 | Concept art is AI-generated **reference only** and is not shipped | Keeps the open-source asset licensing clean | Approved |
| D15 | Music is made in **Suno**, then looped, blended, loudness-matched and compressed to Ogg by `tools/audio/make_loop.py` | Owner direction | Approved |
| D16 | Skip StreetPass, amiibo, face tracking and online play | Not practical for homebrew on an old 3DS | Approved |
