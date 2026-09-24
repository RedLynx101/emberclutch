#include "core/particles.hpp"

#include <cmath>

namespace ec {
namespace {

constexpr float kGravity = 6.0f;  // crumbs (adult units / s^2, x scale)

bool ambientKind(Fx k) { return k == Fx::Ember || k == Fx::Mote || k == Fx::Glint || k == Fx::Puff; }

}  // namespace

float Particle::alpha() const {
    const float x = age / life;
    const float in = kind == Fx::Mote ? 0.25f : 0.08f;
    if (x < in) return x / in;
    const float out = kind == Fx::Glint ? 0.5f : 0.35f;  // fades over the last part of its life
    return x > 1 - out ? (1 - x) / out : 1.0f;
}

float Particle::sizeNow() const {
    const float x = age / life;
    switch (kind) {
        case Fx::Heart: return size * (x < 0.2f ? 0.5f + 2.5f * x : 1.0f);
        case Fx::Zzz: return size * (0.55f + 0.45f * x);
        case Fx::Puff: return size * (0.4f + 0.6f * std::sqrt(x));
        case Fx::Glint: return size * std::sin(3.14159f * x);  // twinkle: grows and shrinks
        default: return size;
    }
}

void Particles::emit(Fx kind, Vec3 at, int count, float scale) {
    for (int i = 0; i < count; ++i) {
        int slot = count_;
        if (count_ >= kMax) {
            if (ambientKind(kind)) return;
            slot = -1;
            for (int j = 0; j < count_ && slot < 0; ++j)
                if (ambientKind(p_[j].kind)) slot = j;
            if (slot < 0) return;
        } else {
            ++count_;
        }
        Particle& p = p_[slot];
        p = Particle{};
        p.kind = kind;
        p.seed = static_cast<u8>(rng_.next());
        p.pos = at;
        const float s = scale;
        switch (kind) {
            case Fx::Ember:
                p.pos = at + Vec3{spread() * 0.35f, spread() * 0.35f, 0.25f};
                p.vel = {spread() * 0.15f, spread() * 0.15f, 0.7f + 0.4f * unit()};
                p.life = 1.4f + 1.2f * unit();
                p.size = 0.05f + 0.04f * unit();
                break;
            case Fx::Mote:
                p.vel = {spread() * 0.06f, spread() * 0.06f, spread() * 0.04f};
                p.life = 4.0f + 3.0f * unit();
                p.size = 0.03f + 0.025f * unit();
                break;
            case Fx::Glint:
                p.life = 0.5f + 0.3f * unit();
                p.size = 0.12f + 0.08f * unit();
                break;
            case Fx::Heart:
                p.pos = at + Vec3{spread() * 0.2f * s, spread() * 0.2f * s, 0.1f * s};
                p.vel = Vec3{spread() * 0.15f, spread() * 0.15f, 0.8f} * s;
                p.life = 1.4f;
                p.size = 0.24f * s;
                break;
            case Fx::Zzz:
                p.pos = at + Vec3{0, 0, 0.15f * s};
                p.vel = Vec3{0.18f, -0.07f, 0.4f} * s;  // drifts up and to screen-right
                p.life = 2.4f;
                p.size = 0.32f * s;
                break;
            case Fx::Crumb:
                p.vel = Vec3{spread() * 0.7f, spread() * 0.7f, 0.6f + 0.8f * unit()} * s;
                p.life = 1.0f + 0.3f * unit();
                p.size = 0.05f * s;
                break;
            case Fx::Sparkle:
                p.pos = at + Vec3{spread() * 0.8f * s, spread() * 0.8f * s, spread() * 0.5f * s};
                p.vel = Vec3{spread() * 0.2f, spread() * 0.2f, 0.3f + 0.3f * unit()} * s;
                p.life = 0.7f + 0.3f * unit();
                p.size = 0.16f * s;
                break;
            case Fx::Puff:
                p.pos = at + Vec3{spread() * 0.3f * s, spread() * 0.3f * s, 0.05f};
                p.vel = Vec3{spread() * 0.35f, spread() * 0.35f, 0.15f} * s;
                p.life = 0.6f + 0.2f * unit();
                p.size = 0.4f * s;
                break;
            case Fx::Count: break;
        }
        // Crumbs remember their scale for gravity through their size.
    }
}

void Particles::update(float dt) {
    for (int i = 0; i < count_;) {
        Particle& p = p_[i];
        p.age += dt;
        if (p.age >= p.life) {
            p_[i] = p_[--count_];  // order does not matter
            continue;
        }
        switch (p.kind) {
            case Fx::Ember:  // rises, wavering
                p.vel.x += std::sin(p.age * 5.0f + p.seed) * 0.6f * dt;
                break;
            case Fx::Mote:  // drifts lazily
                p.vel.x += std::sin(p.age * 0.9f + p.seed) * 0.02f * dt;
                p.vel.y += std::cos(p.age * 0.7f + p.seed) * 0.02f * dt;
                break;
            case Fx::Crumb: {
                const float scale = p.size / 0.05f;
                p.vel.z -= kGravity * scale * dt;
                break;
            }
            case Fx::Puff:
                p.vel = p.vel * std::exp(-3.0f * dt);
                break;
            default: break;
        }
        p.pos = p.pos + p.vel * dt;
        if (p.kind == Fx::Crumb && p.pos.z < 0) {  // bounce once, then settle
            p.pos.z = 0;
            p.vel = {p.vel.x * 0.4f, p.vel.y * 0.4f, -p.vel.z * 0.3f};
        }
        ++i;
    }
}

void DenAmbience::update(Particles& fx, const DenLayout& den, float daylight, float dt) {
    auto unit = [this] { return rng.next() * (1.0f / 4294967296.0f); };
    // Embers: a steady trickle, livelier at night when the fire is the main light.
    ember += dt * (4.0f + 3.0f * (1 - daylight));
    for (; ember >= 1; ember -= 1) fx.emit(Fx::Ember, {den.hearth.x, den.hearth.y, 0}, 1);
    // Motes: only where the sunlight is.
    mote += dt * 3.0f * daylight;
    for (; mote >= 1; mote -= 1) {
        const float t = 0.15f + 0.8f * unit();
        const Vec3 floor{den.sunSpot.x, den.sunSpot.y, 0};
        const Vec3 at = den.skylight + (floor - den.skylight) * t;
        fx.emit(Fx::Mote, at + Vec3{(unit() - 0.5f) * 1.6f, (unit() - 0.5f) * 0.8f, 0}, 1);
    }
    // Glints: the hoard catches the light now and then.
    glint += dt * (0.8f + 1.2f * daylight) * (1.0f + hoard);
    for (; glint >= 1; glint -= 1) {
        const float a = unit() * 6.2832f, r = std::sqrt(unit());
        fx.emit(Fx::Glint, {den.hoard.x + std::cos(a) * r * 1.2f, den.hoard.y + std::sin(a) * r * 0.8f, 0.3f + 0.25f * (1 - r)}, 1);
    }
}

}  // namespace ec
