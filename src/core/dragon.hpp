// A single creature record and its care simulation — see docs/design/game-design.md §3.
#pragma once

#include "core/genetics.hpp"
#include "core/rng.hpp"
#include "core/types.hpp"

namespace ec {

struct Needs {
    // 0..100 (clean was Shine until D83). Love (1.0, D89): petting and brushing, apart from Play
    // (games and toys). Energy is shown apart, below the four (spent by games and challenges,
    // back with sleep), and doesn't count toward mood or care stars.
    float belly = 80, energy = 80, clean = 80, play = 80, love = 80;
    float lowest() const;  // of the four cared-for needs: belly, clean, play, love
};

enum class Location : u8 { Den, Sanctuary, Vault /* eggs only */ };

// Body regions for dirt (D46). The model tags every body
// vertex with one (tools/blender/dragon_model.py body_regions); the wings are one region.
enum BodyRegion : u8 {
    kRegionHead,
    kRegionNeck,
    kRegionBack,
    kRegionBelly,
    kRegionLeft,   // left flank and legs
    kRegionRight,  // right flank and legs
    kRegionTail,
    kRegionWings,
    kRegionCount,
};

// Where an egg came from (Alpha 2 WP8: the profile's family page).
enum class Origin : u8 { Starter, Bred, Wild, Market, Festival, Count };  // Festival: the star-born egg (Beta)

constexpr int kDragonStats = 5;   // Wing, Wit, Might, Breath, Stamina (core/kinds)
constexpr int kDragonTraits = 3;  // at most (three only on the rare colouring)
constexpr int kMoveSlots = 4;     // 1.0 (D90): the moves it battles with (core/battle)
constexpr int kWearSlots = 4;     // 1.0: what it wears (core/accessories: head, neck, back, tail)
constexpr u8 kNone = 0xFF;        // an empty move or accessory slot

struct Dragon {
    u32 id = 0;
    BodyPlan bodyPlan = BodyPlan::Draconic;
    u8 modules = kModWings | kModBreath;
    Genome genome{};
    u8 look = kLookClassic;          // core/genetics Look (D54): set when laid, revealed at hatching
    Sex sex = Sex::Female;           // rolled when the egg is laid, shown at hatch
    u32 motherId = 0, fatherId = 0;  // 0 = starter / wild / Market egg
    Origin origin = Origin::Starter;
    u8 known = 0;  // what you've found out (core/profile Known: its sweet spot, its favourite food)
    s64 lastBredAt = 0;
    Personality personality = Personality::Playful;
    u8 favoriteFood = 0;  // index into the food table (later)
    char name[16] = {};

    Stage stage = Stage::Egg;
    Location location = Location::Den;
    u8 denSlot = 0;  // in the den: its bed (0-2, hatched) or egg nest (0-1); see core/den_roster
    s64 wanderSince = 0;  // out on the Wanderings since (0: at home; core/wanderings)
    u32 wanderSteps = 0;  // the step counter when it set off
    s64 laidAt = 0;     // local unix
    s64 hatchedAt = 0;  // 0 while still an egg
    s64 lastVisitAt = 0;

    // Egg
    float warmth = 60;           // 0..100
    s32 incubationSeconds = 0;   // progresses only while warm and not vaulted
    u8 eggTurns = 0;             // turns that counted (a few hours apart): bond at hatch
    s64 lastTurnedAt = 0;        // the last turn that counted

    // Hatched
    Needs needs{};
    u16 bond = 0, bondHigh = 0;  // 0..1000
    u16 careStars = 0;
    bool upset = false;
    bool napping = false;
    float sulkyHours = 0;
    float dirt[kRegionCount] = {};  // 0 clean .. 100 dusty, per body region (D46)
    float mud[kRegionCount] = {};   // 0 clean .. 100 muddy, per body region (D46, core/mud)

    // Current-day accounting for care stars
    s32 day = 0;
    float dayLowestSum = 0;
    float dayHours = 0;
    bool dayVisited = false;

