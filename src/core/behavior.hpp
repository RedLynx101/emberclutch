// What a dragon does in the den, moment to moment (WP5): a small state machine of
// activities driven by its needs, mood, personality and the time of day, interrupted by the
// player's care. It walks the dragon around the den and names the clip that shows it
// (clip names match tools/anim/clips.py). Pure logic: PC-tested in tests/test_behavior.cpp.
#pragma once

#include "core/dragon.hpp"
#include "core/math3d.hpp"
#include "core/rng.hpp"

namespace ec {

struct Ball;  // core/props.hpp

// Something solid on the den floor (hearth, egg nest, hoard): dragons walk around it.
struct DenObstacle {
    Vec2 at;
    float radius;
};

// The den floor in adult units, around the origin; the camera looks in from -Y.
// tools/blender/den_model.py builds the room (romfs/models/den.esm) around the same spots:
// keep them in sync (tests/test_den.cpp checks the two against each other).
struct DenLayout {
    static constexpr int kObstacles = 3;
    static constexpr int kSpots = 3;  // a bed and a sulking spot for each den dragon
    float radius = 6.0f;              // walkable circle around home (the room's walls stand at 9.5)
    // Beds: [0] the big nest by the hearth, then two straw beds. Sulking spots: [0] the nook,
    // a shadowy alcove among rocks (a sulking dragon faces +Y, back to the player), then
    // two quiet corners.
    Vec2 beds[kSpots] = {{4.4f, 3.6f}, {-0.6f, 5.4f}, {2.8f, -3.2f}};
    Vec2 sulkSpots[kSpots] = {{-4.6f, 3.0f}, {-5.6f, 0.0f}, {-4.0f, -3.0f}};
    Vec2 eggNest{5.4f, -1.6f};    // the egg nest, warm by the hearth
    Vec2 home{0.0f, 0.6f};        // the rug: where it greets you and eats
    Vec2 player{0.0f, -2.6f};     // where "you" are: fetched toys come back here, called dragons come
    Vec2 tub{0.0f, -1.6f};        // where the bath tub is set down (WP7)
    Vec2 hearth{7.9f, 0.8f};
    Vec2 hoard{2.4f, 7.4f};
    Vec2 sunSpot{1.0f, 4.6f};     // where the skylight's beam meets the floor
    Vec3 skylight{1.91f, 8.97f, 4.8f};
    DenObstacle obstacles[kObstacles] = {{{7.9f, 0.8f}, 1.5f}, {{5.4f, -1.6f}, 1.2f}, {{2.4f, 7.4f}, 1.6f}};
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
    // hands-on care (WP7, docs/design/care-interactions.md)
    Fetch, HandFeed, Refuse, Bath, Groomed, Kick, Sneeze, PullAway, Come,
    // just hatched: in the egg nest, shaking off the shell, then sitting to meet you; a
    // Greet (after naming) sends it out of the nest
    Hatch,
    Count,
};
const char* activityName(Activity a);

enum class ClipId : u8 {
    Idle, LookAround, Scratch, Walk, Trot, Shuffle, Carry, Sit, SitLoop, LieDown, LieLoop, CurlUp, Sleep,
    Wake, Yawn, NapFlop, Eat, FavWiggle, PetHead, PetChin, RollOver, BellyRub, Shake, Hop, Pounce, TailWag,
    WingFlutter, Sulk, SulkLoop, Nuzzle, Greet,
    PickUp, DropWait, LeapCatch, LegKick, SniffRefuse, LiftWing, Sneeze, PullAway,
    Count,
};
const char* clipName(ClipId c);  // the clip's name in the .eca

// Where a touch lands (core/care zoneOf). The first four have their own reactions; the rest
// share the head-scratch lean-in until they get theirs.
enum class PetZone : u8 { Head, Chin, Back, Belly, Cheek, Neck, Tail, Paw, Wing, Heart };
enum class Care : u8 {
    Pet, Feed, FeedFavorite, Groom, Play, MakeUp, Greet,
    // hands-on care (WP7): the scene reports what the stylus is doing
    Throw,       // a ball was thrown (DenBehavior::ball)
    Call,        // "come here"
    OfferFood,   // food held out near it (every frame while held)
    Bath,        // the tub is out: hop in
    BathDone,    // rinsed: hop out and shake off
    GroomBody,   // being brushed or polished (every stroke)
    GroomBelly,  // ...on the belly: sit up for it
    GroomWing,   // ...on a wing: lift it
    SweetSpot,   // scratched just right
    Poke,        // a tap on the nose
    Rough,       // too hard or too long
};

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
    // Hands-on care (WP7). The scene points `ball` at the den's ball; the dragon keeps it in
    // its mouth while holdingBall (the scene places it at the jaw) and sets dropBall when it
    // lets go (the scene releases it and clears the flag).
    Ball* ball = nullptr;
    // The bath tub is set down just in front of the dragon, sized to it (the scene draws it).
    Vec2 tubAt;
    float tubSize = 0.95f;  // its radius, den units
    bool fumbled = false;   // a baby drops the ball at most once on the way back
    bool holdingBall = false, dropBall = false;
    s8 groomSide = 1;       // which flank it shows while groomed (+1 its right, -1 its left)
    ClipId walkClip = ClipId::Walk;  // what walking looks like (carrying a toy: Carry)
    // Ground speeds (den units per second) that match the walk and trot cycles for this
    // dragon's body, so its feet stay planted (see locomotionSpeed in core/den_actor).
    float walkSpeed = 0.55f, trotSpeed = 1.8f;
    // Small dragons step quicker (DenActor sets it from their size): walking and trotting play
    // this much faster, and cover ground this much faster, so the feet still stay planted.
    float gait = 1.0f;
    float size = 1.0f;  // the last moveScale: how much room the body needs around obstacles
    u8 spot = 0;        // which bed and sulking spot are its own (its place in the den)
    // The other den dragons, set each frame by shareCrowd: it walks around them too.
    DenObstacle crowd[DenLayout::kSpots - 1];
    int crowdCount = 0;
    DenLayout den;
    Rng rng{1};

