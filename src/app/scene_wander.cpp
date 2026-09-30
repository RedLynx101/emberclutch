// The Wanderings (Alpha 2 WP4, GDD 11): the trailhead. Choose a juvenile-or-older dragon from
// the den and set off together; while it's out, the trail map shows how far you've walked
// (the 3DS pedometer); call it back to see what it found (core/wanderings).
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/storybook.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/tips_ui.hpp"
#include "app/ui_draw.hpp"
#include "core/den_roster.hpp"
#include "core/egg.hpp"
#include "core/genetics.hpp"
#include "core/kinds.hpp"

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

// ------------------------------------------------------------------ the top screen (1.0, D89)
// The trailhead as a storybook picture (workstream U): far mountains and hills under the time of
// day's sky, the trail winding from your feet into the distance past the same landmarks as the
// map below (the lily pond, the old copse, the ridge), a signpost where it starts. The one picked
// waits by the sign; the one out walks along the trail, further and smaller as your steps add up;
// back home, its finds lie on a picnic blanket.

// A point on the painted trail (0 at your feet .. 1 far off) and how wide it is there.
Vec2 paintedTrail(float t) {
    const float near = 1.0f - t;
    return {70 + 228 * t + 58 * std::sin(t * 5.4f) * near, 252 - 134 * (1.0f - near * near * std::sqrt(near))};
}
float trailWidth(float t) { return 3 + 38 * std::pow(1.0f - t, 1.4f); }

