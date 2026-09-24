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
    static constexpr int kObstacles = 4;
    static constexpr int kSpots = 3;  // a bed and a sulking spot for each den dragon
    static constexpr int kNests = 2;  // egg nests (Alpha 2: two eggs in the den)
    float radius = 6.0f;              // walkable circle around home (the room's walls stand at 9.5)
    // Beds: [0] the big nest by the hearth, then two straw beds. Sulking spots: [0] the nook,
    // a shadowy alcove among rocks (a sulking dragon faces +Y, back to the player), then
    // two quiet corners.
    Vec2 beds[kSpots] = {{4.4f, 3.6f}, {-0.6f, 5.4f}, {2.8f, -3.2f}};
    Vec2 sulkSpots[kSpots] = {{-4.6f, 3.0f}, {-5.6f, 0.0f}, {-4.0f, -3.0f}};
    // The egg nests, warm by the hearth ([1] sits just beyond the walkable floor: eggs don't
    // walk, and a hatchling climbing out of it is let through).
    Vec2 eggNests[kNests] = {{5.4f, -1.6f}, {6.5f, -3.7f}};
    Vec2 home{0.0f, 0.6f};        // the rug: where it greets you and eats
    Vec2 player{0.0f, -2.6f};     // where "you" are: fetched toys come back here, called dragons come
    Vec2 tub{0.0f, -1.6f};        // where the bath tub is set down (WP7)
    Vec2 hearth{7.9f, 0.8f};
    Vec2 hoard{2.4f, 7.4f};
    Vec2 sunSpot{1.0f, 4.6f};     // where the skylight's beam meets the floor
    Vec3 skylight{1.91f, 8.97f, 4.8f};
    // The hearth, egg nest 0, the hoard, egg nest 1.
    DenObstacle obstacles[kObstacles] = {
        {{7.9f, 0.8f}, 1.5f}, {{5.4f, -1.6f}, 1.2f}, {{2.4f, 7.4f}, 1.6f}, {{6.5f, -3.7f}, 1.2f}};
    // The room itself, for what flies and rolls (the ball, the orb): its wall round the room's
    // centre (den_model.py R = 9.5, less its bumps) and the rocks standing against it. A ball
    // bounced off the walking circle before (Noah, run 3); past it the floor rises toward the
    // wall, so a ball rolls back within reach.
    static constexpr int kWallRocks = 5;
    Vec2 room{0.0f, 0.0f};
    float wallRadius = 8.9f;
    DenObstacle wallRocks[kWallRocks] = {
        {{-4.2f, 7.9f}, 1.2f}, {{-6.2f, 6.4f}, 1.1f}, {{-7.5f, 4.35f}, 1.2f}, {{6.5f, 5.8f}, 1.0f}, {{8.4f, -2.7f}, 0.9f}};
};

// The den's toys on the floor (Alpha 2 WP7), shared by every den dragon: the scene fills this
// each frame and applies what the dragons did with them (core/items has the toys' order).
constexpr int kDenToys = 4;  // the feather wand, the tug rope, the puzzle orb, the food bowl
struct DenToys {
    bool here[kDenToys] = {};  // out on the floor (not in a mouth, not mid tug-of-war)
    Vec2 at[kDenToys];
    bool bowlFood = false;  // something to eat in the bowl
    bool orbTreat = false;  // the orb has a treat in it
    Ball* orb = nullptr;    // the orb rolls when nudged
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
    // life together (Alpha 2 WP1, denSocial): a game of chase (one chases, one flees), a
    // nuzzle, lying in the sunbeam
    Chase, Flee, Nuzzle, Bask,
    // toys (Alpha 2 WP7): batting at the dangled feather, tugging the rope, trotting off
    // proudly with a toy in its mouth and dropping it; on its own, playing with a toy on the
    // floor, eating from the food bowl, and a tug-of-war with another dragon (denSocial)
    Bat, Tug, ToyRun, Play, Bowl, TugWar,
    Count,
};
const char* activityName(Activity a);

