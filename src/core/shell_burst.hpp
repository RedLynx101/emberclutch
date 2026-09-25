// The hatching (Alpha 2 WP12a, Noah's direction after run 2): the egg bursts into bits. Each
// piece of the shell (romfs:/models/egg.ecm's "shards" mesh, one bone each) is flung up and
// out from where it sat on the egg, tumbles, lands on the egg nest's straw or the floor,
// bounces and settles flat (outside or inside up, whichever it landed nearer). Once the
// hatchling is named they sink into the straw and are gone. Pure logic (PC-tested);
// src/app/render3d.cpp draws them in one call.
#pragma once

#include "core/math3d.hpp"
#include "core/model.hpp"
#include "core/rng.hpp"

namespace ec {

constexpr int kShards = 24;  // tools/blender/egg_model.py SHARDS (bones s00..s23 after root and cap)

// What the pieces land on: the den floor, the egg nest's straw (a torus round a thin straw
// disc: tools/blender/den_model.py nests()) and the room's wall.
struct BurstGround {
    Vec2 nest{0, 0};
    float strawFloor = 0.06f;  // the straw disc inside the ring
    float ringRadius = 0.85f;  // the straw torus: its major radius...
    float ringTube = 0.28f;    // ...and its tube (the rim's height)
    Vec2 room{0, 0};
    float wallRadius = 8.9f;
    float heightAt(Vec2 p) const;
};

// A piece as it sat on the egg (egg model space), measured from the model.
struct ShardShape {
    Vec3 centre;         // its bone's rest position: the middle of its outside
    Vec3 normal;         // outward, from the egg's middle
    float outDrop = 0;   // lying outside up: how far its rim hangs below its centre
    float inDrop = 0;    // lying inside up: how far its outside bulges below its centre
};
// The pieces from the egg model (bones "s00".., the "shards" mesh); false if it has none.
bool shardShapes(const ModelData& egg, ShardShape out[kShards]);

struct Shard {
    Vec3 pos;                  // its centre, den space
    Vec3 vel;
    Quat rot{0, 0, 0, 1};      // from its place on the egg
    Vec3 spinAxis{0, 0, 1};
    float spin = 0;            // radians per second
    Vec3 normal{0, 0, 1};      // its outward direction on the egg (its own frame)
    float outDrop = 0, inDrop = 0;  // its ShardShape's
    Quat from, to;             // settling: eased from `from` to lying flat (`to`)
    float settle = -1;         // seconds into settling (< 0: still flying)
    int bounces = 0;
};

struct ShellBurst {
    static constexpr float kHalfThick = 0.012f;  // the shell's half thickness (egg_model.py THICK / 2)
    static constexpr float kSettleTime = 0.25f;
    static constexpr float kVanishTime = 1.6f;

    Shard shard[kShards];
    bool active = false;
    float age = 0;
    float gone = 0;    // 0 .. 1: sinking away after the naming
    bool vanishing = false;
    BurstGround ground;

    // Bursts the egg whose base (its model's origin) stands at `base`, den space.
    void start(Vec3 base, const ShardShape* shapes, const BurstGround& g, Rng& rng);
    void update(float dt);
    void settleNow();   // skipped: every piece where it lands, lying flat
    void vanish() { vanishing = true; }
    bool allSettled() const;
    // Piece i's transform (den space) for skinning: times its bone's inverse rest matrix.
    Mat34 transform(int i) const;
};

// The hatchling taking shape where the egg was, `t` seconds after the burst: a small white
// blob swells (the model scaled up from its feet), then shapes itself into the baby (the
// dragon shader's morph, eased with a little overshoot) and its colours come in. Exactly
// scale 1 and morph 1 once it's done: the dragon as it is.
constexpr float kBlobSwell = 0.35f;
constexpr float kBlobShape = 1.3f;
struct BlobShape {
    float scale = 1;
    float morph = 1;
};
BlobShape blobAt(float t);

}  // namespace ec
