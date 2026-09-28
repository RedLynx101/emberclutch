#include "core/hollow.hpp"

#include <cmath>

#include "core/battle_spots.hpp"
#include "core/care.hpp"
#include "core/kinds.hpp"
#include "core/trainer.hpp"
#include "core/wanderings.hpp"

namespace ec::hollow {
namespace {

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
int clampFloor(int floor) { return floor < 1 ? 1 : (floor > kFloors ? kFloors : floor); }
int bandOf(int floor) { return (clampFloor(floor) - 1) / kBand; }  // 0 .. 5

// The claims' Hollow range (trainer kClaimHollow, 16 bits): a guardian's Gleam bonus, then its
// stat point, once a day each.
constexpr int kClaimGuardianGleam = kClaimHollow;
constexpr int kClaimGuardianPoint = kClaimHollow + kCheckpoints;
static_assert(kCheckpoints * 2 <= 16, "the Hollow's claims fit its range");

}  // namespace

int wildLevel(int floor) {
    floor = clampFloor(floor);
    const int level = static_cast<int>(2.0f + floor * 1.2f) + (guardian(floor) ? 2 : 0);
    return level > kMaxLevel ? kMaxLevel : level;
}

bool guardian(int floor) { return floor >= 1 && floor <= kFloors && floor % kBand == 0; }

int skillAt(int floor) {
    floor = clampFloor(floor);
    const int skill = floor / 8 + (guardian(floor) ? 1 : 0);  // 0 on floors 1-7, 3 from 24
    return skill > 3 ? 3 : skill;
}

Dragon wildOf(int floor, s32 day) {
    floor = clampFloor(floor);
    Rng rng(0xF505711Eull ^ (static_cast<u64>(static_cast<u32>(day)) * 2654435761ull) ^ (static_cast<u64>(floor) << 40));
    rng.next();
    // Its kind: commons near the top, the harder-to-find further down, rare ones deep; the cold's
    // own (Frost) twice as likely in the Hollow; crossbreeds count as harder to find.
    int weights[kMaxKinds] = {}, total = 0;
    for (int k = 0; k < kindCount() && k < kMaxKinds; ++k) {
        const KindInfo& ki = kindInfo(k);
        int w = 0;
        switch (ki.rarity) {
            case Rarity::Common: w = floor < 20 ? 12 - floor / 2 : 2; break;
            case Rarity::Uncommon: w = 3 + floor / 3; break;
            case Rarity::Rare: w = floor < 6 ? 0 : floor / 4; break;
        }
        for (int e = 0; e < ki.elementCount; ++e)
            if (ki.elements[e] == battle::kFrost) w *= 2;
        weights[k] = w;
        total += w;
    }
    int kind = 0;
    if (total > 0) {
        int pick = static_cast<int>(rng.below(static_cast<u32>(total)));
        while (kind < kindCount() - 1 && pick >= weights[kind]) pick -= weights[kind++];
    }
    // The rare colouring: 1 in 30 at the top, about one in five at the bottom; a guardian's likelier.
    const int rareIn1000 = 30 + floor * 6 + (guardian(floor) ? 250 : 0);
    const int variant = rng.below(1000) < static_cast<u32>(rareIn1000) ? kindInfo(kind).rareVariant
                                                                       : static_cast<int>(rng.below(kKindVariants - 1));
    Dragon d;
    d.id = 0xF0000000u + static_cast<u32>(floor);  // apart from the save's and the league's
    d.stage = Stage::Adult;
    rollKind(d, kind, variant, rng);
    d.genome.build = static_cast<u8>(rng.below(3));
    d.genome.size = static_cast<u8>(rng.below(256));
    d.xp = trainer::xpForLevel(wildLevel(floor));
    const int trained = floor / 4 + (guardian(floor) ? 2 : 0);  // the deep ones are hardened
    for (u8& t : d.trained) t = static_cast<u8>(trained);
    d.needs = Needs{};
    return d;
}

int checkpoints(const Dragon& d, int out[kCheckpoints]) {
    int n = 0;
    for (int b = 0; b < kCheckpoints; ++b) {
        const int start = b * kBand + 1;
        if (b == 0 || d.frostDeepest >= start - 1) out[n++] = start;
    }
    return n;
}

Reward record(SaveData& s, int dragonIndex, int floor, const Dragon& wild, battle::Outcome o, s32 today, Rng& rng) {
    Reward r;
    if (dragonIndex < 0 || dragonIndex >= s.dragonCount) return r;
    floor = clampFloor(floor);
    Dragon& d = s.dragons[dragonIndex];
    r.growth = battle::grow(d, battle::battleXp(trainer::levelOf(d), trainer::levelOf(wild), o, guardian(floor) ? 1.6f : 1.3f));
    if (o != battle::Outcome::Won) {
        trainer::recordWild(d, false, floor);
        return r;
    }
    trainer::recordWild(d, true, floor);
    trainer::count(s, kCountWild);
    r.newDeepest = floor > s.progress.hollowDeepest;
    if (r.newDeepest) s.progress.hollowDeepest = static_cast<u8>(floor);
    r.gleam = 2 + static_cast<u32>(floor) / 2;
    const int band = bandOf(floor);
    // The stat the wild one was strongest in.
    int best = 0;
    for (int st = 1; st < kDragonStats; ++st)
        if (trainer::statPoints(wild, st) > trainer::statPoints(wild, best)) best = st;
    if (guardian(floor)) {
        if (trainer::claimToday(s, kClaimGuardianGleam + band, today)) r.gleam += 10 * static_cast<u32>(band + 1);
        if (trainer::claimToday(s, kClaimGuardianPoint + band, today) && trainer::train(d, best)) r.trained = best;
        if (r.newDeepest) {  // the first time down past this guardian: a prize
            static constexpr Food kFoods[kCheckpoints] = {Food::Frostmelon, Food::Frostmelon, Food::GlimmerCookie,
                                                          Food::GlimmerCookie, Food::Starfruit, Food::Starfruit};
            static constexpr Trinket kTrinkets[kCheckpoints] = {Trinket::Pebble, Trinket::Crystal, Trinket::Crystal,
                                                                Trinket::Fossil, Trinket::Pearl, Trinket::Pearl};
            r.prizeFood = static_cast<u8>(kFoods[band]);
            r.prizeCount = 2;
            r.prizeTrinket = static_cast<u8>(kTrinkets[band]);
            u16& pouch = s.pouch[static_cast<int>(kFoods[band])];
            pouch = static_cast<u16>(pouch + r.prizeCount > 999 ? 999 : pouch + r.prizeCount);
            u16& hoard = s.hoard[static_cast<int>(kTrinkets[band])];
            if (hoard < 0xFFFF) ++hoard;
        }
    } else if (floor >= 3 && rng.chance(1, 8) && trainer::train(d, best)) {
        r.trained = best;
    }
    s.gleam += r.gleam;
    return r;
}

float depth(int floor) { return clampf((clampFloor(floor) - 1) / static_cast<float>(kFloors - 1), 0.0f, 1.0f); }

Rgb chill(Rgb c, int floor) {
    // Toward a deep, icy blue-grey, and darker: a third of the way at the bottom.
    const float k = depth(floor) * 0.55f;
    const Rgb cold{58, 82, 120};
    auto mix = [&](u8 a, u8 b) { return static_cast<u8>(a + (b - a) * k); };
    return {mix(c.r, cold.r), mix(c.g, cold.g), mix(c.b, cold.b)};
}

// The bowl (core/battle_spots): +Y the way out, the cave door at the back.
Vec2 arenaSpot() { return spots::kHollowArena; }
Vec2 wildDoor() { return spots::kHollowWildDoor; }
Vec2 keeperSpot() { return spots::kHollowKeeper; }
float keeperFacing() {  // toward the bowl's middle
    const Vec2 k = keeperSpot(), a = arenaSpot();
    return std::atan2(k.x - a.x, a.y - k.y);  // (0 faces the place's front, +Y)
}

}  // namespace ec::hollow
