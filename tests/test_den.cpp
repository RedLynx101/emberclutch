// Den scene tests (WP6): the room file (romfs/models/den.esm, tools/blender/den_model.py)
// loads, fits the frame budget and matches the behavior's layout; the lighting sets and the
// time-of-day blend read right; particles live, fall and die within their pool.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "check.hpp"
#include "core/behavior.hpp"
#include "core/clock.hpp"
#include "core/daylight.hpp"
#include "core/particles.hpp"
#include "core/prop_mesh.hpp"
#include "core/static_mesh.hpp"

using namespace ec;

namespace {

std::vector<u8> fileBytes(const char* path) {
    std::vector<u8> data;
    FILE* f = std::fopen(path, "rb");
    if (!f) return data;
    std::fseek(f, 0, SEEK_END);
    data.resize(static_cast<std::size_t>(std::ftell(f)));
    std::fseek(f, 0, SEEK_SET);
    if (std::fread(data.data(), 1, data.size(), f) != data.size()) data.clear();
    std::fclose(f);
    return data;
}

const std::vector<u8>& denBytes() {
    static const std::vector<u8> bytes = fileBytes("../romfs/models/den.esm");
    return bytes;
}

const StaticScene& den() {
    static StaticScene s;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        CHECK(loadStaticScene(denBytes().data(), denBytes().size(), s));
    }
    return s;
}

// xy centre of a part's vertices (z too, for wall pieces).
Vec3 centreOf(const StaticPart& p) {
    Vec3 c{0, 0, 0};
    for (int v = p.firstVertex; v < p.firstVertex + p.vertexCount; ++v) c = c + den().pos[v];
    return c * (1.0f / p.vertexCount);
}

float dist2d(Vec3 a, Vec2 b) { return std::hypot(a.x - b.x, a.y - b.y); }

float luminance(const u8* rgba) { return 0.3f * rgba[0] + 0.59f * rgba[1] + 0.11f * rgba[2]; }

float partLuminance(const StaticPart& p, int set) {
    float sum = 0;
    for (int v = p.firstVertex; v < p.firstVertex + p.vertexCount; ++v) sum += luminance(den().colors(set) + v * 4);
    return sum / p.vertexCount;
}

constexpr s64 kMidnight = 1767571200;  // 2026-01-05 00:00 local

TEST(den_file_loads_and_fits_the_frame_budget) {
    const StaticScene& s = den();
    CHECK(s.sets == kLightSets && s.backdrop.size() == std::size_t(kLightSets) * 4);
    // The den frame is <= 8k triangles with a full den: three dragons, 3,000 (LOD0) + 2 x 1,200
    // (LOD1), and two eggs in the nests at 300 (the egg's LOD1).
    std::printf("  den: %d parts, %d triangles, %d vertices, %zu bytes\n", static_cast<int>(s.parts.size()),
                s.triangles(), s.vertexCount, denBytes().size());
    CHECK(s.triangles() > 1000 && s.triangles() <= 8000 - 3000 - 2 * 1200 - 2 * 300);
    for (const char* name : {"floor", "walls", "sky", "nest", "bed_1", "bed_2", "egg_nest", "nook_moss",
                             "embers", "hoard", "hearth_stones", "shelves", "sunbeam", "flames"})
        CHECK(s.find(name) != nullptr);
    bool additiveSeen = false, orderOk = true;
    for (const StaticPart& p : s.parts) {
        if (p.flags & kStaticAdditive) additiveSeen = true;
        else if (additiveSeen) orderOk = false;
    }
    CHECK(orderOk);  // opaque first: one draw for the room, one after the dragons for the glow
    CHECK(s.find("sunbeam") && (s.find("sunbeam")->flags & kStaticAdditive));
    CHECK(s.find("flames") && s.find("flames")->flags == (kStaticAdditive | kStaticFlicker));
    CHECK(s.find("walls") && s.find("walls")->flags == 0);

    // Damaged files are refused.
    StaticScene bad;
    CHECK(!loadStaticScene(denBytes().data(), 10, bad));
    CHECK(!loadStaticScene(denBytes().data(), denBytes().size() - 2, bad));
    std::vector<u8> broken = denBytes();
    broken[broken.size() - 1] = 0xFF;  // the last index points past its part
    broken[broken.size() - 2] = 0xFF;
    CHECK(!loadStaticScene(broken.data(), broken.size(), bad));
    broken = denBytes();
    broken[0] = 'X';
    CHECK(!loadStaticScene(broken.data(), broken.size(), bad));
}

