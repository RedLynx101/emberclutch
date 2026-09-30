// Finds about the valley (Beta WP7): twenty-two spots worth a look, each with something waiting
// once (Gleam, a trinket for the hoard, twice a wild egg), shown as a glint until found; six
// on the floating islands' tops and four up high, reached only from the air on a grown dragon.
// And (run 19) ten little finds a day, scattered afresh each morning at random spots on open
// ground: a little Gleam, a treat for the pouch, now and then a trinket.
// And the map's fog: the valley in 32 x 32 cells, each cleared once you've been near. Pure
// logic (PC-tested).
#pragma once

#include "core/math3d.hpp"
#include "core/save.hpp"
#include "core/valley.hpp"

namespace ec {

constexpr int kFindSpots = 22;   // the treasures, found once
constexpr int kDailyFinds = 10;  // the day's little finds (world.finds' bits 22-31, cleared each day)
constexpr int kAllFinds = kFindSpots + kDailyFinds;
constexpr int kFogCells = 32;  // a side (world.explored holds a bit each)

// The picnic on the Nesting Stone's hill (1.0, D133): a blanket and a basket from a date there, a
// letter left on it, and Gleam in the basket. Read once (kFlagLoveLetter).
constexpr Vec2 kPicnicAt{608, 550};
constexpr u16 kLetterGleam = 250;
constexpr float kLetterReach = 2.4f;  // how near you come to pick it up
// Picks the letter up: the flag, and the Gleam; false if it was read already.
bool takeLetter(SaveData& s);
// The picnic's triangles, in the valley's space (the blanket on the ground, the basket, two cups,
// a little cake; the letter on the blanket unless it's been taken).
void buildPicnic(const Valley& v, bool letter, ValleyMesh& out);

struct FindSpot {
    Vec2 at;
    s8 island = -1;   // on this floating island's top (Valley::islands), else on the ground
    bool fromAir = false;  // only reached flying (the islands, the plateau, the heights)
    u16 gleam = 0;    // what it holds: Gleam,
    s8 trinket = -1;  // or a trinket (core/wanderings Trinket),
    bool egg = false; // or a wild egg
};
const FindSpot& findSpot(int i);
// Where its glint shows (on the ground, or on its island's top). A daily find's spot comes from
// the day and the save's seed (the overload with the save; the other only knows the treasures).
Vec3 findAt(const Valley& v, int i);
Vec3 findAt(const Valley& v, const SaveData& s, int i);
// A new day: the day's finds scattered afresh (the taken ones back). True if they were renewed.
bool renewFinds(SaveData& s, s32 today);
bool findDone(const SaveData& s, int i);
// The spot not yet found within reach of `at` (-1: none): 4 m on foot, 9 m from the air.
int findNear(const SaveData& s, const Valley& v, Vec3 at, bool flying);

struct FindReward {
    u16 gleam = 0;
    s8 trinket = -1;
    s8 food = -1;  // a treat into the pouch (core/care Food)
    int egg = -1;  // its SaveData index (in a free nest or the Cold Vault), -1: none
    int accessory = -1;  // a thing to wear, tucked in with a treasure (the 3rd, 7th and 12th found)
};
// Takes it: what it held goes to the save (an egg's kind by rarity), and it's marked found.
FindReward takeFind(SaveData& s, int i, s64 now, Rng& rng);

// The map's fog: clears the cells within `radius` metres of `at`; true if any cleared.
bool explore(SaveData& s, const Valley& v, Vec2 at, float radius);
bool explored(const SaveData& s, int cx, int cy);

}  // namespace ec
