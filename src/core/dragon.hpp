// A single creature record and its care simulation — see docs/design/game-design.md §3.
#pragma once

#include "core/genetics.hpp"
#include "core/rng.hpp"
#include "core/types.hpp"

namespace ec {

struct Needs {
    float belly = 80, energy = 80, shine = 80, play = 80;  // 0..100
    float lowest() const;
};

enum class Location : u8 { Den, Sanctuary, Vault /* eggs only */ };

// Body regions for dirt (D46) and, later, shine (WP7 brushing). The model tags every body
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

struct Dragon {
    u32 id = 0;
    BodyPlan bodyPlan = BodyPlan::Draconic;
    u8 modules = kModWings | kModBreath;
    Genome genome{};
    Sex sex = Sex::Female;           // rolled when the egg is laid, shown at hatch
    u32 motherId = 0, fatherId = 0;  // 0 = starter / wild / Market egg
    s64 lastBredAt = 0;
    Personality personality = Personality::Playful;
    u8 favoriteFood = 0;  // index into the food table (later)
    char name[16] = {};

    Stage stage = Stage::Egg;
    Location location = Location::Den;
    s64 laidAt = 0;     // local unix
    s64 hatchedAt = 0;  // 0 while still an egg
    s64 lastVisitAt = 0;

    // Egg
    float warmth = 60;           // 0..100
    s32 incubationSeconds = 0;   // progresses only while warm and not vaulted

    // Hatched
    Needs needs{};
    u16 bond = 0, bondHigh = 0;  // 0..1000
    u16 careStars = 0;
    bool upset = false;
    bool napping = false;
    float sulkyHours = 0;
    float dirt[kRegionCount] = {};  // 0 clean .. 100 dusty, per body region (D46)

    // Current-day accounting for care stars
    s32 day = 0;
    float dayLowestSum = 0;
    float dayHours = 0;
    bool dayVisited = false;
};

constexpr s32 kIncubationSeconds = 24 * 3600;

// Stage gates: minimum days since hatching and minimum total care stars.
Stage stageFor(int daysSinceHatch, int careStars);
int stageMinDay(Stage s);
int stageMinStars(Stage s);

Sex rollSex(Rng& rng);
Dragon makeEgg(u32 id, const Genome& g, Sex sex, s64 now);
// Hatches the egg if incubation is complete. Returns true if it hatched.
bool tryHatch(Dragon& d, s64 now, Rng& rng);

// Advance the simulation from d's last update time to `now`, in <= 1 hour steps.
// `from` is the last time this dragon was simulated (the save's timestamp).
void simulate(Dragon& d, s64 from, s64 now);

// Called whenever the player interacts with the dragon.
void markVisit(Dragon& d, s64 now);

// Interactions (clamped, bond-aware). Amounts are need points.
void feed(Dragon& d, float amount, bool favorite);
void pet(Dragon& d, float amount);
void addBond(Dragon& d, int amount);  // nothing while upset; tracks the high-water mark
void groom(Dragon& d, float amount);  // a quick groom: shine up, dust off every region
void cleanRegion(Dragon& d, int region, float amount);  // brushing one region (WP7)
void bathe(Dragon& d);                                   // the bath: every region clean
void play(Dragon& d, float amount);
void warmEgg(Dragon& d, float amount);
// The make-up interaction completes: clears Upset.
void makeUp(Dragon& d);

Mood moodOf(const Dragon& d);
int daysSinceHatch(const Dragon& d, s64 now);
// How far through its current stage the dragon is: 0 at the stage-up, capped at 0.95 until
// the next promotion; 1 for adults. Drives in-stage growth of the model (rig.hpp growthFor).
float stageProgress(const Dragon& d, s64 now);
// Overall body scale (0.25 hatchling .. 1.0 adult), continuous within a stage.
float bodyScale(const Dragon& d, s64 now);

}  // namespace ec
