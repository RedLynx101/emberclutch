// Hooks from the profile (workstream U) to the other 1.0 workstreams, in one place for the lead
// to wire after merging: a dragon's moves (workstream B, core/battle) and what it wears and its
// dye (workstream P, core/accessories and scene_wardrobe). Until those land, these say there's
// nothing yet, and the profile shows empty slots.
#pragma once

#include "app/app.hpp"

namespace ec::hooks {

// ---- Workstream B (core/battle): a move as the profile lists it.
struct MoveView {
    const char* name = "";
    int element = -1;     // core/kinds element (its colour); -1: a body move
    int power = 0;        // 0 for a status move
    bool status = false;  // roar, preen, rest ...
};
// False for an empty slot (kNone) or an unknown move.
bool moveView(u8 move, MoveView& out);
// The moves it knows now (its kind's elements, its level), best first; at most cap.
int knownMoves(const Dragon& d, u8* out, int cap);
// Its four battle moves: Dragon::moves, empty slots filled from the best known.
void equippedMoves(const Dragon& d, u8 out[kMoveSlots]);
// Swaps a known move into a slot (a move already in another slot trades places); false if it
// can't (not known).
bool equipMove(Dragon& d, int slot, u8 move);

// ---- Workstream P (core/accessories, scene_wardrobe).
const char* accessoryName(u8 accessory);  // "" for kNone or unknown
const char* dyeName(u8 dye);              // 0: its own colours
Rgb dyeColour(u8 dye);
// Opens the wardrobe for a dragon (SaveData index) from the den; false if it can't (not yet).
bool openWardrobe(App& app, int dragonIndex);

}  // namespace ec::hooks
