// The .esm static mesh: the den room (written by tools/blender/den_model.py). Positions
// and baked vertex colours for each lighting set (day, evening, night: core/daylight), so
// the room is drawn unlit with two sets blended by the time of day. See architecture §5.
#pragma once

#include <cstddef>
#include <vector>

#include "core/math3d.hpp"
#include "core/types.hpp"

namespace ec {

enum StaticFlags : u8 {
    kStaticAdditive = 1,  // glow added over what is behind it (sunbeam, flames), drawn last
    kStaticFlicker = 2,   // brightness flickers at runtime (flames)
};

struct StaticPart {
    char name[16] = {};
    u8 flags = 0;
    int firstVertex = 0, vertexCount = 0;
    int firstIndex = 0, indexCount = 0;  // into StaticScene::indices
};

struct StaticScene {
    int sets = 0;
    int vertexCount = 0;
    std::vector<Vec3> pos;          // every part's vertices, concatenated
    std::vector<u8> color;          // sets x vertexCount x RGBA8: one array per lighting set
    std::vector<u16> indices;       // triangle list over the concatenated vertices
    std::vector<StaticPart> parts;  // file order: opaque parts first, additive last
    std::vector<u8> backdrop;       // sets x RGBA8: the dark beyond the room

    const StaticPart* find(const char* name) const;
    const u8* colors(int set) const { return color.data() + std::size_t(set) * vertexCount * 4; }
    int triangles() const { return static_cast<int>(indices.size() / 3); }
    // Average colour (0..1) of a part's vertices within `radius` of `at` on the floor plane,
    // nearer ones counting more, in one lighting set. False if none are that close.
    bool lightNear(const char* part, Vec2 at, float radius, int set, float out[3]) const;
};

bool loadStaticScene(const u8* data, std::size_t size, StaticScene& out);

}  // namespace ec
