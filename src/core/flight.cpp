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

FlightTuning flightTuningFor(float wingLevel, float staminaLevel) {
    auto edge = [](float level) {  // (as core/challenges statEdge: 0 average, 1 at 10, up to 1.4 trained)
        const float e = (level - 5.0f) / 5.0f;
        return e < -0.8f ? -0.8f : (e > 1.4f ? 1.4f : e);
    };
    const float w = edge(wingLevel), st = edge(staminaLevel);
    FlightTuning t;
    const float cost = 1.0f / (1.0f + 0.5f * st);  // (10: half as long again on a breath; 1: two thirds)
    t.flapCost *= cost;
    t.burstCost *= cost;
    t.restRate *= 1.0f + 0.3f * st;
    t.glideRate *= 1.0f + 0.3f * st;
    t.flapSpeed *= 1.0f + 0.08f * w;
    t.burstSpeed *= 1.0f + 0.1f * w;
    t.glideSpeed *= 1.0f + 0.06f * w;
    t.flapLift *= 1.0f + 0.1f * w;
    t.turnRate *= 1.0f + 0.08f * w;
    return t;
}

bool glidedFromHeights(Vec3 vault, float radius, Vec3 leftGroundAt, Vec3 downAt) {
    const float fromVault = std::hypot(leftGroundAt.x - vault.x, leftGroundAt.y - vault.y);
    return fromVault <= radius && leftGroundAt.z >= vault.z - 12.0f && leftGroundAt.z - downAt.z >= 25.0f;
}

