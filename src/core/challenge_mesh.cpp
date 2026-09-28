#include "core/challenge_mesh.hpp"

#include <cmath>

namespace ec {
namespace {

constexpr float kPi = 3.14159265f;

// Appends vertices and triangles, counter-clockwise seen from outside (the dragons' program
// culls back faces), as core/prop_mesh's builder does; only what these things need.
struct Shaper {
    PropMesh& m;
    u8 slot = 0;
    u8 emit = 0;
    Mat34 place = Mat34::identity();  // where what's built goes (the den's shelves)

    u16 vert(Vec3 p, Vec3 n) {
        m.pos.push_back(transformPoint(place, p));
        m.nrm.push_back(normalize(transformDir(place, n)));
        m.paint.insert(m.paint.end(), {slot, slot, 0, emit});
        return static_cast<u16>(m.pos.size() - 1);
    }
    void tri(u16 a, u16 b, u16 c) { m.idx.insert(m.idx.end(), {a, b, c}); }
    void quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 n, bool twoSided = false) {
        for (int side = 0; side < (twoSided ? 2 : 1); ++side) {
            const Vec3 nn = side ? n * -1.0f : n;
            const u16 i0 = vert(a, nn), i1 = vert(b, nn), i2 = vert(c, nn), i3 = vert(d, nn);
            if (side) {
                tri(i0, i2, i1);
                tri(i0, i3, i2);
            } else {
                tri(i0, i1, i2);
                tri(i0, i2, i3);
            }
        }
    }
    // A flat fan round a centre (the ring counter-clockwise seen from n).
    void fan(Vec3 c, const Vec3* ring, int n, Vec3 normal) {
        const u16 mid = vert(c, normal), first = static_cast<u16>(m.pos.size());
        for (int k = 0; k < n; ++k) vert(ring[k], normal);
        for (int k = 0; k < n; ++k) tri(mid, static_cast<u16>(first + k), static_cast<u16>(first + (k + 1) % n));
    }
    // A tube along +Z-ish points with radii (smooth sides), capped if asked; `inward` faces in.
    void lathe(const Vec3* pts, const float* r, int n, int sides, bool capStart, bool capEnd, float twist = 0,
               bool inward = false) {
        const Vec3 w = normalize(pts[n - 1] - pts[0]);
        const Vec3 helper = std::fabs(w.z) < 0.9f ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        const Vec3 u = normalize(cross(helper, w)), v = cross(w, u);
        auto dir = [&](int k) {
            const float a = twist + 2 * kPi * k / sides;
            return u * std::cos(a) + v * std::sin(a);
        };
        const u16 first = static_cast<u16>(m.pos.size());
        for (int i = 0; i < n; ++i)
            for (int k = 0; k < sides; ++k) vert(pts[i] + dir(k) * r[i], inward ? dir(k) * -1.0f : dir(k));
        for (int i = 0; i + 1 < n; ++i)
            for (int k = 0; k < sides; ++k) {
                const u16 a0 = static_cast<u16>(first + i * sides + k), a1 = static_cast<u16>(first + i * sides + (k + 1) % sides);
                const u16 b0 = static_cast<u16>(a0 + sides), b1 = static_cast<u16>(a1 + sides);
                if (inward) {
                    tri(a0, b1, a1);
                    tri(a0, b0, b1);
                } else {
                    tri(a0, a1, b1);
                    tri(a0, b1, b0);
                }
            }
        auto cap = [&](int i, bool end) {
            Vec3 ring[12];
            for (int k = 0; k < sides && k < 12; ++k) ring[end ? k : sides - 1 - k] = pts[i] + dir(k) * r[i];
            fan(pts[i], ring, sides, end ? w : w * -1.0f);
        };
        if (capStart) cap(0, false);
        if (capEnd) cap(n - 1, true);
    }
    void frustum(Vec3 a, Vec3 b, float ra, float rb, int sides, bool capA, bool capB, float twist = 0) {
        const Vec3 pts[2] = {a, b};
        const float r[2] = {ra, rb};
        lathe(pts, r, 2, sides, capA, capB, twist);
    }
    template <typename SlotFn>
    void ellipsoid(Vec3 c, Vec3 radii, int segs, int rings, SlotFn slotAt) {
        const u16 first = static_cast<u16>(m.pos.size());
        const u8 keep = slot;
        for (int i = 0; i <= rings; ++i) {
            const float th = kPi * i / rings;
            for (int k = 0; k <= segs; ++k) {
                const float ph = 2 * kPi * k / segs;
                const Vec3 s{std::sin(th) * std::cos(ph), std::sin(th) * std::sin(ph), std::cos(th)};
                slot = slotAt(s);
                vert(c + Vec3{s.x * radii.x, s.y * radii.y, s.z * radii.z}, {s.x / radii.x, s.y / radii.y, s.z / radii.z});
            }
        }
        slot = keep;
        for (int i = 0; i < rings; ++i)
            for (int k = 0; k < segs; ++k) {
                const u16 a = static_cast<u16>(first + i * (segs + 1) + k), b = static_cast<u16>(a + segs + 1);
                if (i > 0) tri(a, b, static_cast<u16>(a + 1));
                if (i < rings - 1) tri(static_cast<u16>(a + 1), b, static_cast<u16>(b + 1));
            }
    }
    void ellipsoid(Vec3 c, Vec3 radii, int segs, int rings) {
        const u8 s = slot;
        ellipsoid(c, radii, segs, rings, [s](Vec3) { return s; });
    }
    // A torus round `axis` through c (major radius R, tube r); slotAt picks by the tube's angle.
    template <typename SlotFn>
    void torus(Vec3 c, Vec3 axis, float R, float r, int segs, int sides, SlotFn slotAt) {
        const Vec3 w = normalize(axis);
        const Vec3 helper = std::fabs(w.z) < 0.9f ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        const Vec3 u = normalize(cross(helper, w)), v = cross(w, u);
        const u16 first = static_cast<u16>(m.pos.size());
        const u8 keep = slot;
        for (int i = 0; i < segs; ++i) {
            const float a = 2 * kPi * i / segs;
            const Vec3 out = u * std::cos(a) + v * std::sin(a);
            for (int k = 0; k < sides; ++k) {
                const float b = 2 * kPi * k / sides;
                const Vec3 n = out * std::cos(b) + w * std::sin(b);
                slot = slotAt(std::cos(b));  // -1: the inside of the ring, 1: its outside
                vert(c + out * R + n * r, n);
            }
        }
        slot = keep;
        for (int i = 0; i < segs; ++i)
            for (int k = 0; k < sides; ++k) {
                const u16 a0 = static_cast<u16>(first + i * sides + k), a1 = static_cast<u16>(first + i * sides + (k + 1) % sides);
                const u16 b0 = static_cast<u16>(first + ((i + 1) % segs) * sides + k),
                          b1 = static_cast<u16>(first + ((i + 1) % segs) * sides + (k + 1) % sides);
                tri(a0, b0, b1);
                tri(a0, b1, a1);
            }
    }
    // A little diamond (an octahedron): the ring's studs, a lantern's sparkle.
    void diamond(Vec3 c, float r, float tall) {
        const Vec3 top = c + Vec3{0, 0, tall}, bottom = c - Vec3{0, 0, tall};
        const Vec3 e[4] = {c + Vec3{r, 0, 0}, c + Vec3{0, r, 0}, c + Vec3{-r, 0, 0}, c + Vec3{0, -r, 0}};
        for (int k = 0; k < 4; ++k) {
            const Vec3 a = e[k], b = e[(k + 1) % 4];
            const Vec3 nt = cross(b - a, top - a), nb = cross(bottom - a, b - a);
            const u16 t0 = vert(a, nt), t1 = vert(b, nt), t2 = vert(top, nt);
            tri(t0, t1, t2);
            const u16 b0 = vert(a, nb), b1 = vert(bottom, nb), b2 = vert(b, nb);
            tri(b0, b1, b2);
        }
    }
};

Vec3 onCircle(float a, float r, float z = 0) { return {std::cos(a) * r, std::sin(a) * r, z}; }

PropLook look(Rgb a, Rgb b, Rgb c, Rgb d, float glow = 0) {
    PropLook l;
    l.colour[0] = a;
    l.colour[1] = b;
    l.colour[2] = c;
    l.colour[3] = d;
    l.glow = glow;
    return l;
}

Rgb mix(Rgb a, Rgb b, float t) {
    return {static_cast<u8>(a.r + (b.r - a.r) * t), static_cast<u8>(a.g + (b.g - a.g) * t),
            static_cast<u8>(a.b + (b.b - a.b) * t)};
}

// A round fruit (or a pear: a smaller bulb on top), a stem and a leaf; 1 m across.
void fruit(Shaper& b, challenge::Fruit f, Vec3 at, float scale) {
    b.slot = 0;
    const bool pear = f == challenge::Fruit::Pear || f == challenge::Fruit::Golden;
    const float h = f == challenge::Fruit::Plum ? 0.54f : 0.47f;
    b.ellipsoid(at, Vec3{0.5f, 0.5f, h} * scale, 8, 6);
    float top = h;
    if (pear) {
        b.ellipsoid(at + Vec3{0, 0, 0.42f * scale}, Vec3{0.3f, 0.3f, 0.32f} * scale, 7, 4);
        top = 0.72f;
    }
    b.slot = 1;
    b.frustum(at + Vec3{0, 0, (top - 0.05f) * scale}, at + Vec3{0.04f, 0, (top + 0.2f) * scale}, 0.05f * scale,
              0.035f * scale, 4, false, true);
    b.slot = 2;
    const Vec3 base = at + Vec3{0.02f, 0, (top + 0.1f) * scale};
    const Vec3 tip = base + Vec3{0.34f, 0.08f, 0.1f} * scale, mid = lerp(base, tip, 0.45f) + Vec3{0, 0, 0.06f} * scale;
    const Vec3 side = Vec3{-0.05f, 0.2f, 0} * scale;
    b.quad(base, mid + side * 0.5f, tip, mid - side * 0.5f, {0, 0, -1}, true);  // (its winding faces down)
}

}  // namespace

