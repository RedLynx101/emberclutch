#include "core/flight.hpp"

#include <cmath>

#include "core/valley.hpp"

namespace ec {
namespace {

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
float approach(float v, float target, float rate, float dt) {
    const float k = rate * dt > 1 ? 1 : rate * dt;
    return v + (target - v) * k;
}
float wrap(float a) {
    while (a > 3.14159265f) a -= 6.2831853f;
    while (a < -3.14159265f) a += 6.2831853f;
    return a;
}

}  // namespace

Vec3 Flight::forward() const { return {std::sin(heading), -std::cos(heading), 0}; }

void Flight::update(const FlightInput& in, const Valley& v, float dt, const FlightTuning& tune) {
    landed = tookOff = flapped = false;
    sinceFlap += dt;
    const float ground = v.heightAt(pos.x, pos.y);
    if (grounded) {
        stamina = std::fmin(1.0f, stamina + 0.25f * dt);
        speed = 0;
        climb = 0;
        pos.z = ground;
        heading = wrap(heading - in.steer * tune.turnRate * dt);  // right: towards the screen's right
        pitch = approach(pitch, 0, 6, dt);
        roll = approach(roll, 0, 6, dt);
        if (in.flap && stamina > 0.1f) {  // up: a strong first wingbeat
            grounded = false;
            tookOff = flapped = true;
            climb = 7.5f;
            speed = 6;
            sinceFlap = 0;
            flapIn = tune.flapEvery;
            stamina -= tune.flapCost;
        }
        return;
    }
    // Turning: the pad, tighter with L/R; the body leans into it.
    const float turn = clampf(in.steer + in.bank * tune.bankBoost * (in.steer * in.bank >= 0 ? 1.0f : 0.5f), -1.8f, 1.8f);
    // (Seen from behind, the screen's right is the dragon's right: heading goes down, as
    // heading 0 faces -Y and grows towards +X.)
    heading = wrap(heading - turn * tune.turnRate * dt);
    roll = approach(roll, clampf(turn * 0.55f, -0.9f, 0.9f), 4, dt);
    // Speed and sink: gliding settles to glide speed and a gentle sink; wingbeats lift and
    // speed up; a dive trades height for speed.
    float targetSpeed = tune.glideSpeed, targetClimb = -tune.sinkRate;
    const float thin = clampf((pos.z - tune.ceiling) / 30.0f, 0, 1);  // the air thins near the ceiling
    if (in.dive) {
        targetSpeed = tune.diveSpeed;
        targetClimb = -tune.diveSpeed * 0.55f;
    } else {
        targetSpeed += in.pitch * 4.0f;            // nose down: faster; up: slower
        targetClimb += -in.pitch * 2.5f + 0.0f;    // and it rises or sinks a little
        if (sinceFlap < 1.2f) targetSpeed = tune.flapSpeed;
    }
    if (in.flap && !in.dive && (flapIn -= dt) <= 0) {  // a wingbeat
        flapIn = tune.flapEvery;
        const float power = (stamina > 0.02f ? 1.0f : 0.35f) * (1.0f - 0.8f * thin);
        climb += tune.flapLift * power;
        speed += 1.4f * power;
        stamina = std::fmax(0.0f, stamina - tune.flapCost);
        sinceFlap = 0;
        flapped = true;
    }
    if (!in.flap) flapIn = 0;  // the next press beats at once
    if (!in.flap || in.dive) stamina = std::fmin(1.0f, stamina + 0.04f * dt);
    speed = approach(speed, targetSpeed, in.dive ? 1.2f : 0.6f, dt);
    climb = approach(climb, targetClimb, in.dive ? 2.0f : 1.4f, dt);
    pitch = approach(pitch, clampf(-climb / std::fmax(4.0f, speed) * 1.2f, -0.9f, 0.6f), 5, dt);
    // Move, and keep inside the valley (turned back gently at its edges).
    const Vec3 f = forward();
    pos = pos + f * (speed * dt) + Vec3{0, 0, climb * dt};
    const float margin = 30.0f, lo = v.x0 + margin, hiX = v.x0 + v.size() - margin, loY = v.y0 + margin,
                hiY = v.y0 + v.size() - margin;
    if (pos.x < lo || pos.x > hiX || pos.y < loY || pos.y > hiY) {
        pos.x = clampf(pos.x, lo, hiX);
        pos.y = clampf(pos.y, loY, hiY);
        const float home = std::atan2(v.x0 + v.size() * 0.5f - pos.x, -(v.y0 + v.size() * 0.5f - pos.y));
        heading = wrap(heading + clampf(wrap(home - heading), -2.0f * dt, 2.0f * dt));
    }
    // The ground: skim it, or land when slow and it's flat enough.
    const float g = v.heightAt(pos.x, pos.y) + tune.clearance;
    if (pos.z <= g) {
        const Vec3 n = v.normalAt(pos.x, pos.y);
        if (speed < tune.landSpeed && n.z > 0.85f && !in.flap) {
            grounded = true;
            landed = true;
            pos.z = g - tune.clearance;
            speed = climb = 0;
            return;
        }
        pos.z = g;
        climb = std::fmax(climb, 0.5f);  // pulled up off the slope
        speed *= 1.0f - 0.8f * dt;
    }
}

void ChaseCamera::update(const Flight& f, const Valley& v, float dt) {
    const Vec3 fwd = f.forward();
    const float back = f.grounded ? 11.0f : 13.0f + f.speed * 0.25f;
    const float up = f.grounded ? 5.0f : 4.5f + std::fmax(0.0f, -f.climb) * 0.2f;
    const Vec3 wantTarget = f.pos + Vec3{0, 0, f.grounded ? 2.5f : 2.0f} + fwd * 3.0f;
    Vec3 wantEye = f.pos - fwd * back + Vec3{0, 0, up};
    const float ground = v.heightAt(wantEye.x, wantEye.y) + 2.0f;
    if (wantEye.z < ground) wantEye.z = ground;
    if (!set) {
        eye = wantEye;
        target = wantTarget;
        set = true;
        return;
    }
    const float k = std::fmin(1.0f, dt * 4.0f), kt = std::fmin(1.0f, dt * 8.0f);
    eye = eye + (wantEye - eye) * k;
    target = target + (wantTarget - target) * kt;
    const float g2 = v.heightAt(eye.x, eye.y) + 1.5f;
    if (eye.z < g2) eye.z = g2;
}

}  // namespace ec
