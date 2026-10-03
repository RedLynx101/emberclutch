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
#include "core/finds.hpp"

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
    // The painted texture's mix: grass and leaves the strokes, paths, rock and bark the speckle,
    // whatever the light (a shaded path is still a path); every tile vertex carries it.
    CHECK(surfaceWeight(126, 178, 84) == 0 && surfaceWeight(42, 73, 36) == 0 && surfaceWeight(64, 128, 58) == 0);
    CHECK(surfaceWeight(214, 184, 132) == 255 && surfaceWeight(107, 92, 66) == 255 && surfaceWeight(118, 84, 58) == 255);
    CHECK(surfaceWeight(150, 134, 142) > 200 && surfaceWeight(240, 244, 252) > 60 && surfaceWeight(240, 244, 252) < 190);
    bool mixed = false;
    for (std::size_t i = 3; i < a.color.size(); i += 4) mixed |= a.color[i] != 255;
    CHECK(mixed);
    ValleyMesh extras, water;
    buildValleyExtras(v, extras);
    buildValleyWater(v, water, {0, 0}, 360.0f);
    std::printf("  extras %d, water %d triangles\n", extras.triangles(), water.triangles());
    CHECK(extras.triangles() > 50 && extras.triangles() < 3600 && water.triangles() >= 2);
    // The islands (run 28, made pretty): each in its runs, within its budget and the bounds render3d
    // culls it by (and the sky rings keep clear of), its top flat at its height out past where it's
    // stood on, trees on it.
    CHECK(extras.parts.size() == v.islands.size() * kIslandRuns + 1);
    for (std::size_t k = 0; k + 1 < extras.parts.size(); ++k) CHECK(extras.parts[k] <= extras.parts[k + 1]);
    for (std::size_t n = 0; n < v.islands.size(); ++n) {
        const ValleyIsland& isl = v.islands[n];
        const u32* run = &extras.parts[n * kIslandRuns];
        const int tris = static_cast<int>(run[kIslandRuns] - run[0]) / 3;
        std::printf("  island %d (radius %.0f): %d triangles, %d of them its props\n", static_cast<int>(n), isl.radius, tris,
                    static_cast<int>(run[kIsleProps + 1] - run[kIsleProps]) / 3);
        CHECK(tris > 200 && tris < 720 && run[kIsleProps + 1] > run[kIsleProps]);
        float rim = isl.radius * 2;
        for (u32 i = run[0]; i < run[kIslandRuns]; ++i) {
            const Vec3 p = extras.pos[extras.idx[i]];
            const float out = std::hypot(p.x - isl.at.x, p.y - isl.at.y);
            CHECK(out < isl.radius * 1.3f && p.z < isl.at.z + 14.0f && p.z > isl.at.z - isl.radius * 1.9f);
            if (i < run[kIsleProps] && std::fabs(p.z - isl.at.z) < 1e-4f && out > isl.radius * 0.7f) rim = std::fmin(rim, out);
        }
        CHECK(rim > isl.radius * 0.88f && rim < isl.radius);
    }
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
    // R bursts ahead, level, spending stamina; L brakes (the valley's L/R since take 4, D112).
    FlightInput burst, brake;
    burst.burst = true;
    brake.brake = true;
    f.stamina = 1;
    const float zb = f.pos.z;
    run(burst, 3);
    std::printf("  a burst: %.1f m/s, stamina %.2f, %.1f m down\n", f.speed, f.stamina, zb - f.pos.z);
    CHECK(f.speed > 20 && f.stamina < 0.6f && zb - f.pos.z < 3);
    run(brake, 3);
    std::printf("  braking: %.1f m/s\n", f.speed);
    CHECK(!f.grounded && f.speed < 8);
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

