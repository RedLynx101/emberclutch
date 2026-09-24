# Art & Asset License

Original Emberclutch art and assets (models, textures, sprites, icons, sound effects) are
licensed under **Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)**:
https://creativecommons.org/licenses/by-sa/4.0/

Attribution: "Emberclutch by Noah Hicks".

Exceptions:

- **Music** in `assets/audio/music/` and `romfs/music/` has its own terms. See
  [`audio/music/LICENSE-MUSIC.md`](audio/music/LICENSE-MUSIC.md).
- **Concept images** in `docs/art/concept/` are AI-generated reference material. They
  are not part of the shipped game and are not offered under this license.
- **Fonts** in `assets/fonts/` are under the SIL Open Font License 1.1 (each folder has its
  `OFL.txt`): Nunito (The Nunito Project Authors) and Cinzel Decorative (Natanael Gama,
  Reserved Font Name "Cinzel"). `romfs/fonts/ui.bcfnt` and `title.bcfnt` are subsets of them
  converted by `tools/make_fonts.ps1`, released under the same license and, being modified
  versions, not named after the originals.
- `assets/icon.png` is the emblem (`tools/blender/emblem.py`, D48) and `assets/banner.png`
  the flat banner rendered from the game's own dragon model (`tools/blender/banner3d.py`,
  which also builds the animated 3D banner, D50); original art under this license. The
  wordmark uses Cinzel Decorative (SIL OFL). `tools/blender/app_icon.py` made Alpha 1's
  interim icon and banner.