// Flying a grown dragon over the valley (Beta WP1/WP5, D73 5A): arcade controls. The circle
// pad steers (left/right turns, up/down noses down/up a little), A flaps to climb, B dives,
// L/R bank for a tighter turn, and letting go glides, sinking slowly. Stamina drains with
// each flap and comes back gliding or on the ground. Takes off from the ground with A and
// lands on flat ground when slow. On the ground it walks (pad up, turning with the pad) and
// runs with B; in deep water it swims, floating (D81); it stops at slopes too steep to climb,
// and walking off a drop it glides. Coming down slowly onto the lake it splashes in and swims;
// faster, it skims the water. Pure logic (PC-tested); the valley scene drives it.
#pragma once

#include "core/math3d.hpp"
#include "core/types.hpp"

namespace ec {

struct Valley;

struct FlightInput {
    float steer = 0;   // -1 (left) .. 1 (right)
    float pitch = 0;   // -1 (nose up) .. 1 (nose down)
    float bank = 0;    // -1 (L) .. 1 (R)
    bool flap = false; // held: a wingbeat every so often
    bool dive = false;
};

// Tuning, in metres and seconds (a grown dragon crosses the 1 km valley in 2-3 minutes).
struct FlightTuning {
    float glideSpeed = 11, flapSpeed = 15, diveSpeed = 28;
    float sinkRate = 1.6f;         // gliding: metres a second down
    float flapLift = 4.2f;         // up, per wingbeat
    float flapEvery = 0.55f;       // seconds between wingbeats while A is held
    float flapCost = 0.07f;        // stamina per wingbeat (of 1)
    float turnRate = 1.1f;         // radians a second at full steer
    float bankBoost = 0.9f;        // how much tighter L/R make a turn
    float ceiling = 250;           // thin air above: wingbeats weaker
    float landSpeed = 18;          // slower than this over flat ground: it lands (run 15: 13 made it glide on too long)
    float clearance = 1.2f;        // its feet above the ground in flight
    float groundTurn = 1.7f;       // radians a second, turning on foot
    float wadeDepth = 0.4f;        // on foot: deeper water than this, it swims
    float swimDepth = 1.1f;        // swimming: its feet this far under the surface (it floats, half in)
    float steepest = 0.7f;         // on foot: no climbing slopes whose normal is flatter than this
    float drop = 2.0f;             // on foot: a step down this far is an edge, and it glides off
};

struct Flight {
    Vec3 pos;              // its feet
    float heading = 0;     // radians about Z; 0 faces -Y (as in the den)
    float speed = 0;       // forward, m/s
    float climb = 0;       // vertical, m/s
    // How the body tilts (radians), smoothed for the model: pitch < 0 nose up (climbing),
    // roll > 0 leaning into a right turn.
    float pitch = 0, roll = 0;
    float stamina = 1;
    bool grounded = true;
    bool swimming = false;  // grounded in deep water: floating, paddling
    float walkSpeed = 2.2f, runSpeed = 8.0f;  // on foot, m/s: the scene sets them from its legs
    float flapIn = 0;      // seconds to the next wingbeat while A is held
    float sinceFlap = 9;   // seconds since the last wingbeat (the flap clip plays a while after)
    bool landed = false, tookOff = false, flapped = false;  // this step (sounds, clips)
    bool splashed = false;  // this step: into the water (walking in or coming down onto it)
    bool skimming = false;  // this step: flying fast along the water's surface

    Vec3 forward() const;
    // One step. The valley gives the ground (and keeps it inside its edges).
    void update(const FlightInput& in, const Valley& v, float dt, const FlightTuning& tune = FlightTuning{});
    bool diving(const FlightInput& in) const { return !grounded && in.dive; }
    bool running(const FlightInput& in) const { return grounded && in.dive; }
};

// The camera behind and above, easing after the dragon, never under the ground.
struct ChaseCamera {
    Vec3 eye, target;
    bool set = false;
    void update(const Flight& f, const Valley& v, float dt);
};

}  // namespace ec
