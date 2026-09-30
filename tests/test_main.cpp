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
#include "core/kinds.hpp"
#include "core/den_roster.hpp"
#include "core/dragondex.hpp"
#include "core/items.hpp"
#include "core/market.hpp"
#include "core/mud.hpp"
#include "core/profile.hpp"
#include "core/wanderings.hpp"
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
    // (hours since hatching, stars: D136, grown in 5.5 days with the best care)
    CHECK(stageFor(0, 0) == Stage::Hatchling);
    CHECK(stageFor(36, 2) == Stage::Hatchling);  // time without care is not enough
    CHECK(stageFor(35, 9) == Stage::Hatchling);  // care without time is not enough
    CHECK(stageFor(36, 3) == Stage::Juvenile);
    CHECK(stageFor(480, 3) == Stage::Juvenile);
    CHECK(stageFor(78, 7) == Stage::Adolescent);
    CHECK(stageFor(131, 40) == Stage::Adolescent);
    CHECK(stageFor(132, 12) == Stage::Adult);
    CHECK(kIncubationSeconds == 36 * 3600);
}

TEST(egg_hatches_when_kept_warm_and_pauses_in_vault) {
    Rng rng(3);
    Dragon egg = makeEgg(1, makePurebred(Element::Ember, rng), Sex::Female, kT0);
    s64 t = kT0;
    for (int h = 0; h < 40; ++h) {  // rub every 3 hours for 40 hours (it needs 36 warm: D136)
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
            bathe(d);
            play(d, 100);
            pet(d, 100);
        }
    }
    simulate(d, t, kT0 + days * kDay + 9 * kHour);
    return d;
}

TEST(good_care_reaches_adult_in_about_five_and_a_half_days) {  // (D136)
    const Dragon early = raise(3, 5);  // (5 days and an hour after hatching)
    CHECK(early.stage == Stage::Adolescent);
    const Dragon d = raise(3, 6);
    CHECK(d.stage == Stage::Adult);
    CHECK(d.bond > 60);
    std::printf("  3 visits/day: day 5 %s (%d stars), day 6 %s (%d stars, bond %d)\n", stageName(early.stage), early.careStars,
                stageName(d.stage), d.careStars, d.bond);
}

TEST(light_care_grows_slower_but_still_grows) {
    const Dragon d = raise(1, 6);
    CHECK(d.stage >= Stage::Juvenile);
    CHECK(d.stage < Stage::Adult);
    std::printf("  1 visit/day for 6 days: stage=%s stars=%d\n", stageName(d.stage), d.careStars);
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
    d.needs = Needs{90, 90, 90, 90, 90};
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
    f.bond = 0;  // (no trust needed, D129)
    CHECK(breedingBlock(m, f, kT0) == BreedBlock::None);
    f.bond = 400;
    f.upset = true;
    CHECK(breedingBlock(m, f, kT0) == BreedBlock::Unhappy);
    f.upset = false;
    f.location = Location::Sanctuary;
    CHECK(breedingBlock(m, f, kT0) == BreedBlock::NotInDen);
    for (int b = 0; b <= static_cast<int>(BreedBlock::NotInDen); ++b)
        CHECK(std::strlen(breedBlockHint(static_cast<BreedBlock>(b))) > 0);
}

// The Nesting Stone (Alpha 2 WP3): a ready pair settles; the egg comes the next calendar day,
// with its parents, into a free nest (else the Vault); the pair stops nesting.
TEST(the_nesting_stone) {
    static SaveData s;
    s = SaveData{};
    Rng rng(21);
    s.dragons[s.dragonCount++] = readyAdult(1, Element::Ember, Sex::Male, rng);
    s.dragons[s.dragonCount++] = readyAdult(2, Element::Tide, Sex::Female, rng);
    s.dragons[1].denSlot = 1;
    s.nextId = 3;
    CHECK(!settleToNest(s, 0, 0, kT0));  // one dragon isn't a pair
    CHECK(settleToNest(s, 0, 1, kT0) && s.nestA == 1 && s.nestB == 2);
    CHECK(layDueEgg(s, kT0 + kHour, rng) == -1);  // not until tomorrow
    const s64 tomorrow = kT0 + kDay;
    const int e = layDueEgg(s, tomorrow, rng);
    CHECK(e == 2 && s.dragonCount == 3 && s.nestA == 0 && s.nestB == 0);
    const Dragon& egg = s.dragons[e];
    CHECK(egg.stage == Stage::Egg && egg.motherId == 2 && egg.fatherId == 1 && egg.id == 3 && egg.origin == Origin::Bred);
    CHECK(egg.location == Location::Den && denRoster(s).eggCount == 1);
    CHECK(layDueEgg(s, tomorrow + kDay, rng) == -1);  // one egg per pairing
    CHECK(!settleToNest(s, 0, 1, tomorrow));  // they rest for three days
}

