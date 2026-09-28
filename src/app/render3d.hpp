// The 3D renderer (WP4, WP6): citro3d drawn inside a citro2d scene. Dragons use the skinning
// shader (dragon.v.pica); the den room uses the static shader (static.v.pica) with baked
// lighting for the time of day. See docs/tech/architecture.md section 4.
#pragma once

#include "app/app.hpp"
#include "core/behavior.hpp"
#include "core/den_actor.hpp"
#include "core/egg.hpp"
#include "core/items.hpp"
#include "core/kinds.hpp"
#include "core/particles.hpp"
#include "core/prop_mesh.hpp"
#include "core/props.hpp"
#include "core/shell_burst.hpp"

namespace ec {
struct Valley;  // core/valley.hpp
}  // namespace ec

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
// U (the Market's and the Wanderings' top screens): the next drawShowcase only, framed `zoom`
// times smaller (1: as it frames itself) with its middle (dx, dy) px from the screen's (+y down).
void frameShowcase(float zoom, float dx, float dy);
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
    bool breedBanner = false;  // a breed's banner in the banner spot (the Dragondex), in these colours
    PropLook breedLook;
    float daylight = 1;  // 0 night .. 1 day: lanterns and moonflowers glow brighter at night
};
void setDenThings(const DenThings* things);
// Every dragon is drawn as its kind (DR3; romfs:/dragons/<kind>/, its plan's clips). The classic
// look loads with init() (the fallback); call loadNextLook() once a frame: it loads one piece of
// a kind the save has and hasn't loaded yet (its clips, then a form at a time; false when all
// are in), so they arrive while the splash plays and a wanderer's kind is in before it walks in
// (run 17: a kind loading on the spot stalled the den). One still needed at once loads then.
// The old looks (D54) load only when the dev menu shows them.
bool loadNextLook(const SaveData& s);
// A kind wanted besides the save's own (the star dragon in the sky): read ahead the same way
// (-1: none); ready once all of it is in.
void wantKind(int kind);
bool kindReady(int kind);

// The den's view swung round (radians, + to the right) and tilted (+ higher) by the circle pad
// (D85); the scene eases it back to 0 when the pad is let go.
void setDenNudge(float yaw, float pitch);
// Dev: every dragon drawn in one look (-1: their own).
void setForceLook(int look);
int forceLook();
// The look slots: the genome's looks, then the new kinds (D77, core/kinds; romfs:/dragons/).
constexpr int kLookSlots = kLookCount + kMaxKinds;
// Dev (the dragon revamp's preview): every hatched dragon drawn as this kind in this colouring
// (variant 3 is the rare one; kind -1: back to their own looks).
void setDevKind(int kind, int variant);
int devKind();
int devVariant();
// The look a dragon is drawn in (its own, the dev menu's, or classic if missing).
int lookFor(const Dragon& d);
// The den camera watches this point too (a thrown ball) while `weight` > 0.
void followInDen(Vec3 at, float weight);
// Stereoscopic 3D (WP11e): which eye the top screen's 3D is drawn for and how far apart
// (-1 .. 1, the 3D slider with the left eye negative; 0 flat). main.cpp sets it per eye.
void setEye(float eye);
// How far (top-screen pixels, + to the right) this eye sees something at `depthOverFocus`
// times the distance to what the view frames (1: the dragon's depth; more: behind it). 2D
// drawn with the 3D (a showcase's platform, the hills behind) moves by it to sit at that
// depth instead of on the screen (Noah, run 13). 0 when flat. project() adds it itself.
float eyeShift(float depthOverFocus = 1.0f);
// Photo mode (D66): the den camera frames the one you care for (drawn first) alone.
void setDenClose(bool close);

// Projects a den-space point with the last den camera: top-screen pixels and pixels per
// den unit at that depth. False before the first drawDen or behind the camera.
bool project(Vec3 p, float& x, float& y, float& pixelsPerUnit);
// Den dragon i's head (den space) in the last drawDen; false if it was not drawn.
bool headOf(int i, Vec3& out);
// ...and the top of its back, from its chest to its hips.
bool backOf(int i, Vec3& chest, Vec3& hips);
// The dark beyond the room for the time of day (a citro2d colour).
u32 backdrop(s64 now);

// The dragon animation clips (romfs:/anims/dragon.eca), or nullptr if they failed to load,
// and the library index of each behavior clip for a body form.
const AnimLibrary* anims();
const int* clipIndex(int form);
// The same for the body a dragon is drawn with (a kind's plan has its own clips).
const AnimLibrary* animsFor(const Dragon& d);
const int* clipIndexFor(const Dragon& d, int form);
// A body form's model (LOD0) and its clip binding in a look, for measuring walking speeds
// (nullptr until that look is loaded).
const ModelData* model(int form, int look = 0);
const AnimBinding* binding(int form, int look = 0);

