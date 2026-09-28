// Driftwood Cove (1.0, D90; workstream C): fishing off the cove's shore and shells on its beach,
// as a valley feature (app/valley_ext: its line in valley_ext.cpp's kFeatures). The rules are
// core/fishing; cove.cpp plays them.
#pragma once

#include "app/app.hpp"
#include "app/valley_ext.hpp"

namespace ec {
struct Valley;
}  // namespace ec

namespace ec::cove {

// The feature's hooks (valley_ext's Feature).
int folk(const App& app, const Valley& v, Vec3 near, float radius, vext::Folk* out, int cap);
void act(App& app, const vext::Folk& who, vext::Stage& stage);
bool active(const App& app);
void update(App& app, const Input& in, vext::Stage& stage);
void drawTop(App& app, const vext::Stage& stage);
void drawBottom(App& app, const Input& in, const vext::Stage& stage);

// The cove's things in 3D, after drawValley (scene_valley calls it every frame, fishing or not):
// the day's shells on the beach, and while fishing the bobber and the catch.
void drawCoveThings(App& app, const Valley& v, s64 now);

// Scripted runs (autotest `cove <what>`: 0 talk to Tam, 1 start fishing at the water's edge, 2
// pick up the first shell left today); `autoplay on` fishes by itself (strikes at the bite,
// reels carefully, casts again).
void autotest(App& app, int what);
void setAutoplay(bool on);

}  // namespace ec::cove
