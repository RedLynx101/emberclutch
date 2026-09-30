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

bool takeLetter(SaveData& s) {
    if (s.world.flags & kFlagLoveLetter) return false;
    s.world.flags |= kFlagLoveLetter;
    s.gleam += kLetterGleam;
    return true;
}

namespace {

// A box on the picnic (its middle at `c`, half sizes, turned `yaw`), its faces shaded a little
// (the static program has no light: the top brightest, the sides darker).
void picnicBox(ValleyMesh& m, Vec3 c, Vec3 half, float yaw, const u8 rgb[3]) {
    const float cs = std::cos(yaw), sn = std::sin(yaw);
    auto at = [&](float x, float y, float z) {
        return Vec3{c.x + x * cs - y * sn, c.y + x * sn + y * cs, c.z + z};
    };
    const float sx = half.x, sy = half.y, sz = half.z;
    const Vec3 p[8] = {at(-sx, -sy, -sz), at(sx, -sy, -sz), at(sx, sy, -sz), at(-sx, sy, -sz),
                       at(-sx, -sy, sz),  at(sx, -sy, sz),  at(sx, sy, sz),  at(-sx, sy, sz)};
    struct Face { int a, b, c, d; float shade; };
    const Face faces[5] = {{4, 5, 6, 7, 1.0f}, {0, 1, 5, 4, 0.78f}, {1, 2, 6, 5, 0.86f}, {2, 3, 7, 6, 0.7f}, {3, 0, 4, 7, 0.82f}};
    for (const Face& f : faces) {
        const u8 r = static_cast<u8>(rgb[0] * f.shade), g = static_cast<u8>(rgb[1] * f.shade), b = static_cast<u8>(rgb[2] * f.shade);
        const u16 base = static_cast<u16>(m.pos.size());
        for (int k : {f.a, f.b, f.c, f.d}) {
            m.pos.push_back(p[k]);
            m.color.insert(m.color.end(), {r, g, b, 255});
        }
        m.idx.insert(m.idx.end(), {base, static_cast<u16>(base + 1), static_cast<u16>(base + 2), base, static_cast<u16>(base + 2),
                                   static_cast<u16>(base + 3)});
    }
}

// A flat quad on the picnic (corners given counter-clockwise from above).
void picnicQuad(ValleyMesh& m, Vec3 a, Vec3 b, Vec3 c, Vec3 d, const u8 rgb[3]) {
    const u16 base = static_cast<u16>(m.pos.size());
    for (const Vec3& p : {a, b, c, d}) {
        m.pos.push_back(p);
        m.color.insert(m.color.end(), {rgb[0], rgb[1], rgb[2], 255});
    }
    m.idx.insert(m.idx.end(), {base, static_cast<u16>(base + 1), static_cast<u16>(base + 2), base, static_cast<u16>(base + 2),
                               static_cast<u16>(base + 3)});
}

}  // namespace

void buildPicnic(const Valley& v, bool letter, ValleyMesh& out) {
    out.clear();
    constexpr float kYaw = 0.35f;  // (turned a little to the view down the valley)
    const float cs = std::cos(kYaw), sn = std::sin(kYaw);
    const float z0 = v.heightAt(kPicnicAt.x, kPicnicAt.y) + 0.03f;
    auto at = [&](float x, float y, float z) {
        return Vec3{kPicnicAt.x + x * cs - y * sn, kPicnicAt.y + x * sn + y * cs, z0 + z};
    };
    // The blanket: red and cream checks, 2.2 x 1.6 m, each square on the ground where it lies.
    constexpr int kCols = 6, kRows = 4;
    constexpr float kW = 2.2f, kH = 1.6f;
    const u8 red[3] = {206, 64, 72}, cream[3] = {246, 236, 214};
    for (int cx = 0; cx < kCols; ++cx)
        for (int cy = 0; cy < kRows; ++cy) {
            const float x0 = -kW / 2 + kW * cx / kCols, x1 = -kW / 2 + kW * (cx + 1) / kCols;
            const float y0 = -kH / 2 + kH * cy / kRows, y1 = -kH / 2 + kH * (cy + 1) / kRows;
            auto ground = [&](float x, float y) {
                const Vec3 p = at(x, y, 0);
                return Vec3{p.x, p.y, v.heightAt(p.x, p.y) + 0.03f};
            };
            picnicQuad(out, ground(x0, y0), ground(x1, y0), ground(x1, y1), ground(x0, y1), (cx + cy) % 2 ? red : cream);
        }
    // The basket at the back corner: wicker, its lid, and a handle arched over it.
    const u8 wicker[3] = {178, 126, 72}, lid[3] = {150, 100, 58}, white[3] = {250, 248, 242}, pink[3] = {242, 176, 190};
    const Vec3 basket = at(0.62f, 0.38f, 0.15f);
    picnicBox(out, basket, {0.28f, 0.19f, 0.14f}, kYaw, wicker);
    picnicBox(out, basket + Vec3{0, 0, 0.155f}, {0.3f, 0.21f, 0.02f}, kYaw, lid);
    for (int k = 0; k < 5; ++k) {  // the handle: five little pieces round an arch
        const float a0 = 3.14159f * k / 5.0f, a1 = 3.14159f * (k + 1) / 5.0f, am = (a0 + a1) * 0.5f;
        const Vec3 c = basket + Vec3{0, 0, 0.17f} + Vec3{-std::cos(am) * 0.22f * cs, -std::cos(am) * 0.22f * sn, std::sin(am) * 0.2f};
        picnicBox(out, c, {0.03f, 0.03f, 0.03f}, kYaw, lid);
    }
    // Two cups, and a little pink cake on a plate.
    picnicBox(out, at(-0.35f, 0.3f, 0.05f), {0.045f, 0.045f, 0.05f}, kYaw, white);
    picnicBox(out, at(-0.1f, 0.42f, 0.05f), {0.045f, 0.045f, 0.05f}, kYaw, white);
    picnicBox(out, at(-0.55f, -0.2f, 0.012f), {0.14f, 0.14f, 0.012f}, kYaw, white);
    picnicBox(out, at(-0.55f, -0.2f, 0.06f), {0.08f, 0.08f, 0.045f}, kYaw, pink);
    // The letter, on the blanket's front: a folded card sealed with a red heart.
    if (letter) {
        const Vec3 c = at(0.15f, -0.35f, 0.012f);
        picnicBox(out, c, {0.15f, 0.1f, 0.008f}, kYaw + 0.2f, white);
        const u8 seal[3] = {214, 40, 64};
        picnicBox(out, c + Vec3{0, 0, 0.01f}, {0.04f, 0.04f, 0.004f}, kYaw + 0.2f + 0.785f, seal);  // (its seal)
    }
}

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
