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
constexpr float kToyHold = 0.6f;                     // batting or tugging outlasts the last touch by this
constexpr float kBowlHungry = 35;                    // Belly below this sends it to the food bowl (core/items)
constexpr int kFeather = 0, kRope = 1, kOrb = 2, kBowl = 3;  // DenToys

constexpr const char* kActivityNames[] = {
    "Idle", "LookAround", "Scratch", "Wander", "Sit", "Lie", "Yawn", "TailWag", "Flutter",
    "GoNap", "Sleep", "Wake", "Eat", "Favorite", "PetHead", "PetChin", "BellyRub", "Shake", "Hop", "Pounce",
    "GoSulk", "Sulk", "MakeUp", "Greet",
    "Fetch", "HandFeed", "Refuse", "Bath", "Groomed", "Kick", "Sneeze", "PullAway", "Come", "Hatch",
    "Chase", "Flee", "Nuzzle", "Bask", "Bat", "Tug", "ToyRun", "Play", "Bowl", "TugWar",
};
static_assert(sizeof(kActivityNames) / sizeof(kActivityNames[0]) == static_cast<int>(Activity::Count),
              "one name per activity");

constexpr const char* kClipNames[] = {
    "idle", "look_around", "scratch", "walk", "trot", "shuffle", "carry", "sit", "sit_loop", "lie_down",
    "lie_loop", "curl_up", "sleep", "wake", "yawn", "nap_flop", "eat", "fav_wiggle", "pet_head", "pet_chin",
    "roll_over", "belly_rub", "shake", "hop", "pounce", "tail_wag", "wing_flutter", "sulk", "sulk_loop",
    "nuzzle", "greet",
    "pick_up", "drop_wait", "leap_catch", "leg_kick", "sniff_refuse", "lift_wing", "sneeze", "pull_away",
    "paw_bat", "tug",
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
    // Another dragon is standing on the spot: close enough (not when curling up beside it).
    for (int i = 0; i < crowdCount && !(activity == Activity::GoNap && snuggle); ++i)
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
    speed = (trotting ? trotSpeed : walkSpeed) * gait;
    const float step = std::fmin(speed * dt, dist);
    pos.x += std::sin(heading) * step;
    pos.y -= std::cos(heading) * step;
    return false;
}

