// 1.0's battles (core/battle, D90): the elements' wheel, the moves and what a dragon knows, its
// four, battle stats, a turn's resolution, the choices by league, experience; then balance by
// simulation (equal levels near even, five levels up about 80%, no kind pairing hopeless, no
// move always best), the league's challengers and champions (core/league) and Frostspire Hollow
// (core/hollow).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>

#include "check.hpp"
#include "core/battle.hpp"
#include "core/care.hpp"
#include "core/clock.hpp"
#include "core/hollow.hpp"
#include "core/league.hpp"
#include "core/place_layout.hpp"
#include "core/save.hpp"
#include "core/trainer.hpp"

using namespace ec;
using namespace ec::battle;

namespace {

Dragon grownOf(int kind, int level, int trained, u32 seed, int variant = 0) {
    Rng rng(seed * 2654435761u + 17);
    Dragon d;
    d.id = seed;
    d.stage = Stage::Adult;
    rollKind(d, kind, variant, rng);
    d.xp = trainer::xpForLevel(level);
    for (u8& t : d.trained) t = static_cast<u8>(trained);
    return d;
}

// One battle to its end: the winner (0 or 1), and its turns.
int fight(const Dragon& a, const Dragon& b, int skillA, int skillB, Rng& rng, int* turns = nullptr) {
    Battle bt;
    begin(bt, makeBattler(a), makeBattler(b));
    while (!bt.over) resolveTurn(bt, chooseMove(bt, 0, skillA, rng), chooseMove(bt, 1, skillB, rng), rng);
    if (turns) *turns = bt.turn;
    return bt.winner;
}

// Side 0's share of wins over many battles between random kinds at these levels.
float winRate(int levelA, int levelB, int trainedA, int trainedB, int n, u32 seed, float* avgTurns = nullptr) {
    Rng rng(seed);
    int wins = 0, turns = 0;
    for (int i = 0; i < n; ++i) {
        const int ka = static_cast<int>(rng.below(static_cast<u32>(kindCount())));
        const int kb = static_cast<int>(rng.below(static_cast<u32>(kindCount())));
        const Dragon a = grownOf(ka, levelA, trainedA, seed + i * 7 + 1), b = grownOf(kb, levelB, trainedB, seed + i * 7 + 2);
        int t = 0;
        wins += fight(a, b, 3, 3, rng, &t) == 0;
        turns += t;
    }
    if (avgTurns) *avgTurns = static_cast<float>(turns) / n;
    return static_cast<float>(wins) / n;
}

}  // namespace

TEST(battle_elements_wheel) {
    // Each of the six is strong against one and weak to one, around the wheel; Lumen and Shade
    // against each other; the rest even.
    const int six[6] = {kEmber, kFrost, kStone, kGale, kGrove, kTide};
    for (int i = 0; i < 6; ++i) {
        const int e = six[i];
        int strong = 0, weak = 0;
        for (int d = 0; d < kElements; ++d) {
            strong += matchup(e, d) > 1.5f;
            weak += matchup(e, d) < 0.75f;
        }
        CHECK(strong == 1 && weak == 1);
        CHECK(strongAgainst(e) == six[(i + 1) % 6] && weakTo(e) == six[(i + 5) % 6]);
        CHECK(matchup(e, strongAgainst(e)) == 2.0f && matchup(strongAgainst(e), e) == 0.5f);
    }
    CHECK(matchup(kEmber, kFrost) == 2.0f && matchup(kTide, kEmber) == 2.0f && matchup(kEmber, kTide) == 0.5f);
    CHECK(matchup(kGrove, kTide) == 2.0f && matchup(kGale, kGrove) == 2.0f && matchup(kStone, kGale) == 2.0f);
    CHECK(matchup(kLumen, kShade) == 2.0f && matchup(kShade, kLumen) == 2.0f && matchup(kLumen, kEmber) == 1.0f);
    CHECK(matchup(kBody, kFrost) == 1.0f && matchup(kEmber, kEmber) == 1.0f);
    // Against two elements: multiplied, kept within a half and double.
    const int frostcurl = findKind("frostcurl"), kindlemoss = findKind("kindlemoss");
    CHECK(frostcurl >= 0 && kindlemoss >= 0);
    CHECK(effectiveness(kEmber, frostcurl) == 2.0f);   // melts its Frost, Stone even
    CHECK(effectiveness(kTide, kindlemoss) == 1.0f);   // douses its Ember, its Grove drinks it
    CHECK(effectiveness(kGale, findKind("bloomstone")) == 1.0f);  // strips Grove, stopped by Stone
    for (int e = 0; e <= kElements; ++e) CHECK(elementLabel(e)[0] != 0);
}

TEST(battle_moves_table) {
    CHECK(moveCount() >= 40);
    std::set<std::string> names;
    for (int i = 0; i < moveCount(); ++i) {
        const MoveInfo& m = moveInfo(i);
        names.insert(m.name);
        CHECK(m.accuracy > 0 && m.accuracy <= 100 && m.level >= 1 && m.level <= kMaxLevel);
        CHECK((m.kind == MoveKind::Status) == (m.power == 0));
        CHECK(m.kind == MoveKind::Status ? m.effect != Effect::None : m.effect == Effect::None);
        CHECK(m.blurb && m.blurb[0]);
    }
    CHECK(static_cast<int>(names.size()) == moveCount());
    CHECK(!validMove(kNone) && std::strcmp(moveName(kNone), "---") == 0);
    // Every element: breath moves at three levels, stronger ones less sure (hard hitters trade
    // accuracy for power, but hit harder on average).
    for (int e = 0; e < kElements; ++e) {
        int breaths = 0, specials = 0, bodies = 0;
        float lastExpected = 0;
        for (int i = 0; i < moveCount(); ++i) {
            const MoveInfo& m = moveInfo(i);
            if (m.element != e) continue;
            if (m.kind == MoveKind::Breath) {
                ++breaths;
                const float expected = m.power * m.accuracy / 100.0f;
                CHECK(expected > lastExpected);
                lastExpected = expected;
            }
            specials += m.kind == MoveKind::Status;
            bodies += m.kind == MoveKind::Body;
        }
        CHECK(breaths == 3 && specials == 1 && bodies == 1);
    }
}

