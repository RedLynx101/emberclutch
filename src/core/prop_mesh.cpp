#include "core/prop_mesh.hpp"

#include <cmath>

namespace ec {
namespace {

constexpr float kPi = 3.14159265f;

// Appends vertices and triangles. Triangles are counter-clockwise seen from outside (the
// dragon program culls the back faces); `twoSided` pieces get a back face of their own.
struct Builder {
    PropMesh& m;
    u8 slot = 0;
    u8 emit = 0;

    u16 vert(Vec3 p, Vec3 n) {
        m.pos.push_back(p);
        m.nrm.push_back(normalize(n));
        m.paint.insert(m.paint.end(), {slot, slot, 0, emit});
        return static_cast<u16>(m.pos.size() - 1);
    }
    void tri(u16 a, u16 b, u16 c) { m.idx.insert(m.idx.end(), {a, b, c}); }

    // A flat polygon fan (ring counter-clockwise seen from n), optionally from a centre point.
    void fan(Vec3 centre, const Vec3* ring, int n, Vec3 normal, bool twoSided = false, u8 centreSlot = 0xFF) {
        for (int side = 0; side < (twoSided ? 2 : 1); ++side) {
            const Vec3 nn = side ? normal * -1.0f : normal;
            const u8 keep = slot;
            if (centreSlot != 0xFF) slot = centreSlot;
            const u16 c = vert(centre, nn);
            slot = keep;
            const u16 first = static_cast<u16>(m.pos.size());
            for (int k = 0; k < n; ++k) vert(ring[k], nn);
            for (int k = 0; k < n; ++k) {
                const u16 a = static_cast<u16>(first + k), b = static_cast<u16>(first + (k + 1) % n);
                if (side) tri(c, b, a);
                else tri(c, a, b);
            }
        }
    }

    // A quad a b c d, counter-clockwise seen from n.
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

    // A tube along the polyline pts (radii r), `sides` round, smooth sides, optional end caps.
    // twist turns the cross-section (a square post with flat faces front and back).
    void tube(const Vec3* pts, const float* r, const u8* slots, int n, int sides, bool capStart, bool capEnd,
              float twist = 0, bool inward = false) {
        const Vec3 w = normalize(pts[n - 1] - pts[0]);
        const Vec3 helper = std::fabs(w.z) < 0.9f ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        const Vec3 u = normalize(cross(helper, w)), v = cross(w, u);
        auto dir = [&](int k) {
            const float a = twist + 2 * kPi * k / sides;
            return u * std::cos(a) + v * std::sin(a);
        };
        const u16 first = static_cast<u16>(m.pos.size());
        const u8 keep = slot;
        for (int i = 0; i < n; ++i) {
            if (slots) slot = slots[i];
            for (int k = 0; k < sides; ++k) vert(pts[i] + dir(k) * r[i], inward ? dir(k) * -1.0f : dir(k));
        }
        slot = keep;
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
            if (slots) slot = slots[i];
            fan(pts[i], ring, sides, end ? w : w * -1.0f);
            slot = keep;
        };
        if (capStart) cap(0, false);
        if (capEnd) cap(n - 1, true);
    }
    void frustum(Vec3 a, Vec3 b, float ra, float rb, int sides, bool capA, bool capB, float twist = 0,
                 bool inward = false) {
        const Vec3 pts[2] = {a, b};
        const float r[2] = {ra, rb};
        tube(pts, r, nullptr, 2, sides, capA, capB, twist, inward);
    }

