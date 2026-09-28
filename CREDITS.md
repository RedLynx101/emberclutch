# Credits

**Emberclutch** was designed and directed by **Noah Hicks**, who also made its music and
sound effects, gave the villagers their voices and played it on the hardware at every step.
Much of the code, the models' build scripts and the tools were written with **Claude**
(Anthropic's AI model), working from Noah's design, reviews and decisions.

## Made with

- [devkitPro](https://devkitpro.org/) and devkitARM, **libctru**, **citro3d** and
  **citro2d** (zlib licence), **picasso**, **tex3ds**, **makerom** and **bannertool** for
  building and packaging.
- **Tremor** (libvorbisidec) and **libogg** from the Xiph.Org Foundation (BSD licence) for
  the music. See [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
- [Blender](https://www.blender.org/) as a tool: the dragons, the people and the valley's
  places are built by scripts in `tools/blender/` (the models are original work).
- [Azahar](https://azahar-emu.org/) for scripted test runs.
- [pycgfx](https://github.com/skyfloogle/pycgfx) for the animated 3D HOME Menu banner.

## Fonts

- **Nunito** by The Nunito Project Authors, SIL Open Font License 1.1.
- **Cinzel Decorative** by Natanael Gama, SIL Open Font License 1.1 (Reserved Font Name
  "Cinzel"). The game's font files are subsets of these, under the same licence.

## Music and sound

- The soundtrack was made by Noah Hicks with **Suno**; the sound effects with **ElevenLabs**
  Sound Effects; the villagers' letter-by-letter voices from alphabet recordings Noah made
  and provided (the men's from his own voice). Some
  effects (the 1.0 set: battles, riding, fishing, the new places' ambience) are synthesised
  by `tools/audio/make_synth_sfx.py`. Their terms are in
  [LICENSE-MUSIC](assets/audio/music/LICENSE-MUSIC.md).

## Thanks

To the 3DS homebrew community whose guides, libraries and tools make games like this
possible.