void Flight::update(const FlightInput& in, const Valley& v, float dt, const FlightTuning& tune) {
    landed = tookOff = flapped = splashed = skimming = false;
    sinceFlap += dt;
    const float ground = v.groundAt(pos.x, pos.y, pos.z);  // (an island's top too)
    const bool onIsland = v.islandAt(pos.x, pos.y, pos.z) >= 0 || v.deckAt(pos.x, pos.y, pos.z) >= 0;  // (or a deck)
    const float floatAt = v.water - tune.swimDepth;  // where a swimmer's feet are
    if (grounded) {
        stamina = std::fmin(1.0f, stamina + tune.restRate * dt);
        climb = 0;
        pitch = approach(pitch, 0, 6, dt);
        roll = approach(roll, 0, 6, dt);
        // On foot: the pad pushed up walks, B runs; it turns as it goes (a little wider running).
        // Swimming it paddles at about its walk, a little faster with B.
        const float want = clampf(in.pitch, 0, 1) *
                           (swimming ? walkSpeed * (in.dive ? 2.0f : 1.2f) : (in.dive ? runSpeed : walkSpeed));
        speed = approach(speed, want, want > speed ? 3.0f : 5.0f, dt);
        if (want == 0 && speed < 0.05f) speed = 0;
        const float turn = tune.groundTurn * (speed > walkSpeed * 1.5f ? 0.7f : 1.0f);
        heading = wrap(heading - in.steer * turn * dt);  // right: towards the screen's right
        swimming = !onIsland && ground < v.water - tune.wadeDepth;
        pos.z = swimming ? std::fmax(ground, floatAt) : ground;
        if (speed > 0) {
            const Vec3 next = pos + forward() * (speed * dt);
            const float g = v.groundAt(next.x, next.y, pos.z), margin = 30.0f;
            const bool outside = next.x < v.x0 + margin || next.x > v.x0 + v.size() - margin ||
                                 next.y < v.y0 + margin || next.y > v.y0 + v.size() - margin;
            const bool deep = g < v.water - tune.wadeDepth;
            const bool steep = !deep && g > ground + 0.02f && v.islandAt(next.x, next.y, pos.z) < 0 &&
                               v.normalAt(next.x, next.y).z < tune.steepest;
            if (outside || steep) {
                speed = 0;  // a cliff face, the valley's edge: it stops
            } else if (deep) {  // in, or on, the water: it swims
                if (!swimming) splashed = true;
                pos = next;
                pos.z = std::fmax(g, floatAt);  // floating, never below the bed
                swimming = true;
            } else if (!swimming && (g < ground - tune.drop ||
                                     (g < ground - 0.25f && v.normalAt(next.x, next.y).z < tune.steepest - 0.1f))) {
                // (over the edge, or down a slope too steep to walk: gliding, D132)
                pos = next;  // over the edge: gliding down
                grounded = false;
                speed = std::fmax(speed, 6.0f);
                sinceFlap = 9;
                return;
            } else {
                pos = next;  // walking on, or out of the water onto the shore
                pos.z = g;
                swimming = false;
            }
        }
        if (in.flap && stamina > 0.1f) {  // up: a strong first wingbeat
            if (swimming) splashed = true;
            swimming = false;
            grounded = false;
            tookOff = flapped = true;
            climb = 7.5f;
            speed = std::fmax(6.0f, speed);
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
        if (in.brake) {  // L: slowing, a steeper glide
            targetSpeed = tune.brakeSpeed;
            targetClimb = -tune.sinkRate * 1.5f;
        } else if (in.burst && stamina > 0.0f) {  // R: a burst ahead, level, while it has the breath
            targetSpeed = tune.burstSpeed;
            targetClimb = std::fmax(targetClimb, 0.0f);
            stamina = std::fmax(0.0f, stamina - tune.burstCost * dt);
        }
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
    const bool bursting = in.burst && !in.brake && !in.dive && stamina > 0.0f;
    if ((!in.flap || in.dive) && !bursting) stamina = std::fmin(1.0f, stamina + tune.glideRate * dt);
    speed = approach(speed, targetSpeed, in.dive ? 1.2f : in.brake ? 1.5f : bursting ? 1.0f : 0.6f, dt);
    climb = approach(climb, targetClimb, in.dive ? 2.0f : 1.4f, dt);
    pitch = approach(pitch, clampf(-climb / std::fmax(4.0f, speed) * 1.2f, -0.9f, 0.6f), 5, dt);
    // Move, and keep inside the valley (turned back gently at its edges). Into a slope too steep
    // to skim up (run 19: it shot up mountainsides at its full speed), it keeps its height and
    // slides along the slope's face, slowing, sinking down it as it glides.
    const Vec3 f = forward();
    Vec3 step = f * (speed * dt);
    {
        const Vec3 ahead = pos + step;
        const float there = v.groundAt(ahead.x, ahead.y, pos.z), here = v.groundAt(pos.x, pos.y, pos.z);
        const float run = std::fmax(0.01f, speed * dt);
        // a slope steeper than ~24 degrees rising ahead, and up to where it flies: in the way
        if (there + tune.clearance > pos.z && there - here > run * 0.45f) {
            const Vec3 n = v.normalAt(ahead.x, ahead.y);
            const float h = std::hypot(n.x, n.y);
            if (h > 1e-3f) {
                const Vec3 downhill{n.x / h, n.y / h, 0};  // the slope's face looks this way
                const float into = -(step.x * downhill.x + step.y * downhill.y);
                if (into > 0) step = step + downhill * into;  // what went into the face, gone: along it
            }
            speed *= 1.0f - 1.5f * dt;  // (rubbing along it)
            climb = std::fmin(climb, 0.0f);
        }
    }
    pos = pos + step + Vec3{0, 0, climb * dt};
    const float margin = 30.0f, lo = v.x0 + margin, hiX = v.x0 + v.size() - margin, loY = v.y0 + margin,
                hiY = v.y0 + v.size() - margin;
    if (pos.x < lo || pos.x > hiX || pos.y < loY || pos.y > hiY) {
        pos.x = clampf(pos.x, lo, hiX);
        pos.y = clampf(pos.y, loY, hiY);
        const float home = std::atan2(v.x0 + v.size() * 0.5f - pos.x, -(v.y0 + v.size() * 0.5f - pos.y));
        heading = wrap(heading + clampf(wrap(home - heading), -2.0f * dt, 2.0f * dt));
    }
    // The ground, or the lake's surface over deep water: skim it, or come down when slow (on
    // flat enough ground it lands; onto the water it splashes in and swims).
    const float bed = v.groundAt(pos.x, pos.y, pos.z);
    const bool isle = v.islandAt(pos.x, pos.y, pos.z) >= 0;  // (its flat top: land on it)
    const bool overWater = !isle && bed < v.water - tune.wadeDepth;
    const float g = (overWater ? v.water : bed) + tune.clearance;
    if (pos.z <= g) {
        const Vec3 n = overWater || isle ? Vec3{0, 0, 1} : v.normalAt(pos.x, pos.y);
        if (speed < tune.landSpeed && n.z > 0.8f && !in.flap) {
            grounded = true;
            landed = !overWater;
            splashed = overWater;
            swimming = overWater;
            pos.z = overWater ? std::fmax(bed, floatAt) : g - tune.clearance;
            speed = std::fmin(speed, overWater ? walkSpeed : 0.0f);
            climb = 0;
            return;
        }
        if (n.z < 0.8f) {  // on a steep face: eased down and off it, not pushed up it
            pos = pos + Vec3{n.x, n.y, 0} * (3.0f * dt);
            pos.z = std::fmax(pos.z, v.groundAt(pos.x, pos.y, pos.z) + tune.clearance);
            climb = std::fmax(climb, 0.0f);
        } else {
            pos.z = g;
            climb = std::fmax(climb, 0.5f);  // pulled up off gentle ground
        }
        speed *= 1.0f - 1.2f * dt;       // (ground or water rubbing at it: it slows to land soon)
        skimming = overWater;
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