void drawTrailScene(App& app, const paint::Light& l) {
    namespace pal = theme::paint;
    paint::sky(l, app.t, 124);
    paint::clouds(l, app.t, 50, 3.4f);
    paint::mountains(l, 122, 3.0f);
    paint::hill(l, 70, 104, 320, 60, pal::kHillFar, 2.4f);
    paint::hill(l, 340, 98, 300, 70, pal::kHillFar, 2.4f);
    paint::hill(l, 30, 128, 380, 90, pal::kHillMid, 1.7f);
    paint::hill(l, 320, 118, 380, 100, pal::kHillMid, 1.7f);
    paint::hill(l, 200, 168, 520, 120, pal::kHillNear, 1.2f);
    // The trail, a band from your feet into the distance (its edges across the way it runs), with
    // a lighter middle where feet have worn it; each part at its depth in 3D.
    for (int band = 0; band < 2; ++band) {
        const u32 c = band ? paint::lit(l, pal::kPathLight, 0.75f) : paint::lit(l, pal::kPath);
        const float k = band ? 0.36f : 0.5f;  // half its width
        constexpr int kSteps = 48;
        Vec2 prevL{}, prevR{};
        for (int i = 0; i <= kSteps; ++i) {
            const float t = i / static_cast<float>(kSteps) * (band ? 0.9f : 1.0f);
            const Vec2 p = paintedTrail(t), q = paintedTrail(t + 0.01f);
            const float dx = q.x - p.x, dy = (q.y - p.y) * 3.0f;  // (flattened: the ground lies away from you)
            const float len = std::fmax(1e-3f, std::sqrt(dx * dx + dy * dy));
            const float w = trailWidth(t) * k, shift = r3d::eyeShift(1.0f + 1.6f * t);
            const Vec2 L{p.x + shift - dy / len * w, p.y + dx / len * w * 0.35f};
            const Vec2 R{p.x + shift + dy / len * w, p.y - dx / len * w * 0.35f};
            if (i > 0) {
                C2D_DrawTriangle(prevL.x, prevL.y, c, prevR.x, prevR.y, c, R.x, R.y, c, 0);
                C2D_DrawTriangle(prevL.x, prevL.y, c, R.x, R.y, c, L.x, L.y, c, 0);
            }
            prevL = L;
            prevR = R;
        }
    }
    // The landmarks, as on the map below: the lily pond, the old copse, the ridge's cairn, and a
    // little flag on the far hill where the long trail turns for home.
    const Vec2 pond = paintedTrail(0.2f), copse = paintedTrail(0.5f), ridge = paintedTrail(0.8f), end = paintedTrail(1.0f);
    const float ps = 1.0f - 0.2f * 0.8f, cs = 1.0f - 0.5f * 0.8f, rs = 1.0f - 0.8f * 0.8f;
    C2D_DrawEllipseSolid(pond.x + 50 * ps - 26 * ps, pond.y - 8 * ps, 0, 52 * ps, 16 * ps, paint::lit(l, pal::kPond));
    C2D_DrawEllipseSolid(pond.x + 50 * ps - 18 * ps, pond.y - 6 * ps, 0, 20 * ps, 5 * ps, paint::lit(l, pal::kCloud, 0.5f));
    for (int k = 0; k < 3; ++k)  // lily pads
        C2D_DrawEllipseSolid(pond.x + (42 + k * 10) * ps, pond.y - (4 - k % 2 * 4) * ps, 0, 6 * ps, 3 * ps, paint::lit(l, pal::kLeafLight));
    paint::tree(l, copse.x - 30 * cs, copse.y - 2, 50 * cs, 0, 1.8f);
    paint::tree(l, copse.x - 14 * cs, copse.y + 3, 60 * cs, 2, 1.8f);
    paint::tree(l, copse.x + 26 * cs, copse.y - 1, 46 * cs, 1, 1.8f);
    const float rsh = r3d::eyeShift(2.3f);
    C2D_DrawEllipseSolid(ridge.x + 12 * rs + rsh, ridge.y - 16 * rs, 0, 22 * rs, 16 * rs, paint::lit(l, pal::kRock));
    C2D_DrawEllipseSolid(ridge.x + 15 * rs + rsh, ridge.y - 26 * rs, 0, 15 * rs, 11 * rs, paint::lit(l, pal::kPebble));
    C2D_DrawEllipseSolid(ridge.x + 17 * rs + rsh, ridge.y - 33 * rs, 0, 10 * rs, 8 * rs, paint::lit(l, pal::kRock));
    const float esh = r3d::eyeShift(2.6f);
    C2D_DrawRectSolid(end.x + 6 + esh, end.y - 16, 0, 1.2f, 14, paint::lit(l, pal::kWoodDark));
    C2D_DrawTriangle(end.x + 7 + esh, end.y - 16, theme::kEmber, end.x + 15 + esh, end.y - 13, theme::kEmber, end.x + 7 + esh,
                     end.y - 10, theme::kEmber, 0);
    // Flowers in the near grass.
    for (int i = 0; i < 16; ++i) {
        const float fx = std::fmod(i * 97.0f + 13, 400.0f), fy = 196 + std::fmod(i * 29.0f, 40.0f);
        if (std::fabs(fx - paintedTrail(0.05f).x) < 34 && fy > 210) continue;  // not on the path
        C2D_DrawCircleSolid(fx, fy, 0, 2.2f, paint::lit(l, pal::kFlower[i % 3]));
        C2D_DrawCircleSolid(fx, fy, 0, 0.9f, paint::lit(l, pal::kCloud));
    }
    // The signpost where the trail begins.
    const u32 wood = paint::lit(l, pal::kWood), light = paint::lit(l, pal::kWoodLight);
    C2D_DrawRectSolid(26, 150, 0, 6, 72, wood);
    C2D_DrawTriangle(10, 156, light, 70, 156, light, 78, 166, light, 0);
    C2D_DrawTriangle(10, 156, light, 78, 166, light, 10, 176, light, 0);
    C2D_DrawTriangle(10, 176, light, 78, 166, light, 70, 176, light, 0);
    text(app, str::kTrailSign, 40, 159, 0.4f, theme::kDenPlum, C2D_AlignCenter, 56);
    paint::lantern(l, 29, 146, 5.5f, app.t);
}

