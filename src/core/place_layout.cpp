#include "core/place_layout.hpp"

#include <cmath>
#include <cstring>

namespace ec {
namespace {

#include "core/places_data.inc"

std::vector<PlaceLayout> build() {
    std::vector<PlaceLayout> t(kPlaceCount);
    for (int k = 0; k < kPlaceCount; ++k) {
        const PlaceRow& r = kPlaceRows[k];
        PlaceLayout& p = t[k];
        p.flat = r.flat;
        p.hasDoor = r.hasDoor;
        p.door = {r.door[0], r.door[1]};
        p.hasLantern = r.hasLantern;
        p.lantern = {r.lantern[0], r.lantern[1], r.lantern[2]};
        for (int i = 0; i < r.solids; ++i) {
            const float* c = kPlaceSolids[r.firstSolid + i];
            p.solids.push_back({{c[0], c[1]}, c[2]});
        }
        p.waterZ = r.waterZ;
    }
    PlaceLayout& m = t[kPlaceMarket];
    m.eggStand = {kMarketEggStand[0], kMarketEggStand[1], kMarketEggStand[2]};
    for (int i = 0; i < 4; ++i) m.goods[i] = {kMarketGoods[i][0], kMarketGoods[i][1], kMarketGoods[i][2]};
    t[kPlaceMill].hub = {kMillHub[0], kMillHub[1], kMillHub[2]};
    t[kPlaceMill].hubAxis = {kMillAxis[0], kMillAxis[1], kMillAxis[2]};
    // Arriving: before its door, or out in front of it; the mill from its east bank (the river
    // runs through its front), the grotto from the falls' side (it opens toward its -Y).
    for (PlaceLayout& p : t) p.arrive = p.hasDoor ? Vec2{p.door.x, p.door.y + 5.0f} : Vec2{0, 12.0f};
    t[kPlaceMarket].arrive = {0.0f, -2.5f};  // the square's middle: the stalls ahead
    t[kPlaceMill].arrive = {13.0f, 6.0f};
    t[kPlaceGrotto].arrive = {0.0f, -11.0f};
    t[kPlaceLake].arrive = {4.0f, 9.0f};
    t[kPlaceCove].arrive = {0.0f, -2.0f};  // (behind its anchor: looking at it is looking out to the water)
    return t;
}

}  // namespace

const PlaceLayout& placeLayout(int place) {
    static const std::vector<PlaceLayout> kTable = build();
    return kTable[place >= 0 && place < kPlaceCount ? place : 0];
}

Vec3 PlaceAnchor::at(int i) const {
    if (count <= 0) return {0, 0, 0};
    const float* q = points[i < 0 ? 0 : (i >= count ? count - 1 : i)];
    return {q[0], q[1], q[2]};
}

PlaceAnchor placeAnchor(int place, const char* name) {
    for (const AnchorRow& r : kAnchorRows)
        if (r.place == place && std::strcmp(r.name, name) == 0) return {kAnchorPoints + r.first, r.count};
    return {};
}

Vec2 placeToWorld(const ValleyPlaceInfo& p, Vec2 local) {
    const Vec2 f{std::sin(p.heading), -std::cos(p.heading)};  // its front (+Y)
    const Vec2 r{f.y, -f.x};                                   // its right (+X)
    return {p.at.x + r.x * local.x + f.x * local.y, p.at.y + r.y * local.x + f.y * local.y};
}

Vec3 placeToWorld3(const Valley& v, const ValleyPlaceInfo& p, Vec3 local) {
    const Vec2 w = placeToWorld(p, {local.x, local.y});
    return {w.x, w.y, v.heightAt(w.x, w.y) + local.z};
}

Vec3 placeFrameToWorld(const ValleyPlaceInfo& p, Vec3 local) {
    const Vec2 w = placeToWorld(p, {local.x, local.y});
    return {w.x, w.y, p.at.z + local.z};
}

Vec3 placeArrival(const Valley& v, const ValleyPlaceInfo& p, bool outward) {
    const PlaceLayout& l = placeLayout(p.id);
    const Vec2 w = placeToWorld(p, outward && l.hasDoor ? Vec2{l.door.x, l.door.y + 11.0f} : l.arrive);
    return {w.x, w.y, v.groundAt(w.x, w.y, p.at.z + 2.0f)};
}

void addPlaceDecks(Valley& v) {
    v.decks.clear();
    // The mill's bridge (tools/blender/valley_places.py build_mill): across the river along its
    // local X from -9 to 9, 1.45 m either side of the middle, its top 0.12 m over the anchor at
    // the ends rising 1.45 m in the middle.
    if (const ValleyPlaceInfo* m = v.place(kPlaceMill)) {
        ValleyDeck d;
        d.a = placeToWorld(*m, {-9.0f, 0.0f});
        d.b = placeToWorld(*m, {9.0f, 0.0f});
        d.halfWidth = 1.45f;
        d.z0 = d.z1 = m->at.z + 0.12f;
        d.arch = 1.45f;
        v.decks.push_back(d);
    }
    // The cove's jetty (build_cove): along its local +Y from 33.5 m (0.3 m over the sand there)
    // sloping to the deck's level at 41 m (its fish_spot's height), level out to 53 m, 0.95 m wide
    // either side.
    if (const ValleyPlaceInfo* c = v.place(kPlaceCove)) {
        const PlaceAnchor spot = placeAnchor(kPlaceCove, "fish_spot");
        const float level = c->at.z + (spot ? spot.at(0).z : -0.4f);
        const Vec2 foot = placeToWorld(*c, {0.0f, 33.5f}), bend = placeToWorld(*c, {0.0f, 41.0f}),
                   end = placeToWorld(*c, {0.0f, 53.0f});
        ValleyDeck slope;
        slope.a = foot;
        slope.b = bend;
        slope.halfWidth = 0.95f;
        slope.z0 = v.heightAt(foot.x, foot.y) + 0.3f;
        slope.z1 = level;
        v.decks.push_back(slope);
        ValleyDeck out;
        out.a = bend;
        out.b = end;
        out.halfWidth = 0.95f;
        out.z0 = out.z1 = level;
        v.decks.push_back(out);
    }
    // The lake's jetty (build_lake; 1.0.1: 1.0's lay under the sand with the rest of the place): level, from the
    // shore out over the water between its "jetty" anchor's two ends, 1.1 m either side.
    if (const ValleyPlaceInfo* l = v.place(kPlaceLake)) {
        const PlaceAnchor ends = placeAnchor(kPlaceLake, "jetty");
        if (ends.count >= 2) {
            ValleyDeck d;
            d.a = placeToWorld(*l, {ends.at(0).x, ends.at(0).y});
            d.b = placeToWorld(*l, {ends.at(1).x, ends.at(1).y});
            d.halfWidth = 1.1f;
            d.z0 = d.z1 = l->at.z + ends.at(0).z;
            v.decks.push_back(d);
        }
    }
}

std::vector<CameraWall> cameraWalls(const Valley& v) {
    std::vector<CameraWall> out;
    if (const ValleyPlaceInfo* den = v.place(kPlaceDen)) {  // its arch stands out of the cliff at local y -1.9
        const Vec2 f{std::sin(den->heading), -std::cos(den->heading)};
        out.push_back({placeToWorld(*den, {0, 1.4f}), f, 13.0f, 10.0f});  // (clear of the moss on its top)
    }
    return out;
}

std::vector<Solid> worldSolids(const Valley& v) {
    std::vector<Solid> out;
    for (const ValleyPlaceInfo& p : v.places)
        for (const Solid& s : placeLayout(p.id).solids) out.push_back({placeToWorld(p, s.at), s.radius});
    return out;
}

}  // namespace ec
