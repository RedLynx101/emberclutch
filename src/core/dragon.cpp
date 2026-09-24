#include "core/dragon.hpp"

#include <initializer_list>

#include "core/clock.hpp"

namespace ec {
namespace {

float clamp100(float v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }

}  // namespace

void addBond(Dragon& d, int amount) {
    if (d.upset) return;  // bond only grows again after making up
    int b = d.bond + amount;
    if (b > 1000) b = 1000;
    d.bond = static_cast<u16>(b);
    if (d.bond > d.bondHigh) d.bondHigh = d.bond;
}

namespace {

// Care stars for one finished day: average of the lowest need over the hours the
// dragon spent in the den. A day without a visit earns at most one star.
int starsForDay(float lowestSum, float hours, bool visited) {
    if (hours < 1.0f) return 0;
    const float avg = lowestSum / hours;
    int stars = avg >= 65 ? 3 : (avg >= 40 ? 2 : (avg >= 20 ? 1 : 0));
    if (!visited && stars > 1) stars = 1;
    return stars;
}

void closeDay(Dragon& d, s32 newDay) {
    d.careStars = static_cast<u16>(d.careStars + starsForDay(d.dayLowestSum, d.dayHours, d.dayVisited));
    d.day = newDay;
    d.dayLowestSum = 0;
    d.dayHours = 0;
    d.dayVisited = false;
}

void promote(Dragon& d, s64 now) {
    if (d.stage == Stage::Egg) return;
    const Stage s = stageFor(daysSinceHatch(d, now), d.careStars);
    if (s > d.stage) d.stage = s;  // never regress
}

void stepEgg(Dragon& d, float hours, s32 dt) {
    if (d.location == Location::Vault) return;  // incubation paused
    d.warmth = clamp100(d.warmth - 4.0f * hours);
    if (d.warmth > 20) d.incubationSeconds += dt;
}

void stepHatched(Dragon& d, s64 t, float hours) {
    const bool sanctuary = d.location == Location::Sanctuary;
    const float scale = sanctuary ? 0.1f : 1.0f;
    const bool night = isNight(t);
    Needs& n = d.needs;

    // Tired dragons nap on their own in the den (and always sleep at night).
    if (!sanctuary && !night) {
        if (n.energy < 25) d.napping = true;
        else if (n.energy >= 70) d.napping = false;
    }
    const bool asleep = night || d.napping;
    const float sleepy = d.personality == Personality::Sleepy ? 1.5f : 1.0f;

    n.belly -= (asleep ? 3.0f : 6.0f) * hours * scale;
    n.shine -= 2.0f * hours * scale;
    const float playDrain = (asleep ? 1.0f : 4.0f) * (d.personality == Personality::Playful ? 0.75f : 1.0f);
    n.play -= playDrain * hours * scale;
    if (night) {
        n.energy += 12.0f * hours * sleepy;
    } else if (d.napping) {
        n.energy += 10.0f * hours * sleepy;
    } else {
        n.energy -= 3.0f * hours * scale;
    }

    n.belly = clamp100(n.belly);
    n.energy = clamp100(n.energy);
    n.shine = clamp100(n.shine);
    n.play = clamp100(n.play);

    // Dust settles over a day or two (D46): fastest where a dragon meets the floor, slowest
    // on the wings; the keepers keep Sanctuary dragons tidy.
    static const float kDirtRate[kRegionCount] = {0.8f, 0.8f, 0.9f, 1.3f, 1.1f, 1.1f, 1.2f, 0.6f};
    for (int r = 0; r < kRegionCount; ++r) d.dirt[r] = clamp100(d.dirt[r] + 2.6f * kDirtRate[r] * hours * scale);

    if (sanctuary) {
        // Keepers tend stored dragons: needs never fall below 50, mood holds.
        if (n.belly < 50) n.belly = 50;
        if (n.energy < 50) n.energy = 50;
        if (n.shine < 50) n.shine = 50;
        if (n.play < 50) n.play = 50;
        return;
    }

    d.dayLowestSum += n.lowest() * hours;
    d.dayHours += hours;

    if (moodOf(d) == Mood::Sulky) {
        d.sulkyHours += hours;
        if (d.sulkyHours >= 24) d.upset = true;
    } else if (!d.upset) {
        d.sulkyHours = 0;
    }
    if (t - d.lastVisitAt >= 3 * kDay) d.upset = true;
}

}  // namespace

float Needs::lowest() const {
    float m = belly;
    if (energy < m) m = energy;
    if (shine < m) m = shine;
    if (play < m) m = play;
    return m;
}

int stageMinDay(Stage s) {
    switch (s) {
        case Stage::Juvenile: return 4;
        case Stage::Adolescent: return 8;
        case Stage::Adult: return 14;
        default: return 0;
    }
}

int stageMinStars(Stage s) {
    switch (s) {
        case Stage::Juvenile: return 6;
        case Stage::Adolescent: return 14;
        case Stage::Adult: return 26;
        default: return 0;
    }
}

Stage stageFor(int days, int stars) {
    for (Stage s : {Stage::Adult, Stage::Adolescent, Stage::Juvenile})
        if (days >= stageMinDay(s) && stars >= stageMinStars(s)) return s;
    return Stage::Hatchling;
}

Sex rollSex(Rng& rng) { return rng.chance(1, 2) ? Sex::Male : Sex::Female; }

Dragon makeEgg(u32 id, const Genome& g, Sex sex, s64 now) {
    Dragon d;
    d.id = id;
    d.genome = g;
    d.sex = sex;
    d.stage = Stage::Egg;
    d.laidAt = now;
    d.day = dayIndex(now);
    return d;
}

bool tryHatch(Dragon& d, s64 now, Rng& rng) {
    if (d.stage != Stage::Egg || d.incubationSeconds < kIncubationSeconds) return false;
    d.stage = Stage::Hatchling;
    d.hatchedAt = now;
    d.lastVisitAt = now;
    d.needs = Needs{70, 70, 70, 70};
    d.personality = static_cast<Personality>(rng.below(static_cast<u32>(Personality::Count)));
    // Favourite food leans toward the breed's own (food index == element for now).
    d.favoriteFood = rng.chance(60, 100) ? d.genome.elementA : static_cast<u8>(rng.below(kElementCount));
    d.day = dayIndex(now);
    d.dayLowestSum = 0;
    d.dayHours = 0;
    d.dayVisited = true;
    return true;
}

void simulate(Dragon& d, s64 from, s64 now) {
    const s64 elapsed = safeElapsed(from, now);
    s64 t = now - elapsed;
    s64 remaining = elapsed;
    while (remaining > 0) {
        // Step to the next hour boundary so day roll-over is exact.
        s64 step = kHour - (t % kHour + kHour) % kHour;
        if (step > remaining) step = remaining;
        const float hours = static_cast<float>(step) / kHour;

        if (d.stage == Stage::Egg) {
            stepEgg(d, hours, static_cast<s32>(step));
        } else {
            stepHatched(d, t, hours);
        }
        t += step;
        remaining -= step;

        const s32 today = dayIndex(t);
        if (today != d.day) {
            if (d.stage != Stage::Egg) closeDay(d, today);
            else d.day = today;
            promote(d, t);
        }
    }
    promote(d, now);
}

void markVisit(Dragon& d, s64 now) {
    d.lastVisitAt = now;
    if (dayIndex(now) == d.day) d.dayVisited = true;
}

void feed(Dragon& d, float amount, bool favorite) {
    d.needs.belly = clamp100(d.needs.belly + amount * (favorite ? 1.5f : 1.0f));
    addBond(d, favorite ? 2 : 1);
}

void pet(Dragon& d, float amount) {
    d.needs.play = clamp100(d.needs.play + amount * 0.5f);
    addBond(d, d.personality == Personality::Shy ? 2 : 1);
}

void groom(Dragon& d, float amount) {
    d.needs.shine = clamp100(d.needs.shine + amount);
    for (float& dust : d.dirt) dust = clamp100(dust - amount * 1.5f);
    addBond(d, 1);
}

void cleanRegion(Dragon& d, int region, float amount) {
    if (region >= 0 && region < kRegionCount) d.dirt[region] = clamp100(d.dirt[region] - amount);
}

void bathe(Dragon& d) {
    for (float& dust : d.dirt) dust = 0;
    d.needs.shine = clamp100(d.needs.shine + 30);
    addBond(d, 1);
}

void play(Dragon& d, float amount) {
    d.needs.play = clamp100(d.needs.play + amount);
    d.needs.energy = clamp100(d.needs.energy - amount * 0.1f);
    addBond(d, 1);
}

void warmEgg(Dragon& d, float amount) {
    if (d.stage == Stage::Egg) d.warmth = clamp100(d.warmth + amount);
}

void makeUp(Dragon& d) {
    d.upset = false;
    d.sulkyHours = 0;
    addBond(d, 5);
}

Mood moodOf(const Dragon& d) {
    if (d.upset) return Mood::Upset;
    const Needs& n = d.needs;
    const float score = (n.belly + n.energy + n.shine + n.play + n.lowest()) / 5.0f;
    if (score >= 80) return Mood::Joyful;
    if (score >= 60) return Mood::Content;
    if (score >= 40) return Mood::Restless;
    return Mood::Sulky;
}

int daysSinceHatch(const Dragon& d, s64 now) {
    if (d.stage == Stage::Egg || now <= d.hatchedAt) return 0;
    return static_cast<int>((now - d.hatchedAt) / kDay);
}

float stageProgress(const Dragon& d, s64 now) {
    if (d.stage == Stage::Egg) return 0.0f;
    if (d.stage == Stage::Adult) return 1.0f;
    const Stage next = static_cast<Stage>(static_cast<int>(d.stage) + 1);
    const float span = static_cast<float>(stageMinDay(next) - stageMinDay(d.stage));
    const float days = now > d.hatchedAt ? static_cast<float>(now - d.hatchedAt) / kDay : 0.0f;
    float p = (days - stageMinDay(d.stage)) / span;
    if (p < 0) p = 0;
    if (p > 0.95f) p = 0.95f;  // the last step waits for promotion
    return p;
}

float bodyScale(const Dragon& d, s64 now) {
    struct Range {
        float lo, hi;
    };
    static constexpr Range kRanges[] = {{0.20f, 0.20f}, {0.25f, 0.45f}, {0.45f, 0.70f}, {0.70f, 1.00f}, {1.0f, 1.0f}};
    const Range& r = kRanges[static_cast<int>(d.stage)];
    return r.lo + (r.hi - r.lo) * stageProgress(d, now);
}

}  // namespace ec
