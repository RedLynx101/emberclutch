// The valley's people (Beta WP13, D74-D75): who each is, where they stand (at a place, in its
// frame) and how their voice sounds; the talk shape the dialogue box plays (lines, their feelings
// and speakers). What they say is the story's (core/story, D137). Pure logic (PC-tested).
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

constexpr int kMaxLines = 12;
struct Talk {
    const char* lines[kMaxLines] = {};
    u8 feel[kMaxLines] = {};         // each line's feeling (core/story Feel; a line's own "[tag]" too)
    s8 speaker[kMaxLines] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};  // a story person saying it (-1: whoever this is with)
    int count = 0;
    u32 sets = 0;        // WorldFlag bits it settles once the last line is read
    s16 rule = -1;       // the story rule it came from (its effects once read; core/story), or -1
    s16 pickup = -1;     // or the story pickup it came from
    s8 person = -1;      // the story person it's with (-1: a villager by the dialogue's `who`, or a custom speaker)
};

// (What they say is the story's now: core/story talkTo, from story/*.story, D137.)

// A talk in progress (the dialogue box, app/dialogue).
struct DialogueState {
    bool active = false;
    Villager who = Villager::Keeper;
    Talk talk;
    int line = 0;
    float shown = 0;      // letters of the line shown so far
    float blipFor = 0;    // seconds to the next voiced letter
    char text[160] = {};  // the line, filled in
    // Someone who isn't one of the villagers (1.0: a challenger, a keeper, a host): their name,
    // title, voice and pitch, and a portrait from the people sheet (-1: their initial, drawn).
    bool custom = false;
    const char* name = "";
    const char* title = "";
    u8 voice = 0;
    float pitch = 1.0f;
    s8 portrait = -1;
    Rgb tint{250, 226, 196};  // the initial's disc (a custom speaker without a portrait)
    // This line (D137): its feeling (core/story Feel), who says it (a story person; -1: the talk's own
    // speaker), seconds since its feeling popped (-1: no pop), and the voice alphabet loaded now.
    u8 feel = 0;
    s8 speaker = -1;
    float emoteT = -1;
    u8 voiceLoaded = 0xFF;
    u16 serial = 0;  // counts the lines begun (a figure shows each line's feeling in its body once, D138)
};

// A line with {D} (your partner's name) and {P} (yours) filled in.
void fillLine(const char* line, const SaveData& s, char* out, int cap);

}  // namespace ec
