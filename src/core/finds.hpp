// Finds about the valley (Beta WP7): twenty spots worth a look, each with something waiting
// once (Gleam, a trinket for the hoard, twice a wild egg), shown as a glint until found; six
// on the floating islands' tops and two up high, reached only from the air on a grown dragon.
// And the map's fog: the valley in 32 x 32 cells, each cleared once you've been near. Pure
// logic (PC-tested).
#pragma once

#include "core/math3d.hpp"
#include "core/save.hpp"
#include "core/valley.hpp"

namespace ec {

constexpr int kFindSpots = 20;
constexpr int kFogCells = 32;  // a side (world.explored holds a bit each)

struct FindSpot {
    Vec2 at;
    s8 island = -1;   // on this floating island's top (Valley::islands), else on the ground
    bool fromAir = false;  // only reached flying (the islands, the plateau, the heights)
    u16 gleam = 0;    // what it holds: Gleam,
    s8 trinket = -1;  // or a trinket (core/wanderings Trinket),
    bool egg = false; // or a wild egg
};
const FindSpot& findSpot(int i);
// Where its glint shows (on the ground, or on its island's top).
Vec3 findAt(const Valley& v, int i);
bool findDone(const SaveData& s, int i);
// The spot not yet found within reach of `at` (-1: none): 4 m on foot, 9 m from the air.
int findNear(const SaveData& s, const Valley& v, Vec3 at, bool flying);

struct FindReward {
    u16 gleam = 0;
    s8 trinket = -1;
    int egg = -1;  // its SaveData index (in a free nest or the Cold Vault), -1: none
};
// Takes it: what it held goes to the save (an egg's kind by rarity), and it's marked found.
FindReward takeFind(SaveData& s, int i, s64 now, Rng& rng);

// The map's fog: clears the cells within `radius` metres of `at`; true if any cleared.
bool explore(SaveData& s, const Valley& v, Vec2 at, float radius);
bool explored(const SaveData& s, int cx, int cy);

}  // namespace ec
