// The valley's people (Beta WP13, D74-D75) and what they say (WP14: the Lantern Festival):
// who each is, where they stand (at a place, in its frame), how their voice sounds, and their
// lines, which follow the festival's quests. Talking to someone plays their lines (the dialogue
// box shows them letter by letter, voiced); finishing sets what the talk settles (a flag the
// quests watch). Pure logic (PC-tested).
#pragma once

#include "core/math3d.hpp"
#include "core/save.hpp"

namespace ec {

enum class Villager : u8 { Keeper, Market, Sanctuary, Steward, Child, Traveller, Count };
constexpr int kVillagers = static_cast<int>(Villager::Count);

struct VillagerInfo {
    const char* id;      // its model and portrait: romfs/people/<id>.ecm, assets/sprites/people/<id>.png
    const char* name;
    const char* title;   // under the name in the dialogue box
    int place;           // core/valley ValleyPlace it stands at
    Vec2 at;             // in the place's frame
    float facing;        // radians, turned from the place's front
    u8 voice;            // which voice alphabet (0, 1)
    float pitch;         // how high it's played
};
const VillagerInfo& villagerInfo(Villager v);

constexpr int kMaxLines = 6;
struct Talk {
    const char* lines[kMaxLines] = {};
    int count = 0;
    u32 sets = 0;        // WorldFlag bits it settles once the last line is read
};

// What they say now (the festival's progress, the flags, your dragon's name for {D}, yours for {P}).
Talk talkTo(const SaveData& s, Villager v);
// The last line read: what it settles is set; true if anything new was.
bool finishTalk(SaveData& s, Villager v, const Talk& t);

// A talk in progress (the dialogue box, app/dialogue).
struct DialogueState {
    bool active = false;
    Villager who = Villager::Keeper;
    Talk talk;
    int line = 0;
    float shown = 0;      // letters of the line shown so far
    float blipFor = 0;    // seconds to the next voiced letter
    char text[160] = {};  // the line, filled in
};

// A line with {D} (your partner's name) and {P} (yours) filled in.
void fillLine(const char* line, const SaveData& s, char* out, int cap);

}  // namespace ec
