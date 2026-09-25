#include "core/model.hpp"

#include <cstring>

#include "core/byte_reader.hpp"

namespace ec {

const MeshData* ModelData::findMesh(u8 kind, u8 group, u8 variant, u8 sex) const {
    for (const MeshData& m : meshes)
        if (m.kind == kind && m.group == group && m.variant == variant &&
            (m.sex == kSexAny || sex == kSexAny || m.sex == sex))
            return &m;
    return nullptr;
}

bool loadModel(const u8* data, std::size_t size, ModelData& out) {
    ByteReader c(data, size);
    char magic[4];
    c.bytes(magic, 4);
    if (!c.ok() || std::memcmp(magic, "ECM1", 4) != 0) return false;
    const u16 version = c.u16v();  // 3: UVs and body regions; 4: parts seated per build
    if (version != 3 && version != 4) return false;
    const u16 boneCount = c.u16v();
    if (boneCount == 0 || boneCount > kMaxBones) return false;

    Skeleton& s = out.skel;
    s.count = boneCount;
    for (int i = 0; i < boneCount; ++i) {
        c.bytes(s.name[i], 16);
        s.name[i][15] = '\0';
        s.parent[i] = c.s8v();
        s.flags[i] = c.u8v();
        c.skip(2);
        for (int r = 0; r < 3; ++r)
            for (int k = 0; k < 4; ++k) s.rest[i].m[r][k] = c.f32();
        if (s.parent[i] >= i) return false;  // parents must come first
    }
    finalizeSkeleton(s);

    for (int i = 0; i < boneCount; ++i) out.hatchScale[i] = c.vec3();
    for (int b = 0; b < kModelBuilds; ++b)
        for (int i = 0; i < boneCount; ++i) {
            out.build[b][i][0] = c.f32();
            out.build[b][i][1] = c.f32();
        }
    for (int i = 0; i < boneCount; ++i) out.poseEulerDeg[i] = c.vec3();
    for (int i = 0; i < boneCount; ++i) out.hatchPoseXDeg[i] = c.f32();

    const u16 meshCount = c.u16v();
    out.meshes.clear();
    out.meshes.resize(meshCount);
    for (MeshData& m : out.meshes) {
        c.bytes(m.name, 16);
        m.name[15] = '\0';
        m.kind = c.u8v();
        m.group = c.u8v();
        m.variant = c.u8v();
        m.sex = c.u8v();
        m.paletteCount = c.u8v();
        c.bytes(m.palette, kMaxPalette);
        c.skip(kPaletteField - kMaxPalette);
        m.keyCount = c.u8v();
        c.skip(3);
        for (float& t : m.keyT) t = c.f32();
        m.vertexCount = c.u16v();
        const u16 indexCount = c.u16v();
        if (!c.ok() || m.paletteCount > kMaxPalette || m.keyCount == 0 || m.keyCount > kMaxKeys ||
            indexCount % 3 != 0)
            return false;
        for (int i = 0; i < m.paletteCount; ++i)
            if (m.palette[i] >= boneCount) return false;

        const std::size_t n = std::size_t(m.keyCount) * m.vertexCount;
        m.pos.resize(n);
        m.nrm.resize(n);
        for (int k = 0; k < m.keyCount; ++k) {
            for (int v = 0; v < m.vertexCount; ++v) m.pos[std::size_t(k) * m.vertexCount + v] = c.vec3();
            for (int v = 0; v < m.vertexCount; ++v) m.nrm[std::size_t(k) * m.vertexCount + v] = c.vec3();
        }
        m.skin.resize(std::size_t(m.vertexCount) * 4);
        c.bytes(m.skin.data(), m.skin.size());
        m.paint.resize(std::size_t(m.vertexCount) * 4);
        c.bytes(m.paint.data(), m.paint.size());
        m.uv.resize(std::size_t(m.vertexCount) * 2);
        for (float& t : m.uv) t = c.f32();
        m.region.resize(m.vertexCount);
        c.bytes(m.region.data(), m.region.size());
        m.indices.resize(indexCount);
        for (u16& ix : m.indices) ix = c.u16v();
        m.pieceCount = 0;
        m.piece.clear();
        m.buildShift.clear();
        if (version >= 4 && (m.pieceCount = c.u8v()) > 0) {
            m.piece.resize(m.vertexCount);
            c.bytes(m.piece.data(), m.piece.size());
            m.buildShift.resize(std::size_t(m.keyCount) * kModelBuilds * m.pieceCount);
            for (Vec3& s : m.buildShift) s = c.vec3();
            for (u8 p : m.piece)
                if (p >= m.pieceCount) return false;
        }
        if (!c.ok()) return false;
        for (int v = 0; v < m.vertexCount; ++v)
            if (m.skin[v * 4] >= m.paletteCount || m.skin[v * 4 + 1] >= m.paletteCount) return false;
        for (u16 ix : m.indices)
            if (ix >= m.vertexCount) return false;
        for (u8 r : m.region)
            if (r > kRegionClean) return false;
    }
    const char* const feet[4] = {"hand_L", "hand_R", "foot_L", "foot_R"};
    for (int i = 0; i < 4; ++i) out.contacts[i] = static_cast<s8>(out.skel.find(feet[i]));
    return c.ok();
}

}  // namespace ec
