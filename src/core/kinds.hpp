// Dragons, version 2 (D77-D78): the kinds of dragon, their body plans, the eight elements,
// manners and traits. The tables are generated from tools/dragons (tools/dragons/gen_tables.py
// -> kinds_data.inc); each kind's models are in romfs:/dragons/<name>/ and its plan's clips in
// romfs:/anims/<plan>.eca. Until the revamp replaces the old dragons (DR3) the game shows a
// kind only through the dev menu.
#pragma once

#include "core/dragon_mesh.hpp"
#include "core/model.hpp"
#include "core/types.hpp"

namespace ec {

enum class Rarity : u8 { Common, Uncommon, Rare };

constexpr int kKindVariants = 4;
constexpr int kMaxKinds = 16;  // room in the game's tables (36 in the end: grows with them)
constexpr int kMaxPlans = 12;
constexpr int kKindStats = 5;  // wing, wit, might, breath, stamina (tools/dragons/lore.py STATS)

struct KindVariant {
    const char* name;
    Rgb pal[kPalCount];  // model.hpp Palette slots
    s8 patternChannel;   // skin texture channel shown in the pattern colour (0 R, 1 G, 2 B; -1 none)
    s8 glowChannel;      // channel that glows in the glow colour (the rare variant's marks; -1 none)
    Rgb egg[2];          // the egg's shell and markings
};

struct PlanInfo {
    const char* name;         // romfs:/anims/<name>.eca
    const char* contacts[4];  // front left, front right, back left, back right: the bones on the ground
    const char* seatBone;     // where a rider sits
    Vec3 seat;
};

struct KindInfo {
    const char* name;   // romfs:/dragons/<name>/
    const char* title;
    u8 dex;
    u8 elements[2];
    u8 elementCount;
    s8 parents[2];      // crossbreeds: the two base kinds (indices into the kinds); -1 none
    Rarity rarity;
    u8 plan;
    float size;         // grown size relative to the Pouncer
    u8 stats[kKindStats];
    u8 manners[4];
    u8 mannerCount;
    u8 traits[6];
    u8 traitCount;
    const char* blurb;
    KindVariant variants[kKindVariants];
    u8 rareVariant;
    bool rareReplaces;  // a rare part group replaces the common one (else it adds to it)
};

int kindCount();
const KindInfo& kindInfo(int kind);
int findKind(const char* name);  // -1 if none
int planCount();
const PlanInfo& planInfo(int plan);
int elementCount();
const char* elementName(int element);
Rgb elementGlow(int element);
int mannerCount();
const char* mannerName(int manner);
int traitCount();
const char* traitName(int trait);
int traitTier(int trait);  // 0 common .. 3 legendary

// A kind's colours for one variant, shifted a little per dragon (D78: no two quite alike):
// seed 0 gives the variant exactly; any other seed nudges the hue and brightness of the body
// colours by a few steps, the same every time for the same seed.
void kindPalette(int kind, int variant, u32 seed, Rgb out[kPalCount]);

// The part meshes a kind shows: eyes (round pupils, or slit ones when startled or cross),
// heart, mouth and every group's common part; the rare variant's parts replace or add to
// their group's (KindInfo::rareReplaces). Returns how many were written (at most 16).
int selectKindParts(const ModelData& m, bool rare, bool rareReplaces, bool slitEyes, const MeshData* out[16]);
// The kind's wings: the rare variant's own if it has them.
const MeshData* kindWings(const ModelData& m, bool rare);
// Those parts blended at growth t for a build and merged into one draw.
bool buildKindParts(const ModelData& m, bool rare, bool rareReplaces, bool slitEyes, float t, int build,
                    PartsMesh& out);

}  // namespace ec
