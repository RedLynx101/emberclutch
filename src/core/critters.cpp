#include "core/critters.hpp"

#include <cmath>

#include "core/dragon.hpp"
#include "core/save.hpp"
#include "core/valley.hpp"

namespace ec::critters {
namespace {

constexpr float kPi = 3.14159265f, kTau = 6.2831853f;
constexpr float kCellChance = 0.4f;  // a cell's chance of holding a group (where the ground suits one)
constexpr u32 kSoarCell = 0xFFFFFFFFu;  // the soaring pair's (never a spawn cell)

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
float clamp100(float v) { return clampf(v, 0.0f, 100.0f); }
float smooth(float e0, float e1, float x) {
    const float t = clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3 - 2 * t);
}

u32 hash3(u32 a, u32 b, u32 c) {
    u32 h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u ^ (c + 0x165667B1u) * 0xC2B2AE3Du;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    h *= 0x297A2D39u;
    h ^= h >> 15;
    return h;
}
float unit(u32 h) { return (h & 0xFFFFFF) / 16777216.0f; }

u32 nextRand(u32& s) {  // xorshift32
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}
float rnd(u32& s) { return (nextRand(s) & 0xFFFFFF) / 16777216.0f; }
float rnd(u32& s, float lo, float hi) { return lo + (hi - lo) * rnd(s); }

float flat(Vec3 a, Vec3 b) { return std::hypot(a.x - b.x, a.y - b.y); }
float headingTo(Vec3 from, Vec3 to) { return std::atan2(to.x - from.x, -(to.y - from.y)); }
Vec3 forwardOf(float h) { return {std::sin(h), -std::cos(h), 0}; }
float turn(float h, float want, float step) {
    const float e = std::remainder(want - h, kTau);
    return h + clampf(e, -step, step);
}

u32 cellKey(int cx, int cy) { return (static_cast<u32>(cx + 32768) << 16) | (static_cast<u32>(cy + 32768) & 0xFFFFu); }

bool landKind(Kind k) { return k != Kind::Duck && k != Kind::Frog; }

// The nearest prop of these kinds (a bit each, ValleyPropKind) within r of (x, y).
bool propNear(const Valley& v, float x, float y, float r, u32 kinds, Vec2* out = nullptr) {
    const int t = v.tiles();
    if (t <= 0) return false;
    const float ts = v.tileSize();
    const int tx0 = static_cast<int>(std::floor((x - r - v.x0) / ts)), tx1 = static_cast<int>(std::floor((x + r - v.x0) / ts));
    const int ty0 = static_cast<int>(std::floor((y - r - v.y0) / ts)), ty1 = static_cast<int>(std::floor((y + r - v.y0) / ts));
    float best = r;
    bool found = false;
    for (int ty = ty0; ty <= ty1; ++ty)
        for (int tx = tx0; tx <= tx1; ++tx) {
            if (tx < 0 || ty < 0 || tx >= t || ty >= t) continue;
            for (int k : v.tileTrees[std::size_t(ty) * t + tx]) {
                const ValleyTree& p = v.trees[std::size_t(k)];
                if (!(kinds & (1u << p.kind))) continue;
                const float d = std::hypot(p.x - x, p.y - y);
                if (d < best) {
                    best = d;
                    found = true;
                    if (out) *out = {p.x, p.y};
                }
            }
        }
    return found;
}

constexpr u32 kCoverKinds = (1u << kPropBush) | (1u << kPropTree) | (1u << kPropPine) | (1u << kPropFruit);
constexpr u32 kTreeKinds = (1u << kPropTree) | (1u << kPropPine) | (1u << kPropFruit);

bool wetAt(const Valley& v, float x, float y, float below = 0.15f) { return v.heightAt(x, y) < v.water - below; }

// A point of water from (x, y) (the nearest along eight ways, out to `reach`), deeper than `depth`.
bool waterNear(const Valley& v, Vec3 at, float reach, float depth, Vec3& out) {
    for (float r = 0.75f; r <= reach; r += 0.75f)
        for (int k = 0; k < 8; ++k) {
            const float a = k * (kTau / 8);
            const float x = at.x + std::cos(a) * r, y = at.y + std::sin(a) * r;
            if (v.heightAt(x, y) < v.water - depth) {
                out = {x, y, v.water};
                return true;
            }
        }
    return false;
}

void emit(Life& life, Ev ev, Kind kind, Vec3 at, float pitch = 1.0f) {
    if (life.eventCount >= kMaxEvents) return;
    life.events[life.eventCount++] = {ev, kind, at, pitch};
}

// ------------------------------------------------------------------------------- spawning
Critter* freeSlot(Life& life) {
    for (Critter& c : life.c)
        if (!c.alive) return &c;
    return nullptr;
}

float surfaceFor(const Valley& v, Kind k, float x, float y) {
    return k == Kind::Duck ? v.water : std::fmax(v.heightAt(x, y), k == Kind::Frog ? v.water - 0.05f : -1e9f);
}

bool suits(const Valley& v, Kind k, float x, float y) {
    if (!v.inside(x, y)) return false;
    const float h = v.heightAt(x, y);
    if (k == Kind::Duck) return h < v.water - 0.5f;
    if (k == Kind::Frog) return h > v.water - 0.1f && h < v.water + 1.4f;
    if (h < v.water + 0.2f) return false;
    return v.normalAt(x, y).z > 0.84f;
}

int spawnGroup(Life& life, const Valley& v, Kind k, Vec3 spot, int size, u32 cell, u32 h) {
    int first = -1;
    u32 r = h | 1u;
    const u8 variant = static_cast<u8>(nextRand(r) % 4);
    const float face = rnd(r) * kTau;
    for (int i = 0; i < size; ++i) {
        Critter* c = freeSlot(life);
        if (!c) break;
        *c = Critter{};
        c->alive = true;
        c->kind = k;
        c->slot = static_cast<u8>(i);
        c->variant = variant;
        c->cell = cell;
        c->rng = hash3(h, static_cast<u32>(i), 0xB1Du) | 1u;
        c->anim = rnd(c->rng) * 10.0f;
        c->timer = rnd(c->rng, 0.2f, 2.0f);
        c->heading = face + rnd(c->rng, -0.8f, 0.8f);
        Vec3 at = spot;
        const float spread = k == Kind::Songbird ? 2.2f : k == Kind::Butterfly ? 1.6f : k == Kind::Frog ? 1.8f : 2.5f;
        if (k == Kind::Duck) {  // the ducklings in a line behind their mother
            at = spot - forwardOf(c->heading = face) * (0.2f + 0.55f * i);
        } else if (i > 0) {
            const float a = rnd(c->rng) * kTau, d = rnd(c->rng, 0.5f, spread);
            at = {spot.x + std::cos(a) * d, spot.y + std::sin(a) * d, 0};
        }
        if (!suits(v, k, at.x, at.y)) at = spot;
        at.z = surfaceFor(v, k, at.x, at.y);
        c->pos = c->home = c->goal = c->from = at;
        if (k == Kind::Butterfly) c->air = 0.6f + 0.4f * rnd(c->rng);
        if (k == Kind::Frog) {  // facing the water
            Vec3 w;
            if (waterNear(v, at, 4.0f, 0.3f, w)) {
                c->heading = headingTo(at, w);
                c->goal = w;
            }
        }
        if (first < 0) first = static_cast<int>(c - life.c);
    }
    return first;
}

bool cellEmptied(const Life& life, u32 key) {
    for (int k = 0; k < life.emptiedCount; ++k)
        if (life.emptied[k] == key) return true;
    return false;
}

void markEmptied(Life& life, u32 key) {
    if (key == kSoarCell || cellEmptied(life, key)) return;
    if (life.emptiedCount < 16) {
        life.emptied[life.emptiedCount++] = key;
    } else {  // (full: the oldest goes)
        for (int k = 1; k < 16; ++k) life.emptied[k - 1] = life.emptied[k];
        life.emptied[15] = key;
    }
}

bool cellLive(const Life& life, u32 key) {
    for (const Critter& c : life.c)
        if (c.alive && c.cell == key) return true;
    return false;
}

void fill(Life& life, const Valley& v, const Around& a) {
    const int cx0 = static_cast<int>(std::floor((a.you.x - kLiveRadius) / kCell));
    const int cx1 = static_cast<int>(std::floor((a.you.x + kLiveRadius) / kCell));
    const int cy0 = static_cast<int>(std::floor((a.you.y - kLiveRadius) / kCell));
    const int cy1 = static_cast<int>(std::floor((a.you.y + kLiveRadius) / kCell));
    for (int cy = cy0; cy <= cy1; ++cy)
        for (int cx = cx0; cx <= cx1; ++cx) {
            const float mx = (cx + 0.5f) * kCell, my = (cy + 0.5f) * kCell;
            if (std::hypot(mx - a.you.x, my - a.you.y) > kLiveRadius) continue;
            const u32 key = cellKey(cx, cy);
            if (cellEmptied(life, key) || cellLive(life, key)) continue;
            Vec3 spot;
            int size = 0;
            const Kind k = cellKind(v, life.seed, cx, cy, a.day, a.dusk, a.night, spot, size);
            if (k == Kind::Count) continue;
            if (spawnGroup(life, v, k, spot, size, key, hash3(life.seed, key, 77u)) < 0) return;  // (full)
        }
}

// ------------------------------------------------------------------------------- behaviour
struct Radii {
    float calm, run;  // walking within calm, or running within run, sends it off
};
Radii radiiOf(Kind k) {
    switch (k) {
        case Kind::Songbird: return {3.6f, 8.0f};
        case Kind::Rabbit:
        case Kind::SnowHare: return {3.2f, 8.5f};
        case Kind::Frog: return {2.4f, 5.0f};
        case Kind::Fox: return {5.0f, 11.0f};
        default: return {0.0f, 0.0f};
    }
}

// Whether you or your dragon send it off (and from where).
bool threatened(const Valley& v, const Critter& c, const Around& a, Vec3& from) {
    const Radii r = radiiOf(c.kind);
    if (c.kind == Kind::Duck) {  // ducks mind only a dragon in the water beside them (or skimming over it)
        const Vec3 d = a.riding || a.hasPal ? a.pal : a.you;
        const bool wet = wetAt(v, d.x, d.y, 0.2f) && d.z < v.water + (a.riding ? 4.0f : 0.6f);
        if (wet && flat(d, c.pos) < (a.riding ? 9.0f : 6.0f)) {
            from = d;
            return true;
        }
        return false;
    }
    if (r.calm <= 0) return false;
    const float ground = v.heightAt(c.pos.x, c.pos.y);
    struct Mover {
        Vec3 at;
        float speed, size;
    };
    Mover m[2];
    int n = 0;
    if (!a.riding) m[n++] = {a.you, a.youSpeed, 1.0f};
    if (a.hasPal || a.riding) m[n++] = {a.pal, a.palSpeed, a.riding ? 1.7f : 1.3f};
    for (int i = 0; i < n; ++i) {
        if (m[i].at.z - ground > 9.0f) continue;  // flying high over it
        const float d = flat(m[i].at, c.pos);
        if (d < 1.1f * m[i].size || (d < r.calm * m[i].size && m[i].speed > 0.9f) ||
            (d < r.run * m[i].size && m[i].speed > 3.4f)) {
            from = m[i].at;
            return true;
        }
    }
    return false;
}

// Hops (or walks) toward goal: true on arrival. A hop is `len` metres over `time` seconds, a
// `pause` between; `lift` its height.
bool hopToward(Critter& c, const Valley& v, Vec3 goal, float len, float time, float pause, float lift, float dt) {
    if (c.air <= 0.0f && c.timer > 0.0f) {  // resting between hops
        c.timer -= dt;
        return false;
    }
    if (c.air <= 0.0f && c.speed <= 0.0f) {  // the next hop
        const float d = flat(c.pos, goal);
        if (d < 0.08f) return true;
        c.from = c.pos;
        c.heading = headingTo(c.pos, goal);
        c.speed = std::fmin(len, d) / time;
        c.puff = 0;  // (the hop's clock)
    }
    c.puff += dt / time;
    const Vec3 f = forwardOf(c.heading);
    c.pos.x += f.x * c.speed * dt;
    c.pos.y += f.y * c.speed * dt;
    c.air = c.puff < 1.0f ? lift * std::sin(kPi * c.puff) : 0.0f;
    c.pos.z = std::fmax(v.heightAt(c.pos.x, c.pos.y), v.water - 0.05f);
    if (c.puff >= 1.0f) {
        c.speed = 0;
        c.puff = 0;
        c.air = 0;
        c.timer = pause;
        if (flat(c.pos, goal) < 0.12f) return true;
    }
    return false;
}

// Walks toward goal at `speed`, turning at most `turnRate` rad/s: true on arrival.
bool walkToward(Critter& c, const Valley& v, Vec3 goal, float speed, float turnRate, float dt) {
    const float d = flat(c.pos, goal);
    if (d < 0.15f) {
        c.speed = 0;
        return true;
    }
    c.heading = turn(c.heading, headingTo(c.pos, goal), turnRate * dt);
    c.speed = std::fmin(speed, d * 3.0f);
    const Vec3 f = forwardOf(c.heading);
    const float nx = c.pos.x + f.x * c.speed * dt, ny = c.pos.y + f.y * c.speed * dt;
    if (!landKind(c.kind) || !wetAt(v, nx, ny, 0.1f)) {
        c.pos.x = nx;
        c.pos.y = ny;
    }
    c.pos.z = v.heightAt(c.pos.x, c.pos.y);
    return false;
}

// Somewhere near home it can go (tries a few; else stays).
Vec3 wanderGoal(Critter& c, const Valley& v, float radius) {
    for (int k = 0; k < 5; ++k) {
        const float a = rnd(c.rng) * kTau, d = rnd(c.rng, 0.3f, radius);
        const float x = c.home.x + std::cos(a) * d, y = c.home.y + std::sin(a) * d;
        if (suits(v, c.kind, x, y)) return {x, y, surfaceFor(v, c.kind, x, y)};
    }
    return c.pos;
}

// Where it runs to: cover away from the threat (a bush, a tree), else open ground that way.
Vec3 escapeGoal(Critter& c, const Valley& v, Vec3 from, float reach) {
    const float away = headingTo(from, c.pos);
    Vec2 cover;
    if (c.kind != Kind::Songbird && propNear(v, c.pos.x + std::sin(away) * reach * 0.5f, c.pos.y - std::cos(away) * reach * 0.5f,
                                            reach * 0.55f, c.kind == Kind::Fox ? kTreeKinds : kCoverKinds, &cover))
        return {cover.x, cover.y, v.heightAt(cover.x, cover.y)};
    for (int k = 0; k < 7; ++k) {  // straight away, then a little either side
        const float h = away + (k == 0 ? 0.0f : ((k & 1) ? 1.0f : -1.0f) * 0.45f * ((k + 1) / 2));
        const float x = c.pos.x + std::sin(h) * reach, y = c.pos.y - std::cos(h) * reach;
        if (suits(v, c.kind, x, y)) return {x, y, v.heightAt(x, y)};
    }
    return {c.pos.x + std::sin(away) * reach, c.pos.y - std::cos(away) * reach, c.pos.z};
}

void startFlee(Life& life, const Valley& v, Critter& c, Vec3 from, bool calm = false) {
    c.state = State::Flee;
    c.t = 0;
    c.from = c.pos;
    c.speed = 0;
    c.air = c.kind == Kind::Songbird ? c.air : 0.0f;
    c.zig = (nextRand(c.rng) & 1) ? 1.0f : -1.0f;
    switch (c.kind) {
        case Kind::Songbird:
            c.goal = from;  // (flies away from it)
            break;
        case Kind::Rabbit:
        case Kind::SnowHare:
            c.goal = escapeGoal(c, v, from, 20.0f);
            emit(life, Ev::Hop, c.kind, c.pos);
            break;
        case Kind::Fox:
            c.goal = escapeGoal(c, v, from, 24.0f);
            if (!calm) emit(life, Ev::Rustle, c.kind, c.pos);
            break;
        case Kind::Duck: {
            c.goal = c.pos;
            const float away = headingTo(from, c.pos);
            for (int k = 0; k < 5; ++k) {
                const float h = away + (k - 2) * 0.4f;
                const float x = c.pos.x + std::sin(h) * 12.0f, y = c.pos.y - std::cos(h) * 12.0f;
                if (suits(v, Kind::Duck, x, y)) {
                    c.goal = {x, y, v.water};
                    break;
                }
            }
            emit(life, Ev::Quack, c.kind, c.pos, 1.1f);
            break;
        }
        default: break;
    }
}

// A flock or family goes together: the others follow a moment later.
void alarmGroup(Life& life, const Valley& v, const Critter& first, Vec3 from) {
    bool flutter = false;
    for (Critter& c : life.c) {
        if (!c.alive || c.cell != first.cell || c.kind != first.kind) continue;
        if (c.state == State::Flee || c.state == State::Gone || c.state == State::Leap || c.state == State::Chased) continue;
        if (c.kind == Kind::Frog && &c != &first) continue;  // (frogs go one by one: the one scared leaps)
        if (c.kind == Kind::Songbird) flutter = true;
        startFlee(life, v, c, from);
        c.t = -0.07f * c.slot;  // (not all at once)
    }
    if (flutter) emit(life, Ev::Flutter, first.kind, first.pos);
}

// The critter a moment is about keeps to the moment (the chase's rabbit waits to be pounced at).
bool inMoment(const Life& life, const Critter& c) { return life.moment.active() && &life.c[life.moment.who] == &c; }

void startLeap(Critter& c, const Valley& v) {
    c.state = State::Leap;
    c.t = 0;
    c.from = c.pos;
    Vec3 w;
    if (waterNear(v, c.pos, 4.0f, 0.3f, w)) c.goal = w;
    else c.goal = c.pos + forwardOf(c.heading) * 2.0f;
    c.heading = headingTo(c.pos, c.goal);
}

void stepSongbird(Life& life, const Valley& v, Critter& c, const Around& a, float dt) {
    Vec3 from;
    const bool calm = (c.state == State::Idle || c.state == State::Move) && !inMoment(life, c);
    if (calm && c.born > 0.6f && threatened(v, c, a, from)) {
        alarmGroup(life, v, c, from);
        return;
    }
    switch (c.state) {
        case State::Idle:
            if ((c.timer -= dt) <= 0) {  // a hop or two about
                Vec3 g = c.pos + forwardOf(rnd(c.rng) * kTau) * rnd(c.rng, 0.3f, 0.9f);
                if (flat(g, c.home) > 3.2f) g = c.pos + (c.home - c.pos) * 0.3f;
                if (suits(v, c.kind, g.x, g.y)) {
                    c.goal = g;
                    c.state = State::Move;
                    c.timer = 0;
                } else {
                    c.timer = rnd(c.rng, 0.4f, 1.5f);
                }
            }
            if (rnd(c.rng) < dt * 0.1f && flat(c.pos, a.you) < 22.0f) emit(life, Ev::Chirp, c.kind, c.pos, rnd(c.rng, 0.9f, 1.25f));
            break;
        case State::Move:
            if (hopToward(c, v, c.goal, 0.45f, 0.2f, 0.12f, 0.1f, dt)) {
                c.state = State::Idle;
                c.timer = rnd(c.rng, 0.5f, 2.2f);
            }
            break;
        case State::Come:
            if (a.youSpeed > 2.6f) {  // you ran at it after all
                alarmGroup(life, v, c, a.you);
                return;
            }
            if (hopToward(c, v, c.goal, 0.45f, 0.24f, 0.1f, 0.12f, dt)) {
                c.state = State::Visit;
                c.t = 0;
            }
            break;
        case State::Visit:
            c.heading = turn(c.heading, headingTo(c.pos, a.you), 6.0f * dt);
            if (c.t > 0.25f && c.t - dt <= 0.25f) emit(life, Ev::Chirp, c.kind, c.pos, 1.2f);
            if (c.t > 1.3f && c.t - dt <= 1.3f) emit(life, Ev::Chirp, c.kind, c.pos, 1.35f);
            if (a.youSpeed > 2.6f) {
                alarmGroup(life, v, c, a.you);
                return;
            }
            if (c.t > 3.2f) {  // back to its flock
                c.state = State::Move;
                c.goal = c.home;
                c.timer = 0.2f;
            }
            break;
        case State::Flee: {
            if (c.t < 0) break;  // (a moment after the first)
            const float away = headingTo(c.goal, c.from);
            if (c.speed <= 0) {
                c.heading = away + rnd(c.rng, -0.6f, 0.6f);
                c.speed = 3.0f;
            }
            c.speed = std::fmin(9.0f, c.speed + 7.0f * dt);
            const Vec3 f = forwardOf(c.heading);
            c.pos.x += f.x * c.speed * dt;
            c.pos.y += f.y * c.speed * dt;
            if (c.air < 14.0f) c.air = std::fmin(14.0f, c.air + (c.t < 1.2f ? 5.0f : 2.0f) * dt);
            c.pos.z = v.heightAt(c.pos.x, c.pos.y) + c.air;
            if (c.t > 6.0f || flat(c.pos, c.from) > 42.0f) {
                c.state = State::Gone;
                c.t = 0;
            }
            break;
        }
        case State::Soar: {
            // High over you, in wide circles; flying near them on your dragon sends them off.
            if (flat(c.home, a.you) > 20.0f) {
                const Vec3 to = a.you - c.home;
                const float d = flat(c.home, a.you);
                c.home.x += to.x / d * 4.0f * dt;
                c.home.y += to.y / d * 4.0f * dt;
            }
            const float r = 13.0f + 3.0f * c.slot, w = 0.32f - 0.05f * c.slot;
            const float ang = c.anim + life.clock * w;
            c.pos = {c.home.x + std::cos(ang) * r, c.home.y + std::sin(ang) * r, 0};
            const float ground = std::fmax(v.heightAt(c.pos.x, c.pos.y), v.water);
            c.pos.z = std::fmax(ground + 16.0f, v.heightAt(c.home.x, c.home.y) + 20.0f + 3.0f * c.slot) + 1.5f * std::sin(life.clock * 0.4f + c.anim);
            c.heading = std::atan2(-std::sin(ang), -std::cos(ang));  // along the circle
            const Vec3 d = a.pal - c.pos;
            if ((a.riding && length(d) < 12.0f) || a.day < 0.35f) {
                c.state = State::Flee;
                c.t = 0;
                c.from = c.pos;
                c.goal = a.riding ? a.pal : c.home;
                c.heading = headingTo(c.goal, c.pos);
                c.speed = 6.0f;
                c.air = c.pos.z - v.heightAt(c.pos.x, c.pos.y);
                emit(life, Ev::Flutter, c.kind, c.pos, 1.2f);
            }
            break;
        }
        default: break;
    }
}

void stepRabbit(Life& life, const Valley& v, Critter& c, const Around& a, float dt) {
    Vec3 from;
    const bool calm = (c.state == State::Idle || c.state == State::Move) && !inMoment(life, c);
    if (calm && c.born > 0.6f && threatened(v, c, a, from)) {
        alarmGroup(life, v, c, from);
        return;
    }
    switch (c.state) {
        case State::Idle:
            if (c.timer < 0.5f || c.puff < 0.99f) c.puff = std::fmax(0.0f, c.puff - dt * 2.5f);  // (sat up a while, then down)
            if ((c.timer -= dt) <= 0) {
                if (rnd(c.rng) < 0.45f) {  // sits up, ears high, and looks about
                    c.puff = 1.0f;
                    c.timer = rnd(c.rng, 1.4f, 2.6f);
                } else {
                    c.goal = wanderGoal(c, v, 4.5f);
                    c.state = State::Move;
                    c.timer = 0;
                    c.puff = 0;
                }
            }
            break;
        case State::Move:
            if (hopToward(c, v, c.goal, 0.6f, 0.28f, 0.14f, 0.18f, dt)) {
                c.state = State::Idle;
                c.timer = rnd(c.rng, 1.2f, 3.5f);
                c.puff = 0;
            }
            break;
        case State::Flee:
        case State::Chased: {
            if (c.t < 0) break;
            // Zigzag hops for its cover, quick as anything; in it (or far enough), it's gone.
            const float speed = c.state == State::Chased ? c.speed : 6.5f;
            if (std::fmod(c.t, 0.34f) < dt) c.zig = -c.zig;
            const float want = headingTo(c.pos, c.goal) + c.zig * (c.state == State::Chased ? 0.7f : 0.5f);
            c.heading = turn(c.heading, want, 14.0f * dt);
            const Vec3 f = forwardOf(c.heading);
            const float nx = c.pos.x + f.x * speed * dt, ny = c.pos.y + f.y * speed * dt;
            if (!wetAt(v, nx, ny, 0.1f)) {
                c.pos.x = nx;
                c.pos.y = ny;
            } else {
                c.zig = -c.zig;
            }
            c.pos.z = v.heightAt(c.pos.x, c.pos.y);
            c.air = 0.2f * std::fabs(std::sin(c.t * kPi / 0.3f));
            if (flat(c.pos, c.goal) < 0.8f || c.t > 6.0f) {
                c.state = State::Gone;
                c.t = 0;
                c.air = 0;
                emit(life, Ev::Rustle, c.kind, c.pos);
            }
            break;
        }
        default: break;
    }
}

void stepButterfly(Life& life, const Valley& v, Critter& c, const Around& a, float dt) {
    const float ground = std::fmax(v.heightAt(c.pos.x, c.pos.y), v.water);
    if (a.day < 0.3f && c.state == State::Idle && !inMoment(life, c) && flat(c.pos, a.you) > 12.0f) {  // off to bed at dusk
        c.state = State::Gone;
        c.t = 0;
        return;
    }
    switch (c.state) {
        case State::Idle: {
            // Fluttering over its flowers in loose loops, now and then settling on one.
            const float k = life.clock * 0.45f + c.anim;
            Vec3 want{c.home.x + std::cos(k * 1.3f) * 1.8f, c.home.y + std::sin(k * 0.9f + c.slot) * 1.6f, 0};
            const float yd = flat(c.pos, a.you);
            if (yd < 1.2f) want = c.pos + (c.pos - a.you) * 1.5f;  // (drifts away from you, never far)
            const Vec3 to = want - c.pos;
            const float d = std::hypot(to.x, to.y);
            if (d > 0.01f) {
                const float s = std::fmin(1.3f, d * 1.5f);
                c.pos.x += to.x / d * s * dt;
                c.pos.y += to.y / d * s * dt;
                c.heading = turn(c.heading, std::atan2(to.x, -to.y), 5.0f * dt);
            }
            c.air += ((0.55f + 0.45f * std::sin(life.clock * 1.7f + c.anim)) - c.air) * std::fmin(1.0f, dt * 2.0f);
            c.pos.z = ground + c.air + 0.08f * std::sin(life.clock * 9.0f + c.anim);
            if ((c.timer -= dt) <= 0) {
                c.timer = rnd(c.rng, 4.0f, 9.0f);
                if (rnd(c.rng) < 0.5f) {
                    c.state = State::Move;  // resting on a flower
                    c.t = 0;
                    c.goal = c.pos;
                    c.puff = rnd(c.rng, 2.0f, 4.0f);  // (how long)
                }
            }
            break;
        }
        case State::Move:  // settled: wings slowly opening and closing
            c.air += (0.28f - c.air) * std::fmin(1.0f, dt * 3.0f);
            c.pos.z = ground + c.air;
            if (c.t > c.puff || flat(c.pos, a.you) < 1.0f) {
                c.state = State::Idle;
                c.t = 0;
            }
            break;
        case State::Come: {  // to the perch (the moment sets goal)
            const Vec3 to = c.goal - c.pos;
            const float d = length(to);
            if (d < 0.06f) {
                c.state = State::Perch;
                c.t = 0;
                c.pos = c.goal;
                emit(life, Ev::Shimmer, c.kind, c.pos);
                break;
            }
            const float s = std::fmin(2.4f, d * 2.5f + 0.4f);
            c.pos = c.pos + to * (std::fmin(d, s * dt) / d);
            if (d > 0.3f) c.pos.z += 0.25f * dt * std::sin(life.clock * 10.0f + c.anim);
            if (std::hypot(to.x, to.y) > 0.02f) c.heading = turn(c.heading, std::atan2(to.x, -to.y), 6.0f * dt);
            break;
        }
        case State::Perch:
            c.pos = c.goal;  // (the moment keeps goal on the head)
            break;
        default: break;
    }
}

void stepFrog(Life& life, const Valley& v, Critter& c, const Around& a, float dt) {
    Vec3 from;
    if (c.state == State::Idle && !inMoment(life, c) && c.born > 0.6f && threatened(v, c, a, from)) {
        startLeap(c, v);
        return;
    }
    switch (c.state) {
        case State::Idle: {
            // A croak now and then (more often at night): the throat puffs out.
            const float croakFor = 0.5f;
            if (c.puff > 0) {
                c.puff += dt / croakFor;
                if (c.puff >= 1.0f) c.puff = 0;
            }
            if ((c.timer -= dt) <= 0) {
                c.timer = rnd(c.rng, 3.0f, 8.0f) * (a.night > 0.5f ? 0.5f : 1.0f);
                c.puff = 0.001f;
                if (flat(c.pos, a.you) < 24.0f) emit(life, Ev::Croak, c.kind, c.pos, rnd(c.rng, 0.92f, 1.1f));
            }
            break;
        }
        case State::Visit: {  // croaking back at you, twice, then a happy leap
            c.heading = turn(c.heading, headingTo(c.pos, a.you), 5.0f * dt);
            for (float at : {0.7f, 1.4f})
                if (c.t >= at && c.t - dt < at) {
                    c.puff = 0.001f;
                    emit(life, Ev::Croak, c.kind, c.pos, at < 1.0f ? 1.0f : 1.12f);
                }
            if (c.puff > 0) {
                c.puff += dt / 0.45f;
                if (c.puff >= 1.0f) c.puff = 0;
            }
            if (c.t > 2.3f) startLeap(c, v);
            break;
        }
        case State::Leap: {
            constexpr float kLeap = 0.55f;
            const float u = clampf(c.t / kLeap, 0.0f, 1.0f);
            c.pos.x = c.from.x + (c.goal.x - c.from.x) * u;
            c.pos.y = c.from.y + (c.goal.y - c.from.y) * u;
            const float base = c.from.z + (v.water - 0.05f - c.from.z) * u;
            c.air = 0.65f * std::sin(kPi * u);
            c.pos.z = base;
            if (u >= 1.0f) {
                c.state = State::Gone;
                c.t = 0;
                c.air = 0;
                c.goal = {c.pos.x, c.pos.y, v.water};
                emit(life, Ev::Splash, c.kind, c.pos);
            }
            break;
        }
        default: break;
    }
}

void stepDuck(Life& life, const Valley& v, Critter& c, const Around& a, float dt) {
    Vec3 from;
    const bool calm = (c.state == State::Idle || c.state == State::Move) && !inMoment(life, c);
    if (c.slot == 0 && calm && c.born > 0.6f && threatened(v, c, a, from)) {
        startFlee(life, v, c, from);
        return;
    }
    auto paddle = [&](Vec3 goal, float speed, float turnRate) {
        const float d = flat(c.pos, goal);
        if (d > 0.05f) c.heading = turn(c.heading, headingTo(c.pos, goal), turnRate * dt);
        c.speed += (std::fmin(speed, d * 1.6f) - c.speed) * std::fmin(1.0f, dt * 3.0f);
        const Vec3 f = forwardOf(c.heading);
        const float nx = c.pos.x + f.x * c.speed * dt, ny = c.pos.y + f.y * c.speed * dt;
        if (v.heightAt(nx, ny) < v.water - 0.3f) {  // (never onto the land)
            c.pos.x = nx;
            c.pos.y = ny;
        } else {
            c.speed *= 0.5f;
        }
        c.pos.z = v.water;
        return d < 0.3f;
    };
    if (c.slot > 0) {  // a duckling: behind the one before it
        const Critter* lead = nullptr;
        for (const Critter& o : life.c)
            if (o.alive && o.cell == c.cell && o.kind == Kind::Duck && o.slot == c.slot - 1 && o.state != State::Gone) lead = &o;
        if (!lead) {  // (its mother's gone: it keeps to itself)
            paddle(c.home, 0.4f, 1.5f);
            return;
        }
        const Vec3 want = lead->pos - forwardOf(lead->heading) * (c.slot == 1 ? 0.78f : 0.52f);
        const bool hurry = lead->state == State::Flee || lead->state == State::Come;
        paddle(want, hurry ? 3.0f : 1.4f, 4.0f);
        c.state = lead->state == State::Visit ? State::Visit : State::Idle;
        if (lead->state == State::Visit && rnd(c.rng) < dt * 0.6f) emit(life, Ev::Quack, c.kind, c.pos, 1.7f + 0.1f * c.slot);
        return;
    }
    switch (c.state) {
        case State::Idle:
        case State::Move:
            if ((c.timer -= dt) <= 0 || flat(c.pos, c.goal) < 0.4f) {
                c.goal = wanderGoal(c, v, 8.0f);
                c.timer = rnd(c.rng, 5.0f, 10.0f);
            }
            paddle(c.goal, 0.55f, 1.1f);
            if (rnd(c.rng) < dt * 0.06f && flat(c.pos, a.you) < 20.0f) emit(life, Ev::Quack, c.kind, c.pos, 1.0f);
            break;
        case State::Flee:
            paddle(c.goal, 2.6f, 3.0f);
            if (c.t > 2.6f) {
                c.state = State::Idle;
                c.home = c.pos;
                c.goal = c.pos;
            }
            break;
        case State::Come:
            if (std::fmod(c.t, 1.6f) < dt) emit(life, Ev::Quack, c.kind, c.pos, 1.0f);
            if (paddle(c.goal, 1.1f, 2.5f) || c.t > 10.0f) {
                c.state = State::Visit;
                c.t = 0;
            }
            break;
        case State::Visit:
            c.heading = turn(c.heading, headingTo(c.pos, a.you), 2.5f * dt);
            c.speed = 0;
            if (c.t > 0.15f && c.t - dt <= 0.15f) emit(life, Ev::Quack, c.kind, c.pos, 1.05f);
            if (c.t > 4.0f) {
                c.state = State::Idle;
                c.goal = c.home;
                c.timer = 8.0f;
            }
            break;
        default: break;
    }
}

void stepFox(Life& life, const Valley& v, Critter& c, const Around& a, float dt) {
    Vec3 from;
    const bool calm = (c.state == State::Idle || c.state == State::Move) && !inMoment(life, c);
    if (calm && c.born > 0.6f && threatened(v, c, a, from)) {
        startFlee(life, v, c, from);
        return;
    }
    switch (c.state) {
        case State::Idle:
            c.puff = std::fmin(1.0f, c.puff + dt * 2.0f);  // (sitting)
            if (flat(c.pos, a.you) < 14.0f) c.heading = turn(c.heading, headingTo(c.pos, a.you), 1.2f * dt);  // curious
            if ((c.timer -= dt) <= 0) {
                c.goal = wanderGoal(c, v, 4.0f);
                c.state = State::Move;
                c.timer = rnd(c.rng, 4.0f, 9.0f);
            }
            break;
        case State::Move:
            c.puff = std::fmax(0.0f, c.puff - dt * 3.0f);
            if (walkToward(c, v, c.goal, 1.3f, 3.0f, dt)) c.state = State::Idle;
            break;
        case State::Come:
            c.puff = std::fmax(0.0f, c.puff - dt * 3.0f);
            if (walkToward(c, v, c.goal, 1.5f, 4.0f, dt)) {
                c.state = State::Visit;
                c.t = 0;
            }
            break;
        case State::Visit:
            c.speed = 0;
            if (c.t > 1.9f) startFlee(life, v, c, c.pos + forwardOf(c.heading), true);  // off to the woods, content
            break;
        case State::Flee:
            c.puff = 0;
            walkToward(c, v, c.goal, 6.0f, 6.0f, dt);
            if (flat(c.pos, c.goal) < 0.6f || flat(c.pos, c.from) > 22.0f || c.t > 6.0f) {
                c.state = State::Gone;
                c.t = 0;
                emit(life, Ev::Rustle, c.kind, c.pos);
            }
            break;
        default: break;
    }
}

// ------------------------------------------------------------------------------- the moment
void befriendNow(Life& life, Critter& c) {
    Moment& m = life.moment;
    if (m.befriended) return;
    m.befriended = true;
    emit(life, Ev::Befriend, c.kind, c.pos);
}

// Moves the partner toward `to` at `speed` (never into water, never far from you).
void palToward(Moment& m, const Valley& v, const Around& a, Vec3 to, float speed, float stopShort, float dt) {
    const float d = flat(m.pal, to);
    if (d <= stopShort) return;
    const float step = std::fmin(speed * dt, d - stopShort);
    const float h = headingTo(m.pal, to);
    const float nx = m.pal.x + std::sin(h) * step, ny = m.pal.y - std::cos(h) * step;
    m.palHeading = turn(m.palHeading, h, 10.0f * dt);
    if (wetAt(v, nx, ny, 0.3f) || std::hypot(nx - a.you.x, ny - a.you.y) > 14.0f || v.normalAt(nx, ny).z < 0.75f) return;
    m.pal.x = nx;
    m.pal.y = ny;
    m.pal.z = v.heightAt(nx, ny);
}

void runMoment(Life& life, const Valley& v, const Around& a, float dt) {
    Moment& m = life.moment;
    if (!m.active()) return;
    if (m.who < 0 || m.who >= kMaxCritters || !life.c[m.who].alive) {
        endMoment(life);
        return;
    }
    Critter& c = life.c[m.who];
    m.t += dt;
    if (!m.takesPal) {
        m.pal = a.pal;
        m.palHeading = a.palHeading;
    }
    switch (m.act) {
        case Act::Whistle:
            if (c.state == State::Visit && m.mark < 0) {
                m.mark = m.t;
                befriendNow(life, c);
            }
            if ((c.state != State::Come && c.state != State::Visit && m.mark < 0) || (m.mark >= 0 && m.t > m.mark + 1.2f) ||
                m.t > 9.0f)
                endMoment(life);
            break;
        case Act::CroakBack:
            if (c.state == State::Visit && c.t >= 1.4f) befriendNow(life, c);
            if (c.state != State::Visit) endMoment(life);
            break;
        case Act::Call:
            if (c.state == State::Visit && m.mark < 0) {
                m.mark = m.t;
                befriendNow(life, c);
            }
            if ((c.state != State::Come && c.state != State::Visit) || (m.mark >= 0 && m.t > m.mark + 0.6f)) endMoment(life);
            break;
        case Act::Chase: {
            // It creeps up, pounces, gives chase; the rabbit is always a hop ahead and dives into
            // cover; your dragon bounces about, delighted.
            constexpr float kStalk = 0.9f, kPounce = 1.35f, kRun = 3.4f, kEnd = 4.9f;
            if (m.t < kStalk) {
                m.move = PalMove::Stalk;
                c.state = State::Idle;
                c.puff = 1.0f;  // sat up, ears high: it's seen it coming
                c.timer = 10.0f;
                palToward(m, v, a, c.pos, 0.8f, 2.4f, dt);
            } else if (m.t < kPounce) {
                m.move = PalMove::Pounce;
                if (c.state != State::Chased && c.state != State::Gone) {
                    c.state = State::Chased;
                    c.t = 0;
                    c.speed = 6.8f;
                    c.goal = escapeGoal(c, v, m.pal, 16.0f);
                    c.air = 0;
                    emit(life, Ev::Hop, c.kind, c.pos);
                }
                palToward(m, v, a, c.pos, 5.0f, 1.3f, dt);
            } else if (m.t < kRun) {
                if (c.state == State::Gone) {
                    m.t = kRun;  // (it's hidden already)
                } else {
                    m.move = PalMove::Run;
                    const float gap = flat(m.pal, c.pos);
                    if (gap < 1.6f) c.speed = 8.5f;  // (a burst: never caught)
                    else c.speed = 6.8f;
                    palToward(m, v, a, c.pos, std::fmin(5.2f, c.speed - 1.4f), 1.4f, dt);
                }
            } else {
                if (c.state == State::Chased) {  // into the bushes
                    c.state = State::Gone;
                    c.t = 0;
                    emit(life, Ev::Rustle, c.kind, c.pos);
                }
                m.move = PalMove::Happy;
                if (m.t > kRun + 0.15f) befriendNow(life, c);
                if (m.t > kEnd) endMoment(life);
            }
            break;
        }
        case Act::Still: {
            // Your dragon sits still; the butterfly lands on its head, stays a while, and its
            // tickle makes it sneeze.
            // (on top of its head: the head bone is at the skull's base, the head a good size above it)
            const float headH = std::fmax(0.3f, a.palHead.z - m.pal.z);
            const Vec3 lift = forwardOf(m.palHeading) * (0.08f * headH) + Vec3{0, 0, 0.1f + 0.38f * headH};
            const Vec3 perch = a.hasPal ? (a.palHeadSet ? a.palHead + lift : m.pal + Vec3{0, 0, 1.0f})
                                        : a.you + Vec3{0, 0, 1.55f};
            c.goal = perch;
            if (c.state == State::Perch) c.heading = turn(c.heading, m.palHeading, 4.0f * dt);  // (facing the way it faces)
            if (m.mark < 0) {
                m.move = PalMove::Sit;
                if (c.state == State::Perch) {
                    m.mark = m.t;
                    befriendNow(life, c);
                } else if (m.t > 3.5f) {
                    c.pos = perch;
                    c.state = State::Perch;
                    c.t = 0;
                    emit(life, Ev::Shimmer, c.kind, c.pos);
                } else if (c.state != State::Come) {
                    c.state = State::Come;
                }
            } else {
                const float sneeze = m.mark + 4.2f;
                if (m.t < sneeze) {
                    m.move = PalMove::Sit;
                } else if (m.t < sneeze + 0.9f) {
                    m.move = PalMove::Sneeze;
                    if (m.t >= sneeze + 0.25f && c.state == State::Perch) {  // off it flutters
                        c.state = State::Idle;
                        c.t = 0;
                        c.air = std::fmax(0.6f, c.pos.z - v.heightAt(c.pos.x, c.pos.y));
                    }
                } else {
                    m.move = PalMove::Happy;
                    if (c.state == State::Perch) c.state = State::Idle;
                    if (m.t > sneeze + 2.2f) endMoment(life);
                }
            }
            break;
        }
        case Act::Quiet: {
            // Your dragon sits quietly; the fox trots up and they sniff noses, then off it goes.
            const Vec3 fwd = forwardOf(m.palHeading);
            const float reach = a.palHeadSet ? std::hypot(a.palHead.x - m.pal.x, a.palHead.y - m.pal.y) + 0.5f : 1.3f;
            const Vec3 nose{m.pal.x + fwd.x * reach, m.pal.y + fwd.y * reach, 0};
            if (m.mark < 0) {
                m.move = PalMove::Sit;
                c.goal = {nose.x, nose.y, v.heightAt(nose.x, nose.y)};
                if (c.state == State::Visit) {
                    m.mark = m.t;
                    c.heading = headingTo(c.pos, m.pal);
                } else if (m.t > 7.0f) {
                    c.pos = c.goal;
                    c.state = State::Visit;
                    c.t = 0;
                } else if (c.state != State::Come) {
                    c.state = State::Come;
                }
            } else {
                m.move = m.t < m.mark + 1.8f ? PalMove::Sniff : PalMove::Happy;
                if (m.t > m.mark + 0.6f && !m.befriended) {
                    befriendNow(life, c);
                    emit(life, Ev::Yip, c.kind, c.pos);
                }
                if (m.t > m.mark + 3.4f) endMoment(life);
            }
            break;
        }
        case Act::None: break;
    }
}

}  // namespace

