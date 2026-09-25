// Dragons, version 2 (D77-D78): every kind's exported files work the way the game uses them.
// For each kind in the generated table (core/kinds): its two forms and their lower detail
// load, the skeleton is sound and matches Blender's deformation (tests/data/kinds/<kind>_<form>
// _reference.ecr, written by tools/blender/dragonkit/export.py), its parts merge into one draw
// for the common and the rare variant and both pupils, the triangle budget holds, and its
// plan's clips (romfs/anims/<plan>.eca) cover every clip the game plays.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/anim.hpp"
#include "core/den_actor.hpp"
#include "core/kinds.hpp"
#include "core/rig.hpp"

using namespace ec;

namespace {

std::vector<u8> readAll(const std::string& path) {
    std::vector<u8> data;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return data;
    std::fseek(f, 0, SEEK_END);
    data.resize(static_cast<std::size_t>(std::ftell(f)));
    std::fseek(f, 0, SEEK_SET);
    if (std::fread(data.data(), 1, data.size(), f) != data.size()) data.clear();
    std::fclose(f);
    return data;
}

bool loadKindModel(int kind, const char* form, bool lod1, ModelData& out) {
    const std::string path = std::string("../romfs/dragons/") + kindInfo(kind).name + "/" + form + (lod1 ? "_lod1" : "") + ".ecm";
    const std::vector<u8> bytes = readAll(path);
    return !bytes.empty() && loadModel(bytes.data(), bytes.size(), out);
}

struct Reader {
    const std::vector<u8>& d;
    std::size_t at = 0;
    template <typename T>
    T get() {
        T v{};
        if (at + sizeof(T) <= d.size()) std::memcpy(&v, d.data() + at, sizeof(T));
        at += sizeof(T);
        return v;
    }
};

// The classic dragon's parity check (tests/test_model.cpp) for a kind's form.
void checkParity(const ModelData& m, const std::string& refPath, const char* label) {
    const std::vector<u8> ref = readAll(refPath);
    CHECK(ref.size() > 16 && std::memcmp(ref.data(), "ECR1", 4) == 0);
    if (ref.size() < 16 || m.skel.count == 0) return;
    Reader r{ref, 4};
    const u16 cases = r.get<u16>();
    static Vec3 keyPos[4096], keyNrm[4096];
    for (int c = 0; c < cases; ++c) {
        const float t = r.get<float>();
        const u8 buildByte = r.get<u8>();
        r.at += 3;
        const u16 bones = r.get<u16>();
        CHECK(bones == m.skel.count);
        const int build = buildByte == 255 ? kBuildNeutral : buildByte;
        BonePose pose[kMaxBones];
        Vec3 expectScale[kMaxBones];
        boneScales(m, t, build, expectScale);
        BonePose idle[kMaxBones];
        idlePose(m, t, build, idle);
        float scaleErr = 0, idleErr = 0;
        for (int i = 0; i < bones; ++i) {
            pose[i].rot.x = r.get<float>();
            pose[i].rot.y = r.get<float>();
            pose[i].rot.z = r.get<float>();
            pose[i].rot.w = r.get<float>();
            pose[i].scale.x = r.get<float>();
            pose[i].scale.y = r.get<float>();
            pose[i].scale.z = r.get<float>();
            if (i < m.skel.count) scaleErr = std::fmax(scaleErr, length(pose[i].scale - expectScale[i]));
            if (c == 0 && i < m.skel.count) {
                const Quat a = pose[i].rot, b = idle[i].rot;
                idleErr = std::fmax(idleErr, 1.0f - std::fabs(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w));
            }
        }
        CHECK(scaleErr < 1e-4f);
        if (c == 0) CHECK(idleErr < 1e-5f);
        Mat34 poseMat[kMaxBones], skin[kMaxBones];
        evaluatePose(m.skel, pose, poseMat, skin);
        const u32 samples = r.get<u32>();
        float maxErr[3] = {0, 0, 0};
        int lastMesh = -1;
        for (u32 s = 0; s < samples; ++s) {
            const u16 mi = r.get<u16>(), vi = r.get<u16>();
            Vec3 want;
            want.x = r.get<float>();
            want.y = r.get<float>();
            want.z = r.get<float>();
            if (mi >= m.meshes.size()) {
                CHECK(false);
                continue;
            }
            const MeshData& mesh = m.meshes[mi];
            if (mi != lastMesh) {
                blendKeys(mesh, t, keyPos, keyNrm);
                applyBuildShift(mesh, t, build, keyPos);
                lastMesh = mi;
            }
            const Vec3 got = skinPoint(mesh, vi, keyPos[vi], skin);
            maxErr[mesh.kind] = std::fmax(maxErr[mesh.kind], length(got - want));
        }
        std::printf("  %s case %d: %u samples, max error body %.5f wings %.5f parts %.5f\n", label, c, samples,
                    maxErr[0], maxErr[1], maxErr[2]);
        CHECK(maxErr[0] < 0.01f && maxErr[1] < 0.01f && maxErr[2] < 0.01f);
    }
}

int drawnTriangles(const ModelData& m, bool rare, bool rareReplaces) {
    const MeshData* body = m.findMesh(kMeshBody, kGroupBody, 0);
    const MeshData* wings = kindWings(m, rare);
    int tris = (body ? int(body->indices.size() / 3) : 0) + (wings ? int(wings->indices.size() / 3) : 0);
    const MeshData* parts[16];
    const int n = selectKindParts(m, rare, rareReplaces, false, parts);
    for (int i = 0; i < n; ++i) tris += int(parts[i]->indices.size() / 3);
    return tris;
}

TEST(the_kinds_table_is_sound) {
    CHECK(kindCount() >= 1);
    CHECK(elementCount() == 8 && mannerCount() == 10 && traitCount() >= 30);
    for (int k = 0; k < kindCount(); ++k) {
        const KindInfo& ki = kindInfo(k);
        CHECK(findKind(ki.name) == k);
        CHECK(ki.plan < planCount());
        CHECK(ki.elementCount >= 1 && ki.elementCount <= 2);
        for (int e = 0; e < ki.elementCount; ++e) CHECK(ki.elements[e] < elementCount());
        CHECK(ki.size >= 0.6f && ki.size <= 1.55f);  // +-50% from the smallest breed to the largest (D77)
        CHECK(ki.rareVariant == kKindVariants - 1);
        for (int s = 0; s < kKindStats; ++s) CHECK(ki.stats[s] >= 1 && ki.stats[s] <= 10);
        for (int j = 0; j < ki.mannerCount; ++j) CHECK(ki.manners[j] < mannerCount());
        for (int j = 0; j < ki.traitCount; ++j) CHECK(ki.traits[j] < traitCount());
        for (int j = 0; j < k; ++j) CHECK(kindInfo(j).dex != ki.dex);
        for (int p = 0; p < 2; ++p) CHECK(ki.parents[p] < kindCount());
    }
}

TEST(kind_colours_shift_a_little_per_dragon) {
    Rgb exact[kPalCount], shifted[kPalCount];
    for (int k = 0; k < kindCount(); ++k)
        for (int v = 0; v < kKindVariants; ++v) {
            kindPalette(k, v, 0, exact);
            for (int i = 0; i < kPalCount; ++i)
                CHECK(exact[i].r == kindInfo(k).variants[v].pal[i].r && exact[i].b == kindInfo(k).variants[v].pal[i].b);
            for (u32 seed : {1u, 77u, 123456u}) {
                kindPalette(k, v, seed, shifted);
                const Rgb a = exact[kPalBase], b = shifted[kPalBase];
                const int diff = std::abs(a.r - b.r) + std::abs(a.g - b.g) + std::abs(a.b - b.b);
                CHECK(diff <= 90);                        // still the variant's colour
                CHECK(shifted[kPalGlow].r == exact[kPalGlow].r);  // the heartglow is the element's
            }
        }
}

TEST(every_kind_loads_and_fits) {
    for (int k = 0; k < kindCount(); ++k) {
        const KindInfo& ki = kindInfo(k);
        const PlanInfo& plan = planInfo(ki.plan);
        for (const char* form : {"hatchling", "grown"}) {
            for (bool lod1 : {false, true}) {
                static ModelData m;
                m = ModelData{};
                const bool ok = loadKindModel(k, form, lod1, m);
                std::printf("  %s %s%s: %s", ki.name, form, lod1 ? " LOD1" : "", ok ? "" : "MISSING\n");
                CHECK(ok);
                if (!ok) continue;
                CHECK(m.skel.count <= kMaxBones);
                for (const char* bone : {"head", "snout", "jaw", "eyes", "chest"}) CHECK(m.skel.find(bone) >= 0);
                for (const char* bone : plan.contacts) CHECK(m.skel.find(bone) >= 0);
                const MeshData* body = m.findMesh(kMeshBody, kGroupBody, 0);
                CHECK(body && body->paletteCount <= kMaxPalette);
                CHECK(kindWings(m, false) != nullptr);
                CHECK(m.findMesh(kMeshPart, kGroupEyes, 0) && m.findMesh(kMeshPart, kGroupEyes, 1));
                CHECK(m.findMesh(kMeshPart, kGroupHeart, 0) && m.findMesh(kMeshPart, kGroupMouth, 0));
                for (const MeshData& mesh : m.meshes) {
                    CHECK(mesh.paletteCount <= kMaxPalette);
                    for (int v = 0; v < mesh.vertexCount; ++v) CHECK(mesh.skin[v * 4 + 2] + mesh.skin[v * 4 + 3] == 255);
                }
                static PartsMesh parts;
                for (bool rare : {false, true})
                    for (bool slit : {false, true}) {
                        const bool merged = buildKindParts(m, rare, ki.rareReplaces, slit, 0.5f, kBuildNeutral, parts);
                        CHECK(merged && parts.paletteCount <= kMaxPalette);
                    }
                const int common = drawnTriangles(m, false, ki.rareReplaces), rare = drawnTriangles(m, true, ki.rareReplaces);
                std::printf("%d / %d triangles (common / rare)\n", common, rare);
                CHECK(common <= (lod1 ? 1200 : 3000) && rare <= (lod1 ? 1200 : 3000));
                if (!lod1) checkParity(m, std::string("data/kinds/") + ki.name + "_" + form + "_reference.ecr",
                                       (std::string(ki.name) + " " + form).c_str());
            }
        }
    }
}

TEST(every_plan_has_every_clip) {
    for (int p = 0; p < planCount(); ++p) {
        const std::vector<u8> bytes = readAll(std::string("../romfs/anims/") + planInfo(p).name + ".eca");
        static AnimLibrary lib;
        lib = AnimLibrary{};
        CHECK(!bytes.empty() && loadAnims(bytes.data(), bytes.size(), lib));
        int clips[static_cast<int>(ClipId::Count)];
        CHECK(resolveClips(lib, kFormHatchling, clips) && resolveClips(lib, kFormGrown, clips));
        // Bound to every kind on this plan: each clip's bones are the skeleton's.
        for (int k = 0; k < kindCount(); ++k) {
            if (kindInfo(k).plan != p) continue;
            static ModelData m;
            m = ModelData{};
            if (!loadKindModel(k, "grown", false, m)) continue;
            AnimBinding bind;
            bindAnims(lib, m.skel, bind);
            int bound = 0;
            for (int i = 0; i < m.skel.count; ++i) bound += bind.libBone[i] >= 0;
            CHECK(bound == m.skel.count);
        }
        std::printf("  plan %s: %d clips\n", planInfo(p).name, static_cast<int>(lib.clips.size()));
    }
}

}  // namespace

void runKindTests() {
    RUN(the_kinds_table_is_sound);
    RUN(kind_colours_shift_a_little_per_dragon);
    RUN(every_kind_loads_and_fits);
    RUN(every_plan_has_every_clip);
}
