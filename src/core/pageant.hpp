// The pageant (1.0, D90): themed shows at Moonpetal Glade. Each show has a theme (the Frost Ball,
// the Harvest Fair ...) favouring some elements, colours and styles, so different dragons shine in
// different shows. Three rounds against rivals picked for the league: Look (clean, dressed for
// the theme in things that suit its colours, the kind's and colouring's rarity, the dye), Poise
// (bond, manner, mood, how well it's cared for) and Performance (tricks you cue in time: a short
// rhythm game; bond widens the window). Three judges hold up cards each round; the most points
// wins. The pageant's board has its own leagues (Ember -> Starfire), four shows a league whose
// themes change by the day; win all four for the league's title. Rewards once a day per show;
// ribbons (a theme each) and titles are kept per dragon (core/trainer). Pure logic (PC-tested).
#pragma once

#include "core/accessories.hpp"
#include "core/dragon.hpp"
#include "core/rng.hpp"
#include "core/trainer.hpp"

namespace ec {

struct SaveData;

constexpr int kThemes = 8;
constexpr int kShowSlots = 4;  // shows a league (Progress::showWon's bits)
constexpr int kEntrants = 4;   // you and three rivals
constexpr int kRivals = kEntrants - 1;
constexpr int kJudges = 3;
constexpr int kRounds = 3;
enum ShowRound : u8 { kRoundLook, kRoundPoise, kRoundPerformance };

struct ThemeInfo {
    const char* name;   // "Frost Ball"
    const char* blurb;  // what it's about, for the board
    u8 elements;        // favoured elements (core/kinds order), a bit each
    Rgb colours[2];     // the colours it favours
    u8 styles;          // favoured StyleTag bits
};
const ThemeInfo& themeInfo(int theme);
const char* roundName(int round);

namespace pageant {

// ---- The board. League 1..4 (Ember .. Starfire, core/trainer leagueName).
// Today's theme for a league's show slot (the four of a league all different on a day).
int showTheme(int league, int slot, s32 day);
bool leagueOpen(const SaveData& s, int league);  // Ember always; each after the one before is won
bool slotWon(const SaveData& s, int league, int slot);
int slotsWon(const SaveData& s, int league);
// The league to show you first on the board: the lowest open one not yet won (Starfire once all are).
int boardLeague(const SaveData& s);
bool paidToday(const SaveData& s, int league, int slot, s32 today);  // its win's reward already given today

// ---- The rounds, 0..100 each. How it looks in this theme (with its palette: its kind's colours
// and its dye), and the parts of that for the results.
struct LookParts {
    float clean = 0, style = 0, themeColour = 0, suits = 0, rarity = 0, element = 0;
    float total() const { return clean + style + themeColour + suits + rarity + element; }
};
LookParts lookParts(const Dragon& d, int theme, const Rgb pal[kPalCount]);
float lookScore(const Dragon& d, int theme, const Rgb pal[kPalCount]);
struct PoiseParts {
    float bond = 0, manner = 0, mood = 0, care = 0;
    float total() const { return bond + manner + mood + care; }
};
PoiseParts poiseParts(const Dragon& d);
float poiseScore(const Dragon& d);
// The element a kind shows in this theme: 2 its first favoured, 1 its second, 0 neither.
int themeFavours(int theme, int kind);

// ---- The Performance: tricks cued by buttons or a touch, in time with the music.
enum class Cue : u8 { A, B, X, Y, Touch, Count };
constexpr int kMaxCues = 16;
struct Routine {
    float at[kMaxCues] = {};  // seconds from the start
    Cue cue[kMaxCues] = {};
    int count = 0;
    float beat = 0.7f;        // seconds a beat
    float length = 0;         // when it ends
};
Routine makeRoutine(int league, u32 seed);
enum class Hit : u8 { Perfect, Good, Miss };
// The timing windows (seconds either side), wider with more bond.
float perfectWindow(const Dragon& d);
float goodWindow(const Dragon& d);
Hit judgeHit(float error, float perfect, float good);
float performanceScore(const Hit* hits, int count);

// ---- The rivals, picked for the league (the same for a show all day). Their kinds come from the
// theme's favourites and your own dragons' kinds (at most one kind you don't have a show, so a
// show loads little that's new).
struct Rival {
    const char* trainer = "";
    Dragon dragon;          // a grown dragon of its kind, named, dressed (not in the save)
    float strength = 0;     // its rounds' middle
};
void makeRivals(const SaveData& s, int league, int slot, s32 day, Rival out[kRivals]);
// A rival's round, with a little of the day's luck.
float rivalRound(const Rival& r, int round, int theme, Rng& rng);

// ---- Judging: each judge's card (1..10, in halves) for a round's score; the round's points are
// their sum.
void judgeCards(float score, int round, Rng& rng, float cards[kJudges]);
// Placing by total points (a tie goes to the better Performance, then to you, entrant 0).
void placings(const float totals[kEntrants], const float performance[kEntrants], int order[kEntrants]);

// ---- After a show: your record and the rewards. `place` 0 first .. 3 fourth.
struct ShowReward {
    u32 gleam = 0;
    int accessory = -1;       // a prize (a show slot's first win)
    int dye = 0;              // a prize dye (a league's first win)
    bool paid = true;         // false: this show's win already paid today (still counts on the board)
    bool firstWin = false;    // this slot's first win
    bool leagueWon = false;   // the fourth slot: the league's title for this dragon
};
ShowReward finishShow(SaveData& s, Dragon& d, int league, int slot, int theme, int place, s32 today);

}  // namespace pageant
}  // namespace ec
