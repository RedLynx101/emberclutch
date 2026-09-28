// A pageant show on Moonpetal Glade's stage (1.0, D90; app/glade_show.cpp), run by the glade's
// feature (app/glade.cpp) while it has the valley: the host's welcome, Look and Poise judged one
// entrant at a time, your Performance (the rhythm game), the rivals', and the results.
#pragma once

#include "app/glade.hpp"

namespace ec::glade {

// Starts a show (Energy already spent) with the partner the stage lends.
void beginShow(App& app, vext::Stage& stage, int league, int slot);
// One frame of it; false once it's over (the valley goes back to walking).
bool updateShow(App& app, const Input& in, vext::Stage& stage);
void showView(App& app, const vext::Stage& stage, r3d::ValleyView& view);
void showDrawTop(App& app, const vext::Stage& stage);
void showDrawBottom(App& app, const Input& in, const vext::Stage& stage);
// Where the host stands while a show is on (world), for the glade's people; false: no show.
bool showHostSpot(const Valley& v, Vec3& at, float& heading);
bool showOn();  // a show is running
// The judges are at their table while the camera may see them (the welcome, their own shot, the
// results): three people less in the close views keeps them in budget.
bool showJudgesSeen();
// Scripted runs: the Performance plays itself.
void setShowAutoplay(bool on);

// Shared with the show: the glade's place in the valley and a point in its frame.
const ValleyPlaceInfo* gladePlace(const Valley& v);
Vec3 gladePoint(const Valley& v, Vec2 local, float lift = 0);
float headingTo(Vec3 from, Vec3 to);

}  // namespace ec::glade
