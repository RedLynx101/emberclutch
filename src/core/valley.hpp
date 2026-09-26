// Skyreach Valley (Beta WP1, docs/plan/beta.md): the open landscape as a height field with its
// trees, floating islands and places, loaded from romfs/valley/skyreach.evl
// (tools/valley/make_valley.py). The renderer asks for tiles near the camera at one of three
// detail levels; each is built here from the height field, with skirts that hide the cracks
// between levels, and (near) its trees. Pure logic (PC-tested); src/app/render3d draws it.
#pragma once

#include <cstddef>
#include <vector>

#include "core/math3d.hpp"
#include "core/types.hpp"

namespace ec {

constexpr int kTileQuads = 16;   // quads a side at full detail (a tile is 64 m at 4 m spacing)
constexpr int kValleyLods = 3;   // 16, 8 and 4 quads a side
constexpr int kTreeLods = 2;     // trees on the two nearer levels only

// The places (core/world names them; the valley file places them). Beta adds the keeper's lodge
// by the waterfall, the floating isles' top, an orchard, a windmill bridge and two secrets.
enum ValleyPlace : u8 {
    kPlaceDen, kPlaceMarket, kPlaceStone, kPlaceSanctuary, kPlaceVault, kPlaceTrailhead, kPlaceArena, kPlaceLake,
    kPlaceKeeper, kPlaceIsles, kPlaceOrchard, kPlaceMill, kPlaceGrotto, kPlaceRuins,
    kPlaceCount
};

struct ValleyTree {
    float x, y;
    u8 height, shade;
};
struct ValleyIsland {
    Vec3 at;       // the middle of its grassy top
    float radius;
};
struct ValleyPlaceInfo {
    u8 id;
    Vec3 at;
    float heading;  // which way it faces (the den's cave mouth: out of the cliff)
};

struct Valley {
    int n = 0;                       // samples a side
    float spacing = 4, x0 = 0, y0 = 0, hmin = 0, hmax = 0, water = 0;
    std::vector<float> h;            // n * n heights, row j (y) then column i (x)
    std::vector<u8> rgb;             // n * n colours, the light baked in
    std::vector<ValleyTree> trees;
    std::vector<ValleyIsland> islands;
    std::vector<ValleyPlaceInfo> places;
    std::vector<float> tileLow, tileHigh;  // per tile: its lowest and highest ground (culling)
    std::vector<std::vector<int>> tileTrees;  // per tile: the trees standing in it

    int tiles() const { return n > 1 ? (n - 1) / kTileQuads : 0; }  // a side
    float size() const { return (n - 1) * spacing; }
    float tileSize() const { return kTileQuads * spacing; }
    // Ground height (bilinear), clamped at the edges; the water's surface isn't counted.
    float heightAt(float x, float y) const;
    Vec3 normalAt(float x, float y) const;
    const ValleyPlaceInfo* place(u8 id) const;
    bool inside(float x, float y) const;
};

bool loadValley(const u8* data, std::size_t size, Valley& out);

// Vertices (position, RGBA colour) and a triangle list, for the static program.
struct ValleyMesh {
    std::vector<Vec3> pos;
    std::vector<u8> color;  // 4 per vertex
    std::vector<u16> idx;
    int triangles() const { return static_cast<int>(idx.size() / 3); }
    void clear();
};

// One tile (tx, ty from the valley's south-west corner) at a detail level: its ground, the
// skirts round its edges (hanging down, so neighbours at another level never show a gap)
// and, on the nearer levels, its trees.
void buildValleyTile(const Valley& v, int tx, int ty, int lod, ValleyMesh& out);
// What isn't ground: the floating islands (grassy tops, rocky undersides, a few trees) and
// the den's cave mouth in its cliff.
void buildValleyExtras(const Valley& v, ValleyMesh& out);
// See-through things: the water's sheet over the whole valley and the den's waterfall.
void buildValleyWater(const Valley& v, ValleyMesh& out);
// The detail level for a tile this far from the camera (metres).
int valleyLodFor(float distance);

}  // namespace ec
