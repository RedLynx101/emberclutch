#include "core/place_layout.hpp"

#include <cmath>

namespace ec {
namespace {

PlaceLayout make(bool door, Vec2 d, bool lantern, Vec3 l, std::vector<Solid> solids) {
    PlaceLayout p;
    p.hasDoor = door;
    p.door = d;
    p.hasLantern = lantern;
    p.lantern = l;
    p.solids = std::move(solids);
    return p;
}

std::vector<PlaceLayout> defaults() {
    std::vector<PlaceLayout> t(kPlaceCount);
    t[kPlaceDen] = make(true, {0, 4}, true, {6, 7, 2.2f}, {});
    t[kPlaceMarket] = make(true, {0, 0}, true, {9, 9, 2.4f},
                           {{{-18, 10}, 4.0f}, {{18, 10}, 4.0f}, {{-15, -16}, 4.0f}, {{15, -16}, 4.0f}, {{0, 20}, 4.0f}});
    t[kPlaceMarket].eggStand = {0, -4, 1.0f};
    for (int k = 0; k < 4; ++k) t[kPlaceMarket].goods[k] = {-3.0f + 2.0f * k, 5, 1.0f};
    t[kPlaceStone] = make(true, {0, 3.5f}, true, {5, -3, 2.2f}, {{{0, 0}, 2.5f}});
    t[kPlaceSanctuary] = make(true, {0, 6}, true, {7, 2, 2.2f}, {{{-6, 0}, 3.5f}, {{6, -2}, 3.5f}});
    t[kPlaceVault] = make(true, {0, 4}, true, {5, 5, 2.2f}, {});
    t[kPlaceTrailhead] = make(true, {0, 3}, true, {4, 4, 2.2f}, {{{-5, -2}, 1.5f}});
    t[kPlaceArena] = make(true, {0, 26}, true, {0, -8, 4.0f}, {});
    t[kPlaceLake] = make(false, {}, false, {}, {});
    t[kPlaceKeeper] = make(true, {0, 5}, false, {}, {{{0, -1}, 4.0f}});
    t[kPlaceIsles] = make(false, {}, true, {0, 0, 2.2f}, {});
    t[kPlaceOrchard] = make(true, {0, 3}, false, {}, {});
    t[kPlaceMill] = make(false, {}, false, {}, {{{9, 4}, 4.0f}});
    t[kPlaceMill].hub = {9, 4, 12};
    t[kPlaceGrotto] = make(true, {0, 3}, false, {}, {});
    t[kPlaceRuins] = make(false, {}, false, {}, {{{0, 0}, 5.0f}});
    return t;
}

}  // namespace

const PlaceLayout& placeLayout(int place) {
    static const std::vector<PlaceLayout> kTable = defaults();
    return kTable[place >= 0 && place < kPlaceCount ? place : 0];
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

std::vector<Solid> worldSolids(const Valley& v) {
    std::vector<Solid> out;
    for (const ValleyPlaceInfo& p : v.places)
        for (const Solid& s : placeLayout(p.id).solids) out.push_back({placeToWorld(p, s.at), s.radius});
    return out;
}

}  // namespace ec
