#include "app/app.hpp"

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

void saveNow(App& app) {
    if (!saveGame(app.game, app.slots, nowLocal(app))) showToast(app, "Couldn't save to the SD card.");
}

const SceneFns& sceneFns(SceneId id) {
    switch (id) {
        case SceneId::PickStarter: return kStarterScene;
        case SceneId::Den: return kDenScene;
        default: return kTitleScene;
    }
}

}  // namespace ec
