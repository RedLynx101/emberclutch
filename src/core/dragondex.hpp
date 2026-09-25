// The Dragondex (D55, D66; Alpha 2 WP12): the collection book of every breed in every look
// (21 x 4 = 84) and the four rare traits, filled in as your dragons hatch. Seeing all four
// looks of a breed completes it: some Gleam and a banner for the den in that breed's
// colours. Pure logic (PC-tested); src/app/dragondex_ui.cpp shows it.
#pragma once

#include "core/save.hpp"

namespace ec {

constexpr int kDexEntries = kBreedCount * kLookCount;  // 84
constexpr u32 kDexBreedGleam = 150;                   // the reward for a completed breed

struct DexNews {
    bool newEntry = false;  // this breed and look weren't in the book yet
    bool newRare = false;   // nor one of its rare traits
    int completed = -1;     // a breed it completed (its index), rewarded now
};

// A hatched dragon is in the book (eggs don't count: the look is a surprise until it hatches).
// Completing a breed pays kDexBreedGleam and gives its banner (hung at once if the banner spot
// has no breed banner yet).
DexNews dexSee(SaveData& s, const Dragon& d);
// Every hatched dragon in the save (a save from before the Dragondex). No rewards are missed:
// completions pay as usual.
void dexSeeAll(SaveData& s);

bool dexHas(const SaveData& s, int breed, int look);
bool dexComplete(const SaveData& s, int breed);  // every look seen (and rewarded)
bool dexRare(const SaveData& s, u8 rareFlag);
int dexCount(const SaveData& s);                  // entries seen, of kDexEntries

// The breed banner hung in the den's banner spot (-1: none; the spot shows its decor, if any).
int bannerBreed(const SaveData& s);
bool hangBanner(SaveData& s, int breed);  // false unless the breed is complete
void takeDownBanner(SaveData& s);

// The dragon the book shows for an entry: a grown one of that breed in that look (the same
// every time).
Dragon dexDragon(int breed, int look);
// A breed's colours for its banner: its body, its accent, its heartglow.
void breedColours(int breed, Rgb& base, Rgb& accent, Rgb& glow);

}  // namespace ec
