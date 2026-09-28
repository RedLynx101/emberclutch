// Sky Rings (Beta WP9, D74: ridden; 1.0, D89: a race): you race the course on your grown partner
// against two or three rival dragons (core/challenges rivalFor: faster, steadier and cannier cup by
// cup), in the race's flight (core/challenges raceStep: speed carries, hard turns cost it, R bursts
// while the Stamina meter lasts, L brakes into the tight turns), tuned by its Wing and Stamina. A
// countdown hovering over the arena, then through the rings in order: the next one gold, a lift
// through each, three seconds for each missed. First to the last ring wins the cup (misses and
// all), second places. A wisp flies your best run beside you (its ghost, kept on the SD card per
// cup); the wind rushes louder the faster you go. The Ember cup's course climbs to the floating
// isles and ends at the high lantern, which your dragon breathes alight as you pass (the
// campaign's quest 6). The bottom screen maps the course with the rivals on it, your Stamina and
// the standings.
#include <sys/stat.h>

#include <cmath>
#include <cstdio>
#include <utility>
#include <vector>

#include "app/audio.hpp"
#include "app/challenge_stage.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/campaign.hpp"
#include "core/kinds.hpp"
#include "core/place_layout.hpp"
#include "core/rig.hpp"
#include "core/world.hpp"

