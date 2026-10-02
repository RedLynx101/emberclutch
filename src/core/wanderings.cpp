#include "core/wanderings.hpp"

#include <cmath>

#include "core/den_roster.hpp"
#include "core/genetics.hpp"
#include "core/kinds.hpp"

namespace ec {
namespace {

float unit(Rng& rng) { return rng.next() * (1.0f / 4294967296.0f); }

}  // namespace

const char* trinketName(Trinket t) {
    switch (t) {
        case Trinket::Pebble: return "Shiny pebble";
        case Trinket::Coin: return "Old coin";
        case Trinket::Feather: return "Feather";
        case Trinket::Crystal: return "Crystal shard";
        case Trinket::Pearl: return "Pearl";
        case Trinket::Fossil: return "Fossil tooth";
        default: return "?";
    }
}

u32 trinketValue(Trinket t) {
    static constexpr u32 kValue[kTrinkets] = {5, 12, 8, 25, 40, 60};
    return t < Trinket::Count ? kValue[static_cast<int>(t)] : 0;
}

namespace {
// The loop (tools/valley/make_valley.py PATHS): the trailhead, up by the meadow's edge, round
// the lake's south, and back down to the trailhead.
constexpr Vec2 kLoop[] = {{60, -880}, {-80, -790}, {-260, -640}, {-420, -470}, {-300, -330}, {-160, -520},
                          {60, -600}, {200, -500}, {180, -700}, {120, -820}};
constexpr int kLoopPoints = sizeof(kLoop) / sizeof(kLoop[0]);
}  // namespace

int wanderLoop(const Vec2*& points) {
    points = kLoop;
    return kLoopPoints;
}

WanderSpot wanderSpot(u32 steps) {
    float total = 0;
    for (int k = 0; k < kLoopPoints; ++k) {
        const Vec2 a = kLoop[k], b = kLoop[(k + 1) % kLoopPoints];
        total += std::hypot(b.x - a.x, b.y - a.y);
    }
    float d = std::fmod(static_cast<float>(steps) * 0.75f, total);
    for (int k = 0; k < kLoopPoints; ++k) {
        const Vec2 a = kLoop[k], b = kLoop[(k + 1) % kLoopPoints];
        const float len = std::hypot(b.x - a.x, b.y - a.y);
        if (d <= len || k == kLoopPoints - 1) {
            const float t = len > 0 ? std::fmin(1.0f, d / len) : 0.0f;
            WanderSpot w;
            w.at = {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
            w.heading = std::atan2(b.x - a.x, -(b.y - a.y));
            return w;
        }
        d -= len;
    }
    return WanderSpot{};
}

int wandererIndex(const SaveData& s) {
    for (int i = 0; i < s.dragonCount; ++i)
        if (s.dragons[i].wanderSince != 0) return i;
    return -1;
}

const char* cantWander(const SaveData& s, int index) {
    if (index < 0 || index >= s.dragonCount) return "Choose a dragon.";
    const Dragon& d = s.dragons[index];
    if (d.stage == Stage::Egg || d.stage == Stage::Hatchling) return "Hatchlings are too little for the trails yet.";
    if (d.location != Location::Den) return "Bring it home to the den first.";
    if (d.upset) return "It's too upset to go anywhere with you.";
    if (wandererIndex(s) >= 0) return "Someone is already out wandering.";
    return nullptr;
}

bool setOff(SaveData& s, int index, u32 stepCount, s64 now) {
    if (cantWander(s, index)) return false;
    Dragon& d = s.dragons[index];
    d.wanderSince = now;
    d.wanderSteps = stepCount;
    return true;
}

u32 stepsSince(const Dragon& d, u32 stepCount) {
    return stepCount >= d.wanderSteps ? stepCount - d.wanderSteps : stepCount;  // a reset counter
}

WanderFinds rollFinds(const Dragon& d, u32 steps, Rng& rng) {
    WanderFinds f;
    f.steps = steps;
    // A chance every kStepsPerFind steps; adults and the curious sniff out more.
    float chances = static_cast<float>(steps) / kStepsPerFind;
    if (d.stage == Stage::Adult) chances *= 1.3f;
    if (d.personality == Personality::Curious) chances *= 1.25f;
    if (hasTrait(d, kTraitKeenNose)) chances *= 1.25f;  // (D150: Keen Nose; Lucky's rare finds, Treasure Hunter's Gleam)
    const bool lucky = hasTrait(d, kTraitLucky);
    const int rolls = static_cast<int>(chances) + (unit(rng) < chances - static_cast<int>(chances) ? 1 : 0);
    static constexpr float kTrinketWeight[kTrinkets] = {30, 22, 22, 12, 8, 6};
    for (int i = 0; i < rolls; ++i) {
        const float r = unit(rng);
        if (r < 0.55f) {
            f.gleam += 5 + rng.below(21);  // 5..25
        } else if (r < (lucky ? 0.9f : 0.95f)) {
            float pick = unit(rng) * 100.0f;
            int t = 0;
            while (t < kTrinkets - 1 && pick >= kTrinketWeight[t]) pick -= kTrinketWeight[t++];
            if (f.trinkets[t] < 255) ++f.trinkets[t];
        } else {
            f.gleam += 30 + rng.below(31);  // a little hoard of someone else's: 30..60
        }
    }
    if (hasTrait(d, kTraitTreasureHunter)) f.gleam += f.gleam / 2;
    return f;
}

WanderFinds comeBack(SaveData& s, int index, u32 stepCount, s64 now, Rng& rng) {
    WanderFinds f;
    if (index < 0 || index >= s.dragonCount || s.dragons[index].wanderSince == 0) return f;
    Dragon& d = s.dragons[index];
    f = rollFinds(d, stepsSince(d, stepCount), rng);
    d.wanderSince = 0;
    d.wanderSteps = 0;
    if (s.world.flags & kFlagMetTraveller) s.world.flags |= kFlagWandered;  // the trailhead's quest (Beta)
    s.gleam += f.gleam;
    for (int t = 0; t < kTrinkets; ++t) s.hoard[t] = static_cast<u16>(s.hoard[t] + f.trinkets[t] > 60000 ? 60000 : s.hoard[t] + f.trinkets[t]);
    // Rarely a wild egg: one chance per 2,000 steps, at most one a trip. Mostly the breeds
    // you can't start with (Grove, Frost, Lumen).
    const u32 eggRolls = f.steps / 2000;
    bool egg = false;
    for (u32 i = 0; i < eggRolls && !egg; ++i) egg = rng.chance(hasTrait(d, kTraitLucky) ? 6 : 3, 100);
    if (egg && s.dragonCount < static_cast<int>(kMaxDragons)) {
        const Element e = static_cast<Element>(rng.chance(3, 4) ? 3 + rng.below(3) : rng.below(3));
        const Genome g = makePurebred(e, rng);
        const Sex sex = rollSex(rng);
        Dragon wild = makeEgg(s.nextId++, g, sex, now, rollLook(rng));
        wild.origin = Origin::Wild;
        placeEgg(s, wild);
        s.dragons[s.dragonCount] = wild;
        f.wildEgg = s.dragonCount++;
    }
    // Home muddy: spots on the legs, the belly and the tail most of all, and a little dust
    // (D46; the bath sorts it out).
    const float mud = static_cast<float>(f.steps) / 60.0f;
    auto dirty = [&](int region, float k) {
        d.mud[region] = std::fmin(100.0f, d.mud[region] + mud * k);
        d.dirt[region] = std::fmin(100.0f, d.dirt[region] + 0.3f * mud * k);
    };
    dirty(kRegionBelly, 1.0f);
    dirty(kRegionLeft, 0.8f);
    dirty(kRegionRight, 0.8f);
    dirty(kRegionTail, 0.9f);
    dirty(kRegionBack, 0.3f);
    d.needs.clean = std::fmax(0.0f, d.needs.clean - 0.4f * std::fmin(mud, 100.0f));  // not so clean now (D83)
    markVisit(d, now);
    return f;
}

}  // namespace ec
