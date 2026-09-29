#include "core/dragondex.hpp"

namespace ec {

namespace {

bool validKind(int k) { return k >= 0 && k < kindCount() && k < kDexKindSlots; }

}  // namespace

int dexEntries() { return kindCount() * kKindVariants; }

DexNews dexSee(SaveData& s, const Dragon& d) {
    DexNews news;
    if (d.stage == Stage::Egg || !validKind(d.kind)) return news;
    const int k = d.kind, v = d.variant % kKindVariants;
    const u8 bit = static_cast<u8>(1u << v);
    if (!(s.dexKinds[k] & bit)) {
        s.dexKinds[k] = static_cast<u8>(s.dexKinds[k] | bit);
        news.newEntry = true;
        news.newRare = v == kindInfo(k).rareVariant;
    }
    const u64 done = 1ull << k;
    if (s.dexKinds[k] == (1u << kKindVariants) - 1 && !(s.dexKindsDone & done)) {
        s.dexKindsDone |= done;
        s.gleam += kDexBreedGleam;
        if (s.bannerKind == 0xFF) s.bannerKind = static_cast<u8>(k);  // up at once, the first time
        news.completed = k;
    }
    return news;
}

void dexSeeAll(SaveData& s) {
    for (int i = 0; i < s.dragonCount; ++i) dexSee(s, s.dragons[i]);
}

bool dexHas(const SaveData& s, int kind, int variant) {
    return validKind(kind) && variant >= 0 && variant < kKindVariants && (s.dexKinds[kind] >> variant) & 1;
}

bool dexComplete(const SaveData& s, int kind) { return validKind(kind) && (s.dexKindsDone >> kind) & 1; }

int dexCount(const SaveData& s) {
    int n = 0;
    for (int k = 0; k < kindCount() && k < kDexKindSlots; ++k)
        for (int v = 0; v < kKindVariants; ++v) n += (s.dexKinds[k] >> v) & 1;
    return n;
}

int dexRareCount(const SaveData& s) {
    int n = 0;
    for (int k = 0; k < kindCount() && k < kDexKindSlots; ++k) n += (s.dexKinds[k] >> kindInfo(k).rareVariant) & 1;
    return n;
}

int bannerKind(const SaveData& s) { return s.bannerKind < kindCount() ? s.bannerKind : -1; }

bool hangBanner(SaveData& s, int kind) {
    if (!dexComplete(s, kind)) return false;
    s.bannerKind = static_cast<u8>(kind);
    return true;
}

void takeDownBanner(SaveData& s) { s.bannerKind = 0xFF; }

Dragon dexDragon(int kind, int variant, Stage stage) {
    const u32 id = 0xFFFFFF00u + static_cast<u32>(kind * kKindVariants + variant);
    Dragon d = makeEgg(id, Genome{}, Sex::Male, 0);
    Rng rng(0xD1A6u + id * 977u);
    rollKind(d, kind, variant, rng);
    d.stage = stage == Stage::Egg ? Stage::Adult : stage;
    d.incubationSeconds = kIncubationSeconds;
    d.needs = Needs{90, 90, 90, 90, 90};
    return d;
}

Stage dexStage(const SaveData& s, int kind, int variant) {
    Stage oldest = Stage::Egg;
    for (int i = 0; i < s.dragonCount; ++i) {
        const Dragon& d = s.dragons[i];
        if (d.kind == kind && d.variant == variant && d.stage > oldest) oldest = d.stage;
    }
    return oldest == Stage::Egg ? Stage::Adult : oldest;
}

void kindColours(int kind, Rgb& base, Rgb& accent, Rgb& glow) {
    const KindInfo& k = kindInfo(validKind(kind) ? kind : 0);
    base = k.variants[0].pal[kPalBase];
    accent = k.variants[0].pal[kPalAccent];
    glow = elementGlow(k.elements[0]);
}

}  // namespace ec