namespace ec::rings {
namespace {

using stage::Set;

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// A rival: its flier (flight, tuning, pilot, run), how it looks, its clip.
struct Rival {
    challenge::Racer racer;
    Dragon look;
    DenActor actor;
    ClipId clip = ClipId::Count;
    float total = -1;    // its time, misses and all, once it's through the last ring (or reckoned)
    bool hidden = false; // far off while the valley's busy (the budget): not drawn this frame
};

struct Run {
    challenge::Course course;
    int courseCup = -1;
    const Valley* courseOf = nullptr;
    Flight flight;
    ChaseCamera cam;
    challenge::RaceInput last;
    challenge::RaceTuning tune;
    challenge::Pilot autoPilot;  // autoplay: the expert
    challenge::RingRun run;
    challenge::Ghost best;       // the best run's, from the SD card
    challenge::Ghost recording;  // this run's
    Rival rivals[challenge::kMaxRivals];
    int rivalCount = 0;
    int place = 1;               // where you are now (the standings)
    float countdown = 0;         // seconds to go
    int shownCount = 4;
    float flash = 0;             // the ring just flown through, glowing out
    int flashRing = -1;
    bool flashPassed = false;
    float endT = -1;             // seconds since the last ring (its flourish, then the results)
    float skimFor = 0, lookFor = 0;
    bool timeUp = false;
    bool wasBursting = false, wasBraking = false;
    float streaks = 0;           // the burst's speed lines, fading in and out
};

Run& run() {
    static Run r;
    return r;
}

// Each rival's colour on the map and its name tag (yours is the heart).
u32 rivalColour(int k) {
    static const u32 kColours[challenge::kMaxRivals] = {theme::kSkyTeal, theme::kEmber, theme::rgba(0xB0, 0x8C, 0xE6)};
    return kColours[k >= 0 && k < challenge::kMaxRivals ? k : 0];
}

// ---------------------------------------------------------------------- ghosts on the SD card
u8 g_ghostBytes[6 + 8 * challenge::kGhostMaxPoints];
std::size_t g_ghostSize = 0;
char g_ghostPath[64];
volatile bool g_ghostBusy = false;

void ghostPath(int cup, char* out, std::size_t cap) {
    std::snprintf(out, cap, "sdmc:/3ds/emberclutch/ghost-rings-%d.bin", cup);
}

void writeGhost(void*) {
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/emberclutch", 0777);
    if (FILE* f = std::fopen(g_ghostPath, "wb")) {
        std::fwrite(g_ghostBytes, 1, g_ghostSize, f);
        std::fclose(f);
    }
    g_ghostBusy = false;
}

// On a thread of its own (an SD card write can stall, run 17), at the thread's lower priority.
void saveGhost(int cup, const challenge::Ghost& g) {
    if (g_ghostBusy) return;
    g_ghostSize = challenge::encodeGhost(g, g_ghostBytes, sizeof(g_ghostBytes));
    if (!g_ghostSize) return;
    ghostPath(cup, g_ghostPath, sizeof(g_ghostPath));
    g_ghostBusy = true;
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    if (!threadCreate(writeGhost, nullptr, 8 * 1024, prio + 1, -2, true)) writeGhost(nullptr);
}

bool loadGhost(int cup, challenge::Ghost& g) {
    g.clear();
    if (g_ghostBusy) return false;
    char path[64];
    ghostPath(cup, path, sizeof(path));
    FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    std::vector<u8> bytes;
    u8 buf[2048];
    std::size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0 && bytes.size() < sizeof(g_ghostBytes)) bytes.insert(bytes.end(), buf, buf + n);
    std::fclose(f);
    return challenge::decodeGhost(bytes.data(), bytes.size(), g);
}

// ---------------------------------------------------------------------- the rivals
// Their looks: grown dragons of the kinds your den already has (their models are in; a kind no
// dragon of yours is would load on the spot), your partner's own last, in other colourings.
void setUpRivals(App& app, Set& s, Run& r) {
    int kinds[kMaxKinds + 1];
    int nKinds = 0;
    for (int i = 0; i < app.game.dragonCount; ++i) {
        const int k = app.game.dragons[i].kind < kindCount() ? app.game.dragons[i].kind : 0;
        bool seen = k == s.shown.kind;
        for (int j = 0; j < nKinds && !seen; ++j) seen = kinds[j] == k;
        if (!seen && nKinds < kMaxKinds) kinds[nKinds++] = k;
    }
    kinds[nKinds++] = s.shown.kind < kindCount() ? s.shown.kind : 0;
    const u32 seed = app.rng.next();
    r.rivalCount = challenge::rivalCount(s.cup);
    for (int k = 0; k < r.rivalCount; ++k) {
        Rival& v = r.rivals[k];
        const challenge::RivalInfo info = challenge::rivalFor(s.cup, k);
        v = Rival{};
        v.look.id = 0xFFFFFF80u + static_cast<u32>(k);  // (its own mesh cache: not any of yours)
        v.look.stage = Stage::Adult;
        v.look.kind = static_cast<u8>(kinds[k % nKinds]);
        // Another colouring than yours; the Starfire cup's fastest in its kind's rare one.
        const int variant = s.cup == challenge::kStarfire && k == r.rivalCount - 1 ? 3 : (s.shown.variant + 1 + k) % 3;
        v.look.variant = static_cast<u8>(variant);
        std::snprintf(v.look.name, sizeof(v.look.name), "%s", info.name);
        v.racer.tune = challenge::raceTuning(info.wing, info.stamina);
        v.racer.pilot.skill = info.skill;
        v.racer.pilot.rng = Rng(seed + 7919u * static_cast<u32>(k + 1));
        v.racer.pilot.phase = k * 2.1f;
        v.racer.start(r.course, challenge::rivalStartAt(r.course, k), r.course.heading);
        v.racer.flight.speed = 0;
    }
}

void animateRival(App& app, Rival& v) {
    const AnimLibrary* lib = r3d::animsFor(v.look);
    if (!lib) return;
    const int* clips = r3d::clipIndexFor(v.look, kFormGrown);
    const Flight& f = v.racer.flight;
    const ClipId want = v.racer.last.dive ? ClipId::FlyDive : f.sinceFlap < 0.8f ? ClipId::FlyFlap : ClipId::FlyGlide;
    if (want != v.clip && clips && clips[static_cast<int>(want)] >= 0) {
        v.actor.anim.play(clips[static_cast<int>(want)], 0.3f, want == ClipId::FlyFlap);
        v.clip = want;
    }
    v.actor.anim.update(*lib, app.dt, nullptr, 0);
    v.actor.eyes.update(0.0f, app.dt);
}

// Keeps the rivals out of the picture's way: off the line from the camera to you (one coming up
// behind you filled the screen), out of your dragon's space and out of each other's. It nudges
// them a little each frame; their flight takes it from there.
void keepClear(Run& r, const Set& s, float dt) {
    const float k = std::fmin(1.0f, 6.0f * dt);
    const Vec3 a = s.eye, ab = r.flight.pos - s.eye;
    Vec3 aside{-ab.y, ab.x, 0};
    aside = length(aside) > 1e-3f ? normalize(aside) : Vec3{1, 0, 0};
    for (int i = 0; i < r.rivalCount; ++i) {
        Vec3& p = r.rivals[i].racer.flight.pos;
        auto away = [&](Vec3 from, float clear) {
            Vec3 d = p - from;
            float len = length(d);
            if (len >= clear) return;
            if (len < 1e-3f) {
                d = aside * (i % 2 ? -1.0f : 1.0f);
                len = 1.0f;
            }
            p = p + d * ((clear - len) / len * k);
        };
        const float t = clampf(dot(p - a, ab) / std::fmax(1e-3f, dot(ab, ab)), 0.0f, 1.0f);
        away(a + ab * t, 4.5f);
        away(r.flight.pos, 5.5f);
        for (int j = 0; j < r.rivalCount; ++j)
            if (j != i) away(r.rivals[j].racer.flight.pos, 4.0f);
    }
}

// Where you stand now: ahead of each rival who's further along (or through, faster).
int standing(const Run& r) {
    int place = 1;
    const float mine = r.run.progress(r.course, r.flight.pos);
    for (int k = 0; k < r.rivalCount; ++k) {
        const Rival& v = r.rivals[k];
        if (r.run.finished) {
            if (v.racer.run.finished && v.racer.run.total() < r.run.total()) ++place;
        } else if (v.racer.run.finished || v.racer.run.progress(r.course, v.racer.flight.pos) > mine) {
            ++place;
        }
    }
    return place;
}

// ---------------------------------------------------------------------- play
void rideClip(Set& s, const Flight& f, bool diving) {
    stage::playDragon(s, diving ? ClipId::FlyDive : f.sinceFlap < 0.8f ? ClipId::FlyFlap : ClipId::FlyGlide, 0.3f);
    const float roll = f.roll;
    stage::playPerson(s.you, roll > 0.3f ? "ride_lean_left" : roll < -0.3f ? "ride_lean_right" : "ride", 1.0f, 0.3f);
}

// Places found on the way (the isles from the air, for the Ember cup).
void lookRound(App& app, Set& s, Run& r) {
    if ((r.lookFor -= app.dt) > 0) return;
    r.lookFor = 0.5f;
    for (const ValleyPlaceInfo& p : s.valley->places) {
        const world::PlaceInfo& info = world::placeInfo(p.id);
        const float d = std::hypot(r.flight.pos.x - p.at.x, r.flight.pos.y - p.at.y);
        if (d < info.findRadius * 1.5f && world::findPlace(app.game, p.id)) {
            audio::playStinger("place-found");
            queueToastf(app, str::kFoundPlace, info.name);
        }
    }
}

// The high lantern on the isles, breathed alight as the Ember course ends there.
void lightTheIsles(App& app, Set& s) {
    const ValleyPlaceInfo* isles = s.valley->place(kPlaceIsles);
    if (!isles) return;
    const PlaceLayout& L = placeLayout(kPlaceIsles);
    const Vec2 w = placeToWorld(*isles, {L.lantern.x, L.lantern.y});
    const Vec3 flame{w.x, w.y, isles->at.z + L.lantern.z};
    const int element = kindInfo(s.shown.kind < kindCount() ? s.shown.kind : 0).elements[0];
    s.breath.emit(challenge::breathFor(element), stage::mouthOf(s), flame, 28, 0.55f);
    world::findPlace(app.game, kPlaceIsles);
    if (world::lightLantern(app.game, kPlaceIsles)) {
        audio::playSfx(audio::Sfx::LanternRelight);
        queueToastf(app, "%s", str::kHighLantern);
    } else {
        audio::playSfx(audio::Sfx::LanternLight);
    }
    stage::burst(s, Fx::Sparkle, flame, 10, 1.5f);
}

// The run's end: the rivals still flying are flown on to the end to know their times, your place
// among them, and the results' lines.
void judge(App& app, Set& s, Run& r) {
    float totals[challenge::kMaxRivals] = {-1, -1, -1};
    const float limit = r.course.par * challenge::kRingTimeLimit;
    for (int k = 0; k < r.rivalCount; ++k) {
        Rival& v = r.rivals[k];
        if (v.total < 0) v.total = v.racer.run.finished ? v.racer.run.total() : challenge::finishRace(v.racer, *s.valley, r.course, limit);
        totals[k] = v.total;
    }
    char line[64], line2[64] = {};
    if (r.timeUp) {
        std::snprintf(line, sizeof(line), "%s", str::kTimeUp);
        stage::finish(app, s, 0, challenge::Outcome::TryAgain, line);
        return;
    }
    const float total = r.run.total();
    const int place = challenge::racePlace(total, totals, r.rivalCount);
    const char* nth = str::kPlaceNth[place - 1 < 3 ? place - 1 : 3];
    if (r.run.missed) std::snprintf(line, sizeof(line), str::kRaceMissed, nth, total, r.run.missed);
    else std::snprintf(line, sizeof(line), str::kRaceDone, nth, total);
    // Who won (if not you), or how far ahead of the nearest rival you were.
    int fastest = -1, nearest = -1;
    for (int k = 0; k < r.rivalCount; ++k) {
        if (totals[k] < 0) continue;
        if (fastest < 0 || totals[k] < totals[fastest]) fastest = k;
    }
    nearest = fastest;
    if (place > 1 && fastest >= 0) std::snprintf(line2, sizeof(line2), str::kRaceWinner, r.rivals[fastest].look.name, totals[fastest]);
    else if (nearest >= 0) std::snprintf(line2, sizeof(line2), str::kRaceBeatAll, r.rivals[nearest].look.name, totals[nearest] - total);
    stage::finish(app, s, static_cast<int>(std::lround(total * 10.0f)), challenge::raceOutcome(true, place), line);
    std::snprintf(s.detail2, sizeof(s.detail2), "%s", line2);
}

}  // namespace

