// Skyreach Valley (Beta WP1): the landscape's data and tiles, and flying over it.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/flight.hpp"
#include "core/valley.hpp"

using namespace ec;

namespace {

const Valley& valley() {
    static Valley v;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        FILE* f = std::fopen("../romfs/valley/skyreach.evl", "rb");
        if (!f) return v;
        std::vector<u8> data;
        u8 buf[4096];
        std::size_t n;
        while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) data.insert(data.end(), buf, buf + n);
        std::fclose(f);
        loadValley(data.data(), data.size(), v);
    }
    return v;
}

Vec3 faceNormal(const ValleyMesh& m, std::size_t i) {
    const Vec3 a = m.pos[m.idx[i]], b = m.pos[m.idx[i + 1]], c = m.pos[m.idx[i + 2]];
    return cross(b - a, c - a);
}

}  // namespace

TEST(the_valley_loads) {
    const Valley& v = valley();
    CHECK(v.n == 257 && v.tiles() == 16);
    CHECK(std::fabs(v.size() - 1024.0f) < 0.01f && std::fabs(v.tileSize() - 64.0f) < 0.01f);
    CHECK(v.hmax > 150 && v.hmin < v.water);
    CHECK(v.trees.size() > 500 && v.islands.size() == 4 && v.places.size() == kPlaceCount);
    for (u8 p = 0; p < kPlaceCount; ++p) CHECK(v.place(p) != nullptr);
    // Heights between samples blend smoothly; at a sample they're the sample.
    const float x = v.x0 + 40 * v.spacing, y = v.y0 + 70 * v.spacing;
    CHECK(std::fabs(v.heightAt(x, y) - v.h[70 * v.n + 40]) < 1e-3f);
    const float mid = v.heightAt(x + v.spacing * 0.5f, y);
    const float lo = std::fmin(v.h[70 * v.n + 40], v.h[70 * v.n + 41]), hi = std::fmax(v.h[70 * v.n + 40], v.h[70 * v.n + 41]);
    CHECK(mid >= lo - 1e-3f && mid <= hi + 1e-3f);
    // The den's cliff: the plateau stands well above the valley floor in front of the mouth.
    const ValleyPlaceInfo& den = *v.place(kPlaceDen);
    CHECK(v.heightAt(den.at.x - 30, den.at.y) > v.heightAt(den.at.x + 30, den.at.y) + 30);
    // The islands float well clear of the ground.
    for (const ValleyIsland& isl : v.islands) CHECK(isl.at.z - isl.radius * 1.7f > v.heightAt(isl.at.x, isl.at.y) + 40);
}

TEST(valley_tiles_join_and_fit_the_budget) {
    const Valley& v = valley();
    ValleyMesh a, b;
    int worst[kValleyLods] = {};
    double mean[kValleyLods] = {};
    for (int lod = 0; lod < kValleyLods; ++lod)
        for (int ty = 0; ty < v.tiles(); ++ty)
            for (int tx = 0; tx < v.tiles(); ++tx) {
                buildValleyTile(v, tx, ty, lod, a);
                worst[lod] = std::max(worst[lod], a.triangles());
                mean[lod] += a.triangles() / double(v.tiles() * v.tiles());
                CHECK(a.pos.size() < 65536 && a.color.size() == a.pos.size() * 4);
            }
    std::printf("  valley tiles: worst %d / %d / %d, mean %.0f / %.0f / %.0f triangles\n", worst[0], worst[1], worst[2],
                mean[0], mean[1], mean[2]);
    CHECK(worst[0] <= 1000 && worst[1] <= 360 && worst[2] <= 80);
    // A view: three tiles near, six mid, twelve far (culled by the view, in fog beyond): a
    // typical one, and the worst (the densest forest tile everywhere, which can't happen).
    const double typical = 3 * mean[0] + 6 * mean[1] + 12 * mean[2];
    const int view = 3 * worst[0] + 6 * worst[1] + 12 * worst[2];
    std::printf("  a view: typically %.0f terrain triangles, at most %d\n", typical, view);
    // With the ridden dragon (3,000) that's still under the full den's ~9,800 triangles, which
    // runs at 17-18 ms on the old 3DS; the valley's target is 30 fps (Beta plan, WP1).
    CHECK(typical <= 4500 && view <= 6000);
    // Neighbours at the same level share their edge exactly; the ground faces up; the skirts
    // face out of the tile.
    buildValleyTile(v, 5, 7, 0, a);
    buildValleyTile(v, 6, 7, 0, b);
    const int side = kTileQuads + 1;
    for (int k = 0; k < side; ++k) {
        const Vec3 pa = a.pos[std::size_t(k) * side + kTileQuads], pb = b.pos[std::size_t(k) * side];
        CHECK(length(pa - pb) < 1e-4f);
    }
    const std::size_t groundTris = std::size_t(kTileQuads) * kTileQuads * 2 * 3;
    bool up = true;
    for (std::size_t i = 0; i < groundTris; i += 3) up &= faceNormal(a, i).z > 0;
    CHECK(up);
    const Vec3 south = faceNormal(a, groundTris);  // the first skirt: the south edge
    CHECK(south.y < 0 && std::fabs(south.z) < std::fabs(south.y));
    // The skirt hangs below its edge at every level.
    for (int lod = 0; lod < kValleyLods; ++lod) {
        buildValleyTile(v, 8, 8, lod, a);
        const int q = kTileQuads >> lod, s = q + 1;
        CHECK(a.pos[std::size_t(s) * s].z < a.pos[0].z - 2.0f);
    }
    CHECK(valleyLodFor(10) == 0 && valleyLodFor(100) == 1 && valleyLodFor(400) == 2);
    ValleyMesh extras, water;
    buildValleyExtras(v, extras);
    buildValleyWater(v, water);
    std::printf("  extras %d, water %d triangles\n", extras.triangles(), water.triangles());
    CHECK(extras.triangles() > 50 && extras.triangles() < 800 && water.triangles() >= 2);
}