// Where along the trail it's got to, in words.
const char* trailStage(float t) {
    if (t < 0.12f) return str::kTrailStart;
    if (t < 0.4f) return str::kTrailPond;
    if (t < 0.68f) return str::kTrailCopse;
    if (t < 0.95f) return str::kTrailRidge;
    return str::kTrailFar;
}

// The finds laid out on a picnic blanket (bottom right): Gleam, the trinkets, a wild egg.
void drawBlanket(App& app, const paint::Light& l) {
    namespace pal = theme::paint;
    const WanderFinds& f = app.finds;
    // A checked cloth lying on the grass: its far edge narrower (it lies away from you).
    constexpr float kTop = 172, kBottom = 232, kCols = 6, kRows = 3;
    auto edge = [](float v, float u) {  // (u across, v down) -> screen
        const float left = 222 - 22 * v, right = 362 + 26 * v;
        return Vec2{left + (right - left) * u, kTop + (kBottom - kTop) * v};
    };
    for (int r = 0; r < kRows; ++r)
        for (int c = 0; c < kCols; ++c) {
            const u32 col = paint::lit(l, (r + c) % 2 ? pal::kAwningCream : pal::kAwning[0]);
            const Vec2 a = edge(r / kRows, c / kCols), b = edge(r / kRows, (c + 1) / kCols);
            const Vec2 d = edge((r + 1) / kRows, c / kCols), e = edge((r + 1) / kRows, (c + 1) / kCols);
            C2D_DrawTriangle(a.x, a.y, col, b.x, b.y, col, e.x, e.y, col, 0);
            C2D_DrawTriangle(a.x, a.y, col, e.x, e.y, col, d.x, d.y, col, 0);
        }
    // What it found, in two rows: Gleam, the trinkets (how many on a little tag), a wild egg.
    struct Thing {
        int trinket;  // -1 Gleam, -2 the egg
        int count;
    } things[kTrinkets + 2];
    int n = 0;
    if (f.gleam > 0) things[n++] = {-1, 0};
    for (int k = 0; k < kTrinkets; ++k)
        if (f.trinkets[k]) things[n++] = {k, f.trinkets[k]};
    if (f.wildEgg >= 0 && f.wildEgg < app.game.dragonCount) things[n++] = {-2, 0};
    const int back = n > 4 ? (n + 1) / 2 : n;
    char line[16];
    for (int i = 0; i < n; ++i) {
        const bool front = i >= back;
        const int inRow = front ? n - back : back, at = front ? i - back : i;
        const Vec2 p = edge(front ? 0.72f : 0.34f, (at + 0.5f) / inRow);
        const Thing& t = things[i];
        C2D_DrawEllipseSolid(p.x - 9, p.y + 3, 0, 18, 5, withAlpha(theme::kDenPlum, 0.2f));  // its shadow
        if (t.trinket == -1) {
            paint::coinPile(p.x, p.y + 3, 13, 3 + static_cast<int>(f.gleam / 60));
        } else if (t.trinket == -2) {
            const Dragon& e = app.game.dragons[f.wildEgg];
            egg(p.x, p.y - 6, 15, 20, kindShell(e), kindGlow(e), 0.9f);
        } else {
            paint::trinket(static_cast<Trinket>(t.trinket), p.x - 3, p.y - 2, 15);
            std::snprintf(line, sizeof(line), "x%d", t.count);
            const float w = textWidth(app, line, 0.36f) + 6;
            panel({p.x + 3, p.y - 3, w, 12}, withAlpha(theme::kDenPlum, 0.85f));
            text(app, line, p.x + 6, p.y - 3, 0.36f, theme::kShell, C2D_AlignLeft);
        }
    }
}

