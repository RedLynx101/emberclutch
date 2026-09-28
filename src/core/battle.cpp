#include "core/battle.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "core/trainer.hpp"

namespace ec::battle {
namespace {

// ---- The balance numbers (tests/test_battle.cpp simulates them: equal levels near 50/50, five
// levels up about 80%, battles of four to seven turns).
constexpr float kLevelGrowth = 1.015f;    // every stat grows this much a level
constexpr float kHealthPerLevel = 0.025f; // and health this much more (of level 1's): the
                                          // stronger moves learned later don't end battles sooner
constexpr float kTrainedWeight = 0.5f;   // a trained point counts half a kind's point in battle
constexpr float kPowerSoftening = 14.0f;  // (ratingPower)
constexpr float kQuickSoftening = 2.0f;   // (ratingQuick)
constexpr float kWitHit = 0.2f;           // a hit's chance, per unit of the Wit ratio
constexpr float kWingDodge = 0.12f;       // a dodge, per unit of the Wing ratio
constexpr float kDamage = 0.85f;       // a move's power into damage
constexpr float kStagePerStep = 0.25f;
constexpr float kCritDamage = 1.5f;

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ---- The wheel: each strong against the next; Lumen and Shade against each other.
constexpr u8 kWheel[6] = {kEmber, kFrost, kStone, kGale, kGrove, kTide};

int wheelIndex(int e) {
    for (int i = 0; i < 6; ++i)
        if (kWheel[i] == e) return i;
    return -1;
}

// ---- The moves. Their order is saved (Dragon::moves): add new ones at the end only.
using M = MoveKind;
using E = Effect;
constexpr MoveInfo kMoves[] = {
    // Body moves: anyone, from Might.
    {"Tackle", kBody, M::Body, 45, 100, 1, E::None, 0, 0, "A running bump."},
    {"Claw Swipe", kBody, M::Body, 50, 95, 3, E::None, 0, 0, "A quick swipe of the claws."},
    {"Quick Swoop", kBody, M::Body, 40, 100, 5, E::None, 0, 1, "Always strikes first."},
    {"Tail Swipe", kBody, M::Body, 60, 90, 7, E::None, 0, 0, "A sweep of the tail."},
    {"Wing Buffet", kBody, M::Body, 60, 95, 11, E::None, 0, 0, "A clap of both wings."},
    {"Headbutt", kBody, M::Body, 75, 85, 16, E::None, 0, 0, "Head down and charge!"},
    {"Crushing Pounce", kBody, M::Body, 90, 85, 30, E::None, 0, 0, "A great leap, all its weight. Tiring."},
    // Anyone's stat moves.
    {"Roar", kBody, M::Status, 0, 90, 4, E::LowerMight, 1, 0, "Lowers the foe's Might."},
    {"Preen", kBody, M::Status, 0, 100, 8, E::RaiseWit, 1, 0, "Raises its Wit."},
    {"Rest", kBody, M::Status, 0, 100, 14, E::Heal, 35, 0, "Heals a good part of its health, once."},
    {"Flex", kBody, M::Status, 0, 100, 20, E::RaiseMight, 1, 0, "Raises its Might."},
    // Each element: a first breath, its own stat move, a stronger breath, a body move of its
    // element (from Might) and its great breath.
    {"Ember Spark", kEmber, M::Breath, 45, 100, 1, E::None, 0, 0, "A puff of sparks."},
    {"Kindle", kEmber, M::Status, 0, 100, 9, E::RaiseBreath, 1, 0, "Stokes its fire: Breath up."},
    {"Flame Breath", kEmber, M::Breath, 65, 95, 12, E::None, 0, 0, "A stream of flame."},
    {"Blaze Claw", kEmber, M::Body, 70, 90, 18, E::None, 0, 0, "Claws glowing hot."},
    {"Inferno", kEmber, M::Breath, 90, 85, 26, E::None, 0, 0, "A roaring blaze. Tiring."},
    {"Seed Spit", kGrove, M::Breath, 45, 100, 1, E::None, 0, 0, "A spray of seeds."},
    {"Mossy Mend", kGrove, M::Status, 0, 100, 9, E::Heal, 30, 0, "Soft moss heals it, once."},
    {"Spore Burst", kGrove, M::Breath, 65, 95, 12, E::None, 0, 0, "A cloud of stinging spores."},
    {"Vine Lash", kGrove, M::Body, 70, 90, 18, E::None, 0, 0, "A whip of green vines."},
    {"Bramble Storm", kGrove, M::Breath, 90, 85, 26, E::None, 0, 0, "A whirl of thorns. Tiring."},
    {"Grit Spray", kStone, M::Breath, 45, 100, 1, E::None, 0, 0, "A gust of sand."},
    {"Stoneskin", kStone, M::Status, 0, 100, 9, E::RaiseGuard, 1, 0, "Hardens: it takes less damage."},
    {"Sand Blast", kStone, M::Breath, 65, 95, 12, E::None, 0, 0, "A blast of stinging sand."},
    {"Stone Slam", kStone, M::Body, 70, 90, 18, E::None, 0, 0, "A body like a boulder."},
    {"Rockfall", kStone, M::Breath, 90, 85, 26, E::None, 0, 0, "Pebbles and stones rain down. Tiring."},
    {"Gust", kGale, M::Breath, 45, 100, 1, E::None, 0, 0, "A puff of wind."},
    {"Tailwind", kGale, M::Status, 0, 100, 9, E::RaiseWing, 2, 0, "The wind behind it: Wing up."},
    {"Cyclone", kGale, M::Breath, 65, 95, 12, E::None, 0, 0, "A spinning gale."},
    {"Wing Cutter", kGale, M::Body, 70, 90, 18, E::None, 0, 0, "Wings sharp as the wind."},
    {"Tempest", kGale, M::Breath, 90, 85, 26, E::None, 0, 0, "A howling storm. Tiring."},
    {"Bubble Spray", kTide, M::Breath, 45, 100, 1, E::None, 0, 0, "A spray of bubbles."},
    {"Drench", kTide, M::Status, 0, 90, 9, E::LowerBreath, 1, 0, "Soaks the foe: its Breath down."},
    {"Water Jet", kTide, M::Breath, 65, 95, 12, E::None, 0, 0, "A jet of water."},
    {"Wave Crash", kTide, M::Body, 70, 90, 18, E::None, 0, 0, "Crashes in like a wave."},
    {"Tidal Surge", kTide, M::Breath, 90, 85, 26, E::None, 0, 0, "A wall of water. Tiring."},
    {"Frost Puff", kFrost, M::Breath, 45, 100, 1, E::None, 0, 0, "A chilly puff."},
    {"Chill", kFrost, M::Status, 0, 90, 9, E::LowerWing, 1, 0, "Numbs the foe: its Wing down."},
    {"Icy Breath", kFrost, M::Breath, 65, 95, 12, E::None, 0, 0, "A freezing stream."},
    {"Frost Fang", kFrost, M::Body, 70, 90, 18, E::None, 0, 0, "A bite cold as ice."},
    {"Blizzard", kFrost, M::Breath, 90, 85, 26, E::None, 0, 0, "A whirl of snow and ice. Tiring."},
    {"Glimmer", kLumen, M::Breath, 45, 100, 1, E::None, 0, 0, "A twinkle of light."},
    {"Dazzle", kLumen, M::Status, 0, 90, 9, E::LowerWit, 1, 0, "A flash: the foe's Wit down."},
    {"Radiant Beam", kLumen, M::Breath, 65, 95, 12, E::None, 0, 0, "A beam of warm light."},
    {"Sunflare Strike", kLumen, M::Body, 70, 90, 18, E::None, 0, 0, "A charge wrapped in light."},
    {"Starburst", kLumen, M::Breath, 90, 85, 26, E::None, 0, 0, "A burst of starlight. Tiring."},
    {"Dusk Puff", kShade, M::Breath, 45, 100, 1, E::None, 0, 0, "A puff of twilight."},
    {"Shadow Veil", kShade, M::Status, 0, 100, 9, E::RaiseWit, 2, 0, "Melts into shadow: Wit up."},
    {"Umbral Breath", kShade, M::Breath, 65, 95, 12, E::None, 0, 0, "A stream of deep dusk."},
    {"Shadow Pounce", kShade, M::Body, 70, 90, 18, E::None, 0, 0, "Leaps out of the dark."},
    {"Eclipse", kShade, M::Breath, 90, 85, 26, E::None, 0, 0, "The light swallowed whole. Tiring."},
};
constexpr int kMoveCount = static_cast<int>(sizeof(kMoves) / sizeof(kMoves[0]));
static_assert(kMoveCount < kNone, "move ids fit a byte, kNone apart");

bool damaging(const MoveInfo& m) { return m.kind != M::Status; }
bool selfTargeted(Effect e) {
    return e == E::RaiseMight || e == E::RaiseBreath || e == E::RaiseWit || e == E::RaiseWing || e == E::RaiseGuard ||
           e == E::Heal;
}
int stageOf(Effect e) {
    switch (e) {
        case E::RaiseMight:
        case E::LowerMight: return kStageMight;
        case E::RaiseBreath:
        case E::LowerBreath: return kStageBreath;
        case E::RaiseWit:
        case E::LowerWit: return kStageWit;
        case E::RaiseWing:
        case E::LowerWing: return kStageWing;
        case E::RaiseGuard: return kStageGuard;
        default: return -1;
    }
}

bool kindHas(int kind, int element) {
    const KindInfo& k = kindInfo(kind);
    for (int i = 0; i < k.elementCount && i < 2; ++i)
        if (k.elements[i] == element) return true;
    return false;
}

// A move a kind learns at all (whatever the level).
bool inLearnset(int kind, const MoveInfo& m) { return m.element == kBody || kindHas(kind, m.element); }

float stageMult(int s) { return s >= 0 ? 1.0f + kStagePerStep * s : 1.0f / (1.0f - kStagePerStep * s); }

// A stat's rating from its points, a square root so no stat runs away with it: Might, Breath and
// Stamina (the damage and health, straight into the arithmetic) held closer together than Wit and
// Wing (which only tip chances: going first, hitting, dodging, a critical hit).
float ratingPower(float points) { return 10.0f * std::sqrt((points < 1 ? 1.0f : points) + kPowerSoftening); }
float ratingQuick(float points) { return 10.0f * std::sqrt((points < 1 ? 1.0f : points) + kQuickSoftening); }

float witRatio(const Battle& b, int side) {
    const Battler& me = b.side[side];
    const Battler& foe = b.side[1 - side];
    const float a = me.stats.wit * stageMult(me.stage[kStageWit]);
    const float d = foe.stats.wit * stageMult(foe.stage[kStageWit]);
    return d > 0 ? a / d : 2.0f;
}

float wingOf(const Battler& b) { return b.stats.wing * stageMult(b.stage[kStageWing]); }

float attackOf(const Battler& b, const MoveInfo& m) {
    return m.kind == M::Body ? b.stats.might * stageMult(b.stage[kStageMight])
                             : b.stats.breath * stageMult(b.stage[kStageBreath]);
}

float guardOf(const Battler& b) { return 1.0f + kStagePerStep * b.stage[kStageGuard]; }

// The damage before its random roll and a critical hit.
float baseDamage(const Battler& me, const Battler& foe, const MoveInfo& m) {
    return m.power / 100.0f * attackOf(me, m) * kDamage * effectiveness(m.element, foe.kind) / guardOf(foe);
}

float unit(Rng& rng) { return rng.below(10000) / 9999.0f; }

void push(Battle& b, const Event& e) {
    if (b.logCount < kMaxEvents) b.log[b.logCount++] = e;
}

// One side's move: its events into the log. False if it didn't move.
bool act(Battle& b, int side, int slot, Rng& rng) {
    Battler& me = b.side[side];
    Battler& foe = b.side[1 - side];
    if (slot < 0 || slot >= kMoveSlots || !usable(me, slot)) return false;
    const int id = me.moves[slot];
    const MoveInfo& m = kMoves[id];
    Event use;
    use.kind = Ev::Use;
    use.side = static_cast<u8>(side);
    use.move = static_cast<u8>(id);
    push(b, use);
    me.tired = static_cast<s8>(m.power >= kTiringPower ? slot : -1);
    Event e;
    e.move = static_cast<u8>(id);
    if (damaging(m)) {
        e.side = static_cast<u8>(1 - side);
        if (unit(rng) >= hitChance(b, side, slot)) {
            e.kind = Ev::Miss;
            push(b, e);
            return true;
        }
        const bool crit = unit(rng) < critChance(b, side);
        const float eff = effectiveness(m.element, foe.kind);
        float dmg = baseDamage(me, foe, m) * (0.8f + 0.2f * unit(rng)) * (crit ? kCritDamage : 1.0f);
        const int amount = dmg < 1.0f ? 1 : static_cast<int>(dmg + 0.5f);
        e.kind = Ev::Hit;
        e.amount = static_cast<s16>(amount > 30000 ? 30000 : amount);
        e.flags = static_cast<u8>((crit ? kCrit : 0) | (eff > 1.01f ? kStrong : 0) | (eff < 0.99f ? kWeak : 0));
        push(b, e);
        foe.hp = foe.hp > amount ? foe.hp - amount : 0;
        if (foe.hp == 0) {
            Event f;
            f.kind = Ev::Faint;
            f.side = static_cast<u8>(1 - side);
            push(b, f);
            b.over = true;
            b.winner = side;
        }
        return true;
    }
    if (m.effect == E::Heal) {
        e.side = static_cast<u8>(side);
        ++me.healsUsed[slot];
        const int want = me.maxHp * m.amount / 100, missing = me.maxHp - me.hp;
        const int back = want < missing ? want : missing;
        if (back <= 0) {
            e.kind = Ev::NoEffect;
        } else {
            e.kind = Ev::Heal;
            e.amount = static_cast<s16>(back);
            me.hp += back;
        }
        push(b, e);
        return true;
    }
    const int st = stageOf(m.effect);
    if (st < 0) return true;
    const bool self = selfTargeted(m.effect);
    Battler& who = self ? me : foe;
    e.side = static_cast<u8>(self ? side : 1 - side);
    e.stat = static_cast<u8>(st);
    if (!self && unit(rng) >= hitChance(b, side, slot)) {
        e.kind = Ev::Miss;
        push(b, e);
        return true;
    }
    const int dir = self ? 1 : -1;
    const int before = who.stage[st];
    int after = before + dir * m.amount;
    if (after > kMaxStage) after = kMaxStage;
    if (after < -kMaxStage) after = -kMaxStage;
    if (after == before) {
        e.kind = Ev::NoEffect;
    } else {
        who.stage[st] = static_cast<s8>(after);
        e.kind = self ? Ev::StatUp : Ev::StatDown;
        e.amount = static_cast<s16>(after > before ? after - before : before - after);
    }
    push(b, e);
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------- the elements
float matchup(int attack, int defend) {
    if (attack == kBody || attack < 0 || attack >= kElements || defend < 0 || defend >= kElements) return 1.0f;
    if ((attack == kLumen && defend == kShade) || (attack == kShade && defend == kLumen)) return 2.0f;
    const int a = wheelIndex(attack), d = wheelIndex(defend);
    if (a < 0 || d < 0) return 1.0f;
    if ((a + 1) % 6 == d) return 2.0f;
    if ((d + 1) % 6 == a) return 0.5f;
    return 1.0f;
}

float effectiveness(int moveElement, int defenderKind) {
    const KindInfo& k = kindInfo(defenderKind);
    float m = 1.0f;
    for (int i = 0; i < k.elementCount && i < 2; ++i) m *= matchup(moveElement, k.elements[i]);
    return clampf(m, 0.5f, 2.0f);
}

int strongAgainst(int element) {
    if (element == kLumen) return kShade;
    if (element == kShade) return kLumen;
    const int i = wheelIndex(element);
    return i < 0 ? -1 : kWheel[(i + 1) % 6];
}

int weakTo(int element) {
    if (element == kLumen) return kShade;
    if (element == kShade) return kLumen;
    const int i = wheelIndex(element);
    return i < 0 ? -1 : kWheel[(i + 5) % 6];
}

Rgb elementColour(int element) {
    static constexpr Rgb kColours[kElements + 1] = {
        {232, 102, 43},   // Ember: the hearth's orange
        {104, 170, 72},   // Grove: leaf green
        {186, 142, 92},   // Stone: sandstone
        {110, 196, 214},  // Gale: sky
        {64, 128, 212},   // Tide: deep water
        {150, 198, 240},  // Frost: ice
        {240, 196, 72},   // Lumen: gold
        {138, 96, 196},   // Shade: dusk violet
        {170, 150, 150},  // Body: a warm grey
    };
    return kColours[element >= 0 && element <= kElements ? element : kElements];
}

const char* elementLabel(int element) { return element == kBody ? "Body" : elementName(element); }

// ---------------------------------------------------------------------------- the moves
int moveCount() { return kMoveCount; }
bool validMove(int move) { return move >= 0 && move < kMoveCount; }
const MoveInfo& moveInfo(int move) { return kMoves[validMove(move) ? move : 0]; }
const char* moveName(int move) { return validMove(move) ? kMoves[move].name : "---"; }

// ---------------------------------------------------------------------------- what it knows
bool knows(const Dragon& d, int move) {
    if (!validMove(move) || d.stage == Stage::Egg) return false;
    const MoveInfo& m = kMoves[move];
    return m.level <= trainer::levelOf(d) && inLearnset(d.kind, m);
}

int knownMoves(const Dragon& d, u8* out, int cap) {
    return movesLearned(d, 0, d.stage == Stage::Egg ? 0 : trainer::levelOf(d), out, cap);
}

int bestKnownMoves(const Dragon& d, u8* out, int cap) {
    const int n = knownMoves(d, out, cap);
    auto value = [](int move) {  // how hard it hits on average (the stat moves after, as learned)
        const MoveInfo& m = kMoves[move];
        return m.kind == M::Status ? -1.0f + m.level * 0.001f : m.power * m.accuracy / 100.0f;
    };
    for (int a = 1; a < n; ++a)  // (a handful: an insertion sort)
        for (int b = a; b > 0 && value(out[b]) > value(out[b - 1]); --b) {
            const u8 t = out[b];
            out[b] = out[b - 1];
            out[b - 1] = t;
        }
    return n;
}

int movesLearned(const Dragon& d, int fromLevel, int toLevel, u8* out, int cap) {
    int n = 0;
    for (int level = fromLevel + 1; level <= toLevel && level <= kMaxLevel; ++level)
        for (int i = 0; i < kMoveCount; ++i)
            if (kMoves[i].level == level && inLearnset(d.kind, kMoves[i]) && n < cap) out[n++] = static_cast<u8>(i);
    return n;
}

namespace {
bool has(const u8 set[kMoveSlots], int move) {
    for (int i = 0; i < kMoveSlots; ++i)
        if (set[i] == move) return true;
    return false;
}

// Puts a move in the first empty slot (if it isn't there already).
void offer(u8 set[kMoveSlots], int move) {
    if (move < 0 || has(set, move)) return;
    for (int i = 0; i < kMoveSlots; ++i)
        if (set[i] == kNone) {
            set[i] = static_cast<u8>(move);
            return;
        }
}
}  // namespace

void equippedMoves(const Dragon& d, u8 out[kMoveSlots]) {
    for (int i = 0; i < kMoveSlots; ++i) {
        const u8 m = d.moves[i];
        out[i] = kNone;
        if (knows(d, m) && !has(out, m)) out[i] = m;
    }
    u8 known[64];
    const int n = knownMoves(d, known, 64);
    // The best of what it knows: each element's strongest breath, the strongest body move, its
    // element's stat move (else Roar), then whatever hits hardest.
    const KindInfo& k = kindInfo(d.kind);
    for (int e = 0; e < k.elementCount && e < 2; ++e) {
        int best = -1;
        for (int i = 0; i < n; ++i) {
            const MoveInfo& m = kMoves[known[i]];
            if (m.kind == M::Breath && m.element == k.elements[e] && (best < 0 || m.power > kMoves[best].power)) best = known[i];
        }
        offer(out, best);
    }
    int body = -1, special = -1, roar = -1;
    for (int i = 0; i < n; ++i) {
        const MoveInfo& m = kMoves[known[i]];
        if (m.kind == M::Body && (body < 0 || m.power > kMoves[body].power)) body = known[i];
        if (m.kind == M::Status && m.element == k.elements[0] && special < 0) special = known[i];
        if (m.effect == E::LowerMight && m.element == kBody) roar = known[i];
    }
    offer(out, body);
    offer(out, special >= 0 ? special : roar);
    for (int round = 0; round < 2; ++round) {  // the rest: damaging moves by power, then the others
        for (;;) {
            int best = -1;
            for (int i = 0; i < n; ++i) {
                const MoveInfo& m = kMoves[known[i]];
                if (has(out, known[i]) || damaging(m) != (round == 0)) continue;
                if (best < 0 || m.power > kMoves[best].power) best = known[i];
            }
            if (best < 0 || has(out, kNone) == false) break;
            offer(out, best);
        }
    }
}

bool equipMove(Dragon& d, int slot, int move) {
    if (slot < 0 || slot >= kMoveSlots || !knows(d, move)) return false;
    u8 cur[kMoveSlots];
    equippedMoves(d, cur);
    for (int i = 0; i < kMoveSlots; ++i)
        if (cur[i] == move) cur[i] = cur[slot];
    cur[slot] = static_cast<u8>(move);
    for (int i = 0; i < kMoveSlots; ++i) d.moves[i] = cur[i];
    return true;
}

// ---------------------------------------------------------------------------- battle stats
Stats statsFor(const float points[kDragonStats], int level) {
    if (level < 1) level = 1;
    if (level > kMaxLevel) level = kMaxLevel;
    const float f = std::pow(kLevelGrowth, static_cast<float>(level - 1));
    auto st = [&](int s, bool quick) {
        const int v = static_cast<int>((quick ? ratingQuick(points[s]) : ratingPower(points[s])) * f + 0.5f);
        return v < 1 ? 1 : v;
    };
    Stats out;
    const float sturdier = 1.0f + kHealthPerLevel * static_cast<float>(level - 1);
    out.hp = static_cast<int>((10.0f + 2.0f * ratingPower(points[kStatStamina])) * f * sturdier + 0.5f);
    out.might = st(kStatMight, false);
    out.breath = st(kStatBreath, false);
    out.wit = st(kStatWit, true);
    out.wing = st(kStatWing, true);
    return out;
}

Stats battleStats(const Dragon& d) {
    float points[kDragonStats];
    for (int s = 0; s < kDragonStats; ++s) points[s] = battlePoints(d, s);
    return statsFor(points, trainer::levelOf(d));
}

float battlePoints(const Dragon& d, int stat) {
    const int trained = d.trained[stat] > kMaxTrained ? kMaxTrained : d.trained[stat];
    return trainer::statPoints(d, stat) - (1.0f - kTrainedWeight) * trained;
}

Battler makeBattler(const Dragon& d) {
    Battler b;
    std::snprintf(b.name, sizeof(b.name), "%s", d.name[0] ? d.name : kindInfo(d.kind).title);
    b.kind = static_cast<u8>(d.kind < kindCount() ? d.kind : 0);
    b.level = static_cast<u8>(trainer::levelOf(d));
    b.stats = battleStats(d);
    b.maxHp = b.hp = b.stats.hp;
    equippedMoves(d, b.moves);
    int best = 0;
    for (int s = 1; s < kDragonStats; ++s)
        if (trainer::statPoints(d, s) > trainer::statPoints(d, best)) best = s;
    b.strongest = best;
    return b;
}

bool usable(const Battler& b, int slot) {
    if (slot < 0 || slot >= kMoveSlots || !validMove(b.moves[slot])) return false;
    const MoveInfo& m = kMoves[b.moves[slot]];
    return slot != b.tired && (m.effect != E::Heal || b.healsUsed[slot] < kHealUses);
}

// ---------------------------------------------------------------------------- a turn
void begin(Battle& b, const Battler& yours, const Battler& theirs) {
    b = Battle{};
    b.side[0] = yours;
    b.side[1] = theirs;
}

float hitChance(const Battle& b, int side, int slot) {
    const Battler& me = b.side[side];
    if (!validMove(me.moves[slot])) return 0.0f;
    const MoveInfo& m = kMoves[me.moves[slot]];
    if (m.kind == M::Status && selfTargeted(m.effect)) return 1.0f;
    // Its Wit against theirs; a quicker foe (Wing) dodges a little more, a slower one less.
    const float k = clampf(1.0f + kWitHit * (witRatio(b, side) - 1.0f), 0.7f, 1.12f);
    const float quick = wingOf(b.side[1 - side]) / std::fmax(1.0f, wingOf(me));
    const float dodge = clampf(1.0f - kWingDodge * (quick - 1.0f), 0.85f, 1.06f);
    return clampf(m.accuracy / 100.0f * k * dodge, 0.05f, 1.0f);
}

float critChance(const Battle& b, int side) { return 0.02f + 0.06f * clampf(witRatio(b, side), 0.5f, 2.0f); }

float expectedDamage(const Battle& b, int side, int slot) {
    const Battler& me = b.side[side];
    if (slot < 0 || slot >= kMoveSlots || !validMove(me.moves[slot])) return 0.0f;
    const MoveInfo& m = kMoves[me.moves[slot]];
    if (!damaging(m)) return 0.0f;
    const float crit = critChance(b, side);
    return baseDamage(me, b.side[1 - side], m) * 0.9f * (1.0f + crit * (kCritDamage - 1.0f)) * hitChance(b, side, slot);
}

int firstMover(const Battle& b, int slot0, int slot1, Rng& rng) {
    auto priority = [&](int side, int slot) {
        const Battler& me = b.side[side];
        return slot >= 0 && slot < kMoveSlots && validMove(me.moves[slot]) ? kMoves[me.moves[slot]].priority : 0;
    };
    const int p0 = priority(0, slot0), p1 = priority(1, slot1);
    if (p0 != p1) return p0 > p1 ? 0 : 1;
    const float w0 = wingOf(b.side[0]), w1 = wingOf(b.side[1]);
    if (std::fabs(w0 - w1) <= 0.02f * (w0 > w1 ? w0 : w1)) return rng.chance(1, 2) ? 0 : 1;
    return w0 > w1 ? 0 : 1;
}

void resolveTurn(Battle& b, int slot0, int slot1, Rng& rng) {
    b.logCount = 0;
    if (b.over) return;
    const int first = firstMover(b, slot0, slot1, rng);
    const int slots[2] = {slot0, slot1};
    bool acted[2] = {false, false};
    acted[first] = act(b, first, slots[first], rng);
    if (!b.over) acted[1 - first] = act(b, 1 - first, slots[1 - first], rng);
    for (int k = 0; k < 2; ++k)  // one that didn't move has had its breather too
        if (!acted[k]) b.side[k].tired = -1;
    ++b.turn;
    if (!b.over && b.turn >= kMaxTurns) {  // a long stand-off: whoever has more of its health left
        const float a = static_cast<float>(b.side[0].hp) / b.side[0].maxHp, c = static_cast<float>(b.side[1].hp) / b.side[1].maxHp;
        b.over = true;
        b.winner = a > c ? 0 : 1;
        Event e;
        e.kind = Ev::TimeUp;
        e.side = static_cast<u8>(b.winner);
        push(b, e);
    }
}

// ---------------------------------------------------------------------------- choosing
namespace {

float bestDamage(const Battle& b, int side, int kind /* -1 any, 0 body, 1 breath */) {
    float best = 0;
    for (int s = 0; s < kMoveSlots; ++s) {
        const Battler& me = b.side[side];
        if (!validMove(me.moves[s])) continue;
        const MoveInfo& m = kMoves[me.moves[s]];
        if (!damaging(m) || (kind == 0 && m.kind != M::Body) || (kind == 1 && m.kind != M::Breath)) continue;
        const float e = expectedDamage(b, side, s);
        if (e > best) best = e;
    }
    return best;
}

float score(const Battle& b, int side, int slot) {
    const Battler& me = b.side[side];
    const Battler& foe = b.side[1 - side];
    const MoveInfo& m = kMoves[me.moves[slot]];
    const float myBest = std::fmax(1.0f, bestDamage(b, side, -1));
    const float foeBest = std::fmax(1.0f, bestDamage(b, 1 - side, -1));
    const float hpFrac = static_cast<float>(me.hp) / me.maxHp;
    if (damaging(m)) {
        const float e = expectedDamage(b, side, slot);
        const float raw = e / std::fmax(0.05f, hitChance(b, side, slot));  // if it lands
        if (raw * 0.9f >= foe.hp) {  // it would finish them: the surest way first
            float s = foe.hp * (1.5f + hitChance(b, side, slot));
            if (m.priority > 0) s += foe.hp * 0.3f;
            return s;
        }
        return e;
    }
    if (m.effect == E::Heal) {
        if (!usable(me, slot)) return -1.0f;
        const float back = std::fmin(me.maxHp * m.amount / 100.0f, static_cast<float>(me.maxHp - me.hp));
        if (foeBest >= me.hp + back) return 0.0f;  // it wouldn't save it
        return back * (hpFrac < 0.35f ? 1.3f : hpFrac < 0.55f ? 0.7f : 0.1f);
    }
    const int st = stageOf(m.effect);
    if (st < 0) return 0.0f;
    if (selfTargeted(m.effect)) {
        const int cur = me.stage[st];
        if (cur >= kMaxStage) return 0.0f;
        const float turns = clampf(foe.hp / myBest, 0.0f, 4.0f) - 1.0f;  // the turns it has to pay off
        if (turns <= 0) return 0.0f;
        float value = 0;
        switch (st) {
            case kStageMight: value = bestDamage(b, side, 0) * kStagePerStep * m.amount; break;
            case kStageBreath: value = bestDamage(b, side, 1) * kStagePerStep * m.amount; break;
            case kStageWit: value = myBest * 0.07f * m.amount; break;
            case kStageWing: value = wingOf(me) < wingOf(foe) ? myBest * 0.3f : myBest * 0.03f; break;
            case kStageGuard: value = foeBest * 0.2f * m.amount; break;
            default: break;
        }
        return value * turns * hpFrac * (cur > 0 ? 0.5f : 1.0f);
    }
    const int cur = foe.stage[st];
    if (cur <= -kMaxStage) return 0.0f;
    const float turns = clampf(me.hp / foeBest, 0.0f, 4.0f) - 1.0f;
    if (turns <= 0) return 0.0f;
    float value = 0;
    switch (st) {
        case kStageMight: value = bestDamage(b, 1 - side, 0) * 0.2f * m.amount; break;
        case kStageBreath: value = bestDamage(b, 1 - side, 1) * 0.2f * m.amount; break;
        case kStageWit: value = foeBest * 0.07f * m.amount; break;
        case kStageWing: value = wingOf(foe) > wingOf(me) ? myBest * 0.3f : myBest * 0.03f; break;
        default: break;
    }
    return value * turns * hitChance(b, side, slot) * (cur < 0 ? 0.5f : 1.0f);
}

}  // namespace

int chooseMove(const Battle& b, int side, int skill, Rng& rng) {
    if (skill < 0) skill = 0;
    if (skill > 3) skill = 3;
    int slots[kMoveSlots], n = 0;
    for (int s = 0; s < kMoveSlots; ++s)
        if (usable(b.side[side], s)) slots[n++] = s;
    if (n == 0) return -1;
    static constexpr u32 kRandom[4] = {65, 40, 18, 4};  // per cent picked at random, by league
    if (rng.below(100) < kRandom[skill]) return slots[rng.below(static_cast<u32>(n))];
    int pick = slots[0];
    float best = -1e9f;
    for (int i = 0; i < n; ++i) {
        float sc = score(b, side, slots[i]);
        if (skill < 3) sc *= 0.85f + 0.3f * unit(rng);  // not quite sure which is best
        if (sc > best) {
            best = sc;
            pick = slots[i];
        }
    }
    return pick;
}

// ---------------------------------------------------------------------------- after
u32 battleXp(int level, int foeLevel, Outcome o, float factor) {
    if (o == Outcome::GaveUp) return 0;
    const float base = 20.0f + 10.0f * foeLevel;
    const float k = clampf(1.0f + 0.12f * (foeLevel - level), 0.25f, 1.8f);
    float xp = base * k * factor;
    if (o == Outcome::Lost) xp *= 0.33f;  // (a loss still teaches something)
    return static_cast<u32>(xp + 0.5f);
}

Growth grow(Dragon& d, u32 xp) {
    Growth g;
    g.xp = xp;
    g.levelBefore = trainer::levelOf(d);
    trainer::gainXp(d, xp);
    g.levelAfter = trainer::levelOf(d);
    g.learnedCount = movesLearned(d, g.levelBefore, g.levelAfter, g.learned, 4);
    return g;
}

}  // namespace ec::battle
