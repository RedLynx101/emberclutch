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

// Deletes both slots and the legacy dev save.
void deleteGame();

}  // namespace ec
