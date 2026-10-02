// 1.0's trainer game (core/trainer, D89-D90): levels from experience, energy spent and slept
// back, Love apart from Play, walking together, the per-dragon record, the day's rewards paid
// once, and the save keeping it all (older saves and records carry over with defaults).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "check.hpp"
#include "core/battle.hpp"
#include "core/challenges.hpp"
#include "core/clock.hpp"
#include "core/flight.hpp"
#include "core/kinds.hpp"
#include "core/pageant.hpp"
#include "core/save.hpp"
#include "core/trainer.hpp"
#include "core/wanderings.hpp"

using namespace ec;

namespace {
constexpr s64 kT0 = 1767225600;  // 2026-01-01 00:00 local

Dragon hatched(u32 id) {
    Rng rng(id);
    Dragon d = makeEgg(id, makePurebred(Element::Ember, rng), Sex::Female, kT0);
    d.incubationSeconds = kIncubationSeconds;
    tryHatch(d, kT0 + 10 * kHour, rng);
    return d;
}
}  // namespace

TEST(trainer_levels) {
    CHECK(trainer::xpForLevel(1) == 0 && trainer::xpForLevel(2) == 50);
    for (int l = 2; l <= kMaxLevel; ++l) CHECK(trainer::xpForLevel(l) > trainer::xpForLevel(l - 1));
    CHECK(trainer::levelOf(0u) == 1 && trainer::levelOf(49u) == 1 && trainer::levelOf(50u) == 2);
    CHECK(trainer::levelOf(trainer::xpForLevel(kMaxLevel) + 99999) == kLevelCap);  // Skyreach's ceiling
    Dragon d = hatched(1);
    d.traitCount = 0;  // (no Quick Learner: the arithmetic below is the plain curve)
    CHECK(trainer::levelOf(d) == 1);
    CHECK(trainer::gainXp(d, 49) == 0 && trainer::gainXp(d, 1) == 1 && trainer::levelOf(d) == 2);
    const int gained = trainer::gainXp(d, trainer::xpForLevel(10) - d.xp);
    CHECK(gained == 8 && trainer::levelOf(d) == 10);
    u32 into = 0, span = 0;
    trainer::levelProgress(d, into, span);
    CHECK(into == 0 && span == trainer::xpForLevel(11) - trainer::xpForLevel(10));
    trainer::gainXp(d, 0xFFFFFFF0u);  // never wraps
    CHECK(trainer::levelOf(d) == kLevelCap && d.xp == trainer::xpForLevel(kLevelCap));
    trainer::levelProgress(d, into, span);
    CHECK(span == 0);
    std::printf("  level 5 at %u xp, 10 at %u, 30 at %u, 50 at %u\n", trainer::xpForLevel(5), trainer::xpForLevel(10),
                trainer::xpForLevel(30), trainer::xpForLevel(50));
}

TEST(trainer_stats_and_training) {
    Dragon d = hatched(2);
    const int wing = d.stats[kStatWing];
    CHECK(trainer::statPoints(d, kStatWing) == (wing < 1 ? 1 : wing));
    CHECK(trainer::train(d, kStatWing, 5) && trainer::statPoints(d, kStatWing) == (wing < 1 ? 1 : wing) + 5);
    CHECK(trainer::train(d, kStatWing, 100) && d.trained[kStatWing] == kMaxTrained);
    CHECK(!trainer::train(d, kStatWing));  // full
    CHECK(!trainer::train(d, 7) && trainer::statPoints(d, 9) == 1);
}

