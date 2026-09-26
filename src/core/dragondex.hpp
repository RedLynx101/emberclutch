// The Dragondex (D55, D66; WP12; by kind since DR3, D80): the collection book of every kind in
// every colouring (9 x 4 now, 36 x 4 in the end), filled in as your dragons hatch. Seeing all
// four colourings of a kind completes it: some Gleam and a banner for the den in that kind's
// colours. Pure logic (PC-tested); src/app/dragondex_ui.cpp shows it.
#pragma once

#include "core/kinds.hpp"
#include "core/save.hpp"

namespace ec {

constexpr u32 kDexBreedGleam = 150;  // the reward for a completed kind

int dexEntries();  // kinds x colourings

struct DexNews {
    bool newEntry = false;  // this kind and colouring weren't in the book yet
    bool newRare = false;   // its rare colouring, seen for the first time
    int completed = -1;     // a kind it completed (its index), rewarded now
};

// A hatched dragon is in the book (eggs don't count: the colouring is a surprise until it
// hatches). Completing a kind pays kDexBreedGleam and gives its banner (hung at once if the
// banner spot has no banner yet).
DexNews dexSee(SaveData& s, const Dragon& d);
// Every hatched dragon in the save (a save from before this book). Completions pay as usual.
void dexSeeAll(SaveData& s);

bool dexHas(const SaveData& s, int kind, int variant);
bool dexComplete(const SaveData& s, int kind);  // every colouring seen (and rewarded)
int dexCount(const SaveData& s);                // entries seen, of dexEntries()
int dexRareCount(const SaveData& s);            // rare colourings seen

// The kind's banner hung in the den's banner spot (-1: none; the spot shows its decor, if any).
int bannerKind(const SaveData& s);
bool hangBanner(SaveData& s, int kind);  // false unless the kind is complete
void takeDownBanner(SaveData& s);

// The dragon the book shows for an entry: a grown one of that kind in that colouring (the
// same every time).
Dragon dexDragon(int kind, int variant);
// A kind's colours for its banner: its body, its accent, its element's glow.
void kindColours(int kind, Rgb& base, Rgb& accent, Rgb& glow);

}  // namespace ec
