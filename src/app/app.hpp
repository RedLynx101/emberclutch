// App-wide context shared by every scene.
#pragma once

#include <3ds.h>
#include <citro2d.h>

#include "app/storage.hpp"
#include "core/den_actor.hpp"
#include "core/dragon.hpp"
#include "core/egg.hpp"
#include "core/particles.hpp"
#include "core/rng.hpp"
#include "core/save.hpp"

#ifndef EC_DEV
#define EC_DEV 1  // dev builds show the budget overlay and the dev menu (SELECT)
#endif

namespace ec {

constexpr float kTopW = 400, kBotW = 320, kScreenH = 240;

struct Input {
    u32 down = 0, held = 0;
    float tx = 0, ty = 0;
    bool touching = false, tapped = false;
    // The stylus lifted this frame, last seen at (rx, ry). Buttons fire on release: a press
    // can slide off to cancel, and a first touch frame never counts with a stale position.
    bool released = false;
    float rx = 0, ry = 0;
};

enum class SceneId : u8 { Title, PickStarter, Den, Count };

// Per-frame counters the renderer fills in; the debug overlay checks them against the
// budgets in docs/tech/architecture.md section 1.
struct RenderStats {
    u32 tris = 0, draws = 0, maxBonesPerDraw = 0, particles = 0;
    void reset() { *this = RenderStats{}; }
};

struct App {
    SceneId scene = SceneId::Title;
    C3D_RenderTarget* top = nullptr;
    C3D_RenderTarget* bottom = nullptr;
    C2D_TextBuf textBuf = nullptr;
    SaveData game;  // large: App lives in static storage (see main.cpp)
    SaveSlots slots;
    Rng rng{1};
    float t = 0;   // seconds since boot, for animation
    float dt = 0;  // seconds since the last frame
    bool romfsOk = false;

    // Scene-local state that must survive between frames.
    int starterHover = 0;
    float lastTouchX = -1, lastTouchY = -1;
    float petCooldown = 0;
    float simAccum = 0, saveAccum = 0;

    // Den life (WP5): the dragon's actor (behavior + animation), plus two stand-ins for the
    // dev 3-dragon test.
    DenActor actors[3];
    bool actorsReady = false;
    // Den effects (WP6): the particle pool, the room's own emitters, and when each den
    // dragon next breathes out a "z" while asleep.
    Particles fx;
    DenAmbience ambience;
    float zzz[3] = {};
    // The egg before hatching: how it rocks, and how many cracks it had last frame (-1:
    // not seen yet, so cracks it already had make no sound).
    EggMotion egg;
    int eggCracks = -1;

    const char* toast = nullptr;
    float toastTime = 0;

    // Debug
    bool overlay = EC_DEV;
    bool devMenu = false;
    bool denTest = false;  // dev: two stand-in dragons join the den (3-dragon budget check)
    RenderStats stats;
    float frameMs = 16.7f;
};

s64 nowLocal(const App& app);
void showToast(App& app, const char* msg);

// Alpha 1 keeps one dragon; the den with several arrives in Alpha 2.
inline bool hasDragon(const App& app) { return app.game.dragonCount > 0; }
inline Dragon& activeDragon(App& app) { return app.game.dragons[0]; }
inline const Dragon& activeDragon(const App& app) { return app.game.dragons[0]; }

// Saves to the next A/B slot; shows a toast if the SD card write fails.
void saveNow(App& app);

}  // namespace ec
