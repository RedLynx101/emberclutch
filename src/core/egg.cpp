#include "core/egg.hpp"

#include <cmath>

#include "core/genetics.hpp"
#include "core/kinds.hpp"

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
    const float open = std::fmin(1.0f, m.capLift), gone = std::fmax(0.0f, std::fmin(1.0f, m.capLift - 1.0f));
    const float s = 1.0f - gone;  // flying off, it shrinks away (into the sparkles)
    const Quat tip = quatAxisAngle({1, 0, 0}, -0.9f * open - 1.4f * gone);
    const Vec3 up{0, 0, 0.45f * open + 0.9f * gone};
    const Mat34 lift = fromQuatScale(tip, {s, s, s}, hinge + up - rotate(tip, hinge) * s);
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

float eggYaw(const Dragon& d) { return static_cast<float>((d.id * 40503u + 17u) % 360u) * 0.0174533f; }

int eggCracks(const Dragon& d) {
    const float p = eggProgress(d);
    return p >= 0.985f ? 3 : (p >= 0.93f ? 2 : (p >= 0.85f ? 1 : 0));
}

void eggPalette(const Dragon& d, float pulse, Rgb out[kPalCount], float glow[kPalCount], float t) {
    // DR3: its kind's egg (the shell and its markings, in the colouring it was laid in: a rare
    // egg looks it) and the kind's elements' light inside, drifting between two.
    const KindInfo& ki = kindInfo(d.kind < kindCount() ? d.kind : 0);
    Rgb light = elementGlow(ki.elements[0]);
    if (ki.elementCount > 1) light = mixRgb(light, elementGlow(ki.elements[1]), 0.5f + 0.5f * std::sin(t * 0.9f));
    // Each element's shell (WP12): its own tint and speckles; a hybrid's speckles are its
    // second element's, so the shell hints at both.
    static constexpr Rgb kShell[kElementCount] = {
        {252, 236, 214},  // Ember: warm cream
        {226, 244, 240},  // Tide: pale sea-glass
        {236, 243, 252},  // Gale: sky white
        {232, 238, 210},  // Grove: sage cream
        {240, 236, 250},  // Frost: frosted lavender
        {252, 244, 218},  // Lumen: gold cream
    };
    static constexpr Rgb kSpeckle[kElementCount] = {
        {150, 62, 34}, {38, 116, 128}, {92, 128, 198}, {92, 100, 52}, {146, 136, 206}, {196, 148, 58},
    };
    (void)kShell;
    const KindVariant& kv = ki.variants[d.variant % kKindVariants];
    const Rgb shell = kv.egg[0];
    const float warm = d.warmth / 100.0f;
    for (int i = 0; i < kPalCount; ++i) {
        out[i] = shell;
        glow[i] = 0;
    }
    (void)kSpeckle;
    out[kPalAccent] = kv.egg[1];  // its markings
    out[kPalIris] = mixRgb(shell, {200, 170, 150}, 0.35f);       // the inside of the shell
    out[kPalGlow] = mixRgb(shell, light, 0.3f + 0.7f * warm);    // the light inside
    glow[kPalGlow] = (0.2f + 0.6f * warm + 0.2f * eggProgress(d)) * pulse;
    // Cracks: shell-coloured and dark until they open, then light leaks through. The glow
    // doubles their colour on screen, so it stays the light's own hue, not white.
    const Rgb crack = mixRgb(light, {255, 230, 170}, 0.2f);
    const Rgb crackDim{static_cast<u8>(crack.r * 0.8f), static_cast<u8>(crack.g * 0.8f),
                       static_cast<u8>(crack.b * 0.8f)};
    const int open = eggCracks(d);
    // Which crack opens first, second and third is the egg's own (run 19: always the same ones);
    // the renderer turns each egg its own way round too, so they open anywhere on the shell.
    static const u8 kOrders[6][kEggCracks] = {{kPalPattern, kPalHorn, kPalMembrane}, {kPalPattern, kPalMembrane, kPalHorn},
                                             {kPalHorn, kPalPattern, kPalMembrane}, {kPalHorn, kPalMembrane, kPalPattern},
                                             {kPalMembrane, kPalPattern, kPalHorn}, {kPalMembrane, kPalHorn, kPalPattern}};
    const u8* slots = kOrders[(d.id * 2654435761u >> 16) % 6];
    for (int k = 0; k < kEggCracks; ++k)
        if (k < open) {
            out[slots[k]] = crackDim;
            glow[slots[k]] = pulse;
        } else if (d.variant == ki.rareVariant) {  // a rare colouring's egg: a faint glowing crack pattern from the start
            out[slots[k]] = mixRgb(shell, crack, 0.45f);
            glow[slots[k]] = 0.3f * pulse;
        }
}

}  // namespace ec
