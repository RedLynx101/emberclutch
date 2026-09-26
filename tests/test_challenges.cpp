// Beta's challenges (core/challenges, core/challenge_mesh): who may enter which cup, what a run
// wins, the Sky Rings courses (every one flown through by the steady pilot, the Ember cup's up to
// the isles' high lantern), ring judging and ghosts, the Lantern Trial's patterns, the fruit's
// flight and the catch (the goals within a good thrower's reach), the breath, and the meshes.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "check.hpp"
#include "core/challenge_mesh.hpp"
#include "core/challenges.hpp"
#include "core/place_layout.hpp"
#include "core/save.hpp"
#include "core/valley.hpp"

using namespace ec;
using namespace ec::challenge;

namespace {

const Valley& valley() {
    static Valley v;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        FILE* f = std::fopen("../romfs/valley/skyreach.evl", "rb");
        if (!f) return v;
        std::vector<u8> data;
        u8 buf[4096];
        std::size_t n;
        while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) data.insert(data.end(), buf, buf + n);
        std::fclose(f);
        loadValley(data.data(), data.size(), v);
    }
    return v;
}

Dragon dragonAt(Stage stage) {
    Dragon d;
    d.id = 3;
    d.stage = stage;
    for (u8& s : d.stats) s = 5;
    return d;
}

}  // namespace

// The cups open one after another; Sky Rings wants a grown partner to ride, the Blaze cups a
// juvenile or older, the Starfire cups a grown dragon.
TEST(challenge_cups_and_entry) {
    static SaveData s;
    s = SaveData{};
    const Dragon baby = dragonAt(Stage::Hatchling), juvenile = dragonAt(Stage::Juvenile), grown = dragonAt(Stage::Adult);
    CHECK(entry(s, &baby, Challenge::FruitCatch, kEmber) == Entry::Open);
    CHECK(entry(s, &baby, Challenge::LanternTrial, kEmber) == Entry::Open);
    CHECK(entry(s, nullptr, Challenge::FruitCatch, kEmber) == Entry::NoPartner);
    CHECK(entry(s, &baby, Challenge::SkyRings, kEmber) == Entry::NotGrown);
    CHECK(entry(s, &grown, Challenge::SkyRings, kEmber) == Entry::Open);
    CHECK(entry(s, &grown, Challenge::FruitCatch, kFlame) == Entry::WinBefore);
    s.world.cups[static_cast<int>(Challenge::FruitCatch)] = kFlame;
    CHECK(entry(s, &baby, Challenge::FruitCatch, kBlaze) == Entry::TooYoung);
    CHECK(entry(s, &juvenile, Challenge::FruitCatch, kBlaze) == Entry::Open);
    CHECK(entry(s, &juvenile, Challenge::FruitCatch, kStarfire) == Entry::WinBefore);
    s.world.cups[static_cast<int>(Challenge::FruitCatch)] = kBlaze;
    CHECK(entry(s, &juvenile, Challenge::FruitCatch, kStarfire) == Entry::NotGrown);
    CHECK(entry(s, &grown, Challenge::FruitCatch, kStarfire) == Entry::Open);
    CHECK(entry(s, &grown, Challenge::FruitCatch, kEmber) == Entry::Open);  // an old cup, run again
    CHECK(young(baby) && young(juvenile) && !young(grown) && !young(dragonAt(Stage::Adolescent)));
    CHECK(placeOf(Challenge::FruitCatch) == kPlaceOrchard && placeOf(Challenge::SkyRings) == kPlaceArena);
    for (int c = kEmber; c <= kStarfire; ++c) CHECK(cupName(c)[0] != 0);
    CHECK(entryText(Entry::Open)[0] == 0 && entryText(Entry::NotGrown)[0] != 0);
}

