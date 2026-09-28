#include "core/valley.hpp"

#include <array>
#include <cmath>
#include <cstring>

#include "core/byte_reader.hpp"

namespace ec {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kSkirt = 3.0f;         // metres a skirt hangs below the edge, at full detail
constexpr float kLodNear = 34.0f;      // tiles nearer than this: full detail (Beta: the places and people need room)
constexpr float kLodMid = 115.0f;      // then half; beyond, a quarter
constexpr float kLodFar = 175.0f;      // ...with its trees as far cones; beyond, the ground alone (the fog has them)

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

u16 addVertex(ValleyMesh& m, Vec3 p, u8 r, u8 g, u8 b, u8 a = 255) {
    m.pos.push_back(p);
    m.color.insert(m.color.end(), {r, g, b, a});
    return static_cast<u16>(m.pos.size() - 1);
}

void tri(ValleyMesh& m, u16 a, u16 b, u16 c) { m.idx.insert(m.idx.end(), {a, b, c}); }

// Shade a face colour by how it faces the baked sun (the terrain's own light).
void shaded(const u8 base[3], Vec3 n, u8 out[3]) {
    const Vec3 sun = normalize(Vec3{-0.45f, -0.5f, 0.74f});
    const float k = 0.58f + 0.62f * std::fmax(0.0f, dot(normalize(n), sun));
    for (int i = 0; i < 3; ++i) out[i] = static_cast<u8>(clampf(base[i] * k, 0, 255));
}

// A cone of `sides` faces from the ground up: a conifer.
void tree(ValleyMesh& m, Vec3 base, float h, float r, int sides, const u8 green[3]) {
    const Vec3 apex{base.x, base.y, base.z + h};
    for (int s = 0; s < sides; ++s) {
        const float a0 = 2 * kPi * s / sides, a1 = 2 * kPi * (s + 1) / sides;
        const Vec3 p0{base.x + std::cos(a0) * r, base.y + std::sin(a0) * r, base.z};
        const Vec3 p1{base.x + std::cos(a1) * r, base.y + std::sin(a1) * r, base.z};
        u8 c[3];
        shaded(green, cross(p1 - p0, apex - p0), c);
        tri(m, addVertex(m, p0, c[0], c[1], c[2]), addVertex(m, p1, c[0], c[1], c[2]),
            addVertex(m, apex, c[0], c[1], c[2]));
    }
}

// A ring of `sides` points round `c` at radius r (x and y scaled by sx, sy), turned by `turn`.
Vec3 ringPoint(Vec3 c, float r, float sx, float sy, float a, float turn) {
    return {c.x + std::cos(a + turn) * r * sx, c.y + std::sin(a + turn) * r * sy, c.z};
}

// A bipyramid blob: a waist ring of `sides` at `mid`, a top point `up` above and a bottom point
// `down` below (0: no bottom, it sits on the ground); faces shaded, the top a little lighter.
// A little painted variety (Beta 1 review: "texture them"): a steady pseudo-random 0..1 from a
// point, and a colour lightened or darkened by k with a touch of warm sunlight.
float dapple(Vec3 p) {
    const float v = std::sin(p.x * 12.9898f + p.y * 78.233f + p.z * 37.719f) * 43758.5453f;
    return v - std::floor(v);
}
void tinted(const u8 c[3], float k, float warm, u8 out[3]) {
    const float add[3] = {22.0f * warm, 16.0f * warm, 0.0f};
    for (int i = 0; i < 3; ++i) out[i] = static_cast<u8>(std::fmin(255.0f, std::fmax(0.0f, c[i] * k + add[i])));
}

void blob(ValleyMesh& m, Vec3 mid, float r, float up, float down, int sides, float turn, const u8 col[3],
          float squash = 1.0f) {
    const Vec3 top{mid.x, mid.y, mid.z + up};
    const Vec3 bottom{mid.x, mid.y, mid.z - down};
    const u8 light[3] = {static_cast<u8>(col[0] + (255 - col[0]) / 6), static_cast<u8>(col[1] + (255 - col[1]) / 6),
                         static_cast<u8>(col[2] + (255 - col[2]) / 6)};
    for (int s = 0; s < sides; ++s) {
        const float a0 = 2 * kPi * s / sides, a1 = 2 * kPi * (s + 1) / sides;
        const Vec3 p0 = ringPoint(mid, r, 1.0f, squash, a0, turn), p1 = ringPoint(mid, r, 1.0f, squash, a1, turn);
        u8 c[3], c0[3], c1[3], ct[3];
        // Each facet painted: its edge dappled leaf by leaf, its crown sunlit and warm.
        shaded(light, cross(p1 - p0, top - p0), c);
        tinted(c, 0.82f + 0.22f * dapple(p0), 0.0f, c0);
        tinted(c, 0.82f + 0.22f * dapple(p1), 0.0f, c1);
        tinted(c, 1.1f, 1.0f, ct);
        tri(m, addVertex(m, p0, c0[0], c0[1], c0[2]), addVertex(m, p1, c1[0], c1[1], c1[2]), addVertex(m, top, ct[0], ct[1], ct[2]));
        if (down > 0) {  // the underside in shade, deepest in the middle
            shaded(col, cross(bottom - p0, p1 - p0), c);
            u8 cb[3];
            tinted(c, 0.86f + 0.14f * dapple(p0 + Vec3{0, 0, 1}), 0.0f, c0);
            tinted(c, 0.86f + 0.14f * dapple(p1 + Vec3{0, 0, 1}), 0.0f, c1);
            tinted(c, 0.66f, 0.0f, cb);
            tri(m, addVertex(m, p0, c0[0], c0[1], c0[2]), addVertex(m, bottom, cb[0], cb[1], cb[2]),
                addVertex(m, p1, c1[0], c1[1], c1[2]));
        }
    }
}

// A trunk: a thin prism of `sides` from the ground up to h.
void trunk(ValleyMesh& m, Vec3 base, float h, float r, int sides, const u8 col[3]) {
    for (int s = 0; s < sides; ++s) {
        const float a0 = 2 * kPi * s / sides, a1 = 2 * kPi * (s + 1) / sides;
        const Vec3 p0 = ringPoint(base, r, 1, 1, a0, 0.3f), p1 = ringPoint(base, r, 1, 1, a1, 0.3f);
        const Vec3 q0{p0.x, p0.y, p0.z + h}, q1{p1.x, p1.y, p1.z + h};
        u8 c[3], lo[3], hi[3];
        shaded(col, cross(p1 - p0, q0 - p0), c);
        tinted(c, 0.72f, 0.0f, lo);  // bark darker at the foot, lighter up under the leaves
        tinted(c, 1.06f, 0.3f, hi);
        const u16 a = addVertex(m, p0, lo[0], lo[1], lo[2]), b = addVertex(m, p1, lo[0], lo[1], lo[2]),
                  d = addVertex(m, q1, hi[0], hi[1], hi[2]), e = addVertex(m, q0, hi[0], hi[1], hi[2]);
        tri(m, a, b, d);
        tri(m, a, d, e);
    }
}

// One prop at a detail level (0 near, 1 further, 2 far: trees only, as cones; nothing beyond).
void prop(ValleyMesh& m, const Valley& v, const ValleyTree& p, int lod) {
    const float z = v.heightAt(p.x, p.y);
    const float h = p.height, turn = p.yaw * (2 * kPi / 256.0f);
    const u8 k = p.shade;
    if (lod >= 2) {  // far off: a three-sided cone in its leaves' colour, the same size (no pop as it nears)
        u8 c[3];
        if (p.kind == kPropPine) {
            c[0] = static_cast<u8>(40 + k % 22), c[1] = static_cast<u8>(96 + k % 28), c[2] = static_cast<u8>(62 + k % 18);
            tree(m, {p.x, p.y, z - 0.3f}, h, h * 0.3f, 3, c);
        } else if (p.kind == kPropTree || p.kind == kPropFruit) {
            c[0] = static_cast<u8>(64 + k % 30), c[1] = static_cast<u8>(128 + k % 40), c[2] = static_cast<u8>(58 + k % 22);
            blob(m, {p.x, p.y, z + h * 0.2f}, h * 0.42f, h * 0.7f, 0, 3, turn, c);  // (a crown with no underside: far below the eye)
        }
        return;
    }
    switch (p.kind) {
        case kPropTree: {  // a round storybook tree: a stout trunk under two soft clumps of leaves
            const u8 leaf[3] = {static_cast<u8>(64 + k % 30), static_cast<u8>(128 + k % 40), static_cast<u8>(58 + k % 22)};
            const u8 bark[3] = {118, 84, 58};
            if (lod == 0) {
                trunk(m, {p.x, p.y, z - 0.3f}, h * 0.45f, h * 0.07f, 3, bark);
                blob(m, {p.x, p.y, z + h * 0.5f}, h * 0.38f, h * 0.26f, h * 0.2f, 5, turn, leaf);
                const float ox = std::cos(turn) * h * 0.1f, oy = std::sin(turn) * h * 0.1f;
                blob(m, {p.x + ox, p.y + oy, z + h * 0.74f}, h * 0.27f, h * 0.24f, h * 0.12f, 5, turn + 0.6f, leaf);
            } else {  // further off: one round clump (five sides reads round, four reads as a gem)
                blob(m, {p.x, p.y, z + h * 0.6f}, h * 0.4f, h * 0.3f, h * 0.24f, 5, turn, leaf);
            }
            break;
        }
        case kPropPine: {  // two stacked cones, deep green
            const u8 needle[3] = {static_cast<u8>(40 + k % 22), static_cast<u8>(96 + k % 28), static_cast<u8>(62 + k % 18)};
            if (lod == 0) {
                blob(m, {p.x, p.y, z - 0.2f}, h * 0.3f, h * 0.55f, 0, 5, turn, needle);
                blob(m, {p.x, p.y, z + h * 0.42f}, h * 0.22f, h * 0.58f, 0, 5, turn + 0.6f, needle);
            } else {
                tree(m, {p.x, p.y, z - 0.3f}, h, h * 0.3f, 4, needle);
            }
            break;
        }
        case kPropFruit: {  // a round fruit tree, and its fruit
            const u8 leaf[3] = {static_cast<u8>(84 + k % 20), static_cast<u8>(146 + k % 30), static_cast<u8>(64 + k % 16)};
            const u8 bark[3] = {128, 92, 62};
            if (lod == 0) trunk(m, {p.x, p.y, z - 0.3f}, h * 0.45f, h * 0.07f, 3, bark);
            blob(m, {p.x, p.y, z + h * 0.62f}, h * 0.42f, h * 0.36f, h * 0.2f, lod == 0 ? 6 : 4, turn, leaf, 0.95f);
            if (lod == 0)
                for (int f = 0; f < 4; ++f) {  // little fruit on the leaves' sunny side
                    const float a = turn + f * 1.6f;
                    const Vec3 c{p.x + std::cos(a) * h * 0.36f, p.y + std::sin(a) * h * 0.36f, z + h * (0.62f + 0.05f * (f % 2))};
                    const u8 fruit[3] = {230, static_cast<u8>(f % 2 ? 180 : 96), 70};
                    blob(m, c, 0.28f, 0.28f, 0.28f, 3, a, fruit);
                }
            break;
        }
        case kPropBush: {
            const u8 leaf[3] = {static_cast<u8>(70 + k % 26), static_cast<u8>(136 + k % 34), static_cast<u8>(62 + k % 20)};
            blob(m, {p.x, p.y, z + h * 0.1f}, h * 0.6f, h * 0.75f, 0, lod == 0 ? 5 : 3, turn, leaf, 0.85f);
            if (lod == 0 && k % 3 == 0) {  // one in three in flower
                const u8 bloom[3] = {246, static_cast<u8>(k % 2 ? 150 : 214), static_cast<u8>(k % 2 ? 190 : 110)};
                blob(m, {p.x + h * 0.2f, p.y - h * 0.3f, z + h * 0.62f}, h * 0.14f, h * 0.14f, h * 0.1f, 3, turn, bloom);
            }
            break;
        }
        case kPropRock: {  // a squat, faceted rock, soft lilac grey
            const u8 stone[3] = {static_cast<u8>(146 + k % 24), static_cast<u8>(134 + k % 20), static_cast<u8>(140 + k % 22)};
            blob(m, {p.x, p.y, z - h * 0.15f}, h * 0.55f, h * 0.55f, 0, lod == 0 ? 5 : 3, turn, stone, 0.75f);
            break;
        }
        case kPropFlowers: {  // a patch of little flowers: petals just above the grass
            if (lod > 0) break;
            static const u8 kBloom[4][3] = {{250, 168, 196}, {252, 226, 110}, {250, 250, 244}, {170, 180, 250}};
            const u8* c = kBloom[k % 4];
            for (int f = 0; f < 5; ++f) {
                const float a = turn + f * 1.3f, r = h * (0.2f + 0.16f * f);
                const float fx = p.x + std::cos(a) * r, fy = p.y + std::sin(a) * r, fz = v.heightAt(fx, fy) + 0.18f;
                const float s = 0.32f;
                tri(m, addVertex(m, {fx - s, fy - s * 0.5f, fz}, c[0], c[1], c[2]),
                    addVertex(m, {fx + s, fy - s * 0.5f, fz}, c[0], c[1], c[2]),
                    addVertex(m, {fx, fy + s, fz + 0.05f}, c[0], c[1], c[2]));
            }
            break;
        }
        case kPropReeds: {  // a few thin blades
            if (lod > 0) break;
            const u8 reed[3] = {110, 150, 78};
            for (int b = 0; b < 4; ++b) {
                const float a = turn + b * 1.7f;
                const Vec3 base{p.x + std::cos(a) * 0.5f, p.y + std::sin(a) * 0.5f, z - 0.2f};
                const Vec3 tip{base.x + std::cos(a) * 0.3f, base.y + std::sin(a) * 0.3f, base.z + h};
                const Vec3 side{-std::sin(a) * 0.12f, std::cos(a) * 0.12f, 0};
                tri(m, addVertex(m, base - side, reed[0], reed[1], reed[2]), addVertex(m, base + side, reed[0], reed[1], reed[2]),
                    addVertex(m, tip, 150, 180, 96));
                tri(m, addVertex(m, base + side, reed[0], reed[1], reed[2]), addVertex(m, base - side, reed[0], reed[1], reed[2]),
                    addVertex(m, tip, 150, 180, 96));
            }
            break;
        }
        default: break;
    }
}

}  // namespace

