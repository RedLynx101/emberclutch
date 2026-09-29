#include "core/tips.hpp"

#include "core/save.hpp"
#include "core/trainer.hpp"

namespace ec::tips {
namespace {

// Warm and short, in the game's voice: what it is and the one thing to know (the tests keep
// each line to a card's width).
const TipInfo kTips[kTipCount] = {
    {"Caring", "Stroke your dragon to fill its Love.\nFood, brushes and toys wait in the tray."},
    {"Love", "Love fills with petting and brushing.\nA dragon low on Love feels lonely."},
    {"Energy", "Games, challenges and battles tire it out.\nA good sleep brings its Energy back."},
    {"The Journal", "Tap a flag to track a goal:\nthe valley's map shows where to go."},
    {"Skyreach Valley", "Walk with the pad, hold B to run.\nL and R turn the view; A does what's near."},
    {"The map", "Tap a pin on the map to travel there at\nonce. The den's pin takes you home."},
    {"Riding", "A takes off and flaps; B dives.\nLet go to glide. Land, then Down to get off."},
    {"The lanterns", "Stand by a dark lantern and press A:\nyour dragon breathes it alight."},
    {"The Market", "Food, goods that change each day\nand an egg of the day. L and R: the stalls."},
    {"The Wanderings", "Pick a dragon and set off, then close\nyour 3DS and walk: your steps take it along."},
    {"Growing strong", "Challenges and battles bring experience;\neach level makes your dragon stronger."},
    {"Challenges", "A cup pays once a day, the first win more.\nEach go costs your dragon some Energy."},
    {"Battles", "Pick one of four moves each turn.\nSome elements hit others extra hard."},
    {"Pageants", "Look, Poise and Performance each score.\nA clean, well-dressed dragon shines."},
    {"Fishing", "A to cast, and wait for the bobber to dip.\nStrike, then keep the line in the band."},
    {"Frostspire Hollow", "Wild dragons, floor after floor, deeper\nand harder. Every fifth is a checkpoint."},
    {"The wardrobe", "Try on hats, scarves and dyes. Some suit\na show's theme better than others."},
    {"Level up!", "Each level adds to its stats: see them\nin its profile (tap the heartglow)."},
    {"Walking together", "Walks raise bond, Love and Play, and\nmake your dragon hungry a little faster."},
    {"On the map", "The gold marker is your tracked goal;\na soft circle is somewhere to search."},
};

static_assert(kTipCount <= kMaxTips, "Progress::tips holds 32");

}  // namespace

const TipInfo& info(int tip) { return kTips[tip >= 0 && tip < kTipCount ? tip : 0]; }

bool due(const SaveData& s, int tip) { return tip >= 0 && tip < kTipCount && !trainer::tipSeen(s, tip); }

void reset(SaveData& s) { s.progress.tips = 0; }

int shownCount(const SaveData& s) {
    int n = 0;
    for (int t = 0; t < kTipCount; ++t) n += trainer::tipSeen(s, t);
    return n;
}

}  // namespace ec::tips
