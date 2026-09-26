// The first campaign, *The Lantern Festival* (Beta WP14, D74; its outline is docs/plan/beta.md):
// eight quests, each a few steps. A step is done when what it asks for is true of the world
// (a place found, a lantern lit, a cup won, someone met: core/world), so the quests follow
// whatever you do, in any order, and never get stuck; update() moves them on and says what
// happened. Pure logic (PC-tested).
#pragma once

#include "core/save.hpp"

namespace ec::campaign {

constexpr int kQuests = 8;
constexpr u8 kQuestDone = 0xFF;

struct QuestView {
    const char* title = "";
    const char* step = "";  // what to do next (the done quest's last line once it's done)
    int stepIndex = 0, stepCount = 0;
    bool started = false, done = false;
};

int questCount();
QuestView view(const SaveData& s, int quest);

struct News {
    int started = -1;    // a quest begun
    int stepped = -1;    // a quest moved on a step
    int finished = -1;   // a quest done (its reward given)
    u32 gleam = 0;       // Gleam the finished quest gave
    bool starEgg = false;  // the festival's end: the star-born egg is to be given (the game makes it)
};
// Moves every quest on as far as the world allows; call after anything changes the world.
News update(SaveData& s);

// The quest to show first (the earliest begun and not done), or -1.
int currentQuest(const SaveData& s);

}  // namespace ec::campaign
