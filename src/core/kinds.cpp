#include "core/kinds.hpp"

#include <cstdio>
#include <cstring>

namespace ec {
namespace {

#include "core/kinds_data.inc"

template <typename T, int N>
constexpr int count(const T (&)[N]) {
    return N;
}

static_assert(sizeof(kKinds) / sizeof(kKinds[0]) <= kMaxKinds, "raise kMaxKinds");
static_assert(sizeof(kPlans) / sizeof(kPlans[0]) <= kMaxPlans, "raise kMaxPlans");

u8 clampByte(int v) { return static_cast<u8>(v < 0 ? 0 : (v > 255 ? 255 : v)); }

// Turns a colour's hue by `turn` (in 1/256 of a circle) and scales its brightness by `bright`.
Rgb nudge(Rgb c, int turn, float bright) {
    int mx = c.r > c.g ? (c.r > c.b ? c.r : c.b) : (c.g > c.b ? c.g : c.b);
    int mn = c.r < c.g ? (c.r < c.b ? c.r : c.b) : (c.g < c.b ? c.g : c.b);
    if (mx == mn) return {clampByte(int(c.r * bright)), clampByte(int(c.g * bright)), clampByte(int(c.b * bright))};
    const int d = mx - mn;
    int h;  // 0..1535
    if (mx == c.r) h = ((c.g - c.b) * 256 / d + 1536) % 1536;
    else if (mx == c.g) h = (c.b - c.r) * 256 / d + 512;
    else h = (c.r - c.g) * 256 / d + 1024;
    h = ((h + turn * 6) % 1536 + 1536) % 1536;
    const int v = mx, s = d * 255 / mx;
    const int region = h / 256, rem = h % 256;
    const int p = v * (255 - s) / 255, q = v * (255 - s * rem / 256) / 255, t = v * (255 - s * (255 - rem) / 256) / 255;
    int r, g, b;
    switch (region) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        default: r = v, g = p, b = q; break;
    }
    return {clampByte(int(r * bright)), clampByte(int(g * bright)), clampByte(int(b * bright))};
}

const MeshData* part(const ModelData& m, u8 group, u8 variant) { return m.findMesh(kMeshPart, group, variant); }

}  // namespace

int kindCount() { return count(kKinds); }
const KindInfo& kindInfo(int kind) { return kKinds[kind >= 0 && kind < kindCount() ? kind : 0]; }
int findKind(const char* name) {
    for (int i = 0; i < kindCount(); ++i)
        if (std::strcmp(kKinds[i].name, name) == 0) return i;
    return -1;
}
int planCount() { return count(kPlans); }
const PlanInfo& planInfo(int plan) { return kPlans[plan >= 0 && plan < planCount() ? plan : 0]; }
int elementCount() { return count(kElementNames); }
const char* elementName(int e) { return e >= 0 && e < elementCount() ? kElementNames[e] : "?"; }
Rgb elementGlow(int e) { return e >= 0 && e < elementCount() ? kElementGlow[e] : Rgb{255, 255, 255}; }
int mannerCount() { return count(kMannerNames); }
const char* mannerName(int m) { return m >= 0 && m < mannerCount() ? kMannerNames[m] : "?"; }

Personality personalityOf(int m) {
    static constexpr Personality kActsLike[] = {Personality::Sleepy, Personality::Playful, Personality::Curious,
                                                Personality::Proud};
    constexpr int kOwn = static_cast<int>(Personality::Count);
    if (m >= 0 && m < kOwn) return static_cast<Personality>(m);
    return m - kOwn < static_cast<int>(sizeof(kActsLike) / sizeof(kActsLike[0])) ? kActsLike[m - kOwn]
                                                                                 : Personality::Playful;
}
int traitCount() { return count(kTraitNames); }
const char* traitName(int t) { return t >= 0 && t < traitCount() ? kTraitNames[t] : "?"; }
int traitTier(int t) { return t >= 0 && t < traitCount() ? kTraitTier[t] : 0; }

