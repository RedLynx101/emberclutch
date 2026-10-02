// Den behavior tests: the real clip file drives the real behavior loop (core/den_actor), as
// on the 3DS. Every activity is reachable, feelings and sleep take the dragon to the right
// spot, care interrupts the right things, and the dragon stays on the floor.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "check.hpp"
#include "core/den_actor.hpp"
#include "core/kinds.hpp"
#include "core/props.hpp"
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
    d.needs = Needs{75, 75, 75, 75, 75};
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
    CHECK(baby[static_cast<int>(ClipId::Walk)] == world().lib.find("walk_h"));  // the toddle (run 13)
    CHECK(baby[static_cast<int>(ClipId::Trot)] == world().clips[static_cast<int>(ClipId::Trot)]);  // none: shared
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
    a.behavior.care(Care::Feed, d);  // asleep: your touch wakes it (D143); the food waits till it's up
    CHECK(a.behavior.activity == Activity::Wake && !a.behavior.awake());
    CHECK(run(a, d, false, 10, [](const DenBehavior& b) { return b.activity == Activity::Idle; }));
    CHECK(a.behavior.awake());
    // Still tired, it stays up the woken minute, then goes back to bed.
    CHECK(!run(a, d, false, kWokenHold - 15, [](const DenBehavior& b) { return b.activity == Activity::GoNap || b.activity == Activity::Sleep; }));
    CHECK(run(a, d, false, 40, [](const DenBehavior& b) { return b.activity == Activity::Sleep; }));
    d.napping = false;
    CHECK(run(a, d, false, 2, [](const DenBehavior& b) { return b.activity == Activity::Wake; }));
    CHECK(run(a, d, false, 10, [](const DenBehavior& b) { return b.activity == Activity::Idle; }));
    // Night sends it to bed too.
    CHECK(run(a, d, true, 40, [](const DenBehavior& b) { return b.activity == Activity::Sleep; }));
}

// Run 24 (Noah: "allow the player to wake them up, then they stay up for a minute or until something
// else happens. Then after a minute of nighttime and nothing, they sleep again"): woken at night it
// takes your care, every bit of care keeps it up a minute more, and a minute of nothing sends it to bed.
TEST(woken_at_night_a_dragon_stays_up_while_you_care_for_it) {
    DenActor a;
    const DenLayout den;
    a.reset(den, 9);
    Dragon d = contentDragon();
    CHECK(run(a, d, true, 40, [](const DenBehavior& b) { return b.activity == Activity::Sleep; }));
    CHECK(a.behavior.wake());
    CHECK(run(a, d, true, 10, [](const DenBehavior& b) { return b.activity == Activity::Idle; }));
    a.behavior.care(Care::Feed, d);  // up now: it eats
    CHECK(a.behavior.activity == Activity::Eat);
    CHECK(!run(a, d, true, 40, [](const DenBehavior& b) { return b.activity == Activity::GoNap; }));
    a.behavior.care(Care::Pet, d);  // 40 s on: a pat keeps it up another minute
    CHECK(!run(a, d, true, 45, [](const DenBehavior& b) { return b.activity == Activity::GoNap; }));
    CHECK(run(a, d, true, 40, [](const DenBehavior& b) { return b.activity == Activity::Sleep; }));
    CHECK(a.behavior.wake());  // (asleep again: it can be woken again)
}

TEST(dragons_blink_and_shut_their_eyes_to_sleep) {
    DenActor a;
    const DenLayout den;
    a.reset(den, 12);
    Dragon d = contentDragon();
    int blinks = 0;
    bool wasShut = false;
    run(a, d, false, 30, [&](const DenBehavior& b) {
        if (b.activity == Activity::Sleep || b.activity == Activity::GoNap) return false;
        const bool shut = a.eyes.shut > 0.9f;
        blinks += shut && !wasShut;
        wasShut = shut;
        return false;
    });
    std::printf("    %d blinks in 30 s awake\n", blinks);
    CHECK(blinks >= 5 && blinks <= 16);
    d.napping = true;
    CHECK(run(a, d, false, 40, [](const DenBehavior& b) { return b.activity == Activity::Sleep; }));
    CHECK(run(a, d, false, 2, [](const DenBehavior&) { return false; }) == false);
    CHECK(a.eyes.shut > 0.99f);
    d.napping = false;  // wakes: the eyes open again
    CHECK(run(a, d, false, 12, [](const DenBehavior& b) { return b.activity == Activity::Idle; }));
    CHECK(run(a, d, false, 1, [](const DenBehavior&) { return false; }) == false);
    CHECK(a.eyes.level < 0.05f);
    a.behavior.care(Care::Pet, d, PetZone::Chin);  // a content squint
    CHECK(run(a, d, false, 1, [&](const DenBehavior&) { a.behavior.petTimer = 1.0f; return false; }) == false);
    CHECK(std::fabs(a.eyes.level - 0.6f) < 0.05f);
}

