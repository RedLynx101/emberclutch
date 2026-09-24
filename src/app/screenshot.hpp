// Screenshots, in every build (Noah, 2026-09-24): Y saves the game as it is, both screens and
// whatever is on them (the dev overlay too), as sdmc:/3ds/emberclutch/screenshots/shot_NNNN.bmp
// (400 x 480: the top screen above the bottom one, centred), and adds a line to
// screenshots/log.txt with what was going on: the scene, the frame time, triangles, free
// memory and the build. Pulled off the SD card after a test (FTP or a card reader), so what
// looked off on the 3DS can be seen here. (The den's photo mode, D55, is a separate thing.)
#pragma once

#include "app/app.hpp"

namespace ec::screenshot {

// Y was pressed: this frame is the picture, as drawn.
void request();
// Around each frame, like the autotest's shots: after both screens are drawn, which
// framebuffers the picture lands in (and the numbers for the log); once the next frame has
// begun (the transfer is done), saving it. The toast comes after, so it's never in the picture.
void beforeFrameEnd(const App& app);
void afterFrameBegin(App& app);

}  // namespace ec::screenshot
