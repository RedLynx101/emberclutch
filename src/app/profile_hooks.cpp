// The stand-ins until workstreams B and P are merged. Lead: replace each body with the call into
// core/battle, core/accessories or the wardrobe scene (the comments say which), then the profile's
// Training and About pages show real moves and clothes with no other change.
#include "app/profile_hooks.hpp"

#include "app/wardrobe.hpp"
#include "core/accessories.hpp"
#include "core/battle.hpp"

namespace ec::hooks {

bool moveView(u8 move, MoveView& out) {  // (the battles' move table)
    out = MoveView{};
    if (move == kNone || move >= battle::moveCount()) return false;
    const battle::MoveInfo& m = battle::moveInfo(move);
    out.name = m.name;
    out.element = m.element == battle::kBody ? -1 : m.element;
    out.power = m.power;
    out.status = m.power == 0;
    return true;
}

int knownMoves(const Dragon& d, u8* out, int cap) { return battle::bestKnownMoves(d, out, cap); }

void equippedMoves(const Dragon& d, u8 out[kMoveSlots]) { battle::equippedMoves(d, out); }

bool equipMove(Dragon& d, int slot, u8 move) { return battle::equipMove(d, slot, move); }

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
