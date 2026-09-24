#include "core/egg.hpp"

#include <cmath>

#include "core/genetics.hpp"

namespace ec {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kRockHz = 2.4f;     // a rocking egg's swing
constexpr float kMaxRock = 0.3f;    // radians
constexpr float kCapHinge = 0.25f;  // the cap tips back on a hinge this far behind the axis

Rgb mixRgb(Rgb a, Rgb b, float t) {
    return {static_cast<u8>(a.r + (b.r - a.r) * t), static_cast<u8>(a.g + (b.g - a.g) * t),
            static_cast<u8>(a.b + (b.b - a.b) * t)};
}

float unit(Rng& r) { return r.next() * (1.0f / 4294967296.0f); }

}  // namespace

void EggMotion::rub(float amount, float dx, float dy) {
    axis = std::fabs(dx) > std::fabs(dy) ? kPi * 0.5f : 0.0f;
    rock = std::fmin(kMaxRock, rock + amount * 0.06f);
}

void EggMotion::knock(float strength, float axisAngle) {
    axis = axisAngle;
    rock = std::fmin(kMaxRock, rock + strength);
    phase = 0;
}

bool EggMotion::update(float dt, float progress, Rng& rng) {
    phase = std::fmod(phase + dt * 2 * kPi * kRockHz, 2 * kPi);
    rock *= std::exp(-2.2f * dt);
    yaw += (yawGoal - yaw) * std::fmin(1.0f, dt * 4.0f);
    if (yawGoal > 2 * kPi && yaw > 2 * kPi) {  // keep the numbers small
        yaw -= 2 * kPi;
        yawGoal -= 2 * kPi;
    }
    if (progress <= 0.6f) return false;
    if ((knockIn -= dt) > 0) return false;
    knockIn = (1.5f + 4.0f * unit(rng)) * (1.6f - progress);  // livelier near the end
    knock(0.06f + 0.1f * progress, unit(rng) * 2 * kPi);
    return true;
}

void EggMotion::turn() {
    yawGoal += kPi * 0.5f;
    knock(0.05f, 0.0f);  // it wobbles as it settles
}

float EggMotion::angle() const { return rock * std::sin(phase); }

void eggSkin(const ModelData& egg, const EggMotion& m, Mat34 skin[2]) {
    const Vec3 one{1, 1, 1};
    // Spin about its own axis (which passes through the pivot), then rock.
    const Quat q = mul(quatAxisAngle({std::cos(m.axis), std::sin(m.axis), 0}, m.angle()), quatAxisAngle({0, 0, 1}, m.yaw));
    const Vec3 pivot{0, 0, kEggPivot};
    skin[0] = fromQuatScale(q, one, pivot - rotate(q, pivot));
    const int cap = egg.skel.find("cap");
    const float seam = cap >= 0 ? egg.skel.rest[cap].translation().z : 0.63f;
    const Vec3 hinge{0, kCapHinge, seam};
    const Quat tip = quatAxisAngle({1, 0, 0}, -0.9f * m.capLift);
    const Mat34 lift = fromQuatScale(tip, one, hinge + Vec3{0, 0, 0.45f * m.capLift} - rotate(tip, hinge));
    skin[1] = mul(skin[0], lift);
}

Heartbeat heartbeatOf(const Dragon& d) {
    static constexpr float kBpm[] = {96, 128, 140, 84, 66, 116};  // Brave Shy Playful Proud Sleepy Curious
    static_assert(sizeof(kBpm) / sizeof(kBpm[0]) == static_cast<int>(Personality::Count), "one per personality");
    const float p = eggProgress(d);
    Heartbeat h;
    const float pace = kBpm[static_cast<int>(temperamentOf(d))];
    h.bpm = 58 + (pace - 58) * (p < 0.3f ? p / 0.3f : 1.0f);  // a young egg's heart is slow whatever it'll be
    h.strength = 0.25f + 0.75f * p;
    return h;
}

float eggProgress(const Dragon& d) {
    const float p = static_cast<float>(d.incubationSeconds) / kIncubationSeconds;
    return p < 0 ? 0 : (p > 1 ? 1 : p);
}

int eggCracks(const Dragon& d) {
    const float p = eggProgress(d);
    return p >= 0.985f ? 3 : (p >= 0.93f ? 2 : (p >= 0.85f ? 1 : 0));
}

void eggPalette(const Dragon& d, float pulse, Rgb out[kPalCount], float glow[kPalCount]) {
    const Rgb light = heartglowColor(static_cast<Element>(d.genome.elementA));
    const Rgb breed = hsvToRgb(d.genome.baseH, d.genome.baseS, d.genome.baseV);
    const Rgb shell = mixRgb({250, 240, 225}, breed, 0.16f);
    const float warm = d.warmth / 100.0f;
    for (int i = 0; i < kPalCount; ++i) {
        out[i] = shell;
        glow[i] = 0;
    }
    out[kPalAccent] = mixRgb(breed, {52, 35, 63}, 0.35f);        // speckles
    out[kPalIris] = mixRgb(shell, {200, 170, 150}, 0.35f);       // the inside of the shell
    out[kPalGlow] = mixRgb(shell, light, 0.3f + 0.7f * warm);    // the light inside
    glow[kPalGlow] = (0.2f + 0.6f * warm + 0.2f * eggProgress(d)) * pulse;
    // Cracks: shell-coloured and dark until they open, then light leaks through. The glow
    // doubles their colour on screen, so it stays the light's own hue, not white.
    const Rgb crack = mixRgb(light, {255, 230, 170}, 0.2f);
    const Rgb crackDim{static_cast<u8>(crack.r * 0.8f), static_cast<u8>(crack.g * 0.8f),
                       static_cast<u8>(crack.b * 0.8f)};
    const int open = eggCracks(d);
    const u8 slots[kEggCracks] = {kPalPattern, kPalHorn, kPalMembrane};
    for (int k = 0; k < kEggCracks; ++k)
        if (k < open) {
            out[slots[k]] = crackDim;
            glow[slots[k]] = pulse;
        }
}

}  // namespace ec
