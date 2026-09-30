// App-wide context shared by every scene.
#pragma once

#include <3ds.h>
#include <citro2d.h>

#include "app/storage.hpp"
#include "core/villagers.hpp"
#include "core/care.hpp"
#include "core/den_actor.hpp"
#include "core/den_roster.hpp"
#include "core/dragon.hpp"
#include "core/dragondex.hpp"
#include "core/egg.hpp"
#include "core/shell_burst.hpp"
#include "core/particles.hpp"
#include "core/props.hpp"
#include "core/rng.hpp"
#include "core/save.hpp"
#include "core/wanderings.hpp"

#ifndef EC_DEV
#define EC_DEV 1  // dev builds show the budget overlay and the dev menu (SELECT); 0 is the player build (D135)
#endif
#ifndef EC_VERSION
#define EC_VERSION "0.0.0"  // (the Makefile's VERSION)
#endif

namespace ec {

constexpr float kTopW = 400, kBotW = 320, kScreenH = 240;
// The splash before the title (D68): our gold wordmark, just after the system's homebrew logo.
constexpr float kSplashSeconds = 2.6f;

struct Input {
    u32 down = 0, held = 0;
    float tx = 0, ty = 0;
    bool touching = false, tapped = false;
    float padX = 0, padY = 0;  // the circle pad, -1 .. 1 (up is +y), a small dead zone
    // The stylus lifted this frame, last seen at (rx, ry). Buttons fire on release: a press
    // can slide off to cancel, and a first touch frame never counts with a stale position.
    bool released = false;
    float rx = 0, ry = 0;
};

enum class SceneId : u8 {
    Title, PickStarter, Den, Map, Sanctuary, Vault, NestingStone, Wanderings, Market, Valley, Creator, Challenge,
    Wardrobe,  // the pageant: dressing a dragon (app/wardrobe.hpp)
    Count
};

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
enum class CarePage : u8 { None, Outing, Journal, Den };

struct CareState {
    Tool tool = Tool::Hand;
    Food food = Food::HearthBread;
    bool holdingFood = false;  // a food at the stylus (picked from the tray)
    int bitesLeft = 0;
    float biteWait = 0, chomp = 0;
    float chewT = 0;   // chewing a bite (run 15): into the current chew
    int chewLeft = 0;  // ...and the chews still to come
    StrokeTracker stroke;
    bool withGrain = true;  // the brush's last stroke went head to tail
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
    bool profileOpen = false;    // the profile (tap the heartglow): about it, its family (WP8)
    u8 profileTab = 0;           // 0 about, 1 family
    // The tray (D85): Care (the hand, the brush or the sponge, picked from its pop-up), Food,
    // Toys (the one shown, and its pop-up), then the three places.
    Tool careTool = Tool::Hand;
    bool careRow = false;
    Tool toy = Tool::Ball;
    bool toyRow = false;
    CarePage page = CarePage::None;
    u8 journalTab = 0;
    float swatWait = 0, tugWait = 0;  // between swats; between growls on the rope
    float ballTug = 0;                // seconds of pulling on the ball in its mouth (Noah, run 13)
    bool featherNear = false;         // the feather was close to its face when let go
    // The puzzle orb, rolled about the close-up (screen pixels): a treat drops out after
    // enough rolling, then it's empty for a while.
    Vec2 orbAt{230, 120}, orbVel;
    float orbSpin = 0, orbRolled = 0, orbWait = 0;
    bool orbHeld = false;
    Vec2 treatFrom;
    float treatT = -1;  // a treat on its way to the mouth, 0..1 (< 0: none)
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
// The hatching (WP12a): the egg shakes harder and harder, stills, flashes and bursts into
// bits; the hatchling takes shape out of a white blob where it stood, blinks, cries, and is
// named. The pieces lie in the nest until then, then sink away (scene_den.cpp).
struct HatchState {
    bool active = false;
    int index = -1;         // the hatching egg (SaveData::dragons)
    int nest = 0;           // its egg nest
    float t = 0;            // seconds into it
    float nextKnock = 0;
    bool skippable = false; // seen once before (settings.seenHatch)
    bool popped = false;    // the egg has burst: it has hatched
    bool blinked = false;
    bool asked = false;     // the keyboard was asked for
    bool named = false;     // the keyboard is done
    float flash = 0;        // the burst's warm flash, fading (1 .. 0)
    float sparkIn = 0;      // seconds to the next sparkle round the blob
    DexNews dex;            // what it added to the Dragondex (told once it's named)
    ShellBurst burst;       // the pieces of the shell
};

// The den's photo mode (D66, src/app/photo.cpp): the den holds still while it's open.
struct PhotoState {
    bool active = false;
    bool snap = false;  // this frame is the picture: drawn in its frame, nothing else over it
    float flash = 0;    // the shutter's white flash after, fading (1 .. 0)
    bool close = false; // the camera on the one you care for (else the whole den)
    bool tapped = false; // the big button: snapped at the next update (the top is drawn first)
};

// The 3DS keyboard runs between frames (it takes over both screens): main.cpp opens it for
// whatever a scene asked (src/app/keyboard.cpp).
enum class KeyboardFor : u8 { None, PlayerName, NameHatchling, Rename };

// START's system menu (src/app/system_menu.cpp): the game waits while it's open.
enum class MenuPage : u8 { Closed, Main, Settings, DeleteAsk, DeleteSure, Dex, Credits };

struct App {
    DialogueState talk;       // talking to someone in the valley (app/dialogue)
    bool fromValley = false;  // a place's scene was entered from the valley (leaving goes back out)
    SceneId scene = SceneId::Title;
    C3D_RenderTarget* top = nullptr;
    C3D_RenderTarget* topRight = nullptr;  // the right eye, drawn while the 3D slider is up (WP11e)
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