// The Wanderings (Alpha 2 WP4, GDD 11): one juvenile-or-older dragon at a time; finds grow
// with the steps (more for adults and the curious); Gleam and trinkets go home; now and then a
// wild egg, mostly the breeds you can't start with; muddy paws after.
TEST(the_wanderings) {
    static SaveData s;
    s = SaveData{};
    Rng rng(31);
    Dragon baby = readyAdult(1, Element::Ember, Sex::Male, rng);
    baby.stage = Stage::Hatchling;
    Dragon young = readyAdult(2, Element::Tide, Sex::Female, rng);
    young.stage = Stage::Juvenile;
    young.denSlot = 1;
    s.dragons[s.dragonCount++] = baby;
    s.dragons[s.dragonCount++] = young;
    s.nextId = 3;
    CHECK(cantWander(s, 0) != nullptr && !setOff(s, 0, 1000, kT0));  // too little
    CHECK(cantWander(s, 1) == nullptr && setOff(s, 1, 1000, kT0));
    CHECK(wandererIndex(s) == 1 && cantWander(s, 0) != nullptr);  // one at a time
    const DenRoster r = denRoster(s);
    CHECK(r.dragonCount == 2 && r.presentCount() == 1 && r.away[1]);  // it keeps its bed
    CHECK(!storeAway(s, 1));
    CHECK(stepsSince(s.dragons[1], 4000) == 3000 && stepsSince(s.dragons[1], 200) == 200);  // a reset counter

    CHECK(rollFinds(young, 0, rng).gleam == 0);
    // More steps, more finds; the curious and the grown find more.
    auto worth = [&](const Dragon& d, u32 steps) {
        u32 total = 0;
        for (int i = 0; i < 200; ++i) {
            const WanderFinds f = rollFinds(d, steps, rng);
            total += f.gleam;
            for (int k = 0; k < kTrinkets; ++k) total += f.trinkets[k] * trinketValue(static_cast<Trinket>(k));
        }
        return total;
    };
    Dragon curious = young;
    curious.personality = Personality::Curious;
    curious.stage = Stage::Adult;
    Dragon plain = young;
    plain.personality = Personality::Brave;
    CHECK(worth(plain, 4000) > worth(plain, 1000) * 3);
    CHECK(worth(curious, 4000) > worth(plain, 4000) * 1.3f);

    const WanderFinds f = comeBack(s, 1, 1000 + 5000, kT0 + 3 * kHour, rng);
    CHECK(f.steps == 5000 && wandererIndex(s) == -1 && s.dragons[1].wanderSince == 0);
    u32 trinkets = 0;
    for (int k = 0; k < kTrinkets; ++k) {
        trinkets += f.trinkets[k];
        CHECK(s.hoard[k] == f.trinkets[k]);
    }
    CHECK(s.gleam == 50 + f.gleam && f.gleam + trinkets > 0);  // on top of the starting 50
    CHECK(s.dragons[1].mud[kRegionBelly] > 50 && s.dragons[1].mud[kRegionBelly] > s.dragons[1].mud[kRegionBack]);
    CHECK(s.dragons[1].dirt[kRegionBelly] > 20);  // and a little dusty

    // Long trips sometimes bring a wild egg, mostly Grove, Frost or Lumen.
    int eggs = 0, newBreeds = 0;
    for (int trip = 0; trip < 80; ++trip) {
        s.dragons[1].wanderSince = 0;
        CHECK(setOff(s, 1, 0, kT0));
        const WanderFinds w = comeBack(s, 1, 10000, kT0 + kHour, rng);
        if (w.wildEgg >= 0) {
            ++eggs;
            newBreeds += s.dragons[w.wildEgg].genome.elementA >= 3;
            CHECK(s.dragons[w.wildEgg].stage == Stage::Egg);
        }
    }
    std::printf("  80 long trips: %d wild eggs, %d of the other breeds\n", eggs, newBreeds);
    CHECK(eggs >= 3 && eggs <= 40 && newBreeds * 2 > eggs);
}

// The Market (Alpha 2 WP5): it opens at Juvenile; food for Gleam into the pouch and eaten
// from it; trinkets sold; one egg of the day, labelled, the same all day.
// Things to keep (Alpha 2 WP7): bought once; toys set down on the den floor; decor up in its
// spot at once if it's free, swapped later; the food bowl feeds the hungry; warm stones.
// The profile (Alpha 2 WP8): trait names, stats from breed aptitude and stage, what you've
// found out, and the family: parents, grandparents, young, and where an egg came from.
TEST(a_dragons_profile) {
    for (u8 v = 0; v < 6; ++v)
        CHECK(buildName(v) && hornsName(v) && frillName(v) && wingsName(v) && tailName(v) && patternName(v));
    CHECK(std::strcmp(hornsName(kHornsCrystal), "Crystal") == 0 && std::strcmp(patternName(kPatternRunes), "Runes") == 0);
    CHECK(rareName(0) == nullptr && std::strcmp(rareName(kRareLeucistic), "Leucistic") == 0);

    Rng rng(47);
    static SaveData s;
    s = SaveData{};
    // Two grandparent pairs, two parents, a child.
    auto add = [&](Element e, Sex sex, u32 mother, u32 father, Stage stage) {
        Dragon d = readyAdult(s.nextId++, e, sex, rng);
        d.stage = stage;
        d.motherId = mother;
        d.fatherId = father;
        s.dragons[s.dragonCount++] = d;
        return d.id;
    };
    s.nextId = 1;
    const u32 gm1 = add(Element::Ember, Sex::Female, 0, 0, Stage::Adult), gf1 = add(Element::Tide, Sex::Male, 0, 0, Stage::Adult);
    const u32 gm2 = add(Element::Gale, Sex::Female, 0, 0, Stage::Adult), gf2 = add(Element::Frost, Sex::Male, 0, 0, Stage::Adult);
    const u32 mum = add(Element::Ember, Sex::Female, gm1, gf1, Stage::Adult), dad = add(Element::Gale, Sex::Male, gm2, gf2, Stage::Adult);
    add(Element::Lumen, Sex::Female, mum, dad, Stage::Hatchling);
    const Dragon& child = s.dragons[6];
    const Family f = familyOf(s, child);
    CHECK(f.mother == 4 && f.father == 5 && f.young == 0);
    CHECK(f.grand[0] == 0 && f.grand[1] == 1 && f.grand[2] == 2 && f.grand[3] == 3);
    CHECK(familyOf(s, s.dragons[4]).young == 1 && familyOf(s, s.dragons[0]).young == 1);
    CHECK(familyOf(s, s.dragons[0]).mother == -1 && indexOfId(s, 0) == -1 && indexOfId(s, dad) == 5);

    // Stats: the breed's aptitude shows (a pure Ember sparks, a Gale flies), and growing up adds.
    Dragon ember = s.dragons[0], gale = s.dragons[2];
    ember.genome.elementA = ember.genome.elementB = static_cast<u8>(Element::Ember);
    gale.genome.elementA = gale.genome.elementB = static_cast<u8>(Element::Gale);
    const Stats se = statsOf(ember), sg = statsOf(gale);
    CHECK(se.spark > se.wing && se.spark > se.wit && sg.wing > sg.spark);
    Dragon young = ember;
    young.stage = Stage::Hatchling;
    CHECK(statsOf(young).spark < se.spark && se.spark <= 100);

    // What you know: the sweet spot reads as a place; the egg's origin as a story.
    for (u32 id = 1; id < 40; ++id) {
        Dragon d;
        d.id = id;
        CHECK(sweetSpotText(d) && std::strlen(sweetSpotText(d)) > 4);
    }
    CHECK(std::strcmp(originText(child), "Your very first egg") == 0);  // (no origin set here)
    s.gleam = 1000;
    const int bought = buyDailyEgg(s, kT0);
    CHECK(bought >= 0 && s.dragons[bought].origin == Origin::Market);
    CHECK(std::strstr(originText(s.dragons[bought]), "Market") != nullptr);
}

