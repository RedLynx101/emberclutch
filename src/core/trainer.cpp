#include "core/trainer.hpp"

#include "core/kinds.hpp"
#include "core/save.hpp"

namespace ec::trainer {
namespace {

float clamp100(float v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }

}  // namespace

// A gentle curve: level 5 after a few battles, 10 in a couple of days' play, 30 for the leagues'
// finals, 42 (Skyreach's cap, kLevelCap) for the devoted; the curve runs on to 50 for later valleys.
u32 xpForLevel(int level) {
    if (level <= 1) return 0;
    if (level > kMaxLevel) level = kMaxLevel;
    const u32 n = static_cast<u32>(level - 1);
    return 12 * n * n + 38 * n;
}

int levelOf(u32 xp) {
    int level = 1;
    while (level < kLevelCap && xp >= xpForLevel(level + 1)) ++level;
    return level;
}

int levelOf(const Dragon& d) { return levelOf(d.xp); }

void levelProgress(const Dragon& d, u32& into, u32& span) {
    const int level = levelOf(d.xp);
    if (level >= kLevelCap) {
        into = span = 0;
        return;
    }
    into = d.xp - xpForLevel(level);
    span = xpForLevel(level + 1) - xpForLevel(level);
}

u32 xpTaken(const Dragon& d, u32 amount) { return hasTrait(d, kTraitQuickLearner) ? amount + amount / 5 : amount; }

int gainXp(Dragon& d, u32 amount) {
    amount = xpTaken(d, amount);
    const int before = levelOf(d.xp);
    const u32 cap = xpForLevel(kLevelCap);
    d.xp = d.xp + amount > cap || d.xp + amount < d.xp ? cap : d.xp + amount;
    return levelOf(d.xp) - before;
}

int statPoints(const Dragon& d, int stat) {
    if (stat < 0 || stat >= kDragonStats) return 1;
    const int base = d.stats[stat] < 1 ? 1 : d.stats[stat];
    const int extra = d.trained[stat] > kMaxTrained ? kMaxTrained : d.trained[stat];
    return base + extra + (hasTrait(d, kTraitStarborn) ? 1 : 0);
}

bool train(Dragon& d, int stat, int points) {
    if (stat < 0 || stat >= kDragonStats || d.trained[stat] >= kMaxTrained || points <= 0) return false;
    const int v = d.trained[stat] + points;
    d.trained[stat] = static_cast<u8>(v > kMaxTrained ? kMaxTrained : v);
    return true;
}

float energyCost(const Dragon& d, float energy) { return hasTrait(d, kTraitSturdy) ? energy * 0.75f : energy; }

bool canSpend(const Dragon& d, float energy) { return d.needs.energy >= energyCost(d, energy); }

bool spendEnergy(Dragon& d, float energy) {
    if (!canSpend(d, energy)) return false;
    d.needs.energy = clamp100(d.needs.energy - energyCost(d, energy));
    return true;
}

void walkTogether(Dragon& d, float metres, float& carry) {
    if (metres <= 0 || d.stage == Stage::Egg) return;
    Needs& n = d.needs;
    // per 100 m: Play +3, Love +2, Belly -1.5 (on top of the hours' own drain); a bond point every 150 m
    n.play = clamp100(n.play + metres * 0.03f);
    n.love = clamp100(n.love + metres * 0.02f);
    n.belly = clamp100(n.belly - metres * 0.015f);
    carry += metres;
    const float per = hasTrait(d, kTraitChatty) ? 75.0f : 150.0f;  // (Chatty: twice as fast, D150)
    while (carry >= per) {
        carry -= per;
        addBond(d, 1);
    }
}

void recordCup(Dragon& d, int challenge, int cup) {
    if (challenge < 0 || challenge >= kChallenges || cup < 1 || cup > kCups) return;
    d.cupsWon = static_cast<u16>(d.cupsWon | (1u << (challenge * kCups + cup - 1)));
}

bool wonCup(const Dragon& d, int challenge, int cup) {
    if (challenge < 0 || challenge >= kChallenges || cup < 1 || cup > kCups) return false;
    return (d.cupsWon >> (challenge * kCups + cup - 1)) & 1u;
}

namespace {
void bump(u16& v) {
    if (v < 0xFFFF) ++v;
}
}  // namespace

void recordBattle(Dragon& d, bool won) {
    if (won) bump(d.battleWins);
}

void recordLeague(Dragon& d, int league) {
    if (league > d.battleTitle && league <= kLeagues) d.battleTitle = static_cast<u8>(league);
}

void recordShow(Dragon& d, bool won, int theme) {
    if (!won) return;
    bump(d.showWins);
    if (theme >= 0 && theme < 32) d.ribbons |= 1u << theme;
}

void recordShowLeague(Dragon& d, int league) {
    if (league > d.showTitle && league <= kLeagues) d.showTitle = static_cast<u8>(league);
}

void recordWild(Dragon& d, bool won, int floor) {
    if (!won) return;
    bump(d.wildWins);
    if (floor > d.frostDeepest && floor < 256) d.frostDeepest = static_cast<u8>(floor);
}

namespace {
int bits(u32 v) {
    int n = 0;
    for (; v; v &= v - 1) ++n;
    return n;
}
}  // namespace

int ribbonCount(const Dragon& d) { return bits(d.ribbons); }
int cupCount(const Dragon& d) { return bits(d.cupsWon); }

const char* leagueName(int league) {
    static const char* const kNames[] = {"", "Ember", "Flame", "Blaze", "Starfire"};
    return league >= 0 && league <= kLeagues ? kNames[league] : "";
}

const char* battleTitleName(int league) {
    static const char* const kNames[] = {"", "Ember Victor", "Flame Victor", "Blaze Victor", "Starfire Champion"};
    return league >= 0 && league <= kLeagues ? kNames[league] : "";
}

const char* showTitleName(int league) {
    static const char* const kNames[] = {"", "Ember Darling", "Flame Beauty", "Blaze Belle", "Starfire Star"};
    return league >= 0 && league <= kLeagues ? kNames[league] : "";
}

bool claimedToday(const SaveData& s, int bit, s32 today) {
    if (bit < 0 || bit >= 64 || s.progress.claimDay != today) return false;
    return (s.progress.claims >> bit) & 1u;
}

bool claimToday(SaveData& s, int bit, s32 today) {
    if (bit < 0 || bit >= 64) return false;
    if (s.progress.claimDay != today) {
        s.progress.claimDay = today;
        s.progress.claims = 0;
    }
    const u64 mask = u64(1) << bit;
    if (s.progress.claims & mask) return false;
    s.progress.claims |= mask;
    return true;
}

bool ownsAccessory(const SaveData& s, int a) {
    if (a < 0 || a >= kAccessoryBytes * 8) return false;
    return (s.progress.accessories[a >> 3] >> (a & 7)) & 1u;
}

void giveAccessory(SaveData& s, int a) {
    if (a < 0 || a >= kAccessoryBytes * 8) return;
    s.progress.accessories[a >> 3] = static_cast<u8>(s.progress.accessories[a >> 3] | (1u << (a & 7)));
}

bool ownsDye(const SaveData& s, int dye) { return dye == 0 || (dye > 0 && dye < 32 && ((s.progress.dyes >> dye) & 1u)); }

void giveDye(SaveData& s, int dye) {
    if (dye > 0 && dye < 32) s.progress.dyes |= 1u << dye;
}

bool tipSeen(const SaveData& s, int tip) { return tip >= 0 && tip < 32 && ((s.progress.tips >> tip) & 1u); }

void markTip(SaveData& s, int tip) {
    if (tip >= 0 && tip < 32) s.progress.tips |= 1u << tip;
}

void count(SaveData& s, RecordCount c, int by) {
    if (c >= kRecordCounts || by <= 0) return;
    const int v = s.progress.counts[c] + by;
    s.progress.counts[c] = static_cast<u16>(v > 0xFFFF ? 0xFFFF : v);
}

void track(SaveData& s, Tracked kind, int id) {
    s.progress.trackKind = static_cast<u8>(kind < Tracked::Count ? kind : Tracked::None);
    s.progress.trackId = static_cast<u8>(id < 0 ? 0 : (id > 255 ? 255 : id));
}

Tracked tracked(const SaveData& s) {
    return s.progress.trackKind < static_cast<u8>(Tracked::Count) ? static_cast<Tracked>(s.progress.trackKind)
                                                                    : Tracked::None;
}

}  // namespace ec::trainer