    // An ellipsoid, segs around and rings from top to bottom; slotAt picks each vertex's slot.
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
                vert(c + Vec3{s.x * radii.x, s.y * radii.y, s.z * radii.z},
                     {s.x / radii.x, s.y / radii.y, s.z / radii.z});
            }
        }
        slot = keep;
        for (int i = 0; i < rings; ++i)
            for (int k = 0; k < segs; ++k) {
                const u16 a = static_cast<u16>(first + i * (segs + 1) + k), b = static_cast<u16>(a + segs + 1);
                if (i > 0) tri(a, b, static_cast<u16>(a + 1));  // no slivers at the poles
                if (i < rings - 1) tri(static_cast<u16>(a + 1), b, static_cast<u16>(b + 1));
            }
    }
    void ellipsoid(Vec3 c, Vec3 radii, int segs, int rings) {
        const u8 s = slot;
        ellipsoid(c, radii, segs, rings, [s](Vec3) { return s; });
    }

    // A flat ring on the floor between two outlines (both counter-clockwise from +Z).
    void floorRing(const Vec3* inner, const Vec3* outer, int n) {
        const u16 first = static_cast<u16>(m.pos.size());
        for (int k = 0; k < n; ++k) {
            vert(inner[k], {0, 0, 1});
            vert(outer[k], {0, 0, 1});
        }
        for (int k = 0; k < n; ++k) {
            const u16 i0 = static_cast<u16>(first + 2 * k), o0 = static_cast<u16>(i0 + 1);
            const u16 i1 = static_cast<u16>(first + 2 * ((k + 1) % n)), o1 = static_cast<u16>(i1 + 1);
            tri(i0, o0, o1);
            tri(i0, o1, i1);
        }
    }

    // A leaf or feather: a diamond from base to tip, bulging `width` at `mid` (0..1 along it),
    // `up` giving the side it faces; both sides.
    void leaf(Vec3 base, Vec3 midRaise, Vec3 tip, Vec3 side, float width) {
        const Vec3 mid = lerp(base, tip, 0.45f) + midRaise;
        const Vec3 n = normalize(cross(side, tip - base));
        quad(base, mid + side * (width * 0.5f), tip, mid - side * (width * 0.5f), n * -1.0f, true);
    }
};

Vec3 onFloor(float angle, float r, float z = 0) { return {std::cos(angle) * r, std::sin(angle) * r, z}; }

// ------------------------------------------------------------------------------ toys
void featherWand(Builder& b) {
    b.slot = 0;  // the stick
    b.frustum({-0.45f, 0, 0.03f}, {0.45f, 0, 0.03f}, 0.025f, 0.02f, 4, false, false, kPi / 4);
    b.slot = 1;  // the feather, and a smaller one in the tip colour
    b.leaf({0.45f, 0, 0.04f}, {0, 0, 0.05f}, {0.86f, 0.08f, 0.05f}, {0, 0.5f, 0.85f}, 0.2f);
    b.slot = 2;
    b.leaf({0.47f, 0, 0.05f}, {0, 0, 0.04f}, {0.72f, -0.12f, 0.05f}, {0, 0.5f, 0.85f}, 0.12f);
}

void rope(Builder& b) {
    const Vec3 pts[6] = {{-0.5f, 0, 0}, {-0.43f, 0, 0}, {-0.34f, 0, 0}, {0.34f, 0, 0}, {0.43f, 0, 0}, {0.5f, 0, 0}};
    const float r[6] = {0.05f, 0.085f, 0.045f, 0.045f, 0.085f, 0.05f};
    const u8 slots[6] = {1, 1, 0, 0, 1, 1};  // red knots at both ends
    b.tube(pts, r, slots, 6, 4, true, true, kPi / 4);
}

void orb(Builder& b) {
    b.ellipsoid({0, 0, 0}, {kOrbRadius, kOrbRadius, kOrbRadius}, 6, 4, [](Vec3 s) -> u8 {
        return std::fabs(s.z) < 0.35f || s.z > 0.9f ? 1 : 0;  // a gold band, and the treat lid on top
    });
}

void bowl(Builder& b) {
    constexpr int kSides = 6;
    b.slot = 0;
    b.frustum({0, 0, 0}, {0, 0, 0.13f}, 0.24f, 0.32f, kSides, false, false);
    Vec3 inner[kSides], outer[kSides];
    for (int k = 0; k < kSides; ++k) {
        const float a = 2 * kPi * k / kSides;
        inner[k] = onFloor(a, 0.25f, 0.11f);
        outer[k] = onFloor(a, 0.32f, 0.13f);
    }
    b.slot = 2;
    b.floorRing(inner, outer, kSides);  // the rim, sloping in
    b.slot = 1;
    b.fan({0, 0, 0.1f}, inner, kSides, {0, 0, 1});
}

