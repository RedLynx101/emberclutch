// Hands-on care tests (core/care, core/props): touch picking on the real models, zones and
// regions, stroke names, each dragon's quirks, grooming sessions and the ball's physics.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/care.hpp"
#include "core/dragon.hpp"
#include "core/genetics.hpp"
#include "core/props.hpp"
#include "core/rig.hpp"
#include "core/rng.hpp"

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

const ModelData& model(int form) {
    static ModelData m[kFormCount];
    static bool loaded[kFormCount] = {};
    if (!loaded[form]) {
        const std::string path = std::string("../romfs/models/") + (form == kFormHatchling ? "hatchling" : "grown") + ".ecm";
        const std::vector<u8> bytes = readAll(path.c_str());
        loaded[form] = !bytes.empty() && loadModel(bytes.data(), bytes.size(), m[form]);
    }
    return m[form];
}

Dragon dragonWithId(u32 id, Element e = Element::Ember) {
    Rng rng(id);
    Dragon d = makeEgg(id, makePurebred(e, rng), Sex::Female, 0);
    d.favoriteFood = static_cast<u8>(e);
    return d;
}

}  // namespace

TEST(capsules_cover_the_body) {
    for (int f = 0; f < kFormCount; ++f) {
        const ModelData& m = model(f);
        BoneCapsule caps[kMaxCapsules];
        const int n = buildCapsules(m, caps, kMaxCapsules);
        std::printf("  %s: %d capsules\n", f == kFormHatchling ? "hatchling" : "grown", n);
        CHECK(n >= 18);
        bool head = false, chest = false, tail = false, belly = false;
        for (int i = 0; i < n; ++i) {
            CHECK(caps[i].radius > 0.01f && caps[i].t1 > caps[i].t0);
            const char* name = m.skel.name[caps[i].bone];
            head |= std::string(name) == "head";
            chest |= std::string(name) == "chest";
            belly |= std::string(name) == "belly";
            tail |= std::string(name) == "tail2";
        }
        CHECK(head && chest && belly && tail);
    }
}

TEST(touches_pick_the_nearest_capsule) {
    ScreenCapsule caps[2];
    caps[0] = {{100, 100}, {200, 100}, 30, 5.0f};  // a horizontal capsule, further away
    caps[1] = {{150, 60}, {150, 160}, 20, 2.0f};   // a vertical one crossing it, nearer
    float t = 0, across = 0;
    CHECK(pickCapsule(caps, 2, {150, 110}, t, across) == 1);  // inside both: the nearer wins
    CHECK(std::fabs(t - 0.5f) < 0.01f);
    CHECK(pickCapsule(caps, 2, {110, 100}, t, across) == 0);
    CHECK(std::fabs(t - 0.1f) < 0.01f && std::fabs(across) < 0.01f);
    CHECK(pickCapsule(caps, 2, {110, 120}, t, across) == 0 && across > 0.6f);   // below the axis
    CHECK(pickCapsule(caps, 2, {110, 80}, t, across) == 0 && across < -0.6f);   // above it
    CHECK(pickCapsule(caps, 2, {300, 300}, t, across) == -1);
}

TEST(zones_and_regions_follow_the_body) {
    const Vec3 up{0, 0, 1}, down{0, 0, -1}, front{0, -1, 0}, left{-1, 0, 0}, right{1, 0, 0};
    CHECK(zoneOf("head", up, 0.5f) == PetZone::Head);
    CHECK(zoneOf("head", down, 0.5f) == PetZone::Chin);
    CHECK(zoneOf("head", left, 0.5f) == PetZone::Cheek);
    CHECK(zoneOf("jaw", front, 0.5f) == PetZone::Chin);
    CHECK(zoneOf("neck2", up, 0.5f) == PetZone::Neck);
    CHECK(zoneOf("neck2", down, 0.5f) == PetZone::Chin);
    CHECK(zoneOf("chest", front, 0.3f) == PetZone::Heart);
    CHECK(zoneOf("belly", down, 0.5f) == PetZone::Belly);
    CHECK(zoneOf("belly", up, 0.5f) == PetZone::Back);
    CHECK(zoneOf("tail3", right, 0.5f) == PetZone::Tail);
    CHECK(zoneOf("leg_up_L", left, 0.5f) == PetZone::Paw);
    CHECK(regionOf("snout", up) == kRegionHead);
    CHECK(regionOf("neck1", up) == kRegionNeck);
    CHECK(regionOf("hips", up) == kRegionBack);
    CHECK(regionOf("hips", down) == kRegionBelly);
    CHECK(regionOf("belly", left) == kRegionLeft && regionOf("belly", right) == kRegionRight);
    CHECK(regionOf("foot_R", down) == kRegionRight && regionOf("hand_L", up) == kRegionLeft);
    CHECK(regionOf("tail4", up) == kRegionTail);
}