TEST(things_to_keep) {
    static SaveData s;
    s = SaveData{};
    Rng rng(43);
    s.dragons[s.dragonCount++] = readyAdult(1, Element::Ember, Sex::Male, rng);
    s.dragons[0].location = Location::Den;
    s.gleam = 1000;

    CHECK(!owns(s, Item::TugRope) && buyItem(s, Item::TugRope) && owns(s, Item::TugRope));
    CHECK(s.gleam == 1000 - itemInfo(Item::TugRope).price);
    CHECK(!buyItem(s, Item::TugRope));  // once: things last
    const Vec2 rope = toyAt(s, static_cast<int>(Item::TugRope)), spot = defaultToySpot(static_cast<int>(Item::TugRope));
    CHECK(std::fabs(rope.x - spot.x) < 0.01f && std::fabs(rope.y - spot.y) < 0.01f);
    setToyAt(s, 1, {3.21f, -4.5f});
    CHECK(std::fabs(toyAt(s, 1).x - 3.21f) < 0.01f && std::fabs(toyAt(s, 1).y + 4.5f) < 0.01f);

    // Decor: the first of a kind goes up; the next waits in the chest until swapped in.
    CHECK(decorAt(s, decorSpot(ItemKind::Rug)) == Item::Count);
    CHECK(buyItem(s, Item::RugTide) && decorAt(s, decorSpot(ItemKind::Rug)) == Item::RugTide);
    CHECK(buyItem(s, Item::RugGrove) && isUp(s, Item::RugTide) && !isUp(s, Item::RugGrove));
    CHECK(putUp(s, Item::RugGrove) && isUp(s, Item::RugGrove) && !isUp(s, Item::RugTide));
    CHECK(!putUp(s, Item::RugLumen) && !putUp(s, Item::TugRope));  // not owned; not decor
    takeDown(s, decorSpot(ItemKind::Rug));
    CHECK(decorAt(s, decorSpot(ItemKind::Rug)) == Item::Count);
    for (int k = 0; k < kItems; ++k) {  // every item has a name, a price and a sensible spot
        const ItemInfo& info = itemInfo(static_cast<Item>(k));
        CHECK(info.name && info.price > 0);
        CHECK((decorSpot(info.kind) >= 0) == (info.kind >= ItemKind::Rug));
        CHECK(decorSpot(info.kind) < kDecorSpots);
    }
    s.gleam = 5;
    CHECK(!buyItem(s, Item::PlantFern));  // not enough Gleam

    // The food bowl: any foods from the pouch, six portions; the hungry eat from it.
    CHECK(!fillBowl(s, Food::HearthBread));  // no bowl yet
    s.gleam = 500;
    CHECK(buyItem(s, Item::FoodBowl));
    for (u16& n : s.pouch) n = 5;
    CHECK(fillBowl(s, Food::HearthBread) && fillBowl(s, Food::HearthBread));
    CHECK(s.pouch[static_cast<int>(Food::HearthBread)] == 3 && bowlCount(s) == 2);
    CHECK(fillBowl(s, Food::Skyberry) && bowlFood(s) == Food::Skyberry);  // any food, stacked
    CHECK(fillBowl(s, Food::HearthBread) && fillBowl(s, Food::HearthBread) && fillBowl(s, Food::HearthBread));
    CHECK(bowlCount(s) == kBowlPortions && !fillBowl(s, Food::HearthBread));  // full
    s.dragons[0].needs.belly = 60;
    CHECK(feedFromBowl(s, kT0) == 0);  // not hungry
    s.dragons[0].needs.belly = 20;
    CHECK(feedFromBowl(s, kT0) == 1 && s.dragons[0].needs.belly > 20 && bowlCount(s) == kBowlPortions - 1);
    s.dragons[0].location = Location::Sanctuary;
    s.dragons[0].needs.belly = 20;
    CHECK(feedFromBowl(s, kT0) == 0);  // the keepers feed that one
    s.dragons[0].location = Location::Den;
    // Its favourite goes first, wherever it lies in the bowl (favourites are element foods).
    s.dragons[0].favoriteFood = static_cast<u8>(Food::Skyberry);
    CHECK(eatFromBowl(s, 0, kT0));
    for (int k = 0; k < bowlCount(s); ++k) CHECK(s.bowl[k] != static_cast<u8>(Food::Skyberry));
    while (bowlCount(s) > 0) CHECK(eatFromBowl(s, 0, kT0));
    CHECK(bowlFood(s) == Food::Count && !eatFromBowl(s, 0, kT0));  // empty

    // Warm stones: eggs cool half as fast.
    Dragon a = makeEgg(9, makePurebred(Element::Tide, rng), Sex::Female, kT0), b = a;
    a.warmth = b.warmth = 90;
    simulate(a, kT0, kT0 + 10 * kHour);
    simulate(b, kT0, kT0 + 10 * kHour, eggCooling(s));
    CHECK(eggCooling(s) == 1.0f && std::fabs(a.warmth - b.warmth) < 0.01f);
    CHECK(buyItem(s, Item::WarmStones) && eggCooling(s) == 0.5f);
    b.warmth = 90;
    simulate(b, kT0, kT0 + 10 * kHour, eggCooling(s));
    CHECK(std::fabs((90 - b.warmth) * 2 - (90 - a.warmth)) < 0.01f);
}