PropMesh ringMesh() {
    PropMesh m;
    Shaper b{m};
    b.emit = 0;
    // The band: its inner half glows (slot 1), its outer half is the ring's colour.
    b.torus({0, 0, 0}, {0, 1, 0}, 1.0f, 0.1f, 18, 5, [](float out) -> u8 { return out < -0.2f ? 1 : 0; });
    for (std::size_t v = 0; v < m.paint.size(); v += 4)
        if (m.paint[v] == 1) m.paint[v + 3] = 255;
    b.slot = 2;  // four little stars round it
    for (int k = 0; k < 4; ++k) {
        const float a = kPi * 0.25f + k * kPi * 0.5f;
        b.diamond({std::cos(a) * 1.0f, 0, std::sin(a) * 1.0f}, 0.12f, 0.18f);
    }
    return m;
}

PropMesh crystalLanternMesh() {
    PropMesh m;
    Shaper b{m};
    b.slot = 0;  // the stone post, a cap
    b.frustum({0, 0, 0}, {0, 0, 1.0f}, 0.32f, 0.24f, 6, false, false);
    b.frustum({0, 0, 1.0f}, {0, 0, 1.1f}, 0.31f, 0.3f, 6, true, true);
    b.slot = 1;  // a brass cup, and four prongs up round the crystal
    b.frustum({0, 0, 1.1f}, {0, 0, 1.24f}, 0.2f, 0.28f, 6, false, true, kPi / 6);
    for (int k = 0; k < 4; ++k) {
        const float a = kPi * 0.25f + k * kPi * 0.5f;
        b.frustum(onCircle(a, 0.24f, 1.2f), onCircle(a, 0.3f, 1.6f), 0.03f, 0.022f, 3, false, true);
    }
    b.slot = 2;  // the crystal: six-sided, pointed above and below
    b.emit = 255;
    constexpr int kSides = 6;
    const Vec3 top{0, 0, 2.0f}, bottom{0, 0, 1.22f};
    for (int k = 0; k < kSides; ++k) {
        const Vec3 a = onCircle(2 * kPi * k / kSides, 0.27f, kCrystalHeight), c = onCircle(2 * kPi * (k + 1) / kSides, 0.27f, kCrystalHeight);
        const Vec3 nt = cross(c - a, top - a), nb = cross(bottom - a, c - a);
        const u16 t0 = b.vert(a, nt), t1 = b.vert(c, nt), t2 = b.vert(top, nt);
        b.tri(t0, t1, t2);
        const u16 b0 = b.vert(a, nb), b1 = b.vert(bottom, nb), b2 = b.vert(c, nb);
        b.tri(b0, b1, b2);
    }
    b.emit = 0;
    return m;
}