// ------------------------------------------------------------------------------- the ground
Ground groundAt(const Valley& v, float x, float y) {
    Ground g;
    if (v.n < 2 || !v.inside(x, y)) return g;
    g.height = v.heightAt(x, y);
    const float w = v.water;
    g.water = g.height < w - 0.15f;
    g.deep = g.height < w - 0.7f;
    g.steep = v.normalAt(x, y).z < 0.86f;
    const int i = static_cast<int>(clampf((x - v.x0) / v.spacing + 0.5f, 0.0f, v.n - 1.0f));
    const int j = static_cast<int>(clampf((y - v.y0) / v.spacing + 0.5f, 0.0f, v.n - 1.0f));
    const u8* c = &v.rgb[(std::size_t(j) * v.n + i) * 3];
    const int r = c[0], gg = c[1], b = c[2];
    if (!g.water) {
        g.snow = b >= r + 4 && b > 150 && gg > 150;
        g.grass = !g.snow && gg > r + 12 && gg > b + 18;
        g.earth = !g.snow && r > gg && gg > b && r - b > 45;
        if (g.height < w + 1.3f)
            for (int k = 0; k < 8 && !g.shore; ++k) {
                const float a = k * (kTau / 8);
                g.shore = v.heightAt(x + std::cos(a) * 2.5f, y + std::sin(a) * 2.5f) < w - 0.15f;
            }
        g.flowers = propNear(v, x, y, 10.0f, 1u << kPropFlowers);
        g.cover = propNear(v, x, y, 9.0f, kCoverKinds);
    }
    for (const ValleyPlaceInfo& p : v.places) {  // (the Market's square kept clear: its view is the valley's busiest)
        if (p.id == kPlaceLake || p.id == kPlaceIsles || p.id == kPlaceOrchard) continue;
        if (std::hypot(p.at.x - x, p.at.y - y) < (p.id == kPlaceMarket ? 60.0f : 14.0f)) g.nearPlace = true;
    }
    return g;
}

