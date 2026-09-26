// Things you buy once at the Market and keep (Alpha 2 WP7, content inventory section 7):
// toys that live on the den floor, two grooming upgrades, warm stones for the egg nests and
// den decor, one piece in each of five spots. Pure logic: PC-tested in tests/test_main.cpp.
#pragma once

#include "core/care.hpp"
#include "core/math3d.hpp"
#include "core/save.hpp"

namespace ec {

enum class Item : u8 {
    // toys, in DenToys order: they lie on the den floor and the dragons play with them
    FeatherWand, TugRope, PuzzleOrb, FoodBowl,
    // grooming: the silver brush pleases more, bubble soap leaves a bath's sparkle
    SilverBrush, BubbleSoap,
    // the egg nests: eggs cool half as fast
    WarmStones,
    // decor, by spot
    RugEmber, RugTide, RugGrove, RugLumen,
    LanternBrass, LanternGlass, LanternPaper,
    PerchDriftwood, PerchStone,
    PlantFern, PlantMoonflower, PlantEmberbloom,
    BannerFlame, BannerWave, BannerStar,
    Count,
};
constexpr int kItems = static_cast<int>(Item::Count);
constexpr int kToys = 4;  // the first four items

enum class ItemKind : u8 { Toy, Grooming, Nest, Rug, Lantern, Perch, Plant, Banner };
constexpr int kDecorSpots = 5;  // one each: rug, lantern, perch, plant, banner

struct ItemInfo {
    const char* name;
    const char* blurb;
    u32 price;
    ItemKind kind;
    u8 variant;  // which of its kind (the look: colours, shape)
};
const ItemInfo& itemInfo(Item i);
// The decor spot a kind goes in (0..4), or -1 for toys and the rest.
int decorSpot(ItemKind k);

bool owns(const SaveData& s, Item i);
// Buys it (once: things last). A toy is set down on the den floor; decor goes up at once if
// its spot is empty. False: already yours, or not enough Gleam.
bool buyItem(SaveData& s, Item i);
// The decor in a spot (Item::Count: none).
Item decorAt(const SaveData& s, int spot);
// Puts owned decor up in its spot (whatever was there goes back in the chest). False if
// it isn't yours or isn't decor.
bool putUp(SaveData& s, Item i);
void takeDown(SaveData& s, int spot);
bool isUp(const SaveData& s, Item i);

// Where a toy lies on the den floor (den units, core/behavior DenLayout).
Vec2 toyAt(const SaveData& s, int toy);
void setToyAt(SaveData& s, int toy, Vec2 at);
Vec2 defaultToySpot(int toy);

// The food bowl: any foods from the pouch, up to kBowlPortions portions, and hungry dragons
// eat from it on their own, even while you're away.
constexpr int kBowlPortions = kBowlSlots;
constexpr float kBowlHungry = 35;  // a dragon's Belly below this sends it to the bowl
bool fillBowl(SaveData& s, Food f);  // false: no bowl, none in the pouch, or the bowl is full
int bowlCount(const SaveData& s);    // portions in it
Food bowlFood(const SaveData& s);    // the last one put in (what shows on top), Food::Count when empty
// Dragon `i` eats a portion (fills its Belly): its favourite if that's in there, else the
// newest one it doesn't dislike. False if the bowl is empty or it dislikes everything in it.
bool eatFromBowl(SaveData& s, int i, s64 now);
// Every hungry den dragon eats from the bowl while there's food (the world's tick, and time
// away). Returns how many ate.
int feedFromBowl(SaveData& s, s64 now);

// How fast eggs cool in the den's nests (warm stones halve it).
float eggCooling(const SaveData& s);
// How much a brush stroke counts (the silver brush) and the bath's extra sparkle (bubble soap).
float brushRate(const SaveData& s);
float bathShine(const SaveData& s);

}  // namespace ec
