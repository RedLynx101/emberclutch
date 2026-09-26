// Long frames and what happened in them (run 17: the den stalled for a few seconds once, by
// itself). Anything slow that might block marks the frame (saving, a kind's models loading);
// a frame over kHitchMs is kept with its marks, the scene and the CPU split, the last 16 of
// them, and written to sdmc:/3ds/emberclutch/hitches.txt when the game closes (appended, with
// the date), so a hardware run brings back what the stall was.
#pragma once

namespace ec::hitch {

constexpr float kHitchMs = 250.0f;

void mark(const char* what);             // this frame: something slow happened (a literal)
void endFrame(float ms, int scene);      // at the top of each frame: last frame's length
int count();                             // hitches this session (the dev overlay shows it)
void write();                            // append them to the SD card (at exit)

}  // namespace ec::hitch
