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
#include "core/kinds.hpp"
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
        CHECK(walk[f] > 0.05f && trot[f] > walk[f] * 1.2f);  // (run 21 take 4: measured by each foot's sweep)
        CHECK(young > 0 && young < walk[f]);  // shorter legs, shorter strides
    }
    CHECK(walk[kFormHatchling] < walk[kFormGrown] && trot[kFormHatchling] < trot[kFormGrown]);
    // A baby's own toddle (Noah, run 13: the grown walk was far too slow on it): at least twice
    // the grown walk's pace on the same body, short of its scamper.
    {
        const ModelData& m = form(kFormHatchling);
        AnimBinding bind;
        bindAnims(lib, m.skel, bind);
        const float toddle = locomotionSpeed(m, bind, lib.clips[lib.find("walk_h")], 1.0f, kBuildNeutral);
        std::printf("  hatchling: toddles at %.2f units/s (the grown walk %.2f)\n", toddle, walk[kFormHatchling]);
        const int sc = lib.find("scamper");
        CHECK(sc >= 0 && toddle > walk[kFormHatchling] * 2.0f &&
              toddle < locomotionSpeed(m, bind, lib.clips[sc], 1.0f, kBuildNeutral));
    }
    // Running (WP12c): the hatchling's scamper and the grown dragon's gallop outrun their trots.
    for (int f = 0; f < kFormCount; ++f) {
        const ModelData& m = form(f);
        AnimBinding bind;
        bindAnims(lib, m.skel, bind);
        const int run = lib.find(f == kFormHatchling ? "scamper" : "gallop");
        CHECK(run >= 0);
        if (run < 0) continue;
        const float speed = locomotionSpeed(m, bind, lib.clips[run], 1.0f, kBuildNeutral);
        std::printf("  %s: runs at %.2f units/s\n", f == kFormHatchling ? "hatchling" : "grown", speed);
        CHECK(speed > trot[f] * 1.3f && speed < trot[f] * 3.0f);
    }
}

// Every kind's grown walk and trot measured on its own body with its plan's clips (run 21 take 4:
// the Crestwing's walk measured 0, fell back to the slowest pace and crawled). The per-foot motion
// is printed for a kind that fails, to see why.
TEST(every_kind_walks_at_a_measured_speed) {
    for (int k = 0; k < kindCount(); ++k) {
        const KindInfo& kind = kindInfo(k);
        const PlanInfo& plan = planInfo(kind.plan);
        const std::vector<u8> mb = readAll((std::string("../romfs/dragons/") + kind.name + "/grown.ecm").c_str());
        const std::vector<u8> ab = readAll((std::string("../romfs/anims/") + plan.name + ".eca").c_str());
        static ModelData m;
        static AnimLibrary lib;
        m = ModelData{};
        lib = AnimLibrary{};
        if (mb.empty() || ab.empty() || !loadModel(mb.data(), mb.size(), m) || !loadAnims(ab.data(), ab.size(), lib)) {
            std::printf("  %s: files missing\n", kind.name);
            CHECK(false);
            continue;
        }
        for (int i = 0; i < 4; ++i) m.contacts[i] = static_cast<s8>(m.skel.find(plan.contacts[i]));
        AnimBinding bind;
        bindAnims(lib, m.skel, bind);
        const int wi = lib.find("walk"), ti = lib.find("trot");
        const float walk = wi >= 0 ? locomotionSpeed(m, bind, lib.clips[wi], 1.0f, kBuildNeutral) : 0.0f;
        const float trot = ti >= 0 ? locomotionSpeed(m, bind, lib.clips[ti], 1.0f, kBuildNeutral) : 0.0f;
        std::printf("  %-12s (%s): walk %.2f, trot %.2f units/s\n", kind.name, plan.name, walk, trot);
        CHECK(walk > 0.2f && trot > walk);
    }
}

TEST(one_bone_chain_matches_the_whole_pose) {
    // The look-at evaluates only the head's chain (WP11d): it must give what evaluatePose gives.
    const AnimLibrary& lib = anims();
    for (int f = 0; f < kFormCount; ++f) {
        const ModelData& m = form(f);
        AnimBinding bind;
        bindAnims(lib, m.skel, bind);
        BonePose pose[kMaxBones];
        idlePose(m, 0.6f, kBuildNeutral, pose);
        Quat delta[kMaxBones];
        float root[2];
        sampleClip(lib.clips[lib.find("tail_wag")], bind, m.skel.count, 0.4f, delta, root);
        applyDeltas(pose, delta, m.skel.count);
        Mat34 poseMat[kMaxBones], skin[kMaxBones];
        evaluatePose(m.skel, pose, poseMat, skin);
        float worst = 0;
        for (int b = 0; b < m.skel.count; ++b) {
            const Mat34 one = bonePoseMatrix(m.skel, pose, b);
            for (int r = 0; r < 3; ++r)
                for (int c = 0; c < 4; ++c) worst = std::fmax(worst, std::fabs(one.m[r][c] - poseMat[b].m[r][c]));
        }
        CHECK(worst < 1e-5f);
    }
}