// A win takes the cup (never lowering it), its ribbon once and the Gleam; winning it again pays
// less; placing and trying pay a little. Bests: the lowest time for Sky Rings, the most points
// for the others; the save keeps them.
TEST(challenge_rewards_and_bests) {
    static SaveData s;
    s = SaveData{};
    s.gleam = 0;
    Reward r = record(s, Challenge::FruitCatch, kEmber, Outcome::Won, 520);
    CHECK(r.firstWin && r.best && r.gleam == 60 && s.gleam == 60);
    CHECK(s.world.cups[static_cast<int>(Challenge::FruitCatch)] == kEmber && ribbon(s, Challenge::FruitCatch, kEmber));
    CHECK(best(s, Challenge::FruitCatch, kEmber) == 520 && ribbonCount(s) == 1);
    r = record(s, Challenge::FruitCatch, kEmber, Outcome::Won, 480);
    CHECK(!r.firstWin && !r.best && r.gleam == 20 && best(s, Challenge::FruitCatch, kEmber) == 520 && ribbonCount(s) == 1);
    r = record(s, Challenge::FruitCatch, kFlame, Outcome::Placed, 600);
    CHECK(r.gleam == 15 && s.world.cups[static_cast<int>(Challenge::FruitCatch)] == kEmber && r.best);
    r = record(s, Challenge::FruitCatch, kFlame, Outcome::Won, 760);
    CHECK(r.firstWin && s.world.cups[static_cast<int>(Challenge::FruitCatch)] == kFlame && ribbonCount(s) == 2);
    record(s, Challenge::FruitCatch, kEmber, Outcome::Won, 530);  // an old cup won again: the cup stays Flame
    CHECK(s.world.cups[static_cast<int>(Challenge::FruitCatch)] == kFlame);
    // Sky Rings: tenths of a second, lower is better; an unfinished run keeps no time.
    r = record(s, Challenge::SkyRings, kEmber, Outcome::TryAgain, 0);
    CHECK(!r.best && r.gleam == 5 && best(s, Challenge::SkyRings, kEmber) == 0);
    r = record(s, Challenge::SkyRings, kEmber, Outcome::Placed, 812);
    CHECK(r.best && best(s, Challenge::SkyRings, kEmber) == 812);
    r = record(s, Challenge::SkyRings, kEmber, Outcome::Won, 640);
    CHECK(r.best && r.firstWin && best(s, Challenge::SkyRings, kEmber) == 640);
    CHECK(!record(s, Challenge::SkyRings, kEmber, Outcome::Won, 700).best);
    CHECK(ribbon(s, Challenge::SkyRings, kEmber) && !ribbon(s, Challenge::SkyRings, kFlame) && ribbonCount(s) == 3);
    std::vector<u8> buf(maxEncodedSize());
    const std::size_t n = encodeSave(s, 1, 1000, buf.data(), buf.size());
    static SaveData out;
    CHECK(decodeSave(buf.data(), n, out) == LoadResult::Ok);
    CHECK(std::memcmp(out.world.best, s.world.best, sizeof(s.world.best)) == 0 && out.world.ribbons == s.world.ribbons);
}

