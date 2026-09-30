// Scene table. Scenes are immediate-mode: drawBottom both draws and handles touch input.
#pragma once

#include "app/app.hpp"

namespace ec {

struct Valley;  // core/valley.hpp

struct SceneFns {
    void (*update)(App&, const Input&);  // simulation, may be null
    void (*drawTop)(App&);
    void (*drawBottom)(App&, const Input&);
    // Before the frame begins, while the GPU still draws the last one: CPU work drawing will
    // need (the den poses its dragons, WP11d). May be null.
    void (*prepare)(App&) = nullptr;
};

const SceneFns& sceneFns(SceneId id);

// Each scene file provides one of these.
extern const SceneFns kTitleScene;
extern const SceneFns kStarterScene;
extern const SceneFns kDenScene;
extern const SceneFns kMapScene;
extern const SceneFns kSanctuaryScene;
extern const SceneFns kVaultScene;
extern const SceneFns kNestingStoneScene;
extern const SceneFns kWanderingsScene;
extern const SceneFns kMarketScene;
extern const SceneFns kValleyScene;
extern const SceneFns kCreatorScene;

// Beta's technical test (WP1): into Skyreach Valley with your dragon, grown (scene_valley).
void openValley(App& app);
// Out into the valley, on foot before a place (core/valley ValleyPlace) with your partner.
void openValleyAt(App& app, int place);  // (not without a dragon: D127; it says so and stays)
int valleyPartner(const App& app);        // who'd come out with you (its index; -1: nobody can)
void resumeValley(App& app);  // Continue, left in the valley: back where you were
void backToValley(App& app);  // from a scene opened over it (the wardrobe): where you stood, the valley still loaded
void openCreator(App& app, SceneId back);  // your look, then back to `back`

// ---- The challenges (Beta WP8-WP11, scene_challenge.cpp): the notice boards by the arena and
// the orchard (A at one opens the picker there), the scene itself, and its music.
extern const SceneFns kChallengeScene;
bool challengeBoardNear(const Valley& v, Vec3 at, int& place, float& distance);
void drawChallengeBoards(App& app, const Valley& v, s64 now);  // after r3d::drawValley
void openChallenges(App& app, int place);
// A talk just ended that was Wren's hello or her "pick a challenge": the picker follows it.
bool wrenOpensChallenges(const App& app);
const char* challengeMusic(const App& app);
// The valley's music near a 1.0 place with its own (Suno batch 4, as each file arrives); null: the valley's.
const char* valleyPlaceMusic(const App& app);
// Scripted runs (autotest): straight into a cup; the challenges play themselves while on.
void openChallengeCup(App& app, int challenge, int cup);
void setChallengeAutoplay(bool on);
// What the valley lends them (scene_valley.cpp): its landscape once loaded (nullptr before), its
// sky for the time of day (the top, the horizon and fog, the light on the land).
const Valley* loadedValley();
void valleySky(s64 now, Rgb& top, Rgb& horizon, Rgb& tint);

}  // namespace ec
