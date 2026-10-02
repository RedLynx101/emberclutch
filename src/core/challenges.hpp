// The valley's challenges (Beta WP8-WP11, D73 6A, D74; retuned for 1.0, D89): Sky Rings (ridden:
// you race the course against rival dragons), the Lantern Trial and Fruit Catch (cued from the
// ground), each with four cups, Ember, Flame, Blaze and Starfire. Here are their rules: which cup
// you may enter with which dragon (and whether it has the energy), how a run is judged and what it
// wins (Gleam and experience once a day per cup, a ribbon for the den, the cup's trophy, a best,
// the dragon's own record), the Sky Rings courses over the valley, the race's flight (momentum, a
// burst and a brake, the Stamina meter), the pilots (yours on autoplay, the par's, the rivals'),
// the ghost of your best run, the Lantern Trial's patterns, the fruit's flight in the wind and the
// catch, and each element's breath. Pure logic (PC-tested: tests/test_challenges.cpp);
// src/app/scene_challenge.cpp and challenge_*.cpp play them.
#pragma once

#include <cstddef>
#include <vector>

#include "core/flight.hpp"
#include "core/math3d.hpp"
#include "core/rng.hpp"
#include "core/save.hpp"

namespace ec {

struct Valley;

namespace challenge {

// ------------------------------------------------------------------------------ the cups (WP8)
// WorldState::cups keeps the highest won per challenge: 0 none, then these.
enum Cup : u8 { kNoCup, kEmber, kFlame, kBlaze, kStarfire };

const char* name(Challenge c);   // "Sky Rings"
const char* blurb(Challenge c);  // a line for the picker
int placeOf(Challenge c);        // the core/valley ValleyPlace it's held at (the arena, the orchard)
const char* cupName(int cup);    // "Ember cup"
Rgb cupColour(int cup);          // its ribbon and trophy (0: a plain grey)

// A stat's worth in the challenges (1.0, D89): its points (the kind's and what training added,
// core/trainer statPoints), training past 10 counting for less (so a trained dragon has an edge,
// never a walkover). 5 is an average dragon's.
float statLevel(const Dragon& d, int stat);
// ...as a share either side of average: (level - 5) / 5, kept within -0.8 .. 1.4.
float statEdge(float level);

// Whether a cup can be entered with this partner (nullptr: none with you): the cup before it
// won, and a dragon that can do it (Sky Rings is ridden, so grown; the Blaze cups want a juvenile
// or older, the Starfire cups a grown dragon) with the energy for it (trainer::kEnergyChallenge).
enum class Entry : u8 { Open, NoPartner, WinBefore, TooYoung, NotGrown, Tired };
Entry entry(const SaveData& s, const Dragon* partner, Challenge c, int cup);
const char* entryText(Entry e);  // why not, for the picker ("" when open)
// The hop version of Fruit Catch (a hatchling or a juvenile), and the Lantern Trial's small breath.
bool young(const Dragon& d);

// What a cup asks (the picker, and the results' note of what the next cup needs).
struct CupNeeds {
    int rivals = 0;                          // Sky Rings: the rivals to beat to the last ring
    int goal = 0;                            // Fruit Catch: the points
    int lanterns = 0, rounds = 0, hearts = 0;  // the Lantern Trial
    bool juvenile = false, grown = false;    // the partner it needs
};
CupNeeds cupNeeds(Challenge c, int cup, bool youngPartner);

// A run's outcome: the cup won, placed (close: a small prize), or not this time.
enum class Outcome : u8 { TryAgain, Placed, Won };
struct Reward {
    Outcome outcome = Outcome::TryAgain;
    u32 gleam = 0;
    u32 xp = 0;               // the partner's experience (core/trainer)
    int levels = 0;           // levels it gained with it
    bool firstWin = false;    // the cup won for the first time: its ribbon, and its trophy goes up
    bool dragonFirst = false; // the partner's own first win of this cup (its record)
    bool paidToday = false;   // won again, but today's prize for this cup was already given
    bool best = false;        // a new best for this cup
};
// The prizes (D89: harder cups worth more, no farming): a cup's first win pays its first prize;
// after that a win pays once a day per cup (trainer::claimToday, kClaimCup + challenge * 4 +
// cup - 1); placing pays a little while the day's prize is still to win; not this time, nothing.
// Experience with the day's prize, a little for any other finished run.
u32 firstPrize(int cup);
u32 dayPrize(int cup);
u32 placedPrize(int cup);
u32 winXp(int cup);
constexpr u32 kRunXp = 4;
// Records a run: the cup (won, and higher than before), its ribbon, the best, the Gleam, the
// partner's record (trainer::recordCup) and experience, your count of cups won. `score` is the
// run's own: Sky Rings in tenths of a second (lower is better), the others in points. `today`:
// the day (core/clock dayIndex); `partner` may be null.
Reward record(SaveData& s, Challenge c, int cup, Outcome o, int score, s32 today, Dragon* partner);
int claimBit(Challenge c, int cup);  // its bit in Progress::claims
bool lowerIsBetter(Challenge c);
int best(const SaveData& s, Challenge c, int cup);  // 0: none yet
// The ribbons (a bit per cup won, WorldState::ribbons): for the den's shelves.
bool ribbon(const SaveData& s, Challenge c, int cup);
int ribbonCount(const SaveData& s);

// ------------------------------------------------------------------------------ Sky Rings (WP9)
struct Ring {
    Vec3 at;
    Vec3 normal;  // the way through it
    float radius;
};
struct Course {
    int cup = 0;
    std::vector<Ring> rings;
    Vec3 start;            // where the run starts, hovering (the countdown), facing `heading`
    float heading = 0;
    float length = 0;      // metres from the start through every ring
    float par = 0;         // seconds (misses added): the steady pilot's time with a little slack
    bool toIsles = false;  // the last ring is at the floating isles' high lantern (the Wings quest)
};
constexpr float kMissPenalty = 3.0f;   // seconds a missed ring adds
constexpr float kRingTimeLimit = 2.5f; // times par: the run ends there

// The cup's course over this valley (false if its places are missing): rings along a curve
// through its places, longer, lower and trickier cup by cup; its par is the steady pilot's time
// with a little slack, so every course is proven flyable.
bool makeCourse(const Valley& v, int cup, Course& out);

// The race's flight (D89): arcade flying with momentum. Speed carries: level flight eases back
// to cruising speed only slowly, climbing costs speed and diving gives it back, hard turns bleed
// it (and a fast dragon turns wider, a slow one tighter). R bursts toward top speed while the
// Stamina meter lasts; L brakes (and turns tighter). A wingbeat (A) lifts and costs a little of
// the meter too; easing off fills it again, and each ring passed tops it up. The dragon's Wing
// sets its speeds and turning, its Stamina the meter's size. Flight::stamina is the meter's
// share full (0..1); the race never lands (it skims the ground or the lake instead).
struct RaceInput {
    float steer = 0;  // -1 (left) .. 1 (right)
    float pitch = 0;  // -1 (nose up) .. 1 (nose down)
    bool flap = false, dive = false;
    bool burst = false, brake = false;
};
struct RaceTuning {
    float cruise = 14, top = 19, diveTop = 30, slow = 7;  // m/s: level, a burst's, a dive's, braked
    float turnRate = 1.25f;  // radians a second at full steer, at cruising speed
    float turnDrag = 0.14f;  // the share of its speed a second a full-rate turn costs, per radian a second
    float meter = 3.5f;      // the Stamina meter, full: seconds of burst
    float flapCost = 0.1f;   // meter-seconds a wingbeat
    float regen = 0.2f;      // meter-seconds a second back while easing off
    float flapLift = 4.2f, flapEvery = 0.55f, sinkRate = 1.6f, clearance = 1.2f;
};
// The tuning for a dragon's Wing and Stamina levels (statLevel).
RaceTuning raceTuning(float wing, float stamina);
RaceTuning raceTuning(const Dragon& d);
// One step of the race's flight.
void raceStep(Flight& f, const RaceInput& in, const Valley& v, float dt, const RaceTuning& t);
bool bursting(const Flight& f, const RaceInput& in);  // this step's burst took hold (the meter not empty)
// Through a ring: a lift (the meter topped up, a little speed).
void ringLift(Flight& f, const RaceTuning& t);

struct RingRun {
    enum Event : u8 { kNothing, kPassed, kMissed };
    int next = 0, passed = 0, missed = 0;
    float time = 0;
    bool finished = false;
    // One step from `from` to `to` over dt seconds: the next ring passed or missed.
    Event step(const Course& c, Vec3 from, Vec3 to, float dt);
    float total() const { return time + missed * kMissPenalty; }
    // How far along the course (rings passed or missed, and a share of the way to the next), for
    // the race's standings.
    float progress(const Course& c, Vec3 at) const;
};

// A pilot (the par's steady flier, your autoplay, the rivals): it lines up the next ring (aiming
// inside it toward the one after, the racing line), flaps to climb and dives to drop; a good one
// bursts along the straights and brakes into hard turns; a shaky one wanders about its line and
// now and then goes wide of a ring.
struct PilotSkill {
    float aim = 1.0f;       // how firmly it steers for its line
    float wobble = 0;       // radians its aim wanders
    float burst = 0;        // 0 never .. 1 whenever it's straight and the meter allows
    bool brakes = false;    // brakes into hard turns
    float blunder = 0;      // the chance it goes wide of a ring
    float line = 0.5f;      // how far inside the ring (share of its radius) it cuts toward the next
};
PilotSkill steadyPilot();  // no bursts, no mistakes: the par's
PilotSkill expertPilot();  // bursts and brakes, no mistakes: your autoplay
struct Pilot {
    PilotSkill skill;
    Rng rng{1};
    float t = 0, phase = 0;
    int decidedFor = -1;     // the ring its blunder roll is for
    bool wide = false;       // going wide of it
    bool pushes = false;     // bursting along the way to it
    float side = 1;
    RaceInput fly(const Flight& f, const Course& c, int next, const RaceTuning& tune, float dt);
};

// A flier on the course: its flight, tuning, pilot and run (the rivals, the par, the tests).
struct Racer {
    Flight flight;
    RaceTuning tune;
    Pilot pilot;
    RingRun run;
    RaceInput last;
    void start(const Course& c, Vec3 at, float heading);
    // A step (a lift through each ring passed); after the last ring it eases to a hover.
    RingRun::Event step(const Valley& v, const Course& c, float dt);
};
// Flies a racer on to the end (a copy): its total, or < 0 if it can't finish by `limit` seconds.
float finishRace(Racer r, const Valley& v, const Course& c, float limit);

// The rivals (D89): two at the Ember cup, three after; faster, steadier and cannier cup by cup.
constexpr int kMaxRivals = 3;
struct RivalInfo {
    const char* name;
    float wing, stamina;  // stat levels
    PilotSkill skill;
};
int rivalCount(int cup);
RivalInfo rivalFor(int cup, int k);
// Where they wait at the start: beside and a little behind you (metres to your right, ahead),
// and so where that is over the course.
Vec2 rivalStart(int k);
Vec3 rivalStartAt(const Course& c, int k);
// Where you came: 1 + the rivals whose totals beat yours (a tie goes to you).
int racePlace(float yours, const float* rivals, int n);
// First place wins the cup, second places; unfinished (time's up) is not this time.
Outcome raceOutcome(bool finished, int place);

// The ghost of your best run: where it was every kGhostStep seconds.
struct GhostPoint {
    Vec3 at;
    float heading = 0;
};
constexpr float kGhostStep = 0.2f;
constexpr int kGhostMaxPoints = 1500;  // five minutes
struct Ghost {
    std::vector<GhostPoint> points;
    void clear() { points.clear(); }
    void record(float time, Vec3 at, float heading);  // call every frame: keeps one a step
    bool at(float time, Vec3& pos, float& heading) const;  // false before and after it
    float duration() const { return points.size() > 1 ? (points.size() - 1) * kGhostStep : 0.0f; }
};
std::size_t ghostBytes(const Ghost& g);
std::size_t encodeGhost(const Ghost& g, u8* out, std::size_t cap);
bool decodeGhost(const u8* data, std::size_t size, Ghost& out);

// Flies a course with a pilot at this tuning: its total (misses added), or < 0 if it can't
// finish; the ghost of the flight too, if asked.
float pilotTime(const Valley& v, const Course& c, const RaceTuning& t, const PilotSkill& skill, int* missed = nullptr,
                Ghost* ghost = nullptr, u32 seed = 1);

// ------------------------------------------------------------------------------ the Lantern Trial (WP10)
constexpr int kTrialLanterns = 6;
constexpr int kTrialLongest = 12;
struct TrialSetup {
    int lanterns, rounds, first, hearts;
    float show;  // seconds each lantern is lit while the pattern is shown
};
TrialSetup trialSetup(int cup);
// Where each crystal lantern stands (the arena's frame: metres, +Y its gate), in an arc before
// the stage; where you stand, and your dragon.
Vec2 trialLantern(int lanterns, int k);
Vec2 trialYou();
Vec2 trialDragon();
Rgb trialColour(int k);
float trialPitch(int k);  // its chime, a pentatonic step

struct Trial {
    TrialSetup setup{};
    u8 pattern[kTrialLongest] = {};
    int round = 0;  // 0-based
    int input = 0;  // lanterns lit right so far this round
    int hearts = 0;
    int score = 0;
    bool over = false, won = false;
    void begin(int cup, u32 seed, int extraHearts = 0);  // (Sure-Footed's heart: D150)
    int length() const;  // this round's pattern
    enum class Press : u8 { Right, RoundDone, Wrong, Won, Lost };
    Press press(int lantern);
    int roundsCleared() const { return won ? setup.rounds : round; }
};
Outcome trialOutcome(const Trial& t);

// ------------------------------------------------------------------------------ Fruit Catch (WP11)
constexpr float kGravity = 9.8f;
enum class Fruit : u8 { Apple, Pear, Plum, Golden, Count };
struct FruitSetup {
    int throws;
    int goal;    // points to win the cup
    int placed;  // points to place
};
FruitSetup fruitSetup(int cup, bool young);
Fruit fruitFor(int throwIndex, u32 seed);  // the 4th and 8th are golden (double points)
Rgb fruitColour(Fruit f);
// Where you stand in the orchard (its frame), your dragon, and the way you throw.
Vec2 fruitYou();
Vec2 fruitDragon(bool young);  // a young one waits nearer the throw

struct Toss {
    Vec3 from, vel;
    Vec3 wind;  // the breeze's push on the fruit (m/s^2, level): it drifts as it flies
};
// A flick on the bottom screen (its speed: pixels a second, +y down the screen) thrown from
// `hand` facing `heading` (radians, 0 faces -Y). The hop version throws softer. False: too weak,
// or not away from you. (No wind: the caller adds the throw's.)
bool tossFrom(float flickX, float flickY, Vec3 hand, float heading, bool young, Toss& out);
Vec3 fruitAt(const Toss& t, float time);
float landTime(const Toss& t, float groundZ);  // when it comes down to the ground
// The breeze for a throw (D89: the higher cups want reading the wind): none at the Ember cup,
// stronger cup by cup, a new one each throw from any way (a tailwind carries a throw further,
// a headwind holds it back, a crosswind drifts it). Level, m/s^2.
Vec3 windFor(int cup, int throwIndex, u32 seed);

// Your dragon as a catcher: where it waits, how fast it runs (m/s), how high its mouth is
// standing, how high it can jump, how far it can dive at the end, how long it watches the throw.
// Wing sets its running, Wit its reach (a clever dragon judges the throw sooner, leaps higher
// and stretches further in a dive), D89.
struct Catcher {
    Vec3 at;
    float run = 8, reach = 1.5f, leap = 1.8f, dive = 1.6f;
    float react = 0.25f;
    bool young = false;
};
Catcher catcherFor(const Dragon& d, Vec3 at, float runSpeed, float size);
enum class Style : u8 { Missed, Snap, Leap, SkyLeap, Dive, Hop, Tumble };
const char* styleName(Style s);  // "Leaping catch!"
struct CatchPlan {
    Style style = Style::Missed;
    float t = 0;          // when it's caught (missed: when it lands)
    Vec3 at;              // where the fruit is then
    float leaveAt = 0;    // when the dragon sets off (it watches the throw first)
    float arriveAt = 0;   // when it gets under it
    float jump = 0;       // how high its feet leave the ground for the catch
    float distance = 0;   // from the throw, metres along the ground
};
CatchPlan planCatch(const Toss& t, const Catcher& c, float groundZ);
int catchPoints(const CatchPlan& p, bool golden, bool young);
Outcome fruitOutcome(const FruitSetup& s, int score);

// ------------------------------------------------------------------------------ breath (WP10)
// Each element's breath: flame (Ember), spores (Grove), a sandy gust (Stone), a gust (Gale), mist
// (Tide), frost (Frost), light (Lumen) and dusk-light (Shade).
enum class Breath : u8 { Flame, Mist, Gust, Spores, Frost, Light };
struct BreathLook {
    Breath kind;
    Rgb a, b;           // the colour it starts and fades to
    float spread;       // how wide the stream fans (radians)
    float rise;         // m/s up (spores float, frost sinks)
    float size0, size1; // a puff's size, metres, as it goes
    float wobble;       // spores drift about
};
BreathLook breathFor(int element);
struct Puff {
    Vec3 pos, vel;
    float age = 0, life = 1, size0 = 0.2f, size1 = 0.4f;
    u8 seed = 0;
    float t() const { return age / life; }
};
// The breath's puffs, a fixed pool: a stream from the mouth to what it's breathed at.
class BreathFx {
public:
    static constexpr int kMax = 72;
    void clear() { count_ = 0; }
    // `count` puffs from `from` to `to`, arriving in about `travel` seconds.
    void emit(const BreathLook& look, Vec3 from, Vec3 to, int count, float travel);
    void update(float dt);
    int count() const { return count_; }
    const Puff& operator[](int i) const { return p_[i]; }
    BreathLook look{};

private:
    Puff p_[kMax];
    int count_ = 0;
    u32 seed_ = 0x51A7u;
    float unit();
};

}  // namespace challenge
}  // namespace ec