Kind cellKind(const Valley& v, u32 seed, int cx, int cy, float day, float dusk, float night, Vec3& spot, int& size) {
    const u32 h = hash3(seed, static_cast<u32>(cx), static_cast<u32>(cy));
    size = 0;
    if (unit(h) > kCellChance) return Kind::Count;
    spot = {(cx + 0.1f + 0.8f * unit(hash3(h, 1, 0))) * kCell, (cy + 0.1f + 0.8f * unit(hash3(h, 2, 0))) * kCell, 0};
    if (!v.inside(spot.x, spot.y)) return Kind::Count;
    const Ground g = groundAt(v, spot.x, spot.y);
    struct Cand {
        Kind k;
        float w;
        Vec3 at;
    };
    Cand cand[kKinds];
    int n = 0;
    auto add = [&](Kind k, float w, Vec3 at) {
        if (w > 0.01f) cand[n++] = {k, w, at};
    };
    if (!g.water && !g.steep && !g.nearPlace) {
        if (g.grass) {
            add(Kind::Songbird, 2.0f * smooth(0.3f, 0.6f, day), spot);
            if (g.cover) add(Kind::Rabbit, 3.0f * smooth(0.25f, 0.55f, day + dusk), spot);
            if (g.cover) add(Kind::Fox, 2.0f * smooth(0.35f, 0.7f, dusk + night), spot);
            add(Kind::Butterfly, (g.flowers ? 8.0f : 0.6f) * smooth(0.45f, 0.75f, day), spot);  // (over any meadow now and then)
        }
        if (g.snow) add(Kind::SnowHare, 4.0f * smooth(0.1f, 0.4f, day + dusk), spot);
    }
    // By the water: the shore for frogs, the open water for ducks (looked for from the spot).
    if (g.height < v.water + 4.0f && !g.snow) {
        Vec3 shore{};
        bool found = g.shore && !g.nearPlace;
        if (found) shore = spot;
        for (float r = 1.5f; r <= 10.0f && !found; r += 1.5f)
            for (int k = 0; k < 8 && !found; ++k) {
                const float a = k * (kTau / 8) + unit(h) * 3.0f;
                const float x = spot.x + std::cos(a) * r, y = spot.y + std::sin(a) * r;
                const float sh = v.heightAt(x, y);
                if (sh < v.water - 0.1f || sh > v.water + 1.2f) continue;  // (cheap first: the band just above the water)
                const Ground s = groundAt(v, x, y);
                if (s.shore && !s.steep && !s.snow && !s.nearPlace) {
                    shore = {x, y, s.height};
                    found = true;
                }
            }
        if (found) add(Kind::Frog, 3.5f * (1.0f + 0.5f * night), shore);
        Vec3 open{};
        bool deep = g.deep;
        if (deep) open = spot;
        if (!deep && waterNear(v, spot, 10.0f, 0.9f, open)) deep = v.heightAt(open.x, open.y) < v.water - 0.9f;
        if (deep) {
            bool nearShore = false;  // open water with a shore within sight (not the middle of the lake)
            for (int k = 0; k < 8 && !nearShore; ++k) {
                const float a = k * (kTau / 8);
                for (float r = 4.0f; r <= 16.0f && !nearShore; r += 4.0f)
                    nearShore = v.heightAt(open.x + std::cos(a) * r, open.y + std::sin(a) * r) > v.water;
            }
            if (nearShore) add(Kind::Duck, 3.5f * smooth(0.25f, 0.55f, day + dusk), open);
        }
    }
    if (n == 0) return Kind::Count;
    float total = 0;
    for (int k = 0; k < n; ++k) total += cand[k].w;
    float pick = unit(hash3(h, 3, 0)) * total;
    int k = 0;
    while (k < n - 1 && pick > cand[k].w) pick -= cand[k++].w;
    // Some are shyer than others: the fox is a rare sight, the songbirds everywhere.
    static const float kKeep[kKinds] = {0.8f, 0.85f, 0.85f, 1.0f, 1.0f, 0.9f, 0.3f};
    if (unit(hash3(h, 5, 0)) > kKeep[static_cast<int>(cand[k].k)]) return Kind::Count;
    spot = cand[k].at;
    const u32 s = hash3(h, 4, 0);
    switch (cand[k].k) {
        case Kind::Songbird: size = 3 + static_cast<int>(s % 3); break;
        case Kind::Rabbit:
        case Kind::SnowHare:
        case Kind::Frog: size = 1 + static_cast<int>(s % 2); break;
        case Kind::Butterfly: size = 2 + static_cast<int>(s % 2); break;
        case Kind::Duck: size = 3 + static_cast<int>(s % 2); break;
        default: size = 1; break;
    }
    spot.z = surfaceFor(v, cand[k].k, spot.x, spot.y);
    return cand[k].k;
}

