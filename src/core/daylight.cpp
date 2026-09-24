#include "core/daylight.hpp"

#include "core/clock.hpp"

namespace ec {
namespace {

// Per lighting set: day is the look the dragons were designed in (R1/R2 reviews); evening
// warms and dims, night turns blue and dim but stays readable on the 3DS screen.
constexpr DragonLight kLights[kLightSets] = {
    {{0.42f, 0.36f, 0.44f}, {0.57f, 0.62f, 0.53f}, {90 / 255.0f, 64 / 255.0f, 40 / 255.0f}},
    {{0.44f, 0.33f, 0.36f}, {0.66f, 0.46f, 0.30f}, {110 / 255.0f, 62 / 255.0f, 30 / 255.0f}},
    {{0.24f, 0.25f, 0.40f}, {0.33f, 0.37f, 0.56f}, {78 / 255.0f, 54 / 255.0f, 48 / 255.0f}},
};

float smooth(float x) {
    x = x < 0 ? 0 : (x > 1 ? 1 : x);
    return x * x * (3 - 2 * x);
}

DayBlend between(LightSet a, LightSet b, float hour, float from, float to) {
    return {a, b, smooth((hour - from) / (to - from))};
}

}  // namespace

float DayBlend::weight(int set) const {
    float w = 0;
    if (set == a) w += 1 - t;
    if (set == b) w += t;
    return w;
}

DayBlend dayBlend(s64 localUnix) {
    const float hour = static_cast<float>(localUnix - dayIndex(localUnix) * kDay) / kHour;
    if (hour < 5.5f) return {kLightNight, kLightNight, 0};
    if (hour < 6.75f) return between(kLightNight, kLightEvening, hour, 5.5f, 6.75f);
    if (hour < 8.0f) return between(kLightEvening, kLightDay, hour, 6.75f, 8.0f);
    if (hour < 17.5f) return {kLightDay, kLightDay, 0};
    if (hour < 19.25f) return between(kLightDay, kLightEvening, hour, 17.5f, 19.25f);
    if (hour < 21.5f) return between(kLightEvening, kLightNight, hour, 19.25f, 21.5f);
    return {kLightNight, kLightNight, 0};
}

DragonLight dragonLight(const DayBlend& blend) {
    DragonLight out{};
    for (int s = 0; s < kLightSets; ++s) {
        const float w = blend.weight(s);
        for (int k = 0; k < 3; ++k) {
            out.ambient[k] += kLights[s].ambient[k] * w;
            out.key[k] += kLights[s].key[k] * w;
            out.rim[k] += kLights[s].rim[k] * w;
        }
    }
    return out;
}

}  // namespace ec