void kindPalette(int kind, int variant, u32 seed, Rgb out[kPalCount]) {
    const KindVariant& v = kindInfo(kind).variants[variant >= 0 && variant < kKindVariants ? variant : 0];
    for (int i = 0; i < kPalCount; ++i) out[i] = v.pal[i];
    if (seed == 0) return;
    // A small, stable shift: hue by up to +-4 steps of 256, brightness by up to +-6%.
    u32 x = seed * 2654435761u;
    x ^= x >> 15;
    const int turn = static_cast<int>(x % 9) - 4;
    const float bright = 0.94f + 0.12f * static_cast<float>((x >> 8) % 101) / 100.0f;
    for (u8 slot : {kPalBase, kPalAccent, kPalPattern, kPalHorn, kPalMembrane}) out[slot] = nudge(out[slot], turn, bright);
}

int selectKindParts(const ModelData& m, bool rare, bool rareReplaces, bool slitEyes, const MeshData* out[16]) {
    int n = 0;
    auto add = [&](const MeshData* mesh) {
        if (mesh && n < 16) out[n++] = mesh;
    };
    add(part(m, kGroupEyes, slitEyes ? 1 : 0) ? part(m, kGroupEyes, slitEyes ? 1 : 0) : part(m, kGroupEyes, 0));
    add(part(m, kGroupHeart, 0));
    add(part(m, kGroupMouth, 0));
    for (u8 g : {kGroupHorns, kGroupFrill, kGroupSpikes, kGroupTailTip, kGroupRunes}) {
        const MeshData* common = part(m, g, 0);
        const MeshData* special = rare ? part(m, g, 1) : nullptr;
        if (special) {
            if (!rareReplaces) add(common);
            add(special);
        } else {
            add(common);
        }
    }
    return n;
}

const MeshData* kindWings(const ModelData& m, bool rare) {
    if (rare)
        if (const MeshData* w = m.findMesh(kMeshWings, kGroupWings, 1)) return w;
    return m.findMesh(kMeshWings, kGroupWings, 0);
}

bool buildKindParts(const ModelData& m, bool rare, bool rareReplaces, bool slitEyes, float t, int build,
                    PartsMesh& out) {
    const MeshData* meshes[16];
    const int n = selectKindParts(m, rare, rareReplaces, slitEyes, meshes);
    return mergeParts(meshes, n, t, build, out);
}


// ---- DR3: every dragon a kind

namespace {

static_assert(kDragonStats == kKindStats, "a dragon's stats are its kind's");

u8 clampStat(int v) { return static_cast<u8>(v < 1 ? 1 : (v > 10 ? 10 : v)); }

bool isCrossbreed(int k) { return kindInfo(k).parents[0] >= 0; }

}  // namespace

void rollKind(Dragon& d, int kind, int variant, Rng& rng) {
    if (kind < 0 || kind >= kindCount()) kind = 0;
    const KindInfo& k = kindInfo(kind);
    const bool rare = variant == k.rareVariant;
    d.kind = static_cast<u8>(kind);
    d.variant = static_cast<u8>(variant >= 0 && variant < kKindVariants ? variant : 0);
    // A manner it leans to (the first most, four to one), now and then any.
    if (k.mannerCount > 0 && !rng.chance(1, 5)) {
        int total = 0;
        for (int i = 0; i < k.mannerCount; ++i) total += k.mannerCount - i;
        int pick = static_cast<int>(rng.below(static_cast<u32>(total)));
        int i = 0;
        while (pick >= k.mannerCount - i) pick -= k.mannerCount - i++;
        d.manner = k.manners[i];
    } else {
        d.manner = static_cast<u8>(rng.below(static_cast<u32>(mannerCount())));
    }
    // Stats: the kind's, a point either way, one more all round on the rare colouring, then the
    // manner's nudge (one up, one down).
    for (int s = 0; s < kKindStats; ++s) d.stats[s] = clampStat(k.stats[s] + rng.range(-1, 1) + (rare ? 1 : 0));
    const u8* nudge = kMannerNudge[d.manner % mannerCount()];
    d.stats[nudge[0]] = clampStat(d.stats[nudge[0]] + 1);
    d.stats[nudge[1]] = clampStat(d.stats[nudge[1]] - 1);
    // Traits from those it leans to (earlier in its list more likely): one or two, three on
    // the rare colouring. The rare colouring alone reaches the rarest tier, and weights rarer
    // traits up; a rare kind reaches one tier further than the rest.
    const int want = rare ? 3 : (rng.chance(2, 5) ? 2 : 1);
    const int reach = rare ? 3 : (k.rarity == Rarity::Rare ? 2 : 1);
    d.traitCount = 0;
    for (int n = 0; n < want && n < kDragonTraits; ++n) {
        int weights[6] = {}, total = 0;
        for (int i = 0; i < k.traitCount; ++i) {
            const int t = k.traits[i], tier = kTraitTier[t];
            bool taken = false;
            for (int j = 0; j < d.traitCount; ++j) taken |= d.traits[j] == t;
            if (taken || tier > reach) continue;
            weights[i] = (k.traitCount - i) * (rare ? 1 + tier : 1);
            total += weights[i];
        }
        if (total == 0) break;
        int pick = static_cast<int>(rng.below(static_cast<u32>(total)));
        int i = 0;
        while (pick >= weights[i]) pick -= weights[i++];
        d.traits[d.traitCount++] = k.traits[i];
    }
    d.personality = personalityOf(d.manner);  // (an egg's is set again as it hatches: the same)
}