// A full fetch round trip with the real ball physics: the dragon chases the ball, picks it
// up, carries it to the player, drops it and waits; the test plays the scene's part (the
// ball rides at the mouth while held, and is let go when the dragon drops it).
TEST(dragons_fetch_the_ball_and_bring_it_back) {
    for (Stage stage : {Stage::Hatchling, Stage::Adult}) {
        DenActor a;
        const DenLayout den;
        a.reset(den, 31);
        Dragon d = contentDragon();
        d.stage = stage;
        d.personality = Personality::Brave;  // no keep-away in this test
        Ball ball;
        a.behavior.ball = &ball;
        ball.launch({den.player.x, den.player.y, 1.0f}, {1.2f, 5.5f, 3.2f});
        a.behavior.care(Care::Throw, d);
        CHECK(a.behavior.activity == Activity::Fetch);
        bool picked = false, dropped = false;
        u8 ev[16];
        float t = 0;
        for (; t < 40.0f; t += 1.0f / 30) {
            ball.step(den, 1.0f / 30);
            a.update(d, false, stage == Stage::Adult ? 1.0f : 0.45f, 1.0f / 30, world().lib, world().clips, ev, 16);
            const DenBehavior& b = a.behavior;
            if (b.holdingBall) {  // the scene keeps the ball at the jaw
                picked = true;
                ball.pos = {b.pos.x + std::sin(b.heading) * 0.8f, b.pos.y - std::cos(b.heading) * 0.8f, 0.5f};
            }
            if (b.dropBall) {
                a.behavior.dropBall = false;
                ball.release(ball.pos, {0, -0.8f, 0});
                dropped = true;
            }
            if (dropped && b.activity == Activity::Idle) break;
        }
        const float fromPlayer = dist(a.behavior.pos, den.player);
        std::printf("  %s: fetched in %.1f s, dropped %.2f from the player\n", stageName(stage), t, fromPlayer);
        CHECK(picked && dropped && !a.behavior.holdingBall);
        CHECK(fromPlayer < 1.6f);
        CHECK(t < 30.0f);
    }
}

// The ball's way cleared (D124, Noah: "other dragons can get in the way and block the main dragon
// from getting the ball"): one sitting right where the ball lands trots aside, and yours gets it.
TEST(a_dragon_in_the_way_makes_way_for_the_ball) {
    const DenLayout den;
    DenActor a[2];
    DenBehavior* crowd[2];
    for (int i = 0; i < 2; ++i) {
        a[i].reset(den, 71 + i, i);
        crowd[i] = &a[i].behavior;
    }
    Dragon d = contentDragon();
    d.stage = Stage::Adult;
    d.personality = Personality::Brave;
    Ball ball;
    const Vec2 lands{0.8f, 2.4f};
    a[0].behavior.pos = {den.player.x, den.player.y + 0.8f};
    a[1].behavior.pos = lands;  // sitting right on the spot
    a[1].behavior.force(Activity::Sit);
    for (int i = 0; i < 2; ++i) a[i].behavior.ball = &ball;
    ball.launch({lands.x, lands.y, ball.radius + 0.02f}, {0, 0, 0});
    a[0].behavior.care(Care::Throw, d);
    CHECK(a[0].behavior.activity == Activity::Fetch);
    bool got = false;
    float t = 0;
    for (; t < 20.0f && !got; t += 1.0f / 30) {
        ball.step(den, 1.0f / 30);
        shareCrowd(crowd, 2);
        makeWayForBall(crowd, 2);
        for (int i = 0; i < 2; ++i) a[i].update(d, false, 1.0f, 1.0f / 30, world().lib, world().clips, nullptr, 0);
        got = a[0].behavior.holdingBall;
    }
    const float moved = dist(a[1].behavior.pos, lands);
    std::printf("  made way: the ball picked up in %.1f s, the one in the way moved %.2f\n", t, moved);
    CHECK(got);
    CHECK(moved > 0.6f);
}

// Walking at every pace (D130: "Curlstone should only use its walking animation when following the
// player and when in the den. Rolling only for human riders"): a walk-only body's zoomies are its
// walk, quicker; never its run. Kinds on the Curlstone's plan roll to run, the others don't.
TEST(a_walk_only_body_never_rolls) {
    const DenLayout den;
    DenActor a;
    a.reset(den, 5);
    a.walkOnly = true;
    Dragon d = contentDragon();
    a.behavior.force(Activity::Zoomies);
    const int walk = world().clips[static_cast<int>(ClipId::Walk)], run = world().clips[static_cast<int>(ClipId::Gallop)];
    bool ran = false, walked = false;
    float fastest = 0;
    for (int f = 0; f < 120; ++f) {
        a.update(d, false, 1.0f, 1.0f / 30, world().lib, world().clips, nullptr, 0);
        ran |= run != walk && a.anim.clip == run;
        walked |= a.anim.clip == walk && a.behavior.speed > 0;
        fastest = std::fmax(fastest, a.behavior.speed);
    }
    std::printf("  walk only: zoomies at %.2f a second, its walk %.2f\n", fastest, a.behavior.walkSpeed);
    CHECK(!ran && walked);
    Dragon stone;
    for (int k = 0; k < kindCount(); ++k) {
        stone.kind = static_cast<u8>(k);
        if (std::strcmp(kindInfo(k).name, "curlstone") == 0) CHECK(rollsToRun(stone));
        if (std::strcmp(kindInfo(k).name, "pouncer") == 0) CHECK(!rollsToRun(stone));
    }
}

