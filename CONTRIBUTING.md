# Contributing to Emberclutch

Thanks for wanting to help with *Emberclutch: Skyreach Valley*. Bug reports, ideas, fixes, translations
of the docs and play-testing notes are all welcome. This page says how to send each, and what a change
needs before it can be merged.

By taking part you agree to the [Code of Conduct](CODE_OF_CONDUCT.md).

## Reporting a bug
Open an issue with the **Bug report** form. The most useful reports say:
- the game's version (on the title screen) and how you installed it (CIA or 3DSX);
- your console (old 3DS, old 2DS, New 3DS, New 2DS XL) or emulator;
- what you did, what happened, and what you expected;
- a screenshot or photo if it's visual, and if the game froze, the file
  `sdmc:/3ds/emberclutch/watchdog.txt` (written when frames stop for a few seconds).

Your save (`sdmc:/3ds/emberclutch/save.a` and `save.b`) helps with save-related bugs; attach it only
if you're happy to share it. It holds your game's progress, with your player name and your dragons'
names.

Security problems: see [SECURITY.md](SECURITY.md) instead of opening a public issue.

## Suggesting an idea
Use the **Idea** form. Say what you'd like to do in the game and why it would be fun; the design
documents in `docs/design/` describe how the game is meant to feel, and `docs/plan/decisions.md` lists
what has already been decided (those decisions are binding on changes, but can be revisited with good
reason).

## Changing the code
1. **Build it.** [docs/DEVELOPING.md](docs/DEVELOPING.md): devkitPro with the `3ds-dev` group, then
   `make` (the dev build) or `make DEV=0` (the player build). On Windows, `tools\build.ps1`.
2. **Read first.** `docs/STATUS.md` (the current state), `docs/plan/decisions.md` (the decisions), and the
   design doc for the part you're touching.
3. **Keep to the layout.** `src/core/` is plain C++17 with no 3DS libraries, no clock reads and no file
   I/O, and everything in it is unit-tested on the PC; `src/app/` is the 3DS layer. Source file names are
   unique across both (the Makefile flattens object names). UI colours come only from
   `src/app/theme.hpp`.
4. **Match the style.** Four-space indents, the `ec` namespace, `kConstant` names, short comments that
   say *why*. Look at the code around your change and write like it.
5. **The save format is a promise.** The `Genome` struct and the creature records are in players' saves;
   change them only with a version bump and a migration, and a test for it.
6. **Test.** `tools\test.ps1` (or `make -C tests run`) must pass. For anything visible, run a scripted
   emulator check (`tools\autotest.ps1 <script> -Headless`, see `docs/tech/headless-emulator.md`) and
   include the before/after screenshots in your pull request.
7. **Mind the old 3DS.** It's the performance floor. If your change draws more, say how much (the dev
   build's overlay shows triangles, draws and frame time) and, if you can, how it runs on real hardware.

## Art, models, music and sound
- The dragons, people and places are built by scripts (`tools/blender/`, `tools/people/`,
  `tools/dragons/`); change the script, not an exported file, and re-export.
- **No AI-generated art or models ship in the game.** The concept images in `docs/art/concept/` are
  reference only. Music and sound effects follow `assets/audio/music/LICENSE-MUSIC.md`; ask in an issue
  before adding new audio.
- Fonts must be under an open licence that allows embedding and subsetting (the game uses SIL OFL fonts).

## Pull requests
- One change per pull request, with a clear description: what, why, and how you checked it.
- Fill in the pull request template's checklist.
- Commit messages: a short summary line, then what changed and why.
- Expect review comments; a change that touches gameplay or the look may be play-tested on hardware
  before it's merged.

## Licensing of contributions
Contributions are accepted under the same terms as the project ("inbound = outbound"):
- **code** under the [MIT licence](LICENSE);
- **original art and assets** under [CC BY-SA 4.0](assets/LICENSE-ART.md).

By opening a pull request you confirm you have the right to contribute it under those terms.