void ValleyMesh::clear() {
    pos.clear();
    color.clear();
    idx.clear();
    skirtFrom = 0;
    parts.clear();
}

bool loadValley(const u8* data, std::size_t size, Valley& out) {
    ByteReader r(data, size);
    char magic[4];
    r.bytes(magic, 4);
    const bool v2 = std::memcmp(magic, "EVL2", 4) == 0;  // Beta: props of every kind, and the paths
    if (!r.ok() || (!v2 && std::memcmp(magic, "EVL1", 4) != 0) || r.u16v() != (v2 ? 2 : 1)) return false;
    out = Valley{};
    out.n = r.u16v();
    out.spacing = r.f32();
    out.x0 = r.f32();
    out.y0 = r.f32();
    out.hmin = r.f32();
    out.hmax = r.f32();
    out.water = r.f32();
    r.f32();  // reserved
    if (!r.ok() || out.n < kTileQuads + 1 || (out.n - 1) % kTileQuads != 0 || out.spacing <= 0) return false;
    const std::size_t cells = std::size_t(out.n) * out.n;
    out.h.resize(cells);
    for (float& v : out.h) v = out.hmin + (out.hmax - out.hmin) * (r.u16v() / 65535.0f);
    out.rgb.resize(cells * 3);
    r.bytes(out.rgb.data(), out.rgb.size());
    out.trees.resize(r.u16v());
    for (ValleyTree& t : out.trees) {
        t.x = r.f32();
        t.y = r.f32();
        if (v2) {
            t.kind = r.u8v();
            t.height = r.u8v() / 10.0f;
            t.shade = r.u8v();
            t.yaw = r.u8v();
            if (t.kind >= kPropKinds) t.kind = kPropBush;
        } else {
            t.height = r.u8v();
            t.shade = r.u8v();
        }
    }
    out.islands.resize(r.u16v());
    for (ValleyIsland& isl : out.islands) {
        isl.at = r.vec3();
        isl.radius = r.f32();
    }
    out.places.resize(r.u16v());
    for (ValleyPlaceInfo& p : out.places) {
        p.id = r.u8v();
        p.at = r.vec3();
        p.heading = r.f32();
    }
    if (v2) {
        out.paths.resize(r.u16v());
        for (std::vector<Vec2>& path : out.paths) {
            path.resize(r.u16v());
            for (Vec2& p : path) {
                p.x = r.f32();
                p.y = r.f32();
            }
        }
    }
    if (!r.ok()) return false;
    // Each tile's height range, and the trees standing in it.
    const int t = out.tiles();
    out.tileLow.assign(std::size_t(t) * t, 1e9f);
    out.tileHigh.assign(std::size_t(t) * t, -1e9f);
    out.tileTrees.assign(std::size_t(t) * t, {});
    for (int ty = 0; ty < t; ++ty)
        for (int tx = 0; tx < t; ++tx)
            for (int j = ty * kTileQuads; j <= (ty + 1) * kTileQuads; ++j)
                for (int i = tx * kTileQuads; i <= (tx + 1) * kTileQuads; ++i) {
                    const float h = out.h[std::size_t(j) * out.n + i];
                    float& lo = out.tileLow[std::size_t(ty) * t + tx];
                    float& hi = out.tileHigh[std::size_t(ty) * t + tx];
                    lo = std::fmin(lo, h);
                    hi = std::fmax(hi, h);
                }
    for (std::size_t k = 0; k < out.trees.size(); ++k) {
        const int tx = static_cast<int>((out.trees[k].x - out.x0) / out.tileSize());
        const int ty = static_cast<int>((out.trees[k].y - out.y0) / out.tileSize());
        if (tx < 0 || ty < 0 || tx >= t || ty >= t) continue;
        out.tileTrees[std::size_t(ty) * t + tx].push_back(static_cast<int>(k));
        float& hi = out.tileHigh[std::size_t(ty) * t + tx];
        hi = std::fmax(hi, out.heightAt(out.trees[k].x, out.trees[k].y) + out.trees[k].height);
    }
    return true;
}

