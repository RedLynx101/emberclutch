// Minimal 3D math for skinning (pure C++, used on the 3DS and in PC tests).
// Conventions match Blender: right-handed, Z up, column vectors (v' = M v), matrices stored
// row-major as m[row][col] with the translation in column 3.
#pragma once

#include <cmath>

namespace ec {

struct Vec2 {
    float x = 0, y = 0;
};

struct Vec3 {
    float x = 0, y = 0, z = 0;
};

inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline float length(Vec3 a) { return std::sqrt(dot(a, a)); }
inline Vec3 normalize(Vec3 a) {
    const float l = length(a);
    return l > 1e-12f ? a * (1.0f / l) : Vec3{0, 0, 1};
}
inline Vec3 lerp(Vec3 a, Vec3 b, float t) { return a + (b - a) * t; }

struct Quat {
    float x = 0, y = 0, z = 0, w = 1;
};

// Hamilton product: applying b, then a.
inline Quat mul(Quat a, Quat b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

inline Quat conjugate(Quat q) { return {-q.x, -q.y, -q.z, q.w}; }
inline float dot(Quat a, Quat b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
inline Quat normalize(Quat q) {
    const float l = std::sqrt(dot(q, q));
    return l > 1e-12f ? Quat{q.x / l, q.y / l, q.z / l, q.w / l} : Quat{};
}

// Normalized linear blend along the shorter arc (good enough between close rotations).
inline Quat nlerp(Quat a, Quat b, float t) {
    const float s = dot(a, b) < 0 ? -t : t;
    return normalize({a.x + (b.x * s - a.x * t), a.y + (b.y * s - a.y * t), a.z + (b.z * s - a.z * t),
                      a.w + (b.w * s - a.w * t)});
}

inline Quat quatAxisAngle(Vec3 axis, float radians) {
    const float s = std::sin(radians * 0.5f);
    return {axis.x * s, axis.y * s, axis.z * s, std::cos(radians * 0.5f)};
}

// The animation convention (tools/anim/eca.py): pitch + tips a bone up/forward (about -X),
// yaw + turns it to the dragon's left (about +Z), roll + leans it right (about -Y); roll is
// applied first, then pitch, then yaw. Radians, armature axes.
inline Quat quatFromPitchYawRoll(float pitch, float yaw, float roll) {
    Quat q = quatAxisAngle({0, -1, 0}, roll);
    q = mul(quatAxisAngle({-1, 0, 0}, pitch), q);
    return mul(quatAxisAngle({0, 0, 1}, yaw), q);
}

inline Vec3 rotate(Quat q, Vec3 v) {
    const Vec3 u{q.x, q.y, q.z};
    const Vec3 t = cross(u, v) * 2.0f;
    return v + t * q.w + cross(u, t);
}

// Blender's Euler 'XYZ' (X applied first) to quaternion, radians — same formula as eul_to_quat.
inline Quat quatFromEulerXYZ(float rx, float ry, float rz) {
    const float ci = std::cos(rx * 0.5f), si = std::sin(rx * 0.5f);
    const float cj = std::cos(ry * 0.5f), sj = std::sin(ry * 0.5f);
    const float ch = std::cos(rz * 0.5f), sh = std::sin(rz * 0.5f);
    const float cc = ci * ch, cs = ci * sh, sc = si * ch, ss = si * sh;
    return {cj * sc - sj * cs, cj * ss + sj * cc, cj * cs - sj * sc, cj * cc + sj * ss};
}

struct Mat34 {
    float m[3][4];

    static Mat34 identity() { return {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}}}; }
    Vec3 translation() const { return {m[0][3], m[1][3], m[2][3]}; }
    void setTranslation(Vec3 t) {
        m[0][3] = t.x;
        m[1][3] = t.y;
        m[2][3] = t.z;
    }
};

// a * b as 4x4 affine matrices (implicit last row 0 0 0 1).
inline Mat34 mul(const Mat34& a, const Mat34& b) {
    Mat34 r;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j];
        r.m[i][3] += a.m[i][3];
    }
    return r;
}

inline Vec3 transformPoint(const Mat34& a, Vec3 p) {
    return {a.m[0][0] * p.x + a.m[0][1] * p.y + a.m[0][2] * p.z + a.m[0][3],
            a.m[1][0] * p.x + a.m[1][1] * p.y + a.m[1][2] * p.z + a.m[1][3],
            a.m[2][0] * p.x + a.m[2][1] * p.y + a.m[2][2] * p.z + a.m[2][3]};
}

inline Vec3 transformDir(const Mat34& a, Vec3 d) {
    return {a.m[0][0] * d.x + a.m[0][1] * d.y + a.m[0][2] * d.z, a.m[1][0] * d.x + a.m[1][1] * d.y + a.m[1][2] * d.z,
            a.m[2][0] * d.x + a.m[2][1] * d.y + a.m[2][2] * d.z};
}

// [R(q) * diag(s) | t]
inline Mat34 fromQuatScale(Quat q, Vec3 s, Vec3 t) {
    const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    const float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    const float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    return {{{(1 - 2 * (yy + zz)) * s.x, 2 * (xy - wz) * s.y, 2 * (xz + wy) * s.z, t.x},
             {2 * (xy + wz) * s.x, (1 - 2 * (xx + zz)) * s.y, 2 * (yz - wx) * s.z, t.y},
             {2 * (xz - wy) * s.x, 2 * (yz + wx) * s.y, (1 - 2 * (xx + yy)) * s.z, t.z}}};
}

// The rotation of a pure rotation (+ translation) matrix as a quaternion.
inline Quat quatFromRotation(const Mat34& a) {
    const float m00 = a.m[0][0], m11 = a.m[1][1], m22 = a.m[2][2];
    const float trace = m00 + m11 + m22;
    Quat q;
    if (trace > 0) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        q = {(a.m[2][1] - a.m[1][2]) / s, (a.m[0][2] - a.m[2][0]) / s, (a.m[1][0] - a.m[0][1]) / s, 0.25f * s};
    } else if (m00 > m11 && m00 > m22) {
        const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
        q = {0.25f * s, (a.m[0][1] + a.m[1][0]) / s, (a.m[0][2] + a.m[2][0]) / s, (a.m[2][1] - a.m[1][2]) / s};
    } else if (m11 > m22) {
        const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
        q = {(a.m[0][1] + a.m[1][0]) / s, 0.25f * s, (a.m[1][2] + a.m[2][1]) / s, (a.m[0][2] - a.m[2][0]) / s};
    } else {
        const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
        q = {(a.m[0][2] + a.m[2][0]) / s, (a.m[1][2] + a.m[2][1]) / s, 0.25f * s, (a.m[1][0] - a.m[0][1]) / s};
    }
    return normalize(q);
}

// Inverse of a rotation + translation matrix (no scale).
inline Mat34 inverseRigid(const Mat34& a) {
    Mat34 r;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) r.m[i][j] = a.m[j][i];
    const Vec3 t = transformDir(r, a.translation());
    r.setTranslation(t * -1.0f);
    return r;
}

// Remove scale from the rotation part (unit-length axis columns).
inline void normalizeColumns(Mat34& a) {
    for (int c = 0; c < 3; ++c) {
        const float l = std::sqrt(a.m[0][c] * a.m[0][c] + a.m[1][c] * a.m[1][c] + a.m[2][c] * a.m[2][c]);
        if (l > 1e-12f)
            for (int r = 0; r < 3; ++r) a.m[r][c] /= l;
    }
}

}  // namespace ec
