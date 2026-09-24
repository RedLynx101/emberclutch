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

// The eyelids (the model's "eyes" bone is squashed vertically to shut the eyes): a blink
// every few seconds (now and then a double one), eased toward how shut the activity wants
// them (DenBehavior::eyesClosed). No blinking while they are mostly shut already.
constexpr float kBlinkSquash = 0.9f;  // shut eyes lose this much of their height (dragon_model BLINK_SQUASH)
struct Eyelids {
    float shut = 0;     // 0 open .. 1 shut, this frame
    float level = 0;    // the activity's level, eased
    float next = 2.5f;  // seconds to the next blink
    float blink = -1;   // seconds into the current blink (< 0: none)
    Rng rng{7};
    void update(float target, float dt);
};

struct DenActor {
    DenBehavior behavior;
    Animator anim;
    Eyelids eyes;
    // Hands-on care (WP7), set by the scene each frame: where it looks instead of at the
    // player (armature space; food held out, a hand petting), and extra jaw opening.
    Vec3 gazeLocal;
    float gazeWeight = 0;  // 0..1
    float jawOpen = 0;     // 0..1
    u16 playedSerial = 0xFFFF;
    int speedForm = -1;  // the body the walking speeds were measured on
    float speedT = -1;
    float look = 0;      // smoothed look-at-the-player weight

    void reset(const DenLayout& den, u32 seed, int spot = 0);  // spot: its bed and sulking spot
    // Re-measures walk/trot speeds when the body changes (form, growth, build, size).
    void updateSpeeds(const ModelData& m, const AnimBinding& bind, const AnimLibrary& lib, const int* clipIndex,
                      int form, float t, int build, float size);
    // One frame: the behavior decides, the animator follows (new clips crossfade in, and a
    // finished one-shot tells the behavior to move on). Returns the events crossed.
    int update(const Dragon& d, bool night, float moveScale, float dt, const AnimLibrary& lib, const int* clipIndex,
               u8* events, int maxEvents);
};

}  // namespace ec
