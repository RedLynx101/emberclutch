#include "core/behavior.hpp"

#include <cmath>

namespace ec {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kTurnRate = 110.0f * kPi / 180.0f;   // turning in place (shuffle), rad/s
constexpr float kSteerRate = 150.0f * kPi / 180.0f;  // steering while walking, rad/s
constexpr float kPounceLength = 1.3f;                // leap distance, adult units
constexpr float kPounceFrom = 0.65f, kPounceTo = 1.05f;  // airborne part of the "pounce" clip, seconds
constexpr float kPetHold = 1.2f;                     // a petting reaction outlasts the last stroke by this

constexpr const char* kActivityNames[] = {
    "Idle", "LookAround", "Scratch", "Wander", "Sit", "Lie", "Yawn", "TailWag", "Flutter",
    "GoNap", "Sleep", "Wake", "Eat", "Favorite", "PetHead", "PetChin", "BellyRub", "Shake", "Hop", "Pounce",
    "GoSulk", "Sulk", "MakeUp", "Greet",
};
static_assert(sizeof(kActivityNames) / sizeof(kActivityNames[0]) == static_cast<int>(Activity::Count),
              "one name per activity");

constexpr const char* kClipNames[] = {
    "idle", "look_around", "scratch", "walk", "trot", "shuffle", "carry", "sit", "sit_loop", "lie_down",
    "lie_loop", "curl_up", "sleep", "wake", "yawn", "nap_flop", "eat", "fav_wiggle", "pet_head", "pet_chin",
    "roll_over", "belly_rub", "shake", "hop", "pounce", "tail_wag", "wing_flutter", "sulk", "sulk_loop",
    "nuzzle", "greet",
};
static_assert(sizeof(kClipNames) / sizeof(kClipNames[0]) == static_cast<int>(ClipId::Count), "one name per clip");

float unit(Rng& r) { return r.next() * (1.0f / 4294967296.0f); }
float between(Rng& r, float lo, float hi) { return lo + (hi - lo) * unit(r); }

float wrapAngle(float a) {
    while (a > kPi) a -= 2 * kPi;
    while (a < -kPi) a += 2 * kPi;
    return a;
}

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// The heading that faces from `from` toward `to` (heading 0 faces -Y).
float headingTo(Vec2 from, Vec2 to) { return std::atan2(to.x - from.x, -(to.y - from.y)); }

float distance(Vec2 a, Vec2 b) { return std::hypot(a.x - b.x, a.y - b.y); }

bool ambient(Activity a) { return a <= Activity::Flutter; }
bool sulking(Activity a) { return a == Activity::GoSulk || a == Activity::Sulk || a == Activity::MakeUp; }
bool asleep(const DenBehavior& b) {
    return b.activity == Activity::Sleep || (b.activity == Activity::GoNap && b.step > 0);
}

}  // namespace

const char* activityName(Activity a) {
    return a < Activity::Count ? kActivityNames[static_cast<int>(a)] : "?";
}

const char* clipName(ClipId c) { return c < ClipId::Count ? kClipNames[static_cast<int>(c)] : "idle"; }

void DenBehavior::reset(const DenLayout& layout, u32 seed) {
    *this = DenBehavior{};
    den = layout;
    rng = Rng(seed);
    pos = den.home;
    start(Activity::Idle);
}

void DenBehavior::setClip(ClipId c, float crossfade, bool restart) {
    if (c == clip && !restart) return;
    clip = c;
    blend = crossfade;
    ++clipSerial;
    clipDone = false;
}

bool DenBehavior::turnTo(float goal, float dt) {
    const float err = wrapAngle(goal - heading);
    if (std::fabs(err) < 0.08f) return true;
    setClip(ClipId::Shuffle, 0.25f);
    heading = wrapAngle(heading + clampf(err, -kTurnRate * dt, kTurnRate * dt));
    return false;
}

