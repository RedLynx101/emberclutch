#include "core/world.hpp"

#include "core/save.hpp"
#include "core/valley.hpp"

namespace ec::world {
namespace {

// In ValleyPlace order. Pins: home warm gold, villages rose, landmarks leaf, challenges sky,
// trails amber, secrets violet.
constexpr Rgb kHome{245, 196, 81}, kVillage{236, 128, 140}, kLand{120, 196, 110}, kChal{110, 190, 235},
    kTrail{230, 160, 80}, kSecret{176, 130, 230};
const PlaceInfo kPlaces[kPlaceCount] = {
    {"Your den", "Home: a cave in the cliff by the falls.", PlaceKind::Home, kHome, true, false, 30},
    {"Market Village", "Stalls, lanterns and the egg of the day.", PlaceKind::Village, kVillage, true, false, 45},
    {"The Nesting Stone", "An old stone on the hilltop where pairs settle.", PlaceKind::Landmark, kLand, true, false, 30},
    {"Sanctuary Meadow", "The keepers' meadow, where dragons rest.", PlaceKind::Village, kVillage, true, false, 45},
    {"The Cold Vault", "A cave in the cold heights where eggs keep.", PlaceKind::Landmark, kLand, true, false, 30},
    {"Wanderers' Trailhead", "Where the long walks start.", PlaceKind::Trail, kTrail, true, false, 30},
    {"The Arena", "Challenges, cups and the festival's great lantern.", PlaceKind::Challenge, kChal, true, false, 40},
    {"Mirror Lake", "Still water under the isles.", PlaceKind::Landmark, kLand, false, false, 60},
    {"The Keeper's Lodge", "The old keeper's home beside the falls.", PlaceKind::Village, kVillage, false, false, 25},
    {"The Floating Isles", "Green islands in the sky: wings only.", PlaceKind::Landmark, kLand, true, true, 40},
    {"Honeyroot Orchard", "Fruit trees in rows, and the best Fruit Catch.", PlaceKind::Landmark, kLand, false, false, 35},
    {"Windmill Bridge", "A bridge over the river by a sleepy windmill.", PlaceKind::Landmark, kLand, false, false, 30},
    {"The Hidden Grotto", "Behind the falls, where the light is green.", PlaceKind::Secret, kSecret, false, false, 14},
    {"Starwatch Ruins", "An old tower on a crag, looking at the sky.", PlaceKind::Secret, kSecret, false, true, 20},
};

bool valid(int p) { return p >= 0 && p < kPlaceCount; }

}  // namespace

int placeCount() { return kPlaceCount; }
const PlaceInfo& placeInfo(int p) { return kPlaces[valid(p) ? p : 0]; }

bool placeFound(const SaveData& s, int p) { return valid(p) && (s.world.placesFound >> p) & 1; }

bool findPlace(SaveData& s, int p) {
    if (!valid(p) || placeFound(s, p)) return false;
    s.world.placesFound |= 1u << p;
    return true;
}

int placesFound(const SaveData& s) {
    int n = 0;
    for (int p = 0; p < kPlaceCount; ++p) n += placeFound(s, p);
    return n;
}

bool lanternLit(const SaveData& s, int p) { return valid(p) && (s.world.lanternsLit >> p) & 1; }

bool lightLantern(SaveData& s, int p) {
    if (!valid(p) || !kPlaces[p].lantern || lanternLit(s, p)) return false;
    s.world.lanternsLit |= 1u << p;
    return true;
}

int lanternsLit(const SaveData& s) {
    int n = 0;
    for (int p = 0; p < kPlaceCount; ++p) n += kPlaces[p].lantern && lanternLit(s, p);
    return n;
}

int lanternCount() {
    int n = 0;
    for (const PlaceInfo& p : kPlaces) n += p.lantern;
    return n;
}

void startWorld(SaveData& s) {
    if (s.world.placesFound == 0) s.world.placesFound = 1u << kPlaceDen;
}

}  // namespace ec::world
