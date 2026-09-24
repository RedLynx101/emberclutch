#include "core/behavior.hpp"

#include <cmath>

#include "core/props.hpp"

namespace ec {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kTurnRate = 110.0f * kPi / 180.0f;   // turning in place (shuffle), rad/s
constexpr float kSteerRate = 150.0f * kPi / 180.0f;  // steering while walking, rad/s
constexpr float kPounceLength = 1.3f;                // leap distance, adult units
constexpr float kPounceFrom = 0.65f, kPounceTo = 1.05f;  // airborne part of the "pounce" clip, seconds
constexpr float kPetHold = 1.2f;                     // a petting reaction outlasts the last stroke by this
constexpr float kClearance = 0.8f;                   // body room around obstacles (adult units, x size)
constexpr float kWanderClearance = 1.6f;             // wander targets keep further off
constexpr float kBodyRadius = 1.2f;                  // how close two dragons come (adult units, x size)
constexpr float kGrabAt = 0.35f;                     // into the pick-up and leap clips: the jaw closes on the ball
constexpr float kDropAt = 0.45f;                     // into "drop_wait": the ball falls from its mouth
constexpr float kGroomHold = 1.6f;                   // standing for grooming outlasts the last stroke by this
constexpr float kBathMax = 20.0f;                    // it hops out on its own after this long

constexpr const char* kActivityNames[] = {
    "Idle", "LookAround", "Scratch", "Wander", "Sit", "Lie", "Yawn", "TailWag", "Flutter",
    "GoNap", "Sleep", "Wake", "Eat", "Favorite", "PetHead", "PetChin", "BellyRub", "Shake", "Hop", "Pounce",
    "GoSulk", "Sulk", "MakeUp", "Greet",
    "Fetch", "HandFeed", "Refuse", "Bath", "Groomed", "Kick", "Sneeze", "PullAway", "Come",
};
static_assert(sizeof(kActivityNames) / sizeof(kActivityNames[0]) == static_cast<int>(Activity::Count),
              "one name per activity");

constexpr const char* kClipNames[] = {
    "idle", "look_around", "scratch", "walk", "trot", "shuffle", "carry", "sit", "sit_loop", "lie_down",
    "lie_loop", "curl_up", "sleep", "wake", "yawn", "nap_flop", "eat", "fav_wiggle", "pet_head", "pet_chin",
    "roll_over", "belly_rub", "shake", "hop", "pounce", "tail_wag", "wing_flutter", "sulk", "sulk_loop",
    "nuzzle", "greet",
    "pick_up", "drop_wait", "leap_catch", "leg_kick", "sniff_refuse", "lift_wing", "sneeze", "pull_away",
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

// Distance from p to the segment a-b.
float segmentDistance(Vec2 a, Vec2 b, Vec2 p) {
    const float dx = b.x - a.x, dy = b.y - a.y;
    const float len2 = dx * dx + dy * dy;
    const float t = len2 > 1e-6f ? clampf(((p.x - a.x) * dx + (p.y - a.y) * dy) / len2, 0.0f, 1.0f) : 0.0f;
    return distance(p, {a.x + dx * t, a.y + dy * t});
}

// Everything solid on the floor, with the room this dragon keeps from it: obstacles with
// `margin`, other dragons with their body and this one's.
template <typename Fn>
void eachSolid(const DenBehavior& b, float margin, Fn fn) {
    for (const DenObstacle& o : b.den.obstacles) fn(o.at, o.radius + margin);
    for (int i = 0; i < b.crowdCount; ++i)
        fn(b.crowd[i].at, b.crowd[i].radius + std::fmax(margin, kBodyRadius * b.size));
}

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

void DenBehavior::reset(const DenLayout& layout, u32 seed, int place) {
    *this = DenBehavior{};
    den = layout;
    spot = static_cast<u8>(place >= 0 && place < DenLayout::kSpots ? place : 0);
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

bool DenBehavior::clearAt(Vec2 p, float margin) const {
    bool clear = true;
    eachSolid(*this, margin, [&](Vec2 at, float room) { clear = clear && distance(p, at) >= room; });
    return clear;
}

bool DenBehavior::clearPath(Vec2 a, Vec2 b, float margin) const {
    bool clear = true;
    eachSolid(*this, margin, [&](Vec2 at, float room) { clear = clear && segmentDistance(a, b, at) >= room; });
    return clear;
}

Vec2 DenBehavior::steerTarget(Vec2 goal) const {
    Vec2 via = goal;
    bool found = false;
    eachSolid(*this, kClearance * size, [&](Vec2 at, float r) {
        if (found || distance(goal, at) < r || segmentDistance(pos, goal, at) >= r) return;
        const float dx = goal.x - pos.x, dy = goal.y - pos.y;
        const float len = std::hypot(dx, dy);
        if (len < 1e-4f) return;
        // Pass just outside it, on the side of the path the dragon is already on.
        const Vec2 left{-dy / len, dx / len};
        const float s = (at.x - pos.x) * left.x + (at.y - pos.y) * left.y > 0 ? -1.0f : 1.0f;
        via = {at.x + left.x * s * (r + 0.3f), at.y + left.y * s * (r + 0.3f)};
        found = true;
    });
    return via;
}

bool DenBehavior::walkTo(Vec2 goal, bool trotting, float moveScale, float dt) {
    const float dist = distance(pos, goal);
    if (dist < 0.2f * moveScale + 0.05f) return true;
    // Another dragon is standing on the spot: close enough.
    for (int i = 0; i < crowdCount; ++i)
        if (distance(goal, crowd[i].at) < crowd[i].radius + kBodyRadius * size &&
            dist < crowd[i].radius + kBodyRadius * size + 0.3f)
            return true;
    const Vec2 via = steerTarget(goal);
    const float err = wrapAngle(headingTo(pos, via) - heading);
    if (std::fabs(err) > 0.7f) {  // face the way first
        turnTo(headingTo(pos, via), dt);
        return false;
    }
    heading = wrapAngle(heading + clampf(err, -kSteerRate * dt, kSteerRate * dt));
    setClip(trotting ? ClipId::Trot : walkClip, 0.3f);
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
            // Somewhere else on the floor, not too near, clear of the hearth and the hoard.
            bool found = false;
            for (int tries = 0; tries < 12 && !found; ++tries) {
                const float ang = between(rng, -kPi, kPi), r = den.radius * std::sqrt(unit(rng)) * 0.8f;
                target = {den.home.x + std::sin(ang) * r, den.home.y + std::cos(ang) * r};
                found = distance(target, pos) > den.radius * 0.35f && clearAt(target, kWanderClearance * size);
            }
            if (!found) start(Activity::LookAround);
            break;
        }
        case Activity::Sit: setClip(ClipId::Sit, 0.3f, true); timer = between(rng, 6.0f, 14.0f); break;
        case Activity::Lie: setClip(ClipId::LieDown, 0.3f, true); timer = between(rng, 8.0f, 18.0f); break;
        case Activity::Yawn: setClip(ClipId::Yawn, 0.3f, true); break;
        case Activity::TailWag: setClip(ClipId::TailWag, 0.25f); timer = between(rng, 2.0f, 3.5f); break;
        case Activity::Flutter: setClip(ClipId::WingFlutter, 0.3f, true); break;
        case Activity::GoNap: target = den.beds[spot]; trot = false; break;
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
        case Activity::GoSulk: target = den.sulkSpots[spot]; trot = false; break;
        case Activity::Sulk: setClip(ClipId::Sulk, 0.4f, true); break;
        case Activity::MakeUp: setClip(ClipId::Nuzzle, 0.5f); timer = 3.0f; break;
        case Activity::Greet: setClip(ClipId::Greet, 0.3f, true); break;
        case Activity::Fetch:
            walkClip = ClipId::Walk;
            holdingBall = false;
            fumbled = false;
            setClip(ClipId::Idle, 0.2f);  // no ball in play: fetch() goes back to idle
            break;
        case Activity::HandFeed: setClip(ClipId::Idle, 0.3f); timer = 1.5f; break;
        case Activity::Refuse: setClip(ClipId::SniffRefuse, 0.2f, true); break;
        case Activity::Bath: target = den.tub; trot = false; timer = 0; break;
        case Activity::Groomed: setClip(ClipId::Idle, 0.3f); petTimer = kGroomHold; break;
        case Activity::Kick: setClip(ClipId::LegKick, 0.2f, true); break;
        case Activity::Sneeze: setClip(ClipId::Sneeze, 0.15f, true); break;
        case Activity::PullAway: setClip(ClipId::PullAway, 0.15f, true); petTimer = 0; break;
        case Activity::Come:
            target = {den.player.x, den.player.y + 0.8f};
            trot = distance(pos, target) > 3.0f;
            break;
        case Activity::Count: break;
    }
}

void DenBehavior::grab() {
    holdingBall = true;
    if (ball) {
        ball->held = true;
        ball->resting = true;
    }
}

// Fetch (care interactions §7): watch the throw, chase the ball (or leap for it), pick it up,
// carry it back to the player, drop it at their feet and wait for the next throw.
void DenBehavior::fetch(const Dragon& d, float moveScale, float dt) {
    timer += dt;
    if (!ball || !ball->active) {
        holdingBall = false;
        walkClip = ClipId::Walk;
        start(Activity::Idle);
        return;
    }
    const Vec2 b{ball->pos.x, ball->pos.y};
    const bool airborne = !ball->held && ball->pos.z > ball->radius + 0.25f;
    const float reach = 0.9f * size + ball->radius;
    const bool canLeap = d.stage >= Stage::Juvenile && airborne && distance(pos, b) < reach * 1.6f && ball->vel.z < 0.5f;
    auto leap = [&] {
        step = 6;
        timer = 0;
        heading = headingTo(pos, b);
        setClip(ClipId::LeapCatch, 0.15f, true);
    };
    switch (step) {
        case 0:  // watch it fly
            if (canLeap) {
                leap();
                break;
            }
            turnTo(headingTo(pos, b), dt);
            if (timer > 0.45f || !airborne) {
                step = 1;
                timer = 0;
            }
            break;
        case 1: {  // chase it: where it rests, or where it's rolling to
            if (canLeap) {
                leap();
                break;
            }
            if (distance(pos, b) < reach && !airborne) {
                step = 2;
                timer = 0;
                heading = headingTo(pos, b);
                setClip(ClipId::PickUp, 0.2f, true);
                break;
            }
            walkTo(ball->resting ? b : ballHeading(*ball, 0.5f), true, moveScale, dt);
            if (timer > 12.0f) start(Activity::Idle);  // lost interest
            break;
        }
        case 2:  // pick it up
            if (!holdingBall && timer >= kGrabAt) grab();
            if (clipDone) {
                timer = 0;
                walkClip = ClipId::Carry;
                if (d.personality == Personality::Playful && rng.chance(1, 4)) {  // keep-away!
                    step = 7;
                    const float ang = between(rng, -kPi, kPi);
                    target = {den.home.x + std::sin(ang) * den.radius * 0.6f, den.home.y + std::cos(ang) * den.radius * 0.5f};
                } else {
                    step = 3;
                }
            }
            break;
        case 3: {  // carry it back; a shy one stops a little further off
            const Vec2 drop{den.player.x, den.player.y + (d.personality == Personality::Shy ? 1.6f : 0.7f)};
            walkClip = ClipId::Carry;
            if (d.stage == Stage::Hatchling && !fumbled && holdingBall && rng.chance(1, 240)) {  // oops
                fumbled = true;
                holdingBall = false;
                dropBall = true;
                walkClip = ClipId::Walk;
                step = 1;
                timer = 0;
                break;
            }
            if (walkTo(drop, false, moveScale, dt)) {
                step = 4;
                timer = 0;
            }
            break;
        }
        case 4:  // face the player
            if (turnTo(0.0f, dt)) {
                step = 5;
                timer = 0;
                walkClip = ClipId::Walk;
                setClip(ClipId::DropWait, 0.25f, true);
            }
            break;
        case 5:  // drop it at their feet, wag, and wait for the next throw
            if (holdingBall && timer >= kDropAt) {
                holdingBall = false;
                dropBall = true;
            }
            if (timer > 4.5f) start(Activity::Idle);
            break;
        case 6:  // leap for it
            if (!holdingBall && timer >= kGrabAt && distance(pos, b) < reach * 2.0f) grab();
            if (clipDone) {
                timer = 0;
                step = holdingBall ? 3 : 1;
                walkClip = holdingBall ? ClipId::Carry : ClipId::Walk;
            }
            break;
        case 7:  // keep-away: trot off with it and sit, until called (or it gives up)
            walkClip = ClipId::Carry;
            if (walkTo(target, true, moveScale, dt) && clip != ClipId::SitLoop) setClip(ClipId::SitLoop, 0.3f);
            if (timer > 7.0f) {
                step = 3;
                timer = 0;
            }
            break;
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
    size = moveScale;
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
        case Activity::Fetch:
            fetch(d, moveScale, dt);
            break;
        case Activity::HandFeed:  // face the player and wait for the next bite
            if (turnTo(0.0f, dt) && clip != ClipId::Idle) setClip(ClipId::Idle, 0.25f);
            if ((timer -= dt) <= 0) start(Activity::Idle);
            break;
        case Activity::Refuse:
        case Activity::Sneeze:
        case Activity::PullAway:
            if (clipDone) start(Activity::Idle);
            break;
        case Activity::Kick:
            if (clipDone) {
                start(Activity::PetHead);
                petTimer = kPetHold;
            }
            break;
        case Activity::Bath:
            timer += dt;
            if (step == 0) {  // walk to the tub, face the player, hop in
                if (walkTo(target, false, moveScale, dt) && turnTo(0.0f, dt)) {
                    step = 1;
                    setClip(ClipId::Hop, 0.2f, true);
                }
            } else if (step == 1) {
                if (clipDone) {
                    step = 2;
                    timer = 0;
                    setClip(ClipId::Sit, 0.25f, true);
                }
            } else if (step == 2) {  // sitting in the water
                if (clipDone && clip == ClipId::Sit) setClip(ClipId::SitLoop, 0.2f);
                if (timer > kBathMax) {
                    step = 3;
                    setClip(ClipId::Hop, 0.25f, true);
                }
            } else if (step == 3) {  // hop out...
                if (clipDone) {
                    step = 4;
                    setClip(ClipId::Shake, 0.2f, true);
                }
            } else if (clipDone) {  // ...and shake off
                start(Activity::Idle);
            }
            break;
        case Activity::Groomed:
            if (step == 0) {  // show a flank to the camera, face half turned toward it
                if (turnTo(groomSide * 1.25f, dt)) {
                    step = 1;
                    setClip(ClipId::Idle, 0.25f);
                }
            } else if (step == 1) {
                if (std::fabs(wrapAngle(groomSide * 1.25f - heading)) > 0.3f) step = 0;  // asked to turn
            } else if (step == 2) {  // sitting up for the belly
                if (clipDone && clip == ClipId::Sit) setClip(ClipId::SitLoop, 0.2f);
            } else if (step == 3 && clipDone) {  // the wing goes back down
                step = 1;
                setClip(ClipId::Idle, 0.3f);
            }
            if (petTimer <= 0) start(Activity::Shake);  // done: shake off the loose scales
            break;
        case Activity::Come:
            if (step == 0) {
                if (walkTo(target, trot, moveScale, dt)) step = 1;
            } else if (step == 1) {
                if (turnTo(0.0f, dt)) {
                    step = 2;
                    timer = 5.0f;
                    setClip(ClipId::Sit, 0.3f, true);
                }
            } else {
                if (clipDone && clip == ClipId::Sit) setClip(ClipId::SitLoop, 0.2f);
                if ((timer -= dt) <= 0) start(Activity::Idle);
            }
            break;
        case Activity::Count:
            start(Activity::Idle);
            break;
    }

    // Stay on the floor, and out of the solid things on it.
    const float r = distance(pos, den.home);
    if (r > den.radius) {
        pos.x = den.home.x + (pos.x - den.home.x) * den.radius / r;
        pos.y = den.home.y + (pos.y - den.home.y) * den.radius / r;
    }
    auto pushOut = [&](Vec2 at, float room) {
        const float d = distance(pos, at);
        if (d < room && d > 1e-4f) {
            pos.x = at.x + (pos.x - at.x) * room / d;
            pos.y = at.y + (pos.y - at.y) * room / d;
        }
    };
    for (const DenObstacle& o : den.obstacles) pushOut(o.at, o.radius + kClearance * size);
    // ...and out of the other dragons. One lying down, eating or sulking stays put: the
    // others make way around it.
    const bool settled = asleep(*this) || activity == Activity::Sulk || activity == Activity::Eat ||
                         activity == Activity::BellyRub || activity == Activity::MakeUp ||
                         activity == Activity::Groomed || activity == Activity::HandFeed ||
                         (activity == Activity::Bath && step > 0) ||
                         ((activity == Activity::Sit || activity == Activity::Lie) && step == 1);
    if (!settled)
        for (int i = 0; i < crowdCount; ++i) pushOut(crowd[i].at, crowd[i].radius + kBodyRadius * size);
    clipDone = false;  // consumed
}

void shareCrowd(DenBehavior* const* dragons, int count) {
    for (int i = 0; i < count; ++i) {
        DenBehavior& b = *dragons[i];
        b.crowdCount = 0;
        for (int j = 0; j < count && b.crowdCount < DenLayout::kSpots - 1; ++j)
            if (j != i) b.crowd[b.crowdCount++] = {dragons[j]->pos, kBodyRadius * dragons[j]->size};
    }
}

float DenBehavior::eyesClosed() const {
    switch (activity) {
        case Activity::Sleep:
            return 1.0f;
        case Activity::GoNap:
            return step == 2 ? 1.0f : 0.0f;  // curling up: drifting off
        case Activity::PetHead:
        case Activity::PetChin:
            return 0.6f;
        case Activity::BellyRub:
            return step == 1 ? 0.6f : 0.0f;
        case Activity::Yawn:
            return 0.5f;
        case Activity::Kick:
            return 0.7f;
        case Activity::Groomed:
            return 0.3f;  // content
        case Activity::Bath:
            return step == 2 ? 0.35f : 0.0f;
        case Activity::Sneeze:
            return 0.8f;
        default:
            return 0.0f;
    }
}

float DenBehavior::lookWeight() const {
    switch (activity) {
        case Activity::Idle:
        case Activity::Sit:
        case Activity::TailWag:
        case Activity::Greet:
        case Activity::Favorite:
            return 1.0f;
        case Activity::Lie:
        case Activity::MakeUp:
        case Activity::PetHead:
            return 0.7f;
        case Activity::Wander:
        case Activity::BellyRub:
            return 0.3f;
        case Activity::Fetch:
            return step == 5 ? 1.0f : 0.0f;  // waiting for the next throw; otherwise eyes on the ball
        case Activity::Come:
            return step == 2 ? 1.0f : 0.4f;
        case Activity::Groomed:
        case Activity::Kick:
            return 0.5f;
        case Activity::Bath:
            return step == 2 ? 0.8f : 0.0f;
        default:
            return 0.0f;  // eating, sleeping, sulking, its own business (hand-feeding looks at the food)
    }
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
        case Care::Throw:
            if (!ball) break;
            // Too tired (or a sleepy dragon, sometimes): it just watches the ball go.
            if (d.needs.energy < 20 || (d.personality == Personality::Sleepy && d.needs.energy < 45 && rng.chance(1, 2)))
                start(Activity::LookAround);
            else
                start(Activity::Fetch);
            break;
        case Care::Call:
            if (activity == Activity::Fetch && step == 7) {  // playing keep-away: bring it back
                step = 3;
                timer = 0;
            } else if (activity != Activity::Fetch && activity != Activity::Bath) {
                start(Activity::Come);
            }
            break;
        case Care::OfferFood:
            if (activity != Activity::HandFeed) start(Activity::HandFeed);
            timer = 1.5f;
            break;
        case Care::Bath:
            if (activity != Activity::Bath) start(Activity::Bath);
            break;
        case Care::BathDone:
            if (activity == Activity::Bath && step <= 2) {
                step = 3;
                setClip(ClipId::Hop, 0.25f, true);
            }
            break;
        case Care::GroomBody:
        case Care::GroomBelly:
        case Care::GroomWing:
            if (activity != Activity::Groomed) start(Activity::Groomed);
            petTimer = kGroomHold;
            if (c == Care::GroomBelly && step < 2) {
                step = 2;
                setClip(ClipId::Sit, 0.3f, true);
            } else if (c == Care::GroomWing && step == 1) {
                step = 3;
                setClip(ClipId::LiftWing, 0.25f, true);
            }
            break;
        case Care::SweetSpot:
            if (activity != Activity::Kick) start(Activity::Kick);
            break;
        case Care::Poke:
            start(d.personality == Personality::Shy ? Activity::PullAway : Activity::Sneeze);
            break;
        case Care::Rough:
            if (activity != Activity::PullAway) start(Activity::PullAway);
            break;
    }
}

void DenBehavior::feedBite(bool disliked, bool last, bool favourite) {
    if (disliked) {
        start(Activity::Refuse);
    } else if (last) {
        favorite = favourite;
        start(favourite ? Activity::Favorite : Activity::Idle);
    } else if (activity == Activity::HandFeed) {
        timer = 1.5f;
    }
}

}  // namespace ec
