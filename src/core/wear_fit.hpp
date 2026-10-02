// Where the accessories sit on a dragon (1.0, D90): for each wear slot, the bones it rides on and
// the frame (core/wear_mesh's slot frames) in the model's rest armature space, found by measuring
// the body itself: the top of the skull between the head's joint and the eyes, the neck's girth a
// little up from the chest, the top of the back between the chest and the hips, the tail's girth
// a third of the way along. So every kind's plan and both forms fit without a table of numbers;
// a small table of nudges per plan (kWearNudges in wear_fit.cpp) trims what the measuring misses.
// The renderer carries the frame with the bones' skinning matrices (skin * frame), as the body's
// own vertices are carried. Pure logic: tests/test_pageant.cpp fits every kind in both forms.
#pragma once

#include "core/accessories.hpp"
#include "core/math3d.hpp"
#include "core/model.hpp"

namespace ec {

struct WearFit {
    bool ok[kWearSlots] = {};
    s8 boneA[kWearSlots] = {-1, -1, -1, -1};  // blended half and half (the same bone twice: rigid)
    s8 boneB[kWearSlots] = {-1, -1, -1, -1};
    Mat34 frame[kWearSlots];  // the slot's frame (its axes scaled to its unit) in rest armature space
    Mat34 headNarrow;  // the head slot's frame for the narrow things (a crown, a party hat: no wide brim; run 23)
    Mat34 headWide;    // ...and for the straw sunhat's wide brim (run 24)
    // (how far the hat moved back and up off the skull to clear the eyes, in its units, and its size)
    float hatBack = 0, hatLift = 0, hatScale = 1, narrowLift = 0, wideLift = 0;
    bool onPad = false;  // (a pad on its head, the Lilyfin's: the hats sit on it, run 24)
};

// Measures a model (a kind's form); `plan` picks its nudges (core/kinds plan index, -1 none) and
// `grown` the form's.
bool fitWear(const ModelData& m, int plan, bool grown, WearFit& out);

// Hats clear the eyes (D126): no eye comes up through a brim this wide (in the hat's units, the
// top hat's 0.72 and the crowns' with a little over), its underside this high.
constexpr float kHatBrim = 0.8f, kHatBrimZ = -0.08f;
// The narrow head things (a crown, a tiara, a circlet, a party hat, a feather crest: run 23) clear
// the eyes over this much (their bands' 0.44-0.62), so they sit down on the skull where a brim can't.
constexpr float kHatNarrow = 0.62f;
inline bool narrowOnHead(WearShape s) {
    return s == WearShape::Crown || s == WearShape::Tiara || s == WearShape::Circlet || s == WearShape::PartyHat ||
           s == WearShape::FeatherCrest;
}
// The eyes' vertices (skinned to the "eyes" bone) in a head frame's units: how high the highest one
// within `radius` of the frame's middle stands above the brim's underside, plus `margin` (0 or less:
// clear).
float eyesThroughBrim(const ModelData& m, const Mat34& frame, float radius, float margin);

}  // namespace ec