void begin(App& app, Set& s) {
    Run& r = run();
    if (r.courseCup != s.cup || r.courseOf != s.valley) {  // (made once: its par flies it with the pilot)
        challenge::makeCourse(*s.valley, s.cup, r.course);
        r.courseCup = s.cup;
        r.courseOf = s.valley;
    }
    r.flight = Flight{};
    r.flight.pos = r.course.start;
    r.flight.heading = r.course.heading;
    r.flight.grounded = false;
    r.flight.speed = 0;
    r.flight.sinceFlap = 0;
    r.flight.stamina = 1;
    r.cam = ChaseCamera{};
    r.last = challenge::RaceInput{};
    r.tune = challenge::raceTuning(s.shown);
    r.autoPilot = challenge::Pilot{};
    r.autoPilot.skill = challenge::expertPilot();
    r.run = challenge::RingRun{};
    r.recording.clear();
    loadGhost(s.cup, r.best);
    setUpRivals(app, s, r);
    r.place = 1;
    r.countdown = 3.4f;
    r.shownCount = 4;
    r.flash = 0;
    r.flashRing = -1;
    r.endT = -1;
    r.timeUp = false;
    r.wasBursting = r.wasBraking = false;
    r.streaks = 0;
    s.detail2[0] = 0;
    s.riding = true;
    s.hostShown = false;
    s.dragonAt = r.flight.pos;
    s.dragonHeading = r.flight.heading;
    s.dragonPitch = s.dragonRoll = 0;
    s.snapCam = true;
    r.cam.update(r.flight, *s.valley, 0);
    s.eye = s.wantEye = r.cam.eye;
    s.target = s.wantTarget = r.cam.target;
    stage::playDragon(s, ClipId::FlyFlap, 0.2f);
    stage::playPerson(s.you, "ride");
    audio::playSfx(audio::Sfx::Takeoff);
}

