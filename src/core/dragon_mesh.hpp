// Per-dragon mesh assembly: which parts and wings a genome shows, the merged part mesh at
// a growth t, the colour palette, and the ground offset. Pure C++ so it is PC-tested; the
// 3DS renderer (src/app/render3d.cpp) uploads the results.
#pragma once

#include <vector>

#include "core/genetics.hpp"
#include "core/model.hpp"

namespace ec {

// All of a dragon's rigid parts merged into one draw: one bone palette (<= 24 bones),
// palette-local skin indices, vertex paint and triangle indices.
struct PartsMesh {
    u8 paletteCount = 0;
    u8 palette[kMaxPalette] = {};
    std::vector<Vec3> pos, nrm;
    std::vector<u8> skin, paint;  // 4 bytes per vertex each (as in MeshData)
    std::vector<u16> indices;
    void clear();
};

inline u8 partSex(Sex s) { return s == Sex::Male ? kSexMale : kSexFemale; }

// The part meshes a genome shows, with fallbacks for variants not modelled yet (Alpha 2
// adds the parts library): horns -> swept, dorsal ridge -> spikes. Frill None and tail
// Plain draw nothing. Returns the number of meshes written to out (at most 8).
int selectParts(const ModelData& m, const Genome& g, Sex sex, const MeshData* out[8]);

// The genome's wing mesh (falls back to Classic).
const MeshData* selectWings(const ModelData& m, const Genome& g);

// Blends the selected parts at growth t and merges them. Returns false if the merged
// palette would exceed kMaxPalette bones (never for the shipped models).
bool buildParts(const ModelData& m, const Genome& g, Sex sex, float t, PartsMesh& out);

// Height of the lowest body vertex for the given skinning matrices; the renderer lifts the
// dragon by -groundOffset so its feet touch the floor.
float groundOffset(const ModelData& m, const Mat34* skin);

// Per-dragon colours for the model's palette slots (kPal*). Base follows element A, accent
// element B (breeds-and-genetics.md), iris and heartglow come from the elements.
void dragonPalette(const Genome& g, Rgb out[kPalCount]);

}  // namespace ec