TEST(battle_what_it_knows) {
    const int pouncer = findKind("pouncer");  // Ember
    Dragon d = grownOf(pouncer, 1, 0, 1);
    CHECK(knows(d, 0) && !knows(d, 1));  // Tackle; Claw Swipe at 3
    int emberSpark = -1, seedSpit = -1, inferno = -1;
    for (int i = 0; i < moveCount(); ++i) {
        if (std::strcmp(moveInfo(i).name, "Ember Spark") == 0) emberSpark = i;
        if (std::strcmp(moveInfo(i).name, "Seed Spit") == 0) seedSpit = i;
        if (std::strcmp(moveInfo(i).name, "Inferno") == 0) inferno = i;
    }
    CHECK(knows(d, emberSpark) && !knows(d, seedSpit) && !knows(d, inferno));
    u8 known[64];
    CHECK(knownMoves(d, known, 64) == 2);
    Dragon egg = d;
    egg.stage = Stage::Egg;
    CHECK(knownMoves(egg, known, 64) == 0 && !knows(egg, 0));
    // Up to level 12: what it learned on the way, in order.
    u8 learned[16];
    const int n = movesLearned(d, 1, 12, learned, 16);
    CHECK(n >= 6);
    for (int i = 1; i < n; ++i) CHECK(moveInfo(learned[i]).level >= moveInfo(learned[i - 1]).level);
    d.xp = trainer::xpForLevel(30);
    CHECK(knows(d, inferno));
    const int all = knownMoves(d, known, 64);
    CHECK(all == 7 + 4 + 5);  // the body moves, anyone's stat moves, its element's five
    // A two-element kind learns both elements' moves.
    const Dragon b = grownOf(findKind("blazeplume"), 30, 0, 2);
    CHECK(knownMoves(b, known, 64) == 7 + 4 + 10);
}

TEST(battle_its_four) {
    Dragon d = grownOf(findKind("crestwing"), 1, 0, 3);
    u8 four[kMoveSlots];
    equippedMoves(d, four);
    CHECK(validMove(four[0]) && validMove(four[1]) && four[2] == kNone && four[3] == kNone);  // two known at level 1
    d.xp = trainer::xpForLevel(30);
    equippedMoves(d, four);
    std::set<int> seen;
    for (u8 m : four) {
        CHECK(validMove(m) && knows(d, m));
        seen.insert(m);
    }
    CHECK(seen.size() == 4);
    CHECK(moveInfo(four[0]).kind == MoveKind::Breath && moveInfo(four[0]).level == 26);  // its great breath first
    // The profile's swap: a known move into a slot; one already there trades places.
    u8 known[64];
    const int n = knownMoves(d, known, 64);
    const int tackle = 0;
    CHECK(equipMove(d, 3, tackle) && d.moves[3] == tackle);
    const u8 first = d.moves[0];
    CHECK(equipMove(d, 0, tackle) && d.moves[0] == tackle && d.moves[3] == first);
    CHECK(!equipMove(d, 4, tackle) && !equipMove(d, 0, kNone));
    int seedSpit = -1;
    for (int i = 0; i < moveCount(); ++i)
        if (std::strcmp(moveInfo(i).name, "Seed Spit") == 0) seedSpit = i;
    CHECK(!equipMove(d, 1, seedSpit));  // not a Grove kind
    // A save's odd ids (or moves it no longer knows) are dropped and the slot filled.
    d.moves[1] = 200;
    d.moves[2] = static_cast<u8>(seedSpit);
    equippedMoves(d, four);
    for (u8 m : four) CHECK(knows(d, m));
    CHECK(n > 4);
}

TEST(battle_stats_by_level_and_points) {
    float points[kDragonStats] = {5, 5, 5, 5, 5};
    const Stats a = statsFor(points, 1), b = statsFor(points, 10), c = statsFor(points, 50);
    CHECK(b.hp > a.hp && c.hp > b.hp && b.might > a.might && c.wing > b.wing);
    points[kStatStamina] = 10;
    CHECK(statsFor(points, 10).hp > b.hp);
    points[kStatMight] = 40;
    CHECK(statsFor(points, 10).might > b.might * 1.5f);
    std::printf("  level 1: hp %d might %d; level 10: hp %d; level 30: hp %d; level 50: hp %d\n", a.hp, a.might, b.hp,
                statsFor(points, 30).hp, c.hp);
    // A dragon's own: its kind's points and its level.
    Dragon d = grownOf(findKind("curlstone"), 20, 0, 4);
    const Stats s = battleStats(d);
    CHECK(s.might > s.wing);  // a Curlstone: strong, slow
    const Battler bt = makeBattler(d);
    CHECK(bt.level == 20 && bt.maxHp == s.hp && bt.hp == bt.maxHp);
}