TEST(trainer_energy_love_and_walks) {
    Dragon d = hatched(3);
    d.needs.energy = 30;
    CHECK(trainer::spendEnergy(d, trainer::kEnergyChallenge) && d.needs.energy == 12);
    CHECK(!trainer::spendEnergy(d, trainer::kEnergyChallenge) && d.needs.energy == 12);  // too tired: nothing spent
    // Energy is shown apart: it doesn't count as the lowest need any more (Love does).
    d.needs = Needs{80, 5, 80, 80, 60};
    CHECK(d.needs.lowest() == 60);
    // Petting and brushing fill Love, not Play.
    d.needs.play = 50;
    d.needs.love = 50;
    pet(d, 10);
    brushed(d, 10);
    CHECK(d.needs.play == 50 && d.needs.love > 60);
    // Love wanes through the day, less asleep; the Sanctuary keeps it at 50 or more.
    Dragon e = hatched(4);
    e.lastVisitAt = kT0 + 12 * kHour;
    e.needs.love = 90;
    simulate(e, kT0 + 12 * kHour, kT0 + 18 * kHour);
    CHECK(e.needs.love < 90 && e.needs.love > 60);
    // Walking together: Play and Love up, Belly down a little, a bond point every 150 m.
    Dragon w = hatched(5);
    w.needs = Needs{80, 80, 80, 40, 40};
    const u16 bond = w.bond;
    float carry = 0;
    for (int k = 0; k < 10; ++k) trainer::walkTogether(w, 50.0f, carry);
    CHECK(std::fabs(w.needs.play - 55) < 0.01f && std::fabs(w.needs.love - 50) < 0.01f);
    CHECK(std::fabs(w.needs.belly - 72.5f) < 0.01f && w.bond == bond + 3 && std::fabs(carry - 50) < 0.01f);
}

TEST(trainer_record_and_titles) {
    Dragon d = hatched(6);
    trainer::recordCup(d, 1, 3);
    trainer::recordCup(d, 2, 4);
    trainer::recordCup(d, 9, 1);  // no such challenge
    CHECK(trainer::wonCup(d, 1, 3) && trainer::wonCup(d, 2, 4) && !trainer::wonCup(d, 0, 1) && trainer::cupCount(d) == 2);
    trainer::recordBattle(d, true);
    trainer::recordBattle(d, false);
    trainer::recordLeague(d, 2);
    trainer::recordLeague(d, 1);  // never down
    CHECK(d.battleWins == 1 && d.battleTitle == 2 && std::strcmp(trainer::battleTitleName(d.battleTitle), "Flame Victor") == 0);
    trainer::recordShow(d, true, 3);
    trainer::recordShow(d, true, 3);
    trainer::recordShow(d, false, 5);
    trainer::recordShowLeague(d, 4);
    CHECK(d.showWins == 2 && trainer::ribbonCount(d) == 1 && d.showTitle == 4);
    trainer::recordWild(d, true, 7);
    trainer::recordWild(d, true, 3);
    CHECK(d.wildWins == 2 && d.frostDeepest == 7);
    CHECK(std::strcmp(trainer::leagueName(4), "Starfire") == 0 && trainer::leagueName(9)[0] == '\0');
}

TEST(trainer_daily_claims_and_owning) {
    static SaveData s;
    s = SaveData{};
    const s32 day = 100;
    CHECK(trainer::claimToday(s, kClaimCup + 5, day));
    CHECK(!trainer::claimToday(s, kClaimCup + 5, day));  // the same cup again today: no reward
    CHECK(trainer::claimedToday(s, kClaimCup + 5, day) && !trainer::claimedToday(s, kClaimCup + 5, day + 1));
    CHECK(trainer::claimToday(s, kClaimHollow + 15, day));  // the top bit of the u64
    CHECK(trainer::claimToday(s, kClaimCup + 5, day + 1));  // a new day
    CHECK(!trainer::claimedToday(s, kClaimHollow + 15, day + 1));
    CHECK(!trainer::claimToday(s, 64, day));
    CHECK(!trainer::ownsAccessory(s, 42));
    trainer::giveAccessory(s, 42);
    trainer::giveAccessory(s, 63);
    trainer::giveAccessory(s, 64);  // out of room: ignored
    CHECK(trainer::ownsAccessory(s, 42) && trainer::ownsAccessory(s, 63) && !trainer::ownsAccessory(s, 41));
    CHECK(trainer::ownsDye(s, 0) && !trainer::ownsDye(s, 3));
    trainer::giveDye(s, 3);
    CHECK(trainer::ownsDye(s, 3));
    trainer::markTip(s, 4);
    CHECK(trainer::tipSeen(s, 4) && !trainer::tipSeen(s, 5));
    trainer::count(s, kCountFish, 3);
    trainer::count(s, kCountFish, 70000);
    CHECK(s.progress.counts[kCountFish] == 0xFFFF);
    trainer::track(s, Tracked::Hollow, 300);
    CHECK(trainer::tracked(s) == Tracked::Hollow && s.progress.trackId == 255);
}

