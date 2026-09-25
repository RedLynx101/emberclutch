#include "core/dragon_mesh.hpp"

#include <cmath>
#include <initializer_list>

#include "core/rig.hpp"

namespace ec {
namespace {

// First mesh that exists among the candidate variants, or nullptr.
const MeshData* firstOf(const ModelData& m, u8 group, u8 sex, std::initializer_list<u8> variants) {
    for (u8 v : variants)
        if (const MeshData* mesh = m.findMesh(kMeshPart, group, v, sex)) return mesh;
    return nullptr;
}

Rgb mix(Rgb a, Rgb b, float f) {
    return {static_cast<u8>(a.r + (b.r - a.r) * f), static_cast<u8>(a.g + (b.g - a.g) * f),
            static_cast<u8>(a.b + (b.b - a.b) * f)};
}

Rgb scaled(Rgb c, float f) {
    return {static_cast<u8>(c.r * f), static_cast<u8>(c.g * f), static_cast<u8>(c.b * f)};
}

}  // namespace

void PartsMesh::clear() {
    paletteCount = 0;
    pos.clear();
    nrm.clear();
    skin.clear();
    paint.clear();
    uv.clear();
    region.clear();
    indices.clear();
}

int selectParts(const ModelData& m, const Genome& g, Sex sex, const MeshData* out[8]) {
    const u8 s = partSex(sex);
    int n = 0;
    auto add = [&](const MeshData* mesh) {
        if (mesh && n < 8) out[n++] = mesh;
    };
    add(firstOf(m, kGroupEyes, s, {0}));
    add(firstOf(m, kGroupHeart, s, {0}));
    add(firstOf(m, kGroupMouth, s, {0}));
    add(firstOf(m, kGroupHorns, s, {g.horns, kHornsSwept}));
    if (g.frill != kFrillNone) add(firstOf(m, kGroupFrill, s, {g.frill}));
    add(firstOf(m, kGroupSpikes, s, {g.frill, kFrillNone}));  // the ridge follows the frill gene
    if (g.tailTip != kTailPlain) add(firstOf(m, kGroupTailTip, s, {g.tailTip}));
    if (g.pattern == kPatternRunes) add(firstOf(m, kGroupRunes, s, {0}));
    return n;
}

const MeshData* selectWings(const ModelData& m, const Genome& g) {
    if (const MeshData* w = m.findMesh(kMeshWings, kGroupWings, g.wings)) return w;
    return m.findMesh(kMeshWings, kGroupWings, kWingsClassic);
}

bool buildParts(const ModelData& m, const Genome& g, Sex sex, float t, PartsMesh& out) {
    out.clear();
    const MeshData* meshes[8];
    const int count = selectParts(m, g, sex, meshes);
    static Vec3 keyPos[4096], keyNrm[4096];
    for (int i = 0; i < count; ++i) {
        const MeshData& mesh = *meshes[i];
        if (mesh.vertexCount > 4096) return false;
        // Map this mesh's palette into the merged palette.
        u8 remap[kMaxPalette];
        for (int b = 0; b < mesh.paletteCount; ++b) {
            int slot = 0;
            while (slot < out.paletteCount && out.palette[slot] != mesh.palette[b]) ++slot;
            if (slot == out.paletteCount) {
                if (out.paletteCount == kMaxPalette) return false;
                out.palette[out.paletteCount++] = mesh.palette[b];
            }
            remap[b] = static_cast<u8>(slot);
        }
        const std::size_t base = out.pos.size();
        if (base + mesh.vertexCount > 65535) return false;
        blendKeys(mesh, t, keyPos, keyNrm);
        applyBuildShift(mesh, t, g.build < kModelBuilds ? g.build : kBuildNeutral, keyPos);  // seated for its build
        for (int v = 0; v < mesh.vertexCount; ++v) {
            out.pos.push_back(keyPos[v]);
            out.nrm.push_back(keyNrm[v]);
            const u8* sk = &mesh.skin[std::size_t(v) * 4];
            out.skin.insert(out.skin.end(), {remap[sk[0]], remap[sk[1]], sk[2], sk[3]});
            const u8* pt = &mesh.paint[std::size_t(v) * 4];
            out.paint.insert(out.paint.end(), pt, pt + 4);
            out.uv.insert(out.uv.end(), {mesh.uv[std::size_t(v) * 2], mesh.uv[std::size_t(v) * 2 + 1]});
            out.region.push_back(mesh.region[v]);
        }
        for (u16 ix : mesh.indices) out.indices.push_back(static_cast<u16>(base + ix));
    }
    return true;
}

float groundOffset(const ModelData& m, const Mat34* skin) {
    const MeshData* body = m.findMesh(kMeshBody, kGroupBody, 0);
    if (!body || body->vertexCount == 0) return 0.0f;
    float low = 1e9f;
    for (int v = 0; v < body->vertexCount; ++v) {
        const Vec3 p = skinPoint(*body, v, body->pos[v], skin);
        if (p.z < low) low = p.z;
    }
    return low;
}

void rarePalette(u8 rareFlags, Rgb pal[kPalCount]) {
    if (rareFlags & kRareMelanistic) {
        for (u8 slot : {kPalBase, kPalAccent, kPalPattern, kPalHorn, kPalMembrane})
            pal[slot] = mix(scaled(pal[slot], 0.18f), {20, 14, 22}, 0.3f);
        const Rgb g = pal[kPalGlow];  // the glow looks brighter against the dark
        pal[kPalGlow] = {static_cast<u8>(g.r + (255 - g.r) / 3), static_cast<u8>(g.g + (255 - g.g) / 3),
                         static_cast<u8>(g.b + (255 - g.b) / 3)};
    } else if (rareFlags & kRareLeucistic) {
        for (u8 slot : {kPalBase, kPalAccent, kPalPattern, kPalHorn, kPalMembrane})
            pal[slot] = mix(pal[slot], {250, 244, 246}, 0.62f);
        pal[kPalGlow] = mix(pal[kPalGlow], {255, 150, 190}, 0.55f);
        pal[kPalIris] = mix(pal[kPalIris], {235, 120, 150}, 0.5f);
    }
}

Genome shimmer(const Genome& g, float t) {
    Genome out = g;
    const int turn = static_cast<int>(22.0f * std::sin(t * 0.7f));  // about +-30 degrees
    out.baseH = static_cast<u8>(g.baseH + turn);
    out.accentH = static_cast<u8>(g.accentH - turn);
    out.patternH = static_cast<u8>(g.patternH + 2 * turn);
    return out;
}

void dragonPalette(const Genome& g, Rgb out[kPalCount]) {
    const Rgb base = hsvToRgb(g.baseH, g.baseS, g.baseV);
    const Rgb accent = hsvToRgb(g.accentH, static_cast<u8>(g.baseS / 2), g.accentV);  // as the 2D art
    out[kPalBase] = base;
    out[kPalAccent] = accent;
    out[kPalPattern] = hsvToRgb(g.patternH, g.baseS, static_cast<u8>(g.baseV * 3 / 4));
    out[kPalHorn] = hsvToRgb(g.accentH, static_cast<u8>(g.baseS * 17 / 20), 245);
    out[kPalMembrane] = mix(base, accent, 0.45f);
    out[kPalIris] = scaled(heartglowColor(static_cast<Element>(g.elementB)), 0.85f);
    out[kPalPupil] = {18, 10, 16};
    out[kPalGlint] = {255, 255, 255};
    out[kPalGlow] = heartglowColor(static_cast<Element>(g.elementA));
    out[kPalTongue] = {236, 116, 140};
}

}  // namespace ec
