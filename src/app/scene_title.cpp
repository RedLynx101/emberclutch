// Title screen: the wordmark over a glowing egg; Continue, or a new game (your name with the
// keyboard, then the egg). Starting over when there's a dragon asks twice.
// Before it, the splash (D68): EMBERCLUTCH in gold on black, straight after the system's
// homebrew logo (a boot logo of our own can't be signed), then the title fades up from black.
// Any button or a tap skips it.
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"

namespace ec {
namespace {

constexpr float kReveal = 0.45f;  // the title's fade up from black, the splash's last moments
constexpr const char* kSplashWord = "EMBERCLUTCH";

float smooth(float a, float b, float x) {
    x = (x - a) / (b - a);
    x = x < 0 ? 0 : (x > 1 ? 1 : x);
    return x * x * (3 - 2 * x);
}

void blackout(float w, float amount) {
    C2D_DrawRectSolid(0, 0, 0, w, kScreenH, withAlpha(theme::rgba(0, 0, 0), amount));
}

// The wordmark rises a little as it comes in, glows, and fades before the title.
void drawSplashTop(App& app) {
    const float t = kSplashSeconds - app.splash;  // seconds in
    const float a = smooth(0.15f, 0.8f, t) * (1.0f - smooth(1.7f, kSplashSeconds - kReveal, t));
    blackout(kTopW, 1.0f);
    for (int i = 16; i >= 1; --i)  // a soft ember glow: many faint rings (glow()'s five band on black)
        C2D_DrawCircleSolid(200, 118, 0, 150 * i / 16.0f, withAlpha(theme::kEmber, 0.035f * a));
    textCentered(app, kSplashWord, 200, 118 + 6 * (1 - a), 1.9f, withAlpha(theme::kClutchGold, a), 370, Face::Title);
}

void drawTop(App& app) {
    if (app.splash > 0) app.splash -= app.dt;
    if (app.splash > kReveal) {
        drawSplashTop(app);
        return;
    }
    verticalGradient(0, 0, kTopW, kScreenH, theme::kDenPlum, theme::kDusk);
    embers(app.t, kTopW);
    const float pulse = 0.7f + 0.3f * std::sin(app.t * 2.5f);
    egg(200, 138, 64, 84, {255, 236, 205}, {255, 140, 40}, pulse);
    textCentered(app, str::kGameTitle, 200, 42, 1.75f, theme::kClutchGold, 380, Face::Title);
    textCentered(app, str::kTagline, 200, 206, 0.52f, theme::kShell, 380);
    if (app.splash > 0) blackout(kTopW, app.splash / kReveal);
}

void newGame(App& app) {
    app.keyboard = KeyboardFor::PlayerName;  // then the egg (keyboard.cpp)
}

void drawBottom(App& app, const Input& in) {
    if (app.splash > kReveal) {
        blackout(kBotW, 1.0f);
        if (in.down) app.splash = kReveal;  // skipped: straight to the fade
        return;
    }
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    embers(app.t, kBotW);
    if (app.titleConfirm == 0) {
        if (hasDragon(app)) {
            if (button(app, {60, 62, 200, 46}, str::kContinue, in) || (in.down & KEY_A)) {
                app.scene = SceneId::Den;
                audio::playSfx(audio::Sfx::Confirm);
            }
            char line[48];
            std::snprintf(line, sizeof(line), "%s  -  %s", activeDragon(app).name, breedName(activeDragon(app).genome));
            textCentered(app, line, 160, 122, 0.45f, withAlpha(theme::kShell, 0.75f), 280);
            if (button(app, {90, 146, 140, 34}, str::kNewGame, in)) app.titleConfirm = 1;
        } else if (button(app, {60, 84, 200, 46}, str::kNewGame, in) || (in.down & KEY_A)) {
            newGame(app);
        }
    } else if (app.titleConfirm == 1) {  // there's a dragon: sure?
        textCentered(app, str::kStartOverAsk, 160, 62, 0.65f, theme::kShell, 296);
        char line[80];
        std::snprintf(line, sizeof(line), str::kStartOverBody, activeDragon(app).name);
        textCentered(app, line, 160, 92, 0.48f, withAlpha(theme::kShell, 0.8f), 296);
        if (button(app, {16, 150, 136, 40}, str::kKeepPlaying, in) || (in.down & KEY_B)) app.titleConfirm = 0;
        if (button(app, {168, 150, 136, 40}, str::kStartOver, in, theme::kRose)) app.titleConfirm = 2;
    } else {  // really?
        textCentered(app, str::kReally, 160, 80, 0.6f, theme::kShell, 296);
        if (button(app, {16, 150, 136, 40}, str::kNo, in) || (in.down & KEY_B)) app.titleConfirm = 0;
        if (button(app, {168, 150, 136, 40}, str::kYesStartOver, in, theme::kRose)) newGame(app);
    }
    text(app, str::kBuildLabel, 160, 216, 0.4f, theme::kAsh);
    if (app.splash > 0) blackout(kBotW, app.splash / kReveal);
}

}  // namespace

const SceneFns kTitleScene{nullptr, drawTop, drawBottom};

}  // namespace ec
