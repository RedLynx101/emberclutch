// Den behavior tests: the real clip file drives the real behavior loop (core/den_actor), as
// on the 3DS. Every activity is reachable, feelings and sleep take the dragon to the right
// spot, care interrupts the right things, and the dragon stays on the floor.
#include <cmath>
#include <cstdio>
#include <vector>

#include "check.hpp"
#include "core/den_actor.hpp"
#include "core/rig.hpp"

using namespace ec;

namespace {

std::vector<u8> fileBytes(const char* path) {
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

struct World {
    AnimLibrary lib;
    int clips[static_cast<int>(ClipId::Count)] = {};
    bool ok = false;
    World() {
        const std::vector<u8> bytes = fileBytes("../romfs/anims/dragon.eca");
        ok = !bytes.empty() && loadAnims(bytes.data(), bytes.size(), lib) && resolveClips(lib, kFormGrown, clips);
    }
};

const World& world() {
    static World w;
    return w;
}

Dragon contentDragon() {
    Dragon d;
    d.stage = Stage::Adult;
    d.needs = Needs{75, 75, 75, 75};
    d.personality = Personality::Curious;
    return d;
}

// Runs the actor for `seconds` at 30 fps; returns false if an activity check never passed.
template <typename Until>
bool run(DenActor& a, const Dragon& d, bool night, float seconds, Until until, float moveScale = 1.0f) {
    u8 ev[16];
    for (int f = 0; f < static_cast<int>(seconds * 30); ++f) {
        a.update(d, night, moveScale, 1.0f / 30, world().lib, world().clips, ev, 16);
        if (until(a.behavior)) return true;
    }
    return false;
}

float dist(Vec2 a, Vec2 b) { return std::hypot(a.x - b.x, a.y - b.y); }

TEST(behavior_clips_exist_in_the_clip_file) {
    CHECK(world().ok);  // every ClipId name is a clip in tools/anim/clips.py
    // Hatchlings get their own variants where one exists.
    int baby[static_cast<int>(ClipId::Count)];
    CHECK(resolveClips(world().lib, kFormHatchling, baby));
    CHECK(baby[static_cast<int>(ClipId::Eat)] == world().lib.find("eat_h"));
    CHECK(baby[static_cast<int>(ClipId::Walk)] == world().clips[static_cast<int>(ClipId::Walk)]);
    for (int a = 0; a < static_cast<int>(Activity::Count); ++a)
        CHECK(activityName(static_cast<Activity>(a))[0] != '?');
}

TEST(a_content_dragon_leads_a_varied_life_on_the_floor) {
    DenActor a;
    const DenLayout den;
    a.reset(den, 7);
    const Dragon d = contentDragon();
    bool seen[static_cast<int>(Activity::Count)] = {};
    float maxR = 0, travelled = 0;
    Vec2 last = a.behavior.pos;
    u8 ev[16];
    int footsteps = 0;
    for (int f = 0; f < 20 * 60 * 30; ++f) {  // 20 minutes
        const int n = a.update(d, false, 1.0f, 1.0f / 30, world().lib, world().clips, ev, 16);
        for (int i = 0; i < n; ++i) footsteps += ev[i] == kAnimFootstep;
        seen[static_cast<int>(a.behavior.activity)] = true;
        maxR = std::fmax(maxR, dist(a.behavior.pos, den.home));
        travelled += dist(a.behavior.pos, last);
        last = a.behavior.pos;
        CHECK(std::isfinite(a.behavior.pos.x) && std::isfinite(a.behavior.heading));
    }
    int ambient = 0;
    for (int i = 0; i <= static_cast<int>(Activity::Flutter); ++i) ambient += seen[i];
    std::printf("  20 min: %d ambient activities, travelled %.1f, max radius %.2f, %d footsteps\n", ambient,
                travelled, maxR, footsteps);
    CHECK(ambient >= 7);
    CHECK(travelled > 5.0f && maxR <= den.radius + 1e-3f);
    CHECK(footsteps > 20);
    CHECK(!seen[static_cast<int>(Activity::Sleep)] && !seen[static_cast<int>(Activity::Sulk)]);
}

TEST(an_upset_dragon_sulks_in_the_nook_until_you_make_up) {
    DenActor a;
    const DenLayout den;
    a.reset(den, 3);
    Dragon d = contentDragon();
    d.upset = true;
    CHECK(run(a, d, false, 40, [](const DenBehavior& b) { return b.activity == Activity::Sulk; }));
    CHECK(dist(a.behavior.pos, den.sulkSpots[0]) < 0.4f);
    CHECK(std::fabs(std::fabs(a.behavior.heading) - 3.14159f) < 0.2f);  // back turned
    a.behavior.care(Care::Pet, d);                                       // ignored while upset
    CHECK(a.behavior.activity == Activity::Sulk);
    CHECK(run(a, d, false, 10, [](const DenBehavior& b) { return b.clip == ClipId::SulkLoop; }));
    makeUp(d);
    a.behavior.care(Care::MakeUp, d);
    CHECK(a.behavior.activity == Activity::MakeUp);
    CHECK(run(a, d, false, 10, [](const DenBehavior& b) { return b.activity == Activity::Greet; }));
    CHECK(run(a, d, false, 10, [](const DenBehavior& b) { return b.activity == Activity::Idle; }));
}

TEST(tired_dragons_nap_in_the_nest_and_wake_up) {
    DenActor a;
    const DenLayout den;
    a.reset(den, 5);
    Dragon d = contentDragon();
    d.napping = true;
    CHECK(run(a, d, false, 40, [](const DenBehavior& b) { return b.activity == Activity::Sleep; }));
    CHECK(dist(a.behavior.pos, den.beds[0]) < 0.4f);
    a.behavior.care(Care::Feed, d);  // asleep: ignored
    CHECK(a.behavior.activity == Activity::Sleep);
    d.napping = false;
    CHECK(run(a, d, false, 2, [](const DenBehavior& b) { return b.activity == Activity::Wake; }));
    CHECK(run(a, d, false, 10, [](const DenBehavior& b) { return b.activity == Activity::Idle; }));
    // Night sends it to bed too.
    CHECK(run(a, d, true, 40, [](const DenBehavior& b) { return b.activity == Activity::Sleep; }));
}

TEST(care_interrupts_everyday_life) {
    DenActor a;
    const DenLayout den;
    a.reset(den, 9);
    const Dragon d = contentDragon();
    a.behavior.care(Care::Pet, d, PetZone::Head);
    CHECK(a.behavior.activity == Activity::PetHead);
    CHECK(run(a, d, false, 1.0f, [](const DenBehavior&) { return false; }) == false);
    CHECK(a.behavior.activity == Activity::PetHead);  // still leaning in shortly after a stroke
    CHECK(run(a, d, false, 3, [](const DenBehavior& b) { return b.activity == Activity::Idle; }));

    a.behavior.care(Care::Pet, d, PetZone::Belly);
    CHECK(a.behavior.activity == Activity::BellyRub && a.behavior.clip == ClipId::RollOver);
    bool rubbing = run(a, d, false, 3, [&](const DenBehavior& b) {
        a.behavior.petTimer = 1.0f;  // keep rubbing
        return b.clip == ClipId::BellyRub;
    });
    CHECK(rubbing);

    a.behavior.care(Care::FeedFavorite, d);
    CHECK(a.behavior.activity == Activity::Eat);
    CHECK(run(a, d, false, 6, [](const DenBehavior& b) { return b.activity == Activity::Favorite; }));

    // A pounce carries the dragon forward by the leap length (scaled to its size).
    a.reset(den, 11);
    a.behavior.heading = 0;
    const Vec2 before = a.behavior.pos;
    a.behavior.force(Activity::Pounce);
    CHECK(run(a, d, false, 4, [](const DenBehavior& b) { return b.activity == Activity::Idle; }, 0.5f));
    const float moved = dist(a.behavior.pos, before);
    CHECK(moved > 0.6f && moved < 0.7f);             // 1.3 x 0.5
    CHECK(a.behavior.pos.y < before.y);              // forward is -Y at heading 0
}

TEST(dragons_walk_around_the_hearth_and_hoard) {
    const DenLayout den;
    // A day of everyday life never puts the body inside an obstacle.
    DenActor a;
    a.reset(den, 13);
    const Dragon d = contentDragon();
    float closest = 1e9f;
    for (int f = 0; f < 20 * 60 * 30; ++f) {
        a.update(d, false, 1.0f, 1.0f / 30, world().lib, world().clips, nullptr, 0);
        for (const DenObstacle& o : den.obstacles) closest = std::fmin(closest, dist(a.behavior.pos, o.at) - o.radius);
    }
    std::printf("  closest approach to an obstacle's edge: %.2f\n", closest);
    CHECK(closest >= 0.8f - 1e-3f);

    // Bedtime from the far side of the egg nest: walks round it to the nest, not through it.
    a.reset(den, 14);
    a.behavior.pos = {4.2f, -3.4f};
    a.behavior.heading = 3.14159f;  // facing +Y, the nest beyond the egg nest
    CHECK(!a.behavior.clearPath(a.behavior.pos, den.beds[0], 0.8f));
    Dragon tired = contentDragon();
    tired.napping = true;
    float nearest = 1e9f;
    const bool slept = run(a, tired, false, 60, [&](const DenBehavior& b) {
        nearest = std::fmin(nearest, dist(b.pos, den.eggNest));
        return b.activity == Activity::Sleep;
    });
    CHECK(slept && dist(a.behavior.pos, den.beds[0]) < 0.4f);
    CHECK(nearest >= den.obstacles[1].radius + 0.8f - 1e-3f);
}

TEST(three_dragons_share_the_den) {
    const DenLayout den;
    DenActor a[3];
    DenBehavior* crowd[3];
    for (int i = 0; i < 3; ++i) {
        a[i].reset(den, 31 + i, i);
        crowd[i] = &a[i].behavior;
    }
    a[1].behavior.pos = {-1.9f, 1.0f};
    a[2].behavior.pos = {2.0f, 1.3f};
    Dragon d = contentDragon();
    // Everyday life: they walk around each other (a little overlap while two are both
    // moving is fine; never deep inside one another).
    float closest = 1e9f;
    for (int f = 0; f < 10 * 60 * 30; ++f) {
        shareCrowd(crowd, 3);
        for (int i = 0; i < 3; ++i) a[i].update(d, false, 1.0f, 1.0f / 30, world().lib, world().clips, nullptr, 0);
        for (int i = 0; i < 3; ++i)
            for (int j = i + 1; j < 3; ++j) closest = std::fmin(closest, dist(a[i].behavior.pos, a[j].behavior.pos));
    }
    std::printf("  3 dragons, 10 min: closest %.2f apart\n", closest);
    CHECK(closest > 2.0f);  // two adult bodies (2 x 1.2), less a little jostling
    // Bedtime: each goes to its own bed.
    bool asleep[3] = {};
    for (int f = 0; f < 90 * 30; ++f) {
        shareCrowd(crowd, 3);
        for (int i = 0; i < 3; ++i) {
            a[i].update(d, true, 1.0f, 1.0f / 30, world().lib, world().clips, nullptr, 0);
            asleep[i] = a[i].behavior.activity == Activity::Sleep;
        }
        if (asleep[0] && asleep[1] && asleep[2]) break;
    }
    for (int i = 0; i < 3; ++i) CHECK(asleep[i] && dist(a[i].behavior.pos, den.beds[i]) < 0.4f);
    // Upset: each sulks in its own spot.
    d.upset = true;
    bool sulking[3] = {};
    for (int f = 0; f < 90 * 30; ++f) {
        shareCrowd(crowd, 3);
        for (int i = 0; i < 3; ++i) {
            a[i].update(d, false, 1.0f, 1.0f / 30, world().lib, world().clips, nullptr, 0);
            sulking[i] = a[i].behavior.activity == Activity::Sulk;
        }
        if (sulking[0] && sulking[1] && sulking[2]) break;
    }
    for (int i = 0; i < 3; ++i) CHECK(sulking[i] && dist(a[i].behavior.pos, den.sulkSpots[i]) < 0.4f);
}

TEST(every_activity_is_reachable_and_settles) {
    const DenLayout den;
    const Dragon d = contentDragon();
    for (int i = 0; i < static_cast<int>(Activity::Count); ++i) {
        DenActor a;
        a.reset(den, 20 + i);
        a.behavior.force(static_cast<Activity>(i));
        CHECK(a.behavior.activity == static_cast<Activity>(i));
        a.behavior.petTimer = 0.5f;
        // Everything returns to everyday life on a content, rested dragon by day.
        const bool settled = run(a, d, false, 60, [](const DenBehavior& b) {
            return b.activity == Activity::Idle || b.activity == Activity::Wander || b.activity == Activity::LookAround ||
                   b.activity == Activity::Sit || b.activity == Activity::Lie;
        });
        if (!settled) std::printf("  stuck in %s\n", activityName(a.behavior.activity));
        CHECK(settled);
    }
}

}  // namespace

void runBehaviorTests() {
    RUN(behavior_clips_exist_in_the_clip_file);
    RUN(a_content_dragon_leads_a_varied_life_on_the_floor);
    RUN(an_upset_dragon_sulks_in_the_nook_until_you_make_up);
    RUN(tired_dragons_nap_in_the_nest_and_wake_up);
    RUN(care_interrupts_everyday_life);
    RUN(dragons_walk_around_the_hearth_and_hoard);
    RUN(three_dragons_share_the_den);
    RUN(every_activity_is_reachable_and_settles);
}
