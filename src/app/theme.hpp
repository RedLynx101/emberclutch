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
inline const u32 kSkyTeal = rgba(0x3F, 0xA7, 0xA8);
inline const u32 kAsh = rgba(0x8C, 0x7A, 0x86);
inline const u32 kRose = rgba(0xD9, 0x54, 0x6A);

}  // namespace ec::theme
