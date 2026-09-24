// PC unit tests for src/core. Build and run with tools/test.ps1 (or `make -C tests`).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>

#include "check.hpp"
#include "core/breeding.hpp"
#include "core/clock.hpp"
#include "core/dragon.hpp"
#include "core/genetics.hpp"
#include "core/save.hpp"

#include <vector>

using namespace ec;

int g_failures = 0;
int g_checks = 0;

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
    Dragon egg = makeEgg(1, makePurebred(Element::Ember, rng), Sex::Female, kT0);
    s64 t = kT0;
    for (int h = 0; h < 30; ++h) {  // rub every 3 hours for 30 hours
        if (h % 3 == 0) warmEgg(egg, 30);
        simulate(egg, t, t + kHour);
        t += kHour;
    }
    CHECK(tryHatch(egg, t, rng));
    CHECK(egg.stage == Stage::Hatchling);

    Dragon vaulted = makeEgg(2, makePurebred(Element::Tide, rng), Sex::Female, kT0);
    vaulted.location = Location::Vault;
    simulate(vaulted, kT0, kT0 + 5 * kDay);
    CHECK(vaulted.incubationSeconds == 0);
    CHECK(!tryHatch(vaulted, kT0 + 5 * kDay, rng));

    Dragon cold = makeEgg(3, makePurebred(Element::Gale, rng), Sex::Female, kT0);
    simulate(cold, kT0, kT0 + 3 * kDay);  // never rubbed: stalls, never breaks
    CHECK(cold.incubationSeconds < kIncubationSeconds);
    CHECK(cold.stage == Stage::Egg);
}

// Raise a hatchling with `visitsPerDay` full-care visits for `days` days.
static Dragon raise(int visitsPerDay, int days) {
    Rng rng(11);
    Dragon d = makeEgg(1, makePurebred(Element::Ember, rng), Sex::Female, kT0);
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
    Dragon d = makeEgg(1, makePurebred(Element::Frost, rng), Sex::Female, kT0);
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
    Dragon d = makeEgg(1, makePurebred(Element::Grove, rng), Sex::Female, kT0);
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
    Dragon d = makeEgg(1, makePurebred(Element::Lumen, rng), Sex::Female, kT0);
    d.incubationSeconds = kIncubationSeconds;
    tryHatch(d, kT0, rng);
    const float s0 = bodyScale(d, kT0);
    const float s2 = bodyScale(d, kT0 + 2 * kDay);
    CHECK(s0 >= 0.25f && s0 < 0.26f);
    CHECK(s2 > s0);
    CHECK(s2 < 0.45f);
}

static Dragon readyAdult(u32 id, Element e, Sex sex, Rng& rng) {
    Dragon d = makeEgg(id, makePurebred(e, rng), sex, kT0);
    d.stage = Stage::Adult;
    d.hatchedAt = kT0 - 20 * kDay;
    d.bond = d.bondHigh = 400;
    d.needs = Needs{90, 90, 90, 90};
    return d;
}

TEST(breeding_needs_one_male_and_one_female) {
    Rng rng(8);
    const s64 now = kT0;
    Dragon m = readyAdult(1, Element::Ember, Sex::Male, rng);
    Dragon f = readyAdult(2, Element::Tide, Sex::Female, rng);
    Dragon f2 = readyAdult(3, Element::Gale, Sex::Female, rng);
    CHECK(breedingBlock(m, f, now) == BreedBlock::None);
    CHECK(breedingBlock(f, f2, now) == BreedBlock::SameSex);
    CHECK(breedingBlock(m, m, now) == BreedBlock::SameDragon);

    const Dragon egg = layEgg(10, m, f, now, rng);  // argument order doesn't matter
    CHECK(egg.stage == Stage::Egg);
    CHECK(egg.motherId == 2 && egg.fatherId == 1);
    CHECK(std::strcmp(breedName(egg.genome), "Steam") == 0);
    CHECK(breedingBlock(m, f, now + kDay) == BreedBlock::Resting);
    CHECK(breedingBlock(m, f, now + 3 * kDay) == BreedBlock::None);
}

TEST(breeding_requirements) {
    Rng rng(9);
    const Dragon m = readyAdult(1, Element::Ember, Sex::Male, rng);
    Dragon f = readyAdult(2, Element::Frost, Sex::Female, rng);
    f.stage = Stage::Adolescent;
    CHECK(breedingBlock(m, f, kT0) == BreedBlock::NotAdult);
    f.stage = Stage::Adult;
    f.bond = 100;
    CHECK(breedingBlock(m, f, kT0) == BreedBlock::LowBond);
    f.bond = 400;
    f.upset = true;
    CHECK(breedingBlock(m, f, kT0) == BreedBlock::Unhappy);
    f.upset = false;
    f.location = Location::Sanctuary;
    CHECK(breedingBlock(m, f, kT0) == BreedBlock::NotInDen);
    for (int b = 0; b <= static_cast<int>(BreedBlock::NotInDen); ++b)
        CHECK(std::strlen(breedBlockHint(static_cast<BreedBlock>(b))) > 0);
}

