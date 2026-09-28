// Skyreach Valley (Beta WP1): the landscape's data and tiles, and flying over it.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/flight.hpp"
#include "core/place_layout.hpp"
#include "core/valley.hpp"
#include "core/walker.hpp"

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
        addPlaceDecks(v);  // (as the game: the mill's bridge, the cove's jetty)
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
    CHECK(v.n == 577 && v.tiles() == 36);  // Beta's valley: 2.3 km across (D81)
    CHECK(std::fabs(v.size() - 2304.0f) < 0.01f && std::fabs(v.tileSize() - 64.0f) < 0.01f);
    CHECK(v.hmax > 250 && v.hmin < v.water);
    CHECK(v.trees.size() > 3000 && v.islands.size() == 6 && v.places.size() == kPlaceCount && v.paths.size() >= 8);
    for (u8 p = 0; p < kPlaceCount; ++p) CHECK(v.place(p) != nullptr);
    int kinds[kPropKinds] = {};
    for (const ValleyTree& t : v.trees) ++kinds[t.kind];
    for (int k = 0; k < kPropKinds; ++k) CHECK(kinds[k] > 0);  // every kind of prop out there
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
    std::printf("  valley tiles: worst %d / %d / %d / %d, mean %.0f / %.0f / %.0f / %.0f triangles\n", worst[0], worst[1],
                worst[2], worst[3], mean[0], mean[1], mean[2], mean[3]);
    CHECK(worst[0] <= 1800 && worst[1] <= 560 && worst[2] <= 220 && worst[3] <= 80);  // (the densest wood; far cones)
    // A view: three tiles near, six mid, five with far trees, eight far (culled by the view, in
    // fog beyond): a typical one, and the worst (the densest forest tile everywhere, which can't happen).
    const double typical = 3 * mean[0] + 6 * mean[1] + 5 * mean[2] + 8 * mean[3];
    const int view = 3 * worst[0] + 6 * worst[1] + 5 * worst[2] + 8 * worst[3];
    std::printf("  a view: typically %.0f terrain triangles, at most %d\n", typical, view);
    // With the ridden dragon (3,000) and you (600) a typical view stays under the full den's
    // ~9,800 triangles, which runs at 17-18 ms on the old 3DS; the test valley's 7,000 ran at
    // 17 ms there (run 16). The valley's target is 30 fps (33 ms, Beta plan), so even the
    // impossible worst (the densest wood on every near tile) has room.
    // (Counted with every tile's skirts; since the flicker pass they're drawn only beside another
    // level, a few hundred triangles fewer in a real view.)
    CHECK(typical <= 5000 && view <= 10000);
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
    const Vec3 south = faceNormal(a, a.skirtFrom);  // the first skirt (after the props): the south edge
    CHECK(south.y < 0 && std::fabs(south.z) < std::fabs(south.y));
    // The skirt hangs below its edge at every level.
    for (int lod = 0; lod < kValleyLods; ++lod) {
        buildValleyTile(v, 8, 8, lod, a);
        const u16 top = a.idx[a.skirtFrom], low = a.idx[a.skirtFrom + 1];  // (reversed south skirt: top, low, ...)
        CHECK(a.pos[low].z < a.pos[top].z - 2.0f);
    }
    CHECK(valleyLodFor(10) == 0 && valleyLodFor(100) == 1 && valleyLodFor(150) == 2 && valleyLodFor(400) == 3);
    ValleyMesh extras, water;
    buildValleyExtras(v, extras);
    buildValleyWater(v, water, {0, 0}, 360.0f);
    std::printf("  extras %d, water %d triangles\n", extras.triangles(), water.triangles());
    CHECK(extras.triangles() > 50 && extras.triangles() < 1400 && water.triangles() >= 2);
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

