#include "core/skeleton.hpp"

#include <cstring>

namespace ec {

int Skeleton::find(const char* boneName) const {
    for (int i = 0; i < count; ++i)
        if (std::strncmp(name[i], boneName, sizeof(name[i])) == 0) return i;
    return -1;
}

void finalizeSkeleton(Skeleton& s) {
    for (int i = 0; i < s.count; ++i) {
        s.invRest[i] = inverseRigid(s.rest[i]);
        s.offs[i] = s.parent[i] < 0 ? s.rest[i] : mul(s.invRest[s.parent[i]], s.rest[i]);
    }
}

void evaluatePose(const Skeleton& s, const BonePose* pose, Mat34* poseMat, Mat34* skin) {
    for (int i = 0; i < s.count; ++i) {
        const Mat34 chan = fromQuatScale(pose[i].rot, pose[i].scale, Vec3{});
        const int p = s.parent[i];
        if (p < 0) {
            poseMat[i] = mul(s.rest[i], chan);
        } else {
            // Orientation: the parent's rotation without its scale (Blender orthogonalizes it).
            Mat34 parentRot = poseMat[p];
            normalizeColumns(parentRot);
            Mat34 m = mul(mul(parentRot, s.offs[i]), chan);
            // Position: the rest offset carried by the parent's full, scaled matrix.
            m.setTranslation(transformPoint(poseMat[p], s.offs[i].translation()));
            poseMat[i] = m;
        }
        skin[i] = mul(poseMat[i], s.invRest[i]);
    }
}

Mat34 bonePoseMatrix(const Skeleton& s, const BonePose* pose, int bone) {
    int chain[kMaxBones];
    int n = 0;
    for (int b = bone; b >= 0 && n < kMaxBones; b = s.parent[b]) chain[n++] = b;
    Mat34 m = Mat34::identity();
    for (int k = n - 1; k >= 0; --k) {  // from the root down, as evaluatePose does
        const int i = chain[k];
        const Mat34 chan = fromQuatScale(pose[i].rot, pose[i].scale, Vec3{});
        if (s.parent[i] < 0) {
            m = mul(s.rest[i], chan);
        } else {
            Mat34 parentRot = m;
            normalizeColumns(parentRot);
            Mat34 next = mul(mul(parentRot, s.offs[i]), chan);
            next.setTranslation(transformPoint(m, s.offs[i].translation()));
            m = next;
        }
    }
    return m;
}

}  // namespace ec
