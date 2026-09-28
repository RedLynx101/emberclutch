// A battle in the valley (1.0, D90), shared by the league (feature_league.cpp) and Frostspire
// Hollow (feature_hollow.cpp): right where you stand, you and your partner on one side, the foe
// (and a challenger behind it) across from you, the camera framing both. The bottom screen holds
// the four moves (their element's colour, power, how well they'd do) and giving up; the top the
// health bars, names and levels, and a line of what happened. Each turn's events (core/battle)
// play one by one: a lunge for a body move, a breath stream, hit flashes and shakes, sounds.
// At the end a card with what it earned (the feature records it: experience, Gleam, titles).
#pragma once

#include "app/app.hpp"
#include "app/render3d.hpp"
#include "app/valley_ext.hpp"
#include "core/battle.hpp"
#include "core/walker.hpp"

namespace ec::bview {

// The card after a battle: a title and a few lines (the feature fills them in).
struct Results {
    char title[40] = {};
    char lines[7][64] = {};
    int count = 0;
    bool levelUp = false;
    void add(const char* fmt, ...);
};

struct Setup {
    int partner = -1;            // your dragon (app.game.dragons)
    Dragon foe;                  // theirs, as drawn and battled
    char foeName[24] = {};       // as the battle's lines call it ("Pepper", "the wild Flurrytail")
    int skill = 0;               // the foe's choosing (battle::chooseMove)
    char intro[48] = {};         // the banner as it begins ("Tamsin wants to battle!")
    // Where everyone stands (in the valley, on the ground); the foe comes in from foeFrom.
    Vec3 youAt;
    Vec3 palAt, foeAt, foeFrom;
    float foeScale = 1.0f;       // a guardian a little bigger
    float camSide = 1.0f;        // the camera over your right shoulder (1) or your left (-1)
    float camReach = 1.0f;       // how far back and aside it stands (less in a ringed bowl: the Hollow)
    bool trainer = false;        // a challenger behind it
    r3d::PersonView trainerLook; // (its anim is the battle's own)
    // The battle over: the feature records it and fills in the card; then, the card closed.
    void (*finish)(App& app, battle::Outcome o, Results& out) = nullptr;
    void (*done)(App& app, battle::Outcome o) = nullptr;
};

void start(App& app, const Setup& s);
bool running();
void stop();
// While running: the stage (you, your partner, the camera), its dragons and people in the view,
// the top screen's bars and lines over the picture, and the bottom screen.
void update(App& app, const Input& in, vext::Stage& stage);
void view(App& app, const vext::Stage& stage, r3d::ValleyView& view);
void drawTop(App& app);
void drawBottom(App& app, const Input& in);
// The camera as the last battle left it (the Hollow holds it between floors).
void lastCamera(Vec3& eye, Vec3& target);
// Scripted runs: it plays itself (your moves chosen for you, the lines and the card moving on).
void setAutoplay(bool on);
bool autoplay();
void setBreathOnly(bool on);  // (scripted runs: both sides breathe every turn, to look at breath)

// Staging: a dragon's size (metres-ish; smaller while young), and whether a spot is good ground
// to stand on (not under the water, not up a slope from `from`).
float dragonSize(const Dragon& d);
// How far a dragon of this size reaches from where it stands (its nose ahead of it): a lunge stops
// with two dragons' reaches touching; they stand a clear gap more apart than that.
constexpr float kReachPerSize = 1.6f;
constexpr float kStandingGap = 2.0f;
inline float standApart(float sizeA, float sizeB) { return kReachPerSize * (sizeA + sizeB) + kStandingGap; }
bool goodGround(const Valley& v, Vec3 from, Vec2 at);
// The camera's place for a battle staged so (you, your dragon, theirs; the larger's size), for
// the features to check it's clear before they settle on a side.
void cameraFor(const Setup& setup, Vec3 you, Vec3 pal, Vec3 foe, float size, Vec3& eye, Vec3& target);
// A clear view: the eye well away from the places' solids (a spire, a wall, a tent), the nearer
// part of its line to the target past them, and the whole line above the ground (a bowl's rim).
bool viewClear(const Valley& v, const std::vector<Solid>& solids, Vec3 eye, Vec3 target);

}  // namespace ec::bview
