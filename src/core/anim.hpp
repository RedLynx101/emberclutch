// Dragon animation (docs/tech/architecture.md section 4, "Animation"): clips from
// romfs/anims/dragon.eca (written by tools/anim/build_anims.py), sampling, crossfades and
// event markers. Clips hold rotation deltas in armature axes; binding a clip set to a
// skeleton converts them into each bone's local frame, so one set serves both body forms.
#pragma once

#include <cstddef>
#include <vector>

#include "core/math3d.hpp"
#include "core/skeleton.hpp"

namespace ec {

// Event markers (tools/anim/eca.py EVENTS).
enum AnimEvent : u8 {
    kAnimEventNone,
    kAnimFootstep,
    kAnimChomp,
    kAnimSwallow,
    kAnimFlap,
    kAnimYawn,
    kAnimThump,
    kAnimLand,
    kAnimSniff,
    kAnimShake,
    kAnimPurr,
    kAnimCall,     // a happy call: a trill from the young, a rumble from grown-ups
    kAnimWhimper,
    kAnimSqueak,
    kAnimSneeze,
};

struct AnimClip {
    char name[16] = {};
    float fps = 30;
    u16 frames = 0;
    bool loop = false;
    float speed = 0;                   // locomotion speed in adult units per second (0 = in place)
    u32 first[kMaxBones] = {};         // per library bone: index of its first key in `keys`
    u16 keyCount[kMaxBones] = {};      // 0 = no rotation, 1 = constant, frames = animated
    std::vector<Quat> keys;            // armature-space deltas
    std::vector<float> root;           // frames x (forward, up) in adult units; empty if none
    struct Event {
        u16 frame;
        u8 id;
    };
    std::vector<Event> events;

    // Seconds: a loop's period, or the time of a one-shot's last frame.
    float duration() const { return frames == 0 ? 0 : (loop ? frames : frames - 1) / fps; }
};

struct AnimLibrary {
    u16 boneCount = 0;
    char boneName[kMaxBones][16] = {};
    std::vector<AnimClip> clips;

    int find(const char* name) const;  // clip index or -1
};

bool loadAnims(const u8* data, std::size_t size, AnimLibrary& out);

// A clip set bound to one skeleton (one per body form).
struct AnimBinding {
    s8 libBone[kMaxBones] = {};  // skeleton bone -> library bone (-1: no track)
    Quat rest[kMaxBones];        // the bone's rest rotation in armature space
};

void bindAnims(const AnimLibrary& lib, const Skeleton& skel, AnimBinding& out);

// Samples a clip at time t in seconds (loops wrap, one-shots hold their last frame):
// a bone-local rotation delta per skeleton bone, and the root offset (forward, up).
void sampleClip(const AnimClip& clip, const AnimBinding& bind, int boneCount, float t, Quat* delta,
                float* root);

// Plays one clip at a time with crossfades and reports the event markers it crosses.
struct Animator {
    int clip = -1;
    float time = 0;
    int prev = -1;  // the clip fading out
    float prevTime = 0;
    float fade = 1;  // crossfade progress, 1 = finished
    float fadeLength = 0.25f;

    // Starts a clip, crossfading from the current one. Asking for the clip that is already
    // playing does nothing unless restart is set.
    void play(int clipIndex, float crossfade = 0.25f, bool restart = false);
    // Advances time; writes the events the current clip crossed (up to max) and returns
    // how many.
    int update(const AnimLibrary& lib, float dt, u8* events, int maxEvents);
    // A one-shot clip reached its last frame (loops never finish).
    bool finished(const AnimLibrary& lib) const;
    // The blended deltas and root offset.
    void sample(const AnimLibrary& lib, const AnimBinding& bind, int boneCount, Quat* delta, float* root) const;
};

// Applies deltas after the idle pose, in each bone's local frame: rot = rot * delta.
void applyDeltas(BonePose* pose, const Quat* delta, int count);

// Turns the neck and head toward `target` (armature space) on top of the pose, within
// comfortable limits (yaw 55 degrees, pitch -30..25); weight 0..1 fades it in and out.
void applyLookAt(const Skeleton& skel, const AnimBinding& bind, BonePose* pose, Vec3 target, float weight);

}  // namespace ec
