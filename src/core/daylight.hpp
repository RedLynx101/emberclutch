// Time of day in the den (WP6): which two of the room's baked lighting sets to blend
// (tools/blender/den_model.py bakes day, evening and night, in that order) and the matching
// light on the dragons, so the room and its dragons change together through the day.
#pragma once

#include "core/types.hpp"

namespace ec {

enum LightSet : u8 { kLightDay, kLightEvening, kLightNight, kLightSets };

// Colour = lerp(set a, set b, t).
struct DayBlend {
    u8 a = kLightDay, b = kLightDay;
    float t = 0;
    float weight(int set) const;  // share of a set in the mix (the three sum to 1)
};

// Night until 05:30, dawn (night -> evening glow -> day) until 08:00, day until 17:30, dusk
// (day -> evening -> night) until 21:30. Dragons go to bed at 22:00 (core/clock isNight).
DayBlend dayBlend(s64 localUnix);

// Light on the dragons (fragment lighting in src/app/render3d.cpp), each channel 0..1.
struct DragonLight {
    float ambient[3];
    float key[3];  // the directional light's colour, added where the toon ramp is lit
    float rim[3];
};
DragonLight dragonLight(const DayBlend& blend);

}  // namespace ec
