#include "core/profile.hpp"

#include "core/care.hpp"
#include "core/genetics.hpp"

namespace ec {
namespace {

template <std::size_t N>
const char* pick(const char* const (&names)[N], u8 i) {
    return i < N ? names[i] : names[0];
}

// Each element's aptitude (breeds-and-genetics section 1): 0 Wing, 1 Wit, 2 Spark.
constexpr u8 kAptitude[kElementCount] = {2, 1, 0, 1, 0, 2};  // Ember, Tide, Gale, Grove, Frost, Lumen

}  // namespace

const char* buildName(u8 v) { return pick({"Sturdy", "Sleek", "Long"}, v); }
const char* hornsName(u8 v) { return pick({"Nubs", "Swept", "Crown", "Crystal", "Antler"}, v); }
const char* frillName(u8 v) { return pick({"None", "Fin", "Leaf", "Feather"}, v); }
const char* wingsName(u8 v) { return pick({"Classic", "Plumed", "Sail"}, v); }
const char* tailName(u8 v) { return pick({"Plain", "Spade", "Tuft", "Fan"}, v); }
const char* patternName(u8 v) { return pick({"Solid", "Stripes", "Spots", "Dapple", "Runes"}, v); }

const char* rareName(u8 flags) {
    if (flags & kRareIridescent) return "Iridescent";
    if (flags & kRareMelanistic) return "Melanistic";
    if (flags & kRareLeucistic) return "Leucistic";
    if (flags & kRareStarspeckle) return "Starspeckled";
    return nullptr;
}

Stats statsOf(const Dragon& d) {
    const int base = 8 + 6 * static_cast<int>(d.stage);
    int stat[3] = {base, base, base};
    for (u8 e : {d.genome.elementA, d.genome.elementB})
        if (e < kElementCount) stat[kAptitude[e]] += 12;
    return {stat[0], stat[1], stat[2]};
}

const char* sweetSpotText(const Dragon& d) {
    const SweetSpot s = sweetSpotOf(d);
    switch (s.zone) {
        case PetZone::Head: return s.side < 0 ? "behind its left ear" : s.side > 0 ? "behind its right ear" : "the top of its head";
        case PetZone::Chin: return "under its chin";
        case PetZone::Back: return "along its back";
        case PetZone::Belly: return "its round belly";
        case PetZone::Cheek: return s.side < 0 ? "its left cheek" : "its right cheek";
        case PetZone::Neck: return "the back of its neck";
        case PetZone::Tail: return "the base of its tail";
        case PetZone::Paw: return "its front paws";
        case PetZone::Wing: return s.side < 0 ? "under its left wing" : "under its right wing";
        default: return "its heartglow";
    }
}

int indexOfId(const SaveData& s, u32 id) {
    if (id == 0) return -1;
    for (int i = 0; i < s.dragonCount; ++i)
        if (s.dragons[i].id == id) return i;
    return -1;
}

Family familyOf(const SaveData& s, const Dragon& d) {
    Family f;
    f.mother = indexOfId(s, d.motherId);
    f.father = indexOfId(s, d.fatherId);
    if (f.mother >= 0) {
        f.grand[0] = indexOfId(s, s.dragons[f.mother].motherId);
        f.grand[1] = indexOfId(s, s.dragons[f.mother].fatherId);
    }
    if (f.father >= 0) {
        f.grand[2] = indexOfId(s, s.dragons[f.father].motherId);
        f.grand[3] = indexOfId(s, s.dragons[f.father].fatherId);
    }
    for (int i = 0; i < s.dragonCount; ++i)
        if (d.id != 0 && (s.dragons[i].motherId == d.id || s.dragons[i].fatherId == d.id)) ++f.young;
    return f;
}

const char* originText(const Dragon& d) {
    switch (d.origin) {
        case Origin::Bred: return "Laid at the Nesting Stone";
        case Origin::Wild: return "A wild egg from the Wanderings";
        case Origin::Market: return "An egg from the Market";
        default: return "Your very first egg";
    }
}

}  // namespace ec