float Valley::heightAt(float x, float y) const {
    if (n < 2) return 0;
    const float fx = clampf((x - x0) / spacing, 0, n - 1.001f), fy = clampf((y - y0) / spacing, 0, n - 1.001f);
    const int i = static_cast<int>(fx), j = static_cast<int>(fy);
    const float u = fx - i, w = fy - j;
    const float* row0 = &h[std::size_t(j) * n + i];
    const float* row1 = row0 + n;
    return (row0[0] * (1 - u) + row0[1] * u) * (1 - w) + (row1[0] * (1 - u) + row1[1] * u) * w;
}

int Valley::islandAt(float x, float y, float z) const {
    for (std::size_t k = 0; k < islands.size(); ++k) {
        const ValleyIsland& isl = islands[k];
        // (the grass top wobbles 12% round its radius: stand within the inside of it; from a
        // little below its top a body steps up onto it, far below it's under it)
        if (std::hypot(x - isl.at.x, y - isl.at.y) < isl.radius * 0.86f && z > isl.at.z - 2.5f) return static_cast<int>(k);
    }
    return -1;
}

int Valley::deckAt(float x, float y, float z) const {
    for (std::size_t k = 0; k < decks.size(); ++k) {
        const ValleyDeck& d = decks[k];
        const float dx = d.b.x - d.a.x, dy = d.b.y - d.a.y, L2 = dx * dx + dy * dy;
        if (L2 < 1e-6f) continue;
        const float t = ((x - d.a.x) * dx + (y - d.a.y) * dy) / L2;
        if (t < 0 || t > 1) continue;
        const float side = std::fabs((x - d.a.x) * dy - (y - d.a.y) * dx) / std::sqrt(L2);
        if (side > d.halfWidth) continue;
        if (z > deckTop(static_cast<int>(k), x, y) - 1.2f) return static_cast<int>(k);  // (from a little below: stepping up)
    }
    return -1;
}

