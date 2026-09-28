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
};

// Measures a model (a kind's form); `plan` picks its nudges (core/kinds plan index, -1 none) and
// `grown` the form's.
bool fitWear(const ModelData& m, int plan, bool grown, WearFit& out);

}  // namespace ec
