// A breadcrumb trail for freezes on the hardware only (run 20: Continue froze on Noah's old 3DS,
// never in the emulator). On only while sdmc:/3ds/emberclutch/trace.on exists (put there over
// FTP): each mark goes into a ring of the last lines, rewritten to trace.txt at once, so after a
// freeze and a restart the file ends at the last thing the game got to.
#pragma once

#include <3ds/types.h>

namespace ec::trace {

void start();  // checks for trace.on (after the SD card is up)
bool on();
void mark(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
// Each frame: the trail is kept for the first 10 seconds and 10 seconds after each change of
// scene (writing every stage slows the game), quiet in between.
void frame(unsigned long n, int scene);

}  // namespace ec::trace
