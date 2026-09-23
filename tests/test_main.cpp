// PC unit tests for src/core. Build and run with tools/test.ps1 (or `make -C tests`).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>

#include "core/clock.hpp"
#include "core/dragon.hpp"
#include "core/genetics.hpp"

using namespace ec;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_checks;                                                          \
        if (!(cond)) {                                                       \
            ++g_failures;                                                    \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
        }                                                                    \
    } while (0)

#define TEST(name) static void name()
#define RUN(name)                            \
    do {                                     \
        std::printf("%s\n", #name);          \
        name();                              \
    } while (0)

// 2026-01-05 00:00 "local unix"; a Monday, so nothing depends on it except the hour.
static constexpr s64 kT0 = 1767571200;

TEST(breed_names_are_symmetric_and_complete) {
    std::set<std::string> names;
    for (int a = 0; a < kElementCount; ++a)
        for (int b = 0; b < kElementCount; ++b) {
            const char* ab = breedName(static_cast<Element>(a), static_cast<Element>(b));
            const char* ba = breedName(static_cast<Element>(b), static_cast<Element>(a));
            CHECK(ab != nullptr);
            CHECK(ab && ba && std::strcmp(ab, ba) == 0);
            if (ab) names.insert(ab);
        }
    CHECK(names.size() == 21);
    CHECK(std::strcmp(breedName(Element::Ember, Element::Tide), "Steam") == 0);
    CHECK(std::strcmp(breedName(Element::Gale, Element::Lumen), "Aurora") == 0);
}

TEST(purebreds_look_like_their_breed) {
    Rng rng(1);
    for (int e = 0; e < kElementCount; ++e) {
        const Genome g = makePurebred(static_cast<Element>(e), rng);
        CHECK(g.elementA == e && g.elementB == e);
        CHECK(!isHybrid(g));
        CHECK(g.build < kBuildCount && g.horns < kHornsCount && g.pattern < kPatternCount);
    }
}

TEST(elements_follow_mendelian_ratios) {
    Rng rng(42);
    Genome ember = makePurebred(Element::Ember, rng);
    Genome tide = makePurebred(Element::Tide, rng);
    // Purebred x purebred of different elements: always the hybrid.
    for (int i = 0; i < 200; ++i) {
        const Genome c = breed(ember, tide, rng);
        CHECK(std::strcmp(breedName(c), "Steam") == 0);
    }
    // Steam x Steam: ~25% Ember, ~50% Steam, ~25% Tide.
    const Genome steam = breed(ember, tide, rng);
    int counts[3] = {0, 0, 0};
    const int n = 20000;
    for (int i = 0; i < n; ++i) {
        const Genome c = breed(steam, steam, rng);
        const char* name = breedName(c);
        if (std::strcmp(name, "Ember") == 0) ++counts[0];
        else if (std::strcmp(name, "Steam") == 0) ++counts[1];
        else if (std::strcmp(name, "Tide") == 0) ++counts[2];
    }
    CHECK(counts[0] + counts[1] + counts[2] == n);
    CHECK(std::fabs(counts[0] / double(n) - 0.25) < 0.02);
    CHECK(std::fabs(counts[1] / double(n) - 0.50) < 0.02);
    CHECK(std::fabs(counts[2] / double(n) - 0.25) < 0.02);
}

TEST(breeding_is_deterministic_for_a_seed) {
    Rng r1(7), r2(7);
    const Genome a1 = makePurebred(Element::Gale, r1), b1 = makePurebred(Element::Frost, r1);
    const Genome a2 = makePurebred(Element::Gale, r2), b2 = makePurebred(Element::Frost, r2);
    const Genome c1 = breed(a1, b1, r1), c2 = breed(a2, b2, r2);
    CHECK(std::memcmp(&c1, &c2, sizeof(Genome)) == 0);
}

TEST(rare_traits_are_bounded_and_consistent) {
    Rng rng(99);
    Genome a = makePurebred(Element::Lumen, rng);
    Genome b = makePurebred(Element::Grove, rng);
    a.rareFlags = kRareIridescent | kRareMelanistic;
    b.rareFlags = kRareLeucistic | kRareStarspeckle;
    int withAny = 0;
    for (int i = 0; i < 5000; ++i) {
        const Genome c = breed(a, b, rng);
        CHECK(rareCount(c.rareFlags) <= 2);
        CHECK(!((c.rareFlags & kRareMelanistic) && (c.rareFlags & kRareLeucistic)));
        if (c.rareFlags) ++withAny;
    }
    CHECK(withAny > 2500);  // rare parents pass traits on often
}

TEST(hybrid_colours_follow_each_element) {
    Rng rng(5);
    const Genome ember = makePurebred(Element::Ember, rng);
    const Genome tide = makePurebred(Element::Tide, rng);
    for (int i = 0; i < 500; ++i) {
        const Genome c = breed(ember, tide, rng);  // elementA = Ember, elementB = Tide
        const int baseFromEmber = static_cast<std::int8_t>(static_cast<u8>(c.baseH - 13));
        const int accentFromTide = static_cast<std::int8_t>(static_cast<u8>(c.accentH - 138));
        CHECK(baseFromEmber >= -9 && baseFromEmber <= 9);
        CHECK(accentFromTide >= -8 && accentFromTide <= 8);
    }
}

TEST(clock_rollback_and_cap) {
    CHECK(safeElapsed(1000, 900) == 0);
    CHECK(safeElapsed(1000, 1000) == 0);
    CHECK(safeElapsed(1000, 4600) == 3600);
    CHECK(safeElapsed(0, 100 * kDay) == kMaxCatchUp);
    CHECK(isNight(kT0 + 23 * kHour));
    CHECK(!isNight(kT0 + 12 * kHour));
    CHECK(dayIndex(kT0 + kDay - 1) + 1 == dayIndex(kT0 + kDay));
}

TEST(stage_gates) {
    CHECK(stageFor(0, 0) == Stage::Hatchling);
    CHECK(stageFor(4, 5) == Stage::Hatchling);  // time without care is not enough
    CHECK(stageFor(4, 6) == Stage::Juvenile);
    CHECK(stageFor(20, 6) == Stage::Juvenile);
    CHECK(stageFor(8, 14) == Stage::Adolescent);
    CHECK(stageFor(13, 40) == Stage::Adolescent);  // care without time is not enough
    CHECK(stageFor(14, 26) == Stage::Adult);
}

TEST(egg_hatches_when_kept_warm_and_pauses_in_vault) {
    Rng rng(3);
    Dragon egg = makeEgg(1, makePurebred(Element::Ember, rng), kT0);
    s64 t = kT0;
    for (int h = 0; h < 30; ++h) {  // rub every 3 hours for 30 hours
        if (h % 3 == 0) warmEgg(egg, 30);
        simulate(egg, t, t + kHour);
        t += kHour;
    }
    CHECK(tryHatch(egg, t, rng));
    CHECK(egg.stage == Stage::Hatchling);

    Dragon vaulted = makeEgg(2, makePurebred(Element::Tide, rng), kT0);
    vaulted.location = Location::Vault;
    simulate(vaulted, kT0, kT0 + 5 * kDay);
    CHECK(vaulted.incubationSeconds == 0);
    CHECK(!tryHatch(vaulted, kT0 + 5 * kDay, rng));

    Dragon cold = makeEgg(3, makePurebred(Element::Gale, rng), kT0);
    simulate(cold, kT0, kT0 + 3 * kDay);  // never rubbed: stalls, never breaks
    CHECK(cold.incubationSeconds < kIncubationSeconds);
    CHECK(cold.stage == Stage::Egg);
}

// Raise a hatchling with `visitsPerDay` full-care visits for `days` days.
static Dragon raise(int visitsPerDay, int days) {
    Rng rng(11);
    Dragon d = makeEgg(1, makePurebred(Element::Ember, rng), kT0);
    d.incubationSeconds = kIncubationSeconds;
    tryHatch(d, kT0 + 8 * kHour, rng);
    const int visitHours[] = {9, 13, 18};
    s64 t = kT0 + 8 * kHour;
    for (int day = 0; day < days; ++day) {
        for (int v = 0; v < visitsPerDay; ++v) {
            const s64 visit = kT0 + day * kDay + visitHours[v] * kHour;
            if (visit <= t) continue;
            simulate(d, t, visit);
            t = visit;
            markVisit(d, t);
            if (d.upset) makeUp(d);
            feed(d, 100, false);
            groom(d, 100);
            play(d, 100);
            pet(d, 100);
        }
    }
    simulate(d, t, kT0 + days * kDay + 9 * kHour);
    return d;
}

TEST(good_care_reaches_adult_in_about_two_weeks) {
    const Dragon d = raise(3, 15);
    CHECK(d.stage == Stage::Adult);
    CHECK(d.bond > 100);
    std::printf("  3 visits/day for 15 days: stage=%s stars=%d bond=%d\n", stageName(d.stage), d.careStars, d.bond);
}

TEST(light_care_grows_slower_but_still_grows) {
    const Dragon d = raise(1, 15);
    CHECK(d.stage >= Stage::Juvenile);
    CHECK(d.stage < Stage::Adult);
    std::printf("  1 visit/day for 15 days: stage=%s stars=%d\n", stageName(d.stage), d.careStars);
}

TEST(neglect_upsets_but_never_harms) {
    Rng rng(2);
    Dragon d = makeEgg(1, makePurebred(Element::Frost, rng), kT0);
    d.incubationSeconds = kIncubationSeconds;
    tryHatch(d, kT0, rng);
    const Stage before = d.stage;
    simulate(d, kT0, kT0 + 10 * kDay);
    CHECK(d.upset);
    CHECK(moodOf(d) == Mood::Upset);
    CHECK(d.stage >= before);  // no regression
    feed(d, 100, true);
    CHECK(d.bond == 0);  // bond waits for making up
    makeUp(d);
    CHECK(!d.upset);
    CHECK(d.bond > 0);
}

TEST(sanctuary_keeps_needs_safe) {
    Rng rng(4);
    Dragon d = makeEgg(1, makePurebred(Element::Grove, rng), kT0);
    d.incubationSeconds = kIncubationSeconds;
    tryHatch(d, kT0, rng);
    d.location = Location::Sanctuary;
    simulate(d, kT0, kT0 + 14 * kDay);
    CHECK(d.needs.lowest() >= 50);
    CHECK(!d.upset);
    CHECK(d.careStars == 0);  // growth pauses in the Sanctuary
}

TEST(body_scale_grows_every_day) {
    Rng rng(6);
    Dragon d = makeEgg(1, makePurebred(Element::Lumen, rng), kT0);
    d.incubationSeconds = kIncubationSeconds;
    tryHatch(d, kT0, rng);
    const float s0 = bodyScale(d, kT0);
    const float s2 = bodyScale(d, kT0 + 2 * kDay);
    CHECK(s0 >= 0.25f && s0 < 0.26f);
    CHECK(s2 > s0);
    CHECK(s2 < 0.45f);
}

int main() {
    RUN(breed_names_are_symmetric_and_complete);
    RUN(purebreds_look_like_their_breed);
    RUN(elements_follow_mendelian_ratios);
    RUN(breeding_is_deterministic_for_a_seed);
    RUN(rare_traits_are_bounded_and_consistent);
    RUN(hybrid_colours_follow_each_element);
    RUN(clock_rollback_and_cap);
    RUN(stage_gates);
    RUN(egg_hatches_when_kept_warm_and_pauses_in_vault);
    RUN(good_care_reaches_adult_in_about_two_weeks);
    RUN(light_care_grows_slower_but_still_grows);
    RUN(neglect_upsets_but_never_harms);
    RUN(sanctuary_keeps_needs_safe);
    RUN(body_scale_grows_every_day);
    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