// Every cup's course over the real valley: rings up in the air (clear of the ground and of the
// floating isles), longer, smaller and more of them cup by cup, each flown through by the steady
// pilot inside par; the Ember cup's last ring at the isles' high lantern. A faster Wing flies it
// faster.
TEST(sky_rings_courses) {
    const Valley& v = valley();
    int lastRings = 0;
    float lastRadius = 1e9f;
    for (int cup = kEmber; cup <= kStarfire; ++cup) {
        Course c;
        CHECK(makeCourse(v, cup, c));
        CHECK(static_cast<int>(c.rings.size()) >= lastRings && c.rings[0].radius < lastRadius);
        lastRings = static_cast<int>(c.rings.size());
        lastRadius = c.rings[0].radius;
        for (const Ring& r : c.rings) {
            CHECK(r.at.z - r.radius > std::fmax(v.heightAt(r.at.x, r.at.y), v.water) + 2.0f);
            CHECK(std::fabs(length(r.normal) - 1) < 1e-3f);
            if (!c.toIsles)
                for (const ValleyIsland& isl : v.islands) {
                    const float d = std::hypot(r.at.x - isl.at.x, r.at.y - isl.at.y);
                    CHECK(d > isl.radius + 10 || r.at.z > isl.at.z + 12 || r.at.z < isl.at.z - isl.radius * 2.2f);
                }
        }
        int missed = 0;
        Ghost ghost;
        const float t = pilotTime(v, c, courseTuning(5, 5), &missed, &ghost);
        const float fast = pilotTime(v, c, courseTuning(9, 5)), slow = pilotTime(v, c, courseTuning(2, 5));
        std::printf("  %s: %d rings, %.0f m, par %.1f s; the pilot %.1f s (%d missed), Wing 9 %.1f s, Wing 2 %.1f s\n",
                    cupName(cup), static_cast<int>(c.rings.size()), c.length, c.par, t, missed, fast, slow);
        CHECK(t > 0 && missed == 0 && t < c.par && c.par < t * 1.5f);
        CHECK(fast > 0 && fast < t && (slow < 0 || slow > t));
        CHECK(ghost.duration() > t * 0.8f);
        if (cup == kEmber) {
            CHECK(c.toIsles);
            const ValleyPlaceInfo& isles = *v.place(kPlaceIsles);
            const PlaceLayout& L = placeLayout(kPlaceIsles);
            const Vec2 lan = placeToWorld(isles, {L.lantern.x, L.lantern.y});
            const Ring& last = c.rings.back();
            CHECK(std::hypot(last.at.x - lan.x, last.at.y - lan.y) < 12 && last.at.z > isles.at.z);
        }
    }
}

// Rings are judged as you go through their plane the right way: inside, passed; outside, missed;
// one skipped wide with the next flown through, both counted and on from there.
TEST(sky_rings_judging_and_ghosts) {
    Course c;
    c.par = 20;
    for (int k = 0; k < 4; ++k) c.rings.push_back({{0, -30.0f * (k + 1), 50}, {0, -1, 0}, 5});
    RingRun run;
    const float dt = 1.0f / 30;
    auto fly = [&](Vec3 from, Vec3 to) { return run.step(c, from, to, dt); };
    CHECK(fly({0, -25, 50}, {0, -31, 50}) == RingRun::kPassed && run.next == 1);
    CHECK(fly({0, -40, 50}, {0, -45, 50}) == RingRun::kNothing);          // not there yet
    CHECK(fly({0, -58, 58}, {0, -61, 58}) == RingRun::kMissed && run.next == 2);  // over the top
    CHECK(fly({80, -85, 50}, {80, -95, 50}) == RingRun::kNothing);        // far wide of ring 3: not judged yet...
    CHECK(fly({1, -118, 50}, {1, -122, 50}) == RingRun::kPassed && run.finished);  // ...the last flown: 3 skipped
    CHECK(run.passed == 2 && run.missed == 2 && std::fabs(run.total() - (run.time + 2 * kMissPenalty)) < 1e-4f);
    CHECK(ringsOutcome(c, run) == Outcome::Won);
    run.time = c.par - 2 * kMissPenalty + 1;  // a second over par, misses and all
    CHECK(ringsOutcome(c, run) == Outcome::Placed);
    run.time = c.par * 2;
    CHECK(ringsOutcome(c, run) == Outcome::TryAgain);
    // A ghost keeps a point every step and plays them back smoothly; its file round-trips.
    Ghost g;
    for (int f = 0; f <= 90; ++f) g.record(f * dt, {f * 0.5f, -f * 1.0f, 40 + f * 0.1f}, 0.01f * f);
    CHECK(g.points.size() == 16 && std::fabs(g.duration() - 3.0f) < 1e-3f);
    Vec3 p;
    float h;
    CHECK(g.at(1.5f, p, h) && std::fabs(p.x - 22.5f) < 0.3f && std::fabs(p.y + 45) < 0.6f);
    CHECK(!g.at(3.5f, p, h) && !g.at(-0.1f, p, h));
    std::vector<u8> bytes(ghostBytes(g));
    CHECK(encodeGhost(g, bytes.data(), bytes.size()) == bytes.size());
    Ghost back;
    CHECK(decodeGhost(bytes.data(), bytes.size(), back) && back.points.size() == g.points.size());
    for (std::size_t k = 0; k < g.points.size(); ++k)
        CHECK(length(back.points[k].at - g.points[k].at) < 0.1f && std::fabs(back.points[k].heading - g.points[k].heading) < 1e-3f);
    CHECK(!decodeGhost(bytes.data(), 5, back) && !decodeGhost(bytes.data(), bytes.size() - 1, back));
}

