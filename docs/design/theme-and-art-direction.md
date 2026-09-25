# Theme & Art Direction

Status: **v0.2 — approved** (2026-09-23, review round 1). **Beta (D75):** the storybook look of
a cozy life-sim, spelled out in [look and feel](look-and-feel.md); where the two differ, that
page wins. The dragons are being redesigned to match ([R11](../art/reviews/R11-new-dragons.md), D76).

## 1. The idea in one line

**Warm light inside.** Every dragon — and every egg — carries an ember of light in its
chest. The world is a cozy den that opens onto a wide, cool sky. Warm home, cool
adventure.

## 2. Signature: the heartglow

The heartglow is Emberclutch's visual signature and its main UI. It shows up everywhere:

- **On dragons:** a soft, **heart-shaped** light behind the chest scales in the dragon's
  element color (see `breed-lineup.png`).
  Brightness and pulse speed show mood (bright + quick = joyful, dim = upset).
  Hybrids swirl two colors.
- **On eggs:** the glow shines through the shell and warms up as incubation progresses.
  Its colors hint at what will hatch.
- **In the UI:** selection highlights, the cursor and confirm buttons pulse with the same
  glow. The app icon is a glowing egg.
- **In audio:** a soft heartbeat-like thrum under petting and hatching moments.

## 3. Palette

UI and world tokens. Dragons use their own genetic colors on top of this.

| Token | Hex | Use |
|---|---|---|
| `ember` | `#E8662B` | Primary accent, active states, flame gauges |
| `clutch-gold` | `#F5C451` | Highlights, rewards, Gleam, selected glow |
| `shell` | `#FFF3DC` | Panels, text on dark, eggshell surfaces |
| `den-plum` | `#34233F` | Backgrounds, text on light, deep shadows |
| `dusk` | `#5E4466` | Secondary panels, inactive tabs |
| `sky-teal` | `#3FA7A8` | Flight, sky, Skyreach Valley UI, "go" actions |
| `ash` | `#8C7A86` | Disabled, muted text |
| `rose` | `#D9546A` | Warnings, upset state (used sparingly) |

Rules: the den and menus lean **warm** (ember, gold, plum); flight, the valley and
competitions lean **cool** (sky-teal) with warm accents. Never pure black or pure white.

## 4. Shape language

- **Cute → majestic.** Babies are round, big-headed and soft-cornered. Adults are long,
  tapered and elegant with swept lines. Hatchlings have their own baby model; the
  stage-up to juvenile is **the first molt** (a glow hides the model swap, D36). From
  then on growth is continuous.

  | Stage | Head : body length | Eye height / head | Neck | Wingspan / body |
  |---|---|---|---|---|
  | Hatchling | 1 : 1.3 | 30% | stubby | 0.6× |
  | Juvenile | 1 : 2 | 24% | short | 1.0× |
  | Adolescent | 1 : 3 | 18% | long | 1.6× |
  | Adult | 1 : 4.5 | 12% | long, S-curve | 2.4× |

- **Majestic, never menacing.** Adults have kind eyes, calm brows and a proud posture. No
  snarling or dripping fangs. Teeth only show when roaring.
- **Silhouette first.** At 400×240, a dragon must read from its outline alone: horn
  shape, wing type and tail tip are the silhouette variables.

## 5. Rendering look (old-3DS friendly)

- **Soft toon shading:** 2–3 light bands via fragment-light LUTs, plus a warm rim light.
  No per-pixel outlines. An optional inverted-hull outline on dragons only, and only if
  the frame budget allows.
- **Color by mask:** dragons use a shared grayscale-plus-mask texture and are tinted by the
  GPU combiner (see [architecture](../tech/architecture.md)). Painted detail is in the
  value channel only, which keeps every color variant looking hand-painted.
- **Heartglow:** additive emissive region from the mask's alpha channel, with a pulse
  driven by mood. Cheap and very readable. It always has a **white-hot core** inside the
  element-colored rim, so it reads even when the glow hue matches the body (an aqua glow
  on a teal Tide dragon). Keep the chest unoccluded in every idle pose, including the
  hatchling's big head.
- **Environments:** vertex-colored low-poly with baked lighting and gradient skies.
  Atmospheric fog hides the short draw distance in Skyreach Valley. From Beta (D75):
  small hand-painted tiling textures tinted by the vertex colour, round chunky props, and
  the ground curving away on foot (the rolling log).
- **Particles:** a small budget of embers, sparkles, leaves and breath effects.
- Stereoscopic 3D is supported but optional. The game must look complete in 2D mode.

## 6. UI style

- **Eggshell panels:** rounded cream (`shell`) panels with a subtle cracked top edge,
  sitting on `den-plum`.
- **Ember gauges:** needs are four small icons (drumstick = Belly, moon = Energy,
  sparkle = Shine, ball = Play), each with a flame that shrinks as the need drains.
- **Heartglow orb:** a circle in the bottom-screen corner mirrors the active dragon's
  heartglow — the mood indicator.
- **Scale texture:** faint scale-pattern backgrounds on menus.
- **Touch targets:** at least 32×32 px on the 320×240 bottom screen.
- **Motion:** gentle ease-out and a little overshoot on panels, like a hatching wobble.

## 7. Typography

- **UI:** Nunito (SIL Open Font License), converted to BCFNT with `mkbcfnt`. Rounded,
  friendly, legible at small sizes.
- **Titles / wordmark:** Cinzel Decorative (OFL) for a touch of majesty, used only for
  large headings and baked into the logo texture.
- System font fallback during early development.

## 8. Logo and icon

- **Wordmark:** "Emberclutch" set in Cinzel Decorative, with a small glowing egg as a
  mark before the E. The ember glow warms the letters from below.
- **App icon (48×48):** a speckled egg in a woven nest, glowing orange from within.
- **HOME Menu banner:** the egg cracks and light spills out; banner audio is a soft
  heartbeat thrum and a chirp.

## 9. Audio direction

- **Instruments:** kalimba, warm woodwinds, harp, hand drums. Adult and flight moments
  add low horns and soft choir for majesty.
- **Dragon voices:** chirps and trills for hatchlings, deepening to rumbles and calls as
  they grow. Voices are pitch-shifted per individual so no two dragons sound identical.
- **Den:** a crackling hearth and distant wind. The valley: open wind and birdsong.
- **People (Beta, D75):** speech voiced letter by letter from one recorded alphabet, sped
  up and pitched per speaker; dragons never talk.

## 10. Concept art

Exploration images live in [`docs/art/concept/`](../art/concept/). They are **AI-generated
reference material** used for direction only — they are not shipped in the game, and
final assets will be made (or remade) as original, openly licensed work.