TEST(battle_turns) {
    Rng rng(9);
    Dragon a = grownOf(findKind("pouncer"), 20, 0, 5), b = grownOf(findKind("flurrytail"), 20, 0, 6);
    Battle bt;
    begin(bt, makeBattler(a), makeBattler(b));
    // A hit logs its use, then the damage; the one hit loses that much.
    int slot = -1;
    for (int s = 0; s < kMoveSlots; ++s)
        if (validMove(bt.side[0].moves[s]) && moveInfo(bt.side[0].moves[s]).element == kEmber &&
            moveInfo(bt.side[0].moves[s]).kind == MoveKind::Breath)
            slot = s;
    CHECK(slot >= 0);
    int hits = 0, strong = 0;
    for (int i = 0; i < 30 && !bt.over; ++i) {
        const int before = bt.side[1].hp;
        resolveTurn(bt, slot, -1, rng);
        for (int k = 0; k < bt.logCount; ++k) {
            const Event& e = bt.log[k];
            if (e.kind == Ev::Hit && e.side == 1) {
                ++hits;
                strong += (e.flags & kStrong) != 0;
                CHECK(before - bt.side[1].hp == e.amount || bt.side[1].hp == 0);
            }
        }
    }
    CHECK(hits > 0 && strong == hits);  // Ember into Frost: always strong
    CHECK(bt.over && bt.winner == 0 && bt.side[1].hp == 0 && bt.log[bt.logCount - 1].kind == Ev::Faint);
    // Once over, nothing more happens.
    resolveTurn(bt, slot, 0, rng);
    CHECK(bt.logCount == 0);
    // Quick Swoop goes first even from the slower side.
    Dragon slow = grownOf(findKind("curlstone"), 20, 0, 7), fast = grownOf(findKind("crestwing"), 20, 0, 8);
    begin(bt, makeBattler(slow), makeBattler(fast));
    bt.side[0].moves[0] = 2;  // Quick Swoop
    bt.side[1].moves[0] = 0;  // Tackle
    CHECK(firstMover(bt, 0, 0, rng) == 0);
    bt.side[0].moves[0] = 0;
    CHECK(firstMover(bt, 0, 0, rng) == 1);  // Wing decides otherwise
    // Stat moves: stages up to their cap, then nothing; a heal twice a battle.
    int tailwind = -1, rest = -1;
    for (int i = 0; i < moveCount(); ++i) {
        if (std::strcmp(moveInfo(i).name, "Tailwind") == 0) tailwind = i;
        if (std::strcmp(moveInfo(i).name, "Rest") == 0) rest = i;
    }
    begin(bt, makeBattler(fast), makeBattler(slow));
    bt.side[0].moves[1] = static_cast<u8>(tailwind);
    bt.side[0].moves[2] = static_cast<u8>(rest);
    resolveTurn(bt, 1, -1, rng);
    CHECK(bt.side[0].stage[kStageWing] == 2 && bt.log[1].kind == Ev::StatUp && bt.log[1].amount == 2);
    resolveTurn(bt, 1, -1, rng);
    CHECK(bt.log[1].kind == Ev::NoEffect);
    bt.side[0].hp = bt.side[0].maxHp / 4;
    resolveTurn(bt, 2, -1, rng);
    CHECK(bt.log[1].kind == Ev::Heal && bt.side[0].hp > bt.side[0].maxHp / 4);
    CHECK(!usable(bt.side[0], 2));  // once a battle
    // A tiring move (the great breaths) can't be used two turns running.
    int inferno = -1;
    for (int i = 0; i < moveCount(); ++i)
        if (std::strcmp(moveInfo(i).name, "Inferno") == 0) inferno = i;
    CHECK(moveInfo(inferno).power >= kTiringPower);
    bt.side[0].moves[3] = static_cast<u8>(inferno);
    resolveTurn(bt, 3, -1, rng);
    CHECK(!usable(bt.side[0], 3) && usable(bt.side[0], 1));
    resolveTurn(bt, 1, -1, rng);
    CHECK(usable(bt.side[0], 3));
    // A stand-off ends: the one with more of its health wins.
    begin(bt, makeBattler(fast), makeBattler(slow));
    for (int i = 0; i < kMaxTurns; ++i) resolveTurn(bt, -1, -1, rng);
    CHECK(bt.over && bt.log[bt.logCount - 1].kind == Ev::TimeUp);
}

TEST(battle_balance_levels) {
    float turns = 0;
    // Equal levels: near even, whatever the level; four to seven turns or so.
    for (int level : {5, 15, 30}) {
        const float r = winRate(level, level, 0, 0, 1200, 100 + level, &turns);
        std::printf("  equal at %d: %.2f (%.1f turns)\n", level, r, turns);
        CHECK(r > 0.44f && r < 0.56f);
        CHECK(turns > 3.0f && turns < 9.0f);
    }
    // The same kind against itself: even (going first isn't everything).
    Rng rng(4);
    int wins = 0;
    for (int i = 0; i < 1000; ++i) {
        const int k = static_cast<int>(rng.below(static_cast<u32>(kindCount())));
        wins += fight(grownOf(k, 12, 0, 1000 + i), grownOf(k, 12, 0, 1000 + i), 3, 3, rng) == 0;
    }
    std::printf("  mirror: %.2f\n", wins / 1000.0f);
    CHECK(wins > 440 && wins < 560);
    // Five levels up: about four in five.
    for (int level : {8, 15, 25, 35}) {
        const float r = winRate(level + 5, level, 0, 0, 1200, 300 + level);
        std::printf("  +5 levels at %d: %.2f\n", level, r);
        CHECK(r > 0.70f && r < 0.90f);
    }
    // (Very young, five levels is more than double: more moves as well as stronger.)
    std::printf("  +5 levels at 3: %.2f\n", winRate(8, 3, 0, 0, 1200, 303));
    // Training shows: ten points in every stat is worth a few levels.
    const float trained = winRate(20, 20, 10, 0, 1200, 555);
    std::printf("  +10 trained each: %.2f\n", trained);
    CHECK(trained > 0.6f && trained < 0.9f);
}

