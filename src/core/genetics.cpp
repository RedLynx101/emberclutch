#include "core/genetics.hpp"

namespace ec {
namespace {

// Per-element breed profile: palette ranges and favoured parts.
struct BreedProfile {
    u8 baseH, baseRange, baseS, baseV;
    u8 accentH, accentRange, accentV;
    u8 patternH;
    u8 build, horns, frill, wings, tailTip;
    u8 patterns[2];
};

// Hue bytes: 0..255 == 0..360 degrees.
constexpr BreedProfile kProfiles[kElementCount] = {
    // Ember: ember-orange, cream-gold accents, swept horns, spade tail.
    {13, 9, 205, 228, 30, 8, 240, 30, kBuildSturdy, kHornsSwept, kFrillNone, kWingsClassic, kTailSpade,
     {kPatternStripes, kPatternSolid}},
    // Tide: sea-teal, fin frills and sail wings, long build.
    {128, 10, 170, 195, 138, 8, 232, 110, kBuildLong, kHornsNubs, kFrillFin, kWingsSail, kTailFan,
     {kPatternSpots, kPatternDapple}},
    // Gale: pale sky-blue, feather frill and plumed wings, sleek.
    {146, 10, 120, 238, 152, 8, 252, 160, kBuildSleek, kHornsSwept, kFrillFeather, kWingsPlumed, kTailTuft,
     {kPatternDapple, kPatternSolid}},
    // Grove: moss green, leafy frill, antler horns.
    {66, 10, 150, 172, 45, 8, 215, 30, kBuildSturdy, kHornsAntler, kFrillLeaf, kWingsClassic, kTailFan,
     {kPatternDapple, kPatternSpots}},
    // Frost: white-lavender, crystal horns, runes.
    {185, 10, 62, 240, 180, 8, 255, 190, kBuildSleek, kHornsCrystal, kFrillNone, kWingsClassic, kTailSpade,
     {kPatternRunes, kPatternSolid}},
    // Lumen: gold-white, crown horns, plumed wings.
    {34, 8, 72, 250, 36, 8, 255, 40, kBuildSleek, kHornsCrown, kFrillFeather, kWingsPlumed, kTailTuft,
     {kPatternRunes, kPatternSolid}},
};

// Upper triangle of the breed table, indexed [min][max].
constexpr const char* kBreedNames[kElementCount][kElementCount] = {
    {"Ember", "Steam", "Wildfire", "Cinderbloom", "Solstice", "Sunflare"},
    {nullptr, "Tide", "Squall", "Lotus", "Glacier", "Pearl"},
    {nullptr, nullptr, "Gale", "Thistledown", "Blizzard", "Aurora"},
    {nullptr, nullptr, nullptr, "Grove", "Evergreen", "Glowmoss"},
    {nullptr, nullptr, nullptr, nullptr, "Frost", "Prism"},
    {nullptr, nullptr, nullptr, nullptr, nullptr, "Lumen"},
};

const BreedProfile& profile(u8 element) { return kProfiles[element % kElementCount]; }

int hueDelta(u8 from, u8 to) { return static_cast<int>(static_cast<std::int8_t>(static_cast<u8>(to - from))); }

u8 clampHue(u8 h, u8 center, u8 range) {
    const int d = hueDelta(center, h);
    if (d > range) return static_cast<u8>(center + range);
    if (d < -range) return static_cast<u8>(center - range);
    return h;
}

u8 clampByte(int v) { return static_cast<u8>(v < 0 ? 0 : (v > 255 ? 255 : v)); }

u8 inheritHue(u8 a, u8 b, Rng& rng) {
    if (rng.chance(15, 100)) return rng.chance(1, 2) ? a : b;  // lines can breed true
    const u8 mid = static_cast<u8>(a + hueDelta(a, b) / 2);
    return static_cast<u8>(mid + rng.range(-6, 6));  // about ±8 degrees
}

u8 inheritValue(u8 a, u8 b, Rng& rng) {
    if (rng.chance(15, 100)) return rng.chance(1, 2) ? a : b;
    return clampByte((a + b) / 2 + rng.range(-8, 8));
}

// Parts rule: parent A 45%, parent B 45%, fresh roll from the child's breed pool 10%.
u8 inheritPart(u8 a, u8 b, u8 favouredA, u8 favouredB, u8 count, Rng& rng) {
    const u32 roll = rng.below(100);
    if (roll < 45) return a;
    if (roll < 90) return b;
    if (rng.chance(70, 100)) return rng.chance(1, 2) ? favouredA : favouredB;
    return static_cast<u8>(rng.below(count));
}

u8 inheritRare(u8 a, u8 b, Rng& rng) {
    static constexpr struct {
        u8 flag;
        u32 spontaneousDen;
    } kRares[] = {
        {kRareIridescent, 64}, {kRareMelanistic, 64}, {kRareLeucistic, 64}, {kRareStarspeckle, 128}};

    u8 out = 0;
    for (const auto& r : kRares) {
        const bool inherited = ((a | b) & r.flag) && rng.chance(1, 2);
        if (inherited || rng.chance(1, r.spontaneousDen)) out |= r.flag;
    }
    // Melanistic and leucistic are opposites; keep one.
    if ((out & kRareMelanistic) && (out & kRareLeucistic))
        out &= static_cast<u8>(~(rng.chance(1, 2) ? kRareMelanistic : kRareLeucistic));
    // At most two rare traits.
    while (rareCount(out) > 2) {
        const u8 bit = static_cast<u8>(1u << rng.below(4));
        out &= static_cast<u8>(~bit);
    }
    return out;
}

}  // namespace

const char* breedName(Element a, Element b) {
    int i = static_cast<int>(a) % kElementCount;
    int j = static_cast<int>(b) % kElementCount;
    if (i > j) {
        const int t = i;
        i = j;
        j = t;
    }
    return kBreedNames[i][j];
}

const char* breedName(const Genome& g) {
    return breedName(static_cast<Element>(g.elementA), static_cast<Element>(g.elementB));
}

bool isHybrid(const Genome& g) { return g.elementA != g.elementB; }

Genome makePurebred(Element e, Rng& rng) {
    const u8 el = static_cast<u8>(e);
    const BreedProfile& p = profile(el);
    Genome g{};
    g.elementA = g.elementB = el;
    g.build = p.build;
    g.horns = p.horns;
    g.frill = p.frill;
    g.wings = p.wings;
    g.tailTip = p.tailTip;
    g.pattern = p.patterns[rng.below(2)];
    g.baseH = static_cast<u8>(p.baseH + rng.range(-p.baseRange / 2, p.baseRange / 2));
    g.baseS = clampByte(p.baseS + rng.range(-10, 10));
    g.baseV = clampByte(p.baseV + rng.range(-10, 10));
    g.accentH = static_cast<u8>(p.accentH + rng.range(-p.accentRange / 2, p.accentRange / 2));
    g.accentV = clampByte(p.accentV + rng.range(-8, 8));
    g.patternH = static_cast<u8>(p.patternH + rng.range(-6, 6));
    g.size = clampByte(128 + rng.range(-20, 20));
    g.rareFlags = inheritRare(0, 0, rng);
    return g;
}

Genome breed(const Genome& a, const Genome& b, Rng& rng) {
    Genome c{};
    // Mendelian elements: one random allele from each parent.
    c.elementA = rng.chance(1, 2) ? a.elementA : a.elementB;
    c.elementB = rng.chance(1, 2) ? b.elementA : b.elementB;

    const BreedProfile& pa = profile(c.elementA);
    const BreedProfile& pb = profile(c.elementB);

    c.build = inheritPart(a.build, b.build, pa.build, pb.build, kBuildCount, rng);
    c.horns = inheritPart(a.horns, b.horns, pa.horns, pb.horns, kHornsCount, rng);
    c.frill = inheritPart(a.frill, b.frill, pa.frill, pb.frill, kFrillCount, rng);
    c.wings = inheritPart(a.wings, b.wings, pa.wings, pb.wings, kWingsCount, rng);
    c.tailTip = inheritPart(a.tailTip, b.tailTip, pa.tailTip, pb.tailTip, kTailCount, rng);
    c.pattern = inheritPart(a.pattern, b.pattern, pa.patterns[0], pb.patterns[0], kPatternCount, rng);

    // Base colour belongs to element A's range, accent to element B's, so a hybrid
    // always reads as its two elements (Steam = orange body / teal accents, or reverse).
    c.baseH = clampHue(inheritHue(a.baseH, b.baseH, rng), pa.baseH, pa.baseRange);
    c.baseS = inheritValue(a.baseS, b.baseS, rng);
    c.baseV = inheritValue(a.baseV, b.baseV, rng);
    c.accentH = clampHue(inheritHue(a.accentH, b.accentH, rng), pb.accentH, pb.accentRange);
    c.accentV = inheritValue(a.accentV, b.accentV, rng);
    c.patternH = inheritHue(a.patternH, b.patternH, rng);

    c.size = clampByte((a.size + b.size) / 2 + rng.range(-38, 38));
    c.rareFlags = inheritRare(a.rareFlags, b.rareFlags, rng);
    return c;
}

Rgb heartglowColor(Element e) {
    static constexpr Rgb kGlow[kElementCount] = {
        {255, 140, 40},   // Ember
        {80, 230, 210},   // Tide
        {150, 230, 255},  // Gale
        {150, 240, 70},   // Grove
        {190, 150, 255},  // Frost
        {255, 220, 110},  // Lumen
    };
    return kGlow[static_cast<int>(e) % kElementCount];
}

float sizeScale(const Genome& g) { return 0.90f + 0.20f * (static_cast<float>(g.size) / 255.0f); }

int rareCount(u8 flags) {
    int n = 0;
    for (; flags; flags &= static_cast<u8>(flags - 1)) ++n;
    return n;
}

}  // namespace ec
