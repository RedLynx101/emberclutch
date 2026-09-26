// SD card persistence for the save format in core/save (A/B slots).
#pragma once

#include "core/save.hpp"

namespace ec {

struct SaveSlots {
    u32 seq = 0;    // sequence number of the save last loaded or written
    int slot = -1;  // slot it lives in (0 = save.a, 1 = save.b)
};

// Loads the newest valid slot (falling back to the other if it fails to decode). If no
// save exists yet, imports the pre-WP8 dev save once. Returns false for a fresh game.
bool loadGame(SaveData& out, SaveSlots& slots);

// Writes to the slot that does NOT hold the newest save. Returns false on I/O failure.
bool saveGame(const SaveData& data, SaveSlots& slots, s64 savedAt);
// The same, but only the encoding happens now: the write goes to a thread of its own, so a slow
// SD card never stalls the game (run 17: the den froze for seconds once, and it saves every
// minute). A save asked for while one is being written waits its turn (the newest wins).
// Returns false if the save can't be encoded; a failed write shows up in saveWriteFailed().
bool saveGameAsync(const SaveData& data, SaveSlots& slots, s64 savedAt);
bool saveWriteFailed();  // a write on the thread failed since the last call
void finishSaves();      // at exit: the last write lands, and the thread ends

// Deletes both slots and the legacy dev save (after any write in progress).
void deleteGame();

}  // namespace ec
