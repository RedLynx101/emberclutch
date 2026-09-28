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
// Anyone else (1.0: a challenger, the Hollow's keeper, a pageant host): their lines, nothing settled.
struct Speaker {
    const char* name = "";   // (static strings: the box keeps the pointers)
    const char* title = "";
    u8 voice = 0;
    float pitch = 1.0f;
    s8 portrait = -1;        // the people sheet's (villager order), or -1: their initial on a disc
    Rgb tint{250, 226, 196};
};
void startSpeech(App& app, const Speaker& who, const Talk& lines);
bool talking(const App& app);
// While talking: advance the letters, read on with A or a tap. Returns true while it's open.
bool updateTalk(App& app, const Input& in);
// The box over the bottom screen (call last).
void drawTalk(App& app);

}  // namespace ec
