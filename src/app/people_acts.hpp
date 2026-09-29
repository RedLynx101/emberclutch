// The valley's people at their doings (workstream D): the villagers by the hour at their spots
// (core/routines: Maple tidying her stall, Bram scattering feed, Wren writing on her clipboard, Pip
// flying his toy dragon, sitting a while in the evening, dozing at night with a soft snore when
// you're near, a look about or a stretch now and then), and a pageant show's audience by Moonpetal
// Glade's stage, clapping for the results. scene_valley and glade_show call these.
#pragma once

#include "app/app.hpp"
#include "app/render3d.hpp"
#include "core/anim.hpp"
#include "core/valley.hpp"

namespace ec::acts {

// A villager (core/villagers index) resting at their spot: the hour's doing on their animator
// (call it each frame while they aren't talking, nodding or waving). `dist`: from you, metres.
void villagerRest(App& app, int villager, Animator& anim, float dist);
// Sat on the ground or dozing: they stay put (no getting up to wave, no turning to you).
bool villagerStill(const App& app, int villager);

// A show's audience (two, by the stage's front corner), in the view's people (the farthest of the
// rest give way if it's full); clapping (with the sound of it) when `applause`. The show calls it in
// its wide shots (the welcome, the results).
void showAudience(App& app, const Valley& v, r3d::ValleyView& view, bool applause);
// Someone into the view's people: a free place, else instead of the farthest from the camera if
// they're nearer (never you, the first); false if there's no room.
bool addPerson(r3d::ValleyView& view, const r3d::PersonView& p);

}  // namespace ec::acts