float Valley::deckTop(int k, float x, float y) const {
    if (k < 0 || k >= static_cast<int>(decks.size())) return heightAt(x, y);
    const ValleyDeck& d = decks[std::size_t(k)];
    const float dx = d.b.x - d.a.x, dy = d.b.y - d.a.y, L2 = dx * dx + dy * dy;
    float t = L2 > 1e-6f ? ((x - d.a.x) * dx + (y - d.a.y) * dy) / L2 : 0.0f;
    t = t < 0 ? 0 : (t > 1 ? 1 : t);
    return d.z0 + (d.z1 - d.z0) * t + d.arch * 4.0f * t * (1.0f - t);
}

float Valley::groundAt(float x, float y, float z) const {
    const int k = islandAt(x, y, z);
    if (k >= 0) return islands[std::size_t(k)].at.z;
    const int d = deckAt(x, y, z);
    const float land = heightAt(x, y);
    return d >= 0 ? std::fmax(land, deckTop(d, x, y)) : land;
}

Vec3 Valley::normalAt(float x, float y) const {
    const float e = spacing;
    const float dx = (heightAt(x + e, y) - heightAt(x - e, y)) / (2 * e);
    const float dy = (heightAt(x, y + e) - heightAt(x, y - e)) / (2 * e);
    return normalize(Vec3{-dx, -dy, 1});
}