void drawTop(App& app) {
    const s64 now = nowLocal(app);
    const paint::Light l = paint::lightFor(now);
    // The dragon in 3D from the scene's fourth frame (D119: the 3DS froze on this scene's first
    // frame twice, the GPU never finishing its top screen; the painting alone first).
    static float lastT = -1.0f;
    static int fresh = 0;
    if (app.t - lastT > 0.25f) fresh = 3;
    lastT = app.t;
    const bool show3d = r3d::ready() && fresh == 0;
    if (fresh > 0) --fresh;
    drawTrailScene(app, l);
    const int out = wandererIndex(app.game);
    char line[80], sub[80];
    static EggMotion none;
    if (app.findsFrom >= 0 && app.findsFrom < app.game.dragonCount) {  // back, its finds on the blanket
        const Dragon& d = app.game.dragons[app.findsFrom];
        if (show3d) {
            r3d::frameShowcase(1.7f, -70, 38);
            r3d::drawShowcase(app, d, &none, now, 0.35f * std::sin(app.t * 0.5f));
        }
        drawBlanket(app, l);
        const WanderFinds& f = app.finds;
        bool any = f.gleam > 0 || f.wildEgg >= 0;
        for (int k = 0; k < kTrinkets; ++k) any |= f.trinkets[k] > 0;
        std::snprintf(line, sizeof(line), any ? str::kBackWith : str::kBackEmpty, d.name);
        std::snprintf(sub, sizeof(sub), str::kWalked, static_cast<unsigned long>(f.steps));
        paint::caption(app, line, sub, 42);
    } else if (out >= 0) {  // out on the trail: walking along it, further and smaller with your steps
        const Dragon& d = app.game.dragons[out];
        const u32 steps = stepsSince(d, stepCount(app));
        const float t = std::fmin(1.0f, steps / kTrailSteps);
        const Vec2 p = paintedTrail(t * 0.92f);
        const float zoom = 2.6f + 6.0f * t;
        if (show3d) {
            r3d::frameShowcase(zoom, p.x - kTopW / 2, p.y - 118.0f / zoom - kScreenH / 2);
            r3d::drawShowcase(app, d, &none, now, 1.25f, ClipId::Walk);
        }
        std::snprintf(line, sizeof(line), str::kOutWandering, d.name);
        std::snprintf(sub, sizeof(sub), str::kStepsOut, static_cast<unsigned long>(steps));
        char both[128];
        std::snprintf(both, sizeof(both), "%s  -  %s", trailStage(t), sub);
        paint::caption(app, line, both, 198);
    } else if (app.wanderPick >= 0 && app.wanderPick < app.game.dragonCount) {  // the one picked, by the sign
        const Dragon& d = app.game.dragons[app.wanderPick];
        if (show3d) {
            r3d::frameShowcase(1.7f, -70, 38);
            r3d::drawShowcase(app, d, &none, now, 0.35f * std::sin(app.t * 0.5f));
        }
        std::snprintf(line, sizeof(line), str::kReadyToGo, d.name);
        std::snprintf(sub, sizeof(sub), str::kFindsHint, static_cast<int>(kStepsPerFind));
        paint::caption(app, line, sub, 198);
    }
    paint::hangingSign(app, str::kWanderings, 200, 6, 210);
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
        paint::trinket(static_cast<Trinket>(k), x, ty + 7, 11);  // (U: its little picture)
        text(app, line, x + 10, ty, 0.44f, theme::kShell, C2D_AlignLeft, 130);
        ++shown;
        any = true;
    }
    y += ((shown + 1) / 2) * 17.0f;
    if (f.wildEgg >= 0) {
        std::snprintf(line, sizeof(line), str::kFoundWildEgg, kindTitle(app.game.dragons[f.wildEgg]));
        egg(30, y + 8, 10, 13, kindShell(app.game.dragons[f.wildEgg]), kindGlow(app.game.dragons[f.wildEgg]), 0.8f);
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
    showTip(app, tips::kTipWanderings);  // U: the tutorial
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
                  fromRgb(kindGlow(app.game.dragons[out])));
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