bool DenBehavior::walkTo(Vec2 goal, bool trotting, float moveScale, float dt) {
    const float dist = distance(pos, goal);
    if (dist < 0.2f * moveScale + 0.05f) return true;
    const float err = wrapAngle(headingTo(pos, goal) - heading);
    if (std::fabs(err) > 0.7f) {  // face the goal first
        turnTo(headingTo(pos, goal), dt);
        return false;
    }
    heading = wrapAngle(heading + clampf(err, -kSteerRate * dt, kSteerRate * dt));
    setClip(trotting ? ClipId::Trot : ClipId::Walk, 0.3f);
    speed = trotting ? trotSpeed : walkSpeed;
    const float step = std::fmin(speed * dt, dist);
    pos.x += std::sin(heading) * step;
    pos.y -= std::cos(heading) * step;
    return false;
}

void DenBehavior::start(Activity a) {
    activity = a;
    step = 0;
    timer = 0;
    switch (a) {
        case Activity::Idle: setClip(ClipId::Idle, 0.5f); timer = between(rng, 4.0f, 9.0f); break;
        case Activity::LookAround: setClip(ClipId::LookAround, 0.3f, true); break;
        case Activity::Scratch: setClip(ClipId::Scratch, 0.3f, true); break;
        case Activity::Wander: {
            // Somewhere else on the floor, not too near.
            for (int tries = 0; tries < 8; ++tries) {
                const float ang = between(rng, -kPi, kPi), r = den.radius * std::sqrt(unit(rng)) * 0.8f;
                target = {den.home.x + std::sin(ang) * r, den.home.y + std::cos(ang) * r};
                if (distance(target, pos) > den.radius * 0.35f) break;
            }
            break;
        }
        case Activity::Sit: setClip(ClipId::Sit, 0.3f, true); timer = between(rng, 6.0f, 14.0f); break;
        case Activity::Lie: setClip(ClipId::LieDown, 0.3f, true); timer = between(rng, 8.0f, 18.0f); break;
        case Activity::Yawn: setClip(ClipId::Yawn, 0.3f, true); break;
        case Activity::TailWag: setClip(ClipId::TailWag, 0.25f); timer = between(rng, 2.0f, 3.5f); break;
        case Activity::Flutter: setClip(ClipId::WingFlutter, 0.3f, true); break;
        case Activity::GoNap: target = den.napSpot; trot = false; break;
        case Activity::Sleep: setClip(ClipId::Sleep, 0.8f); break;
        case Activity::Wake: setClip(ClipId::Wake, 0.6f, true); break;
        case Activity::Eat: setClip(ClipId::Eat, 0.35f); timer = 3.5f; break;
        case Activity::Favorite: setClip(ClipId::FavWiggle, 0.3f, true); break;
        case Activity::PetHead: setClip(ClipId::PetHead, 0.3f); break;
        case Activity::PetChin: setClip(ClipId::PetChin, 0.3f); break;
        case Activity::BellyRub: setClip(ClipId::RollOver, 0.3f, true); break;
        case Activity::Shake: setClip(ClipId::Shake, 0.25f, true); break;
        case Activity::Hop: setClip(ClipId::Hop, 0.2f, true); break;
        case Activity::Pounce: setClip(ClipId::Pounce, 0.25f, true); break;
        case Activity::GoSulk: target = den.sulkNook; trot = false; break;
        case Activity::Sulk: setClip(ClipId::Sulk, 0.4f, true); break;
        case Activity::MakeUp: setClip(ClipId::Nuzzle, 0.5f); timer = 3.0f; break;
        case Activity::Greet: setClip(ClipId::Greet, 0.3f, true); break;
        case Activity::Count: break;
    }
}

