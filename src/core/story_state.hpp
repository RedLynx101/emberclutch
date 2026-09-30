// What the save keeps of the story (D137): each quest's step, the story's flags and small counters,
// the mailbox, and the days things happened. Slots come from story/ids.lock (a name keeps its slot
// for good), so a save survives the scripts growing. core/story reads and moves it; core/save
// writes it (a block of its own, its size first).
#pragma once

#include "core/types.hpp"

namespace ec::story {

constexpr int kMaxQuests = 64;
constexpr int kFlagBytes = 32;   // 256 flags
constexpr int kMaxVars = 32;
constexpr int kMaxLetters = 64;
constexpr int kMaxEvents = 16;
constexpr u8 kQuestDone = 0xFF;

struct StoryState {
    u8 quest[kMaxQuests] = {};       // by slot: 0 not begun, 1.. the step it's on, kQuestDone
    u8 flags[kFlagBytes] = {};       // a bit each
    u8 vars[kMaxVars] = {};          // small counters and bit sets (map pages found ...)
    u64 mailIn = 0, mailRead = 0;    // letters delivered to the mailbox, and read (a bit a slot)
    s32 questDay[kMaxQuests] = {};   // the day (core/clock dayIndex) each quest was finished
    s32 eventDay[kMaxEvents] = {};   // the day each event happened (0: not yet)
};

}  // namespace ec::story