TEST(strokes_are_named_by_how_they_move) {
    const float dt = 1.0f / 30;
    StrokeTracker s;
    s.begin({50, 100});  // slow and straight
    Stroke k = Stroke::None;
    for (int f = 1; f <= 30; ++f) k = s.update({50.0f + f * 4.0f, 100}, dt);
    CHECK(k == Stroke::Gentle);
    s.begin({100, 100});  // quick back-and-forth
    for (int f = 1; f <= 24; ++f) k = s.update({100.0f + ((f / 3) % 2 ? 12.0f : -12.0f) + (f % 3) * 4.0f, 100}, dt);
    CHECK(k == Stroke::Scrub);
    s.begin({160, 120});  // small circles
    for (int f = 1; f <= 45; ++f) {
        const float a = f * 0.55f;
        k = s.update({160.0f + 10.0f * std::cos(a), 120.0f + 10.0f * std::sin(a)}, dt);
    }
    CHECK(k == Stroke::Scratch);
    s.begin({10, 10});  // a tap
    s.update({11, 10}, dt);
    CHECK(s.end() == Stroke::Poke);
    s.begin({0, 100});  // far too hard
    for (int f = 1; f <= 10; ++f) k = s.update({f * 45.0f, 100}, dt);
    CHECK(k == Stroke::Rough);
}

TEST(each_dragon_has_its_own_quirks) {
    int zones[10] = {};
    int dislikes = 0, twoDislikes = 0;
    for (u32 id = 1; id <= 300; ++id) {
        const Dragon d = dragonWithId(id, static_cast<Element>(id % kElementCount));
        const SweetSpot s = sweetSpotOf(d);
        ++zones[static_cast<int>(s.zone)];
        CHECK(sweetSpotOf(d).zone == s.zone && sweetSpotOf(d).side == s.side);  // stable
        int n = 0;
        for (int f = 0; f < static_cast<int>(Food::Count); ++f) {
            const Taste t = tasteOf(d, static_cast<Food>(f));
            if (f == d.favoriteFood) CHECK(t == Taste::Favorite);
            if (f >= kElementCount) CHECK(t != Taste::Disliked);
            n += t == Taste::Disliked;
        }
        CHECK(n >= 1 && n <= 2);
        dislikes += n;
        twoDislikes += n == 2;
    }
    for (PetZone z : {PetZone::Head, PetZone::Cheek, PetZone::Chin, PetZone::Neck, PetZone::Back, PetZone::Belly,
                      PetZone::Tail})
        CHECK(zones[static_cast<int>(z)] > 10);
    std::printf("  300 dragons: %d dislikes, %d with two\n", dislikes, twoDislikes);
    CHECK(twoDislikes > 30 && twoDislikes < 200);
    CHECK(bathMoodOf(dragonWithId(3, Element::Tide)) == BathMood::Loves);
    CHECK(bathMoodOf(dragonWithId(3, Element::Ember)) == BathMood::Grudging);
    CHECK(bathMoodOf(dragonWithId(3, Element::Gale)) == BathMood::Fine);
}

