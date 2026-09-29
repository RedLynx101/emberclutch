#include "core/routines.hpp"

namespace ec::routine {
namespace {

Doing doing(const char* clip, const char* now = nullptr, bool seated = false, bool asleep = false) {
    Doing d;
    d.clip = clip;
    d.now = now;
    d.seated = seated;
    d.asleep = asleep;
    return d;
}

bool between(int hour, int from, int until) { return from <= until ? hour >= from && hour < until : hour >= from || hour < until; }

}  // namespace

Doing villager(Villager v, int hour) {
    hour = ((hour % 24) + 24) % 24;
    switch (v) {
        case Villager::Keeper:  // Old Rowan by his lodge: a morning stretch, the evening on the porch step
            if (between(hour, 21, 6)) return doing("doze", nullptr, true, true);
            if (between(hour, 17, 21)) return doing("sit_ground", nullptr, true);
            return doing("idle", between(hour, 6, 9) ? "stretch" : "look_around");
        case Villager::Market:  // Maple sets out her stall in the morning and packs it up at dusk
            if (between(hour, 21, 6)) return doing("doze_stand", nullptr, false, true);
            if (between(hour, 6, 10) || between(hour, 18, 21)) return doing("tidy", "look_around");
            return doing("idle", between(hour, 12, 15) ? "stretch" : "look_around");
        case Villager::Sanctuary:  // Bram feeds the resting dragons morning and afternoon
            if (between(hour, 22, 6)) return doing("doze", nullptr, true, true);
            if (between(hour, 19, 22)) return doing("sit_ground", nullptr, true);
            if (between(hour, 6, 10) || between(hour, 16, 19)) return doing("scatter");
            return doing("idle", "look_around");
        case Villager::Steward:  // Wren keeps the arena's book all day
            if (between(hour, 22, 7)) return doing("doze_stand", nullptr, false, true);
            if (between(hour, 19, 22)) return doing("idle", "stretch");
            return doing("write", "look_around");
        case Villager::Child:  // Pip flies his toy dragon about, sits down tired in the evening
            if (between(hour, 21, 8)) return doing("doze", nullptr, true, true);
            if (between(hour, 19, 21)) return doing("sit_ground", nullptr, true);
            if (between(hour, 12, 15)) return doing("idle", "cheer");
            return doing("fly_toy");
        case Villager::Traveller:  // Sable rests by the trailhead's camp in the evening
            if (between(hour, 22, 6)) return doing("doze", nullptr, true, true);
            if (between(hour, 17, 22)) return doing("sit_ground", nullptr, true);
            return doing("idle", between(hour, 6, 9) ? "stretch" : "look_around");
        case Villager::Count: break;
    }
    return doing("idle");
}

float nextGap(u32 seed) {
    seed = seed * 1103515245u + 12345u;
    return 8.0f + static_cast<float>((seed >> 8) % 1200u) / 100.0f;
}

}  // namespace ec::routine