PropMesh fruitMesh(challenge::Fruit f) {
    PropMesh m;
    Shaper b{m};
    fruit(b, f, {0, 0, 0}, 1.0f);
    return m;
}

PropMesh basketMesh() {
    PropMesh m;
    Shaper b{m};
    b.slot = 0;  // wicker, both faces, and a rolled rim
    const Vec3 pts[2] = {{0, 0, 0}, {0, 0, 0.36f}};
    const float r[2] = {0.3f, 0.4f}, rin[2] = {0.27f, 0.37f};
    b.lathe(pts, r, 2, 8, true, false);
    b.lathe(pts, rin, 2, 8, false, false, 0, true);
    b.torus({0, 0, 0.36f}, {0, 0, 1}, 0.385f, 0.035f, 8, 4, [](float) -> u8 { return 0; });
    const Vec3 heap[3] = {{-0.12f, -0.06f, 0.3f}, {0.13f, -0.02f, 0.32f}, {0.0f, 0.12f, 0.34f}};
    for (int k = 0; k < 3; ++k) {
        b.slot = static_cast<u8>(1 + k);
        b.ellipsoid(heap[k], {0.14f, 0.14f, 0.13f}, 5, 3);
    }
    return m;
}

PropMesh boardMesh() {
    PropMesh m;
    Shaper b{m};
    b.slot = 0;  // two posts, the board between them
    for (float x : {-0.95f, 0.95f}) b.frustum({x, 0, 0}, {x, 0, kBoardHeight}, 0.07f, 0.06f, 4, false, true, kPi / 4);
    const float x0 = -1.0f, x1 = 1.0f, z0 = 1.0f, z1 = 2.1f, y0 = -0.04f, y1 = 0.04f;
    b.quad({x1, y1, z0}, {x0, y1, z0}, {x0, y1, z1}, {x1, y1, z1}, {0, 1, 0});   // front (+Y)
    b.quad({x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}, {0, -1, 0});  // back
    b.quad({x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}, {0, 0, 1});   // top edge
    b.quad({x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0}, {x1, y0, z0}, {0, 0, -1});  // bottom edge
    b.slot = 1;  // a little gabled roof
    const float rz = 2.25f, peak = 2.55f, ry = 0.32f;
    b.quad({1.2f, ry, rz}, {-1.2f, ry, rz}, {-1.2f, 0, peak}, {1.2f, 0, peak}, {0, 1, 1.0f}, true);
    b.quad({-1.2f, -ry, rz}, {1.2f, -ry, rz}, {1.2f, 0, peak}, {-1.2f, 0, peak}, {0, -1, 1.0f}, true);
    // Three posters, a little proud of the board, each with its challenge's picture.
    for (int k = 0; k < 3; ++k) {
        const float cx = -0.62f + 0.62f * k, cz = 1.55f + (k == 1 ? 0.05f : -0.02f);
        const float w = 0.26f, h = 0.36f, y = 0.05f;
        b.slot = 2;
        b.quad({cx + w, y, cz - h}, {cx - w, y, cz - h}, {cx - w, y, cz + h}, {cx + w, y, cz + h}, {0, 1, 0});
        b.slot = 3;
        const float py = 0.06f;
        if (k == 0) {  // a ring
            for (int s = 0; s < 8; ++s) {
                const float a0 = 2 * kPi * s / 8, a1 = 2 * kPi * (s + 1) / 8;
                auto at = [&](float a, float r) { return Vec3{cx + std::cos(a) * r, py, cz + 0.05f + std::sin(a) * r}; };
                b.quad(at(a0, 0.16f), at(a0, 0.1f), at(a1, 0.1f), at(a1, 0.16f), {0, 1, 0});
            }
        } else if (k == 1) {  // a lantern's crystal
            b.quad({cx, py, cz - 0.2f}, {cx - 0.12f, py, cz + 0.04f}, {cx, py, cz + 0.28f}, {cx + 0.12f, py, cz + 0.04f}, {0, 1, 0});
        } else {  // an apple
            Vec3 ring[8];
            for (int s = 0; s < 8; ++s) ring[7 - s] = {cx + std::cos(2 * kPi * s / 8) * 0.14f, py, cz + std::sin(2 * kPi * s / 8) * 0.14f};
            b.fan({cx, py, cz}, ring, 8, {0, 1, 0});
            b.quad({cx + 0.02f, py, cz + 0.12f}, {cx - 0.01f, py, cz + 0.12f}, {cx, py, cz + 0.22f}, {cx + 0.03f, py, cz + 0.22f}, {0, 1, 0});
        }
    }
    return m;
}

