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
    int dragonCount = 0, eggCount = 0;
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

// The bed a hatchling would take, or -1: with all three beds taken, a ready egg waits.
int bedForHatchling(const SaveData& s);

constexpr int kVaultEggs = 50;  // GDD 8

// Out of the den: a dragon to the Sanctuary, an egg to the Cold Vault (false if the Vault is
// full, or it isn't in the den).
bool storeAway(SaveData& s, int index);
// Back into the den: a free bed or nest. False if there's none (or it's already home).
bool bringHome(SaveData& s, int index, s64 now);
int vaultCount(const SaveData& s);

}  // namespace ec
