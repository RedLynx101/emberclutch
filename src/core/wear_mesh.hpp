// The accessories' meshes (1.0, D90), built in code from a few primitives like the den's things
// (core/prop_mesh), each in its slot's own frame (core/wear_fit puts that frame on the dragon):
//   head: the top of the skull at the origin, +Z up, -Y toward the snout; a unit is the head's
//         half width.
//   neck: the neck's middle at the origin, +Z up the neck toward the head, -Y the throat; a unit
//         is the neck's radius (the skin at 1).
//   back: the top of the back at the origin, +Z up, -Y toward the head; a unit is the body's half
//         width; the body's curve is a circle of radius 1 under the origin (centred at z -1).
//   tail: the tail's middle at the origin, +Y toward the tip, +Z up; a unit is the tail's radius.
// Palette slots: 0 the accessory's main colour, 1 its second, 2 its trim, 3 its gem (emissive
// where it glows). Pure geometry: tests/test_pageant.cpp checks the triangle budget.
#pragma once

#include "core/accessories.hpp"
#include "core/prop_mesh.hpp"

namespace ec {

constexpr int kWearMaxTriangles = 160;  // one accessory's budget (four at most on a dragon)

PropMesh wearMesh(WearShape shape);

}  // namespace ec
