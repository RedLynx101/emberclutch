#include "core/rig.hpp"

namespace ec {
namespace {

constexpr float kDegToRad = 3.14159265358979f / 180.0f;

float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

}  // namespace

Growth growthFor(Stage stage, float progress) {
    progress = clamp01(progress);
    switch (stage) {
        case Stage::Egg: return {kFormHatchling, 0.0f};
        case Stage::Hatchling: return {kFormHatchling, progress};
        case Stage::Juvenile: return {kFormGrown, kAdolescentT * progress};
        case Stage::Adolescent: return {kFormGrown, kAdolescentT + (1.0f - kAdolescentT) * progress};
        default: return {kFormGrown, 1.0f};
    }
}

float growthScale(const Growth& g) {
    const float t = clamp01(g.t);
    return g.form == kFormHatchling ? 0.4f * (0.62f + 0.38f * t) : 0.5f + 0.5f * t;
}

void boneScales(const ModelData& m, float t, int build, Vec3* out) {
    t = clamp01(t);
    for (int i = 0; i < m.skel.count; ++i) {
        const Vec3 h = m.hatchScale[i];
        Vec3 s = lerp(h, Vec3{1, 1, 1}, t);
        if (build >= 0 && build < kModelBuilds) {
            const float g = m.build[build][i][0], l = m.build[build][i][1];
            s = {s.x * g, s.y * l, s.z * g};
        }
        out[i] = s;
    }
}

void idlePose(const ModelData& m, float t, int build, BonePose* out) {
    Vec3 scales[kMaxBones];
    boneScales(m, t, build, scales);
    t = clamp01(t);
    for (int i = 0; i < m.skel.count; ++i) {
        const Vec3 e = m.poseEulerDeg[i];
        const float x = e.x + m.hatchPoseXDeg[i] * (1.0f - t);
        out[i].rot = quatFromEulerXYZ(x * kDegToRad, e.y * kDegToRad, e.z * kDegToRad);
        out[i].scale = scales[i];
    }
}

void applyBuildShift(const MeshData& mesh, float t, int build, Vec3* pos) {
    if (build < 0 || build >= kModelBuilds || mesh.buildShift.empty()) return;
    int k = 0;
    float f = 0.0f;
    if (mesh.keyCount > 1) {  // the keys and weight blendKeys uses
        t = clamp01(t);
        while (k < mesh.keyCount - 2 && t > mesh.keyT[k + 1]) ++k;
        const float span = mesh.keyT[k + 1] - mesh.keyT[k];
        f = span > 1e-6f ? clamp01((t - mesh.keyT[k]) / span) : 0.0f;
    }
    const int k1 = mesh.keyCount > 1 ? k + 1 : k, pieces = mesh.pieceCount;
    const Vec3* a = &mesh.buildShift[(std::size_t(k) * kModelBuilds + build) * pieces];
    const Vec3* b = &mesh.buildShift[(std::size_t(k1) * kModelBuilds + build) * pieces];
    for (int v = 0; v < mesh.vertexCount; ++v) {
        const int p = mesh.piece[v];
        pos[v] = pos[v] + lerp(a[p], b[p], f);
    }
}

void blendKeys(const MeshData& mesh, float t, Vec3* pos, Vec3* nrm) {
    const int n = mesh.vertexCount;
    int k = 0;
    float f = 0.0f;
    if (mesh.keyCount > 1) {
        t = clamp01(t);
        while (k < mesh.keyCount - 2 && t > mesh.keyT[k + 1]) ++k;
        const float span = mesh.keyT[k + 1] - mesh.keyT[k];
        f = span > 1e-6f ? clamp01((t - mesh.keyT[k]) / span) : 0.0f;
    }
    const Vec3* a = &mesh.pos[std::size_t(k) * n];
    const Vec3* an = &mesh.nrm[std::size_t(k) * n];
    if (mesh.keyCount == 1 || f <= 0.0f) {
        for (int v = 0; v < n; ++v) {
            pos[v] = a[v];
            nrm[v] = an[v];
        }
        return;
    }
    const Vec3* b = &mesh.pos[std::size_t(k + 1) * n];
    const Vec3* bn = &mesh.nrm[std::size_t(k + 1) * n];
    for (int v = 0; v < n; ++v) {
        pos[v] = lerp(a[v], b[v], f);
        nrm[v] = normalize(lerp(an[v], bn[v], f));
    }
}

Vec3 skinPoint(const MeshData& mesh, int vertex, Vec3 p, const Mat34* skin) {
    const u8* s = &mesh.skin[std::size_t(vertex) * 4];
    const float w0 = s[2] / 255.0f, w1 = s[3] / 255.0f;
    const Vec3 a = transformPoint(skin[mesh.palette[s[0]]], p);
    const Vec3 b = transformPoint(skin[mesh.palette[s[1]]], p);
    return a * w0 + b * w1;
}

}  // namespace ec
