// The den's photo mode (D55, D66; WP12): the camera button beside the heartglow. The den holds
// still and the interface hides; a snap (A, or the big button) saves the top screen in a gold
// frame with the dragon's name and the date, as sdmc:/3ds/emberclutch/photos/photo_NNNN.bmp
// (400 x 240, written by the screenshot module's thread). B, or Back, returns to the den.
#pragma once

#include "app/app.hpp"

namespace ec::photo {

inline bool active(const App& app) { return app.photo.active; }
void open(App& app);
// Instead of the den's own update while it's open: nothing moves.
void update(App& app, const Input& in);
// Over the den on the top screen (instead of its names and hints): the frame in the picture's
// frame, the shutter's flash after.
void drawTop(App& app, const Dragon& d, s64 now);
void drawBottom(App& app, const Input& in);
// The den's care screen: the little camera under the heartglow. True if it was tapped.
constexpr float kButtonX = 280, kButtonY = 36, kButtonW = 38, kButtonH = 28;
bool cameraButton(App& app, const Input& in);

}  // namespace ec::photo