namespace {

// A trophy: a stepped base, a stem, the cup with two handles, its challenge's sign on top; a
// cup's colour in `cupSlot`, the base in `baseSlot`, the sign in `signSlot` (about 120 triangles).
void trophy(Shaper& b, Challenge c, u8 cupSlot, u8 baseSlot, u8 signSlot) {
    b.slot = baseSlot;
    b.frustum({0, 0, 0}, {0, 0, 0.1f}, 0.2f, 0.18f, 4, false, true, kPi / 4);
    b.slot = cupSlot;
    b.emit = 255;  // it gleams a little (its palette slot's glow), even at the back of the den
    b.frustum({0, 0, 0.1f}, {0, 0, 0.29f}, 0.05f, 0.03f, 5, false, false);
    const Vec3 cup[3] = {{0, 0, 0.29f}, {0, 0, 0.38f}, {0, 0, 0.48f}};
    const float r[3] = {0.03f, 0.14f, 0.17f};
    b.lathe(cup, r, 3, 7, false, true);  // closed on top: it gleams there
    for (float sx : {-1.0f, 1.0f}) {
        const Vec3 arc[3] = {{sx * 0.15f, 0, 0.45f}, {sx * 0.25f, 0, 0.41f}, {sx * 0.12f, 0, 0.33f}};
        for (int k = 0; k < 2; ++k) b.frustum(arc[k], arc[k + 1], 0.02f, 0.02f, 3, false, false);
    }
    b.emit = 0;
    b.slot = signSlot;
    const Vec3 top{0, 0, 0.5f};
    switch (c) {
        case Challenge::SkyRings:  // a little ring, standing up to face the room
            b.torus(top + Vec3{0, 0, 0.08f}, {0, 1, 0}, 0.07f, 0.02f, 7, 3, [signSlot](float) { return signSlot; });
            break;
        case Challenge::LanternTrial:  // a flame
            b.emit = 255;
            b.ellipsoid(top + Vec3{0, 0, 0.07f}, {0.045f, 0.045f, 0.075f}, 5, 3);
            b.emit = 0;
            break;
        default:  // an apple
            b.ellipsoid(top + Vec3{0, 0, 0.055f}, {0.06f, 0.06f, 0.055f}, 6, 4);
            b.frustum(top + Vec3{0, 0, 0.1f}, top + Vec3{0.01f, 0, 0.14f}, 0.012f, 0.01f, 3, false, false);
            break;
    }
}

// A rosette for a cup's ribbon, its back to +Y (the wall): pleats, a button, two tails.
void rosette(Shaper& b, u8 pleatSlot, u8 buttonSlot) {
    constexpr int kPoints = 10;
    const Vec3 face{0, -1, 0};
    b.slot = pleatSlot;
    b.emit = 255;  // the satin catches the light
    Vec3 ring[kPoints];
    for (int k = 0; k < kPoints; ++k) {  // counter-clockwise seen from the room
        const float a = 2 * kPi * k / kPoints, r = k % 2 ? 0.13f : 0.17f;
        ring[k] = {std::cos(a) * r, 0, std::sin(a) * r};
    }
    b.fan({0, -0.01f, 0}, ring, kPoints, face);
    b.emit = 0;
    b.slot = buttonSlot;
    Vec3 button[5];
    for (int k = 0; k < 5; ++k) {
        const float a = 2 * kPi * k / 5 + kPi / 2;
        button[k] = {std::cos(a) * 0.075f, -0.02f, std::sin(a) * 0.075f};
    }
    b.fan({0, -0.03f, 0}, button, 5, face);
    b.slot = pleatSlot;  // two short tails in a V under it (longer ones read as legs from across the den)
    for (float sx : {-1.0f, 1.0f}) {
        const Vec3 a{sx * 0.03f, 0.005f, -0.08f}, c{sx * 0.08f, 0.005f, -0.25f};
        const Vec3 side{0.04f, 0, 0};
        b.quad(c - side, c + side, a + side, a - side, face);
    }
}

// The palette slots of the den's shelf (shelfMesh): a cup's colour, a challenge's sign, the base.
constexpr u8 kShelfCup = 0, kShelfSign = 4, kShelfBase = 7;

Rgb signColour(Challenge c) {
    switch (c) {
        case Challenge::SkyRings: return {120, 200, 240};
        case Challenge::LanternTrial: return {255, 214, 110};
        default: return {226, 72, 64};
    }
}

}  // namespace

