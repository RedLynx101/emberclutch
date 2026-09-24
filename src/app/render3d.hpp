// The 3D dragon renderer (WP4): citro3d with the skinning shader (dragon.v.pica), drawn
// inside a citro2d scene. See docs/tech/architecture.md section 4.
#pragma once

#include "app/app.hpp"
#include "core/den_actor.hpp"

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

// A dragon in the den and the actor animating it (nullptr: stands in its idle pose).
struct DenDragon {
    const Dragon* dragon;
    const DenActor* actor;
};

// Draws up to three dragons in the den on the current top-screen target: flushes pending
// 2D, renders in 3D, then hands the GPU back to citro2d. Eggs are skipped. Fills app.stats.
// The first dragon is the one being cared for (full detail); the camera follows them.
void drawDen(App& app, const DenDragon* dragons, int count, s64 now);

// Bottom-screen close-up of the dragon's head and chest (petting), same hand-over.
void drawCloseUp(App& app, const Dragon& d, const DenActor* actor, s64 now);

// The dragon animation clips (romfs:/anims/dragon.eca), or nullptr if they failed to load,
// and the library index of each behavior clip for a body form.
const AnimLibrary* anims();
const int* clipIndex(int form);
// A body form's model (LOD0) and its clip binding, for measuring walking speeds.
const ModelData* model(int form);
const AnimBinding* binding(int form);

}  // namespace ec::r3d
