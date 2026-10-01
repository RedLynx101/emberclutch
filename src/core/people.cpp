#include "core/people.hpp"

#include <cstring>

namespace ec {
namespace {

#include "core/people_looks.inc"

constexpr Rgb kEyes[kEyeColours] = {{86, 56, 40}, {70, 118, 176}, {76, 128, 78}, {118, 110, 124}};
constexpr const char* kEyeNames[kEyeColours] = {"Brown", "Blue", "Green", "Grey"};
constexpr const char* kHairNames[kHairStyles] = {"Swept", "Bob", "Ponytail", "Buns", "Spiky", "Braid"};
constexpr const char* kBodyNames[2] = {"Tunic", "Dress"};

static_assert(sizeof(kRows) / sizeof(kRows[0]) == kPeople, "a row per body (tools/people/gen_looks.py)");
static_assert(sizeof(kStoryPalettes) / sizeof(kStoryPalettes[0]) == kPeople - static_cast<int>(Person::Fig),
              "a palette per story body");
static_assert(sizeof(kFeelFaces) / sizeof(kFeelFaces[0]) == 20, "a face per feeling (core/story Feel)");
constexpr float kWalkClip = 0.69f, kRunClip = 1.708f, kClipHips = 0.315f;  // tools/people/person_clips.py

u8 pick(const u8 look[kLookParts], int part) { return look[part] < kLookChoices[part] ? look[part] : 0; }
const PersonRow& row(Person p) { return kRows[static_cast<int>(p) < kPeople ? static_cast<int>(p) : 0]; }

}  // namespace

const char* personFile(Person p) { return row(p).file; }

Person personByName(const char* id) {
    if (!id || !id[0]) return Person::Count;
    for (int k = 0; k < kPeople; ++k)
        if (std::strcmp(kRows[k].id, id) == 0) return static_cast<Person>(k);
    return Person::Count;
}

FaceLook faceFor(int feel) {
    const u8* f = kFeelFaces[feel >= 0 && feel < 20 ? feel : 0];
    return {f[0], f[1], f[2]};
}

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

void personPalette(Person p, Rgb out[kPalCount]) {
    const int k = static_cast<int>(p);
    if (k >= static_cast<int>(Person::Fig) && k < kPeople) {
        for (int i = 0; i < kPalCount; ++i) out[i] = kStoryPalettes[k - static_cast<int>(Person::Fig)][i];
        return;
    }
    switch (p) {
        case Person::Keeper: villagerPalette(Villager::Keeper, out); return;
        case Person::Market: villagerPalette(Villager::Market, out); return;
        case Person::Sanctuary: villagerPalette(Villager::Sanctuary, out); return;
        case Person::Steward: villagerPalette(Villager::Steward, out); return;
        case Person::Child: villagerPalette(Villager::Child, out); return;
        case Person::Traveller: villagerPalette(Villager::Traveller, out); return;
        default: {
            const u8 look[kLookParts] = {};
            playerPalette(look, out);
        }
    }
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