TEST(battle_balance_kinds_and_moves) {
    // Kinds at equal levels: where the elements don't meet, no kind is hopeless against another;
    // where one's element beats the other's (and not the other way), the wheel tells: it wins most.
    Rng rng(77);
    float worst = 1, best = 0, edge = 0;
    int edges = 0;
    for (int a = 0; a < kindCount(); ++a)
        for (int b = 0; b < kindCount(); ++b) {
            if (a == b) continue;
            float aOnB = 1, bOnA = 1;  // the best each side's elements do against the other
            for (int i = 0; i < kindInfo(a).elementCount; ++i) aOnB = std::fmax(aOnB, effectiveness(kindInfo(a).elements[i], b));
            for (int i = 0; i < kindInfo(b).elementCount; ++i) bOnA = std::fmax(bOnA, effectiveness(kindInfo(b).elements[i], a));
            int wins = 0;
            for (int i = 0; i < 80; ++i) wins += fight(grownOf(a, 20, 0, a * 97 + i), grownOf(b, 20, 0, b * 89 + i), 3, 3, rng) == 0;
            const float r = wins / 80.0f;
            if (aOnB == 1.0f && bOnA == 1.0f) {
                worst = std::fmin(worst, r);
                best = std::fmax(best, r);
                if (r < 0.1f || r > 0.9f) std::printf("    %s against %s: %.2f\n", kindInfo(a).title, kindInfo(b).title, r);
            } else if (aOnB > 1.0f && bOnA == 1.0f) {
                edge += r;
                ++edges;
            }
        }
    std::printf("  kind pairings at 20, the elements apart: %.2f .. %.2f; with the wheel's edge: %.2f\n", worst, best,
                edges ? edge / edges : 0.0f);
    CHECK(worst > 0.05f && best < 0.95f);  // (a common against a crossbreed: five stat points behind)
    CHECK(edges > 0 && edge / edges > 0.65f);
    // No move always best: dragons with four moves picked at random from what they know; no
    // move is taken most turns it's among the four.
    int picks[256] = {}, offered[256] = {};
    for (int i = 0; i < 1500; ++i) {
        const int ka = static_cast<int>(rng.below(static_cast<u32>(kindCount())));
        const int kb = static_cast<int>(rng.below(static_cast<u32>(kindCount())));
        const int level = 10 + static_cast<int>(rng.below(30));
        Battle bt;
        begin(bt, makeBattler(grownOf(ka, level, 0, 5000 + i)), makeBattler(grownOf(kb, level, 0, 6000 + i)));
        u8 known[64];
        const int n = knownMoves(grownOf(ka, level, 0, 5000 + i), known, 64);
        for (int s = 0; s < kMoveSlots; ++s) {  // four different ones at random
            for (;;) {
                const u8 m = known[rng.below(static_cast<u32>(n))];
                bool taken = false;
                for (int t = 0; t < s; ++t) taken |= bt.side[0].moves[t] == m;
                if (!taken) {
                    bt.side[0].moves[s] = m;
                    break;
                }
            }
        }
        while (!bt.over) {
            const int s0 = chooseMove(bt, 0, 3, rng), s1 = chooseMove(bt, 1, 3, rng);
            for (int s = 0; s < kMoveSlots; ++s) ++offered[bt.side[0].moves[s]];
            if (s0 >= 0) ++picks[bt.side[0].moves[s0]];
            resolveTurn(bt, s0, s1, rng);
        }
    }
    float most = 0;
    int mostMove = 0, chosen = 0;
    for (int m = 0; m < moveCount(); ++m) {
        if (offered[m] < 50) continue;
        const float r = static_cast<float>(picks[m]) / offered[m];
        chosen += picks[m] > offered[m] / 20;
        if (r > most) {
            most = r;
            mostMove = m;
        }
    }
    std::printf("  choices: %d moves chosen often enough; the most taken when there, %s %.0f%%\n", chosen,
                moveName(mostMove), 100.0f * most);
    CHECK(most < 0.85f && chosen > 30);  // (the strongest tire; stat moves and finishing blows take the rest)
    int smartWins = 0;
    for (int i = 0; i < 800; ++i) {
        const int ka = static_cast<int>(rng.below(static_cast<u32>(kindCount())));
        const int kb = static_cast<int>(rng.below(static_cast<u32>(kindCount())));
        Battle bt;
        begin(bt, makeBattler(grownOf(ka, 25, 0, 7000 + i)), makeBattler(grownOf(kb, 25, 0, 7000 + i + 1)));
        while (!bt.over) {
            int hardest = -1;
            for (int s = 0; s < kMoveSlots; ++s)
                if (usable(bt.side[1], s) && moveInfo(bt.side[1].moves[s]).power > 0 &&
                    (hardest < 0 || moveInfo(bt.side[1].moves[s]).power > moveInfo(bt.side[1].moves[hardest]).power))
                    hardest = s;
            resolveTurn(bt, chooseMove(bt, 0, 3, rng), hardest, rng);
        }
        smartWins += bt.winner == 0;
    }
    std::printf("  choosing well against the hardest move every turn: %.2f\n", smartWins / 800.0f);
    CHECK(smartWins > 800 * 0.52f);
    // The leagues' choosing: Starfire's beats Ember's.
    int starfire = 0;
    for (int i = 0; i < 1000; ++i) {
        const int k = static_cast<int>(rng.below(static_cast<u32>(kindCount())));
        const int k2 = static_cast<int>(rng.below(static_cast<u32>(kindCount())));
        starfire += fight(grownOf(k, 20, 0, 9000 + i), grownOf(k2, 20, 0, 9500 + i), 3, 0, rng) == 0;
    }
    std::printf("  Starfire's choosing against Ember's: %.2f\n", starfire / 1000.0f);
    CHECK(starfire > 580);
}

