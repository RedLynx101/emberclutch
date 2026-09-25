#include "core/kinds.hpp"

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

}  // namespace ec
