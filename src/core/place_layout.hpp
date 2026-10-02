// Where things are at each place in the valley (Beta WP4): its door (walk up and press A to go
// in), its festival lantern, the walls to walk round, and the Market's egg stand and goods
// spots. In the place's own frame (metres from its anchor on the ground; +Y the way it faces,
// +X its right) and turned into the valley by the place's heading. The numbers come from the
// places' models (tools/valley/places.json, D85 builders: src/core/places_data.inc).
#pragma once

#include <vector>

#include "core/math3d.hpp"
#include "core/valley.hpp"
#include "core/walker.hpp"

namespace ec {

struct PlaceLayout {
    float flat = 0;        // how far round the anchor the ground is flat (and its props kept clear)
    bool hasDoor = false;
    Vec2 door;             // where you press A to go in
    bool hasLantern = false;
    Vec3 lantern;          // the lantern's flame (z up from the ground)
    std::vector<Solid> solids;  // circles, in the place's frame
    Vec3 eggStand;         // the Market's egg of the day (x, y, z up from the ground)
    Vec3 goods[4];         // the Market's four goods spots
    Vec3 hub;              // the windmill's sails turn round it,
    Vec3 hubAxis{0, 1, 0}; // about this
    float waterZ = 0;      // the lake's and the mill's water, from the anchor
    Vec2 arrive;           // where a trip on the map sets you down, looking at it
};

const PlaceLayout& placeLayout(int place);

// A place's named spots (1.0, places.json "anchors"): the caldera's "ring", "sides" and "board";
// the glade's "stage", "rivals", "judges", "stalls" and "board"; the cove's "fish_spot", "jetty",
// "fisher" and "shells"; the hollow's "arena", "wild_door" and "keeper" (docs/tech/places.md).
// Each is one or more points in the place's frame (x, y); the third value is a height in the
// frame (above the anchor, as the model is drawn: the ring's and stage's tops, the jetty's deck;
// place them with placeFrameToWorld), or for the glade's "stalls" the way the stall faces
// (radians, 0 the place's front, counter-clockwise: its heading in the valley is the place's
// heading plus it), or 0 for a spot on the ground (place it with placeToWorld3).
struct PlaceAnchor {
    const float (*points)[3] = nullptr;
    int count = 0;
    explicit operator bool() const { return count > 0; }
    Vec3 at(int i = 0) const;  // its i-th point (clamped into range; (0, 0, 0) if it has none)
};
PlaceAnchor placeAnchor(int place, const char* name);  // (none: count 0)

// A point in a place's frame, in the valley (x, y), and a height above the ground there.
Vec2 placeToWorld(const ValleyPlaceInfo& p, Vec2 local);
Vec3 placeToWorld3(const Valley& v, const ValleyPlaceInfo& p, Vec3 local);
// A point in a place's frame, its z a height in the frame (above the place's anchor), in the valley.
Vec3 placeFrameToWorld(const ValleyPlaceInfo& p, Vec3 local);
// Where a trip on the map sets you down: its arrive spot, or out of its door (far enough that the
// camera behind you is outside too), standing on what's there at the place's own height (the
// isles: the island's top; run 28 set you down on the land under it).
Vec3 placeArrival(const Valley& v, const ValleyPlaceInfo& p, bool outward);
// The places' decks into the valley (1.0: the mill's bridge, the cove's jetty), after it loads.
void addPlaceDecks(Valley& v);
// Every place's walls in the valley, for walking round.
std::vector<Solid> worldSolids(const Valley& v);
// The faces the walking camera keeps in front of: the den's arch in its cliff.
std::vector<CameraWall> cameraWalls(const Valley& v);

}  // namespace ec
