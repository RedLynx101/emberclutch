// Title screen: the wordmark over a glowing egg; Continue, or a new game (your name with the
// keyboard, then the egg). Starting over when there's a dragon asks twice.
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"

namespace ec {
namespace {

void drawTop(App& app) {
    verticalGradient(0, 0, kTopW, kScreenH, theme::kDenPlum, theme::kDusk);
    embers(app.t, kTopW);
    const float pulse = 0.7f + 0.3f * std::sin(app.t * 2.5f);
    egg(200, 138, 64, 84, {255, 236, 205}, {255, 140, 40}, pulse);
    textCentered(app, str::kGameTitle, 200, 42, 1.75f, theme::kClutchGold, 380, Face::Title);
    textCentered(app, str::kTagline, 200, 206, 0.52f, theme::kShell, 380);
}

void newGame(App& app) {
    app.keyboard = KeyboardFor::PlayerName;  // then the egg (keyboard.cpp)
}

void drawBottom(App& app, const Input& in) {
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
}

}  // namespace

const SceneFns kTitleScene{nullptr, drawTop, drawBottom};

}  // namespace ec
