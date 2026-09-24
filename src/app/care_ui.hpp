// The hands-on care screen (WP7, docs/design/care-interactions.md): the tool tray under the
// close-up, the tool in hand drawn at the stylus, and what each tool does to the dragon.
#pragma once

#include "app/app.hpp"
#include "app/render3d.hpp"

namespace ec::care {

bool loadSprites();
void freeSprites();
// A food's sprite centred at (x, y) (the Market draws them too).
void drawFood(Food f, float x, float y, float scale);

// Which close-up the tool in hand wants (the face for petting, feeding and play; the whole
// body for grooming and the bath).
r3d::CloseUpView view(const App& app);

// Once a frame from the den's update, before the dragons move: the ball's physics (and its
// sounds), the ball in a dragon's mouth, the tub, easing the dragon's gaze and jaw.
void update(App& app, Dragon& d);

// The bottom screen over the close-up: gauges, the tray, the tool at the stylus and its
// effects. Call after r3d::drawCloseUp.
void drawBottom(App& app, const Input& in, Dragon& d, s64 now);

}  // namespace ec::care