void update(App& app, Set& s, const Input& in) {
    Run& r = run();
    const Valley& va = *s.valley;
    if ((in.down & KEY_X) && r.endT < 0) {
        stage::giveUp(s);
        return;
    }
    const float dt = std::fmin(app.dt, 0.05f);  // (a long frame doesn't fling anyone through a ring)
    const bool going = r.countdown <= 0;
    if (!going) {  // hovering at the start, the count
        r.countdown -= app.dt;
        const int n = static_cast<int>(std::ceil(r.countdown));
        if (n != r.shownCount && n >= 0) {
            r.shownCount = n;
            if (n > 0) {
                audio::playSfx(audio::Sfx::Tap, 1.2f);
            } else {
                audio::playSfx(audio::Sfx::WhistleStart);
                r.flight.speed = r.tune.cruise;
                r.flight.sinceFlap = 0;
                for (int k = 0; k < r.rivalCount; ++k) r.rivals[k].racer.flight.speed = r.rivals[k].racer.tune.cruise;
            }
        }
        r.flight.sinceFlap = 0;  // (its wings beat, holding it there)
        for (int k = 0; k < r.rivalCount; ++k) r.rivals[k].racer.flight.sinceFlap = 0;
        stage::playDragon(s, ClipId::FlyFlap, 0.2f);
    } else {
        if (r.countdown > -1.0f) r.countdown -= app.dt;  // (the "Go!" fades)
        challenge::RaceInput fi;
        if (s.autoplay) {
            fi = r.autoPilot.fly(r.flight, r.course, r.run.next, r.tune, dt);
        } else {
            const float dpadX = (in.held & KEY_DRIGHT ? 1.0f : 0.0f) - (in.held & KEY_DLEFT ? 1.0f : 0.0f);
            const float dpadY = (in.held & KEY_DUP ? 1.0f : 0.0f) - (in.held & KEY_DDOWN ? 1.0f : 0.0f);
            fi.steer = clampf(in.padX + dpadX, -1, 1);
            fi.pitch = clampf(in.padY + dpadY, -1, 1);
            fi.flap = in.held & KEY_A;
            fi.dive = in.held & KEY_B;
            fi.burst = in.held & KEY_R;
            fi.brake = in.held & KEY_L;
        }
        const bool wasDiving = r.last.dive;
        r.last = fi;
        const Vec3 from = r.flight.pos;
        if (r.endT >= 0) {  // after the last ring: it slows to a hover, wings beating, level
            r.flight.speed *= 1.0f - std::fmin(1.0f, 1.8f * dt);
            r.flight.pos = r.flight.pos + r.flight.forward() * (r.flight.speed * dt);
            r.flight.pitch *= 1.0f - std::fmin(1.0f, 4.0f * dt);
            r.flight.roll *= 1.0f - std::fmin(1.0f, 4.0f * dt);
            r.flight.sinceFlap = 0;
        } else {
            challenge::raceStep(r.flight, fi, va, dt, r.tune);
            // The burst and the brake, heard as they take hold.
            const bool bursting = challenge::bursting(r.flight, fi) && r.flight.stamina > 0;
            if (bursting && !r.wasBursting) audio::playSfx(audio::Sfx::Burst);
            if (fi.brake && !r.wasBraking) audio::playSfx(audio::Sfx::Brake);
            r.wasBursting = bursting;
            r.wasBraking = fi.brake;
        }
        r.streaks = clampf(r.streaks + (r.wasBursting && r.endT < 0 ? 3.0f : -2.0f) * dt, 0.0f, 1.0f);
        if (fi.dive && !wasDiving && r.endT < 0) audio::playSfx(audio::Sfx::DiveWhoosh);
        r.skimFor -= dt;
        if (r.flight.skimming && r.skimFor <= 0) {
            audio::playSfx(audio::Sfx::WaterSkim);
            r.skimFor = 0.7f;
        }
        // The rivals fly their own lines.
        for (int k = 0; k < r.rivalCount; ++k) {
            Rival& v = r.rivals[k];
            const bool wasThrough = v.racer.run.finished;
            v.racer.step(va, r.course, dt);
            if (v.racer.run.finished && !wasThrough) v.total = v.racer.run.total();
        }
        keepClear(r, s, dt);
        if (r.endT < 0) {
            const int before = r.run.next;
            const challenge::RingRun::Event e = r.run.step(r.course, from, r.flight.pos, dt);
            r.recording.record(r.run.time, r.flight.pos, r.flight.heading);
            if (e == challenge::RingRun::kPassed) {
                challenge::ringLift(r.flight, r.tune);
                const int streak = r.run.passed % 8;
                audio::playSfx(audio::Sfx::RingPass, 0.9f + 0.05f * streak);
                r.flashRing = r.run.next - 1;
                r.flashPassed = true;
                r.flash = 0.6f;
                stage::burst(s, Fx::Sparkle, r.course.rings[r.flashRing].at, 8, 2.0f);
            } else if (e == challenge::RingRun::kMissed) {
                audio::playSfx(audio::Sfx::Whimper, 1.3f, 0.5f);
                r.flashRing = before;
                r.flashPassed = false;
                r.flash = 0.6f;
                char line[24];
                std::snprintf(line, sizeof(line), "+%d s", static_cast<int>(challenge::kMissPenalty));
                stage::popup(s, line, theme::kRose);
            }
            if (r.run.finished) {
                r.endT = 0;
                s.camEase = 1.4f;
                stage::playPerson(s.you, "cheer", 1.0f, 0.2f, true);
                audio::playSfx(audio::Sfx::Chirp, 1.1f);
                if (r.course.toIsles) lightTheIsles(app, s);
            } else if (r.run.time > r.course.par * challenge::kRingTimeLimit) {
                r.timeUp = true;
                r.endT = 0;
                stage::popup(s, str::kTimeUp, theme::kRose);
            }
        }
        r.place = standing(r);
        lookRound(app, s, r);
    }
    if (r.endT >= 0 && (r.endT += app.dt) > 2.2f) judge(app, s, r);  // the flourish over: the results
    if (r.flash > 0) r.flash -= app.dt;
    // The dragon as flown, the camera behind.
    s.dragonAt = r.flight.pos;
    s.dragonHeading = r.flight.heading;
    s.dragonPitch = r.flight.pitch;
    s.dragonRoll = r.flight.roll;
    if (r.endT < 0) rideClip(s, r.flight, r.last.dive && going);
    else stage::playDragon(s, ClipId::FlyFlap, 0.4f);
    stage::stepDragon(app, s, 0);
    for (int k = 0; k < r.rivalCount; ++k) animateRival(app, r.rivals[k]);
    if (r.endT < 0 || r.timeUp) {
        r.cam.update(r.flight, va, app.dt);
        s.wantEye = r.cam.eye;
        s.wantTarget = r.cam.target;
        s.snapCam = true;  // (the chase camera eases itself)
    } else {  // the flourish: the camera comes round beside it, the dragon high in the picture (the card's below)
        const Vec3 f = r.flight.forward(), side{-f.y, f.x, 0};
        s.wantEye = r.flight.pos + side * 13.0f - f * 2.0f + Vec3{0, 0, 1.0f};
        s.wantTarget = r.flight.pos + f * 1.5f + Vec3{0, 0, -3.0f};
    }
    // The wind rushing by, louder the faster you go (a burst or a dive roars); the wings in a glide.
    audio::setBed(audio::Bed::WindHigh, clampf((r.flight.speed - 6.0f) / 20.0f, 0.12f, 1.0f));
    audio::setBed(audio::Bed::Rush, clampf((r.flight.speed - 16.0f) / 14.0f, 0.0f, 1.0f));  // the rush of a burst (sounds)
    if (r.flight.sinceFlap > 0.8f) audio::setBed(audio::Bed::WingFlutter, clampf(r.flight.speed / 18.0f, 0.3f, 1.0f));
}

