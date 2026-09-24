// The 3DS software keyboard (swkbd) for naming (D27): a new hatchling's name, with a
// suggestion filled in, and renaming later.
#pragma once

#include "app/app.hpp"

namespace ec {

// Runs the keyboard for app.keyboard and clears the request. Call between frames, never
// inside C3D_FrameBegin/End: the applet takes over both screens until the player is done.
void runKeyboard(App& app);

}  // namespace ec