    void reset(const DenLayout& layout, u32 seed, int spot = 0);
    // Advances by dt seconds. moveScale: the dragon's size relative to an adult (leaps and
    // arrival distances scale with it; walking uses walkSpeed / trotSpeed).
    void update(const Dragon& d, bool night, float moveScale, float dt);
    // The player's care. Ignored while asleep; only MakeUp reaches an upset dragon.
    void care(Care c, const Dragon& d, PetZone zone = PetZone::Head);
    // A bite of hand-fed food reached its mouth. `disliked`: it won't have it; `last`: that
    // was the last bite (a favourite gets the happy wiggle).
    void feedBite(bool disliked, bool last, bool favourite);
    // Dev menu: jump straight into an activity.
    void force(Activity a) { start(a); }
    // How much the dragon looks at the player right now (0..1): full when idle or greeting,
    // none while eating, sleeping or sulking.
    float lookWeight() const;
    // How shut its eyes should be (0 open .. 1 shut): asleep or drifting off, a content
    // squint while petted, a sleepy one while yawning. Blinks come on top (core/den_actor).
    float eyesClosed() const;
    // True if a dragon of this size can stand at p / walk straight from a to b without
    // touching an obstacle or another dragon.
    bool clearAt(Vec2 p, float margin) const;
    bool clearPath(Vec2 a, Vec2 b, float margin) const;

private:
    void start(Activity a);
    void fetch(const Dragon& d, float moveScale, float dt);
    void grab();
    void chooseAmbient(const Dragon& d, float moveScale);
    bool walkTo(Vec2 goal, bool trot, float moveScale, float dt);  // true on arrival
    Vec2 steerTarget(Vec2 goal) const;  // the goal, or a point beside an obstacle in the way
    bool turnTo(float goal, float dt);  // shuffles in place; true when facing it
    void setClip(ClipId c, float crossfade = 0.25f, bool restart = false);
};

// Tells each den dragon where the others are, so they walk around each other. Call once a
// frame before updating them.
void shareCrowd(DenBehavior* const* dragons, int count);

}  // namespace ec