TEST(trainer_save_round_trip_and_older_records) {
    static SaveData s, out;
    s = SaveData{};
    world::startWorld(s);
    s.progress.accessories[3] = 0x5A;
    s.progress.dyes = 0x12345678u;
    trainer::track(s, Tracked::Quest, 2);
    s.progress.battleLeague = 2;
    s.progress.battleBeaten[1] = 0x0F;
    s.progress.showLeague = 1;
    s.progress.showWon[0] = 0x07;
    s.progress.hollowDeepest = 9;
    s.progress.findsDay = 77;
    s.progress.findsSeed = 0xABCDEF01u;
    s.progress.claimDay = 78;
    s.progress.claims = 0x8000000000000001ull;
    s.progress.tips = 0x00F0u;
    s.progress.counts[kCountShells] = 12;
    s.dragonCount = 1;
    s.dragons[0] = hatched(7);
    s.dragons[0].xp = 1234;
    s.dragons[0].needs.love = 33;
    s.dragons[0].wear[0] = 5;
    std::vector<u8> buf(maxEncodedSize());
    std::size_t n = encodeSave(s, 1, kT0, buf.data(), buf.size());
    CHECK(n > 0 && decodeSave(buf.data(), n, out) == LoadResult::Ok);
    CHECK(std::memcmp(out.progress.accessories, s.progress.accessories, kAccessoryBytes) == 0);
    CHECK(out.progress.dyes == s.progress.dyes && trainer::tracked(out) == Tracked::Quest && out.progress.trackId == 2);
    CHECK(out.progress.battleLeague == 2 && out.progress.battleBeaten[1] == 0x0F && out.progress.showLeague == 1);
    CHECK(out.progress.showWon[0] == 0x07 && out.progress.hollowDeepest == 9 && out.progress.findsDay == 77);
    CHECK(out.progress.findsSeed == 0xABCDEF01u && out.progress.claimDay == 78 && out.progress.claims == s.progress.claims);
    CHECK(out.progress.tips == 0x00F0u && out.progress.counts[kCountShells] == 12);
    CHECK(out.dragons[0].xp == 1234 && out.dragons[0].needs.love == 33 && out.dragons[0].wear[0] == 5);
    // An older record (before 1.0): cut the trainer's 37 bytes off the one dragon's record.
    constexpr std::size_t kTrainer = 37;
    // The record is last: its u16 size sits right before it.
    std::size_t recStart = 0;
    for (std::size_t at = kSaveHeaderSize; at + 2 < n; ++at) {
        const std::size_t size = buf[at] | (buf[at + 1] << 8);
        if (at + 2 + size == n && size > 132 && size < 400) recStart = at;
    }
    CHECK(recStart > 0);
    const std::size_t oldSize = (buf[recStart] | (buf[recStart + 1] << 8)) - kTrainer;
    buf[recStart] = static_cast<u8>(oldSize);
    buf[recStart + 1] = static_cast<u8>(oldSize >> 8);
    n -= kTrainer;
    const u32 payload = static_cast<u32>(n - kSaveHeaderSize);
    std::memcpy(&buf[12], &payload, 4);
    const u32 crc = crc32(buf.data() + kSaveHeaderSize, payload);
    std::memcpy(&buf[16], &crc, 4);
    CHECK(decodeSave(buf.data(), n, out) == LoadResult::Ok);
    const Dragon& old = out.dragons[0];
    CHECK(old.xp == 0 && old.wear[0] == kNone && old.moves[3] == kNone && old.battleWins == 0 && old.dye == 0);
    CHECK(old.needs.love == old.needs.play);  // as fond as it was playful
    // An older save (before the progress block): a fresh one.
    static SaveData fresh;
    fresh = SaveData{};
    CHECK(fresh.progress.findsDay == -1000000 && fresh.progress.claims == 0 && trainer::tracked(fresh) == Tracked::None);
}

