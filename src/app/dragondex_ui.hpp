// The Dragondex screens (WP12), a page of the system menu: the book on the bottom screen,
// the dragon picked on the top (it replaces the scene's top screen while it's open).
#pragma once

#include "app/app.hpp"

namespace ec {

void openDex(App& app);
void drawDexTop(App& app);
void drawDexBottom(App& app, const Input& in);

}  // namespace ec
