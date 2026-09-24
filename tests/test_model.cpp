// Model + skeleton tests. The key one: the C++ rig reproduces Blender's deformation of both
// exported dragon forms (tests/data/<form>_reference.ecr, written by tools/blender/export_dragon.py).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/dragon_mesh.hpp"
#include "core/model.hpp"
#include "core/rig.hpp"

using namespace ec;

namespace {

std::vector<u8> readFile(const char* path) {
    std::vector<u8> data;
    FILE* f = std::fopen(path, "rb");
    if (!f) return data;
    std::fseek(f, 0, SEEK_END);
    data.resize(static_cast<std::size_t>(std::ftell(f)));
    std::fseek(f, 0, SEEK_SET);
    if (std::fread(data.data(), 1, data.size(), f) != data.size()) data.clear();
    std::fclose(f);
    return data;
}

const char* const kFormNames[kFormCount] = {"hatchling", "grown"};

ModelData& model(int form) {
    static ModelData m[kFormCount];
    static bool loaded[kFormCount] = {};
    if (!loaded[form]) {
        const std::vector<u8> bytes = readFile((std::string("../romfs/models/") + kFormNames[form] + ".ecm").c_str());
        loaded[form] = !bytes.empty() && loadModel(bytes.data(), bytes.size(), m[form]);
    }
    return m[form];
}

struct RefReader {
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

TEST(model_loads_and_is_well_formed) {
    for (int form = 0; form < kFormCount; ++form) {
        const ModelData& m = model(form);
        CHECK(m.skel.count == 36);
        CHECK(!m.meshes.empty());
        int wingBones = 0;
        for (int i = 0; i < m.skel.count; ++i) wingBones += m.skel.flags[i] & 1;
        CHECK(wingBones == 12);
        for (int i = 0; i < 24; ++i) CHECK((m.skel.flags[i] & 1) == 0);  // body bones first: one draw
        const MeshData* body = m.findMesh(kMeshBody, kGroupBody, 0);
        CHECK(body && body->paletteCount == 24 && body->keyCount == 1);
        for (u8 w = 0; w < kWingsCount; ++w) CHECK(m.findMesh(kMeshWings, kGroupWings, w) != nullptr);
        CHECK(m.findMesh(kMeshPart, kGroupHorns, kHornsSwept, kSexMale) !=
              m.findMesh(kMeshPart, kGroupHorns, kHornsSwept, kSexFemale));
        // The dorsal ridge follows the frill gene (leaf falls back to spikes at runtime).
        for (u8 f : {kFrillNone, kFrillFin, kFrillFeather}) CHECK(m.findMesh(kMeshPart, kGroupSpikes, f) != nullptr);
        for (const MeshData& mesh : m.meshes) {
            CHECK(mesh.paletteCount <= kMaxPalette);
            CHECK(!mesh.indices.empty() && mesh.indices.size() % 3 == 0);
            for (int v = 0; v < mesh.vertexCount; ++v) CHECK(mesh.skin[v * 4 + 2] + mesh.skin[v * 4 + 3] == 255);
        }
        // Rest matrices are pure rotation + translation.
        for (int i = 0; i < m.skel.count; ++i) {
            const Mat34 id = mul(m.skel.rest[i], m.skel.invRest[i]);
            CHECK(std::fabs(id.m[0][0] - 1) < 1e-4f && std::fabs(id.m[1][1] - 1) < 1e-4f &&
                  std::fabs(id.m[0][3]) < 1e-4f);
        }
    }
}

TEST(dragon_fits_triangle_budget) {
    // Architecture section 1: LOD0 <= 3,000 triangles, even for the heaviest gene combination.
    for (int form = 0; form < kFormCount; ++form) {
        const ModelData& m = model(form);
        int worst[8] = {};  // heaviest variant per part group; slot 7 = body
        for (const MeshData& mesh : m.meshes) {
            const int g = mesh.group == kGroupBody ? 7 : mesh.group;
            const int tris = static_cast<int>(mesh.indices.size() / 3);
            if (g < 8 && tris > worst[g]) worst[g] = tris;
        }
        int total = 0;
        for (int w : worst) total += w;
        std::printf("  %s: worst case %d triangles\n", kFormNames[form], total);
        CHECK(total <= 3000);
    }
}

TEST(growth_maps_stages_to_forms) {
    const Growth hatch = growthFor(Stage::Hatchling, 0.5f);
    CHECK(hatch.form == kFormHatchling && std::fabs(hatch.t - 0.5f) < 1e-6f);
    const Growth juv0 = growthFor(Stage::Juvenile, 0.0f);
    CHECK(juv0.form == kFormGrown && juv0.t == 0.0f);  // the molt: a new juvenile starts the grown form
    const Growth juv1 = growthFor(Stage::Juvenile, 1.0f), ado0 = growthFor(Stage::Adolescent, 0.0f);
    CHECK(std::fabs(juv1.t - ado0.t) < 1e-6f);  // continuous across juvenile -> adolescent
    CHECK(growthFor(Stage::Adult, 0.3f).t == 1.0f);
    CHECK(growthFor(Stage::Adolescent, 2.0f).t <= 1.0f);
}

TEST(euler_matches_blender_convention) {
    // Blender 5.2: Euler((0.3, -0.2, 0.9), 'XYZ').to_quaternion() == (w 0.8794, x 0.17683, y -0.02421, z 0.44137)
    const Quat q = quatFromEulerXYZ(0.3f, -0.2f, 0.9f);
    CHECK(std::fabs(q.w - 0.87940f) < 1e-4f && std::fabs(q.x - 0.17683f) < 1e-4f);
    CHECK(std::fabs(q.y + 0.02421f) < 1e-4f && std::fabs(q.z - 0.44137f) < 1e-4f);
}

void checkRigParity(int form) {
    const ModelData& m = model(form);
    const std::vector<u8> ref = readFile((std::string("data/") + kFormNames[form] + "_reference.ecr").c_str());
    CHECK(ref.size() > 16 && std::memcmp(ref.data(), "ECR1", 4) == 0);
    if (ref.size() < 16 || m.skel.count == 0) return;
    RefReader r{ref, 4};
    const u16 cases = r.get<u16>();
    CHECK(cases == 3);
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
            scaleErr = std::fmax(scaleErr, length(pose[i].scale - expectScale[i]));
            // Case 0 is the unmodified idle pose: our Euler tables must give the same rotations.
            if (c == 0) {
                const Quat a = pose[i].rot, b = idle[i].rot;
                const float d = std::fabs(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
                idleErr = std::fmax(idleErr, 1.0f - d);
            }
        }
        CHECK(scaleErr < 1e-4f);  // growth + build tables reproduce Blender's bone scales
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
                lastMesh = mi;
            }
            const Vec3 got = skinPoint(mesh, vi, keyPos[vi], skin);
            maxErr[mesh.kind] = std::fmax(maxErr[mesh.kind], length(got - want));
        }
        std::printf("  %s case %d (t=%.2f build=%d): %u samples, max error body %.5f wings %.5f parts %.5f\n",
                    kFormNames[form], c, t, build, samples, maxErr[0], maxErr[1], maxErr[2]);
        // The adult is ~6 units long; 0.01 allows for 8-bit weight quantization.
        CHECK(maxErr[0] < 0.01f && maxErr[1] < 0.01f && maxErr[2] < 0.01f);
    }
}

