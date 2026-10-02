## What and why
<!-- What this changes, and why. Link the issue it fixes (Fixes #...). -->

## How I checked it
<!-- Tests, scripted emulator runs (before/after screenshots for anything visible), hardware. -->

## Checklist
- [ ] `tools\test.ps1` (or `make -C tests run`) passes
- [ ] Anything visible: a headless autotest run, with before/after screenshots above
- [ ] `src/core/` stays free of 3DS libraries, clock reads and file I/O, and new logic there has tests
- [ ] The save format is unchanged, or bumped with a migration and a test
- [ ] Performance on the old 3DS considered (triangles, draws, frame time) for anything that draws more
- [ ] No AI-generated art, models or textures added to the game
- [ ] Docs updated where behaviour changed (`docs/design/`, `docs/plan/decisions.md` for a design change)
- [ ] I can contribute this under the project's licences (MIT for code, CC BY-SA 4.0 for art)
