// The Wanderings (Alpha 2 WP4, GDD 11): the trailhead. Choose a juvenile-or-older dragon from
// the den and set off together; while it's out, the trail map shows how far you've walked
// (the 3DS pedometer); call it back to see what it found (core/wanderings).
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/den_roster.hpp"
#include "core/egg.hpp"
#include "core/genetics.hpp"

namespace ec {
namespace {

constexpr float kTrailSteps = 6000;  // the trail map's length: a good day's walk

u32 col(u8 r, u8 g, u8 b, float a = 1.0f) { return withAlpha(theme::rgba(r, g, b), a); }

// A point along the winding trail, t 0..1, on a w x h area at (x, y).
Vec2 trailAt(float t, float x, float y, float w, float h) {
    return {x + w * t, y + h * 0.5f + h * 0.38f * std::sin(t * 9.0f) * (1.0f - 0.3f * t)};
}

// Den dragons old enough to come along (and at home).
int candidates(const App& app, int* out) {
    const DenRoster r = denRoster(app.game);
    int n = 0;
    for (int b = 0; b < kDenDragons; ++b) {
        if (r.dragon[b] < 0 || r.away[b]) continue;
        const Dragon& d = app.game.dragons[r.dragon[b]];
        if (d.stage != Stage::Hatchling) out[n++] = r.dragon[b];
    }
    return n;
}

void update(App& app, const Input& in) {
    app.simAccum += app.dt;
    if (app.simAccum >= 1.0f) {
        tickWorld(app);
        app.simAccum = 0;
    }
    if (in.down & KEY_B) {
        app.findsFrom = -1;
        openMap(app);
        audio::playSfx(audio::Sfx::Back);
    }
}

// The trail painted across a strip: hills, the path, little landmarks, and a marker at `t`.
void drawTrail(float x, float y, float w, float h, float t, u32 marker) {
    C2D_DrawEllipseSolid(x - 20, y + h * 0.3f, 0, w * 0.6f, h * 0.9f, col(150, 180, 118, 0.6f));
    C2D_DrawEllipseSolid(x + w * 0.45f, y + h * 0.2f, 0, w * 0.6f, h, col(136, 170, 108, 0.6f));
    for (int i = 0; i <= 60; ++i) {
        const Vec2 p = trailAt(i / 60.0f, x, y, w, h);
        C2D_DrawCircleSolid(p.x, p.y, 0, 1.8f, col(150, 110, 70, 0.8f));
    }
    const Vec2 pond = trailAt(0.2f, x, y, w, h), copse = trailAt(0.5f, x, y, w, h), ridge = trailAt(0.8f, x, y, w, h);
    C2D_DrawEllipseSolid(pond.x - 9, pond.y + 6, 0, 18, 8, col(110, 160, 214));
    C2D_DrawCircleSolid(copse.x - 6, copse.y - 8, 0, 5, col(58, 104, 72));
    C2D_DrawCircleSolid(copse.x + 4, copse.y - 10, 0, 6, col(70, 118, 80));
    C2D_DrawTriangle(ridge.x - 12, ridge.y - 4, col(128, 124, 150), ridge.x + 12, ridge.y - 4, col(104, 98, 128), ridge.x,
                     ridge.y - 22, col(150, 146, 170), 0);
    const Vec2 m = trailAt(t > 1 ? 1 : t, x, y, w, h);
    glow(m.x, m.y, 10, marker, 0.9f);
    heart(m.x, m.y - 1, 9, marker);
}

void drawTop(App& app) {
    // The trailhead: a morning sky, hills, the path winding off.
    verticalGradient(0, 0, kTopW, kScreenH, col(150, 196, 236), col(248, 226, 190));
    const float hills = r3d::eyeShift(2.5f);  // in 3D: far behind, the path from near to far
    C2D_DrawEllipseSolid(-80 + hills, 120, 0, 300, 130, col(150, 182, 120));
    C2D_DrawEllipseSolid(180 + hills, 110, 0, 320, 140, col(132, 170, 108));
    C2D_DrawRectSolid(0, 200, 0, kTopW, 40, col(120, 156, 96));
    for (int i = 0; i < 26; ++i) {
        const float t = i / 25.0f;
        C2D_DrawCircleSolid(40 + 330 * t + r3d::eyeShift(0.9f + 1.4f * t), 226 - 110 * t + 12 * std::sin(t * 8), 0,
                            6 - 4 * t, col(196, 164, 116));
    }
    const s64 now = nowLocal(app);
    const int out = wandererIndex(app.game);
    int shown = -1;
    if (app.findsFrom >= 0) shown = app.findsFrom;          // back, with its finds
    else if (out >= 0) shown = out;                          // out on the trail: walking along it
    else if (app.wanderPick >= 0) shown = app.wanderPick;    // the one picked to go
    if (shown >= 0 && r3d::ready()) {
        static EggMotion none;
        if (shown == out && app.findsFrom < 0)  // seen side-on, as if passing along the path
            r3d::drawShowcase(app, app.game.dragons[shown], &none, now, 1.25f, ClipId::Walk);
        else
            r3d::drawShowcase(app, app.game.dragons[shown], &none, now, 0.35f * std::sin(app.t * 0.5f));
    }
    textCentered(app, str::kWanderings, 200, 26, 1.0f, theme::kDenPlum, 380, Face::Title);
    if (out >= 0 && app.findsFrom < 0) {  // out: a little heart far along the path
        char line[64];
        std::snprintf(line, sizeof(line), str::kOutWandering, app.game.dragons[out].name);
        textCentered(app, line, 200, 206, 0.55f, theme::kDenPlum, 380);
    }
}

void drawFinds(App& app, const Input& in) {
    const Dragon& d = app.game.dragons[app.findsFrom];
    const WanderFinds& f = app.finds;
    char line[80];
    std::snprintf(line, sizeof(line), str::kIsBack, d.name);
    textCentered(app, line, 160, 44, 0.65f, theme::kClutchGold, 300);
    std::snprintf(line, sizeof(line), str::kWalked, static_cast<unsigned long>(f.steps));
    textCentered(app, line, 160, 66, 0.45f, withAlpha(theme::kShell, 0.8f), 300);
    float y = 88;
    bool any = false;
    if (f.gleam > 0) {
        std::snprintf(line, sizeof(line), str::kFoundGleam, static_cast<unsigned long>(f.gleam));
        C2D_DrawCircleSolid(30, y + 7, 0, 6, theme::kClutchGold);  // a coin
        text(app, line, 42, y, 0.5f, theme::kShell, C2D_AlignLeft, 260);
        y += 20;
        any = true;
    }
    int shown = 0;  // trinkets in two columns
    for (int k = 0; k < kTrinkets; ++k) {
        if (f.trinkets[k] == 0) continue;
        std::snprintf(line, sizeof(line), "%s x%d", trinketName(static_cast<Trinket>(k)), f.trinkets[k]);
        const float x = shown % 2 ? 176.0f : 30.0f, ty = y + (shown / 2) * 17.0f;
        C2D_DrawCircleSolid(x, ty + 7, 0, 4.5f, theme::kSkyTeal);
        text(app, line, x + 10, ty, 0.44f, theme::kShell, C2D_AlignLeft, 130);
        ++shown;
        any = true;
    }
    y += ((shown + 1) / 2) * 17.0f;
    if (f.wildEgg >= 0) {
        std::snprintf(line, sizeof(line), str::kFoundWildEgg, breedName(app.game.dragons[f.wildEgg].genome));
        egg(30, y + 8, 10, 13, {250, 240, 225},
            heartglowColor(static_cast<Element>(app.game.dragons[f.wildEgg].genome.elementA)), 0.8f);
        text(app, line, 42, y, 0.5f, theme::kClutchGold, C2D_AlignLeft, 260);
        y += 20;
        any = true;
    }
    textCentered(app, any ? str::kToTheHoard : str::kFoundNothing, 160, 182, 0.42f, withAlpha(theme::kShell, 0.75f), 300);
    if (button(app, {90, 200, 140, 34}, str::kLovely, in) || (in.down & KEY_A)) {
        app.findsFrom = -1;
        app.wanderPick = -1;
    }
}

void drawBottom(App& app, const Input& in) {
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    textCentered(app, str::kWanderings, 160, 16, 0.75f, theme::kClutchGold, 300, Face::Title);
    if (app.findsFrom >= 0 && app.findsFrom < app.game.dragonCount) {
        drawFinds(app, in);
        return;
    }
    const int out = wandererIndex(app.game);
    char line[80];
    if (out >= 0) {  // out on the trails: how far, and call it back
        const u32 steps = stepsSince(app.game.dragons[out], stepCount(app));
        drawTrail(18, 40, 284, 110, steps / kTrailSteps,
                  fromRgb(heartglowColor(static_cast<Element>(app.game.dragons[out].genome.elementA))));
        std::snprintf(line, sizeof(line), str::kStepsSoFar, static_cast<unsigned long>(steps));
        textCentered(app, line, 160, 170, 0.55f, theme::kShell, 300);
        if (button(app, {16, 200, 150, 34}, str::kCallBack, in)) {
            app.finds = comeBack(app.game, out, stepCount(app), nowLocal(app), app.rng);
            app.findsFrom = out;
            audio::playSfx(audio::Sfx::Trill);
            if (app.finds.gleam > 0) audio::playSfx(audio::Sfx::Coin);
            if (app.finds.wildEgg >= 0) audio::playSfx(audio::Sfx::EggLay);
            if (app.game.dragons[out].location == Location::Sanctuary) showToast(app, str::kBackToSanctuary);  // its bed was lent
            saveNow(app);
        }
        if (button(app, {176, 200, 128, 34}, str::kMap, in)) openMap(app);
        return;
    }
    // Who comes along?
    int list[kDenDragons];
    const int n = candidates(app, list);
    bool pickOk = false;
    for (int k = 0; k < n; ++k) pickOk |= list[k] == app.wanderPick;
    if (!pickOk) app.wanderPick = n > 0 ? list[0] : -1;
    if (n == 0) {
        textCentered(app, str::kNoOneToWander, 160, 100, 0.48f, theme::kShell, 290);
    } else {
        text(app, str::kWhoComes, 160, 40, 0.5f, theme::kShell);
        for (int k = 0; k < n; ++k) {
            const Dragon& d = app.game.dragons[list[k]];
            const Rect r{60, 64.0f + k * 36, 200, 32};
            const bool picked = list[k] == app.wanderPick;
            panel(r, picked ? theme::kClutchGold : withAlpha(theme::kShell, 0.2f));
            char kind[40];
    kindName(d, kind, sizeof(kind));
    std::snprintf(line, sizeof(line), "%s  (%s %s)", d.name, kind, stageName(d.stage));
            textCentered(app, line, r.x + r.w / 2, r.y + r.h / 2, 0.46f, picked ? theme::kDenPlum : theme::kShell, r.w - 10);
            if (in.released && r.contains(in.rx, in.ry)) {
                app.wanderPick = list[k];
                audio::playSfx(audio::Sfx::Tap);
            }
        }
        const char* why = cantWander(app.game, app.wanderPick);
        textCentered(app, why ? why : str::kSetOffHint, 160, 180, 0.42f, why ? theme::kRose : withAlpha(theme::kShell, 0.8f),
                     300);
        if (button(app, {16, 200, 150, 34}, str::kSetOff, in) && !why) {
            if (setOff(app.game, app.wanderPick, stepCount(app), nowLocal(app))) {
                showToast(app, str::kOffWeGo);
                audio::playSfx(audio::Sfx::TrailDepart);
                fixCare(app);
                saveNow(app);
            }
        }
    }
    if (button(app, {176, 200, 128, 34}, str::kMap, in)) openMap(app);
}

}  // namespace

const SceneFns kWanderingsScene{update, drawTop, drawBottom};

}  // namespace ec