const ValleyPlaceInfo* Valley::place(u8 id) const {
    for (const ValleyPlaceInfo& p : places)
        if (p.id == id) return &p;
    return nullptr;
}

bool Valley::inside(float x, float y) const { return x >= x0 && y >= y0 && x <= x0 + size() && y <= y0 + size(); }

int valleyLodFor(float distance) {
    return distance < kLodNear ? 0 : distance < kLodMid ? 1 : distance < kLodFar ? 2 : 3;
}

void buildValleyTile(const Valley& v, int tx, int ty, int lod, ValleyMesh& out) {
    out.clear();
    const int t = v.tiles();
    if (tx < 0 || ty < 0 || tx >= t || ty >= t) return;
    lod = lod < 0 ? 0 : (lod >= kValleyLods ? kValleyLods - 1 : lod);
    const int step = 1 << (lod < 2 ? lod : 2), quads = kTileQuads / step, side = quads + 1;
    const int i0 = tx * kTileQuads, j0 = ty * kTileQuads;
    auto sample = [&](int a, int b) {  // grid point (a, b) of this tile at this level
        const int i = i0 + a * step, j = j0 + b * step;
        return std::size_t(j) * v.n + i;
    };
    for (int b = 0; b < side; ++b)
        for (int a = 0; a < side; ++a) {
            const std::size_t s = sample(a, b);
            addVertex(out, {v.x0 + (i0 + a * step) * v.spacing, v.y0 + (j0 + b * step) * v.spacing, v.h[s]},
                      v.rgb[s * 3], v.rgb[s * 3 + 1], v.rgb[s * 3 + 2]);
        }
    for (int b = 0; b < quads; ++b)
        for (int a = 0; a < quads; ++a) {
            const u16 p = static_cast<u16>(b * side + a), q = static_cast<u16>(p + 1), r = static_cast<u16>(p + side),
                      s = static_cast<u16>(r + 1);
            tri(out, p, q, s);  // counter-clockwise seen from above
            tri(out, p, s, r);
        }
    // Props, nearer levels only (simpler further off; the little ones only near).
    if (lod < kTreeLods)
        for (int k : v.tileTrees[std::size_t(ty) * t + tx]) prop(out, v, v.trees[std::size_t(k)], lod);
    // Skirts last: each edge copied a little lower, facing out of the tile (the renderer draws
    // them only where a neighbour is at another level).
    out.skirtFrom = out.idx.size();
    const float drop = kSkirt * step;
    auto skirt = [&](int a0, int b0, int da, int db, bool reversed) {
        for (int k = 0; k < quads; ++k) {
            const u16 top0 = static_cast<u16>((b0 + db * k) * side + (a0 + da * k));
            const u16 top1 = static_cast<u16>((b0 + db * (k + 1)) * side + (a0 + da * (k + 1)));
            const u8* c0 = &out.color[std::size_t(top0) * 4];
            const u8* c1 = &out.color[std::size_t(top1) * 4];
            const u16 low0 = addVertex(out, out.pos[top0] - Vec3{0, 0, drop}, c0[0], c0[1], c0[2]);
            const u16 low1 = addVertex(out, out.pos[top1] - Vec3{0, 0, drop}, c1[0], c1[1], c1[2]);
            if (reversed) {
                tri(out, top0, low1, top1);
                tri(out, top0, low0, low1);
            } else {
                tri(out, top0, top1, low1);
                tri(out, top0, low1, low0);
            }
        }
    };
    skirt(0, 0, 1, 0, true);            // south edge, west to east: faces south
    skirt(quads, 0, 0, 1, true);        // east edge, south to north: faces east
    skirt(0, quads, 1, 0, false);       // north edge: faces north
    skirt(0, 0, 0, 1, false);           // west edge: faces west
}

