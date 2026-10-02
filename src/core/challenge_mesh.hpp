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

// Driftwood Cove (1.0, workstream C): a shell on the sand, resting on z = 0, about 0.2 m across
// (kind 0 a spiral, 1 a scallop, 2 a cowrie, 3 a pearl in an open half-shell): the shell (slot
// 0), its lip or stripes (1), the pearl (2, glowing a little).
constexpr int kShellKinds = 4;
PropMesh shellMesh(int kind);
// The bobber, 0.18 m across, floating on z = 0: red above (slot 0), white below (1), a stem (2).
PropMesh bobberMesh();
// A fish, 1 m long along +Y (its head) at scale 1, its middle at the origin: back (slot 0),
// belly (1), fins and tail (2), eyes (3).
PropMesh fishMesh();

PropLook ringLook(int cup, bool next);
PropLook crystalLook(int k, float lit);
PropLook fruitLook(challenge::Fruit f);
PropLook basketLook();
PropLook boardLook();
PropLook trophyLook(Challenge c, int cup);
PropLook rosetteLook(Challenge c, int cup);
PropLook shellLook(int kind, int tint);  // tint: which of a few sandy colourings

// The story's props (D137): the mailbox by the den's door, a round-topped box on a post, its door to
// +Y: the post (slot 0), the box (1), the flag (2, up when a letter waits), the door's latch (3).
PropMesh mailboxMesh(bool flagUp);
PropLook mailboxLook();
// A signboard on a post, its face to +Y: wood (slot 0), the board (1), its words' scribbles (2).
PropMesh signMesh();
PropLook signLook();
PropLook bobberLook();
// Tam's rod while he fishes (run 25: drawn in 2D it showed in front of you and your dragons): from his
// hand at the origin out along -Y (his facing) and up to the tip, then the line straight down and out
// to the water `drop` metres below his hand: the rod (slot 0), the line (1).
PropMesh rodMesh(float drop);
PropLook rodLook();
PropLook fishLook(bool big);

// Where the trophies stand and the ribbons hang in the den: on and along its two shelves
// (tools/blender/den_model.py shelves(); den space, adult units). `slot`: a trophy per challenge;
// twelve ribbons, six to a shelf's edge.
DecorPlace trophySpot(Challenge c);
DecorPlace ribbonSpot(int slot);

}  // namespace ec
