// Emberclutch — entry point: init, the scene loop, frame timing and the debug layer.
#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

#include <cstdio>

#include "app/app.hpp"
#include "app/audio.hpp"
#include "app/care_ui.hpp"
#include "app/debug.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/theme.hpp"
#include "core/clock.hpp"
#include "core/dragon.hpp"

using namespace ec;

namespace {

Input readInput() {
    hidScanInput();
    Input in;
    in.down = hidKeysDown();
    in.held = hidKeysHeld();
    touchPosition touch;
    hidTouchRead(&touch);
    in.touching = in.held & KEY_TOUCH;
    in.tapped = in.down & KEY_TOUCH;
    in.tx = touch.px;
    in.ty = touch.py;
    static float lastX = 0, lastY = 0;  // hidTouchRead reads (0, 0) once the stylus lifts
    in.released = hidKeysUp() & KEY_TOUCH;
    in.rx = lastX;
    in.ry = lastY;
    if (in.touching) lastX = in.tx, lastY = in.ty;
    return in;
}

// Which loop fits the moment (docs/audio/suno-music-brief.md).
const char* musicFor(const App& app) {
    if (app.scene != SceneId::Den || !hasDragon(app)) return "title-theme";
    const Dragon& d = activeDragon(app);
    return (d.stage == Stage::Egg || isNight(nowLocal(app))) ? "nestsong" : "den-hearth";
}

bool romfsReady() {
    FILE* f = std::fopen("romfs:/music/loops.json", "rb");
    if (!f) return false;
    std::fclose(f);
    return true;
}

}  // namespace

int main() {
    gfxInitDefault();
    const bool romfsMounted = R_SUCCEEDED(romfsInit());
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS * 2);
    C2D_Prepare();
    r3d::prepare2D();

    static App app;  // holds the whole save (200 dragons): keep it off the stack
    app.top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    app.bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    app.textBuf = C2D_TextBufNew(4096);
    app.romfsOk = romfsMounted && romfsReady();
    if (app.romfsOk) r3d::init();  // otherwise the den keeps its 2D placeholder
    care::loadSprites();            // the care tray's tools and foods (built into the program)

    audio::init();  // silent if the DSP firmware is missing
    if (loadGame(app.game, app.slots) && hasDragon(app)) {
        const s64 now = nowLocal(app);
        for (u16 i = 0; i < app.game.dragonCount; ++i)
            simulate(app.game.dragons[i], app.game.lastSim, now);  // catch up on time away
        app.game.lastSim = now;
        markVisit(activeDragon(app), now);
    }

    audio::setVolumes(app.game.settings.musicVolume, app.game.settings.sfxVolume);

    u64 lastTick = svcGetSystemTick();
    while (aptMainLoop()) {
        const u64 tick = svcGetSystemTick();
        const float ms = static_cast<float>(tick - lastTick) / (SYSCLOCK_ARM11 / 1000.0f);
        lastTick = tick;
        app.frameMs = app.frameMs * 0.9f + ms * 0.1f;  // smoothed for the overlay
        app.dt = ms > 100.0f ? 0.1f : ms / 1000.0f;     // clamp after suspend
        app.t += app.dt;
        if (app.toast && (app.toastTime -= app.dt) <= 0) app.toast = nullptr;

        const Input in = readInput();
        if (in.down & KEY_START) break;

        const SceneFns& scene = sceneFns(app.scene);
        if (scene.update && !app.devMenu) scene.update(app, in);
        audio::playMusic(musicFor(app));
        audio::update(app.dt);

        app.stats.reset();
        C2D_TextBufClear(app.textBuf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C2D_TargetClear(app.top, theme::kDenPlum);
        C2D_SceneBegin(app.top);
        sceneFns(app.scene).drawTop(app);
        debugDrawOverlay(app);

        C2D_TargetClear(app.bottom, theme::kDenPlum);
        C2D_SceneBegin(app.bottom);
        if (!debugMenu(app, in)) sceneFns(app.scene).drawBottom(app, in);
        if (EC_DEV && app.overlay && in.touching) {  // where the game reads the stylus
            C2D_DrawRectSolid(in.tx - 8, in.ty - 0.5f, 0, 17, 1, theme::rgba(0, 255, 120));
            C2D_DrawRectSolid(in.tx - 0.5f, in.ty - 8, 0, 1, 17, theme::rgba(0, 255, 120));
        }

        C3D_FrameEnd(0);
    }

    if (hasDragon(app)) saveNow(app);
    audio::shutdown();
    r3d::shutdown();
    care::freeSprites();
    C2D_TextBufDelete(app.textBuf);
    C2D_Fini();
    C3D_Fini();
    if (romfsMounted) romfsExit();
    gfxExit();
    return 0;
}
