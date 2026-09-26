#include "core/breeding.hpp"

#include "core/clock.hpp"
#include "core/den_roster.hpp"
#include "core/kinds.hpp"

namespace ec {
namespace {

BreedBlock blockFor(const Dragon& d, s64 now) {
    if (d.stage != Stage::Adult) return BreedBlock::NotAdult;
    if (d.location != Location::Den) return BreedBlock::NotInDen;
    if (d.bond < kBreedingBond) return BreedBlock::LowBond;
    if (moodOf(d) < Mood::Content) return BreedBlock::Unhappy;
    if (d.lastBredAt != 0 && now - d.lastBredAt < kBreedingRest) return BreedBlock::Resting;
    return BreedBlock::None;
}

}  // namespace

BreedBlock breedingBlock(const Dragon& a, const Dragon& b, s64 now) {
    if (a.id == b.id) return BreedBlock::SameDragon;
    const BreedBlock ba = blockFor(a, now);
    if (ba != BreedBlock::None) return ba;
    const BreedBlock bb = blockFor(b, now);
    if (bb != BreedBlock::None) return bb;
    if (a.sex == b.sex) return BreedBlock::SameSex;
    return BreedBlock::None;
}

const char* breedBlockHint(BreedBlock b) {
    switch (b) {
        case BreedBlock::None: return "Ready to nest together.";
        case BreedBlock::NotAdult: return "Both dragons must be adults.";
        case BreedBlock::SameSex: return "A pair needs one male and one female.";
        case BreedBlock::SameDragon: return "Choose two different dragons.";
        case BreedBlock::LowBond: return "They need to trust you more first.";
        case BreedBlock::Unhappy: return "They're not in the mood. Cheer them up first.";
        case BreedBlock::Resting: return "They're resting after their last clutch.";
        case BreedBlock::NotInDen: return "Bring both dragons home to the den.";
    }
    return "";
}

bool settleToNest(SaveData& s, int a, int b, s64 now) {
    if (a < 0 || b < 0 || a >= s.dragonCount || b >= s.dragonCount) return false;
    if (breedingBlock(s.dragons[a], s.dragons[b], now) != BreedBlock::None) return false;
    s.nestA = s.dragons[a].id;
    s.nestB = s.dragons[b].id;
    s.nestDay = dayIndex(now);
    return true;
}

int layDueEgg(SaveData& s, s64 now, Rng& rng) {
    if (s.nestA == 0 || s.nestB == 0 || dayIndex(now) <= s.nestDay) return -1;
    int a = -1, b = -1;
    for (int i = 0; i < s.dragonCount; ++i) {
        if (s.dragons[i].id == s.nestA) a = i;
        if (s.dragons[i].id == s.nestB) b = i;
    }
    s.nestA = s.nestB = 0;
    if (a < 0 || b < 0 || s.dragonCount >= static_cast<int>(kMaxDragons)) return -1;
    Dragon egg = layEgg(s.nextId++, s.dragons[a], s.dragons[b], now, rng);
    placeEgg(s, egg);
    s.dragons[s.dragonCount] = egg;
    return s.dragonCount++;
}

Dragon layEgg(u32 id, Dragon& a, Dragon& b, s64 now, Rng& rng) {
    Dragon& mother = a.sex == Sex::Female ? a : b;
    Dragon& father = a.sex == Sex::Female ? b : a;
    Dragon egg = makeEgg(id, breed(mother.genome, father.genome, rng), rollSex(rng), now,
                         inheritLook(mother.look, father.look, rng));
    egg.origin = Origin::Bred;
    egg.motherId = mother.id;
    egg.fatherId = father.id;
    // DR3: its kind from theirs (their crossbreed now and then), the rare colouring likelier
    // if a parent has it.
    const bool rareParent = mother.variant == kindInfo(mother.kind).rareVariant ||
                            father.variant == kindInfo(father.kind).rareVariant;
    rollKind(egg, childKind(mother.kind, father.kind, rng), rollVariant(rng, rareParent), rng);
    mother.lastBredAt = father.lastBredAt = now;
    return egg;
}

}  // namespace ec
