// Hands-on care (docs/design/care-interactions.md, Alpha 1 WP7): where a touch lands on the
// dragon, what kind of stroke it is, and each dragon's quirks (its sweet spot, the foods it
// won't eat, how it feels about baths). Pure C++ so it's PC-tested; the 3DS app turns the
// stylus into these calls and draws the tools.
#pragma once

#include "core/behavior.hpp"
#include "core/dragon.hpp"
#include "core/math3d.hpp"
#include "core/model.hpp"

namespace ec {

// The tools in the care tray.
// The last four are toys: the ball, and three from the Market (Alpha 2 WP7).
enum class Tool : u8 { Hand, Food, Brush, Cloth, Sponge, Ball, Feather, Rope, Orb, Count };

// ------------------------------------------------------------------------------ touch picking
// A capsule around one bone's skin, in the bone's own frame (its Y axis runs along it):
// from y = t0 to t1, `radius` around the axis, which sits at (cx, cz) in the bone's X/Z.
struct BoneCapsule {
    u8 bone = 0;
    float t0 = 0, t1 = 0, radius = 0;
    float cx = 0, cz = 0;
};
constexpr int kMaxCapsules = 32;

// Capsules for every body bone with enough skin on it, measured from the body mesh (the
// vertices each bone moves most). Returns the count.
int buildCapsules(const ModelData& m, BoneCapsule* out, int max);

// A capsule seen on screen: its axis from a to b (pixels), its radius (pixels) and how far
// from the camera it is (smaller = nearer).
struct ScreenCapsule {
    Vec2 a, b;
    float radius = 0, depth = 0;
};

// The capsule under a touch: the nearest (by depth) one whose outline contains it, or -1.
// t: where along the axis (0 at a, 1 at b); across: -1..1 across the axis (sign: which side
// of a->b, measured 90 degrees counter-clockwise on screen).
int pickCapsule(const ScreenCapsule* caps, int n, Vec2 touch, float& t, float& across);

// What's under the stylus on the dragon (filled by the renderer's close-up picking).
struct TouchHit {
    PetZone zone = PetZone::Head;
    int region = 0;          // BodyRegion
    const char* bone = "";
    Vec3 local;              // the touched point on the skin, armature space (a lean / look target)
    Vec3 outward;            // the skin's direction there, armature space
    Vec2 grain;              // on screen: the way its scales lie there (head to tail), unit
};

// The pet zone and body region of a spot on a bone. `outward` is the skin's direction at
// that spot in armature space (the dragon faces -Y, Z up, +X is its right).
PetZone zoneOf(const char* bone, Vec3 outward, float t);
int regionOf(const char* bone, Vec3 outward);  // BodyRegion

// ------------------------------------------------------------------------------ strokes
enum class Stroke : u8 { None, Gentle, Scrub, Scratch, Poke, Rough };

// Follows one stylus contact and names the stroke so far. Feed it every frame while the
// stylus is down (screen pixels, seconds), then call end().
struct StrokeTracker {
    Vec2 start, last, dir;       // dir: the smoothed direction of motion (unit, or zero)
    float time = 0, distance = 0, speed = 0;  // speed: smoothed pixels per second
    float turning = 0;           // smoothed |turn| per pixel travelled (radians per pixel)
    int reversals = 0;           // direction flips (back-and-forth), forgotten after a pause
    float lastReversal = 0;
    bool down = false;

    void begin(Vec2 p);
    Stroke update(Vec2 p, float dt);
    Stroke end();  // a quick tap counts as a poke
    Stroke kind() const;
};

// ------------------------------------------------------------------------------ quirks
// Each dragon's favourite place to be scratched (rolled from its id, fixed for life).
struct SweetSpot {
    PetZone zone = PetZone::Head;
    s8 side = 0;  // -1 its left, 0 the middle, +1 its right
};
SweetSpot sweetSpotOf(const Dragon& d);
bool atSweetSpot(const Dragon& d, PetZone zone, Vec3 outward);
// Another place it likes (D83): petting or brushing there counts half as much again. Never
// the sweet spot's zone.
PetZone likedZoneOf(const Dragon& d);
// How much a stroke there pleases it: 1.5 on its liked zone, else 1.
float zoneLiking(const Dragon& d, PetZone zone);

enum class Food : u8 {
    Firepepper, RiverFish, Skyberry, Honeyroot, Frostmelon, Starfruit,  // each element's favourite, in Element order
    HearthBread, RoastDrumstick, EmberCandy, GlimmerCookie,
    Count,
};
struct FoodInfo {
    const char* name;
    float belly;  // how much it fills
    u8 bites;     // bites to finish it by hand
    bool treat;
};
const FoodInfo& foodInfo(Food f);

enum class Taste : u8 { Favorite, Liked, Disliked };
// The favourite is the dragon's favoriteFood; one or two other foods are dislikes (from its
// id). Treats are always liked (Ember candy even counts as a favourite for making up).
Taste tasteOf(const Dragon& d, Food f);

enum class BathMood : u8 { Loves, Fine, Grudging };
BathMood bathMoodOf(const Dragon& d);  // Tide loves it, Ember grudges it, the rest don't mind

}  // namespace ec
