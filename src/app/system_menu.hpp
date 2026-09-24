// START's system menu (docs/design/screens-and-flow.md): resume, settings (music and sound
// volume, a note on the clock, deleting the save with two confirmations), save & quit. The
// game waits while it's open.
#pragma once

#include "app/app.hpp"

namespace ec {

// START opens it (and closes it again).
void toggleSystemMenu(App& app);

// The bottom screen while it's open.
void drawSystemMenu(App& app, const Input& in);

// Dims the top screen under it.
void dimTopForMenu(App& app);

}  // namespace ec
