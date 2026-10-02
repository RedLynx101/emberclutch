#include "core/wear_fit.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include "core/kinds.hpp"

namespace ec {
namespace {

constexpr Vec3 kUp{0, 0, 1}, kBack{0, 1, 0};
constexpr float kFar = 8.0f;  // rays start this far off (the kinds are 1-4 units long)
// Rays down the middle go a hair off it: a symmetric mesh has its seam there, and a ray right
// along an edge can slip between the two faces.
constexpr float kOffMid = 0.0023f;

// A small trim per plan, slot and form, in the slot's own units: moved (x across, y back, z up)
// and scaled. What measuring the body can't see: horns and crests on the skull, a ruff of fur.
struct Nudge {
    float dx = 0, dy = 0, dz = 0, scale = 1;
};
struct PlanNudges {
    const char* plan;
    Nudge slot[kWearSlots][2];  // [slot][hatchling, grown]
};
const PlanNudges kWearNudges[] = {
    {"pouncer", {{{}, {}}, {{}, {}}, {{}, {}}, {{}, {}}}},
    // (run 23: the grown Glimmermoth's hat sat small behind its brow between the antennae, out of
    // sight: a little up, over the brow; any larger and its brim would cover the eyes)
    {"glimmermoth", {{{}, {0, 0, 0.12f, 1.0f}}, {{}, {}}, {{}, {}}, {{}, {}}}},
};

const Nudge* nudgeFor(int plan, int slot, bool grown) {
    if (plan < 0 || plan >= planCount()) return nullptr;
    for (const PlanNudges& p : kWearNudges)
        if (std::strcmp(p.plan, planInfo(plan).name) == 0) return &p.slot[slot][grown ? 1 : 0];
    return nullptr;
}

bool startsWith(const char* s, const char* prefix) { return std::strncmp(s, prefix, std::strlen(prefix)) == 0; }

Vec3 jointOf(const ModelData& m, int bone) { return m.skel.rest[bone].translation(); }
Vec3 boneY(const ModelData& m, int bone) {
    const Mat34& r = m.skel.rest[bone];
    return normalize(Vec3{r.m[0][1], r.m[1][1], r.m[2][1]});
}

bool descends(const Skeleton& s, int bone, int ancestor) {
    for (int b = bone; b >= 0; b = s.parent[b])
        if (b == ancestor) return true;
    return false;
}

// A limb or wing bone: never part of the head, the neck, the back or the tail.
bool isLimb(const Skeleton& s, int bone) {
    const char* n = s.name[bone];
    return (s.flags[bone] & 1) || startsWith(n, "arm") || startsWith(n, "leg") || startsWith(n, "hand") ||
           startsWith(n, "foot") || startsWith(n, "wing");
}

// A frame's axes square to `up` with y toward `back` (both may be rough).
void axes(Vec3 up, Vec3 back, Vec3& ex, Vec3& ey, Vec3& ez) {
    ez = normalize(up);
    Vec3 y = back - ez * dot(back, ez);
    if (length(y) < 1e-3f) y = kBack - ez * dot(kBack, ez);
    ey = normalize(y);
    ex = cross(ey, ez);
}

Mat34 makeFrame(Vec3 origin, Vec3 ex, Vec3 ey, Vec3 ez, float sx, float sy, float sz) {
    Mat34 f;
    const Vec3 c[3] = {ex * sx, ey * sy, ez * sz};
    for (int k = 0; k < 3; ++k) {
        f.m[0][k] = c[k].x;
        f.m[1][k] = c[k].y;
        f.m[2][k] = c[k].z;
    }
    f.setTranslation(origin);
    return f;
}

int childNamed(const Skeleton& s, int parent, const char* prefix) {
    for (int i = 0; i < s.count; ++i)
        if (s.parent[i] == parent && startsWith(s.name[i], prefix)) return i;
    return -1;
}

// Rays against the body's triangles in its rest pose (the mesh is low: a vertex is seldom right
// where you'd measure, a face always is). A face counts if one of its corners follows a bone the
// filter takes.
struct Body {
    const MeshData& mesh;
    std::vector<int> bone;  // each vertex's skeleton bone (the one it follows most)

    explicit Body(const MeshData& m) : mesh(m), bone(m.vertexCount) {
        for (int v = 0; v < m.vertexCount; ++v) {
            const u8* w = &m.skin[std::size_t(v) * 4];
            bone[v] = m.palette[w[2] >= w[3] ? w[0] : w[1]];
        }
    }

