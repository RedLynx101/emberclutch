// Scene table. Scenes are immediate-mode: drawBottom both draws and handles touch input.
#pragma once

#include "app/app.hpp"

namespace ec {

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
void openValleyAt(App& app, int place);
void resumeValley(App& app);  // Continue, left in the valley: back where you were
void openCreator(App& app, SceneId back);  // your look, then back to `back`

}  // namespace ec
