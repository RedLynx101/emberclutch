// Dev builds only: the budget overlay (top screen) and the dev menu (SELECT, bottom screen).
#pragma once

#include "app/app.hpp"

namespace ec {

// Budgets from docs/tech/architecture.md section 1 / docs/plan/alpha-1.md WP1.
constexpr float kBudgetFrameMs = 33.3f;  // 30 fps in 3D scenes
constexpr u32 kBudgetTris = 8000;
constexpr u32 kBudgetDraws = 40;
constexpr u32 kBudgetBones = 24;

void debugDrawOverlay(App& app);
// Draws and runs the dev menu if open. Returns true when it consumed the bottom screen.
bool debugMenu(App& app, const Input& in);

}  // namespace ec
