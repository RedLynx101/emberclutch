// App-wide context shared by every scene.
#pragma once

#include <3ds.h>
#include <citro2d.h>

#include "core/dragon.hpp"
#include "core/rng.hpp"

#ifndef EC_DEV
#define EC_DEV 1  // dev builds show the budget overlay and the dev menu (SELECT)
#endif

namespace ec {

constexpr float kTopW = 400, kBotW = 320, kScreenH = 240;

struct Input {
    u32 down = 0, held = 0;
    float tx = 0, ty = 0;
    bool touching = false, tapped = false;
};

// Dev-only save: a raw struct dump. Replaced by the versioned A/B format in WP8.
struct DevSave {
    char magic[4] = {'E', 'M', 'B', 'd'};
    u32 version = 2;  // bump whenever Dragon changes shape
    u32 dragonSize = sizeof(Dragon);
    s64 lastSim = 0;
    s64 devOffset = 0;
    u8 hasDragon = 0;
    Dragon dragon{};
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
    DevSave save{};
    Rng rng{1};
    float t = 0;   // seconds since boot, for animation
    float dt = 0;  // seconds since the last frame
    bool romfsOk = false;

    // Scene-local state that must survive between frames.
    int starterHover = 0;
    float lastTouchX = -1, lastTouchY = -1;
    float petCooldown = 0;
    float simAccum = 0, saveAccum = 0;

    const char* toast = nullptr;
    float toastTime = 0;

    // Debug
    bool overlay = EC_DEV;
    bool devMenu = false;
    RenderStats stats;
    float frameMs = 16.7f;
};

s64 nowLocal(const App& app);
void showToast(App& app, const char* msg);
bool loadDevSave(DevSave& out);
void writeDevSave(const DevSave& s);

}  // namespace ec
