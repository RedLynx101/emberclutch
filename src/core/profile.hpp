// A dragon's profile (Alpha 2 WP8): its looks by trait, its stats, what you've found out
// about it, and its family (parents, grandparents, where its egg came from, its young).
// Pure logic: PC-tested in tests/test_main.cpp.
#pragma once

#include "core/save.hpp"

namespace ec {

// Trait names (docs/design/breeds-and-genetics.md section 3).
const char* buildName(u8 build);
const char* hornsName(u8 horns);
const char* frillName(u8 frill);
const char* wingsName(u8 wings);
const char* tailName(u8 tailTip);
const char* patternName(u8 pattern);
const char* rareName(u8 rareFlags);  // the first rare trait, or nullptr

// The three stats (GDD 5.1): training and events grow them from Beta; until then they show
// where it starts, flavoured by its breed's aptitudes and grown a little with each stage.
struct Stats {
    int wing = 0, wit = 0, spark = 0;  // 0..100
};
Stats statsOf(const Dragon& d);

// What you've found out about it: its sweet spot (scratched just right once), its favourite
// food (fed it once). Remembered in Dragon::known.
enum Known : u8 { kKnownSweetSpot = 1, kKnownFavourite = 2 };
// "under its chin", "behind its left ear"... (core/care sweetSpotOf).
const char* sweetSpotText(const Dragon& d);

// Its family, as SaveData::dragons indices (-1: not known, or no such dragon).
struct Family {
    int mother = -1, father = -1;
    int grand[4] = {-1, -1, -1, -1};  // mother's mother, mother's father, father's mother, father's father
    int young = 0;                    // eggs it has had
};
Family familyOf(const SaveData& s, const Dragon& d);
int indexOfId(const SaveData& s, u32 id);
// Where its egg came from, for a dragon without known parents.
const char* originText(const Dragon& d);

}  // namespace ec
