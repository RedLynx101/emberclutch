#include "core/wear_fit.hpp"

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

}  // namespace

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
            const float rx = g.ok ? std::fmax(g.rx, floorR) : 0.5f * headW;
            const float rz = g.ok ? std::fmax(g.ry, floorR) : 0.5f * headW;
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

    // The plan's nudges.
    for (int k = 0; k < kWearSlots; ++k) {
        const Nudge* nd = out.ok[k] ? nudgeFor(plan, k, grown) : nullptr;
        if (!nd) continue;
        Mat34& f = out.frame[k];
        const Vec3 ex{f.m[0][0], f.m[1][0], f.m[2][0]}, ey{f.m[0][1], f.m[1][1], f.m[2][1]}, ez{f.m[0][2], f.m[1][2], f.m[2][2]};
        f.setTranslation(f.translation() + ex * nd->dx + ey * nd->dy + ez * nd->dz);
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c) f.m[r][c] *= nd->scale;
    }
    return out.ok[0] || out.ok[1] || out.ok[2] || out.ok[3];
}

}  // namespace ec