// ------------------------------------------------------------------------------- living
void reset(Life& life, u32 seed) {
    life = Life{};
    life.seed = seed;
}

void update(Life& life, const Valley& v, const Around& a, float dt) {
    // Last frame's events are done with; what begin() said since stays for this frame's listeners.
    for (int k = life.reported; k < life.eventCount; ++k) life.events[k - life.reported] = life.events[k];
    life.eventCount -= life.reported;
    life.reported = 0;
    dt = clampf(dt, 0.0f, 0.1f);
    life.clock += dt;
    if (!life.started || flat(a.you, life.last) > 30.0f) {  // a new visit, or a trip: everything round you anew
        for (Critter& c : life.c) c.alive = false;
        life.moment = Moment{};
        life.emptiedCount = 0;
        life.lookFor = 0;
        life.soarFor = 4.0f;
        life.started = true;
    }
    life.last = a.you;
    // Far ones go (and the cells they left behind come back once you're away from them too).
    for (int i = 0; i < kMaxCritters; ++i) {
        Critter& c = life.c[i];
        if (c.alive && flat(c.pos, a.you) > kDropRadius && !(life.moment.active() && life.moment.who == i)) c.alive = false;
    }
    for (int k = 0; k < life.emptiedCount;) {
        const u32 key = life.emptied[k];
        const float mx = (static_cast<int>(key >> 16) - 32768 + 0.5f) * kCell, my = (static_cast<int>(key & 0xFFFFu) - 32768 + 0.5f) * kCell;
        if (std::hypot(mx - a.you.x, my - a.you.y) > kDropRadius + 10.0f) {
            life.emptied[k] = life.emptied[--life.emptiedCount];
        } else {
            ++k;
        }
    }
    if ((life.lookFor -= dt) <= 0 && !a.quiet) {
        life.lookFor = 0.5f;
        fill(life, v, a);
    }
    // The soaring pair, by day.
    bool soaring = false;
    for (const Critter& c : life.c) soaring = soaring || (c.alive && c.cell == kSoarCell);
    if (!soaring && a.day > 0.55f && !a.quiet && (life.soarFor -= dt) <= 0) {
        const u32 h = hash3(life.seed, static_cast<u32>(life.clock * 10), 99u);
        for (int i = 0; i < 2; ++i) {
            Critter* c = freeSlot(life);
            if (!c) break;
            *c = Critter{};
            c->alive = true;
            c->kind = Kind::Songbird;
            c->state = State::Soar;
            c->slot = static_cast<u8>(i);
            c->variant = static_cast<u8>((h >> (4 * i)) % 4);
            c->cell = kSoarCell;
            c->rng = hash3(h, static_cast<u32>(i), 5u) | 1u;
            c->anim = i * 2.6f + unit(h) * kTau;
            c->home = a.you;
            c->pos = a.you + Vec3{0, 0, 20};
        }
        life.soarFor = 45.0f;
    }
    // Everyone's own business.
    for (int i = 0; i < kMaxCritters; ++i) {
        Critter& c = life.c[i];
        if (!c.alive) continue;
        c.born += dt;
        c.t += dt;
        if (c.state == State::Gone) {
            if (c.t > 1.2f) {  // (its splash faded)
                c.alive = false;
                bool left = true;  // the whole group gone: the cell stays empty while you're near
                for (const Critter& o : life.c) left = left && !(o.alive && o.cell == c.cell);
                if (left) markEmptied(life, c.cell);
            }
            continue;
        }
        switch (c.kind) {
            case Kind::Songbird: stepSongbird(life, v, c, a, dt); break;
            case Kind::Rabbit:
            case Kind::SnowHare: stepRabbit(life, v, c, a, dt); break;
            case Kind::Butterfly: stepButterfly(life, v, c, a, dt); break;
            case Kind::Frog: stepFrog(life, v, c, a, dt); break;
            case Kind::Duck: stepDuck(life, v, c, a, dt); break;
            case Kind::Fox: stepFox(life, v, c, a, dt); break;
            case Kind::Count: break;
        }
        if (!(life.seen & (1u << static_cast<int>(c.kind))) && c.state != State::Gone && c.state != State::Soar &&
            flat(c.pos, a.you) < 16.0f) {  // (seen close: not a speck high overhead)
            life.seen = static_cast<u8>(life.seen | (1u << static_cast<int>(c.kind)));
            emit(life, Ev::Seen, c.kind, c.pos);
        }
    }
    runMoment(life, v, a, dt);
    life.reported = life.eventCount;
}