// The den's bought things (WP7) fit the frame with the room and a full den: every toy out
// and the biggest piece of decor in each spot (a bought rug hides the room's own).
TEST(den_things_fit_the_frame_budget) {
    int toys = bowlFoodMesh().triangles();
    for (int k = 0; k < kToys; ++k) {
        const PropMesh m = toyMesh(k);
        CHECK(m.triangles() > 0 && m.pos.size() == m.nrm.size() && m.paint.size() == m.pos.size() * 4);
        for (u16 i : m.idx) CHECK(i < m.pos.size());
        toys += m.triangles();
    }
    int decor[kDecorSpots] = {}, cheapestRug = 1 << 30;
    for (int i = 0; i < kItems; ++i) {
        const Item it = static_cast<Item>(i);
        const int spot = decorSpot(itemInfo(it).kind);
        if (spot < 0) continue;
        const PropMesh m = decorMesh(it);
        CHECK(m.triangles() > 0 && m.paint.size() == m.pos.size() * 4);
        for (u16 v : m.idx) CHECK(v < m.pos.size());
        for (std::size_t v = 0; v < m.paint.size(); v += 4) CHECK(m.paint[v] < 4);  // four colours each
        decor[spot] = std::max(decor[spot], m.triangles());
        if (spot == 0) cheapestRug = std::min(cheapestRug, m.triangles());
    }
    const StaticScene& s = den();
    int decorSum = 0;
    for (int n : decor) decorSum += n;
    const int homeRug = homeRugMesh().triangles();
    std::printf("  den things: toys %d, decor %d (rug %d, lantern %d, perch %d, plant %d, banner %d)\n", toys,
                decorSum, decor[0], decor[1], decor[2], decor[3], decor[4]);
    CHECK(s.find("rug") == nullptr && homeRug > 0 && homeRug <= cheapestRug);  // the rug is a prop
    // The room, every toy out and the biggest decor in every spot fit the room's share.
    CHECK(s.triangles() + toys + decorSum <= 8000 - 3000 - 2 * 1200 - 2 * 300);
    const DecorPlace rugAt = decorPlace(0);
    const DenLayout lay;
    CHECK(std::fabs(rugAt.at.x - lay.home.x) < 0.01f && std::fabs(rugAt.at.y - lay.home.y) < 0.01f);
}

