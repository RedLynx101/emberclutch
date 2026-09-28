// Frostspire Hollow (D90): the training ground in the cold heights. Floor after floor a wild
// dragon comes out of the cave door at the back of the bowl, stronger the deeper you go (its
// kind likelier rarer, its colouring likelier the rare one), a guardian every fifth floor; you
// go on until you lose or leave, and a dragon may start again from the checkpoints it has
// reached. More experience than the league, Gleam, now and then a trained stat point (the wild
// one's best stat), and the bowl colder and darker deeper. The keeper at the mouth keeps count.
// Pure logic (PC-tested: tests/test_battle.cpp); src/app/feature_hollow.cpp plays it.
#pragma once

#include "core/battle.hpp"
#include "core/math3d.hpp"
#include "core/save.hpp"

namespace ec::hollow {

constexpr int kFloors = 30;
constexpr int kBand = 5;  // a checkpoint every five floors, the fifth a guardian
constexpr int kCheckpoints = kFloors / kBand;

int wildLevel(int floor);  // about 2 + floor x 1.2 (a guardian two more)
bool guardian(int floor);
int skillAt(int floor);    // its choosing (battle::chooseMove): at random near the top, clever deep down
// The wild one on a floor today: the same all day (so a rematch meets it again), another tomorrow.
Dragon wildOf(int floor, s32 day);
// The floors a dragon may start from: the top, and every checkpoint past a floor it has cleared
// (Dragon::frostDeepest). Returns how many (at most kCheckpoints).
int checkpoints(const Dragon& d, int out[kCheckpoints]);

// After a floor: experience (more than a challenger's), the dragon's record and your deepest,
// Gleam (every floor; a guardian's bonus once a day), a trained stat point (a guardian's once a
// day, now and then on other floors), and the first time down past a guardian, a prize.
struct Reward {
    battle::Growth growth;
    u32 gleam = 0;
    int trained = -1;         // the stat a point went to (-1: none)
    bool newDeepest = false;  // deeper than any of your dragons had been
    u8 prizeFood = 0xFF;      // a guardian's first prize: a food (core/care Food) and how many,
    u8 prizeCount = 0;
    u8 prizeTrinket = 0xFF;   // and a trinket (core/wanderings Trinket)
};
Reward record(SaveData& s, int dragonIndex, int floor, const Dragon& wild, battle::Outcome o, s32 today, Rng& rng);

// The cold deeper down: 0 at the top .. 1 at the bottom; a colour (the fog, the light) cooled
// and darkened by it.
float depth(int floor);
Rgb chill(Rgb c, int floor);

// ---- The bowl (its frame: +Y the way out east, the cave door at the back, -Y). Until the
// Hollow's model (workstream A) gives its anchors, spots on its flat floor.
Vec2 arenaSpot();  // the middle of the battle ground
Vec2 wildDoor();   // where the wild ones come out
Vec2 keeperSpot();
float keeperFacing();  // radians from the place's front

}  // namespace ec::hollow
