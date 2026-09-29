// A breadcrumb trail for freezes on the hardware only (run 20: Continue froze on Noah's old 3DS,
// never in the emulator). On only while sdmc:/3ds/emberclutch/trace.on exists (put there over
// FTP): each mark goes into a ring of the last lines, written out to trace.txt in batches, so
// after a freeze and a restart the file ends near the last thing the game got to.
#pragma once

#include <3ds/types.h>

struct C3D_RenderTarget_tag;

namespace ec::trace {

void start();  // checks for trace.on (after the SD card is up)
bool on();
void mark(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
// Each frame: the trail is kept for the first 10 seconds and 10 seconds after each change of
// scene, quiet in between.
void frame(unsigned long n, int scene);
// Just before the frame waits on the GPU: the marks so far go to the card (every frame while
// checking, below; about once a second otherwise: each write costs a frame's time).
void sync();

// GPU checkpoints (run 21 froze with the GPU never finishing the den's first frame): for the first
// frames of each scene, `gpu` sends what's drawn so far, waits for the GPU to finish it and marks
// that it did, so a freeze ends the trail on the part that hung ("sent" with no "drawn").
// `target` names the screen being drawn (drawing goes on there after the wait).
void target(C3D_RenderTarget_tag* t);
void gpu(const char* what);

}  // namespace ec::trace