TEST(egg_sexes_are_roughly_even) {
    Rng rng(10);
    int males = 0;
    for (int i = 0; i < 20000; ++i) males += rollSex(rng) == Sex::Male;
    CHECK(std::fabs(males / 20000.0 - 0.5) < 0.02);
}

// ---------------------------------------------------------------- save format (WP8)

static SaveData& sampleSave() {
    static SaveData s;
    s = SaveData{};
    std::snprintf(s.playerName, sizeof(s.playerName), "Noah");
    s.lastSim = kT0 + 12345;
    s.devOffset = 3 * kDay;
    s.nextId = 42;
    s.settings.musicVolume = 55;
    Rng rng(123);
    s.dragonCount = 5;
    for (int i = 0; i < 5; ++i) {
        Dragon d = makeEgg(10 + i, makePurebred(static_cast<Element>(i % kElementCount), rng), rollSex(rng), kT0);
        if (i > 0) {
            d.incubationSeconds = kIncubationSeconds;
            tryHatch(d, kT0 + kHour, rng);
            simulate(d, kT0 + kHour, kT0 + (i + 1) * kDay);
            pet(d, 5);
            for (float& dust : d.dirt) dust = std::round(dust * 100.0f) / 100.0f;  // saved in hundredths
        }
        std::snprintf(d.name, sizeof(d.name), "Drake%d", i);
        d.motherId = i;
        d.location = static_cast<Location>(i % 2);
        s.dragons[i] = d;
    }
    return s;
}

static bool sameDragon(const Dragon& a, const Dragon& b) {
    return a.id == b.id && std::memcmp(&a.genome, &b.genome, sizeof(Genome)) == 0 && a.sex == b.sex &&
           a.motherId == b.motherId && std::strcmp(a.name, b.name) == 0 && a.stage == b.stage &&
           a.location == b.location && a.hatchedAt == b.hatchedAt && a.needs.belly == b.needs.belly &&
           a.needs.play == b.needs.play && a.bond == b.bond && a.careStars == b.careStars &&
           a.personality == b.personality && a.warmth == b.warmth && a.dayLowestSum == b.dayLowestSum &&
           a.upset == b.upset && a.napping == b.napping &&
           std::memcmp(a.dirt, b.dirt, sizeof(a.dirt)) == 0;
}

// Dust settles over a day or two, faster on the belly than the wings; grooming, brushing a
// region and the bath clear it; Sanctuary keepers keep it low (D46).
TEST(dirt_settles_and_grooming_clears_it) {
    Rng rng(5);
    Dragon d = makeEgg(1, makePurebred(Element::Ember, rng), Sex::Female, kT0);
    d.incubationSeconds = kIncubationSeconds;
    tryHatch(d, kT0 + 8 * kHour, rng);
    for (float dust : d.dirt) CHECK(dust == 0.0f);
    simulate(d, kT0 + 8 * kHour, kT0 + 32 * kHour);  // a day
    std::printf("  after a day: belly %.0f, back %.0f, wings %.0f\n", d.dirt[kRegionBelly], d.dirt[kRegionBack],
                d.dirt[kRegionWings]);
    CHECK(d.dirt[kRegionBelly] > 60 && d.dirt[kRegionBelly] < 100);
    CHECK(d.dirt[kRegionWings] > 25 && d.dirt[kRegionWings] < d.dirt[kRegionBelly]);
    simulate(d, kT0 + 32 * kHour, kT0 + 80 * kHour);  // two more days: fully dusty
    CHECK(d.dirt[kRegionBack] == 100.0f);
    cleanRegion(d, kRegionBack, 60);
    CHECK(d.dirt[kRegionBack] == 40.0f && d.dirt[kRegionBelly] == 100.0f);
    groom(d, 40);
    CHECK(d.dirt[kRegionBack] == 0.0f && d.dirt[kRegionBelly] == 40.0f);
    bathe(d);
    for (float dust : d.dirt) CHECK(dust == 0.0f);
    d.location = Location::Sanctuary;
    simulate(d, kT0 + 80 * kHour, kT0 + 104 * kHour);
    CHECK(d.dirt[kRegionBelly] < 10.0f);
}

