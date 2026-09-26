// The Market (Alpha 2 WP5, content inventory section 7): food for Gleam into the pouch,
// trinkets from the hoard sold for Gleam, and the egg of the day (sex-labelled, D24; mostly
// the breeds you can't start with). It opens once a dragon has grown to Juvenile.
#pragma once

#include "core/care.hpp"
#include "core/save.hpp"
#include "core/wanderings.hpp"

namespace ec {

bool marketOpen(const SaveData& s);  // a dragon has reached Juvenile

u32 foodPrice(Food f);
// Buys one into the pouch. False if there isn't Gleam enough (or the pouch is full: 99).
bool buyFood(SaveData& s, Food f);
// Uses one from the pouch (fed by hand). False if there's none.
bool useFood(SaveData& s, Food f);
int pouchCount(const SaveData& s, Food f);

// Sells one trinket from the hoard. False if there's none.
bool sellTrinket(SaveData& s, Trinket t);

struct DailyEgg {
    Genome genome;
    Sex sex = Sex::Female;
    u8 look = kLookClassic;  // (the old looks: archived)
    u8 kind = 0;             // DR3: its kind (mostly the harder to find ones: the commons are the starters)
    u8 variant = 0;          // and its colouring (a surprise at hatching)
    u32 price = 0;           // by the kind's rarity
};
// Today's egg, the same all day for everyone with this save (from the day and the save's
// first dragon): a kind by rarity, weighted toward the harder to find.
DailyEgg dailyEgg(const SaveData& s, s32 day);
// Buys it (once a day) into a free nest, else the Cold Vault. Returns its index, or -1 (not
// enough Gleam, already bought today, the Vault is full).
int buyDailyEgg(SaveData& s, s64 now);

}  // namespace ec