TEST(den_room_matches_the_behavior_layout) {
    const StaticScene& s = den();
    const DenLayout lay;
    const struct { const char* part; Vec2 spot; } spots[] = {
        {"nest", lay.beds[0]}, {"bed_1", lay.beds[1]},  {"bed_2", lay.beds[2]}, {"nook_moss", lay.sulkSpots[0]},
        {"egg_nest", lay.eggNests[0]}, {"egg_nest_2", lay.eggNests[1]}, {"embers", lay.hearth}, {"hoard", lay.hoard},
    };
    for (const auto& sp : spots) {
        const StaticPart* p = s.find(sp.part);
        CHECK(p && dist2d(centreOf(*p), sp.spot) < 0.3f);
    }
    if (const StaticPart* sky = s.find("sky")) CHECK(length(centreOf(*sky) - lay.skylight) < 0.3f);

    // Walkable spots are on the floor and clear of the obstacles.
    DenBehavior probe;
    probe.den = lay;
    std::vector<Vec2> places{lay.home};
    for (int i = 0; i < DenLayout::kSpots; ++i) places.insert(places.end(), {lay.beds[i], lay.sulkSpots[i]});
    for (Vec2 p : places) {
        CHECK(std::hypot(p.x - lay.home.x, p.y - lay.home.y) <= lay.radius);
        CHECK(probe.clearAt(p, 0.8f));
    }
    // Room for three curled-up adults: beds and sulking spots keep apart.
    for (std::size_t i = 1; i < places.size(); ++i)
        for (std::size_t j = i + 1; j < places.size(); ++j)
            CHECK(std::hypot(places[i].x - places[j].x, places[i].y - places[j].y) > 2.6f);
    // The walls leave room for an adult's head at the edge of the walkable circle (the nose is
    // ~2.3 ahead of its origin), and the floor runs on well past them into the dark.
    const float reach = lay.radius + std::hypot(lay.home.x, lay.home.y) + 2.3f;
    float wallMin = 1e9f, floorMax = 0;
    if (const StaticPart* w = s.find("walls"))
        for (int v = w->firstVertex; v < w->firstVertex + w->vertexCount; ++v)
            if (s.pos[v].z > 0 && s.pos[v].z < 3) wallMin = std::fmin(wallMin, std::hypot(s.pos[v].x, s.pos[v].y));
    if (const StaticPart* f = s.find("floor"))
        for (int v = f->firstVertex; v < f->firstVertex + f->vertexCount; ++v)
            floorMax = std::fmax(floorMax, std::hypot(s.pos[v].x, s.pos[v].y));
    std::printf("  walls from %.2f (reach %.2f), floor to %.1f\n", wallMin, reach, floorMax);
    CHECK(wallMin > reach);
    CHECK(floorMax > 12.0f);

    // The obstacles cover the solid props they stand for.
    const struct { const char* part; int obstacle; } solid[] = {{"hearth_stones", 0}, {"egg_nest", 1}, {"hoard", 2}, {"egg_nest_2", 3}};
    for (const auto& sd : solid) {
        const StaticPart* p = s.find(sd.part);
        const DenObstacle& o = lay.obstacles[sd.obstacle];
        float far = 0;
        for (int v = p ? p->firstVertex : 0; p && v < p->firstVertex + p->vertexCount; ++v)
            far = std::fmax(far, dist2d(s.pos[v], o.at));
        CHECK(p && far <= o.radius + 0.1f);
    }

    // A sulking dragon faces the nook's rocks (+Y) without its head going into them.
    const Vec2 nose{lay.sulkSpots[0].x, lay.sulkSpots[0].y + 2.3f};
    float clearance = 1e9f;
    if (const StaticPart* r = s.find("nook_rocks"))
        for (int v = r->firstVertex; v < r->firstVertex + r->vertexCount; ++v)
            if (s.pos[v].z < 2.5f) clearance = std::fmin(clearance, dist2d(s.pos[v], nose));
    std::printf("  sulking nose to the nook rocks: %.2f\n", clearance);
    CHECK(clearance > 0.3f);

    // Props don't clip into each other: the shelves and jars stand clear of the rocks.
    float gap = 1e9f;
    for (const char* a : {"shelves", "jars"})
        for (const char* b : {"nook_rocks", "boulders", "hoard"}) {
            const StaticPart* pa = s.find(a);
            const StaticPart* pb = s.find(b);
            if (!pa || !pb) continue;
            for (int u = pa->firstVertex; u < pa->firstVertex + pa->vertexCount; ++u)
                for (int v = pb->firstVertex; v < pb->firstVertex + pb->vertexCount; ++v)
                    gap = std::fmin(gap, length(s.pos[u] - s.pos[v]));
        }
    std::printf("  shelves to the nearest rock: %.2f\n", gap);
    CHECK(gap > 0.3f);
}

TEST(den_lighting_sets_read_right) {
    const StaticScene& s = den();
    const StaticPart* floor = s.find("floor");
    const StaticPart* sky = s.find("sky");
    if (!floor || !sky) return;
    const float day = partLuminance(*floor, kLightDay), eve = partLuminance(*floor, kLightEvening),
                night = partLuminance(*floor, kLightNight);
    std::printf("  floor luminance: day %.0f, evening %.0f, night %.0f\n", day, eve, night);
    CHECK(day > eve && eve > night);
    CHECK(night >= 30);  // dark, but still readable on the 3DS screen
    for (int set = 0; set < kLightSets; ++set) CHECK(luminance(&s.backdrop[set * 4]) < partLuminance(*floor, set));
    // Blue sky by day, a sunset in the evening.
    const u8* skyDay = s.colors(kLightDay) + sky->firstVertex * 4;
    const u8* skyEve = s.colors(kLightEvening) + sky->firstVertex * 4;
    CHECK(skyDay[2] > skyDay[0] && skyEve[0] > skyEve[2]);
    // The floor under the skylight is brighter than in the sulk nook.
    const DenLayout lay;
    float sun[3], nook[3];
    CHECK(s.lightNear("floor", lay.sunSpot, 2.2f, kLightDay, sun));
    CHECK(s.lightNear("floor", lay.sulkSpots[0], 2.2f, kLightDay, nook));
    CHECK(sun[0] + sun[1] + sun[2] > nook[0] + nook[1] + nook[2]);
    CHECK(!s.lightNear("floor", {40, 40}, 1.0f, kLightDay, sun));
}