TEST(rig_matches_blender_deformation) {
    for (int form = 0; form < kFormCount; ++form) checkRigParity(form);
}

TEST(part_keys_blend_between_stages) {
    const ModelData& m = model(kFormGrown);
    const MeshData* horns = m.findMesh(kMeshPart, kGroupHorns, kHornsSwept, kSexFemale);
    CHECK(horns && horns->keyCount == 4);
    if (!horns) return;
    static Vec3 p0[512], p1[512], pm[512], n[512];
    blendKeys(*horns, 0.0f, p0, n);
    blendKeys(*horns, 0.35f, p1, n);
    blendKeys(*horns, 0.175f, pm, n);
    const Vec3 mid = lerp(p0[5], p1[5], 0.5f);
    CHECK(length(pm[5] - mid) < 1e-5f);
    CHECK(std::fabs(length(n[5]) - 1.0f) < 1e-4f);
}

TEST(parts_follow_the_genome_and_merge_into_one_draw) {
    Rng rng(11);
    for (int form = 0; form < kFormCount; ++form) {
        const ModelData& m = model(form);
        for (Element e : {Element::Ember, Element::Tide, Element::Gale}) {
            const Genome g = makePurebred(e, rng);
            for (Sex sex : {Sex::Female, Sex::Male}) {
                PartsMesh parts;
                CHECK(buildParts(m, g, sex, 0.4f, parts));
                CHECK(parts.paletteCount > 0 && parts.paletteCount <= kMaxPalette);
                CHECK(!parts.indices.empty() && parts.indices.size() % 3 == 0);
                CHECK(parts.skin.size() == parts.pos.size() * 4 && parts.paint.size() == parts.pos.size() * 4);
                bool indicesOk = true, skinOk = true;
                for (u16 ix : parts.indices) indicesOk &= ix < parts.pos.size();
                for (std::size_t v = 0; v < parts.pos.size(); ++v)
                    skinOk &= parts.skin[v * 4] < parts.paletteCount && parts.skin[v * 4 + 1] < parts.paletteCount;
                CHECK(indicesOk && skinOk);
                CHECK(selectWings(m, g) && selectWings(m, g)->variant == g.wings);
            }
        }
        // Variants not modelled yet fall back; None/Plain draw nothing.
        Genome g = makePurebred(Element::Ember, rng);
        g.horns = kHornsCrown;
        g.frill = kFrillLeaf;
        g.tailTip = kTailPlain;
        const MeshData* sel[8];
        const int n = selectParts(m, g, Sex::Male, sel);
        int horns = 0, ridge = 0, frill = 0, tail = 0;
        for (int i = 0; i < n; ++i) {
            horns += sel[i]->group == kGroupHorns && sel[i]->variant == kHornsSwept;
            ridge += sel[i]->group == kGroupSpikes && sel[i]->variant == kFrillNone;
            frill += sel[i]->group == kGroupFrill;
            tail += sel[i]->group == kGroupTailTip;
        }
        CHECK(horns == 1 && ridge == 1 && frill == 0 && tail == 0);
    }
}

