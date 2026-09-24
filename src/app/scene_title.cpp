// Title screen: wordmark, glowing egg, "touch to begin".
#include <cmath>

#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"

namespace ec {
namespace {

void drawTop(App& app) {
    verticalGradient(0, 0, kTopW, kScreenH, theme::kDenPlum, theme::kDusk);
    embers(app.t, kTopW);
    const float pulse = 0.7f + 0.3f * std::sin(app.t * 2.5f);
    egg(200, 130, 64, 84, {255, 236, 205}, {255, 140, 40}, pulse);
    text(app, str::kGameTitle, 200, 22, 1.1f, theme::kClutchGold);
    text(app, str::kTagline, 200, 200, 0.5f, theme::kShell);
}

void drawBottom(App& app, const Input& in) {
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    panel({40, 90, 240, 60}, theme::kShell);
    const float a = 0.6f + 0.4f * std::sin(app.t * 3.0f);
    text(app, str::kTouchToBegin, 160, 108, 0.7f, withAlpha(theme::kDenPlum, a));
    text(app, str::kBuildLabel, 160, 212, 0.4f, theme::kAsh);
    if (in.tapped || (in.down & KEY_A)) app.scene = hasDragon(app) ? SceneId::Den : SceneId::PickStarter;
}

}  // namespace

const SceneFns kTitleScene{nullptr, drawTop, drawBottom};

}  // namespace ec