void scene(App& app, Set& s) {
    Run& r = run();
    const int n = static_cast<int>(r.course.rings.size());
    auto ring = [&](int k, float glow, bool next) {
        const challenge::Ring& g = r.course.rings[k];
        r3d::ChallengeProp p;
        p.kind = r3d::PropKind::Ring;
        p.at = g.at;
        p.yaw = std::atan2(-g.normal.x, g.normal.y);
        p.pitch = std::atan2(g.normal.z, std::hypot(g.normal.x, g.normal.y));
        p.scale = g.radius * (next ? 1.0f + 0.04f * std::sin(app.t * 6.0f) : 1.0f);
        p.look = ringLook(s.cup, next);
        if (k == n - 1) p.look.colour[0] = challenge::cupColour(s.cup);  // the last: the cup's own colour
        p.look.glow *= glow;
        stage::addProp(s, p);
    };
    // The next three, the next one gold; the one just flown through glowing out.
    const int from = r.endT >= 0 ? n : r.run.next;
    for (int k = from; k < n && k < from + 3; ++k) ring(k, 1.0f - 0.25f * (k - from), k == from);
    if (r.flash > 0 && r.flashRing >= 0 && r.flashRing < n) {
        const challenge::Ring& g = r.course.rings[r.flashRing];
        if (length(s.eye - g.at) > g.radius * 2.2f) {
            ring(r.flashRing, r.flashPassed ? 1.6f : 0.3f, false);
            s.props[s.propCount - 1].scale *= 1.0f + (0.6f - r.flash) * 0.8f;
        }
    }
    // The rivals, on the light model (no rebuilding as they pass). Low over the valley the ground
    // alone takes most of the budget: then a rival far off (small by then) isn't drawn, with a
    // margin so none flickers at the edge.
    const r3d::ValleyStats busy = r3d::valleyStats();
    const bool crowded = busy.ground + busy.places > 5000;
    for (int k = 0; k < r.rivalCount && s.otherCount < r3d::kMaxOthers; ++k) {
        Rival& v = r.rivals[k];
        const float d = length(v.racer.flight.pos - s.eye);
        v.hidden = crowded && d > (v.hidden ? 42.0f : 50.0f);
        if (v.hidden) continue;
        r3d::ValleyDragon& o = s.others[s.otherCount++];
        o.dragon = &v.look;
        o.actor = &v.actor;
        o.at = v.racer.flight.pos;
        o.heading = v.racer.flight.heading;
        o.pitch = v.racer.flight.pitch;
        o.roll = v.racer.flight.roll;
        o.lod = 1;
    }
}

