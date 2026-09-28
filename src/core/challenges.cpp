#include "core/challenges.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "core/place_layout.hpp"
#include "core/rng.hpp"
#include "core/trainer.hpp"
#include "core/valley.hpp"

namespace ec::challenge {
namespace {

constexpr float kPi = 3.14159265f;

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
float approach(float v, float target, float rate, float dt) {
    const float k = rate * dt > 1 ? 1 : rate * dt;
    return v + (target - v) * k;
}
float wrap(float a) {
    while (a > kPi) a -= 2 * kPi;
    while (a < -kPi) a += 2 * kPi;
    return a;
}
int ci(Challenge c) { return static_cast<int>(c); }
bool validCup(int cup) { return cup >= kEmber && cup <= kStarfire; }
float unit(Rng& r) { return r.next() * (1.0f / 4294967296.0f); }

}  // namespace

// ------------------------------------------------------------------------------ the stats
float statLevel(const Dragon& d, int stat) {
    if (stat < 0 || stat >= kDragonStats) return 5.0f;
    if (d.stats[stat] == 0 && d.trained[stat] == 0) return 5.0f;  // (unset: an average dragon)
    const float points = static_cast<float>(trainer::statPoints(d, stat));
    return points <= 10.0f ? points : 10.0f + (points - 10.0f) * 0.25f;
}

float statEdge(float level) { return clampf((level - 5.0f) / 5.0f, -0.8f, 1.4f); }

// ------------------------------------------------------------------------------ the cups
const char* name(Challenge c) {
    switch (c) {
        case Challenge::SkyRings: return "Sky Rings";
        case Challenge::LanternTrial: return "Lantern Trial";
        default: return "Fruit Catch";
    }
}

const char* blurb(Challenge c) {
    switch (c) {
        case Challenge::SkyRings: return "Race rival dragons round the ring course on your grown partner!";
        case Challenge::LanternTrial: return "Watch the crystal lanterns, then light them in order.";
        default: return "Flick fruit down the orchard: your dragon leaps to catch it.";
    }
}

int placeOf(Challenge c) { return c == Challenge::FruitCatch ? kPlaceOrchard : kPlaceArena; }

const char* cupName(int cup) {
    switch (cup) {
        case kEmber: return "Ember cup";
        case kFlame: return "Flame cup";
        case kBlaze: return "Blaze cup";
        case kStarfire: return "Starfire cup";
        default: return "";
    }
}

Rgb cupColour(int cup) {  // warm to bright, the last a starlit pearl
    switch (cup) {
        case kEmber: return {214, 110, 72};
        case kFlame: return {242, 150, 52};
        case kBlaze: return {245, 196, 81};
        case kStarfire: return {196, 222, 250};
        default: return {150, 140, 150};
    }
}

bool young(const Dragon& d) { return d.stage == Stage::Hatchling || d.stage == Stage::Juvenile; }

Entry entry(const SaveData& s, const Dragon* partner, Challenge c, int cup) {
    if (!validCup(cup)) return Entry::WinBefore;
    if (s.world.cups[ci(c)] < cup - 1) return Entry::WinBefore;
    if (!partner || partner->stage == Stage::Egg) return Entry::NoPartner;
    if ((c == Challenge::SkyRings || cup == kStarfire) && partner->stage != Stage::Adult) return Entry::NotGrown;
    if (cup == kBlaze && partner->stage == Stage::Hatchling) return Entry::TooYoung;
    if (!trainer::canSpend(*partner, trainer::kEnergyChallenge)) return Entry::Tired;
    return Entry::Open;
}

const char* entryText(Entry e) {
    switch (e) {
        case Entry::NoPartner: return "Bring a dragon along first.";
        case Entry::WinBefore: return "Win the cup before this one first.";
        case Entry::TooYoung: return "For a juvenile dragon or older.";
        case Entry::NotGrown: return "For a grown dragon.";
        case Entry::Tired: return "Too tired for a cup. A good sleep brings its energy back.";
        default: return "";
    }
}

CupNeeds cupNeeds(Challenge c, int cup, bool youngPartner) {
    CupNeeds n;
    if (!validCup(cup)) return n;
    n.grown = c == Challenge::SkyRings || cup == kStarfire;
    n.juvenile = !n.grown && cup == kBlaze;
    switch (c) {
        case Challenge::SkyRings: n.rivals = rivalCount(cup); break;
        case Challenge::LanternTrial: {
            const TrialSetup t = trialSetup(cup);
            n.lanterns = t.lanterns;
            n.rounds = t.rounds;
            n.hearts = t.hearts;
            break;
        }
        default: n.goal = fruitSetup(cup, youngPartner && !n.grown).goal; break;
    }
    return n;
}

// The prizes, Ember to Starfire (D89: a hard cup is worth a trip across the valley).
u32 firstPrize(int cup) {
    static constexpr u32 k[kCups] = {80, 140, 220, 350};
    return validCup(cup) ? k[cup - 1] : 0;
}
u32 dayPrize(int cup) {
    static constexpr u32 k[kCups] = {30, 45, 65, 90};
    return validCup(cup) ? k[cup - 1] : 0;
}
u32 placedPrize(int cup) {
    static constexpr u32 k[kCups] = {10, 15, 20, 30};
    return validCup(cup) ? k[cup - 1] : 0;
}
u32 winXp(int cup) {
    static constexpr u32 k[kCups] = {20, 35, 55, 80};
    return validCup(cup) ? k[cup - 1] : 0;
}

int claimBit(Challenge c, int cup) { return kClaimCup + ci(c) * kCups + cup - 1; }

bool lowerIsBetter(Challenge c) { return c == Challenge::SkyRings; }

int best(const SaveData& s, Challenge c, int cup) { return validCup(cup) ? s.world.best[ci(c)][cup - 1] : 0; }

bool ribbon(const SaveData& s, Challenge c, int cup) {
    return validCup(cup) && (s.world.ribbons >> (ci(c) * kCups + cup - 1)) & 1u;
}

int ribbonCount(const SaveData& s) {
    int n = 0;
    for (int b = 0; b < kChallenges * kCups; ++b) n += (s.world.ribbons >> b) & 1u;
    return n;
}

Reward record(SaveData& s, Challenge c, int cup, Outcome o, int score, s32 today, Dragon* partner) {
    Reward r;
    r.outcome = o;
    if (!validCup(cup)) return r;
    const int k = cup - 1;
    if (o == Outcome::Won) {
        // The first win ever pays its first prize (it can only happen once); after that the day's
        // prize, once a day per cup. Either way today's is taken.
        r.firstWin = !ribbon(s, c, cup);
        const bool fresh = trainer::claimToday(s, claimBit(c, cup), today);
        r.gleam = r.firstWin ? firstPrize(cup) : fresh ? dayPrize(cup) : 0;
        r.paidToday = !r.firstWin && !fresh;
        r.xp = r.firstWin || fresh ? winXp(cup) : kRunXp;
        s.world.ribbons |= static_cast<u16>(1u << (ci(c) * kCups + k));
        if (s.world.cups[ci(c)] < cup) s.world.cups[ci(c)] = static_cast<u8>(cup);
        trainer::count(s, kCountCups);
        if (partner) {
            r.dragonFirst = !trainer::wonCup(*partner, ci(c), cup);
            trainer::recordCup(*partner, ci(c), cup);
        }
    } else {
        // Placing: a small prize while the day's is still to be won (it doesn't take it).
        if (o == Outcome::Placed && !trainer::claimedToday(s, claimBit(c, cup), today)) r.gleam = placedPrize(cup);
        r.xp = kRunXp;
    }
    s.gleam += r.gleam;
    if (partner && partner->stage != Stage::Egg && r.xp) r.levels = trainer::gainXp(*partner, r.xp);
    if (score > 0) {  // (an unfinished Sky Rings run comes as 0: no time worth keeping)
        const u16 kept = static_cast<u16>(score > 65535 ? 65535 : score);
        u16& b = s.world.best[ci(c)][k];
        if (b == 0 || (lowerIsBetter(c) ? kept < b : kept > b)) {
            b = kept;
            r.best = true;
        }
    }
    return r;
}

// ------------------------------------------------------------------------------ Sky Rings
namespace {

// A course is drawn through its places: each a point in a place's frame, this high over the
// ground there (or the water, over the lake).
struct Way {
    u8 place;
    float x, y, up;
};
struct Design {
    Way ways[6];
    int count;
    float radius, spacing, slalom, bob, slack;
    bool toIsles;
};
// Ember: the first cup, a steady climb over the orchard and the Market to the isles' high lantern
// (the campaign's quest 6 flies it); Flame: low over the lake, past the Market to the orchard;
// Blaze: the other way round, lower, weaving, to finish skimming the lake; Starfire: the long
// one, up to the Nesting Stone's hill and down into the Market, weaving hard.
const Design kDesigns[kCups] = {
    {{{kPlaceArena, 0, 0, 34}, {kPlaceOrchard, 0, 0, 104}, {kPlaceMarket, 0, 0, 153}}, 3, 6.5f, 80, 0, 0, 1.45f, true},
    {{{kPlaceArena, 0, 0, 30}, {kPlaceLake, 0, -40, 7}, {kPlaceMarket, 0, -4, 24}, {kPlaceOrchard, 0, 2, 18}},
     4, 5.5f, 66, 6, 3, 1.3f, false},
    {{{kPlaceArena, 0, 0, 24}, {kPlaceOrchard, -4, 6, 10}, {kPlaceMarket, 0, -4, 16}, {kPlaceLake, -20, -60, 4}},
     4, 4.8f, 56, 10, 4, 1.2f, false},
    {{{kPlaceArena, 0, 0, 30}, {kPlaceOrchard, 0, 0, 22}, {kPlaceStone, 0, 0, 20}, {kPlaceMarket, 0, 0, 14}},
     4, 4.2f, 50, 13, 6, 1.1f, false},
};

float surfaceAt(const Valley& v, float x, float y) { return std::fmax(v.heightAt(x, y), v.water); }

Vec3 catmull(Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, float t) {
    const float t2 = t * t, t3 = t2 * t;
    return (p1 * 2.0f + (p2 - p0) * t + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 + (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) *
           0.5f;
}

}  // namespace

// ------------------------------------------------------------------------------ the race's flight
RaceTuning raceTuning(float wing, float stamina) {
    RaceTuning t;
    const float w = statEdge(wing);
    t.cruise *= 1.0f + 0.1f * w;
    t.top *= 1.0f + 0.1f * w;
    t.diveTop *= 1.0f + 0.08f * w;
    t.slow *= 1.0f + 0.05f * w;
    t.turnRate *= 1.0f + 0.12f * w;
    // The meter: 3.5 s of burst for an average dragon, about 5 at Stamina 10, never under 2.
    t.meter = std::fmax(2.0f, 2.0f + 0.3f * stamina);
    return t;
}

RaceTuning raceTuning(const Dragon& d) { return raceTuning(statLevel(d, kStatWing), statLevel(d, kStatStamina)); }

bool bursting(const Flight& f, const RaceInput& in) { return in.burst && !in.brake && f.stamina > 0.0f; }

void raceStep(Flight& f, const RaceInput& in, const Valley& v, float dt, const RaceTuning& t) {
    f.flapped = f.splashed = f.skimming = f.landed = f.tookOff = false;
    f.grounded = f.swimming = false;
    f.sinceFlap += dt;
    // Turning: a fast dragon turns wider, a slow (or braking) one tighter; the body leans into it.
    const float steer = clampf(in.steer, -1.0f, 1.0f);
    const float agility = clampf(t.cruise / std::fmax(f.speed, 4.0f), 0.55f, 1.5f) * (in.brake ? 1.3f : 1.0f);
    const float turn = steer * t.turnRate * agility;
    f.heading = wrap(f.heading - turn * dt);  // (right on the screen: heading goes down, as in core/flight)
    f.roll = approach(f.roll, clampf(steer * agility * 0.55f, -0.9f, 0.9f), 4, dt);
    // Speed carries: it eases back toward cruising only slowly from above (quicker from below);
    // a burst pushes toward top speed while the meter lasts, a dive toward its own, the brake down.
    const bool burst = bursting(f, in);
    float target = t.cruise, rate = f.speed > t.cruise ? 0.3f : 0.5f;
    if (burst) {
        target = t.top;
        rate = 1.4f;
        f.stamina = std::fmax(0.0f, f.stamina - dt / t.meter);
    }
    if (in.dive && !in.brake) {
        target = std::fmax(target, t.diveTop);
        rate = std::fmax(rate, 0.9f);
    }
    if (in.brake) {
        target = t.slow;
        rate = 2.2f;
    }
    f.speed = approach(f.speed, target, rate, dt);
    f.speed -= std::fabs(turn) * t.turnDrag * f.speed * dt;                               // hard turns bleed it
    f.speed -= 2.5f * clampf(f.climb / std::fmax(f.speed, 4.0f), -1.0f, 1.0f) * dt;  // climbing costs it, diving gives it
    f.speed = clampf(f.speed, 4.0f, t.diveTop + 4.0f);
    // Height: wingbeats lift (and cost a little of the meter), gliding sinks slowly, the pad
    // noses up or down, B dives.
    float targetClimb = -t.sinkRate - in.pitch * 2.5f;
    if (in.dive) targetClimb = -f.speed * 0.55f;
    if (in.flap && !in.dive && (f.flapIn -= dt) <= 0) {
        f.flapIn = t.flapEvery;
        const float power = f.stamina > 0.02f ? 1.0f : 0.4f;  // (worn out: weak wingbeats)
        f.climb += t.flapLift * power;
        f.stamina = std::fmax(0.0f, f.stamina - t.flapCost / t.meter);
        f.sinceFlap = 0;
        f.flapped = true;
    }
    if (!in.flap) f.flapIn = 0;  // the next press beats at once
    if (!in.burst && f.sinceFlap > 0.4f) f.stamina = std::fmin(1.0f, f.stamina + t.regen / t.meter * dt);  // (easing off)
    f.climb = approach(f.climb, targetClimb, in.dive ? 2.0f : 1.4f, dt);
    f.pitch = approach(f.pitch, clampf(-f.climb / std::fmax(4.0f, f.speed) * 1.2f, -0.9f, 0.6f), 5, dt);
    // Move, kept inside the valley (turned back gently at its edges).
    f.pos = f.pos + f.forward() * (f.speed * dt) + Vec3{0, 0, f.climb * dt};
    const float margin = 30.0f, lo = v.x0 + margin, hiX = v.x0 + v.size() - margin, loY = v.y0 + margin,
                hiY = v.y0 + v.size() - margin;
    if (f.pos.x < lo || f.pos.x > hiX || f.pos.y < loY || f.pos.y > hiY) {
        f.pos.x = clampf(f.pos.x, lo, hiX);
        f.pos.y = clampf(f.pos.y, loY, hiY);
        const float home = std::atan2(v.x0 + v.size() * 0.5f - f.pos.x, -(v.y0 + v.size() * 0.5f - f.pos.y));
        f.heading = wrap(f.heading + clampf(wrap(home - f.heading), -2.0f * dt, 2.0f * dt));
    }
    // The ground or the lake under it: it skims along (rubbing costs speed), never lands.
    const float bed = v.heightAt(f.pos.x, f.pos.y);
    const bool overWater = bed < v.water - 0.4f;
    const float floor = (overWater ? v.water : bed) + t.clearance;
    if (f.pos.z <= floor) {
        f.pos.z = floor;
        f.climb = std::fmax(f.climb, 0.5f);
        f.speed *= 1.0f - 0.8f * dt;
        f.skimming = overWater;
    }
}

void ringLift(Flight& f, const RaceTuning& t) {
    f.stamina = std::fmin(1.0f, f.stamina + 0.1f);
    if (f.speed < t.top) f.speed = std::fmin(t.top, f.speed + 1.0f);
}

bool makeCourse(const Valley& v, int cup, Course& out) {
    out = Course{};
    if (!validCup(cup)) return false;
    const Design& d = kDesigns[cup - 1];
    std::vector<Vec3> ways;
    for (int k = 0; k < d.count; ++k) {
        const ValleyPlaceInfo* p = v.place(d.ways[k].place);
        if (!p) return false;
        const Vec2 w = placeToWorld(*p, {d.ways[k].x, d.ways[k].y});
        ways.push_back({w.x, w.y, surfaceAt(v, w.x, w.y) + d.ways[k].up});
    }
    Vec3 lantern;
    if (d.toIsles) {  // up to the high lantern: a level run in over the island's rim, the last ring at the flame
        const ValleyPlaceInfo* isles = v.place(kPlaceIsles);
        if (!isles) return false;
        const PlaceLayout& L = placeLayout(kPlaceIsles);
        const Vec2 w = placeToWorld(*isles, {L.lantern.x, L.lantern.y});
        lantern = {w.x, w.y, isles->at.z + L.lantern.z};
        Vec3 in = lantern - ways.back();
        in.z = 0;
        in = normalize(in);
        ways.push_back(lantern - in * 70.0f + Vec3{0, 0, 4});  // (the island's rim is about 55 m out)
        ways.push_back(lantern - in * 8.0f + Vec3{0, 0, 7});
    }
    // The curve through the ways, sampled every couple of metres, and how far along each sample is.
    std::vector<Vec3> pts;
    std::vector<float> along;
    const int n = static_cast<int>(ways.size());
    for (int i = 0; i + 1 < n; ++i) {
        const Vec3 p0 = ways[i > 0 ? i - 1 : 0], p1 = ways[i], p2 = ways[i + 1], p3 = ways[i + 2 < n ? i + 2 : n - 1];
        const int steps = std::max(8, static_cast<int>(length(p2 - p1) / 2.0f));
        for (int s = (i == 0 ? 0 : 1); s <= steps; ++s) {
            const Vec3 p = catmull(p0, p1, p2, p3, static_cast<float>(s) / steps);
            along.push_back(pts.empty() ? 0.0f : along.back() + length(p - pts.back()));
            pts.push_back(p);
        }
    }
    const float total = along.back();
    auto sample = [&](float s) {
        std::size_t i = std::upper_bound(along.begin(), along.end(), s) - along.begin();
        if (i == 0) return pts.front();
        if (i >= pts.size()) return pts.back();
        const float k = (s - along[i - 1]) / std::fmax(1e-4f, along[i] - along[i - 1]);
        return lerp(pts[i - 1], pts[i], k);
    };
    // Rings along it, evenly (the last on the last way), weaving side to side and bobbing a
    // little the higher the cup; never lower than a clear pass over the ground.
    const int count = std::max(3, static_cast<int>(std::round((total - d.spacing * 0.6f) / d.spacing)) + 1);
    const float first = d.spacing * 0.6f, gap = (total - first) / (count - 1);
    for (int k = 0; k < count; ++k) {
        const float s = k + 1 < count ? first + gap * k : total;
        Vec3 at = sample(s);
        const Vec3 tangent = normalize(sample(s + 3.0f) - sample(s - 3.0f));
        Vec3 side = cross(tangent, Vec3{0, 0, 1});
        side = length(side) > 1e-3f ? normalize(side) : Vec3{1, 0, 0};
        const bool ends = k == 0 || k + 1 == count;
        if (!ends) at = at + side * (d.slalom * std::sin(k * 1.7f)) + Vec3{0, 0, d.bob * std::sin(k * 1.1f + 0.5f)};
        at.z = std::fmax(at.z, surfaceAt(v, at.x, at.y) + d.radius + 3.5f);
        out.rings.push_back({at, tangent, d.radius});
    }
    out.start = pts.front();
    const Vec3 toFirst = out.rings[0].at - out.start;
    out.heading = std::atan2(toFirst.x, -toFirst.y);
    // The way through each ring: from the one before to the one after, so a weave reads.
    for (std::size_t k = 0; k < out.rings.size(); ++k) {
        const Vec3 a = k == 0 ? out.start : out.rings[k - 1].at;
        const Vec3 b = k + 1 < out.rings.size() ? out.rings[k + 1].at : out.rings[k].at + out.rings[k].normal;
        out.rings[k].normal = normalize(b - a);
    }
    Vec3 at = out.start;
    for (const Ring& r : out.rings) {
        out.length += length(r.at - at);
        at = r.at;
    }
    out.cup = cup;
    out.toIsles = d.toIsles;
    int missed = 0;
    const float t = pilotTime(v, out, raceTuning(5, 5), steadyPilot(), &missed);
    out.par = (t > 0 ? t : out.length / 12.0f) * d.slack;
    return true;
}

RingRun::Event RingRun::step(const Course& c, Vec3 from, Vec3 to, float dt) {
    if (finished) return kNothing;
    time += dt;
    const int n = static_cast<int>(c.rings.size());
    auto crossed = [&](int k, float& off) {  // through ring k's plane the right way: how far off its middle
        const Ring& r = c.rings[k];
        const float a = dot(from - r.at, r.normal), b = dot(to - r.at, r.normal);
        if (!(a < 0 && b >= 0)) return false;
        off = length(from + (to - from) * (a / (a - b)) - r.at);
        return true;
    };
    float off = 0;
    Event e = kNothing;
    if (next < n && crossed(next, off) && off < c.rings[next].radius * 12.0f) {  // judged (far past it: not yet)
        e = off <= c.rings[next].radius ? kPassed : kMissed;
        ++(e == kPassed ? passed : missed);
        ++next;
    } else if (next + 1 < n && crossed(next + 1, off) && off <= c.rings[next + 1].radius) {
        ++missed;  // one skipped wide, the one after flown: on from there
        ++passed;
        next += 2;
        e = kPassed;
    }
    if (next >= n) finished = true;
    return e;
}

float RingRun::progress(const Course& c, Vec3 at) const {
    const int n = static_cast<int>(c.rings.size());
    if (finished || next >= n) return static_cast<float>(n);
    const Vec3 prev = next == 0 ? c.start : c.rings[next - 1].at;
    const float span = std::fmax(1.0f, length(c.rings[next].at - prev));
    return next + clampf(1.0f - length(c.rings[next].at - at) / span, 0.0f, 1.0f);
}

// ------------------------------------------------------------------------------ the pilots
PilotSkill steadyPilot() {
    PilotSkill k;
    k.line = 0.4f;
    return k;
}

PilotSkill expertPilot() {
    PilotSkill k;
    k.burst = 1.0f;
    k.brakes = true;
    k.line = 0.55f;
    return k;
}

RaceInput Pilot::fly(const Flight& f, const Course& c, int next, const RaceTuning& tune, float dt) {
    RaceInput in;
    const int n = static_cast<int>(c.rings.size());
    if (next < 0 || next >= n) return in;
    t += dt;
    const Ring& r = c.rings[next];
    // Across the ring's face, level: a blunder goes wide that way (decided once a ring).
    Vec3 across = cross(r.normal, Vec3{0, 0, 1});
    across = length(across) > 1e-3f ? normalize(across) : Vec3{1, 0, 0};
    if (decidedFor != next) {
        decidedFor = next;
        wide = skill.blunder > 0 && unit(rng) < skill.blunder;
        side = unit(rng) < 0.5f ? -1.0f : 1.0f;
        pushes = skill.burst >= 1.0f || unit(rng) < skill.burst;  // (a timid one bursts on some straights, not all)
    }
    Vec3 aim = r.at;
    if (wide) {
        aim = aim + across * (side * r.radius * 1.9f);
    } else if (next + 1 < n) {  // the racing line: inside the ring, toward the one after
        Vec3 on = c.rings[next + 1].at - r.at;
        on = on - r.normal * dot(on, r.normal);
        if (length(on) > 1e-3f) aim = aim + normalize(on) * (r.radius * skill.line);
    }
    const Vec3 d = aim - f.pos;
    const float horiz = std::sqrt(d.x * d.x + d.y * d.y), dist = length(d);
    const float err = wrap(std::atan2(d.x, -d.y) - f.heading) + skill.wobble * std::sin(t * 0.9f + phase);
    in.steer = clampf(-err * 2.5f * skill.aim, -1, 1);
    const float slope = d.z / std::fmax(horiz, 1.0f);
    in.flap = d.z > 0.5f && (slope > 0.02f || f.speed < tune.cruise * 0.7f) && f.stamina > 0.04f;
    in.dive = d.z < -8.0f && slope < -0.35f;
    in.pitch = clampf(-d.z / 10.0f, -1, 1);
    // The bend to come at the ring: from the way in to the way on to the next one.
    float bend = 0;
    if (next + 1 < n) {
        const Vec3 on = c.rings[next + 1].at - r.at;
        bend = std::fabs(wrap(std::atan2(on.x, -on.y) - std::atan2(r.at.x - f.pos.x, -(r.at.y - f.pos.y))));
    }
    // Bursts along the straights (lined up, the ring far off or the bend after it gentle), keeping
    // some of the meter for wingbeats; a canny one brakes into a hard turn.
    if (skill.burst > 0 && pushes) {
        const float reserve = 0.15f + 0.3f * (1.0f - skill.burst);
        const bool straight = std::fabs(err) < 0.18f && ((dist > 45.0f && bend < 0.9f) || bend < 0.35f);
        in.burst = straight && f.stamina > reserve && std::fabs(slope) < 0.5f;
    }
    if (skill.brakes) {
        const bool beside = std::fabs(err) > 1.0f && horiz < 40.0f;  // the ring off to the side or behind
        const bool bendAhead = bend > 0.8f && dist < 22.0f && f.speed > tune.cruise * 1.05f;
        in.brake = beside || bendAhead;
        if (in.brake) in.burst = false;
    }
    return in;
}

// ------------------------------------------------------------------------------ racers
void Racer::start(const Course& c, Vec3 at, float heading) {
    flight = Flight{};
    flight.pos = at;
    flight.heading = heading;
    flight.grounded = false;
    flight.speed = tune.cruise;
    flight.sinceFlap = 0;
    flight.stamina = 1;
    run = RingRun{};
    last = RaceInput{};
    pilot.t = 0;
    pilot.decidedFor = -1;
    (void)c;
}

RingRun::Event Racer::step(const Valley& v, const Course& c, float dt) {
    if (run.finished) {  // past the last ring: it eases to a hover, wings beating, level
        flight.speed *= 1.0f - std::fmin(1.0f, 1.8f * dt);
        flight.pos = flight.pos + flight.forward() * (flight.speed * dt);
        flight.pitch *= 1.0f - std::fmin(1.0f, 4.0f * dt);
        flight.roll *= 1.0f - std::fmin(1.0f, 4.0f * dt);
        flight.climb = 0;
        flight.sinceFlap = 0;
        last = RaceInput{};
        return RingRun::kNothing;
    }
    last = pilot.fly(flight, c, run.next, tune, dt);
    const Vec3 from = flight.pos;
    raceStep(flight, last, v, dt, tune);
    const RingRun::Event e = run.step(c, from, flight.pos, dt);
    if (e == RingRun::kPassed) ringLift(flight, tune);
    return e;
}

float finishRace(Racer r, const Valley& v, const Course& c, float limit) {
    const float dt = 1.0f / 30.0f;
    while (!r.run.finished && r.run.time < limit) r.step(v, c, dt);
    return r.run.finished ? r.run.total() : -1.0f;
}

float pilotTime(const Valley& v, const Course& c, const RaceTuning& t, const PilotSkill& skill, int* missed, Ghost* ghost,
                u32 seed) {
    Racer r;
    r.tune = t;
    r.pilot.skill = skill;
    r.pilot.rng = Rng(seed);
    r.start(c, c.start, c.heading);
    const float dt = 1.0f / 30.0f, limit = std::fmax(240.0f, c.length / 3.0f);
    if (ghost) ghost->clear();
    while (!r.run.finished && r.run.time < limit) {
        r.step(v, c, dt);
        if (ghost) ghost->record(r.run.time, r.flight.pos, r.flight.heading);
    }
    if (missed) *missed = r.run.missed;
    return r.run.finished ? r.run.total() : -1.0f;
}

// ------------------------------------------------------------------------------ the rivals
int rivalCount(int cup) { return cup <= kEmber ? 2 : kMaxRivals; }

RivalInfo rivalFor(int cup, int k) {
    // Each cup's field, slowest first: the Ember cup's two wander and never burst; the Flame cup's
    // burst a little; the Blaze cup's burst well and brake into the turns; the Starfire cup's are
    // strong, steady fliers who burst at every chance (D89: Starfire should need real skill).
    struct Row {
        const char* name;
        float wing, stamina, aim, wobble, burst;
        bool brakes;
        float blunder, line;
    };
    static const Row kRows[kCups][kMaxRivals] = {
        {{"Puddle", 3.5f, 4.0f, 0.8f, 0.12f, 0.0f, false, 0.10f, 0.3f},
         {"Sprig", 4.5f, 4.5f, 0.85f, 0.10f, 0.0f, false, 0.08f, 0.3f},
         {"", 0, 0, 0, 0, 0, false, 0, 0}},
        {{"Breeze", 4.5f, 5.0f, 0.9f, 0.08f, 0.0f, false, 0.07f, 0.4f},
         {"Tumble", 5.0f, 5.0f, 0.9f, 0.07f, 0.15f, false, 0.06f, 0.4f},
         {"Cinderwisp", 5.5f, 5.5f, 0.95f, 0.06f, 0.25f, false, 0.06f, 0.45f}},
        {{"Gale", 5.5f, 6.0f, 1.0f, 0.05f, 0.5f, true, 0.05f, 0.5f},
         {"Flicker", 6.0f, 6.0f, 1.0f, 0.05f, 0.6f, true, 0.04f, 0.5f},
         {"Swoop", 6.0f, 6.5f, 1.0f, 0.04f, 0.7f, true, 0.04f, 0.5f}},
        {{"Starling", 6.0f, 7.0f, 1.0f, 0.03f, 1.0f, true, 0.03f, 0.55f},
         {"Tempest", 6.5f, 7.0f, 1.0f, 0.03f, 1.0f, true, 0.03f, 0.55f},
         {"Aurora", 7.0f, 7.5f, 1.0f, 0.02f, 1.0f, true, 0.02f, 0.55f}},
    };
    const Row& r = kRows[validCup(cup) ? cup - 1 : 0][k >= 0 && k < kMaxRivals ? k : 0];
    RivalInfo info;
    info.name = r.name;
    info.wing = r.wing;
    info.stamina = r.stamina;
    info.skill.aim = r.aim;
    info.skill.wobble = r.wobble;
    info.skill.burst = r.burst;
    info.skill.brakes = r.brakes;
    info.skill.blunder = r.blunder;
    info.skill.line = r.line;
    return info;
}

Vec2 rivalStart(int k) {
    static constexpr Vec2 kAt[kMaxRivals] = {{7.0f, -1.5f}, {-7.0f, -1.5f}, {13.0f, -4.0f}};
    return kAt[k >= 0 && k < kMaxRivals ? k : 0];
}

Vec3 rivalStartAt(const Course& c, int k) {
    const Vec3 fwd{std::sin(c.heading), -std::cos(c.heading), 0}, right{fwd.y, -fwd.x, 0};  // (heading 0 faces -Y: its right is -X)
    const Vec2 at = rivalStart(k);
    return c.start + right * at.x + fwd * at.y;
}

int racePlace(float yours, const float* rivals, int n) {
    int place = 1;
    for (int k = 0; k < n; ++k)
        if (rivals[k] >= 0 && rivals[k] < yours) ++place;
    return place;
}

Outcome raceOutcome(bool finished, int place) {
    if (!finished) return Outcome::TryAgain;
    return place == 1 ? Outcome::Won : place == 2 ? Outcome::Placed : Outcome::TryAgain;
}

// ------------------------------------------------------------------------------ ghosts
void Ghost::record(float time, Vec3 at, float heading) {
    if (static_cast<int>(points.size()) >= kGhostMaxPoints) return;
    if (time + 1e-4f < points.size() * kGhostStep) return;
    points.push_back({at, heading});
}

bool Ghost::at(float time, Vec3& pos, float& heading) const {
    if (points.size() < 2 || time < 0 || time > duration()) return false;
    const float f = time / kGhostStep;
    const std::size_t i = std::min(points.size() - 2, static_cast<std::size_t>(f));
    const float k = clampf(f - i, 0, 1);
    pos = lerp(points[i].at, points[i + 1].at, k);
    heading = points[i].heading + wrap(points[i + 1].heading - points[i].heading) * k;
    return true;
}

namespace {
constexpr u8 kGhostMagic[4] = {'E', 'G', 'H', '1'};
s16 deci(float v) { return static_cast<s16>(std::lround(clampf(v * 10.0f, -32767, 32767))); }
}  // namespace

std::size_t ghostBytes(const Ghost& g) { return 6 + g.points.size() * 8; }

std::size_t encodeGhost(const Ghost& g, u8* out, std::size_t cap) {
    const std::size_t need = ghostBytes(g);
    if (need > cap || g.points.size() > static_cast<std::size_t>(kGhostMaxPoints)) return 0;
    std::memcpy(out, kGhostMagic, 4);
    const u16 n = static_cast<u16>(g.points.size());
    out[4] = static_cast<u8>(n & 0xFF);
    out[5] = static_cast<u8>(n >> 8);
    u8* o = out + 6;
    for (const GhostPoint& p : g.points) {
        const s16 v[4] = {deci(p.at.x), deci(p.at.y), deci(p.at.z),
                          static_cast<s16>(std::lround(clampf(wrap(p.heading) * 10000.0f, -32767, 32767)))};
        for (s16 x : v) {
            *o++ = static_cast<u8>(static_cast<u16>(x) & 0xFF);
            *o++ = static_cast<u8>(static_cast<u16>(x) >> 8);
        }
    }
    return need;
}

bool decodeGhost(const u8* data, std::size_t size, Ghost& out) {
    if (size < 6 || std::memcmp(data, kGhostMagic, 4) != 0) return false;
    const std::size_t n = data[4] | (data[5] << 8);
    if (n > static_cast<std::size_t>(kGhostMaxPoints) || size < 6 + n * 8) return false;
    out.points.resize(n);
    const u8* p = data + 6;
    auto get = [&]() {
        const s16 v = static_cast<s16>(p[0] | (p[1] << 8));
        p += 2;
        return v;
    };
    for (GhostPoint& g : out.points) {
        g.at.x = get() / 10.0f;
        g.at.y = get() / 10.0f;
        g.at.z = get() / 10.0f;
        g.heading = get() / 10000.0f;
    }
    return true;
}

// ------------------------------------------------------------------------------ the Lantern Trial
TrialSetup trialSetup(int cup) {
    switch (cup) {
        case kFlame: return {5, 5, 3, 3, 0.72f};
        case kBlaze: return {6, 5, 4, 2, 0.6f};
        case kStarfire: return {6, 6, 5, 2, 0.5f};
        default: return {4, 4, 2, 3, 0.85f};
    }
}

Vec2 trialLantern(int lanterns, int k) {
    const float step = (lanterns >= 6 ? 24.0f : lanterns == 5 ? 28.0f : 32.0f) * kPi / 180.0f;
    const float a = (k - (lanterns - 1) * 0.5f) * step;
    return {6.5f * std::sin(a), -6.5f * std::cos(a)};  // round the arena's middle, toward the stage
}

Vec2 trialYou() { return {1.9f, 3.2f}; }
Vec2 trialDragon() { return {-0.4f, 1.2f}; }  // on the arena's star, the lanterns round it

Rgb trialColour(int k) {
    static constexpr Rgb kColours[kTrialLanterns] = {{240, 120, 140}, {245, 190, 80}, {126, 206, 110},
                                                     {110, 190, 240}, {180, 136, 236}, {250, 240, 224}};
    return kColours[k >= 0 && k < kTrialLanterns ? k : 0];
}

float trialPitch(int k) {
    static constexpr float kSteps[kTrialLanterns] = {0.8f, 0.9f, 1.0f, 1.2f, 1.35f, 1.6f};
    return kSteps[k >= 0 && k < kTrialLanterns ? k : 0];
}

void Trial::begin(int cup, u32 seed) {
    *this = Trial{};
    setup = trialSetup(cup);
    hearts = setup.hearts;
    Rng rng(seed ? seed : 1);
    const int longest = std::min(kTrialLongest, setup.first + setup.rounds - 1);
    for (int i = 0; i < longest; ++i) {
        int k = static_cast<int>(rng.below(static_cast<u32>(setup.lanterns)));
        if (i > 0 && k == pattern[i - 1]) k = (k + 1 + static_cast<int>(rng.below(setup.lanterns - 1))) % setup.lanterns;
        pattern[i] = static_cast<u8>(k);  // never the same lantern twice running: each step shows
    }
}

int Trial::length() const { return std::min(kTrialLongest, setup.first + round); }

Trial::Press Trial::press(int lantern) {
    if (over) return won ? Press::Won : Press::Lost;
    if (lantern != pattern[input]) {
        input = 0;
        if (--hearts <= 0) {
            over = true;
            return Press::Lost;
        }
        return Press::Wrong;
    }
    score += 10;
    if (++input < length()) return Press::Right;
    input = 0;
    score += 25 * (round + 1);
    if (++round >= setup.rounds) {
        over = won = true;
        score += 40 * hearts;
        return Press::Won;
    }
    return Press::RoundDone;
}

Outcome trialOutcome(const Trial& t) {
    if (t.won) return Outcome::Won;
    return t.roundsCleared() * 2 >= t.setup.rounds ? Outcome::Placed : Outcome::TryAgain;
}

// ------------------------------------------------------------------------------ Fruit Catch
FruitSetup fruitSetup(int cup, bool young) {
    static constexpr int kGrown[kCups] = {1100, 1600, 2050, 2400}, kYoung[kCups] = {800, 1150, 1500, 1750};
    const int k = validCup(cup) ? cup - 1 : 0;
    const int goal = young ? kYoung[k] : kGrown[k];
    return {8, goal, goal * 7 / 10};
}

Fruit fruitFor(int throwIndex, u32 seed) {
    if (throwIndex % 4 == 3) return Fruit::Golden;
    return static_cast<Fruit>((seed + throwIndex * 7u) % 3u);
}

Rgb fruitColour(Fruit f) {
    switch (f) {
        case Fruit::Pear: return {196, 214, 86};
        case Fruit::Plum: return {150, 90, 170};
        case Fruit::Golden: return {255, 206, 70};
        default: return {226, 64, 56};
    }
}

Vec2 fruitYou() { return {0.0f, 9.0f}; }
Vec2 fruitDragon(bool youngOne) { return youngOne ? Vec2{1.3f, 10.2f} : Vec2{2.4f, 9.6f}; }

bool tossFrom(float flickX, float flickY, Vec3 hand, float heading, bool youngOne, Toss& out) {
    const float speed = std::sqrt(flickX * flickX + flickY * flickY);
    if (speed < 250.0f || flickY > -150.0f) return false;
    const float power = clampf((speed - 250.0f) / 1500.0f, 0, 1);
    const float yaw = clampf(std::atan2(flickX, -flickY), -0.45f, 0.45f);
    const float h = heading - yaw;  // to the right on the screen: to your right
    const float v0 = youngOne ? 2.2f + 7.3f * power : 5.0f + 13.0f * power;
    const float up = (youngOne ? 62.0f : 50.0f) * kPi / 180.0f;  // a young one's game: lobbed, more time to get under it
    const Vec3 fwd{std::sin(h), -std::cos(h), 0};
    out.from = hand;
    out.vel = fwd * (v0 * std::cos(up)) + Vec3{0, 0, v0 * std::sin(up)};
    return true;
}

Vec3 fruitAt(const Toss& t, float time) {
    return t.from + t.vel * time + (t.wind + Vec3{0, 0, -kGravity}) * (0.5f * time * time);
}

Vec3 windFor(int cup, int throwIndex, u32 seed) {
    static constexpr float kStrength[kCups] = {0.0f, 0.6f, 1.0f, 1.4f};  // m/s^2 at its strongest
    if (!validCup(cup) || kStrength[cup - 1] <= 0) return {};
    Rng rng((static_cast<u64>(seed) << 8) ^ (0x9E3779B9u * static_cast<u32>(throwIndex + 1)));
    const float a = unit(rng) * 2 * kPi, k = kStrength[cup - 1] * (0.45f + 0.55f * unit(rng));
    return {std::sin(a) * k, std::cos(a) * k, 0};
}

float landTime(const Toss& t, float groundZ) {
    const float h = std::fmax(0.0f, t.from.z - groundZ), vz = t.vel.z;
    return (vz + std::sqrt(vz * vz + 2 * kGravity * h)) / kGravity;
}

Catcher catcherFor(const Dragon& d, Vec3 at, float runSpeed, float size) {
    Catcher c;
    c.at = at;
    c.young = young(d);
    // Wing: its running; Wit: its reach (how soon it reads the throw, its leap, its dive), D89.
    const float wing = statEdge(statLevel(d, kStatWing)), wit = statEdge(statLevel(d, kStatWit));
    c.react = clampf(0.25f - 0.06f * wit, 0.16f, 0.32f);
    if (c.young) {  // the hop version: short hops, and a tumble at the end
        c.run = clampf(runSpeed * 1.8f, 3.8f, 6.5f) * (1.0f + 0.1f * wing);
        c.reach = std::fmax(0.35f, 0.55f * size);
        c.leap = 0.55f * (1.0f + 0.15f * wit);
        c.dive = 0.7f * (1.0f + 0.25f * wit);
    } else {
        // (a slow gallop plays quicker: every kind can win)
        c.run = clampf(runSpeed * 1.5f, 8.5f, 12.0f) * (1.0f + 0.1f * wing);
        c.reach = 1.45f * size;
        c.leap = 1.7f * (1.0f + 0.15f * wit);
        c.dive = 1.8f * (1.0f + 0.25f * wit);
    }
    return c;
}

const char* styleName(Style s) {
    switch (s) {
        case Style::Snap: return "Caught!";
        case Style::Leap: return "Leaping catch!";
        case Style::SkyLeap: return "Sky-high leap!";
        case Style::Dive: return "Diving catch!";
        case Style::Hop: return "Hop and catch!";
        case Style::Tumble: return "Tumbling catch!";
        default: return "Missed!";
    }
}

CatchPlan planCatch(const Toss& t, const Catcher& c, float groundZ) {
    CatchPlan p;
    const float land = landTime(t, groundZ);
    const float mouth = c.young ? 0.3f : 0.7f;  // its mouth reaches this far ahead of where it stands
    auto need = [&](Vec3 at, float extra) {
        const float d = std::hypot(at.x - c.at.x, at.y - c.at.y) - mouth - extra;
        return c.react + std::fmax(0.0f, d) / c.run;
    };
    auto done = [&](Style s, float tt, float extra) {
        p.style = s;
        p.t = tt;
        p.at = fruitAt(t, tt);
        p.leaveAt = c.react;
        p.arriveAt = std::fmin(tt, need(p.at, extra));
        p.jump = std::fmax(0.0f, p.at.z - groundZ - c.reach);
        p.distance = std::hypot(p.at.x - t.from.x, p.at.y - t.from.y);
        return p;
    };
    auto style = [&](float h) {
        const float jump = h - c.reach;
        if (c.young) return jump > 0.05f ? Style::Hop : Style::Snap;
        return jump > 0.6f * c.leap ? Style::SkyLeap : jump > 0.05f ? Style::Leap : Style::Snap;
    };
    // The first moment it could be under the fruit on its way down, within reach of a jump.
    for (float tt = 0.3f; tt <= land; tt += 0.02f) {
        const Vec3 at = fruitAt(t, tt);
        const float h = at.z - groundZ;
        if (t.vel.z - kGravity * tt > 0 || h > c.reach + c.leap || need(at, 0) > tt) continue;
        if (need(at, 0) < tt - 0.05f) {  // there early (the fruit still high): it waits, then leaps easily
            const float comfy = c.reach + 0.45f * c.leap;
            for (float wait = tt; wait <= land; wait += 0.02f)
                if (fruitAt(t, wait).z - groundZ <= comfy) return done(style(fruitAt(t, wait).z - groundZ), wait, 0);
        }
        return done(style(h), tt, 0);  // just in time: however high it is then
    }
    // Not in time running: a dive at the very end (a young one tumbles), low, just before it lands.
    for (float tt = std::fmax(0.3f, land - 0.4f); tt <= land; tt += 0.02f) {
        const Vec3 at = fruitAt(t, tt);
        if (at.z - groundZ <= c.reach && need(at, c.dive) <= tt) return done(c.young ? Style::Tumble : Style::Dive, tt, c.dive);
    }
    done(Style::Missed, land, 0);
    p.at.z = groundZ;
    p.arriveAt = need(p.at, 0);
    return p;
}

int catchPoints(const CatchPlan& p, bool golden, bool youngOne) {
    if (p.style == Style::Missed) return 0;
    int bonus = 0;
    switch (p.style) {
        case Style::Leap: bonus = 25; break;
        case Style::SkyLeap: bonus = 45; break;
        case Style::Dive: bonus = 60; break;
        case Style::Hop: bonus = 15; break;
        case Style::Tumble: bonus = 40; break;
        default: break;
    }
    const int points = static_cast<int>(std::lround(p.distance * (youngOne ? 25.0f : 10.0f))) + bonus;
    return golden ? points * 2 : points;
}

Outcome fruitOutcome(const FruitSetup& s, int score) {
    return score >= s.goal ? Outcome::Won : (score >= s.placed ? Outcome::Placed : Outcome::TryAgain);
}

// ------------------------------------------------------------------------------ breath
BreathLook breathFor(int element) {
    switch (element) {
        case 0: return {Breath::Flame, {255, 214, 120}, {236, 90, 40}, 0.16f, 1.2f, 0.22f, 0.55f, 0.0f};
        case 1: return {Breath::Spores, {232, 250, 160}, {130, 200, 90}, 0.35f, 0.7f, 0.1f, 0.18f, 1.2f};
        case 2: return {Breath::Gust, {244, 228, 184}, {196, 164, 116}, 0.14f, 0.0f, 0.14f, 0.34f, 0.2f};
        case 3: return {Breath::Gust, {244, 252, 255}, {176, 214, 236}, 0.12f, 0.0f, 0.14f, 0.34f, 0.2f};
        case 4: return {Breath::Mist, {226, 244, 255}, {130, 196, 236}, 0.28f, 0.3f, 0.25f, 0.9f, 0.3f};
        case 5: return {Breath::Frost, {240, 250, 255}, {150, 212, 255}, 0.18f, -0.6f, 0.1f, 0.22f, 0.1f};
        case 6: return {Breath::Light, {255, 252, 226}, {255, 214, 110}, 0.07f, 0.0f, 0.2f, 0.08f, 0.0f};
        default: return {Breath::Light, {236, 220, 255}, {166, 124, 230}, 0.09f, 0.0f, 0.2f, 0.08f, 0.0f};
    }
}

float BreathFx::unit() {
    seed_ = seed_ * 1664525u + 1013904223u;
    return (seed_ >> 8) * (1.0f / 16777216.0f);
}

void BreathFx::emit(const BreathLook& l, Vec3 from, Vec3 to, int count, float travel) {
    look = l;
    const Vec3 d = to - from;
    const float dist = length(d);
    const Vec3 dir = normalize(d);
    Vec3 side = cross(dir, Vec3{0, 0, 1});
    side = length(side) > 1e-3f ? normalize(side) : Vec3{1, 0, 0};
    const Vec3 up = cross(side, dir);
    for (int i = 0; i < count && count_ < kMax; ++i) {
        Puff& p = p_[count_++];
        const float a = (unit() * 2 - 1) * l.spread, b = (unit() * 2 - 1) * l.spread;
        const float speed = dist / std::fmax(0.1f, travel) * (0.85f + 0.3f * unit());
        p.vel = normalize(dir + side * a + up * b) * speed - Vec3{0, 0, l.rise * 0.5f};
        p.pos = from + dir * (0.2f * unit());
        p.age = -0.25f * travel * unit();  // a stream, not a single burst
        p.life = travel * (1.0f + 0.25f * unit());
        p.size0 = l.size0;
        p.size1 = l.size1;
        p.seed = static_cast<u8>(unit() * 255);
    }
}

void BreathFx::update(float dt) {
    for (int i = 0; i < count_;) {
        Puff& p = p_[i];
        p.age += dt;
        if (p.age >= p.life) {
            p_[i] = p_[--count_];
            continue;
        }
        if (p.age > 0) {
            const float w = look.wobble * std::sin(p.age * 7.0f + p.seed * 0.1f);
            p.pos = p.pos + p.vel * dt + Vec3{w * dt, -w * dt * 0.6f, look.rise * dt};
        }
        ++i;
    }
}

}  // namespace ec::challenge
