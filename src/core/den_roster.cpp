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

int DenRoster::presentCount() const {
    int n = 0;
    for (int b = 0; b < kDenDragons; ++b) n += dragon[b] >= 0 && !away[b];
    return n;
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
    for (bool& a : r.away) a = false;
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
            r.away[d.denSlot] = d.wanderSince != 0;
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

int bedForHatchling(const SaveData& s) {
    const DenRoster r = denRoster(s);
    if (r.freeBed() >= 0) return r.freeBed();
    for (int b = 0; b < kDenDragons; ++b)
        if (r.away[b]) return b;
    return -1;
}

int makeRoomForHatchling(SaveData& s) {
    const DenRoster r = denRoster(s);
    const int bed = bedForHatchling(s);
    if (bed >= 0 && r.dragon[bed] >= 0) {  // a wanderer's: it'll come home to the Sanctuary
        Dragon& wanderer = s.dragons[r.dragon[bed]];
        wanderer.location = Location::Sanctuary;
        wanderer.denSlot = 0;
    }
    return bed;
}

int vaultCount(const SaveData& s) {
    int n = 0;
    for (int i = 0; i < s.dragonCount; ++i) n += s.dragons[i].location == Location::Vault;
    return n;
}

bool storeAway(SaveData& s, int index) {
    if (index < 0 || index >= s.dragonCount) return false;
    Dragon& d = s.dragons[index];
    if (!inDen(d) || d.wanderSince != 0) return false;
    if (isEgg(d)) {
        if (vaultCount(s) >= kVaultEggs) return false;
        d.location = Location::Vault;
    } else {
        d.location = Location::Sanctuary;
    }
    d.denSlot = 0;
    return true;
}

bool bringHome(SaveData& s, int index, s64 now) {
    if (index < 0 || index >= s.dragonCount) return false;
    Dragon& d = s.dragons[index];
    if (inDen(d)) return false;
    const DenRoster r = denRoster(s);
    const int place = isEgg(d) ? r.freeNest() : r.freeBed();
    if (place < 0) return false;
    d.location = Location::Den;
    d.denSlot = static_cast<u8>(place);
    if (!isEgg(d)) markVisit(d, now);  // the keepers kept it company: no "you haven't visited"
    return true;
}

}  // namespace ec
