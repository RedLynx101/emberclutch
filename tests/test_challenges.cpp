// The challenges (core/challenges, core/challenge_mesh; Beta, retuned for 1.0, D89): who may enter
// which cup (and with the energy for it), what a run wins (once a day per cup, the dragon's own
// record), the Sky Rings courses (every one flown through by the steady pilot, the Ember cup's up
// to the isles' high lantern), the race's flight (momentum, the burst, the brake, the Stamina
// meter), the rivals against the pilots cup by cup, ring judging and ghosts, the Lantern Trial's
// patterns, the fruit's flight in the wind and the catch (the goals within a good thrower's
// reach, the stats' part), the breath, and the meshes.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "check.hpp"
#include "core/challenge_mesh.hpp"
#include "core/challenges.hpp"
#include "core/place_layout.hpp"
#include "core/save.hpp"
#include "core/trainer.hpp"
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
    // Too tired (D89): a cup costs trainer::kEnergyChallenge; below it, it can't enter.
    Dragon tired = grown;
    tired.needs.energy = trainer::kEnergyChallenge - 1;
    CHECK(entry(s, &tired, Challenge::FruitCatch, kEmber) == Entry::Tired && entryText(Entry::Tired)[0] != 0);
    tired.needs.energy = trainer::kEnergyChallenge;
    CHECK(entry(s, &tired, Challenge::FruitCatch, kEmber) == Entry::Open);
    // What each cup asks.
    CHECK(cupNeeds(Challenge::SkyRings, kEmber, false).rivals == 2 && cupNeeds(Challenge::SkyRings, kStarfire, false).rivals == 3);
    CHECK(cupNeeds(Challenge::SkyRings, kEmber, false).grown && cupNeeds(Challenge::FruitCatch, kBlaze, true).juvenile);
    CHECK(cupNeeds(Challenge::FruitCatch, kFlame, false).goal == fruitSetup(kFlame, false).goal);
    CHECK(cupNeeds(Challenge::LanternTrial, kStarfire, false).lanterns == trialSetup(kStarfire).lanterns);
    // The stats: an average dragon's 5; training counts, less past 10.
    Dragon d = grown;
    CHECK(std::fabs(statLevel(d, kStatWing) - 5) < 1e-4f);
    d.stats[kStatWing] = 8;
    d.trained[kStatWing] = 6;
    CHECK(statLevel(d, kStatWing) > 10 && statLevel(d, kStatWing) < 12);
    CHECK(statEdge(5) == 0 && statEdge(1) < 0 && statEdge(40) <= 1.4f);
}

