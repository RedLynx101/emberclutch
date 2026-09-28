#include "core/finds.hpp"

#include <cmath>

#include "core/accessories.hpp"
#include "core/care.hpp"
#include "core/den_roster.hpp"
#include "core/trainer.hpp"
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

namespace {

// The day's finds' spots: open, dry, gentle ground within the valley's floor, away from the places'
// middles, fixed by the day and the save's seed (cached: the same valley asks every frame).
struct DailySpots {
    const Valley* valley = nullptr;
    s32 day = 0;
    u32 seed = 0;
    Vec3 at[kDailyFinds];
};

const DailySpots& dailySpots(const Valley& v, const SaveData& s) {
    static DailySpots cache;
    const s32 day = s.progress.findsDay;
    if (cache.valley == &v && cache.day == day && cache.seed == s.progress.findsSeed) return cache;
    cache.valley = &v;
    cache.day = day;
    cache.seed = s.progress.findsSeed;
    Rng rng((static_cast<std::uint64_t>(static_cast<u32>(day)) << 32 | s.progress.findsSeed) * 0x9E3779B97F4A7C15ull + 7);
    const float half = v.size() * 0.5f, cx = v.x0 + half, cy = v.y0 + half;
    for (int k = 0; k < kDailyFinds; ++k) {
        Vec3 pick{cx, cy, v.heightAt(cx, cy) + 0.6f};
        for (int tries = 0; tries < 40; ++tries) {
            const float a = rng.below(36000) * (6.2831853f / 36000.0f), r = half * 0.62f * std::sqrt(rng.below(10000) / 10000.0f);
            const float x = cx + std::cos(a) * r, y = cy + std::sin(a) * r, h = v.heightAt(x, y);
            if (h < v.water + 0.6f || v.normalAt(x, y).z < 0.9f) continue;
            bool clear = true;
            for (const ValleyPlaceInfo& p : v.places) clear &= std::hypot(x - p.at.x, y - p.at.y) > 22.0f;
            if (!clear) continue;
            pick = {x, y, h + 0.6f};
            break;
        }
        cache.at[k] = pick;
    }
    return cache;
}

}  // namespace

Vec3 findAt(const Valley& v, const SaveData& s, int i) {
    if (i >= kFindSpots && i < kAllFinds) return dailySpots(v, s).at[i - kFindSpots];
    return findAt(v, i);
}

bool renewFinds(SaveData& s, s32 today) {
    if (s.progress.findsDay == today) return false;
    s.progress.findsDay = today;
    if (s.progress.findsSeed == 0) s.progress.findsSeed = s.nextId * 2654435761u + 12345u;  // (the save's own)
    s.world.finds &= (1u << kFindSpots) - 1;  // the day's finds back; the treasures stay found
    return true;
}

bool findDone(const SaveData& s, int i) { return i >= 0 && i < kAllFinds && ((s.world.finds >> i) & 1u); }

int findNear(const SaveData& s, const Valley& v, Vec3 at, bool flying) {
    const float reach = flying ? 9.0f : 4.0f;
    for (int i = 0; i < kAllFinds; ++i) {
        if (findDone(s, i)) continue;
        if (i >= kFindSpots && flying) continue;  // (the day's little finds are for walking to)
        const Vec3 p = findAt(v, s, i);
        if (length(p - at) < reach) return i;
    }
    return -1;
}

FindReward takeFind(SaveData& s, int i, s64 now, Rng& rng) {
    FindReward r;
    if (i >= kFindSpots && i < kAllFinds && !findDone(s, i)) {  // a day's little find
        s.world.finds |= 1u << i;
        const u32 roll = rng.below(100);
        if (roll < 55) {
            r.gleam = static_cast<u16>(8 + rng.below(18));
            s.gleam += r.gleam;
        } else if (roll < 85) {  // a treat: bread, a drumstick, candy or a cookie
            r.food = static_cast<s8>(static_cast<int>(Food::HearthBread) + static_cast<int>(rng.below(4)));
            if (s.pouch[r.food] < 999) ++s.pouch[r.food];
        } else {
            r.trinket = static_cast<s8>(rng.below(kTrinkets));
            if (s.hoard[r.trinket] < 60000) ++s.hoard[r.trinket];
        }
        return r;
    }
    if (i < 0 || i >= kFindSpots || findDone(s, i)) return r;
    const FindSpot& f = kSpots[i];
    s.world.finds |= 1u << i;
    int treasures = 0;  // (found so far, this one too)
    for (int k = 0; k < kFindSpots; ++k) treasures += (s.world.finds >> k) & 1u;
    if (treasures == 3 || treasures == 7 || treasures == 12) {
        r.accessory = acc::unownedFrom(s, WearSource::Find, static_cast<u32>(i * 7 + treasures));
        if (r.accessory >= 0) trainer::giveAccessory(s, r.accessory);
    }
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
