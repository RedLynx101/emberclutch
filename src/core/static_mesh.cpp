#include "core/static_mesh.hpp"

#include <cmath>
#include <cstring>

#include "core/byte_reader.hpp"

namespace ec {
namespace {

constexpr int kMaxSets = 4;
constexpr int kMaxParts = 64;

}  // namespace

const StaticPart* StaticScene::find(const char* name) const {
    for (const StaticPart& p : parts)
        if (std::strncmp(p.name, name, sizeof(p.name)) == 0) return &p;
    return nullptr;
}

bool StaticScene::lightNear(const char* part, Vec2 at, float radius, int set, float out[3]) const {
    const StaticPart* p = find(part);
    if (!p || set < 0 || set >= sets) return false;
    const u8* c = colors(set);
    float sum[3] = {0, 0, 0}, weight = 0;
    for (int v = p->firstVertex; v < p->firstVertex + p->vertexCount; ++v) {
        const float dx = pos[v].x - at.x, dy = pos[v].y - at.y, d2 = dx * dx + dy * dy;
        if (d2 >= radius * radius) continue;  // (runs per dragon per frame on the 3DS)
        const float w = 1.0f - std::sqrt(d2) / radius;
        for (int k = 0; k < 3; ++k) sum[k] += w * c[v * 4 + k];
        weight += w;
    }
    if (weight <= 0) return false;
    for (int k = 0; k < 3; ++k) out[k] = sum[k] / (weight * 255.0f);
    return true;
}

bool loadStaticScene(const u8* data, std::size_t size, StaticScene& out) {
    out = StaticScene{};
    ByteReader r(data, size);
    char magic[4];
    r.bytes(magic, 4);
    const u16 version = r.u16v(), partCount = r.u16v(), sets = r.u16v();
    r.u16v();  // reserved
    if (!r.ok() || std::memcmp(magic, "ESM1", 4) != 0 || version != 1 || sets < 1 || sets > kMaxSets ||
        partCount > kMaxParts)
        return false;
    out.sets = sets;
    out.backdrop.resize(std::size_t(sets) * 4);
    r.bytes(out.backdrop.data(), out.backdrop.size());

    // Parts store their colours set by set; gather them per set across parts afterwards.
    std::vector<std::vector<u8>> partColors;
    for (int i = 0; i < partCount; ++i) {
        StaticPart p;
        r.bytes(p.name, sizeof(p.name));
        p.name[sizeof(p.name) - 1] = 0;
        p.flags = r.u8v();
        r.u8v();  // reserved
        p.vertexCount = r.u16v();
        p.indexCount = r.u16v();
        p.firstVertex = static_cast<int>(out.pos.size());
        p.firstIndex = static_cast<int>(out.indices.size());
        if (!r.ok() || p.indexCount % 3 != 0 || p.firstVertex + p.vertexCount > 0xFFFF) return false;
        for (int v = 0; v < p.vertexCount; ++v) out.pos.push_back(r.vec3());
        partColors.emplace_back(std::size_t(sets) * p.vertexCount * 4);
        r.bytes(partColors.back().data(), partColors.back().size());
        for (int k = 0; k < p.indexCount; ++k) {
            const u16 idx = r.u16v();
            if (idx >= p.vertexCount) return false;
            out.indices.push_back(static_cast<u16>(p.firstVertex + idx));
        }
        if (!r.ok()) return false;
        out.parts.push_back(p);
    }
    out.vertexCount = static_cast<int>(out.pos.size());
    out.color.resize(std::size_t(sets) * out.vertexCount * 4);
    for (std::size_t i = 0; i < out.parts.size(); ++i) {
        const StaticPart& p = out.parts[i];
        for (int s = 0; s < sets; ++s)
            std::memcpy(out.color.data() + (std::size_t(s) * out.vertexCount + p.firstVertex) * 4,
                        partColors[i].data() + std::size_t(s) * p.vertexCount * 4, std::size_t(p.vertexCount) * 4);
    }
    return true;
}

}  // namespace ec
