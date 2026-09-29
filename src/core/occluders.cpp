#include "core/occluders.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "core/valley.hpp"

namespace ec {
namespace {

constexpr float kPi = 3.14159265f;

// Where the line p + d t (t in 0..1) crosses the triangle, either face (-1: it doesn't).
float crossing(Vec3 p, Vec3 d, Vec3 a, Vec3 b, Vec3 c) {
    const Vec3 e1 = b - a, e2 = c - a;
    const Vec3 h = cross(d, e2);
    const float det = dot(e1, h);
    if (std::fabs(det) < 1e-9f) return -1.0f;
    const float f = 1.0f / det;
    const Vec3 s = p - a;
    const float u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f) return -1.0f;
    const Vec3 q = cross(s, e1);
    const float w = f * dot(d, q);
    if (w < 0.0f || u + w > 1.0f) return -1.0f;
    const float t = f * dot(e2, q);
    return t >= 0.0f && t <= 1.0f ? t : -1.0f;
}

}  // namespace

bool occludes(const char* partName, u8 flags) {
    if (flags & 1) return false;  // (additive: a glow over what's behind it)
    return std::strcmp(partName, "lantern_light") != 0 && std::strcmp(partName, "glow") != 0 &&
           std::strcmp(partName, "sails") != 0;
}

void OccluderMesh::clear() {
    pos.clear();
    start.clear();
    tris.clear();
    nx = ny = 0;
}

void OccluderMesh::add(const float* xyz, const u16* idx, int indexCount) {
    for (int i = 0; i + 2 < indexCount; i += 3)
        for (int k = 0; k < 3; ++k) {
            const float* v = xyz + 3 * idx[i + k];
            pos.push_back({v[0], v[1], v[2]});
        }
}

void OccluderMesh::bin() {
    if (pos.empty()) return;
    Vec2 lo2{1e9f, 1e9f}, hi2{-1e9f, -1e9f};
    for (const Vec3& p : pos) {
        lo2 = {std::fmin(lo2.x, p.x), std::fmin(lo2.y, p.y)};
        hi2 = {std::fmax(hi2.x, p.x), std::fmax(hi2.y, p.y)};
    }
    lo = lo2;
    nx = std::max(1, static_cast<int>(std::ceil((hi2.x - lo2.x) / cell)) + 1);
    ny = std::max(1, static_cast<int>(std::ceil((hi2.y - lo2.y) / cell)) + 1);
    const int count = static_cast<int>(pos.size() / 3);
    std::vector<std::vector<u16>> cells(std::size_t(nx) * ny);
    for (int t = 0; t < count; ++t) {
        const Vec3 &a = pos[t * 3], &b = pos[t * 3 + 1], &c = pos[t * 3 + 2];
        const int x0 = static_cast<int>((std::fmin(a.x, std::fmin(b.x, c.x)) - lo.x) / cell);
        const int x1 = static_cast<int>((std::fmax(a.x, std::fmax(b.x, c.x)) - lo.x) / cell);
        const int y0 = static_cast<int>((std::fmin(a.y, std::fmin(b.y, c.y)) - lo.y) / cell);
        const int y1 = static_cast<int>((std::fmax(a.y, std::fmax(b.y, c.y)) - lo.y) / cell);
        for (int y = std::max(0, y0); y <= std::min(ny - 1, y1); ++y)
            for (int x = std::max(0, x0); x <= std::min(nx - 1, x1); ++x) cells[std::size_t(y) * nx + x].push_back(static_cast<u16>(t));
    }
    start.assign(cells.size() + 1, 0);
    tris.clear();
    for (std::size_t i = 0; i < cells.size(); ++i) {
        start[i] = static_cast<u32>(tris.size());
        tris.insert(tris.end(), cells[i].begin(), cells[i].end());
    }
    start.back() = static_cast<u32>(tris.size());
}

float OccluderMesh::firstHit(Vec3 a, Vec3 b) const {
    if (pos.empty() || start.empty()) return 1.0f;
    const Vec3 d = b - a;
    const int x0 = std::max(0, static_cast<int>((std::fmin(a.x, b.x) - lo.x) / cell));
    const int x1 = std::min(nx - 1, static_cast<int>((std::fmax(a.x, b.x) - lo.x) / cell));
    const int y0 = std::max(0, static_cast<int>((std::fmin(a.y, b.y) - lo.y) / cell));
    const int y1 = std::min(ny - 1, static_cast<int>((std::fmax(a.y, b.y) - lo.y) / cell));
    float best = 1.0f;
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            const std::size_t c = std::size_t(y) * nx + x;
            for (u32 i = start[c]; i < start[c + 1]; ++i) {
                const int t = tris[i];
                const float hit = crossing(a, d, pos[t * 3], pos[t * 3 + 1], pos[t * 3 + 2]);
                if (hit >= 0.0f && hit < best) best = hit;
            }
        }
    return best;
}