// Every trait does its one thing (D150, docs/design/traits.md), each against the same dragon without it.
TEST(every_trait_does_its_thing) {
    CHECK(kTraitIds == traitCount());
    CHECK(std::strcmp(traitName(kTraitSwift), "Swift") == 0 && std::strcmp(traitName(kTraitChatty), "Chatty") == 0 &&
          std::strcmp(traitName(kTraitLoyal), "Loyal") == 0 && std::strcmp(traitName(kTraitMossback), "Mossback") == 0 &&
          std::strcmp(traitName(kTraitSunkissed), "Sunkissed") == 0);
    Dragon plain = hatched(7);
    plain.kind = static_cast<u8>(findKind("pouncer"));  // (Ember)
    plain.variant = 0;
    plain.traitCount = 0;
    plain.manner = 0;  // Brave: no manner of its own on the needs
    plain.personality = Personality::Brave;
    auto with = [&](int t, int t2 = -1) {
        Dragon d = plain;
        d.traits[0] = static_cast<u8>(t);
        d.traitCount = 1;
        if (t2 >= 0) {
            d.traits[1] = static_cast<u8>(t2);
            d.traitCount = 2;
        }
        return d;
    };
    int checked = 0;
    auto mark = [&](int t, bool ok) {
        CHECK(ok);
        if (!ok) std::printf("  %s: no effect\n", traitName(t));
        ++checked;
    };
    // ---- The needs and moods: hours lived from a time of day (kT0 is a midnight).
    auto lived = [&](Dragon d, int fromHour, int hours, float level = 80.0f) {
        d.needs = Needs{level, level, level, level, level};
        d.napping = false;
        d.upset = false;
        d.sulkyHours = 0;
        d.lastVisitAt = kT0 + fromHour * kHour;
        simulate(d, kT0 + fromHour * kHour, kT0 + (fromHour + hours) * kHour, 1.0f);
        return d;
    };
    mark(kTraitTidy, lived(with(kTraitTidy), 10, 4).needs.clean > lived(plain, 10, 4).needs.clean);
    {
        const Dragon a = lived(with(kTraitEarlyRiser), 8, 3), p = lived(plain, 8, 3), later = lived(with(kTraitEarlyRiser), 13, 3);
        mark(kTraitEarlyRiser, a.needs.play > p.needs.play && a.needs.love > p.needs.love &&
                                   std::fabs(later.needs.play - lived(plain, 13, 3).needs.play) < 0.01f);
    }
    {
        const Dragon a = lived(with(kTraitNightOwl), 18, 3), p = lived(plain, 18, 3);
        mark(kTraitNightOwl, a.needs.play > p.needs.play && a.needs.love > p.needs.love);
    }
    {
        const Dragon a = lived(with(kTraitSunbather), 11, 3), p = lived(plain, 11, 3);
        mark(kTraitSunbather, a.needs.love > p.needs.love && std::fabs(a.needs.play - p.needs.play) < 0.01f);
    }
    mark(kTraitDeepSleeper, lived(with(kTraitDeepSleeper), 23, 3, 20.0f).needs.energy > lived(plain, 23, 3, 20.0f).needs.energy);
    // A long sulk (needs low for 30 hours): upset, but not with Phoenix Heart.
    mark(kTraitPhoenixHeart, lived(plain, 9, 30, 10.0f).upset && !lived(with(kTraitPhoenixHeart), 9, 30, 10.0f).upset);
    // Four days away: upset (Phoenix Heart keeps the sulk out of it), but not when it's Loyal too.
    mark(kTraitLoyal, lived(with(kTraitPhoenixHeart), 9, 96).upset && !lived(with(kTraitPhoenixHeart, kTraitLoyal), 9, 96).upset);
    {
        Dragon a = with(kTraitGlowheart), p = plain;
        a.upset = p.upset = false;
        a.needs = p.needs = Needs{77, 50, 77, 77, 77};
        mark(kTraitGlowheart, moodOf(a) == Mood::Joyful && moodOf(p) == Mood::Content);
    }
    // ---- Care.
    {
        Dragon a = with(kTraitHeartyEater), p = plain;
        a.needs.belly = p.needs.belly = 10;
        feed(a, 20, false);
        feed(p, 20, false);
        mark(kTraitHeartyEater, a.needs.belly > p.needs.belly + 4.9f);
    }
    {
        Dragon a = with(kTraitCuddly), p = plain;
        a.upset = p.upset = false;
        a.bond = p.bond = 100;
        pet(a, 10);
        pet(p, 10);
        mark(kTraitCuddly, a.bond == p.bond + 1);
    }
    {
        Dragon a = with(kTraitWaterLover), p = plain;
        a.needs.play = p.needs.play = a.needs.love = p.needs.love = 40;
        bathe(a);
        bathe(p);
        mark(kTraitWaterLover, a.needs.play == p.needs.play + 20 && a.needs.love == p.needs.love + 10);
    }
    // ---- Energy, experience, stats and walks.
    {
        Dragon a = with(kTraitSturdy), p = plain;
        a.needs.energy = p.needs.energy = 50;
        trainer::spendEnergy(a, 12);
        trainer::spendEnergy(p, 12);
        mark(kTraitSturdy, std::fabs(a.needs.energy - 41.0f) < 0.01f && std::fabs(p.needs.energy - 38.0f) < 0.01f);
    }
    {
        Dragon a = with(kTraitQuickLearner), p = plain;
        a.xp = p.xp = 0;
        trainer::gainXp(a, 100);
        trainer::gainXp(p, 100);
        mark(kTraitQuickLearner, a.xp == 120 && p.xp == 100);
    }
    {
        const Dragon a = with(kTraitStarborn);
        bool all = true;
        for (int s = 0; s < kDragonStats; ++s) all &= trainer::statPoints(a, s) == trainer::statPoints(plain, s) + 1;
        mark(kTraitStarborn, all);
    }
    {
        Dragon a = with(kTraitChatty), p = plain;
        a.upset = p.upset = false;
        a.bond = p.bond = 100;
        float ca = 0, cp = 0;
        trainer::walkTogether(a, 300, ca);
        trainer::walkTogether(p, 300, cp);
        mark(kTraitChatty, a.bond == 104 && p.bond == 102);
    }
    // ---- The Wanderings: the same walk, many times.
    {
        u32 keen = 0, base = 0, lucky = 0, baseGleam = 0;
        bool hunter = true;
        for (u32 i = 0; i < 300; ++i) {
            Rng r1(500 + i), r2(500 + i), r3(500 + i), r4(500 + i);
            const WanderFinds p = rollFinds(plain, 4000, r1);
            const WanderFinds k = rollFinds(with(kTraitKeenNose), 4000, r2);
            const WanderFinds l = rollFinds(with(kTraitLucky), 4000, r3);
            const WanderFinds h = rollFinds(with(kTraitTreasureHunter), 4000, r4);
            for (int t = 0; t < kTrinkets; ++t) {
                base += p.trinkets[t];
                keen += k.trinkets[t];
            }
            baseGleam += p.gleam;
            lucky += l.gleam;
            hunter &= h.gleam == p.gleam + p.gleam / 2;
        }
        mark(kTraitKeenNose, keen > base * 1.1f);
        mark(kTraitLucky, lucky > baseGleam * 1.1f);
        mark(kTraitTreasureHunter, hunter);
    }
    {
        Rng r(77);
        int rare = 0, ancient = 0;
        for (int i = 0; i < 20000; ++i) {
            rare += rollVariant(r) == kKindVariants - 1;
            ancient += rollVariant(r, false, true) == kKindVariants - 1;
        }
        mark(kTraitAncientBlood, ancient > rare * 1.6f);
    }
    // ---- Flight and the race.
    {
        const FlightTuning p = flightTuningFor(5, 5);
        FlightTuning s = p, k = p, w = p;
        flightTraits(s, with(kTraitSwift));
        flightTraits(k, with(kTraitSkydancer));
        flightTraits(w, with(kTraitStrongWings));
        const challenge::RaceTuning rp = challenge::raceTuning(plain);
        mark(kTraitSwift, s.glideSpeed > p.glideSpeed && s.burstSpeed > p.burstSpeed &&
                              challenge::raceTuning(with(kTraitSwift)).top > rp.top);
        mark(kTraitSkydancer, k.turnRate > p.turnRate && k.sinkRate < p.sinkRate &&
                                  challenge::raceTuning(with(kTraitSkydancer)).turnRate > rp.turnRate);
        mark(kTraitStrongWings, w.flapCost < p.flapCost && w.burstCost < p.burstCost &&
                                    challenge::raceTuning(with(kTraitStrongWings)).meter > rp.meter);
    }
    {
        challenge::Trial t, u;
        t.begin(1, 9, hasTrait(with(kTraitSureFooted), kTraitSureFooted) ? 1 : 0);
        u.begin(1, 9, hasTrait(plain, kTraitSureFooted) ? 1 : 0);
        mark(kTraitSureFooted, t.hearts == u.hearts + 1);
    }
    // ---- Shows (the Harvest Fair, theme 1, favours Grove and Stone: not a Pouncer's).
    {
        Rgb pal[kPalCount];
        kindPalette(plain.kind, plain.variant, 0, pal);
        const float look = pageant::lookScore(plain, 1, pal), poise = pageant::poiseScore(plain);
        mark(kTraitShowoff, std::fabs(pageant::lookScore(with(kTraitShowoff), 1, pal) - look - 6.0f) < 0.01f);
        mark(kTraitGentleGiant, std::fabs(pageant::poiseScore(with(kTraitGentleGiant)) - poise - 6.0f) < 0.01f);
        mark(kTraitSongbird, std::fabs(pageant::perfectWindow(with(kTraitSongbird)) / pageant::perfectWindow(plain) - 1.2f) < 0.001f);
        mark(kTraitMossback, pageant::themeFavours(1, plain) == 0 && pageant::themeFavours(1, with(kTraitMossback)) == 1);
    }
    // ---- Battles.
    {
        auto expected = [&](const Dragon& attacker, const Dragon& defender, int element) {
            battle::Battle b;
            battle::begin(b, battle::makeBattler(attacker), battle::makeBattler(defender));
            for (int slot = 0; slot < kMoveSlots; ++slot) {
                const int m = b.side[0].moves[slot];
                if (battle::validMove(m) && battle::moveInfo(m).element == element && battle::moveInfo(m).kind != battle::MoveKind::Status)
                    return battle::expectedDamage(b, 0, slot);
            }
            return -1.0f;
        };
        const float ember = expected(plain, plain, battle::kEmber), body = expected(plain, plain, battle::kBody);
        mark(kTraitIronhide, body > 0 && std::fabs(expected(plain, with(kTraitIronhide), battle::kBody) / body - 0.9f) < 0.01f);
        mark(kTraitElemental, ember > 0 && std::fabs(expected(with(kTraitElemental), plain, battle::kEmber) / ember - 1.15f) < 0.01f);
        mark(kTraitCoolHeaded, std::fabs(expected(plain, with(kTraitCoolHeaded), battle::kEmber) / ember - 0.75f) < 0.01f);
        Dragon frosty = plain;
        frosty.kind = static_cast<u8>(findKind("flurrytail"));  // (Frost)
        const float frost = expected(frosty, plain, battle::kFrost);
        mark(kTraitWarmBlooded, frost > 0 && std::fabs(expected(frosty, with(kTraitWarmBlooded), battle::kFrost) / frost - 0.75f) < 0.01f);
        // Brave Heart: a Roar (lowers Might) lands about half as often.
        Dragon roarer = plain;
        trainer::gainXp(roarer, trainer::xpForLevel(5));
        int roar = -1;
        for (int m = 0; m < battle::moveCount(); ++m)
            if (std::strcmp(battle::moveInfo(m).name, "Roar") == 0) roar = m;
        CHECK(roar >= 0 && battle::equipMove(roarer, 0, roar));
        auto lowered = [&](const Dragon& target) {
            Rng r(5);
            int n = 0;
            for (int i = 0; i < 400; ++i) {
                battle::Battle b;
                battle::begin(b, battle::makeBattler(roarer), battle::makeBattler(target));
                battle::resolveTurn(b, 0, -1, r);
                for (int e = 0; e < b.logCount; ++e) n += b.log[e].kind == battle::Ev::StatDown;
            }
            return n;
        };
        const int plainDown = lowered(plain), braveDown = lowered(with(kTraitBraveHeart));
        mark(kTraitBraveHeart, plainDown > 300 && braveDown > plainDown * 0.35f && braveDown < plainDown * 0.65f);
        const battle::Stats night = battle::makeBattler(with(kTraitMoonlit), 23).stats, noon = battle::makeBattler(with(kTraitMoonlit), 12).stats;
        mark(kTraitMoonlit, night.might > noon.might && night.hp > noon.hp);
        const battle::Stats sunny = battle::makeBattler(with(kTraitSunkissed), 12).stats, dark = battle::makeBattler(with(kTraitSunkissed), 23).stats;
        mark(kTraitSunkissed, sunny.wing > dark.wing && sunny.breath > dark.breath);
    }
    std::printf("  %d of %d traits checked\n", checked, kTraitIds);
    CHECK(checked == kTraitIds);
}

void runTrainerTests() {
    RUN(trainer_levels);
    RUN(trainer_stats_and_training);
    RUN(trainer_energy_love_and_walks);
    RUN(trainer_record_and_titles);
    RUN(trainer_daily_claims_and_owning);
    RUN(trainer_save_round_trip_and_older_records);
    RUN(every_trait_does_its_thing);
}
