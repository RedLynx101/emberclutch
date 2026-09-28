// The gentle tutorial's card (1.0, core/tips): a small parchment card that slides in at the top
// screen's upper left, stays a few seconds and slides away again. It never waits for a button,
// so play goes on under it. Each tip shows once (marked in the save as it's shown); several at
// once wait their turn.
#pragma once

#include "app/app.hpp"
#include "core/tips.hpp"

namespace ec {

// The first time only: the card for this tip (after the one showing, if any). Cheap to call
// every frame from wherever the moment is ("Energy is low", "in the valley").
void showTip(App& app, tips::Tip tip);
// Over the top screen after the scene, with the toast (main.cpp).
void drawTipCard(App& app);
// The settings' "Show tips again": every tip due once more, and nothing waiting.
void resetTips(App& app);
// While something has the whole top screen's upper part (a battle's health bars, a show), the card
// waits hidden and its time stands still (set every frame; false lets it go on).
void holdTips(bool hold);

}  // namespace ec
