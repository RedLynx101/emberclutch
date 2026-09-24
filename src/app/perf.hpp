// Where a frame's milliseconds go (Alpha 2 WP11d, the hardware performance pass): the CPU
// time split into sections, shown on the dev overlay and written into each screenshot's log
// line, so a Y press on the 3DS brings back the numbers. A section counts its own time only
// (an inner section's time is taken out of the one around it).
#pragma once

#include <3ds/types.h>

namespace ec::perf {

enum Section : u8 {
    Update,  // the scene's update (behaviour, physics, care)
    Audio,   // music streaming and the mixer
    Pose,    // dragons posed: animation, look-at, the floor contact, skinning matrices
    Submit,  // dragons' draw calls (uniforms, buffers)
    Room,    // the den room, its props and toys
    Text,    // laying out and drawing text (both screens)
    Fx,      // particles
    Top,     // the rest of the top screen (2D shapes, sprites)
    Bottom,  // the rest of the bottom screen
    Count
};

void frameStart();  // at the top of each frame: last frame's totals become the shown ones

class Scope {
public:
    explicit Scope(Section s);
    ~Scope();
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

private:
    Section section_;
    Section outer_;
    u64 start_;
};

float ms(Section s);  // last frames', smoothed
// "upd 1.2 aud 0.4 pose 3.1 sub 1.0 room 0.8 txt 0.9 fx 0.3 top 1.9 bot 2.2" (the overlay, the log)
const char* line();

}  // namespace ec::perf
