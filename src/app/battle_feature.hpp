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
// "hollow <floor>" into a floor of the Hollow, "board" the league's board at
// the arena, "level <n>" your partner's level, "league <won> [beaten bits]" the leagues'
// progress, "auto on|off" battles playing themselves.
void battleCommand(App& app, const char* args);
// (feature_hollow.cpp, for the command) A floor of the Hollow at once, from wherever you are.
void startHollowFloor(App& app, int floor);

}  // namespace ec
