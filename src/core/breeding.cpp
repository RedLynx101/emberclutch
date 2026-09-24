#include "core/breeding.hpp"

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

Dragon layEgg(u32 id, Dragon& a, Dragon& b, s64 now, Rng& rng) {
    Dragon& mother = a.sex == Sex::Female ? a : b;
    Dragon& father = a.sex == Sex::Female ? b : a;
    Dragon egg = makeEgg(id, breed(mother.genome, father.genome, rng), rollSex(rng), now);
    egg.motherId = mother.id;
    egg.fatherId = father.id;
    mother.lastBredAt = father.lastBredAt = now;
    return egg;
}

}  // namespace ec