PropMesh trophyMesh(Challenge c) {
    PropMesh m;
    Shaper b{m};
    trophy(b, c, 0, 1, 2);
    return m;
}

PropMesh rosetteMesh() {
    PropMesh m;
    Shaper b{m};
    rosette(b, 0, 1);
    return m;
}

PropMesh shelfMesh(const u8 cups[kChallenges], u16 ribbons) {
    PropMesh m;
    Shaper b{m};
    auto at = [&](const DecorPlace& p) { b.place = fromQuatScale(quatAxisAngle({0, 0, 1}, p.yaw), {p.scale, p.scale, p.scale}, p.at); };
    for (int c = 0; c < kChallenges; ++c) {
        if (cups[c] < challenge::kEmber) continue;
        const int cup = cups[c] > challenge::kStarfire ? static_cast<int>(challenge::kStarfire) : static_cast<int>(cups[c]);
        DecorPlace spot = trophySpot(static_cast<Challenge>(c));
        spot.scale *= 0.85f + 0.1f * cup;  // a grander trophy cup by cup (D89: trophies that mean something)
        at(spot);
        trophy(b, static_cast<Challenge>(c), static_cast<u8>(kShelfCup + cup - 1), kShelfBase, static_cast<u8>(kShelfSign + c));
    }
    int slot = 0;
    for (int bit = 0; bit < kChallenges * kCups; ++bit) {
        if (!((ribbons >> bit) & 1u)) continue;
        at(ribbonSpot(slot++));
        rosette(b, static_cast<u8>(kShelfCup + bit % kCups), static_cast<u8>(kShelfSign + bit / kCups));
    }
    return m;
}