int spawnNear(Life& life, const Valley& v, Kind kind, Vec3 you, float heading, float within) {
    auto fits = [&](float x, float y) {
        if (!v.inside(x, y)) return false;
        const Ground g = groundAt(v, x, y);
        switch (kind) {
            case Kind::Songbird: return g.grass && !g.steep && !g.water;
            case Kind::Rabbit:
            case Kind::Fox: return (g.grass || g.earth) && !g.steep && !g.water;
            case Kind::SnowHare: return (g.snow || g.grass) && !g.steep && !g.water;
            case Kind::Butterfly: return !g.water && !g.steep;
            case Kind::Frog: return g.shore && !g.steep;
            case Kind::Duck: return g.deep;
            case Kind::Count: break;
        }
        return false;
    };
    // Ahead of you first (within 70 degrees either side, nearest first), then round about: the
    // nearest spot the kind suits.
    for (int pass = 0; pass < 2; ++pass)
        for (float r = 6.0f; r <= within; r += 1.5f)
            for (int k = pass == 0 ? 0 : 7; k < (pass == 0 ? 7 : 16); ++k) {
                const float a = heading + (k % 2 ? 1.0f : -1.0f) * ((k + 1) / 2) * (kTau / 16);
                const float x = you.x + std::sin(a) * r, y = you.y - std::cos(a) * r;
                if (!fits(x, y)) continue;
                const int size = kind == Kind::Songbird || kind == Kind::Duck ? 4 : kind == Kind::Butterfly ? 2 : 1;
                const u32 key = 0xFFFF0000u | (static_cast<u32>(life.clock * 60) * 8 + static_cast<u32>(kind)) % 0xFFF0u;  // (not a grid cell)
                const int i = spawnGroup(life, v, kind, {x, y, surfaceFor(v, kind, x, y)}, size, key,
                                         hash3(life.seed, key, static_cast<u32>(kind)));
                for (Critter& c : life.c)
                    if (i >= 0 && c.alive && c.cell == key) {
                        c.born = 1.0f;  // (already grown in)
                        if (kind != Kind::Frog && kind != Kind::Duck) c.heading = heading + kPi;  // facing you
                    }
                return i;
            }
    return -1;
}

// ------------------------------------------------------------------------------- the A moment
Act actFor(Kind kind) {
    switch (kind) {
        case Kind::Songbird: return Act::Whistle;
        case Kind::Rabbit:
        case Kind::SnowHare: return Act::Chase;
        case Kind::Butterfly: return Act::Still;
        case Kind::Frog: return Act::CroakBack;
        case Kind::Duck: return Act::Call;
        case Kind::Fox: return Act::Quiet;
        case Kind::Count: break;
    }
    return Act::None;
}

Offer offer(const Life& life, Vec3 you, Vec3 forward, bool hasPal) {
    Offer best;
    if (life.moment.active()) return best;
    float bestScore = 1e9f;
    for (int i = 0; i < kMaxCritters; ++i) {
        const Critter& c = life.c[i];
        if (!c.alive || c.born < 0.5f) continue;
        if (c.state != State::Idle && c.state != State::Move) continue;
        const bool needsPal = c.kind == Kind::Rabbit || c.kind == Kind::SnowHare || c.kind == Kind::Fox;
        if (needsPal && !hasPal) continue;
        if (c.kind == Kind::Duck && c.slot != 0) continue;  // (the mother answers for them)
        float reach = 6.0f, cone = 0.34f;  // cos 70
        switch (c.kind) {
            case Kind::Songbird: reach = 7.0f; break;
            case Kind::Rabbit:
            case Kind::SnowHare: reach = 6.5f; break;
            case Kind::Butterfly: reach = 5.5f; break;
            case Kind::Duck: reach = 14.0f, cone = 0.6f; break;
            case Kind::Fox: reach = 8.5f; break;
            default: break;
        }
        const float dx = c.pos.x - you.x, dy = c.pos.y - you.y, d = std::hypot(dx, dy);
        if (d > reach || d < 0.3f) continue;
        if ((dx * forward.x + dy * forward.y) / d < cone) continue;
        const float score = d / reach;
        if (score < bestScore) {
            bestScore = score;
            best = {i, actFor(c.kind), d};
        }
    }
    return best;
}