// Fighting for the ball back (Noah, run 13): held near its mouth it hangs on and tugs; let go
// and it keeps it; pull long enough (the scene decides when) and the ball is yours.
TEST(tug_of_war_over_the_ball) {
    const DenLayout den;
    DenActor a;
    a.reset(den, 41);
    Dragon d = contentDragon();
    d.personality = Personality::Brave;
    Ball ball;
    a.behavior.ball = &ball;
    ball.launch({den.player.x, den.player.y, 1.0f}, {0.4f, 3.0f, 2.0f});
    a.behavior.care(Care::Throw, d);
    u8 ev[16];
    auto frame = [&] {
        ball.step(den, 1.0f / 30);
        a.update(d, false, 1.0f, 1.0f / 30, world().lib, world().clips, ev, 16);
        if (a.behavior.holdingBall) ball.pos = {a.behavior.pos.x, a.behavior.pos.y - 0.8f, 0.5f};
    };
    for (int f = 0; f < 30 * 20 && !a.behavior.holdingBall; ++f) frame();
    CHECK(a.behavior.holdingBall);
    for (int f = 0; f < 30; ++f) {  // a second of pulling
        a.behavior.care(Care::BallTug, d);
        frame();
    }
    CHECK(a.behavior.activity == Activity::Fetch && a.behavior.step == 8 && a.behavior.clip == ClipId::Tug);
    for (int f = 0; f < 30; ++f) frame();  // let go: it keeps it
    CHECK(a.behavior.holdingBall && a.behavior.step != 8);
    a.behavior.care(Care::BallTug, d);
    frame();
    a.behavior.care(Care::BallWon, d);  // pulled free
    CHECK(!a.behavior.holdingBall && !a.behavior.dropBall && a.behavior.activity == Activity::Hop);
}

TEST(hands_on_care_reactions) {
    const DenLayout den;
    Dragon d = contentDragon();
    auto fresh = [&](u32 seed) {
        DenActor a;
        a.reset(den, seed);
        return a;
    };
    // Hand-feeding: it waits for each bite; the last one ends in the happy wiggle; a food it
    // dislikes gets refused.
    DenActor a = fresh(41);
    a.behavior.care(Care::OfferFood, d);
    CHECK(a.behavior.activity == Activity::HandFeed);
    CHECK(run(a, d, false, 1.0f, [](const DenBehavior&) { return false; }) == false);
    a.behavior.feedBite(false, false, true);
    CHECK(a.behavior.activity == Activity::HandFeed);
    a.behavior.feedBite(false, true, true);
    CHECK(a.behavior.activity == Activity::Favorite);
    a.behavior.care(Care::OfferFood, d);
    a.behavior.feedBite(true, false, false);
    CHECK(a.behavior.activity == Activity::Refuse);
    CHECK(run(a, d, false, 4, [](const DenBehavior& b) { return b.activity == Activity::Idle; }));
    a.behavior.care(Care::OfferFood, d);  // held out and taken away: it gives up waiting
    CHECK(run(a, d, false, 4, [](const DenBehavior& b) { return b.activity == Activity::Idle; }));

    // The bath: to the tub, in, sitting; rinsed, it hops out and shakes off.
    a = fresh(42);
    a.behavior.care(Care::Bath, d);
    CHECK(run(a, d, false, 20, [](const DenBehavior& b) { return b.activity == Activity::Bath && b.step == 2; }));
    CHECK(dist(a.behavior.pos, a.behavior.tubAt) < 0.5f);
    CHECK(dist(a.behavior.tubAt, den.player) < dist(den.home, den.player));  // set down toward you
    a.behavior.care(Care::BathDone, d);
    CHECK(run(a, d, false, 6, [](const DenBehavior& b) { return b.activity == Activity::Idle; }));

    // Grooming: it turns a flank to the camera, sits up for the belly, lifts a wing, then
    // shakes off when the brushing stops.
    a = fresh(43);
    CHECK(a.behavior.groomFacing == 0);  // facing you to begin with
    a.behavior.turnAround(-1);  // L: its left flank (D83)
    CHECK(a.behavior.groomFacing == 3);
    a.behavior.care(Care::GroomBody, d);
    CHECK(a.behavior.activity == Activity::Groomed);
    CHECK(run(a, d, false, 3, [&](const DenBehavior& b) {
        a.behavior.petTimer = 1.0f;
        return b.step == 1;
    }));
    CHECK(std::fabs(a.behavior.heading + 1.25f) < 0.15f);
    a.behavior.turnAround(-1);  // its back to you
    CHECK(run(a, d, false, 4, [&](const DenBehavior& b) {
        a.behavior.petTimer = 1.0f;
        return b.step == 1 && std::fabs(std::fabs(b.heading) - 3.14159f) < 0.15f;
    }));
    a.behavior.care(Care::GroomWing, d);
    CHECK(a.behavior.clip == ClipId::LiftWing);
    CHECK(run(a, d, false, 3, [&](const DenBehavior& b) {
        a.behavior.petTimer = 1.0f;
        return b.step == 1;
    }));
    a.behavior.care(Care::GroomBelly, d);
    CHECK(a.behavior.clip == ClipId::Sit);
    CHECK(run(a, d, false, 6, [](const DenBehavior& b) { return b.activity == Activity::Idle; }));

    // Petting quirks: the sweet spot kicks a leg, a poke makes it sneeze (a shy one pulls
    // back), rough handling makes it pull away.
    a = fresh(44);
    a.behavior.care(Care::SweetSpot, d);
    CHECK(a.behavior.activity == Activity::Kick);
    CHECK(run(a, d, false, 3, [](const DenBehavior& b) { return b.activity == Activity::PetHead; }));
    a.behavior.care(Care::Poke, d);
    CHECK(a.behavior.activity == Activity::Sneeze);
    Dragon shy = d;
    shy.personality = Personality::Shy;
    a.behavior.care(Care::Poke, shy);
    CHECK(a.behavior.activity == Activity::PullAway);
    a.behavior.care(Care::Rough, d);
    CHECK(a.behavior.activity == Activity::PullAway);
    CHECK(run(a, d, false, 3, [](const DenBehavior& b) { return b.activity == Activity::Idle; }));

    // Called, it comes over and sits in front of the player.
    a = fresh(45);
    a.behavior.pos = {3.0f, 3.0f};
    a.behavior.care(Care::Call, d);
    CHECK(run(a, d, false, 15, [](const DenBehavior& b) { return b.activity == Activity::Come && b.step == 2; }));
    CHECK(dist(a.behavior.pos, den.player) < 1.4f);
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
        nearest = std::fmin(nearest, dist(b.pos, den.eggNests[0]));
        return b.activity == Activity::Sleep;
    });
    CHECK(slept && dist(a.behavior.pos, den.beds[0]) < 0.4f);
    CHECK(nearest >= den.obstacles[1].radius + 0.8f - 1e-3f);
}