void hud(App& app, Set& s) {
    Run& r = run();
    const bool racing = r.countdown <= 0 && r.endT < 0;
    // The burst's speed lines: streaks flying out from the middle toward the edges.
    if (r.streaks > 0.02f) {
        for (int k = 0; k < 14; ++k) {
            const float a = k * 2.399f + 0.3f, ph = std::fmod(app.t * 2.6f + k * 0.37f, 1.0f);
            const float r0 = 90 + 150 * ph, r1 = r0 + 26 + 30 * ph;
            const float cx = std::cos(a), cy = std::sin(a) * 0.62f;
            C2D_DrawLine(200 + cx * r0, 120 + cy * r0, withAlpha(theme::kShell, 0.0f), 200 + cx * r1, 120 + cy * r1,
                         withAlpha(theme::kShell, 0.45f * r.streaks * (1.0f - ph)), 1.6f, 0);
        }
    }
    // Your best, a wisp flying beside you.
    Vec3 g;
    float gh;
    if (racing && r.best.at(r.run.time, g, gh)) {
        float x, y, ppu;
        if (r3d::project(g + Vec3{0, 0, 1.4f}, x, y, ppu)) {
            const u32 c = fromRgb(challenge::cupColour(s.cup));
            for (int k = 5; k >= 1; --k) {  // its trail
                Vec3 t;
                float th, tx, ty, tp;
                if (r.best.at(r.run.time - k * 0.08f, t, th) && r3d::project(t + Vec3{0, 0, 1.4f}, tx, ty, tp))
                    C2D_DrawCircleSolid(tx, ty, 0, std::fmax(1.5f, 0.35f * tp * (1.0f - k * 0.15f)), withAlpha(c, 0.12f * (6 - k)));
            }
            const float rr = clampf(0.7f * ppu, 3.0f, 16.0f);
            C2D_DrawCircleSolid(x, y, 0, rr * 1.8f, withAlpha(c, 0.25f));
            C2D_DrawCircleSolid(x, y, 0, rr, withAlpha(theme::kShell, 0.85f));
        }
    }
    // The rivals' names over them, near enough to read.
    for (int k = 0; k < r.rivalCount; ++k) {
        const Rival& v = r.rivals[k];
        const Vec3 at = v.racer.flight.pos + Vec3{0, 0, 3.4f};
        float x, y, ppu;
        if (v.hidden || length(at - s.eye) > 70.0f || !r3d::project(at, x, y, ppu) || x < 10 || x > kTopW - 10 || y < 10 || y > kScreenH - 30)
            continue;
        const float w = textWidth(app, v.look.name, 0.4f) + 10;
        panel({x - w / 2, y - 8, w, 15}, withAlpha(rivalColour(k), 0.75f));
        textCentered(app, v.look.name, x, y, 0.4f, theme::kShell, w);
    }
    // The next ring, when it's out of sight: an arrow at the edge pointing its way.
    if (r.run.next < static_cast<int>(r.course.rings.size()) && racing) {
        float x, y, ppu;
        const bool seen = r3d::project(r.course.rings[r.run.next].at, x, y, ppu) && x > 10 && x < kTopW - 10 && y > 10 && y < kScreenH - 10;
        if (!seen) {
            const Vec3 d = r.course.rings[r.run.next].at - r.flight.pos;
            const float rel = std::remainder(std::atan2(d.x, -d.y) - r.flight.heading, 6.2831853f);
            const float ax = 200 - std::sin(rel) * 150, ay = 120 + (std::fabs(rel) > 1.5f ? 80.0f : -std::cos(rel) * 20 - 60);
            const float dir = -rel;  // screen angle: right is +x
            const u32 c = theme::kClutchGold;
            const float cx = std::cos(dir - 1.5708f), sy = std::sin(dir - 1.5708f);
            C2D_DrawTriangle(ax + cx * 14, ay + sy * 14, c, ax - sy * 8, ay + cx * 8, c, ax + sy * 8, ay - cx * 8, c, 0);
            if (std::fabs(rel) > 2.2f) textCentered(app, str::kRingBehind, 200, 196, 0.45f, theme::kShell, 380);
        }
    }
    // The count: big, in the middle.
    if (r.countdown > -0.8f) {
        char num[8];
        const int n = static_cast<int>(std::ceil(r.countdown));
        std::snprintf(num, sizeof(num), "%d", n);
        const float frac = r.countdown - std::floor(r.countdown), k = r.countdown > 0 ? 1.0f + 0.5f * frac : 1.2f;
        const float a = r.countdown > 0 ? 1.0f : clampf(1.0f + r.countdown / 0.8f, 0.0f, 1.0f);
        textCentered(app, r.countdown > 0 ? num : str::kGoCount, 202, 98, 1.6f * k, withAlpha(theme::kDenPlum, 0.5f * a), 380, Face::Title);
        textCentered(app, r.countdown > 0 ? num : str::kGoCount, 200, 96, 1.6f * k, withAlpha(r.countdown > 0 ? theme::kShell : theme::kClutchGold, a),
                     380, Face::Title);
    }
    // The clock and your place; the rings.
    char line[48];
    std::snprintf(line, sizeof(line), "%.1f", r.run.time);
    panel({8, 8, 92, 30}, withAlpha(theme::kDenPlum, 0.7f));
    textCentered(app, line, 54, 23, 0.8f, theme::kShell, 88);
    if (r.rivalCount > 0) {
        const bool first = r.place == 1;
        panel({104, 8, 52, 30}, withAlpha(first ? theme::kClutchGold : theme::kDenPlum, first ? 0.9f : 0.7f));
        textCentered(app, str::kPlaceNth[r.place - 1 < 3 ? r.place - 1 : 3], 130, 23, 0.7f, first ? theme::kDenPlum : theme::kShell, 50,
                     Face::Title);
    }
    std::snprintf(line, sizeof(line), str::kRingsCount, r.run.passed, static_cast<int>(r.course.rings.size()));
    panel({292, 8, 100, 30}, withAlpha(theme::kDenPlum, 0.7f));
    textCentered(app, line, 342, 23, 0.55f, theme::kClutchGold, 96);
    if (r.run.missed) {
        std::snprintf(line, sizeof(line), str::kRingsMissed, r.run.missed, r.run.missed * static_cast<int>(challenge::kMissPenalty));
        textCentered(app, line, 342, 46, 0.4f, theme::kRose, 110);
    }
    // The Stamina meter, under the clock: as long as the dragon's Stamina makes it; gold, glowing
    // as it bursts, rose when nearly spent.
    if (r.endT < 0) {
        const float w = 26.0f * r.tune.meter, x = 12, y = 60;
        panel({x - 4, y - 18, w + 8, 30}, withAlpha(theme::kDenPlum, 0.65f));
        text(app, str::kStamina, x, y - 16, 0.38f, withAlpha(theme::kShell, 0.85f), C2D_AlignLeft, 80);
        panel({x, y, w, 8}, theme::kTrack);
        const float full = r.flight.stamina;
        const u32 fill = full < 0.2f ? theme::kRose : theme::kClutchGold;
        panel({x, y, w * full, 8}, r.wasBursting ? withAlpha(theme::kShell, 0.6f + 0.4f * std::sin(app.t * 20.0f)) : fill);
    }
}

