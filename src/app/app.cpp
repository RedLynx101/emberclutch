#include "app/app.hpp"

#include <cstdio>

#include "app/audio.hpp"
#include "app/scenes.hpp"
#include "core/genetics.hpp"

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
    for (u32& id : app.actorId) id = 0;
    for (EggMotion& e : app.eggs) e = EggMotion{};
    for (int& c : app.eggCracks) c = -1;
    app.careIndex = -1;
    app.care = CareState{};
    app.ball = Ball{};
    app.eggCare = EggCare{};
    app.hatch = HatchState{};
    app.nameRoll = 0;
}

void fixCare(App& app) {
    const SaveData& s = app.game;
    const int i = app.careIndex;
    if (i >= 0 && i < s.dragonCount && s.dragons[i].location == Location::Den) return;
    const DenRoster r = denRoster(s);
    app.careIndex = -1;
    for (int b = 0; b < kDenDragons && app.careIndex < 0; ++b) app.careIndex = r.dragon[b];
    for (int n = 0; n < kDenEggs && app.careIndex < 0; ++n) app.careIndex = r.egg[n];
    if (app.careIndex < 0 && s.dragonCount > 0) app.careIndex = 0;
}

int careBed(const App& app) {
    if (app.careIndex < 0 || app.careIndex >= app.game.dragonCount) return -1;
    const Dragon& d = app.game.dragons[app.careIndex];
    return d.location == Location::Den && d.stage != Stage::Egg && d.denSlot < kDenDragons ? d.denSlot : -1;
}

DenActor* careActor(App& app) {
    const int bed = careBed(app);
    return bed >= 0 && app.actorId[bed] == activeDragon(app).id ? &app.actors[bed] : nullptr;
}

int careNest(const App& app) {
    if (app.careIndex < 0 || app.careIndex >= app.game.dragonCount) return -1;
    const Dragon& d = app.game.dragons[app.careIndex];
    return d.location == Location::Den && d.stage == Stage::Egg && d.denSlot < kDenEggs ? d.denSlot : -1;
}

void cycleCare(App& app, int dir) {
    const DenRoster r = denRoster(app.game);
    int order[kDenDragons + kDenEggs], n = 0, at = -1;
    for (int b = 0; b < kDenDragons; ++b)
        if (r.dragon[b] >= 0) order[n++] = r.dragon[b];
    for (int e = 0; e < kDenEggs; ++e)
        if (r.egg[e] >= 0) order[n++] = r.egg[e];
    if (n < 2) return;
    for (int k = 0; k < n; ++k)
        if (order[k] == app.careIndex) at = k;
    if (DenActor* a = careActor(app)) {  // leaving it: a bath ends, a held food goes back
        if (app.care.bathOut) a->behavior.care(Care::BathDone, activeDragon(app));
        a->gazeWeight = 0;
        a->jawOpen = 0;
    }
    app.careIndex = order[((at < 0 ? 0 : at) + dir + n) % n];
    const bool card = app.care.profileOpen;  // switching from the profile card keeps it open
    app.care = CareState{};
    app.care.profileOpen = card && app.game.dragons[app.careIndex].stage != Stage::Egg;
    app.eggCare = EggCare{};
    audio::playSfx(audio::Sfx::Tap);
    const Dragon& d = activeDragon(app);
    if (d.stage == Stage::Egg)
        showToastf(app, "%s egg", breedName(d.genome));
    else
        showToastf(app, "%s", d.name);
}

const SceneFns& sceneFns(SceneId id) {
    switch (id) {
        case SceneId::PickStarter: return kStarterScene;
        case SceneId::Den: return kDenScene;
        default: return kTitleScene;
    }
}

}  // namespace ec
