#include "core/wear_mesh.hpp"

#include <cmath>

namespace ec {
namespace {

constexpr float kPi = 3.14159265f;

// A right-handed frame (x cross y = z) to place a piece in: points and directions in it.
struct Frame {
    Vec3 o{0, 0, 0}, x{1, 0, 0}, y{0, 1, 0}, z{0, 0, 1};
    Vec3 at(float a, float b, float c) const { return o + x * a + y * b + z * c; }
    Vec3 dir(float a, float b, float c) const { return x * a + y * b + z * c; }
};

// A frame at `o` whose z is `up` and whose y leans toward `toward` (made square to up).
Frame frameAt(Vec3 o, Vec3 up, Vec3 toward) {
    Frame f;
    f.o = o;
    f.z = normalize(up);
    f.y = normalize(toward - f.z * dot(toward, f.z));
    f.x = cross(f.y, f.z);
    return f;
}

// The tail's own: its axis (+Y) as z, angle 0 straight up.
const Frame kTailRing{{0, 0, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}};

// Appends vertices and triangles, counter-clockwise seen from outside (the dragon program culls
// back faces); two-sided pieces get a back face of their own.
struct Builder {
    PropMesh& m;
    u8 slot = 0;
    u8 emit = 0;

    u16 vert(Vec3 p, Vec3 n, u8 s) {
        m.pos.push_back(p);
        m.nrm.push_back(normalize(n));
        m.paint.insert(m.paint.end(), {s, s, 0, emit});
        return static_cast<u16>(m.pos.size() - 1);
    }
    u16 vert(Vec3 p, Vec3 n) { return vert(p, n, slot); }
    void tri(u16 a, u16 b, u16 c) { m.idx.insert(m.idx.end(), {a, b, c}); }