TEST(battle_experience) {
    CHECK(battleXp(5, 5, Outcome::Won) > battleXp(5, 5, Outcome::Lost));
    CHECK(battleXp(5, 5, Outcome::GaveUp) == 0);
    CHECK(battleXp(5, 10, Outcome::Won) > battleXp(10, 10, Outcome::Won) * 0.9f);  // stronger foes teach more
    CHECK(battleXp(30, 5, Outcome::Won) < battleXp(5, 5, Outcome::Won));           // much weaker, little
    CHECK(battleXp(10, 10, Outcome::Won, 1.5f) > battleXp(10, 10, Outcome::Won));
    // Level 5 after a few early battles.
    Dragon d = grownOf(findKind("pouncer"), 1, 0, 11);
    u32 total = 0;
    int battles = 0;
    while (trainer::levelOf(d) < 5 && battles < 20) {
        const int foe = 3 + battles;
        const Growth g = grow(d, battleXp(trainer::levelOf(d), foe, Outcome::Won));
        total += g.xp;
        ++battles;
    }
    std::printf("  level 5 after %d wins (%u xp)\n", battles, total);
    CHECK(battles >= 3 && battles <= 6);
    // A level-up's new moves.
    Dragon e = grownOf(findKind("ribbontail"), 11, 0, 12);
    const Growth g = grow(e, trainer::xpForLevel(12) - e.xp);
    CHECK(g.levelBefore == 11 && g.levelAfter == 12 && g.learnedCount == 1 &&
          std::strcmp(moveInfo(g.learned[0]).name, "Water Jet") == 0);
}

namespace {
SaveData& freshSave() {
    static SaveData s;
    s = SaveData{};
    Rng rng(3);
    s.dragons[0] = grownOf(findKind("pouncer"), 10, 0, 1);
    std::snprintf(s.dragons[0].name, sizeof(s.dragons[0].name), "Kindle");
    s.dragonCount = 1;
    return s;
}
}  // namespace

TEST(league_challengers) {
    static constexpr int kLow[kLeagues] = {3, 10, 18, 28}, kHigh[kLeagues] = {8, 16, 26, 38};
    std::set<std::string> names;
    std::set<int> outfits;
    for (int league = 0; league < kLeagues; ++league) {
        std::set<int> places;
        for (int slot = 0; slot < league::kSlots; ++slot) {
            const int id = league::idOf(league, slot);
            CHECK(league::leagueOf(id) == league && league::slotOf(id) == slot);
            const league::Challenger& c = league::challenger(id);
            names.insert(c.name);
            CHECK(c.hello[0] && c.hello[1] && c.again && c.beaten && c.victory && c.dragonName && c.title);
            CHECK(findKind(c.kind) >= 0 && c.variant < kKindVariants && c.skill <= 3 && c.look.hair < 6 && c.look.body < 2);
            outfits.insert(c.look.outfit.r * 65536 + c.look.outfit.g * 256 + c.look.outfit.b);
            const Dragon d = league::dragonOf(id);
            CHECK(d.stage == Stage::Adult && trainer::levelOf(d) == c.level && std::strcmp(d.name, c.dragonName) == 0);
            CHECK(d.id >= 0xB0000000u);  // apart from the save's
            if (league::isChampion(id)) {
                CHECK(c.place == kPlaceCaldera && c.level > kHigh[league]);  // a little stronger
            } else {
                CHECK(c.level >= kLow[league] && c.level <= kHigh[league]);
                CHECK(places.insert(c.place).second);  // four places a league
                const Vec2 at = league::spotOf(id);
                CHECK(std::hypot(at.x, at.y) < placeLayout(c.place).flat);  // on its place's flat ground
                for (const Solid& w : placeLayout(c.place).solids)       // clear of its walls
                    CHECK(std::hypot(at.x - w.at.x, at.y - w.at.y) > w.radius + 1.5f);
            }
            Rgb pal[kPalCount];
            league::palette(c.look, pal);
            CHECK(pal[kPalBase].r == c.look.skin.r && pal[kPalAccent].g == c.look.outfit.g && pal[kPalHorn].b == c.look.hairColour.b);
        }
    }
    CHECK(static_cast<int>(names.size()) == league::kChallengers);
    CHECK(outfits.size() >= 18);  // every one dressed their own way
    CHECK(league::boardCount() == 2 && league::board(0).place == kPlaceArena && league::board(1).place == kPlaceCaldera);
    const Vec2 ring = league::ringCentre(), a = league::ringSide(0), b = league::ringSide(1);
    CHECK(std::hypot(a.x - b.x, a.y - b.y) > 9.0f && std::hypot(ring.x, ring.y) < 10.0f);
    CHECK(std::hypot(league::spotOf(league::idOf(2, league::kChampion)).x - b.x, league::spotOf(league::idOf(2, league::kChampion)).y - b.y) < 0.01f);
}

