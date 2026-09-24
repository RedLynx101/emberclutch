// The world map (docs/design/world-map-and-travel.md, Alpha 2): Skyreach Valley painted on
// the bottom screen with a pin for each place; tap one, then Go, and a short trip takes you
// there. The top screen shows the place picked. Places open as their packages arrive; the
// rest wait as grey pins.
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"

namespace ec {
namespace {

constexpr float kTripTime = 1.1f;  // seconds of travel between places

struct Place {
    const char* name;
    const char* blurb;
    float x, y;       // on the bottom screen
    SceneId scene;    // where Go takes you
    bool open;        // built yet
};

constexpr Place kPlaces[] = {
    {"The Den", "Home: your dragons, their beds and the egg nests.", 160, 138, SceneId::Den, true},
    {"The Sanctuary", "The keepers' meadow: dragons rest here, looked after.", 62, 96, SceneId::Sanctuary, true},
    {"The Cold Vault", "Eggs keep here, cool and waiting, until a nest is free.", 262, 62, SceneId::Vault, true},
    {"The Nesting Stone", "Where a pair lays an egg.", 214, 176, SceneId::Den, false},
    {"The Market", "Food, toys and treasures, and an egg of the day.", 92, 182, SceneId::Den, false},
    {"The Wanderings", "Trails to walk with your dragon, and what you find.", 276, 138, SceneId::Den, false},
};
constexpr int kPlaceCount = sizeof(kPlaces) / sizeof(kPlaces[0]);

u32 col(u8 r, u8 g, u8 b, float a = 1.0f) { return withAlpha(theme::rgba(r, g, b), a); }

// A soft blob of colour: a few overlapping ellipses (watercolour on parchment).
void wash(float x, float y, float w, float h, u32 c) {
    C2D_DrawEllipseSolid(x - w * 0.5f, y - h * 0.5f, 0, w, h, c);
    C2D_DrawEllipseSolid(x - w * 0.42f, y - h * 0.62f, 0, w * 0.7f, h * 0.9f, c);
    C2D_DrawEllipseSolid(x - w * 0.2f, y - h * 0.4f, 0, w * 0.75f, h * 0.85f, c);
}

void mountain(float x, float base, float w, float h) {
    C2D_DrawTriangle(x - w * 0.5f, base, col(128, 124, 150), x + w * 0.5f, base, col(104, 98, 128), x, base - h,
                     col(150, 146, 170), 0);
    C2D_DrawTriangle(x - w * 0.17f, base - h * 0.66f, col(250, 246, 240), x + w * 0.17f, base - h * 0.66f,
                     col(236, 232, 226), x, base - h, col(255, 255, 255), 0);
}

void tree(float x, float y, float s) {
    C2D_DrawCircleSolid(x, y, 0, 4.2f * s, col(58, 104, 72));
    C2D_DrawCircleSolid(x - 1.2f * s, y - 1.4f * s, 0, 2.6f * s, col(82, 132, 88));
}

// The valley, painted: parchment, the mountains to the north, meadows, woods, the river and
// its lake, and the paths from the den.
void drawValley(App& app) {
    verticalGradient(0, 0, kBotW, kScreenH, col(242, 230, 204), col(228, 208, 172));
    for (int i = 0; i < 7; ++i) mountain(20.0f + i * 48.0f, 46, 64, 34 + 10 * ((i * 5) % 3));
    wash(70, 102, 150, 90, col(168, 196, 128, 0.55f));   // the meadow
    wash(170, 150, 190, 90, col(186, 200, 120, 0.5f));   // around the den
    wash(250, 180, 150, 70, col(160, 180, 110, 0.45f));
    wash(118, 196, 120, 50, col(214, 186, 128, 0.5f));   // the market fields
    // the river from the mountains into the lake by the meadow
    for (int i = 0; i <= 40; ++i) {
        const float t = i / 40.0f;
        const float x = 300 - 200 * t + 14 * std::sin(t * 6.3f), y = 44 + 70 * t + 8 * std::cos(t * 5.0f);
        C2D_DrawCircleSolid(x, y, 0, 3.2f + 2.0f * t, col(118, 168, 214, 0.9f));
    }
    wash(96, 124, 46, 22, col(110, 162, 214, 0.95f));  // the lake
    static const float kWoods[][3] = {{18, 150, 1.1f}, {30, 162, 1.0f}, {14, 172, 0.9f}, {236, 110, 1.0f},
                                      {250, 100, 0.9f}, {300, 190, 1.1f}, {312, 176, 0.9f}, {140, 78, 0.9f},
                                      {128, 70, 0.8f}, {200, 92, 1.0f}};
    for (const auto& w : kWoods) tree(w[0], w[1], w[2]);
    // paths from the den, dotted
    for (int p = 1; p < kPlaceCount; ++p) {
        const Place& a = kPlaces[0];
        const Place& b = kPlaces[p];
        const float len = std::hypot(b.x - a.x, b.y - a.y);
        for (float s = 16; s < len - 14; s += 7) {
            const float t = s / len;
            C2D_DrawCircleSolid(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, 0, 1.3f, col(150, 110, 70, 0.7f));
        }
    }
    // a little compass rose
    C2D_DrawTriangle(157, 216, col(150, 110, 70), 163, 216, col(150, 110, 70), 160, 202, col(190, 60, 40), 0);
    C2D_DrawTriangle(157, 216, col(150, 110, 70), 163, 216, col(150, 110, 70), 160, 230, col(150, 110, 70), 0);
    (void)app;
}

void drawPins(App& app) {
    for (int p = 0; p < kPlaceCount; ++p) {
        const Place& pl = kPlaces[p];
        const bool picked = p == app.mapPick;
        const float r = picked ? 12.0f + std::sin(app.t * 4.0f) : 10.0f;
        const u32 fill = pl.open ? (p == 0 ? theme::kEmber : theme::kSkyTeal) : col(150, 144, 150);
        if (picked) C2D_DrawCircleSolid(pl.x, pl.y, 0, r + 5, withAlpha(theme::kClutchGold, 0.45f));
        C2D_DrawCircleSolid(pl.x, pl.y, 0, r + 2, theme::kShell);
        C2D_DrawCircleSolid(pl.x, pl.y, 0, r, fill);
        if (p == 0) heart(pl.x, pl.y, r * 0.9f, theme::kShell);  // home
        else C2D_DrawCircleSolid(pl.x, pl.y, 0, r * 0.35f, theme::kShell);
        textCentered(app, pl.name + 4, pl.x, pl.y + r + 9, 0.38f, pl.open ? theme::kDenPlum : col(120, 110, 120), 110);
    }
}

// The top screen: the place picked, painted in a few shapes over its sky.
void drawPlaceView(App& app, int p) {
    const bool night = false;
    (void)night;
    verticalGradient(0, 0, kTopW, kScreenH, col(120, 170, 220), col(250, 214, 170));
    C2D_DrawEllipseSolid(-60, 150, 0, 300, 140, col(128, 170, 104));
    C2D_DrawEllipseSolid(160, 160, 0, 320, 140, col(112, 156, 96));
    C2D_DrawRectSolid(0, 200, 0, kTopW, 40, col(104, 146, 88));
    switch (p) {
        case 0:  // the den: a cave mouth in the hill, warm light inside
            C2D_DrawEllipseSolid(110, 70, 0, 180, 150, col(150, 130, 120));
            C2D_DrawEllipseSolid(160, 120, 0, 80, 90, col(52, 36, 60));
            glow(200, 170, 40, theme::kEmber, 0.9f);
            break;
        case 1:  // the meadow: a fence and the keepers' hut
            for (int i = 0; i < 12; ++i) C2D_DrawRectSolid(20.0f + i * 32, 176, 0, 5, 30, col(150, 110, 70));
            C2D_DrawRectSolid(20, 184, 0, 360, 4, col(150, 110, 70));
            C2D_DrawRectSolid(270, 120, 0, 80, 60, col(210, 180, 140));
            C2D_DrawTriangle(260, 122, col(170, 90, 70), 360, 122, col(170, 90, 70), 310, 86, col(190, 100, 80), 0);
            C2D_DrawRectSolid(300, 150, 0, 18, 30, col(110, 80, 60));
            break;
        case 2:  // the vault: an icy cliff with a round door
            C2D_DrawTriangle(90, 210, col(170, 200, 230), 330, 210, col(150, 180, 214), 220, 30, col(230, 244, 255), 0);
            C2D_DrawCircleSolid(210, 170, 0, 26, col(96, 120, 160));
            C2D_DrawCircleSolid(210, 170, 0, 20, col(70, 90, 130));
            break;
        default:  // not open yet: a misty silhouette
            C2D_DrawEllipseSolid(120, 110, 0, 160, 110, col(170, 170, 190, 0.6f));
            break;
    }
    const Place& pl = kPlaces[p];
    textCentered(app, pl.name, 200, 28, 1.0f, theme::kDenPlum, 380, Face::Title);
    textCentered(app, pl.open ? pl.blurb : str::kNotOpenYet, 200, 222, 0.5f, theme::kDenPlum, 380);
}

void update(App& app, const Input& in) {
    if (app.travel > 0) {
        if ((app.travel -= app.dt) <= 0) {
            app.travel = 0;
            app.scene = app.travelTo;
            app.storePick = app.storePage = 0;
        }
        return;
    }
    if (in.down & KEY_B) {  // back home
        app.scene = SceneId::Den;
        audio::playSfx(audio::Sfx::Back);
    }
    if (in.down & KEY_DRIGHT) app.mapPick = static_cast<u8>((app.mapPick + 1) % kPlaceCount);
    if (in.down & KEY_DLEFT) app.mapPick = static_cast<u8>((app.mapPick + kPlaceCount - 1) % kPlaceCount);
}

void travel(App& app) {
    const Place& pl = kPlaces[app.mapPick];
    if (!pl.open) {
        audio::playSfx(audio::Sfx::Error);
        showToast(app, str::kNotOpenYet);
        return;
    }
    audio::playSfx(audio::Sfx::Flap);
    app.travel = kTripTime;
    app.travelTo = pl.scene;
}

void drawTop(App& app) {
    drawPlaceView(app, app.mapPick);
    if (app.travel > 0) {  // the trip: the view drifts away into warm light
        const float k = 1.0f - app.travel / kTripTime;
        C2D_DrawRectSolid(0, 0, 0.5f, kTopW, kScreenH, withAlpha(theme::kDenPlum, std::fmin(1.0f, k * 1.4f)));
        textCentered(app, kPlaces[app.mapPick].name, 200, 120, 0.9f, withAlpha(theme::kClutchGold, std::fmin(1.0f, k * 2)),
                     380, Face::Title);
    }
}

void drawBottom(App& app, const Input& in) {
    drawValley(app);
    drawPins(app);
    if (app.travel > 0) {
        C2D_DrawRectSolid(0, 0, 0.5f, kBotW, kScreenH,
                          withAlpha(theme::kDenPlum, std::fmin(1.0f, (1.0f - app.travel / kTripTime) * 1.4f)));
        return;
    }
    for (int p = 0; p < kPlaceCount; ++p)  // tap a pin to pick it (a second tap goes)
        if (in.released && std::hypot(in.rx - kPlaces[p].x, in.ry - kPlaces[p].y) < 16) {
            if (app.mapPick == p) {
                travel(app);
            } else {
                app.mapPick = static_cast<u8>(p);
                audio::playSfx(audio::Sfx::Tap);
            }
        }
    if (button(app, {212, 206, 100, 30}, str::kGo, in) || (in.down & KEY_A)) travel(app);
    if (button(app, {8, 206, 100, 30}, str::kHome, in)) {
        app.scene = SceneId::Den;
    }
}

}  // namespace

const SceneFns kMapScene{update, drawTop, drawBottom};

}  // namespace ec
