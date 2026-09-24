#include "core/den_actor.hpp"

#include "core/rig.hpp"

namespace ec {

bool resolveClips(const AnimLibrary& lib, int out[static_cast<int>(ClipId::Count)]) {
    bool all = true;
    for (int i = 0; i < static_cast<int>(ClipId::Count); ++i) {
        out[i] = lib.find(clipName(static_cast<ClipId>(i)));
        all &= out[i] >= 0;
    }
    return all;
}

float locomotionSpeed(const ModelData& m, const AnimBinding& bind, const AnimClip& clip, float t, int build) {
    const int feet[4] = {m.skel.find("hand_L"), m.skel.find("hand_R"), m.skel.find("foot_L"), m.skel.find("foot_R")};
    for (int f : feet)
        if (f < 0) return clip.speed;
    BonePose idle[kMaxBones], pose[kMaxBones];
    idlePose(m, t, build, idle);
    Quat delta[kMaxBones];
    float root[2];
    Mat34 poseMat[kMaxBones], skin[kMaxBones];
    constexpr int kSteps = 48;
    const float dt = clip.duration() / kSteps;
    float prev[4] = {}, sum = 0;
    for (int k = 0; k <= kSteps; ++k) {
        sampleClip(clip, bind, m.skel.count, k * dt, delta, root);
        for (int i = 0; i < m.skel.count; ++i) pose[i] = idle[i];
        applyDeltas(pose, delta, m.skel.count);
        evaluatePose(m.skel, pose, poseMat, skin);
        // The lowest foot carries the body: its backward (+Y) speed is the ground speed.
        int low = 0;
        float y[4];
        for (int f = 0; f < 4; ++f) {
            const Vec3 p = poseMat[feet[f]].translation();
            y[f] = p.y;
            if (p.z < poseMat[feet[low]].translation().z) low = f;
        }
        if (k > 0) sum += (y[low] - prev[low]) / dt;
        for (int f = 0; f < 4; ++f) prev[f] = y[f];
    }
    const float speed = sum / kSteps;
    return speed > 0 ? speed : clip.speed;
}

void DenActor::reset(const DenLayout& den, u32 seed) {
    behavior.reset(den, seed);
    anim = Animator{};
    playedSerial = 0xFFFF;
    speedForm = -1;
}

void DenActor::updateSpeeds(const ModelData& m, const AnimBinding& bind, const AnimLibrary& lib, const int* clipIndex,
                            int form, float t, int build, float size) {
    if (form == speedForm && t - speedT < 0.01f && speedT - t < 0.01f) return;
    const int walk = clipIndex[static_cast<int>(ClipId::Walk)], trot = clipIndex[static_cast<int>(ClipId::Trot)];
    if (walk < 0 || trot < 0) return;
    behavior.walkSpeed = locomotionSpeed(m, bind, lib.clips[walk], t, build) * size;
    behavior.trotSpeed = locomotionSpeed(m, bind, lib.clips[trot], t, build) * size;
    speedForm = form;
    speedT = t;
}

int DenActor::update(const Dragon& d, bool night, float moveScale, float dt, const AnimLibrary& lib,
                     const int* clipIndex, u8* events, int maxEvents) {
    behavior.clipDone = playedSerial == behavior.clipSerial && anim.finished(lib);
    behavior.update(d, night, moveScale, dt);
    if (behavior.clipSerial != playedSerial) {
        const int index = clipIndex[static_cast<int>(behavior.clip)];
        if (index >= 0) anim.play(index, behavior.blend, true);
        playedSerial = behavior.clipSerial;
    }
    return anim.update(lib, dt, events, maxEvents);
}

}  // namespace ec
