// Moonpetal Glade (1.0, D90): the pageant's home, a valley feature (app/valley_ext). Celestine the
// host stands by the board (A: the pageant's leagues and today's four shows a league; enter one,
// or dress your dragon first in the wardrobe), three judges at their table, and two stalls:
// Linnet's accessories and Madder's dyes (tap to see a thing on your dragon, then buy it). A show
// takes the valley over on the stage: rivals picked for the league stand with your dragon, the
// judges score Look and Poise, you cue tricks in time for the Performance (app/glade_show.cpp),
// then the placings, ribbons and prizes.
#pragma once

#include "app/valley_ext.hpp"

namespace ec {

namespace glade {
extern const vext::Feature kFeature;

// Where things stand in the glade's frame (metres from its anchor, +Y the way it faces), the
// names workstream A's anchors will have (tools/valley/places.json). Until those are in, spots
// picked on the glade's flat ground: swap gladeLayout() for placeAnchor(kPlaceGlade, ...) then.
struct Layout {
    Vec3 stage{0, -5.0f, 0};                 // the stage's middle; z its floor above the ground
    // On the stage: a show uses the first three (you and two rivals, core/pageant kEntrants).
    Vec2 rivals[4] = {{-4.8f, -5.4f}, {0.0f, -5.8f}, {4.8f, -5.4f}, {0.0f, -8.8f}};
    Vec2 judges{-7.6f, 1.6f};                // their table (they stand along it, facing the stage)
    // Accessories, dyes: x, y, and the way the keeper faces (radians in the glade's frame, as a
    // dragon's heading: 0 toward -Y; the world's heading is the glade's + this + pi).
    Vec3 stalls[2] = {{8.6f, 6.2f, -1.12f}, {11.2f, 0.6f, -1.70f}};
    Vec2 board{-6.2f, 7.0f};
};
const Layout& gladeLayout();
}  // namespace glade

// Scripted runs (autotest `pg ...`): `give` (everything owned), `wear a b c d` (the dragon cared
// for, -1 leaves a slot), `dye n`, `kind k v`, `grown` / `hatchling`, `clean`, `wardrobe`, `spin a`,
// `board`, `stall 0|1`, `show league slot` (at the glade: straight into a show), `autoplay on|off`,
// `bond n`.
void pageantCommand(App& app, const char* text);

}  // namespace ec