TEST(league_progress_and_rewards) {
    SaveData& s = freshSave();
    const s32 day = 20000;
    CHECK(league::currentLeague(s) == 0 && league::nextChallenger(s) == league::idOf(0, 0));
    CHECK(league::standing(s, league::idOf(0, 2)) && !league::standing(s, league::idOf(1, 0)));
    CHECK(!league::championOpen(s, 0));
    // A loss: a little experience, no Gleam, nothing beaten.
    const u32 gleam = s.gleam;
    league::Reward r = league::record(s, 0, league::idOf(0, 0), Outcome::Lost, day);
    CHECK(r.growth.xp > 0 && r.gleam == 0 && !league::beaten(s, league::idOf(0, 0)) && s.gleam == gleam);
    CHECK(s.progress.counts[kCountBattles] == 1);
    // Giving up: nothing at all.
    r = league::record(s, 0, league::idOf(0, 0), Outcome::GaveUp, day);
    CHECK(r.growth.xp == 0 && s.progress.counts[kCountBattles] == 1);
    // A first win pays well; a rematch the same day nothing; the next day a little.
    r = league::record(s, 0, league::idOf(0, 0), Outcome::Won, day);
    CHECK(r.firstWin && r.gleam > 0 && league::beaten(s, league::idOf(0, 0)) && s.dragons[0].battleWins == 1);
    CHECK(league::nextChallenger(s) == league::idOf(0, 1));
    r = league::record(s, 0, league::idOf(0, 0), Outcome::Won, day);
    CHECK(!r.firstWin && r.gleam == 0 && r.paidBefore);
    r = league::record(s, 0, league::idOf(0, 0), Outcome::Won, day + 1);
    CHECK(!r.firstWin && r.gleam > 0 && !r.paidBefore);
    // The Journal's tracked challenger comes first.
    trainer::track(s, Tracked::BattleBoard, league::idOf(0, 3));
    CHECK(league::nextChallenger(s) == league::idOf(0, 3));
    // The four beaten: the champion opens.
    for (int slot = 1; slot < league::kPerLeague; ++slot) {
        r = league::record(s, 0, league::idOf(0, slot), Outcome::Won, day);
        CHECK(r.championOpened == (slot == league::kPerLeague - 1));
    }
    CHECK(league::championOpen(s, 0) && league::nextChallenger(s) == league::idOf(0, league::kChampion));
    // The final: the title, the league, a prize, and the next league about the valley.
    const u16 pouchBefore = s.pouch[static_cast<int>(Food::EmberCandy)];
    r = league::record(s, 0, league::idOf(0, league::kChampion), Outcome::Won, day);
    CHECK(r.leagueWon && r.title && r.firstWin && r.gleam >= 150);
    CHECK(s.progress.battleLeague == 1 && s.dragons[0].battleTitle == 1);
    CHECK(std::strcmp(trainer::battleTitleName(s.dragons[0].battleTitle), "Ember Victor") == 0);
    CHECK(s.pouch[static_cast<int>(Food::EmberCandy)] == pouchBefore + r.prizeCount && r.prizeTrinket != 0xFF);
    CHECK(league::currentLeague(s) == 1 && league::standing(s, league::idOf(1, 0)) && !league::standing(s, league::idOf(0, 0)));
    CHECK(league::beaten(s, league::idOf(0, league::kChampion)) && !league::beaten(s, league::idOf(1, 0)));
    // Another dragon winning the same final later earns its own title (not the league again).
    s.dragons[1] = grownOf(findKind("puffback"), 12, 0, 2);
    s.dragonCount = 2;
    r = league::record(s, 1, league::idOf(0, league::kChampion), Outcome::Won, day + 2);
    CHECK(!r.leagueWon && r.title && s.dragons[1].battleTitle == 1 && s.progress.battleLeague == 1);
    // All four won: the Starfire league stays for rematches.
    s.progress.battleLeague = kLeagues;
    CHECK(league::currentLeague(s) == kLeagues - 1);
}

