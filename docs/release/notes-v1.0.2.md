A small update: bigger words on a dragon's profile, and a fix for swapping moves. **Your save carries over as
it is**: install this over 1.0.1 or 1.0.0.

## Changed

- **A dragon's profile** (About, Training, Record and Family, on the bottom screen) has its words a size larger
  on every page, with the rows respaced so nothing touches. About lists what a dragon is, its traits and what it
  wears in rows across the page, so long names keep their size.

## Fixed

- **Swapping a move:** tapping one of a dragon's four moves opened the list of moves it knows and, with the
  same tap, took whichever move happened to lie under the stylus, so the list was gone before you saw it. The
  list now stays until you pick from it or cancel.

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
| `emberclutch-1.0.2.zip` | Both of the above |
| `Emberclutch-Guide.pdf` | A Keeper's Handbook (A5, prints as a folded booklet) |

New here? [1.0.0's notes](https://github.com/RedLynx101/emberclutch/releases/tag/v1.0.0) say what the game is,
[1.0.1's](https://github.com/RedLynx101/emberclutch/releases/tag/v1.0.1) what was fixed first, and here's
[the trailer](https://youtu.be/GZZhfeaHGZA).

Made by Noah Hicks, with Claude (Anthropic) writing much of the code and tools. *Emberclutch* is a fan-made
homebrew project, not affiliated with or endorsed by Nintendo. "Nintendo 3DS" is a trademark of Nintendo.
