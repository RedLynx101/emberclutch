#include "core/props.hpp"

#include <cmath>

namespace ec {
namespace {

constexpr float kGravity = 9.0f;          // a touch floaty: easier to follow on a small screen
constexpr float kBounce = 0.55f;          // vertical speed kept on stone
constexpr float kBounceRug = 0.38f;       // the rug is soft
constexpr float kRugRadius = 2.3f;        // around den.home
constexpr float kRollFriction = 1.6f;     // speed lost per second while rolling
constexpr float kAirDrag = 0.15f;
constexpr float kWallBounce = 0.6f;
constexpr float kRestSpeed = 0.08f;

float len2(Vec2 v) { return v.x * v.x + v.y * v.y; }

}  // namespace

void Ball::launch(Vec3 from, Vec3 v) {
    pos = from;
    vel = v;
    active = true;
    held = false;
    resting = false;
    lastImpact = 0;
}

void Ball::release(Vec3 from, Vec3 nudge) { launch(from, nudge); }

BallEvent Ball::step(const DenLayout& den, float dt) {
    if (!active || held || resting) return BallEvent::None;
    BallEvent ev = BallEvent::None;
    vel.z -= kGravity * dt;
    vel = vel * (1.0f - kAirDrag * dt);
    pos = pos + vel * dt;

    // The floor.
    if (pos.z < radius) {
        pos.z = radius;
        if (vel.z < -0.6f) {  // a real bounce
            const float dx = pos.x - den.home.x, dy = pos.y - den.home.y;
            const bool onRug = dx * dx + dy * dy < kRugRadius * kRugRadius;
            lastImpact = -vel.z;
            vel.z = -vel.z * (onRug ? kBounceRug : kBounce);
            vel.x *= 0.85f;
            vel.y *= 0.85f;
            ev = BallEvent::Bounce;
        } else {  // rolling
            vel.z = 0;
            const float s = std::sqrt(vel.x * vel.x + vel.y * vel.y);
            const float slow = s > 1e-4f ? std::fmax(0.0f, s - kRollFriction * dt) / s : 0.0f;
            vel.x *= slow;
            vel.y *= slow;
        }
    }

    // The walls: the walkable circle around the den's centre.
    const Vec2 c = den.home;
    const Vec2 off{pos.x - c.x, pos.y - c.y};
    const float r = std::sqrt(len2(off));
    const float wall = den.radius - radius;
    if (r > wall && r > 1e-4f) {
        const Vec2 n{off.x / r, off.y / r};
        pos.x = c.x + n.x * wall;
        pos.y = c.y + n.y * wall;
        const float vn = vel.x * n.x + vel.y * n.y;
        if (vn > 0) {
            vel.x -= (1.0f + kWallBounce) * vn * n.x;
            vel.y -= (1.0f + kWallBounce) * vn * n.y;
            lastImpact = vn;
            ev = BallEvent::Bounce;
        }
    }

    // Furniture (the hearth, the egg nest, the hoard): circles on the floor.
    for (const DenObstacle& o : den.obstacles) {
        const Vec2 d{pos.x - o.at.x, pos.y - o.at.y};
        const float dist = std::sqrt(len2(d));
        const float reach = o.radius + radius;
        if (dist < reach && dist > 1e-4f && pos.z < 1.2f) {
            const Vec2 n{d.x / dist, d.y / dist};
            pos.x = o.at.x + n.x * reach;
            pos.y = o.at.y + n.y * reach;
            const float vn = vel.x * n.x + vel.y * n.y;
            if (vn < 0) {
                vel.x -= (1.0f + kWallBounce) * vn * n.x;
                vel.y -= (1.0f + kWallBounce) * vn * n.y;
                lastImpact = -vn;
                ev = BallEvent::Bounce;
            }
        }
    }

    if (pos.z <= radius + 1e-3f && std::fabs(vel.z) < 1e-3f && vel.x * vel.x + vel.y * vel.y < kRestSpeed * kRestSpeed) {
        vel = {0, 0, 0};
        resting = true;
        return BallEvent::Rest;
    }
    return ev;
}

Vec2 ballHeading(const Ball& b, float lookAhead) {
    return {b.pos.x + b.vel.x * lookAhead, b.pos.y + b.vel.y * lookAhead};
}

}  // namespace ec
