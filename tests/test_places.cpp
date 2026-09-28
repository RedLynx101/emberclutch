// The valley's places (Beta WP4; 1.0, v1-work A): every place's model (romfs/valley/places/*.esm,
// tools/blender/valley_places.py) loads with the parts the renderer knows and fits its triangle
// budget; 1.0's named anchors (core/place_layout placeAnchor) are all there, where they belong,
// and the spots people stand on are clear of the places' walls.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/daylight.hpp"
#include "core/place_layout.hpp"
#include "core/static_mesh.hpp"
#include "core/valley.hpp"

using namespace ec;

namespace {

std::vector<u8> fileBytes(const std::string& path) {
    std::vector<u8> data;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return data;
    std::fseek(f, 0, SEEK_END);
    data.resize(static_cast<std::size_t>(std::ftell(f)));
    std::fseek(f, 0, SEEK_SET);
    if (std::fread(data.data(), 1, data.size(), f) != data.size()) data.clear();
    std::fclose(f);
    return data;
}

// ValleyPlace order, as the renderer's file names.
const char* const kIds[kPlaceCount] = {"den",    "market", "stone",   "sanctuary", "vault", "trailhead",
                                       "arena",  "lake",   "keeper",  "isles",     "orchard", "mill",
                                       "grotto", "ruins",  "caldera", "glade",     "cove",  "hollow"};

// Inside one of the place's walls (grown by `margin`, a body's radius)?
bool inWall(int place, Vec3 p, float margin) {
    for (const Solid& s : placeLayout(place).solids)
        if (std::hypot(p.x - s.at.x, p.y - s.at.y) < s.radius + margin) return true;
    return false;
}

float dist2d(Vec3 a, Vec3 b) { return std::hypot(a.x - b.x, a.y - b.y); }

}  // namespace

TEST(every_place_loads_within_its_budget) {
    std::printf("  triangles:");
    for (int p = 0; p < kPlaceCount; ++p) {
        const std::vector<u8> bytes = fileBytes(std::string("../romfs/valley/places/") + kIds[p] + ".esm");
        StaticScene s;
        const bool ok = !bytes.empty() && loadStaticScene(bytes.data(), bytes.size(), s);
        CHECK(ok);
        if (!ok) {
            std::printf("\n  FAIL: %s.esm missing or unreadable\n", kIds[p]);
            continue;
        }
        std::printf(" %s %d", kIds[p], s.triangles());
        CHECK(s.sets == kLightSets && s.parts.size() <= 8 && s.find("solid") != nullptr);  // (render3d kMaxParts)
        CHECK(s.triangles() <= (p == kPlaceMarket ? 2500 : 1500));
        for (const StaticPart& part : s.parts) {
            const bool glow = std::strcmp(part.name, "glow") == 0, light = std::strcmp(part.name, "lantern_light") == 0;
            CHECK(glow || light || std::strcmp(part.name, "solid") == 0 || std::strcmp(part.name, "lantern") == 0 ||
                  std::strcmp(part.name, "sails") == 0);
            CHECK(((part.flags & kStaticAdditive) != 0) == (glow || light));
        }
        // All of it near its anchor (the renderer's reach and culling box come from it).
        float reach = 0;
        for (const Vec3& v : s.pos) reach = std::fmax(reach, std::fmax(std::fabs(v.x), std::fabs(v.y)));
        CHECK(reach < 60.0f);
    }
    std::printf("\n");
}

// 1.0's places each export the spots their features need (v1-work A).
TEST(the_new_places_name_their_spots) {
    struct Want {
        int place;
        const char* name;
        int count;
    };
    const Want wants[] = {{kPlaceCaldera, "ring", 1},     {kPlaceCaldera, "sides", 2},    {kPlaceCaldera, "board", 1},
                          {kPlaceGlade, "stage", 1},      {kPlaceGlade, "rivals", 4},     {kPlaceGlade, "judges", 1},
                          {kPlaceGlade, "stalls", 2},     {kPlaceGlade, "board", 1},      {kPlaceCove, "fish_spot", 1},
                          {kPlaceCove, "jetty", 1},       {kPlaceCove, "fisher", 1},      {kPlaceCove, "shells", 5},
                          {kPlaceHollow, "arena", 1},     {kPlaceHollow, "wild_door", 1}, {kPlaceHollow, "keeper", 1}};
    for (const Want& w : wants) {
        const PlaceAnchor a = placeAnchor(w.place, w.name);
        CHECK(a && a.count == w.count);
        for (int i = 0; i < a.count; ++i) CHECK(std::hypot(a.at(i).x, a.at(i).y) < 60.0f);
    }
    // Names are per place, and a missing one is empty (its point the origin, never a crash).
    CHECK(!placeAnchor(kPlaceCaldera, "stage") && !placeAnchor(kPlaceDen, "ring") && !placeAnchor(99, "ring"));
    CHECK(!placeAnchor(kPlaceGlade, "Stage") && !placeAnchor(kPlaceGlade, ""));
    const PlaceAnchor none = placeAnchor(kPlaceCove, "treasure");
    CHECK(none.count == 0 && none.at(0).x == 0 && none.at(3).y == 0);
    // at() stays in range.
    const PlaceAnchor shells = placeAnchor(kPlaceCove, "shells");
    CHECK(shells.at(-1).x == shells.at(0).x && shells.at(9).x == shells.at(4).x);
}

