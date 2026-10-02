# 1.0: tying it up (the short-term plan, 2026-10-01)

Noah, after run 23: *"So fixes, more polish up to 1.0 (including nailing down the new 3D banner), PDF
and md guide in repo, public release version, universal updater request, marketing/fun video
featuring the game, release on YT for fun with links (so plan title and desc). That's the tentative
short-term plan to tie this up before later polish or update/additions. Make sure the repo is set up
as MIT open source, with a standard for issues and contributions."*

This page is the order of work and its gates. The guide has its own plan ([guide.md](guide.md)) and
so does the video ([trailer.md](trailer.md)). D142 records the plan. **Anything public waits for
Noah's word**: making the repo public, the GitHub release, the Universal-DB request, the YouTube
upload.

## The order

| # | Step | What "done" is | Gate |
|---|------|----------------|------|
| R1 | **Fixes and polish to 1.0** | Hardware runs come back with nothing broken or confusing; the 1.0 list below is ticked | Noah calls 1.0 done |
| R2 | **Open source, set up** | MIT licence (code), CC BY-SA 4.0 (art), contribution standard: `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`, issue forms and a PR template in `.github/` | Done in 0.10.0 (D142); the open questions below |
| R3 | **The guide** (Markdown in the repo, and a PDF) | `docs/guide/guide.md` and `Emberclutch-Guide.pdf`, mechanics and the world, light on spoilers | Outline Keep/Change, then a draft read |
| R4 | **The public release** | `make DEV=0` build, `v1.0.0` tag, GitHub Release with the CIA, the 3DSX, the guide PDF, notes and FBI's QR | Noah: the repo public, then the release |
| R5 | **Universal Updater** | Universal-DB request sent from `docs/release/universal-db.json` | Noah's word (after R4: it reads the releases) |
| R6 | **The video** | A ~75 s trailer of the game from in-emulator camera shots, 1080p | Plan, storyboard, voices approved first |
| R7 | **YouTube** | Uploaded by Noah with the title, description, chapters and thumbnail prepared here | Noah uploads (after R4, so the links work) |

R3 and R6 can be drafted while R1's runs go on; R4 waits for R1, R5 and R7 wait for R4.

## R1: what's left for 1.0
*Run 30 (2026-10-02, D149): the banner is settled (28G, D148), the trace is off, the player build is on the
3DS as the 1.0.0 candidate, the release shots are retaken and the pass for clipped words found and fixed two.
What's left is Noah's run 30 and his call on 1.0.*
- **Hardware runs** until the notes are clean (run 24 is 0.10.0: run 23's fixes).
- **The 3D banner, nailed down.** Run 23 passed the layout (title clear of the corners, the hatchling
  and egg smaller). Next, per Noah ("we'll test other banners later with actual in-game hatchling
  textures"): two or three variants of the banner's hatchling wearing a kind's real baked skin (from
  `romfs/models/*_skin.t3x`'s sources), shown on the review page and then on the HOME Menu; the one
  picked becomes `tools/blender/banner3d.py`'s default and the release banner.
- **The player build checked on the 3DS** once before the release (no dev menu, no overlay, no trace;
  the watchdog kept, D138 "if it doesn't really hurt performance").
- **The trace off** for runs once the Trailhead has held twice.
- **Release shots** refreshed for the README and Universal-DB (`tests/autotest/release_shots.txt`,
  the Storybook people and 1.0's places).
- A last pass over every screen's text for clipped words (run 23 found one on the mailbox).

## R2: open source (done in 0.10.0)
- `LICENSE` (MIT, code) was already there; art is CC BY-SA 4.0 (`assets/LICENSE-ART.md`), music and
  generated sound effects their own terms (`assets/audio/music/LICENSE-MUSIC.md`). The README says so.
- Added: `CONTRIBUTING.md` (how to build, test and send changes; the house style; the asset rules: no
  AI-generated art shipped, D74; inbound = outbound licensing), `CODE_OF_CONDUCT.md` (adapted from the
  Contributor Covenant 2.1), `SECURITY.md`, `.github/ISSUE_TEMPLATE/` (bug report and idea forms, and a
  config pointing questions to the guide), `.github/PULL_REQUEST_TEMPLATE.md`.
- **Answered (2026-10-02):** the conduct contact is Noah's email (in `CODE_OF_CONDUCT.md`); Suno's paid
  plan lets Noah use his songs for anything and every paid ElevenLabs plan (his Starter) carries a
  commercial license, videos included (`LICENSE-MUSIC.md`, which now also lets videos and streams of
  the game use its music); the history is clean (all 377 commits: no keys, tokens, passwords or private
  files; the only addresses are the co-author line, the Cinzel font's designer in its licence, and the
  commits' author, Noah's email, which GitHub shows on a public repo). Still Noah's to do when it goes
  public: turn on private vulnerability reporting in the repo's settings.
- **The questions were** (kept for the record):
  1. A contact for conduct and security reports. GitHub's private vulnerability reporting needs
     turning on in the repo's settings when it goes public; for conduct reports, a contact address
     of your choosing (I haven't put any address in).
  2. Suno's and ElevenLabs' terms for the music and sound effects in a public repo and a YouTube
     video (your plans' terms decide it; `LICENSE-MUSIC.md` should say what they allow).
  3. Before the repo goes public: a look through the history for anything private (the tree is
     clean; old commits carry Noah's notes and the review pages' text).

## R4: the release checklist (when Noah says)
1. ~~Version 1.0.0 in the Makefile; `tools\package_cia.ps1 -Player` (the CIA, the 3DSX and the zip in
   `dist\player\`).~~ Done (D149, D150).
2. ~~The player build installed over a dev save on the 3DS once (the save carries over).~~ Runs 30 and 31:
   "It all runs great" (2026-10-02).
3. Ready (D151): the notes `docs/release/notes-v1.0.0.md`; FBI's QR code `docs/release/fbi-qr.png` for
   `releases/latest/download/emberclutch.cia` (so it always fetches the newest; `tools/release/make_qr.py`,
   read back by OpenCV); the guide's PDF in `docs/guide/`. **Noah makes the repo public** (and turns on private
   vulnerability reporting in its settings); then `tools\release\release.ps1` tags `v1.0.0` on main and makes
   the release as a draft with its four files (it refuses while the repo is private), and after a look,
   `tools\release\release.ps1 -Publish` publishes it and checks the QR link answers with the CIA.
4. The README's install section checked against the live release; Noah scans the QR code with FBI once.

## R5: Universal Updater
`docs/release/universal-db.json` holds the entry (repo, title, author, descriptions, licence, icon,
image, screenshots). It goes to [Universal-DB](https://github.com/Universal-Team/db) through its app
request form once the repo is public and v1.0.0 is released (Universal-DB reads the releases). Its title is
*Emberclutch: Skyreach Valley* and its screenshots are the refreshed ones (D149). On Noah's word.

## R6-R7: the video and YouTube
See [trailer.md](trailer.md): the stack, the capture build, the shot list, the voices to sample (none
used until Noah picks), the title and description drafts.
