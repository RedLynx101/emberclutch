// Keeping the camera clear (Noah, 2026-09-29: "textures ... clipping into frame from behind ...
// maybe make an elegant solution for keeping the camera safe"). Whatever sets the camera (walking,
// riding, a battle, fishing, a show), the valley pulls its eye in toward what it looks at until the
// way between is open: past the places' own triangles (a cave's walls and roof, a house, a crystal
// cluster) and above the ground (a cliff, a bowl's rim, a cave cut into the hills), with a margin,
// testing a small bundle of lines so the frame's edges stay clear too. Pure (PC-tested); the
// renderer keeps each loaded place's triangles binned here.
#pragma once

#include <vector>

#include "core/math3d.hpp"
#include "core/types.hpp"

namespace ec {

struct Valley;

// One place's solid triangles in its own frame, binned on a grid over the ground plane so a line
// only meets the few near it.
struct OccluderMesh {
    std::vector<Vec3> pos;       // the triangles' corners, three a triangle
    Vec2 lo{0, 0};               // the grid's corner (place frame)
    float cell = 4.0f;           // metres a cell
    int nx = 0, ny = 0;
    std::vector<u32> start;      // nx * ny + 1 offsets into `tris`
    std::vector<u16> tris;       // triangle numbers, cell by cell
    void clear();
    // Adds a part's triangles (vertex positions x, y, z packed; a triangle list over them).
    void add(const float* xyz, const u16* idx, int indexCount);
    void bin();  // after every add: the grid
    bool empty() const { return pos.empty(); }
    // The first triangle crossed going from a to b (place frame), as a share of the way (1: none).
    float firstHit(Vec3 a, Vec3 b) const;
};

// Whether a place's part stands in the camera's way: its solid parts do; glows and lit shells
// (see-through), and the mill's turning sails (they move) don't.
bool occludes(const char* partName, u8 flags);

// A place's occluders placed in the valley (its anchor and heading: core/place_layout's frame).
struct PlacedOccluders {
    const OccluderMesh* mesh = nullptr;
    Vec3 at;
    float heading = 0;
    float reach = 0;  // how far its triangles go from its anchor
};

// The eye pulled in along pivot -> eye until the way is clear: of the places' triangles and the
// valley's ground, with `margin` metres to spare, a bundle of five lines (the middle and four
// `spread` metres out at the eye's end) standing for the view's near corners. At least `nearest`
// metres from the pivot.
// Then with `room` metres round the eye clear of walls; in the tightest spots it rises to look down.
Vec3 clearEye(const Valley& v, const std::vector<PlacedOccluders>& places, Vec3 pivot, Vec3 eye, float margin = 0.45f,
              float spread = 0.3f, float nearest = 1.2f, float room = 1.6f);

}  // namespace ec
