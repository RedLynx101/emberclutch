// Dragon rig helpers: growth -> bone scales and idle pose, blending the baked part keys, and
// CPU skinning (for tests and for the ground offset). Mirrors tools/blender/dragon_model.py.
#pragma once

#include "core/model.hpp"

namespace ec {

constexpr int kBuildNeutral = -1;

// core bodyScale (0.25 hatchling .. 1 adult) -> rig growth t (0 .. 1).
float growthT(float bodyScale);

// Per-bone pose scales at growth t for a build (kBuildNeutral or genome Build 0..2).
void boneScales(const ModelData& m, float t, int build, Vec3* out);

// Idle pose at growth t: Euler table + the hatchling neck lift fading out, plus bone scales.
void idlePose(const ModelData& m, float t, int build, BonePose* out);

// Parts: piecewise-linear blend of the baked keys at growth t (normals renormalized).
void blendKeys(const MeshData& mesh, float t, Vec3* pos, Vec3* nrm);

// Skin one vertex position with the mesh's bone palette (2 weights).
Vec3 skinPoint(const MeshData& mesh, int vertex, Vec3 p, const Mat34* skin);

}  // namespace ec
