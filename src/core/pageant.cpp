#include "core/pageant.hpp"

#include <cmath>
#include <cstring>

#include "core/kinds.hpp"
#include "core/save.hpp"

namespace ec {
namespace {

// The elements in core/kinds' order (kinds_data.inc; tests/test_pageant.cpp checks the names).
enum : u8 { kEmber, kGrove, kStone, kGale, kTide, kFrost, kLumen, kShade };
constexpr u8 bit(int e) { return static_cast<u8>(1u << e); }

constexpr ThemeInfo kThemeInfo[kThemes] = {
    {"Frost Ball", "Cool colours and frosty finery.", bit(kFrost) | bit(kTide) | bit(kLumen), {{160, 210, 250}, {240, 246, 255}},
     kStyleFrosty | kStyleElegant},
    {"Harvest Fair", "Autumn gold, flowers and the wild.", bit(kGrove) | bit(kStone), {{232, 150, 60}, {130, 170, 80}},
     kStyleFloral | kStyleWild},
    {"Starlight Gala", "Night blues and starry gold.", bit(kLumen) | bit(kShade) | bit(kGale), {{50, 60, 130}, {240, 200, 90}},
     kStyleStarry | kStyleElegant},
    {"Ember Carnival", "Flame reds, fire and festivity.", bit(kEmber) | bit(kStone), {{220, 70, 50}, {245, 160, 60}},
     kStyleFiery | kStyleFestive},
    {"Tide Regatta", "Sea blues, bunting and fun.", bit(kTide) | bit(kGale), {{70, 170, 190}, {245, 245, 240}},
     kStyleCute | kStyleFestive},
    {"Blossom Festival", "Spring pinks and flowers.", bit(kGrove) | bit(kLumen), {{245, 170, 200}, {170, 220, 130}},
     kStyleFloral | kStyleCute},
    {"Shadow Masquerade", "Deep violets, mystery and flair.", bit(kShade) | bit(kEmber), {{110, 60, 150}, {40, 36, 50}},
     kStyleElegant | kStyleWild},
    {"Sunrise Parade", "Dawn golds, warm and bright.", bit(kEmber) | bit(kLumen) | bit(kGale), {{250, 190, 80}, {250, 160, 140}},
     kStyleStarry | kStyleFiery},
};

const char* const kTrainers[] = {"Posy", "Bram", "Juniper", "Tamsin", "Oren", "Marlow", "Ivy", "Cobb",
                                 "Elsie", "Rook", "Hazel", "Fenn", "Lark", "Nell", "Quill", "Briar"};
const char* const kDragonNames[] = {"Biscuit", "Sable", "Pip", "Clover", "Nimbus", "Pebble", "Saffron", "Mistral",
                                    "Duchess", "Sprocket", "Velvet", "Tinder", "Opal", "Fable", "Juno", "Rumble",
                                    "Kestrel", "Marzipan", "Solstice", "Thistle"};
constexpr int kTrainerCount = sizeof(kTrainers) / sizeof(kTrainers[0]);
constexpr int kDragonNameCount = sizeof(kDragonNames) / sizeof(kDragonNames[0]);

// How each manner carries itself on a stage (0..1), in core/kinds' manner order: Brave, Shy,
// Playful, Proud, Sleepy, Curious, Gentle, Mischievous, Greedy, Stubborn.
constexpr float kMannerPoise[] = {0.7f, 0.3f, 0.6f, 1.0f, 0.4f, 0.6f, 0.9f, 0.5f, 0.4f, 0.5f};

// A league's rivals' middle, the rewards (Gleam) by league and place.
constexpr float kRivalStrength[kLeagues] = {36, 50, 64, 77};
constexpr u32 kFirstWin[kLeagues] = {50, 90, 140, 220}, kWinAgain[kLeagues] = {20, 30, 45, 60};
constexpr u32 kPlaced[3][kLeagues] = {{12, 18, 25, 35}, {6, 10, 14, 20}, {3, 5, 7, 10}};
constexpr u32 kLeagueBonus[kLeagues] = {100, 200, 350, 500};

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
int clampLeague(int league) { return league < 1 ? 1 : (league > kLeagues ? kLeagues : league); }
std::uint64_t showSeed(s32 day, int league, int slot, u32 salt) {
    return (static_cast<std::uint64_t>(static_cast<u32>(day)) * 0x9E3779B97F4A7C15ull) ^
           (static_cast<std::uint64_t>(league * 16 + slot + 1) * 0xD1B54A32D192ED03ull) ^ salt;
}
float rescaleMatch(float m) { return clampf((m - 0.3f) / 0.7f, 0.0f, 1.0f); }

}  // namespace

const ThemeInfo& themeInfo(int theme) { return kThemeInfo[theme >= 0 && theme < kThemes ? theme : 0]; }

const char* roundName(int round) {
    static const char* const kNames[kRounds] = {"Look", "Poise", "Performance"};
    return round >= 0 && round < kRounds ? kNames[round] : "";
}

namespace pageant {

int showTheme(int league, int slot, s32 day) {
    // A shuffle of the eight for the league's day; its four slots take the first four.
    int order[kThemes];
    for (int t = 0; t < kThemes; ++t) order[t] = t;
    Rng rng(showSeed(day, clampLeague(league), 0, 0x7E3Eu));
    for (int t = kThemes - 1; t > 0; --t) {
        const int j = static_cast<int>(rng.below(static_cast<u32>(t + 1)));
        const int k = order[t];
        order[t] = order[j];
        order[j] = k;
    }
    return order[slot >= 0 && slot < kShowSlots ? slot : 0];
}

bool leagueOpen(const SaveData& s, int league) { return league >= 1 && league <= kLeagues && s.progress.showLeague >= league - 1; }

bool slotWon(const SaveData& s, int league, int slot) {
    if (league < 1 || league > kLeagues || slot < 0 || slot >= kShowSlots) return false;
    return (s.progress.showWon[league - 1] >> slot) & 1u;
}

int slotsWon(const SaveData& s, int league) {
    int n = 0;
    for (int k = 0; k < kShowSlots; ++k) n += slotWon(s, league, k);
    return n;
}

int boardLeague(const SaveData& s) {
    for (int l = 1; l <= kLeagues; ++l)
        if (leagueOpen(s, l) && s.progress.showLeague < l) return l;
    return kLeagues;
}

bool paidToday(const SaveData& s, int league, int slot, s32 today) {
    return trainer::claimedToday(s, kClaimShow + (clampLeague(league) - 1) * kShowSlots + slot, today);
}

int themeFavours(int theme, int kind) {
    const ThemeInfo& t = themeInfo(theme);
    const KindInfo& k = kindInfo(kind);
    if ((t.elements >> k.elements[0]) & 1u) return 2;
    if (k.elementCount > 1 && ((t.elements >> k.elements[1]) & 1u)) return 1;
    return 0;
}

LookParts lookParts(const Dragon& d, int theme, const Rgb pal[kPalCount]) {
    const ThemeInfo& t = themeInfo(theme);
    LookParts p;
    // Clean: dust counts, mud half as much again.
    float grime = 0;
    for (int r = 0; r < kRegionCount; ++r) grime += std::fmin(100.0f, d.dirt[r] + 1.5f * d.mud[r]);
    p.clean = 20.0f * (1.0f - grime / (100.0f * kRegionCount));
    // Dressed for the theme: each thing worn that carries one of its styles counts most.
    float style = 0;
    for (int s = 0; s < kWearSlots; ++s) {
        const int a = acc::worn(d, static_cast<WearSlot>(s));
        if (a >= 0) style += (accessoryInfo(a).styles & t.styles) ? 8.0f : 2.0f;
    }
    p.style = std::fmin(25.0f, style);
    // Its colours (with its dye) in the theme's, and what it wears against its own.
    float best = 0;
    for (const Rgb& c : t.colours)
        best = std::fmax(best, 0.7f * acc::colourMatch(pal[kPalBase], c) + 0.3f * acc::colourMatch(pal[kPalAccent], c));
    p.themeColour = 15.0f * rescaleMatch(best);
    p.suits = acc::wornCount(d) ? 10.0f * rescaleMatch(acc::suitsColours(d, pal)) : 3.0f;
    // A rarer kind and the rare colouring catch the judges' eye.
    const KindInfo& k = kindInfo(d.kind < kindCount() ? d.kind : 0);
    p.rarity = std::fmin(12.0f, (k.rarity == Rarity::Rare ? 10.0f : (k.rarity == Rarity::Uncommon ? 7.0f : 4.0f)) +
                                    (d.variant == k.rareVariant ? 2.0f : 0.0f));
    const int fav = themeFavours(theme, d.kind < kindCount() ? d.kind : 0);
    p.element = fav == 2 ? 18.0f : (fav == 1 ? 11.0f : 0.0f);
    return p;
}

float lookScore(const Dragon& d, int theme, const Rgb pal[kPalCount]) { return clampf(lookParts(d, theme, pal).total(), 0, 100); }

PoiseParts poiseParts(const Dragon& d) {
    PoiseParts p;
    p.bond = 35.0f * std::sqrt(clampf(d.bond / 1000.0f, 0.0f, 1.0f));
    constexpr int kManners = static_cast<int>(sizeof(kMannerPoise) / sizeof(kMannerPoise[0]));
    const int m = d.manner < kManners ? d.manner : 0;
    p.manner = 15.0f * kMannerPoise[m];
    static constexpr float kMood[] = {0, 4, 10, 19, 25};  // Upset .. Joyful
    p.mood = kMood[static_cast<int>(moodOf(d))];
    p.care = 25.0f * std::fmin(1.0f, d.careStars / 30.0f);
    return p;
}

float poiseScore(const Dragon& d) { return clampf(poiseParts(d).total(), 0, 100); }

Routine makeRoutine(int league, u32 seed) {
    static constexpr float kBeat[kLeagues] = {0.75f, 0.66f, 0.58f, 0.5f};
    static constexpr int kCount[kLeagues] = {8, 10, 12, 14};
    const int l = clampLeague(league) - 1;
    Routine r;
    r.beat = kBeat[l];
    Rng rng(static_cast<std::uint64_t>(seed) * 0x9E3779B97F4A7C15ull + 0xDA5Cu);
    float t = r.beat * 4;  // a bar to find the beat
    Cue last = Cue::Count, before = Cue::Count;
    while (r.count < kCount[l] && r.count < kMaxCues) {
        const Cue c = static_cast<Cue>(rng.below(10) < 2 ? static_cast<u32>(Cue::Touch) : rng.below(4));
        if (c == last && c == before) continue;  // never three the same
        r.at[r.count] = t;
        r.cue[r.count++] = c;
        before = last;
        last = c;
        // Mostly a beat apart, now and then a rest; the higher leagues a half-beat now and then.
        const u32 roll = rng.below(10);
        t += roll < 2 ? 2 * r.beat : (l >= 2 && roll >= 8 ? 0.5f * r.beat : r.beat);
    }
    r.length = r.at[r.count - 1] + 2 * r.beat;
    return r;
}

float perfectWindow(const Dragon& d) { return 0.075f * (1.0f + 0.6f * clampf(d.bond / 1000.0f, 0, 1)); }
float goodWindow(const Dragon& d) { return 0.17f * (1.0f + 0.6f * clampf(d.bond / 1000.0f, 0, 1)); }

Hit judgeHit(float error, float perfect, float good) {
    const float e = std::fabs(error);
    return e <= perfect ? Hit::Perfect : (e <= good ? Hit::Good : Hit::Miss);
}

float performanceScore(const Hit* hits, int count) {
    if (count <= 0) return 0;
    float sum = 0;
    int streak = 0, longest = 0;
    for (int i = 0; i < count; ++i) {
        sum += hits[i] == Hit::Perfect ? 1.0f : (hits[i] == Hit::Good ? 0.6f : 0.0f);
        streak = hits[i] == Hit::Miss ? 0 : streak + 1;
        if (streak > longest) longest = streak;
    }
    return clampf(90.0f * sum / count + 10.0f * longest / count, 0, 100);
}

void makeRivals(const SaveData& s, int league, int slot, s32 day, Rival out[kRivals]) {
    league = clampLeague(league);
    const int theme = showTheme(league, slot, day);
    Rng rng(showSeed(day, league, slot, 0x21A1u));
    // The guest kind: one the theme favours (any kind if none does), in two or three colourings.
    int favoured[kMaxKinds], nf = 0;
    for (int k = 0; k < kindCount(); ++k)
        if (themeFavours(theme, k) == 2) favoured[nf++] = k;
    const int guest = nf ? favoured[rng.below(static_cast<u32>(nf))] : static_cast<int>(rng.below(static_cast<u32>(kindCount())));
    // One of your own dragons' kinds (already loaded), if you have a hatched one.
    int own[kMaxKinds], no = 0;
    for (int i = 0; i < s.dragonCount; ++i) {
        const Dragon& d = s.dragons[i];
        if (d.stage == Stage::Egg || d.kind >= kindCount()) continue;
        bool seen = false;
        for (int j = 0; j < no; ++j) seen |= own[j] == d.kind;
        if (!seen && no < kMaxKinds) own[no++] = d.kind;
    }
    // Three different trainers and three different names.
    auto distinct = [&](int pool, int* picked, int i) {
        for (;;) {
            const int v = static_cast<int>(rng.below(static_cast<u32>(pool)));
            bool clash = false;
            for (int j = 0; j < i; ++j) clash |= picked[j] == v;
            if (!clash) return picked[i] = v;
        }
    };
    int trainers[kRivals], names[kRivals];
    for (int i = 0; i < kRivals; ++i) {
        Rival& r = out[i];
        r.trainer = kTrainers[distinct(kTrainerCount, trainers, i)];
        Dragon& d = r.dragon;
        d = Dragon{};
        d.id = 0xFEE00000u + static_cast<u32>(league * 64 + slot * 8 + i);  // (never a save's: ids count up from 1)
        const int kind = (i == 1 && no) ? own[rng.below(static_cast<u32>(no))] : guest;
        d.kind = static_cast<u8>(kind);
        d.variant = static_cast<u8>(rng.below(kKindVariants - 1));
        if (league >= 3 && rng.chance(1, 4)) d.variant = kKindVariants - 1;  // the rare colouring, now and then
        if (i == 2 && kind == guest && d.variant == out[0].dragon.variant) d.variant = static_cast<u8>((d.variant + 1) % (kKindVariants - 1));
        d.stage = Stage::Adult;
        d.hatchedAt = 1;
        d.genome.size = static_cast<u8>(rng.below(256));
        d.genome.build = static_cast<u8>(rng.below(3));
        d.manner = static_cast<u8>(rng.below(static_cast<u32>(mannerCount())));
        d.bond = static_cast<u16>(200 + 200 * league);
        std::strncpy(d.name, kDragonNames[distinct(kDragonNameCount, names, i)], sizeof(d.name) - 1);
        // Dressed for the league: a thing or two, the higher leagues for the theme.
        const int wearCount = league == 1 ? static_cast<int>(rng.below(2)) : (league == 4 ? 2 : 1 + static_cast<int>(rng.below(2)));
        for (int w = 0; w < wearCount; ++w) {
            int pick = -1;
            for (int tries = 0; tries < 12 && pick < 0; ++tries) {
                const int a = static_cast<int>(rng.below(kAccessoryCount));
                const Accessory& x = accessoryInfo(a);
                if (d.wear[static_cast<int>(x.slot)] != kNone) continue;
                if (league >= 3 && !(x.styles & themeInfo(theme).styles) && tries < 8) continue;
                pick = a;
            }
            if (pick >= 0) d.wear[static_cast<int>(accessoryInfo(pick).slot)] = static_cast<u8>(pick);
        }
        if (league >= 3 && rng.chance(1, 3)) d.dye = static_cast<u8>(1 + rng.below(kDyeCount - 1));
        r.strength = kRivalStrength[league - 1] + static_cast<float>(rng.range(-6, 6)) + (themeFavours(theme, kind) ? 3.0f : 0.0f);
    }
}

float rivalRound(const Rival& r, int round, int theme, Rng& rng) {
    float v = r.strength;
    if (round == kRoundLook) v += (acc::wornStyles(r.dragon) & themeInfo(theme).styles) ? 4.0f : -3.0f;
    const int spread = round == kRoundPerformance ? 10 : 6;
    v += static_cast<float>(rng.range(-spread, spread));
    return clampf(v, 5, 98);
}

void judgeCards(float score, int round, Rng& rng, float cards[kJudges]) {
    // Each judge leans a little: the first to looks, the second to poise, the third to the show.
    static constexpr float kLean[kJudges][kRounds] = {{0.4f, -0.1f, -0.2f}, {-0.1f, 0.4f, -0.1f}, {-0.2f, -0.1f, 0.4f}};
    for (int j = 0; j < kJudges; ++j) {
        const float raw = score / 10.0f + kLean[j][round < kRounds ? round : 0] + static_cast<float>(rng.range(-4, 4)) * 0.1f;
        cards[j] = clampf(std::round(raw * 2.0f) / 2.0f, 1.0f, 10.0f);
    }
}

void placings(const float totals[kEntrants], const float performance[kEntrants], int order[kEntrants]) {
    for (int i = 0; i < kEntrants; ++i) order[i] = i;
    auto better = [&](int a, int b) {
        if (totals[a] != totals[b]) return totals[a] > totals[b];
        if (performance[a] != performance[b]) return performance[a] > performance[b];
        return a < b;
    };
    for (int i = 1; i < kEntrants; ++i)
        for (int j = i; j > 0 && better(order[j], order[j - 1]); --j) {
            const int t = order[j];
            order[j] = order[j - 1];
            order[j - 1] = t;
        }
}

ShowReward finishShow(SaveData& s, Dragon& d, int league, int slot, int theme, int place, s32 today) {
    ShowReward r;
    league = clampLeague(league);
    if (slot < 0 || slot >= kShowSlots) slot = 0;
    const int l = league - 1;
    trainer::count(s, kCountShows);
    trainer::recordShow(d, place == 0, theme);
    if (place != 0) {  // a thank-you for taking part (no claim: Energy limits the day's shows)
        r.gleam = kPlaced[place - 1 < 3 ? place - 1 : 2][l];
        s.gleam += r.gleam;
        return r;
    }
    r.firstWin = !slotWon(s, league, slot);
    s.progress.showWon[l] = static_cast<u8>(s.progress.showWon[l] | (1u << slot));
    r.paid = trainer::claimToday(s, kClaimShow + l * kShowSlots + slot, today);
    if (r.paid) r.gleam = r.firstWin ? kFirstWin[l] : kWinAgain[l];
    if (r.firstWin) {  // a slot's first win: one of the shows' prizes you don't have yet
        r.accessory = acc::unownedFrom(s, WearSource::Prize, static_cast<u32>(today * 31 + league * 7 + slot));
        if (r.accessory >= 0) trainer::giveAccessory(s, r.accessory);
        else r.gleam += kFirstWin[l];
    }
    if (slotsWon(s, league) == kShowSlots && s.progress.showLeague < league) {  // the league is yours
        r.leagueWon = true;
        s.progress.showLeague = static_cast<u8>(league);
        trainer::recordShowLeague(d, league);
        r.gleam += kLeagueBonus[l];
        r.dye = acc::prizeDye(s, static_cast<u32>(today + league));
        if (r.dye > 0) trainer::giveDye(s, r.dye);
    }
    s.gleam += r.gleam;
    return r;
}

}  // namespace pageant
}  // namespace ec
