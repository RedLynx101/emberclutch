// A breadcrumb trail for freezes on the hardware only (run 20: Continue froze on Noah's old 3DS,
// never in the emulator). On only while sdmc:/3ds/emberclutch/trace.on exists (put there over
// FTP): marks go into a ring of the last lines, written out to trace.txt in batches, so after a
// freeze and a restart the file ends near the last thing the game got to. The session before
// is kept as trace-prev.txt.
#pragma once

#include <3ds/types.h>

struct C3D_RenderTarget_tag;

namespace ec::trace {

void start();  // checks for trace.on and reads hangs.txt (after the SD card is up)
bool on();
void mark(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
// Each frame, with what's on screen (the scene and the menu page): the marks are kept only for
// the first frames after it changes (run 21: writing them all slowed the game to a crawl), with
// a line every 5 seconds in between.
void frame(unsigned long n, int view);
// Just before the frame waits on the GPU: the marks so far go to the card.
void sync();

// GPU checkpoints (run 21 froze with the GPU never finishing the den's first frame): for the first
// frames of each view, `gpu` sends what's drawn so far, waits for the GPU to finish it and marks
// that it did, so a freeze ends the trail on the part that hung ("sent" with no "drawn").
// `target` names the screen being drawn (drawing goes on there after the wait).
void target(C3D_RenderTarget_tag* t);
void gpu(const char* what);
// A part a session froze in (its trail ended on "gpu: <part> sent"): listed in hangs.txt at the
// next start, so the game can draw it a safer way (the valley's ground: plain). Read whether the
// trace is on or not; deleting hangs.txt clears the list.
bool hung(const char* part);

}  // namespace ec::trace
