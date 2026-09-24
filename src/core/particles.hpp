// Den particles (WP6): embers over the hearth, motes in the sunbeam, glints on the hoard,
// and the care effects (hearts, Zzz, crumbs, sparkles, dust puffs). Pure simulation in den
// space (adult units, Z up); src/app/render3d.cpp projects and draws them. Fixed pool.
#pragma once

#include "core/behavior.hpp"
#include "core/math3d.hpp"
#include "core/rng.hpp"

namespace ec {

enum class Fx : u8 { Ember, Mote, Glint, Heart, Zzz, Crumb, Sparkle, Puff, Count };

struct Particle {
    Vec3 pos, vel;
    float age = 0, life = 1;
    float size = 0.1f;  // world size at full growth (hearts, Zzz and puffs grow into it)
    Fx kind = Fx::Ember;
    u8 seed = 0;        // per-particle variety (flicker phase, tint)

    float alpha() const;  // fades in quickly and out over the last part of its life
    float sizeNow() const;
};

class Particles {
public:
    static constexpr int kMax = 96;

    void clear() { count_ = 0; }
    // Emits `count` particles of a kind around `at`. scale: the emitting dragon's size
    // (1 = adult). When the pool is full, care effects replace ambient ones; ambient
    // particles are dropped.
    void emit(Fx kind, Vec3 at, int count, float scale = 1.0f);
    void update(float dt);
    int count() const { return count_; }
    const Particle& operator[](int i) const { return p_[i]; }
    // Drawn over the dragons (the rest are drawn between the room and the dragons).
    static bool foreground(Fx k) { return k == Fx::Heart || k == Fx::Zzz || k == Fx::Crumb || k == Fx::Sparkle; }

private:
    Particle p_[kMax];
    int count_ = 0;
    Rng rng_{0x5EEDu};
    float unit() { return rng_.next() * (1.0f / 4294967296.0f); }
    float spread() { return unit() * 2 - 1; }
};

// The den's own emitters: embers over the hearth, motes drifting in the sunbeam (by day)
// and glints twinkling on the hoard.
struct DenAmbience {
    float ember = 0, mote = 0, glint = 0;  // emission accumulators
    Rng rng{0xA1B2u};
    // daylight: 1 at noon .. 0 at night (the day set's weight, core/daylight).
    void update(Particles& fx, const DenLayout& den, float daylight, float dt);
};

}  // namespace ec