// On foot (Beta WP5, D81): you walk the way the pad points from the camera's view, run with B,
// stop at deep water and walls; your partner keeps its spot at your side and, lost far behind,
// comes when called; the camera stays above the ground and turns with L and R.
TEST(on_foot_with_your_partner) {
    const Valley& v = valley();
    const ValleyPlaceInfo& market = *v.place(kPlaceMarket);
    Walker you;
    you.pos = market.at;
    std::vector<Solid> solids = {{{market.at.x + 12, market.at.y}, 3.0f}};  // a cottage to the east
    const float dt = 1.0f / 30;
    WalkInput east;
    east.x = 1;  // the pad to the right, the camera looking north (yaw pi): walks east
    const float camYaw = 3.14159f;
    for (int k = 0; k < 90; ++k) you.update(east, camYaw, v, solids, dt);
    CHECK(you.pos.x > market.at.x + 4);                              // off it went, east
    CHECK(you.pos.x < market.at.x + 12 - 3.0f + 0.01f);              // and up against the cottage, not in it
    const float walked = you.speed;
    WalkInput run = east;
    run.run = true;
    Walker runner;
    runner.pos = market.at + Vec3{0, 20, 0};
    for (int k = 0; k < 60; ++k) runner.update(run, camYaw, v, {}, dt);
    CHECK(runner.speed > 5.0f && runner.speed > walked);  // a chibi's run (D86)
    // Into the lake: it stops at the water's edge.
    const ValleyPlaceInfo& lake = *v.place(kPlaceLake);
    Walker wader;
    wader.pos = lake.at + Vec3{0, 30, 0};
    wader.pos.z = v.heightAt(wader.pos.x, wader.pos.y);
    WalkInput south;
    south.y = 1;  // the camera looking south (yaw 0): pad up walks south, into the lake
    for (int k = 0; k < 400; ++k) wader.update(south, 0.0f, v, {}, dt);
    CHECK(v.heightAt(wader.pos.x, wader.pos.y) > v.water - 0.61f);
    // The partner: at your side after a walk, facing your way.
    Follower pal;
    pal.pos = market.at + Vec3{-6, 4, 0};
    Walker me;
    me.pos = market.at + Vec3{0, 30, 0};
    WalkInput north;
    north.y = 1;
    for (int k = 0; k < 120; ++k) {
        me.update(north, camYaw, v, {}, dt);
        pal.update(me, v, {}, dt);
    }
    for (int k = 0; k < 90; ++k) {
        me.update(WalkInput{}, camYaw, v, {}, dt);
        pal.update(me, v, {}, dt);
    }
    // (Standing still, it stays where it caught up rather than circling to its exact spot.)
    const float near = std::hypot(pal.pos.x - me.pos.x, pal.pos.y - me.pos.y);
    std::printf("  on foot: walked %.1f m/s, ran %.1f m/s; partner %.1f m from you\n", walked, runner.speed, near);
    CHECK(near < pal.gap * 1.9f && near > 0.5f && pal.settled && !pal.lost());
    const Vec3 still = pal.pos;
    me.heading += 1.5f;  // looking about: it stays put
    for (int k = 0; k < 60; ++k) pal.update(me, v, {}, dt);
    CHECK(std::hypot(pal.pos.x - still.x, pal.pos.y - still.y) < 0.05f);
    // Left far behind across the lake: lost, then called to your side.
    pal.pos = lake.at + Vec3{0, -60, 0};
    pal.pos.z = v.heightAt(pal.pos.x, pal.pos.y);
    std::vector<Solid> wall;
    for (int k = 0; k < 150; ++k) pal.update(me, v, wall, dt);
    pal.call(me, v);
    const Vec3 side = pal.spot(me);
    CHECK(std::hypot(pal.pos.x - side.x, pal.pos.y - side.y) < 0.5f && !pal.lost());
    // The camera: above the ground, turning with L/R.
    WalkCamera cam;
    cam.update(me, 0, v, dt);
    const float before = cam.yaw;
    for (int k = 0; k < 30; ++k) cam.update(me, 1.0f, v, dt);
    CHECK(cam.yaw != before && cam.eye.z > v.heightAt(cam.eye.x, cam.eye.y) + 1.0f);
}

