// The den's bought things in 3D (Alpha 2 WP7): toys on the floor and the decor, built in
// code from a few primitives and drawn with the dragons' program (render3d), so they take
// the den's light. Pure geometry: tests/test_den.cpp checks them against the frame budget.
#pragma once

#include <vector>

#include "core/items.hpp"
#include "core/math3d.hpp"
#include "core/types.hpp"

namespace ec {

struct PropMesh {
    std::vector<Vec3> pos, nrm;
    std::vector<u8> paint;  // per vertex, as the dragon shader reads it: slot A, slot B, mix, emissive
    std::vector<u16> idx;
    int triangles() const { return static_cast<int>(idx.size() / 3); }
};

// A toy lying on the floor, around its own origin (the rope along X, one unit long: it's
// scaled to its length, or stretched between two mouths in a tug-of-war).
PropMesh toyMesh(int toy);
PropMesh bowlFoodMesh();  // the food heaped in the bowl (drawn while there's some)
PropMesh decorMesh(Item i);
// The den's own rug, at home until one is bought (the room model has none of its own).
PropMesh homeRugMesh();

// Each item's colours (palette slots 0..3) and how much the emissive parts glow.
struct PropLook {
    Rgb colour[4];
    float glow = 0;  // palette alpha for emissive vertices (lanterns, moonflowers)
};
PropLook propLook(Item i);  // Item::Count: the den's own rug
// A completed breed's banner (the Dragondex, WP12), hung in the banner spot in its colours:
// the cloth its body, an egg on it in its accent, the egg's heart in its heartglow.
PropMesh breedBannerMesh();
PropLook breedBannerLook(Rgb base, Rgb accent, Rgb glow);

// Where a decor spot is: the rug on the floor at home, the lantern and the banner on the
// back wall, the perch by the hearth, the plant under the shelves. Local +Y points at the
// wall (yaw about +Z); wall pieces are drawn bigger to read from across the room.
struct DecorPlace {
    Vec3 at;
    float yaw;
    float scale = 1.0f;
};
DecorPlace decorPlace(int spot);

constexpr float kRopeLength = 0.9f;  // a rope lying on the floor
constexpr float kOrbRadius = 0.17f;

}  // namespace ec