// Skyreach Valley (Beta WP1): the ground round the camera (tiles at three levels by distance,
// only those in view, built a few a frame and kept), the islands and the den's mouth, the
// dragon flying, then the water, all fogged into the sky's horizon colour. The scene draws
// the sky itself (2D) first.
// A person in the valley (core/people): who, where, which way, their clip (app-driven), colours.
struct PersonView {
    u8 form = 0;                // core/people Person
    Vec3 at;
    float heading = 0;          // radians (0 faces -Y), as the dragons
    const Animator* anim = nullptr;
    Rgb pal[kPalCount];
    s8 hair = -1;               // the player's style (-1: none)
    float blink = 0;            // 0 open .. 1 shut
    float scale = 1.0f;
    bool seated = false;        // riding: drawn on the flown dragon's seat
};
constexpr int kMaxPeopleShown = 8;
constexpr int kMaxGlints = 8;
// The people's clip library (romfs:/anims/person.eca), for the scenes to play clips.
const AnimLibrary* personAnims();
// A person on their own on the top screen, lit by the time of day (the creator).
void drawPersonShowcase(App& app, const PersonView& p, s64 now);

// Another dragon about (1.0): a challenger's in a battle, a wild one in Frostspire Hollow, a
// pageant rival, a racer in Sky Rings. Drawn as the wanderer, tipped by pitch (nose down +) and
// banked by roll.
struct ValleyDragon {
    const Dragon* dragon = nullptr;
    const DenActor* actor = nullptr;
    Vec3 at;
    float heading = 0, pitch = 0, roll = 0;
    float scale = 1;
    s8 lod = -1;  // the challenges (workstream C): 1 keeps a racer on the light model near or far (-1: by distance)
};
constexpr int kMaxOthers = 4;

struct ValleyView {
    const Valley* valley = nullptr;
    const Dragon* dragon = nullptr;    // the one flown
    const DenActor* actor = nullptr;   // its animation
    Vec3 at;                           // its feet
    float heading = 0, pitch = 0, roll = 0;
    Vec3 eye, target;                  // the camera
    Rgb fog{200, 225, 240};            // the sky at the horizon
    Rgb tint{255, 255, 255};           // the day's light on the land
    Vec3 shadowAt;                     // the ground (or the water) under it
    float shadow = 0, shadowRadius = 0;  // its shadow's darkness (0: none) and size, metres
    u32 lanternsLit = 0;               // the festival's lanterns alight (core/world, a bit per place)
    bool riderOn = false;              // you on its back
    bool youShown = false;             // you on foot: where, which way, how fast
    Vec3 you;
    float youHeading = 0, youSpeed = 0;
    const Dragon* marketEgg = nullptr;  // the egg of the day on the Market's stand (null: bought)
    Item goods[4] = {Item::Count, Item::Count, Item::Count, Item::Count};  // the goods stall (Count: sold out)
    bool lead = false;                   // your partner on its lead (too small to ride, D81)
    const Dragon* wanderer = nullptr;    // out on the Wanderings, seen along its loop (D69)
    const DenActor* wandererActor = nullptr;
    Vec3 wandererAt;
    float wandererHeading = 0;
    const Dragon* skyDragon = nullptr;   // the star dragon, circling high (the Lantern Festival)
    const DenActor* skyActor = nullptr;
    Vec3 skyAt;
    float skyHeading = 0;
    Vec3 glints[8];                      // the finds not yet taken, near enough to glint (WP7)
    int glintCount = 0;
    PersonView people[kMaxPeopleShown];  // you first, then the villagers
    int peopleCount = 0;
    ValleyDragon others[kMaxOthers];     // other dragons about (1.0)
    int otherCount = 0;
    // The challenges (workstream C): the 3D's zero-parallax distance (0: the flown dragon's). Fruit
    // Catch keeps it on you, so you don't split into two when your dragon runs far off (run 19).
    float focus = 0;
};
void drawValley(App& app, const ValleyView& view, s64 now);
void releaseValley();  // leaving the valley: its GPU memory back
struct ValleyStats {
    int tiles = 0, built = 0, ground = 0;  // drawn and built last frame; the ground's triangles
    int places = 0;                        // the places' triangles drawn
};
ValleyStats valleyStats();
// The valley from above (north up), for the bottom screen's map (nullptr if it can't be made).
const C2D_Image* valleyMap(const Valley& v);

// ---- The challenges (Beta WP8-WP11, app/scene_challenge.cpp): their things (core/challenge_mesh),
// drawn after drawValley with its camera, fog and depth. (The den's shelf of trophies and ribbons
// is drawn with the den's things, from the save.)
enum class PropKind : u8 { Ring, Crystal, Fruit, Basket, Board, Trophy, Shell, Bobber, Fish };  // (Shell ..: Driftwood Cove, workstream C)
struct ChallengeProp {
    PropKind kind = PropKind::Ring;
    u8 variant = 0;            // the fruit's kind, the trophy's challenge
    Vec3 at;
    float yaw = 0, pitch = 0;  // turned about +Z, then tipped about its own X (a ring's +Y: its way through)
    float roll = 0;            // then about its own Y (a fruit's tumble)
    float scale = 1;
    PropLook look;
};
void drawChallengeProps(App& app, const ChallengeProp* props, int count, Rgb fog, s64 now);

}  // namespace ec::r3d
