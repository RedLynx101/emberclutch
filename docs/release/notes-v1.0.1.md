A first round of fixes from play after 1.0. **Your save carries over as it is**: install this over 1.0.0.

[![Watch the Emberclutch trailer on YouTube](https://raw.githubusercontent.com/RedLynx101/emberclutch/main/docs/release/trailer.jpg)](https://youtu.be/GZZhfeaHGZA)

## Fixed

- **Mirror Lake** is on its shore. In 1.0 the whole place sat under the beach, with only the fishing rod and
  the lamp showing, and the buried boat stopped you where nothing could be seen. The jetty, the boat, the bench,
  the reeds and the lily pads are all there now, and you can walk out along the jetty.
- **Frostspire Hollow** can always be entered. Tove waited at the Cold Vault after your first glide until you
  met her, which left the Hollow with nobody to let you in. She's home once you've found the Hollow, and A at
  the cave door takes you to her.
- **Wren** says her piece once: it no longer started again inside the challenge board.
- **Sky Rings picked at Honeyroot Orchard's board** brings you back to the orchard, not to the Arena.
- **A first win's trophy** no longer leaps out of the screen in 3D, and it stands clear of the race's clock.
- **The backs of heads** have their hair: Wren, Fig and several others showed bare scalp from behind. Tam's
  and Bram's heads are round at the back, and Tam's rain hat sits in the middle of its brim.
- **Emberpeak Caldera's league board** was drawn twice, one through the other.
- People the story moves about no longer leave an unseen wall where they stood.

## Changed

- **The Journal's Places page** lists all eighteen places, with `???` for the ones you haven't found. Tap any
  of them, found or not, to track it: a place you haven't found gets a search circle on the map.
- **The festival's lanterns** each have a dot, gold when lit and grey when dark, on the Places page and on the
  map's pins, with a count. The eighth is the Arena's great lantern, which lights on the festival night itself.
- **The den's cliff** is sheer and straight only by the den and the falls now. North and south of them it
  wanders and eases into hillside, on the land and on the map.

## Install or update

You need a 3DS with custom firmware (for example [Luma3DS](https://3ds.hacks.guide/)). It runs on every 3DS,
the original old 3DS included.

- **FBI, by QR code:** open FBI, choose *Remote Install → Scan QR Code*, and scan this. It always fetches
  the latest release.

  ![QR code for FBI's remote install](https://raw.githubusercontent.com/RedLynx101/emberclutch/main/docs/release/fbi-qr.png)

- **By hand:** copy `emberclutch.cia` (below) to your SD card and install it with FBI. Or copy
  `emberclutch.3dsx` to `sdmc:/3ds/` and start it from the Homebrew Launcher.

**Sound** needs your 3DS's DSP firmware (`sdmc:/3ds/dspfirm.cdc`, made once with
[DSP1](https://github.com/zoogie/DSP1)); without it the game runs silently. **Your save** lives in
`sdmc:/3ds/emberclutch/`, kept as two copies so an interrupted save never loses both.

## Files

| File | What it is |
|---|---|
| `emberclutch.cia` | The game, to install with FBI |
| `emberclutch.3dsx` | The game, for the Homebrew Launcher |
| `emberclutch-1.0.1.zip` | Both of the above |
| `Emberclutch-Guide.pdf` | A Keeper's Handbook (A5, prints as a folded booklet) |

New here? [1.0.0's notes](https://github.com/RedLynx101/emberclutch/releases/tag/v1.0.0) say what the game is.

Made by Noah Hicks, with Claude (Anthropic) writing much of the code and tools. *Emberclutch* is a fan-made
homebrew project, not affiliated with or endorsed by Nintendo. "Nintendo 3DS" is a trademark of Nintendo.