TEST(the_new_places_spots_are_where_they_belong) {
    // The caldera: the ring at the crater's middle a low step up, the trainers either side of it
    // (about its width apart, on the floor), the board by the way in (+Y).
    const Vec3 ring = placeAnchor(kPlaceCaldera, "ring").at(0);
    CHECK(std::hypot(ring.x, ring.y) < 3.0f && ring.z > 0.1f && ring.z < 0.5f);
    const PlaceAnchor sides = placeAnchor(kPlaceCaldera, "sides");
    const float apart = dist2d(sides.at(0), sides.at(1));
    CHECK(apart > 12.0f && apart < 22.0f);
    CHECK(std::fabs(dist2d(sides.at(0), ring) - dist2d(sides.at(1), ring)) < 0.5f);
    CHECK(placeAnchor(kPlaceCaldera, "board").at(0).y > 12.0f);
    // The glade: four rivals on the stage (within its boards), the judges beside it, the stalls
    // facing each other across the way in, the board toward the path.
    const Vec3 stage = placeAnchor(kPlaceGlade, "stage").at(0);
    CHECK(stage.z > 0.3f && stage.z < 1.0f);
    const PlaceAnchor rivals = placeAnchor(kPlaceGlade, "rivals");
    for (int i = 0; i < 4; ++i) CHECK(dist2d(rivals.at(i), stage) < 4.0f);
    for (int i = 0; i < 3; ++i) CHECK(dist2d(rivals.at(i), rivals.at(i + 1)) > 1.5f);
    const Vec3 judges = placeAnchor(kPlaceGlade, "judges").at(0);
    CHECK(dist2d(judges, stage) > 5.0f && dist2d(judges, stage) < 12.0f);
    const PlaceAnchor stalls = placeAnchor(kPlaceGlade, "stalls");
    CHECK(stalls.at(0).x < 0 && stalls.at(1).x > 0);
    for (int i = 0; i < 2; ++i) {  // each faces the way in's middle (x = 0)
        const float facing = stalls.at(i).z;
        const float fx = -std::sin(facing);  // (the place's front turned counter-clockwise by it)
        CHECK(fx * -stalls.at(i).x > 0.9f * std::fabs(stalls.at(i).x));
    }
    CHECK(placeAnchor(kPlaceGlade, "board").at(0).y > 10.0f);
    // The cove: the jetty's end far out over the water, its deck above the water; the shells
    // along the water's edge; the fisher by the shack, up the beach.
    const Vec3 fish = placeAnchor(kPlaceCove, "fish_spot").at(0);
    CHECK(fish.y > 40.0f && std::fabs(fish.x) < 2.0f);
    const Vec3 jetty = placeAnchor(kPlaceCove, "jetty").at(0);
    CHECK(jetty.y > 25.0f && jetty.y < fish.y - 15.0f);
    const PlaceAnchor shells = placeAnchor(kPlaceCove, "shells");
    for (int i = 0; i < 5; ++i) CHECK(shells.at(i).y > 30.0f && shells.at(i).y < fish.y);
    CHECK(placeAnchor(kPlaceCove, "fisher").at(0).y < jetty.y);
    // The hollow: the bout in the bowl's middle, the wild ones' door at the back (-Y), the keeper
    // toward the way out (+Y).
    const Vec3 arena = placeAnchor(kPlaceHollow, "arena").at(0);
    CHECK(std::hypot(arena.x, arena.y) < 3.0f);
    CHECK(placeAnchor(kPlaceHollow, "wild_door").at(0).y < -6.0f && placeAnchor(kPlaceHollow, "keeper").at(0).y > 3.0f);
    // Where people stand is clear of the places' walls (so a walker gets there).
    const struct {
        int place;
        const char* name;
    } standing[] = {{kPlaceCaldera, "sides"}, {kPlaceGlade, "stalls"}, {kPlaceCove, "jetty"}, {kPlaceCove, "fisher"},
                    {kPlaceCove, "shells"},   {kPlaceHollow, "arena"}, {kPlaceHollow, "wild_door"},
                    {kPlaceHollow, "keeper"}};
    for (const auto& s : standing) {
        const PlaceAnchor a = placeAnchor(s.place, s.name);
        for (int i = 0; i < a.count; ++i) {
            const bool clear = !inWall(s.place, a.at(i), 0.3f);
            CHECK(clear);
            if (!clear) std::printf("  FAIL: %s %s %d is inside a wall\n", kIds[s.place], s.name, i);
        }
    }
}

// Heights in a place's frame go on its anchor's height (the model's), not the ground under them:
// the jetty's end stands over the water, not on the lake's bed.
TEST(frame_heights_stay_in_the_frame) {
    ValleyPlaceInfo p{kPlaceCove, {320.0f, -508.0f, 9.45f}, 3.14159265f};
    const Vec3 local{0.2f, 52.1f, -0.545f};
    const Vec3 w = placeFrameToWorld(p, local);
    const Vec2 xy = placeToWorld(p, {local.x, local.y});
    CHECK(std::fabs(w.x - xy.x) < 1e-4f && std::fabs(w.y - xy.y) < 1e-4f && std::fabs(w.z - (9.45f - 0.545f)) < 1e-4f);
    CHECK(std::fabs(w.y - (-508.0f + 52.1f)) < 1e-3f);  // (the cove faces north: its front is +y)
}

void runPlaceTests() {
    RUN(every_place_loads_within_its_budget);
    RUN(the_new_places_name_their_spots);
    RUN(the_new_places_spots_are_where_they_belong);
    RUN(frame_heights_stay_in_the_frame);
}
