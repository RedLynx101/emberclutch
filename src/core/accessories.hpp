// 1.0's beauty (D90): the accessories a dragon wears in its four slots (head, neck, back, tail)
// and the dyes that tint its colours. Each accessory has a name, a slot, the shape its mesh is
// built in (core/wear_mesh), two colours and a trim, the styles it carries (the pageant's themes
// favour some) and where it comes from: Moonpetal Glade's stall, the shows' prizes, Frostspire
// Hollow's rewards or the valley's finds. Owning is kept in SaveData::progress (core/trainer),
// wearing in Dragon::wear and Dragon::dye. Pure logic (PC-tested in tests/test_pageant.cpp).
#pragma once

#include "core/dragon.hpp"
#include "core/model.hpp"
#include "core/types.hpp"

namespace ec {

struct SaveData;

// Dragon::wear's order.
enum class WearSlot : u8 { Head, Neck, Back, Tail, Count };
static_assert(static_cast<int>(WearSlot::Count) == kWearSlots, "a slot per Dragon::wear");

// The styles an accessory carries (a bit each); the themes favour some (core/pageant).
enum StyleTag : u8 {
    kStyleCute = 1,
    kStyleElegant = 2,
    kStyleWild = 4,
    kStyleFestive = 8,
    kStyleFrosty = 16,
    kStyleFiery = 32,
    kStyleFloral = 64,
    kStyleStarry = 128,
};
constexpr int kStyleTags = 8;
const char* styleName(int bit);  // the tag's name by its bit index (0 cute .. 7 starry)

enum class WearSource : u8 { Stall, Prize, Hollow, Find, Gift };  // (Gift: a story's reward, never sold: D137)

// The meshes (core/wear_mesh builds each in its slot's own frame, core/wear_fit puts it on).
enum class WearShape : u8 {
    SunHat, TopHat, FlowerCrown, Tiara, Crown, Circlet, PartyHat, FeatherCrest,        // head
    NeckBow, Scarf, Collar, Bell, Pendant, Ruff, Lei,                                   // neck
    Saddle, Cape, Blanket, Sash, Garland,                                               // back
    TailBow, TailRibbons, TailRing, StarCharm, TailBell, SnowCharm, TailWreath, Tassel,  // tail
    Count,
};
constexpr int kWearShapes = static_cast<int>(WearShape::Count);

struct Accessory {
    const char* name;
    WearSlot slot;
    WearShape shape;
    u16 price;        // Gleam at the stall (a prize's: what it would cost)
    Rgb colour[2];    // its main colour and its second
    Rgb trim;         // metal, lace, a gem's setting
    Rgb gem;          // what glows (a gem, a star, a flower's heart)
    u8 styles;        // StyleTag bits
    WearSource source;
};

constexpr int kAccessoryCount = 37;  // (room for 64: Progress::accessories)
int accessoryCount();
const Accessory& accessoryInfo(int accessory);
const char* slotName(WearSlot slot);

// ---- Dyes: a dragon's body colours tinted toward the dye's (0 is its own colours, always yours).
struct DyeInfo {
    const char* name;
    Rgb main;   // the body's colour toward
    Rgb light;  // the belly's and the accent's
    u16 price;
    WearSource source;
    float depth;  // how far toward its own lightness (0 keeps the dragon's; coal and snow go far)
};
constexpr int kDyeCount = 13;  // natural and twelve (room for 32: Progress::dyes)
int dyeCount();
const DyeInfo& dyeInfo(int dye);
// The dragon's palette (core/kinds kindPalette) with its dye: the body, the accent, the pattern
// and the wings' membrane move toward the dye's colours, each keeping its own lightness.
void applyDye(int dye, Rgb pal[kPalCount]);

namespace acc {

// What it wears in a slot (-1: nothing).
int worn(const Dragon& d, WearSlot slot);
int wornCount(const Dragon& d);
// The styles of everything it wears, together.
u8 wornStyles(const Dragon& d);
// Puts an accessory you own on in its slot (whatever was there comes off). False if it isn't yours.
bool putOn(const SaveData& s, Dragon& d, int accessory);
void takeOff(Dragon& d, WearSlot slot);
// Dyes it (a dye you own; 0 its own colours). False if the dye isn't yours.
bool dyeWith(const SaveData& s, Dragon& d, int dye);
// Owned accessories for a slot, in the table's order; returns how many (at most cap).
int ownedFor(const SaveData& s, WearSlot slot, int* out, int cap);

// ---- The glade's stalls. The accessory stall shows kStallShown of its stock each day (the same
// all day; one you own shows as yours); the dye stall has every dye.
constexpr int kStallShown = 6;
void stallPicks(s32 day, int out[kStallShown]);
bool buyAccessory(SaveData& s, int accessory);  // false: yours already, not enough Gleam
bool buyDye(SaveData& s, int dye);

// ---- Rewards, for whoever gives them: a show's prize, Frostspire Hollow's, a find's. Something
// from that source you don't own yet (-1 if you have them all), picked by the seed.
int unownedFrom(const SaveData& s, WearSource source, u32 seed);
int prizeDye(const SaveData& s, u32 seed);  // a prize dye not owned yet (0: none left)

// ---- How well colours go together (the pageant's Look): 1 for a close match or a pleasing
// complement, less for a clash; greys go with anything.
float colourMatch(Rgb a, Rgb b);
// How well what it wears suits its own colours (0..1; 0.5 with nothing on).
float suitsColours(const Dragon& d, const Rgb pal[kPalCount]);

}  // namespace acc
}  // namespace ec