    // A flat quad a b c d, counter-clockwise seen from the side it faces.
    void quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, bool twoSided = false) {
        const Vec3 n = normalize(cross(b - a, c - a));
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
    void triangle(Vec3 a, Vec3 b, Vec3 c, bool twoSided = false) {
        const Vec3 n = normalize(cross(b - a, c - a));
        for (int side = 0; side < (twoSided ? 2 : 1); ++side) {
            const Vec3 nn = side ? n * -1.0f : n;
            const u16 i0 = vert(a, nn), i1 = vert(b, nn), i2 = vert(c, nn);
            if (side) tri(i0, i2, i1);
            else tri(i0, i1, i2);
        }
    }

    // A fan from a centre over a ring counter-clockwise seen from `normal`; the centre may take
    // another slot (a flower's heart), shading the petals toward it.
    void fan(Vec3 centre, const Vec3* ring, int n, Vec3 normal, bool twoSided = false, u8 centreSlot = 0xFF,
             u8 centreEmit = 0xFF) {
        for (int side = 0; side < (twoSided ? 2 : 1); ++side) {
            const Vec3 nn = side ? normal * -1.0f : normal;
            const u8 keepEmit = emit;
            if (centreEmit != 0xFF) emit = centreEmit;
            const u16 c = vert(centre, nn, centreSlot != 0xFF ? centreSlot : slot);
            emit = keepEmit;
            const u16 first = static_cast<u16>(m.pos.size());
            for (int k = 0; k < n; ++k) vert(ring[k], nn);
            for (int k = 0; k < n; ++k) {
                const u16 a = static_cast<u16>(first + k), b = static_cast<u16>(first + (k + 1) % n);
                if (side) tri(c, b, a);
                else tri(c, a, b);
            }
        }
    }

    // A band round frame f's z: radius r0 at height h0 to r1 at h1 (x, y scaled by sx, sy),
    // `segs` round from angle a0 to a1 (a full turn by default), facing out (or in).
    void band(const Frame& f, float r0, float r1, float h0, float h1, int segs, float a0 = 0, float a1 = 2 * kPi,
              bool inward = false) {
        const bool full = std::fabs(a1 - a0 - 2 * kPi) < 1e-3f;
        const int cols = full ? segs : segs + 1;
        const u16 first = static_cast<u16>(m.pos.size());
        const float slope = r0 - r1, rise = h1 - h0;
        // Going up from h0 to h1 the quads face out; going down they'd face in: turned round.
        const bool flip = (rise < 0) != inward;
        for (int row = 0; row < 2; ++row)
            for (int k = 0; k < cols; ++k) {
                const float a = a0 + (a1 - a0) * k / segs, c = std::cos(a), s = std::sin(a);
                const float r = row ? r1 : r0, h = row ? h1 : h0;
                const Vec3 radial = f.dir(c, s, 0);
                const Vec3 n = radial * rise + f.z * slope;
                vert(f.o + radial * r + f.z * h, flip ? n * -1.0f : n);
            }
        for (int k = 0; k < segs; ++k) {
            const u16 a0i = static_cast<u16>(first + k), a1i = static_cast<u16>(first + (k + 1) % cols);
            const u16 b0 = static_cast<u16>(a0i + cols), b1 = static_cast<u16>(a1i + cols);
            if (flip) {
                tri(a0i, b1, a1i);
                tri(a0i, b0, b1);
            } else {
                tri(a0i, a1i, b1);
                tri(a0i, b1, b0);
            }
        }
    }
    // A flat ring (annulus) square to f's z, facing +z (and -z too if twoSided).
    void annulus(const Frame& f, float rIn, float rOut, float hIn, float hOut, int segs, bool twoSided) {
        for (int side = 0; side < (twoSided ? 2 : 1); ++side) {
            const u16 first = static_cast<u16>(m.pos.size());
            for (int k = 0; k < segs; ++k) {
                const float a = 2 * kPi * k / segs, c = std::cos(a), s = std::sin(a);
                const Vec3 radial = f.dir(c, s, 0);
                // Normals tipped with the slope from the inside edge to the outside one.
                const Vec3 n = f.z * (rOut - rIn) - radial * (hOut - hIn);
                const Vec3 nn = side ? n * -1.0f : n;
                vert(f.o + radial * rIn + f.z * hIn, nn);
                vert(f.o + radial * rOut + f.z * hOut, nn);
            }
            for (int k = 0; k < segs; ++k) {
                const u16 i0 = static_cast<u16>(first + 2 * k), o0 = static_cast<u16>(i0 + 1);
                const u16 i1 = static_cast<u16>(first + 2 * ((k + 1) % segs)), o1 = static_cast<u16>(i1 + 1);
                if (side) {
                    tri(i0, o1, o0);
                    tri(i0, i1, o1);
                } else {
                    tri(i0, o0, o1);
                    tri(i0, o1, i1);
                }
            }
        }
    }
    // A disc capping a band's end (facing +z or -z).
    void cap(const Frame& f, float r, float h, int segs, bool up) {
        Vec3 ring[16];
        for (int k = 0; k < segs && k < 16; ++k) {
            const float a = 2 * kPi * (up ? k : segs - k) / segs;
            ring[k] = f.at(std::cos(a) * r, std::sin(a) * r, h);
        }
        fan(f.at(0, 0, h), ring, segs, up ? f.z : f.z * -1.0f);
    }
    // A torus round f's z.
    void torus(const Frame& f, float R, float r, int segs, int sides) {
        const u16 first = static_cast<u16>(m.pos.size());
        for (int i = 0; i < segs; ++i) {
            const float a = 2 * kPi * i / segs;
            const Vec3 radial = f.dir(std::cos(a), std::sin(a), 0);
            for (int j = 0; j < sides; ++j) {
                const float b = 2 * kPi * j / sides;
                const Vec3 n = radial * std::cos(b) + f.z * std::sin(b);
                vert(f.o + radial * R + n * r, n);
            }
        }
        for (int i = 0; i < segs; ++i)
            for (int j = 0; j < sides; ++j) {
                const u16 a0 = static_cast<u16>(first + i * sides + j), a1 = static_cast<u16>(first + ((i + 1) % segs) * sides + j);
                const u16 b0 = static_cast<u16>(first + i * sides + (j + 1) % sides),
                          b1 = static_cast<u16>(first + ((i + 1) % segs) * sides + (j + 1) % sides);
                tri(a0, a1, b1);
                tri(a0, b1, b0);
            }
    }
    // An ellipsoid with half-axes ax, ay, az (square to each other, ax x ay along az).
    void ellipsoid(Vec3 c, Vec3 ax, Vec3 ay, Vec3 az, int segs, int rings) {
        const u16 first = static_cast<u16>(m.pos.size());
        const float lx = dot(ax, ax), ly = dot(ay, ay), lz = dot(az, az);
        for (int i = 0; i <= rings; ++i) {
            const float th = kPi * i / rings;
            for (int k = 0; k <= segs; ++k) {
                const float ph = 2 * kPi * k / segs;
                const float sx = std::sin(th) * std::cos(ph), sy = std::sin(th) * std::sin(ph), sz = std::cos(th);
                vert(c + ax * sx + ay * sy + az * sz, ax * (sx / lx) + ay * (sy / ly) + az * (sz / lz));
            }
        }
        for (int i = 0; i < rings; ++i)
            for (int k = 0; k < segs; ++k) {
                const u16 a = static_cast<u16>(first + i * (segs + 1) + k), b = static_cast<u16>(a + segs + 1);
                if (i > 0) tri(a, b, static_cast<u16>(a + 1));
                if (i < rings - 1) tri(static_cast<u16>(a + 1), b, static_cast<u16>(b + 1));
            }
    }
    void ball(Vec3 c, float r, int segs = 5, int rings = 3) { ellipsoid(c, {r, 0, 0}, {0, r, 0}, {0, 0, r}, segs, rings); }

    // A flower facing `normal`: petals round a heart (the gem's slot), `points` tips.
    void flower(Vec3 c, Vec3 normal, float r, int petals, u8 petalSlot, u8 heartSlot, bool glowHeart = false) {
        const Frame f = frameAt(c, normal, std::fabs(normal.z) < 0.9f ? Vec3{0, 0, 1} : Vec3{0, 1, 0});
        Vec3 ring[16];
        const int n = petals * 2 > 16 ? 16 : petals * 2;
        for (int k = 0; k < n; ++k) {
            const float a = 2 * kPi * k / n, rr = k % 2 ? r * 0.45f : r;
            ring[k] = f.at(std::cos(a) * rr, std::sin(a) * rr, k % 2 ? 0.0f : -r * 0.15f);
        }
        const u8 keep = slot;
        slot = petalSlot;
        fan(c + f.z * (r * 0.1f), ring, n, f.z, false, heartSlot, glowHeart ? 255 : 0xFF);
        slot = keep;
    }
    // A star (both faces), `points` tips, facing `normal` with a tip toward `up`.
    void star(Vec3 c, Vec3 normal, Vec3 up, float r, int points, bool twoSided) {
        const Frame f = frameAt(c, normal, up);
        Vec3 ring[16];
        const int n = points * 2 > 16 ? 16 : points * 2;
        for (int k = 0; k < n; ++k) {
            const float a = kPi / 2 + 2 * kPi * k / n, rr = k % 2 ? r * 0.45f : r;
            ring[k] = f.at(std::cos(a) * rr, std::sin(a) * rr, 0);
        }
        fan(c, ring, n, f.z, twoSided);
    }
    // A leaf or feather from base to tip, bulging `width` a little past halfway; both faces.
    void leaf(Vec3 base, Vec3 tip, Vec3 side, float width, Vec3 bow = {0, 0, 0}) {
        const Vec3 mid = lerp(base, tip, 0.45f) + bow;
        quad(base, mid + side * (width * 0.5f), tip, mid - side * (width * 0.5f), true);
    }
    // A bow: two loops either side of a knot, and two tails; `out` the way it faces, `up` its top.
    void bow(Vec3 c, Vec3 out, Vec3 up, float size, u8 loopSlot, u8 knotSlot) {
        const Frame f = frameAt(c, up, out * -1.0f);  // x across, y inward (toward what it's tied on), z up
        const u8 keep = slot;
        slot = loopSlot;
        for (int side = -1; side <= 1; side += 2)
            ellipsoid(f.at(side * 0.55f * size, 0, 0.1f * size), f.x * (0.5f * size), f.y * (0.16f * size),
                      f.z * (0.32f * size), 5, 3);
        for (int side = -1; side <= 1; side += 2)
            quad(f.at(side * 0.06f * size, -0.05f * size, -0.05f * size), f.at(side * 0.22f * size, -0.05f * size, -0.05f * size),
                 f.at(side * 0.42f * size, -0.02f * size, -0.75f * size), f.at(side * 0.2f * size, -0.02f * size, -0.8f * size),
                 true);
        slot = knotSlot;
        ellipsoid(f.at(0, -0.06f * size, 0.02f * size), f.x * (0.18f * size), f.y * (0.14f * size), f.z * (0.18f * size), 5, 3);
        slot = keep;
    }
    // A bell hanging from `top` along `down`.
    void bell(Vec3 top, Vec3 down, float size, u8 bellSlot, u8 clapperSlot) {
        const Frame f = frameAt(top, down * -1.0f, std::fabs(down.z) < 0.9f ? Vec3{0, 0, 1} : Vec3{0, 1, 0});
        const u8 keep = slot;
        slot = bellSlot;
        band(f, size * 0.3f, size * 0.36f, -size * 0.05f, -size * 0.55f, 7);
        band(f, size * 0.36f, size * 0.62f, -size * 0.55f, -size * 1.0f, 7);
        cap(f, size * 0.3f, -size * 0.05f, 7, true);
        slot = clapperSlot;
        ball(f.at(0, 0, -size * 1.0f), size * 0.16f, 5, 3);
        slot = keep;
    }
};

// ---- The back's curve: a point on it (theta across from the spine, y along), lifted off the skin.
Vec3 onBack(float theta, float y, float lift) {
    return {std::sin(theta) * (1.0f + lift), y, std::cos(theta) * (1.0f + lift) - 1.0f};
}
Vec3 backNormal(float theta) { return {std::sin(theta), 0, std::cos(theta)}; }

// A patch over the back, `cols` across and `rows` along, each quad its own colour from slotAt.
template <typename SlotAt>
void backPatch(Builder& b, float t0, float t1, float y0, float y1, int cols, int rows, float lift0, float lift1, SlotAt slotAt) {
    for (int i = 0; i < cols; ++i)
        for (int j = 0; j < rows; ++j) {
            const float ta = t0 + (t1 - t0) * i / cols, tb = t0 + (t1 - t0) * (i + 1) / cols;
            const float ya = y0 + (y1 - y0) * j / rows, yb = y0 + (y1 - y0) * (j + 1) / rows;
            const float la = lift0 + (lift1 - lift0) * j / rows, lb = lift0 + (lift1 - lift0) * (j + 1) / rows;
            const u8 s = slotAt(i, j);
            // across (+x) then back (+y): counter-clockwise seen from above
            const u16 a = b.vert(onBack(ta, ya, la), backNormal(ta), s), c = b.vert(onBack(tb, ya, la), backNormal(tb), s);
            const u16 d = b.vert(onBack(tb, yb, lb), backNormal(tb), s), e = b.vert(onBack(ta, yb, lb), backNormal(ta), s);
            b.tri(a, c, d);
            b.tri(a, d, e);
        }
}

// ------------------------------------------------------------------------------ head
void sunHat(Builder& b) {
    const Frame f;
    b.slot = 0;
    b.band(f, 0.52f, 0.46f, -0.12f, 0.5f, 8);
    b.cap(f, 0.46f, 0.5f, 8, true);
    b.slot = 1;
    b.band(f, 0.535f, 0.52f, -0.02f, 0.16f, 8);
    b.slot = 0;
    b.annulus(f, 0.5f, 1.08f, -0.05f, -0.14f, 10, true);
    b.flower({-0.34f, -0.42f, 0.08f}, normalize(Vec3{-0.6f, -0.8f, 0.2f}), 0.15f, 5, 3, 2);
}

void topHat(Builder& b) {
    const Frame f = frameAt({0.05f, 0, 0}, {0.18f, 0, 1}, {0, 1, 0});  // a jaunty tilt to the right
    b.slot = 0;
    b.annulus(f, 0.36f, 0.72f, -0.06f, -0.02f, 10, true);
    b.band(f, 0.38f, 0.42f, -0.06f, 0.92f, 8);
    b.cap(f, 0.42f, 0.92f, 8, true);
    b.slot = 1;
    b.band(f, 0.395f, 0.405f, 0.02f, 0.2f, 8);
}

void flowerCrown(Builder& b) {
    const Frame f = frameAt({0, -0.02f, -0.08f}, {0, -0.12f, 1}, {0, 1, 0});
    b.slot = 0;
    b.torus(f, 0.62f, 0.07f, 10, 3);
    for (int k = 0; k < 5; ++k) {
        const float a = -kPi / 2 + (k - 2) * 0.7f;  // round the front and sides
        const Vec3 at = f.at(std::cos(a) * 0.64f, std::sin(a) * 0.64f, 0.06f);
        b.flower(at, normalize(f.dir(std::cos(a), std::sin(a), 0.9f)), 0.17f, 5, k % 2 ? 1 : 2, 3);
    }
}

void tiara(Builder& b) {
    const Frame f;
    b.slot = 0;
    b.band(f, 0.62f, 0.6f, -0.12f, 0.02f, 6, kPi * 1.1f, kPi * 1.9f);
    for (int k = 0; k < 5; ++k) {
        const float a = kPi * 1.1f + kPi * 0.8f * (k + 0.5f) / 5.0f;
        const Vec3 radial{std::cos(a), std::sin(a), 0}, side{-std::sin(a), std::cos(a), 0};
        const float h = k == 2 ? 0.36f : (k % 2 ? 0.22f : 0.16f);
        const Vec3 base = radial * 0.61f + Vec3{0, 0, 0.0f};
        b.slot = 0;
        b.triangle(base - side * 0.09f, base + side * 0.09f, base + Vec3{0, 0, h} + radial * 0.02f, true);
        b.slot = 3;
        b.emit = 255;
        const Vec3 gem = radial * 0.635f + Vec3{0, 0, -0.04f};
        b.quad(gem + Vec3{0, 0, -0.05f}, gem + side * 0.045f, gem + Vec3{0, 0, 0.05f}, gem - side * 0.045f, false);
        b.emit = 0;
    }
}

void crown(Builder& b) {
    const Frame f;
    b.slot = 0;
    b.band(f, 0.5f, 0.56f, -0.12f, 0.16f, 8);
    b.band(f, 0.48f, 0.54f, -0.12f, 0.16f, 8, 0, 2 * kPi, true);  // its inside, seen from above
    for (int k = 0; k < 8; ++k) {
        const float a = 2 * kPi * k / 8, a1 = 2 * kPi * (k + 1) / 8, am = (a + a1) * 0.5f;
        const Vec3 p0{std::cos(a) * 0.56f, std::sin(a) * 0.56f, 0.16f}, p1{std::cos(a1) * 0.56f, std::sin(a1) * 0.56f, 0.16f};
        const Vec3 tip{std::cos(am) * 0.6f, std::sin(am) * 0.6f, 0.4f};
        b.triangle(p0, p1, tip, true);
    }
    b.slot = 1;
    b.cap(f, 0.5f, 0.08f, 8, true);  // the velvet
    b.slot = 3;
    b.emit = 255;
    for (int k = 0; k < 4; ++k) {
        const float a = -kPi / 2 + k * kPi / 2;
        const Vec3 radial{std::cos(a), std::sin(a), 0};
        b.star(radial * 0.555f + Vec3{0, 0, 0.02f}, radial, {0, 0, 1}, 0.08f, 4, false);
    }
    b.emit = 0;
}

void circlet(Builder& b) {
    const Frame f = frameAt({0, 0.06f, -0.2f}, {0, -0.25f, 1}, {0, 1, 0});  // lower at the brow
    b.slot = 0;
    b.torus(f, 0.66f, 0.045f, 10, 3);
    b.slot = 3;
    b.emit = 255;
    b.star(f.at(0, -0.69f, 0.02f), normalize(f.dir(0, -1, 0.2f)), f.z, 0.17f, 5, false);
    b.emit = 0;
}

void partyHat(Builder& b) {
    const Frame f = frameAt({0.04f, 0, 0}, {0.22f, 0, 1}, {0, 1, 0});
    constexpr int kSegs = 8;
    for (int k = 0; k < kSegs; ++k) {  // stripes: every other side in the second colour
        b.slot = static_cast<u8>(k % 2);
        b.band(f, 0.44f, 0.03f, -0.08f, 0.95f, 1, 2 * kPi * k / kSegs, 2 * kPi * (k + 1) / kSegs);
    }
    b.slot = 2;
    b.ball(f.at(0, 0, 0.98f), 0.11f);
}

void featherCrest(Builder& b) {
    const Vec3 base{0, 0.18f, -0.04f};
    for (int k = 0; k < 5; ++k) {
        const float a = (k - 2) * 0.32f;  // fanned across
        const Vec3 tip = base + Vec3{std::sin(a) * 0.5f, 0.35f, 0.9f - 0.12f * std::fabs(k - 2.0f)};
        b.slot = static_cast<u8>(k % 2);
        b.leaf(base, tip, normalize(Vec3{std::cos(a), 0, -std::sin(a) * 0.3f}), 0.2f, {0, 0.05f, 0});
    }
    b.slot = 2;
    b.ellipsoid(base + Vec3{0, -0.02f, 0.02f}, {0.12f, 0, 0}, {0, 0.1f, 0}, {0, 0, 0.09f}, 5, 3);
}

// ------------------------------------------------------------------------------ neck
void neckBand(Builder& b, float r, float h, int segs = 10) {
    const Frame f;
    b.band(f, r, r, -h, h, segs);
}

void neckBow(Builder& b) {
    b.slot = 0;
    neckBand(b, 1.05f, 0.07f);
    b.bow({0, -1.12f, 0}, {0, -1, 0}, {0, 0, 1}, 0.55f, 0, 0);
}

void scarf(Builder& b) {
    const Frame f;
    b.slot = 0;
    b.band(f, 1.1f, 1.11f, -0.22f, -0.07f, 10);
    b.slot = 1;
    b.band(f, 1.11f, 1.11f, -0.07f, 0.07f, 10);
    b.slot = 0;
    b.band(f, 1.11f, 1.1f, 0.07f, 0.22f, 10);
    for (int k = 0; k < 2; ++k) {  // the ends, hanging down the front
        const float x = k ? -0.25f : -0.55f, drop = k ? 0.95f : 0.75f;
        b.slot = static_cast<u8>(k);
        b.quad({x - 0.16f, -1.12f, -0.05f}, {x + 0.16f, -1.12f, -0.05f}, {x + 0.18f, -1.22f, -drop}, {x - 0.14f, -1.24f, -drop}, true);
    }
}

void collar(Builder& b) {
    const Frame f;
    b.slot = 1;
    b.band(f, 1.075f, 1.075f, -0.12f, -0.08f, 10);
    b.slot = 0;
    b.band(f, 1.06f, 1.06f, -0.08f, 0.08f, 10);
    b.slot = 1;
    b.band(f, 1.075f, 1.075f, 0.08f, 0.12f, 10);
    b.slot = 2;
    for (int k = 0; k < 5; ++k) {  // studs round the front and sides
        const float a = -kPi / 2 + (k - 2) * 0.62f;
        const Vec3 radial{std::cos(a), std::sin(a), 0}, side{-std::sin(a), std::cos(a), 0};
        const Vec3 c = radial * 1.065f, tip = radial * 1.16f;
        const Vec3 up{0, 0, 0.055f}, s = side * 0.055f;
        b.triangle(c - s - up, c + s - up, tip);
        b.triangle(c + s - up, c + s + up, tip);
        b.triangle(c + s + up, c - s + up, tip);
        b.triangle(c - s + up, c - s - up, tip);
    }
}

void bellCollar(Builder& b) {
    b.slot = 0;
    neckBand(b, 1.05f, 0.07f);
    b.bell({0, -1.1f, -0.06f}, normalize(Vec3{0, -0.3f, -1}), 0.34f, 2, 3);
}

void pendant(Builder& b) {
    b.slot = 2;
    neckBand(b, 1.04f, 0.025f);
    const Frame f = frameAt({0, -1.08f, -0.14f}, {0, -0.25f, 1}, {0, 1, 0});
    b.band(f, 0.035f, 0.035f, -0.12f, 0.1f, 4);  // the link
    b.band(f, 0.13f, 0.16f, -0.16f, -0.24f, 7);   // the setting
    b.slot = 3;
    b.emit = 255;
    b.ellipsoid(f.at(0, -0.04f, -0.36f), f.x * 0.15f, f.y * 0.08f, f.z * 0.2f, 6, 4);
    b.emit = 0;
}

void ruff(Builder& b) {
    constexpr int kSegs = 16;
    Vec3 inner[kSegs], outer[kSegs];
    for (int k = 0; k < kSegs; ++k) {
        const float a = 2 * kPi * k / kSegs;
        const Vec3 radial{std::cos(a), std::sin(a), 0};
        inner[k] = radial * 1.0f + Vec3{0, 0, 0.02f};
        outer[k] = radial * 1.5f + Vec3{0, 0, k % 2 ? 0.02f : -0.22f};  // pleated
    }
    for (int k = 0; k < kSegs; ++k) {
        const int n = (k + 1) % kSegs;
        for (int side = 0; side < 2; ++side) {  // both faces: seen from above and below
            const Vec3 nrm = normalize(cross(outer[k] - inner[k], inner[n] - inner[k])) * (side ? -1.0f : 1.0f);
            const u16 i0 = b.vert(inner[k], nrm, 0), i1 = b.vert(inner[n], nrm, 0);
            const u16 o0 = b.vert(outer[k], nrm, 1), o1 = b.vert(outer[n], nrm, 1);
            if (side) {
                b.tri(i0, i1, o1);
                b.tri(i0, o1, o0);
            } else {
                b.tri(i0, o0, o1);
                b.tri(i0, o1, i1);
            }
        }
    }
}

void lei(Builder& b) {
    b.slot = 2;
    neckBand(b, 1.03f, 0.03f);
    for (int k = 0; k < 8; ++k) {
        const float a = 2 * kPi * (k + 0.5f) / 8;
        const Vec3 radial{std::cos(a), std::sin(a), 0};
        b.flower(radial * 1.1f, normalize(radial + Vec3{0, 0, 0.35f}), 0.26f, 4, static_cast<u8>(k % 2), 3);
    }
}

// ------------------------------------------------------------------------------ back
void saddle(Builder& b) {
    backPatch(b, -1.15f, 1.15f, -0.62f, 0.62f, 6, 2, 0.03f, 0.03f, [](int, int) -> u8 { return 1; });  // the blanket
    backPatch(b, -0.85f, 0.85f, -0.45f, 0.45f, 4, 2, 0.09f, 0.09f, [](int, int) -> u8 { return 0; });  // the seat
    b.slot = 0;
    for (int i = 0; i < 3; ++i) {  // the cantle, a lip rising at the back
        const float ta = -0.6f + 0.4f * i, tb = ta + 0.4f;
        b.quad(onBack(ta, 0.45f, 0.09f), onBack(tb, 0.45f, 0.09f), onBack(tb, 0.52f, 0.24f), onBack(ta, 0.52f, 0.24f), true);
    }
    b.ellipsoid(onBack(0, -0.45f, 0.14f), {0.13f, 0, 0}, {0, 0.1f, 0}, {0, 0, 0.1f}, 5, 3);  // the pommel
    b.slot = 2;
    for (int side = -1; side <= 1; side += 2) {  // the stirrup straps
        const float t0 = side * 0.85f, t1 = side * 1.4f;
        b.quad(onBack(t0, -0.05f, 0.1f), onBack(t0, 0.05f, 0.1f), onBack(t1, 0.05f, 0.1f), onBack(t1, -0.05f, 0.1f), true);
    }
}

void cape(Builder& b) {
    constexpr int kCols = 6, kRows = 4;
    for (int i = 0; i < kCols; ++i)
        for (int j = 0; j < kRows; ++j) {
            // Narrow at the shoulders, wide and loose at the hem; the hem in the second colour.
            auto at = [](int ii, int jj) {
                const float v = static_cast<float>(jj) / kRows, width = 0.8f + 0.65f * v;
                const float t = -width + 2.0f * width * ii / kCols;
                return onBack(t, -1.0f + 1.9f * v, 0.05f + 0.1f * v);
            };
            auto nrm = [](int ii, int jj) {
                const float v = static_cast<float>(jj) / kRows, width = 0.8f + 0.65f * v;
                return backNormal(-width + 2.0f * width * ii / kCols);
            };
            auto slotOf = [](int jj) -> u8 { return jj >= kRows ? 1 : 0; };
            const u16 a = b.vert(at(i, j), nrm(i, j), slotOf(j)), c = b.vert(at(i + 1, j), nrm(i + 1, j), slotOf(j));
            const u16 d = b.vert(at(i + 1, j + 1), nrm(i + 1, j + 1), slotOf(j + 1)), e = b.vert(at(i, j + 1), nrm(i, j + 1), slotOf(j + 1));
            b.tri(a, c, d);
            b.tri(a, d, e);
        }
    b.slot = 2;
    b.ellipsoid(onBack(0, -1.0f, 0.1f), {0.14f, 0, 0}, {0, 0.08f, 0}, {0, 0, 0.1f}, 5, 3);  // the clasp
    b.slot = 3;
    b.emit = 255;
    const float stars[3][2] = {{-0.5f, 0.1f}, {0.35f, -0.25f}, {0.15f, 0.55f}};
    for (const auto& s : stars) {
        const float lift = 0.06f + 0.1f * (s[1] + 1.0f) / 1.9f + 0.01f;
        b.star(onBack(s[0], s[1], lift), backNormal(s[0]), {0, -1, 0}, 0.09f, 5, false);
    }
    b.emit = 0;
}

void blanket(Builder& b) {
    backPatch(b, -1.3f, 1.3f, -0.7f, 0.7f, 6, 4, 0.04f, 0.04f, [](int i, int j) -> u8 {
        if (i == 0 || i == 5 || j == 0 || j == 3) return 2;  // the border
        return static_cast<u8>((i + j) % 2);                  // the patches
    });
}

void sash(Builder& b) {
    constexpr int kSegs = 8;
    for (int k = 0; k < kSegs; ++k) {  // across the back from the front of one side to the back of the other
        const float ta = -1.5f + 3.0f * k / kSegs, tb = -1.5f + 3.0f * (k + 1) / kSegs;
        const float ya = ta * 0.3f, yb = tb * 0.3f;
        const u16 a = b.vert(onBack(ta, ya - 0.11f, 0.05f), backNormal(ta), 0), c = b.vert(onBack(tb, yb - 0.11f, 0.05f), backNormal(tb), 0);
        const u16 d = b.vert(onBack(tb, yb + 0.11f, 0.05f), backNormal(tb), 0), e = b.vert(onBack(ta, ya + 0.11f, 0.05f), backNormal(ta), 0);
        b.tri(a, c, d);
        b.tri(a, d, e);
    }
    b.flower(onBack(0, 0, 0.09f), {0, 0, 1}, 0.24f, 6, 1, 3);
    b.slot = 1;
    b.quad(onBack(-0.05f, 0.05f, 0.08f), onBack(0.05f, 0.05f, 0.08f), onBack(0.16f, 0.42f, 0.07f), onBack(0.06f, 0.44f, 0.07f), true);
}

void garland(Builder& b) {
    for (int s = 0; s < 2; ++s) {
        const float y = s ? 0.35f : -0.35f;
        b.slot = 2;
        backPatch(b, -1.2f, 1.2f, y - 0.03f, y + 0.03f, 8, 1, 0.05f, 0.05f, [](int, int) -> u8 { return 0; });
        for (int k = 0; k < 4; ++k) {
            const float t = -0.9f + 0.6f * k;
            b.flower(onBack(t, y, 0.08f), backNormal(t), 0.2f, 4, static_cast<u8>((k + s) % 2), 3);
        }
    }
}

// ------------------------------------------------------------------------------ tail
void tailBand(Builder& b, float r, float h, int segs = 8) { b.band(kTailRing, r, r, -h, h, segs); }

void tailBow(Builder& b) {
    b.slot = 0;
    tailBand(b, 1.1f, 0.18f);
    b.bow({0, 0, 1.2f}, {0, 0, 1}, {0, -1, 0.2f}, 1.05f, 0, 1);
}

void tailRibbons(Builder& b) {
    b.slot = 2;
    tailBand(b, 1.08f, 0.15f);
    for (int k = 0; k < 3; ++k) {
        const float a = (k - 1) * 0.9f;  // from the top and the sides
        const Vec3 radial{std::sin(a), 0, std::cos(a)}, side{std::cos(a), 0, -std::sin(a)};
        b.slot = static_cast<u8>(k % 2);
        Vec3 prev = radial * 1.1f;
        for (int s = 1; s <= 3; ++s) {  // trailing toward the tip, drooping, a little wave
            const float t = s / 3.0f;
            const Vec3 next = radial * (1.1f + 0.3f * t) + Vec3{0.25f * std::sin(t * 5.0f + k), 3.2f * t, -1.4f * t * t};
            b.quad(prev - side * 0.18f, prev + side * 0.18f, next + side * 0.16f, next - side * 0.16f, true);
            prev = next;
        }
    }
}

void tailRing(Builder& b) {
    b.slot = 0;
    b.torus(kTailRing, 1.12f, 0.22f, 10, 4);
}

void hanging(Builder& b) {  // a band and a short chain down to what hangs from it
    b.slot = 0;
    tailBand(b, 1.06f, 0.12f);
    b.slot = 2;
    b.quad({-0.06f, 0, -1.02f}, {0.06f, 0, -1.02f}, {0.06f, 0.05f, -1.6f}, {-0.06f, 0.05f, -1.6f}, true);
}

void starCharm(Builder& b) {
    hanging(b);
    b.slot = 3;
    b.emit = 255;
    b.star({0, 0.06f, -2.0f}, {1, 0, 0}, {0, 0, 1}, 0.5f, 5, true);
    b.emit = 0;
}

void tailBell(Builder& b) {
    b.slot = 0;
    tailBand(b, 1.06f, 0.14f);
    b.bell({0, 0, -1.05f}, {0, 0, -1}, 0.75f, 2, 3);
}

void snowCharm(Builder& b) {
    hanging(b);
    b.slot = 3;
    b.emit = 255;
    const Vec3 c{0, 0.06f, -2.05f};
    for (int k = 0; k < 6; ++k) {  // six arms (both faces), square to the tail's side
        const float a = kPi / 2 + k * kPi / 3;
        const Vec3 d{0, std::cos(a), std::sin(a)}, s{0, -std::sin(a), std::cos(a)};
        b.quad(c - s * 0.06f, c + d * 0.55f - s * 0.06f, c + d * 0.55f + s * 0.06f, c + s * 0.06f, true);
        b.triangle(c + d * 0.3f, c + d * 0.42f + s * 0.16f, c + d * 0.38f, true);
    }
    b.emit = 0;
}

void tailWreath(Builder& b) {
    b.slot = 0;
    tailBand(b, 1.05f, 0.12f);
    for (int k = 0; k < 6; ++k) {
        const float a = 2 * kPi * k / 6;
        const Vec3 radial{std::sin(a), 0, std::cos(a)};
        b.flower(radial * 1.14f, radial, 0.4f, 4, k % 2 ? 1 : 2, 3);
    }
}

void tassel(Builder& b) {
    b.slot = 1;
    tailBand(b, 1.06f, 0.14f);
    b.slot = 2;
    b.quad({-0.06f, 0, -1.02f}, {0.06f, 0, -1.02f}, {0.06f, 0, -1.45f}, {-0.06f, 0, -1.45f}, true);
    b.ball({0, 0, -1.55f}, 0.16f);
    const Frame f = frameAt({0, 0, -1.65f}, {0, 0, 1}, {0, 1, 0});
    b.slot = 0;
    b.band(f, 0.12f, 0.36f, 0.0f, -0.95f, 6);
    b.cap(f, 0.36f, -0.95f, 6, false);
}

}  // namespace

PropMesh wearMesh(WearShape shape) {
    PropMesh m;
    Builder b{m};
    switch (shape) {
        case WearShape::SunHat: sunHat(b); break;
        case WearShape::TopHat: topHat(b); break;
        case WearShape::FlowerCrown: flowerCrown(b); break;
        case WearShape::Tiara: tiara(b); break;
        case WearShape::Crown: crown(b); break;
        case WearShape::Circlet: circlet(b); break;
        case WearShape::PartyHat: partyHat(b); break;
        case WearShape::FeatherCrest: featherCrest(b); break;
        case WearShape::NeckBow: neckBow(b); break;
        case WearShape::Scarf: scarf(b); break;
        case WearShape::Collar: collar(b); break;
        case WearShape::Bell: bellCollar(b); break;
        case WearShape::Pendant: pendant(b); break;
        case WearShape::Ruff: ruff(b); break;
        case WearShape::Lei: lei(b); break;
        case WearShape::Saddle: saddle(b); break;
        case WearShape::Cape: cape(b); break;
        case WearShape::Blanket: blanket(b); break;
        case WearShape::Sash: sash(b); break;
        case WearShape::Garland: garland(b); break;
        case WearShape::TailBow: tailBow(b); break;
        case WearShape::TailRibbons: tailRibbons(b); break;
        case WearShape::TailRing: tailRing(b); break;
        case WearShape::StarCharm: starCharm(b); break;
        case WearShape::TailBell: tailBell(b); break;
        case WearShape::SnowCharm: snowCharm(b); break;
        case WearShape::TailWreath: tailWreath(b); break;
        case WearShape::Tassel: tassel(b); break;
        case WearShape::Count: break;
    }
    return m;
}

}  // namespace ec