TEST(the_market) {
    static SaveData s;
    s = SaveData{};
    Rng rng(41);
    Dragon d = readyAdult(1, Element::Ember, Sex::Male, rng);
    d.stage = Stage::Hatchling;
    s.dragons[s.dragonCount++] = d;
    s.nextId = 2;
    CHECK(!marketOpen(s));
    s.dragons[0].stage = Stage::Juvenile;
    CHECK(marketOpen(s));

    CHECK(s.gleam == 50 && pouchCount(s, Food::HearthBread) == 8);  // the starting pouch
    CHECK(buyFood(s, Food::RoastDrumstick) && s.gleam == 50 - foodPrice(Food::RoastDrumstick));
    CHECK(pouchCount(s, Food::RoastDrumstick) == 3);
    s.gleam = 3;
    CHECK(!buyFood(s, Food::HearthBread) && pouchCount(s, Food::HearthBread) == 8);  // not enough Gleam
    CHECK(useFood(s, Food::HearthBread) && pouchCount(s, Food::HearthBread) == 7);
    CHECK(!useFood(s, Food::Starfruit));  // none in the pouch

    s.hoard[static_cast<int>(Trinket::Pearl)] = 1;
    CHECK(sellTrinket(s, Trinket::Pearl) && s.gleam == 3 + trinketValue(Trinket::Pearl));
    CHECK(!sellTrinket(s, Trinket::Pearl));

    const s32 day = dayIndex(kT0);
    const DailyEgg a = dailyEgg(s, day), b = dailyEgg(s, day);
    CHECK(std::memcmp(&a.genome, &b.genome, sizeof(Genome)) == 0 && a.sex == b.sex);  // the same all day
    int others = 0;
    for (int k = 0; k < 60; ++k) others += dailyEgg(s, day + k).genome.elementA >= 3;
    CHECK(others > 25);  // mostly Grove, Frost and Lumen
    s.gleam = 1000;
    const int e = buyDailyEgg(s, kT0);
    CHECK(e >= 0 && s.dragons[e].stage == Stage::Egg && s.dragons[e].sex == a.sex && s.gleam == 1000 - a.price);
    CHECK(s.dragons[e].location == Location::Den);  // into a nest
    CHECK(buyDailyEgg(s, kT0 + kHour) == -1);  // one a day
    CHECK(buyDailyEgg(s, kT0 + kDay) >= 0);
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
    s.settings.seenHatch = 1;
    s.nestA = 11;
    s.nestB = 12;
    s.nestDay = 77;
    s.gleam = 1234;
    s.hoard[2] = 5;
    s.pouch[4] = 9;
    s.eggBoughtDay = 321;
    s.owned = 0x5u;
    s.decor[1] = 12;
    s.toyPos[2][1] = -345;
    s.bowl[0] = 6;
    s.bowl[1] = 2;
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
        d.eggTurns = static_cast<u8>(i % 3);
        d.denSlot = static_cast<u8>(i % 3);
        d.wanderSince = i == 2 ? kT0 + 5 : 0;
        d.wanderSteps = static_cast<u32>(i * 100);
        d.origin = static_cast<Origin>(i % static_cast<int>(Origin::Count));
        d.known = static_cast<u8>(i & 3);
        d.look = static_cast<u8>(i % kLookCount);
        d.lastTurnedAt = kT0 + i * kHour;
        d.motherId = i;
        d.location = static_cast<Location>(i % 2);
        d.xp = static_cast<u32>(i * 777);  // 1.0: a trainer's dragon and its record
        d.trained[i] = static_cast<u8>(i + 2);
        d.moves[0] = static_cast<u8>(i);
        d.wear[1] = static_cast<u8>(10 + i);
        d.dye = static_cast<u8>(i);
        d.battleTitle = static_cast<u8>(i % 5);
        d.showTitle = static_cast<u8>((i + 1) % 5);
        d.battleWins = static_cast<u16>(100 + i);
        d.showWins = static_cast<u16>(200 + i);
        d.wildWins = static_cast<u16>(300 + i);
        d.cupsWon = static_cast<u16>(0x0A5 << i & 0xFFF);
        d.ribbons = 0x80000001u >> i;
        d.frostDeepest = static_cast<u8>(i * 3);
        d.needs.love = 12.5f * i;
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
           std::memcmp(a.dirt, b.dirt, sizeof(a.dirt)) == 0 && a.eggTurns == b.eggTurns &&
           a.lastTurnedAt == b.lastTurnedAt && a.denSlot == b.denSlot && a.wanderSince == b.wanderSince &&
           a.wanderSteps == b.wanderSteps && a.origin == b.origin && a.known == b.known && a.look == b.look &&
           a.needs.love == b.needs.love && a.xp == b.xp && std::memcmp(a.trained, b.trained, sizeof(a.trained)) == 0 &&
           std::memcmp(a.moves, b.moves, sizeof(a.moves)) == 0 && std::memcmp(a.wear, b.wear, sizeof(a.wear)) == 0 &&
           a.dye == b.dye && a.battleTitle == b.battleTitle && a.showTitle == b.showTitle &&
           a.battleWins == b.battleWins && a.showWins == b.showWins && a.wildWins == b.wildWins &&
           a.cupsWon == b.cupsWon && a.ribbons == b.ribbons && a.frostDeepest == b.frostDeepest;
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
    d.needs.clean = 30;
    bathe(d);  // the bath is the only way to clean up (D83)
    for (float dust : d.dirt) CHECK(dust == 0.0f);
    CHECK(d.needs.clean == 100.0f);
    d.location = Location::Sanctuary;
    simulate(d, kT0 + 80 * kHour, kT0 + 104 * kHour);
    CHECK(d.dirt[kRegionBelly] < 10.0f);
}

