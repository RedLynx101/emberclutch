// Where the battles stand at Emberpeak Caldera and Frostspire Hollow (workstream B), in one table:
// stand-ins for the places' named anchors (core/place_layout placeAnchor, places.json "anchors",
// workstream A) with the anchors' own values, until the lead swaps these for placeAnchor calls.
// In each place's frame (metres from its anchor; +Y its front).
#pragma once

#include "core/math3d.hpp"

namespace ec::spots {

// The caldera: its battle ring (its top a low step up) and the two sides where the trainers
// stand (you, then the champion); the league's board by the way in.
constexpr Vec2 kCalderaRing{0.0f, 0.0f};                              // "ring"
constexpr float kCalderaRingTop = 0.25f;                              // (the ring's top, above the anchor)
constexpr float kCalderaRingRadius = 7.0f;
constexpr Vec2 kCalderaSides[2] = {{-8.9f, 0.0f}, {8.9f, 0.0f}};      // "sides"
constexpr Vec2 kCalderaBoard{-7.2f, 20.8f};                           // "board"
// The Hollow: the battle ground on the bowl's floor, the cave door the wild ones come out of,
// and the keeper's camp near the mouth.
constexpr Vec2 kHollowArena{0.0f, -1.0f};                             // "arena"
constexpr Vec2 kHollowWildDoor{0.0f, -8.2f};                          // "wild_door"
constexpr Vec2 kHollowKeeper{-4.6f, 6.4f};                            // "keeper"

}  // namespace ec::spots