void buildValleyExtras(const Valley& v, ValleyMesh& out) {
    out.clear();
    constexpr int kSides = 9;
    for (const ValleyIsland& isl : v.islands) {
        out.parts.push_back(static_cast<u32>(out.idx.size()));  // (each island its own range: culled apart)
        const Vec3 c = isl.at;
        const float r = isl.radius;
        const u8 grass[3] = {104, 156, 78};
        const u16 top = addVertex(out, c, grass[0], grass[1], grass[2]);
        u16 rim[kSides];
        for (int s = 0; s < kSides; ++s) {
            const float a = 2 * kPi * s / kSides, wob = 1.0f + 0.12f * std::sin(a * 3 + c.x);
            rim[s] = addVertex(out, {c.x + std::cos(a) * r * wob, c.y + std::sin(a) * r * wob, c.z}, grass[0], grass[1],
                               grass[2]);
        }
        for (int s = 0; s < kSides; ++s) tri(out, top, rim[s], rim[(s + 1) % kSides]);
        // The rocky underside, a storybook turnip: a mossy lip under the grass, then banded lilac
        // rock bulging and narrowing in four rings down to a point, faceted.
        constexpr int kRings = 4;
        constexpr float kDown[kRings] = {0.26f, 0.62f, 1.02f, 1.42f}, kWide[kRings] = {0.94f, 0.82f, 0.56f, 0.26f};
        const u8 bands[kRings][3] = {{96, 132, 84}, {150, 134, 160}, {126, 110, 124}, {140, 124, 150}};  // moss, then rock
        Vec3 ring[kRings + 1][kSides];
        for (int s = 0; s < kSides; ++s) ring[0][s] = out.pos[rim[s]];
        for (int k = 0; k < kRings; ++k)
            for (int s = 0; s < kSides; ++s) {
                const float a = 2 * kPi * s / kSides, wob = 1.0f + 0.1f * std::sin(a * 2 + k * 1.7f + c.y);
                ring[k + 1][s] = {c.x + std::cos(a) * r * kWide[k] * wob + r * 0.05f * k, c.y + std::sin(a) * r * kWide[k] * wob,
                                  c.z - r * kDown[k]};
            }
        const Vec3 apex{c.x + r * 0.22f, c.y - r * 0.06f, c.z - r * 1.8f};
        for (int k = 0; k <= kRings; ++k)
            for (int s = 0; s < kSides; ++s) {
                const int e = (s + 1) % kSides;
                const Vec3 a0 = ring[k][s], a1 = ring[k][e];
                const Vec3 b0 = k < kRings ? ring[k + 1][s] : apex, b1 = k < kRings ? ring[k + 1][e] : apex;
                u8 col[3];
                shaded(bands[k < kRings ? k : kRings - 1], cross(a1 - a0, b0 - a0) * -1.0f, col);
                const u16 i0 = addVertex(out, a0, col[0], col[1], col[2]), i1 = addVertex(out, a1, col[0], col[1], col[2]);
                const u16 j0 = addVertex(out, b0, col[0], col[1], col[2]);
                tri(out, i0, j0, i1);
                if (k < kRings) {
                    const u16 j1 = addVertex(out, b1, col[0], col[1], col[2]);
                    tri(out, i1, j0, j1);
                }
            }
        // Round storybook trees round the top (Beta 1 review), clear of the middle where a place
        // may stand (the isles' lantern).
        const u8 bark[3] = {118, 84, 58};
        for (int k = 0; k < 3; ++k) {
            const float a = k * 2.1f + c.y * 0.01f, d = r * (0.42f + 0.12f * (k % 2));
            const Vec3 base{c.x + std::cos(a) * d, c.y + std::sin(a) * d, c.z - 0.3f};
            const float h = 6.5f + 1.5f * (k % 3);
            const u8 leaf[3] = {static_cast<u8>(70 + 10 * (k % 3)), static_cast<u8>(136 + 8 * k), static_cast<u8>(62 + 6 * (k % 2))};
            trunk(out, base, h * 0.45f, h * 0.07f, 3, bark);
            blob(out, {base.x, base.y, base.z + h * 0.55f}, h * 0.38f, h * 0.26f, h * 0.2f, 5, a, leaf);
            blob(out, {base.x + h * 0.12f, base.y - h * 0.08f, base.z + h * 0.78f}, h * 0.24f, h * 0.18f, h * 0.1f, 5, a + 0.8f, leaf);
        }
    }
    out.parts.push_back(static_cast<u32>(out.idx.size()));
    // (The places themselves are models: romfs/valley/places, drawn by render3d.)
}

