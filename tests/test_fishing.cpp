// Driftwood Cove (core/fishing, 1.0 D90): the cove's spots on the real valley (you on dry sand at
// the water's edge, the bobber out on the water, shells on the beach), what bites and when, the
// bite's timing, the reel (a careful reeler lands the fish; reeling flat out snaps the line;
// never reeling lets it slip away), the day's shells, and your partner's nibble.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "check.hpp"
#include "core/dragon.hpp"
#include "core/fishing.hpp"
#include "core/place_layout.hpp"
#include "core/valley.hpp"

using namespace ec;
using namespace ec::fishing;

namespace {

const Valley& coveValley() {
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
        addPlaceDecks(v);  // (the jetty, as the game)
    }
    return v;
}

// A careful reeler: reels while the line is slack enough and the fish isn't running.
float careful(const Reel& r) { return !r.running() && r.tension < 0.55f ? 1.0f : 0.0f; }

}  // namespace

TEST(fishing_cove_spots) {
    const Valley& v = coveValley();
    const ValleyPlaceInfo* p = v.place(kPlaceCove);
    CHECK(p != nullptr);
    if (!p) return;
    const CoveSpots s = coveSpots(v);
    auto above = [&](Vec2 local) {  // what you'd stand on there (the jetty's deck, the sand) over the water
        const Vec2 w = placeToWorld(*p, local);
        return v.groundAt(w.x, w.y, v.water + 3.0f) - v.water;
    };
    std::printf("  the cove: you at (%.1f %.1f) %.2f m above the water, the bobber at (%.1f %.1f) %.2f m, the fisher at (%.1f %.1f)\n",
                s.fishSpot.x, s.fishSpot.y, above(s.fishSpot), s.castTo.x, s.castTo.y, above(s.castTo), s.fisher.x, s.fisher.y);
    CHECK(above(s.fishSpot) > 0.0f && above(s.fishSpot) < 1.5f);  // the jetty's end, just over the water
    CHECK(above(s.castTo) < -0.3f);                                // out on the water
    CHECK(above(s.fisher) > 0.0f && above(s.partner) > 0.0f);
    CHECK(s.castTo.y > s.fishSpot.y + 5);
    CHECK(std::hypot(s.fisher.x - s.fishSpot.x, s.fisher.y - s.fishSpot.y) > 2.5f);
    {  // Tam's line (app/cove.cpp drawOver: 4.2 m out along his facing) lands in the water (run 24)
        const Vec2 tam = placeToWorld(*p, s.fisher);
        const float h = p->heading + s.fisherFacing;
        const Vec2 end{tam.x + std::sin(h) * 4.2f, tam.y - std::cos(h) * 4.2f};
        std::printf("  Tam at (%.1f %.1f) %.2f m above the water, his line's end %.2f m\n", s.fisher.x, s.fisher.y, above(s.fisher),
                    v.heightAt(end.x, end.y) - v.water);
        CHECK(v.heightAt(end.x, end.y) < v.water - 0.2f);
        for (int k = 0; k < kShellSpots; ++k) CHECK(std::hypot(s.shells[k].x - s.fisher.x, s.shells[k].y - s.fisher.y) > 1.5f);
    }
    CHECK(std::hypot(s.partner.x - s.fishSpot.x, s.partner.y - s.fishSpot.y) > 1.2f);
    for (int k = 0; k < kShellSpots; ++k) {
        CHECK(above(s.shells[k]) > -0.05f && above(s.shells[k]) < 1.5f);  // on the beach, by the water
        for (int j = 0; j < k; ++j) CHECK(std::hypot(s.shells[k].x - s.shells[j].x, s.shells[k].y - s.shells[j].y) > 3.0f);
        CHECK(std::hypot(s.shells[k].x - s.fishSpot.x, s.shells[k].y - s.fishSpot.y) > 3.0f);  // not underfoot
    }
}

TEST(fishing_what_bites) {
    int counts[static_cast<int>(Catch::Count)] = {}, golden[static_cast<int>(Catch::Count)] = {};
    Rng rng(7);
    for (int i = 0; i < 20000; ++i) {
        ++counts[static_cast<int>(rollCatch(rng, 13))];
        ++golden[static_cast<int>(rollCatch(rng, 18))];
    }
    std::printf("  a day's 20000 bites: %d River Fish, %d big, %d foods, %d shells, %d pearls\n", counts[0], counts[1],
                counts[2] + counts[3] + counts[4], counts[5], counts[6]);
    CHECK(counts[0] > 10000 && counts[0] > counts[1] * 3);  // mostly River Fish
    CHECK(counts[6] > 100 && counts[6] < 800);               // pearls rare
    CHECK(golden[1] > counts[1] * 1.3f);                     // more big ones at dusk
    CHECK(goldenHour(6) && goldenHour(18) && !goldenHour(13) && !goldenHour(0));
    for (int c = 0; c < static_cast<int>(Catch::Count); ++c) {
        const CatchInfo& info = catchInfo(static_cast<Catch>(c));
        CHECK(info.name[0] != 0 && info.strength > 0.4f && info.strength < 1.8f);
        CHECK((info.food != Food::Count) == (info.foodCount > 0) && (info.food == Food::Count) == (info.gleam > 0));
    }
    CHECK(catchInfo(Catch::RiverFish).food == Food::RiverFish && catchInfo(Catch::BigFish).foodCount == 2);
    CHECK(catchInfo(Catch::RiverFish).fish && catchInfo(Catch::BigFish).fish && !catchInfo(Catch::Pearl).fish);
    CHECK(catchInfo(Catch::Pearl).gleam > catchInfo(Catch::Shell).gleam * 5);
    CHECK(std::strchr(shellName(20000, 0), '!') == nullptr);  // (the toast adds its own)
    // The bite: a few seconds' wait (quicker at dusk), nibbles well before it and apart, a moment to strike.
    float dayWait = 0, duskWait = 0;
    for (int i = 0; i < 400; ++i) {
        const Bite b = rollBite(rng, 13, rollCatch(rng, 13)), d = rollBite(rng, 18, Catch::BigFish);
        dayWait += b.wait;
        duskWait += d.wait;
        CHECK(b.wait > 1.5f && b.wait < 8 && b.window >= 0.6f && b.window <= 1.0f && b.nibbles <= kMaxNibbles);
        for (int k = 0; k < b.nibbles; ++k) {
            CHECK(b.nibbleAt[k] > 0.6f && b.nibbleAt[k] + kNibbleTime < b.wait - 0.4f);
            if (k) CHECK(b.nibbleAt[k] - b.nibbleAt[k - 1] > 0.7f);
        }
        CHECK(d.window < b.window + 0.3f);
    }
    CHECK(duskWait < dayWait * 0.8f);
}

