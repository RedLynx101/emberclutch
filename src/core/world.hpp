// Skyreach Valley's places (Beta WP4, WP7; D73-D86) and the world state the save keeps: your
// look, where you are, your travel partner, the places found, the festival's lanterns lit, the
// cups won and a few things done. A place's position comes from the valley file (core/valley:
// its ValleyPlace id is its index here); what it is and how it shows on the map is here. Pure
// logic (PC-tested).
#pragma once

#include "core/types.hpp"

namespace ec {

struct SaveData;

// Your look (the creator, D74): two body shapes, six hair styles and colours, five skin tones,
// three outfit colours; your eyes.
enum LookPart : u8 { kLookBody, kLookHair, kLookHairColour, kLookSkin, kLookOutfit, kLookEyes, kLookParts };
constexpr u8 kLookChoices[kLookParts] = {2, 6, 6, 5, 3, 4};

// Things done that the campaign and the world remember (a bit each in WorldState::flags).
enum WorldFlag : u32 {
    kFlagEnteredValley = 1u << 0,  // out of the den into the valley for the first time
    kFlagMetKeeper = 1u << 1,      // the old keeper at the waterfall
    kFlagHeardStory = 1u << 2,     // the keeper's story, on the hilltop
    kFlagFoundStray = 1u << 3,     // the stray dragon in the meadow, sniffed out
    kFlagGlided = 1u << 4,         // a young dragon's first glide from a height
    kFlagRode = 1u << 5,           // the first ride on a grown dragon
    kFlagMetTraveller = 1u << 6,   // the traveller at the trailhead
    kFlagWandered = 1u << 7,       // a Wandering since the traveller asked
    kFlagFestival = 1u << 8,       // the festival night: the star-born egg left
    kFlagMetMarket = 1u << 9,      // the Market's keeper
    kFlagMetSanctuary = 1u << 10,  // the Sanctuary's keeper
    kFlagMetSteward = 1u << 11,    // the arena's steward
    kFlagStarEgg = 1u << 12,       // the star-born egg given
    kFlagLoveLetter = 1u << 13,    // the letter left at the picnic on the Stone's hill, read (D133)
};

enum class Challenge : u8 { FruitCatch, SkyRings, LanternTrial, Count };
constexpr int kChallenges = static_cast<int>(Challenge::Count);
constexpr int kCups = 4;  // Ember, Flame, Blaze, Starfire

// The world's bytes in the save (after its size byte): look 6, made 1, x y heading 12, in the
// valley 1, partner 4, places 4, lanterns 4, quests 8, cups 3, flags 4, ribbons 2, the stall's day 4 and
// its spots 4, the finds 4, the map's fog 128, the challenges' bests 24.
constexpr int kStallSpots = 4;  // the Market's goods stall (D86)
constexpr int kExploredBytes = 128;  // the map's fog: 32 x 32 cells, a bit each (core/finds)
constexpr int kWorldBytes = kLookParts + 1 + 12 + 1 + 4 + 4 + 4 + 8 + kChallenges + 4 + 2 + 4 + kStallSpots + 4 +
                            kExploredBytes + kChallenges * kCups * 2;

struct WorldState {
    u8 look[kLookParts] = {0, 0, 0, 1, 0, 0};
    u8 lookMade = 0;                    // the creator has been through once
    float x = 0, y = 0, heading = 0;    // where you were in the valley (metres; start: the den's door)
    u8 inValley = 0;                    // left in the valley (Continue puts you back there)
    u32 partnerId = 0;                  // the travel partner (D81; 0: none chosen)
    u32 placesFound = 0;                // a bit per place (the den from the start)
    u32 lanternsLit = 0;                // a bit per place with a festival lantern
    u8 quest[8] = {};                   // per quest (core/campaign): 0 not begun, 1.. its step, 0xFF done
    u8 cups[kChallenges] = {};          // per challenge: the highest cup won (0 none .. 4 Starfire)
    u32 flags = 0;                      // WorldFlag bits
    u16 ribbons = 0;                    // the cups' ribbons won (den decor): a bit per cup, challenge * 4 + cup - 1
    s32 stallDay = -1000000;            // the day the Market's goods stall was last picked (D86)
    u8 stall[kStallSpots] = {0xFF, 0xFF, 0xFF, 0xFF};  // its spots (core/items Item; 0xFF: nothing)
    u32 finds = 0;                      // the valley's finds taken, a bit each (core/finds)
    u8 explored[kExploredBytes] = {};   // the map's fog cleared, a bit a cell (core/finds)
    // Per challenge and cup (core/challenges), your best: Sky Rings in tenths of a second, the
    // others in points (0: not run yet).
    u16 best[kChallenges][kCups] = {};
};

namespace world {

enum class PlaceKind : u8 { Home, Village, Landmark, Challenge, Trail, Secret };

struct PlaceInfo {
    const char* name;
    const char* blurb;
    PlaceKind kind;
    Rgb pin;            // its pin on the painted map
    bool lantern;       // one of the festival's lanterns stands there
    bool fromAir;       // only reached from the air (a grown dragon)
    float findRadius;   // metres: coming this near finds it
};

int placeCount();
const PlaceInfo& placeInfo(int place);  // by core/valley ValleyPlace id
bool placeFound(const SaveData& s, int place);
bool findPlace(SaveData& s, int place);  // true the first time
int placesFound(const SaveData& s);

bool lanternLit(const SaveData& s, int place);
bool lightLantern(SaveData& s, int place);  // true the first time (and only where there's one)
int lanternsLit(const SaveData& s);
int lanternCount();

// A fresh world for a new game, or for a save from before Beta: the den found.
void startWorld(SaveData& s);

}  // namespace world
}  // namespace ec