TEST(look_at_turns_the_head_within_limits) {
    const ModelData& m = form(kFormGrown);
    AnimBinding bind;
    bindAnims(anims(), m.skel, bind);
    const int head = m.skel.find("head");
    BonePose idle[kMaxBones], pose[kMaxBones];
    idlePose(m, 1.0f, kBuildNeutral, idle);
    Mat34 poseMat[kMaxBones], skin[kMaxBones];
    evaluatePose(m.skel, idle, poseMat, skin);
    const Vec3 headPos = poseMat[head].translation();
    const Vec3 rest = boneDir(m, idle, head);
    auto yawOf = [](Vec3 d) { return std::atan2(d.x, -d.y); };
    auto lookAt = [&](Vec3 target, float weight) {
        for (int i = 0; i < m.skel.count; ++i) pose[i] = idle[i];
        applyLookAt(m.skel, bind, pose, target, weight);
        return boneDir(m, pose, head);
    };
    // To its left (+X) and ahead: the head turns left.
    CHECK(yawOf(lookAt(headPos + Vec3{3, -3, 0}, 1.0f)) > yawOf(rest) + 0.3f);
    // Up high in front: the head tips up.
    CHECK(lookAt(headPos + Vec3{0, -2, 3}, 1.0f).z > rest.z + 0.2f);
    // Weight 0 changes nothing; half weight turns about half as far.
    CHECK(std::fabs(dot(lookAt(headPos + Vec3{3, -3, 0}, 0.0f), rest) - 1.0f) < 1e-5f);
    const float full = yawOf(lookAt(headPos + Vec3{3, -3, 0}, 1.0f)) - yawOf(rest);
    const float half = yawOf(lookAt(headPos + Vec3{3, -3, 0}, 0.5f)) - yawOf(rest);
    CHECK(std::fabs(half - full * 0.5f) < 0.1f);
    // Right behind it: the turn stops at the limit instead of wrapping the neck around.
    const float behind = yawOf(lookAt(headPos + Vec3{0.3f, 5, 0}, 1.0f)) - yawOf(rest);
    CHECK(std::fabs(behind) < 60.0f * 3.14159f / 180.0f);
}

// The floor contact (the lowest body point, which the renderer puts on the floor) stays
// nearly level through the walk and trot cycles: a stray vertex swinging with one leg once
// lifted the adult every stride (Noah's "limp", 2026-09-24).
TEST(walking_keeps_the_body_level) {
    const AnimLibrary& lib = anims();
    for (int f = 0; f < kFormCount; ++f) {
        const ModelData& m = form(f);
        const MeshData* body = m.findMesh(kMeshBody, kGroupBody, 0);
        CHECK(body != nullptr);
        if (!body) continue;
        AnimBinding bind;
        bindAnims(lib, m.skel, bind);
        BonePose idle[kMaxBones];
        idlePose(m, 1.0f, kBuildNeutral, idle);
        for (const char* name : {"walk", "trot"}) {
            const AnimClip& clip = lib.clips[lib.find(name)];
            float lo = 1e9f, hi = -1e9f;
            for (int k = 0; k < 30; ++k) {
                Quat delta[kMaxBones];
                float root[2];
                sampleClip(clip, bind, m.skel.count, clip.duration() * k / 30.0f, delta, root);
                BonePose pose[kMaxBones];
                for (int i = 0; i < m.skel.count; ++i) pose[i] = idle[i];
                applyDeltas(pose, delta, m.skel.count);
                Mat34 poseMat[kMaxBones], skin[kMaxBones];
                evaluatePose(m.skel, pose, poseMat, skin);
                float low = 1e9f;
                for (int v = 0; v < body->vertexCount; ++v) low = std::fmin(low, skinPoint(*body, v, body->pos[v], skin).z);
                lo = std::fmin(lo, low);
                hi = std::fmax(hi, low);
            }
            std::printf("  %s %s: floor contact moves %.3f\n", f == kFormHatchling ? "hatchling" : "grown", name, hi - lo);
            CHECK(hi - lo < 0.08f);
        }
    }
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
    RUN(every_kind_walks_at_a_measured_speed);
    RUN(one_bone_chain_matches_the_whole_pose);
    RUN(look_at_turns_the_head_within_limits);
    RUN(walking_keeps_the_body_level);
    RUN(anim_loader_rejects_bad_files);
}
