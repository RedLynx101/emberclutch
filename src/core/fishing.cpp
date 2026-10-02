#include "core/fishing.hpp"

#include <cmath>

#include "core/dragon.hpp"
#include "core/place_layout.hpp"
#include "core/valley.hpp"

namespace ec::fishing {
namespace {

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
float unit(Rng& r) { return r.next() * (1.0f / 4294967296.0f); }
u64 mixDay(s32 day, u32 salt) { return (static_cast<u64>(static_cast<u32>(day)) * 0x9E3779B97F4A7C15ull) ^ salt; }

}  // namespace

// ------------------------------------------------------------------------------ the cove
CoveSpots coveSpots(const Valley& v) {
    CoveSpots s;
    const ValleyPlaceInfo* p = v.place(kPlaceCove);
    // The waterline straight out from x (the first point along +Y the lake covers); the ground
    // as the valley has it now puts it about 42 m out.
    auto shoreAt = [&](float x) {
        if (p)
            for (float y = 8.0f; y < 90.0f; y += 0.5f) {
                const Vec2 w = placeToWorld(*p, {x, y});
                if (v.heightAt(w.x, w.y) < v.water + 0.2f) return y;
            }
        return 42.0f;
    };
    const float shore = shoreAt(0.0f);
    s.fishSpot = {0.0f, shore - 1.3f};
    // The cove's own anchors (workstream A): the jetty's end to fish from (a deck: core/place_layout
    // addPlaceDecks), Tam by his shack, the shells along the water's edge.
    if (const PlaceAnchor spot = placeAnchor(kPlaceCove, "fish_spot")) {
        s.fishSpot = {spot.at(0).x, spot.at(0).y};
        s.castTo = {s.fishSpot.x + 0.4f, s.fishSpot.y + 9.0f};
        s.partner = {s.fishSpot.x - 0.85f, s.fishSpot.y - 0.9f};  // at your side on the narrow jetty (in the camera's view)
        // Tam fishes at the water's edge straight out from his shack's anchor, looking out over the
        // lake (run 24, Noah: "Tam has his pole out fishing, but he's nowhere near the water": he
        // stood by the shack, 30 m up the beach, his line down onto the sand).
        const PlaceAnchor tam = placeAnchor(kPlaceCove, "fisher");
        const float tamX = tam ? tam.at(0).x : -5.2f;
        s.fisher = {tamX, shoreAt(tamX) - 1.2f};
        s.fisherFacing = 0.0f;
        const PlaceAnchor shells = placeAnchor(kPlaceCove, "shells");
        for (int k = 0; k < kShellSpots; ++k)
            s.shells[k] = k < shells.count ? Vec2{shells.at(k).x, shells.at(k).y} : Vec2{-18.0f + 8.0f * k, shore - 0.9f};
        return s;
    }
    // The bobber and your partner follow your spot (with the anchors: fish_spot at the jetty's
    // end, your partner beside you on its deck, the bobber out past it).
    s.castTo = {s.fishSpot.x + 0.6f, s.fishSpot.y + 10.8f};
    s.partner = {s.fishSpot.x - 1.8f, s.fishSpot.y - 0.6f};
    s.fisher = {-5.2f, shore - 3.4f};  // beyond your partner, on your left
    s.fisherFacing = 0.55f;             // looking out over the water, turned a little toward your spot
    static constexpr float kShellX[kShellSpots] = {-18.0f, -10.5f, -5.0f, 9.0f, 16.0f};
    for (int k = 0; k < kShellSpots; ++k) s.shells[k] = {kShellX[k], shoreAt(kShellX[k]) - 0.9f};  // on the wet sand
    return s;
}

// ------------------------------------------------------------------------------ what bites
const CatchInfo& catchInfo(Catch c) {
    static const CatchInfo kInfo[static_cast<int>(Catch::Count)] = {
        {"a River Fish", Food::RiverFish, 1, 0, 1.0f, true},
        {"a big River Fish", Food::RiverFish, 2, 0, 1.55f, true},
        {"a Honeyroot", Food::Honeyroot, 1, 0, 0.8f, false},   // a sunken root, snagged
        {"a Skyberry sprig", Food::Skyberry, 1, 0, 0.6f, false},  // floating by
        {"a Frostmelon", Food::Frostmelon, 1, 0, 0.9f, false},  // cooling in the shallows
        {"a shell", Food::Count, 0, 20, 0.6f, false},         // tangled on the hook (20: D128)
        {"a pearl!", Food::Count, 0, 120, 0.7f, false},
        {"Old Whiskers", Food::Count, 0, 50, 1.75f, true},     // the one that got away (D137): let go again
    };
    return kInfo[c < Catch::Count ? static_cast<int>(c) : 0];
}

bool goldenHour(int hour) { return (hour >= 5 && hour <= 7) || (hour >= 17 && hour <= 19); }

Catch rollCatch(Rng& rng, int hour) {
    // River, big, honeyroot, skyberry, frostmelon, shell, pearl (a hundred each).
    static constexpr int kDay[static_cast<int>(Catch::Count)] = {58, 12, 7, 6, 5, 10, 2, 0};
    static constexpr int kGolden[static_cast<int>(Catch::Count)] = {51, 20, 7, 5, 5, 10, 2, 0};
    const int* w = goldenHour(hour) ? kGolden : kDay;
    int r = static_cast<int>(rng.below(100));
    for (int k = 0; k < static_cast<int>(Catch::Count); ++k) {
        if (r < w[k]) return static_cast<Catch>(k);
        r -= w[k];
    }
    return Catch::RiverFish;
}

// ------------------------------------------------------------------------------ the bite
Bite rollBite(Rng& rng, int hour, Catch c) {
    Bite b;
    b.wait = goldenHour(hour) ? 1.6f + 3.0f * unit(rng) : 2.6f + 4.4f * unit(rng);
    const bool big = c == Catch::BigFish || c == Catch::Whiskers;
    b.nibbles = static_cast<int>(rng.below(big ? 4u : 3u));  // a big one teases more
    if (b.nibbles > kMaxNibbles) b.nibbles = kMaxNibbles;
    // The nibbles spread before the bite, well apart and clear of the plop and the bite.
    while (b.nibbles > 0 && (b.wait - 1.4f) / b.nibbles < 0.9f) --b.nibbles;
    for (int k = 0; k < b.nibbles; ++k) {
        const float slot = (b.wait - 1.4f) / b.nibbles;
        b.nibbleAt[k] = 0.7f + slot * k + slot * 0.2f * unit(rng);
    }
    b.window = big ? 0.7f : c == Catch::Skyberry ? 1.0f : 0.85f;
    return b;
}

// ------------------------------------------------------------------------------ the reel
void Reel::start(float s, u32 seed) {
    *this = Reel{};
    strength = s;
    rng = Rng(seed ? seed : 1);
    runIn = 1.0f + 1.5f * unit(rng);
}

Reel::Step Reel::update(float reel, float dt) {
    if (step != Step::Reeling) return step;
    reel = clampf(reel, 0.0f, 1.0f);
    // Now and then it runs (a big one longer): it pulls hard and slips back a little.
    if (runFor > 0) {
        runFor -= dt;
    } else if ((runIn -= dt) <= 0) {
        runFor = (0.5f + 0.7f * unit(rng)) * (0.7f + 0.3f * strength);
        runIn = 1.8f + 2.4f * unit(rng);
    }
    const float pull = strength * (running() ? 0.85f : 0.22f);
    // Reeling tightens it; letting up gives line (the drag gives more the tighter it is).
    tension += (reel * 0.6f + pull - (1.0f - reel) * (0.5f + 1.0f * tension)) * dt;
    if (reel > 0) progress += reel * (inBand() ? 0.36f : 0.12f) / (0.6f + 0.4f * strength) * dt;
    if (running()) progress -= 0.05f * strength * dt;
    tension = clampf(tension, 0.0f, 1.0f);
    progress = clampf(progress, 0.0f, 1.0f);
    if (tension >= 1.0f) {
        step = Step::Snapped;
    } else if (progress >= 1.0f) {
        step = Step::Caught;
    } else {
        slackFor = tension < kSlack ? slackFor + dt : 0.0f;
        idleFor = reel < 0.05f ? idleFor + dt : 0.0f;
        if (slackFor > kSlackLimit || idleFor > kIdleLimit) step = Step::Escaped;
    }
    return step;
}

// ------------------------------------------------------------------------------ the day
u8 shellsToday(s32 day) {
    Rng rng(mixDay(day, 0x5E11u));
    const int count = 3 + static_cast<int>(rng.below(3));  // three to five
    u8 bits = 0;
    int have = 0;
    while (have < count) {
        const u8 b = static_cast<u8>(1u << rng.below(kShellSpots));
        if (!(bits & b)) {
            bits |= b;
            ++have;
        }
    }
    return bits;
}

namespace {
Rng shellRng(s32 day, int spot) { return Rng(mixDay(day, 0xC0A57u + 977u * static_cast<u32>(spot))); }
}  // namespace

Catch shellAt(s32 day, int spot) {
    Rng rng = shellRng(day, spot);
    return rng.below(100) < 4 ? Catch::Pearl : Catch::Shell;  // a pearl now and then
}

namespace {
int shellPick(s32 day, int spot) {  // which of the four shells
    Rng rng = shellRng(day, spot);
    rng.next();
    return static_cast<int>(rng.below(4));
}
}  // namespace

const char* shellName(s32 day, int spot) {
    if (shellAt(day, spot) == Catch::Pearl) return "a pearl";
    static const char* const kNames[] = {"a spiral shell", "a scallop shell", "a pink cowrie", "a sand dollar"};
    return kNames[shellPick(day, spot)];
}

int shellKind(s32 day, int spot) {
    if (shellAt(day, spot) == Catch::Pearl) return 3;
    static constexpr int kKinds[4] = {0, 1, 2, 1};  // (a sand dollar lies flat, like a scallop)
    return kKinds[shellPick(day, spot)];
}

u32 shellGleam(s32 day, int spot) {
    if (shellAt(day, spot) == Catch::Pearl) return catchInfo(Catch::Pearl).gleam;
    return catchInfo(Catch::Shell).gleam;  // (1.0, D128: 20 a shell, was 5..12)
}

// ------------------------------------------------------------------------------ your partner
void nibble(Dragon& d) {
    if (d.stage == Stage::Egg) return;
    d.needs.love = clampf(d.needs.love + kNibbleLove, 0.0f, 100.0f);
    d.needs.belly = clampf(d.needs.belly + kNibbleBelly, 0.0f, 100.0f);
}

}  // namespace ec::fishing