void DenBehavior::chooseAmbient(const Dragon& d, float moveScale) {
    // Weights for what to do next, coloured by mood, personality and tiredness
    // (docs/design/game-design.md sections 3.3 and 3.5).
    float w[9] = {0, 3, 1, 4, 2, 1, 1, 1, 0.5f};  // Idle, LookAround, Scratch, Wander, Sit, Lie, Yawn, TailWag, Flutter
    enum { kLook = 1, kScratch, kWander, kSit, kLie, kYawn, kWag, kFlutter };
    switch (moodOf(d)) {
        case Mood::Joyful: w[kWag] += 3; w[kFlutter] += 1.5f; w[kWander] += 1; break;
        case Mood::Restless: w[kWander] += 5; w[kLook] += 2; break;
        case Mood::Sulky: w[kLie] += 4; w[kSit] += 2; w[kWander] = 0.5f; w[kWag] = 0; break;
        default: break;
    }
    switch (d.personality) {
        case Personality::Sleepy: w[kYawn] += 2; w[kLie] += 2; break;
        case Personality::Playful: w[kWander] += 2; w[kWag] += 2; break;
        case Personality::Curious: w[kLook] += 3; w[kWander] += 2; break;
        case Personality::Proud: w[kFlutter] += 2; w[kSit] += 1; break;
        case Personality::Shy: w[kLook] += 1; w[kSit] += 1; break;
        case Personality::Brave: w[kWander] += 1; w[kFlutter] += 1; break;
        default: break;
    }
    if (d.needs.energy < 40) {
        w[kLie] += 2;
        w[kYawn] += 2;
    }
    float total = 0;
    for (int i = 1; i < 9; ++i) total += w[i];
    float pick = unit(rng) * total;
    int choice = kLook;
    for (int i = 1; i < 9; ++i) {
        if (pick < w[i]) {
            choice = i;
            break;
        }
        pick -= w[i];
    }
    const Activity next = static_cast<Activity>(choice);
    if (next == Activity::Wander) {
        const Mood m = moodOf(d);
        trot = (m == Mood::Joyful || d.personality == Personality::Playful) && rng.chance(2, 5);
    }
    // Drifted far from the middle: come back toward the player.
    if (next == Activity::Wander && distance(pos, den.home) > den.radius * 0.6f && rng.chance(1, 2)) {
        start(Activity::Wander);
        target = {den.home.x + between(rng, -0.8f, 0.8f), den.home.y + between(rng, -0.6f, 0.6f)};
        return;
    }
    (void)moveScale;
    start(next);
}

