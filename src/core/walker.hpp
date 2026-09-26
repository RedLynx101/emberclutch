// Getting about on foot in the valley (Beta WP5, D74, D81): you walking and running in the way
// of a cozy life-sim (speed follows the circle pad, turns with a little arc, no jumping, round
// the places' walls, not into deep water), your partner dragon following at your side (on its
// lead when it's small: it finds its own way round and catches up), and the camera tilted down
// behind you, turned with L and R. Pure logic (PC-tested); scene_valley drives it.
#pragma once

#include <vector>

#include "core/math3d.hpp"
#include "core/types.hpp"

namespace ec {

struct Valley;

// Something to walk round: a place's building, a well, a tree trunk (metres, valley space).
struct Solid {
    Vec2 at;
    float radius = 1;
};

struct WalkInput {
    float x = 0, y = 0;  // the circle pad: -1..1, up +y, relative to the camera's view
    bool run = false;    // B held
};

struct WalkTuning {
    float walkSpeed = 2.8f, runSpeed = 5.6f;  // m/s at a full push (a chibi's jog and run, D86)
    float accel = 14.0f, turnRate = 9.0f;     // m/s^2; radians a second at most
    float wade = 0.6f;                        // no deeper into water than this
    float steepest = 0.72f;                   // no climbing ground whose normal is flatter than this
    float radius = 0.35f;                     // your body, for the walls
};

struct Walker {
    Vec3 pos;           // your feet
    float heading = 0;  // radians about Z; 0 faces -Y (as the dragons)
    float speed = 0;    // m/s, along the heading
    bool blocked = false;  // this step: pushed back by water, a slope or a wall

    Vec3 forward() const;
    // One step; `cameraYaw` turns the pad's up into the direction the camera looks.
    void update(const WalkInput& in, float cameraYaw, const Valley& v, const std::vector<Solid>& solids, float dt,
                const WalkTuning& tune = WalkTuning{});
};

// The partner at your side (D81): it keeps a spot beside you and a little behind (on your
// right), walking, trotting or running to it at its own gaits' speeds, turning to face the way
// you face when it gets there; round the walls, never into deep water. Far behind (lost or
// stuck), it says so, and call() brings it to your side.
struct Follower {
    Vec3 pos;
    float heading = 0, speed = 0;
    float walk = 2.2f, trot = 4.0f, run = 7.0f;  // its gaits' speeds, set from its legs
    float gap = 2.2f;                            // how far to the side it walks (bigger dragons, further)
    float stuckFor = 0;                          // seconds getting no nearer while far off
    bool lost() const { return stuckFor > 3.0f; }

    Vec3 spot(const Walker& you) const;  // where it wants to be
    void update(const Walker& you, const Valley& v, const std::vector<Solid>& solids, float dt);
    void call(const Walker& you, const Valley& v);  // to your side at once
};

// The camera on foot: behind you and above, looking down at about 40 degrees, turned round you
// by L and R (and easing round behind you when you walk away from it); never in the ground.
// A face the walking camera stays in front of (a place built into a cliff: the den's arch):
// a vertical wall through `at` facing `normal`, `halfWidth` either side, up to `top` metres.
struct CameraWall {
    Vec2 at, normal;
    float halfWidth = 0, top = 0;
};

struct WalkCamera {
    Vec3 eye, target;
    float yaw = 0;       // the way it looks, radians about Z (0 faces -Y)
    float distance = 10.5f;
    bool set = false;
    void update(const Walker& you, float turn, const Valley& v, float dt,
                const std::vector<CameraWall>* walls = nullptr);
};

}  // namespace ec