// Run 19: the floating islands stood on (landing on one, walking its top, never off its edge),
// and flying into a mountainside keeps its height instead of shooting up the slope.
TEST(islands_and_mountainsides) {
    const Valley& v = valley();
    CHECK(v.islands.size() >= 2);
    if (v.islands.size() < 2) return;
    const ValleyIsland& isl = v.islands[1];  // over the lake
    CHECK(v.islandAt(isl.at.x, isl.at.y, isl.at.z + 0.1f) == 1 && v.islandAt(isl.at.x, isl.at.y, isl.at.z - 10) < 0);
    CHECK(std::fabs(v.groundAt(isl.at.x, isl.at.y, isl.at.z + 3) - isl.at.z) < 1e-3f);
    // Gliding in from the west a little above its top: it comes down on it and lands.
    Flight f;
    f.pos = isl.at + Vec3{-isl.radius - 25, 0, 9};
    f.heading = 1.5707963f;  // east
    f.grounded = false;
    f.speed = 11;
    FlightInput none;
    for (int k = 0; k < 30 * 10 && !f.grounded; ++k) f.update(none, v, 1.0f / 30);
    std::printf("  onto the island: grounded %d at (%.0f %.0f %.1f), its top %.1f\n", f.grounded, f.pos.x, f.pos.y, f.pos.z,
                isl.at.z);
    CHECK(f.grounded && std::fabs(f.pos.z - isl.at.z) < 0.05f && v.islandAt(f.pos.x, f.pos.y, f.pos.z) == 1);
    // On foot there: walking east for a while never steps off its edge.
    Walker you;
    you.pos = isl.at;
    WalkInput east;
    east.x = 1;
    std::vector<Solid> none2;
    for (int k = 0; k < 30 * 12; ++k) {
        you.update(east, 0.0f, v, none2, 1.0f / 30);
        CHECK(v.islandAt(you.pos.x, you.pos.y, you.pos.z) == 1 && std::fabs(you.pos.z - isl.at.z) < 1e-3f);
    }
    // A partner called beside you there stands on it too.
    Follower pal;
    pal.call(you, v);
    CHECK(v.islandAt(pal.pos.x, pal.pos.y, pal.pos.z) == 1);
    // Flying west at the den's cliff: it slides along the face, no higher than a wingbeat's worth.
    Flight g;
    g.grounded = false;
    g.pos = {-575.0f, 60.0f, v.heightAt(-575.0f, 60.0f) + 25.0f};
    g.heading = -1.5707963f;  // west, at the cliff
    g.speed = 16;
    const float z0 = g.pos.z;
    float top = z0;
    for (int k = 0; k < 30 * 6; ++k) {
        g.update(none, v, 1.0f / 30);
        top = std::fmax(top, g.pos.z);
        CHECK(g.pos.z >= v.heightAt(g.pos.x, g.pos.y) - 0.01f);
    }
    std::printf("  into the cliff: rose %.1f m (from %.0f), now x %.0f z %.0f, speed %.1f\n", top - z0, z0, g.pos.x, g.pos.z,
                g.speed);
    CHECK(top - z0 < 6.0f);
}

// Run 19: every earth path can be walked end to end (the Cold Vault's trail wasn't): a walker
// steered at each next point along it arrives at its end.
TEST(every_path_walks) {
    const Valley& v = valley();
    CHECK(!v.paths.empty());
    std::vector<Solid> none;
    for (std::size_t k = 0; k < v.paths.size(); ++k) {
        const std::vector<Vec2>& path = v.paths[k];
        if (path.size() < 2) continue;
        Walker w;
        w.pos = {path[0].x, path[0].y, v.heightAt(path[0].x, path[0].y)};
        std::size_t next = 1;
        float stuck = 0;
        for (int step = 0; step < 30 * 600 && next < path.size(); ++step) {
            const Vec2 to = path[next], from = path[next - 1];
            if (std::hypot(to.x - w.pos.x, to.y - w.pos.y) < 3.0f) {
                ++next;
                continue;
            }
            // Steered along the path (a point 5 m on from the nearest one on this stretch), as a
            // player follows it, not straight at the next bend across the hillside.
            const float sx = to.x - from.x, sy = to.y - from.y, len = std::hypot(sx, sy);
            float along = len > 0 ? ((w.pos.x - from.x) * sx + (w.pos.y - from.y) * sy) / len : 0;
            along = std::fmin(len, std::fmax(0.0f, along) + 5.0f);
            const float dx = from.x + sx / len * along - w.pos.x, dy = from.y + sy / len * along - w.pos.y;
            // Pad up with the camera turned toward the point: the walker heads there.
            WalkInput in;
            in.y = 1;
            in.run = true;
            const float yaw = std::atan2(dx, -dy);
            const Vec3 before = w.pos;
            w.update(in, yaw, v, none, 1.0f / 30);
            stuck = length(w.pos - before) < 0.01f ? stuck + 1.0f / 30 : 0.0f;
            if (stuck > 3.0f) break;
        }
        if (next < path.size())
            std::printf("  path %d stuck at (%.0f %.0f), heading for point %d (%.0f %.0f)\n", static_cast<int>(k), w.pos.x, w.pos.y,
                        static_cast<int>(next), path[next].x, path[next].y);
        CHECK(next >= path.size());
    }
}

void runValleyTests() {
    RUN(islands_and_mountainsides);
    RUN(every_path_walks);
    RUN(the_valley_loads);
    RUN(valley_tiles_join_and_fit_the_budget);
    RUN(flying_over_the_valley);
    RUN(walking_in_the_valley);
    RUN(on_foot_with_your_partner);
}
