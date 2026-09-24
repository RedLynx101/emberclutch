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

}  // namespace ec
