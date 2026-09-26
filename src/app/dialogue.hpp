// Talking to the valley's people (Beta WP13, D75): the dialogue box on the bottom screen, their
// name on a tab and their portrait beside it, each line coming in letter by letter with a quick
// voiced blip a letter (their voice, pitched their way); A (or a tap) shows the rest of the line,
// then the next; the last line read settles what the talk settles (core/villagers) and the
// Lantern Festival moves on.
#pragma once

#include "app/app.hpp"
#include "core/villagers.hpp"

namespace ec {

// (DialogueState lives in core/villagers.hpp: App keeps one.)

void startTalk(App& app, Villager v);
// Someone saying lines of the game's own (the challenges' hosts): no flags settle after.
void startLines(App& app, Villager v, const Talk& lines);
bool talking(const App& app);
// While talking: advance the letters, read on with A or a tap. Returns true while it's open.
bool updateTalk(App& app, const Input& in);
// The box over the bottom screen (call last).
void drawTalk(App& app);

}  // namespace ec
