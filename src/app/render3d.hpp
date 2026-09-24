// The 3D dragon renderer (WP4): citro3d with the skinning shader (dragon.v.pica), drawn
// inside a citro2d scene. See docs/tech/architecture.md section 4.
#pragma once

#include "app/app.hpp"

namespace ec::r3d {

// Loads the shader and both dragon forms (romfs:/models/*.ecm). Returns false, and the den
// keeps its 2D placeholder, if romfs or a model is missing.
bool init();
void shutdown();
bool ready();

// Puts citro2d in the state the 3D pass relies on: 2D draws never write depth, so the
// depth buffer stays clear for dragons and 2D overlays always land on top. Call after
// C2D_Prepare().
void prepare2D();

// Draws up to three dragons in the den on the current top-screen target: flushes pending
// 2D, renders in 3D, then hands the GPU back to citro2d. Eggs are skipped. Fills app.stats.
void drawDen(App& app, const Dragon* const* dragons, int count, s64 now);

// Bottom-screen close-up of the dragon's head and chest (petting), same hand-over.
void drawCloseUp(App& app, const Dragon& d, s64 now);

}  // namespace ec::r3d
