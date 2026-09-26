#include "core/finds.hpp"

#include <cmath>

#include "core/den_roster.hpp"
#include "core/genetics.hpp"
#include "core/kinds.hpp"
#include "core/wanderings.hpp"

namespace ec {
namespace {

// (tools/valley/make_valley.py: the islands, the den's plateau, the cold heights, the places.)
constexpr FindSpot kSpots[kFindSpots] = {
    {{120, 200}, 0, true, 150, -1, false},   // the biggest island (the isles' lantern): a good purse
    {{300, -330}, 1, true, 90, -1, false},   // over the lake
    {{-250, 480}, 2, true, 0, 4, false},     // a pearl
    {{480, 300}, 3, true, 0, -1, true},      // an egg, over the village's hills
    {{-150, -250}, 4, true, 0, 3, false},    // a crystal
    {{700, 120}, 5, true, 120, -1, false},   // over the orchard's east
    {{-700, 100}, -1, true, 0, 5, false},    // up on the den's plateau: a fossil
    {{100, 900}, -1, true, 0, -1, true},     // the cold heights' peak: an egg
    {{-560, 222}, -1, false, 40, -1, false}, // by the falls' pool
    {{505, -330}, -1, false, 0, 1, false},   // the lake's east shore: a coin
    {{-520, -560}, -1, false, 50, -1, false},  // deep in the meadow's flowers
    {{-40, 180}, -1, false, 0, 2, false},    // up the river from the mill: a feather
    {{620, 90}, -1, false, 45, -1, false},   // the orchard's far row
    {{640, 590}, -1, false, 0, 0, false},    // the Stone's hilltop: a pebble
    {{300, 760}, -1, false, 60, -1, false},  // the Vault's snowy shelf
    {{900, 640}, -1, true, 0, 3, false},     // the crag's steep shoulder (from the air): a crystal
    {{60, -915}, -1, false, 55, -1, false},  // up the valley's south gap
    {{740, -400}, -1, false, 0, 1, false},   // behind the arena: a coin
    {{-520, 300}, -1, false, 35, -1, false}, // the keeper's garden
    {{-120, -700}, -1, false, 0, 4, false},  // by the outlet's bank: a pearl
    {{-625, 192}, -1, false, 100, -1, false},  // the grotto behind the falls: its gold chest
    {{960, 700}, -1, true, 0, 3, false},     // Starwatch Ruins on the crag: a star-bright crystal
};

}  // namespace

const FindSpot& findSpot(int i) { return kSpots[i >= 0 && i < kFindSpots ? i : 0]; }

Vec3 findAt(const Valley& v, int i) {
    const FindSpot& f = findSpot(i);
    if (f.island >= 0 && f.island < static_cast<int>(v.islands.size())) {
        const ValleyIsland& is = v.islands[f.island];
        return {is.at.x, is.at.y, is.at.z + 0.6f};
    }
    return {f.at.x, f.at.y, std::fmax(v.heightAt(f.at.x, f.at.y), v.water) + 0.6f};
}

bool findDone(const SaveData& s, int i) { return i >= 0 && i < kFindSpots && ((s.world.finds >> i) & 1u); }

int findNear(const SaveData& s, const Valley& v, Vec3 at, bool flying) {
    const float reach = flying ? 9.0f : 4.0f;
    for (int i = 0; i < kFindSpots; ++i) {
        if (findDone(s, i)) continue;
        const Vec3 p = findAt(v, i);
        if (length(p - at) < reach) return i;
    }
    return -1;
}

FindReward takeFind(SaveData& s, int i, s64 now, Rng& rng) {
    FindReward r;
    if (i < 0 || i >= kFindSpots || findDone(s, i)) return r;
    const FindSpot& f = kSpots[i];
    s.world.finds |= 1u << i;
    if (f.gleam) {
        s.gleam += f.gleam;
        r.gleam = f.gleam;
    }
    if (f.trinket >= 0 && f.trinket < kTrinkets) {
        if (s.hoard[f.trinket] < 60000) ++s.hoard[f.trinket];
        r.trinket = f.trinket;
    }
    if (f.egg) {
        if (s.dragonCount < static_cast<int>(kMaxDragons)) {  // a kind by rarity, the harder to find likelier
            Dragon egg = makeEgg(s.nextId++, makePurebred(static_cast<Element>(rng.below(6)), rng), rollSex(rng), now);
            rollKind(egg, randomKind(rng, 3, 5, 2), rollVariant(rng), rng);
            egg.origin = Origin::Wild;
            placeEgg(s, egg);
            s.dragons[s.dragonCount] = egg;
            r.egg = s.dragonCount++;
        } else {
            s.gleam += 150;  // no room for it: its worth instead
            r.gleam = 150;
        }
    }
    return r;
}

bool explore(SaveData& s, const Valley& v, Vec2 at, float radius) {
    const float cell = v.size() / kFogCells;
    if (cell <= 0) return false;
    bool any = false;
    const int r = static_cast<int>(radius / cell) + 1;
    const int cx = static_cast<int>((at.x - v.x0) / cell), cy = static_cast<int>((at.y - v.y0) / cell);
    for (int y = cy - r; y <= cy + r; ++y)
        for (int x = cx - r; x <= cx + r; ++x) {
            if (x < 0 || y < 0 || x >= kFogCells || y >= kFogCells) continue;
            const float mx = v.x0 + (x + 0.5f) * cell, my = v.y0 + (y + 0.5f) * cell;
            if (std::hypot(mx - at.x, my - at.y) > radius + cell * 0.5f) continue;
            const int bit = y * kFogCells + x;
            u8& b = s.world.explored[bit >> 3];
            if (!(b & (1u << (bit & 7)))) {
                b = static_cast<u8>(b | (1u << (bit & 7)));
                any = true;
            }
        }
    return any;
}

bool explored(const SaveData& s, int cx, int cy) {
    if (cx < 0 || cy < 0 || cx >= kFogCells || cy >= kFogCells) return false;
    const int bit = cy * kFogCells + cx;
    return (s.world.explored[bit >> 3] >> (bit & 7)) & 1u;
}

}  // namespace ec
