// Sky Rings (Beta WP9, D74: ridden): you fly the course yourself on your grown partner, the arcade
// flight of the valley (core/flight) tuned by its Wing and Stamina (core/challenges courseTuning).
// A countdown hovering over the arena, then through the rings in order against the clock: the
// next one gold, a lift through each, three seconds for each missed; a wisp flies your best run
// beside you (its ghost, kept on the SD card per cup). The Ember cup's course climbs to the
// floating isles and ends at the high lantern, which your dragon breathes alight as you pass (the
// campaign's quest 6). The bottom screen maps the course.
#include <sys/stat.h>

#include <cmath>
#include <cstdio>
#include <vector>

#include "app/audio.hpp"
#include "app/challenge_stage.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/campaign.hpp"
#include "core/kinds.hpp"
#include "core/place_layout.hpp"
#include "core/world.hpp"

namespace ec::rings {
namespace {

using stage::Set;

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

struct Run {
    challenge::Course course;
    int courseCup = -1;
    const Valley* courseOf = nullptr;
    Flight flight;
    ChaseCamera cam;
    FlightInput last;
    FlightTuning tune;
    challenge::RingRun run;
    challenge::Ghost best;       // the best run's, from the SD card
    challenge::Ghost recording;  // this run's
    float countdown = 0;         // seconds to go
    int shownCount = 4;
    float flash = 0;             // the ring just flown through, glowing out
    int flashRing = -1;
    bool flashPassed = false;
    float endT = -1;             // seconds since the last ring (its flourish, then the results)
    float skimFor = 0, lookFor = 0;
    bool timeUp = false;
};

Run& run() {
    static Run r;
    return r;
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

// ---------------------------------------------------------------------- play
void rideClip(Set& s, const Flight& f, bool diving) {
    if (!f.grounded) stage::playDragon(s, diving ? ClipId::FlyDive : f.sinceFlap < 0.8f ? ClipId::FlyFlap : ClipId::FlyGlide, 0.3f);
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
    r.cam = ChaseCamera{};
    r.last = FlightInput{};
    r.tune = challenge::courseTuning(s.shown.stats[0] ? s.shown.stats[0] : 5, s.shown.stats[4] ? s.shown.stats[4] : 5);
    r.run = challenge::RingRun{};
    r.recording.clear();
    loadGhost(s.cup, r.best);
    r.countdown = 3.4f;
    r.shownCount = 4;
    r.flash = 0;
    r.flashRing = -1;
    r.endT = -1;
    r.timeUp = false;
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
                r.flight.speed = r.tune.flapSpeed;
                r.flight.sinceFlap = 0;
            }
        }
        r.flight.sinceFlap = 0;  // (its wings beat, holding it there)
        stage::playDragon(s, ClipId::FlyFlap, 0.2f);
    } else {
        if (r.countdown > -1.0f) r.countdown -= app.dt;  // (the "Go!" fades)
        FlightInput fi;
        if (s.autoplay) {
            fi = challenge::pilot(r.flight, r.course, r.run.next);
        } else {
            const float dpadX = (in.held & KEY_DRIGHT ? 1.0f : 0.0f) - (in.held & KEY_DLEFT ? 1.0f : 0.0f);
            const float dpadY = (in.held & KEY_DUP ? 1.0f : 0.0f) - (in.held & KEY_DDOWN ? 1.0f : 0.0f);
            fi.steer = clampf(in.padX + dpadX, -1, 1);
            fi.pitch = clampf(in.padY + dpadY, -1, 1);
            fi.bank = (in.held & KEY_R ? 1.0f : 0.0f) - (in.held & KEY_L ? 1.0f : 0.0f);
            fi.flap = in.held & KEY_A;
            fi.dive = in.held & KEY_B;
        }
        const bool wasDiving = r.flight.diving(r.last);
        r.last = fi;
        const Vec3 from = r.flight.pos;
        if (r.endT >= 0) {  // after the last ring: it slows to a hover, wings beating, level
            r.flight.speed *= 1.0f - std::fmin(1.0f, 1.8f * app.dt);
            r.flight.pos = r.flight.pos + r.flight.forward() * (r.flight.speed * app.dt);
            r.flight.pitch *= 1.0f - std::fmin(1.0f, 4.0f * app.dt);
            r.flight.roll *= 1.0f - std::fmin(1.0f, 4.0f * app.dt);
            r.flight.sinceFlap = 0;
        } else {
            r.flight.update(fi, va, app.dt, r.tune);
        }
        if (r.flight.grounded && r.endT < 0) {  // down on the ground mid-course: up again
            r.flight.grounded = false;
            r.flight.climb = 5.0f;
        }
        if (r.flight.diving(fi) && !wasDiving) audio::playSfx(audio::Sfx::DiveWhoosh);
        if (r.flight.splashed) audio::playSfx(audio::Sfx::SplashBig);
        r.skimFor -= app.dt;
        if (r.flight.skimming && r.skimFor <= 0) {
            audio::playSfx(audio::Sfx::WaterSkim);
            r.skimFor = 0.7f;
        }
        if (r.endT < 0) {
            const int before = r.run.next;
            const challenge::RingRun::Event e = r.run.step(r.course, from, r.flight.pos, app.dt);
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
        lookRound(app, s, r);
    }
    if (r.endT >= 0 && (r.endT += app.dt) > 2.2f) {  // the flourish over: the results
        char line[64];
        if (r.timeUp) {
            std::snprintf(line, sizeof(line), "%s", str::kTimeUp);
            stage::finish(app, s, 0, challenge::Outcome::TryAgain, line);
        } else {
            const float total = r.run.total();
            if (r.run.missed)
                std::snprintf(line, sizeof(line), "%.1f s (%d missed, +%d s)", total, r.run.missed,
                              r.run.missed * static_cast<int>(challenge::kMissPenalty));
            else
                std::snprintf(line, sizeof(line), "%.1f s, every ring!  (par %.1f s)", total, r.course.par);
            stage::finish(app, s, static_cast<int>(std::lround(total * 10.0f)), challenge::ringsOutcome(r.course, r.run), line);
        }
    }
    if (r.flash > 0) r.flash -= app.dt;
    // The dragon as flown, the camera behind.
    s.dragonAt = r.flight.pos;
    s.dragonHeading = r.flight.heading;
    s.dragonPitch = r.flight.pitch;
    s.dragonRoll = r.flight.roll;
    if (r.endT < 0) rideClip(s, r.flight, r.flight.diving(r.last));
    else stage::playDragon(s, ClipId::FlyFlap, 0.4f);
    stage::stepDragon(app, s, 0);
    if (r.endT < 0 || r.timeUp) {
        r.cam.update(r.flight, va, app.dt);
        s.wantEye = r.cam.eye;
        s.wantTarget = r.cam.target;
        s.snapCam = true;  // (the chase camera eases itself)
    } else {  // the flourish: the camera comes round beside it, the dragon high in the picture (the card's below)
        const Vec3 f = r.flight.forward(), side{-f.y, f.x, 0};
        s.wantEye = r.flight.pos + side * 8.0f - f * 1.0f + Vec3{0, 0, 1.6f};
        s.wantTarget = r.flight.pos + f * 1.5f + Vec3{0, 0, -1.4f};
    }
    // The wind high up, and the wings in a glide.
    const float surface = std::fmax(va.heightAt(r.flight.pos.x, r.flight.pos.y), va.water);
    const float high = std::fmax(0.0f, r.flight.pos.z - surface);
    audio::setBed(audio::Bed::WindHigh, clampf((high - 8.0f) / 60.0f, 0.0f, 0.8f) + clampf(r.flight.speed / 40.0f, 0.0f, 0.35f));
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
    // The next four, the next one gold; the one just flown through glowing out.
    const int from = r.endT >= 0 ? n : r.run.next;
    for (int k = from; k < n && k < from + 4; ++k) ring(k, 1.0f - 0.2f * (k - from), k == from);
    if (r.flash > 0 && r.flashRing >= 0 && r.flashRing < n) {
        const challenge::Ring& g = r.course.rings[r.flashRing];
        if (length(s.eye - g.at) > g.radius * 2.2f) {
            ring(r.flashRing, r.flashPassed ? 1.6f : 0.3f, false);
            s.props[s.propCount - 1].scale *= 1.0f + (0.6f - r.flash) * 0.8f;
        }
    }
}

void hud(App& app, Set& s) {
    Run& r = run();
    // Your best, a wisp flying beside you.
    Vec3 g;
    float gh;
    if (r.countdown <= 0 && r.endT < 0 && r.best.at(r.run.time, g, gh)) {
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
    // The next ring, when it's out of sight: an arrow at the edge pointing its way.
    if (r.run.next < static_cast<int>(r.course.rings.size()) && r.countdown <= 0 && r.endT < 0) {
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
            if (std::fabs(rel) > 2.2f) textCentered(app, str::kRingBehind, 200, 222, 0.45f, theme::kShell, 380);
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
    // The clock and the rings.
    char line[48];
    const float shown = r.run.time;
    std::snprintf(line, sizeof(line), "%.1f", shown);
    panel({8, 8, 92, 30}, withAlpha(theme::kDenPlum, 0.7f));
    textCentered(app, line, 54, 23, 0.8f, shown > r.course.par ? theme::kRose : theme::kShell, 88);
    std::snprintf(line, sizeof(line), str::kRingsCount, r.run.passed, static_cast<int>(r.course.rings.size()));
    panel({292, 8, 100, 30}, withAlpha(theme::kDenPlum, 0.7f));
    textCentered(app, line, 342, 23, 0.55f, theme::kClutchGold, 96);
    if (r.run.missed) {
        std::snprintf(line, sizeof(line), str::kRingsMissed, r.run.missed, r.run.missed * static_cast<int>(challenge::kMissPenalty));
        textCentered(app, line, 342, 46, 0.4f, theme::kRose, 110);
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
    const Vec2 me = mapAt(r.flight.pos);
    C2D_DrawLine(me.x, me.y, theme::kShell, me.x + std::sin(r.flight.heading) * 9, me.y + std::cos(r.flight.heading) * 9,
                 theme::kShell, 2, 0);
    heart(me.x, me.y, 8, theme::kRose);
    // The side: the cup, stamina, the help.
    const float x = 216;
    text(app, challenge::cupName(s.cup), x, 30, 0.45f, fromRgb(challenge::cupColour(s.cup)), C2D_AlignLeft, 100);
    char line[40];
    std::snprintf(line, sizeof(line), str::kParTime, r.course.par);
    text(app, line, x, 48, 0.4f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft, 100);
    const Rect bar{x, 68, 96, 8};
    panel(bar, theme::kTrack);
    panel({bar.x, bar.y, bar.w * r.flight.stamina, bar.h}, theme::kClutchGold);
    text(app, str::kHelpRings, x, 84, 0.34f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft, 100);
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