// ------------------------------------------------------------------------------ decor
void rug(Builder& b, int variant) {
    constexpr int kSeg = 12;
    constexpr float kSy = 0.9f, kZ = 0.03f;
    Vec3 centre[kSeg], bandIn[kSeg], bandOut[kSeg], edgeIn[kSeg], edgeOut[kSeg];
    for (int k = 0; k < kSeg; ++k) {
        const float a = 2 * kPi * k / kSeg;
        float rc = 1.0f, rb = 1.9f;
        switch (variant) {
            case 0:
            case 4: rb += (k % 2 ? 0.16f : -0.16f); break;           // ember and the den's own: a zigzag
            case 1: rb += 0.15f * std::sin(a * 3); break;              // tide: waves
            case 2: rc += 0.14f * std::fabs(std::sin(a * 3)); break;  // grove: scallops
            default: rc = k % 2 ? 0.55f : 1.15f; break;                // lumen: a six-point star
        }
        auto at = [&](float r) { return Vec3{std::cos(a) * r, std::sin(a) * r * kSy, kZ}; };
        centre[k] = bandIn[k] = at(rc);
        bandOut[k] = edgeIn[k] = at(rb);
        edgeOut[k] = at(2.5f);
    }
    b.slot = 2;
    b.fan({0, 0, kZ}, centre, kSeg, {0, 0, 1});
    b.slot = 1;
    b.floorRing(bandIn, bandOut, kSeg);
    b.slot = 0;
    b.floorRing(edgeIn, edgeOut, kSeg);
}

// Hung from the wall: an arm out of the wall, a hook, and the lantern below it.
void lantern(Builder& b, int variant) {
    b.slot = 0;
    b.frustum({0, 0.1f, 0.1f}, {0, -0.6f, 0.02f}, 0.035f, 0.03f, 4, false, false);
    const Vec3 at{0, -0.6f, 0};
    if (variant == 2) {  // paper: a round glowing ball under a little cap
        b.frustum(at, at + Vec3{0, 0, -0.14f}, 0.04f, 0.12f, 5, false, false);
        b.slot = 1;
        b.emit = 255;
        b.ellipsoid(at + Vec3{0, 0, -0.42f}, {0.27f, 0.27f, 0.31f}, 5, 3);
        b.emit = 0;
        return;
    }
    b.frustum(at, at + Vec3{0, 0, -0.14f}, 0.04f, 0.21f, 5, false, false);  // the cap
    b.slot = 1;
    b.emit = 255;
    b.frustum(at + Vec3{0, 0, -0.14f}, at + Vec3{0, 0, -0.56f}, 0.16f, 0.16f, 5, false, false);  // the glass
    b.emit = 0;
    b.slot = 0;
    b.frustum(at + Vec3{0, 0, -0.56f}, at + Vec3{0, 0, -0.66f}, 0.2f, 0.12f, 5, false, true);  // the base
}

void perch(Builder& b, int variant) {
    const bool stone = variant == 1;
    const int sides = stone ? 4 : 5;
    const float twist = stone ? kPi / 4 : 0;
    b.slot = 1;
    b.frustum({0, 0, 0}, {0, 0, 0.16f}, stone ? 0.42f : 0.38f, stone ? 0.36f : 0.3f, 4, false, true, kPi / 4);
    b.slot = 0;
    b.frustum({0, 0, 0.16f}, {stone ? 0 : 0.06f, 0, 2.0f}, 0.12f, stone ? 0.11f : 0.08f, sides, false, false, twist);
    b.frustum({-0.7f, 0, 2.0f}, {0.7f, 0, stone ? 2.0f : 2.06f}, 0.07f, stone ? 0.07f : 0.055f, 4, true, true, kPi / 4);
    if (!stone) {  // a mossy twig
        b.slot = 2;
        b.frustum({0.04f, 0, 1.15f}, {0.36f, -0.08f, 1.5f}, 0.045f, 0.02f, 4, false, false);
    }
}

