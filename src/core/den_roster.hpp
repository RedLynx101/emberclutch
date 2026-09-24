// Who lives in the den (Alpha 2 WP1): up to three hatched dragons, one to a bed, and two
// eggs, one to an egg nest. Each keeps its place (Dragon::denSlot); anyone beyond the den's
// room goes out to the Sanctuary (dragons) or the Cold Vault (eggs).
#pragma once

#include "core/save.hpp"

namespace ec {

constexpr int kDenDragons = 3;  // DenLayout::kSpots
constexpr int kDenEggs = 2;     // DenLayout::kNests

struct DenRoster {
    int dragon[kDenDragons];  // SaveData::dragons index by bed, -1: an empty bed
    int egg[kDenEggs];        // by egg nest
    bool away[kDenDragons];   // its dragon is out on the Wanderings (the bed is kept for it)
    int dragonCount = 0, eggCount = 0;
    int presentCount() const;  // dragons in the den right now (not away)
    int freeBed() const;   // the first empty bed, or -1
    int freeNest() const;  // the first empty nest, or -1
};

// Gives everyone in the den a place of their own (keeping the places they have). Returns how
// many had to move out because the den was full.
int settleDen(SaveData& s);

// The den as it is (call settleDen first after anything moves).
DenRoster denRoster(const SaveData& s);

// A new egg goes into a free nest, else the Cold Vault. True if it's in the den.
bool placeEgg(SaveData& s, Dragon& egg);

// The bed a hatchling would take, or -1: a free bed, else the bed of a dragon out on the
// Wanderings (it doesn't need it while it's away). With all three beds slept in, a ready egg
// waits. (A ready egg waited behind a wanderer's empty bed, with no way to see why: Noah,
// 2026-09-24.)
int bedForHatchling(const SaveData& s);
// Frees that bed for the hatchling: if it was a wanderer's, the wanderer moves to the
// Sanctuary (its trip goes on, and it comes home there). Returns the bed, or -1.
int makeRoomForHatchling(SaveData& s);

constexpr int kVaultEggs = 50;  // GDD 8

// Out of the den: a dragon to the Sanctuary, an egg to the Cold Vault (false if the Vault is
// full, or it isn't in the den).
bool storeAway(SaveData& s, int index);
// Back into the den: a free bed or nest. False if there's none (or it's already home).
bool bringHome(SaveData& s, int index, s64 now);
int vaultCount(const SaveData& s);

}  // namespace ec