Vec3 clearEye(const Valley& v, const std::vector<PlacedOccluders>& places, Vec3 pivot, Vec3 eye, float margin, float spread,
              float nearest, float room) {
    const Vec3 way = eye - pivot;
    const float len = length(way);
    if (len < 1e-3f) return eye;
    const Vec3 dir = way * (1.0f / len);
    Vec3 side = cross(dir, Vec3{0, 0, 1});
    side = length(side) > 1e-4f ? normalize(side) : Vec3{1, 0, 0};
    const Vec3 up = cross(side, dir);
    const Vec3 offsets[5] = {{0, 0, 0}, side * spread, side * -spread, up * spread, up * -spread};
    float share = 1.0f;
    for (const Vec3& off : offsets) {
        // (each line from near the pivot out past the eye by the margin: the near plane's corners)
        const Vec3 a = pivot + off * 0.35f, b = eye + off + dir * margin;
        const float span = length(b - a);
        float s = 1.0f;
        for (const PlacedOccluders& p : places) {
            if (!p.mesh || p.mesh->empty()) continue;
            const Vec2 mid{(a.x + b.x) * 0.5f - p.at.x, (a.y + b.y) * 0.5f - p.at.y};
            if (std::hypot(mid.x, mid.y) > p.reach + span) continue;
            // (valley to place frame: placeToWorld's front and right, undone)
            const Vec2 f{std::sin(p.heading), -std::cos(p.heading)}, r{f.y, -f.x};
            auto local = [&](Vec3 w) {
                const float x = w.x - p.at.x, y = w.y - p.at.y;
                return Vec3{x * r.x + y * r.y, x * f.x + y * f.y, w.z - p.at.z};
            };
            s = std::fmin(s, p.mesh->firstHit(local(a), local(b)));
        }
        for (int k = 1; k <= 16; ++k) {  // the ground: a cliff, a rim, a hillside's cave
            const float f = k / 16.0f;
            if (f >= s) break;
            const Vec3 q = a + (b - a) * f;
            if (v.heightAt(q.x, q.y) + 0.35f > q.z) {
                s = (k - 1) / 16.0f;
                break;
            }
        }
        // back to a share of pivot -> eye (the line ran from near the pivot to past the eye)
        const float along = dot(a + (b - a) * s - pivot, dir) - margin;
        share = std::fmin(share, along / len);
    }
    share = std::fmax(share, std::fmin(1.0f, nearest / len));
    // Room round the eye too: a wall close beside or behind it (the ground rising at the eye's
    // height, a place's triangles within `room` metres) fills the frame's edges, so it comes in
    // further, into the open the pivot stands in.
    auto roomy = [&](Vec3 e) {
        for (int k = 0; k < 8; ++k) {
            const float a = k * (kPi / 4);
            const Vec3 q = e + Vec3{std::cos(a), std::sin(a), 0} * room;
            if (v.heightAt(q.x, q.y) > e.z - 0.3f) return false;
            for (const PlacedOccluders& p : places) {
                if (!p.mesh || p.mesh->empty() || std::hypot(e.x - p.at.x, e.y - p.at.y) > p.reach + room) continue;
                const Vec2 f{std::sin(p.heading), -std::cos(p.heading)}, r{f.y, -f.x};
                auto local = [&](Vec3 w) {
                    const float x = w.x - p.at.x, y = w.y - p.at.y;
                    return Vec3{x * r.x + y * r.y, x * f.x + y * f.y, w.z - p.at.z};
                };
                if (p.mesh->firstHit(local(e), local(q)) < 1.0f) return false;
            }
        }
        return true;
    };
    while (share > nearest / len + 1e-4f && !roomy(pivot + way * share)) share = std::fmax(nearest / len, share - 0.6f / len);
    Vec3 out = pivot + way * share;
    // In the tightest spots (a cave, a corner) it rises to look down past the pivot, if nothing's
    // overhead in the way.
    const float d = share * len;
    if (d < 3.0f) {
        Vec3 lifted = out + Vec3{0, 0, 0.5f * (3.0f - d)};
        bool open = v.heightAt(lifted.x, lifted.y) + 0.35f < lifted.z;
        for (const PlacedOccluders& p : places) {
            if (!open || !p.mesh || p.mesh->empty() || std::hypot(lifted.x - p.at.x, lifted.y - p.at.y) > p.reach + 3.0f) continue;
            const Vec2 f{std::sin(p.heading), -std::cos(p.heading)}, r{f.y, -f.x};
            auto local = [&](Vec3 w) {
                const float x = w.x - p.at.x, y = w.y - p.at.y;
                return Vec3{x * r.x + y * r.y, x * f.x + y * f.y, w.z - p.at.z};
            };
            open = p.mesh->firstHit(local(pivot), local(lifted + Vec3{0, 0, margin})) >= 1.0f;
        }
        if (open) out = lifted;
    }
    return share >= 1.0f && d >= 3.0f ? eye : out;
}

}  // namespace ec
