// Toys in the den (docs/design/care-interactions.md §7): a ball you throw that flies,
// bounces, rolls and comes to rest, softer on the rug; the dragon carries it in its mouth.
// Den space: metres-ish adult units, Z up, the floor at z = 0 (core/behavior DenLayout).
#pragma once

#include "core/behavior.hpp"
#include "core/math3d.hpp"

namespace ec {

enum class BallEvent : u8 { None, Bounce, Rest };

struct Ball {
    Vec3 pos{0, -1.5f, 0.12f};
    Vec3 vel{0, 0, 0};
    float radius = 0.12f;
    bool active = false;  // in the den at all
    bool held = false;    // in a dragon's mouth: the renderer places it, physics pauses
    bool resting = true;
    float lastImpact = 0;  // speed of the last bounce (for the sound's volume)

    // Throw from `from` with velocity `v` (den units per second).
    void launch(Vec3 from, Vec3 v);
    // One physics step. Returns what happened (a bounce is reported once per impact).
    BallEvent step(const DenLayout& den, float dt);
    // Let go of a held ball at `from` with a nudge (a dropped ball rolls a little).
    void release(Vec3 from, Vec3 nudge);
};

// How far the ball will roll before resting, roughly: where a fetching dragon should aim
// while it's still moving (the prediction is along the current velocity on the floor).
Vec2 ballHeading(const Ball& b, float lookAhead);

}  // namespace ec