// Mud (D46, WP12): brushing lifts it at half the dust's rate, the bath at once, and it flakes
// off by itself over a few days. The dirt ramp puts the mud over the dust.
TEST(mud_brushes_out_and_washes_off) {
    Rng rng(6);
    Dragon d = makeEgg(1, makePurebred(Element::Tide, rng), Sex::Male, kT0);
    d.incubationSeconds = kIncubationSeconds;
    tryHatch(d, kT0 + 8 * kHour, rng);
    for (float m : d.mud) CHECK(m == 0.0f);
    d.mud[kRegionBelly] = 80;
    d.mud[kRegionTail] = 50;
    simulate(d, kT0 + 8 * kHour, kT0 + 32 * kHour);  // a day: 36 flaked off
    CHECK(std::fabs(d.mud[kRegionBelly] - 44.0f) < 0.5f && d.mud[kRegionTail] < 15.0f);
    d.mud[kRegionLeft] = 100;
    bathe(d);
    for (float m : d.mud) CHECK(m == 0.0f);
    Rgb c;
    float a;
    dirtTexel(0, 0, c, a);
    CHECK(a == 0.0f);
    dirtTexel(0.4f, 0, c, a);
    CHECK(std::fabs(a - 0.4f) < 1e-4f && c.r == kDustColor.r && c.g == kDustColor.g && c.b == kDustColor.b);
    dirtTexel(0, 1, c, a);
    CHECK(std::fabs(a - kMudMax) < 1e-4f && c.r == kMudColor.r && c.g == kMudColor.g && c.b == kMudColor.b);
    dirtTexel(0.4f, 0.5f, c, a);
    CHECK(a > 0.4f && a < 1.0f && c.r < kDustColor.r && c.r > kMudColor.r);
    // Spots: some of the surface, not all of it, and the same every time.
    int spotted = 0, samples = 0;
    for (float x = -0.6f; x <= 0.6f; x += 0.013f)
        for (float z = 0.1f; z <= 0.7f; z += 0.031f, ++samples) spotted += mudSpots({x, 0.07f, z}) > 0.5f;
    std::printf("  mud spots over %.0f%% of a slice\n", 100.0f * spotted / samples);
    CHECK(spotted > samples / 5 && spotted < samples * 7 / 10);
    CHECK(mudSpots({0.21f, -0.1f, 0.4f}) == mudSpots({0.21f, -0.1f, 0.4f}));
}

// The den (Alpha 2 WP1): three dragons, one to a bed, and two eggs, one to a nest; places are
// kept; whoever doesn't fit moves out; a ready egg waits for a free bed.
TEST(the_den_has_three_beds_and_two_nests) {
    static SaveData s;
    s = SaveData{};
    Rng rng(8);
    for (int i = 0; i < 7; ++i) {
        Dragon d = makeEgg(100 + i, makePurebred(Element::Ember, rng), Sex::Female, kT0);
        if (i < 4) {
            d.incubationSeconds = kIncubationSeconds;
            tryHatch(d, kT0 + kHour, rng);
        }
        d.denSlot = static_cast<u8>(i == 2 ? 1 : 0);  // clashes on purpose
        s.dragons[s.dragonCount++] = d;
    }
    CHECK(settleDen(s) == 2);  // one dragon to the Sanctuary, one egg to the Vault
    const DenRoster r = denRoster(s);
    CHECK(r.dragonCount == 3 && r.eggCount == 2);
    CHECK(r.freeBed() == -1 && r.freeNest() == -1 && bedForHatchling(s) == -1);
    CHECK(r.dragon[1] == 2);  // it kept the bed it had
    int sanctuary = 0, vault = 0;
    for (int i = 0; i < s.dragonCount; ++i) {
        sanctuary += s.dragons[i].location == Location::Sanctuary;
        vault += s.dragons[i].location == Location::Vault;
    }
    CHECK(sanctuary == 1 && vault == 1);
    CHECK(settleDen(s) == 0);  // settled stays settled

    Dragon extra = makeEgg(200, makePurebred(Element::Tide, rng), Sex::Male, kT0);
    CHECK(!placeEgg(s, extra) && extra.location == Location::Vault);  // the nests are full
    // A dragon out on the Wanderings lends its bed: the hatchling takes it, the wanderer
    // comes home to the Sanctuary.
    s.dragons[r.dragon[0]].wanderSince = kT0;
    CHECK(bedForHatchling(s) == 0);
    CHECK(makeRoomForHatchling(s) == 0 && s.dragons[r.dragon[0]].location == Location::Sanctuary);
    CHECK(s.dragons[r.dragon[0]].wanderSince == kT0);  // still out walking
    CHECK(denRoster(s).freeBed() == 0);
    s.dragons[r.dragon[0]].wanderSince = 0;
    CHECK(bedForHatchling(s) == 0);  // (that bed is free now)
    s.dragons[r.egg[1]].location = Location::Vault;
    Dragon egg = makeEgg(201, makePurebred(Element::Gale, rng), Sex::Male, kT0);
    CHECK(placeEgg(s, egg) && egg.location == Location::Den && egg.denSlot == 1);
}