TEST(league_balance) {
    // A keeper's dragon at the challenger's level (a few trained points a league), choosing well,
    // beats most challengers more often than not; a champion is a step up.
    Rng rng(123);
    // The very first: a newly hatched dragon (level 1 or 2) has a fair chance against Tamsin.
    for (int level : {1, 2}) {
        int wins = 0;
        for (int i = 0; i < 400; ++i) {
            const int k = static_cast<int>(rng.below(static_cast<u32>(kindCount())));
            Dragon mine = grownOf(k, level, 0, 30000 + i);
            mine.stage = Stage::Hatchling;
            wins += fight(mine, league::dragonOf(league::idOf(0, 0)), 3, league::challenger(league::idOf(0, 0)).skill, rng) == 0;
        }
        std::printf("  a level %d hatchling against Tamsin: %.2f\n", level, wins / 400.0f);
        CHECK(wins > 400 * (level == 1 ? 0.2f : 0.3f));
    }
    for (int league = 0; league < kLeagues; ++league) {
        for (int slot = 0; slot < league::kSlots; ++slot) {
            const int id = league::idOf(league, slot);
            const league::Challenger& c = league::challenger(id);
            const Dragon foe = league::dragonOf(id);
            int wins = 0, winsLower = 0;
            constexpr int kRuns = 300;
            for (int i = 0; i < kRuns; ++i) {
                const int k = static_cast<int>(rng.below(static_cast<u32>(kindCount())));
                const Dragon mine = grownOf(k, c.level, league * 2, 40000 + id * 1000 + i);
                const Dragon lower = grownOf(k, c.level > 3 ? c.level - 3 : 1, league * 2, 40000 + id * 1000 + i);
                wins += fight(mine, foe, 3, c.skill, rng) == 0;
                winsLower += fight(lower, foe, 3, c.skill, rng) == 0;
            }
            const float r = static_cast<float>(wins) / kRuns, lr = static_cast<float>(winsLower) / kRuns;
            std::printf("  %-13s %-10s L%-2d: at its level %.2f, three below %.2f\n", c.name, c.dragonName, c.level, r, lr);
            if (league::isChampion(id)) CHECK(r > 0.3f && r < 0.8f);
            else CHECK(r > 0.35f && r < 0.95f);  // (the later leagues choose cleverly)
            CHECK(lr < r);
        }
    }
}

TEST(hollow_floors) {
    for (int f = 2; f <= hollow::kFloors; ++f) CHECK(hollow::wildLevel(f) >= hollow::wildLevel(f - 1) || hollow::guardian(f - 1));
    CHECK(hollow::wildLevel(1) == 3 && hollow::wildLevel(10) >= 14 && hollow::wildLevel(30) >= 38);
    CHECK(hollow::guardian(5) && hollow::guardian(30) && !hollow::guardian(7) && !hollow::guardian(0));
    CHECK(hollow::skillAt(1) == 0 && hollow::skillAt(30) == 3);
    // The same wild one all day; others on other days.
    const Dragon a = hollow::wildOf(7, 20000), b = hollow::wildOf(7, 20000);
    CHECK(a.kind == b.kind && a.variant == b.variant && a.xp == b.xp && a.stage == Stage::Adult);
    int differ = 0;
    for (int day = 0; day < 20; ++day) differ += hollow::wildOf(7, 20000 + day).kind != a.kind;
    CHECK(differ > 5);
    // Deeper: rarer kinds and the rare colouring likelier.
    int rareTop = 0, rareDeep = 0, commonTop = 0, commonDeep = 0;
    for (int day = 0; day < 600; ++day) {
        const Dragon t = hollow::wildOf(2, day), d = hollow::wildOf(28, day);
        rareTop += t.variant == kindInfo(t.kind).rareVariant;
        rareDeep += d.variant == kindInfo(d.kind).rareVariant;
        commonTop += kindInfo(t.kind).rarity == Rarity::Common;
        commonDeep += kindInfo(d.kind).rarity == Rarity::Common;
    }
    std::printf("  rare colourings: floor 2 %d, floor 28 %d of 600; commons %d, %d\n", rareTop, rareDeep, commonTop, commonDeep);
    CHECK(rareDeep > rareTop * 3 && commonTop > commonDeep * 2);
    // Checkpoints: the top, and past what it has cleared.
    Dragon d = grownOf(findKind("flurrytail"), 10, 0, 5);
    int cps[hollow::kCheckpoints];
    CHECK(hollow::checkpoints(d, cps) == 1 && cps[0] == 1);
    d.frostDeepest = 5;
    CHECK(hollow::checkpoints(d, cps) == 2 && cps[1] == 6);
    d.frostDeepest = 29;
    CHECK(hollow::checkpoints(d, cps) == 6 && cps[5] == 26);
    // The cold deeper down.
    const Rgb c{200, 220, 240}, deep = hollow::chill(c, 30), top = hollow::chill(c, 1);
    CHECK(top.r == c.r && deep.r < c.r && deep.g < c.g && hollow::depth(30) == 1.0f);
}

TEST(hollow_rewards) {
    SaveData& s = freshSave();
    Rng rng(8);
    const s32 day = 21000;
    const Dragon w1 = hollow::wildOf(1, day);
    const u32 gleam = s.gleam;
    hollow::Reward r = hollow::record(s, 0, 1, w1, Outcome::Won, day, rng);
    CHECK(r.growth.xp > 0 && r.gleam > 0 && r.newDeepest && s.progress.hollowDeepest == 1 && s.dragons[0].frostDeepest == 1);
    CHECK(s.gleam == gleam + r.gleam && s.dragons[0].wildWins == 1 && s.progress.counts[kCountWild] == 1);
    // More than a challenger of the same level would give.
    CHECK(r.growth.xp > battleXp(10, trainer::levelOf(w1), Outcome::Won));
    // A guardian: its bonus and a stat point once a day, and the first time a prize.
    const Dragon g = hollow::wildOf(5, day);
    const int trainedBefore = s.dragons[0].trained[0] + s.dragons[0].trained[1] + s.dragons[0].trained[2] +
                              s.dragons[0].trained[3] + s.dragons[0].trained[4];
    r = hollow::record(s, 0, 5, g, Outcome::Won, day, rng);
    CHECK(r.trained >= 0 && r.prizeFood != 0xFF && r.gleam >= 10 && s.progress.hollowDeepest == 5);
    const int trainedAfter = s.dragons[0].trained[0] + s.dragons[0].trained[1] + s.dragons[0].trained[2] +
                             s.dragons[0].trained[3] + s.dragons[0].trained[4];
    CHECK(trainedAfter == trainedBefore + 1);
    const hollow::Reward again = hollow::record(s, 0, 5, g, Outcome::Won, day, rng);
    CHECK(again.trained < 0 && again.prizeFood == 0xFF && again.gleam < r.gleam && !again.newDeepest);
    // A loss: a little experience, nothing else.
    const hollow::Reward lost = hollow::record(s, 0, 9, hollow::wildOf(9, day), Outcome::Lost, day, rng);
    CHECK(lost.gleam == 0 && lost.growth.xp > 0 && s.dragons[0].frostDeepest == 5);
}