// Run 26 (Noah: "When I walked off of the sky island above the lake, it auto teleported me to the ground,
// rather than putting me into flight mode. Fix this for all sky islands"): riding, walking off the edge of
// every floating island takes off into a glide (over the lake or over land), never dropping to the water.
TEST(walking_off_a_floating_island_glides) {
    const Valley& v = valley();
    int tried = 0;
    for (const ValleyIsland& isl : v.islands) {
        if (isl.radius < 3.0f) continue;
        for (float a : {0.0f, 1.57f, 3.14f, 4.71f}) {  // walking out from its middle four ways
            Flight f;
            f.pos = isl.at;
            f.pos.z = v.groundAt(isl.at.x, isl.at.y, isl.at.z + 1.0f);
            if (v.islandAt(f.pos.x, f.pos.y, f.pos.z) < 0) continue;
            f.heading = a;
            FlightInput walk;
            walk.pitch = 1;
            walk.dive = true;  // (running)
            const float top = f.pos.z;
            bool glided = false;
            for (int k = 0; k < 30 * 20 && f.grounded; ++k) {
                f.update(walk, v, 1.0f / 30);
                if (!f.grounded) glided = true;
                if (f.speed == 0) break;  // (stopped at the valley's edge or a wall)
            }
            if (f.speed == 0 && f.grounded) continue;
            ++tried;
            CHECK(glided);
            CHECK(f.pos.z > top - 3.0f);  // (it left at the island's height: no drop to the water or ground)
            if (!glided || f.pos.z <= top - 3.0f)
                std::printf("  FAIL: off the island at (%.0f %.0f %.0f) heading %.2f: z %.1f, grounded %d\n", isl.at.x, isl.at.y,
                            isl.at.z, a, f.pos.z, f.grounded ? 1 : 0);
        }
    }
    std::printf("  walked off the floating islands %d ways\n", tried);
    CHECK(tried >= 4);
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



// Riding by the dragon's stats (D123): an average dragon flies as the defaults; a strong one's
// wingbeats and bursts cost less and it's a little faster; a weak one tires sooner.
TEST(flight_follows_wing_and_stamina) {
    const FlightTuning plain, avg = flightTuningFor(5, 5), strong = flightTuningFor(10, 10), weak = flightTuningFor(1, 1);
    CHECK(std::fabs(avg.flapCost - plain.flapCost) < 1e-6f && std::fabs(avg.burstSpeed - plain.burstSpeed) < 1e-6f);
    CHECK(strong.flapCost < plain.flapCost && strong.burstCost < plain.burstCost && strong.restRate > plain.restRate);
    CHECK(strong.burstSpeed > plain.burstSpeed && strong.flapLift > plain.flapLift);
    CHECK(weak.flapCost > plain.flapCost && weak.burstCost > plain.burstCost && weak.burstSpeed < plain.burstSpeed);
    // A full breath of bursting: about 9 s strong, 6 s average, under 4 s weak.
    std::printf("  flight: a breath of bursting %.1f s strong, %.1f s average, %.1f s weak\n", 1.0f / strong.burstCost,
                1.0f / plain.burstCost, 1.0f / weak.burstCost);
    CHECK(1.0f / strong.burstCost > 8.5f && 1.0f / weak.burstCost < 4.0f);
}

// The cold heights' glide (D132): up by the Vault and down 25 m counts; too far from it, not high
// enough, or not far enough down, no; and there's that far to glide down below it.
TEST(the_cold_heights_glide) {
    const Valley& v = valley();
    const ValleyPlaceInfo& vault = *v.place(kPlaceVault);
    const Vec3 up = vault.at + Vec3{20, 10, 0};
    CHECK(glidedFromHeights(vault.at, 70, up, up + Vec3{60, 0, -30}));
    CHECK(!glidedFromHeights(vault.at, 70, up, up + Vec3{60, 0, -10}));             // not far enough down
    CHECK(!glidedFromHeights(vault.at, 70, up + Vec3{120, 0, 0}, up + Vec3{160, 0, -40}));  // not by the Vault
    CHECK(!glidedFromHeights(vault.at, 70, up - Vec3{0, 0, 20}, up + Vec3{60, 0, -60}));   // below the heights
    // Somewhere 25 m or more below the heights within an easy glide (the valley below the Vault).
    float lowest = 1e9f;
    for (int dx = -240; dx <= 240; dx += 8)
        for (int dy = -240; dy <= 240; dy += 8)
            if (v.inside(vault.at.x + dx, vault.at.y + dy))
                lowest = std::fmin(lowest, std::fmax(v.water, v.heightAt(vault.at.x + dx, vault.at.y + dy)));
    std::printf("  cold heights: the Vault at %.0f m, the valley below it down to %.0f m\n", vault.at.z, lowest);
    CHECK(vault.at.z - lowest > 30.0f);
}

// The picnic on the Stone's hill (D133): on open, gentle ground above the water, its blanket and
// basket within a couple of metres of the spot, the letter read once for its Gleam.
TEST(the_picnic_and_its_letter) {
    const Valley& v = valley();
    CHECK(v.heightAt(kPicnicAt.x, kPicnicAt.y) > v.water + 1.0f);
    CHECK(v.normalAt(kPicnicAt.x, kPicnicAt.y).z > 0.95f);
    ValleyMesh with, without;
    buildPicnic(v, true, with);
    buildPicnic(v, false, without);
    CHECK(with.triangles() > without.triangles() && without.triangles() > 40);
    for (const Vec3& p : with.pos) CHECK(std::hypot(p.x - kPicnicAt.x, p.y - kPicnicAt.y) < 2.0f && p.z > v.water);
    std::printf("  picnic: %d triangles (%d with the letter)\n", without.triangles(), with.triangles());
    SaveData s;
    const u32 before = s.gleam;
    CHECK(takeLetter(s) && s.gleam == before + kLetterGleam && (s.world.flags & kFlagLoveLetter));
    CHECK(!takeLetter(s) && s.gleam == before + kLetterGleam);
}

// Flying through a treetop (D122): inside a tree's crown it's that tree; over it, beside it or
// under its leaves (by the trunk), none.
TEST(a_treetop_has_a_crown) {
    const Valley& v = valley();
    int checked = 0;
    for (std::size_t k = 0; k < v.trees.size() && checked < 40; ++k) {
        const ValleyTree& tr = v.trees[k];
        if (tr.kind != kPropTree && tr.kind != kPropPine && tr.kind != kPropFruit) continue;
        const float g = v.heightAt(tr.x, tr.y);
        const int in = crownAt(v, {tr.x, tr.y, g + tr.height * 0.6f});
        CHECK(in >= 0);
        if (in >= 0) {
            const ValleyTree& found = v.trees[std::size_t(in)];
            CHECK(std::hypot(found.x - tr.x, found.y - tr.y) < tr.height);  // (itself, or one its leaves touch)
        }
        CHECK(crownAt(v, {tr.x, tr.y, g + tr.height * 1.3f + 1.0f}) < 0 || crownAt(v, {tr.x, tr.y, g + tr.height * 1.3f + 1.0f}) != static_cast<int>(k));
        ++checked;
    }
    std::printf("  treetops: %d checked of %zu props\n", checked, v.trees.size());
    CHECK(checked > 10);
    CHECK(crownAt(v, {v.x0 - 50, v.y0 - 50, 20}) < 0);  // (outside the valley)
}

// Swimming (1.0, D121): set down in the lake you float, the water at your shoulders, and swim where
// the pad points; out at a shore you walk again. Your partner may swim out after you.
TEST(you_swim_in_deep_water) {
    const Valley& v = valley();
    const ValleyPlaceInfo& lake = *v.place(kPlaceLake);
    WalkTuning tune;
    tune.swim = 0.95f;
    // Dropped off a dragon's back over the deepest water near the lake's anchor: afloat, not on its bed.
    Vec3 deep = lake.at;
    for (int dx = -40; dx <= 40; dx += 4)
        for (int dy = -40; dy <= 40; dy += 4)
            if (v.heightAt(lake.at.x + dx, lake.at.y + dy) < v.heightAt(deep.x, deep.y))
                deep = {lake.at.x + dx, lake.at.y + dy, 0};
    CHECK(v.heightAt(deep.x, deep.y) < v.water - 1.5f);
    Walker you;
    you.pos = deep + Vec3{0, 0, v.water + 6.0f};
    CHECK(you.drop(v, tune));
    CHECK(you.swimming && std::fabs(you.pos.z - (v.water - 0.95f)) < 1e-3f);
    // It swims: every way the pad points it goes, and stays afloat on the surface.
    const float dt = 1.0f / 30;
    const Vec3 start = you.pos;
    WalkInput east;
    east.x = 1;
    for (int k = 0; k < 60; ++k) you.update(east, 3.14159f, v, {}, dt, tune);
    const float swam = std::hypot(you.pos.x - start.x, you.pos.y - start.y);
    CHECK(swam > 1.5f && !you.blocked);
    CHECK(you.speed > 1.0f && you.speed <= tune.swimSpeed + 0.01f);
    // Out the other side at a shore: swimming on toward it, it walks out of the water.
    WalkInput north;
    north.y = 1;
    bool outOfWater = false;
    for (int k = 0; k < 30 * 120 && !outOfWater; ++k) {
        you.update(north, 3.14159f, v, {}, dt, tune);
        outOfWater = !you.swimming && v.heightAt(you.pos.x, you.pos.y) > v.water;
    }
    std::printf("  swimming: %.1f m in 2 s from the deep water, then out on the shore at %.0f %.0f\n", swam, you.pos.x, you.pos.y);
    CHECK(outOfWater);
    // Without swimming (the default), deep water stays a wall.
    Walker wader;
    wader.pos = deep;
    CHECK(!wader.drop(v));
    CHECK(wader.pos.z < v.water - 0.5f);
    // The partner: told it may swim, it follows you out into deep water.
    Walker me;
    me.pos = deep;
    me.drop(v, tune);
    Follower pal;
    pal.swim = true;
    pal.pos = deep + Vec3{6, 0, 0};
    pal.pos.z = std::fmax(v.heightAt(pal.pos.x, pal.pos.y), v.water - 0.8f);
    me.heading = 1.0f;
    for (int k = 0; k < 90; ++k) {
        me.update(east, 3.14159f, v, {}, dt, tune);
        pal.update(me, v, {}, dt);
    }
    CHECK(std::hypot(pal.pos.x - me.pos.x, pal.pos.y - me.pos.y) < 4.0f && !pal.lost());
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
    // Into the lake: it stops at the water's edge (beside the jetty: straight down the middle walks out along it).
    const ValleyPlaceInfo& lake = *v.place(kPlaceLake);
    Walker wader;
    wader.pos = lake.at + Vec3{20, 30, 0};
    wader.pos.z = v.heightAt(wader.pos.x, wader.pos.y);
    WalkInput south;
    south.y = 1;  // the camera looking south (yaw 0): pad up walks south, into the lake
    for (int k = 0; k < 400; ++k) wader.update(south, 0.0f, v, {}, dt);
    CHECK(v.heightAt(wader.pos.x, wader.pos.y) > v.water - 0.61f);
    // The lake's jetty (1.0.1): from the beach behind it you walk up onto its deck and out to its end, dry.
    Walker stroller;
    stroller.pos = lake.at + Vec3{0, 12, 0};
    stroller.pos.z = v.heightAt(stroller.pos.x, stroller.pos.y);
    for (int k = 0; k < 400; ++k) stroller.update(south, 0.0f, v, {}, dt);
    CHECK(stroller.pos.y < lake.at.y - 8.0f && stroller.pos.y > lake.at.y - 8.7f);  // its far end, and no further
    CHECK(v.deckAt(stroller.pos.x, stroller.pos.y, stroller.pos.z) >= 0);
    CHECK(std::fabs(stroller.pos.z - (lake.at.z + 0.3f)) < 0.02f);
    CHECK(v.heightAt(stroller.pos.x, stroller.pos.y) < v.water - 0.8f);  // (deep water under it)
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

// The den's tunnel floor over the ground (run 21 take 4: "a bit of the ground over the floor
// entrance"): its model's floor sits 3 cm over the anchor (the arch spans x +-3.6, y -3.6 .. 0.35
// in its frame), and the ground under all of it stays below.
TEST(the_den_floor_stays_over_the_ground) {
    const Valley& v = valley();
    const ValleyPlaceInfo& den = *v.place(kPlaceDen);
    const float floor = den.at.z + 0.03f;
    float worst = -1e9f, wx = 0, wy = 0;
    for (float x = -3.5f; x <= 3.5f; x += 0.1f)
        for (float y = -3.6f; y <= 0.35f; y += 0.05f) {
            const Vec2 w = placeToWorld(den, {x, y});
            const float over = v.heightAt(w.x, w.y) - floor;
            if (over > worst) worst = over, wx = x, wy = y;
        }
    std::printf("  the ground under the den's floor: at most %.3f m from it (at %.1f, %.2f)\n", worst, wx, wy);
    CHECK(worst < -0.02f);
}

// Frostspire Hollow's room level with the place's anchor (take 4: the river's broad valley had
// lowered it 3 m, and the cave and the frost ring floated over the ground you stood on).
TEST(the_hollow_floor_meets_its_place) {
    const Valley& v = valley();
    const ValleyPlaceInfo& hollow = *v.place(kPlaceHollow);
    float worst = 0;
    for (float r = 0; r <= 11.0f; r += 1.0f)  // (the floor proper: past it the rim rises, spires stand)
        for (int k = 0; k < 16; ++k) {
            const float a = k * 0.3927f;
            worst = std::fmax(worst, std::fabs(v.heightAt(hollow.at.x + r * std::cos(a), hollow.at.y + r * std::sin(a)) - hollow.at.z));
        }
    std::printf("  the Hollow's room: at most %.3f m off its place's height\n", worst);
    CHECK(worst < 0.05f);
}

// 1.0.1: Mirror Lake stands on its shore. (1.0 had its anchor at its path's end, 14 m up the beach where the sand
// is 2 m higher: the jetty, the boat and the bench lay under it, and the boat's wall stood in the way unseen.)
TEST(the_lake_stands_on_its_shore) {
    const Valley& v = valley();
    const ValleyPlaceInfo& lake = *v.place(kPlaceLake);
    CHECK(std::fabs(v.heightAt(lake.at.x, lake.at.y) - lake.at.z) < 0.25f);  // the sand at its anchor, at its height
    CHECK(std::fabs(lake.at.z + placeLayout(kPlaceLake).waterZ - v.water) < 0.01f);
    // Nothing of it under the ground: the sand at every wall of its (rocks, bench, boat, lamp) is no higher than
    // half a metre over the place's own floor, and where the boat floats and the jetty ends there's water.
    float highest = -1e9f;
    for (const Solid& s : placeLayout(kPlaceLake).solids) {
        const Vec2 w = placeToWorld(lake, s.at);
        highest = std::fmax(highest, v.heightAt(w.x, w.y) - lake.at.z);
    }
    std::printf("  the lake's things: the sand at most %.2f m over its floor\n", highest);
    CHECK(highest < 0.5f);
    const PlaceAnchor ends = placeAnchor(kPlaceLake, "jetty");
    CHECK(ends.count == 2);
    if (ends.count != 2) return;
    const Vec2 shore = placeToWorld(lake, {ends.at(0).x, ends.at(0).y}), out = placeToWorld(lake, {ends.at(1).x, ends.at(1).y});
    const float top = lake.at.z + ends.at(0).z;
    CHECK(top - v.heightAt(shore.x, shore.y) > 0.0f && top - v.heightAt(shore.x, shore.y) < 0.4f);  // a step up from the sand
    CHECK(v.heightAt(out.x, out.y) < v.water - 0.8f);
    CHECK(v.deckAt(out.x, out.y, top) >= 0 && std::fabs(v.groundAt(out.x, out.y, top) - top) < 0.01f);
}

void runValleyTests() {
    RUN(islands_and_mountainsides);
    RUN(every_path_walks);
    RUN(the_valley_loads);
    RUN(valley_tiles_join_and_fit_the_budget);
    RUN(flying_over_the_valley);
    RUN(walking_off_a_floating_island_glides);
    RUN(walking_in_the_valley);
    RUN(on_foot_with_your_partner);
    RUN(you_swim_in_deep_water);
    RUN(a_treetop_has_a_crown);
    RUN(flight_follows_wing_and_stamina);
    RUN(the_cold_heights_glide);
    RUN(the_picnic_and_its_letter);
    RUN(the_den_floor_stays_over_the_ground);
    RUN(the_hollow_floor_meets_its_place);
    RUN(the_lake_stands_on_its_shore);
}
