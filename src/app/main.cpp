// Emberclutch — entry point: init, the scene loop, frame timing and the debug layer.
#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

#include <cstdio>
#include <cstdlib>

#include "app/app.hpp"
#include "app/audio.hpp"
#include "app/autotest.hpp"
#include "app/care_ui.hpp"
#include "app/keyboard.hpp"
#include "app/perf.hpp"
#include "app/debug.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/screenshot.hpp"
#include "app/system_menu.hpp"
#include "app/dragondex_ui.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/clock.hpp"
#include "core/dragon.hpp"
#include "core/items.hpp"

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
    circlePosition pad;
    hidCircleRead(&pad);
    auto axis = [](int v) { return std::abs(v) < 20 ? 0.0f : (v > 150 ? 1.0f : (v < -150 ? -1.0f : v / 150.0f)); };
    in.padX = axis(pad.dx);
    in.padY = axis(pad.dy);
    static float lastX = 0, lastY = 0;  // hidTouchRead reads (0, 0) once the stylus lifts
    in.released = hidKeysUp() & KEY_TOUCH;
    in.rx = lastX;
    in.ry = lastY;
    if (in.touching) lastX = in.tx, lastY = in.ty;
    return in;
}

// Which loop fits the moment (docs/audio/suno-music-brief.md).
const char* musicFor(const App& app) {
    if (app.scene == SceneId::Market) return "market-bustle";
    if (app.scene == SceneId::Valley) return "skyreach";  // batch 1's flight theme
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
    app.topRight = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
    gfxSet3D(true);  // the right eye is drawn only while the slider is up (WP11e)
    app.textBuf = C2D_TextBufNew(4096);
    if (romfsMounted) loadFonts();  // Nunito and Cinzel Decorative (the system font if missing)
    app.romfsOk = romfsMounted && romfsReady();
    if (app.romfsOk) r3d::init();  // otherwise the den keeps its 2D placeholder
    care::loadSprites();            // the care tray's tools and foods (built into the program)

    audio::init();  // silent if the DSP firmware is missing
    ptmuInit();     // the pedometer, for the Wanderings
    if (loadGame(app.game, app.slots) && hasDragon(app)) {
        const s64 now = nowLocal(app);
        for (u16 i = 0; i < app.game.dragonCount; ++i)  // catch up on time away
            simulate(app.game.dragons[i], app.game.lastSim, now, eggCooling(app.game));
        app.game.lastSim = now;
        settleDen(app.game);  // everyone in the den has a bed or a nest (Alpha 2)
        feedFromBowl(app.game, now);  // the hungry ate from the bowl while you were away
        fixCare(app);
        markVisit(activeDragon(app), now);
        dexSeeAll(app.game);  // a save from before the Dragondex: everyone hatched is in it
    }

    audio::setVolumes(app.game.settings.musicVolume, app.game.settings.sfxVolume);
    autotest::start(app);  // dev builds: a scripted run if sdmc:/3ds/emberclutch/autotest.txt exists
    if (autotest::active()) app.splash = 0;  // scripts start at the title (`splash` shows it)

    u64 lastTick = svcGetSystemTick();
    while (aptMainLoop()) {
        const u64 tick = svcGetSystemTick();
        const float ms = static_cast<float>(tick - lastTick) / (SYSCLOCK_ARM11 / 1000.0f);
        lastTick = tick;
        app.frameMs = app.frameMs * 0.9f + ms * 0.1f;  // smoothed for the overlay
        {  // and the hitches the smoothing hides (the valley's tiles are built as you fly, run 14)
            static float window = 0, worst = 0;
            static int slow = 0;
            if (ms > worst) worst = ms;
            if (ms > 34.0f) ++slow;
            if ((window += ms) >= 1000.0f) {
                app.frameWorst = worst;
                app.framesSlow = static_cast<u8>(slow > 255 ? 255 : slow);
                window = worst = 0;
                slow = 0;
            }
        }
        app.dt = ms > 100.0f ? 0.1f : ms / 1000.0f;     // clamp after suspend
        app.t += app.dt;
        tickToast(app);
        if (app.saveFlash > 0) app.saveFlash -= app.dt;

        const Input in = autotest::active() ? autotest::next(app) : readInput();
        if (in.down & KEY_START) toggleSystemMenu(app);
        if (in.down & KEY_Y) screenshot::request();  // anywhere, in every build: both screens to the SD card
        const bool paused = app.menu != MenuPage::Closed;  // the game waits under the menu

        perf::frameStart();
        if (r3d::ready()) r3d::loadNextLook();  // the other looks, a form a frame (the splash hides it)
        const SceneFns& scene = sceneFns(app.scene);
        if (scene.update && !app.devMenu && !paused) {
            perf::Scope timed(perf::Update);
            scene.update(app, in);
        }
        {
            perf::Scope timed(perf::Audio);
            audio::playMusic(musicFor(app));
            audio::update(app.dt);
        }

        app.stats.reset();
        C2D_TextBufClear(app.textBuf);
        if (const SceneFns& s = sceneFns(app.scene); s.prepare) s.prepare(app);  // alongside the GPU's last frame
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        r3d::frameBegun();  // the last frame is drawn: what it read can go now
        autotest::afterFrameBegin();  // last frame's picture is finished now
        screenshot::afterFrameBegin(app);

        // The top screen, once per eye while the 3D slider is up (WP11e: 30 fps is fine in 3D).
        // The right eye is drawn with no time passing, so nothing moves on twice.
        const u32 topClear = app.topClear ? app.topClear : theme::kDenPlum;
        app.topClear = 0;  // a scene's prepare sets it again
        const float slider = osGet3DSliderState();
        for (int eye = 0; eye < (slider > 0.0f ? 2 : 1); ++eye) {
            C3D_RenderTarget* target = eye ? app.topRight : app.top;
            const float dt = app.dt;
            if (eye) app.dt = 0;
            r3d::setEye(app.stereoPreview && !eye ? 1.0f : (slider > 0.0f ? (eye ? slider : -slider) : 0.0f));
            C2D_TargetClear(target, topClear);
            C2D_SceneBegin(target);
            perf::Scope timed(perf::Top);
            if (app.menu == MenuPage::Dex) {
                drawDexTop(app);  // the book's dragon instead of the scene
            } else {
                sceneFns(app.scene).drawTop(app);
                if (paused) dimTopForMenu(app);
            }
            if (!app.photo.snap) {  // the photo's picture has nothing over it
                drawToast(app);
                drawSaveIcon(app);
                if (!app.photo.active) debugDrawOverlay(app);
            }
            app.dt = dt;
        }
        r3d::setEye(0);

        const u32 topTris = app.stats.tris;
        C2D_TargetClear(app.bottom, theme::kDenPlum);
        C2D_SceneBegin(app.bottom);
        {
            perf::Scope timed(perf::Bottom);
            if (paused)
                drawSystemMenu(app, in);
            else if (!debugMenu(app, in))
                sceneFns(app.scene).drawBottom(app, in);
        }
        if (EC_DEV && app.overlay && in.touching) {  // where the game reads the stylus
            C2D_DrawRectSolid(in.tx - 8, in.ty - 0.5f, 0, 17, 1, theme::rgba(0, 255, 120));
            C2D_DrawRectSolid(in.tx - 0.5f, in.ty - 8, 0, 1, 17, theme::rgba(0, 255, 120));
        }

        app.bottomTris = app.stats.tris - topTris;
        autotest::beforeFrameEnd();
        screenshot::beforeFrameEnd(app);
        C3D_FrameEnd(0);
        if (app.keyboard != KeyboardFor::None) runKeyboard(app);  // between frames: it takes both screens
        if (app.quit) break;
    }

    if (hasDragon(app) && !app.quit) saveNow(app);  // (Save & quit has just saved)
    autotest::finish();
    screenshot::finish();
    audio::shutdown();
    ptmuExit();
    r3d::shutdown();
    care::freeSprites();
    freeFonts();
    C2D_TextBufDelete(app.textBuf);
    C2D_Fini();
    C3D_Fini();
    if (romfsMounted) romfsExit();
    gfxExit();
    return 0;
}
