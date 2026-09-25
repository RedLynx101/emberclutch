#include "core/valley.hpp"

#include <cmath>
#include <cstring>

#include "core/byte_reader.hpp"

namespace ec {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kSkirt = 3.0f;         // metres a skirt hangs below the edge, at full detail
constexpr float kLodNear = 48.0f;      // tiles nearer than this: full detail
constexpr float kLodMid = 140.0f;      // then half; beyond, a quarter
constexpr int kTreeSides[kTreeLods] = {4, 3};

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

}  // namespace

void ValleyMesh::clear() {
    pos.clear();
    color.clear();
    idx.clear();
}

bool loadValley(const u8* data, std::size_t size, Valley& out) {
    ByteReader r(data, size);
    char magic[4];
    r.bytes(magic, 4);
    if (!r.ok() || std::memcmp(magic, "EVL1", 4) != 0 || r.u16v() != 1) return false;
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
        t.height = r.u8v();
        t.shade = r.u8v();
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

int valleyLodFor(float distance) { return distance < kLodNear ? 0 : (distance < kLodMid ? 1 : 2); }

void buildValleyTile(const Valley& v, int tx, int ty, int lod, ValleyMesh& out) {
    out.clear();
    const int t = v.tiles();
    if (tx < 0 || ty < 0 || tx >= t || ty >= t) return;
    lod = lod < 0 ? 0 : (lod >= kValleyLods ? kValleyLods - 1 : lod);
    const int step = 1 << lod, quads = kTileQuads / step, side = quads + 1;
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
    // Skirts: each edge copied a little lower, facing out of the tile.
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
    // Trees, nearer levels only (fewer sides further off).
    if (lod < kTreeLods)
        for (int k : v.tileTrees[std::size_t(ty) * t + tx]) {
            const ValleyTree& tr = v.trees[std::size_t(k)];
            const u8 green[3] = {static_cast<u8>(40 + tr.shade % 24), static_cast<u8>(86 + tr.shade % 30),
                                 static_cast<u8>(46 + tr.shade % 16)};
            tree(out, {tr.x, tr.y, v.heightAt(tr.x, tr.y) - 0.4f}, tr.height, tr.height * 0.32f, kTreeSides[lod], green);
        }
}

void buildValleyExtras(const Valley& v, ValleyMesh& out) {
    out.clear();
    constexpr int kSides = 10;
    for (const ValleyIsland& isl : v.islands) {
        const Vec3 c = isl.at;
        const float r = isl.radius;
        const u8 grass[3] = {104, 156, 78}, rock[3] = {120, 104, 96};
        const u16 top = addVertex(out, c, grass[0], grass[1], grass[2]);
        u16 rim[kSides], lip[kSides];
        for (int s = 0; s < kSides; ++s) {
            const float a = 2 * kPi * s / kSides, wob = 1.0f + 0.12f * std::sin(a * 3 + c.x);
            rim[s] = addVertex(out, {c.x + std::cos(a) * r * wob, c.y + std::sin(a) * r * wob, c.z}, grass[0], grass[1],
                               grass[2]);
        }
        for (int s = 0; s < kSides; ++s) tri(out, top, rim[s], rim[(s + 1) % kSides]);
        // The rocky underside: a lip under the grass, then down to a point.
        const Vec3 apex{c.x + r * 0.1f, c.y - r * 0.08f, c.z - r * 1.7f};
        for (int s = 0; s < kSides; ++s) {
            const float a = 2 * kPi * s / kSides;
            const Vec3 p{c.x + std::cos(a) * r * 0.92f, c.y + std::sin(a) * r * 0.92f, c.z - r * 0.28f};
            u8 col[3];
            shaded(rock, Vec3{std::cos(a), std::sin(a), -0.2f}, col);
            lip[s] = addVertex(out, p, col[0], col[1], col[2]);
        }
        for (int s = 0; s < kSides; ++s) {
            const int e = (s + 1) % kSides;
            const Vec3 a0 = out.pos[rim[s]], a1 = out.pos[rim[e]];
            u8 col[3];
            shaded(rock, cross(a1 - a0, out.pos[lip[s]] - a0) * -1.0f, col);
            const u16 r0 = addVertex(out, a0, col[0], col[1], col[2]), r1 = addVertex(out, a1, col[0], col[1], col[2]);
            tri(out, r0, lip[s], lip[e]);
            tri(out, r0, lip[e], r1);
            const u16 tip = addVertex(out, apex, col[0] / 2, col[1] / 2, col[2] / 2);
            tri(out, lip[s], tip, lip[e]);
        }
        const u8 green[3] = {44, 96, 50};
        for (int k = 0; k < 3; ++k) {
            const float a = k * 2.1f + c.y;
            tree(out, {c.x + std::cos(a) * r * 0.45f, c.y + std::sin(a) * r * 0.45f, c.z - 0.2f}, 7.0f + 2 * k, 2.4f, 6,
                 green);
        }
    }
    // The den's cave mouth: a dark arch in the cliff, facing out of it.
    if (const ValleyPlaceInfo* den = v.place(kPlaceDen)) {
        const Vec3 fwd{std::sin(den->heading), -std::cos(den->heading), 0};
        const Vec3 side{-fwd.y, fwd.x, 0};
        const Vec3 base = den->at + fwd * 8.0f;
        const float floor = v.heightAt(base.x + fwd.x * 6, base.y + fwd.y * 6) - 1.0f;
        const Vec3 mid{base.x, base.y, floor};
        const u16 c = addVertex(out, mid, 22, 16, 30);
        constexpr int kArch = 8;
        u16 ring[kArch + 1];
        for (int s = 0; s <= kArch; ++s) {
            const float a = kPi * s / kArch;
            ring[s] = addVertex(out, mid + side * (std::cos(a) * 9.0f) + Vec3{0, 0, std::sin(a) * 12.0f}, 40, 30, 50);
        }
        for (int s = 0; s < kArch; ++s) {
            tri(out, c, ring[s], ring[s + 1]);  // both faces: seen from the valley and from above
            tri(out, c, ring[s + 1], ring[s]);
        }
    }
}

void buildValleyWater(const Valley& v, ValleyMesh& out) {
    out.clear();
    const float x1 = v.x0 + v.size(), y1 = v.y0 + v.size(), z = v.water;
    const u8 blue[4] = {70, 140, 178, 170};
    const u16 a = addVertex(out, {v.x0, v.y0, z}, blue[0], blue[1], blue[2], blue[3]);
    const u16 b = addVertex(out, {x1, v.y0, z}, blue[0], blue[1], blue[2], blue[3]);
    const u16 c = addVertex(out, {x1, y1, z}, blue[0], blue[1], blue[2], blue[3]);
    const u16 d = addVertex(out, {v.x0, y1, z}, blue[0], blue[1], blue[2], blue[3]);
    tri(out, a, b, c);
    tri(out, a, c, d);
    // The waterfall: a ribbon down the cliff from the plateau's stream, just proud of the rock.
    if (const ValleyPlaceInfo* den = v.place(kPlaceDen)) {
        const float y = den->at.y + 60.0f;
        constexpr int kSteps = 8;
        u16 prev[2] = {};
        for (int s = 0; s <= kSteps; ++s) {
            const float x = den->at.x - 8.0f + 26.0f * s / kSteps;
            const float z = v.heightAt(x, y) + 0.8f;
            const u8 w = static_cast<u8>(200 + 40 * (s % 2));
            const u16 l = addVertex(out, {x, y - 3.5f, z}, w, w, 255, 200), r = addVertex(out, {x, y + 3.5f, z}, w, w, 255, 200);
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
