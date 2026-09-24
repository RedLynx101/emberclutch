// Breeding rules — see docs/design/breeds-and-genetics.md §4.
#pragma once

#include "core/dragon.hpp"
#include "core/save.hpp"

namespace ec {

constexpr u16 kBreedingBond = 300;
constexpr s64 kBreedingRest = 3 * 24 * 3600;

// Why a pair can't breed right now (None = they can). Shown to the player as a hint.
enum class BreedBlock : u8 {
    None,
    NotAdult,
    SameSex,
    SameDragon,
    LowBond,
    Unhappy,  // mood below Content, or Upset
    Resting,  // bred within the last 3 days
    NotInDen,
};

BreedBlock breedingBlock(const Dragon& a, const Dragon& b, s64 now);
const char* breedBlockHint(BreedBlock b);

// Lays the pair's egg (call only when breedingBlock() == None). Order of a and b doesn't
// matter. Both parents start their rest period.
Dragon layEgg(u32 id, Dragon& a, Dragon& b, s64 now, Rng& rng);

// The Nesting Stone (Alpha 2 WP3): a ready pair settles there today; the egg comes the next
// calendar day. False (and nothing changes) if they can't breed.
bool settleToNest(SaveData& s, int a, int b, s64 now);
// If the nesting pair's day has passed, lays their egg (into a free nest, else the Cold
// Vault) and returns its SaveData index; -1 if no egg is due. A pair that has gone (moved
// out, or there's no room left in the save) just stops nesting.
int layDueEgg(SaveData& s, s64 now, Rng& rng);

}  // namespace ec