// The Lantern Trial: a pattern that grows a lantern a round (never the same one twice running);
// lighting it in order clears the round, a wrong lantern costs a heart and shows the round again;
// out of hearts, it's over. Placed for half the rounds.
TEST(lantern_trial) {
    for (int cup = kEmber; cup <= kStarfire; ++cup) {
        const TrialSetup ts = trialSetup(cup);
        CHECK(ts.lanterns >= 4 && ts.lanterns <= kTrialLanterns && ts.first + ts.rounds - 1 <= kTrialLongest);
        Trial t;
        t.begin(cup, 1234u + cup);
        for (int i = 1; i < ts.first + ts.rounds - 1; ++i) CHECK(t.pattern[i] != t.pattern[i - 1] && t.pattern[i] < ts.lanterns);
        for (int k = 0; k < ts.lanterns; ++k) {  // in an arc before the stage, inside the arena's wall
            const Vec2 at = trialLantern(ts.lanterns, k);
            CHECK(std::hypot(at.x, at.y) < 14 && at.y < 0);
        }
        // Played right all the way through.
        Trial::Press last = Trial::Press::Right;
        while (!t.over) {
            const int len = t.length();
            for (int i = 0; i < len && !t.over; ++i) last = t.press(t.pattern[i]);
        }
        CHECK(t.won && last == Trial::Press::Won && t.roundsCleared() == ts.rounds && trialOutcome(t) == Outcome::Won);
        CHECK(t.hearts == ts.hearts && t.score > 0);
    }
    Trial t;
    t.begin(kEmber, 99);
    CHECK(t.press(t.pattern[0]) == Trial::Press::Right);
    CHECK(t.press((t.pattern[1] + 1) % 4) == Trial::Press::Wrong && t.input == 0 && t.hearts == 2);
    CHECK(t.press(t.pattern[0]) == Trial::Press::Right && t.press(t.pattern[1]) == Trial::Press::RoundDone && t.round == 1);
    CHECK(t.press((t.pattern[0] + 1) % 4) == Trial::Press::Wrong);
    CHECK(t.press((t.pattern[0] + 1) % 4) == Trial::Press::Lost && t.over && !t.won);
    CHECK(trialOutcome(t) == Outcome::TryAgain);  // one round of four
    t.begin(kEmber, 99);
    t.round = 2;
    t.over = true;
    CHECK(trialOutcome(t) == Outcome::Placed);
    CHECK(trialPitch(0) < trialPitch(5));
}