void plant(Builder& b, int variant) {
    constexpr int kSides = 6;
    b.slot = 0;
    b.frustum({0, 0, 0}, {0, 0, 0.58f}, 0.3f, 0.42f, kSides, false, false);
    Vec3 soil[kSides];
    for (int k = 0; k < kSides; ++k) soil[k] = onFloor(2 * kPi * k / kSides, 0.42f, 0.56f);
    b.slot = 1;
    b.fan({0, 0, 0.56f}, soil, kSides, {0, 0, 1});
    b.slot = 2;
    const Vec3 base{0, 0, 0.56f};
    const int leaves = variant == 0 ? 6 : 4;
    for (int k = 0; k < leaves; ++k) {  // fronds arch out and up (a fern's longer)
        const float a = 2 * kPi * (k + 0.3f) / leaves;
        const Vec3 out{std::cos(a), std::sin(a), 0}, side{-std::sin(a), std::cos(a), 0};
        const float len = variant == 0 ? 1.05f : 0.7f;
        b.leaf(base, {0, 0, 0.45f}, base + out * len + Vec3{0, 0, variant == 0 ? 0.35f : 0.55f}, side,
               variant == 0 ? 0.3f : 0.34f);
    }
    if (variant == 0) return;
    b.slot = 3;  // blooms: four-petalled stars, facing out and up
    b.emit = variant == 1 ? 255 : 90;
    for (int k = 0; k < 2; ++k) {
        const float a = kPi * k + 0.9f;
        const Vec3 c{std::cos(a) * 0.26f, std::sin(a) * 0.26f, 1.3f + 0.15f * k};
        const Vec3 n = normalize(Vec3{std::cos(a), std::sin(a), 1.2f});
        const Vec3 u = normalize(cross(n, Vec3{0, 0, 1})), v = cross(n, u);  // u x v = n: counter-clockwise
        Vec3 ring[8];
        for (int p = 0; p < 8; ++p) {
            const float t = 2 * kPi * p / 8, r = p % 2 ? 0.08f : 0.2f;
            ring[p] = c + u * (std::cos(t) * r) + v * (std::sin(t) * r);
        }
        b.fan(c, ring, 8, n);
    }
    b.emit = 0;
}

