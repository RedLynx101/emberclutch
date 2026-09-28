// Roaming trainers (1.0, workstream D): a few trainers out walking Skyreach Valley's earth paths
// each day (three to five of eight, picked by the day), their dragon at their heels. They walk
// from junction to junction, stop at the paths' ends to take in the view (sitting down for a
// while), look about at the places they pass, never stop in the Market (they walk through it),
// and wave as you go by. Walk up and press A: their lines, then a friendly duel, their dragon's
// level matched to your partner's (a fair fight). A duel always teaches a little; each trainer's
// first win of the day pays a little Gleam. Pure logic (PC-tested: tests/test_roamers.cpp);
// src/app/feature_roamers.cpp walks them, draws them and stages the duels.
#pragma once

#include <vector>

#include "core/battle.hpp"
#include "core/league.hpp"
#include "core/math3d.hpp"
#include "core/save.hpp"

namespace ec {

struct Valley;
struct Solid;

namespace roam {

constexpr int kRoamers = 8;
constexpr int kMinOut = 3, kMaxOut = 5;        // out on a day
constexpr int kOutFrom = 8, kOutUntil = 20;    // the hours they walk (8:00 to 20:00)
constexpr u8 kNoHair = 0xFF;                   // (a villager's body: its own hair)

struct Roamer {
    const char* name;
    const char* title;       // under the name in the dialogue box
    u8 person;               // core/people Person: a player body, or the traveller's or the child's
    league::Look look;       // colours (its `body` unused: `person` says), hair style (kNoHair: none)
    u8 voice;                // 0: men (Noah's recording), 1: women and children
    float pitch;
    const char* greet[2];    // a bubble as you pass
    const char* hello[2];    // A: before the first duel of the day ({D}: your partner, {P}: you)
    const char* again;       // A: after a duel today
    const char* won;         // after they win
    const char* lost;        // after you win
    const char* dragonName;
    const char* kind;        // core/kinds name
    u8 variant;
    s8 edge;                 // their dragon's level against your partner's (-1 .. +1)
    u8 skill;                // battle::chooseMove's
    float pace;              // how fast they walk, m/s
};

const Roamer& roamer(int id);
void palette(int id, Rgb out[kPalCount]);  // their colours, in the people kit's palette

// ---- Who's out, and when.
int roster(s32 day, u8 out[kMaxOut]);   // the day's (3..5 distinct ids)
bool isOut(s32 day, int id);
bool walkingHour(int hour);             // kOutFrom <= hour < kOutUntil
// Seconds into their walking day at a local time (negative before kOutFrom).
float dayClock(s64 localUnix);

// ---- The paths as a network (Valley::paths): junctions where paths meet or end, stretches
// between them. A path's end at a place stops short of it (its buildings), looking at it; a
// junction in a place (the Market's square, the arena, the lodge) is walked round rather than
// through: each stretch starts and stops `round` metres out, and a trainer crosses straight
// from one stretch to the next.
struct NetNode {
    Vec2 at;          // (a dead end's: where it stops short)
    Vec2 view;        // what they look at, pausing here (the place)
    int place = -1;   // core/valley ValleyPlace within reach (-1: none)
    bool end = false;     // a path's end: a viewpoint
    bool market = false;  // in the Market: they never stop there
    float round = 0;      // metres out from it each stretch starts and stops
    // The crossings between its stretches' ends, round the walls: one per pair of its links
    // (a < b, in pair order), from links[a]'s end to links[b]'s.
    std::vector<std::vector<Vec2>> cross;
};
struct NetEdge {
    int a = 0, b = 0;
    std::vector<Vec2> pts;   // from a to b (trimmed by their `round`)
    std::vector<float> cum;  // distance along it at each point
    float length = 0;
};
struct PathNet {
    std::vector<NetNode> nodes;
    std::vector<NetEdge> edges;
    std::vector<std::vector<int>> links;  // per node: its edges
    bool ok() const { return !edges.empty(); }
};
void buildNet(const Valley& v, const std::vector<Solid>& solids, PathNet& out);
// The way across junction `node` from the end of stretch `from` to the start of stretch `to`
// (both its links; empty if they meet at one point).
std::vector<Vec2> crossing(const PathNet& net, int node, int from, int to);
// A point `d` metres along a polyline (its cumulative lengths), and the way it runs there.
Vec2 along(const std::vector<Vec2>& pts, const std::vector<float>& cum, float d, float* heading = nullptr);

// ---- A day's walk, the same every time for a trainer and a day: legs from junction to
// junction (the crossing from the last stretch, the stretch, a pause at its far end).
struct Walk {
    int id = -1;
    s32 day = 0;
    u64 rng = 1;
    int from = 0, to = 0, edge = -1, prevEdge = -1;
    float legStart = 0, walkFor = 0, pauseFor = 0;
    std::vector<Vec2> pts;   // this leg's way (the crossing, then the stretch)
    std::vector<float> cum;
    int legs = 0;
};
struct Pose {
    Vec2 at;
    float heading = 0;     // the way they walk, or face while paused (0 faces -Y)
    bool walking = false;
    float speed = 0;       // m/s
    bool sitting = false;  // sat down at a viewpoint
    int node = -1;         // paused at
};
void startWalk(const PathNet& net, int id, s32 day, Walk& w);
// Where they are `t` seconds into their day (legs taken as needed; a clock gone back starts over).
Pose walkAt(const PathNet& net, Walk& w, float t);

// ---- A friendly duel: a fair fight, a little experience always, a little Gleam for each
// trainer's first win of the day (Progress::roamDay / roamPaid), the duels won counted.
int duelLevel(int partnerLevel, int id);
Dragon dragonOf(int id, int level);  // grown, its kind and colouring, trained a little by level
u32 duelGleam(int level);
constexpr u32 kGleamCap = 30;
bool paidToday(const SaveData& s, int id, s32 today);
struct Reward {
    battle::Growth growth;
    u32 gleam = 0;
    bool paidBefore = false;  // a win already paid today: none this time
};
Reward record(SaveData& s, int dragonIndex, int id, int foeLevel, battle::Outcome o, s32 today);

}  // namespace roam
}  // namespace ec