void shelfPalette(Rgb out[kPalCount], float glow[kPalCount]) {
    for (int k = 0; k < kPalCount; ++k) {
        out[k] = {110, 72, 52};
        glow[k] = 0;
    }
    for (int cup = challenge::kEmber; cup <= challenge::kStarfire; ++cup) {
        out[kShelfCup + cup - 1] = challenge::cupColour(cup);
        glow[kShelfCup + cup - 1] = 0.3f;
    }
    for (int c = 0; c < kChallenges; ++c) out[kShelfSign + c] = signColour(static_cast<Challenge>(c));
    glow[kShelfSign + static_cast<int>(Challenge::LanternTrial)] = 0.8f;  // the lantern trophy's flame
}

// ------------------------------------------------------------------------------ Driftwood Cove
namespace {

// A scallop's fan: striped (the two slots in turn), domed, its hinge toward -Y.
void scallop(Shaper& b, Vec3 at, float size, u8 slotA, u8 slotB) {
    constexpr int kRibs = 9;
    const Vec3 middle = at + Vec3{0, -0.01f, 0.03f} * size;
    Vec3 ring[kRibs + 1];
    for (int k = 0; k <= kRibs; ++k) {  // an arc from the right round the far side to the left (counter-clockwise from above)
        const float a = -0.15f * kPi + 1.3f * kPi * k / kRibs;
        ring[k] = at + Vec3{std::cos(a) * 0.1f, std::sin(a) * 0.09f + 0.01f, 0.004f} * size;
    }
    for (int k = 0; k < kRibs; ++k) {
        b.slot = k % 2 ? slotB : slotA;
        const Vec3 n = normalize(cross(ring[k] - middle, ring[k + 1] - middle));
        b.tri(b.vert(middle, n), b.vert(ring[k], n), b.vert(ring[k + 1], n));
    }
    b.slot = slotA;  // the hinge's little ears
    const Vec3 h = at + Vec3{0, -0.075f, 0.01f} * size;
    b.quad(h + Vec3{-0.04f, -0.02f, 0} * size, h + Vec3{0.04f, -0.02f, 0} * size, h + Vec3{0.03f, 0.02f, 0.012f} * size,
           h + Vec3{-0.03f, 0.02f, 0.012f} * size, {0, 0, 1});
}

}  // namespace

