// The challenges' things in 3D (Beta WP8-WP11): the Sky Rings' rings, the Lantern Trial's
// crystal lanterns, the fruit and its basket, the notice boards by the arena and the orchard,
// and the trophies and ribbons that go up on the den's shelves. Built in code from a few round
// primitives, chunky and storybook-soft like the den's props (core/prop_mesh), and drawn with
// the dragons' program (render3d). Pure geometry: tests/test_challenges.cpp keeps them in budget.
#pragma once

#include "core/challenges.hpp"
#include "core/model.hpp"
#include "core/prop_mesh.hpp"

namespace ec {

// A ring, radius 1 about +Y (scaled to the ring's radius, turned to face its way): a band
// (slot 0) with a bright inner stripe (slot 1, glowing) and four little star studs (slot 2).
PropMesh ringMesh();
// A crystal lantern on its stone post, 1.9 m tall: stone (slot 0), brass (slot 1), the crystal
// (slot 2, glowing when lit).
PropMesh crystalLanternMesh();
constexpr float kCrystalHeight = 1.55f;  // the crystal's middle, above the ground
// A fruit, 1 m across at scale 1 (drawn small): its skin (slot 0), the stem (1), a leaf (2).
PropMesh fruitMesh(challenge::Fruit f);
// The basket by your feet at the orchard: wicker (slot 0), fruit heaped in it (slots 1-3).
PropMesh basketMesh();
// A notice board on two posts under a little roof, three posters pinned on it: wood (slot 0),
// the roof (slot 1), the posters' paper (slot 2), their pictures (slot 3).
PropMesh boardMesh();
constexpr float kBoardHeight = 2.4f;
// A cup's trophy, 0.6 tall: the cup (slot 0, the cup's colour), its base (slot 1), and on top a
// little sign of its challenge (slot 2: a ring, a lantern's flame, an apple).
PropMesh trophyMesh(Challenge c);
constexpr float kTrophyHeight = 0.6f;
// A rosette for a cup's ribbon, its back to +Y (the wall): pleats (slot 0), a button (slot 1),
// two tails (slot 0).
PropMesh rosetteMesh();
// Everything won on the den's shelves as one mesh in den space (one draw): each challenge's
// trophy in the colour of its highest cup, a rosette per cup ribbon; its ten palette colours
// (the cups', the challenges' signs, the wood) from shelfPalette.
PropMesh shelfMesh(const u8 cups[kChallenges], u16 ribbons);
void shelfPalette(Rgb out[kPalCount], float glow[kPalCount]);

PropLook ringLook(int cup, bool next);
PropLook crystalLook(int k, float lit);
PropLook fruitLook(challenge::Fruit f);
PropLook basketLook();
PropLook boardLook();
PropLook trophyLook(Challenge c, int cup);
PropLook rosetteLook(Challenge c, int cup);

// Where the trophies stand and the ribbons hang in the den: on and along its two shelves
// (tools/blender/den_model.py shelves(); den space, adult units). `slot`: a trophy per challenge;
// twelve ribbons, six to a shelf's edge.
DecorPlace trophySpot(Challenge c);
DecorPlace ribbonSpot(int slot);

}  // namespace ec
