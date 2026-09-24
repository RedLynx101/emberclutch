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

}  // namespace ec
