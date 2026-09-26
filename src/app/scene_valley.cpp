// Skyreach Valley, Beta's technical test (WP1, docs/plan/beta.md): a grown dragon flown over a
// placeholder valley with the arcade controls, walking and running on the ground, the camera
// following, the sky and fog by the time of day, and on the bottom screen the valley from
// above with where you are. Reached from the dev menu (page 2: Valley test); a kind chosen on
// page 1 (Next kind) is the one flown. The places, the player on foot, riding and the real
// map come with the rest of Beta.
#include <cmath>
#include <cstdio>
#include <vector>

#include "app/audio.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/daylight.hpp"
#include "core/dragondex.hpp"
#include "core/flight.hpp"
#include "core/genetics.hpp"
#include "core/model.hpp"
#include "core/rig.hpp"
#include "core/valley.hpp"

namespace ec {
namespace {

struct ValleyScene {
    Valley valley;
    bool loaded = false, tried = false;
    Flight flight;
    ChaseCamera cam;
    DenActor flyer;
    ClipId clip = ClipId::Count;
    Dragon shown;
    FlightInput last;
    bool speedsSet = false;  // its walking speeds, measured on its own legs once its body is loaded
};

ValleyScene& vs() {
    static ValleyScene s;
    return s;
}

bool loadValleyFile(Valley& v) {
    FILE* f = std::fopen("romfs:/valley/skyreach.evl", "rb");
    if (!f) return false;
    std::vector<u8> data;
    u8 buf[4096];
    std::size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) data.insert(data.end(), buf, buf + n);
    std::fclose(f);
    return loadValley(data.data(), data.size(), v);
}

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

void leaveValley(App& app) {
    r3d::releaseValley();  // its tiles' memory back for the den
    app.scene = SceneId::Den;
}

// The sky by the time of day: its top, the horizon (the fog), and the light on the land.
struct Sky {
    Rgb top, horizon, tint;
};
Sky skyFor(s64 now) {
    const DayBlend b = dayBlend(now);
    const Sky day{{96, 160, 226}, {206, 228, 242}, {255, 255, 255}};
    const Sky evening{{84, 70, 128}, {244, 170, 120}, {255, 206, 176}};
    const Sky night{{12, 16, 40}, {40, 52, 86}, {110, 124, 176}};
    const float wd = b.weight(kLightDay), we = b.weight(kLightEvening), wn = 1.0f - wd - we;
    auto mix = [&](Rgb Sky::*c) {
        const Rgb a = day.*c, e = evening.*c, n = night.*c;
        return Rgb{static_cast<u8>(a.r * wd + e.r * we + n.r * wn), static_cast<u8>(a.g * wd + e.g * we + n.g * wn),
                   static_cast<u8>(a.b * wd + e.b * we + n.b * wn)};
    };
    return {mix(&Sky::top), mix(&Sky::horizon), mix(&Sky::tint)};
}

void update(App& app, const Input& in) {
    ValleyScene& s = vs();
    app.simAccum += app.dt;
    if (app.simAccum >= 1.0f) {
        tickWorld(app);
        app.simAccum = 0;
    }
    if (in.down & KEY_X) {  // back home
        audio::playSfx(audio::Sfx::Back);
        leaveValley(app);
        return;
    }
    if (!s.loaded) return;
    FlightInput fi;
    const float dpadX = (in.held & KEY_DRIGHT ? 1.0f : 0.0f) - (in.held & KEY_DLEFT ? 1.0f : 0.0f);
    const float dpadY = (in.held & KEY_DUP ? 1.0f : 0.0f) - (in.held & KEY_DDOWN ? 1.0f : 0.0f);
    fi.steer = clampf(in.padX + dpadX, -1, 1);
    fi.pitch = clampf(in.padY + dpadY, -1, 1);  // pushed up: nose down
    fi.bank = (in.held & KEY_R ? 1.0f : 0.0f) - (in.held & KEY_L ? 1.0f : 0.0f);
    fi.flap = in.held & KEY_A;
    fi.dive = in.held & KEY_B;
    s.last = fi;
    s.flight.update(fi, s.valley, app.dt);
    s.cam.update(s.flight, s.valley, app.dt);
    if (s.flight.tookOff) audio::playSfx(audio::Sfx::Flap, 0.9f);
    if (s.flight.landed) audio::playSfx(audio::Sfx::Thump, 0.8f);
    const AnimLibrary* lib = r3d::animsFor(s.shown);  // a kind's plan has its own clips
    const int* clips = r3d::clipIndexFor(s.shown, kFormGrown);
    if (!s.speedsSet && lib) {  // walk, trot and gallop at the speed its feet move (no skating)
        const int look = r3d::lookFor(s.shown);
        const ModelData* m = r3d::model(kFormGrown, look);
        const AnimBinding* bind = r3d::binding(kFormGrown, look);
        if (m && bind) {
            const int build = s.shown.genome.build < kModelBuilds ? s.shown.genome.build : kBuildNeutral;
            s.flyer.updateSpeeds(*m, *bind, *lib, clips, kFormGrown * r3d::kLookSlots + look, 1.0f, build,
                                 sizeScale(s.shown.genome), false);
            s.flight.walkSpeed = clampf(s.flyer.behavior.walkSpeed, 0.8f, 4.0f);
            s.flight.runSpeed = clampf(s.flyer.behavior.runSpeed, s.flight.walkSpeed * 2.0f, 14.0f);
            s.speedsSet = true;
        }
    }
    // Its wings: beating, gliding or swept back in a dive; on the ground standing, walking,
    // trotting or galloping, each played at the speed it's going.
    const DenBehavior& b = s.flyer.behavior;
    const float walk = s.flight.walkSpeed, run = s.flight.runSpeed;
    const float trot = b.trotSpeed > walk && b.trotSpeed < run ? b.trotSpeed : (walk + run) * 0.5f;
    ClipId want = ClipId::Idle;
    float natural = 0;  // the clip's own ground speed
    if (!s.flight.grounded) {
        want = s.flight.diving(fi) ? ClipId::FlyDive : s.flight.sinceFlap < 0.8f ? ClipId::FlyFlap : ClipId::FlyGlide;
    } else if (s.flight.speed > 0.15f) {
        const bool running = s.flight.speed > (trot + run) * 0.5f, trotting = s.flight.speed > walk * 1.25f;
        want = running ? ClipId::Gallop : trotting ? ClipId::Trot : ClipId::Walk;
        natural = running ? run : trotting ? trot : walk;
    }
    if (want != s.clip && lib) {
        const int index = clips[static_cast<int>(want)];
        if (index >= 0) s.flyer.anim.play(index, 0.3f, want != ClipId::FlyDive && want != ClipId::FlyGlide);
        s.clip = want;
    }
    s.flyer.anim.rate = natural > 0 ? clampf(s.flight.speed / natural, 0.5f, 1.6f) : 1.0f;
    if (lib) {
        u8 events[8];
        const int n = s.flyer.anim.update(*lib, app.dt, events, 8);
        for (int k = 0; k < n; ++k) {
            if (events[k] == kAnimFlap) audio::playSfx(audio::Sfx::Flap, 1.0f, 0.7f);
            if (events[k] == kAnimFootstep && s.flight.grounded) audio::playSfx(audio::Sfx::Step, 0.9f, 0.6f);
        }
    }
    s.flyer.eyes.update(0.0f, app.dt);
}

void drawTop(App& app) {
    ValleyScene& s = vs();
    const s64 now = nowLocal(app);
    const Sky sky = skyFor(now);
    verticalGradient(0, 0, kTopW, kScreenH, fromRgb(sky.top), fromRgb(sky.horizon));
    if (!s.loaded) {
        textCentered(app, "The valley couldn't be loaded.", 200, 120, 0.6f, theme::kShell, 380);
        return;
    }
    r3d::ValleyView view;
    view.valley = &s.valley;
    view.dragon = &s.shown;
    view.actor = &s.flyer;
    view.at = s.flight.pos;
    view.heading = s.flight.heading;
    view.pitch = s.flight.pitch;
    view.roll = s.flight.roll;
    view.eye = s.cam.eye;
    view.target = s.cam.target;
    view.fog = sky.horizon;
    view.tint = sky.tint;
    if (r3d::ready()) r3d::drawValley(app, view, now);
}

void drawBottom(App& app, const Input& in) {
    ValleyScene& s = vs();
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    textCentered(app, "Skyreach Valley (test)", 160, 12, 0.6f, theme::kClutchGold, 300, Face::Title);
    if (!s.loaded) return;
    // The valley from above, north up, and where you are (a heart pointing the way you face).
    constexpr float kMapX = 8, kMapY = 28, kMapSize = 150;
    if (const C2D_Image* map = r3d::valleyMap(s.valley)) {
        C2D_DrawImageAt(*map, kMapX, kMapY, 0.5f, nullptr, kMapSize / 128.0f, kMapSize / 128.0f);
        const float mx = kMapX + (s.flight.pos.x - s.valley.x0) / s.valley.size() * kMapSize;
        const float my = kMapY + (1.0f - (s.flight.pos.y - s.valley.y0) / s.valley.size()) * kMapSize;
        const Vec3 f = s.flight.forward();
        C2D_DrawLine(mx, my, theme::kShell, mx + f.x * 12, my - f.y * 12, theme::kShell, 2, 0.5f);
        heart(mx, my, 10, theme::kRose);
    }
    char line[64];
    const float above = s.flight.pos.z - s.valley.heightAt(s.flight.pos.x, s.flight.pos.y);
    std::snprintf(line, sizeof(line), "%s  %.0f m up  %.0f m/s", s.flight.grounded ? "On the ground" : "Flying", above,
                  s.flight.speed);
    text(app, line, 166, 32, 0.45f, theme::kShell, C2D_AlignLeft, 150);
    text(app, "Stamina", 166, 54, 0.42f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft);
    const Rect bar{166, 70, 146, 10};
    panel(bar, theme::kDenPlum);
    panel({bar.x, bar.y, bar.w * s.flight.stamina, bar.h}, theme::kClutchGold);
    const r3d::ValleyStats st = r3d::valleyStats();
    std::snprintf(line, sizeof(line), "tiles %d (%d tris), built %d", st.tiles, st.ground, st.built);
    text(app, line, 166, 86, 0.38f, withAlpha(theme::kShell, 0.6f), C2D_AlignLeft);
    // The frame time for run 14: smoothed, the worst of the last second, and how many were slow.
    std::snprintf(line, sizeof(line), "%.1f ms, worst %.0f, %d slow", app.frameMs, app.frameWorst, app.framesSlow);
    text(app, line, 166, 98, 0.38f, withAlpha(theme::kShell, 0.6f), C2D_AlignLeft);
    if (s.flight.grounded) {
        text(app, "Pad: walk and turn   B: run", 166, 114, 0.38f, theme::kShell, C2D_AlignLeft, 150);
        text(app, "A: take off", 166, 130, 0.38f, theme::kShell, C2D_AlignLeft, 150);
        text(app, "Walk off a cliff to glide", 166, 146, 0.38f, theme::kShell, C2D_AlignLeft, 150);
    } else {
        text(app, "Pad: steer   A: flap   B: dive", 166, 114, 0.38f, theme::kShell, C2D_AlignLeft, 150);
        text(app, "L/R: bank   let go: glide", 166, 130, 0.38f, theme::kShell, C2D_AlignLeft, 150);
        text(app, "Land slowly on flat ground", 166, 146, 0.38f, theme::kShell, C2D_AlignLeft, 150);
    }
    if (button(app, {166, 196, 146, 36}, "Home (X)", in)) {
        audio::playSfx(audio::Sfx::Back);
        leaveValley(app);
    }
}

}  // namespace