enum class ClipId : u8 {
    Idle, LookAround, Scratch, Walk, Trot, Shuffle, Carry, Sit, SitLoop, LieDown, LieLoop, CurlUp, Sleep,
    Wake, Yawn, NapFlop, Eat, FavWiggle, PetHead, PetChin, RollOver, BellyRub, Shake, Hop, Pounce, TailWag,
    WingFlutter, Sulk, SulkLoop, Nuzzle, Greet,
    PickUp, DropWait, LeapCatch, LegKick, SniffRefuse, LiftWing, Sneeze, PullAway,
    PawBat, Tug,
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
    // toys (Alpha 2 WP7)
    Dangle,      // the feather wand held out (every frame): it faces you and watches it
    Swat,        // the feather flicked near its face: a paw swat
    TugPull,     // the rope held (every frame): it bites on and tugs
    TugLetGo,    // let go of the rope: it trots off with it, proud
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
    Vec2 hatchAt{5.4f, -1.6f};  // the nest a Hatch starts in (the scene sets it)
    // The bath tub is set down just in front of the dragon, sized to it (the scene draws it).
    Vec2 tubAt;
    float tubSize = 0.95f;  // its radius, den units
    bool fumbled = false;   // a baby drops the ball at most once on the way back
    bool holdingBall = false, dropBall = false;
    // A toy in its mouth (core/items toy index, -1: none), and one it has just let go of (the
    // scene sets it down on the floor and clears this).
    s8 carrying = -1, dropToy = -1;
    // Toys on the floor (the scene points this at the den's), the one it's playing with, and
    // what came of it this frame (the scene takes these and clears them): a portion eaten from
    // the bowl, a treat from the orb, the orb nudged along (nudge), the feather knocked (knock).
    DenToys* toys = nullptr;
    s8 toy = -1;
    bool ateFromBowl = false, gotTreat = false;
    Vec2 nudge, knock;
    bool nudged = false, knocked = false;
    bool tugWinner = false;  // a tug-of-war: this one ends up with the rope
    u8 pushes = 0;           // the orb pushed along so far
    s8 groomSide = 1;       // which flank it shows while groomed (+1 its right, -1 its left)
    ClipId walkClip = ClipId::Walk;  // what walking looks like (carrying a toy: Carry)
    // Ground speeds (den units per second) that match the walk and trot cycles for this
    // dragon's body, so its feet stay planted (see locomotionSpeed in core/den_actor).
    float walkSpeed = 0.55f, trotSpeed = 1.8f;
    // Small dragons step quicker (DenActor sets it from their size): walking and trotting play
    // this much faster, and cover ground this much faster, so the feet still stay planted.
    float gait = 1.0f;
    // Walking covers this much more ground than its steps would (the steps play as ever):
    // babies get about with 30% more pep (Noah, 2026-09-24), their feet sliding a touch.
    float haste = 1.0f;
    float size = 1.0f;  // the last moveScale: how much room the body needs around obstacles
    u8 spot = 0;        // which bed and sulking spot are its own (its place in the den)
    // Life together (denSocial): its partner in a game or a nuzzle (an index into the den's
    // dragons this frame, -1: none) and what the partner is up to; where it sleeps tonight if
    // it curls up with another in the big nest.
    s8 partner = -1;
    Vec2 partnerAt;
    Activity partnerDoing = Activity::Idle;
    int partnerStep = 0;
    bool snuggle = false;
    Vec2 snuggleAt;
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
    // Starts a shared activity (denSocial calls it): Chase (after `partner`), Flee (from it),
    // Nuzzle (walks to meet it at `at`), Bask (lies in the sunbeam at `at`, partner -1),
    // TugWar (takes its end of the rope at `at`).
    void join(Activity a, s8 withPartner, Vec2 at);
    // True while it's free to start something with another dragon.
    bool sociable() const;
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
    void playWithToy(float moveScale, float dt);
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

// Life together (Alpha 2 WP1): now and then two idle dragons start a game of chase or meet
// for a nuzzle; by bright day one goes to lie in the sunbeam (and another may join it); most
// nights two curl up together in the big nest. Call once a frame after shareCrowd, before
// updating them. daylight: 0 (night) .. 1 (full day).
struct DenSocial {
    float clock = 6.0f;          // seconds to the next chance of something together
    bool wasNight = false;
    bool snuggleTonight = false;
};
void denSocial(DenSocial& s, DenBehavior* const* dragons, const Dragon* const* who, int count, bool night,
               float daylight, float dt, Rng& rng);

}  // namespace ec
