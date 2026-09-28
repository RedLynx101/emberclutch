// The valley's critters on the 3DS (1.0, workstream L): core/critters' life round you each frame,
// what they sound like, the A moment (its prompt, what A starts, and your partner's part in it as
// a valley feature: app/valley_ext), the toasts and rewards when one's befriended, the triangles
// for the renderer (app/render_critters.inc), and the Journal's "Valley critters" page.
#pragma once

#include "app/app.hpp"
#include "app/render3d.hpp"
#include "app/valley_ext.hpp"

namespace ec {
struct Valley;
}  // namespace ec

namespace ec::wildlife {

// What the valley scene knows each frame: you, your partner (riding: both where the flight is).
struct Here {
    Vec3 you;
    float youHeading = 0, youSpeed = 0;
    bool riding = false;
    int partner = -1;  // its index in app.game.dragons (-1: out alone)
    Vec3 pal;
    float palSpeed = 0, palHeading = 0;
};

void tick(App& app, const Valley& v, const Here& here);  // once a frame (scene_valley's update)
bool offer(Vec3 you, Vec3 forward, bool hasPal);        // a critter's A moment in reach (findAction, last)
const char* prompt();                                   // what A does then ("A: whistle to the birds")
void act(App& app);                                     // A pressed
void fillView(App& app, r3d::ValleyView& view);         // the critters in view, for drawValley
void drawJournal(App& app, const Input& in);            // the Journal's page (app/care_pages, tab 4)
// Scripted runs (autotest `critters <what>`): spawn <kind 0-6>, act [kind], clear, journal,
// friends (every kind spotted and befriended, for the Journal's shot).
void command(App& app, const char* text);

// The partner's part in a moment (the chase, the butterfly's landing, the fox's visit): its line
// in valley_ext.cpp's kFeatures.
extern const vext::Feature kFeature;

}  // namespace ec::wildlife