bool begin(Life& life, const Valley& v, const Offer& o, const Around& a) {
    if (o.who < 0 || o.who >= kMaxCritters || life.moment.active()) return false;
    Critter& c = life.c[o.who];
    if (!c.alive || c.state == State::Gone || c.state == State::Flee) return false;
    Moment m;
    m.act = o.act;
    m.who = o.who;
    m.pal = a.pal;
    m.palHeading = a.palHeading;
    switch (o.act) {
        case Act::Whistle: {
            const float d = flat(c.pos, a.you);
            const Vec3 dir = d > 0.01f ? (c.pos - a.you) * (1.0f / d) : Vec3{0, 1, 0};
            c.goal = {a.you.x + dir.x * 1.0f, a.you.y + dir.y * 1.0f, 0};
            c.goal.z = v.heightAt(c.goal.x, c.goal.y);
            c.state = State::Come;
            c.timer = 0.15f;
            c.speed = 0;
            c.air = 0;
            for (Critter& o2 : life.c)  // the rest of the flock stays put to watch
                if (o2.alive && o2.cell == c.cell && &o2 != &c && o2.state == State::Idle) o2.timer += 2.5f;
            emit(life, Ev::YouWhistle, c.kind, a.you);
            break;
        }
        case Act::Chase:
            if (!a.hasPal || a.riding) return false;
            m.takesPal = true;
            m.palHeading = headingTo(a.pal, c.pos);
            c.state = State::Idle;
            c.puff = 1.0f;
            c.timer = 10.0f;
            c.speed = 0;
            c.air = 0;
            break;
        case Act::Still:
            m.takesPal = a.hasPal && !a.riding;
            c.state = State::Come;
            c.goal = a.hasPal ? a.pal + Vec3{0, 0, 1.0f} : a.you + Vec3{0, 0, 1.55f};
            break;
        case Act::CroakBack:
            c.state = State::Visit;
            c.t = 0;
            c.puff = 0;
            emit(life, Ev::YouCroak, c.kind, a.you);
            break;
        case Act::Call: {
            // To the water's edge nearest you: along the way from it to you, the last water deep
            // enough to float in.
            Vec3 edge = c.pos;
            const float d = flat(c.pos, a.you);
            for (float s = 0.5f; s < d; s += 0.5f) {
                const float x = c.pos.x + (a.you.x - c.pos.x) * s / d, y = c.pos.y + (a.you.y - c.pos.y) * s / d;
                if (v.heightAt(x, y) >= v.water - 0.35f || std::hypot(x - a.you.x, y - a.you.y) < 1.5f) break;
                edge = {x, y, v.water};
            }
            c.goal = edge;
            c.state = State::Come;
            c.t = 0;
            emit(life, Ev::YouWhistle, c.kind, a.you);
            break;
        }
        case Act::Quiet:
            if (!a.hasPal || a.riding) return false;
            m.takesPal = true;
            m.palHeading = headingTo(a.pal, c.pos);
            c.state = State::Come;
            c.puff = 0;
            c.goal = a.pal + forwardOf(m.palHeading) * 1.3f;  // (runMoment keeps it at the nose)
            c.goal.z = v.heightAt(c.goal.x, c.goal.y);
            break;
        case Act::None: return false;
    }
    life.moment = m;
    return true;
}

void endMoment(Life& life) {
    Moment& m = life.moment;
    if (m.who >= 0 && m.who < kMaxCritters) {
        Critter& c = life.c[m.who];
        if (c.alive) {
            switch (c.state) {
                case State::Come:
                case State::Perch:
                    c.state = c.kind == Kind::Songbird ? State::Move : State::Idle;
                    c.goal = c.home;
                    c.t = 0;
                    c.timer = 0.3f;
                    break;
                case State::Idle:
                    if (c.kind == Kind::Rabbit || c.kind == Kind::SnowHare) c.timer = 1.0f;
                    break;
                default: break;
            }
        }
    }
    m = Moment{};
}

// ------------------------------------------------------------------------------- rewards
Reward befriend(SaveData& s, Dragon* partner, Kind kind, s32 today) {
    Reward r;
    const int k = static_cast<int>(kind);
    if (k < 0 || k >= kKinds) return r;
    Progress& p = s.progress;
    if (p.critterDay != today) {
        p.critterDay = today;
        p.critterToday = 0;
        p.critterPaid = 0;
    }
    const u8 bit = static_cast<u8>(1u << k);
    r.firstEver = !(p.critterFriends & bit);
    r.firstToday = !(p.critterToday & bit);
    p.critterFriends = static_cast<u8>(p.critterFriends | bit);
    p.critterSeen = static_cast<u8>(p.critterSeen | bit);
    p.critterToday = static_cast<u8>(p.critterToday | bit);
    if (p.critterCounts[k] < 255) ++p.critterCounts[k];
    if (r.firstToday) r.gleam += kGleamDaily;
    if (r.firstEver) r.gleam += kGleamFirst;
    s.gleam += static_cast<u32>(r.gleam);
    if (partner && partner->stage != Stage::Egg && p.critterPaid < kDailyPaid) {
        ++p.critterPaid;
        r.paid = true;
        switch (kind) {
            case Kind::Songbird: r.love = 2; break;
            case Kind::Rabbit:
            case Kind::SnowHare: r.play = 5; break;
            case Kind::Butterfly: r.love = 4; break;
            case Kind::Frog: r.play = 2; break;
            case Kind::Duck: r.love = 2, r.bond = 1; break;
            case Kind::Fox: r.love = 1, r.bond = 2; break;
            case Kind::Count: break;
        }
        partner->needs.play = clamp100(partner->needs.play + r.play);
        partner->needs.love = clamp100(partner->needs.love + r.love);
        if (r.bond) addBond(*partner, r.bond);
    }
    return r;
}

void markSeen(SaveData& s, Kind kind) {
    if (static_cast<int>(kind) < kKinds) s.progress.critterSeen = static_cast<u8>(s.progress.critterSeen | (1u << static_cast<int>(kind)));
}
bool seen(const SaveData& s, Kind kind) { return (s.progress.critterSeen >> static_cast<int>(kind)) & 1u; }
bool befriended(const SaveData& s, Kind kind) { return (s.progress.critterFriends >> static_cast<int>(kind)) & 1u; }
int friendCount(const SaveData& s, Kind kind) {
    return static_cast<int>(kind) < kKinds ? s.progress.critterCounts[static_cast<int>(kind)] : 0;
}
int paidToday(const SaveData& s, s32 today) { return s.progress.critterDay == today ? s.progress.critterPaid : 0; }

