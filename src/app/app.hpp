// App-wide context shared by every scene.
#pragma once

#include <3ds.h>
#include <citro2d.h>

#include "app/storage.hpp"
#include "core/care.hpp"
#include "core/den_actor.hpp"
#include "core/dragon.hpp"
#include "core/egg.hpp"
#include "core/particles.hpp"
#include "core/props.hpp"
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

// Hands-on care (WP7, src/app/care_ui.cpp): the tool in hand and what it's doing.
struct CareFx {  // a 2D effect over the close-up
    Vec2 pos, vel;
    float life = 0, maxLife = 1, size = 4;
    u8 kind = 0;
};
struct CareState {
    Tool tool = Tool::Hand;
    Food food = Food::HearthBread;
    bool holdingFood = false;  // a food at the stylus (picked from the tray)
    int bitesLeft = 0;
    float biteWait = 0, chomp = 0;
    StrokeTracker stroke;
    GroomSession groom;
    bool onDragon = false;       // this contact started on the dragon
    bool reported = false;       // a rough stroke / a call was reported this contact
    TouchHit lastHit;            // (render3d) the last spot touched
    bool hadHit = false;
    float sweetTime = 0, sweetCooldown = 0, stillTime = 0;
    float soundWait = 0, heartWait = 0, petTick = 0;
    bool sweetFound = false;     // this visit
    bool bathOut = false;        // the tub is in the den
    float suds = 0;              // 0..1
    Vec2 samples[6];             // recent stylus positions (a flick throws the ball)
    float sampleT[6] = {};
    int sampleCount = 0;
    CareFx fx[64];
    int fxCount = 0;
    bool profileOpen = false;    // the little profile card (tap the heartglow): name, rename
};

// Egg care (WP7): listening for the heartbeat.
struct EggCare {
    float still = 0;      // seconds the stylus has rested on the egg (a long rest listens)
    float listening = 0;  // seconds of heartbeat left to hear
    float beatIn = 0;     // to the next "lub"
    float dubIn = -1;     // to its "dub" (< 0: none due)
};

// The hatching (WP7): the egg shakes harder and harder, the cap pops off, the hatchling climbs
// out of the shell, blinks at its first light, looks at you; then it's named.
struct HatchState {
    bool active = false;
    float t = 0;            // seconds into it
    float nextKnock = 0;
    bool skippable = false; // seen once before (settings.seenHatch)
    bool popped = false;    // the cap is off: it has hatched
    bool blinked = false;
    bool asked = false;     // the keyboard was asked for
    bool named = false;     // the keyboard is done
    EggMotion shell;        // the empty shell left in the nest
    float shellTime = 0;    // seconds it stays there
};

// The 3DS keyboard runs between frames (it takes over both screens): main.cpp opens it for
// whatever a scene asked (src/app/keyboard.cpp).
enum class KeyboardFor : u8 { None, PlayerName, NameHatchling, Rename };

// START's system menu (src/app/system_menu.cpp): the game waits while it's open.
enum class MenuPage : u8 { Closed, Main, Settings, DeleteAsk, DeleteSure };

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
    // Hands-on care (WP7): the tool in hand, and the den's ball.
    CareState care;
    Ball ball;
    EggCare eggCare;
    HatchState hatch;
    KeyboardFor keyboard = KeyboardFor::None;
    u32 nameRoll = 0;  // the next name suggestion

    const char* toast = nullptr;
    float toastTime = 0;
    char toastText[64] = {};  // for toasts with a name in them (showToastf)
    float saveFlash = 0;      // seconds the save icon still shows
    MenuPage menu = MenuPage::Closed;
    bool quit = false;        // Save & quit: leave after this frame
    u8 titleConfirm = 0;      // the title's "start over?" steps (0 none, 1 asked, 2 really?)

    // Debug
    bool overlay = EC_DEV;
    bool devMenu = false;
    bool denTest = false;  // dev: two stand-in dragons join the den (3-dragon budget check)
    RenderStats stats;
    float frameMs = 16.7f;
};

s64 nowLocal(const App& app);
void showToast(App& app, const char* msg);
void showToastf(App& app, const char* fmt, const char* arg);  // one %s

// Alpha 1 keeps one dragon; the den with several arrives in Alpha 2.
inline bool hasDragon(const App& app) { return app.game.dragonCount > 0; }
inline Dragon& activeDragon(App& app) { return app.game.dragons[0]; }
inline const Dragon& activeDragon(const App& app) { return app.game.dragons[0]; }

// Saves to the next A/B slot; shows a toast if the SD card write fails.
void saveNow(App& app);

// A fresh game in memory (the settings stay): nothing is written until the new egg is chosen.
void resetForNewGame(App& app);

}  // namespace ec