// Grooming, simpler (D83): the brush pleases more than the hand (Play and bond), its liked zone
// half as much again; only the bath cleans, and it fills Clean.
TEST(brushing_pleases_and_the_bath_cleans) {
    Dragon d = dragonWithId(7);
    d.stage = Stage::Hatchling;
    Dragon e = d;
    d.needs.play = e.needs.play = 20;
    for (int k = 0; k < 4; ++k) pet(d, 3.0f);     // a second of the hand (0.25 s ticks)
    for (int k = 0; k < 5; ++k) brushed(e, 4.0f); // a second of the brush (0.2 s ticks)
    CHECK(e.needs.play > d.needs.play && e.bond > d.bond);
    for (float& dust : e.dirt) dust = 60;
    e.needs.clean = 30;
    brushed(e, 4.0f);
    CHECK(e.dirt[kRegionBack] == 60.0f && e.needs.clean == 30.0f);  // brushing doesn't clean
    bathe(e);
    CHECK(e.dirt[kRegionBack] == 0.0f && e.needs.clean == 100.0f);
    int differ = 0;
    for (u32 id = 1; id <= 200; ++id) {
        const Dragon x = dragonWithId(id);
        const PetZone liked = likedZoneOf(x);
        CHECK(liked != sweetSpotOf(x).zone);
        CHECK(zoneLiking(x, liked) == 1.5f && zoneLiking(x, liked == PetZone::Head ? PetZone::Tail : PetZone::Head) == 1.0f);
        differ += liked != likedZoneOf(dragonWithId(id + 1));
    }
    CHECK(differ > 100);
}

TEST(a_thrown_ball_bounces_rolls_and_rests) {
    const DenLayout den;
    Ball b;
    b.launch({den.home.x, den.home.y - 4.0f, 1.2f}, {0.4f, 5.0f, 3.0f});
    int bounces = 0;
    float t = 0;
    bool rested = false;
    while (t < 20.0f && !rested) {
        const BallEvent e = b.step(den, 1.0f / 60);
        bounces += e == BallEvent::Bounce;
        rested = e == BallEvent::Rest;
        t += 1.0f / 60;
        CHECK(b.pos.z >= b.radius - 1e-4f);
        const float dx = b.pos.x - den.home.x, dy = b.pos.y - den.home.y;
        CHECK(std::sqrt(dx * dx + dy * dy) <= den.radius + 1e-3f);
    }
    std::printf("  thrown ball: %d bounces, rests after %.1f s\n", bounces, t);
    CHECK(rested && bounces >= 3 && t > 1.5f);

    // The rug is softer than the stone: a ball dropped on it bounces lower.
    auto firstBounceHeight = [&](Vec2 at) {
        Ball d;
        d.launch({at.x, at.y, 2.0f}, {0, 0, 0});
        bool bounced = false;
        float peak = 0;
        for (int i = 0; i < 600; ++i) {
            if (d.step(den, 1.0f / 120) == BallEvent::Bounce) {
                if (bounced) break;
                bounced = true;
            }
            if (bounced) peak = std::fmax(peak, d.pos.z);
        }
        return peak;
    };
    const float rug = firstBounceHeight(den.home), stone = firstBounceHeight({den.home.x + 4.5f, den.home.y - 2.0f});
    std::printf("  first bounce: rug %.2f, stone %.2f\n", rug, stone);
    CHECK(rug < stone * 0.8f);

    // Held, it stays put; released, it rolls.
    b.held = true;
    const Vec3 before = b.pos;
    b.step(den, 0.5f);
    CHECK(b.pos.x == before.x && b.pos.y == before.y);
    b.release({den.home.x, den.home.y, 0.3f}, {0, -1.0f, 0});
    CHECK(!b.held && !b.resting);
}

void runCareTests() {
    RUN(capsules_cover_the_body);
    RUN(touches_pick_the_nearest_capsule);
    RUN(zones_and_regions_follow_the_body);
    RUN(strokes_are_named_by_how_they_move);
    RUN(each_dragon_has_its_own_quirks);
    RUN(brushing_pleases_and_the_bath_cleans);
    RUN(a_thrown_ball_bounces_rolls_and_rests);
}
