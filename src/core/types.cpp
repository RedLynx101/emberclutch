#include "core/types.hpp"

namespace ec {

Rgb hsvToRgb(u8 h, u8 s, u8 v) {
    if (s == 0) return {v, v, v};
    const int region = h / 43;
    const int remainder = (h - region * 43) * 6;
    const u8 p = static_cast<u8>((v * (255 - s)) >> 8);
    const u8 q = static_cast<u8>((v * (255 - ((s * remainder) >> 8))) >> 8);
    const u8 t = static_cast<u8>((v * (255 - ((s * (255 - remainder)) >> 8))) >> 8);
    switch (region) {
        case 0: return {v, t, p};
        case 1: return {q, v, p};
        case 2: return {p, v, t};
        case 3: return {p, q, v};
        case 4: return {t, p, v};
        default: return {v, p, q};
    }
}

const char* elementName(Element e) {
    static constexpr const char* kNames[] = {"Ember", "Tide", "Gale", "Grove", "Frost", "Lumen"};
    const int i = static_cast<int>(e);
    return i < kElementCount ? kNames[i] : "?";
}

const char* stageName(Stage s) {
    static constexpr const char* kNames[] = {"Egg", "Hatchling", "Juvenile", "Adolescent", "Adult"};
    return kNames[static_cast<int>(s)];
}

const char* moodName(Mood m) {
    static constexpr const char* kNames[] = {"Upset", "Sulky", "Restless", "Content", "Joyful"};
    return kNames[static_cast<int>(m)];
}

const char* personalityName(Personality p) {
    static constexpr const char* kNames[] = {"Brave", "Shy", "Playful", "Proud", "Sleepy", "Curious"};
    const int i = static_cast<int>(p);
    return i < static_cast<int>(Personality::Count) ? kNames[i] : "?";
}

const char* sexName(Sex s) { return s == Sex::Male ? "Male" : "Female"; }

}  // namespace ec
