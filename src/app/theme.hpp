// Emberclutch palette — the single source of UI colours.
// Mirrors docs/design/theme-and-art-direction.md §3.
#pragma once

#include <citro2d.h>

namespace ec::theme {

inline u32 rgba(u8 r, u8 g, u8 b, u8 a = 255) { return C2D_Color32(r, g, b, a); }

inline const u32 kEmber = rgba(0xE8, 0x66, 0x2B);
inline const u32 kClutchGold = rgba(0xF5, 0xC4, 0x51);
inline const u32 kShell = rgba(0xFF, 0xF3, 0xDC);
inline const u32 kDenPlum = rgba(0x34, 0x23, 0x3F);
inline const u32 kDusk = rgba(0x5E, 0x44, 0x66);
inline const u32 kTrack = rgba(0x1C, 0x11, 0x22);  // an empty gauge: dark against the dusk backgrounds (Noah, run 3)
inline const u32 kSkyTeal = rgba(0x3F, 0xA7, 0xA8);
inline const u32 kAsh = rgba(0x8C, 0x7A, 0x86);
inline const u32 kRose = rgba(0xD9, 0x54, 0x6A);

}  // namespace ec::theme

#include "core/types.hpp"

// U (1.0): the storybook scenes painted in 2D (the Market's square, the Wanderings' trail;
// app/storybook), before the time of day's light is laid over them: soft, warm, low-contrast,
// the palette of the valley's painted map.
namespace ec::theme::paint {

inline constexpr Rgb kCloud{255, 250, 242};
inline constexpr Rgb kMountain{170, 160, 200};
inline constexpr Rgb kSnow{248, 244, 255};
inline constexpr Rgb kHillFar{158, 186, 150};
inline constexpr Rgb kHillMid{140, 180, 114};
inline constexpr Rgb kHillNear{122, 166, 98};
inline constexpr Rgb kGrass{110, 154, 90};
inline constexpr Rgb kCobble{224, 198, 158};
inline constexpr Rgb kCobbleDark{198, 168, 126};
inline constexpr Rgb kCobbleLight{240, 222, 188};
inline constexpr Rgb kWood{164, 114, 74};
inline constexpr Rgb kWoodDark{118, 80, 54};
inline constexpr Rgb kWoodLight{214, 170, 118};
inline constexpr Rgb kAwning[4] = {{214, 86, 70}, {70, 158, 156}, {150, 98, 156}, {232, 164, 58}};
inline constexpr Rgb kAwningCream{252, 242, 222};
inline constexpr Rgb kRoof{178, 96, 76};
inline constexpr Rgb kRoofBlue{104, 110, 160};
inline constexpr Rgb kWall{242, 224, 194};
inline constexpr Rgb kWindowLit{255, 212, 118};
inline constexpr Rgb kWindowDark{104, 90, 118};
inline constexpr Rgb kPath{210, 172, 120};
inline constexpr Rgb kPathLight{232, 204, 154};
inline constexpr Rgb kPond{118, 176, 222};
inline constexpr Rgb kTrunk{128, 90, 62};
inline constexpr Rgb kLeaf{84, 138, 84};
inline constexpr Rgb kLeafLight{124, 172, 98};
inline constexpr Rgb kRock{152, 146, 172};
inline constexpr Rgb kPebble{172, 166, 178};
inline constexpr Rgb kCoin{245, 196, 81};
inline constexpr Rgb kFeather{236, 238, 250};
inline constexpr Rgb kCrystal{110, 206, 214};
inline constexpr Rgb kPearl{250, 244, 240};
inline constexpr Rgb kFossil{236, 220, 186};
inline constexpr Rgb kFlower[3] = {{244, 146, 170}, {252, 226, 120}, {196, 170, 240}};
inline constexpr Rgb kSun{255, 236, 170};
inline constexpr Rgb kSunset{255, 170, 110};
inline constexpr Rgb kStraw{236, 206, 140};
inline constexpr Rgb kMoon{236, 234, 250};

}  // namespace ec::theme::paint
