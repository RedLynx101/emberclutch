// Mud (D46; Alpha 2 WP12): spots of it where a dragon has been through the wet (it comes home
// from the Wanderings muddy), over the even dust that settles in the den. Brushing lifts it
// slowly, a bath at once, and it flakes off on its own over a few days. Pure logic
// (PC-tested); render3d draws both through its dirt ramp (texel (dust, mud) = the colour to
// blend toward and how far).
#pragma once

#include "core/math3d.hpp"
#include "core/types.hpp"

namespace ec {

constexpr float kMudMax = 0.8f;             // a muddy spot at its worst: this far toward the mud
constexpr Rgb kMudColor{88, 60, 38};        // wet earth
constexpr Rgb kDustColor{148, 133, 117};    // render3d's dust (dragon_texture.py DIRT_COLOR)
constexpr float kMudFlakesPerHour = 1.5f;   // levels a day or so of den life knocks off

// How much of a region's mud lands at this rest-pose point (0..1): blotches a few scales
// across over about half the body, smooth at their edges; the same spots every time.
float mudSpots(Vec3 rest);

// The dirt ramp's texel for a dust factor (0..1, already scaled by kDirtMax) and a mud level
// (0..1, of kMudMax): the colour to blend toward and how far, the mud over the dust.
void dirtTexel(float dust, float mud, Rgb& colour, float& amount);

}  // namespace ec
