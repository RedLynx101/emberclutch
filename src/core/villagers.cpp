#include "core/villagers.hpp"

#include <cstdio>
#include <cstring>

#include "core/valley.hpp"
#include "core/world.hpp"

namespace ec {
namespace {

// Voices (Noah, 2026-09-28): 0 is the alphabet made from his own voice, for the men; 1 the
// recording he sent for the women and the child (romfs/voice/v1, v2).
const VillagerInfo kVillagers_[kVillagers] = {
    {"keeper", "Old Rowan", "the valley's keeper", kPlaceKeeper, {2.5f, 6.0f}, 0.0f, 0, 1.15f},
    {"market", "Maple", "keeps the Market", kPlaceMarket, {3.0f, -2.0f}, 0.3f, 1, 1.45f},
    {"sanctuary", "Bram", "keeps the Sanctuary", kPlaceSanctuary, {-2.0f, 8.0f}, 0.0f, 0, 1.3f},
    {"steward", "Wren", "the arena's steward", kPlaceArena, {4.0f, 24.0f}, 0.0f, 1, 1.35f},
    {"child", "Pip", "loves dragons", kPlaceMarket, {-8.0f, 6.0f}, -0.6f, 1, 1.8f},
    {"traveller", "Sable", "a traveller", kPlaceTrailhead, {-4.0f, 5.0f}, 0.4f, 0, 1.3f},
};

}  // namespace

const VillagerInfo& villagerInfo(Villager v) {
    return kVillagers_[static_cast<int>(v) < kVillagers ? static_cast<int>(v) : 0];
}

void fillLine(const char* line, const SaveData& s, char* out, int cap) {
    const char* partner = "your dragon";
    for (int i = 0; i < s.dragonCount; ++i)
        if (s.dragons[i].id == s.world.partnerId && s.dragons[i].name[0]) partner = s.dragons[i].name;
    const char* you = s.playerName[0] ? s.playerName : "friend";
    int n = 0;
    for (const char* p = line; *p && n < cap - 1; ++p) {
        if (p[0] == '{' && (p[1] == 'D' || p[1] == 'P') && p[2] == '}') {
            for (const char* q = p[1] == 'D' ? partner : you; *q && n < cap - 1; ++q) out[n++] = *q;
            p += 2;
        } else {
            out[n++] = *p;
        }
    }
    out[n] = 0;
}

}  // namespace ec
