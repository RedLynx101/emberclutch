// Files read ahead on a thread of their own (run 18's hitch log: a piece of a kind took
// 0.3-2.3 s to read from the card on the 3DS, and the game waited for it). Ask for the files a
// thing will need (want); the reading happens off the main thread; once a file is in (ready),
// the loader's readFile() takes it from memory at once. A file asked for and not in yet is
// waited for rather than read twice; one never asked for is read on the spot, as before.
#pragma once

#include <cstddef>
#include <vector>

#include <3ds/types.h>

namespace ec::prefetch {

void want(const char* path);   // queue it (no-op if it's queued, in or being read)
bool ready(const char* path);  // read and waiting to be taken
// If the file was asked for, waits until it's read and hands it over (true; false if it
// couldn't be read); if it wasn't, returns false at once and the caller reads it itself.
bool take(const char* path, std::vector<u8>& out, bool& found);
void shutdown();

}  // namespace ec::prefetch
