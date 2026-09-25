#include "core/mud.hpp"

#include <cmath>

namespace ec {
namespace {

constexpr float kSpotCell = 0.07f;  // the noise lattice, armature units (a grown dragon is ~1.3 long)

float lattice(int x, int y, int z) {
    u32 h = static_cast<u32>(x) * 73856093u ^ static_cast<u32>(y) * 19349663u ^ static_cast<u32>(z) * 83492791u;
    h ^= h >> 13;
    h *= 0x5BD1E995u;
    h ^= h >> 15;
    return static_cast<float>(h & 0xFFFF) / 65535.0f;
}

float smooth(float t) { return t * t * (3 - 2 * t); }

}  // namespace

float mudSpots(Vec3 rest) {
    const float x = rest.x / kSpotCell, y = rest.y / kSpotCell, z = rest.z / kSpotCell;
    const int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y)),
              iz = static_cast<int>(std::floor(z));
    const float fx = smooth(x - ix), fy = smooth(y - iy), fz = smooth(z - iz);
    auto lerp1 = [](float a, float b, float t) { return a + (b - a) * t; };
    const float n = lerp1(lerp1(lerp1(lattice(ix, iy, iz), lattice(ix + 1, iy, iz), fx),
                                lerp1(lattice(ix, iy + 1, iz), lattice(ix + 1, iy + 1, iz), fx), fy),
                          lerp1(lerp1(lattice(ix, iy, iz + 1), lattice(ix + 1, iy, iz + 1), fx),
                                lerp1(lattice(ix, iy + 1, iz + 1), lattice(ix + 1, iy + 1, iz + 1), fx), fy),
                          fz);
    const float t = (n - 0.4f) / 0.22f;
    return smooth(t < 0 ? 0 : (t > 1 ? 1 : t));
}

void dirtTexel(float dust, float mud, Rgb& colour, float& amount) {
    const float d = dust < 0 ? 0 : (dust > 1 ? 1 : dust);
    const float m = (mud < 0 ? 0 : (mud > 1 ? 1 : mud)) * kMudMax;
    // prev -> dust by d, then -> mud by m: prev (1-d)(1-m) + dust d(1-m) + mud m
    amount = 1 - (1 - d) * (1 - m);
    if (amount <= 1e-4f) {
        colour = kDustColor;
        amount = 0;
        return;
    }
    const float wd = d * (1 - m) / amount, wm = m / amount;
    auto mix = [&](u8 a, u8 b) { return static_cast<u8>(a * wd + b * wm + 0.5f); };
    colour = {mix(kDustColor.r, kMudColor.r), mix(kDustColor.g, kMudColor.g), mix(kDustColor.b, kMudColor.b)};
}

}  // namespace ec