// Fruit Catch: a flick throws the fruit away from you (soft flicks and backward ones don't); your
// dragon runs, leaps and dives for it; points for distance and style; the goals a good thrower
// reaches and a careless one doesn't, grown or young (the hop version).
TEST(fruit_catch) {
    const Vec3 hand{0, 0, 1.2f}, dog{2.2f, -0.8f, 0};
    Toss t;
    CHECK(!tossFrom(0, -120, hand, 0, false, t));   // too soft
    CHECK(!tossFrom(0, 900, hand, 0, false, t));    // toward you
    CHECK(tossFrom(0, -1200, hand, 0, false, t) && t.vel.y < 0 && t.vel.z > 0);  // heading 0 throws toward -Y
    CHECK(tossFrom(600, -1000, hand, 0, false, t) && t.vel.x < 0);               // flicked right: to your right (-X, facing -Y)
    const float land = landTime(t, 0);
    CHECK(std::fabs(fruitAt(t, land).z) < 1e-3f);
    const Dragon grown = dragonAt(Stage::Adult), baby = dragonAt(Stage::Juvenile);
    const Catcher big = catcherFor(grown, dog, 6.5f, 1.0f), small = catcherFor(baby, dog, 3.0f, 0.55f);
    CHECK(big.run > small.run && big.leap > small.leap && !big.young && small.young);
    auto plan = [&](float power, bool youngOne, const Catcher& c) {
        Toss toss;
        tossFrom(0, -(250 + 1500 * power), hand, 0, youngOne, toss);
        return planCatch(toss, c, 0);
    };
    const CatchPlan soft = plan(0.1f, false, big), far = plan(1.0f, false, big);
    CHECK(soft.style == Style::Leap || soft.style == Style::SkyLeap || soft.style == Style::Snap);
    CHECK(far.style == Style::Missed && far.distance > 25);
    // Points grow with distance; a dive's worth the most; golden fruit doubles it.
    int bestPower = 0, bestPoints = 0, lastPoints = 0;
    for (int p = 0; p <= 20; ++p) {
        const CatchPlan cp = plan(p / 20.0f, false, big);
        const int pts = catchPoints(cp, false, false);
        if (pts > bestPoints) bestPoints = pts, bestPower = p;
        if (cp.style != Style::Missed) CHECK(pts >= lastPoints - 30);
        if (cp.style != Style::Missed) lastPoints = pts;
    }
    const CatchPlan top = plan(bestPower / 20.0f, false, big);
    CHECK(catchPoints(top, true, false) == 2 * bestPoints);
    // A round of eight at the best power (two golden) against each grown cup's goal, and a
    // careless round (everything thrown as hard as possible) that shouldn't win anything.
    const int goodGrown = bestPoints * 6 + bestPoints * 2 * 2;
    int youngBest = 0;
    for (int p = 0; p <= 20; ++p) youngBest = std::max(youngBest, catchPoints(plan(p / 20.0f, true, small), false, true));
    const int goodYoung = youngBest * 10;
    std::printf("  grown: best throw %d pts (power %.2f, %s), a good round %d; young: best %d, a good round %d\n", bestPoints,
                bestPower / 20.0f, styleName(top.style), goodGrown, youngBest, goodYoung);
    for (int cup = kEmber; cup <= kStarfire; ++cup) {
        const FruitSetup g = fruitSetup(cup, false), y = fruitSetup(cup, true);
        std::printf("  %s: grown goal %d (placed %d), young goal %d\n", cupName(cup), g.goal, g.placed, y.goal);
        CHECK(g.throws == 8 && g.goal < goodGrown && g.placed < g.goal);
        if (cup < kStarfire) CHECK(y.goal < goodYoung);
        CHECK(fruitOutcome(g, g.goal) == Outcome::Won && fruitOutcome(g, g.placed) == Outcome::Placed &&
              fruitOutcome(g, g.placed - 1) == Outcome::TryAgain);
    }
    CHECK(catchPoints(far, false, false) == 0 && fruitOutcome(fruitSetup(kEmber, false), 0) == Outcome::TryAgain);
    CHECK(fruitFor(3, 7) == Fruit::Golden && fruitFor(7, 7) == Fruit::Golden && fruitFor(0, 7) != Fruit::Golden);
    // The young version: shorter throws, hops.
    const CatchPlan hop = plan(0.2f, true, small);
    CHECK(hop.style != Style::Missed && hop.distance < 8);
}

// Breath: each element's own; a stream of puffs from the mouth that reaches what it's breathed at.
TEST(breath_streams) {
    for (int e = 0; e < 8; ++e) {
        const BreathLook l = breathFor(e);
        CHECK(l.size0 > 0 && l.size1 > 0 && l.spread >= 0);
    }
    CHECK(breathFor(0).kind == Breath::Flame && breathFor(4).kind == Breath::Mist && breathFor(3).kind == Breath::Gust &&
          breathFor(1).kind == Breath::Spores && breathFor(5).kind == Breath::Frost && breathFor(6).kind == Breath::Light);
    BreathFx fx;
    const Vec3 from{0, 0, 1}, to{0, -10, 1.5f};
    fx.emit(breathFor(0), from, to, 30, 0.5f);
    CHECK(fx.count() == 30);
    float nearest = 1e9f;
    for (int f = 0; f < 18; ++f) {
        fx.update(1.0f / 30);
        for (int i = 0; i < fx.count(); ++i) nearest = std::fmin(nearest, length(fx[i].pos - to));
    }
    CHECK(nearest < 2.5f);
    for (int f = 0; f < 60; ++f) fx.update(1.0f / 30);
    CHECK(fx.count() == 0);
    for (int k = 0; k < 10; ++k) fx.emit(breathFor(6), from, to, 20, 0.5f);
    CHECK(fx.count() == BreathFx::kMax);
}