TEST(daylight_blends_smoothly_through_the_day) {
    CHECK(dayBlend(kMidnight + 12 * kHour).weight(kLightDay) == 1.0f);
    CHECK(dayBlend(kMidnight + 1 * kHour).weight(kLightNight) == 1.0f);
    const DayBlend dusk = dayBlend(kMidnight + 18 * kHour + 15 * 60);
    CHECK(dusk.weight(kLightDay) > 0.1f && dusk.weight(kLightEvening) > 0.1f);
    const DayBlend dawn = dayBlend(kMidnight + 6 * kHour);
    CHECK(dawn.weight(kLightNight) > 0.1f && dawn.weight(kLightEvening) > 0.1f);
    float last[kLightSets] = {0, 0, 1};
    float worst = 0;
    for (int minute = 0; minute <= 24 * 60; ++minute) {
        const s64 t = kMidnight + minute * 60;
        const DayBlend b = dayBlend(t);
        float sum = 0;
        for (int s = 0; s < kLightSets; ++s) {
            const float w = b.weight(s);
            sum += w;
            worst = std::fmax(worst, std::fabs(w - last[s]));
            last[s] = w;
        }
        CHECK(std::fabs(sum - 1.0f) < 1e-5f);
        if (isNight(t)) CHECK(b.weight(kLightDay) < 0.2f);  // dragons sleep in the dark
    }
    CHECK(worst < 0.03f);  // no visible jumps from minute to minute
    // Midday light on the dragons is the look they were designed in.
    const DragonLight noon = dragonLight(dayBlend(kMidnight + 12 * kHour));
    CHECK(std::fabs(noon.ambient[0] - 0.42f) < 1e-5f && std::fabs(noon.key[1] - 0.62f) < 1e-5f);
    const DragonLight night = dragonLight(dayBlend(kMidnight + 2 * kHour));
    CHECK(night.key[2] > night.key[0]);  // moonlight is blue
}

TEST(particles_live_fall_and_die) {
    Particles fx;
    fx.emit(Fx::Ember, {7.9f, 0.8f, 0}, 10);
    CHECK(fx.count() == 10);
    const float z0 = fx[0].pos.z;
    for (int i = 0; i < 15; ++i) fx.update(1.0f / 30);
    CHECK(fx.count() == 10 && fx[0].pos.z > z0);  // embers rise
    for (int i = 0; i < 90; ++i) fx.update(1.0f / 30);
    CHECK(fx.count() == 0);  // and burn out

    fx.emit(Fx::Crumb, {0, 0, 1.0f}, 20, 0.5f);
    bool above = true;
    for (int i = 0; i < 45; ++i) {
        fx.update(1.0f / 30);
        for (int k = 0; k < fx.count(); ++k) {
            above = above && fx[k].pos.z >= 0;
            CHECK(fx[k].alpha() >= 0 && fx[k].alpha() <= 1 && fx[k].sizeNow() >= 0);
        }
    }
    CHECK(above);  // crumbs land on the floor, never through it

    // A full pool of ambient particles still makes room for care effects.
    fx.clear();
    fx.emit(Fx::Ember, {0, 0, 0}, 200);
    CHECK(fx.count() == Particles::kMax);
    fx.emit(Fx::Heart, {0, 0, 2}, 5);
    int hearts = 0;
    for (int k = 0; k < fx.count(); ++k) hearts += fx[k].kind == Fx::Heart;
    CHECK(fx.count() == Particles::kMax && hearts == 5);
    CHECK(Particles::foreground(Fx::Heart) && !Particles::foreground(Fx::Ember));

    // The den's own ambience: embers always, motes only while the sun shines.
    const DenLayout lay;
    for (float daylight : {1.0f, 0.0f}) {
        Particles amb;
        DenAmbience den;
        int motes = 0, embers = 0;
        for (int i = 0; i < 300; ++i) {
            den.update(amb, lay, daylight, 1.0f / 30);
            amb.update(1.0f / 30);
        }
        for (int k = 0; k < amb.count(); ++k) {
            motes += amb[k].kind == Fx::Mote;
            embers += amb[k].kind == Fx::Ember;
        }
        CHECK(embers > 3);
        CHECK(daylight > 0 ? motes > 3 : motes == 0);
    }
}

}  // namespace

void runDenTests() {
    RUN(den_file_loads_and_fits_the_frame_budget);
    RUN(den_things_fit_the_frame_budget);
    RUN(den_room_matches_the_behavior_layout);
    RUN(den_lighting_sets_read_right);
    RUN(daylight_blends_smoothly_through_the_day);
    RUN(particles_live_fall_and_die);
}
