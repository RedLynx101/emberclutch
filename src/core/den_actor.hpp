// A dragon living in the den: its behavior and its animation, stepped together each frame.
// The 3DS den scene and the PC tests share this loop.
#pragma once

#include "core/anim.hpp"
#include "core/behavior.hpp"
#include "core/model.hpp"

namespace ec {

// Library index of every behavior clip for a body form (a clip named "<name>_h" replaces
// "<name>" on the hatchling). False if the clip file lacks one.
bool resolveClips(const AnimLibrary& lib, int form, int out[static_cast<int>(ClipId::Count)]);

// How fast a locomotion clip's planted feet move backward under this body (model units per
// second, at growth t and build): the ground speed at which the feet do not slide.
float locomotionSpeed(const ModelData& m, const AnimBinding& bind, const AnimClip& clip, float t, int build);

struct DenActor {
    DenBehavior behavior;
    Animator anim;
    u16 playedSerial = 0xFFFF;
    int speedForm = -1;  // the body the walking speeds were measured on
    float speedT = -1;
    float look = 0;      // smoothed look-at-the-player weight

    void reset(const DenLayout& den, u32 seed);
    // Re-measures walk/trot speeds when the body changes (form, growth, build, size).
    void updateSpeeds(const ModelData& m, const AnimBinding& bind, const AnimLibrary& lib, const int* clipIndex,
                      int form, float t, int build, float size);
    // One frame: the behavior decides, the animator follows (new clips crossfade in, and a
    // finished one-shot tells the behavior to move on). Returns the events crossed.
    int update(const Dragon& d, bool night, float moveScale, float dt, const AnimLibrary& lib, const int* clipIndex,
               u8* events, int maxEvents);
};

}  // namespace ec