// The Sanctuary and the Cold Vault (Alpha 2 WP2, GDD 8): a stored dragon is looked after and
// doesn't grow; a vaulted egg waits; bringing one home needs a free bed or nest.
TEST(the_sanctuary_and_the_vault) {
    static SaveData s;
    s = SaveData{};
    Rng rng(12);
    Dragon d = makeEgg(1, makePurebred(Element::Tide, rng), Sex::Male, kT0);
    d.incubationSeconds = kIncubationSeconds;
    tryHatch(d, kT0, rng);
    s.dragons[s.dragonCount++] = d;
    Dragon egg = makeEgg(2, makePurebred(Element::Gale, rng), Sex::Female, kT0);
    egg.incubationSeconds = kIncubationSeconds / 2;
    CHECK(placeEgg(s, egg));
    s.dragons[s.dragonCount++] = egg;

    CHECK(storeAway(s, 0) && s.dragons[0].location == Location::Sanctuary);
    CHECK(storeAway(s, 1) && s.dragons[1].location == Location::Vault && vaultCount(s) == 1);
    CHECK(!storeAway(s, 0));  // already away
    const s64 later = kT0 + 10 * kDay;
    for (int i = 0; i < s.dragonCount; ++i) simulate(s.dragons[i], kT0, later);
    const Dragon& kept = s.dragons[0];
    CHECK(daysSinceHatch(kept, later) == 0 && kept.stage == Stage::Hatchling);  // no growing up while away
    CHECK(kept.needs.lowest() >= 50 && !kept.upset);
    CHECK(s.dragons[1].incubationSeconds == kIncubationSeconds / 2);  // the egg waited

    CHECK(bringHome(s, 0, later) && s.dragons[0].location == Location::Den);
    simulate(s.dragons[0], later, later + 2 * kHour);
    CHECK(!s.dragons[0].upset);  // welcomed home, not "you never visit"
    CHECK(bringHome(s, 1, later) && s.dragons[1].location == Location::Den && denRoster(s).eggCount == 1);

    // Full places: the den's three beds, the Vault's fifty eggs.
    for (int i = 0; i < 3; ++i) {
        Dragon x = makeEgg(10 + i, makePurebred(Element::Ember, rng), Sex::Female, kT0);
        x.incubationSeconds = kIncubationSeconds;
        tryHatch(x, kT0, rng);
        x.location = Location::Sanctuary;
        s.dragons[s.dragonCount++] = x;
    }
    CHECK(bringHome(s, 2, later) && bringHome(s, 3, later) && !bringHome(s, 4, later));
    while (vaultCount(s) < kVaultEggs) {
        Dragon x = makeEgg(100 + s.dragonCount, makePurebred(Element::Frost, rng), Sex::Male, kT0);
        x.location = Location::Vault;
        s.dragons[s.dragonCount++] = x;
    }
    CHECK(!storeAway(s, 1));  // the Vault is full
}