void DenBehavior::start(Activity a) {
    if (carrying >= 0 && a != Activity::Tug && a != Activity::ToyRun && a != Activity::Play) {  // it lets the toy fall
        dropToy = carrying;
        carrying = -1;
        walkClip = ClipId::Walk;
    }
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
        case Activity::GoNap: target = snuggle ? snuggleAt : den.beds[spot]; trot = false; break;
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
        case Activity::Bath: {
            // The tub goes down just in front of it, toward you (in its usual spot if that's in
            // the way), so it only has to hop in.
            tubSize = 0.35f + 0.6f * size;
            const float dx = den.player.x - pos.x, dy = den.player.y - pos.y, len = std::hypot(dx, dy);
            const float away = tubSize + 0.35f * size;
            Vec2 spot = len > 1e-3f ? Vec2{pos.x + dx / len * away, pos.y + dy / len * away} : den.tub;
            if (distance(spot, den.home) > den.radius - tubSize || !clearAt(spot, tubSize)) spot = den.tub;
            tubAt = target = spot;
            trot = distance(pos, target) > 2.0f;  // eager
            timer = 0;
            break;
        }
        case Activity::Groomed: setClip(ClipId::Idle, 0.3f); petTimer = kGroomHold; break;
        case Activity::Kick: setClip(ClipId::LegKick, 0.2f, true); break;
        case Activity::Sneeze: setClip(ClipId::Sneeze, 0.15f, true); break;
        case Activity::PullAway: setClip(ClipId::PullAway, 0.15f, true); petTimer = 0; break;
        case Activity::Come:
            target = {den.player.x, den.player.y + 0.8f};
            trot = distance(pos, target) > 3.0f;
            break;
        case Activity::Chase:
        case Activity::Flee:
            trot = true;
            target = pos;
            break;
        case Activity::Nuzzle:
        case Activity::Bask:
            trot = false;
            timer = activity == Activity::Bask ? between(rng, 15.0f, 30.0f) : 0.0f;
            break;
        case Activity::Hatch:
            pos = hatchAt;
            heading = headingTo(pos, den.player);
            setClip(ClipId::Shake, 0.1f, true);
            break;
        case Activity::Bat: setClip(ClipId::Idle, 0.25f); petTimer = kToyHold; break;
        case Activity::Tug: petTimer = kToyHold; break;
        case Activity::Play:  // (chooseAmbient picks the toy)
        case Activity::Bowl:
            if (!toys) {
                start(Activity::Idle);
                break;
            }
            trot = false;
            timer = 0;
            break;
        case Activity::TugWar: trot = false; timer = 0; break;
        case Activity::ToyRun: {
            // Off to somewhere of its own, not far, clear of everything, with the toy held high.
            walkClip = ClipId::Carry;
            target = pos;
            for (int tries = 0; tries < 12; ++tries) {
                const float ang = between(rng, -kPi, kPi), r = between(rng, 1.6f, 3.2f) * std::fmax(0.6f, size);
                const Vec2 p{pos.x + std::sin(ang) * r, pos.y - std::cos(ang) * r};
                if (distance(p, den.home) < den.radius * 0.8f && clearAt(p, kClearance * size)) {
                    target = p;
                    break;
                }
            }
            break;
        }
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

// Playing with a toy on the floor (Alpha 2 WP7): walk over, then bat the feather about and
// pounce on it, pick up the rope and shake it before trotting off with it, or nudge the orb
// along with its nose until a treat drops out.
void DenBehavior::playWithToy(float moveScale, float dt) {
    timer += dt;
    Ball* orb = toy == kOrb ? toys->orb : nullptr;
    const Vec2 at = orb ? Vec2{orb->pos.x, orb->pos.y} : toys->at[toy];
    const float reach = 0.55f * size + (orb ? orb->radius : 0.25f);
    if (!toys->here[toy] && carrying != toy) {  // someone else has it
        start(Activity::Idle);
        return;
    }
    const Vec2 fwd{std::sin(heading), -std::cos(heading)};
    switch (step) {
        case 0:  // over to it (a rolling orb: to where it's going)
            if (distance(pos, at) < reach && (!orb || orb->resting)) {
                step = 1;
                timer = 0;
            } else {
                walkTo(orb && !orb->resting ? ballHeading(*orb, 0.4f) : at, trot, moveScale, dt);
                if (timer > 12.0f) start(Activity::Idle);  // lost interest
            }
            break;
        case 1:  // face it, then the game
            if (turnTo(headingTo(pos, at), dt)) {
                step = 2;
                timer = 0;
                setClip(toy == kFeather ? ClipId::PawBat : ClipId::PickUp, 0.2f, true);
            }
            break;
        case 2:
            if (toy == kFeather) {  // a swat or two, then the pounce
                if (clipDone) {
                    if (timer < 2.5f && rng.chance(2, 3)) {
                        setClip(ClipId::PawBat, 0.1f, true);
                    } else {
                        step = 3;
                        timer = 0;
                        setClip(ClipId::Pounce, 0.25f, true);
                    }
                }
            } else if (toy == kRope) {  // up it comes, and a good shake
                if (carrying < 0 && timer >= kGrabAt) carrying = kRope;
                if (clipDone) {
                    step = 3;
                    timer = 0;
                    setClip(ClipId::Tug, 0.2f);
                }
            } else if (timer >= kGrabAt) {  // the orb: a push with its nose
                nudged = true;
                nudge = fwd;
                ++pushes;
                step = 4;
            }
            break;
        case 3:
            if (toy == kFeather) {  // the pounce carries it onto the feather, which skitters off
                const float from = clampf(timer - dt, kPounceFrom, kPounceTo), to = clampf(timer, kPounceFrom, kPounceTo);
                const float travel = kPounceLength * moveScale * 0.5f * (to - from) / (kPounceTo - kPounceFrom);
                pos.x += fwd.x * travel;
                pos.y += fwd.y * travel;
                if (timer >= kPounceTo) {
                    knocked = true;
                    knock = {pos.x + fwd.x * (0.9f * size + 0.3f), pos.y + fwd.y * (0.9f * size + 0.3f)};
                    step = 5;
                }
            } else if (timer > 2.5f) {  // the rope: off with it, sometimes to its own bed
                const bool toBed = rng.chance(1, 3);
                start(Activity::ToyRun);
                if (toBed) {
                    const Vec2 bed = den.beds[spot];
                    const float dx = den.home.x - bed.x, dy = den.home.y - bed.y, len = std::hypot(dx, dy);
                    target = {bed.x + dx / len * 1.4f * size, bed.y + dy / len * 1.4f * size};
                }
            }
            break;
        case 4:  // the orb rolls on: after a moment, follow it and push again; three pushes, a treat
            if (timer > kGrabAt + 1.0f && (!orb || orb->resting || timer > 3.5f)) {
                if (pushes >= 3) {
                    gotTreat = toys->orbTreat;
                    start(gotTreat ? Activity::Eat : Activity::TailWag);
                } else {
                    step = 0;
                    timer = 0;
                    trot = true;
                }
            }
            break;
        default:  // the feather's gone skittering: a happy wag
            if (clipDone) start(Activity::TailWag);
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
    // Toys on the floor (WP7): hungry, the food bowl; otherwise now and then a game.
    if (toys) {
        if (toys->here[kBowl] && toys->bowlFood && d.needs.belly < kBowlHungry) {
            start(Activity::Bowl);
            return;
        }
        float want = 1.0f + (d.needs.play < 50 ? 2.0f : 0.0f) + (d.personality == Personality::Playful ? 2.0f : 0.0f) +
                     (moodOf(d) == Mood::Joyful ? 1.0f : 0.0f);
        int choices[3], n = 0;
        for (int k = kFeather; k <= kOrb; ++k)
            if (toys->here[k]) choices[n++] = k;
        if (n > 0 && d.needs.energy > 30 && unit(rng) * (want + 8.0f) < want) {
            toy = static_cast<s8>(choices[rng.below(static_cast<u32>(n))]);
            start(Activity::Play);
            trot = d.personality == Personality::Playful || rng.chance(1, 3);
            pushes = 0;
            return;
        }
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
        case Activity::Hatch:
            if (step == 0) {  // shaking off the shell
                if (clipDone) {
                    step = 1;
                    setClip(ClipId::Sit, 0.3f, true);
                }
            } else if (step == 1) {  // sitting in the nest, looking at you, until it has a name
                if (clipDone && clip == ClipId::Sit) setClip(ClipId::SitLoop, 0.2f);
            } else if (step == 2) {  // named: face the way out...
                if (turnTo(headingTo(pos, target), dt)) {
                    step = 3;
                    setClip(ClipId::Hop, 0.2f, true);
                }
            } else if (step == 3) {  // ...hop over the rim and walk clear of the nest
                const float left = distance(pos, target);
                if (left < 0.05f) {
                    start(Activity::Greet);
                } else {
                    if (clipDone) setClip(walkClip, 0.3f);
                    const float stepLen = std::fmin(walkSpeed * gait * dt, left);
                    heading = headingTo(pos, target);
                    pos.x += std::sin(heading) * stepLen;
                    pos.y -= std::cos(heading) * stepLen;
                    speed = walkSpeed * gait;
                }
            }
            break;
        case Activity::Chase: {  // after the partner; a catch is a happy hop for both
            timer += dt;
            const float reach = kBodyRadius * size * 2.0f + 0.3f * size;
            if (partner < 0 || partnerDoing != Activity::Flee || timer > 8.0f) {
                partner = -1;
                start(Activity::TailWag);
            } else if (distance(pos, partnerAt) < reach) {
                partner = -1;
                start(Activity::Hop);
            } else {
                walkTo(partnerAt, true, moveScale, dt);
            }
            break;
        }
        case Activity::Flee:  // darting away from the chaser, never into a corner
            timer += dt;
            if (partner < 0 || partnerDoing != Activity::Chase || timer > 8.0f) {
                partner = -1;
                start(Activity::Hop);
                break;
            }
            if ((step -= 1) <= 0 || distance(pos, target) < 0.4f * size + 0.1f) {
                step = 20;  // frames until it thinks again
                const float dx = pos.x - partnerAt.x, dy = pos.y - partnerAt.y, len = std::hypot(dx, dy);
                const float away = len > 1e-3f ? std::atan2(dx, -dy) : heading;
                target = pos;
                for (int tries = 0; tries < 6; ++tries) {  // straight away, else veer off to the side
                    const float a = away + (tries == 0 ? 0.0f : (tries % 2 ? 1.0f : -1.0f) * 0.5f * ((tries + 1) / 2));
                    const Vec2 p{pos.x + std::sin(a) * 2.2f * size, pos.y - std::cos(a) * 2.2f * size};
                    if (distance(p, den.home) < den.radius * 0.85f && clearAt(p, kClearance * size)) {
                        target = p;
                        break;
                    }
                }
            }
            walkTo(target, true, moveScale, dt);
            break;
        case Activity::Nuzzle:  // walk to meet, face each other, nuzzle
            timer += dt;
            if (partner < 0 || partnerDoing != Activity::Nuzzle || (step < 2 && timer > 9.0f)) {
                partner = -1;
                start(Activity::Idle);
            } else if (step == 0) {
                if (walkTo(target, false, moveScale, dt)) step = 1;
            } else if (step == 1) {
                if (turnTo(headingTo(pos, partnerAt), dt) && partnerStep >= 1) {
                    step = 2;
                    timer = 0;
                    setClip(ClipId::Nuzzle, 0.35f);
                }
            } else if (timer > 3.0f) {
                partner = -1;
                start(Activity::TailWag);
            }
            break;
        case Activity::Bask:  // to the sunbeam, lie down, soak it up
            if (step == 0) {
                if (walkTo(target, false, moveScale, dt)) {
                    step = 1;
                    setClip(ClipId::LieDown, 0.3f, true);
                }
            } else if (step == 1) {
                if (clipDone) {
                    step = 2;
                    setClip(ClipId::LieLoop, 0.2f);
                }
            } else if ((timer -= dt) <= 0) {
                start(Activity::Idle);
                blend = 0.9f;
            }
            break;
        case Activity::Play:
            if (!toys || toy < 0 || toy >= kBowl) start(Activity::Idle);
            else playWithToy(moveScale, dt);
            break;
        case Activity::Bowl: {  // over to the bowl, and a good long eat
            if (!toys || !toys->here[kBowl]) {
                start(Activity::Idle);
                break;
            }
            const Vec2 bowl = toys->at[kBowl];
            const float dx = pos.x - bowl.x, dy = pos.y - bowl.y, len = std::hypot(dx, dy);
            const Vec2 spotAt = len > 1e-3f ? Vec2{bowl.x + dx / len * (0.6f * size + 0.35f), bowl.y + dy / len * (0.6f * size + 0.35f)}
                                            : bowl;
            timer += dt;
            if (step == 0) {
                if (walkTo(spotAt, false, moveScale, dt) || timer > 12.0f) step = 1;
            } else if (step == 1) {
                if (turnTo(headingTo(pos, bowl), dt)) {
                    step = 2;
                    timer = 0;
                    setClip(ClipId::Eat, 0.35f);
                }
            } else if (timer > 3.5f) {
                ateFromBowl = true;
                start(Activity::Idle);
            } else if (!toys->bowlFood) {
                start(Activity::Idle);  // someone else finished it
            }
            break;
        }
        case Activity::TugWar:  // to its end of the rope, face the other, and pull
            timer += dt;
            if (partner < 0 || partnerDoing != Activity::TugWar || (step < 2 && timer > 10.0f) ||
                !toys || (!toys->here[kRope] && step < 2 && partnerStep < 2)) {  // (the rope's gone to someone else)
                partner = -1;
                start(Activity::Idle);
            } else if (step == 0) {
                if (walkTo(target, true, moveScale, dt)) step = 1;
            } else if (step == 1) {
                if (turnTo(headingTo(pos, partnerAt), dt) && partnerStep >= 1) {
                    step = 2;
                    timer = 0;
                    setClip(ClipId::Tug, 0.25f);
                }
            } else if (timer > 5.0f) {  // the winner trots off with it; the other hops, happy anyway
                partner = -1;
                if (tugWinner) {
                    carrying = kRope;
                    start(Activity::ToyRun);
                } else {
                    start(Activity::Hop);
                }
            }
            break;
        case Activity::Bat:  // face you and watch the feather; a swat when it comes close (care Swat)
            if (clip == ClipId::PawBat) {
                if (clipDone) setClip(ClipId::Idle, 0.2f);
            } else if (turnTo(0.0f, dt) && clip != ClipId::Idle) {
                setClip(ClipId::Idle, 0.2f);
            }
            if (petTimer <= 0) start(Activity::Idle);
            break;
        case Activity::Tug:  // braced, the rope in its teeth, shaking its head, until you let go
            if (step == 0 && turnTo(0.0f, dt)) {
                step = 1;
                setClip(ClipId::Tug, 0.2f);
            }
            if (petTimer <= 0) start(Activity::ToyRun);
            break;
        case Activity::ToyRun:  // trots off with it, proud, drops it and wags
            timer += dt;
            if (step == 0) {
                walkClip = ClipId::Carry;
                if (walkTo(target, true, moveScale, dt) || timer > 8.0f) {
                    step = 1;
                    timer = 0;
                    walkClip = ClipId::Walk;
                    setClip(ClipId::DropWait, 0.25f, true);
                }
            } else {
                if (carrying >= 0 && timer >= kDropAt) {
                    dropToy = carrying;
                    carrying = -1;
                }
                if (clipDone) start(Activity::TailWag);
            }
            break;
        case Activity::Count:
            start(Activity::Idle);
            break;
    }

    // Stay on the floor, and out of the solid things on it (a hatchling starts in the nest).
    const bool inNest = activity == Activity::Hatch;
    const float r = distance(pos, den.home);
    if (r > den.radius && !inNest) {
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
    if (!inNest)
        for (const DenObstacle& o : den.obstacles) pushOut(o.at, o.radius + kClearance * size);
    // ...and out of the other dragons. One lying down, eating or sulking stays put: the
    // others make way around it.
    const bool settled = asleep(*this) || activity == Activity::Sulk || activity == Activity::Eat ||
                         activity == Activity::BellyRub || activity == Activity::MakeUp ||
                         activity == Activity::Groomed || activity == Activity::HandFeed || inNest ||
                         (activity == Activity::Bask && step > 0) || (activity == Activity::Nuzzle && step > 0) ||
                         (activity == Activity::Bath && step > 0) || activity == Activity::Bat ||
                         activity == Activity::Tug || (activity == Activity::Bowl && step == 2) ||
                         (activity == Activity::TugWar && step == 2) ||
                         ((activity == Activity::Sit || activity == Activity::Lie) && step == 1);
    if (!settled)
        for (int i = 0; i < crowdCount; ++i) pushOut(crowd[i].at, crowd[i].radius + kBodyRadius * size);
    clipDone = false;  // consumed
}

void DenBehavior::join(Activity a, s8 withPartner, Vec2 at) {
    partner = withPartner;
    start(a);
    if (a == Activity::Nuzzle || a == Activity::Bask || a == Activity::TugWar) target = at;
}

bool DenBehavior::sociable() const {
    return partner < 0 && (activity == Activity::Idle || activity == Activity::LookAround ||
                           activity == Activity::Wander || activity == Activity::Scratch ||
                           activity == Activity::TailWag || (activity == Activity::Sit && step == 1));
}

namespace {

// Partners see what each other are up to (after anything new has started).
void sharePartners(DenBehavior* const* bs, int n) {
    for (int i = 0; i < n; ++i) {
        DenBehavior& b = *bs[i];
        if (b.partner >= n || b.partner == i) b.partner = -1;
        if (b.partner < 0) continue;
        const DenBehavior& p = *bs[b.partner];
        b.partnerAt = p.pos;
        b.partnerDoing = p.activity;
        b.partnerStep = p.step;
    }
}

void startTogether(DenSocial& s, DenBehavior* const* bs, const Dragon* const* ds, int n, bool night, float daylight,
                   float dt, Rng& rng);

}  // namespace

void denSocial(DenSocial& s, DenBehavior* const* bs, const Dragon* const* ds, int n, bool night, float daylight,
               float dt, Rng& rng) {
    startTogether(s, bs, ds, n, night, daylight, dt, rng);
    sharePartners(bs, n);
}

namespace {

void startTogether(DenSocial& s, DenBehavior* const* bs, const Dragon* const* ds, int n, bool night, float daylight,
                   float dt, Rng& rng) {
    // Most nights two curl up together in the big nest.
    if (night && !s.wasNight) s.snuggleTonight = n >= 2 && rng.chance(7, 10);
    s.wasNight = night;
    int pair[2], paired = 0;
    for (int i = 0; i < n; ++i) {
        const bool together = night && s.snuggleTonight && !ds[i]->upset && paired < 2;
        if (together) pair[paired++] = i;
        bs[i]->snuggle = together;
    }
    if (paired == 2) {
        // Side by side in the big nest, along the den's edge (both spots on the floor), each
        // on the side it comes from so they don't cross.
        DenBehavior& a = *bs[pair[0]];
        DenBehavior& b = *bs[pair[1]];
        const Vec2 nest = a.den.beds[0], home = a.den.home;
        const float ox = nest.x - home.x, oy = nest.y - home.y, len = std::hypot(ox, oy);
        const Vec2 side{-oy / len, ox / len};
        const float gap = kBodyRadius * std::fmax(a.size, b.size) * 1.1f;
        const Vec2 p0{nest.x + side.x * gap, nest.y + side.y * gap}, p1{nest.x - side.x * gap, nest.y - side.y * gap};
        const bool swap = distance(a.pos, p0) + distance(b.pos, p1) > distance(a.pos, p1) + distance(b.pos, p0);
        a.snuggleAt = swap ? p1 : p0;
        b.snuggleAt = swap ? p0 : p1;
    } else if (paired == 1) {
        bs[pair[0]]->snuggle = false;
    }
    if (night || n < 2 || (s.clock -= dt) > 0) return;
    s.clock = between(rng, 8.0f, 16.0f);

    int free[DenLayout::kSpots], count = 0;
    for (int i = 0; i < n && count < DenLayout::kSpots; ++i)
        if (bs[i]->sociable() && !ds[i]->upset && ds[i]->needs.energy > 25) free[count++] = i;
    // By bright day, the sunbeam: one goes to lie in it, and another may join.
    int basking = 0;
    for (int i = 0; i < n; ++i) basking += bs[i]->activity == Activity::Bask;
    if (daylight > 0.6f && count > 0 && basking < 2 && rng.chance(1, basking ? 3 : 4)) {
        const int i = free[rng.below(static_cast<u32>(count))];
        const Vec2 sun = bs[i]->den.sunSpot;
        const float off = basking ? 1.1f * bs[i]->size + 0.3f : 0.0f;
        bs[i]->join(Activity::Bask, -1, {sun.x + off, sun.y - 0.2f * off});
        return;
    }
    if (count < 2) return;
    int a = free[rng.below(static_cast<u32>(count))], b = a;
    while (b == a) b = free[rng.below(static_cast<u32>(count))];
    // Playful ones love a chase; the shy and the sleepy would rather have a nuzzle.
    auto playful = [&](int i) {
        const Personality p = ds[i]->personality;
        return p == Personality::Playful ? 2.0f : (p == Personality::Brave ? 1.3f
                                                   : (p == Personality::Shy || p == Personality::Sleepy) ? 0.5f : 1.0f);
    };
    const float chase = playful(a) + playful(b), nuzzle = 1.6f;
    DenToys* toys = bs[a]->toys;
    const float tug = toys && toys->here[1] ? chase * 0.8f : 0.0f;  // the rope's out: tug-of-war!
    const float roll = unit(rng) * (chase + nuzzle + tug);
    if (roll >= chase + nuzzle) {  // each takes an end, facing across the rope
        const Vec2 r = toys->at[1];
        const Vec2 pa = bs[a]->pos, pb = bs[b]->pos;
        float ux = pb.x - pa.x, uy = pb.y - pa.y;
        const float len = std::hypot(ux, uy);
        ux = len > 1e-3f ? ux / len : 1.0f;
        uy = len > 1e-3f ? uy / len : 0.0f;
        const float ra = 0.45f + 0.8f * bs[a]->size, rb = 0.45f + 0.8f * bs[b]->size;
        bs[a]->join(Activity::TugWar, static_cast<s8>(b), {r.x - ux * ra, r.y - uy * ra});
        bs[b]->join(Activity::TugWar, static_cast<s8>(a), {r.x + ux * rb, r.y + uy * rb});
        const bool aWins = rng.chance(1, 2);
        bs[a]->tugWinner = aWins;
        bs[b]->tugWinner = !aWins;
    } else if (roll < chase) {
        if (playful(b) > playful(a)) std::swap(a, b);  // the more playful one does the chasing
        bs[a]->join(Activity::Chase, static_cast<s8>(b), {});
        bs[b]->join(Activity::Flee, static_cast<s8>(a), {});
    } else {
        // Meet in the middle, face to face, just touching.
        const Vec2 pa = bs[a]->pos, pb = bs[b]->pos;
        const Vec2 mid{(pa.x + pb.x) * 0.5f, (pa.y + pb.y) * 0.5f};
        float dx = pb.x - pa.x, dy = pb.y - pa.y;
        const float len = std::hypot(dx, dy);
        dx = len > 1e-3f ? dx / len : 1.0f;
        dy = len > 1e-3f ? dy / len : 0.0f;
        const float ra = kBodyRadius * bs[a]->size + 0.05f, rb = kBodyRadius * bs[b]->size + 0.05f;
        bs[a]->join(Activity::Nuzzle, static_cast<s8>(b), {mid.x - dx * ra, mid.y - dy * ra});
        bs[b]->join(Activity::Nuzzle, static_cast<s8>(a), {mid.x + dx * rb, mid.y + dy * rb});
    }
}

}  // namespace

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
        case Activity::Hatch:
            return step == 0 ? 0.6f : 0.0f;  // squinting at its first light
        case Activity::Bask:
            return step == 2 ? 0.55f : 0.0f;  // warm and drowsy
        case Activity::Nuzzle:
            return step == 2 ? 0.6f : 0.0f;
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
        case Activity::Hatch:
            return step == 1 ? 1.0f : 0.0f;  // meeting you
        case Activity::Bask:
            return step == 0 ? 0.2f : 0.35f;
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
    if (activity == Activity::Hatch && c != Care::Greet) return;  // meeting you comes first
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
        case Care::Greet:
            if (activity == Activity::Hatch) {  // named: out of the nest, toward the rug
                if (step < 2) {
                    const float dx = den.home.x - pos.x, dy = den.home.y - pos.y, len = std::hypot(dx, dy);
                    float out = kClearance * size + 0.15f;  // clear of the egg nest's rim
                    for (const DenObstacle& o : den.obstacles)
                        if (distance(o.at, hatchAt) < 0.1f) out += o.radius;
                    target = len > 1e-4f ? Vec2{pos.x + dx / len * out, pos.y + dy / len * out} : den.home;
                    step = 2;
                }
            } else {
                start(Activity::Greet);
            }
            break;
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
        case Care::Dangle:
            if (activity != Activity::Bat) start(Activity::Bat);
            petTimer = kToyHold;
            break;
        case Care::Swat:
            if (activity == Activity::Bat && clip != ClipId::PawBat) setClip(ClipId::PawBat, 0.12f, true);
            break;
        case Care::TugPull:
            if (activity != Activity::Tug) {
                start(Activity::Tug);
                carrying = 1;  // the rope (core/items TugRope)
            }
            petTimer = kToyHold;
            break;
        case Care::TugLetGo:
            if (activity == Activity::Tug) start(Activity::ToyRun);
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
