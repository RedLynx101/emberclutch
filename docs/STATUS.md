# Emberclutch — Status

*Live handoff page. Update it at the end of every work session.*

**Updated:** 2026-10-02 · **Milestone:** **the road to 1.0 (D142, [plan](plan/release-1.0.md)): **1.0 released** (D152): v1.0.0 public on GitHub; next the Universal-DB request (Noah's, by hand) and the video (R6), the guide and the trailer planned, the repo set up as open source** (before: the Living Valley pass, D135-D141, [plan](plan/living-valley.md), [story](design/story.md)). The game is **Emberclutch: Skyreach Valley** (D120). Run 24 came back with the plans kept and seven fixes (0.10.1, D143); the new banner is settled (D144-D148: the freezes were the textures' alignment; Noah's pick, lab 28G, is the game's banner); the plan to tie up 1.0 is D142. Before: Beta 1 (`v0.3.0-beta`), Alpha 2 (`v0.2.0-alpha2`).
· **Branch:** `main` (private `RedLynx101/emberclutch`)

## Tooling: headless emulator checks (2026-09-30)
- `tools\autotest.ps1 <script> -Headless` runs autotests in Azahar 2126.1.1 inside a WSL distro
  of their own (`emberclutch-test`, set up by `tools\wsl\install.ps1`): no window on Noah's
  desktop, no lock (three tours side by side in 44 s), full speed (the tour in about 33 s, 65 s
  at real time), the old 3DS, the 3DS clock at 2026-06-01 10:00 every run, the DSP firmware
  copied in. Runs repeat (under 0.02% of pixels between two tours); `tools\shotdiff.ps1` saves
  a baseline and lists the pictures that changed. Guide: `docs/tech/headless-emulator.md`.
  Performance is still signed off on the 3DS: no emulator models its timing.
- Found on the way: with the fixed clock, Azahar shifts the time by the PC's time zone (fixed by
  running it in UTC), and the async file reads and presentation made animations drift (off).

## Now: 1.0 released; the video next (2026-10-02)
- **v1.0.0 is public:** https://github.com/RedLynx101/emberclutch/releases/tag/v1.0.0 (the CIA, the 3DSX, the
  zip, the guide); the QR link checked; the repo's description, homepage and topics set; *Built for the
  original 3DS* in the README and the notes (D152).
- **Universal-DB:** Noah's to send by hand (their rules: declare the LLM use as *yes*; no LLM-written request
  text): the form at https://db.universal-team.net/app-request, then an *App request* issue with the JSON
  and his own sentence or two (`docs/plan/release-1.0.md` R5).
- **Next:** the video (R6, `docs/plan/trailer.md`): V1, the film build and the capture spike; V2, the
  storyboard, script and voice samples, then stop for Noah's pick. Tell Noah to switch to max before V4.

## Now: 1.0, called; the release ready to go public (2026-10-02)
- **Run 31 came back:** "It all runs great, get the rest ready for me to make this public." The traits and the
  guide kept.
