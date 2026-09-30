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
void stop();   // the watchdog's thread ended (the game closing)
bool on();
void mark(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
// Each frame, with what's on screen (the scene and the menu page): the marks are kept only for
// the first frames after it changes (run 21: writing them all slowed the game to a crawl), with
// a line every 5 seconds in between.
void frame(unsigned long n, int view);
// After the scene's update: a view it switched to is checked from this, its first frame (take 4:
// the Wanderers' Trailhead froze the 3DS on the frame its door switched to, unchecked).
void view(unsigned long n, int view);
// Just before the frame waits on the GPU: the marks so far go to the card.
void sync();

// GPU checkpoints (run 21 froze with the GPU never finishing the den's first frame): for the first
// frames of each view, `gpu` sends what's drawn so far, waits for the GPU to finish it and marks
// that it did, so a freeze ends the trail on the part that hung ("sent" with no "drawn").
// `target` names the screen being drawn (drawing goes on there after the wait).
void target(C3D_RenderTarget_tag* t);
void screens(C3D_RenderTarget_tag* top, C3D_RenderTarget_tag* topRight, C3D_RenderTarget_tag* bottom);  // (never sent half-drawn)
void gpu(const char* what);
// A part a session froze in (its trail ended on "gpu: <part> sent"): listed in hangs.txt at the
// next start, so the game can draw it a safer way (the valley's ground: plain). Read whether the
// trace is on or not; deleting hangs.txt clears the list.
bool hung(const char* part);

// A watch for one-frame blips in the picture (take 4: the valley's ground went teal for a frame now
// and then on the 3DS): with the trace on, the top screen's lower half is sampled as each frame
// finishes, and a frame unlike the one before and the one after (which match each other) is
// logged with what drew it. `info` is kept with the frame: its triangles, draws and scene.
void watchBeforeFrameEnd(unsigned tris, unsigned draws, int scene);
void watchAfterFrameBegin();
// And the depth the last frame left on the top screen (D112: the teal frames were the ground's depth
// wiped before the water drew): after C3D_FrameBegin, a sample of each half of the depth buffer;
// a valley frame whose lower half is mostly empty (depth 0, as cleared) is a hole, counted in the
// 5-second lines and the first ones logged.
void watchDepth(C3D_RenderTarget_tag* top, int valleyScene);

// The fix (D116, D117): the depth test's state sent again before each of the valley's ground tiles
// (D115's trial: the ground's holes from 74% of frames to about 1%). depthMode() is 1 for it, 0 for
// the old way (held by scripts alone: 0.9.10's window of the old way and its reads of the GPU's
// registers hung the 3DS). Read once per valley frame.
int depthMode();
void holdFix(int fix);  // (scripted runs: that fix held whatever the trace; -1 lets go)
// The commands sent for the ground's first tile (between begin and end), kept for the frame and
// logged for the first holes and a whole frame; and, in a probed frame, the depth after each tile.
void commands(bool begin);
void tileProbe(int i, int tx, int ty, int lod, int count);
// What the valley frame being drawn did, logged with its hole if it has one: its haze table made
// again, tiles built (counted against the holes too), and a note (the haze, the eye).
void frameFacts(bool lutRebuilt, int built);
void frameNote(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
// A GPU checkpoint (gpu() in a view's first frames) and, in a frame probed after a hole, the top
// screen's depth read back after that part (the GPU made to finish it first).
void checkpoint(const char* part);

}  // namespace ec::trace