TEST(hollow_balance) {
    // A dragon a level above the floor's wild one (a few trained points) mostly wins; the guardians
    // are a step up; far below, it doesn't.
    Rng rng(321);
    for (int floor : {1, 4, 5, 8, 12, 15, 18, 23, 25, 27, 30}) {
        int wins = 0, weak = 0;
        constexpr int kRuns = 300;
        for (int i = 0; i < kRuns; ++i) {
            const Dragon wild = hollow::wildOf(floor, i);
            const int k = static_cast<int>(rng.below(static_cast<u32>(kindCount())));
            const int level = hollow::wildLevel(floor) - (hollow::guardian(floor) ? 2 : 0) + 1;
            wins += fight(grownOf(k, level, floor / 4, 60000 + i), wild, 3, hollow::skillAt(floor), rng) == 0;
            weak += fight(grownOf(k, level > 6 ? level - 6 : 1, floor / 4, 60000 + i), wild, 3, hollow::skillAt(floor), rng) == 0;
        }
        std::printf("  floor %2d (L%d%s): %.2f; six levels lower %.2f\n", floor, hollow::wildLevel(floor),
                    hollow::guardian(floor) ? ", guardian" : "", wins / 300.0f, weak / 300.0f);
        if (hollow::guardian(floor)) CHECK(wins > 300 * 0.2f && wins < 300 * 0.7f);
        else CHECK(wins > 300 * 0.45f && wins < 300 * 0.95f);  // (deep down, rarer kinds)
        CHECK(weak < wins);
    }
}

TEST(battle_progress_through_the_leagues) {
    // A keeper's first dragon from level 1: eight battles a day (a full Energy bar), each against
    // the next challenger once it has caught up with them (after two losses running, a few floors
    // first), else a floor of the Hollow near its level. How long the leagues take (pacing: days).
    SaveData& s = freshSave();
    s.dragons[0] = grownOf(findKind("pouncer"), 1, 0, 77);
    Rng rng(99);
    int battles = 0, day = 0, wonAt[kLeagues] = {}, wildBattles = 0, trained = 0, losses = 0, train = 0;
    for (; day < 80 && s.progress.battleLeague < kLeagues; ++day) {
        for (int b = 0; b < 8; ++b, ++battles) {
            Dragon& d = s.dragons[0];
            const int level = trainer::levelOf(d);
            const int next = league::nextChallenger(s);
            const league::Challenger& c = league::challenger(next);
            const bool open = !league::isChampion(next) || league::championOpen(s, league::leagueOf(next));
            if (open && c.level <= level && train <= 0) {
                const int w = fight(d, league::dragonOf(next), 3, c.skill, rng);
                const int before = s.progress.battleLeague;
                league::record(s, 0, next, w == 0 ? Outcome::Won : Outcome::Lost, 20000 + day);
                if (s.progress.battleLeague > before) wonAt[before] = day + 1;
                losses = w == 0 ? 0 : losses + 1;
                if (losses >= 2) train = 4, losses = 0;  // (two losses running: off to train a while)
            } else {
                --train;
                int floor = static_cast<int>((level - 2) / 1.2f);
                floor = floor < 1 ? 1 : (floor > hollow::kFloors ? hollow::kFloors : floor);
                if (hollow::guardian(floor)) --floor;
                const Dragon wild = hollow::wildOf(floor, 20000 + day);
                const int w = fight(d, wild, 3, hollow::skillAt(floor), rng);
                trained += hollow::record(s, 0, floor, wild, w == 0 ? Outcome::Won : Outcome::Lost, 20000 + day, rng).trained >= 0;
                ++wildBattles;
            }
        }
    }
    std::printf("  leagues won on days %d, %d, %d, %d (%d battles, %d in the Hollow); level %d, %d stat points trained\n", wonAt[0],
                wonAt[1], wonAt[2], wonAt[3], battles, wildBattles, trainer::levelOf(s.dragons[0]), trained);
    CHECK(s.progress.battleLeague == kLeagues);
    CHECK(wonAt[0] >= 1 && wonAt[0] <= 6 && wonAt[3] > 8 && wonAt[3] < 45);
}

void runBattleTests() {
    RUN(battle_elements_wheel);
    RUN(battle_moves_table);
    RUN(battle_what_it_knows);
    RUN(battle_its_four);
    RUN(battle_stats_by_level_and_points);
    RUN(battle_turns);
    RUN(battle_balance_levels);
    RUN(battle_balance_kinds_and_moves);
    RUN(battle_experience);
    RUN(league_challengers);
    RUN(league_progress_and_rewards);
    RUN(league_balance);
    RUN(hollow_floors);
    RUN(hollow_rewards);
    RUN(hollow_balance);
    RUN(battle_progress_through_the_leagues);
}
