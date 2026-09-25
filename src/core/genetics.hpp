// Breeds & genetics — see docs/design/breeds-and-genetics.md.
#pragma once

#include "core/rng.hpp"
#include "core/types.hpp"

namespace ec {

enum Build : u8 { kBuildSturdy, kBuildSleek, kBuildLong, kBuildCount };
enum Horns : u8 { kHornsNubs, kHornsSwept, kHornsCrown, kHornsCrystal, kHornsAntler, kHornsCount };
enum Frill : u8 { kFrillNone, kFrillFin, kFrillLeaf, kFrillFeather, kFrillCount };
enum Wings : u8 { kWingsClassic, kWingsPlumed, kWingsSail, kWingsCount };
enum TailTip : u8 { kTailPlain, kTailSpade, kTailTuft, kTailFan, kTailCount };
enum Pattern : u8 { kPatternSolid, kPatternStripes, kPatternSpots, kPatternDapple, kPatternRunes, kPatternCount };

enum RareFlag : u8 {
    kRareIridescent = 1 << 0,
    kRareMelanistic = 1 << 1,
    kRareLeucistic = 1 << 2,
    kRareStarspeckle = 1 << 3,
};

// 16 bytes, stored verbatim in the save file. Colours are HSV bytes.
struct Genome {
    u8 elementA, elementB;  // Element alleles. Base colour follows A, accent follows B.
    u8 build, horns, frill, wings, tailTip, pattern;
    u8 baseH, baseS, baseV;
    u8 accentH, accentV;  // accent saturation follows base
    u8 patternH;
    u8 size;  // 0..255 -> 0.90..1.10 scale
    u8 rareFlags;
};
static_assert(sizeof(Genome) == 16, "Genome is part of the save format");

// The 21 breeds in one order (the Dragondex's): the six purebreds by element, then the 15
// hybrids along the table (Steam, Wildfire, ... Prism).
constexpr int kBreedCount = 21;
int breedIndex(const Genome& g);
void breedAlleles(int index, u8& a, u8& b);  // index 0..20 -> its two elements (a <= b)

// Order-independent: breedName(Ember, Tide) == breedName(Tide, Ember) == "Steam".
const char* breedName(Element a, Element b);
const char* breedName(const Genome& g);
bool isHybrid(const Genome& g);

// A fresh dragon of a pure breed (starter eggs, Market and wild eggs).
Genome makePurebred(Element e, Rng& rng);

// Offspring of two parents. See the rules in breeds-and-genetics.md §2–3.
Genome breed(const Genome& a, const Genome& b, Rng& rng);

// The heartglow colour of an element. Hybrids swirl their two elements' colours.
Rgb heartglowColor(Element e);

float sizeScale(const Genome& g);
int rareCount(u8 flags);

// The looks (D54, R5's outcome): every dragon has one, set when its egg is laid, revealed at
// hatching. Three common looks and a rarer wild one, each drawn by its own models
// (romfs:/models/, v1/, v2/, v3/). Stored on the dragon record, not in the genome.
enum Look : u8 { kLookClassic, kLookPebbleback, kLookTallneck, kLookWild, kLookCount };
// Base odds (starter, Market and wild eggs): 31 / 31 / 30 / 8 %.
Look rollLook(Rng& rng);
// An egg's look from its parents': usually one parent's (50/50), sometimes a fresh one;
// wild stays rare (the fresh roll's 8%) unless a parent is wild (then about 25%).
Look inheritLook(u8 a, u8 b, Rng& rng);
// A save from before the looks: a look by the base odds, the same one every time for a dragon.
Look lookForOldDragon(u32 id);
// The look's own word: "Classic", "Pebbleback", "Tallneck", or the wild one for the dragon's
// base element (its allele A: the body's colour): Cinderveined, Glimmertide, Stormstreak,
// Mossglow, Rimelight, Starveined.
const char* lookName(u8 look, const Genome& g);
// Look + breed: "Classic Ember", "Pebbleback Steam", "Cinderveined Ember"; the wild words that
// are names in their own right stand alone on a purebred ("Glimmertide") and lead a hybrid's
// breed ("Glimmertide Squall").
void lookBreedName(u8 look, const Genome& g, char* out, int cap);

}  // namespace ec
