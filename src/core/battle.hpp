// 1.0's battles (D90): turn-based, one dragon each. The eight elements' wheel, the moves (each
// element's breath by level, body moves anyone can learn, a few that raise or lower a stat or
// heal), what a dragon knows and the four it battles with, its battle stats from its stat points
// and level, a turn's resolution (who goes first, hits and misses, critical hits, the elements'
// matchups, stat stages, fainting), the challengers' and wild dragons' choices, and the
// experience a battle is worth. Pure logic (PC-tested, balanced by simulation: tests/test_battle.cpp);
// src/app/battle_view.cpp shows a battle in the valley, core/league and core/hollow hold who you meet.
#pragma once

#include "core/dragon.hpp"
#include "core/kinds.hpp"
#include "core/rng.hpp"

namespace ec::battle {

// ---------------------------------------------------------------------------- the elements
// core/kinds' elements: Ember, Grove, Stone, Gale, Tide, Frost, Lumen, Shade. The wheel (each one
// strong against the next): Ember melts Frost, Frost cracks Stone, Stone stops Gale, Gale strips
// Grove, Grove drinks Tide, Tide douses Ember. Lumen and Shade stand apart, each strong against
// the other. Strong: twice the damage; into the one that beats it: half.
constexpr int kElements = 8;
constexpr u8 kBody = 8;  // a move of no element (a body move's own)
enum ElementId : u8 { kEmber, kGrove, kStone, kGale, kTide, kFrost, kLumen, kShade };

float matchup(int attack, int defend);         // 2, 1 or 0.5 (kBody: always 1)
// Against a kind (one or two elements): the matchups multiplied, kept within 0.5 .. 2.
float effectiveness(int moveElement, int defenderKind);
int strongAgainst(int element);  // the one it beats (-1: none)
int weakTo(int element);         // the one that beats it (-1: none)
Rgb elementColour(int element);  // for the moves' buttons and the wheel (kBody: a warm grey)
const char* elementLabel(int element);  // "Ember" .. "Shade", kBody: "Body"

// ---------------------------------------------------------------------------- the moves
enum class MoveKind : u8 { Body, Breath, Status };
// What a status move does (a stat's stage up or down by `amount`, or a heal of `amount` per cent).
enum class Effect : u8 { None, RaiseMight, RaiseBreath, RaiseWit, RaiseWing, RaiseGuard, LowerMight, LowerBreath,
                         LowerWit, LowerWing, Heal };
struct MoveInfo {
    const char* name;
    u8 element;    // 0..7, or kBody
    MoveKind kind;
    u8 power;      // 0 for status moves
    u8 accuracy;   // per cent
    u8 level;      // learned at
    Effect effect;
    s8 amount;     // stages, or a heal's per cent
    s8 priority;   // +1: goes first whatever the Wing
    const char* blurb;
};
constexpr int kHealUses = 1;       // a heal works this many times a battle
constexpr u8 kTiringPower = 90;    // moves this strong need a breather: never two turns running
int moveCount();
bool validMove(int move);
const MoveInfo& moveInfo(int move);  // (an invalid id: the first)
// The move's name, "---" for none.
const char* moveName(int move);

// ---------------------------------------------------------------------------- what it knows
// A dragon knows the body and status moves anyone learns and its kind's elements' own, each from
// its level on. Eggs know nothing.
bool knows(const Dragon& d, int move);
// Everything it knows now, in the order it learned them (at most cap).
int knownMoves(const Dragon& d, u8* out, int cap);
// The same, best first: its elements' breath and body moves by how hard they hit, then the stat
// moves (the profile's list; app/profile_hooks' knownMoves).
int bestKnownMoves(const Dragon& d, u8* out, int cap);
// Those a dragon of its kind learns after `fromLevel` up to `toLevel` (a level-up's new moves).
int movesLearned(const Dragon& d, int fromLevel, int toLevel, u8* out, int cap);
// Its four: Dragon::moves as set, with empty slots (and any it doesn't know) filled with the
// best of what it knows (its elements' strongest breath, a body move, a stat move), no repeats.
void equippedMoves(const Dragon& d, u8 out[kMoveSlots]);
// The profile's swap: puts a known move in a slot (the four made explicit first; if the move is
// in another slot the two trade places). False if it doesn't know it or the slot is out of range.
bool equipMove(Dragon& d, int slot, int move);

// ---------------------------------------------------------------------------- battle stats
// From its stat points (trainer::statPoints: the kind's and training's) and its level: health
// from Stamina, the power of body moves from Might and of breath from Breath, who goes first from
// Wing, and accuracy, critical hits and dodging from Wit.
struct Stats {
    int hp, might, breath, wit, wing;
};
Stats statsFor(const float points[kDragonStats], int level);
Stats battleStats(const Dragon& d);
// A stat's points as battles count them: its kind's in full, training's at half (training
// shows, but a well-trained dragon doesn't outclass one ten levels up).
float battlePoints(const Dragon& d, int stat);

enum StatStage : u8 { kStageMight, kStageBreath, kStageWit, kStageWing, kStageGuard, kStages };
constexpr int kMaxStage = 2;

struct Battler {
    char name[24] = {};
    u8 kind = 0;
    u8 level = 1;
    int maxHp = 1, hp = 1;
    Stats stats{};
    s8 stage[kStages] = {};
    u8 moves[kMoveSlots] = {kNone, kNone, kNone, kNone};
    u8 healsUsed[kMoveSlots] = {};
    s8 tired = -1;      // the slot of the tiring move it used last turn (not usable this turn)
    int strongest = 0;  // its best stat (kStatWing ..): what a wild one teaches when beaten
};
Battler makeBattler(const Dragon& d);
bool usable(const Battler& b, int slot);  // a move there (a heal with uses left, not a tiring one again)

// ---------------------------------------------------------------------------- a turn
// What happened, one step at a time, for the valley to show: `side` is the one it happens to
// (Use: the one moving; Hit: the one hit; StatUp/StatDown/Heal: whose it is).
enum class Ev : u8 { Use, Miss, Hit, StatUp, StatDown, NoEffect, Heal, Faint, TimeUp };
enum HitFlags : u8 { kCrit = 1, kStrong = 2, kWeak = 4 };
struct Event {
    Ev kind = Ev::Use;
    u8 side = 0;
    u8 move = 0;
    u8 stat = 0;      // StatUp/StatDown: the StatStage
    u8 flags = 0;     // Hit: HitFlags
    s16 amount = 0;   // Hit: damage; Heal: health back; StatUp/StatDown: stages
};
constexpr int kMaxTurns = 40;  // then the one with more of its health left wins
constexpr int kMaxEvents = 16;

struct Battle {
    Battler side[2];  // 0: yours, 1: the challenger's or the wild one
    int turn = 0;
    bool over = false;
    int winner = -1;
    Event log[kMaxEvents];
    int logCount = 0;
};
void begin(Battle& b, const Battler& yours, const Battler& theirs);
// Who moves first this turn with these slots: 0 or 1.
int firstMover(const Battle& b, int slot0, int slot1, Rng& rng);
// One turn: each side's move slot (-1: none, it waits); the log holds what happened.
void resolveTurn(Battle& b, int slot0, int slot1, Rng& rng);
// The expected damage of a slot's move against the other side (hit chance and critical hits in).
float expectedDamage(const Battle& b, int side, int slot);
// A hit's chance and a critical hit's, for the move in a slot.
float hitChance(const Battle& b, int side, int slot);
float critChance(const Battle& b, int side);

// ---------------------------------------------------------------------------- choosing
// The move a side picks: skill 0 (Ember: mostly at random) .. 3 (Starfire: the best it can see).
int chooseMove(const Battle& b, int side, int skill, Rng& rng);

// ---------------------------------------------------------------------------- after
// The experience a battle is worth to a dragon of `level` against one of `foeLevel`: more
// against stronger ones, little against much weaker; `factor` 1 for a challenger, more for a
// champion or a wild one; a loss a quarter of it; giving up nothing.
enum class Outcome : u8 { Won, Lost, GaveUp };
u32 battleXp(int level, int foeLevel, Outcome o, float factor = 1.0f);
// Experience given (trainer::gainXp) and what came of it: the levels gained and the moves learned.
struct Growth {
    u32 xp = 0;
    int levelBefore = 1, levelAfter = 1;
    u8 learned[4] = {};
    int learnedCount = 0;
};
Growth grow(Dragon& d, u32 xp);

}  // namespace ec::battle