    // How far along the ray the nearest face is (-1: none), and that face's first index.
    template <typename Filter>
    float cast(Vec3 o, Vec3 d, Filter take, int* hitFace = nullptr) const {
        float best = -1;
        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            const int ia = mesh.indices[i], ib = mesh.indices[i + 1], ic = mesh.indices[i + 2];
            if (!take(bone[ia]) && !take(bone[ib]) && !take(bone[ic])) continue;
            const Vec3 a = mesh.pos[ia], e1 = mesh.pos[ib] - a, e2 = mesh.pos[ic] - a;
            const Vec3 p = cross(d, e2);
            const float det = dot(e1, p);
            if (std::fabs(det) < 1e-12f) continue;
            const float inv = 1.0f / det;
            const Vec3 s = o - a;
            const float u = dot(s, p) * inv;
            if (u < 0 || u > 1) continue;
            const Vec3 q = cross(s, e1);
            const float v = dot(d, q) * inv;
            if (v < 0 || u + v > 1) continue;
            const float t = dot(e2, q) * inv;
            if (t > 1e-5f && (best < 0 || t < best)) {
                best = t;
                if (hitFace) *hitFace = static_cast<int>(i);
            }
        }
        return best;
    }
    // The skin's distance from `c` out along `dir` (found coming in from far off; -1: none).
    template <typename Filter>
    float surface(Vec3 c, Vec3 dir, Filter take, int* hitFace = nullptr) const {
        const float t = cast(c + dir * kFar, dir * -1.0f, take, hitFace);
        return t < 0 ? -1.0f : kFar - t;
    }
};

// The top of the kind's back plates or moss (its spike parts, either colouring, grown) straight
// down at (x, y); -1e9 where there are none.
float spikesTop(const ModelData& m, float x, float y) {
    float top = -1e9f;
    for (const MeshData& p : m.meshes) {
        if (p.kind != kMeshPart || p.group != kGroupSpikes || p.keyCount == 0) continue;
        const std::size_t key = std::size_t(p.keyCount - 1) * p.vertexCount;  // (the grown key)
        for (std::size_t i = 0; i + 2 < p.indices.size(); i += 3) {
            const Vec3 a = p.pos[key + p.indices[i]], b = p.pos[key + p.indices[i + 1]], c = p.pos[key + p.indices[i + 2]];
            // Is (x, y) inside the triangle seen from above? Then the height there.
            const float d = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
            if (std::fabs(d) < 1e-9f) continue;
            const float l1 = ((b.y - c.y) * (x - c.x) + (c.x - b.x) * (y - c.y)) / d;
            const float l2 = ((c.y - a.y) * (x - c.x) + (a.x - c.x) * (y - c.y)) / d;
            const float l3 = 1.0f - l1 - l2;
            if (l1 < 0 || l2 < 0 || l3 < 0) continue;
            top = std::fmax(top, l1 * a.z + l2 * b.z + l3 * c.z);
        }
    }
    return top;
}

// The girth round an axis at c: the skin's reach out along +-ex and +-ey from it (a missing side
// takes the other's), as half-widths and the middle's offset.
struct Girth {
    float rx = 0, ry = 0, cx = 0, cy = 0;
    bool ok = false;
};
template <typename Filter>
Girth girth(const Body& body, Vec3 c, Vec3 ex, Vec3 ey, Filter take) {
    float px = body.surface(c, ex, take), nx = body.surface(c, ex * -1.0f, take);
    const Vec3 off = c + ex * kOffMid;  // (up and down the middle: off the seam)
    float py = body.surface(off, ey, take), ny = body.surface(off, ey * -1.0f, take);
    Girth g;
    if ((px < 0 && nx < 0) || (py < 0 && ny < 0)) return g;
    if (px < 0) px = nx;
    if (nx < 0) nx = px;
    if (py < 0) py = ny;
    if (ny < 0) ny = py;
    g.rx = (px + nx) * 0.5f;
    g.ry = (py + ny) * 0.5f;
    g.cx = (px - nx) * 0.5f;
    g.cy = (py - ny) * 0.5f;
    g.ok = g.rx > 1e-3f && g.ry > 1e-3f;
    return g;
}

// Every eye vertex in a frame's units (x across, y back, z up about its middle).
void eyePoints(const ModelData& m, const Mat34& frame, std::vector<Vec3>& out) {
    out.clear();
    const int eyes = m.skel.find("eyes");
    if (eyes < 0) return;
    const Vec3 ex{frame.m[0][0], frame.m[1][0], frame.m[2][0]}, ey{frame.m[0][1], frame.m[1][1], frame.m[2][1]},
        ez{frame.m[0][2], frame.m[1][2], frame.m[2][2]}, o = frame.translation();
    const float uu = dot(ex, ex);
    if (uu < 1e-9f) return;
    for (const MeshData& md : m.meshes)
        for (int v = 0; v < md.vertexCount; ++v) {
            const u8* sk = &md.skin[std::size_t(v) * 4];
            if (sk[0] >= md.paletteCount || md.palette[sk[0]] != eyes || sk[2] < 128) continue;
            const Vec3 d = md.pos[std::size_t(v)] - o;
            out.push_back({dot(d, ex) / uu, dot(d, ey) / uu, dot(d, ez) / uu});
        }
}

// The head's own horns, frills and spikes (the parts' vertices on the head's bones, the form's
// key) in a frame's units: what a hat's crown shouldn't be pierced by (run 23).
void headPartPoints(const ModelData& m, int head, bool grown, const Mat34& frame, std::vector<Vec3>& out) {
    out.clear();
    const Vec3 ex{frame.m[0][0], frame.m[1][0], frame.m[2][0]}, ey{frame.m[0][1], frame.m[1][1], frame.m[2][1]},
        ez{frame.m[0][2], frame.m[1][2], frame.m[2][2]}, o = frame.translation();
    const float uu = dot(ex, ex);
    if (uu < 1e-9f) return;
    for (const MeshData& md : m.meshes) {
        if (md.kind != kMeshPart || (md.group != kGroupHorns && md.group != kGroupFrill && md.group != kGroupSpikes)) continue;
        const int key = md.keyCount > 1 ? (grown ? md.keyCount - 1 : 0) : 0;
        for (int v = 0; v < md.vertexCount; ++v) {
            const u8* sk = &md.skin[std::size_t(v) * 4];
            const int b = md.palette[sk[2] >= sk[3] ? sk[0] : sk[1]];
            if (!descends(m.skel, b, head)) continue;
            const Vec3 d = md.pos[std::size_t(key) * md.vertexCount + v] - o;
            out.push_back({dot(d, ex) / uu, dot(d, ey) / uu, dot(d, ez) / uu});
        }
    }
}

// How far up into a hat's crown (`radius` round its middle) the highest of them reaches above its
// base, lifted `lift` (0: clear; at most the crown's height).
float piercing(const std::vector<Vec3>& pts, float radius, float lift) {
    float worst = 0;
    for (const Vec3& p : pts)
        if (p.x * p.x + p.y * p.y < radius * radius) worst = std::fmax(worst, std::fmin(p.z - lift - kHatBrimZ, 0.9f));
    return worst;
}

// A pad worn on the head (the Lilyfin's lily pad: run 24, Noah: "Have the hats on Lilyfin (baby
// and adult) lay flat on top of their pad"): a wide part (horns) over the skull whose rim fits a
// plane. If there is one, `out` is a hat's frame sitting on it, level with it, in its middle.
bool padSeat(const ModelData& m, int head, bool grown, const Mat34& base, Mat34& out) {
    std::vector<Vec3> pts;  // the horns' points in the base frame's units (x across, y back, z up)
    const Vec3 bx{base.m[0][0], base.m[1][0], base.m[2][0]}, by{base.m[0][1], base.m[1][1], base.m[2][1]},
        bz{base.m[0][2], base.m[1][2], base.m[2][2]}, o = base.translation();
    const float uu = dot(bx, bx);
    if (uu < 1e-9f) return false;
    for (const MeshData& md : m.meshes) {
        if (md.kind != kMeshPart || md.group != kGroupHorns || md.variant != 0) continue;
        const int key = md.keyCount > 1 ? (grown ? md.keyCount - 1 : 0) : 0;
        for (int v = 0; v < md.vertexCount; ++v) {
            const u8* sk = &md.skin[std::size_t(v) * 4];
            if (!descends(m.skel, md.palette[sk[2] >= sk[3] ? sk[0] : sk[1]], head)) continue;
            const Vec3 d = md.pos[std::size_t(key) * md.vertexCount + v] - o;
            pts.push_back({dot(d, bx) / uu, dot(d, by) / uu, dot(d, bz) / uu});
        }
    }
    if (pts.size() < 12) return false;
    float cx = 0, cy = 0;
    for (const Vec3& p : pts) cx += p.x, cy += p.y;
    cx /= pts.size(), cy /= pts.size();
    float reach = 0;
    for (const Vec3& p : pts) reach = std::fmax(reach, std::hypot(p.x - cx, p.y - cy));
    if (reach < 0.45f) return false;  // (horns, a crest: nothing wide)
    // The rim (away from the middle, where a flower may stand): z = a x + b y + c, least squares.
    double s[3][4] = {};
    int n = 0;
    for (const Vec3& p : pts) {
        if (std::hypot(p.x - cx, p.y - cy) < 0.6f * reach) continue;
        const double r[3] = {p.x, p.y, 1.0};
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) s[i][j] += r[i] * r[j];
            s[i][3] += r[i] * p.z;
        }
        ++n;
    }
    if (n < 8) return false;
    for (int i = 0; i < 3; ++i) {  // (Gauss-Jordan, pivoting on the diagonal: a plane's sums are well posed)
        if (std::fabs(s[i][i]) < 1e-9) return false;
        for (int k = 0; k < 3; ++k) {
            if (k == i) continue;
            const double f = s[k][i] / s[i][i];
            for (int j = i; j < 4; ++j) s[k][j] -= f * s[i][j];
        }
    }
    const float a = static_cast<float>(s[0][3] / s[0][0]), b = static_cast<float>(s[1][3] / s[1][1]),
                c = static_cast<float>(s[2][3] / s[2][2]);
    double err = 0;
    for (const Vec3& p : pts)
        if (std::hypot(p.x - cx, p.y - cy) >= 0.6f * reach) err += (p.z - (a * p.x + b * p.y + c)) * (p.z - (a * p.x + b * p.y + c));
    const Vec3 up = normalize(Vec3{-a, -b, 1.0f});
    const float zc = a * cx + b * cy + c;
    if (std::sqrt(err / n) > 0.07f * reach || up.z < 0.8f || zc < -0.1f) return false;  // flat, level enough, over the skull
    // Its top there: the highest rim point's height over the plane, so the hat rests on the pad.
    float rise = 0;
    for (const Vec3& p : pts)
        if (std::hypot(p.x - cx, p.y - cy) >= 0.6f * reach) rise = std::fmax(rise, p.z - (a * p.x + b * p.y + c));
    const Vec3 at = o + bx * cx + by * cy + bz * (zc + std::fmin(rise, 0.08f) + 0.03f);
    const float unit = std::sqrt(uu);
    Vec3 ex, ey, ez;
    axes(normalize(bx * up.x + by * up.y + bz * up.z), by, ex, ey, ez);
    out = makeFrame(at, ex, ey, ez, unit, unit, unit);
    return true;
}

