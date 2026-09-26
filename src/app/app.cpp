#include "app/app.hpp"

#include <cstdio>
#include <cstring>

#include "app/audio.hpp"
#include "app/scenes.hpp"
#include "core/breeding.hpp"
#include "core/den_roster.hpp"
#include "core/genetics.hpp"
#include "core/items.hpp"
#include "core/kinds.hpp"
#include "core/campaign.hpp"
#include "core/valley.hpp"
#include "core/world.hpp"
#include "app/strings.hpp"

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

void queueToastf(App& app, const char* fmt, const char* arg) {
    std::snprintf(app.toastNext, sizeof(app.toastNext), fmt, arg);
    if (!app.toast) tickToast(app);
}

void tickToast(App& app) {
    if (app.toast && (app.toastTime -= app.dt) > 0) return;
    app.toast = nullptr;
    if (!app.toastNext[0]) return;
    std::snprintf(app.toastText, sizeof(app.toastText), "%s", app.toastNext);
    app.toastNext[0] = 0;
    showToast(app, app.toastText);
}

void saveNow(App& app) {  // the writing happens on the save thread (app/storage)
    if (!saveGameAsync(app.game, app.slots, nowLocal(app))) {
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
    world::startWorld(app.game);  // Beta: the den found, the first quest begun
    campaign::update(app.game);
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

u32 stepCount(const App& app) {
    u32 steps = 0;
    if (R_FAILED(PTMU_GetTotalStepCount(&steps))) steps = 0;  // no pedometer (the emulator)
    return steps + app.devSteps;
}

// The festival's gift (the campaign's end): a star-born egg, a Glimmermoth in its rare
// colouring with the Starborn trait, into a free nest (else the Cold Vault).
void giveStarEgg(App& app) {
    SaveData& s = app.game;
    if (s.dragonCount >= kMaxDragons) return;
    Dragon egg = makeEgg(s.nextId++, makePurebred(Element::Lumen, app.rng), rollSex(app.rng), nowLocal(app));
    const int kind = findKind("glimmermoth");
    rollKind(egg, kind >= 0 ? kind : 0, kindInfo(kind >= 0 ? kind : 0).rareVariant, app.rng);
    for (int t = 0; t < traitCount(); ++t)
        if (std::strcmp(traitName(t), "Starborn") == 0) {
            egg.traits[0] = static_cast<u8>(t);
            if (egg.traitCount == 0) egg.traitCount = 1;
        }
    egg.origin = Origin::Festival;
    if (!placeEgg(s, egg) && vaultCount(s) >= kVaultEggs) return;
    s.dragons[s.dragonCount++] = egg;
    audio::playSfx(audio::Sfx::StarShimmer);
    queueToastf(app, str::kStarEgg, kindInfo(egg.kind).title);
    saveNow(app);
}

void tickWorld(App& app) {
    const s64 now = nowLocal(app);
    const float cooling = eggCooling(app.game);
    for (int i = 0; i < app.game.dragonCount; ++i) simulate(app.game.dragons[i], app.game.lastSim, now, cooling);
    app.game.lastSim = now;
    settleDen(app.game);
    if (app.scene != SceneId::Den) feedFromBowl(app.game, now);  // in the den they walk over to it
    // The hoard glints more as it grows.
    u32 trinkets = 0;
    for (u16 n : app.game.hoard) trinkets += n;
    app.ambience.hoard = trinkets > 60 ? 3.0f : trinkets / 20.0f;
    {  // the Lantern Festival follows the world (a Wandering home, a flag set elsewhere)
        const campaign::News n = campaign::update(app.game);
        if (n.finished >= 0) {
            audio::playStinger("quest-done");
            queueToastf(app, str::kQuestFinished, campaign::view(app.game, n.finished).title);
        } else if (n.stepped >= 0) {
            audio::playSfx(audio::Sfx::QuestPage);
        }
        if (n.starEgg) giveStarEgg(app);
        if (n.finished >= 0 || n.stepped >= 0) saveNow(app);
    }
    const int egg = layDueEgg(app.game, now, app.rng);  // the pair's egg, the day after they nested
    if (egg >= 0) {
        showToast(app, app.game.dragons[egg].location == Location::Den ? str::kNewEggNest : str::kNewEggVault);
        audio::playSfx(audio::Sfx::EggLay);
        saveNow(app);
    }
}

void openMap(App& app) {
    app.photo.active = false;
    {  // Beta: the valley is the world now: out before the place you're leaving
        int place = kPlaceDen;
        switch (app.scene) {
            case SceneId::Sanctuary: place = kPlaceSanctuary; break;
            case SceneId::Vault: place = kPlaceVault; break;
            case SceneId::NestingStone: place = kPlaceStone; break;
            case SceneId::Market: place = kPlaceMarket; break;
            case SceneId::Wanderings: place = kPlaceTrailhead; break;
            default: break;
        }
        audio::playSfx(audio::Sfx::MapOpen);
        openValleyAt(app, place);
        return;
    }
    // The places' order on the map (scene_map kPlaces): den, sanctuary, vault, stone, market, trails.
    switch (app.scene) {
        case SceneId::Sanctuary: app.mapFrom = 1; break;
        case SceneId::Vault: app.mapFrom = 2; break;
        case SceneId::NestingStone: app.mapFrom = 3; break;
        case SceneId::Market: app.mapFrom = 4; break;
        case SceneId::Wanderings: app.mapFrom = 5; break;
        default: app.mapFrom = 0; break;
    }
    app.mapPick = app.mapFrom;
    app.scene = SceneId::Map;
    app.travel = 0;
    audio::playSfx(audio::Sfx::MapOpen);
}

void fixCare(App& app) {
    const SaveData& s = app.game;
    const int i = app.careIndex;
    if (i >= 0 && i < s.dragonCount && s.dragons[i].location == Location::Den && s.dragons[i].wanderSince == 0) return;
    const DenRoster r = denRoster(s);
    app.careIndex = -1;
    for (int b = 0; b < kDenDragons && app.careIndex < 0; ++b)
        if (!r.away[b]) app.careIndex = r.dragon[b];
    for (int n = 0; n < kDenEggs && app.careIndex < 0; ++n) app.careIndex = r.egg[n];
    if (app.careIndex < 0 && s.dragonCount > 0) app.careIndex = 0;
}

int careBed(const App& app) {
    if (app.careIndex < 0 || app.careIndex >= app.game.dragonCount) return -1;
    const Dragon& d = app.game.dragons[app.careIndex];
    return d.location == Location::Den && d.stage != Stage::Egg && d.wanderSince == 0 && d.denSlot < kDenDragons
               ? d.denSlot
               : -1;
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
        if (r.dragon[b] >= 0 && !r.away[b]) order[n++] = r.dragon[b];
    for (int e = 0; e < kDenEggs; ++e)
        if (r.egg[e] >= 0) order[n++] = r.egg[e];
    if (n < 2) return;
    for (int k = 0; k < n; ++k)
        if (order[k] == app.careIndex) at = k;
    if (DenActor* a = careActor(app)) {  // leaving it: a bath ends, a held food goes back
        if (app.care.bathOut) a->behavior.care(Care::BathDone, activeDragon(app));
        a->gazeWeight = 0;
        a->jawOpen = 0;
        a->behavior.groomFacing = 0;
    }
    app.careIndex = order[((at < 0 ? 0 : at) + dir + n) % n];
    const bool card = app.care.profileOpen;  // switching from the profile card keeps it open
    app.care = CareState{};
    app.care.profileOpen = card && app.game.dragons[app.careIndex].stage != Stage::Egg;
    app.eggCare = EggCare{};
    audio::playSfx(audio::Sfx::Tap);
    const Dragon& d = activeDragon(app);
    if (d.stage == Stage::Egg)
        showToastf(app, "%s egg", kindTitle(d));
    else
        showToastf(app, "%s", d.name);
}

const SceneFns& sceneFns(SceneId id) {
    switch (id) {
        case SceneId::PickStarter: return kStarterScene;
        case SceneId::Den: return kDenScene;
        case SceneId::Map: return kMapScene;
        case SceneId::Sanctuary: return kSanctuaryScene;
        case SceneId::Vault: return kVaultScene;
        case SceneId::NestingStone: return kNestingStoneScene;
        case SceneId::Wanderings: return kWanderingsScene;
        case SceneId::Market: return kMarketScene;
        case SceneId::Valley: return kValleyScene;
        case SceneId::Creator: return kCreatorScene;
        default: return kTitleScene;
    }
}

}  // namespace ec
