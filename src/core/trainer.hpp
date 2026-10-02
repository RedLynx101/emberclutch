// 1.0, a trainer's game (D90): a dragon's level from its experience, the energy games and
// battles spend, walking together, its record (kept per dragon), and your progress through the
// battle and pageant leagues, Frostspire Hollow, the day's rewards (no farming the same cup or
// rematch twice a day), the Journal's tracked goal and the tutorial's tips. The battle rules are
// core/battle, the accessories and shows core/accessories and core/pageant. Pure logic (PC-tested).
#pragma once

#include "core/dragon.hpp"

namespace ec {

struct SaveData;

constexpr int kLeagues = 4;         // Ember, Flame, Blaze, Starfire (as the cups)
constexpr int kMaxLevel = 50;
constexpr int kLevelCap = 42;      // Skyreach's ceiling (D138): the last 8 levels wait for the next valley
constexpr int kAccessoryBytes = 8;  // room for 64 accessories (core/accessories)
constexpr int kRecordCounts = 8;
constexpr int kMaxTrained = 30;     // stat points training can add to one stat

// The stats in the order Dragon::stats keeps them (core/kinds).
enum StatIndex : u8 { kStatWing, kStatWit, kStatMight, kStatBreath, kStatStamina };

// Your own records (Progress::counts).
enum RecordCount : u8 { kCountFish, kCountShells, kCountBattles, kCountShows, kCountWild, kCountPhotos, kCountWalks,
                        kCountCups };

// What the Journal tracks and the map points at (Progress::trackKind; trackId says which).
enum class Tracked : u8 { None, Quest, BattleBoard, ShowBoard, Hollow, Place, Count };

// The day's rewards already given (Progress::claims, a bit each), so the same cup, rematch or
// show pays once a day: a challenge's cup (challenge * 4 + cup - 1), a battle challenger (league
// * 5 + challenger: the fifth the league's champion), a show (league * 4 + theme slot), a floor.
constexpr int kClaimCup = 0;       // 12
constexpr int kClaimBattle = 12;   // 20
constexpr int kClaimShow = 32;     // 16
constexpr int kClaimHollow = 48;   // 16

struct Progress {
    u8 accessories[kAccessoryBytes] = {};  // owned (core/accessories), a bit each
    u32 dyes = 0;                          // owned (core/accessories Dye), a bit each
    u8 trackKind = 0;                      // Tracked
    u8 trackId = 0;                        // the quest (core/story) or place (core/valley) it points at
    u8 battleLeague = 0;                   // battle leagues won (0 .. 4)
    u8 battleBeaten[kLeagues] = {};        // per league, the board's challengers beaten (a bit each)
    u8 showLeague = 0;                     // pageant leagues won (0 .. 4)
    u8 showWon[kLeagues] = {};             // per league, the board's shows won (a bit each)
    u8 hollowDeepest = 0;                  // Frostspire Hollow's deepest floor cleared, by any dragon
    s32 findsDay = -1000000;               // the day the valley's finds were last scattered (they renew)
    u32 findsSeed = 0;
    s32 claimDay = -1000000;               // the day `claims` is for
    u64 claims = 0;
    u32 tips = 0;                          // the tutorial's tips shown, a bit each
    u16 counts[kRecordCounts] = {};        // RecordCount
    // Driftwood Cove's day (app/cove): the day it's for, the catches landed (the stock), the
    // beach's shells picked (a bit each), Tam's rod lent (for good).
    s32 coveDay = -1000000;
    u8 coveFish = 0, coveShells = 0, coveRod = 0;
    // The valley's critters (core/critters, workstream L): the kinds seen and befriended (a bit
    // each), the day the rewards are for, the kinds befriended that day, the moments paid that
    // day, and how many times each kind has been befriended.
    u8 critterSeen = 0, critterFriends = 0;
    s32 critterDay = -1000000;
    u8 critterToday = 0, critterPaid = 0;
    u8 critterCounts[8] = {};
    // The roaming trainers (core/roamers, workstream D): the day their first wins were paid, whose
    // (a bit each), and the friendly duels won in all.
    s32 roamDay = -1000000;
    u8 roamPaid = 0;
    u16 duelsWon = 0;
};

// The progress block's bytes in the save (after its u16 size).
constexpr int kProgressBytes = kAccessoryBytes + 4 + 2 + 1 + kLeagues + 1 + kLeagues + 1 + 8 + 4 + 8 + 4 + 2 * kRecordCounts;
constexpr int kProgressCoveBytes = 4 + 3;  // (after them: the cove's day, read if there)
constexpr int kProgressCritterBytes = 2 + 4 + 2 + 8;  // (then the critters, workstream L, read if there)
constexpr int kProgressRoamBytes = 4 + 1 + 2;  // (after those: the roaming trainers', read if there)

namespace trainer {

// ---- Levels: the experience to reach each (level 1 at 0), and the level for some experience.
u32 xpForLevel(int level);
int levelOf(u32 xp);
int levelOf(const Dragon& d);
// Experience into the current level and the span of it (for the bar); at the top, span 0.
void levelProgress(const Dragon& d, u32& into, u32& span);
// The experience a dragon takes from `amount` (Quick Learner: a fifth more, D150).
u32 xpTaken(const Dragon& d, u32 amount);
// Adds experience (as xpTaken); returns the levels gained (0 most of the time).
int gainXp(Dragon& d, u32 amount);
// A stat's points: its kind's (with its roll), what training added and Starborn's point (1..41).
int statPoints(const Dragon& d, int stat);
// Training adds to a stat (up to kMaxTrained); false once it's full.
bool train(Dragon& d, int stat, int points = 1);

// ---- Energy: what the day's games, challenges and battles cost; sleep brings it back (core/dragon).
constexpr float kEnergyChallenge = 18.0f;
constexpr float kEnergyBattle = 12.0f;
constexpr float kEnergyShow = 10.0f;
constexpr float kEnergyGame = 6.0f;  // a den game (fetch, tug)
bool canSpend(const Dragon& d, float energy);
bool spendEnergy(Dragon& d, float energy);  // false (and nothing spent) if it's too tired
// What something costing `energy` costs this dragon (Sturdy: a quarter less, D150); canSpend and
// spendEnergy take it so.
float energyCost(const Dragon& d, float energy);

// ---- Walking together (D89): out in the valley with you, bond, Love and Play rise slowly and
// Belly falls a little faster. `carry` keeps the part-metres between calls (the scene holds it).
void walkTogether(Dragon& d, float metres, float& carry);

// ---- Its record.
void recordCup(Dragon& d, int challenge, int cup);  // cup 1..4
bool wonCup(const Dragon& d, int challenge, int cup);
void recordBattle(Dragon& d, bool won);
void recordLeague(Dragon& d, int league);  // won a battle league (1..4): its title
void recordShow(Dragon& d, bool won, int theme);
void recordShowLeague(Dragon& d, int league);
void recordWild(Dragon& d, bool won, int floor);
int ribbonCount(const Dragon& d);
int cupCount(const Dragon& d);
// The dragon's title for its profile ("Starfire Champion", "Blaze Belle", ...; "" if none).
const char* battleTitleName(int league);
const char* showTitleName(int league);
const char* leagueName(int league);  // 1..4: "Ember" .. "Starfire"; 0: ""

// ---- Your progress.
// Today's reward for `bit` (the kClaim ranges): true the first time today, and marks it.
bool claimToday(SaveData& s, int bit, s32 today);
bool claimedToday(const SaveData& s, int bit, s32 today);
bool ownsAccessory(const SaveData& s, int accessory);
void giveAccessory(SaveData& s, int accessory);
bool ownsDye(const SaveData& s, int dye);
void giveDye(SaveData& s, int dye);
bool tipSeen(const SaveData& s, int tip);
void markTip(SaveData& s, int tip);
void count(SaveData& s, RecordCount c, int by = 1);
void track(SaveData& s, Tracked kind, int id);
Tracked tracked(const SaveData& s);

}  // namespace trainer
}  // namespace ec
