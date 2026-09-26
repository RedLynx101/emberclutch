// The valley's challenges (Beta WP8-WP11, D73 6A, D74): Sky Rings (ridden: you fly the course),
// the Lantern Trial and Fruit Catch (cued from the ground), each with four cups, Ember, Flame,
// Blaze and Starfire. Here are their rules: which cup you may enter with which dragon, how a
// run is judged and what it wins (Gleam, a ribbon for the den, the cup's trophy, a best), the
// Sky Rings courses over the valley and the ghost of your best run, the Lantern Trial's
// patterns, the fruit's flight and the catch, and each element's breath. Pure logic
// (PC-tested: tests/test_challenges.cpp); src/app/scene_challenge.cpp and challenge_*.cpp play them.
#pragma once

#include <cstddef>
#include <vector>

#include "core/flight.hpp"
#include "core/math3d.hpp"
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

// Whether a cup can be entered with this partner (nullptr: none with you): the cup before it
// won, and a dragon that can do it (Sky Rings is ridden, so grown; the Blaze cups want a juvenile
// or older, the Starfire cups a grown dragon).
enum class Entry : u8 { Open, NoPartner, WinBefore, TooYoung, NotGrown };
Entry entry(const SaveData& s, const Dragon* partner, Challenge c, int cup);
const char* entryText(Entry e);  // why not, for the picker ("" when open)
// The hop version of Fruit Catch (a hatchling or a juvenile), and the Lantern Trial's small breath.
bool young(const Dragon& d);

// A run's outcome: the cup won, placed (close: a smaller prize), or not this time.
enum class Outcome : u8 { TryAgain, Placed, Won };
struct Reward {
    Outcome outcome = Outcome::TryAgain;
    u32 gleam = 0;
    bool firstWin = false;  // the cup won for the first time: its ribbon, and the trophy goes up
    bool best = false;      // a new best for this cup
};
// Records a run: the cup (won, and higher than before), its ribbon, the best, the Gleam. `score`
// is the run's own: Sky Rings in tenths of a second (lower is better), the others in points.
Reward record(SaveData& s, Challenge c, int cup, Outcome o, int score);
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
    float par = 0;         // seconds (misses added): at or under it wins the cup
    bool toIsles = false;  // the last ring is at the floating isles' high lantern (the Wings quest)
};
constexpr float kMissPenalty = 3.0f;   // seconds a missed ring adds
constexpr float kPlacedSlack = 1.15f;  // within this much of par: placed
constexpr float kRingTimeLimit = 2.5f; // times par: the run ends there

// The cup's course over this valley (false if its places are missing): rings along a curve
// through its places, longer, lower and trickier cup by cup; its par is the steady pilot's time
// with a little slack, so every course is proven flyable.
bool makeCourse(const Valley& v, int cup, Course& out);
// The flight on a course: the dragon's Wing sets its speed and turning, its Stamina how much a
// wingbeat tires it; the course's wind makes wingbeats cheaper (and each ring gives a lift).
FlightTuning courseTuning(int wing, int stamina);
// Through a ring: a lift (stamina back, a little speed).
void ringLift(Flight& f, const FlightTuning& t);

struct RingRun {
    enum Event : u8 { kNothing, kPassed, kMissed };
    int next = 0, passed = 0, missed = 0;
    float time = 0;
    bool finished = false;
    // One step from `from` to `to` over dt seconds: the next ring passed or missed.
    Event step(const Course& c, Vec3 from, Vec3 to, float dt);
    float total() const { return time + missed * kMissPenalty; }
};
Outcome ringsOutcome(const Course& c, const RingRun& r);
// A steady flier (the par's, and a scripted run's): steers for the next ring, flapping to climb.
FlightInput pilot(const Flight& f, const Course& c, int next);

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

// Flies a course with the pilot at this tuning: its total (misses added), or < 0 if it can't
// finish; the ghost of the flight too, if asked.
float pilotTime(const Valley& v, const Course& c, const FlightTuning& t, int* missed = nullptr, Ghost* ghost = nullptr);

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
    void begin(int cup, u32 seed);
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
Vec2 fruitDragon();

struct Toss {
    Vec3 from, vel;
};
// A flick on the bottom screen (its speed: pixels a second, +y down the screen) thrown from
// `hand` facing `heading` (radians, 0 faces -Y). The hop version throws softer. False: too weak,
// or not away from you.
bool tossFrom(float flickX, float flickY, Vec3 hand, float heading, bool young, Toss& out);
Vec3 fruitAt(const Toss& t, float time);
float landTime(const Toss& t, float groundZ);  // when it comes down to the ground

// Your dragon as a catcher: where it waits, how fast it runs (m/s), how high its mouth is
// standing, how high it can jump, how far it can dive at the end.
struct Catcher {
    Vec3 at;
    float run = 8, reach = 1.5f, leap = 1.8f, dive = 1.6f;
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
constexpr float kCatchReact = 0.25f;  // seconds before the dragon sets off
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