- **Ready (D151):** the files (`dist\player\`: `emberclutch.cia` sha256 `84feca64...`, `emberclutch.3dsx`
  `b453fea9...`, `emberclutch-1.0.0.zip` `7bf73091...`; `docs/guide/Emberclutch-Guide.pdf` `c22571d2...`), the
  notes (`docs/release/notes-v1.0.0.md`), FBI's QR code (`docs/release/fbi-qr.png`, the latest release's CIA),
  the README (the guide, the QR code), `tools/release/release.ps1` (a draft, then `-Publish`; refuses while
  private). History scanned again: clean.
- **Next (Noah):** make the repo public (and turn on private vulnerability reporting). **Then (on his word):**
  `tools\release\release.ps1` for the draft, a look, `-Publish`; the QR scanned with FBI once; the Universal-DB
  request (R5); the video (R6: stop at the voice samples; tell Noah to switch to max before the edit passes);
  YouTube (R7).

## Now: run 31 (1.0.0), the traits made real, the guide's almanac (2026-10-02)
- **Run 30 came back:** the player build tested fine ("we're good, I tested it. Let's send this one"); the
  handbook "is cool", with one addition: what you can't easily see in the game, "like what exactly each trait
  does... like an old style game guide". Writing that showed the traits did nothing.
- **D150:** each of the 34 traits does one thing (`docs/design/traits.md`); a test checks all 34 (387,513 checks,
  0 failures); the six smoke tours (tour, battle, glade, the Lantern Trial, the race, the Wanderings) run clean
  headless. The Hollow's floor-30 guardian is now its hardest fight (named in the run's notes).
- **The guide, draft 2:** the almanac (31 A5 pages), 60 numbers and rows checked against the code.
- **Sent** to .51: the 1.0.0 player build with the traits (save backed up to `build/3ds-backup/2026-10-02_1036/`).
  Run 31's steps (`docs/plan/hardware-check-16.md`), the traits card and the guide's pages on the review page
  (version 25, collections `run31`, `plans` slugs `traits`, `guide-draft-2`).
- **Came back** (run 31): see 1.0 above.

## Now: run 30 (1.0.0), the 1.0 candidate and the guide's first draft (2026-10-02)
- **Run 29 came back clean** ("Looks good. What's next? The non-dev build and 1.0?"; "On .51 when ready.
  Continue").
- **1.0.0, the player build** (D149) on the 3DS as the candidate: `tools\package_cia.ps1 -Player` (75 MB,
  `dist\player\`: the CIA, the 3DSX and `emberclutch-1.0.0.zip`), sent with `tools\deploy_ftp.ps1 -Player`. The
  same save folder: the save carries over (backed up to `build/3ds-backup/2026-10-02_0924/`; run 29's log: no
  long frames after loading, so the new islands cost nothing).
- **The pass for clipped words** (dev builds log text off its screen or squeezed under 75% on shot frames;
  38 tours): the Journal's long lines and a speaker's caption now take two smaller lines (`textFit`).
- **The release shots** retaken (`docs/release/screenshots/`); README's growth time and flying keys fixed;
  Universal-DB's title.
- **The guide, draft 1** (`docs/guide/guide.md`, 20 A5 pages; `py -3.12 tools/guide/build_guide.py --pages`
  writes `build/guide/`; `tools/guide/check_guide.py` checks its 27 numbers against the code). The PDF goes in
  `docs/guide/` once the draft is kept.
- Run 30's steps (`docs/plan/hardware-check-15.md`) and the guide's pages and PDF on the review page (version
  24, collections `run30`, `plans` slug `guide-draft`). Tests 387,475 checks, 0 failures.
- **Came back** (run 30): see run 31 above.

## Now: run 29 (0.10.5), the banner in the game, the isles made pretty (2026-10-02)
- **Run 28's notes** (collections `run28`, `labs`, `plans`): all eight labs held (28E, 28F and 28H froze before:
  the alignment was it); "Use 28G"; no stutter on the isles and the lamp lit right, but a trip to the Floating
  Isles set him down under the island; "the floating islands need to be textured. Make them pretty."
- **0.10.5** (D148): **28G is the game's banner** (`tools\make_banner.ps1` builds its look; the CIA carries the
  exact file that held). `core/place_layout` `placeArrival`: a trip stands you on what's at the place's height
  (the isles: the island's top), tested for every place; the autotests' `goto` stands on an island's top too.
  **The floating islands** (`buildValleyExtras`, `makeIsleTexture`, render3d's island passes): dappled tops
  painted like the land, the turf rolling over the edge, vines, trees and flowers on top; earth, then lilac rock
  in strata to a hanging point, smaller points, crystals; the rock's own texture laid on run by run (east-west,
  north-south, down). 475-716 triangles an island (the props within 180 m); the valley views of
  `tests/autotest/isles.txt` 1,000-4,100 triangles (the budget ~9,600). Tests 387,475 checks, 0 failures.
- **Run 28's 3DS log:** no long frames after loading (run 27's, with the tracer on, had 2-6 s stalls).
- **Sent** to .51 (CIA 75 MB, 0.10.5; the save backed up to `build/3ds-backup/2026-10-02_0808/`; `/cias/lab/`
  emptied). Run 29's steps (`docs/plan/hardware-check-14.md`) and the islands' before-and-now card on the review
  page (version 23, collections `run29`, `plans` slug `isles-29`).
- **Came back** (run 29, collection `run29`): every step ticked, nothing broken; "Looks good. What's next? The
  non-dev build and 1.0?" (the isles card left unmarked: taken as Keep).
- **Then:** see run 30 above.

## Now: run 28 (0.10.4), the banner freezes found, the isles' stutter, the lamp (2026-10-02)
- **Run 27's notes** (collections `run27`, `labs`): 27A and 27B froze, 27C (25F unchanged) held again; flicker
  and stutter (the music stopping) standing on the two NE islands; a Crestwing's breath at the middle island's
  lamp went off in another direction. Noah: "create slight variants enough for me to just settle on a good
  solution ... You can add a bit of a border".
- **The freezes** (D147, `docs/tech/banner-labs.md`): the textures' offset in the CGFX, 0 or 16 past a 64-byte
  boundary held and 32 or 48 froze, in all ten labs. `tools/banner_cgfx.py` `write_aligned` puts every IMAG blob
  on a 128-byte boundary and refuses otherwise. Round 28, all aligned: **28A-28D** Pouncer and Blazeplume with a
  thin (0.008) or bold (0.016) ink border, **28E/28F** no border (27A/27B aligned), **28G** the Tabby Pouncer,
  **28H** 26A aligned (0xEC173-0xEC17A, 423-489 KB).
- **0.10.4:** the islands drawn after a throwaway triangle, the depth setting before each (D118); the lantern's
  breath aimed at the ground at the place's height (`groundAt`, not the land below the island); the Makefile's
  `build/.version` stamp recompiles the title screen and tracer when VERSION changes. The stutter was the
  tracer's probes and SD writes answering the flicker: **trace.on deleted** on the 3DS. Tests 377,709, 0
  failures.
- **Sent** to .51 (CIA 75 MB, 0.10.4; save backed up to `build/3ds-backup/2026-10-02_0631/`, run 27's
  screenshots in `build/3ds-shots/2026-10-02_0631/`; `/cias/lab/` holds 28A-28H). Run 28's steps
  (`docs/plan/hardware-check-13.md`) on the review page (version 22, collection `run28`).
- **Came back** (run 28): see run 29 above.

## Now: run 27 (0.10.3), the banner's head held still, gliding off the isles (2026-10-02)
- **Run 26's notes** (collections `run26`, `labs`): 26A froze (456 KB, where 25F at 455 held: not the size
  alone), 26B held but its head still parted from the neck as it twisted ("If you cannot fix this, we should
  stop the dragon from moving its head"); the cap should sit to one side of the head; Tam's rod and the hats
  fine; walking off the floating island over the lake dropped him to the water instead of gliding.
- **0.10.3** (D146): `banner3d.py --still-head` (the head and tail joined into the body by hand with bmesh, no
  collars; eyes, heart and the bob still move; the cap on the right side of the head);
  `tools\banner_variants.ps1` takes a `:still` field. Labs **27A** (Pouncer, 419 KB, 0xEC170), **27B**
  (Blazeplume, 426 KB, 0xEC171), **27C** (25F unchanged, the control, 0xEC172). `core/flight.cpp`: off an
  edge over water the drop glides (the deep-water check had come first); a test walks a rider off every
  island four ways (it fails four ways on the old code). Two new tests (run 24's woken-at-night, the isles)
  registered with `RUN`: they hadn't been running. Tests 377,709 checks, 0 failures.
- **Sent** to .51 (CIA 75 MB, 0.10.3; the save backed up to `build/3ds-backup/2026-10-02_0601/`; `/cias/lab/`
  now holds 27A-27C). Run 27's steps (`docs/plan/hardware-check-12.md`) on the review page (version 21,
  collection `run27`).
- **Came back** (run 27): see run 28 above.

## Now: run 26 (0.10.2), the picked banner's joints closed, Tam's rod in 3D (2026-10-02)
- **Run 25's notes** (collections `run25`, `labs`): labs 25B (502 KB) and 25C (509 KB) froze, the rest (448-494 KB)
  held: the HOME Menu's ceiling is the CGFX's size (not the kind, not the compressed banner's), so banners stay
  under 480 KB. Noah's pick: **25F** (the Pouncer, no outline). The joints split as the head and tail moved; Tam's
  rod showed in front of you and your dragons; the Crestwing's hats can be much smaller. Night care, the wanderer,
  the cove, the Lilyfin: fine.
- **0.10.2** (D145): `banner3d.py` closes each piece's openings by its joints with skin (`cap_openings`), wider
  collars (a kind's head 0.15, tail 0.13), the head's roll 8 and tail's wag 16 degrees, one-sided previews and
  `--seams`; labs **26A** (the Pouncer, 456 KB, 0xEC16E) and **26B** (the Blazeplume, 448 KB, 0xEC16F). Tam's rod
  a 3D prop (`rodMesh`, `PropKind::Rod`, drawn in `drawCoveThings`; the 2D `drawOver` gone). Hats down to 50%,
  floating weighed 1.5x: the grown Crestwing's and Curlstone's at 50%, the Cindershell's 60%, none floats. Tests
  377,652 checks, 0 failures; `hatsfit_g`, `cove_cast` checked.
- **The video's effort:** xhigh for V1-V3, and before V4 I tell Noah to switch to max (`docs/plan/trailer.md`;
  memory `video-effort-switch`).
- **Sent** to .51 (CIA 75 MB, 0.10.2; the save backed up to `build/3ds-backup/2026-10-02_0217/`; `/cias/lab/`
  now holds only 26A and 26B). Run 26's steps (`docs/plan/hardware-check-11.md`) on the review page (version 20,
  collection `run26`; the banner and hat cards in `plans`).
- **Came back** (run 26): see run 27 above.

## Now: run 25 (0.10.1), the new banner on test, run 24's fixes (2026-10-02)
- **Run 24's notes** (collections `run24`, `plans`): every plan kept (the road, the polish, the guide and its
  outline, the video's stack and shots with creative freedom, the voices pre-approved: stop at the samples; the
  YouTube title the third); fix: Play rising while asleep (wake them instead), the wanderer gliding near the
  Trailhead, the cast's straight line, Tam fishing on dry land, the Lilyfin's floating legs, hats flat on its
  pad, the Puffback's straw brim. Answers: the conduct contact is Noah's email; Suno and ElevenLabs (Starter)
  both allow commercial use; the history check.
- **0.10.1** (D143): `DenBehavior::wake`/`awake`/`awakeFor` (60 s after the last care at bedtime) and the care
  screen gated on `awake()`; circling dragons flap then glide; the cast's bobber from the rod's tip; Tam at the
  water's edge (`fishing::coveSpots`), his line 4.2 m out; the Lilyfin's grown shoulders and hips raised and
  filled (re-exported); `padSeat` (only the Lilyfin's forms), a head height map so brims rest on the skull,
  `headWide` for the straw sunhat. Tests 377,652 checks, 0 failures (new: woken at night, Tam's line in the
  water, only the Lilyfin on a pad); autotests `hatsfit_h/g`, `lilyfin` (new), `tour`, `cove_cast` clean.
- **The new banner** (D144, `docs/tech/banner-labs.md`): `tools/blender/banner3d.py --kind <kit kind>` at full
  detail, smooth, 256 skin, one-sided solids, tidy names, the egg's insides dropped, `--outline` (ink);
  `tools\banner_variants.ps1`. Labs 25A-25F (0xEC168-0xEC16D): Pouncer, Blazeplume, Crestwing, Puffback with
  ink; Blazeplume, Pouncer without; 448-509 KB. The game keeps X's banner till Noah picks.
- **The open-source answers:** `CODE_OF_CONDUCT.md` has Noah's email; `LICENSE-MUSIC.md` records the Suno and
  ElevenLabs terms (and lets videos and streams use the music); the history: 377 commits, clean.
- **Sent** to **.51** (CIA 75 MB, 0.10.1; the six lab CIAs in `/cias/lab/`; the save backed up to
  `build/3ds-backup/2026-10-02_0116/`; run 24's logs: no freeze, `hangs.txt`'s "bottom" a session closed
  mid-frame). Run 25's steps (`docs/plan/hardware-check-10.md`) on the review page (version 19, collection
  `run25`, the banners' cards in `plans`).
- **Came back** (run 25): see run 26 above.

## Now: run 24 (0.10.0), run 23's fixes, and the plans for 1.0 (2026-10-01)
- **Run 23's notes** (collections `run23`, `looks`): the Trailhead held; no stutter; all 19 looks passed
  (Solenne as is). Fix: text under the mailbox list's Close; the sign by the Trailhead's gate drawn over the
  dragons; the Puffback's hat clipping in the Wanderings (fine in the den and valley); the den's heads flipping
  when a dragon faces away; the hats floating off the heads. Its hitch log: 24 long frames in ~20 minutes, all at
  scene loads; the watchdog's entries are 0.9.14's (run 22).
- **0.10.0** (D141; a CIA's micro version stops at 15, so not 0.9.16): hats seated on the skull
  (`core/wear_fit.cpp`: spots forward to back, re-seated, 100-70%, tipped back to 30 degrees; a narrow-things
  seat `headNarrow` for crowns, tiaras, circlets, party hats and feather crests; the far model wears the near
  model's fit; lifts now 0-0.27, most 0, from up to 0.52); `applyLookAt` eases back to straight ahead from 100 to
  160 degrees round; `settleDepth` (a throwaway draw before the first draw after each `bindDragons`, D118's
  fault) for the signs, mailbox, boards, shells and showcases; the mailbox list's count moved. Tests 377,534
  checks, 0 failures; autotests `hatsfit_h`, `hatsfit_g` (new, `tools/autotest_gen/hats_fit.py`), `wander_hat`,
  `mailbox_list`, `tour`, `story1` clean.
- **Open source** (D142, R2): `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`, `.github/ISSUE_TEMPLATE/`
  (bug report, idea, config), `.github/PULL_REQUEST_TEMPLATE.md`; the README's Contributing section.
- **The plans** (D142): `docs/plan/release-1.0.md` (R1-R7, gates), `docs/plan/guide.md` (A5 handbook, outline,
  spoiler rules, Markdown to PDF by headless Chrome), `docs/plan/trailer.md` (a film build captured in headless
  Azahar, a `seek(t)` page for titles, ffmpeg, the game's music, voices sampled and approved before any narration,
  17 shots, the YouTube title and description). All on the review page to Keep or Change (collection `plans`),
  with the hats on every kind and the open questions.
- **Sent** to .59 (CIA 75 MB, 0.10.0; the save backed up to `build/3ds-backup/2026-10-01_2229/`); run 24's steps
  (`docs/plan/hardware-check-9.md`) on the review page (version 18, collection `run24`).
- **Came back** (run 24): the den's heads and the sign held, every plan kept; see run 25 above.

## Now: run 23 (0.9.15), run 22's fixes (2026-10-01)
- **Run 22's notes** (the review page, collections `run22`, `looks`): the banner's title cut off by the
  HOME Menu's rounded corners; a mailbox sound; the Trailhead froze again (watchdog: the Wanderings' first
  frame, its top screen sent and never drawn, the music going); no stutter otherwise; six looks failed
  (Rowan's see-through forehead; gaps between hat and head on Bram, Fig, Tam; Fig's hat and hair; Tam's
  brow-to-beard band; Tove's hair through her hat; Solenne after Emilia, an elf).
- **0.9.15** (D140): the trail painting's NaN corners (the freeze's likely cause: Azahar skips them, the
  3DS's GPU can hang on them) held to the trail's end; the banner's wordmark redone (Emberclutch back at
  7.9 on its own top half, Skyreach Valley under it, the hatchling and egg at 84%); `mailbox-chime`; the six
  people fixed (heads closed under hats, hands on two rings, Solenne after Emilia); portraits. Tests
  377,419 checks, 0 failures; the trailhead, storybook and pets autotests clean.
- **Sent** to .59 (CIA 75 MB, 0.9.15; the save backed up to `build/3ds-backup/2026-10-01_1839/`); run 23's
  steps (`docs/plan/hardware-check-8.md`) on the review page (collection `run23`; the looks re-markable).
- **Came back** (run 23): the Trailhead held, the six looks passed; see run 24 above.

## Now: the Living Valley pass, ready for run 22 (0.9.14) (2026-09-30)
- **Built** (D135-D139, commits 69ada27 on): the player build (`tools\build.ps1 -Player`, kept building,
  not handed out); growth in hours (egg 36 h, Adult at 132 h and 12 stars); **the story engine** (story
  scripts in `story/*.story`, compiled by `tools/story/build_story.py`; quests that follow the world;
  talks as rules; the mailbox and 28 letters; save v2 with the v1 campaign migrated; the guide anywhere;
  Act 1 rebuilt with Fig, Custard and Cinder; the pageant, league, Hollow and cove lines; Old Whiskers; the
  Frost Warden); **the Storybook look on all 19 people** with **the feelings kit** (10 eyes, 10 mouths, 4
  brows; a face per feeling; 16 feeling clips; the speaker's feeling on their face, in their body and over
  their head; five portraits each in `romfs/portraits`); the level cap at 42; the badge case (five on the
  Record page); Primrose and Duchess at each league's last show; Custard in the meadow, Cinder on the porch;
  the grown cats' curl lies down (D139, Noah's note on Cinder).
- **Checks:** host tests 377,419 checks, 0 failures (the story bot plays all 28 quests; every story spot
  and pickup on dry land clear of walls); headless autotests: `tour`, `story1`, `storybook` (everyone,
  faces, emotes), `pets` (Cinder, Custard), `rival` (Primrose's show, the badges), `creator`, `battle`,
  `glade`, `cove`, `hollowfloor`, all without unmapped accesses.
- **Tools:** `tools/people/people.py` + `faces.py` (the bodies and faces), `gen_looks.py` (the people's
  tables from the builders), `make_portraits.py` (romfs portraits), `people_model.py --sheets
  lineup,feelings,portraits`; `dragonkit/curl.py --views`.
- **Romfs gotcha:** `make` only re-packs romfs when the program re-links; after changing only romfs files,
  delete `emberclutch.3dsx` first (or touch a source).
- **Sent (2026-10-01):** 0.9.14 to the 3DS at **192.168.68.59** (`/cias/emberclutch.cia` 75 MB and the
  3DSX, sizes checked; trace.on kept). Its save backed up first to `build/3ds-backup/2026-10-01_1708/`
  (v1, 2,200 bytes: Red, 7 dragons, 6 lanterns); decoded and migrated on the PC first: market_day and
  hilltop done, cold_heights and trailhead active, 5 letters waiting.
- **Next:** Noah: install 0.9.14 and play run 22 (`docs/plan/hardware-check-7.md`, the review page). Then
  his notes, the people's looks he fails, and Act 2 threads (docs/design/story.md section 10).

## Now: run 21, take 4 with 0.9.13 (2026-09-30)
- **0.9.12 on the 3DS:** the flicker gone (3 flashing frames in 32,819); 3D flicker on every door and
  menu (the tracer's split frames sent a half-drawn eye); the Trailhead froze again.
- **0.9.13** (D119-D134): the name (splash, title, credits for Noah and Emi, SMDH, the 3D banner's
  wordmark, the photo frame); `split()` keeps each screen's `used` flag; the watchdog (`watchdog.txt`
  when frames stop for 3 s); the music can't spin on bad reads; the Wanderings' 3D from the 4th frame;
  swimming for you; tree rush and hurt sounds; flight by Stamina and Wing; making way for the ball; the
  pageant's judges' shot, lead and one lantern string; hats clear the eyes on every kind, tail things
  outside the tail; not alone ("It's dangerous to go alone!"); 20 Gleam shells and the den's Gleam badge;
  the Stone without trust, spaced; the Curlstone walks unless ridden; Linnet's stall back in place; the
  cold heights' glide; the picnic's letter; the rod in your hands and the cast. Tests 375,879 checks, 0
  failures. Sent to .51 (CIA 73.5 MB, 3D banner), trace.on kept, the last trace pulled first.
- **The review page:** 0.9.13's steps 25-42 in section 0, and **the dragons**: every kind as a
  hatchling and grown (`tests/autotest/dragons_hatchling.txt`, `dragons_grown.txt`, the wardrobe's
  whole view), pass or fail with a note (collection `dragons`).
- **The people, two candidate sets** (`tools/people/candidates.py`, the same kit, skeleton and clips,
  588-600 triangles each): Set 1 "Villager" (Animal Crossing-like: head about half the height, dot eyes;
  its short arms would need the wave and cheer raised) and Set 2 "Storybook" (about three heads, irised
  eyes, a keeper's capelet and satchel; a braid, pinafore and egg pouch). Renders:
  `blender -b -P tools/blender/people_model.py -- --candidates --out <abs>/build/people_cand --sheets candidates --res 400`;
  on the review page's People section (Noah's pick in collection `people`, doc `choice`).
- **The story map** (questlines and side lines as a flowchart, open threads for expanding):
  https://claude.ai/artifact/AxoiYqNWSykCUm2epuu2wj
- **Next:** Noah: steps 25-42, the dragons' marks, the people pick. If the Trailhead freezes, pull
  `watchdog.txt` (a music spin or the GPU) with trace.txt. Then 1.0's dragon fixes from the fails, and
  the chosen people set into people.py (players first, then the villagers in its style).

## Run 21, take 4 with 0.9.12 (2026-09-30)
- **0.9.11 on the 3DS** (trace, 20 screenshots): no hang in the valley; holes 11% of valley frames with
  way 6, the first ground tile alone losing its depth (12 probes of one view by the den's door); the
  battle's bars and win card flashed; froze going into the Wanderers' Trailhead (the switched frame,
  unchecked). Noah asked about renaming the game Skyreach / Skyreach Valley (answered: his call; the
  title ID and save folder can stay).
- **0.9.12** (D118): a throwaway draw takes the ground's batch (the smallest tile first) and each
  3D-to-2D hand-over's (two see-through pixels); the tracer checks a switched scene from its first
  frame, and counts holes by frame parity. Sent to .51, trace.on kept.
- **Next:** Noah: steps 22-24. Pull trace.txt: holes (by parity), probes' tile 0, and any "gpu: ...
  sent" at the end of a frozen session's trail.

## Run 21, take 4 with 0.9.11 (2026-09-30)
- **0.9.10 on the 3DS:** froze twice soon after leaving the den (no crash dump). Trails: the GPU never
  finished the bottom screen in the third valley frame; then a hang around the reference probe (valley
  frame 200), the register reads' first use.
- **0.9.11** (D117): the resend before each ground tile only (0.9.9's way 6, byte for byte), trace or
  not; no register reads, no window. The emulator caught a slip first (the resend inside
  drawValleyGpu forced depth writes on the far haze disc). Photos, music buffers, free camera and
  swimming kept. Sent to .51, trace.on kept, hangs.txt cleared.
- **Next:** Noah: step 21 (den to valley a few times, 5+ minutes). If it freezes, pull trace.txt and
  trace-prev.txt (the last "gpu: ... sent"). If it holds, the remaining ~1% flicker: find what else to
  re-send, one draw kind at a time, each tried on the 3DS.

## Run 21, take 4 with 0.9.10 (2026-09-30)
- **0.9.9 on the 3DS** (trace): the trial's holes by way 74%, 66% (untextured), 84%, 85%, 42%, 75%,
  and 0/1620 for the depth test per tile; kept, 1.2% after. Noah: much better everywhere, not quite
  perfect; untextured flickered too; photos in the den stopped the music and held the game.
- **0.9.10** (D116): `resendEffect()` before every 3D draw, trace or not; the trial taken out; with the
  trace on, a window the old way (up to 2,000 valley frames or 10 probes) whose probes read the GPU's
  own registers after each part and tile (GSPGPU_ReadHWRegs), then the fix. Photos in pieces, a deeper
  music buffer, one folder read for the number; the free camera within 100 m / 60 m up; swimming
  quieter. Sent to .51, trace.on kept.
- **Next:** pull trace.txt: the "GPU regs" lines of the probes in the window (fix 0) against the
  reference (valley frame 200) and any later probes (fix 1), and the "tiles:" lines' 0x107/0x115 per
  tile. Noah: steps 17-20 in hardware-check-6.

## Run 21, take 4 with 0.9.9 (2026-09-30)
- **0.9.8 on the 3DS** (70 screenshots, trace): buildings blinking out; the flicker better for a minute,
  then back by the den; the view's angle starts and stops it. The trace: textured tiles write no depth
  in the flicker's frames (every probe), untextured ones do; the first tile's commands the same in good
  and bad frames; the 60-frame turns muddied the counts.
- **0.9.9** (D115): seven ways of drawing the ground in 600-frame turns (as drawn, untextured, bound
  early, small coordinates, depth first, no mipmaps, depth test per tile), the best kept; buildings
  held 15 m past the haze's reach. Sent to .51, trace.on kept.
- **Next:** Noah plays the valley 6+ minutes (the den's door too). Pull trace.txt: "depth trial done:
  holes by way ..." names the way that works; make it the build's own (trace or not) and retire the
  trial.

## Run 21, take 4 with 0.9.8 (2026-09-30)
- **0.9.7 on the 3DS** (80 screenshots, trace): the Hollow's cave and ring floated; the flicker still
  there, even flying at ~3,000 triangles. The trace: the haze's three ways alike (25%, 18%, 18% of
  frames); the probes: the ground's tiles write no depth in the flicker's frames, the places after
  them do.
- **0.9.8** (D114): the ground trial (as drawn / untextured / depth test per tile / places first /
  one colour loader), the first tile's commands logged, per-tile probes; the Hollow's room held at its
  floor after the river's cut, the Hollow rebuilt (a test). Sent to .51, trace.on kept.
- **Next:** Noah plays the valley (the den's door first) 5+ minutes with the trace on. Pull trace.txt:
  "depth trial done: holes by fix ..." (which way works), "commands (a hole ...)" against "commands
  (a whole frame ...)" (what differs in what the GPU is sent), and the "tiles:" lines.

## Run 21, take 4 with 0.9.7 (2026-09-30)
- **0.9.6 on the 3DS** (db `run21d` s0, 90 screenshots, trace): the flicker less but still there
  ("large sections of the ground flicker away"); depth holes in 619 of ~16,900 valley frames with
  no fill: every still thing without depth, the dragons' draws with theirs. Also: the roamers move
  in bursts; Gale and Frost the same blue; the XP card should fill; flap while bursting; the
  Crestwing's rider too far back (walking speed fixed).
- **0.9.7** (D113): the depth trial (fix 0 as before, 1 the haze's test left on + a framebuffer
  flush, 2 left on + a split; 6,000 valley frames, holes per fix, then the best kept), probes after
  holes (the depth after each part), depth maps; the roamers' smooth clock; seats forward (Crestwing,
  Flurrytail) and their height from the body's triangles; the XP bar filling; wingbeats in a burst;
  Gale seafoam. Sent to .51, trace.on kept.
- **Next:** Noah plays the valley 5+ minutes with the trace on. Pull trace.txt: "depth trial done:
  holes by fix a/b c/d e/f" says which fix works; the "probe" lines say after which part the
  depth goes if none does.

## Run 21, take 4 with 0.9.6 (2026-09-29)
- **0.9.5 on the 3DS** (Noah, still testing, notes to come on the page): the ground still flickered
  in and out showing blue; the den's entrance still had ground over its floor; L/R in flight banked.
  The trace's blip watch logged 635 blips in about three minutes of valley, alternating frame by frame between
  the green ground (163, 172, 107) and teal (101, 151, 155), their triangles and draws the same.
- **Diagnosis** (D112): the teal is the lake's water over the ground (the colour matches to a unit);
  the statics' depth wiped by the screen's clear, a GX memory fill the GSP runs beside the frame's
  command list (libctru submits three at once); the dragons, drawn later, kept theirs.
- **0.9.6:** each screen cleared by a quad drawn first (`r3d::clearScreen`); the trace's depth watch
  (holes counted); the den's floor over the ground (`den_pad`, a test); R burst and L brake in flight.
  Sent to .51 with trace.on kept, the old trace off.
- **Next:** Noah goes on with take 4. Pull trace.txt: "depth holes 0 of N" in its 5-second lines and
  no "blip" lines means the flicker is gone; holes would mean the depth still goes some other way.

## Run 21, take 4 with 0.9.5 (2026-09-29)
- **Take 4 (0.9.4)'s first notes** (db `run21d` s0): the 2D glitch gone (every bottom screen whole in
  ~35 screenshots); the ground alone going teal in odd frames (0269, 0285, 0289: fogged ~75% toward the
  fog colour, strokes still there, the dragon untouched); the Crestwing crawling (its walk measured 0);
  the rider inside it; big dragons wedged in the den.
- **0.9.5** (D111): the valley on one program; locomotionSpeed by each foot's sweep (every kind tested);
  a 1.8 m/s walking floor; the seat's wider window; the den's spacing for big dragons (tested); the
  trace's blip watch. Sent to .51 with trace.on kept, the old trace off.
- **Next:** Noah goes on with take 4. If the ground still flickers, pull trace.txt: its "blip" lines
  say which frames, with their scene, triangles and draws.

## Run 21, take 4 (2026-09-29)
- **Take 3 (0.9.3)** held in the valley (the ground's shader rewrite); one freeze in 29 minutes, on the
  bottom screen's checkpoint (hangs.txt: "bottom"; no fallback for it). Noah's notes (db `run21`, s0)
  and ~80 screenshots, 6 photos (pulled read-only to the session's scratchpad).
- **The flicker, found:** frames where every citro2d solid shape is gone or black on both screens while
  text and images stay (run 19's screenshots too), with 3D dropping out: stale GPU data. 0.9.4 flushes
  each frame's command list and 2D buffers, resets the 2D state per screen, reports the heap flush and
  the 2D buffer on the overlay, guards citro2d's text. Noah's haze: a fog pull with the load (D109).
- **Everything else in the notes** fixed (D109; new autotests: take4, getoff, lantern, keeper, crestlie,
  flicker2d). Passed in take 3: Love/Energy, profile, tips, painted ground, fireflies, Journal and
  tracking, pins, the fox, Tamsin, voices, photos, the den's speed.
- **Take 4** = hardware-check-6.md; sent to .51 with trace.on kept, the old trace and hangs list off.
- **Next:** Noah's take 4 results (db `run21d`). If the flicker is gone, the flushes were it; if the
  overlay says `flush ERR`, citro3d's heap flush fails on the hardware.

## Run 21, take 3 (2026-09-29)
- **0.9.2 held in the den** (Continue with an egg, hatching, care); the valley froze on its first
  frame, from Map and from heading out (trace: "gpu: top scene sent", never drawn). The title's and
  the den's slowness was the trace itself (per-frame lines for 10 s after each change of view).
- **0.9.3** (D108): the ground's shader writes its texture output whole (one `mov`, as the shaders
  that run on the 3DS do); the valley's parts are GPU checkpoints; a part a session froze in goes
  in sdmc:/3ds/emberclutch/hangs.txt at the next start, and the ground then draws plain (checked in
  the emulator with a made-up trail); the trace keeps marks for 3 frames a view, a line every 5 s,
  and the session before as trace-prev.txt. Noah's notes fixed: X with only an egg, the sleeping
  dragon's word, the Dragondex's young entries (core `dexStage`, tested), the Journal's goals.
  Sent to .51 (sdmc:/cias/emberclutch.cia), trace.on kept, the old trace and hangs list removed.
- **Next:** pull trace.txt, trace-prev.txt and hangs.txt (read-only) after Noah's try. If the
  ground hung again, the fallback worked around it: find another way to paint the ground (UVs in
  the vertex, or none). Once the hardware holds, delete hangs.txt and trace.on on the card.
- **Azahar 2126.1.2**: the checked installer is in Noah's Downloads (winget still has 2126.1.1;
  admin rights needed, so Noah runs it); afterwards check qt-config.ini's old-3DS lines.

## Run 21, take 2 (2026-09-29)
- **Run 21 (0.9.1) froze again** at Continue, and the title crawled (the trace wrote a line to the
  card per mark). Its trace (pulled read-only): the den's first frame was all sent (room, particles,
  the egg, overlays, bottom), then the next frame's begin never returned: the GPU hung on it.
- **0.9.2** (D107): the static program is run 19's again (no texture output); the valley ground has
  its own program (`ground.v.pica`). With trace.on, GPU checkpoints for each scene's first 3 frames
  name the part that hangs ("gpu: <part> sent" with no "drawn"); the trace writes in batches.
  Checked in the emulator (old 3DS): Continue into the den with the checkpoints on, and the valley's
  ground still painted (ground.txt). Sent to .53 (sdmc:/cias/emberclutch.cia), trace.on kept.
- **If it freezes again:** pull trace.txt (read-only FTP) and read the last "gpu:" line; the part
  named is what hangs. If it holds: run 21's steps go on as written (hardware-check-5.md).
- Azahar to be updated (Noah, 2026-09-29), then set to an old 3DS again.

## Run 21 (2026-09-29)
- **Run 20's results** (the long-run review page, db `run`/`labs`): the game froze at Continue and after
  a new game's egg (D106, fixed); nothing else could be tried. Banner labs: H (names + paint) and A2
  (lab A, fresh ID) froze, F, G, I, J, K held ([banner-labs.md](tech/banner-labs.md): the next round
  when Noah asks). Sound picks applied (the rabbit's hop take 2, the flock take 1).
- **Run 21** (0.9.1) = run 20's steps again ([hardware-check-5.md](plan/hardware-check-5.md)); sent to
  .53 (sdmc:/cias/emberclutch.cia) with sdmc:/3ds/emberclutch/trace.on; run 20's labs removed from
  the card. If it freezes: pull trace.txt (read-only FTP) and read where it stopped.
- **The emulator** now runs as an old 3DS (Azahar's `is_new_3ds\default=false`, `is_new_3ds=false`
  in qt-config.ini; the old settings are backed up in the session's scratchpad).

## Run 20 (2026-09-29)
- **For Noah:** the review page above: run 20's steps (tick as you go, notes per section, the banner labs
  Held/Froze), the sounds as kept (round 2: mark only what's still off, and the two made at his note), the six
  loops' seams to hear. Build: `emberclutch.cia` (0.9.0, 73 MB) and `build/lab/banner-lab-{f..k,a2}.cia`,
  **sent to the 3DS at .53 on 2026-09-29** (sdmc:/cias/emberclutch.cia, sdmc:/cias/lab/; run 19's tested labs
  B-E removed from the card).
- **Since the last update:** the chest heart laid on every kind's chest (D104, the kit's `conform_part`, all 14
  kinds re-exported); the camera kept clear of the places and the ground whatever sets it (core/occluders, the
  grotto); the grotto's chest; the stage 7.2 m; the Hollow's camera and brazier; seen while fishing; the foe's
  bars at the top; D merged; the critters' and duels' sounds; the sound review applied (D105); the music in
  and the Performance on show-stage's beat; a whole-game playthrough script; README screenshots.
- **Tests:** 374,858 PC checks, 0 failures; every autotest (playthrough, battle, glade, critters, roamers, cove,
  Hollow, pages, valleyperf, people_anims, cavecam) without unmapped accesses.
- **After run 20:** fixes from Noah's notes, then `v1.0.0` and the public release only on his word (the GitHub
  release, the Universal-DB entry in `docs/release/`).

## The long run (2026-09-28)
- **Foundation** (on main first): Love and Energy apart, a trainer's record per dragon (xp, trained
  stats, moves, wear, dye, titles, wins, cups, ribbons, deepest floor), the save's progress block
  (accessories, dyes, leagues, the Hollow, the day's claims, tips, records, the cove's day), four
  new places' ground and ids, 31 sound slots, `app/valley_ext` (features that stand people in the
  valley and take it over), custom speakers in the dialogue box. Save buffers 64 KB.
- **Workstreams merged** (Opus 5.5 in worktrees, [v1-work.md](plan/v1-work.md)): A the places
  (Emberpeak Caldera, Moonpetal Glade, Driftwood Cove, Frostspire Hollow, named anchors, run 19's
  model fixes); B battles (core/battle, core/league, core/hollow, app/battle_view: turn-based 1v1,
  the Ember to Starfire league, finals on the caldera ring, the Hollow's 30 floors); C the
  challenges retuned (once-a-day prizes, trophies, Sky Rings races with rivals, burst and brake,
  Fruit Catch by stats) and fishing at the cove; P the pageant (32 accessories on every kind, 13
  dyes, themed shows, the wardrobe); U the interface (storybook Market and Wanderings, Love and
  Energy gauges, the profile's training and record pages, the Journal's tracked goal with a map
  marker, tips, settings); S sounds (31 synthesised effects, five beds, a channel pool).
- **Lead's work:** the flicker (D95), run 19's fixes (D96-D98: islands and decks stood on, graded
  and forded paths, mountainsides not climbed, X for the Journal and the Outing, travel asks,
  home at the den's door, framed photos from the free camera, the hop on and off, eggs' cracks,
  renewing finds, the creator's controls, walking together, feeding out, textured ground,
  fireflies and falling leaves, far trees, the new women's voice D92), release prep (player
  README, CREDITS, THIRD-PARTY-NOTICES, `docs/release/universal-db.json`, `tools/release/make_qr.py`;
  nothing published).
- **The look (D99):** Noah picked A in the look lab; the ground's texture now has two channels
  (grass strokes; earth, path, rock and bark speckle) mixed per vertex. C (`ground 2`) is the
  fallback if the 3DS can't carry it.
- **Since the merges:** the integration pass (tips wait only while a battle, show or race plays;
  the battle's tip at "Battle?"); the Hollow's camera (a wall across the screen: the eye by an ice
  spire; `bview::viewClear`, the Hollow's nearer camera); prizes nothing gave (the Hollow's and the
  found things to wear, a prize dye per league final); the league tracker on the next challenger;
  new music taking over as its files arrive; the ground's detail following the triangle budget
  (D102); the glade's stage 7.2 m round for three dragons (Noah); the camera behind you after a
  battle; the whole-game playthrough script and the README's screenshots.
- **Valley life (D101):** L merged (seven critters with an A moment each, the Journal's page, one
  draw, 0-136 triangles); D merged (below).
- **Workstream D (roaming trainers, duels, people's doings; D103):** `core/roamers` (eight trainers,
  3-5 out a day on the paths' network, a fair duel, a little Gleam a day each), `app/feature_roamers`,
  `core/routines` + `app/people_acts` (villagers by the hour), 17 new people's clips, battles' people
  react, Tam fishes, trainers stop to watch and clap; autotests `roamers.txt`, `people_anims.txt`. A roamer
  encounter view ~10.1k triangles with a place in sight (the trainer adds ~1.7k: their dragon on its
  light model ~1.1k, the trainer ~0.6k); talking ~9.1k; a duel ~10k (as the league's battles).
- **Sounds and music (D100):** ElevenLabs by API (`tools/audio/eleven_sfx.py`): batch 4 (24 foley
  and beds) and the critters' 9; the Suno brief batch 4 (six tracks). All on the review page:
  https://claude.ai/artifact/52Kf8yemiqCHkq3wQKnxoW (`tools/review/make_review.py`; db `sounds/<slug>`
  and `music/<slug>`).
- **Run 20 (0.9.0):** steps drafted in [hardware-check-4.md](plan/hardware-check-4.md) (duels to
  add); banner labs F-K and A2 built (`build/lab/`, [banner-labs.md](tech/banner-labs.md)).
- **Budgets:** most views under ~9.6k triangles (the ground's detail adapts); the Market ~10.6k and
  the shows up to ~12.7k are run 20's frame checks.
- **Tests:** 369,831 PC checks, 0 failures; every autotest run without unmapped accesses.

## Where things stand
- **Design** complete for v1: [GDD](design/game-design.md), [breeds & genetics](design/breeds-and-genetics.md),
  [theme](design/theme-and-art-direction.md), [screens & flow](design/screens-and-flow.md).
  Decisions D1–D71 recorded ([log](plan/decisions.md)); open: the grooming design (Beta).
- **Plan:** [roadmap](plan/roadmap.md) (milestones A1 → 1.0 → 2.0), [content & assets](plan/content-and-assets.md),
  [Alpha 1 plan](plan/alpha-1.md), [Alpha 2 plan](plan/alpha-2.md) (draft: the style review
  R5, the emblem icon and a 3D HOME Menu banner, D47–D50). New specs (2026-09-24): [hands-on care](design/care-interactions.md)
  (the *Nintendogs*-style polish for WP7) and [world map & travel](design/world-map-and-travel.md)
  (fast travel in Alpha 2, free flight in 1.0).
- **Code:** `src/core` (genetics, needs, mood, growth, eggs, clock, breeding, save, model
  format, skeleton/rig, per-dragon mesh assembly, animation, den behavior, den room,
  daylight, particles, items and the den's props) and the dragon kinds (nine, each its own model and body plan) with PC tests (182,810 checks). `src/app` draws the dragons in 3D
  (skinned toon shader) in a 3D den room lit for the time of day, inside themed citro2d
  screens. Runs in Azahar at 60 fps in the den (2026-09-24), sounds and all.
- **Art:** two dragon forms built by script (`tools/blender/dragon_model.py`): a metaball
  hatchling and the skin-modifier grown body, classic wings, part variants, an opening
  mouth with teeth and a tongue (D41), blinking eyes (D42). Exported to `romfs/models/{hatchling,grown}.ecm`.
- **Audio:** music batch 1 processed into `romfs/music/` (6 loops); batch 2 (the hatching and
  Wanderings stingers, the Market loop) and the full Alpha 1 sound-effect set (ElevenLabs,
  2–4 takes per sound) processed on 2026-09-24. The den has hearth and night beds and an
  egg hum under the music.
- **Hardware:** run 1 on Noah's old 3DS (2026-09-24, 0.1.1): the 3D banner plays; the game
  didn't start (no boot logo in the CIA). Run 2 (0.1.2): it starts, the stylus is accurate,
  screenshots work; the 3D-banner CIA froze the HOME Menu (a flag 0.1.2 dropped), and the
  game crashed at its first 3D frame (`C3D_TexBind(1, nullptr)`: a null read that Azahar
  lets pass). Both fixed in 0.1.3; every scripted run now checks for unmapped accesses (D59).
  Run 3 (0.1.3): it plays on the hardware; the full den runs 22–23 ms (a performance pass is
  planned, WP11d); Next style froze the 3DS (GPU memory freed while in use: fixed); the 3D
  banner still freezes the HOME Menu, which caches banners per title (banner-lab titles
  next). Run 4 next.

## Alpha 1 progress
- ✅ **WP1 engine foundation:** scenes split into `src/app/scene_*.cpp`, string table,
  romfs enabled (music packed in, 7 MB `.3dsx`), budget overlay (frame/CPU/GPU ms,
  command buffer, triangles, draw calls, bones, memory, romfs check), dev menu on
  SELECT (time skip, needs, hatch, next stage, save, reset). Verified in Azahar.
- ⏸ **WP2 dragon model:** sculpt **approved** after R1 → R1b → R1c (D36–D39: baby body,
  classic wings, strong breed shapes, nostrils and mouth, bigger late hatchling). **The egg
  is done:** a 3D egg (948 triangles) in the egg nest and up close on the bottom screen; it
  rocks when rubbed, the dragon inside knocks near hatching, the light inside brightens
  with warmth, and three stages of glowing cracks appear in the last stretch (sounds on
  each). **Cuteness pass** (Noah, 2026-09-24): bird-like folded wings with membranes that
  follow the fingers, the chest heart clear of the body, an opening mouth with teeth and a
  tongue (D41), a centred puppy tail wag, blinking eyes that shut in sleep (D42), the
  walking limp fixed. **Textured** (review [R2](art/reviews/R2-textures.md) sent, D51): a baked
  skin per body form with scale detail and the Pattern gene, and visible dust per body region
  (D46). In-game texture check in Azahar still to do.
- ✅ **WP4 renderer:** `dragon.v.pica` (2-bone skinning, palette colours, fragment-light
  outputs) and `render3d` (toon ramp + rim + emissive heartglow with a white-hot core;
  citro3d inside citro2d scenes). Den camera, a bottom-screen petting close-up, per-dragon
  caches, LOD1 for background dragons. Verified in Azahar: hatchling, juvenile, adult;
  three adults at LOD0 were 8,368 triangles, so LOD1 was added (~4,850 expected;
  **confirm in the emulator**). The static-mesh path came with WP6.
- ✅ **WP3 model pipeline:** `export_dragon.py` writes `.ecm` (skeleton, growth/build tables,
  body, wing and part variants baked at 4 growth keys, vertex paint). `src/core/model.cpp`
  loads it; `skeleton.cpp` + `rig.cpp` reproduce Blender's deformation. PC tests: parity for
  both forms (body < 0.001, wings < 0.006, parts exact), a 3,000-triangle worst-case budget,
  and the stage → form mapping.

- ✅ **WP5 animation and behavior:** 32 clips authored in Python (`tools/anim/clips.py`,
  pitch/yaw/roll deltas in armature axes on top of the idle pose) → `romfs/anims/dragon.eca`.
  Runtime animator with crossfades and events (sounds), per-frame floor contact, head
  look-at, and a den behavior state machine (everyday life by mood/personality/energy, naps
  and night sleep at the nest, sulking in the nook, care reactions). Walking speed is
  measured from each body's stride (no skating). Dev menu "Next activity" reaches every
  state. PC tests: 57,381 checks. **R3 sent** (contact sheets). Not yet seen in the
  emulator (paused).

- ✅ **WP6 den scene:** a round cave built by `tools/blender/den_model.py` (2,251 triangles:
  rug, sleeping nest, egg nest, hearth with flames, hoard, shelves, sulk nook, skylight
  with a sunbeam) as a cutaway diorama (D40), with vertex lighting baked for day, evening
  and night and blended by the clock; the dragons' light follows the time of day and the
  room's light where they stand. `static.v.pica` + `.esm` loader; particles (embers, motes,
  glints, hearts, Zzz, crumbs, sparkles, dust). Dragons now walk around the hearth, egg nest
  and hoard. The egg sits in the egg nest. **Review R4 sent**
  ([den](art/reviews/R4-den.md)). Builds clean; **not yet seen in the emulator** (paused).

- ✅ **WP8 save system:** versioned A/B slots, CRC32, per-record sizes, validation,
  legacy dev-save import; 5 new PC tests (22 total, 11,383 checks). In Azahar: slots
  alternate, a corrupted newest slot falls back to the older one and is then rewritten.

- ✅ **WP9 audio:** Tremor Ogg streaming on a worker thread with sample-accurate loops
  (LOOPSTART tag), fades between tracks, stingers that duck the loop, 8-channel sound
  effects with takes and per-dragon voice pitch, looping den beds (all queued in short
  slices so Azahar keeps full speed); music director (title / den day / nestsong at night
  and during incubation). Placeholder SFX synthesized (D35; replaced by the real set on
  2026-09-24). Verified in Azahar (fixed a
  thread race that restarted the stream forever). `make_loop.py --no-loop` for stingers.
  Briefs sent: [music batch 2](audio/suno-music-batch-2.md), [SFX](audio/suno-sfx-alpha1.md).
  Note: the emulator needs `sdmc:/3ds/dspfirm.cdc`; a local dummy file works in Azahar (never commit it).

## Next actions
**Beta and 1.0 in one pass** ([plan](plan/v1.md), D89-D90). In order: run 19's fixes (the flicker
first); care and progression (Love, Energy, experience, moves, per-dragon records); turn-based
battles and their league (finals at Emberpeak Caldera); training at Frostspire Hollow; accessories
and themed shows at Moonpetal Glade; the valley's life (textured ground, particles, Driftwood Cove,
random finds, sounds); the interface (Market, Wanderings, Journal, tutorial); economy; then one
large review, run 20 and a small set of banner labs.

**Earlier:** **Alpha 1 is done** (2026-09-24, tag `v0.1.0-alpha1`): every item of its Definition of done
was checked in Azahar by scripted runs, see the [checklist](plan/alpha-1-checklist.md)
with contact sheets. In its last stretch: hands-on care (WP7: the tool tray, petting with a
sweet spot, hand-feeding, brushing and polishing, the bath, fetch; egg turning and
listening, the hatching, naming and renaming, D52), the UI (WP10: fonts, title, system
menu and settings, toasts, save icon) and the CIA (WP11: an interim icon and banner from
our own model). New tool: `tools/autotest.ps1` plays a script in Azahar with nobody at the
controls and saves screenshots of every step (`tests/autotest/`).

1. **Alpha 2** ([plan](plan/alpha-2.md)), the style-independent systems first. WP1
   ✅ (2026-09-24, D53): three dragons and two eggs in the den, switching who you care for,
   the room and egg trimmed to fit the frame; and their life together: games of chase,
   nuzzles, the sunbeam, two curled up in the big nest at night. WP2 ✅: the Sanctuary and
   the Cold Vault, reached through a first world map (X in the den). WP3 ✅: breeding at
   the Nesting Stone (the pair's egg comes the next day). WP4 ✅: the Wanderings (walk with
   a dragon, the pedometer counts, it finds Gleam, trinkets and sometimes a wild egg). WP5
   ✅: the Market (food for Gleam into the pouch, selling trinkets, the egg of the day). WP6
   ✅: the world map's trips (a heart travels the path from where you are; A skips). WP7 ✅:
   toys and decor from the Market (the feather, the rope, the puzzle orb, the food bowl;
   rugs, lanterns, perches, plants, banners), played with up close and on their own,
   tug-of-war included. WP8 ✅: the profile (about it, its stats and looks, its sweet spot
   and favourite once found, a three-generation family tree), also from the Sanctuary and
   the Vault. WP9 ✅: every sound in brief 2 has its slot and plays where it belongs, with
   a retuned stand-in until its file arrives. WP10 ✅: the emblem app icon and the animated 3D
   HOME Menu banner (the baby Ember in its cracked egg; 284 KB CGFX via pycgfx, rigid pieces
   only; packed with `package_cia.ps1 -Banner3D`, the flat banner stays the default until
   the old 3DS shows the 3D one). **R5 is built** ([review](art/reviews/R5-style.md)): the
   current look next to V1 surface, V2 shape and V3 bold (the ember-veined dragon), as sheets,
   turntables and in the game (dev menu page 2: Next style). **Waiting on Noah's choice.**
2. Then the emblem icon and the animated 3D HOME Menu banner (D48, D50).
3. Then **R5**, three Ember style variants for Noah (blocks, D47); the dragons update in
   the chosen style; the run on Noah's old 3DS (D34).

## Current goal (D31)
**Complete through Alpha 2.** Noah is hands-off until the style variants are ready (D49):
work continues package by package, committing, pushing and updating STATUS and RedWiki
after each. Stops:
- **R5, the style review** (D47): three Ember dragon variants next to the current textured
  style, plus the emblem icon. Blocks the dragons update.
- **The run on Noah's old 3DS** (D34), the last step of Alpha 2.
- R2, R3, R4 and R6 are sent without blocking. Fonts are pre-approved (D33). Sounds that
  haven't arrived use stand-ins (D35). Computer use only while testing in the emulator.

## Waiting on Noah
- **Run 19** (0.3.0 with the big review's fixes, D88, and banner labs B-E,
  [steps](plan/hardware-check-3.md)): on the 3DS at .54 (sdmc:/cias/emberclutch.cia and
  sdmc:/cias/lab/), sent 2026-09-26. The review ([page](https://claude.ai/artifact/6k6yYxfWQjPepJj2hsHuX4),
  [R13](art/reviews/R13-beta1.md)) is answered.
- **Sounds for the challenges** (stand-ins play): a countdown tick, a missed ring, a clean glass
  chime for the crystal lanterns, a soft fizzle for a wrong lantern, fruit bouncing on grass, an
  orchard crowd ([challenges](tech/challenges.md)). Batch 2 and 3's open sounds too, on the
  [Emberclutch Checklists](https://claude.ai/artifact/TLouY2VFEjKJb7YyqFQvAE).

## How to work
- Build: `tools\build.ps1` · Tests: `tools\test.ps1` · Emulator: `tools\emu.ps1`
- 3DS: `tools\package_cia.ps1 -Banner3D -Version x.y.z` · to the 3DS (the .3dsx, and the CIAs
  in `build/cia-test/` with `-Cia`): `tools\deploy_ftp.ps1 -FtpHost <3ds-ip> -Cia` · screenshots off the 3DS:
  `tools\pull_shots.ps1 -FtpHost <3ds-ip>` (Y in the game takes them)
- Music: `python tools/audio/make_loop.py <wav> --bpm <hint> --preview`
- Blender (headless): `blender -b -P tools/blender/<script>.py -- <args>`; model export:
  `export_dragon.py -- --out-dir romfs/models --reference-dir tests/data`
- Agent notes: [`CLAUDE.md`](../CLAUDE.md)
