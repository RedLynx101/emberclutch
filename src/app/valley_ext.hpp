// 1.0's features in the valley (D90): the people they stand about it (challengers, the Hollow's
// keeper, the pageant's hosts, the cove's fisher) and the times one takes the valley over for a
// while (a battle, a show, fishing). scene_valley asks every feature listed in valley_ext.cpp's
// kFeatures: a feature adds itself there with one line and keeps everything else in its own files.
#pragma once

#include "app/app.hpp"
#include "app/render3d.hpp"
#include "core/behavior.hpp"
#include "core/valley.hpp"

namespace ec::vext {

// Someone (or something: a sign, a jetty's end) a feature stands in the valley.
struct Folk {
    r3d::PersonView look;      // form, palette, hair, scale, and where: at, heading (anim is the scene's)
    bool shown = true;         // false: a spot with nothing drawn (a board, the end of a jetty)
    const char* name = "";     // for the prompt
    const char* prompt = "";   // what A does beside them, a format with one %s for the name ("Battle %s")
    u8 id = 0;                 // the feature's own
    float reach = 2.6f;        // metres: how near to stand (and turned toward them) for the prompt
    u8 voice = 0;              // for startSpeech
    float pitch = 1.0f;
};
constexpr int kMaxFolk = 24;   // all features together, per frame

// What the scene lends a feature while it has the valley: you and your partner (it may move and
// turn you both; the scene keeps them on the ground), the clips they play, the camera.
struct Stage {
    const Valley* valley = nullptr;
    Vec3 you;
    float youHeading = 0;
    Vec3 pal;
    float palHeading = 0;
    int partner = -1;                     // index in app.game.dragons (-1: out alone)
    const Dragon* shown = nullptr;        // the partner as drawn (null: none)
    bool riding = false;                  // you were on its back (the feature may refuse)
    // Set by the feature for this frame (the scene clears them before each update):
    ClipId palClip = ClipId::Count;       // Count: the scene animates it (idle, walking)
    float palClipRate = 1.0f;
    const char* youClip = nullptr;        // a people clip ("idle", "wave", "cheer" ...; null: the scene's)
    bool camSet = false;                  // the feature's camera instead of the walking one
    Vec3 eye, target;
};

struct Feature {
    const char* name;
    // The people it stands about now, within `radius` of `near` (fill at most `cap`; return how many).
    int (*folk)(const App& app, const Valley& v, Vec3 near, float radius, Folk* out, int cap);
    // A pressed beside one of them.
    void (*act)(App& app, const Folk& who, Stage& stage);
    // While true, the feature has the valley: the scene stops walking and asks the rest below.
    bool (*active)(const App& app);
    void (*update)(App& app, const Input& in, Stage& stage);
    // Its dragons (view.others), people (after the scene's) and camera, into the view.
    void (*view)(App& app, const Stage& stage, r3d::ValleyView& view);
    void (*drawTop)(App& app, const Stage& stage);                      // 2D over the picture
    void (*drawBottom)(App& app, const Input& in, const Stage& stage);  // the whole bottom screen
};

int featureCount();
const Feature& feature(int i);
// The feature that has the valley now (-1: none).
int activeFeature(const App& app);

}  // namespace ec::vext
