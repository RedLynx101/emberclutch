# The trailer, V4-V5: the cut

**The trailer is cut** (docs/plan/trailer.md, V4-V5): 1:27 at 1080p and 60 frames a second, every frame from
the game, in its own music and its own sounds, with Lily narrating. What changed since the footage check:
- **No more pixels: the game is filmed at six times its resolution.** Azahar draws each frame at 2400×1440 (both
  screens: 4320×1440); the game hands every frame over and waits while a small reader copies it off the
  emulator's screen (`tools/wsl/grab.py`), so nothing new was installed. Scaled down to 1080p, the edges come
  out smooth.
- **The game's own sounds, on their frames.** While filming, the game writes down every sound it plays (the
  file, its pitch and volume, the frame) and the ambience's levels; the trailer plays those same sounds on
  those frames: the egg's heartbeat and knocks, the crack, the hatchling's first cry, the purrs and munching,
  the wingbeats, the flame breath and hits, the reel, the cove's water.
- **A fix on the way:** the headless emulator had always run silent (the sound's firmware file sat where WSL
  couldn't see it), so no headless test had ever had sound. Both runners now copy it somewhere WSL can read,
  and the runner says so if it's missing.
- **Shots changed:** the valley's reveal now cranes up past the cliff's waterfall from the den's door (the old
  one looked into fog: past 260 m the fog has everything); the lanterns are at full night, the village's and
  then home's; the Dragondex taps through the dragons met; the egg and the hatching run longer.
- **The music:** title-theme under the egg, the care and the valley (the burst on a downbeat, the title on the
  strings' entry), dropping away for "And when it's grown... climb on", skyreach in on its drums at the
  take-off, a hush for the lanterns, then title-theme's own close, its last chord under the end card.

## 1. The trailer
1. Watch it (the preview here is 720p; the full 1080p file is `build/film/edit/trailer.mp4`, sent to you too).
   Keep, or Change with notes (a shot, a line, the music, the mix, the end card).

## 2. The vertical cut
1. The 33-second cut for Shorts: Keep, or Change.

## 3. The thumbnail
1. Pick one of three (A, B or C).

## What's next
- YouTube is yours to upload (it's public): the title, description, chapters and tags are in
  `docs/plan/trailer.md`, the captions in `build/film/edit/trailer.srt`.
