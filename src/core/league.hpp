// 1.0's battle league (D90): four challengers a league standing about the valley (Ember, Flame,
// Blaze, Starfire), each with their own look (the people kit's player bodies in their own
// colours and hair), their lines and their dragon, and each league's champion at Emberpeak
// Caldera, who battles you once the four are beaten. Winning the final gives your dragon its
// title, the league is yours, a prize, and the next league's challengers arrive. The league's
// board (at the arena and the caldera) shows who's beaten and where the rest are. Pure logic
// (PC-tested: tests/test_battle.cpp); src/app/feature_league.cpp stands them in the valley.
#pragma once

#include "core/battle.hpp"
#include "core/math3d.hpp"
#include "core/save.hpp"

namespace ec::league {

constexpr int kPerLeague = 4;           // challengers about the valley
constexpr int kSlots = kPerLeague + 1;  // and the champion
constexpr int kChallengers = kLeagues * kSlots;
constexpr int kChampion = kPerLeague;   // the champion's slot

// How a challenger looks: a player body (0 the tunic, 1 the dress), a hair style (0..5) and
// their colours (the people kit's palette slots: skin, hair, outfit, its trim, boots, eyes).
struct Look {
    u8 body, hair;
    Rgb skin, hairColour, outfit, trim, boots, eyes;
};

struct Challenger {
    const char* name;
    const char* title;   // under the name in the dialogue box
    u8 place;            // core/valley ValleyPlace they stand at (the champions: the caldera)
    Vec2 at;             // in the place's frame
    float facing;        // radians, turned from the place's front
    Look look;
    u8 voice;            // the dialogue's voice (0: men, 1: women and children) and its pitch
    float pitch;
    const char* hello[2];  // before the first battle ({D}: your partner, {P}: you)
    const char* again;     // before a rematch
    const char* beaten;    // after you win
    const char* victory;   // after they win
    const char* dragonName;
    const char* kind;    // core/kinds name ("pouncer")
    u8 variant, level, trained;
    u8 skill;            // battle::chooseMove's
};

int idOf(int league, int slot);  // league 0..3, slot 0..4 (4: the champion)
int leagueOf(int id);
int slotOf(int id);
bool isChampion(int id);
const Challenger& challenger(int id);
// Their colours in the people kit's palette (core/model Palette order).
void palette(const Look& look, Rgb out[kPalCount]);
// Their dragon, ready to battle and to draw (grown, its colouring, its level and training, a
// name; an id apart from the save's).
Dragon dragonOf(int id);
// Where they stand (their place's frame; a champion on the caldera ring's far side) and which
// way they face (radians from the place's front: most face their place's middle).
Vec2 spotOf(int id);
float facingOf(int id);

// ---- Your progress through the leagues (Progress::battleLeague, battleBeaten).
int currentLeague(const SaveData& s);   // the league about the valley now (Starfire stays once all are won)
bool beaten(const SaveData& s, int id); // (a champion: their league won)
int beatenCount(const SaveData& s, int league);
bool championOpen(const SaveData& s, int league);  // the four beaten
bool leagueWon(const SaveData& s, int league);
// Standing about the valley now: the current league's four, and its champion at the caldera.
bool standing(const SaveData& s, int id);
// The one to battle next: the tracked one (the Journal, Tracked::BattleBoard with this id) if
// it still stands, else the first of the current league not yet beaten (then its champion).
int nextChallenger(const SaveData& s);

// ---- After a battle: its experience (battle::grow), the dragon's record, your progress, Gleam
// (a first win pays well; a rematch once a day), a final's title and prize.
struct Reward {
    battle::Growth growth;
    u32 gleam = 0;
    bool firstWin = false;        // the first time you beat them
    bool paidBefore = false;      // a win already paid today: no Gleam this time
    bool championOpened = false;  // the fourth beaten: the champion waits at the caldera
    bool leagueWon = false;       // the final won for the first time: the next league arrives
    bool title = false;           // the dragon earned a new title (trainer::recordLeague)
    u8 prizeFood = 0xFF;          // a final's prize: a food (core/care Food) and how many,
    u8 prizeCount = 0;
    u8 prizeTrinket = 0xFF;       // and a trinket for the hoard (core/wanderings Trinket)
};
Reward record(SaveData& s, int dragonIndex, int id, battle::Outcome o, s32 today);

// ---- Where things are (their places' frames): the caldera's ring and its two sides (the
// champion waits on the far one), and the boards. From core/battle_spots (the places' anchors).
Vec2 ringCentre();
Vec2 ringSide(int side);  // 0: you, 1: the champion
struct BoardSpot {
    u8 place;
    Vec2 at;
    float facing;  // radians from the place's front (its posters' way)
};
int boardCount();
BoardSpot board(int i);

}  // namespace ec::league
