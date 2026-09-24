#include "core/items.hpp"

#include <cmath>

namespace ec {
namespace {

constexpr ItemInfo kInfo[kItems] = {
    {"Feather wand", "Dangle it and watch the pounce.", 60, ItemKind::Toy, 0},
    {"Tug rope", "A knotted rope for tug-of-war.", 80, ItemKind::Toy, 1},
    {"Puzzle orb", "Roll it about: a treat drops out.", 120, ItemKind::Toy, 2},
    {"Food bowl", "Fill it, and they eat when hungry.", 90, ItemKind::Toy, 3},
    {"Silver brush", "Brushing shines twice as fast.", 150, ItemKind::Grooming, 0},
    {"Bubble soap", "Baths leave a lasting sparkle.", 100, ItemKind::Grooming, 1},
    {"Warm stones", "Eggs in the nests cool half as fast.", 200, ItemKind::Nest, 0},
    {"Ember rug", "Red and gold, like the hearth.", 120, ItemKind::Rug, 0},
    {"Tide rug", "Blue waves to curl up on.", 120, ItemKind::Rug, 1},
    {"Grove rug", "Soft green, like moss.", 120, ItemKind::Rug, 2},
    {"Lumen rug", "Cream with a golden star.", 120, ItemKind::Rug, 3},
    {"Brass lantern", "A warm glow for the evenings.", 90, ItemKind::Lantern, 0},
    {"Glass lantern", "A cool blue light.", 90, ItemKind::Lantern, 1},
    {"Paper lantern", "A soft round glow.", 70, ItemKind::Lantern, 2},
    {"Driftwood perch", "A branch to sit up high on.", 140, ItemKind::Perch, 0},
    {"Stone perch", "A carved stone post.", 140, ItemKind::Perch, 1},
    {"Fern", "Green fronds in a clay pot.", 70, ItemKind::Plant, 0},
    {"Moonflower", "Pale blooms that open at night.", 90, ItemKind::Plant, 1},
    {"Emberbloom", "Flame-red petals, warm to touch.", 90, ItemKind::Plant, 2},
    {"Flame banner", "An ember crest on red.", 100, ItemKind::Banner, 0},
    {"Wave banner", "A tide crest on blue.", 100, ItemKind::Banner, 1},
    {"Star banner", "A lumen crest on cream.", 100, ItemKind::Banner, 2},
};

constexpr u8 kNone = 0xFF;

// Where each toy is first set down: around the rug, clear of the beds and the nests.
constexpr Vec2 kToySpots[kToys] = {{-3.0f, 1.6f}, {-2.4f, -0.9f}, {2.0f, 1.9f}, {-1.4f, 3.0f}};

bool inDen(const Dragon& d) { return d.location == Location::Den && d.wanderSince == 0 && d.stage != Stage::Egg; }

}  // namespace

const ItemInfo& itemInfo(Item i) { return kInfo[i < Item::Count ? static_cast<int>(i) : 0]; }

int decorSpot(ItemKind k) {
    return k >= ItemKind::Rug ? static_cast<int>(k) - static_cast<int>(ItemKind::Rug) : -1;
}

bool owns(const SaveData& s, Item i) { return i < Item::Count && (s.owned >> static_cast<int>(i)) & 1u; }

bool buyItem(SaveData& s, Item i) {
    if (i >= Item::Count || owns(s, i) || s.gleam < itemInfo(i).price) return false;
    s.gleam -= itemInfo(i).price;
    s.owned |= 1u << static_cast<int>(i);
    const int toy = static_cast<int>(i);
    if (toy < kToys) setToyAt(s, toy, kToySpots[toy]);
    const int spot = decorSpot(itemInfo(i).kind);
    if (spot >= 0 && s.decor[spot] == kNone) s.decor[spot] = static_cast<u8>(i);
    return true;
}

Item decorAt(const SaveData& s, int spot) {
    if (spot < 0 || spot >= kDecorSpots || s.decor[spot] >= kItems) return Item::Count;
    return static_cast<Item>(s.decor[spot]);
}

bool putUp(SaveData& s, Item i) {
    const int spot = i < Item::Count ? decorSpot(itemInfo(i).kind) : -1;
    if (spot < 0 || !owns(s, i)) return false;
    s.decor[spot] = static_cast<u8>(i);
    return true;
}

void takeDown(SaveData& s, int spot) {
    if (spot >= 0 && spot < kDecorSpots) s.decor[spot] = kNone;
}

bool isUp(const SaveData& s, Item i) {
    const int spot = i < Item::Count ? decorSpot(itemInfo(i).kind) : -1;
    return spot >= 0 && decorAt(s, spot) == i;
}

Vec2 defaultToySpot(int toy) { return kToySpots[toy >= 0 && toy < kToys ? toy : 0]; }

Vec2 toyAt(const SaveData& s, int toy) {
    if (toy < 0 || toy >= kToys) return {};
    return {s.toyPos[toy][0] / 100.0f, s.toyPos[toy][1] / 100.0f};
}

void setToyAt(SaveData& s, int toy, Vec2 at) {
    if (toy < 0 || toy >= kToys) return;
    auto centi = [](float v) {
        const float c = std::fmax(-3000.0f, std::fmin(3000.0f, v * 100.0f));
        return static_cast<s16>(std::lround(c));
    };
    s.toyPos[toy][0] = centi(at.x);
    s.toyPos[toy][1] = centi(at.y);
}

int bowlCount(const SaveData& s) {
    int n = 0;
    while (n < kBowlPortions && s.bowl[n] < static_cast<u8>(Food::Count)) ++n;
    return n;
}

Food bowlFood(const SaveData& s) {
    const int n = bowlCount(s);
    return n > 0 ? static_cast<Food>(s.bowl[n - 1]) : Food::Count;
}

bool fillBowl(SaveData& s, Food f) {
    if (!owns(s, Item::FoodBowl) || f >= Food::Count || s.pouch[static_cast<int>(f)] == 0) return false;
    const int n = bowlCount(s);
    if (n >= kBowlPortions) return false;
    --s.pouch[static_cast<int>(f)];
    s.bowl[n] = static_cast<u8>(f);
    return true;
}

bool eatFromBowl(SaveData& s, int i, s64 now) {
    const int n = bowlCount(s);
    if (n == 0 || i < 0 || i >= s.dragonCount) return false;
    Dragon& d = s.dragons[i];
    int pick = -1;
    for (int k = n - 1; k >= 0; --k) {  // the newest first; a favourite wins outright
        const Taste taste = tasteOf(d, static_cast<Food>(s.bowl[k]));
        if (taste == Taste::Favorite) {
            pick = k;
            break;
        }
        if (taste != Taste::Disliked && pick < 0) pick = k;
    }
    if (pick < 0) return false;
    const Food f = static_cast<Food>(s.bowl[pick]);
    feed(d, foodInfo(f).belly, tasteOf(d, f) == Taste::Favorite);
    for (int k = pick; k < n - 1; ++k) s.bowl[k] = s.bowl[k + 1];  // the rest close up
    s.bowl[n - 1] = kNone;
    (void)now;
    return true;
}

int feedFromBowl(SaveData& s, s64 now) {
    int ate = 0;
    for (int i = 0; i < s.dragonCount && bowlFood(s) != Food::Count; ++i)
        if (inDen(s.dragons[i]) && s.dragons[i].needs.belly < kBowlHungry && eatFromBowl(s, i, now)) ++ate;
    return ate;
}

float eggCooling(const SaveData& s) { return owns(s, Item::WarmStones) ? 0.5f : 1.0f; }
float brushRate(const SaveData& s) { return owns(s, Item::SilverBrush) ? 2.0f : 1.0f; }
float bathShine(const SaveData& s) { return owns(s, Item::BubbleSoap) ? 15.0f : 0.0f; }

}  // namespace ec
