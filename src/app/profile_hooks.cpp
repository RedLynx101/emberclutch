// The stand-ins until workstreams B and P are merged. Lead: replace each body with the call into
// core/battle, core/accessories or the wardrobe scene (the comments say which), then the profile's
// Training and About pages show real moves and clothes with no other change.
#include "app/profile_hooks.hpp"

namespace ec::hooks {

bool moveView(u8 move, MoveView& out) {
    // B: fill `out` from core/battle's move table (name, element, power, status).
    (void)move;
    out = MoveView{};
    return false;
}

int knownMoves(const Dragon& d, u8* out, int cap) {
    // B: core/battle's known moves for d (its elements and level), best first.
    (void)d;
    (void)out;
    (void)cap;
    return 0;
}

void equippedMoves(const Dragon& d, u8 out[kMoveSlots]) {
    // B: d.moves with its empty slots filled from the best known (core/battle).
    for (int i = 0; i < kMoveSlots; ++i) out[i] = d.moves[i];
}

bool equipMove(Dragon& d, int slot, u8 move) {
    // B: core/battle's equipMove(d, slot, move).
    (void)d;
    (void)slot;
    (void)move;
    return false;
}

const char* accessoryName(u8 accessory) {
    // P: core/accessories' name for it.
    (void)accessory;
    return "";
}

const char* dyeName(u8 dye) {
    // P: core/accessories' dye name (0: "natural").
    (void)dye;
    return "";
}

Rgb dyeColour(u8 dye) {
    // P: the dye's colour (core/accessories).
    (void)dye;
    return {250, 226, 196};
}

bool openWardrobe(App& app, int dragonIndex) {
    // P: open scene_wardrobe for this dragon, back to the den after.
    (void)app;
    (void)dragonIndex;
    return false;
}

}  // namespace ec::hooks
