#include "core/shell_burst.hpp"

#include <cmath>

namespace ec {

namespace {

constexpr float kGravity = 9.0f;     // the ball's (props.cpp): a touch floaty
constexpr float kBounce = 0.3f;      // vertical speed kept: shell on straw barely bounces
constexpr float kSkid = 0.55f;       // sideways speed kept at each bounce
constexpr float kSettleSpeed = 0.9f; // slower than this on landing, it lies down
constexpr int kMaxBounces = 3;

float frand(Rng& rng, float lo, float hi) { return lo + (hi - lo) * (rng.below(10000) / 10000.0f); }

// The shortest rotation taking direction a to direction b (both unit length).
Quat between(Vec3 a, Vec3 b) {
    const float c = dot(a, b);
    if (c < -0.9999f) {  // opposite: any axis across a
        const Vec3 axis = normalize(std::fabs(a.x) < 0.9f ? cross(a, {1, 0, 0}) : cross(a, {0, 1, 0}));
        return quatAxisAngle(axis, 3.14159265f);
    }
    const Vec3 x = cross(a, b);
    return normalize(Quat{x.x, x.y, x.z, 1.0f + c});
}

float smooth(float t) { return t <= 0 ? 0.0f : (t >= 1 ? 1.0f : t * t * (3 - 2 * t)); }

}  // namespace

bool shardShapes(const ModelData& egg, ShardShape out[kShards]) {
    const MeshData* mesh = egg.findMesh(kMeshPart, kGroupShards, 0);
    if (!mesh || mesh->paletteCount != kShards) return false;
    Vec3 middle{0, 0, 0};
    for (int i = 0; i < kShards; ++i) {
        out[i].centre = egg.skel.rest[mesh->palette[i]].translation();
        middle = middle + out[i].centre * (1.0f / kShards);
    }
    for (int i = 0; i < kShards; ++i) {
        out[i].normal = normalize(out[i].centre - middle);
        out[i].outDrop = out[i].inDrop = 0;
    }
    for (int v = 0; v < mesh->vertexCount; ++v) {
        ShardShape& s = out[mesh->skin[std::size_t(v) * 4]];
        const float along = dot(mesh->pos[v] - s.centre, s.normal);
        s.outDrop = std::fmax(s.outDrop, -along);  // the rim, below the centre when outside up
        s.inDrop = std::fmax(s.inDrop, along);     // (the outside's centre is on the surface: ~0)
    }
    return true;
}

float BurstGround::heightAt(Vec2 p) const {
    const float d = std::hypot(p.x - nest.x, p.y - nest.y);
    const float fromRing = std::fabs(d - ringRadius);
    float h = d < ringRadius ? strawFloor : 0.0f;
    if (fromRing < ringTube) h = std::fmax(h, std::sqrt(ringTube * ringTube - fromRing * fromRing));
    return h;
}

void ShellBurst::start(Vec3 base, const ShardShape* shapes, const BurstGround& g, Rng& rng) {
    ground = g;
    active = true;
    vanishing = false;
    age = gone = 0;
    for (int i = 0; i < kShards; ++i) {
        Shard& s = shard[i];
        s = Shard{};
        s.pos = base + shapes[i].centre;
        s.normal = shapes[i].normal;
        s.outDrop = shapes[i].outDrop;
        s.inDrop = shapes[i].inDrop;
        // Out from where it sat, and up: the top pieces fly highest, the bottom ones skid out.
        const Vec3 flat = normalize(Vec3{s.normal.x, s.normal.y, 0});
        const float out = frand(rng, 0.9f, 1.8f);
        const float up = frand(rng, 1.4f, 2.4f) + 1.4f * std::fmax(0.0f, s.normal.z);
        s.vel = flat * out + Vec3{0, 0, up};
        s.spinAxis = normalize(Vec3{frand(rng, -1, 1), frand(rng, -1, 1), frand(rng, -1, 1)});
        s.spin = frand(rng, 5.0f, 12.0f);
    }
}

void ShellBurst::update(float dt) {
    if (!active) return;
    age += dt;
    if (vanishing) {
        gone += dt / kVanishTime;
        if (gone >= 1.0f) active = false;
    }
    for (Shard& s : shard) {
        if (s.settle >= 0) {
            s.settle = std::fmin(kSettleTime, s.settle + dt);
            s.rot = nlerp(s.from, s.to, smooth(s.settle / kSettleTime));
            continue;
        }
        s.vel.z -= kGravity * dt;
        s.pos = s.pos + s.vel * dt;
        s.rot = normalize(mul(quatAxisAngle(s.spinAxis, s.spin * dt), s.rot));
        // The room's wall: back off it.
        const Vec2 rel{s.pos.x - ground.room.x, s.pos.y - ground.room.y};
        const float far = std::hypot(rel.x, rel.y);
        const float wall = ground.wallRadius - 0.1f;
        if (far > wall) {
            const Vec3 n{rel.x / far, rel.y / far, 0};
            s.pos.x = ground.room.x + n.x * wall;
            s.pos.y = ground.room.y + n.y * wall;
            const float into = dot(s.vel, n);
            if (into > 0) s.vel = s.vel - n * (1.6f * into);
        }
        // The ground (floor or straw): bounce, then lie down.
        const float floor = ground.heightAt({s.pos.x, s.pos.y}) + kHalfThick;
        if (s.pos.z <= floor) {  // never through it (flying low into the straw ring's side, too)
            s.pos.z = floor;
            if (s.vel.z >= 0) continue;
            if (-s.vel.z < kSettleSpeed || ++s.bounces >= kMaxBounces) {
                s.vel = {};
                s.spin = 0;
                // Flat: outside up or inside up, whichever it landed nearer.
                const Vec3 now = rotate(s.rot, s.normal);
                const Vec3 want{0, 0, now.z >= 0 ? 1.0f : -1.0f};
                s.pos.z = floor - kHalfThick + (now.z >= 0 ? s.outDrop : s.inDrop + 2 * kHalfThick);
                s.from = s.rot;
                s.to = normalize(mul(between(now, want), s.rot));
                s.settle = 0;
            } else {
                s.vel.z = -s.vel.z * kBounce;
                s.vel.x *= kSkid;
                s.vel.y *= kSkid;
                s.spin *= 0.5f;
            }
        }
    }
}

void ShellBurst::settleNow() {
    // Fly each piece out in big steps until it lands, then lay it down at once.
    for (int step = 0; step < 600 && !allSettled(); ++step) update(1.0f / 60.0f);
    for (Shard& s : shard) {
        if (s.settle < 0) {  // still up (it can't be, after ten seconds): put it on the ground
            s.pos.z = ground.heightAt({s.pos.x, s.pos.y}) + s.outDrop;
            s.from = s.to = s.rot;
        }
        s.settle = kSettleTime;
        s.rot = s.to;
    }
}

bool ShellBurst::allSettled() const {
    for (const Shard& s : shard)
        if (s.settle < kSettleTime) return false;
    return true;
}

BlobShape blobAt(float t) {
    BlobShape b;
    if (t >= kBlobSwell + kBlobShape) return b;  // done: exactly the dragon
    if (t < kBlobSwell) {
        const float x = t <= 0 ? 0.0f : t / kBlobSwell;
        b.scale = 0.3f + 0.7f * (1.0f - (1.0f - x) * (1.0f - x));  // swells, easing out
        b.morph = 0;
        return b;
    }
    // Slow out of the blob, quick through the middle, a little past the shape and back (an
    // ease-in-out "back" curve, its dip below 0 at the start cut off): the white blob holds a
    // moment before it shapes itself (the ease-out curve had the dragon whole in half a second).
    const float x = (t - kBlobSwell) / kBlobShape;
    constexpr float kBack = 1.2f * 1.525f;
    const float m = x < 0.5f ? (4 * x * x * ((kBack + 1) * 2 * x - kBack)) / 2
                             : ((2 * x - 2) * (2 * x - 2) * ((kBack + 1) * (2 * x - 2) + kBack) + 2) / 2;
    b.morph = std::fmax(0.0f, m);
    return b;
}

Mat34 ShellBurst::transform(int i) const {
    const Shard& s = shard[i];
    // Sinking away: into the straw as it shrinks.
    const float k = vanishing ? std::fmax(0.0f, 1.0f - gone) : 1.0f;
    const Vec3 at = s.pos - Vec3{0, 0, (1.0f - k) * 0.06f};
    return fromQuatScale(s.rot, {k, k, k}, at);
}

}  // namespace ec