// A cup's first win takes the cup (never lowering it), its ribbon and its first prize; after that
// a win pays the day's prize once a day per cup (again that day: nothing, and says so); placing
// pays a little while the day's prize is still to win; not this time pays nothing. Experience
// and the partner's own record go with it. Bests: the lowest time for Sky Rings, the most points
// for the others; the save keeps them.
TEST(challenge_rewards_and_bests) {
    static SaveData s;
    s = SaveData{};
    s.gleam = 0;
    const s32 day = 20000;
    Dragon cinder = dragonAt(Stage::Adult);
    Reward r = record(s, Challenge::FruitCatch, kEmber, Outcome::Won, 520, day, &cinder);
    CHECK(r.firstWin && r.best && r.gleam == firstPrize(kEmber) && s.gleam == firstPrize(kEmber) && !r.paidToday);
    CHECK(r.dragonFirst && trainer::wonCup(cinder, static_cast<int>(Challenge::FruitCatch), kEmber) && r.xp == winXp(kEmber));
    CHECK(cinder.xp == winXp(kEmber) && s.progress.counts[kCountCups] == 1);
    CHECK(s.world.cups[static_cast<int>(Challenge::FruitCatch)] == kEmber && ribbon(s, Challenge::FruitCatch, kEmber));
    CHECK(best(s, Challenge::FruitCatch, kEmber) == 520 && ribbonCount(s) == 1);
    r = record(s, Challenge::FruitCatch, kEmber, Outcome::Won, 480, day, &cinder);  // again the same day: no prize
    CHECK(!r.firstWin && !r.best && r.gleam == 0 && r.paidToday && !r.dragonFirst && r.xp == kRunXp);
    CHECK(best(s, Challenge::FruitCatch, kEmber) == 520 && ribbonCount(s) == 1);
    r = record(s, Challenge::FruitCatch, kEmber, Outcome::Won, 530, day + 1, &cinder);  // the next day: the day's prize
    CHECK(!r.firstWin && r.gleam == dayPrize(kEmber) && !r.paidToday && r.best);
    r = record(s, Challenge::FruitCatch, kFlame, Outcome::Placed, 600, day + 1, nullptr);
    CHECK(r.gleam == placedPrize(kFlame) && s.world.cups[static_cast<int>(Challenge::FruitCatch)] == kEmber && r.best);
    r = record(s, Challenge::FruitCatch, kFlame, Outcome::Placed, 610, day + 1, nullptr);  // placing twice still pays (energy limits it)
    CHECK(r.gleam == placedPrize(kFlame));
    Dragon pip = dragonAt(Stage::Adult);  // someone else wins it: the cup's first win, pip's first too
    r = record(s, Challenge::FruitCatch, kFlame, Outcome::Won, 760, day + 1, &pip);
    CHECK(r.firstWin && r.dragonFirst && s.world.cups[static_cast<int>(Challenge::FruitCatch)] == kFlame && ribbonCount(s) == 2);
    r = record(s, Challenge::FruitCatch, kFlame, Outcome::Placed, 700, day + 1, nullptr);  // the day's prize is taken: placing pays nothing
    CHECK(r.gleam == 0);
    r = record(s, Challenge::FruitCatch, kFlame, Outcome::Won, 770, day + 2, &cinder);  // cinder's own first Flame cup
    CHECK(!r.firstWin && r.dragonFirst && r.gleam == dayPrize(kFlame));
    record(s, Challenge::FruitCatch, kEmber, Outcome::Won, 530, day + 2, nullptr);  // an old cup won again: the cup stays Flame
    CHECK(s.world.cups[static_cast<int>(Challenge::FruitCatch)] == kFlame);
    CHECK(firstPrize(kStarfire) > firstPrize(kBlaze) && dayPrize(kStarfire) > dayPrize(kEmber) && winXp(kStarfire) > winXp(kEmber));
    CHECK(claimBit(Challenge::LanternTrial, kStarfire) == kClaimCup + 11);
    // Sky Rings: tenths of a second, lower is better; an unfinished run keeps no time, and pays nothing.
    r = record(s, Challenge::SkyRings, kEmber, Outcome::TryAgain, 0, day, nullptr);
    CHECK(!r.best && r.gleam == 0 && best(s, Challenge::SkyRings, kEmber) == 0);
    r = record(s, Challenge::SkyRings, kEmber, Outcome::Placed, 812, day, nullptr);
    CHECK(r.best && best(s, Challenge::SkyRings, kEmber) == 812);
    r = record(s, Challenge::SkyRings, kEmber, Outcome::Won, 640, day, nullptr);
    CHECK(r.best && r.firstWin && best(s, Challenge::SkyRings, kEmber) == 640);
    CHECK(!record(s, Challenge::SkyRings, kEmber, Outcome::Won, 700, day, nullptr).best);
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
        int missed = 0, expertMissed = 0;
        Ghost ghost;
        const float t = pilotTime(v, c, raceTuning(5, 5), steadyPilot(), &missed, &ghost);
        const float fast = pilotTime(v, c, raceTuning(9, 5), steadyPilot()), slow = pilotTime(v, c, raceTuning(2, 5), steadyPilot());
        const float expert = pilotTime(v, c, raceTuning(5, 5), expertPilot(), &expertMissed);
        std::printf("  %s: %d rings, %.0f m, par %.1f s; the steady pilot %.1f s (%d missed), Wing 9 %.1f s, Wing 2 %.1f s; "
                    "the expert %.1f s (%d missed)\n",
                    cupName(cup), static_cast<int>(c.rings.size()), c.length, c.par, t, missed, fast, slow, expert, expertMissed);
        CHECK(t > 0 && missed == 0 && t < c.par && c.par < t * 1.5f);
        CHECK(fast > 0 && fast < t && (slow < 0 || slow > t));
        CHECK(expert > 0 && expertMissed == 0 && expert < t);  // bursting and braking well is faster
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
    CHECK(run.progress(c, {0, 0, 50}) == 4.0f);
    RingRun fresh;
    c.start = {0, 0, 50};
    CHECK(fresh.progress(c, {0, -15, 50}) > 0.4f && fresh.progress(c, {0, -15, 50}) < 0.6f);
    // The race: first place wins, second places; a tie goes to you; a rival who didn't finish is behind.
    const float rivals[3] = {50.0f, 61.0f, -1.0f};
    CHECK(racePlace(49.0f, rivals, 3) == 1 && racePlace(50.0f, rivals, 3) == 1 && racePlace(55.0f, rivals, 3) == 2);
    CHECK(racePlace(70.0f, rivals, 3) == 3);
    CHECK(raceOutcome(true, 1) == Outcome::Won && raceOutcome(true, 2) == Outcome::Placed && raceOutcome(true, 3) == Outcome::TryAgain);
    CHECK(raceOutcome(false, 1) == Outcome::TryAgain);
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

// The race's flight (D89): speed carries (it eases back to cruising slowly), a burst pushes it
// up while the Stamina meter lasts, the brake slows it and tightens its turns, hard turns bleed
// speed, climbing costs it and diving gives it; the meter's size from the dragon's Stamina, its
// speeds from its Wing.
TEST(sky_rings_flight) {
    const Valley& v = valley();
    const ValleyPlaceInfo* arena = v.place(kPlaceArena);
    CHECK(arena != nullptr);
    if (!arena) return;
    const RaceTuning t = raceTuning(5, 5);
    const float dt = 1.0f / 30.0f;
    auto fresh = [&]() {
        Flight f;
        f.pos = arena->at + Vec3{0, 0, 160};
        f.grounded = false;
        f.speed = t.cruise;
        f.stamina = 1;
        return f;
    };
    auto fly = [&](Flight& f, RaceInput in, float seconds) {
        for (float s = 0; s < seconds; s += dt) raceStep(f, in, v, dt, t);
    };
    Flight level = fresh();
    fly(level, {}, 5);
    CHECK(level.speed > t.cruise * 0.9f && level.speed < t.cruise * 1.3f && !level.grounded);
    RaceInput burst;
    burst.burst = true;
    Flight b = fresh();
    fly(b, burst, 2);
    CHECK(b.speed > t.cruise * 1.2f && b.stamina < 1.0f - 1.5f / t.meter && b.stamina > 0);
    const float peak = b.speed;
    fly(b, {}, 2);  // let go: it carries
    CHECK(b.speed > t.cruise * 1.1f && b.speed < peak);
    fly(b, burst, t.meter);  // till the meter's empty: the burst gives out
    CHECK(b.stamina < 0.02f && !bursting(b, burst) && b.speed < peak);
    const float emptied = b.stamina;
    fly(b, burst, 1);
    CHECK(b.stamina == emptied);  // held on empty: nothing
    fly(b, {}, 8);
    CHECK(b.stamina > emptied + 0.3f);  // easing off fills it
    RaceInput brake;
    brake.brake = true;
    Flight br = fresh();
    fly(br, brake, 1.5f);
    CHECK(br.speed < t.cruise * 0.7f);
    // Turning: a hard turn costs speed; braked, it turns tighter.
    RaceInput turn;
    turn.steer = 1;
    Flight straight = fresh(), turning = fresh();
    fly(straight, {}, 3);
    fly(turning, turn, 3);
    CHECK(turning.speed < straight.speed - 1.0f);
    RaceInput braking = turn;
    braking.brake = true;
    Flight a = fresh(), c = fresh();
    fly(a, turn, 1);
    fly(c, braking, 1);
    CHECK(std::fabs(c.heading) > std::fabs(a.heading) * 1.2f);
    // Climbing costs speed, diving gives it.
    RaceInput up, down;
    up.flap = true;
    down.dive = true;
    Flight u = fresh(), dn = fresh();
    fly(u, up, 3);
    fly(dn, down, 2);
    CHECK(u.speed < level.speed && u.pos.z > arena->at.z + 160 && dn.speed > t.cruise * 1.4f);
    // The stats.
    CHECK(raceTuning(5, 9).meter > t.meter && t.meter > raceTuning(5, 2).meter && raceTuning(5, 1).meter >= 2.0f);
    CHECK(raceTuning(9, 5).cruise > t.cruise && raceTuning(9, 5).top > t.top && raceTuning(2, 5).turnRate < t.turnRate);
    Dragon d = dragonAt(Stage::Adult);
    d.stats[kStatStamina] = 9;
    CHECK(raceTuning(d).meter > t.meter);
    // A lift through a ring: the meter topped up, a little speed (never slowing a dive).
    Flight l = fresh();
    l.stamina = 0.5f;
    ringLift(l, t);
    CHECK(l.stamina > 0.55f && l.speed > t.cruise);
    l.speed = t.top + 5;
    ringLift(l, t);
    CHECK(l.speed == t.top + 5);
}

// The rivals, cup by cup, against the pilots (D89: the Ember cup a steady flight wins; the
// Starfire cup needs bursts, braking and a strong dragon), over several races each.
TEST(sky_rings_rivals) {
    const Valley& v = valley();
    float lastRatio = 9;
    for (int cup = kEmber; cup <= kStarfire; ++cup) {
        Course c;
        if (!makeCourse(v, cup, c)) {
            CHECK(false);
            continue;
        }
        const float steady = pilotTime(v, c, raceTuning(5, 5), steadyPilot());
        const float expert = pilotTime(v, c, raceTuning(5, 5), expertPilot());
        const float trained = pilotTime(v, c, raceTuning(8, 8), expertPilot());
        std::vector<float> bests, each[kMaxRivals];
        for (u32 seed = 1; seed <= 7; ++seed) {
            float best = 1e9f;
            for (int k = 0; k < rivalCount(cup); ++k) {
                const RivalInfo info = rivalFor(cup, k);
                CHECK(info.name[0] != 0);
                Racer r;
                r.tune = raceTuning(info.wing, info.stamina);
                r.pilot.skill = info.skill;
                r.pilot.rng = Rng(seed * 31 + k);
                r.pilot.phase = k * 2.1f;
                r.start(c, rivalStartAt(c, k), c.heading);
                const float total = finishRace(r, v, c, c.par * kRingTimeLimit);
                CHECK(total > 0);  // every rival finishes
                best = std::fmin(best, total > 0 ? total : 1e9f);
                each[k].push_back(total);
            }
            bests.push_back(best);
        }
        std::sort(bests.begin(), bests.end());
        const float median = bests[bests.size() / 2];
        char field[128] = {};
        for (int k = 0; k < rivalCount(cup); ++k) {
            std::sort(each[k].begin(), each[k].end());
            char one[40];
            std::snprintf(one, sizeof(one), "%s %.1f ", rivalFor(cup, k).name, each[k][each[k].size() / 2]);
            std::strncat(field, one, sizeof(field) - std::strlen(field) - 1);
        }
        std::printf("  %s: %d rivals (%s), the best of them %.1f .. %.1f s (median %.1f); steady pilot %.1f, expert %.1f, "
                    "expert at Wing/Stamina 8 %.1f\n",
                    cupName(cup), rivalCount(cup), field, bests.front(), bests.back(), median, steady, expert, trained);
        const float ratio = median / steady;
        CHECK(ratio < lastRatio);  // the field faster cup by cup (against the same pilot on each course)
        lastRatio = ratio;
        if (cup == kEmber) CHECK(steady < median);  // a steady flight wins the Ember cup
        if (cup == kFlame) CHECK(expert < median);
        if (cup == kStarfire) {
            CHECK(steady > median);   // ...not the Starfire cup
            CHECK(trained < median);  // a strong dragon flown well can
        }
    }
    for (int k = 0; k < kMaxRivals; ++k)
        for (int j = 0; j < k; ++j) CHECK(length(Vec3{rivalStart(k).x, rivalStart(k).y, 0} - Vec3{rivalStart(j).x, rivalStart(j).y, 0}) > 5);
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
    auto goodRound = [&](const Catcher& c, bool youngOne) {
        int bestPts = 0;
        for (int p = 0; p <= 40; ++p) bestPts = std::max(bestPts, catchPoints(plan(p / 40.0f, youngOne, c), false, youngOne));
        return bestPts * 10;  // six throws and two golden ones, each at its best
    };
    const int goodGrown = goodRound(big, false);
    const int goodYoung = goodRound(small, true);
    Dragon strong = grown;  // a quick, clever dragon (Wing and Wit 8): it reaches further
    strong.stats[kStatWing] = strong.stats[kStatWit] = 8;
    const int goodStrong = goodRound(catcherFor(strong, dog, 6.5f, 1.0f), false);
    std::printf("  grown: best throw %d pts (power %.2f, %s), a good round %d (Wing and Wit 8: %d); young: a good round %d\n",
                bestPoints, bestPower / 20.0f, styleName(top.style), goodGrown, goodStrong, goodYoung);
    CHECK(goodStrong > goodGrown);
    for (int cup = kEmber; cup <= kStarfire; ++cup) {
        const FruitSetup g = fruitSetup(cup, false), y = fruitSetup(cup, true);
        std::printf("  %s: grown goal %d (placed %d, %.0f%% of a perfect round), young goal %d (%.0f%%)\n", cupName(cup), g.goal,
                    g.placed, 100.0f * g.goal / goodGrown, y.goal, 100.0f * y.goal / goodYoung);
        CHECK(g.throws == 8 && g.goal < goodGrown && g.placed < g.goal);
        if (cup < kStarfire) CHECK(y.goal < goodYoung);
        CHECK(fruitOutcome(g, g.goal) == Outcome::Won && fruitOutcome(g, g.placed) == Outcome::Placed &&
              fruitOutcome(g, g.placed - 1) == Outcome::TryAgain);
    }
    // D89: the Ember cup is a start (under half a perfect round); the Starfire cup wants nearly
    // every throw at its best (over four fifths of one, for an average dragon), a strong one less.
    CHECK(fruitSetup(kEmber, false).goal * 2 < goodGrown && fruitSetup(kStarfire, false).goal * 5 > goodGrown * 4);
    CHECK(fruitSetup(kStarfire, false).goal < goodStrong * 0.8f);
    // The stats: Wing runs faster, Wit reads the throw sooner and reaches further.
    Dragon quick = grown, clever = grown;
    quick.stats[kStatWing] = 9;
    clever.stats[kStatWit] = 9;
    const Catcher q = catcherFor(quick, dog, 6.5f, 1.0f), w = catcherFor(clever, dog, 6.5f, 1.0f);
    CHECK(q.run > big.run && q.dive == big.dive && w.dive > big.dive && w.leap > big.leap && w.react < big.react && w.run == big.run);
    // The wind: none at the Ember cup; stronger cup by cup, new each throw; it carries the fruit.
    CHECK(length(windFor(kEmber, 2, 7)) == 0.0f);
    float strongest = 0;
    for (int i = 0; i < 8; ++i) {
        const float k = length(windFor(kStarfire, i, 7));
        CHECK(k > 0 && k <= 1.41f && std::fabs(windFor(kStarfire, i, 7).z) < 1e-6f);
        strongest = std::fmax(strongest, k);
        if (i) CHECK(length(windFor(kStarfire, i, 7) - windFor(kStarfire, i - 1, 7)) > 1e-3f);
    }
    CHECK(strongest > 0.8f && length(windFor(kFlame, 3, 9)) <= 0.61f);
    Toss calm, blown;
    tossFrom(0, -1000, hand, 0, false, calm);
    blown = calm;
    blown.wind = {0, -2.0f, 0};  // a tailwind (the throw goes toward -Y)
    const float tl = landTime(blown, 0);
    CHECK(std::fabs(tl - landTime(calm, 0)) < 1e-4f && fruitAt(blown, tl).y < fruitAt(calm, tl).y - 1.0f);
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
    RUN(sky_rings_flight);
    RUN(sky_rings_rivals);
    RUN(lantern_trial);
    RUN(fruit_catch);
    RUN(breath_streams);
    RUN(challenge_meshes);
}