int rollVariant(Rng& rng, bool rareParent) {
    if (rng.chance(1, rareParent ? 10 : 20)) return kKindVariants - 1;
    return static_cast<int>(rng.below(kKindVariants - 1));
}

int crossbreedOf(int a, int b) {
    for (int k = 0; k < kindCount(); ++k) {
        const KindInfo& ki = kindInfo(k);
        if ((ki.parents[0] == a && ki.parents[1] == b) || (ki.parents[0] == b && ki.parents[1] == a)) return k;
    }
    return -1;
}

int childKind(int a, int b, Rng& rng) {
    if (a == b) return a;
    const int cross = crossbreedOf(a, b);
    if (cross >= 0 && rng.chance(1, 3)) return cross;
    return rng.chance(1, 2) ? a : b;
}

int randomKind(Rng& rng, int common, int uncommon, int rare) {
    int total = 0;
    const int weight[3] = {common, uncommon, rare};
    for (int k = 0; k < kindCount(); ++k)
        if (!isCrossbreed(k)) total += weight[static_cast<int>(kindInfo(k).rarity)];
    if (total <= 0) return 0;
    int pick = static_cast<int>(rng.below(static_cast<u32>(total)));
    for (int k = 0; k < kindCount(); ++k) {
        if (isCrossbreed(k)) continue;
        pick -= weight[static_cast<int>(kindInfo(k).rarity)];
        if (pick < 0) return k;
    }
    return 0;
}

void migrateToKind(Dragon& d) {
    Rng rng(static_cast<std::uint64_t>(d.id) * 0x9E3779B97F4A7C15ull + 0xD3A11u);
    const int kind = static_cast<int>(rng.below(static_cast<u32>(kindCount())));  // any, crossbreeds too (Noah: "randomize")
    rollKind(d, kind, static_cast<int>(rng.below(kKindVariants - 1)), rng);
}

const char* kindTitle(const Dragon& d) { return kindInfo(d.kind < kindCount() ? d.kind : 0).title; }

Rgb kindGlow(const Dragon& d) { return elementGlow(kindInfo(d.kind < kindCount() ? d.kind : 0).elements[0]); }

Rgb kindShell(const Dragon& d) {
    return kindInfo(d.kind < kindCount() ? d.kind : 0).variants[d.variant % kKindVariants].egg[0];
}

float kindSize(const Dragon& d) {
    return kindInfo(d.kind < kindCount() ? d.kind : 0).size * (0.94f + 0.12f * (static_cast<float>(d.genome.size) / 255.0f));
}

void kindElements(int kind, char* out, int cap) {
    const KindInfo& k = kindInfo(kind < kindCount() ? kind : 0);
    if (k.elementCount > 1)
        std::snprintf(out, static_cast<std::size_t>(cap), "%s / %s", elementName(k.elements[0]), elementName(k.elements[1]));
    else
        std::snprintf(out, static_cast<std::size_t>(cap), "%s", elementName(k.elements[0]));
}

const char* rarityName(Rarity r) {
    return r == Rarity::Rare ? "Rare" : (r == Rarity::Uncommon ? "Harder to find" : "Common");
}

}  // namespace ec
