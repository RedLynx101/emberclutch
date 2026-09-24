// What a dragon does in the den, moment to moment (WP5): a small state machine of
// activities driven by its needs, mood, personality and the time of day, interrupted by the
// player's care. It walks the dragon around the den and names the clip that shows it
// (clip names match tools/anim/clips.py). Pure logic: PC-tested in tests/test_behavior.cpp.
#pragma once

#include "core/dragon.hpp"
#include "core/math3d.hpp"
#include "core/rng.hpp"

namespace ec {

// The den floor in adult units, around the origin (the room model in WP6 uses these spots).
struct DenLayout {
    float radius = 4.2f;        // walkable circle
    Vec2 napSpot{2.4f, 1.8f};   // the nest
    Vec2 sulkNook{-3.0f, 2.3f}; // a shadowy corner
    Vec2 home{0.0f, 0.0f};      // where it comes to greet you and to eat
};

enum class Activity : u8 {
    // on its own
    Idle, LookAround, Scratch, Wander, Sit, Lie, Yawn, TailWag, Flutter,
    // rest
    GoNap, Sleep, Wake,
    // care reactions
    Eat, Favorite, PetHead, PetChin, BellyRub, Shake, Hop, Pounce,
    // feelings
    GoSulk, Sulk, MakeUp, Greet,
    Count,
};
const char* activityName(Activity a);

enum class ClipId : u8 {
    Idle, LookAround, Scratch, Walk, Trot, Shuffle, Carry, Sit, SitLoop, LieDown, LieLoop, CurlUp, Sleep,
    Wake, Yawn, NapFlop, Eat, FavWiggle, PetHead, PetChin, RollOver, BellyRub, Shake, Hop, Pounce, TailWag,
    WingFlutter, Sulk, SulkLoop, Nuzzle, Greet,
    Count,
};
const char* clipName(ClipId c);  // the clip's name in the .eca

enum class PetZone : u8 { Head, Chin, Back, Belly };
enum class Care : u8 { Pet, Feed, FeedFavorite, Groom, Play, MakeUp, Greet };

struct DenBehavior {
    Vec2 pos;
    float heading = 0;  // radians about Z; 0 faces -Y (toward the camera)
    Activity activity = Activity::Idle;
    int step = 0;       // step within the activity (e.g. Sit: 0 sitting down, 1 sitting)
    float timer = 0;    // seconds left in a timed step
    Vec2 target;        // where a walking activity is heading
    ClipId clip = ClipId::Idle;
    u16 clipSerial = 0;   // bumps on every clip request, so the same one-shot can replay
    float blend = 0.25f;  // suggested crossfade into `clip`, seconds
    float speed = 0;      // ground speed this frame (adult units per second x moveScale)
    float petTimer = 0;   // keeps a petting reaction alive between strokes
    PetZone petZone = PetZone::Head;
    bool clipDone = false;  // set by the caller when a one-shot clip has finished
    bool trot = false;      // the current walk is a trot
    bool favorite = false;  // the meal being eaten is its favourite
    // Ground speeds (den units per second) that match the walk and trot cycles for this
    // dragon's body, so its feet stay planted (see locomotionSpeed in core/den_actor).
    float walkSpeed = 0.55f, trotSpeed = 1.8f;
    DenLayout den;
    Rng rng{1};

    void reset(const DenLayout& layout, u32 seed);
    // Advances by dt seconds. moveScale: the dragon's size relative to an adult (leaps and
    // arrival distances scale with it; walking uses walkSpeed / trotSpeed).
    void update(const Dragon& d, bool night, float moveScale, float dt);
    // The player's care. Ignored while asleep; only MakeUp reaches an upset dragon.
    void care(Care c, const Dragon& d, PetZone zone = PetZone::Head);
    // Dev menu: jump straight into an activity.
    void force(Activity a) { start(a); }

private:
    void start(Activity a);
    void chooseAmbient(const Dragon& d, float moveScale);
    bool walkTo(Vec2 goal, bool trot, float moveScale, float dt);  // true on arrival
    bool turnTo(float goal, float dt);  // shuffles in place; true when facing it
    void setClip(ClipId c, float crossfade = 0.25f, bool restart = false);
};

}  // namespace ec