void openValley(App& app) {
    ValleyScene& s = vs();
    if (!s.tried) {
        s.tried = true;
        s.loaded = loadValleyFile(s.valley);
    }
    // Your dragon, grown (a stand-in of its breed and look if it's young yet).
    const Dragon& d = activeDragon(app);
    if (hasDragon(app) && d.stage == Stage::Adult) {
        s.shown = d;
    } else {
        s.shown = dexDragon(hasDragon(app) ? breedIndex(d.genome) : 0, hasDragon(app) ? d.look : static_cast<u8>(kLookClassic));
        if (hasDragon(app)) std::snprintf(s.shown.name, sizeof(s.shown.name), "%s", d.name);
    }
    s.flight = Flight{};
    s.cam = ChaseCamera{};
    s.flyer = DenActor{};
    s.clip = ClipId::Count;
    s.speedsSet = false;
    if (s.loaded)
        if (const ValleyPlaceInfo* den = s.valley.place(kPlaceDen)) {  // out on the grass before the cave mouth
            const Vec3 f{std::sin(den->heading), -std::cos(den->heading), 0};
            s.flight.pos = den->at + f * 40.0f;
            s.flight.pos.z = s.valley.heightAt(s.flight.pos.x, s.flight.pos.y);
            s.flight.heading = den->heading;
        }
    app.scene = SceneId::Valley;
}

const SceneFns kValleyScene{update, drawTop, drawBottom};

}  // namespace ec
