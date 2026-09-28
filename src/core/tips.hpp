// The gentle tutorial (1.0, D89): short tips the first time things happen (the first walk in the
// valley, the first ride, a first battle or show, fishing, Energy running low, Love ...), each
// shown once as a small card that doesn't stop play (app/tips_ui showTip). Which have been shown
// is SaveData::progress.tips, a bit each (core/trainer tipSeen/markTip), so the table's order is
// the save's: add new tips at the end. Pure logic (PC-tested).
#pragma once

#include "core/types.hpp"

namespace ec {

struct SaveData;

namespace tips {

enum Tip : u8 {
    kTipCare,          // the den's close-up, the first time: petting fills Love, the tray
    kTipLove,          // Love running low
    kTipEnergy,        // Energy running low
    kTipJournal,       // the Journal opened: pick a goal to track
    kTipValley,        // the first walk in the valley
    kTipMap,           // the valley's map: the fog, travelling by its pins
    kTipRide,          // the first ride on a grown partner
    kTipLantern,       // a festival lantern, the first one near
    kTipMarket,        // the Market, the first visit
    kTipWanderings,    // the trailhead, the first visit
    kTipProfile,       // the profile's training page: levels and stats
    kTipChallenge,     // a challenge board: a cup pays once a day, Energy
    kTipBattle,        // a first battle
    kTipShow,          // a first show at the glade
    kTipFishing,       // the first cast at the cove
    kTipHollow,        // Frostspire Hollow, the first floor
    kTipWardrobe,      // the wardrobe, the first time
    kTipLevelUp,       // a first level gained
    kTipWalk,          // walking together: bond, Love and Play, a hungrier dragon
    kTipTracked,       // the tracked goal's marker on the map
    kTipCount
};

constexpr int kMaxTips = 32;     // bits in Progress::tips
constexpr int kLineChars = 44;   // a card holds two lines of about this many letters
constexpr int kTitleChars = 20;

struct TipInfo {
    const char* title;
    const char* text;  // one or two lines, '\n' between
};
const TipInfo& info(int tip);

// Not shown yet (and a tip there is).
bool due(const SaveData& s, int tip);
// Settings: every tip shown again as things happen.
void reset(SaveData& s);
int shownCount(const SaveData& s);

}  // namespace tips
}  // namespace ec