void DenBehavior::update(const Dragon& d, bool night, float moveScale, float dt) {
    speed = 0;
    if (petTimer > 0) petTimer -= dt;
    const bool bedtime = d.napping || night;

    // Priorities that interrupt everyday life.
    if (d.upset && !sulking(activity)) {
        start(Activity::GoSulk);
    } else if (!d.upset && (activity == Activity::GoSulk || activity == Activity::Sulk)) {
        start(Activity::Greet);  // made up some other way (care, dev menu)
    } else if (bedtime && ambient(activity)) {
        start(Activity::GoNap);
    }

    switch (activity) {
        case Activity::Idle:
            // Idle dragons turn to face the player now and then.
            if (step == 1) {
                if (turnTo(0.0f, dt)) {
                    step = 0;
                    setClip(ClipId::Idle, 0.3f);
                }
                break;
            }
            timer -= dt;
            if (timer <= 0) {
                if (std::fabs(wrapAngle(heading)) > 1.2f && rng.chance(1, 2)) {
                    step = 1;
                    timer = between(rng, 3.0f, 6.0f);
                } else {
                    chooseAmbient(d, moveScale);
                }
            }
            break;
        case Activity::LookAround:
        case Activity::Scratch:
        case Activity::Yawn:
        case Activity::Flutter:
        case Activity::Favorite:
        case Activity::Shake:
        case Activity::Hop:
        case Activity::Greet:
        case Activity::Wake:
            if (clipDone) start(Activity::Idle);
            break;
        case Activity::Wander:
            if (walkTo(target, trot, moveScale, dt)) start(Activity::Idle);
            break;
        case Activity::Sit:
        case Activity::Lie:
            if (step == 0) {
                if (clipDone) {
                    step = 1;
                    setClip(activity == Activity::Sit ? ClipId::SitLoop : ClipId::LieLoop, 0.2f);
                }
            } else if ((timer -= dt) <= 0) {
                start(Activity::Idle);
                blend = activity == Activity::Sit ? 0.7f : 0.9f;  // getting up takes a moment
            }
            break;
        case Activity::TailWag:
            if ((timer -= dt) <= 0) start(Activity::Idle);
            break;
        case Activity::GoNap:
            if (step == 0) {
                if (walkTo(target, false, moveScale, dt)) {
                    step = 1;
                    setClip(ClipId::LieDown, 0.3f, true);
                }
            } else if (step == 1 && clipDone) {
                step = 2;
                setClip(ClipId::CurlUp, 0.2f, true);
            } else if (step == 2 && clipDone) {
                start(Activity::Sleep);
            }
            break;
        case Activity::Sleep:
            if (!bedtime) start(Activity::Wake);
            break;
        case Activity::Eat:
            if ((timer -= dt) <= 0) start(favorite ? Activity::Favorite : Activity::Idle);
            break;
        case Activity::PetHead:
        case Activity::PetChin:
            if (petTimer <= 0) {
                start(Activity::Idle);
                blend = 0.5f;
            }
            break;
        case Activity::BellyRub:
            if (step == 0) {
                if (clipDone) {
                    step = 1;
                    setClip(ClipId::BellyRub, 0.2f);
                }
            } else if (petTimer <= 0) {
                start(Activity::Idle);
                blend = 1.0f;  // roll back onto its feet
            }
            break;
        case Activity::Pounce: {
            // The clip only lifts; the leap's distance moves the dragon itself.
            const float before = timer;
            timer += dt;
            const float from = clampf(before, kPounceFrom, kPounceTo), to = clampf(timer, kPounceFrom, kPounceTo);
            const float travel = kPounceLength * moveScale * (to - from) / (kPounceTo - kPounceFrom);
            pos.x += std::sin(heading) * travel;
            pos.y -= std::cos(heading) * travel;
            if (clipDone) start(Activity::Idle);
            break;
        }
        case Activity::GoSulk:
            if (step == 0) {
                if (walkTo(target, false, moveScale, dt)) step = 1;
            } else if (turnTo(kPi, dt)) {  // turn its back to the player
                start(Activity::Sulk);
            }
            break;
        case Activity::Sulk:
            if (step == 0 && clipDone) {
                step = 1;
                setClip(ClipId::SulkLoop, 0.3f);
            }
            break;
        case Activity::MakeUp:
            if ((timer -= dt) <= 0) start(Activity::Greet);
            break;
        case Activity::Count:
            start(Activity::Idle);
            break;
    }

    // Stay on the floor.
    const float r = distance(pos, den.home);
    if (r > den.radius) {
        pos.x = den.home.x + (pos.x - den.home.x) * den.radius / r;
        pos.y = den.home.y + (pos.y - den.home.y) * den.radius / r;
    }
    clipDone = false;  // consumed
}

void DenBehavior::care(Care c, const Dragon& d, PetZone zone) {
    if (asleep(*this) || activity == Activity::Wake) return;
    if (d.upset || sulking(activity)) {
        if (c == Care::MakeUp) start(Activity::MakeUp);
        return;  // an upset dragon turns away from everything else
    }
    switch (c) {
        case Care::Pet: {
            petTimer = kPetHold;
            petZone = zone;
            const Activity want = zone == PetZone::Chin    ? Activity::PetChin
                                  : zone == PetZone::Belly ? Activity::BellyRub
                                                           : Activity::PetHead;
            if (activity != want) start(want);
            break;
        }
        case Care::Feed:
        case Care::FeedFavorite:
            favorite = c == Care::FeedFavorite;
            start(Activity::Eat);
            break;
        case Care::Groom: start(Activity::Shake); break;
        case Care::Play:
            start(d.personality == Personality::Playful || rng.chance(1, 2) ? Activity::Pounce : Activity::Hop);
            break;
        case Care::MakeUp: start(Activity::MakeUp); break;
        case Care::Greet: start(Activity::Greet); break;
    }
}

}  // namespace ec
