// The valley's people at their doings (workstream D): the villagers by the hour at their spots
// (core/routines: Maple tidying her stall, Bram scattering feed, Wren writing on her clipboard, Pip
// flying his toy dragon, sitting a while in the evening, dozing at night with a soft snore when
// you're near, a look about or a stretch now and then). scene_valley calls these.
#pragma once

#include "app/app.hpp"
#include "core/anim.hpp"

namespace ec::acts {

// A villager (core/villagers index) resting at their spot: the hour's doing on their animator
// (call it each frame while they aren't talking, nodding or waving). `dist`: from you, metres.
void villagerRest(App& app, int villager, Animator& anim, float dist);
// Sat on the ground or dozing: they stay put (no getting up to wave, no turning to you).
bool villagerStill(const App& app, int villager);

}  // namespace ec::acts
