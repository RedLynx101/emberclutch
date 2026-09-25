#include "core/dragondex.hpp"

namespace ec {

namespace {

Genome breedGenome(int index) {
    u8 a, b;
    breedAlleles(index, a, b);
    Rng rng(0xD1A6u + static_cast<u32>(index) * 977u);
    const Genome pa = makePurebred(static_cast<Element>(a), rng);
    if (a == b) return pa;
    const Genome pb = makePurebred(static_cast<Element>(b), rng);
    return ec::breed(pa, pb, rng);  // allele A from the first, B from the second: this hybrid
}

}  // namespace

DexNews dexSee(SaveData& s, const Dragon& d) {
    DexNews news;
    if (d.stage == Stage::Egg) return news;
    const int b = breedIndex(d.genome);
    const int look = d.look < kLookCount ? static_cast<int>(d.look) : static_cast<int>(kLookClassic);
    const u8 bit = static_cast<u8>(1u << look);
    if (!(s.dexLooks[b] & bit)) {
        s.dexLooks[b] = static_cast<u8>(s.dexLooks[b] | bit);
        news.newEntry = true;
    }
    const u8 rares = static_cast<u8>(d.genome.rareFlags & 0x0F);
    if (rares & ~s.dexRares) {
        s.dexRares = static_cast<u8>(s.dexRares | rares);
        news.newRare = true;
    }
    const u32 done = 1u << b;
    if (s.dexLooks[b] == (1u << kLookCount) - 1 && !(s.dexDone & done)) {
        s.dexDone |= done;
        s.gleam += kDexBreedGleam;
        if (s.bannerBreed == 0xFF) s.bannerBreed = static_cast<u8>(b);  // up at once, the first time
        news.completed = b;
    }
    return news;
}

void dexSeeAll(SaveData& s) {
    for (int i = 0; i < s.dragonCount; ++i) dexSee(s, s.dragons[i]);
}

bool dexHas(const SaveData& s, int breed, int look) {
    return breed >= 0 && breed < kBreedCount && look >= 0 && look < kLookCount && (s.dexLooks[breed] >> look) & 1;
}

bool dexComplete(const SaveData& s, int breed) { return breed >= 0 && breed < kBreedCount && (s.dexDone >> breed) & 1; }

bool dexRare(const SaveData& s, u8 rareFlag) { return (s.dexRares & rareFlag) != 0; }

int dexCount(const SaveData& s) {
    int n = 0;
    for (int b = 0; b < kBreedCount; ++b)
        for (int l = 0; l < kLookCount; ++l) n += (s.dexLooks[b] >> l) & 1;
    return n;
}

int bannerBreed(const SaveData& s) { return s.bannerBreed < kBreedCount ? s.bannerBreed : -1; }

bool hangBanner(SaveData& s, int breed) {
    if (!dexComplete(s, breed)) return false;
    s.bannerBreed = static_cast<u8>(breed);
    return true;
}

void takeDownBanner(SaveData& s) { s.bannerBreed = 0xFF; }

Dragon dexDragon(int breed, int look) {
    Dragon d = makeEgg(0xFFFFFF00u + static_cast<u32>(breed * kLookCount + look), breedGenome(breed), Sex::Male, 0,
                       static_cast<u8>(look));
    d.stage = Stage::Adult;
    d.incubationSeconds = kIncubationSeconds;
    d.needs = Needs{90, 90, 90, 90};
    return d;
}

void breedColours(int breed, Rgb& base, Rgb& accent, Rgb& glow) {
    const Genome g = breedGenome(breed);
    base = hsvToRgb(g.baseH, g.baseS, g.baseV);
    accent = hsvToRgb(g.accentH, static_cast<u8>(g.baseS / 2), g.accentV);
    glow = heartglowColor(static_cast<Element>(g.elementA));
}

}  // namespace ec
