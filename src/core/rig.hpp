// Dragon rig helpers: growth -> bone scales and idle pose, blending the baked part keys, and
// CPU skinning (for tests and for the ground offset). Mirrors tools/blender/dragon_model.py.
#pragma once

#include "core/dragon.hpp"
#include "core/model.hpp"

namespace ec {

constexpr int kBuildNeutral = -1;

// Two body forms (D36): the baby form for the hatchling stage (romfs/models/hatchling.ecm)
// and the grown form from juvenile to adult (grown.ecm). The stage-up to juvenile swaps
// them behind a glow: the first molt.
enum ModelForm : u8 { kFormHatchling, kFormGrown, kFormCount };

struct Growth {
    ModelForm form;
    float t;  // growth within the form, 0 .. 1
};

// Grown-form t where adolescence starts (tools/blender/dragon_model.py STAGE table).
constexpr float kAdolescentT = 0.45f;

// Stage + progress through it (dragon.hpp stageProgress) -> form and growth t.
Growth growthFor(Stage stage, float progress);

// The dragon's size relative to an adult (0.25 .. 1): walking speeds, step lengths and hop
// heights scale with it. A hatchling is ~0.4 of an adult's length at the end of its stage.
float growthScale(const Growth& g);

// Per-bone pose scales at growth t for a build (kBuildNeutral or genome Build 0..2).
void boneScales(const ModelData& m, float t, int build, Vec3* out);

// Idle pose at growth t: Euler table + the hatchling neck lift fading out, plus bone scales.
void idlePose(const ModelData& m, float t, int build, BonePose* out);

// Parts: piecewise-linear blend of the baked keys at growth t (normals renormalized).
void blendKeys(const MeshData& mesh, float t, Vec3* pos, Vec3* nrm);

// Skin one vertex position with the mesh's bone palette (2 weights).
Vec3 skinPoint(const MeshData& mesh, int vertex, Vec3 p, const Mat34* skin);

}  // namespace ec