// On the wall: a rod, the cloth hanging from it (a notch at the bottom), and its crest.
void banner(Builder& b, int variant) {
    b.slot = 2;
    b.frustum({-0.72f, -0.12f, 0.02f}, {0.72f, -0.12f, 0.02f}, 0.045f, 0.045f, 4, false, false, kPi / 4);
    constexpr float kY = -0.1f;
    const Vec3 notch{0, kY, -1.55f};
    const Vec3 outline[4] = {{0.55f, kY, -1.9f}, {0.55f, kY, 0}, {-0.55f, kY, 0}, {-0.55f, kY, -1.9f}};
    b.slot = 0;
    for (int side = 0; side < 2; ++side) {  // a fan from the notch, both faces
        const Vec3 n{0, side ? 1.0f : -1.0f, 0};
        const u16 c = b.vert(notch, n);
        u16 o[4];
        for (int k = 0; k < 4; ++k) o[k] = b.vert(outline[k], n);
        for (int k = 0; k < 3; ++k) {
            if (side) b.tri(c, o[k + 1], o[k]);
            else b.tri(c, o[k], o[k + 1]);
        }
    }
    constexpr float kFront = -0.115f;
    const Vec3 mid{0, kFront, -0.85f};
    b.slot = 1;
    if (variant == 0) {  // a flame: a teardrop, yellow at its heart
        const Vec3 ring[6] = {{0.3f, kFront, -0.95f}, {0.18f, kFront, -1.18f}, {-0.18f, kFront, -1.18f},
                              {-0.3f, kFront, -0.95f}, {-0.08f, kFront, -0.6f}, {0.02f, kFront, -0.36f}};
        Vec3 ccw[6];
        for (int k = 0; k < 6; ++k) ccw[k] = ring[5 - k];  // counter-clockwise seen from -Y
        b.fan(mid + Vec3{0, 0, -0.1f}, ccw, 6, {0, -1, 0}, false, 3);
    } else if (variant == 1) {  // waves: two crests
        for (int w = 0; w < 2; ++w) {
            const float z0 = -0.72f - w * 0.3f;
            for (int k = 0; k < 3; ++k) {
                const float x0 = -0.36f + k * 0.24f, x1 = x0 + 0.24f;
                const float h0 = 0.1f * std::sin((x0 + 0.36f) * 6.5f), h1 = 0.1f * std::sin((x1 + 0.36f) * 6.5f);
                b.quad({x1, kFront, z0 + h1 - 0.12f}, {x1, kFront, z0 + h1}, {x0, kFront, z0 + h0},
                       {x0, kFront, z0 + h0 - 0.12f}, {0, -1, 0});
            }
        }
    } else if (variant == 2) {  // a star
        Vec3 ring[10];
        for (int k = 0; k < 10; ++k) {
            const float t = kPi / 2 - 2 * kPi * k / 10, r = k % 2 ? 0.14f : 0.34f;
            ring[k] = mid + Vec3{-std::cos(t) * r, 0, std::sin(t) * r};
        }
        b.fan(mid, ring, 10, {0, -1, 0}, false, 3);
    } else {  // a breed's banner (the Dragondex): an egg, its heartglow at the middle
        Vec3 ring[12];
        for (int k = 0; k < 12; ++k) {
            const float t = kPi / 2 - 2 * kPi * k / 12, up = std::sin(t);
            ring[k] = mid + Vec3{-std::cos(t) * 0.26f * (1.0f - 0.18f * up), 0, up * 0.34f};
        }
        b.fan(mid + Vec3{0, 0, -0.06f}, ring, 12, {0, -1, 0}, false, 3);
    }
}

}  // namespace

PropMesh toyMesh(int toy) {
    PropMesh m;
    Builder b{m};
    switch (toy) {
        case 0: featherWand(b); break;
        case 1: rope(b); break;
        case 2: orb(b); break;
        default: bowl(b); break;
    }
    return m;
}

PropMesh bowlFoodMesh() {
    PropMesh m;
    Builder b{m};
    b.slot = 3;
    Vec3 ring[6];
    for (int k = 0; k < 6; ++k) ring[k] = onFloor(2 * kPi * k / 6, 0.25f, 0.1f);
    b.fan({0, 0, 0.2f}, ring, 6, {0, 0, 1});
    return m;
}

PropMesh decorMesh(Item i) {
    PropMesh m;
    Builder b{m};
    const ItemInfo& info = itemInfo(i);
    switch (info.kind) {
        case ItemKind::Rug: rug(b, info.variant); break;
        case ItemKind::Lantern: lantern(b, info.variant); break;
        case ItemKind::Perch: perch(b, info.variant); break;
        case ItemKind::Plant: plant(b, info.variant); break;
        case ItemKind::Banner: banner(b, info.variant); break;
        default: break;
    }
    return m;
}

PropMesh breedBannerMesh() {
    PropMesh m;
    Builder b{m};
    banner(b, 3);
    return m;
}

PropLook breedBannerLook(Rgb base, Rgb accent, Rgb glow) {
    auto luma = [](Rgb c) { return 0.3f * c.r + 0.59f * c.g + 0.11f * c.b; };
    PropLook l;
    l.colour[0] = base;
    // The egg stands out from the cloth: its accent, unless that's too close to it.
    const float lb = luma(base);
    l.colour[1] = std::fabs(luma(accent) - lb) > 60 ? accent : lb > 128 ? Rgb{70, 45, 55} : Rgb{247, 234, 200};
    l.colour[2] = {196, 150, 70};  // the brass rod
    l.colour[3] = glow;
    l.glow = 0.6f;
    return l;
}

PropMesh homeRugMesh() {
    PropMesh m;
    Builder b{m};
    rug(b, 4);
    return m;
}