TEST(flying_over_the_valley) {
    const Valley& v = valley();
    const ValleyPlaceInfo& den = *v.place(kPlaceDen);
    Flight f;
    f.pos = den.at + Vec3{40, 0, 0};
    f.pos.z = v.heightAt(f.pos.x, f.pos.y);
    f.heading = den.heading;
    const float dt = 1.0f / 30;
    auto run = [&](const FlightInput& in, float seconds) {
        for (int k = 0; k < static_cast<int>(seconds * 30); ++k) {
            f.update(in, v, dt);
            CHECK(f.pos.z >= v.heightAt(f.pos.x, f.pos.y) - 0.01f);  // never under the ground
            CHECK(v.inside(f.pos.x, f.pos.y));
        }
    };
    FlightInput none, flap, dive, left, bankLeft;
    flap.flap = true;
    dive.dive = true;
    left.steer = -1;
    bankLeft.steer = -1;
    bankLeft.bank = -1;
    run(none, 1);
    CHECK(f.grounded);
    // Up with a wingbeat, and climbing while A is held (stamina spent).
    run(flap, 6);
    const float high = f.pos.z - v.heightAt(f.pos.x, f.pos.y);
    std::printf("  six seconds of wingbeats: %.0f m up, stamina %.2f, speed %.1f\n", high, f.stamina, f.speed);
    CHECK(!f.grounded && high > 20 && f.stamina < 0.6f);
    // Gliding: a gentle sink at glide speed.
    const float z0 = f.pos.z;
    run(none, 4);
    const float sink = (z0 - f.pos.z) / 4;
    std::printf("  gliding: %.1f m/s down at %.1f m/s\n", sink, f.speed);
    CHECK(sink > 0.3f && sink < 4.0f && std::fabs(f.speed - 11) < 3);
    // Steering turns it; banking with L/R turns it faster.
    const float h0 = f.heading;
    run(left, 1);
    const float plain = std::fabs(std::remainder(f.heading - h0, 6.2831853f));
    const float h1 = f.heading;
    run(bankLeft, 1);
    const float banked = std::fabs(std::remainder(f.heading - h1, 6.2831853f));
    CHECK(plain > 0.5f && banked > plain * 1.4f && f.roll < -0.3f);
    // A dive: fast and down.
    run(flap, 4);
    const float z1 = f.pos.z;
    run(dive, 1.5f);
    std::printf("  a dive: %.1f m/s, %.0f m down in 1.5 s\n", f.speed, z1 - f.pos.z);
    CHECK(f.speed > 18 && z1 - f.pos.z > 8);
    // Down to the ground, slow, over flat ground: it lands; then it stands and gets its breath back.
    for (int k = 0; k < 30 * 60 && !f.grounded; ++k) f.update(none, v, dt);
    CHECK(f.grounded);
    const float s0 = f.stamina;
    run(none, 2);
    CHECK(f.stamina > s0);
    // The camera: behind it and above the ground.
    ChaseCamera cam;
    for (int k = 0; k < 60; ++k) cam.update(f, v, dt);
    CHECK(cam.eye.z > v.heightAt(cam.eye.x, cam.eye.y));
    CHECK(dot(cam.target - cam.eye, f.forward()) > 0);
}

