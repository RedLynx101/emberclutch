// The valley's people in 3D (Beta WP13, D74-D75): your character and the villagers, drawn with
// the dragons' shader from romfs/people/<id>.ecm and played from one clip library
// (romfs/anims/person.eca, tools/people). Who wears which model and colours, the creator's
// choices as a palette, and how fast their walk and run clips cover the ground (so the legs
// keep pace with the feet). Pure logic (PC-tested).
#pragma once

#include "core/model.hpp"
#include "core/types.hpp"
#include "core/villagers.hpp"
#include "core/world.hpp"

namespace ec {

enum class Person : u8 { PlayerA, PlayerB, Keeper, Market, Sanctuary, Steward, Child, Traveller, Count };
constexpr int kPeople = static_cast<int>(Person::Count);
constexpr int kHairStyles = 6;  // the player's hair meshes (kind 2, group kGroupHair, variant 0..5)
constexpr int kEyeColours = 4;

const char* personFile(Person p);   // romfs:/people/<id>.ecm
Person personFor(Villager v);
Person playerBody(const u8 look[kLookParts]);
float personHips(Person p);         // the hips' height, metres (the child's are lower)

// The creator's choice as a palette (skin, hair, outfit, eyes on the fixed rest).
void playerPalette(const u8 look[kLookParts], Rgb out[kPalCount]);
void villagerPalette(Villager v, Rgb out[kPalCount]);
// The creator's names for each choice (kLookChoices of each part).
const char* lookChoiceName(int part, int choice);

// The walk and run clips' ground speed for this body (m/s at rate 1): the clip plays at
// speed / this, so the steps land.
float personWalkSpeed(Person p);
float personRunSpeed(Person p);
// Where a rider's seat is on the person's own model (metres, the bottom of the pelvis).
Vec3 personSeat(Person p);

}  // namespace ec