TEST(save_round_trip) {
    const SaveData& s = sampleSave();
    std::vector<u8> buf(maxEncodedSize());
    const std::size_t n = encodeSave(s, 7, kT0 + 99, buf.data(), buf.size());
    CHECK(n > kSaveHeaderSize);
    CHECK(n == kSaveHeaderSize + 16 + 8 + 8 + 4 + 2 + 4 + 2 + 2 + 5 * (132 + 16 + 2));  // v1 + dirt
    static SaveData out;
    SaveHeaderInfo info;
    CHECK(decodeSave(buf.data(), n, out, &info) == LoadResult::Ok);
    CHECK(info.seq == 7 && info.savedAt == kT0 + 99 && info.version == kSaveVersion);
    CHECK(std::strcmp(out.playerName, "Noah") == 0);
    CHECK(out.lastSim == s.lastSim && out.devOffset == s.devOffset && out.nextId == 42);
    CHECK(out.settings.musicVolume == 55);
    CHECK(out.dragonCount == 5);
    for (int i = 0; i < 5; ++i) CHECK(sameDragon(out.dragons[i], s.dragons[i]));
}

TEST(save_detects_corruption_and_truncation) {
    const SaveData& s = sampleSave();
    std::vector<u8> buf(maxEncodedSize());
    const std::size_t n = encodeSave(s, 1, kT0, buf.data(), buf.size());
    static SaveData out;
    out.nextId = 777;
    std::vector<u8> bad(buf.begin(), buf.begin() + n);
    bad[n / 2] ^= 0x40;
    CHECK(decodeSave(bad.data(), n, out) == LoadResult::BadCrc);
    CHECK(out.nextId == 777);  // untouched on failure
    CHECK(decodeSave(buf.data(), n - 10, out) == LoadResult::Truncated);
    CHECK(decodeSave(buf.data(), 10, out) == LoadResult::Truncated);
    CHECK(decodeSave(nullptr, 0, out) == LoadResult::Empty);
    std::vector<u8> notSave(n, 0x11);
    CHECK(decodeSave(notSave.data(), n, out) == LoadResult::BadMagic);
    std::vector<u8> newer(buf.begin(), buf.begin() + n);
    newer[4] = static_cast<u8>(kSaveVersion + 1);  // version field
    CHECK(decodeSave(newer.data(), n, out) == LoadResult::TooNew);
}

TEST(save_rejects_out_of_range_data) {
    SaveData& s = sampleSave();
    s.dragons[2].stage = static_cast<Stage>(9);
    std::vector<u8> buf(maxEncodedSize());
    const std::size_t n = encodeSave(s, 1, kT0, buf.data(), buf.size());
    static SaveData out;
    CHECK(decodeSave(buf.data(), n, out) == LoadResult::BadData);
}

TEST(save_picks_newest_valid_slot) {
    const SaveData& s = sampleSave();
    std::vector<u8> a(maxEncodedSize()), b(maxEncodedSize());
    const std::size_t na = encodeSave(s, 5, kT0, a.data(), a.size());
    const std::size_t nb = encodeSave(s, 6, kT0 + 60, b.data(), b.size());
    CHECK(pickNewestSlot(a.data(), na, b.data(), nb) == 1);
    b[nb - 1] ^= 1;  // interrupted write corrupted the newer slot
    CHECK(pickNewestSlot(a.data(), na, b.data(), nb) == 0);
    CHECK(pickNewestSlot(nullptr, 0, b.data(), nb) == -1);
    CHECK(pickNewestSlot(nullptr, 0, a.data(), na) == 1);
}

TEST(save_full_capacity_fits) {
    static SaveData s;
    s = SaveData{};
    Rng rng(5);
    s.dragonCount = kMaxDragons;
    for (u32 i = 0; i < kMaxDragons; ++i)
        s.dragons[i] = makeEgg(i + 1, makePurebred(static_cast<Element>(i % kElementCount), rng), rollSex(rng), kT0);
    std::vector<u8> buf(maxEncodedSize());
    const std::size_t n = encodeSave(s, 1, kT0, buf.data(), buf.size());
    CHECK(n > 0 && n <= maxEncodedSize());
    std::printf("  full save (200 dragons): %zu bytes\n", n);
    static SaveData out;
    CHECK(decodeSave(buf.data(), n, out) == LoadResult::Ok && out.dragonCount == kMaxDragons);
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
    RUN(dirt_settles_and_grooming_clears_it);
    RUN(body_scale_grows_every_day);
    RUN(breeding_needs_one_male_and_one_female);
    RUN(breeding_requirements);
    RUN(egg_sexes_are_roughly_even);
    RUN(save_round_trip);
    RUN(save_detects_corruption_and_truncation);
    RUN(save_rejects_out_of_range_data);
    RUN(save_picks_newest_valid_slot);
    RUN(save_full_capacity_fits);
    runModelTests();
    runAnimTests();
    runBehaviorTests();
    runDenTests();
    runEggTests();
    runCareTests();
    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