void bottom(App& app, Set& s, const Input& in) {
    Run& r = run();
    const auto& rings = r.course.rings;
    // The course from above, fitted into the map's box (north up).
    const Rect box{8, 30, 200, 170};
    panel(box, withAlpha(theme::kShell, 0.12f));
    float x0 = r.course.start.x, x1 = x0, y0 = r.course.start.y, y1 = y0;
    for (const challenge::Ring& g : rings) {
        x0 = std::fmin(x0, g.at.x), x1 = std::fmax(x1, g.at.x);
        y0 = std::fmin(y0, g.at.y), y1 = std::fmax(y1, g.at.y);
    }
    const float span = std::fmax(40.0f, std::fmax(x1 - x0, y1 - y0)) * 1.1f;
    const float cx = (x0 + x1) * 0.5f, cy = (y0 + y1) * 0.5f, k = (box.h - 16) / span;
    auto mapAt = [&](Vec3 p) { return Vec2{box.x + box.w / 2 + (p.x - cx) * k, box.y + box.h / 2 - (p.y - cy) * k}; };
    if (const C2D_Image* map = r3d::valleyMap(*s.valley)) {  // the valley under it (the valley's painted map, cropped)
        const Valley& v = *s.valley;
        const float half = span * 0.5f;
        static Tex3DS_SubTexture sub;
        sub = {100, 100, (cx - half - v.x0) / v.size(), (cy + half - v.y0) / v.size(), (cx + half - v.x0) / v.size(),
               (cy - half - v.y0) / v.size()};
        const C2D_Image crop{map->tex, &sub};
        const float side = box.h - 16;
        C2D_ImageTint tint;
        C2D_PlainImageTint(&tint, theme::rgba(255, 255, 255, 170), 0.0f);
        C2D_DrawImageAt(crop, box.x + box.w / 2 - side / 2, box.y + 8, 0.5f, &tint, side / 100.0f, side / 100.0f);
    }
    for (std::size_t i = 0; i < rings.size(); ++i) {
        const Vec2 a = mapAt(i ? rings[i - 1].at : r.course.start), b = mapAt(rings[i].at);
        C2D_DrawLine(a.x, a.y, withAlpha(theme::kShell, 0.3f), b.x, b.y, withAlpha(theme::kShell, 0.3f), 1.0f, 0);
    }
    for (std::size_t i = 0; i < rings.size(); ++i) {
        const Vec2 m = mapAt(rings[i].at);
        const bool done = static_cast<int>(i) < r.run.next, next = static_cast<int>(i) == r.run.next;
        if (next) C2D_DrawCircleSolid(m.x, m.y, 0, 5.5f + std::sin(app.t * 6) * 1.2f, theme::kClutchGold);
        C2D_DrawCircleSolid(m.x, m.y, 0, 3.0f, done ? theme::kClutchGold : withAlpha(theme::kShell, next ? 1.0f : 0.5f));
    }
    Vec3 g;
    float gh;
    if (r.best.at(r.run.time, g, gh) && r.endT < 0) {
        const Vec2 m = mapAt(g);
        C2D_DrawCircleSolid(m.x, m.y, 0, 3.0f, withAlpha(fromRgb(challenge::cupColour(s.cup)), 0.8f));
    }
    for (int j = 0; j < r.rivalCount; ++j) {  // the rivals: a dot each in their colour
        const Vec2 m = mapAt(r.rivals[j].racer.flight.pos);
        C2D_DrawCircleSolid(m.x, m.y, 0, 4.5f, theme::kDenPlum);
        C2D_DrawCircleSolid(m.x, m.y, 0, 3.5f, rivalColour(j));
    }
    const Vec2 me = mapAt(r.flight.pos);
    C2D_DrawLine(me.x, me.y, theme::kShell, me.x + std::sin(r.flight.heading) * 9, me.y + std::cos(r.flight.heading) * 9,
                 theme::kShell, 2, 0);
    heart(me.x, me.y, 8, theme::kRose);
    // The side: the cup, the standings, the Stamina meter, the help.
    const float x = 216;
    text(app, challenge::cupName(s.cup), x, 30, 0.45f, fromRgb(challenge::cupColour(s.cup)), C2D_AlignLeft, 100);
    // Who's ahead: you and the rivals in order of how far along each is.
    struct Entry {
        float along;
        int who;  // -1 you
    } order[challenge::kMaxRivals + 1];
    int count = 0;
    order[count++] = {r.run.finished ? 1000.0f - r.run.total() : r.run.progress(r.course, r.flight.pos), -1};
    for (int j = 0; j < r.rivalCount; ++j) {
        const challenge::Racer& v = r.rivals[j].racer;
        order[count++] = {v.run.finished ? 1000.0f - v.run.total() : v.run.progress(r.course, v.flight.pos), j};
    }
    for (int a = 1; a < count; ++a)
        for (int b = a; b > 0 && order[b].along > order[b - 1].along; --b) std::swap(order[b], order[b - 1]);
    for (int i = 0; i < count; ++i) {
        const float y = 50 + i * 15.0f;
        const bool you = order[i].who < 0;
        if (you) C2D_DrawRectSolid(x - 3, y - 1, 0, 100, 14, withAlpha(theme::kShell, 0.15f));
        text(app, str::kPlaceNth[i < 3 ? i : 3], x, y, 0.38f, you ? theme::kClutchGold : withAlpha(theme::kShell, 0.75f), C2D_AlignLeft, 30);
        if (you) heart(x + 34, y + 6, 7, theme::kRose);
        else C2D_DrawCircleSolid(x + 34, y + 6, 0, 3.5f, rivalColour(order[i].who));
        text(app, you ? str::kRaceYou : r.rivals[order[i].who].look.name, x + 42, y, 0.38f,
             you ? theme::kClutchGold : withAlpha(theme::kShell, 0.75f), C2D_AlignLeft, 58);
    }
    const float my = 52 + count * 15.0f;
    text(app, str::kStamina, x, my, 0.38f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft, 100);
    const Rect bar{x, my + 14, std::fmin(96.0f, 18.0f * r.tune.meter), 8};
    panel(bar, theme::kTrack);
    panel({bar.x, bar.y, bar.w * r.flight.stamina, bar.h}, r.flight.stamina < 0.2f ? theme::kRose : theme::kClutchGold);
    text(app, str::kHelpRings, x, my + 28, 0.34f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft, 100);
    textCentered(app, str::kGiveUp, 160, 218, 0.42f, withAlpha(theme::kShell, 0.7f), 300);
    textCentered(app, str::kChallenges, 108, 16, 0.5f, theme::kClutchGold, 200, Face::Title);
    (void)in;
}

void end(App& app, Set& s) {
    Run& r = run();
    // A finished run that's now the best: its ghost flies in the next runs.
    if (!s.quit && s.score > 0 && challenge::best(app.game, Challenge::SkyRings, s.cup) == s.score) {
        saveGhost(s.cup, r.recording);
        r.best = r.recording;
    }
    s.riding = false;
}

}  // namespace ec::rings
