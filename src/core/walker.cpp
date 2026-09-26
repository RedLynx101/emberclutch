#include "core/walker.hpp"

#include <algorithm>
#include <cmath>

#include "core/valley.hpp"

namespace ec {
namespace {

constexpr float kPi = 3.14159265f;

float wrap(float a) {
    while (a > kPi) a -= 2 * kPi;
    while (a < -kPi) a += 2 * kPi;
    return a;
}

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Can a body stand at (x, y) coming from `from`? Not in deep water, not up too steep a slope.
bool standable(const Valley& v, Vec2 from, Vec2 to, float wade, float steepest) {
    if (!v.inside(to.x, to.y)) return false;
    const float g = v.heightAt(to.x, to.y);
    if (g < v.water - wade) return false;
    const float g0 = v.heightAt(from.x, from.y);
    return !(g > g0 + 0.02f && v.normalAt(to.x, to.y).z < steepest);
}

// Pushed out of every solid it's inside (a circle each, plus the body's own radius).
Vec2 pushOut(Vec2 p, float radius, const std::vector<Solid>& solids, bool& hit) {
    for (const Solid& s : solids) {
        const float dx = p.x - s.at.x, dy = p.y - s.at.y, d = std::hypot(dx, dy), r = s.radius + radius;
        if (d < r) {
            hit = true;
            if (d < 1e-4f) return {s.at.x + r, s.at.y};
            p = {s.at.x + dx / d * r, s.at.y + dy / d * r};
        }
    }
    return p;
}

// A step from `pos` along `dir` of `step` metres: straight if it can, else sliding along
// whatever's in the way (a wall of water, a slope, a solid). Returns the new position.
Vec2 move(const Valley& v, Vec2 pos, Vec2 dir, float step, float radius, float wade, float steepest,
          const std::vector<Solid>& solids, bool& blocked) {
    Vec2 next{pos.x + dir.x * step, pos.y + dir.y * step};
    bool hit = false;
    next = pushOut(next, radius, solids, hit);
    if (standable(v, pos, next, wade, steepest)) {
        // Against a wall it slides round it; only when that hardly gets anywhere is it blocked
        // (a well in the way is walked round, not stopped at).
        if (hit && std::hypot(next.x - pos.x, next.y - pos.y) < step * 0.3f) blocked = true;
        return next;
    }
    blocked = true;
    for (const Vec2 slide : {Vec2{dir.x, 0}, Vec2{0, dir.y}}) {  // along the obstacle
        Vec2 alt{pos.x + slide.x * step, pos.y + slide.y * step};
        bool h2 = false;
        alt = pushOut(alt, radius, solids, h2);
        if (standable(v, pos, alt, wade, steepest)) return alt;
    }
    return hit && standable(v, pos, next, wade, steepest) ? next : pos;
}

}  // namespace

Vec3 Walker::forward() const { return {std::sin(heading), -std::cos(heading), 0}; }

void Walker::update(const WalkInput& in, float cameraYaw, const Valley& v, const std::vector<Solid>& solids, float dt,
                    const WalkTuning& tune) {
    blocked = false;
    const float push = std::fmin(1.0f, std::hypot(in.x, in.y));
    float want = 0;
    if (push > 0.05f) {
        // The pad's direction, turned to the camera's view: up walks away from the camera.
        const float padAngle = std::atan2(in.x, in.y);  // 0 up, + to the right
        const float goal = wrap(cameraYaw - padAngle);  // (the heading turns the other way: right of the view is minus)
        const float err = wrap(goal - heading);
        heading = wrap(heading + clampf(err, -tune.turnRate * dt, tune.turnRate * dt));
        want = push * (in.run ? tune.runSpeed : tune.walkSpeed) * (std::fabs(err) > 1.6f ? 0.3f : 1.0f);  // a little arc
    }
    const float a = tune.accel * dt;
    speed += clampf(want - speed, -a * 1.5f, a);
    if (speed < 0.01f) {
        speed = 0;
        return;
    }
    const Vec2 dir{std::sin(heading), -std::cos(heading)};
    const Vec2 p = move(v, {pos.x, pos.y}, dir, speed * dt, tune.radius, tune.wade, tune.steepest, solids, blocked);
    if (blocked) speed *= 0.6f;
    pos = {p.x, p.y, std::fmax(v.heightAt(p.x, p.y), v.water - tune.wade)};
}

Vec3 Follower::spot(const Walker& you) const {
    const Vec3 f = you.forward();
    const Vec3 right{-f.y, f.x, 0};
    return you.pos + right * gap - f * (gap * 0.1f);
}

void Follower::update(const Walker& you, const Valley& v, const std::vector<Solid>& solids, float dt) {
    const Vec3 goal = spot(you);
    const float dx = goal.x - pos.x, dy = goal.y - pos.y, dist = std::hypot(dx, dy);
    float want = 0;
    if (dist > 0.4f) {
        const float face = std::atan2(dx, -dy);
        const float err = wrap(face - heading);
        heading = wrap(heading + clampf(err, -5.0f * dt, 5.0f * dt));
        // Keep up: amble when close, trot, then run to catch up (and a little faster than you).
        want = dist < 1.5f ? walk * 0.8f : dist < 4.0f ? trot : std::fmax(run, you.speed * 1.25f);
        if (std::fabs(err) > 1.4f) want *= 0.35f;
    } else if (you.speed < 0.1f) {  // there: face the way you face
        heading = wrap(heading + clampf(wrap(you.heading - heading), -3.0f * dt, 3.0f * dt));
    }
    speed += clampf(want - speed, -18.0f * dt, 12.0f * dt);
    if (speed > 0.01f) {
        bool blocked = false;
        const Vec2 p = move(v, {pos.x, pos.y}, {std::sin(heading), -std::cos(heading)}, speed * dt, gap * 0.3f, 0.8f, 0.6f,
                            solids, blocked);
        pos = {p.x, p.y, std::fmax(v.heightAt(p.x, p.y), v.water - 0.8f)};
    } else {
        speed = 0;
    }
    const float after = std::hypot(goal.x - pos.x, goal.y - pos.y);
    stuckFor = after > 12.0f && after >= dist - 0.05f ? stuckFor + dt : (after > 40.0f ? stuckFor + dt : 0.0f);
}

void Follower::call(const Walker& you, const Valley& v) {
    const Vec3 s = spot(you);
    pos = {s.x, s.y, std::fmax(v.heightAt(s.x, s.y), v.water - 0.8f)};
    heading = you.heading;
    speed = 0;
    stuckFor = 0;
}

void WalkCamera::update(const Walker& you, float turn, const Valley& v, float dt, const std::vector<CameraWall>* walls) {
    yaw = wrap(yaw + turn * 1.8f * dt);
    if (turn == 0 && you.speed > 3.0f) {  // walking away from it: it swings round behind, slowly
        const float err = wrap(you.heading - yaw);
        if (std::fabs(err) < 2.2f) yaw = wrap(yaw + clampf(err, -0.6f * dt, 0.6f * dt));
    }
    const Vec3 look{std::sin(yaw), -std::cos(yaw), 0};
    const Vec3 wantTarget = you.pos + Vec3{0, 0, 1.0f} + look * 2.2f;
    Vec3 wantEye = you.pos - look * (distance * 0.78f) + Vec3{0, 0, distance * 0.62f};
    // Kept in the open: stepping out from you toward where it wants to be, it stops short of any
    // ground that rises into the way (the den's cliff behind you, a hillside).
    const Vec3 from = you.pos + Vec3{0, 0, 1.5f};
    for (int k = 1; k <= 12; ++k) {
        const Vec3 p = from + (wantEye - from) * (k / 12.0f);
        if (v.heightAt(p.x, p.y) + 0.9f > p.z) {
            wantEye = from + (wantEye - from) * ((k - 1) / 12.0f);
            break;
        }
    }
    // ...and short of any tree's leaves in the way (the camera would look out through a trunk).
    const float reach = length(wantEye - from);
    if (reach > 1.0f && !v.tileTrees.empty()) {
        const float ts = v.tileSize();
        const int t = v.tiles();
        const int tx0 = static_cast<int>((std::fmin(from.x, wantEye.x) - 8.0f - v.x0) / ts);
        const int tx1 = static_cast<int>((std::fmax(from.x, wantEye.x) + 8.0f - v.x0) / ts);
        const int ty0 = static_cast<int>((std::fmin(from.y, wantEye.y) - 8.0f - v.y0) / ts);
        const int ty1 = static_cast<int>((std::fmax(from.y, wantEye.y) + 8.0f - v.y0) / ts);
        float keep = 1.0f;  // the share of the way out kept
        for (int ty = std::max(0, ty0); ty <= std::min(t - 1, ty1); ++ty)
            for (int tx = std::max(0, tx0); tx <= std::min(t - 1, tx1); ++tx)
                for (int i : v.tileTrees[std::size_t(ty) * t + tx]) {
                    const ValleyTree& tr = v.trees[i];
                    if (tr.kind != kPropTree && tr.kind != kPropPine && tr.kind != kPropFruit) continue;
                    const float h = tr.height;
                    const float r = h * (tr.kind == kPropPine ? 0.3f : 0.4f) + 0.4f;
                    const float cz = v.heightAt(tr.x, tr.y) + h * (tr.kind == kPropFruit ? 0.62f : 0.5f);
                    const float rz = h * 0.4f + 0.4f;
                    for (int k = 2; k <= 12; ++k) {
                        const float f = k / 12.0f;
                        if (f >= keep) break;
                        const Vec3 p = from + (wantEye - from) * f;
                        if (length(p - from) < 1.5f) continue;  // under its own leaves: you stand there
                        const float dx = (p.x - tr.x) / r, dy = (p.y - tr.y) / r, dz = (p.z - cz) / rz;
                        if (dx * dx + dy * dy + dz * dz < 1.0f) {
                            keep = (k - 1) / 12.0f;
                            break;
                        }
                    }
                }
        if (keep < 1.0f) wantEye = from + (wantEye - from) * keep;
    }
    // ...and in front of the walls (the den's arch in its cliff).
    if (walls)
        for (const CameraWall& w : *walls) {
            const Vec2 e{wantEye.x - w.at.x, wantEye.y - w.at.y}, y{from.x - w.at.x, from.y - w.at.y};
            const float de = e.x * w.normal.x + e.y * w.normal.y, dy = y.x * w.normal.x + y.y * w.normal.y;
            const float side = e.x * w.normal.y - e.y * w.normal.x;
            if (de >= 0.8f || dy <= 0.8f || std::fabs(side) > w.halfWidth) continue;
            const float f = (dy - 0.8f) / (dy - de);  // where the way out meets the wall (less a margin)
            const Vec3 hit = from + (wantEye - from) * f;
            if (hit.z - v.heightAt(hit.x, hit.y) > w.top) continue;  // over the top of it
            wantEye = hit;
        }
    const float ground = v.heightAt(wantEye.x, wantEye.y) + 1.2f;
    if (wantEye.z < ground) wantEye.z = ground;
    if (!set) {
        eye = wantEye;
        target = wantTarget;
        set = true;
        return;
    }
    const float k = std::fmin(1.0f, dt * 6.0f);
    eye = eye + (wantEye - eye) * k;
    target = target + (wantTarget - target) * std::fmin(1.0f, dt * 10.0f);
}

}  // namespace ec
