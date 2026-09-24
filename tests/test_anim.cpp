// Animation tests: the clip file written by tools/anim/build_anims.py loads, binds to both body
// forms, follows the documented axis conventions, loops seamlessly, crossfades and reports
// its event markers.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/anim.hpp"
#include "core/den_actor.hpp"
#include "core/model.hpp"
#include "core/rig.hpp"

using namespace ec;

namespace {

std::vector<u8> readAll(const char* path) {
    std::vector<u8> data;
    FILE* f = std::fopen(path, "rb");
    if (!f) return data;
    std::fseek(f, 0, SEEK_END);
    data.resize(static_cast<std::size_t>(std::ftell(f)));
    std::fseek(f, 0, SEEK_SET);
    if (std::fread(data.data(), 1, data.size(), f) != data.size()) data.clear();
    std::fclose(f);
    return data;
}

const AnimLibrary& anims() {
    static AnimLibrary lib;
    static bool loaded = false;
    if (!loaded) {
        const std::vector<u8> bytes = readAll("../romfs/anims/dragon.eca");
        loaded = !bytes.empty() && loadAnims(bytes.data(), bytes.size(), lib);
    }
    return lib;
}

const ModelData& form(int f) {
    static ModelData m[kFormCount];
    static bool loaded[kFormCount] = {};
    if (!loaded[f]) {
        const std::string path = std::string("../romfs/models/") + (f == kFormHatchling ? "hatchling" : "grown") + ".ecm";
        const std::vector<u8> bytes = readAll(path.c_str());
        loaded[f] = !bytes.empty() && loadModel(bytes.data(), bytes.size(), m[f]);
    }
    return m[f];
}

// Armature-space direction of a bone (its local Y axis) in a pose.
Vec3 boneDir(const ModelData& m, const BonePose* pose, int bone) {
    Mat34 poseMat[kMaxBones], skin[kMaxBones];
    evaluatePose(m.skel, pose, poseMat, skin);
    return normalize(Vec3{poseMat[bone].m[0][1], poseMat[bone].m[1][1], poseMat[bone].m[2][1]});
}

TEST(anims_load_and_bind_to_both_forms) {
    const AnimLibrary& lib = anims();
    CHECK(lib.clips.size() >= 25);
    for (const char* name : {"idle", "walk", "trot", "sit", "sit_loop", "lie_down", "lie_loop", "sleep", "wake", "eat",
                             "pet_head", "pet_chin", "belly_rub", "shake", "hop", "pounce", "sulk", "sulk_loop",
                             "nuzzle", "greet"})
        CHECK(lib.find(name) >= 0);
    CHECK(lib.find("no_such_clip") == -1);
    for (int f = 0; f < kFormCount; ++f) {
        AnimBinding bind;
        bindAnims(lib, form(f).skel, bind);
        int mapped = 0;
        for (int i = 0; i < form(f).skel.count; ++i) mapped += bind.libBone[i] >= 0;
        CHECK(mapped == form(f).skel.count);  // every bone has a track slot
    }
}

TEST(anim_axes_follow_the_documented_conventions) {
    // yawn lifts the head (pitch +); eat lowers it to the floor; both on both forms.
    const AnimLibrary& lib = anims();
    for (int f = 0; f < kFormCount; ++f) {
        const ModelData& m = form(f);
        AnimBinding bind;
        bindAnims(lib, m.skel, bind);
        const int head = m.skel.find("head");
        BonePose idle[kMaxBones];
        idlePose(m, 1.0f, kBuildNeutral, idle);
        const Vec3 rest = boneDir(m, idle, head);
        Quat delta[kMaxBones];
        float root[2];
        BonePose pose[kMaxBones];

        sampleClip(lib.clips[lib.find("yawn")], bind, m.skel.count, 0.9f, delta, root);
        for (int i = 0; i < m.skel.count; ++i) pose[i] = idle[i];
        applyDeltas(pose, delta, m.skel.count);
        CHECK(boneDir(m, pose, head).z > rest.z + 0.1f);

        // Eating brings the head down to the food: its joint drops well below the idle height.
        Mat34 poseMat[kMaxBones], skin[kMaxBones];
        evaluatePose(m.skel, idle, poseMat, skin);
        const float idleHeight = poseMat[head].translation().z;
        sampleClip(lib.clips[lib.find("eat")], bind, m.skel.count, 0.0f, delta, root);
        for (int i = 0; i < m.skel.count; ++i) pose[i] = idle[i];
        applyDeltas(pose, delta, m.skel.count);
        evaluatePose(m.skel, pose, poseMat, skin);
        CHECK(poseMat[head].translation().z < idleHeight * 0.75f);
    }
}

TEST(loops_are_seamless_and_one_shots_hold) {
    const AnimLibrary& lib = anims();
    AnimBinding bind;
    bindAnims(lib, form(kFormGrown).skel, bind);
    const int n = form(kFormGrown).skel.count;
    Quat a[kMaxBones], b[kMaxBones];
    float ra[2], rb[2];
    for (const AnimClip& clip : lib.clips) {
        CHECK(clip.duration() > 0.2f);
        if (clip.loop) {
            // The frame just before the wrap sits next to frame 0 (no pop).
            sampleClip(clip, bind, n, clip.duration() - 0.001f, a, ra);
            sampleClip(clip, bind, n, 0.0f, b, rb);
            float worst = 1;
            for (int i = 0; i < n; ++i) worst = std::fmin(worst, std::fabs(dot(a[i], b[i])));
            CHECK(worst > 0.995f);
        } else {
            sampleClip(clip, bind, n, clip.duration(), a, ra);
            sampleClip(clip, bind, n, clip.duration() + 5.0f, b, rb);
            float worst = 1;
            for (int i = 0; i < n; ++i) worst = std::fmin(worst, std::fabs(dot(a[i], b[i])));
            CHECK(worst > 0.9999f);
        }
    }
    Animator anim;
    anim.play(lib.find("hop"));
    u8 ev[8];
    for (int k = 0; k < 40; ++k) anim.update(lib, 1.0f / 30, ev, 8);
    CHECK(anim.finished(lib));
    anim.play(lib.find("idle"));
    for (int k = 0; k < 200; ++k) anim.update(lib, 1.0f / 30, ev, 8);
    CHECK(!anim.finished(lib));
}

TEST(animator_crossfades_between_clips) {
    const AnimLibrary& lib = anims();
    AnimBinding bind;
    bindAnims(lib, form(kFormGrown).skel, bind);
    const int n = form(kFormGrown).skel.count;
    const int idle = lib.find("idle"), lie = lib.find("lie_loop");
    Quat out[kMaxBones], fromIdle[kMaxBones], toLie[kMaxBones];
    float root[2];
    Animator anim;
    anim.play(idle);
    u8 ev[8];
    anim.update(lib, 0.5f, ev, 8);
    sampleClip(lib.clips[idle], bind, n, 0.5f, fromIdle, root);
    anim.play(lie, 0.5f);
    anim.sample(lib, bind, n, out, root);  // fade 0: still the idle pose
    float worst = 1;
    for (int i = 0; i < n; ++i) worst = std::fmin(worst, std::fabs(dot(out[i], fromIdle[i])));
    CHECK(worst > 0.9999f);
    anim.update(lib, 0.6f, ev, 8);  // past the fade: all lie_loop
    anim.sample(lib, bind, n, out, root);
    sampleClip(lib.clips[lie], bind, n, 0.6f, toLie, root);
    worst = 1;
    for (int i = 0; i < n; ++i) worst = std::fmin(worst, std::fabs(dot(out[i], toLie[i])));
    CHECK(worst > 0.9999f);
    CHECK(anim.prev == -1 && anim.fade == 1.0f);
    anim.play(lie);  // asking again does not restart
    CHECK(anim.time > 0.5f);
}

TEST(animator_reports_event_markers) {
    const AnimLibrary& lib = anims();
    Animator anim;
    anim.play(lib.find("walk"));
    const float cycle = lib.clips[lib.find("walk")].duration();
    u8 ev[16];
    int steps = 0;
    const int frames = static_cast<int>(std::round(2.5f * cycle * 30));
    for (int k = 0; k < frames; ++k) {
        const int n = anim.update(lib, 1.0f / 30, ev, 16);
        for (int i = 0; i < n; ++i) steps += ev[i] == kAnimFootstep;
    }
    CHECK(steps == 10);  // four feet per cycle, 2.5 cycles
    // A big time step still reports every marker it crossed.
    Animator eat;
    eat.play(lib.find("eat"));
    const int n = eat.update(lib, lib.clips[lib.find("eat")].duration() * 2.0f, ev, 16);
    int chomps = 0;
    for (int i = 0; i < n; ++i) chomps += ev[i] == kAnimChomp;
    CHECK(chomps == 4);
}

TEST(locomotion_speeds_follow_the_body) {
    // The den walks each dragon at the speed its planted feet move (core/den_actor), so feet
    // never skate. Trots are faster than walks, and babies are slower than adults.
    const AnimLibrary& lib = anims();
    float walk[kFormCount], trot[kFormCount];
    for (int f = 0; f < kFormCount; ++f) {
        const ModelData& m = form(f);
        AnimBinding bind;
        bindAnims(lib, m.skel, bind);
        walk[f] = locomotionSpeed(m, bind, lib.clips[lib.find("walk")], 1.0f, kBuildNeutral);
        trot[f] = locomotionSpeed(m, bind, lib.clips[lib.find("trot")], 1.0f, kBuildNeutral);
        const float young = locomotionSpeed(m, bind, lib.clips[lib.find("walk")], 0.0f, kBuildNeutral);
        std::printf("  %s: walk %.2f (t=0: %.2f), trot %.2f units/s\n", f == kFormHatchling ? "hatchling" : "grown",
                    walk[f], young, trot[f]);
        CHECK(walk[f] > 0.05f && trot[f] > walk[f] * 1.5f);
        CHECK(young > 0 && young < walk[f]);  // shorter legs, shorter strides
    }
    CHECK(walk[kFormHatchling] < walk[kFormGrown] && trot[kFormHatchling] < trot[kFormGrown]);
}

TEST(anim_loader_rejects_bad_files) {
    const std::vector<u8> good = readAll("../romfs/anims/dragon.eca");
    AnimLibrary lib;
    CHECK(!loadAnims(good.data(), good.size() / 2, lib));  // truncated
    std::vector<u8> bad = good;
    bad[0] = 'X';
    CHECK(!loadAnims(bad.data(), bad.size(), lib));
}

}  // namespace

void runAnimTests() {
    RUN(anims_load_and_bind_to_both_forms);
    RUN(anim_axes_follow_the_documented_conventions);
    RUN(loops_are_seamless_and_one_shots_hold);
    RUN(animator_crossfades_between_clips);
    RUN(animator_reports_event_markers);
    RUN(locomotion_speeds_follow_the_body);
    RUN(anim_loader_rejects_bad_files);
}
