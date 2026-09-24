// Model + skeleton tests. The key one: the C++ rig reproduces Blender's deformation of the
// exported dragon (tests/data/dragon_reference.ecr, written by tools/blender/export_dragon.py).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "check.hpp"
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

ModelData& model() {
    static ModelData m;
    static bool loaded = false;
    if (!loaded) {
        const std::vector<u8> bytes = readFile("../romfs/models/dragon.ecm");
        loaded = !bytes.empty() && loadModel(bytes.data(), bytes.size(), m);
    }
    return m;
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
    const ModelData& m = model();
    CHECK(m.skel.count == 34);
    CHECK(!m.meshes.empty());
    int wingBones = 0;
    for (int i = 0; i < m.skel.count; ++i) wingBones += m.skel.flags[i] & 1;
    CHECK(wingBones == 10);
    for (int i = 0; i < 24; ++i) CHECK((m.skel.flags[i] & 1) == 0);  // body bones first: one draw
    const MeshData* body = m.findMesh(kMeshBody, kGroupBody, 0);
    CHECK(body && body->paletteCount == 24 && body->keyCount == 1);
    CHECK(m.findMesh(kMeshWings, kGroupWings, 0) && m.findMesh(kMeshWings, kGroupWings, 1) &&
          m.findMesh(kMeshWings, kGroupWings, 2));
    CHECK(m.findMesh(kMeshPart, kGroupHorns, 1, kSexMale) != m.findMesh(kMeshPart, kGroupHorns, 1, kSexFemale));
    for (const MeshData& mesh : m.meshes) {
        CHECK(mesh.paletteCount <= kMaxPalette);
        CHECK(!mesh.indices.empty() && mesh.indices.size() % 3 == 0);
        for (int v = 0; v < mesh.vertexCount; ++v) CHECK(mesh.skin[v * 4 + 2] + mesh.skin[v * 4 + 3] == 255);
    }
    // Rest matrices are pure rotation + translation.
    for (int i = 0; i < m.skel.count; ++i) {
        const Mat34 id = mul(m.skel.rest[i], m.skel.invRest[i]);
        CHECK(std::fabs(id.m[0][0] - 1) < 1e-4f && std::fabs(id.m[1][1] - 1) < 1e-4f && std::fabs(id.m[0][3]) < 1e-4f);
    }
}

TEST(euler_matches_blender_convention) {
    // Blender 5.2: Euler((0.3, -0.2, 0.9), 'XYZ').to_quaternion() == (w 0.8794, x 0.17683, y -0.02421, z 0.44137)
    const Quat q = quatFromEulerXYZ(0.3f, -0.2f, 0.9f);
    CHECK(std::fabs(q.w - 0.87940f) < 1e-4f && std::fabs(q.x - 0.17683f) < 1e-4f);
    CHECK(std::fabs(q.y + 0.02421f) < 1e-4f && std::fabs(q.z - 0.44137f) < 1e-4f);
}

TEST(rig_matches_blender_deformation) {
    const ModelData& m = model();
    const std::vector<u8> ref = readFile("data/dragon_reference.ecr");
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
        std::printf("  case %d (t=%.2f build=%d): %u samples, max error body %.5f wings %.5f parts %.5f\n", c, t,
                    build, samples, maxErr[0], maxErr[1], maxErr[2]);
        // The dragon is ~6 units long; 0.01 allows for 8-bit weight quantization.
        CHECK(maxErr[0] < 0.01f && maxErr[1] < 0.01f && maxErr[2] < 0.01f);
    }
}

TEST(part_keys_blend_between_stages) {
    const ModelData& m = model();
    const MeshData* horns = m.findMesh(kMeshPart, kGroupHorns, 1, kSexFemale);
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

}  // namespace

void runModelTests() {
    RUN(model_loads_and_is_well_formed);
    RUN(euler_matches_blender_convention);
    RUN(rig_matches_blender_deformation);
    RUN(part_keys_blend_between_stages);
}
