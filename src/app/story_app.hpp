// The story (core/story, D137) the game's way: its news as toasts and sounds (a quest begun, moved on
// or done; a letter in the mailbox; the star-born egg), talks with the story's people, and the
// mailbox by the den's door (the list of letters and a letter's card on the bottom screen).
#pragma once

#include "app/app.hpp"
#include "app/valley_ext.hpp"
#include "core/story.hpp"

namespace ec {

// The news told: a toast (queued behind another, or at once), a sound, the star-born egg, saved.
void storyNews(App& app, const story::News& n, bool queue = true);
// story::update now, and its news told. Call after anything changes the world.
story::News storyUpdate(App& app);

// A talk with a story person (the dialogue box: their name, portrait, voice, and every line's feeling).
// False if they have nothing to say.
bool startStoryTalk(App& app, int person);
// A pickup's or a sign's words (the narrator's), its effects once read.
bool startPickupTalk(App& app, int pickup);

// ---- The mailbox (bottom screen, over whatever scene opened it)
void openMailbox(App& app);
bool mailboxOpen(const App& app);
// While open: A, B and taps; returns true while it's open.
bool updateMailbox(App& app, const Input& in);
void drawMailbox(App& app, const Input& in);

// The story in the valley (app/feature_story.cpp): its people, pickups, signs and the mailbox.
extern const vext::Feature kStoryFeature;
// The mailbox and the signs, drawn after the valley (its camera, fog and depth).
void drawStoryProps(App& app, const Valley& v, s64 now);

}  // namespace ec
