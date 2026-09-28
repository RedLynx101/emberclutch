// What the challenges share (Beta WP8-WP11; scene_challenge.cpp and challenge_rings.cpp,
// challenge_lanterns.cpp, challenge_fruit.cpp): the stage, the valley round the arena or the
// orchard with you, your partner and the host (Wren, or Maple at the orchard) standing in it,
// a camera easing about, the effects, and the run's outcome. Each challenge plays on it and
// adds its own things (rings, crystal lanterns, fruit) for the frame; scene_challenge draws it.
#pragma once

#include "app/app.hpp"
#include "app/render3d.hpp"
#include "core/challenge_mesh.hpp"
#include "core/challenges.hpp"
#include "core/flight.hpp"
#include "core/particles.hpp"
#include "core/valley.hpp"

namespace ec::stage {

// A person on the stage: their clip, blinking.
struct Figure {
    Animator anim;
    float blink = 0, blinkIn = 2;
};

constexpr int kMaxProps = 40;

struct Set {
    const Valley* valley = nullptr;
    int place = kPlaceArena;
    Challenge pick = Challenge::SkyRings;
    int cup = challenge::kEmber;
    bool autoplay = false;  // scripted runs: it plays itself
    // Your partner: drawn where the challenge puts it, its clip, its gaits' natural speeds.
    int partner = -1;
    Dragon shown;
    DenActor actor;
    ClipId clip = ClipId::Count;
    bool speedsSet = false;
    float natWalk = 2.2f, natTrot = 4.0f, natRun = 6.0f;
    Vec3 dragonAt;
    float dragonHeading = 0, dragonPitch = 0, dragonRoll = 0;
    bool riding = false;  // you on its back (Sky Rings)
    // You, and the host.
    Vec3 youAt;
    float youHeading = 0;
    Figure you;
    bool hostShown = true;
    Villager host = Villager::Steward;
    Vec3 hostAt;
    float hostHeading = 0;
    Figure hostFig;
    // The camera, easing toward where it's wanted (camEase: how quickly; snapCam: at once).
    Vec3 eye, target, wantEye, wantTarget;
    float camEase = 3.0f;
    bool snapCam = true;
    // Effects: hearts, sparkles and puffs (core/particles), and the breath.
    Particles fx;
    challenge::BreathFx breath;
    char popup[48] = {};  // a line over the top screen ("+185 Leaping catch!")
    float popupT = 0;
    u32 popupColour = 0;
    // This frame's things (the challenge adds them each frame): props, and other dragons (the
    // Sky Rings rivals); the 3D's zero-parallax distance (0: your partner's).
    r3d::ChallengeProp props[kMaxProps];
    int propCount = 0;
    r3d::ValleyDragon others[r3d::kMaxOthers];
    int otherCount = 0;
    float focus = 0;
    // The run.
    float t = 0;          // seconds since it began
    bool finished = false;
    bool quit = false;    // given up (X): back to the picker, nothing recorded
    int score = 0;        // Sky Rings: tenths of a second (0: not finished)
    challenge::Outcome outcome = challenge::Outcome::TryAgain;
    char detail[64] = {}; // a line for the results ("2 missed", "5 of 6 rounds")
    char detail2[64] = {};  // a second line (Sky Rings: who won, the rivals' times)
};

Set& get();

// Helpers (scene_challenge.cpp).
Vec3 onGround(const Set& s, Vec2 local, float up = 0);  // a point in the place's frame, on the ground
float worldHeading(const Set& s, float local);          // a heading in the place's frame (0: facing its front, +Y)
float headingTo(Vec3 from, Vec3 to);                      // the heading looking from one point to another
void playDragon(Set& s, ClipId c, float fade = 0.25f, bool restart = false);
bool dragonClipDone(const Set& s);
// Moves its clip on (steps and wingbeats heard); `speed` sets a gait's pace (0: as it is).
void stepDragon(App& app, Set& s, float speed);
void playPerson(Figure& f, const char* clip, float rate = 1.0f, float fade = 0.2f, bool restart = false);
bool personClipDone(const Figure& f);
void addProp(Set& s, const r3d::ChallengeProp& p);
void popup(Set& s, const char* text, u32 colour);
// The dragon's mouth (from its head as last drawn), and the way it faces.
Vec3 mouthOf(const Set& s);
float dragonSize(const Set& s);  // its size, metres-ish (the kind's, smaller while young)
// The run's end: its score and outcome; the scene shows the results. Giving up: no results.
void finish(App& app, Set& s, int score, challenge::Outcome o, const char* detail);
void giveUp(Set& s);
// Hearts or sparkles about a point.
void burst(Set& s, Fx kind, Vec3 at, int count, float scale = 1.0f);

}  // namespace ec::stage

// Each challenge's play (challenge_rings.cpp, challenge_lanterns.cpp, challenge_fruit.cpp):
// begin sets up the stage; update runs a frame (and calls stage::finish at the end); scene adds
// this frame's things before the stage is drawn; hud draws over the top screen; bottom draws and
// handles the bottom screen.
namespace ec::rings {
void begin(App& app, stage::Set& s);
void update(App& app, stage::Set& s, const Input& in);
void scene(App& app, stage::Set& s);
void hud(App& app, stage::Set& s);
void bottom(App& app, stage::Set& s, const Input& in);
void end(App& app, stage::Set& s);  // leaving (its ghost saved if it's a new best)
}  // namespace ec::rings
namespace ec::lanterns {
void begin(App& app, stage::Set& s);
void update(App& app, stage::Set& s, const Input& in);
void scene(App& app, stage::Set& s);
void hud(App& app, stage::Set& s);
void bottom(App& app, stage::Set& s, const Input& in);
}  // namespace ec::lanterns
namespace ec::fruit {
void begin(App& app, stage::Set& s);
void update(App& app, stage::Set& s, const Input& in);
void scene(App& app, stage::Set& s);
void hud(App& app, stage::Set& s);
void bottom(App& app, stage::Set& s, const Input& in);
}  // namespace ec::fruit