TEST(a_walk_it_cant_finish_ends_where_it_can) {
    // Run 15: dragons walked into the walls and props for good. A goal past the wall is taken
    // at the wall, one in the middle of the hearth at its edge, and a walk that gets no
    // nearer ends after a moment, so the activity moves on.
    const DenLayout den;
    DenBehavior b;
    b.reset(den, 21);
    const Vec2 goals[] = {{0.0f, -30.0f}, den.hearth, {den.hoard.x, den.hoard.y - 0.1f}};
    for (const Vec2 goal : goals) {
        b.pos = den.home;
        int frames = 0;
        while (!b.walkTo(goal, false, 1.0f, 1.0f / 30) && frames < 30 * 30) ++frames;
        std::printf("  to (%.1f, %.1f): there in %.1f s, at (%.1f, %.1f)\n", goal.x, goal.y, frames / 30.0f, b.pos.x, b.pos.y);
        CHECK(frames < 30 * 20);
        CHECK(dist(b.pos, den.home) <= den.radius + 1e-3f);
    }
    // Something it can't get round (a dragon lying across the way, taken as a wall of crowd):
    // it gives up after the moment, not after the activity's own timeout.
    b.pos = {-3.0f, 0.6f};
    b.heading = -1.5708f;  // facing +X
    b.crowdCount = 1;
    b.crowd[0] = {{-1.0f, 0.6f}, 1.9f};
    int frames = 0;
    while (!b.walkTo({1.0f, 0.6f}, false, 1.0f, 1.0f / 30) && frames < 30 * 30) {
        ++frames;
        for (int i = 0; i < b.crowdCount; ++i) {  // the floor pushes it back out, as update() does
            const float d = dist(b.pos, b.crowd[i].at), room = b.crowd[i].radius + 1.2f;
            if (d < room) b.pos = {b.crowd[i].at.x + (b.pos.x - b.crowd[i].at.x) * room / d, b.crowd[i].at.y + (b.pos.y - b.crowd[i].at.y) * room / d};
        }
    }
    std::printf("  blocked: gave up after %.1f s\n", frames / 30.0f);
    CHECK(frames < 30 * 8);
}