    // Den life (WP5; several dragons since Alpha 2 WP1, core/den_roster): an actor
    // (behavior + animation) for the dragon in each bed, set up for that dragon (actorId: 0
    // until it is), and the egg in each egg nest, how it rocks and how many cracks it had last
    // frame (-1: not seen yet, so cracks it already had make no sound).
    DenActor actors[kDenDragons];
    // The den's toys as the dragons see them (WP7, scene_den), and the orb rolling about.
    DenToys denToys;
    Ball denOrb;
    float denOrbWait = 0;  // seconds until the orb has another treat for them
    u32 actorId[kDenDragons] = {};
    EggMotion eggs[kDenEggs];
    int eggCracks[kDenEggs] = {-1, -1};
    // The one the bottom screen cares for (a dragon or an egg in the den): a SaveData::dragons
    // index, kept valid by fixCare.
    int careIndex = -1;
    // Den effects (WP6): the particle pool, the room's own emitters, and when each bed's
    // dragon next breathes out a "z" while asleep.
    Particles fx;
    DenAmbience ambience;
    float zzz[kDenDragons] = {};
    DenSocial social;  // the den's dragons' life together (core/behavior denSocial)
    // Hands-on care (WP7): the tool in hand, and the den's ball.
    CareState care;
    Ball ball;
    EggCare eggCare;
    HatchState hatch;
    PhotoState photo;
    KeyboardFor keyboard = KeyboardFor::None;
    u32 nameRoll = 0;  // the next name suggestion

    const char* toast = nullptr;
    float toastTime = 0;
    char toastText[64] = {};  // for toasts with a name in them (showToastf)
    char toastNext[64] = {};  // one more, shown when this one is gone (queueToastf)
    float saveFlash = 0;      // seconds the save icon still shows
    MenuPage menu = MenuPage::Closed;
    bool quit = false;        // Save & quit: leave after this frame
    u8 titleConfirm = 0;      // the title's "start over?" steps (0 none, 1 asked, 2 really?)
    float splash = kSplashSeconds;  // seconds of the splash left (scene_title.cpp); scripted runs skip it
    // The world map (Alpha 2): the place picked, and a trip under way (seconds left, where to).
    u8 mapPick = 0;
    u8 mapFrom = 0;   // where you are (the trip starts there)
    float travel = 0;
    SceneId travelTo = SceneId::Den;
    // The Sanctuary and the Cold Vault: the one picked (an index into their list) and the page.
    int storePick = 0, storePage = 0;
    // The Nesting Stone: the pair picked (SaveData indices, -1: none), their animation on the
    // stone (set up for these ids), and the courtship nuzzle's seconds left.
    int stoneMother = -1, stoneFather = -1;
    DenActor stoneActors[2];
    u32 stoneIds[2] = {};
    float stoneCourt = 0;
    // The Wanderings: the dragon picked to go, and what the last one found (shown until it's
    // been looked at).
    int wanderPick = -1;
    WanderFinds finds;
    int findsFrom = -1;  // who found them (SaveData index), -1: nothing to show
    bool storeProfile = false;  // the Sanctuary / Cold Vault: the picked one's profile (WP8)
    u8 storeProfileTab = 0;
    u8 marketTab = 0;    // the Market: food, goods, sell, the egg of the day
    u8 goodsPick = 0;    // the Market's goods (WP7): the item picked (core/items Item)...
    u8 goodsPage = 0;    // ...and the page of the stall
    // The Dragondex (WP12, dragondex_ui.cpp): the entry picked (breed, look) and the page.
    u8 dexPick = 0, dexLook = 0, dexPage = 0;