TEST(palette_and_ground_offset) {
    Rng rng(5);
    Rgb pal[kPalCount];
    dragonPalette(makePurebred(Element::Ember, rng), pal);
    CHECK(pal[kPalBase].r > pal[kPalBase].g && pal[kPalBase].g > pal[kPalBase].b);  // ember orange
    dragonPalette(makePurebred(Element::Tide, rng), pal);
    CHECK(pal[kPalBase].g > pal[kPalBase].r && pal[kPalBase].b > pal[kPalBase].r);  // sea teal
    CHECK(pal[kPalGlint].r == 255 && pal[kPalPupil].r < 40);

    // The adult is modelled standing near z = 0 (the idle pose's tail curl dips a little
    // lower); the juvenile's shorter legs leave its lowest vertex higher, so the renderer
    // shifts every dragon by -groundOffset.
    const ModelData& m = model(kFormGrown);
    BonePose pose[kMaxBones];
    Mat34 poseMat[kMaxBones], skin[kMaxBones];
    idlePose(m, 1.0f, kBuildNeutral, pose);
    evaluatePose(m.skel, pose, poseMat, skin);
    const float low = groundOffset(m, skin);
    CHECK(low > -0.5f && low < 0.2f);
    idlePose(m, 0.0f, kBuildNeutral, pose);
    evaluatePose(m.skel, pose, poseMat, skin);
    CHECK(groundOffset(m, skin) > low);  // shorter juvenile legs leave it floating until lifted
}

}  // namespace

void runModelTests() {
    RUN(model_loads_and_is_well_formed);
    RUN(dragon_fits_triangle_budget);
    RUN(growth_maps_stages_to_forms);
    RUN(euler_matches_blender_convention);
    RUN(rig_matches_blender_deformation);
    RUN(part_keys_blend_between_stages);
    RUN(parts_follow_the_genome_and_merge_into_one_draw);
    RUN(palette_and_ground_offset);
}