// ------------------------------------------------------------------------------- meshes
namespace {

struct Rgb3 {
    u8 r, g, b;
};

// A critter's frame: its local x right, y forward, z up (metres at scale 1), tipped by `pitch`
// (nose up +) about `pivot`, then turned and placed.
struct Frame {
    Vec3 origin, right, fwd, up{0, 0, 1};
    float scale = 1, cp = 1, sp = 0;
    Vec3 pivot;
    void set(Vec3 at, float heading, float s) {
        origin = at;
        fwd = forwardOf(heading);
        right = {-std::cos(heading), -std::sin(heading), 0};  // (heading 0 faces -Y: its right is -X)
        scale = s;
    }
    void tip(float pitch, Vec3 about) {
        cp = std::cos(pitch);
        sp = std::sin(pitch);
        pivot = about;
    }
    Vec3 operator()(Vec3 l) const {
        Vec3 p = l - pivot;
        p = {p.x, p.y * cp - p.z * sp, p.y * sp + p.z * cp};
        p = p + pivot;
        return origin + (right * p.x + fwd * p.y + up * p.z) * scale;
    }
};

const Vec3 kSun = normalize(Vec3{0.35f, 0.45f, 0.82f});

struct Out {
    Mesh& m;
    int budget;  // triangles left
    bool tri(Vec3 a, Vec3 b, Vec3 c, Rgb3 col, Vec3 centre, bool twoSided = false) {
        if (budget <= 0 || m.verts + 3 > kMaxTris * 3) return false;
        Vec3 n = cross(b - a, c - a);
        const float len = length(n);
        n = len > 1e-9f ? n * (1.0f / len) : Vec3{0, 0, 1};
        float lit;
        if (twoSided) {
            lit = 0.72f + 0.28f * std::fabs(dot(n, kSun));
        } else {
            const Vec3 mid = (a + b + c) * (1.0f / 3.0f);
            if (dot(n, mid - centre) < 0) n = n * -1.0f;
            lit = 0.56f + 0.4f * std::fmax(0.0f, dot(n, kSun)) + 0.08f * std::fmax(0.0f, n.z);
        }
        lit = std::fmin(lit, 1.0f);
        const Vec3 p[3] = {a, b, c};
        for (int k = 0; k < 3; ++k) {
            m.pos[m.verts] = p[k];
            u8* q = m.col + m.verts * 4;
            q[0] = static_cast<u8>(col.r * lit);
            q[1] = static_cast<u8>(col.g * lit);
            q[2] = static_cast<u8>(col.b * lit);
            q[3] = 255;
            ++m.verts;
        }
        --budget;
        return true;
    }
};

// Six points, eight faces: the low-poly body (and heads, a fox's tail). Colours: the front's
// upper and lower faces, the back's upper and lower.
void diamond(Out& o, const Frame& f, Vec3 front, Vec3 back, Vec3 left, Vec3 right, Vec3 top, Vec3 bottom, Rgb3 ft, Rgb3 fb,
             Rgb3 bt, Rgb3 bb) {
    const Vec3 F = f(front), B = f(back), L = f(left), R = f(right), T = f(top), D = f(bottom);
    const Vec3 mid = (F + B + L + R + T + D) * (1.0f / 6.0f);
    o.tri(F, T, R, ft, mid);
    o.tri(F, L, T, ft, mid);
    o.tri(B, R, T, bt, mid);
    o.tri(B, T, L, bt, mid);
    o.tri(F, R, D, fb, mid);
    o.tri(F, D, L, fb, mid);
    o.tri(B, D, R, bb, mid);
    o.tri(B, L, D, bb, mid);
}

Rgb3 dim(Rgb3 c, float k) { return {static_cast<u8>(c.r * k), static_cast<u8>(c.g * k), static_cast<u8>(c.b * k)}; }

float growIn(const Critter& c) {
    const float t = clampf(c.born / 0.8f, 0.0f, 1.0f);
    return t * t * (3 - 2 * t);
}

void buildSongbird(Out& o, const Critter& c, float clock) {
    static const Rgb3 kBack[4] = {{168, 118, 80}, {132, 110, 96}, {84, 136, 214}, {236, 196, 70}};
    static const Rgb3 kChest[4] = {{214, 160, 110}, {232, 112, 66}, {236, 160, 110}, {244, 214, 96}};
    static const Rgb3 kBelly[4] = {{236, 220, 188}, {240, 226, 206}, {240, 232, 220}, {248, 236, 190}};
    static const Rgb3 kWing[4] = {{128, 88, 62}, {104, 86, 76}, {60, 104, 180}, {80, 72, 60}};
    const int v = c.variant % 4;
    const bool flying = c.state == State::Flee || c.state == State::Soar;
    Frame f;
    f.set(c.pos + Vec3{0, 0, flying ? 0.0f : c.air}, c.heading, 3.3f * growIn(c));  // (storybook-sized: readable at the walking camera's 9 m)
    float pitch = 0;
    if (c.state == State::Idle) {
        const float peck = std::sin(clock * 7.0f + c.anim);
        pitch = -0.55f * std::fmax(0.0f, peck * peck * peck);
    } else if (c.state == State::Visit) {
        pitch = 0.2f + 0.08f * std::sin(clock * 5.0f);
    } else if (c.state == State::Flee) {
        pitch = 0.25f;
    }
    f.tip(pitch, {0, 0, 0.06f});
    diamond(o, f, {0, 0.1f, 0.11f}, {0, -0.1f, 0.08f}, {-0.055f, 0, 0.085f}, {0.055f, 0, 0.085f}, {0, 0.01f, 0.15f},
            {0, 0, 0.035f}, kBack[v], kChest[v], kBack[v], kBelly[v]);
    const Vec3 mid = f({0, 0, 0.09f});
    o.tri(f({-0.013f, 0.095f, 0.115f}), f({0.013f, 0.095f, 0.115f}), f({0, 0.145f, 0.105f}), {240, 170, 70}, mid, true);
    o.tri(f({0, -0.1f, 0.08f}), f({-0.035f, -0.17f, 0.105f}), f({0.035f, -0.17f, 0.105f}), kWing[v], mid, true);
    float spread = 0, flap = 0;
    if (c.state == State::Flee) {
        spread = 1;
        flap = 0.9f * std::sin(clock * 30.0f + c.anim);
    } else if (c.state == State::Soar) {
        spread = 1;
        const float beat = std::fmax(0.0f, std::sin(clock * 0.9f + c.anim));  // bursts of flaps, then a glide
        flap = 0.1f + 0.6f * beat * std::sin(clock * 16.0f);
    } else if (c.air > 0.01f) {
        spread = 0.3f;
        flap = 0.6f;
    }
    for (int s = -1; s <= 1; s += 2) {
        const Vec3 folded{s * 0.04f, -0.11f, 0.12f};
        const Vec3 open{s * (0.045f + 0.17f * std::cos(flap)), -0.01f, 0.11f + 0.17f * std::sin(flap)};
        o.tri(f({s * 0.045f, 0.035f, 0.11f}), f({s * 0.045f, -0.05f, 0.11f}), f(lerp(folded, open, spread)), kWing[v], mid, true);
    }
}

void buildRabbit(Out& o, const Critter& c, float clock) {
    static const Rgb3 kFur[3] = {{156, 122, 92}, {146, 140, 136}, {190, 144, 96}};
    static const Rgb3 kLight[3] = {{228, 212, 192}, {222, 218, 212}, {240, 222, 196}};
    const bool hare = c.kind == Kind::SnowHare;
    const Rgb3 fur = hare ? Rgb3{206, 212, 230} : kFur[c.variant % 3];  // (a hare: blue-grey enough to read on the snow)
    const Rgb3 light = hare ? Rgb3{250, 251, 255} : kLight[c.variant % 3];
    const Rgb3 ear = hare ? Rgb3{122, 124, 142} : dim(fur, 0.9f);
    Frame f;
    f.set(c.pos + Vec3{0, 0, c.air}, c.heading, 2.4f * growIn(c));
    const bool running = c.state == State::Flee || c.state == State::Chased;
    float pitch = 0;
    if (c.state == State::Idle) {
        pitch = c.puff > 0.01f ? 0.42f * c.puff : -0.2f - 0.05f * std::sin(clock * 9.0f + c.anim);  // sat up, or nibbling
    } else if (running || c.air > 0.01f) {
        pitch = 0.18f * std::sin(c.t * 18.0f);
    }
    f.tip(pitch, {0, -0.15f, 0.05f});
    diamond(o, f, {0, 0.13f, 0.19f}, {0, -0.2f, 0.15f}, {-0.1f, -0.04f, 0.15f}, {0.1f, -0.04f, 0.15f}, {0, -0.06f, 0.28f},
            {0, -0.03f, 0.03f}, fur, light, fur, dim(fur, 0.9f));
    diamond(o, f, {0, 0.27f, 0.25f}, {0, 0.12f, 0.28f}, {-0.058f, 0.19f, 0.28f}, {0.058f, 0.19f, 0.28f}, {0, 0.19f, 0.345f},
            {0, 0.2f, 0.2f}, fur, light, fur, light);
    const Vec3 mid = f({0, 0.15f, 0.36f});
    const float back = running ? 1.0f : c.puff > 0.01f ? 0.0f : 0.45f;  // ears laid back running, up when alert
    for (int s = -1; s <= 1; s += 2) {
        const Vec3 up{s * 0.045f, 0.12f, 0.53f}, laid{s * 0.06f, -0.02f, 0.4f};
        o.tri(f({s * 0.018f, 0.17f, 0.33f}), f({s * 0.05f, 0.15f, 0.33f}), f(lerp(up, laid, back)), ear, mid, true);
    }
    const Vec3 tail = f({0, -0.2f, 0.19f});
    o.tri(f({-0.035f, -0.215f, 0.19f}), f({0, -0.235f, 0.245f}), f({0.035f, -0.215f, 0.19f}), {250, 250, 250}, tail, true);
    o.tri(f({-0.035f, -0.215f, 0.19f}), f({0.035f, -0.215f, 0.19f}), f({0, -0.235f, 0.14f}), {250, 250, 250}, tail, true);
}

void buildButterfly(Out& o, const Critter& c, float clock) {
    static const Rgb3 kUpper[4] = {{244, 150, 60}, {110, 170, 240}, {250, 226, 110}, {244, 146, 170}};
    static const Rgb3 kLower[4] = {{222, 108, 50}, {80, 128, 212}, {236, 190, 76}, {214, 108, 150}};
    const int v = c.variant % 4;
    Frame f;
    f.set(c.pos, c.heading, 2.4f * growIn(c));
    f.tip(c.state == State::Perch || c.state == State::Move ? 0.0f : 0.25f, {0, 0, 0});
    const bool resting = c.state == State::Perch || c.state == State::Move;
    const float flap = resting ? 0.75f + 0.45f * std::sin(clock * 1.4f + c.anim) : 0.15f + 1.25f * std::fabs(std::sin(clock * 15.0f + c.anim));
    const Vec3 mid = f({0, 0, 0});
    const Rgb3 body{60, 48, 56};
    o.tri(f({0, 0.035f, 0.004f}), f({-0.01f, 0, 0.004f}), f({0, -0.045f, 0.004f}), body, mid, true);
    o.tri(f({0, 0.035f, 0.004f}), f({0, -0.045f, 0.004f}), f({0.01f, 0, 0.004f}), body, mid, true);
    const float cf = std::cos(flap), sf = std::sin(flap);
    for (int s = -1; s <= 1; s += 2) {
        o.tri(f({0, 0.02f, 0.004f}), f({0, -0.005f, 0.004f}), f({s * 0.12f * cf, 0.055f, 0.004f + 0.12f * sf}), kUpper[v], mid, true);
        o.tri(f({0, -0.005f, 0.004f}), f({0, -0.04f, 0.004f}), f({s * 0.085f * cf, -0.07f, 0.004f + 0.085f * sf}), kLower[v], mid, true);
    }
}

void buildFrog(Out& o, const Critter& c, float clock) {
    static const Rgb3 kSkin[3] = {{112, 178, 86}, {86, 160, 120}, {150, 190, 80}};
    static const Rgb3 kBelly[3] = {{220, 228, 150}, {210, 226, 170}, {232, 232, 160}};
    const Rgb3 skin = kSkin[c.variant % 3], belly = kBelly[c.variant % 3], leg = dim(skin, 0.78f);
    Frame f;
    f.set(c.pos + Vec3{0, 0, c.air}, c.heading, 3.1f * growIn(c));
    const bool leaping = c.state == State::Leap;
    f.tip(leaping ? 0.45f : 0.12f, {0, -0.08f, 0});
    const float p = c.puff > 0 ? std::sin(kPi * clampf(c.puff, 0.0f, 1.0f)) : 0.0f;  // the throat swelling
    diamond(o, f, {0, 0.11f, 0.07f}, {0, -0.1f, 0.075f}, {-0.085f - 0.02f * p, -0.01f, 0.06f}, {0.085f + 0.02f * p, -0.01f, 0.06f},
            {0, -0.005f, 0.125f}, {0, 0.02f + 0.04f * p, 0.012f - 0.012f * p}, skin, belly, skin, belly);
    const Vec3 mid = f({0, 0, 0.07f});
    for (int s = -1; s <= 1; s += 2) {
        o.tri(f({s * 0.022f, 0.07f, 0.105f}), f({s * 0.056f, 0.07f, 0.105f}), f({s * 0.04f, 0.075f, 0.142f}), {48, 58, 42}, mid, true);
        const Vec3 foot = leaping ? Vec3{s * 0.08f, -0.22f, 0.03f} : Vec3{s * 0.1f, -0.12f, 0.012f};
        o.tri(f({s * 0.06f, -0.05f, 0.03f}), f({s * 0.115f, -0.02f, 0.012f}), f(foot), leg, mid, true);
    }
    (void)clock;
}

void buildDuck(Out& o, const Critter& c, float clock) {
    const bool baby = c.slot > 0;
    const Rgb3 top = baby ? Rgb3{252, 222, 96} : Rgb3{242, 240, 232};
    const Rgb3 under = baby ? Rgb3{240, 204, 80} : Rgb3{222, 218, 206};
    const Rgb3 beak = baby ? Rgb3{240, 150, 60} : Rgb3{244, 160, 50};
    Frame f;
    const float bob = 0.015f * std::sin(clock * 2.5f + c.anim);
    f.set(c.pos + Vec3{0, 0, bob - 0.03f}, c.heading, (baby ? 0.85f : 1.7f) * growIn(c));
    f.tip(c.state == State::Visit ? 0.1f * std::sin(clock * 6.0f) : 0.05f * std::sin(clock * 1.9f + c.anim), {0, 0, 0});
    diamond(o, f, {0, 0.2f, 0.13f}, {0, -0.22f, 0.15f}, {-0.13f, 0, 0.11f}, {0.13f, 0, 0.11f}, {0, -0.03f, 0.22f}, {0, 0, -0.02f},
            top, under, top, under);
    const Vec3 h0 = f({-0.05f, 0.15f, 0.27f}), h1 = f({0.05f, 0.15f, 0.27f}), h2 = f({0, 0.235f, 0.27f}), h3 = f({0, 0.175f, 0.37f});
    const Vec3 hm = (h0 + h1 + h2 + h3) * 0.25f;
    o.tri(h0, h1, h3, top, hm);
    o.tri(h1, h2, h3, top, hm);
    o.tri(h2, h0, h3, top, hm);
    o.tri(h0, h2, h1, under, hm);
    const Vec3 mid = f({0, 0, 0.15f});
    o.tri(f({-0.028f, 0.222f, 0.29f}), f({0.028f, 0.222f, 0.29f}), f({0, 0.3f, 0.28f}), beak, mid, true);
    o.tri(f({0, -0.22f, 0.15f}), f({-0.045f, -0.27f, 0.22f}), f({0.045f, -0.27f, 0.22f}), under, mid, true);
}

void buildFox(Out& o, const Critter& c, float clock) {
    const Rgb3 fur{228, 122, 52}, white{244, 232, 214}, dark{92, 60, 48}, ear{170, 80, 40};
    Frame f;
    f.set(c.pos, c.heading, 1.6f * growIn(c));
    const float sit = c.puff;
    const float sniff = c.state == State::Visit ? 1.0f : 0.0f;
    f.tip(0.5f * sit - 0.22f * sniff, {0, -0.22f, 0.2f});
    diamond(o, f, {0, 0.26f, 0.36f}, {0, -0.27f, 0.34f}, {-0.12f, 0, 0.32f}, {0.12f, 0, 0.32f}, {0, -0.02f, 0.45f}, {0, 0, 0.22f},
            fur, white, fur, dim(fur, 0.88f));
    diamond(o, f, {0, 0.5f, 0.4f}, {0, 0.28f, 0.46f}, {-0.085f, 0.33f, 0.46f}, {0.085f, 0.33f, 0.46f}, {0, 0.33f, 0.54f},
            {0, 0.35f, 0.37f}, fur, white, fur, white);
    const Vec3 mid = f({0, 0.1f, 0.4f});
    for (int s = -1; s <= 1; s += 2)
        o.tri(f({s * 0.03f, 0.31f, 0.52f}), f({s * 0.08f, 0.32f, 0.5f}), f({s * 0.07f, 0.29f, 0.66f}), ear, mid, true);
    // The tail, swishing about its root.
    const float swish = (c.state == State::Flee ? 0.15f : 0.4f) * std::sin(clock * 2.2f + c.anim);
    const float cs = std::cos(swish), ss = std::sin(swish);
    auto tail = [&](Vec3 p) {
        const Vec3 root{0, -0.25f, 0.35f};
        const Vec3 d = p - root;
        return root + Vec3{d.x * cs - d.y * ss, d.x * ss + d.y * cs, d.z - 0.2f * sit * (-d.y)};
    };
    diamond(o, f, tail({0, -0.25f, 0.35f}), tail({0, -0.64f, 0.29f}), tail({-0.075f, -0.44f, 0.33f}), tail({0.075f, -0.44f, 0.33f}),
            tail({0, -0.44f, 0.41f}), tail({0, -0.44f, 0.26f}), fur, fur, white, white);
    // The legs (not tipped with the body: they stay on the ground).
    Frame g = f;
    g.tip(0, {0, 0, 0});
    const bool moving = c.speed > 0.1f;
    const float stride = moving ? 0.1f * std::sin(clock * (c.speed > 3 ? 16.0f : 8.0f) + c.anim) : 0.0f;
    const Vec3 lm = g({0, 0, 0.12f});
    for (int s = -1; s <= 1; s += 2) {
        o.tri(g({s * 0.07f - 0.02f, 0.17f, 0.27f}), g({s * 0.07f + 0.02f, 0.17f, 0.27f}), g({s * 0.07f, 0.19f + s * stride, 0}), dark, lm, true);
        if (sit < 0.5f)
            o.tri(g({s * 0.07f - 0.02f, -0.18f, 0.27f}), g({s * 0.07f + 0.02f, -0.18f, 0.27f}), g({s * 0.07f, -0.2f - s * stride, 0}), dark,
                  lm, true);
    }
}

// Custard: Bram's fluffy sheepdog, seen up close (he's petted), so rounder than the wild ones: blobs
// (ellipsoids, lit top to bottom) for his body, a grey saddle, a big shaggy head with a fringe, a
// muzzle and black nose, ears that flop down, a plume of a tail going like a flag, short legs.
void blob(Out& o, const Frame& f, Vec3 c, Vec3 r, int seg, int rings, Rgb3 top, Rgb3 bottom) {
    const Vec3 centre = f(c);
    auto at = [&](int j, int k) {
        const float el = kTau * 0.25f - kTau * 0.5f * static_cast<float>(j) / static_cast<float>(rings);
        const float az = kTau * static_cast<float>(k) / static_cast<float>(seg);
        return f({c.x + r.x * std::cos(el) * std::sin(az), c.y + r.y * std::cos(el) * std::cos(az), c.z + r.z * std::sin(el)});
    };
    for (int j = 0; j < rings; ++j) {
        const float t = (j + 0.5f) / static_cast<float>(rings);
        const Rgb3 col{static_cast<u8>(top.r + (bottom.r - top.r) * t), static_cast<u8>(top.g + (bottom.g - top.g) * t),
                       static_cast<u8>(top.b + (bottom.b - top.b) * t)};
        for (int k = 0; k < seg; ++k) {
            const Vec3 a = at(j, k), b = at(j, k + 1), c2 = at(j + 1, k), d = at(j + 1, k + 1);
            if (j > 0) o.tri(a, b, d, col, centre);
            if (j < rings - 1) o.tri(a, d, c2, col, centre);
        }
    }
}

void buildDog(Out& o, const DogPose& p) {
    const Rgb3 fur{252, 242, 214}, under{226, 210, 176}, grey{150, 150, 162}, greyU{118, 118, 132}, dark{54, 46, 48};
    Frame f;
    f.set(p.at, p.heading, 1.0f);
    const float sit = clampf(p.sit, 0.0f, 1.0f);
    f.tip(0.4f * sit, {0, -0.2f, 0.22f});
    blob(o, f, {0, 0, 0.36f}, {0.2f, 0.32f, 0.19f}, 7, 4, fur, under);         // the body
    blob(o, f, {0, -0.1f, 0.45f}, {0.18f, 0.22f, 0.11f}, 6, 2, grey, greyU);  // the saddle
    blob(o, f, {0, 0.36f, 0.58f}, {0.16f, 0.15f, 0.15f}, 7, 4, fur, under);   // the head
    blob(o, f, {0, 0.5f, 0.53f}, {0.075f, 0.08f, 0.06f}, 5, 3, fur, under);   // the muzzle
    const Vec3 mid = f({0, 0.36f, 0.58f});
    o.tri(f({-0.03f, 0.575f, 0.565f}), f({0.03f, 0.575f, 0.565f}), f({0, 0.585f, 0.53f}), dark, mid, true);  // the nose
    for (int s = -1; s <= 1; s += 2) {
        o.tri(f({s * 0.12f, 0.36f, 0.7f}), f({s * 0.16f, 0.3f, 0.66f}), f({s * 0.2f, 0.34f, 0.5f}), grey, mid, true);  // a floppy ear
        o.tri(f({s * 0.12f, 0.36f, 0.7f}), f({s * 0.2f, 0.34f, 0.5f}), f({s * 0.15f, 0.4f, 0.55f}), grey, mid, true);
        o.tri(f({s * 0.05f, 0.495f, 0.63f}), f({s * 0.095f, 0.48f, 0.63f}), f({s * 0.07f, 0.497f, 0.6f}), dark, mid, true);  // an eye
    }
    o.tri(f({-0.13f, 0.47f, 0.7f}), f({0.13f, 0.47f, 0.7f}), f({0, 0.52f, 0.62f}), fur, mid, true);  // the fringe
    // The tail, a plume wagging side to side (faster when he's happy).
    const float swing = (0.3f + 0.5f * p.wag) * std::sin(p.clock * (6.0f + 10.0f * p.wag));
    Frame t = f;
    t.set(f({0, -0.3f, 0.44f}), p.heading + swing + kTau * 0.5f, 1.0f);
    blob(o, t, {0, 0.14f, 0.12f}, {0.07f, 0.15f, 0.07f}, 5, 3, fur, under);
    // The legs, short and fluffy (not tipped with the body); sat, the back ones fold away.
    Frame g = f;
    g.tip(0, {0, 0, 0});
    const Vec3 lm = g({0, 0, 0.1f});
    for (int s = -1; s <= 1; s += 2) {
        o.tri(g({s * 0.1f - 0.045f, 0.2f, 0.26f}), g({s * 0.1f + 0.045f, 0.2f, 0.26f}), g({s * 0.1f, 0.22f, 0}), under, lm, true);
        o.tri(g({s * 0.1f, 0.18f, 0.26f}), g({s * 0.1f, 0.25f, 0.26f}), g({s * 0.1f, 0.22f, 0}), under, lm, true);
        if (sit < 0.5f) {
            o.tri(g({s * 0.1f - 0.045f, -0.2f, 0.26f}), g({s * 0.1f + 0.045f, -0.2f, 0.26f}), g({s * 0.1f, -0.22f, 0}), under, lm, true);
            o.tri(g({s * 0.1f, -0.17f, 0.26f}), g({s * 0.1f, -0.24f, 0.26f}), g({s * 0.1f, -0.22f, 0}), under, lm, true);
        }
    }
}

void buildRing(Out& o, const Critter& c) {
    const float u = clampf(c.t / 1.2f, 0.0f, 1.0f);
    const float r0 = 0.12f + 0.85f * u, r1 = r0 + 0.03f + 0.07f * (1.0f - u);
    const Vec3 at{c.goal.x, c.goal.y, c.goal.z + 0.05f};
    constexpr int kSeg = 8;
    const Rgb3 col{static_cast<u8>(236 - 40 * u), static_cast<u8>(246 - 30 * u), 250};
    for (int k = 0; k < kSeg; ++k) {
        const float a0 = k * (kTau / kSeg), a1 = (k + 1) * (kTau / kSeg);
        const Vec3 i0{at.x + std::cos(a0) * r0, at.y + std::sin(a0) * r0, at.z}, i1{at.x + std::cos(a1) * r0, at.y + std::sin(a1) * r0, at.z};
        const Vec3 e0{at.x + std::cos(a0) * r1, at.y + std::sin(a0) * r1, at.z}, e1{at.x + std::cos(a1) * r1, at.y + std::sin(a1) * r1, at.z};
        o.tri(i0, e0, e1, col, at, true);
        o.tri(i0, e1, i1, col, at, true);
    }
}

float reachOf(const Critter& c) {
    switch (c.kind) {
        case Kind::Songbird: return c.state == State::Soar || c.state == State::Flee ? 90.0f : 40.0f;
        case Kind::Rabbit:
        case Kind::SnowHare: return 44.0f;
        case Kind::Butterfly: return 28.0f;
        case Kind::Frog: return 32.0f;
        case Kind::Duck:
        case Kind::Fox: return 44.0f;
        case Kind::Count: break;
    }
    return 30.0f;
}

}  // namespace

