#include "app/app.hpp"

#include <cstdio>

#include "app/audio.hpp"
#include "app/scenes.hpp"

namespace ec {
namespace {

constexpr s64 kNtpToUnix = 2208988800LL;  // osGetTime() counts from 1900-01-01

}  // namespace

s64 nowLocal(const App& app) {
    return static_cast<s64>(osGetTime() / 1000) - kNtpToUnix + app.game.devOffset;
}

void showToast(App& app, const char* msg) {
    app.toast = msg;
    app.toastTime = 3.0f;
}

void showToastf(App& app, const char* fmt, const char* arg) {
    std::snprintf(app.toastText, sizeof(app.toastText), fmt, arg);
    showToast(app, app.toastText);
}

void saveNow(App& app) {
    if (!saveGame(app.game, app.slots, nowLocal(app))) {
        showToast(app, "Couldn't save to the SD card.");
        audio::playSfx(audio::Sfx::Error);
    } else {
        app.saveFlash = 1.4f;
    }
}

void resetForNewGame(App& app) {
    const Settings keep = app.game.settings;
    app.game = SaveData{};
    app.game.settings = keep;
    app.actorsReady = false;
    app.care = CareState{};
    app.ball = Ball{};
    app.egg = EggMotion{};
    app.eggCracks = -1;
    app.eggCare = EggCare{};
    app.hatch = HatchState{};
    app.nameRoll = 0;
    app.denTest = false;
}

const SceneFns& sceneFns(SceneId id) {
    switch (id) {
        case SceneId::PickStarter: return kStarterScene;
        case SceneId::Den: return kDenScene;
        default: return kTitleScene;
    }
}

}  // namespace ec
