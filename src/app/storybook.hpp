// Painted 2D scenery in the storybook look (1.0, D89; workstream U) for the top screens drawn in
// 2D, the Market's square and the Wanderings' trail: a sky by the time of day with its sun, moon
// and stars, drifting clouds, rolling hills and far mountains, cottages, round trees, bunting,
// paper lanterns, a hanging wooden sign for the title, a parchment caption, and little pictures
// of the Wanderings' finds. Colours are theme::paint's, lit by the time of day; far layers sit
// behind the screen in 3D (r3d::eyeShift).
#pragma once

#include "app/app.hpp"
#include "core/wanderings.hpp"

namespace ec::paint {

// The time of day's light, found once a frame; every colour below is lit by it.
struct Light {
    Rgb top, horizon, tint;
    float day = 1, evening = 0, night = 0;  // shares, summing to 1
};
Light lightFor(s64 now);
u32 lit(const Light& l, Rgb c, float a = 1.0f);

// The sky down to horizonY: its gradient, the sun (low and warm at evening) or the moon and stars.
void sky(const Light& l, float t, float horizonY);
// A few soft clouds drifting across near `y`; depth > 1 sits them behind the screen in 3D.
void clouds(const Light& l, float t, float y, float depth);
// A row of far mountains with snow on their tops, standing on baseY.
void mountains(const Light& l, float baseY, float depth);
// A rolling hill: a wide ellipse whose top is at topY (what's below it is covered by what's nearer).
void hill(const Light& l, float cx, float topY, float w, float h, Rgb c, float depth);
// A round tree standing on groundY, s pixels tall; shade 0..2 for a little variety.
void tree(const Light& l, float x, float groundY, float s, int shade, float depth = 1.0f);
// A cottage (walls w by h, its roof above), its window lit at night.
void cottage(const Light& l, float x, float groundY, float w, float h, Rgb roof, float t, float depth);
// Bunting on a sagging string from x0 to x1 (flags stirring in the breeze; little lights at night).
void bunting(const Light& l, float x0, float x1, float y, float sag, float t);
// A paper lantern hung at (x, y), glowing more after dark.
void lantern(const Light& l, float x, float y, float s, float t);
// The scene's title on a wooden sign hanging from the top edge by two ropes.
void hangingSign(App& app, const char* title, float cx, float y, float w);
// A parchment card low on the screen: a line and (if any) a smaller one under it.
void caption(App& app, const char* line1, const char* line2, float y);
// The Wanderings' finds: a trinket's little picture, and a pile of Gleam.
void trinket(Trinket t, float x, float y, float s);
void coinPile(float x, float y, float s, int coins);

}  // namespace ec::paint
