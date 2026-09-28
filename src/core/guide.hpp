// The Journal's tracked goal (1.0, D89: "the Journal tracks a quest and the map shows where to
// go"): what can be tracked (a begun quest, the battle league's board, the pageant's, Frostspire
// Hollow, a place), the goal tracked now, and where in the valley it points: a spot, or a soft
// search area where the way isn't known yet (a place not found, the stray in the meadow's
// flowers, the cold heights' edges). The pick is SaveData::progress (core/trainer track).
// Pure logic (PC-tested).
#pragma once

#include "core/math3d.hpp"
#include "core/trainer.hpp"

namespace ec {

struct SaveData;
struct Valley;

namespace guide {

struct Goal {
    Tracked kind = Tracked::None;
    int id = 0;  // the quest (core/campaign) or the place (core/valley ValleyPlace); else 0
    bool operator==(const Goal& o) const { return kind == o.kind && id == o.id; }
};

// Whether a goal still means something: a begun quest not done; a league's board while there's a
// league left to win, the Hollow, once their places are found; any place.
bool open(const SaveData& s, const Goal& g);
// The goal tracked now: the one picked while it's still open, else the quest in hand
// (campaign::currentQuest), else nothing (kind None).
Goal current(const SaveData& s);
// Picked in the Journal (rather than just being the quest in hand).
bool picked(const SaveData& s, const Goal& g);
// Picks it; picked already, lets it go (back to the quest in hand).
void toggle(SaveData& s, const Goal& g);

// What the Journal's quests tab lists to track, in order: the begun quests not done, then the
// battle league's board, the pageant's and the Hollow (each once its place is found). At most
// `cap`; returns how many.
int trackables(const SaveData& s, Goal* out, int cap);

struct Target {
    bool valid = false;
    bool area = false;  // a search area (a soft circle) rather than a spot
    Vec2 at;            // valley metres
    float radius = 0;   // the area's, in metres
    int place = -1;     // the place it's at or near (-1: none)
};
// Where a goal points in the valley (`from`: where you are, for the nearest of several, as the
// festival's unlit lanterns). Not valid when there's nowhere to go (grow up together...).
Target target(const SaveData& s, const Valley& v, const Goal& g, Vec2 from);

}  // namespace guide
}  // namespace ec