// Three big grown dragons (Noah, run 21 take 4: "the adults were still getting stuck on each
// other in the den"): through an evening they keep moving, never wedged deep in one another, and
// at night each still reaches its own bed.
TEST(three_big_dragons_share_the_den) {
    const DenLayout den;
    DenActor a[3];
    DenBehavior* crowd[3];
    constexpr float kBig = 1.5f;
    for (int i = 0; i < 3; ++i) {
        a[i].reset(den, 57 + i, i);
        crowd[i] = &a[i].behavior;
    }
    a[1].behavior.pos = {-1.9f, 1.0f};
    a[2].behavior.pos = {2.0f, 1.3f};
    Dragon d = contentDragon();
    float closest = 1e9f;
    for (int f = 0; f < 10 * 60 * 30; ++f) {
        shareCrowd(crowd, 3);
        for (int i = 0; i < 3; ++i) a[i].update(d, false, kBig, 1.0f / 30, world().lib, world().clips, nullptr, 0);
        for (int i = 0; i < 3; ++i)
            for (int j = i + 1; j < 3; ++j) closest = std::fmin(closest, dist(a[i].behavior.pos, a[j].behavior.pos));
    }
    std::printf("  3 big dragons, 10 min: closest %.2f apart\n", closest);
    CHECK(closest > 2.2f);
    bool asleep[3] = {};
    for (int f = 0; f < 120 * 30; ++f) {
        shareCrowd(crowd, 3);
        for (int i = 0; i < 3; ++i) {
            a[i].update(d, true, kBig, 1.0f / 30, world().lib, world().clips, nullptr, 0);
            asleep[i] = a[i].behavior.activity == Activity::Sleep;
        }
        if (asleep[0] && asleep[1] && asleep[2]) break;
    }
    for (int i = 0; i < 3; ++i) {
        std::printf("  bed %d: %s %.2f from it\n", i, activityName(a[i].behavior.activity), dist(a[i].behavior.pos, den.beds[i]));
        CHECK(asleep[i] && dist(a[i].behavior.pos, den.beds[i]) < 0.6f);
    }
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

// A hatchling straight out of the egg: named, out of the nest, groomed, then bathed.
TEST(a_new_hatchling_can_be_bathed) {
    const DenLayout den;
    Dragon d = contentDragon();
    d.stage = Stage::Hatchling;
    DenActor a;
    a.reset(den, 77);
    a.behavior.force(Activity::Hatch);
    const float s = 0.35f;
    run(a, d, false, 7, [](const DenBehavior&) { return false; }, s);
    a.behavior.care(Care::Greet, d);
    run(a, d, false, 12, [](const DenBehavior&) { return false; }, s);
    std::printf("  after naming: %s step %d at (%.2f, %.2f)\n", activityName(a.behavior.activity), a.behavior.step,
                a.behavior.pos.x, a.behavior.pos.y);
    for (int i = 0; i < 20; ++i) {
        a.behavior.care(Care::GroomBody, d);
        run(a, d, false, 0.1f, [](const DenBehavior&) { return false; }, s);
    }
    a.behavior.care(Care::Bath, d);
    const bool inTub = run(a, d, false, 20, [](const DenBehavior& b) { return b.activity == Activity::Bath && b.step == 2; }, s);
    std::printf("  bath: %s step %d at (%.2f, %.2f), walk %.2f\n", activityName(a.behavior.activity), a.behavior.step,
                a.behavior.pos.x, a.behavior.pos.y, a.behavior.walkSpeed);
    CHECK(inTub);
}

// Life together (Alpha 2 WP1): over some minutes of a bright day, three dragons play chase
// (one chases, one flees, it ends in a hop), meet for a nuzzle face to face, and lie in the
// sunbeam; at night two curl up side by side in the big nest.
TEST(den_dragons_live_together) {
    const DenLayout den;
    DenActor a[3];
    DenBehavior* bs[3];
    Dragon d[3] = {contentDragon(), contentDragon(), contentDragon()};
    d[0].personality = Personality::Playful;
    d[1].personality = Personality::Curious;
    d[2].personality = Personality::Shy;
    const Dragon* ds[3] = {&d[0], &d[1], &d[2]};
    for (int i = 0; i < 3; ++i) {
        a[i].reset(den, 71 + i, i);
        bs[i] = &a[i].behavior;
    }
    a[0].behavior.pos = {-1.5f, 0.5f};
    a[2].behavior.pos = {2.0f, 1.3f};
    DenSocial social;
    Rng rng(5);
    bool chased = false, caught = false, nuzzled = false, basked = false, galloped = false;
    bool sparred = false, rolled = false, pounced = false, ambushChase = false, tailChased = false;  // run 13
    bool flew = false, landed = false, inside = true;  // D85: up off the floor, down again, never through the walls
    for (int f = 0; f < 12 * 60 * 30; ++f) {  // (twelve minutes: flights take their share of the den's time)
        shareCrowd(bs, 3);
        denSocial(social, bs, ds, 3, false, 1.0f, 1.0f / 30, rng);
        for (int i = 0; i < 3; ++i) {
            a[i].update(d[i], false, 1.0f, 1.0f / 30, world().lib, world().clips, nullptr, 0);
            const DenBehavior& b = *bs[i];
            if (b.activity == Activity::Chase && b.partner >= 0 && bs[b.partner]->activity == Activity::Flee) chased = true;
            if (chased && b.activity == Activity::Hop) caught = true;
            if (b.activity == Activity::Chase && b.clip == ClipId::Gallop) galloped = true;  // at a run (WP12c)
            if (b.activity == Activity::Spar && b.step == 3 && b.partner >= 0 && bs[b.partner]->activity == Activity::Spar &&
                bs[b.partner]->step == 3 && dist(b.pos, bs[b.partner]->pos) < 4.0f)
                sparred = true;
            if (b.activity == Activity::Spar && b.step == 4 && b.clip == ClipId::RollOver) rolled = true;
            if (b.activity == Activity::Stalk && b.step == 1) pounced = true;
            if (pounced && b.activity == Activity::Flee && b.partner >= 0 && bs[b.partner]->activity == Activity::Chase)
                ambushChase = true;
            if (b.activity == Activity::TailChase && b.clip == ClipId::TailChase) tailChased = true;
            if (b.activity == Activity::Nuzzle && b.step == 2 && b.partner >= 0 &&
                bs[b.partner]->activity == Activity::Nuzzle && dist(b.pos, bs[b.partner]->pos) < 3.0f)
                nuzzled = true;
            if (b.activity == Activity::Bask && b.step == 2 && dist(b.pos, den.sunSpot) < 2.5f) basked = true;
            if (b.activity == Activity::Fly && b.air > 0.6f) flew = true;
            if (flew && b.activity != Activity::Fly && b.air == 0.0f) landed = true;
            if (dist(b.pos, den.room) > den.wallRadius) inside = false;
        }
    }
    std::printf("  flying: up %d, down again %d, inside the walls %d\n", flew, landed, inside);
    CHECK(flew && landed && inside);
    std::printf("  a bright day: chase %d (ending in a hop %d), nuzzle %d, sunbeam %d\n", chased, caught, nuzzled,
                basked);
    CHECK(chased && caught && nuzzled && basked && galloped);
    std::printf("  more play: spar %d (one rolls over %d), stalk and pounce %d (then a chase %d), tail chase %d\n",
                sparred, rolled, pounced, ambushChase, tailChased);
    CHECK(sparred && rolled && pounced && ambushChase && tailChased);

    // Night: once two snuggle, they sleep side by side in the big nest.
    bool together = false;
    for (int night = 0; night < 5 && !together; ++night) {
        for (int f = 0; f < 40 * 30; ++f) {  // a spell of day between nights: they wake up
            shareCrowd(bs, 3);
            denSocial(social, bs, ds, 3, false, 1.0f, 1.0f / 30, rng);
            for (int i = 0; i < 3; ++i) a[i].update(d[i], false, 1.0f, 1.0f / 30, world().lib, world().clips, nullptr, 0);
        }
        for (int f = 0; f < 90 * 30 && !together; ++f) {
            shareCrowd(bs, 3);
            denSocial(social, bs, ds, 3, true, 0.0f, 1.0f / 30, rng);
            for (int i = 0; i < 3; ++i) a[i].update(d[i], true, 1.0f, 1.0f / 30, world().lib, world().clips, nullptr, 0);
            int nearNest = 0;
            for (int i = 0; i < 3; ++i)
                nearNest += bs[i]->activity == Activity::Sleep && dist(bs[i]->pos, den.beds[0]) < 3.2f;
            together = nearNest >= 2;
        }
        for (int i = 0; i < 3; ++i)
            std::printf("  night %d: %s snuggle %d at (%.1f %.1f) to nest %.1f\n", night, activityName(bs[i]->activity),
                        bs[i]->snuggle, bs[i]->pos.x, bs[i]->pos.y, dist(bs[i]->pos, den.beds[0]));
    }
    CHECK(together);
}

// Toys up close (Alpha 2 WP7): it watches the dangled feather and swats when it comes near;
// it bites on the rope and tugs while you hold it, then trots off with it, drops it and wags;
// interrupted on the way, it lets the toy fall where it is.
TEST(dragons_play_with_toys_up_close) {
    const DenLayout den;
    const Dragon d = contentDragon();
    DenActor a;
    a.reset(den, 31);
    DenBehavior& b = a.behavior;
    b.care(Care::Dangle, d);
    CHECK(b.activity == Activity::Bat);
    b.care(Care::Swat, d);
    CHECK(b.clip == ClipId::PawBat);
    CHECK(run(a, d, false, 3, [](const DenBehavior& x) { return x.activity != Activity::Bat; }));  // put away

    b.care(Care::TugPull, d);
    CHECK(b.activity == Activity::Tug && b.carrying == 1);
    u8 ev[16];
    for (int f = 0; f < 60; ++f) {  // two seconds of tugging
        b.care(Care::TugPull, d);
        a.update(d, false, 1.0f, 1.0f / 30, world().lib, world().clips, ev, 16);
    }
    CHECK(b.activity == Activity::Tug && b.clip == ClipId::Tug);
    const Vec2 from = b.pos;
    b.care(Care::TugLetGo, d);
    CHECK(b.activity == Activity::ToyRun && b.carrying == 1);
    CHECK(run(a, d, false, 12, [](const DenBehavior& x) { return x.dropToy == 1; }));
    CHECK(b.carrying == -1 && dist(b.pos, from) > 0.8f && dist(b.pos, den.home) <= den.radius);
    b.dropToy = -1;  // the scene sets it down
    CHECK(run(a, d, false, 6, [](const DenBehavior& x) { return x.activity == Activity::TailWag; }));

    b.care(Care::TugPull, d);
    b.care(Care::TugLetGo, d);
    CHECK(b.activity == Activity::ToyRun && b.carrying == 1);
    b.care(Care::Pet, d);  // stopped on the way
    CHECK(b.carrying == -1 && b.dropToy == 1 && b.activity == Activity::PetHead);
}

// A den with every toy out (Alpha 2 WP7): the scene's part, done here: the orb rolls when
// nudged, a knocked feather moves, a toy set down lies where it fell.
struct ToyDen {
    Ball orb;
    DenToys toys;
    ToyDen() {
        const Vec2 at[kDenToys] = {{-3.0f, 1.6f}, {-2.4f, -0.9f}, {2.0f, 1.9f}, {-1.4f, 3.0f}};
        for (int k = 0; k < kDenToys; ++k) {
            toys.here[k] = true;
            toys.at[k] = at[k];
        }
        orb.radius = 0.17f;
        orb.active = true;
        orb.pos = {at[2].x, at[2].y, orb.radius};
        toys.orb = &orb;
        toys.bowlFood = toys.orbTreat = true;
    }
    // After the dragons' update: take what they did.
    void apply(DenBehavior* const* bs, int n, const DenLayout& den, float dt) {
        toys.here[1] = true;
        for (int i = 0; i < n; ++i) {
            DenBehavior& b = *bs[i];
            if (b.nudged) orb.launch(orb.pos, {b.nudge.x * 2.4f, b.nudge.y * 2.4f, 0.6f});
            if (b.knocked) toys.at[0] = b.knock;
            if (b.dropToy >= 0) toys.at[b.dropToy] = b.pos;
            b.nudged = b.knocked = false;
            b.dropToy = -1;
            if (b.carrying == 1 || (b.activity == Activity::TugWar && b.step == 2)) toys.here[1] = false;
        }
        orb.step(den, dt);
    }
};

// On their own (Alpha 2 WP7): a dragon bats the feather and pounces on it, picks up the rope,
// shakes it and trots off with it, pushes the orb along until a treat drops out, eats from
// the bowl when hungry; two have a tug-of-war over the rope and one wins it.
TEST(dragons_play_with_toys_on_their_own) {
    const DenLayout den;
    Dragon d = contentDragon();
    d.personality = Personality::Playful;
    for (int toy = 0; toy < 3; ++toy) {
        ToyDen world_;
        DenActor a;
        a.reset(den, 90 + toy);
        DenBehavior* b = &a.behavior;
        b->toys = &world_.toys;
        b->toy = static_cast<s8>(toy);
        b->force(Activity::Play);
        bool knocked = false, carried = false, dropped = false, treat = false;
        const Vec2 featherWas = world_.toys.at[0];
        for (int f = 0; f < 40 * 30 && !(knocked || dropped || treat); ++f) {
            a.update(d, false, 1.0f, 1.0f / 30, world().lib, world().clips, nullptr, 0);
            knocked = knocked || b->knocked;
            carried = carried || b->carrying == 1;
            dropped = dropped || b->dropToy == 1;
            treat = treat || b->gotTreat;
            b->gotTreat = false;
            world_.apply(&b, 1, den, 1.0f / 30);
        }
        std::printf("  toy %d: knocked %d, carried %d and dropped %d, treat %d (orb pushes %d)\n", toy, knocked, carried,
                    dropped, treat, b->pushes);
        if (toy == 0) CHECK(knocked && dist(world_.toys.at[0], featherWas) > 0.3f);
        if (toy == 1) CHECK(carried && dropped);
        if (toy == 2) CHECK(treat && b->pushes >= 3);
    }

    // Hungry, with food in the bowl: over to it, and a portion eaten.
    ToyDen bowlDen;
    DenActor a;
    a.reset(den, 99);
    a.behavior.toys = &bowlDen.toys;
    Dragon hungry = contentDragon();
    hungry.needs.belly = 20;
    CHECK(run(a, hungry, false, 60, [](const DenBehavior& b) { return b.activity == Activity::Bowl; }));
    CHECK(run(a, hungry, false, 30, [](const DenBehavior& b) { return b.ateFromBowl; }));
    CHECK(dist(a.behavior.pos, bowlDen.toys.at[3]) < 1.6f);

    // Two playful dragons and the rope: a tug-of-war, and one trots off with it.
    ToyDen shared;
    DenActor two[2];
    DenBehavior* bs[2];
    Dragon pair[2] = {contentDragon(), contentDragon()};
    pair[0].personality = pair[1].personality = Personality::Playful;
    const Dragon* ds[2] = {&pair[0], &pair[1]};
    for (int i = 0; i < 2; ++i) {
        two[i].reset(den, 120 + i, i);
        bs[i] = &two[i].behavior;
        bs[i]->toys = &shared.toys;
        shared.toys.here[0] = shared.toys.here[2] = shared.toys.here[3] = false;  // just the rope
    }
    two[0].behavior.pos = {-1.5f, 0.5f};
    two[1].behavior.pos = {1.5f, 0.2f};
    DenSocial social;
    Rng rng(9);
    bool tugging = false, won = false;
    for (int f = 0; f < 10 * 60 * 30 && !won; ++f) {
        shareCrowd(bs, 2);
        denSocial(social, bs, ds, 2, false, 0.3f, 1.0f / 30, rng);
        for (int i = 0; i < 2; ++i) two[i].update(pair[i], false, 1.0f, 1.0f / 30, world().lib, world().clips, nullptr, 0);
        if (bs[0]->activity == Activity::TugWar && bs[1]->activity == Activity::TugWar && bs[0]->step == 2 &&
            bs[1]->step == 2 && dist(bs[0]->pos, bs[1]->pos) < 4.0f)
            tugging = true;
        for (int i = 0; i < 2; ++i) won = won || (tugging && bs[i]->activity == Activity::ToyRun && bs[i]->carrying == 1);
        shared.apply(bs, 2, den, 1.0f / 30);
    }
    std::printf("  tug-of-war %d, won %d\n", tugging, won);
    CHECK(tugging && won);
}

TEST(every_activity_is_reachable_and_settles) {
    const DenLayout den;
    const Dragon d = contentDragon();
    ToyDen toys;
    for (int i = 0; i < static_cast<int>(Activity::Count); ++i) {
        DenActor a;
        a.reset(den, 20 + i);
        a.behavior.toys = &toys.toys;
        a.behavior.force(static_cast<Activity>(i));
        CHECK(a.behavior.activity == static_cast<Activity>(i));
        a.behavior.petTimer = 0.5f;
        if (static_cast<Activity>(i) == Activity::Hatch) {  // it waits in the nest for its name
            CHECK(!run(a, d, false, 8, [](const DenBehavior& b) { return b.activity != Activity::Hatch; }));
            CHECK(a.behavior.step == 1 && dist(a.behavior.pos, den.eggNests[0]) < 0.01f);
            a.behavior.care(Care::Pet, d);  // not now
            CHECK(a.behavior.activity == Activity::Hatch);
            a.behavior.care(Care::Greet, d);  // named: out of the nest, then hello
            CHECK(run(a, d, false, 20, [](const DenBehavior& b) { return b.activity == Activity::Greet; }));
            CHECK(dist(a.behavior.pos, den.eggNests[0]) > den.obstacles[1].radius + 0.5f);
        }
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

// Zoomies (WP12c): a burst of laps round the den at a run (a scamper for a baby, a gallop
// for a grown one), then a happy wag; a favourite food or a bath can set them off.
TEST(zoomies_run_laps_round_the_den) {
    const DenLayout den;
    for (int baby = 0; baby < 2; ++baby) {
        DenActor a;
        a.reset(den, 91 + baby, 0);
        const Dragon d = contentDragon();
        a.behavior.baby = baby != 0;
        a.behavior.force(Activity::Zoomies);
        const ClipId want = baby ? ClipId::Scamper : ClipId::Gallop;
        bool ran = false;
        float travelled = 0;
        Vec2 last = a.behavior.pos;
        const bool done = run(a, d, false, 14, [&](const DenBehavior& b) {
            ran = ran || b.clip == want;
            travelled += dist(b.pos, last);
            last = b.pos;
            return b.activity == Activity::TailWag;
        });
        std::printf("  zoomies (%s): %.1f units, done %d\n", baby ? "baby" : "grown", travelled, done);
        CHECK(done && ran && travelled > den.radius);
        for (int k = 0; k < 20; ++k) {
            const Vec2 p = a.behavior.zoomPoint();
            CHECK(dist(p, den.home) < den.radius * 0.75f);
        }
    }
}

void runBehaviorTests() {
    RUN(zoomies_run_laps_round_the_den);
    RUN(a_walk_it_cant_finish_ends_where_it_can);
    RUN(tug_of_war_over_the_ball);
    RUN(behavior_clips_exist_in_the_clip_file);
    RUN(a_content_dragon_leads_a_varied_life_on_the_floor);
    RUN(an_upset_dragon_sulks_in_the_nook_until_you_make_up);
    RUN(tired_dragons_nap_in_the_nest_and_wake_up);
    RUN(woken_at_night_a_dragon_stays_up_while_you_care_for_it);
    RUN(dragons_blink_and_shut_their_eyes_to_sleep);
    RUN(care_interrupts_everyday_life);
    RUN(dragons_fetch_the_ball_and_bring_it_back);
    RUN(a_dragon_in_the_way_makes_way_for_the_ball);
    RUN(a_walk_only_body_never_rolls);
    RUN(hands_on_care_reactions);
    RUN(dragons_walk_around_the_hearth_and_hoard);
    RUN(three_dragons_share_the_den);
    RUN(three_big_dragons_share_the_den);
    RUN(den_dragons_live_together);
    RUN(a_new_hatchling_can_be_bathed);
    RUN(dragons_play_with_toys_up_close);
    RUN(dragons_play_with_toys_on_their_own);
    RUN(every_activity_is_reachable_and_settles);
}
