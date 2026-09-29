// 1.0's battle features in the valley (D90, workstream B): the league (its challengers about the
// valley, its champions at Emberpeak Caldera, its boards) and Frostspire Hollow (its keeper and
// floors of wild dragons). Each is a vext::Feature (valley_ext.cpp lists them); both play their
// battles with app/battle_view. What the rest of the game calls is here.
#pragma once

#include "app/app.hpp"
#include "app/valley_ext.hpp"

namespace ec {

struct Valley;

extern const vext::Feature kLeagueFeature;  // feature_league.cpp
extern const vext::Feature kHollowFeature;  // feature_hollow.cpp

// The league's boards (at the arena and the caldera), drawn after r3d::drawValley like the
// challenges' boards (scene_valley calls it beside drawChallengeBoards).
void drawLeagueBoards(App& app, const Valley& v, s64 now);
// The music while a battle is on (main.cpp's musicFor; nullptr: the valley's own).
const char* battleMusic(const App& app);
// Scripted runs (autotest's `battle ...` command): "start <league> <slot>" straight into a
// challenger's battle, "talk <league> <slot>" up to them and A (their lines, then "Battle?"),
// "hollow <floor>" into a floor of the Hollow, "keeper" up to its keeper and A, "board" the league's board at
// the arena, "level <n>" your partner's level, "league <won> [beaten bits]" the leagues'
// progress, "auto on|off" battles playing themselves, "breathe on|off" both sides breathing
// every turn (to look at breath).
void battleCommand(App& app, const char* args);
// (feature_hollow.cpp, for the command) A floor of the Hollow at once, from wherever you are;
// or up to its keeper and A.
void startHollowFloor(App& app, int floor);
void startHollowKeeper(App& app);
// (feature_league.cpp, for the roaming trainers' duels, workstream D) A battle staged round
// someone standing at `them` as the league stages its challengers' (you on good ground clear of
// walls, the camera's shoulder clear): false if there's nowhere good to stand.
namespace bview { struct Setup; }
bool stageBattle(const Valley& v, Vec3 them, Vec3 you, float youHeading, Vec2 middle, float sizeYou, float sizeFoe, bool small,
                 bview::Setup& out);

}  // namespace ec
