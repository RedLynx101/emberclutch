// Scene table. Scenes are immediate-mode: drawBottom both draws and handles touch input.
#pragma once

#include "app/app.hpp"

namespace ec {

struct SceneFns {
    void (*update)(App&, const Input&);  // simulation, may be null
    void (*drawTop)(App&);
    void (*drawBottom)(App&, const Input&);
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

}  // namespace ec
