// The egg (WP2): romfs/models/egg.ecm (tools/blender/egg_model.py) has two bones, "root"
// and the "cap" that pops off when it hatches. Its motion is procedural: rubbing rocks it,
// the dragon inside knocks now and then as hatching nears, cracks glow through the shell,
// and the cap lifts off. Pure logic (PC-tested); src/app/render3d.cpp draws it.
#pragma once

#include "core/dragon.hpp"
#include "core/math3d.hpp"
#include "core/model.hpp"
#include "core/rng.hpp"

namespace ec {

constexpr float kEggPivot = 0.3f;  // rocking pivot height: about the centre of the bottom's curve
constexpr int kEggCracks = 3;

struct EggMotion {
    float rock = 0;     // rocking amplitude, radians (dies away)
    float phase = 0;    // rocking phase, radians
    float axis = 0;     // rocking axis, radians about Z (0: about X, rocking toward the camera)
    float capLift = 0;  // 0 closed .. 1 off (the hatching cinematic drives it)
    float knockIn = 3;  // seconds until the dragon inside may knock again

    // A stylus stroke: amount ~ its length in pixels / 100; (dx, dy) its direction on
    // screen. Sideways strokes rock the egg side to side.
    void rub(float amount, float dx, float dy);
    void knock(float strength, float axisAngle);
    // Rocks on. Past 60% incubation the dragon inside knocks now and then, more often as
    // hatching nears. Returns true on a knock (for a sound).
    bool update(float dt, float progress, Rng& rng);
    float angle() const;
};

// Skin matrices for the egg's two bones (root, cap), in the egg's model space: rocking about
// the pivot, the cap lifting and tipping back on a hinge at its back edge.
void eggSkin(const ModelData& egg, const EggMotion& m, Mat34 skin[2]);

// Incubation progress 0..1, and how many cracks show (0..3) in the last stretch.
float eggProgress(const Dragon& d);
int eggCracks(const Dragon& d);

// The egg's colour for each palette slot and each slot's glow (the shader's emissive scale):
// the shell tinted by its breed, speckles, cracks (invisible until they open, then
// glowing), the inside of the shell, and the light inside, brighter as it warms.
void eggPalette(const Dragon& d, float pulse, Rgb out[kPalCount], float glow[kPalCount]);

}  // namespace ec
