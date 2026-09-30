#include "core/den_actor.hpp"

#include <cmath>
#include <cstdio>

#include "core/rig.hpp"

namespace ec {

constexpr float kWalkOnlyTrot = 1.45f, kWalkOnlyRun = 1.9f;  // (a walk-only body's trot and run: its walk this much quicker, D130)
namespace {

// Hatchlings walked 30% faster than their steps (Noah, 2026-09-24); since run 13 their own
// quick toddle (walk_h) sets the pace with the feet planted, so no extra.
constexpr float kBabyHaste = 1.0f;

}  // namespace

bool resolveClips(const AnimLibrary& lib, int form, int out[static_cast<int>(ClipId::Count)]) {
    bool all = true;
    for (int i = 0; i < static_cast<int>(ClipId::Count); ++i) {
        const char* name = clipName(static_cast<ClipId>(i));
        out[i] = -1;
        if (form == kFormHatchling) {
            char baby[16];
            std::snprintf(baby, sizeof(baby), "%s_h", name);
            out[i] = lib.find(baby);
        }
        if (out[i] < 0) out[i] = lib.find(name);
        all &= out[i] >= 0;
    }
    return all;
}

float locomotionSpeed(const ModelData& m, const AnimBinding& bind, const AnimClip& clip, float t, int build) {
    const int feet[4] = {m.contacts[0], m.contacts[1], m.contacts[2], m.contacts[3]};  // the plan's (core/kinds)
    for (int f : feet)
        if (f < 0) return clip.speed;
    BonePose idle[kMaxBones], pose[kMaxBones];
    idlePose(m, t, build, idle);
    Quat delta[kMaxBones];
    float root[2];
    Mat34 poseMat[kMaxBones], skin[kMaxBones];
    // Each foot sweeps back along the ground while it carries the body, at the ground speed, and
    // swings forward faster through the air: its backward sweep over the time it spends sweeping is
    // the ground speed. Averaged over the feet by how far each sweeps. (Run 21 take 4: taking the
    // lowest foot's speed read a long-legged plan's swinging foot, lower than its planted ones,
    // and measured the Crestwing's trot at nothing and its walk as a crawl.)
    constexpr int kSteps = 48;
    const float dt = clip.duration() / kSteps;
    float prevY[4] = {}, back[4] = {}, time[4] = {};
    for (int k = 0; k <= kSteps; ++k) {
        sampleClip(clip, bind, m.skel.count, k * dt, delta, root);
        for (int i = 0; i < m.skel.count; ++i) pose[i] = idle[i];
        applyDeltas(pose, delta, m.skel.count);
        evaluatePose(m.skel, pose, poseMat, skin);
        for (int f = 0; f < 4; ++f) {
            const float y = poseMat[feet[f]].translation().y;
            if (k > 0 && y > prevY[f]) {
                back[f] += y - prevY[f];
                time[f] += dt;
            }
            prevY[f] = y;
        }
    }
    float swept = 0, weighted = 0;
    for (int f = 0; f < 4; ++f)
        if (time[f] > 0) {
            weighted += back[f] * (back[f] / time[f]);
            swept += back[f];
        }
    const float speed = swept > 0 ? weighted / swept : 0.0f;
    return speed > 0 ? speed : clip.speed;
}

void Eyelids::update(float target, float dt) {
    const float rate = target > level ? 1.6f : 3.0f;  // drifting off is slow, waking quicker
    const float step = target - level;
    level += step > rate * dt ? rate * dt : (step < -rate * dt ? -rate * dt : step);
    constexpr float kClose = 0.06f, kHold = 0.05f, kOpen = 0.11f;
    float b = 0;
    if (blink >= 0) {
        blink += dt;
        if (blink < kClose) {
            b = blink / kClose;
        } else if (blink < kClose + kHold) {
            b = 1;
        } else if (blink < kClose + kHold + kOpen) {
            b = 1 - (blink - kClose - kHold) / kOpen;
        } else {
            blink = -1;
            next = rng.chance(1, 6) ? 0.15f : 2.0f + rng.below(4000) * 0.001f;
        }
    } else if (level < 0.8f && (next -= dt) <= 0) {
        blink = 0;
    }
    shut = b > level ? b : level;
}

void DenActor::reset(const DenLayout& den, u32 seed, int spot) {
    behavior.reset(den, seed, spot);
    anim = Animator{};
    eyes = Eyelids{};
    eyes.rng = Rng(seed * 2654435761u + 17);
    playedSerial = 0xFFFF;
    speedForm = -1;
}

void DenActor::updateSpeeds(const ModelData& m, const AnimBinding& bind, const AnimLibrary& lib, const int* clipIndex,
                            int form, float t, int build, float size, bool baby) {
    if (form == speedForm && t - speedT < 0.01f && speedT - t < 0.01f) return;
    // A new body (grown up, another look): what it's doing is played again from its own clips
    // (run 21: a Crestwing grown in its sleep kept the hatchling's sleep, neck down to the floor).
    if (form != speedForm) playedSerial = 0xFFFF;
    const int walk = clipIndex[static_cast<int>(ClipId::Walk)], trot = clipIndex[static_cast<int>(ClipId::Trot)];
    if (walk < 0 || trot < 0) return;
    behavior.walkSpeed = locomotionSpeed(m, bind, lib.clips[walk], t, build) * size;
    behavior.trotSpeed = locomotionSpeed(m, bind, lib.clips[trot], t, build) * size;
    behavior.baby = baby;  // the hatchling's body scampers, the grown one gallops
    const int run = clipIndex[static_cast<int>(behavior.baby ? ClipId::Scamper : ClipId::Gallop)];
    if (run >= 0) behavior.runSpeed = locomotionSpeed(m, bind, lib.clips[run], t, build) * size;
    if (walkOnly) {  // (its walk, quicker: D130)
        behavior.trotSpeed = behavior.walkSpeed * kWalkOnlyTrot;
        behavior.runSpeed = behavior.walkSpeed * kWalkOnlyRun;
    }
    // How far its snout reaches in front of it, standing: its tip is about as far past the
    // snout's joint as that is past the head's.
    const int head = m.skel.find("head"), snout = m.skel.find("snout");
    if (head >= 0 && snout >= 0) {
        BonePose idle[kMaxBones];
        idlePose(m, t, build, idle);
        Mat34 poseMat[kMaxBones], skin[kMaxBones];
        evaluatePose(m.skel, idle, poseMat, skin);
        const Vec3 h = poseMat[head].translation(), s = poseMat[snout].translation();
        behavior.reach = std::fmax(0.5f, -(s.y + 0.8f * (s.y - h.y)));
    }
    speedForm = form;
    speedT = t;
}

int DenActor::update(const Dragon& d, bool night, float moveScale, float dt, const AnimLibrary& lib,
                     const int* clipIndex, u8* events, int maxEvents) {
    behavior.clipDone = playedSerial == behavior.clipSerial && anim.finished(lib);
    // Smaller legs step faster (stride frequency goes about as 1 / sqrt(size)): a hatchling
    // scampers instead of creeping. Up to twice as quick.
    behavior.gait = moveScale < 1.0f ? std::fmin(2.0f, 1.0f / std::sqrt(std::fmax(moveScale, 0.05f))) : 1.0f;
    behavior.haste = d.stage == Stage::Hatchling ? kBabyHaste : 1.0f;
    behavior.update(d, night, moveScale, dt);
    lift = behavior.air;  // off the floor: flying, or a baby's flutter-hop (D85)
    const float k = dt * 3.0f < 1.0f ? dt * 3.0f : 1.0f;
    look += (behavior.lookWeight() - look) * k;
    eyes.update(behavior.eyesClosed(), dt);
    const ClipId c = behavior.clip;
    const bool quickWalk = walkOnly && (c == ClipId::Trot || c == ClipId::Gallop || c == ClipId::Scamper);
    if (behavior.clipSerial != playedSerial) {
        const int index = clipIndex[static_cast<int>(quickWalk ? ClipId::Walk : c)];
        if (index >= 0) anim.play(index, behavior.blend, !quickWalk);
        playedSerial = behavior.clipSerial;
    }
    anim.rate = c == ClipId::Walk || c == ClipId::Trot || c == ClipId::Carry || c == ClipId::Scamper ||
                        c == ClipId::Gallop
                    ? behavior.gait * (quickWalk ? (c == ClipId::Trot ? kWalkOnlyTrot : kWalkOnlyRun) : 1.0f)
                    : 1.0f;
    return anim.update(lib, dt, events, maxEvents);
}

}  // namespace ec
