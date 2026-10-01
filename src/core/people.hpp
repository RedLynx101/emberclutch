// The valley's people in 3D (Beta WP13, D74-D75; the Storybook look on everyone, D138): your
// character, the villagers and the story's own people, drawn with the dragons' shader from
// romfs/people/<id>.ecm and played from one clip library (romfs/anims/person.eca, tools/people).
// Who wears which model and colours, the creator's choices as a palette, each feeling's face, and
// how fast their walk and run clips cover the ground (so the legs keep pace with the feet). Pure
// logic (PC-tested).
#pragma once

#include "core/model.hpp"
#include "core/types.hpp"
#include "core/villagers.hpp"
#include "core/world.hpp"

namespace ec {

enum class Person : u8 {
    PlayerA, PlayerB, Keeper, Market, Sanctuary, Steward, Child, Traveller,
    // the story's own (D138): their bodies are story/people.story's `body`
    Fig, Tam, Tove, Linnet, Madder, Celestine, Primrose, Marigold, Rook, Seraphine, Solenne, Count
};
constexpr int kPeople = static_cast<int>(Person::Count);
constexpr int kHairStyles = 6;  // the player's hair meshes (kind 2, group kGroupHair, variant 0..5)
constexpr int kEyeColours = 4;

// The feelings kit's faces (D138): each person's eyes (kGroupEyes), mouths (kGroupPersonMouth) and
// brows (kGroupBrows) come in variants, one of each drawn: a feeling's face picks them.
constexpr int kEyeKinds = 10, kMouthKinds = 10, kBrowKinds = 4;
struct FaceLook {
    u8 eyes = 0, mouth = 0, brows = 0;
};
FaceLook faceFor(int feel);             // a core/story Feel (out of range: calm)

const char* personFile(Person p);   // romfs:/people/<id>.ecm
Person personByName(const char* id);  // a body by its id ("fig", "keeper"), Count if none
Person personFor(Villager v);
Person playerBody(const u8 look[kLookParts]);
float personHips(Person p);         // the hips' height, metres (the child's are lower)

// The creator's choice as a palette (skin, hair, outfit, eyes on the fixed rest).
void playerPalette(const u8 look[kLookParts], Rgb out[kPalCount]);
void villagerPalette(Villager v, Rgb out[kPalCount]);
// Anyone's own colours: a villager's, a story person's (the players: body A's first choices).
void personPalette(Person p, Rgb out[kPalCount]);
// The creator's names for each choice (kLookChoices of each part).
const char* lookChoiceName(int part, int choice);

// The walk and run clips' ground speed for this body (m/s at rate 1): the clip plays at
// speed / this, so the steps land.
float personWalkSpeed(Person p);
float personRunSpeed(Person p);
// Where a rider's seat is on the person's own model (metres, the bottom of the pelvis).
Vec3 personSeat(Person p);

}  // namespace ec
