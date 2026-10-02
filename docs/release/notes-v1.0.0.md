*Raise, train and ride dragons in a cozy valley: a free, open-source homebrew game for the Nintendo 3DS.*

Made by Noah Hicks. Inspired by Emi, who makes every day feel like hatching day.

![Your dragon and you at the den's door in Skyreach Valley](https://raw.githubusercontent.com/RedLynx101/emberclutch/main/docs/release/screenshots/valley.png)

Hatch an egg, care for a tiny hatchling with the stylus, and watch it grow into a grown dragon over about a
week of real days. Walk it through Skyreach Valley on its lead, ride it into the sky when it's grown, and
train it into a champion of battles and beauty shows. Nothing ever dies: a neglected dragon only sulks until
you make up. It runs on every 3DS, the original old 3DS included.

## What's in 1.0

- **Hands-on care:** feed, pet, brush and bathe your dragon on the touch screen. Belly, Clean, Play and Love
  to look after, Energy that sleep brings back, moods, a manner, traits, and a heartglow that shows how it
  feels.
- **Fourteen kinds of dragon** in four colourings each (one of them rare), with stats, manners and 34 traits
  that each do something. Breed pairs at the Nesting Stone for new kinds and surprises.
- **Skyreach Valley**, an open world of eighteen places to walk with your partner and fly over on a grown
  dragon, with the day's finds, a market, villagers and their Journal.
- **The Lantern Festival**, a first story with the valley's people.
- **Battles:** turn-based duels, four moves each, eight elements, the Ember to Starfire league and its finals
  at Emberpeak Caldera, and wild dragons floor by floor in Frostspire Hollow.
- **Beauty shows** at Moonpetal Glade: themed pageants, accessories and dyes.
- **Challenges:** Sky Rings races, Fruit Catch and the Lantern Trial, with cups and trophies.
- **Fishing**, the Wanderings (your dragon walks while you do), a den to decorate, a Dragondex to fill,
  photos, and a real-time day and night.

## Built for the original 3DS

Everything here runs on the first 3DS from 2011: a 268 MHz ARM11 processor, a PICA200 graphics chip with no
programmable pixel shaders, 64 MB of memory for the game and 6 MB of video memory. Fitting a living 3D world
into that took some doing:

- **Skinned 3D dragons**, animated on the graphics chip's vertex shader with each bone's matrix packed into
  its 96 constant registers (25 bones a draw), up to three grown dragons in the den at once, about 3,000
  triangles each up close.
- **A 2.3 km valley** streamed in tiles at four levels of detail, with eighteen places, villagers and
  critters, held to a triangle budget every frame (about 9,600 in the valley: a heavy view
  draws its distant ground simpler until it's back under), in stereoscopic 3D, which draws the top screen
  twice.
- **An animated 3D HOME Menu banner**, a hatchling in its egg, inside the HOME Menu's 512 KB, after tracking
  down why some banners froze the HOME Menu (where their pictures sat inside the file).
- **Music streamed** and mixed in small slices, and the ground's and rocks' painted textures generated at
  start rather than stored.
- **Tested every step:** more than 387,000 automated checks on a PC, scripted runs in a headless emulator, and
  over thirty rounds of play-testing on a real old 3DS.

## Install

You need a 3DS with custom firmware (for example [Luma3DS](https://3ds.hacks.guide/)).

- **FBI, by QR code:** open FBI, choose *Remote Install → Scan QR Code*, and scan this. It always fetches
  the latest release.

  ![QR code for FBI's remote install](https://raw.githubusercontent.com/RedLynx101/emberclutch/main/docs/release/fbi-qr.png)

- **By hand:** copy `emberclutch.cia` (below) to your SD card and install it with FBI. Or copy
  `emberclutch.3dsx` to `sdmc:/3ds/` and start it from the Homebrew Launcher.
- **Universal Updater:** search for *Emberclutch*, once it's listed there.

**Sound** needs your 3DS's DSP firmware (`sdmc:/3ds/dspfirm.cdc`, made once with
[DSP1](https://github.com/zoogie/DSP1)); without it the game runs silently. **Your save** lives in
`sdmc:/3ds/emberclutch/`, kept as two copies so an interrupted save never loses both. Installing a newer
version keeps it.

## The guide

***A Keeper's Handbook*** (`Emberclutch-Guide.pdf`, below): the game's world and mechanics, with the
Keeper's Almanac at the back for the numbers behind it all (what each trait does, the elements' chart, every
move, how shows are judged), light on spoilers. It's also [readable on GitHub](https://github.com/RedLynx101/emberclutch/blob/main/docs/guide/guide.md).

## Files

| File | What it is |
|---|---|
| `emberclutch.cia` | The game, to install with FBI |
| `emberclutch.3dsx` | The game, for the Homebrew Launcher |
| `emberclutch-1.0.0.zip` | Both of the above |
| `Emberclutch-Guide.pdf` | A Keeper's Handbook (A5, prints as a folded booklet) |

## Licences

Code: MIT. Original art and assets: CC BY-SA 4.0. Music and generated sound effects: their own terms (see
`assets/audio/music/LICENSE-MUSIC.md`). Full credits in `CREDITS.md`. Bug reports and ideas are welcome
through the issue forms; see `CONTRIBUTING.md`.

Made by Noah Hicks, with Claude (Anthropic) writing much of the code and tools. *Emberclutch* is a fan-made
homebrew project, not affiliated with or endorsed by Nintendo. "Nintendo 3DS" is a trademark of Nintendo.