TEST(save_round_trip) {
    const SaveData& s = sampleSave();
    std::vector<u8> buf(maxEncodedSize());
    const std::size_t n = encodeSave(s, 7, kT0 + 99, buf.data(), buf.size());
    CHECK(n > kSaveHeaderSize);
    CHECK(n == kSaveHeaderSize + 16 + 8 + 8 + 4 + 12 + 16 + 24 + 27 + kBowlSlots + kBreedCount + 6 + (1 + kDexKindSlots + 8 + 1) + (1 + kWorldBytes) + (2 + kProgressBytes + kProgressCoveBytes + kProgressCritterBytes + kProgressRoamBytes) + 2 + 5 + 2 + 2 +
                   5 * (132 + 16 + 9 + 1 + 12 + 2 + 2 + 1 + kRegionCount + 12 + 37));
    static SaveData out;
    SaveHeaderInfo info;
    CHECK(decodeSave(buf.data(), n, out, &info) == LoadResult::Ok);
    CHECK(info.seq == 7 && info.savedAt == kT0 + 99 && info.version == kSaveVersion);
    CHECK(std::strcmp(out.playerName, "Noah") == 0);
    CHECK(out.lastSim == s.lastSim && out.devOffset == s.devOffset && out.nextId == 42);
    CHECK(out.nestA == 11 && out.nestB == 12 && out.nestDay == 77 && out.gleam == 1234 && out.hoard[2] == 5);
    CHECK(out.pouch[4] == 9 && out.pouch[6] == 8 && out.eggBoughtDay == 321);
    CHECK(out.owned == 0x5u && out.decor[1] == 12 && out.decor[0] == 0xFF && out.toyPos[2][1] == -345);
    CHECK(out.bowl[0] == 6 && out.bowl[1] == 2 && out.bowl[2] == 0xFF && bowlCount(out) == 2);
    CHECK(out.settings.musicVolume == 55 && out.settings.seenHatch == 1);
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

// The looks (D54): the base odds, inheritance with surprises (wild rare unless a parent is
// wild), the same look every time for an old save's dragon, the names, and the save field
// (records from before it get their look from the id).
TEST(the_looks) {
    Rng rng(77);
    int base[kLookCount] = {};
    for (int i = 0; i < 20000; ++i) ++base[rollLook(rng)];
    std::printf("  base odds: %d %d %d %d (of 20000)\n", base[0], base[1], base[2], base[3]);
    CHECK(std::abs(base[0] - 6200) < 400 && std::abs(base[1] - 6200) < 400 && std::abs(base[2] - 6000) < 400 &&
          std::abs(base[3] - 1600) < 250);
    auto wildShare = [&](u8 a, u8 b) {
        int wild = 0, same = 0;
        for (int i = 0; i < 20000; ++i) {
            const Look l = inheritLook(a, b, rng);
            wild += l == kLookWild;
            same += l == a || l == b;
        }
        return std::pair<float, float>(wild / 20000.0f, same / 20000.0f);
    };
    const auto plain = wildShare(kLookClassic, kLookTallneck);
    const auto oneWild = wildShare(kLookWild, kLookPebbleback);
    const auto bothWild = wildShare(kLookWild, kLookWild);
    std::printf("  inherited: common parents %.1f%% wild (%.0f%% a parent's), one wild %.1f%%, both %.1f%%\n",
                plain.first * 100, plain.second * 100, oneWild.first * 100, bothWild.first * 100);
    CHECK(plain.first < 0.03f && plain.second > 0.8f);  // usually a parent's; wild stays rare
    CHECK(oneWild.first > 0.21f && oneWild.first < 0.29f && bothWild.first > 0.21f && bothWild.first < 0.29f);
    CHECK(lookForOldDragon(12345) == lookForOldDragon(12345));
    int old[kLookCount] = {};
    for (u32 id = 1; id <= 4000; ++id) ++old[lookForOldDragon(id)];
    CHECK(old[3] > 200 && old[3] < 450 && old[0] > 1000 && old[1] > 1000 && old[2] > 1000);

    Genome ember = makePurebred(Element::Ember, rng), tide = makePurebred(Element::Tide, rng);
    char name[40];
    lookBreedName(kLookClassic, ember, name, sizeof(name));
    CHECK(std::strcmp(name, "Classic Ember") == 0);
    lookBreedName(kLookPebbleback, tide, name, sizeof(name));
    CHECK(std::strcmp(name, "Pebbleback Tide") == 0);
    lookBreedName(kLookWild, ember, name, sizeof(name));
    CHECK(std::strcmp(name, "Cinderveined Ember") == 0);
    lookBreedName(kLookWild, tide, name, sizeof(name));
    CHECK(std::strcmp(name, "Glimmertide") == 0);
    Genome squall = tide;
    squall.elementB = static_cast<u8>(Element::Gale);
    lookBreedName(kLookWild, squall, name, sizeof(name));
    CHECK(std::strcmp(name, "Glimmertide Squall") == 0);
    Genome steam = ember;
    steam.elementB = static_cast<u8>(Element::Tide);
    lookBreedName(kLookWild, steam, name, sizeof(name));
    CHECK(std::strcmp(name, "Cinderveined Steam") == 0);
    // Since DR3 a dragon goes by its kind: an egg by the kind alone (its colouring a surprise
    // until it hatches), a hatched one by its colouring and kind.
    Dragon egg = makeEgg(5, tide, Sex::Male, kT0, kLookTallneck);
    egg.kind = static_cast<u8>(findKind("pouncer"));
    egg.variant = 1;
    kindName(egg, name, sizeof(name));
    CHECK(std::strcmp(name, "Pouncer") == 0);
    egg.stage = Stage::Hatchling;
    kindName(egg, name, sizeof(name));
    CHECK(std::strcmp(name, "Tabby Pouncer") == 0);

    // A record from before the looks: cut the field off, and its look comes from its id.
    SaveData& s = sampleSave();
    s.dragonCount = 1;
    s.dragons[0].look = kLookWild;
    std::vector<u8> buf(maxEncodedSize());
    const std::size_t n = encodeSave(s, 1, kT0, buf.data(), buf.size());
    std::vector<u8> old1(buf.begin(), buf.begin() + n);
    constexpr std::size_t kKindBytes = 12;  // DR3's fields, after the look and the mud
    constexpr std::size_t kTrainerBytes = 37;  // 1.0's, after them
    const std::size_t rec = n - (132 + 16 + 9 + 1 + 12 + 2 + 1 + kRegionCount + kKindBytes + kTrainerBytes) - 2;  // the record's size field
    old1[rec] = static_cast<u8>(old1[rec] - 1 - kRegionCount - kKindBytes - kTrainerBytes);  // no look, no mud, no kind
    old1.resize(old1.size() - 1 - kRegionCount - kKindBytes - kTrainerBytes);
    // Fix up the header's payload size and checksum for the shorter payload.
    const u32 payload = static_cast<u32>(old1.size() - kSaveHeaderSize);
    std::memcpy(&old1[12], &payload, 4);
    const u32 crc = crc32(old1.data() + kSaveHeaderSize, payload);
    std::memcpy(&old1[16], &crc, 4);
    static SaveData out;
    CHECK(decodeSave(old1.data(), old1.size(), out) == LoadResult::Ok);
    CHECK(out.dragons[0].look == lookForOldDragon(out.dragons[0].id));
    // ...and, from before the revamp, a kind in a common colouring, fixed by its id (D80).
    Dragon want = out.dragons[0];
    migrateToKind(want);
    CHECK(out.dragons[0].kind == want.kind && out.dragons[0].variant == want.variant && out.dragons[0].variant < 3);
    CHECK(std::memcmp(out.dragons[0].stats, want.stats, sizeof(want.stats)) == 0 && out.dragons[0].manner == want.manner);
    static SaveData again;  // the same every load
    CHECK(decodeSave(old1.data(), old1.size(), again) == LoadResult::Ok && again.dragons[0].kind == want.kind);
}

// The Dragondex (WP12; by kind since DR3): hatched dragons fill it in (eggs don't); all four
// colourings of a kind complete it once, for Gleam and its banner (hung the first time); a
// rare colouring is news; the book saves.
TEST(the_dragondex) {
    static SaveData s;
    s = SaveData{};
    const int pouncer = findKind("pouncer"), puff = findKind("puffback");
    Dragon d = makeEgg(1, Genome{}, Sex::Female, kT0);
    Rng rng(31);
    rollKind(d, pouncer, 0, rng);
    CHECK(!dexSee(s, d).newEntry && dexCount(s) == 0);  // an egg: its colouring is still a surprise
    d.stage = Stage::Hatchling;
    CHECK(dexSee(s, d).newEntry && !dexSee(s, d).newEntry && dexCount(s) == 1);
    CHECK(dexHas(s, pouncer, 0) && !dexHas(s, pouncer, 3) && !dexComplete(s, pouncer));
    const u32 gleam = s.gleam;
    DexNews last;
    for (u8 v : {1, 2, 3}) {
        d.variant = v;
        last = dexSee(s, d);
    }
    CHECK(last.newRare && last.completed == pouncer && dexComplete(s, pouncer) && s.gleam == gleam + kDexBreedGleam);
    CHECK(bannerKind(s) == pouncer);  // up at once
    CHECK(dexSee(s, d).completed < 0 && s.gleam == gleam + kDexBreedGleam);  // paid once
    Dragon e = makeEgg(2, Genome{}, Sex::Male, kT0);
    rollKind(e, puff, 0, rng);
    e.stage = Stage::Adult;
    for (u8 v = 0; v < kKindVariants; ++v) {
        e.variant = v;
        dexSee(s, e);
    }
    CHECK(bannerKind(s) == pouncer && dexComplete(s, puff));  // a banner already hangs: kept
    CHECK(hangBanner(s, puff) && bannerKind(s) == puff);
    CHECK(!hangBanner(s, findKind("duskwing")));  // not complete
    takeDownBanner(s);
    CHECK(bannerKind(s) == -1);
    CHECK(dexCount(s) == 8 && dexRareCount(s) == 2 && dexEntries() == kindCount() * kKindVariants);
    // The book shows the same dragon for an entry every time, of the right kind and colouring.
    for (int k = 0; k < kindCount(); ++k) {
        const Dragon x = dexDragon(k, 3), y = dexDragon(k, 3);
        CHECK(x.kind == k && x.variant == 3 && x.stage == Stage::Adult && std::memcmp(x.stats, y.stats, sizeof(x.stats)) == 0);
    }
    // Each entry as you know it (run 21): young while yours are young, grown once one is, or when
    // none is yours.
    s.dragonCount = 0;
    CHECK(dexStage(s, pouncer, 3) == Stage::Adult);
    s.dragons[0] = d;  // (a pouncer hatchling, colouring 3)
    s.dragonCount = 1;
    CHECK(dexStage(s, pouncer, 3) == Stage::Hatchling && dexStage(s, pouncer, 0) == Stage::Adult);
    s.dragons[1] = d;
    s.dragons[1].stage = Stage::Egg;
    s.dragonCount = 2;
    CHECK(dexStage(s, pouncer, 3) == Stage::Hatchling);  // (an egg adds nothing)
    s.dragons[1].stage = Stage::Adult;
    CHECK(dexStage(s, pouncer, 3) == Stage::Adult);
    CHECK(dexDragon(pouncer, 3, Stage::Hatchling).stage == Stage::Hatchling && dexDragon(pouncer, 3, Stage::Egg).stage == Stage::Adult);
    // It saves.
    s.dragonCount = 0;
    std::vector<u8> buf(maxEncodedSize());
    const std::size_t n = encodeSave(s, 1, kT0, buf.data(), buf.size());
    static SaveData out;
    CHECK(decodeSave(buf.data(), n, out) == LoadResult::Ok);
    CHECK(std::memcmp(out.dexKinds, s.dexKinds, sizeof(s.dexKinds)) == 0 && out.dexKindsDone == s.dexKindsDone &&
          out.bannerKind == s.bannerKind);
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
    RUN(good_care_reaches_adult_in_about_five_and_a_half_days);
    RUN(light_care_grows_slower_but_still_grows);
    RUN(neglect_upsets_but_never_harms);
    RUN(sanctuary_keeps_needs_safe);
    RUN(dirt_settles_and_grooming_clears_it);
    RUN(mud_brushes_out_and_washes_off);
    RUN(body_scale_grows_every_day);
    RUN(breeding_needs_one_male_and_one_female);
    RUN(breeding_requirements);
    RUN(the_nesting_stone);
    RUN(the_wanderings);
    RUN(a_dragons_profile);
    RUN(things_to_keep);
    RUN(the_market);
    RUN(egg_sexes_are_roughly_even);
    RUN(the_den_has_three_beds_and_two_nests);
    RUN(the_sanctuary_and_the_vault);
    RUN(save_round_trip);
    RUN(save_detects_corruption_and_truncation);
    RUN(save_rejects_out_of_range_data);
    RUN(save_picks_newest_valid_slot);
    RUN(save_full_capacity_fits);
    RUN(the_looks);
    RUN(the_dragondex);
    runModelTests();
    runAnimTests();
    runBehaviorTests();
    runDenTests();
    runEggTests();
    runCareTests();
    runValleyTests();
    runKindTests();
    runWorldTests();
    runChallengeTests();
    runTrainerTests();
    runPlaceTests();
    runInterfaceTests();
    runFishingTests();  // Driftwood Cove (workstream C)
    runPageantTests();  // the pageant, accessories and dyes
    runBattleTests();  // 1.0 battles (workstream B)
    runCritterTests();  // the valley's critters (workstream L)
    runRoamerTests();  // roaming trainers and duels (workstream D)
    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
