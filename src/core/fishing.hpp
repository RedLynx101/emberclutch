// Driftwood Cove (1.0, D90): fishing off the cove's shore with the fisher's rod, and shells washed
// up on its beach. Where things stand at the cove; what bites (mostly River Fish for the pouch,
// now and then other foods, a shell or a rare pearl) and when (quicker at dawn and dusk); the bite
// (the bobber's nibbles, then the real dip and the moment to strike); the reel (keep the line's
// tension in its band while the fish runs, or it snaps or slips the hook); the day's stock (the
// fish are plentiful, not endless) and the day's shells; your partner's nibble. Pure logic
// (PC-tested: tests/test_fishing.cpp); src/app/cove.cpp plays it in the valley.
#pragma once

#include "core/care.hpp"
#include "core/math3d.hpp"
#include "core/rng.hpp"
#include "core/types.hpp"

namespace ec {

struct Dragon;
struct Valley;

namespace fishing {

// ------------------------------------------------------------------------------ the cove
constexpr int kShellSpots = 5;
// In the cove's frame (metres from its anchor on the beach; +Y toward the water, +X its right).
struct CoveSpots {
    Vec2 fishSpot;               // where you stand to fish (facing +Y, the water)
    Vec2 castTo;                 // where the bobber lands, out on the water
    Vec2 partner;                // where your partner sits beside you
    Vec2 fisher;                 // the fisher, by the water
    float fisherFacing = 0;      // (radians in the cove's frame: 0 faces +Y)
    Vec2 shells[kShellSpots];    // where shells wash up
};
// The cove's spots on this valley. Until workstream A's anchors are in (placeAnchor(kPlaceCove,
// "fish_spot" / "fisher" / "shells")), they're picked on the cove's own ground: the waterline
// found along +Y, you a step up from it. The lead swaps the anchors in here, in one place.
CoveSpots coveSpots(const Valley& v);

// ------------------------------------------------------------------------------ what bites
// (Whiskers: Old Whiskers, the cove's legendary catfish, never rolled: he bites at dusk while Tam's quest
// is after him, D137; app/cove)
enum class Catch : u8 { RiverFish, BigFish, Honeyroot, Skyberry, Frostmelon, Shell, Pearl, Whiskers, Count };
struct CatchInfo {
    const char* name;   // "a River Fish"
    Food food;          // into the pouch (Count: none)
    int foodCount;      // how many
    u32 gleam;          // its worth (a shell, a pearl)
    float strength;     // how hard it pulls on the line (0.6 a sprig .. 1.6 a big fish)
    bool fish;          // a fish (counted, kCountFish)
};
const CatchInfo& catchInfo(Catch c);
// What takes the bait: mostly River Fish, a big one now and then (more at dawn and dusk), other
// foods, a shell tangled on the hook, very rarely a pearl. `hour` 0..23.
Catch rollCatch(Rng& rng, int hour);
bool goldenHour(int hour);  // dawn and dusk: the fish bite quicker, the big ones come up

// ------------------------------------------------------------------------------ the bite
constexpr int kMaxNibbles = 3;
struct Bite {
    float wait = 3;                     // seconds after the plop to the real bite
    int nibbles = 0;                    // little tugs before it (a strike on one: too soon)
    float nibbleAt[kMaxNibbles] = {};
    float window = 0.8f;                // seconds the bobber stays under: strike in it
};
Bite rollBite(Rng& rng, int hour, Catch c);
constexpr float kNibbleTime = 0.25f;    // a nibble's twitch

// ------------------------------------------------------------------------------ the reel
// The line's tension (0 slack .. 1 it snaps) rises as you reel and as the fish pulls (harder
// while it runs, now and then), and eases when you let up; the fish comes in (progress 0..1)
// while you reel with the tension in its band, slower out of it, and slips back as it runs.
// Snapped at 1; slack too long and it slips the hook.
constexpr float kBandLow = 0.3f, kBandHigh = 0.8f;
constexpr float kSlackLimit = 1.6f;  // seconds under kSlack before it slips away
constexpr float kSlack = 0.12f;
constexpr float kIdleLimit = 5.0f;   // seconds without reeling at all before it wanders off the hook
struct Reel {
    enum class Step : u8 { Reeling, Caught, Snapped, Escaped };
    float tension = 0.35f, progress = 0.15f;
    float strength = 1;
    float slackFor = 0, idleFor = 0;
    float runFor = 0, runIn = 2;  // the fish running: seconds left; seconds to its next run
    Rng rng{1};
    Step step = Step::Reeling;
    void start(float strength, u32 seed);
    bool running() const { return runFor > 0; }
    bool inBand() const { return tension >= kBandLow && tension <= kBandHigh; }
    // `reel` 0..1: how hard you're reeling (A held: 1; the stylus cranking: its speed).
    Step update(float reel, float dt);
};

// ------------------------------------------------------------------------------ the day
constexpr int kFishPerDay = 8;  // bites a day before the fish rest till tomorrow
// Which of the beach's spots have a shell today (a bit each; three to five of them), and what
// each is (a shell of some kind, now and then a pearl) and its worth.
u8 shellsToday(s32 day);
Catch shellAt(s32 day, int spot);
const char* shellName(s32 day, int spot);  // "a spiral shell"
int shellKind(s32 day, int spot);          // its look (core/challenge_mesh shellMesh: 0 spiral, 1 scallop, 2 cowrie, 3 pearl)
u32 shellGleam(s32 day, int spot);

// ------------------------------------------------------------------------------ your partner
// A nibble of the catch: Love up, a bite for the belly.
void nibble(Dragon& d);
constexpr float kNibbleLove = 6.0f, kNibbleBelly = 3.0f;

}  // namespace fishing
}  // namespace ec