    // Debug
    bool overlay = EC_DEV;
    int autoTravel = -1;  // autotest: the valley's place to go to next (-1: none)
    float autoGoto[6] = {0, 0, 0, 0, 0, 0};  // autotest: somewhere in the valley to stand (x, y; [2] set; [3] [4] a point to face, [5] set)
    float autoView[7] = {-1};  // autotest: a free-camera view (place, eye x y z, target x y z; -1: none)
    // Dev (WP11d): leave out one part of the drawing (1 the room and its things, 2 the den's
    // dragons and eggs, 3 the close-up, 4 particles) to see its share of the GPU's time on the
    // 3DS; debug.hpp gpuProbeName. 0: everything drawn.
    u8 gpuProbe = 0;
    // The top screen's clear colour this frame (a scene's prepare can set it: the den's
    // backdrop, instead of a full-screen quad drawn over the clear). Reset after each frame.
    u32 topClear = 0;
    bool devMenu = false;
    bool stereoPreview = false;  // dev (WP11e): the top screen drawn for the right eye at full depth
    u8 devPage = 0;
    u32 devSteps = 0;  // dev: steps added to the pedometer (the emulator's never counts)
    RenderStats stats;
    u32 bottomTris = 0;  // last frame's bottom screen (the overlay is drawn before it)
    float frameMs = 16.7f;
    float frameWorst = 0;  // the slowest frame of the last second, and how many missed 30 fps
    u8 framesSlow = 0;
};

s64 nowLocal(const App& app);
void showToast(App& app, const char* msg);
void showToastf(App& app, const char* fmt, const char* arg);  // one %s
// After the toast showing now (at once if there's none). One waits at a time: a newer one
// takes its place.
void queueToastf(App& app, const char* fmt, const char* arg);
// Each frame: the toast fades on; the one queued follows it.
void tickToast(App& app);

inline bool hasDragon(const App& app) { return app.game.dragonCount > 0; }
// The dragon (or egg) the bottom screen cares for.
inline Dragon& activeDragon(App& app) { return app.game.dragons[app.careIndex > 0 ? app.careIndex : 0]; }
inline const Dragon& activeDragon(const App& app) { return app.game.dragons[app.careIndex > 0 ? app.careIndex : 0]; }
// Keeps careIndex on someone in the den (the first bed's dragon, else the first egg).
void fixCare(App& app);
// Its bed (a hatched dragon in the den), or -1; its actor once set up, or nullptr.
int careBed(const App& app);
DenActor* careActor(App& app);
// Its egg nest (an egg in the den), or -1.
int careNest(const App& app);
// Moves the care to the next (+1) or previous (-1) one in the den: beds, then nests.
void cycleCare(App& app, int dir);
// Opens the world map (from the den: X, or the system menu).
void openMap(App& app);  // Beta: out into the valley, at the place you're leaving
// Once a second from any scene where time runs: everyone's simulation, and the nesting pair's
// egg when its day comes (with a toast).
void tickWorld(App& app);
// The 3DS pedometer's total step count (0 where there's none), plus the dev menu's steps.
u32 stepCount(const App& app);

// Saves to the next A/B slot; shows a toast if the SD card write fails.
void saveNow(App& app);

// A fresh game in memory (the settings stay): nothing is written until the new egg is chosen.
void resetForNewGame(App& app);

}  // namespace ec