    // Dragons, version 2 (D77-D80, DR3): its kind (core/kinds, drawn from the look slots after
    // the genome's old looks), its colouring (0-2 common, 3 the rare one: a surprise until it
    // hatches), its stats (1..10), its manner and its traits (core/kinds tables).
    u8 kind = 0;
    u8 variant = 0;
    u8 stats[kDragonStats] = {};
    u8 manner = 0;
    u8 traits[kDragonTraits] = {};
    u8 traitCount = 0;
    // 1.0, a trainer's dragon (D90): its experience (core/trainer: its level), the stat points
    // training and battles have added to its kind's, the moves it battles with (core/battle ids;
    // kNone: empty, filled from what it knows), what it wears (core/accessories) and its dye (0:
    // its own colours).
    u32 xp = 0;
    u8 trained[kDragonStats] = {};
    u8 moves[kMoveSlots] = {kNone, kNone, kNone, kNone};
    u8 wear[kWearSlots] = {kNone, kNone, kNone, kNone};
    u8 dye = 0;
    // Its record, kept per dragon (D90): the highest battle and pageant leagues it has won (0
    // none .. 4 Starfire), its wins, the challenge cups won with it (a bit per challenge * 4 +
    // cup - 1), the show themes it has won (a bit per core/pageant theme) and the deepest floor
    // of Frostspire Hollow it has cleared.
    u8 battleTitle = 0, showTitle = 0;
    u16 battleWins = 0, showWins = 0, wildWins = 0;
    u16 cupsWon = 0;
    u32 ribbons = 0;
    u8 frostDeepest = 0;
};

constexpr s32 kIncubationSeconds = 36 * 3600;  // a day and a half warm (D136)
constexpr s32 kEggTurnGap = 3 * 3600;  // a turn counts again this long after the last one
constexpr u8 kMaxEggTurns = 4;
constexpr u16 kBondPerEggTurn = 30;    // a well-turned egg hatches already fond of you

// Stage gates: minimum hours since hatching and minimum total care stars (D136: grown in 5.5 days
// with the best care; Juvenile at 1.5 days and 3 stars, Adolescent 3.25 days and 7, Adult 5.5 and 12).
Stage stageFor(int hoursSinceHatch, int careStars);
int stageMinHours(Stage s);
int stageMinStars(Stage s);

Sex rollSex(Rng& rng);
// What kind of dragon it is, as the game says it: an egg's breed ("Ember"; the look is a
// surprise until it hatches), a hatched dragon's look and breed ("Pebbleback Ember").
void kindName(const Dragon& d, char* out, int cap);
Dragon makeEgg(u32 id, const Genome& g, Sex sex, s64 now, u8 look = kLookClassic);
// Hatches the egg if incubation is complete. Returns true if it hatched.
bool tryHatch(Dragon& d, s64 now, Rng& rng);
// The temperament it will hatch with: its manner's (DR3; listening to the egg hints at it).
Personality temperamentOf(const Dragon& d);

// Advance the simulation from d's last update time to `now`, in <= 1 hour steps.
// `from` is the last time this dragon was simulated (the save's timestamp).
// eggCooling: how fast an egg's warmth drains (core/items: warm stones in the nests halve it).
void simulate(Dragon& d, s64 from, s64 now, float eggCooling = 1.0f);

// Called whenever the player interacts with the dragon.
void markVisit(Dragon& d, s64 now);

// Interactions (clamped, bond-aware). Amounts are need points.
void feed(Dragon& d, float amount, bool favorite);
void pet(Dragon& d, float amount);    // a stroke of the hand: Love up, a bond point
void brushed(Dragon& d, float amount); // a stroke of the brush: Love up a little more (D83, D89)
void addBond(Dragon& d, int amount);  // nothing while upset; tracks the high-water mark
void bathe(Dragon& d);  // the bath: every region clean, Clean full (the only way, D83)
void play(Dragon& d, float amount);  // a game: Play up, Energy spent (half as fun when tired)
void warmEgg(Dragon& d, float amount);
// Turning the egg. Returns true when the turn counts (up to kMaxEggTurns, kEggTurnGap apart).
bool turnEgg(Dragon& d, s64 now);
// The make-up interaction completes: clears Upset.
void makeUp(Dragon& d);

Mood moodOf(const Dragon& d);
int daysSinceHatch(const Dragon& d, s64 now);
int hoursSinceHatch(const Dragon& d, s64 now);
// How far through its current stage the dragon is: 0 at the stage-up, capped at 0.95 until
// the next promotion; 1 for adults. Drives in-stage growth of the model (rig.hpp growthFor).
float stageProgress(const Dragon& d, s64 now);
// Overall body scale (0.25 hatchling .. 1.0 adult), continuous within a stage.
float bodyScale(const Dragon& d, s64 now);

}  // namespace ec