PropLook propLook(Item i) {
    auto look = [](Rgb a, Rgb b, Rgb c, Rgb d, float glow = 0) {
        PropLook l;
        l.colour[0] = a;
        l.colour[1] = b;
        l.colour[2] = c;
        l.colour[3] = d;
        l.glow = glow;
        return l;
    };
    constexpr Rgb kBrass{184, 140, 60}, kClay{190, 100, 60}, kSoil{80, 55, 40};
    switch (i) {
        case Item::FeatherWand: return look({150, 98, 56}, {232, 102, 43}, {63, 167, 168}, {0, 0, 0});
        case Item::TugRope: return look({238, 222, 186}, {214, 86, 70}, {0, 0, 0}, {0, 0, 0});
        case Item::PuzzleOrb: return look({63, 167, 168}, {245, 196, 81}, {60, 40, 60}, {0, 0, 0});
        case Item::FoodBowl: return look({178, 74, 56}, {247, 234, 200}, {150, 98, 56}, {170, 110, 60});
        case Item::RugEmber: return look({130, 40, 34}, {214, 86, 70}, {245, 196, 81}, {0, 0, 0});
        case Item::RugTide: return look({40, 70, 120}, {80, 150, 200}, {220, 240, 250}, {0, 0, 0});
        case Item::RugGrove: return look({60, 90, 50}, {110, 160, 90}, {220, 230, 170}, {0, 0, 0});
        case Item::RugLumen: return look({200, 170, 110}, {247, 234, 200}, {245, 196, 81}, {0, 0, 0});
        case Item::LanternBrass: return look(kBrass, {255, 206, 120}, {0, 0, 0}, {0, 0, 0}, 1);
        case Item::LanternGlass: return look({170, 180, 190}, {150, 210, 255}, {0, 0, 0}, {0, 0, 0}, 1);
        case Item::LanternPaper: return look({70, 45, 35}, {245, 120, 90}, {0, 0, 0}, {0, 0, 0}, 1);
        case Item::PerchDriftwood: return look({176, 156, 134}, {120, 100, 80}, {90, 140, 80}, {0, 0, 0});
        case Item::PerchStone: return look({150, 140, 152}, {110, 100, 116}, {0, 0, 0}, {0, 0, 0});
        case Item::PlantFern: return look(kClay, kSoil, {80, 150, 70}, {0, 0, 0});
        case Item::PlantMoonflower: return look({170, 160, 184}, kSoil, {70, 124, 96}, {235, 240, 255}, 1);
        case Item::PlantEmberbloom: return look(kClay, kSoil, {96, 124, 60}, {240, 84, 40}, 1);
        case Item::BannerFlame: return look({170, 40, 40}, {245, 140, 50}, kBrass, {255, 222, 120});
        case Item::BannerWave: return look({40, 70, 130}, {120, 200, 230}, kBrass, {230, 250, 255});
        case Item::BannerStar: return look({240, 230, 200}, {245, 196, 81}, kBrass, {255, 250, 220});
        default: return look({158, 56, 41}, {204, 77, 41}, {237, 199, 117}, {0, 0, 0});  // the den's own rug
    }
}

DecorPlace decorPlace(int spot) {
    auto onWall = [](float degrees, float r, float z, float scale) {
        const float a = degrees * kPi / 180;  // from +Y toward +X, as tools/blender/den_model.py radial()
        return DecorPlace{{std::sin(a) * r, std::cos(a) * r, z}, -a, scale};
    };
    switch (spot) {
        case 0: return {{0.0f, 0.6f, 0}, 0, 1.0f};        // the rug, at home (DenLayout::home)
        case 1: return onWall(-24, 9.2f, 4.4f, 1.7f);     // the lantern, left of the shelves
        case 2: return onWall(64, 7.6f, 0, 1.15f);        // the perch, between the big nest and the hearth
        case 3: return onWall(-16, 7.8f, 0, 1.25f);       // the plant, under the shelves
        default: return onWall(32, 9.05f, 5.6f, 1.35f);  // the banner, right of the skylight
    }
}

}  // namespace ec