// The meshes: sound (indices in range, unit normals) and light (the budget: a course shows a few
// rings at once, the trial six lanterns). The trophies and ribbons sit on the den's shelves.
TEST(challenge_meshes) {
    auto sound = [](const PropMesh& m, int budget) {
        CHECK(!m.idx.empty() && m.idx.size() % 3 == 0 && m.triangles() <= budget);
        CHECK(m.pos.size() == m.nrm.size() && m.paint.size() == m.pos.size() * 4);
        for (u16 i : m.idx) CHECK(i < m.pos.size());
        for (const Vec3& n : m.nrm) CHECK(std::fabs(length(n) - 1) < 1e-3f);
    };
    sound(ringMesh(), 320);
    sound(crystalLanternMesh(), 140);
    for (int f = 0; f < static_cast<int>(Fruit::Count); ++f) sound(fruitMesh(static_cast<Fruit>(f)), 170);
    sound(basketMesh(), 200);
    sound(boardMesh(), 140);
    for (int c = 0; c < kChallenges; ++c) sound(trophyMesh(static_cast<Challenge>(c)), 140);
    sound(rosetteMesh(), 30);
    // Everything won, on the den's shelves: one draw, light enough for a full den.
    const u8 all[kChallenges] = {kStarfire, kStarfire, kStarfire};
    sound(shelfMesh(all, 0x0FFF), 600);
    const u8 some[kChallenges] = {kEmber, 0, kFlame};
    CHECK(shelfMesh(some, 0x0301).triangles() < shelfMesh(all, 0x0FFF).triangles());
    const u8 none[kChallenges] = {};
    CHECK(shelfMesh(none, 0).triangles() == 0);
    Rgb pal[kPalCount];
    float glow[kPalCount];
    shelfPalette(pal, glow);
    CHECK(pal[kStarfire - 1].b > 200 && glow[4 + static_cast<int>(Challenge::LanternTrial)] > 0);
    std::printf("  triangles: ring %d, lantern %d, fruit %d, basket %d, board %d, trophy %d, rosette %d\n", ringMesh().triangles(),
                crystalLanternMesh().triangles(), fruitMesh(Fruit::Apple).triangles(), basketMesh().triangles(),
                boardMesh().triangles(), trophyMesh(Challenge::SkyRings).triangles(), rosetteMesh().triangles());
    for (int c = 0; c < kChallenges; ++c) {
        const DecorPlace p = trophySpot(static_cast<Challenge>(c));
        CHECK(std::hypot(p.at.x, p.at.y) > 8.6f && std::hypot(p.at.x, p.at.y) < 9.5f && p.at.z > 1.5f && p.at.z < 2.8f);
    }
    for (int k = 0; k < kChallenges * kCups; ++k) {
        const DecorPlace p = ribbonSpot(k);
        CHECK(std::hypot(p.at.x, p.at.y) > 8.6f && std::hypot(p.at.x, p.at.y) < 9.3f);
        for (int j = 0; j < k; ++j) CHECK(length(ribbonSpot(j).at - p.at) > 0.3f);
    }
}

void runChallengeTests() {
    RUN(challenge_cups_and_entry);
    RUN(challenge_rewards_and_bests);
    RUN(sky_rings_courses);
    RUN(sky_rings_judging_and_ghosts);
    RUN(lantern_trial);
    RUN(fruit_catch);
    RUN(breath_streams);
    RUN(challenge_meshes);
}