float highestThrough(const std::vector<Vec3>& pts, float backBy, float radius, float margin) {
    float worst = -1e9f;
    for (const Vec3& p : pts) {
        const float y = p.y - backBy;
        if (p.x * p.x + y * y < radius * radius) worst = std::fmax(worst, p.z + margin - kHatBrimZ);
    }
    return worst;
}

}  // namespace

float eyesThroughBrim(const ModelData& m, const Mat34& frame, float radius, float margin) {
    std::vector<Vec3> pts;
    eyePoints(m, frame, pts);
    return highestThrough(pts, 0.0f, radius, margin);
}

bool fitWear(const ModelData& m, int plan, bool grown, WearFit& out) {
    out = WearFit{};
    const Skeleton& s = m.skel;
    const MeshData* mesh = m.findMesh(kMeshBody, kGroupBody, 0);
    const int head = s.find("head"), chest = s.find("chest"), hips = s.find("hips");
    if (!mesh || head < 0 || chest < 0 || hips < 0) return false;
    int eyes = s.find("eyes");
    if (eyes < 0) eyes = s.find("snout");
    if (eyes < 0) return false;
    const Body body(*mesh);

    // ---- The head: the top of the skull a third of the way from the head's joint to the eyes,
    // and its width halfway down to the joint.
    const Vec3 h = jointOf(m, head), e = jointOf(m, eyes);
    const float yc = h.y + (e.y - h.y) * 0.3f;
    auto inHead = [&](int b) { return descends(s, b, head); };
    auto notLimb = [&](int b) { return !isLimb(s, b); };  // (the head's faces may lean on the neck's bones)
    float above = body.surface({kOffMid, yc, h.z}, {0, 0, 1}, inHead);
    if (above < 0) above = body.surface({kOffMid, yc, h.z}, {0, 0, 1}, notLimb);
    if (above < 0) return false;
    const float top = h.z + above;
    const float zw = (top + h.z) * 0.5f;
    auto halfWidth = [&](Vec3 at, auto take) {
        const float r = body.surface(at, {1, 0, 0}, take), l = body.surface(at, {-1, 0, 0}, take);
        return r > 0 && l > 0 ? (r + l) * 0.5f : std::fmax(r, l);
    };
    float headW = halfWidth(Vec3{0, yc, zw}, inHead);
    if (headW <= 1e-3f) headW = halfWidth(Vec3{0, yc, zw}, notLimb);
    if (headW <= 1e-3f) return false;
    {
        const Vec3 f = boneY(m, head);  // along the head toward the snout
        const Vec3 headUp = normalize(kUp - f * dot(f, kUp));
        Vec3 ex, ey, ez;
        axes(normalize(kUp + headUp), f * -1.0f, ex, ey, ez);
        const int k = static_cast<int>(WearSlot::Head);
        out.frame[k] = makeFrame({0, yc, top}, ex, ey, ez, headW, headW, headW);
        out.boneA[k] = out.boneB[k] = static_cast<s8>(head);
        out.ok[k] = true;
    }

    // ---- The neck: its girth 40% up its first bone from the chest.
    const int neck = childNamed(s, chest, "neck");
    if (neck >= 0) {
        int next = childNamed(s, neck, "neck");
        if (next < 0) next = childNamed(s, neck, "head");
        const Vec3 n0 = jointOf(m, neck), n1 = next >= 0 ? jointOf(m, next) : h;
        const float segLen = std::fmax(0.02f, length(n1 - n0));
        const Vec3 axis = normalize(n1 - n0);
        const Vec3 c = n0 + axis * (0.4f * segLen);
        Vec3 ex, ey, ez;
        axes(axis, kUp, ex, ey, ez);  // z up the neck, y its nape
        Girth g = girth(body, c, ex, ey, [&](int b) { return startsWith(s.name[b], "neck"); });
        if (!g.ok) g = girth(body, c, ex, ey, [&](int b) { return !isLimb(s, b) && !startsWith(s.name[b], "tail"); });
        // (a hatchling's neck is hardly there: kept between a third and a bit more than the head's width)
        const float rx = g.ok ? std::fmin(1.2f * headW, std::fmax(g.rx, 0.3f * headW)) : 0.7f * headW;
        const float ry = g.ok ? std::fmin(1.2f * headW, std::fmax(g.ry, 0.3f * headW)) : 0.7f * headW;
        const Vec3 mid = c + ex * g.cx + ey * g.cy;
        const int k = static_cast<int>(WearSlot::Neck);
        out.frame[k] = makeFrame(mid, ex, ey, ez, rx, ry, (rx + ry) * 0.5f);
        out.boneA[k] = out.boneB[k] = static_cast<s8>(neck);
        out.ok[k] = true;
    }

    // ---- The back: its top 45% of the way from the chest to the hips, on the spine bone there;
    // further back if the head sits over it there (a big-headed hatchling).
    {
        const Vec3 c0 = jointOf(m, chest), c1 = jointOf(m, hips);
        auto nearWing = [&](int b) { return !(s.flags[b] & 1) && !startsWith(s.name[b], "wing"); };
        for (float along : {0.45f, 0.6f, 0.75f, 0.9f, 1.05f, 1.2f}) {
            const float ym = c0.y + (c1.y - c0.y) * along;
            const float zmid = (c0.z + c1.z) * 0.5f;
            int face = -1;
            const float up = body.surface({kOffMid, ym, zmid}, {0, 0, 1}, nearWing, &face);
            if (up <= 0 || face < 0) continue;
            bool back = false;  // the face on top there has a corner on the body (not all head or legs)
            for (int k = 0; k < 3; ++k) {
                const int b = body.bone[mesh->indices[face + k]];
                back |= !descends(s, b, head) && !isLimb(s, b);
            }
            if (!back) continue;
            const float down = body.surface({kOffMid, ym, zmid}, {0, 0, -1}, nearWing);
            float hi = zmid + up;
            const float lo = down > 0 ? zmid - down : zmid - up;
            // (from the side a flank may belong to a leg's bone: anything but the wings counts)
            float w = halfWidth(Vec3{0, ym, lo + (hi - lo) * 0.6f}, nearWing);
            if (w <= 1e-3f) continue;
            // A back covered in plates or moss (off the middle too, not just a ridge of spikes): the
            // saddle sits on that, a little wider.
            const float side = std::fmax(spikesTop(m, -0.5f * w, ym), spikesTop(m, 0.5f * w, ym));
            const float mid = std::fmax(spikesTop(m, kOffMid, ym), side);
            if (side > hi - 0.3f * w && mid > hi) {
                w += 0.8f * (mid - hi);
                hi = mid;
            }
            int seg = hips;  // the spine bone whose length holds ym (hips -> belly -> chest ...)
            for (int b = s.parent[chest]; b >= 0; b = s.parent[b]) {
                int child = chest;
                for (int c = chest; s.parent[c] >= 0; c = s.parent[c])
                    if (s.parent[c] == b) child = c;
                const float ya = jointOf(m, b).y, yb = jointOf(m, child).y;
                if ((ym - ya) * (ym - yb) <= 0) seg = b;
                if (b == hips) break;
            }
            const Vec3 dir = normalize(c1 - c0);
            Vec3 ex, ey, ez;
            axes(kUp - dir * dot(dir, kUp), dir, ex, ey, ez);
            const float depth = std::fmin(w, std::fmax(0.3f * w, (hi - lo) * 0.5f));
            const int k = static_cast<int>(WearSlot::Back);
            out.frame[k] = makeFrame({0, ym, hi}, ex, ey, ez, w, w, depth);
            out.boneA[k] = out.boneB[k] = static_cast<s8>(seg);
            out.ok[k] = true;
            break;
        }
    }

    // ---- The tail: its girth a third of the way along.
    {
        int tail[12], n = 0;
        for (int k = 1; k <= 12; ++k) {
            char name[8] = {'t', 'a', 'i', 'l', static_cast<char>('0' + k % 10), 0, 0, 0};
            if (k >= 10) name[4] = '1', name[5] = static_cast<char>('0' + k - 10);
            const int b = s.find(name);
            if (b < 0) break;
            tail[n++] = b;
        }
        if (n >= 2) {
            Vec3 pts[13];
            for (int i = 0; i < n; ++i) pts[i] = jointOf(m, tail[i]);
            pts[n] = pts[n - 1] + boneY(m, tail[n - 1]) * length(pts[n - 1] - pts[n - 2]);
            float total = 0;
            for (int i = 0; i < n; ++i) total += length(pts[i + 1] - pts[i]);
            float want = total * 0.33f;
            int seg = 0;
            while (seg < n - 1 && want > length(pts[seg + 1] - pts[seg])) want -= length(pts[seg + 1] - pts[seg]), ++seg;
            const float segLen = std::fmax(0.02f, length(pts[seg + 1] - pts[seg]));
            const Vec3 axis = normalize(pts[seg + 1] - pts[seg]);
            const Vec3 c = pts[seg] + axis * std::fmin(want, segLen);
            Vec3 ex, ey, ez;
            const Vec3 up = std::fabs(dot(axis, kUp)) > 0.85f ? kBack : kUp;  // a tail curled up: its outside
            axes(up - axis * dot(axis, up), axis, ex, ey, ez);
            const Girth g = girth(body, c, ex, ez, [&](int b) { return startsWith(s.name[b], "tail"); });
            const float floorR = 0.25f * headW;
            float rx = g.ok ? std::fmax(g.rx, floorR) : 0.5f * headW;
            float rz = g.ok ? std::fmax(g.ry, floorR) : 0.5f * headW;
            {  // (D126: the fur, plates and spikes round it too, the parts' meshes the body's girth
               // misses: a Puffback's wreath sat inside its tail's fluff. Most of the way out, not to
               // the tip of a lone spike.)
                std::vector<float> ax, az;
                const float slab = 0.35f * segLen;
                for (const MeshData& md : m.meshes) {
                    const int key = md.keyCount > 1 ? (grown ? md.keyCount - 1 : 0) : 0;
                    for (int v = 0; v < md.vertexCount; ++v) {
                        const u8* sk = &md.skin[std::size_t(v) * 4];
                        const int b = md.palette[sk[2] >= sk[3] ? sk[0] : sk[1]];
                        if (!startsWith(s.name[b], "tail")) continue;
                        const Vec3 d = md.pos[std::size_t(key) * md.vertexCount + v] - c;
                        if (std::fabs(dot(d, axis)) > slab) continue;
                        ax.push_back(std::fabs(dot(d, ex) - g.cx));
                        az.push_back(std::fabs(dot(d, ez) - g.cy));
                    }
                }
                auto share = [](std::vector<float>& v, float at) {
                    if (v.empty()) return 0.0f;
                    std::size_t k = static_cast<std::size_t>(at * (v.size() - 1));
                    std::nth_element(v.begin(), v.begin() + static_cast<long>(k), v.end());
                    return v[k];
                };
                rx = std::fmax(rx, share(ax, 0.85f));
                rz = std::fmax(rz, share(az, 0.85f));
            }
            const Vec3 mid = c + ex * g.cx + ez * g.cy;
            const int k = static_cast<int>(WearSlot::Tail);
            out.frame[k] = makeFrame(mid, ex, ey, ez, rx, (rx + rz) * 0.5f, rz);
            out.boneA[k] = out.boneB[k] = static_cast<s8>(tail[seg]);
            out.ok[k] = true;
        }
    }

    // A grown dragon's head is small beside its body (a hatchling's is big): a hat a third larger
    // than the skull, so it reads from across the den (grown about the skull's top, where it sits).
    if (grown && out.ok[0])
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c) out.frame[0].m[r][c] *= 1.35f;

    // Clear of the eyes (D126, Noah: the hats went through the Crestwing's and the Curlstone's eyes)
    // and on the skull (run 23, Noah: "many are floating quite far off of their heads"). On most
    // grown kinds the eyes' tops stand above the skull's top where the hat sits, and lifting it over
    // them left it floating (up to half its size). So it may also move back toward the crown of the
    // head (or forward), seated again on the skull there, come smaller (to 50%: run 25, Noah) or tip back (its
    // brim's front up over the eyes). Of those, the one that floats least while no eye comes up
    // through its brim: moving, shrinking and tipping cost a little, floating most, and a crest or
    // horn through its crown where it sits counts against a spot (never lifted off it: that's
    // floating too). The narrow things (a crown, a party hat) clear the eyes over less, so they
    // get a seat of their own, down on the skull where a brim couldn't be.
    if (out.ok[0]) {
        const Mat34 base = out.frame[0];
        const float unit = length(Vec3{base.m[0][0], base.m[1][0], base.m[2][0]});
        const Vec3 ey = normalize(Vec3{base.m[0][1], base.m[1][1], base.m[2][1]}),
                   ez = normalize(Vec3{base.m[0][2], base.m[1][2], base.m[2][2]});
        std::vector<Vec3> eyesAt, partsAt;
        constexpr float kMargin = 0.05f, kCrownR = 0.42f;
        // (once for the brimmed hats, once for the narrow things: a crown, a party hat, a circlet)
        // The head's top under a hat, in the base frame's units (x across, y back, z up), from a
        // grid of rays down it once: a brim mustn't cut into the skull (run 24: the Puffback's
        // straw hat did, its wide brim through its domed forehead). Every spot below reads it.
        constexpr int kGrid = 20;
        constexpr float kSpan = 1.4f, kNone = -1e9f;
        const Vec3 ex = normalize(Vec3{base.m[0][0], base.m[1][0], base.m[2][0]}), o = base.translation();
        float top[kGrid][kGrid];
        for (int i = 0; i < kGrid; ++i)
            for (int j = 0; j < kGrid; ++j) {
                const float x = -kSpan + 2 * kSpan * i / (kGrid - 1), y = -kSpan + 2 * kSpan * j / (kGrid - 1);
                const Vec3 w = o + ex * (x * unit) + ey * (y * unit);
                const float t = body.cast(w + ez * kFar, ez * -1.0f, inHead);
                top[i][j] = t < 0 ? kNone : (kFar - t) / unit;
            }
        auto topAt = [&](float x, float y) {  // the highest of the cell's corners (kNone off the head)
            const float fi = (x + kSpan) / (2 * kSpan) * (kGrid - 1), fj = (y + kSpan) / (2 * kSpan) * (kGrid - 1);
            const int i = static_cast<int>(std::floor(fi)), j = static_cast<int>(std::floor(fj));
            float h = kNone;
            for (int a = 0; a < 2; ++a)
                for (int b = 0; b < 2; ++b)
                    if (i + a >= 0 && j + b >= 0 && i + a < kGrid && j + b < kGrid) h = std::fmax(h, top[i + a][j + b]);
            return h;
        };
        // How far up a candidate must go (its units) for its brim's underside, out to `reach`, to
        // clear the skull: there it rests on the head, touching it (not floating).
        auto brimRest = [&](const Mat34& f, float scale, float reach) {
            float need = 0;
            const Vec3 c0{f.m[0][0], f.m[1][0], f.m[2][0]}, c1{f.m[0][1], f.m[1][1], f.m[2][1]}, c2{f.m[0][2], f.m[1][2], f.m[2][2]};
            for (float r : {0.45f, 0.72f, 1.0f})
                for (int k = 0; k < 12; ++k) {
                    const float a = k * 0.5235988f;
                    const Vec3 w = f.translation() + c0 * (r * reach * std::cos(a)) + c1 * (r * reach * std::sin(a)) + c2 * kHatBrimZ;
                    const Vec3 d = w - o;
                    const float h = topAt(dot(d, ex) / unit, dot(d, ey) / unit);
                    if (h > kNone * 0.5f) need = std::fmax(need, (h - dot(d, ez) / unit) / scale);
                }
            return need;
        };
        auto seat = [&](float brim, float reach, float& outBack, float& outLift, float& outScale) {
            float bestCost = 1e9f;
            Mat34 best = base;
            for (int step = -6; step <= 9; ++step) {
                const float back = 0.05f * step;  // (in the hat's units; less than 0: forward)
                Vec3 at = base.translation();
                if (step != 0) {  // seated on the skull's top there (along the hat's up)
                    const Vec3 c = at + ey * (back * unit);
                    const float t = body.cast(c + ez * kFar, ez * -1.0f, inHead);
                    if (t < 0) continue;  // (off the head)
                    at = c + ez * (kFar - t);
                }
                for (int tilt = 0; tilt < 4; ++tilt)  // (tipped back: the brim's front up over the eyes)
                for (float scale : {1.0f, 0.9f, 0.8f, 0.7f, 0.6f, 0.5f}) {  // (smaller still rather than float: run 25, Noah)
                    const float ta = 0.17f * tilt, ct = std::cos(ta), st = std::sin(ta);
                    Mat34 f = base;
                    for (int r = 0; r < 3; ++r) {
                        const float y = base.m[r][1], z = base.m[r][2];
                        f.m[r][1] = y * ct - z * st;
                        f.m[r][2] = z * ct + y * st;
                        for (int c = 0; c < 3; ++c) f.m[r][c] *= scale;
                    }
                    f.setTranslation(at);
                    eyePoints(m, f, eyesAt);
                    headPartPoints(m, head, grown, f, partsAt);
                    const float clear = std::fmax(0.0f, highestThrough(eyesAt, 0.0f, brim, kMargin));
                    const float rest = brimRest(f, scale, reach);  // (its brim on the forehead: resting, not floating)
                    for (int up = 0; up <= 0; ++up) {  // (never higher than the eyes need: up on a crest is floating too)
                        const float lift = std::fmax(clear, rest) + 0.05f * up;
                        const float cost = 1.5f * clear + 0.5f * std::fmax(0.0f, rest - clear) + 0.3f * std::fabs(back) + 0.5f * (1.0f - scale) +
                                           0.4f * ta + 0.5f * piercing(partsAt, kCrownR, 0.0f);  // (as seated: lifting off a crest is no cure)
                        if (cost < bestCost - 1e-4f) {
                            bestCost = cost;
                            best = f;
                            best.setTranslation(at + normalize(Vec3{f.m[0][2], f.m[1][2], f.m[2][2]}) * (lift * unit * scale));
                            outBack = back;
                            outLift = clear;  // (what floats: the eyes' lift; a brim's rest touches)
                            outScale = scale;
                        }
                    }
                }
            }
            return best;
        };
        float back = 0, lift = 0, scale = 1;
        out.headNarrow = seat(kHatNarrow, 0.56f, back, lift, scale);  // (the crown's band)
        out.narrowLift = lift;
        out.headWide = seat(kHatBrim, 1.0f, back, lift, scale);       // (the straw sunhat's wide brim, drooping at its edge)
        out.wideLift = lift;
        out.frame[0] = seat(kHatBrim, 0.72f, out.hatBack, out.hatLift, out.hatScale);  // (the top hat's brim)
        Mat34 pad;
        if (padSeat(m, head, grown, base, pad)) {  // a pad on its head: every hat sits on that, level with it
            out.frame[0] = out.headNarrow = out.headWide = pad;
            out.wideLift = 0;
            out.hatBack = out.hatLift = out.narrowLift = 0;
            out.hatScale = 1;
            out.onPad = true;
        }
    }

    // The plan's nudges.
    for (int k = 0; k < kWearSlots; ++k) {
        const Nudge* nd = out.ok[k] ? nudgeFor(plan, k, grown) : nullptr;
        if (!nd) continue;
        Mat34& f = out.frame[k];
        const Vec3 ex{f.m[0][0], f.m[1][0], f.m[2][0]}, ey{f.m[0][1], f.m[1][1], f.m[2][1]}, ez{f.m[0][2], f.m[1][2], f.m[2][2]};
        f.setTranslation(f.translation() + ex * nd->dx + ey * nd->dy + ez * nd->dz);
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c) f.m[r][c] *= nd->scale;
        if (k == 0)  // (the narrow things' and the wide brim's seats with it)
            for (Mat34* g : {&out.headNarrow, &out.headWide}) {
                g->setTranslation(g->translation() + ex * nd->dx + ey * nd->dy + ez * nd->dz);
                for (int r = 0; r < 3; ++r)
                    for (int c = 0; c < 3; ++c) g->m[r][c] *= nd->scale;
            }
    }
    return out.ok[0] || out.ok[1] || out.ok[2] || out.ok[3];
}

}  // namespace ec