int trianglesOf(Kind kind) {
    switch (kind) {
        case Kind::Songbird: return 12;
        case Kind::Rabbit:
        case Kind::SnowHare: return 20;
        case Kind::Butterfly: return 6;
        case Kind::Frog: return 12;
        case Kind::Duck: return 14;
        case Kind::Fox: return 30;
        case Kind::Count: break;
    }
    return 0;
}

void addDog(Mesh& m, const DogPose& p) {
    Out o{m, kMaxTris - m.verts / 3};
    buildDog(o, p);
}

void buildMesh(const Life& life, Vec3 eye, Vec3 target, Mesh& out) {
    out.verts = 0;
    out.shown = 0;
    const Vec3 look = normalize(target - eye);
    struct Pick {
        float d;
        int i;
    };
    Pick picks[kMaxCritters];
    int n = 0;
    for (int i = 0; i < kMaxCritters; ++i) {
        const Critter& c = life.c[i];
        if (!c.alive) continue;
        if (c.state == State::Gone && !(c.kind == Kind::Frog && c.t < 1.2f)) continue;  // (a frog's splash ring still spreading)
        const Vec3 at = c.state == State::Gone ? c.goal : c.pos;
        const Vec3 to = at - eye;
        const float d = length(to);
        if (d > reachOf(c)) continue;
        if (d > 2.5f && dot(to, look) < 0.62f * d) continue;  // outside the view (a cone a little wider than it)
        picks[n++] = {d, i};
    }
    for (int a = 1; a < n; ++a)  // nearest first (a handful: an insertion sort)
        for (int b = a; b > 0 && picks[b].d < picks[b - 1].d; --b) {
            const Pick t = picks[b];
            picks[b] = picks[b - 1];
            picks[b - 1] = t;
        }
    Out o{out, kMaxTris};
    for (int k = 0; k < n; ++k) {
        const Critter& c = life.c[picks[k].i];
        const int need = c.state == State::Gone ? kRingTris : trianglesOf(c.kind);
        if (need > o.budget) continue;
        if (c.state == State::Gone) {
            buildRing(o, c);
            ++out.shown;
            continue;
        }
        switch (c.kind) {
            case Kind::Songbird: buildSongbird(o, c, life.clock); break;
            case Kind::Rabbit:
            case Kind::SnowHare: buildRabbit(o, c, life.clock); break;
            case Kind::Butterfly: buildButterfly(o, c, life.clock); break;
            case Kind::Frog: buildFrog(o, c, life.clock); break;
            case Kind::Duck: buildDuck(o, c, life.clock); break;
            case Kind::Fox: buildFox(o, c, life.clock); break;
            case Kind::Count: break;
        }
        ++out.shown;
    }
}

}  // namespace ec::critters