TEST(fishing_the_reel) {
    const float dt = 1.0f / 30.0f;
    int landed = 0, landedBig = 0;
    float slowest = 0, total = 0, slowestBig = 0, totalBig = 0;
    for (u32 seed = 1; seed <= 20; ++seed) {
        Reel r;
        r.start(catchInfo(Catch::RiverFish).strength, seed);
        float t = 0;
        while (r.step == Reel::Step::Reeling && t < 40) {
            r.update(careful(r), dt);
            t += dt;
        }
        landed += r.step == Reel::Step::Caught;
        slowest = std::fmax(slowest, t);
        total += t;
        Reel big;
        big.start(catchInfo(Catch::BigFish).strength, seed);
        t = 0;
        while (big.step == Reel::Step::Reeling && t < 60) {
            big.update(careful(big), dt);
            t += dt;
        }
        landedBig += big.step == Reel::Step::Caught;
        slowestBig = std::fmax(slowestBig, t);
        totalBig += t;
    }
    std::printf("  a careful reeler lands %d of 20 River Fish (%.1f s on average, the slowest %.1f) and %d of 20 big ones "
                "(%.1f s, %.1f)\n",
                landed, total / 20, slowest, landedBig, totalBig / 20, slowestBig);
    // A River Fish in well under ten seconds, a big one in about a quarter of a minute (D90: cozy,
    // not a chore).
    CHECK(landed == 20 && total / 20 < 10 && slowest < 16 && slowest > 3);
    CHECK(landedBig >= 17 && totalBig / 20 < 18);
    // Flat out: it snaps. Never: it slips away. Letting the line go slack for a while: away too.
    Reel r;
    r.start(1.0f, 3);
    for (int i = 0; i < 300 && r.step == Reel::Step::Reeling; ++i) r.update(1.0f, dt);
    CHECK(r.step == Reel::Step::Snapped);
    for (u32 seed = 1; seed <= 10; ++seed) {  // (whatever the fish does, it's gone within the idle limit and a little)
        r.start(1.55f, seed);
        float t = 0;
        for (; t < 10 && r.step == Reel::Step::Reeling; t += dt) r.update(0.0f, dt);
        CHECK(r.step == Reel::Step::Escaped && t < kIdleLimit + 0.2f);
    }
    // In the band it comes in faster than out of it.
    Reel a, b;
    a.start(1.0f, 5);
    b.start(1.0f, 5);
    a.tension = 0.5f;
    b.tension = 0.15f;
    a.runIn = b.runIn = 99;
    a.update(0.2f, 0.1f);
    b.update(0.2f, 0.1f);
    CHECK(a.progress > b.progress && a.inBand() && !b.inBand());
}

TEST(fishing_shells_and_nibbles) {
    int total = 0, pearls = 0;
    for (s32 day = 20000; day < 20200; ++day) {
        const u8 bits = shellsToday(day);
        int n = 0;
        for (int k = 0; k < kShellSpots; ++k)
            if (bits & (1u << k)) {
                ++n;
                pearls += shellAt(day, k) == Catch::Pearl;
                CHECK(shellGleam(day, k) >= 5 && shellName(day, k)[0] != 0 && shellKind(day, k) >= 0 && shellKind(day, k) < 4);
                CHECK((shellKind(day, k) == 3) == (shellAt(day, k) == Catch::Pearl));
                CHECK((shellAt(day, k) == Catch::Pearl) == (shellGleam(day, k) == catchInfo(Catch::Pearl).gleam));
            }
        CHECK(n >= 3 && n <= 5 && bits < (1u << kShellSpots));
        CHECK(shellsToday(day) == bits);  // the same all day
        total += n;
    }
    CHECK(shellsToday(20000) != shellsToday(20001) || shellsToday(20001) != shellsToday(20002));  // new ones each day
    std::printf("  200 days: %d shells washed up, %d of them pearls\n", total, pearls);
    CHECK(pearls > 0 && pearls < total / 10);
    Dragon d;
    d.stage = Stage::Adult;
    d.needs.love = 50;
    d.needs.belly = 50;
    nibble(d);
    CHECK(d.needs.love == 50 + kNibbleLove && d.needs.belly == 50 + kNibbleBelly);
    d.needs.love = 98;
    nibble(d);
    CHECK(d.needs.love == 100);
    Dragon egg;
    egg.needs.love = 10;
    nibble(egg);  // (an egg: nothing)
    CHECK(egg.needs.love == 10);
}

void runFishingTests() {
    RUN(fishing_cove_spots);
    RUN(fishing_what_bites);
    RUN(fishing_the_reel);
    RUN(fishing_shells_and_nibbles);
}
