// The .ecm dragon model (written by tools/blender/export_dragon.py). See architecture §5.
#pragma once

#include <cstddef>
#include <vector>

#include "core/math3d.hpp"
#include "core/skeleton.hpp"

namespace ec {

// Mesh kinds and part groups (must match export_dragon.py).
enum MeshKind : u8 { kMeshBody = 0, kMeshWings = 1, kMeshPart = 2 };
enum PartGroup : u8 {
    kGroupEyes = 0,
    kGroupHorns = 1,
    kGroupFrill = 2,
    kGroupSpikes = 3,
    kGroupTailTip = 4,
    kGroupHeart = 5,
    kGroupWings = 6,
    kGroupBody = 255,
};
enum PartSex : u8 { kSexAny = 0, kSexMale = 1, kSexFemale = 2 };

// Palette slots: colours set per dragon at runtime (vertex paint indexes these).
enum Palette : u8 {
    kPalBase,
    kPalAccent,
    kPalPattern,
    kPalHorn,
    kPalMembrane,
    kPalIris,
    kPalPupil,
    kPalGlint,
    kPalGlow,
    kPalCount,
};

constexpr int kMaxPalette = 24;  // bones per draw call (vertex shader uniform budget)
constexpr int kMaxKeys = 4;
constexpr int kModelBuilds = 3;  // sturdy, sleek, long (genome Build order)

struct MeshData {
    char name[16] = {};
    u8 kind = 0, group = 0, variant = 0, sex = 0;
    u8 paletteCount = 0;
    u8 palette[kMaxPalette] = {};  // skeleton bone index for each palette-local bone
    u8 keyCount = 0;
    float keyT[kMaxKeys] = {};     // growth t of each baked key (parts); 1 key for skinned meshes
    u16 vertexCount = 0;
    std::vector<Vec3> pos;         // keyCount * vertexCount, armature rest space
    std::vector<Vec3> nrm;         // keyCount * vertexCount
    std::vector<u8> skin;          // 4 per vertex: bone0, bone1 (palette-local), w0, w1 (sum 255)
    std::vector<u8> paint;         // 4 per vertex: palette A, palette B, mix, emissive
    std::vector<u16> indices;      // triangle list
};

struct ModelData {
    Skeleton skel;
    Vec3 hatchScale[kMaxBones];             // bone scale at t = 0 (adult = 1)
    float build[kModelBuilds][kMaxBones][2];  // (girth, length) multipliers
    Vec3 poseEulerDeg[kMaxBones];           // idle pose (Euler XYZ degrees)
    float hatchPoseXDeg[kMaxBones];         // extra X rotation at t = 0 (babies hold heads up)
    std::vector<MeshData> meshes;

    // First mesh matching kind/group/variant/sex (kSexAny matches either), or nullptr.
    const MeshData* findMesh(u8 kind, u8 group, u8 variant, u8 sex = kSexAny) const;
};

bool loadModel(const u8* data, std::size_t size, ModelData& out);

}  // namespace ec
