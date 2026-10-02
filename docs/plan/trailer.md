# The video: a trailer of the game (planned; not made yet)

Noah, 2026-10-01: *"We will also plan on a well-crafted video you make of the game using various
in-emulator camera shots and such (on a custom version of the game you build to do that)... You could
also use the elevenlabs api for voice, I don't mind. Just sample some voices and get preapproval for
particular voices before the whole thing is made."* Then a YouTube release "for fun with links (so plan
title and desc)". Part of the 1.0 plan ([release-1.0.md](release-1.0.md), R6 and R7).

## What it is
- **A ~75-second trailer**, 16:9, 1080p at 60 fps (H.264 MP4 for YouTube), cut to the game's own music.
  A 30-second vertical cut for Shorts is optional, from the same shots.
- **Real game footage only:** every picture of the game is the game, rendered by the 3DS's own code in the
  emulator; titles and captions are drawn on top. No generated footage (it would change the look and
  stop being the game).
- **The feel:** the storybook valley at golden hour, cozy and bright; the hook in the first five seconds
  (an egg rocking, cracking, a hatchling blinking at you).

## The stack (recommended)
Per the animation-stack notes (`opus-5-5-animation-stack.md`): the smallest stack that makes the look.
Here the hard part is the footage, which comes from the game, so:

| Piece | Why | Notes |
|---|---|---|
| **A film build of the game** (`make FILM=1`) | The custom version for camera shots | No HUD, tips or dev overlay; a *director* that plays shot scripts (camera keyframes with eased, spring-damped moves, the time of day, who's where, what they do: walk, ride, battle, show); a fixed time step, so every frame is the same every run, whatever the emulator's speed |
| **Headless Azahar** (the `emberclutch-test` WSL distro) | Runs the film build out of sight, at full speed, repeatably | Rendering at a raised internal resolution (4-5x: 1600x960 to 2000x1200 for the top screen) and the frames captured by the emulator. **First spike:** proving that capture in the headless build (Azahar's screenshot and video dump at the internal resolution); the fallback is the game's own 400x240 frames scaled up crisply (an honest pixel look, which suits the bottom screen's interface anyway) |
| **One HTML page with `seek(t)`** | Title cards, captions, transitions, the end card | The game's palette and fonts (Cinzel Decorative, Nunito); frames rendered one by one by headless Chrome over its DevTools protocol, driven from Node 24 (both on this PC: nothing to install). Remotion only if the edit outgrows one page (an npm install: your call) |
| **ffmpeg 8.1** | Stitches footage and overlays, mixes the sound, encodes | Already installed |
| **The game's music and sounds** | The soundtrack is Noah's (Suno) and the effects are the game's | Cuts on the music's beat grid (`romfs/music/loops.json` has the BPMs). Check that Suno's terms on your plan allow a YouTube video |
| **ElevenLabs** (only if a voice helps) | A narrator for a few short lines | Optional: a trailer can carry text cards alone. **No voice is used until you pick one** (below) |
| Effort | Stock Claude Code: xhigh to build the film build and the pipeline, max for the edit's critique passes | No Ultracode or video models needed (Noah asked whether max would do much better: see below) |

Every pass ends with a contact sheet of the shots on the review page (Keep or Change each), and the
worst three things are fixed before the next pass. That loop matters more than the tools.

**Max or xhigh?** (Noah, on the plan.) Not much better for building things: the film build, the capture
and the page are engineering, and xhigh does them as well. Where max earns its cost is the edit: looking
hard at each pass's frames and fixing the worst of them, over more passes. So: xhigh for V1-V3, max for
V4-V5's critique passes. A clear reference (the shot list, stills of the look) still matters more than
either.

**Approved (2026-10-02):** the plan, the stack and the shots (Noah: "you keep creative freedom as needed;
you're in charge of this"); the voices are pre-approved for sampling (below).

## Voices: sampled first, approved before anything is made
Noah, 2026-10-02: "You are preapproved for this. When we begin the video pass, just end early with the
sample voices for me to approve before you go off and finish the whole thing." So the video pass stops
after V2 with the samples.

1. Once the script is written, 5 or 6 candidate voices from ElevenLabs' library (warm storyteller,
   gentle young adult, bright and playful, a grandparent-like one like Rowan, one British and one
   American), each reading the same two lines from the script, with `eleven_v4`.
2. The samples go on the review page: you pick one (or ask for others, or a designed voice).
3. Only then is the full narration generated, in that voice. Credits are checked first (90,000 a month;
   a 75-second script is about 1,000 characters, so the whole job is a few thousand credits).

## The shots (a first list; Keep/Change on the review page)
| # | Shot | ~s |
|---|------|----|
| 1 | Black, then the egg in the den's nest by firelight, rocking. A crack. | 4 |
| 2 | The hatchling's head pops out; it blinks at the camera. Title card: *Emberclutch*. | 4 |
| 3 | The stylus strokes it on the bottom screen; hearts rise (the top and bottom screens side by side). | 4 |
| 4 | Feeding, a bath's bubbles, the brush: three quick cuts. | 4 |
| 5 | Time passing: the den from day to night to morning, the hatchling grown a little each time. | 4 |
| 6 | Out of the den's door: Skyreach Valley opens up, camera rising over the meadow. *A valley to explore.* | 5 |
| 7 | Walking the lead past the Market's stalls, villagers waving, feelings over their heads. | 4 |
| 8 | The grown dragon: climbing on, the take-off. | 3 |
| 9 | Flying over the valley at golden hour, through the floating isles. *Raise it. Ride it.* | 6 |
| 10 | A battle: the two dragons facing off, a breath move, the hit, the win card. | 5 |
| 11 | A beauty show at Moonpetal Glade: the pose, the judges, a ribbon. | 4 |
| 12 | The Sky Rings race: through the rings, the cup. | 3 |
| 13 | Fishing at Driftwood Cove: the cast, the bite, the catch. | 3 |
| 14 | The Lantern Festival at night: lanterns rising over the village green. | 5 |
| 15 | The Dragondex scrolling past many kinds (only kinds you see early). | 3 |
| 16 | Back in the den: two dragons curled up asleep by the hearth. *Nothing ever dies; it only sulks until you make up.* | 4 |
| 17 | End card: the wordmark, "Free homebrew for the Nintendo 3DS family", the GitHub and Universal Updater lines. | 6 |

No story spoilers past the festival's lanterns; no champions or deep places.

## Steps and gates
1. **V0, this plan.** Keep or Change it on the review page.
2. **V1, the capture spike.** The film build with one shot (the flight, shot 9) captured headless at 4x;
   its timing; the fallback decided if the high-resolution capture won't work headless.
3. **V2, the storyboard.** A still of every shot from the film build, the captions and the narration
   script, the voice samples. You approve the shots, words and voice.
4. **V3, the capture.** All shots captured; a contact sheet.
5. **V4, the rough cut.** The edit to music, with captions; review. **Before V4 starts, tell Noah to switch
   the session's effort to max** (Noah, 2026-10-02: "Plan on the xhigh to max change during the video
   build. Just let me know when to switch to max."); V1-V3 run at xhigh.
6. **V5, the final.** Sound mixed, the end card, thumbnails (three to pick from); the MP4 and the vertical cut.
7. **V6, YouTube.** You upload it (it's public, so it's yours to do) after the release is public, so the
   links work; the title, description and thumbnail are ready below.

## YouTube: drafts
**Title** (Noah picked the third, 2026-10-02): **Emberclutch: Skyreach Valley, official trailer (free 3DS
homebrew)**. (The others were "... | A cozy dragon-raising game for the 3DS (homebrew trailer)" and "I made a
dragon-raising game for the 3DS: ...".)

**Description**
> Hatch an egg, raise a dragon, and explore Skyreach Valley together. Emberclutch: Skyreach Valley is a
> free, open-source dragon life game for the Nintendo 3DS family (homebrew; it runs on the original old
> 3DS).
>
> Care for your hatchling with the stylus and watch it grow over real days. Walk the valley on its lead,
> meet the villagers, ride your grown dragon through the sky, battle in the league, shine in beauty
> shows, race the Sky Rings, fish at Driftwood Cove and fill your Dragondex. Nothing ever dies: a
> neglected dragon only sulks until you make up.
>
> Download (free): https://github.com/RedLynx101/emberclutch/releases/latest
> Universal Updater: search for "Emberclutch"
> The guide (PDF): (the release's guide link)
> Source code (MIT): https://github.com/RedLynx101/emberclutch
>
> Chapters
> 0:00 Hatching
> 0:12 Caring
> 0:24 Skyreach Valley
> 0:38 Riding
> 0:46 Battles, shows and races
> 1:02 The Lantern Festival
> 1:10 Get it
>
> Made by Noah Hicks, for Emi. Music by Noah Hicks (made with Suno); code and tools written with Claude
> (Anthropic). Footage captured from the game running in the Azahar emulator.
> Emberclutch is a fan-made homebrew project, not affiliated with or endorsed by Nintendo. You need a 3DS
> with custom firmware to play it.

**Tags:** 3DS homebrew, Nintendo 3DS, dragon game, cozy game, virtual pet, pet sim, indie game, open
source game, Emberclutch

**Thumbnail:** the grown dragon mid-flight over the valley at golden hour, the hatchling in the corner,
the wordmark; three made from the film build's frames to pick from.

## Risks
- High-resolution capture in the headless emulator (the V1 spike settles it; the fallback is the game's
  own frames, scaled).
- Music rights for YouTube (Suno's terms on Noah's plan); YouTube's Content ID can misfire on generated
  music, so the description credits it.
- The 3D (stereo) can't show in a flat video; the trailer says the game is in 3D on the end card.
