// Skeleton evaluation that reproduces Blender's pose math for bones with scale inheritance
// off (inherit_scale = NONE), which is how growth stages scale the dragon:
//   * a child's joint position follows the parent's FULL pose matrix (scaled offsets), but
//   * its orientation ignores the parent's scale.
// Verified against Blender-deformed vertices in tests/test_model.cpp.
#pragma once

#include "core/math3d.hpp"
#include "core/types.hpp"

namespace ec {

constexpr int kMaxBones = 40;

struct Skeleton {
    u16 count = 0;
    char name[kMaxBones][16] = {};
    s8 parent[kMaxBones] = {};
    u8 flags[kMaxBones] = {};  // bit 0: wing bone
    Mat34 rest[kMaxBones];     // armature-space rest matrix (Blender bone.matrix_local)
    Mat34 invRest[kMaxBones];
    Mat34 offs[kMaxBones];     // rest relative to the parent's rest (root: rest)

    int find(const char* boneName) const;
};

// Derives invRest and offs from rest/parent. Bones must be ordered parents-first.
void finalizeSkeleton(Skeleton& s);

struct BonePose {
    Quat rot;                    // local rotation in the bone's rest frame
    Vec3 scale{1.0f, 1.0f, 1.0f};  // (girth x, length, girth z) — not inherited
};

// poseMat: armature-space bone matrices; skin: poseMat * invRest (what vertices use).
void evaluatePose(const Skeleton& s, const BonePose* pose, Mat34* poseMat, Mat34* skin);
// One bone's armature-space matrix, as evaluatePose gives it, from its chain alone (the
// look-at needs the head's, not all 37).
Mat34 bonePoseMatrix(const Skeleton& s, const BonePose* pose, int bone);

}  // namespace ec