void buildValleyHorizon(const Valley& v, ValleyMesh& out) {
    out.clear();
    constexpr int kRays = 96;
    const float half = v.size() * 0.5f, cx = v.x0 + half, cy = v.y0 + half;
    Vec3 peak[kRays];
    for (int k = 0; k < kRays; ++k) {
        const float a = k * (2 * kPi / kRays), dx = std::cos(a), dy = std::sin(a);
        peak[k] = {cx + dx * half * 0.6f, cy + dy * half * 0.6f, v.water};
        for (float r = half * 0.55f; r < half * 0.99f; r += 8.0f) {
            const float x = cx + dx * r, y = cy + dy * r, h = v.heightAt(x, y);
            if (h > peak[k].z) peak[k] = {x, y, h};
        }
    }
    for (int k = 0; k < kRays; ++k) {  // three to a ray: the foot, the shoulder, the top
        const Vec3 p = peak[k];
        const float snow = std::fmin(1.0f, std::fmax(0.0f, (p.z - 190.0f) / 90.0f));
        auto shade = [&](float t, float bright) {
            const float r = 150 + (240 - 150) * t, g = 136 + (244 - 136) * t, b = 164 + (252 - 164) * t;
            return std::array<u8, 3>{static_cast<u8>(r * bright), static_cast<u8>(g * bright), static_cast<u8>(b * bright)};
        };
        const float lit = 0.86f + 0.14f * std::cos(k * (2 * kPi / kRays) - 2.4f);  // the sun's side a touch brighter
        const auto foot = shade(0.0f, lit * 0.9f), shoulder = shade(snow * 0.4f, lit), top = shade(snow, lit * 1.04f);
        addVertex(out, {p.x, p.y, v.water - 2.0f}, foot[0], foot[1], foot[2]);
        addVertex(out, {p.x, p.y, v.water + (p.z - v.water) * 0.62f}, shoulder[0], shoulder[1], shoulder[2]);
        addVertex(out, {p.x, p.y, p.z}, top[0], top[1], top[2]);
    }
    for (int k = 0; k < kRays; ++k) {
        const u16 a = static_cast<u16>(k * 3), b = static_cast<u16>(((k + 1) % kRays) * 3);
        for (int s = 0; s < 2; ++s) {
            tri(out, static_cast<u16>(a + s), static_cast<u16>(b + s), static_cast<u16>(b + s + 1));
            tri(out, static_cast<u16>(a + s), static_cast<u16>(b + s + 1), static_cast<u16>(a + s + 1));
        }
    }
}

