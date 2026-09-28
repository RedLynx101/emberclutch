// Roaming trainers (workstream D, core/roamers): three to five trainers out on Skyreach Valley's
// earth paths each day, their dragons at their heels, stopping at the paths' ends to sit and
// take in the view, waving as you pass (a bubble with a word of hello). Walk up and press A: their
// lines, then "Duel?" on the bottom screen; a friendly duel right there (app/battle_view, staged
// as the league stages its battles), their dragon at your partner's level. A vext::Feature
// (valley_ext.cpp lists it); feature_roamers.cpp.
#pragma once

#include "app/app.hpp"
#include "app/valley_ext.hpp"

namespace ec {

extern const vext::Feature kRoamerFeature;

// Scripted runs (autotest's `roamer ...`): "list" (today's trainers and where they are, to the
// log), "near <n>" (you a few steps ahead of trainer n on their way, facing them: they walk up,
// wave and stop), "talk <n>" (up to them and A: their lines, then "Duel?"), "duel <n>" (straight
// into a duel with them), "watch <n> [seconds]" (the camera beside them as they walk; B or the
// time ends it), "level <n>" (their dragon's level from now on, to script a win or a loss; 0: fair
// again), "hour" (nothing: use `hour 10`, they walk from 8:00 to 20:00). n: today's order, 0 first.
void roamerCommand(App& app, const char* args);

}  // namespace ec
