// Dragons, version 2 (D77-D78): the kinds of dragon, their body plans, the eight elements,
// manners and traits. The tables are generated from tools/dragons (tools/dragons/gen_tables.py
// -> kinds_data.inc); each kind's models are in romfs:/dragons/<name>/ and its plan's clips in
// romfs:/anims/<plan>.eca. Since DR3 (D80) every dragon is a kind: the old breeds and looks
// are archived (the genome stays for what it still does: sex, the size gene, favourite food).
#pragma once

#include "core/dragon.hpp"
#include "core/dragon_mesh.hpp"
#include "core/model.hpp"
#include "core/rng.hpp"
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
    Vec3 seat;                // in the plan's units: times the kind's formScale
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
    float formScale[2]; // each form's export scale (hatchling, grown): its model is baked at it, so the
                        // plan's root motion and seat (in the plan's units) are scaled by it too
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
// How a manner behaves in the den: the first six are the den's own temperaments; the rest
// act like the nearest (Gentle: Sleepy, Mischievous: Playful, Greedy: Curious, Stubborn: Proud).
Personality personalityOf(int manner);
int traitCount();
const char* traitName(int trait);
int traitTier(int trait);  // 0 common .. 3 legendary

// ---- DR3: every dragon a kind
// A new dragon or egg of `kind` in `variant` (0-2 common, 3 rare): its stats (the kind's, a point
// either way, a point more on the rare colouring, then its manner's nudge), a manner the kind
// leans to (now and then any), one or two traits it leans to (three on the rare colouring,
// which alone reaches its rarest; a rare kind reaches one tier higher).
void rollKind(Dragon& d, int kind, int variant, Rng& rng);
// The colouring an egg is laid in: the rare one about 1 in 20 (1 in 10 with a rare parent).
int rollVariant(Rng& rng, bool rareParent = false);
// The crossbreed of two kinds (-1 if there isn't one yet).
int crossbreedOf(int a, int b);
// The kind a pair's egg is: the parents' own if they match; else their crossbreed about a third
// of the time if there is one, otherwise one parent's kind.
int childKind(int a, int b, Rng& rng);
// A kind for an egg found or bought, by rarity (weights {common, uncommon, rare}); crossbreeds
// only come from breeding.
int randomKind(Rng& rng, int common = 6, int uncommon = 3, int rare = 1);
// A dragon from before the revamp (or anything without a kind): a random kind in a common
// colouring, its stats, manner and traits rolled, all fixed by its id (the same every load).
void migrateToKind(Dragon& d);
// What the game calls it: "Pouncer"; hatched, with its colouring: "Tabby Pouncer".
const char* kindTitle(const Dragon& d);
// Its heartglow's colour (its kind's first element) and its egg's shell (its colouring's).
Rgb kindGlow(const Dragon& d);
Rgb kindShell(const Dragon& d);
// Its grown size next to a Pouncer (0.67..1.5): the kind's, a little either way (the size gene).
float kindSize(const Dragon& d);
// Its run is a roll (the Curlstone's plan: tucked into a ball, D130): only with you riding; else it
// walks at every pace, following you or in the den.
bool rollsToRun(const Dragon& d);
// The kind's elements (1 or 2) as text: "Ember" or "Ember / Gale".
void kindElements(int kind, char* out, int cap);
const char* rarityName(Rarity r);  // "Common", "Harder to find", "Rare"

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