PropMesh shellMesh(int kind) {
    PropMesh m;
    Shaper b{m};
    switch (kind) {
        case 0: {  // a spiral: a cone lying on the sand, its open mouth ringed with its lip
            b.slot = 0;
            const Vec3 mouth{0, -0.07f, 0.05f}, tip{0.01f, 0.1f, 0.025f};
            b.frustum(mouth, tip, 0.05f, 0.006f, 7, false, true);
            b.slot = 1;
            b.torus(mouth, tip - mouth, 0.05f, 0.012f, 7, 3, [](float) { return static_cast<u8>(1); });
            b.slot = 0;
            b.ellipsoid(mouth + Vec3{0, 0.05f, 0.012f}, {0.045f, 0.04f, 0.04f}, 6, 3);  // the round of its body
            break;
        }
        case 2:  // a cowrie: a glossy dome, its underside's lip showing
            b.ellipsoid({0, 0, 0.028f}, {0.055f, 0.08f, 0.03f}, 8, 4, [](Vec3 s) { return static_cast<u8>(s.z > 0.2f ? 0 : 1); });
            break;
        case 3:  // a pearl in an open half-shell
            scallop(b, {0, 0, 0}, 1.1f, 1, 1);
            b.slot = 2;
            b.emit = 255;
            b.ellipsoid({0, 0.005f, 0.05f}, {0.028f, 0.028f, 0.028f}, 7, 4);
            b.emit = 0;
            break;
        default: scallop(b, {0, 0, 0}, 1.0f, 0, 1); break;  // a scallop
    }
    return m;
}

PropMesh bobberMesh() {
    PropMesh m;
    Shaper b{m};
    b.ellipsoid({0, 0, 0.02f}, {0.09f, 0.09f, 0.09f}, 8, 6, [](Vec3 s) { return static_cast<u8>(s.z > 0.1f ? 0 : 1); });
    b.slot = 2;
    b.frustum({0, 0, 0.1f}, {0, 0, 0.21f}, 0.012f, 0.008f, 4, false, true);
    return m;
}

PropMesh fishMesh() {
    PropMesh m;
    Shaper b{m};
    // A body flattened side to side, its belly pale.
    b.ellipsoid({0, 0, 0}, {0.085f, 0.48f, 0.15f}, 8, 6, [](Vec3 s) { return static_cast<u8>(s.z < -0.25f ? 1 : 0); });
    // The tail and the fin on its back (both sides), the eyes.
    b.slot = 2;
    auto fin = [&](Vec3 a, Vec3 c, Vec3 d) {
        for (int side = 0; side < 2; ++side) {
            const Vec3 n{side ? -1.0f : 1.0f, 0, 0};
            const u16 i0 = b.vert(a, n), i1 = b.vert(c, n), i2 = b.vert(d, n);
            if (side) b.tri(i0, i2, i1);
            else b.tri(i0, i1, i2);
        }
    };
    fin({0, -0.42f, 0}, {0, -0.74f, -0.2f}, {0, -0.74f, 0.2f});
    fin({0, -0.66f, 0}, {0, -0.78f, -0.1f}, {0, -0.78f, 0.1f});
    fin({0, 0.12f, 0.13f}, {0, -0.22f, 0.12f}, {0, -0.08f, 0.27f});
    fin({0, 0.05f, -0.13f}, {0, -0.1f, -0.22f}, {0, -0.14f, -0.12f});
    b.slot = 3;
    for (float sx : {-1.0f, 1.0f}) b.ellipsoid({sx * 0.062f, 0.33f, 0.04f}, {0.018f, 0.026f, 0.026f}, 5, 3);
    return m;
}

