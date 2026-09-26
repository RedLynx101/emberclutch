#include "core/people.hpp"

namespace ec {
namespace {

#include "core/people_looks.inc"

constexpr Rgb kEyes[kEyeColours] = {{86, 56, 40}, {70, 118, 176}, {76, 128, 78}, {118, 110, 124}};
constexpr const char* kEyeNames[kEyeColours] = {"Brown", "Blue", "Green", "Grey"};
constexpr const char* kHairNames[kHairStyles] = {"Tousled", "Bob", "Ponytail", "Buns", "Spiky", "Long"};
constexpr const char* kBodyNames[2] = {"Tunic", "Dress"};

struct PersonRow {
    const char* file;
    float hips;
    float seatZ;
};
constexpr PersonRow kRows[kPeople] = {
    {"romfs:/people/player_a.ecm", 0.315f, 0.33f},  {"romfs:/people/player_b.ecm", 0.315f, 0.33f},
    {"romfs:/people/keeper.ecm", 0.30f, 0.305f},    {"romfs:/people/market.ecm", 0.29f, 0.29f},
    {"romfs:/people/sanctuary.ecm", 0.315f, 0.335f}, {"romfs:/people/steward.ecm", 0.315f, 0.33f},
    {"romfs:/people/child.ecm", 0.21f, 0.215f},     {"romfs:/people/traveller.ecm", 0.33f, 0.355f},
};
constexpr float kWalkClip = 0.69f, kRunClip = 1.76f, kClipHips = 0.315f;  // tools/people/person_clips.py

u8 pick(const u8 look[kLookParts], int part) { return look[part] < kLookChoices[part] ? look[part] : 0; }
const PersonRow& row(Person p) { return kRows[static_cast<int>(p) < kPeople ? static_cast<int>(p) : 0]; }

}  // namespace

const char* personFile(Person p) { return row(p).file; }

Person personFor(Villager v) {
    switch (v) {
        case Villager::Keeper: return Person::Keeper;
        case Villager::Market: return Person::Market;
        case Villager::Sanctuary: return Person::Sanctuary;
        case Villager::Steward: return Person::Steward;
        case Villager::Child: return Person::Child;
        default: return Person::Traveller;
    }
}

Person playerBody(const u8 look[kLookParts]) { return pick(look, kLookBody) ? Person::PlayerB : Person::PlayerA; }
float personHips(Person p) { return row(p).hips; }

void playerPalette(const u8 look[kLookParts], Rgb out[kPalCount]) {
    for (int k = 0; k < kPalCount; ++k) out[k] = kPlayerFixed[k];
    out[kPalBase] = kSkinTones[pick(look, kLookSkin)];
    out[kPalHorn] = kHairColours[pick(look, kLookHairColour)];
    out[kPalAccent] = kOutfits[pick(look, kLookOutfit)][0];
    out[kPalPattern] = kOutfits[pick(look, kLookOutfit)][1];
    out[kPalIris] = kEyes[pick(look, kLookEyes)];
}

void villagerPalette(Villager v, Rgb out[kPalCount]) {
    const int i = static_cast<int>(v) < kVillagers ? static_cast<int>(v) : 0;
    for (int k = 0; k < kPalCount; ++k) out[k] = kVillagerPalettes[i][k];
}

const char* lookChoiceName(int part, int choice) {
    if (part < 0 || part >= kLookParts || choice < 0 || choice >= kLookChoices[part]) return "";
    switch (part) {
        case kLookBody: return kBodyNames[choice];
        case kLookHair: return kHairNames[choice];
        case kLookHairColour: return kHairColourNames[choice];
        case kLookSkin: return kSkinNames[choice];
        case kLookOutfit: return kOutfitNames[choice];
        default: return kEyeNames[choice];
    }
}

float personWalkSpeed(Person p) { return kWalkClip * row(p).hips / kClipHips; }
float personRunSpeed(Person p) { return kRunClip * row(p).hips / kClipHips; }
Vec3 personSeat(Person p) { return {0, 0.02f, row(p).seatZ}; }

}  // namespace ec
