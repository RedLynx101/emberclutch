#include "core/den_roster.hpp"

namespace ec {
namespace {

bool inDen(const Dragon& d) { return d.location == Location::Den; }
bool isEgg(const Dragon& d) { return d.stage == Stage::Egg; }

}  // namespace

int DenRoster::freeBed() const {
    for (int b = 0; b < kDenDragons; ++b)
        if (dragon[b] < 0) return b;
    return -1;
}

int DenRoster::freeNest() const {
    for (int n = 0; n < kDenEggs; ++n)
        if (egg[n] < 0) return n;
    return -1;
}

DenRoster denRoster(const SaveData& s) {
    DenRoster r;
    for (int& i : r.dragon) i = -1;
    for (int& i : r.egg) i = -1;
    for (int i = 0; i < s.dragonCount; ++i) {
        const Dragon& d = s.dragons[i];
        if (!inDen(d)) continue;
        if (isEgg(d)) {
            if (d.denSlot < kDenEggs && r.egg[d.denSlot] < 0) {
                r.egg[d.denSlot] = i;
                ++r.eggCount;
            }
        } else if (d.denSlot < kDenDragons && r.dragon[d.denSlot] < 0) {
            r.dragon[d.denSlot] = i;
            ++r.dragonCount;
        }
    }
    return r;
}

int settleDen(SaveData& s) {
    bool bed[kDenDragons] = {}, nest[kDenEggs] = {};
    bool homeless[kMaxDragons] = {};
    // Everyone keeps the place they have, first come first served...
    for (int i = 0; i < s.dragonCount; ++i) {
        Dragon& d = s.dragons[i];
        if (!inDen(d)) continue;
        bool* taken = isEgg(d) ? nest : bed;
        const int places = isEgg(d) ? kDenEggs : kDenDragons;
        if (d.denSlot < places && !taken[d.denSlot])
            taken[d.denSlot] = true;
        else
            homeless[i] = true;
    }
    // ...then the rest find a free one, or move out.
    int moved = 0;
    for (int i = 0; i < s.dragonCount; ++i) {
        if (!homeless[i]) continue;
        Dragon& d = s.dragons[i];
        bool* taken = isEgg(d) ? nest : bed;
        const int places = isEgg(d) ? kDenEggs : kDenDragons;
        int free = -1;
        for (int p = 0; p < places && free < 0; ++p)
            if (!taken[p]) free = p;
        if (free >= 0) {
            taken[free] = true;
            d.denSlot = static_cast<u8>(free);
        } else {
            d.location = isEgg(d) ? Location::Vault : Location::Sanctuary;
            d.denSlot = 0;
            ++moved;
        }
    }
    return moved;
}

bool placeEgg(SaveData& s, Dragon& egg) {
    const int nest = denRoster(s).freeNest();
    if (nest < 0) {
        egg.location = Location::Vault;
        egg.denSlot = 0;
        return false;
    }
    egg.location = Location::Den;
    egg.denSlot = static_cast<u8>(nest);
    return true;
}

int bedForHatchling(const SaveData& s) { return denRoster(s).freeBed(); }

}  // namespace ec
