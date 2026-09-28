// The stand-ins until workstreams B and P are merged. Lead: replace each body with the call into
// core/battle, core/accessories or the wardrobe scene (the comments say which), then the profile's
// Training and About pages show real moves and clothes with no other change.
#include "app/profile_hooks.hpp"

#include "app/wardrobe.hpp"
#include "core/accessories.hpp"

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
    return accessory < accessoryCount() ? accessoryInfo(accessory).name : "";  // (the pageant's)
}

const char* dyeName(u8 dye) { return dye < dyeCount() ? dyeInfo(dye).name : ""; }

Rgb dyeColour(u8 dye) { return dye < dyeCount() ? dyeInfo(dye).main : Rgb{250, 226, 196}; }

bool openWardrobe(App& app, int dragonIndex) {
    ec::openWardrobe(app, dragonIndex, app.scene);  // (the wardrobe, back to where you were)
    return true;
}

}  // namespace ec::hooks
