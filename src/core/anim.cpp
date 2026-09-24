#include "core/anim.hpp"

#include <cmath>
#include <cstring>

#include "core/byte_reader.hpp"

namespace ec {
namespace {

float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

}  // namespace

int AnimLibrary::find(const char* name) const {
    for (std::size_t i = 0; i < clips.size(); ++i)
        if (std::strncmp(clips[i].name, name, 15) == 0) return static_cast<int>(i);
    return -1;
}

bool loadAnims(const u8* data, std::size_t size, AnimLibrary& out) {
    ByteReader c(data, size);
    char magic[4];
    c.bytes(magic, 4);
    if (!c.ok() || std::memcmp(magic, "ECA1", 4) != 0 || c.u16v() != 1) return false;
    const u16 bones = c.u16v(), clipCount = c.u16v();
    if (bones == 0 || bones > kMaxBones) return false;
    out.boneCount = bones;
    for (int i = 0; i < bones; ++i) {
        c.bytes(out.boneName[i], 16);
        out.boneName[i][15] = '\0';
    }
    out.clips.clear();
    out.clips.resize(clipCount);
    for (AnimClip& a : out.clips) {
        c.bytes(a.name, 16);
        a.name[15] = '\0';
        a.fps = c.f32();
        a.frames = c.u16v();
        const u8 flags = c.u8v(), eventCount = c.u8v();
        a.speed = c.f32();
        if (!c.ok() || a.frames == 0 || !(a.fps > 0.0f) || !(a.speed >= 0.0f)) return false;
        a.loop = flags & 1;
        a.keys.clear();
        for (int b = 0; b < bones; ++b) {
            const u8 mode = c.u8v();
            if (mode > 2) return false;
            const int n = mode == 0 ? 0 : (mode == 1 ? 1 : a.frames);
            a.first[b] = static_cast<u32>(a.keys.size());
            a.keyCount[b] = static_cast<u16>(n);
            for (int k = 0; k < n; ++k) {
                const float w = c.s16v() / 32767.0f, x = c.s16v() / 32767.0f;
                const float y = c.s16v() / 32767.0f, z = c.s16v() / 32767.0f;
                a.keys.push_back(normalize(Quat{x, y, z, w}));
            }
            if (!c.ok()) return false;
        }
        a.root.clear();
        if (flags & 2) {
            a.root.resize(std::size_t(a.frames) * 2);
            for (float& v : a.root) v = c.f32();
        }
        a.events.resize(eventCount);
        for (AnimClip::Event& e : a.events) {
            e.frame = c.u16v();
            e.id = c.u8v();
            c.skip(1);
            if (e.frame >= a.frames) return false;
        }
        if (!c.ok()) return false;
    }
    return c.ok();
}

void bindAnims(const AnimLibrary& lib, const Skeleton& skel, AnimBinding& out) {
    for (int i = 0; i < skel.count; ++i) {
        out.libBone[i] = -1;
        for (int b = 0; b < lib.boneCount; ++b)
            if (std::strncmp(lib.boneName[b], skel.name[i], 16) == 0) out.libBone[i] = static_cast<s8>(b);
        out.rest[i] = quatFromRotation(skel.rest[i]);
    }
}

void sampleClip(const AnimClip& clip, const AnimBinding& bind, int boneCount, float t, Quat* delta,
                float* root) {
    const int n = clip.frames;
    float f = t * clip.fps;
    int i0, i1;
    float frac;
    if (clip.loop) {
        f = std::fmod(f, static_cast<float>(n));
        if (f < 0) f += n;
        i0 = static_cast<int>(f);
        if (i0 >= n) i0 = n - 1;
        frac = f - i0;
        i1 = (i0 + 1) % n;
    } else {
        f = f < 0 ? 0 : (f > n - 1 ? static_cast<float>(n - 1) : f);
        i0 = static_cast<int>(f);
        frac = f - i0;
        i1 = i0 + 1 < n ? i0 + 1 : n - 1;
    }
    for (int i = 0; i < boneCount; ++i) {
        const int b = bind.libBone[i];
        if (b < 0 || clip.keyCount[b] == 0) {
            delta[i] = Quat{};
            continue;
        }
        const Quat* k = &clip.keys[clip.first[b]];
        const Quat q = clip.keyCount[b] == 1 ? k[0] : nlerp(k[i0], k[i1], frac);
        // Armature-axis delta -> the bone's local frame.
        delta[i] = mul(mul(conjugate(bind.rest[i]), q), bind.rest[i]);
    }
    if (clip.root.empty()) {
        root[0] = root[1] = 0;
    } else {
        root[0] = clip.root[i0 * 2] + (clip.root[i1 * 2] - clip.root[i0 * 2]) * frac;
        root[1] = clip.root[i0 * 2 + 1] + (clip.root[i1 * 2 + 1] - clip.root[i0 * 2 + 1]) * frac;
    }
}

void Animator::play(int clipIndex, float crossfade, bool restart) {
    if (clipIndex == clip && !restart) return;
    if (clip >= 0 && crossfade > 0) {
        prev = clip;
        prevTime = time;
        fade = 0;
        fadeLength = crossfade;
    } else {
        prev = -1;
        fade = 1;
    }
    clip = clipIndex;
    time = 0;
}

int Animator::update(const AnimLibrary& lib, float dt, u8* events, int maxEvents) {
    if (fade < 1) {
        prevTime += dt;
        fade += dt / fadeLength;
        if (fade >= 1) {
            fade = 1;
            prev = -1;
        }
    }
    if (clip < 0 || clip >= static_cast<int>(lib.clips.size())) return 0;
    const AnimClip& a = lib.clips[clip];
    const float t0 = time, t1 = time + dt * rate;
    time = t1;
    int count = 0;
    const float period = a.duration();
    for (const AnimClip::Event& e : a.events) {
        const float te = e.frame / a.fps;
        if (a.loop && period > 0) {
            // Every repeat of the marker inside [t0, t1).
            for (float tt = std::floor(t0 / period) * period + te; tt < t1; tt += period)
                if (tt >= t0 && count < maxEvents) events[count++] = e.id;
        } else if (te >= t0 && te < t1 && count < maxEvents) {
            events[count++] = e.id;
        }
    }
    return count;
}

bool Animator::finished(const AnimLibrary& lib) const {
    if (clip < 0 || clip >= static_cast<int>(lib.clips.size())) return true;
    const AnimClip& a = lib.clips[clip];
    return !a.loop && time >= a.duration();
}

void Animator::sample(const AnimLibrary& lib, const AnimBinding& bind, int boneCount, Quat* delta,
                      float* root) const {
    if (clip < 0 || clip >= static_cast<int>(lib.clips.size())) {
        for (int i = 0; i < boneCount; ++i) delta[i] = Quat{};
        root[0] = root[1] = 0;
        return;
    }
    sampleClip(lib.clips[clip], bind, boneCount, time, delta, root);
    if (prev >= 0 && prev < static_cast<int>(lib.clips.size()) && fade < 1) {
        Quat from[kMaxBones];
        float fromRoot[2];
        sampleClip(lib.clips[prev], bind, boneCount, prevTime, from, fromRoot);
        const float w = smooth(fade);
        for (int i = 0; i < boneCount; ++i) delta[i] = nlerp(from[i], delta[i], w);
        root[0] = fromRoot[0] + (root[0] - fromRoot[0]) * w;
        root[1] = fromRoot[1] + (root[1] - fromRoot[1]) * w;
    }
}

void applyDeltas(BonePose* pose, const Quat* delta, int count) {
    for (int i = 0; i < count; ++i) pose[i].rot = mul(pose[i].rot, delta[i]);
}

void applyLookAt(const Skeleton& skel, const AnimBinding& bind, BonePose* pose, Vec3 target, float weight) {
    constexpr float kDeg = 3.14159265f / 180.0f;
    const int bones[3] = {skel.find("neck2"), skel.find("neck3"), skel.find("head")};
    constexpr float kShare[3] = {0.2f, 0.3f, 0.5f};  // the head turns most, the neck follows
    if (weight <= 0.001f || bones[0] < 0 || bones[1] < 0 || bones[2] < 0) return;
    const Mat34 h = bonePoseMatrix(skel, pose, bones[2]);  // the head's chain only, not every bone (WP11d)
    const Vec3 facing = normalize(Vec3{h.m[0][1], h.m[1][1], h.m[2][1]});  // along the head bone
    // Aimed from a little behind the head: a target right at the snout (food held to the mouth,
    // a hand under the chin) turns it gently. Aimed from the head itself, a pixel either side
    // of it swung the head between its limits (Noah, run 3).
    constexpr float kPivotBack = 0.5f;
    const Vec3 want = normalize(target - h.translation() + facing * kPivotBack);
    float yaw = std::atan2(want.x, -want.y) - std::atan2(facing.x, -facing.y);
    while (yaw > 3.14159265f) yaw -= 6.2831853f;
    while (yaw < -3.14159265f) yaw += 6.2831853f;
    auto clampf = [](float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); };
    float pitch = std::asin(clampf(want.z, -1, 1)) - std::asin(clampf(facing.z, -1, 1));
    yaw = clampf(yaw, -55 * kDeg, 55 * kDeg) * weight;
    pitch = clampf(pitch, -30 * kDeg, 25 * kDeg) * weight;
    for (int k = 0; k < 3; ++k) {
        const int b = bones[k];
        const Quat q = quatFromPitchYawRoll(pitch * kShare[k], yaw * kShare[k], 0);
        pose[b].rot = mul(pose[b].rot, mul(mul(conjugate(bind.rest[b]), q), bind.rest[b]));
    }
}

}  // namespace ec