void buildValleyWater(const Valley& v, ValleyMesh& out, Vec2 centre, float radius) {
    out.clear();
    // The water's surface: a disc round `centre` (the camera: past its edge the haze has it).
    const u8 blue[4] = {70, 140, 178, 170};
    constexpr int kRim = 24;
    const u16 mid = addVertex(out, {centre.x, centre.y, v.water}, blue[0], blue[1], blue[2], blue[3]);
    for (int k = 0; k < kRim; ++k) {
        const float a = k * (2 * kPi / kRim);
        addVertex(out, {centre.x + std::cos(a) * radius, centre.y + std::sin(a) * radius, v.water}, blue[0], blue[1],
                  blue[2], blue[3]);
    }
    for (int k = 0; k < kRim; ++k) tri(out, mid, static_cast<u16>(mid + 1 + k), static_cast<u16>(mid + 1 + (k + 1) % kRim));
    // The waterfall: a curtain off the den's plateau, in front of the grotto behind the falls,
    // arcing out a little as it drops into the pool.
    if (const ValleyPlaceInfo* grotto = v.place(kPlaceGrotto)) {
        const float y = grotto->at.y, cliff = grotto->at.x + 6.0f;  // (the grotto sits 6 m into the cliff)
        const float top = v.heightAt(cliff - 22.0f, y) - 1.0f;       // the plateau's stream
        constexpr int kSteps = 10;
        u16 prev[2] = {};
        for (int s = 0; s <= kSteps; ++s) {
            const float t = static_cast<float>(s) / kSteps;
            const float x = cliff - 12.0f + 20.0f * std::sqrt(t), z = top + (v.water - 0.5f - top) * t;
            const u8 w = static_cast<u8>(200 + 40 * (s % 2));
            const float spread = 3.5f + 2.5f * t;  // wider as it falls
            const u16 l = addVertex(out, {x, y - spread, z}, w, w, 255, 200), r = addVertex(out, {x, y + spread, z}, w, w, 255, 200);
            if (s > 0) {
                tri(out, prev[0], l, r);
                tri(out, prev[0], r, prev[1]);
                tri(out, prev[0], r, l);  // seen from either side
                tri(out, prev[0], prev[1], r);
            }
            prev[0] = l;
            prev[1] = r;
        }
    }
}

}  // namespace ec