TEST(walking_in_the_valley) {
    const Valley& v = valley();
    const ValleyPlaceInfo& den = *v.place(kPlaceDen);
    const float dt = 1.0f / 30;
    Flight f;
    f.pos = den.at + Vec3{std::sin(den.heading), -std::cos(den.heading), 0} * 40.0f;
    f.pos.z = v.heightAt(f.pos.x, f.pos.y);
    f.heading = den.heading;
    auto run = [&](const FlightInput& in, float seconds) {
        for (int k = 0; k < static_cast<int>(seconds * 30); ++k) {
            f.update(in, v, dt);
            if (f.grounded && !f.swimming) CHECK(std::fabs(f.pos.z - v.heightAt(f.pos.x, f.pos.y)) < 0.01f);  // on the ground
            if (f.swimming) CHECK(f.pos.z <= v.water && f.pos.z >= v.heightAt(f.pos.x, f.pos.y) - 0.01f);  // afloat
            CHECK(v.inside(f.pos.x, f.pos.y));
        }
    };
    FlightInput walk, runB, walkLeft, none;
    walk.pitch = 1;  // the pad pushed up
    runB.pitch = 1;
    runB.dive = true;
    walkLeft.pitch = 1;
    walkLeft.steer = -1;
    // Walking: forward at its walking speed, on the ground.
    Vec3 p0 = f.pos;
    run(walk, 3);
    const float walked = length(Vec3{f.pos.x - p0.x, f.pos.y - p0.y, 0});
    std::printf("  walking: %.1f m in 3 s (walk speed %.1f)\n", walked, f.walkSpeed);
    CHECK(f.grounded && walked > f.walkSpeed * 2.0f && walked < f.walkSpeed * 3.2f);
    // Running with B: much faster.
    p0 = f.pos;
    run(runB, 3);
    const float ran = length(Vec3{f.pos.x - p0.x, f.pos.y - p0.y, 0});
    std::printf("  running: %.1f m in 3 s\n", ran);
    CHECK(f.grounded && ran > walked * 2.0f);
    // It turns as it walks, and stops when let go.
    const float h0 = f.heading;
    run(walkLeft, 1);
    CHECK(std::fabs(std::remainder(f.heading - h0, 6.2831853f)) > 0.8f);
    run(none, 1);
    CHECK(f.speed == 0 && f.grounded);
    // At the lake it swims (D81): from dry ground a few metres from deep water, walking straight
    // in, it splashes in and floats; turned round, it walks back out onto the shore.
    bool found = false;
    for (float y = v.y0 + 60; y < v.y0 + v.size() - 60 && !found; y += 8)
        for (float x = v.x0 + 60; x < v.x0 + v.size() - 60 && !found; x += 8)
            if (v.heightAt(x, y) > v.water + 0.5f && v.heightAt(x + 12, y) < v.water - 1.5f &&
                v.normalAt(x, y).z > 0.9f) {
                f = Flight{};
                f.pos = {x, y, v.heightAt(x, y)};
                f.heading = 1.5707963f;  // facing +X, the water
                found = true;
            }
    CHECK(found);
    bool splashed = false;
    for (int k = 0; k < 12 * 30; ++k) {
        f.update(walk, v, dt);
        splashed |= f.splashed;
    }
    std::printf("  swimming: %.2f m of water under its feet, afloat %d\n", v.water - f.pos.z, f.swimming ? 1 : 0);
    CHECK(f.grounded && f.swimming && splashed && f.pos.z < v.water);
    f.heading += 3.14159265f;  // back the way it came
    run(walk, 20);
    CHECK(f.grounded && !f.swimming);
    // Coming down slowly onto the lake: in with a splash, swimming.
    f = Flight{};
    f.grounded = false;
    for (float y = v.y0 + 60; y < v.y0 + v.size() - 60 && f.pos.x == 0; y += 8)
        for (float x = v.x0 + 60; x < v.x0 + v.size() - 60; x += 8)
            if (v.heightAt(x, y) < v.water - 3.0f && v.heightAt(x + 40, y) < v.water - 3.0f) {
                f.pos = {x, y, v.water + 6.0f};
                break;
            }
    f.heading = 1.5707963f;
    f.speed = 9;
    FlightInput none2;
    splashed = false;
    for (int k = 0; k < 30 * 8 && !f.grounded; ++k) {
        f.update(none2, v, dt);
        splashed |= f.splashed;
    }
    CHECK(f.grounded && f.swimming && splashed);
}

void runValleyTests() {
    RUN(the_valley_loads);
    RUN(valley_tiles_join_and_fit_the_budget);
    RUN(flying_over_the_valley);
    RUN(walking_in_the_valley);
}