PropLook shellLook(int kind, int tint) {
    static constexpr Rgb kSand[3] = {{240, 214, 178}, {246, 196, 170}, {236, 222, 206}};
    const Rgb base = kSand[(tint % 3 + 3) % 3];
    switch (kind) {
        case 0: return look(base, {244, 160, 140}, {0, 0, 0}, {0, 0, 0});
        case 2: return look({226, 186, 140}, {176, 112, 76}, {0, 0, 0}, {0, 0, 0}, 0.0f);
        case 3: return look({214, 196, 188}, {236, 216, 204}, {252, 248, 255}, {0, 0, 0}, 0.35f);
        default: return look({244, 174, 142}, base, {0, 0, 0}, {0, 0, 0});
    }
}

PropLook bobberLook() { return look({214, 60, 56}, {250, 246, 240}, {70, 60, 60}, {0, 0, 0}); }

PropLook fishLook(bool big) {
    return big ? look({78, 142, 118}, {240, 232, 196}, {236, 146, 92}, {30, 24, 30})
               : look({72, 150, 196}, {244, 238, 220}, {244, 140, 110}, {30, 24, 30});
}

PropLook ringLook(int cup, bool next) {
    const Rgb band = next ? Rgb{250, 204, 90} : mix(challenge::cupColour(cup), Rgb{250, 250, 255}, 0.5f);
    const Rgb glow = next ? Rgb{255, 240, 180} : Rgb{200, 230, 255};
    return look(band, glow, {255, 250, 230}, {0, 0, 0}, next ? 1.0f : 0.45f);
}

PropLook crystalLook(int k, float lit) {
    const Rgb c = challenge::trialColour(k);
    return look({190, 178, 196}, {196, 150, 70}, mix(c, Rgb{255, 255, 255}, 0.25f * lit), {0, 0, 0}, 0.18f + 0.82f * lit);
}

PropLook fruitLook(challenge::Fruit f) {
    return look(challenge::fruitColour(f), {110, 80, 50}, {96, 170, 70}, {0, 0, 0}, f == challenge::Fruit::Golden ? 0.35f : 0.0f);
}

PropLook basketLook() {
    return look({204, 164, 104}, challenge::fruitColour(challenge::Fruit::Apple), challenge::fruitColour(challenge::Fruit::Pear),
                challenge::fruitColour(challenge::Fruit::Plum));
}

PropLook boardLook() { return look({150, 104, 66}, {200, 88, 70}, {250, 240, 220}, {70, 56, 86}); }

PropLook trophyLook(Challenge c, int cup) {
    return look(challenge::cupColour(cup), {110, 72, 52}, signColour(c), {0, 0, 0}, 0.3f);
}

PropLook rosetteLook(Challenge c, int cup) { return look(challenge::cupColour(cup), signColour(c), {0, 0, 0}, {0, 0, 0}); }

namespace {
// The den's shelves (tools/blender/den_model.py shelves(): SHELF_A = -8 degrees, set 0.35 in from the
// wall at 2.2 up; local +Y at the wall, +X along it; the planks at 1.5 and 2.6, 0.12 thick).
constexpr float kShelfA = -8.0f * kPi / 180.0f, kShelfR = 9.16f;
Vec3 shelfPoint(float dx, float dy, float z) {
    const Vec3 o{kShelfR * std::sin(kShelfA), kShelfR * std::cos(kShelfA), 0};
    const Vec3 x{std::cos(-kShelfA), std::sin(-kShelfA), 0}, y{-std::sin(-kShelfA), std::cos(-kShelfA), 0};
    return o + x * dx + y * dy + Vec3{0, 0, z};
}
}  // namespace

DecorPlace trophySpot(Challenge c) {
    // Beside the jars: the top shelf's two ends, the bottom shelf's right end.
    static constexpr float kSpots[kChallenges][2] = {{1.02f, 1.62f}, {-1.02f, 2.72f}, {1.0f, 2.72f}};
    const float* s = kSpots[static_cast<int>(c) < kChallenges ? static_cast<int>(c) : 0];
    return {shelfPoint(s[0], 0.02f, s[1]), -kShelfA, 1.0f};
}

DecorPlace ribbonSpot(int slot) {
    const int row = slot / 6, k = slot % 6;
    return {shelfPoint(-1.1f + 0.44f * k, -0.27f, row == 0 ? 2.47f : 1.37f), -kShelfA, 1.0f};
}

}  // namespace ec
