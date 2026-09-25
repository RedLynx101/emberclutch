// The 3D renderer (WP4, WP6): citro3d drawn inside a citro2d scene. Dragons use the skinning
// shader (dragon.v.pica); the den room uses the static shader (static.v.pica) with baked
// lighting for the time of day. See docs/tech/architecture.md section 4.
#pragma once

#include "app/app.hpp"
#include "core/behavior.hpp"
#include "core/den_actor.hpp"
#include "core/egg.hpp"
#include "core/items.hpp"
#include "core/particles.hpp"
#include "core/props.hpp"
#include "core/shell_burst.hpp"

namespace ec::r3d {

// Loads the shaders, both dragon forms (romfs:/models/*.ecm), the clips and the den room
// (romfs:/models/den.esm). Returns false, and the den keeps its 2D placeholder, if romfs or
// a dragon model is missing; without the room, dragons stand on the 2D backdrop.
bool init();
void shutdown();
// Right after each C3D_FrameBegin: frees the GPU memory released since the last frame began
// (the GPU may have been reading it until then).
void frameBegun();
bool ready();
bool roomReady();
bool eggReady();  // romfs:/models/egg.ecm loaded: eggs are 3D
// The hatching (WP12a): the burst egg's pieces, drawn in the den and up close with the colours
// of `of`'s egg (nullptr: none), and the pieces' shapes to burst them from (nullptr if the egg
// model has none).
void setBurst(const ShellBurst* burst, const Dragon* of);
const ShardShape* eggShards();

// Puts citro2d in the state the 3D pass relies on: 2D draws never write depth, so the
// depth buffer stays clear for dragons and 2D overlays always land on top. Call after
// C2D_Prepare().
void prepare2D();

// A dragon in the den and the actor animating it (nullptr: stands in its idle pose). An egg
// sits in the egg nest, moved by its EggMotion (eggs without one are skipped).
struct DenDragon {
    const Dragon* dragon;
    const DenActor* actor;
    const EggMotion* egg = nullptr;
    s8 nest = 0;  // the egg nest an egg sits in
};
constexpr int kDenShown = 5;  // three dragons and two eggs

// Draws the den on the current top-screen target: the room lit for the time of day, up to
// three dragons, and the particles (fx may be null): ambient ones behind the dragons, care
// effects over them. Flushes pending 2D first and hands the GPU back to citro2d after.
// The first dragon is the one being cared for (full detail); the camera follows them. With
// no dragon out, it looks at the egg nest. Fills app.stats.
void drawDen(App& app, const DenDragon* dragons, int count, s64 now, const Particles* fx);
// Poses those dragons ahead, before C3D_FrameBegin (which waits for the GPU to finish the last
// frame, so CPU work done before it runs alongside the GPU's). drawDen and drawCloseUp take
// this frame's poses from here; without them they pose on the spot. (WP11d: on the 3DS the
// CPU's and GPU's times added up, 18-22 ms in the full den.)
void poseAhead(App& app, const DenDragon* dragons, int count, s64 now);

// Bottom-screen close-up of the dragon's head and chest (petting, feeding: Face), or its
// whole body (grooming, the bath: Body), or of the whole egg you rub; same hand-over.
enum class CloseUpView : u8 { Face, Feed, Body };  // Feed: the face, centred on the mouth
void drawCloseUp(App& app, const Dragon& d, const DenActor* actor, const EggMotion* egg, s64 now,
                 CloseUpView view = CloseUpView::Face);

// Hands-on care (WP7): what's under the stylus on the dragon last drawn by drawCloseUp.
bool pickCloseUp(Vec2 touch, TouchHit& out);

// One dragon (idle, or `clip` in place: the Wanderings show the one out walking) or egg on the
// top screen over whatever 2D the scene drew first, turned `spin` radians toward the viewer's
// left, standing a little below centre: the Sanctuary, the Cold Vault, the Market's egg.
void drawShowcase(App& app, const Dragon& d, const EggMotion* egg, s64 now, float spin, ClipId clip = ClipId::Idle);
// Two dragons on the top screen, animated by their actors (standing where their behaviors
// put them), framed together: the Nesting Stone's pair.
void drawPair(App& app, const Dragon& a, const DenActor& actorA, const Dragon& b, const DenActor& actorB, s64 now);
// The dragon's mouth on the bottom screen (the last close-up); false if it isn't in view.
bool mouthOnCloseUp(Vec2& at);
// A bottom-screen point held out in front of the dragon's face, in its armature space (at
// the head's depth): where it looks when you hold food there.
Vec3 closeUpLocal(Vec2 touch);
// Den dragon i's mouth (den space) in the last drawDen; false if it was not drawn.
bool mouthOf(int i, Vec3& out);
// Props drawn with the dragons, in the den and up close: the ball (nullptr or inactive: none)
// and the bath tub.
void setProps(const Ball* ball, bool tubOut, Vec2 tubAt = {}, float tubSize = 0.95f);
// The den's bought things (WP7): toys on the floor (the rope may hang between two points
// instead: a tug-of-war, or carried crosswise in a mouth), the food in the bowl, and the
// decor in its five spots. Drawn with the dragons, in the den and up close (nullptr: none).
struct DenThings {
    bool toy[kToys] = {};
    Vec3 toyAt[kToys] = {};  // on the floor (the orb: its centre)
    float toyYaw[kToys] = {};
    Quat orbSpin{0, 0, 0, 1};
    bool ropeSpan = false;  // the rope runs from ropeA to ropeB
    Vec3 ropeA, ropeB;
    Food bowlFood = Food::Count;  // Count: the bowl is empty
    Item decor[kDecorSpots] = {Item::Count, Item::Count, Item::Count, Item::Count, Item::Count};
    float daylight = 1;  // 0 night .. 1 day: lanterns and moonflowers glow brighter at night
};
void setDenThings(const DenThings* things);
// Review R5 (D47): the dragons' style. 0 the current look, 1 surface, 2 shape, 3 bold (the
// ember-veined dragon); the variants' models are in romfs:/models/v1..v3. Dev builds switch
// it from the dev menu. False if a style's models are missing (the style stays as it was).
constexpr int kStyleCount = 4;
bool setStyle(int style);
int style();
const char* styleName(int style);
// Dev (D54, before WP12): the other looks' forms loaded as well, as a den of mixed looks will
// need, so the budget overlay shows the memory on the hardware. Toggles; true while loaded.
bool probeAllLooks();
// The den camera watches this point too (a thrown ball) while `weight` > 0.
void followInDen(Vec3 at, float weight);

// Projects a den-space point with the last den camera: top-screen pixels and pixels per
// den unit at that depth. False before the first drawDen or behind the camera.
bool project(Vec3 p, float& x, float& y, float& pixelsPerUnit);
// Den dragon i's head (den space) in the last drawDen; false if it was not drawn.
bool headOf(int i, Vec3& out);
// The dark beyond the room for the time of day (a citro2d colour).
u32 backdrop(s64 now);

// The dragon animation clips (romfs:/anims/dragon.eca), or nullptr if they failed to load,
// and the library index of each behavior clip for a body form.
const AnimLibrary* anims();
const int* clipIndex(int form);
// A body form's model (LOD0) and its clip binding, for measuring walking speeds.
const ModelData* model(int form);
const AnimBinding* binding(int form);

}  // namespace ec::r3d
