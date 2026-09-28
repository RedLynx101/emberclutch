#include "core/league.hpp"

#include <cmath>
#include <cstdio>

#include "core/care.hpp"
#include "core/kinds.hpp"
#include "core/trainer.hpp"
#include "core/valley.hpp"
#include "core/wanderings.hpp"

namespace ec::league {
namespace {

constexpr float kTowardMiddle = 99.0f;  // a facing: toward the place's anchor

// Skins, hair and eyes the people kit uses (tools/people/looks.py), and a few of our own.
constexpr Rgb kPeach{255, 222, 196}, kHoney{240, 192, 152}, kSand{212, 158, 112}, kUmber{164, 108, 72},
    kCocoa{112, 70, 48}, kFair{255, 232, 214};
constexpr Rgb kChestnut{132, 78, 44}, kDarkBrown{82, 54, 40}, kBlack{46, 40, 46}, kGinger{208, 104, 50},
    kGolden{236, 194, 104}, kSilver{220, 222, 232};
constexpr Rgb kBrownEyes{86, 56, 40}, kBlueEyes{70, 118, 176}, kGreenEyes{76, 128, 78}, kGreyEyes{118, 110, 124};

// The four leagues: Ember (the valley's young keepers, levels 3-8), Flame (10-16), Blaze (18-26)
// and Starfire (28-38), each with its champion at the caldera, a little stronger. Their dragons
// train with them (more trained points a league) and choose more cleverly (skill).
const Challenger kTable[kChallengers] = {
    // ---------------------------------------------------------------- Ember
    {"Tamsin", "Ember league", kPlaceMarket, {-4.9f, 18.4f}, kTowardMiddle,
     {1, 3, kPeach, kGinger, {236, 140, 60}, {255, 236, 170}, {126, 84, 54}, kBlueEyes}, 1, 1.75f,
     {"Oh! You're {P}, the new keeper? I'm Tamsin, and this is Pepper!",
      "We've practised all week. Want to battle? Just a friendly one!"},
     "Pepper wants another go! Ready?",
     "Aww, we lost! {D} is really good. Pepper, we'll practise more!",
     "We won! Don't be sad, {D} did great. Come back anytime!",
     "Pepper", "pouncer", 1, 3, 0, 0},
    {"Oren", "Ember league", kPlaceTrailhead, {2.3f, 8.7f}, kTowardMiddle,
     {0, 0, kSand, kDarkBrown, {126, 152, 76}, {236, 210, 150}, {96, 70, 48}, kBrownEyes}, 1, 1.7f,
     {"I'm Oren. Moss here would rather nap than battle...",
      "...but when he's awake, watch out! Let's see what you've got, {P}."},
     "Moss is awake. That doesn't happen often. Battle?",
     "Ha! You two are good. Moss, you can go back to sleep now.",
     "Moss did it! Now he'll want a nap for a week.",
     "Moss", "puffback", 0, 4, 0, 0},
    {"Juniper", "Ember league", kPlaceSanctuary, {3.4f, 12.6f}, kTowardMiddle,
     {1, 2, kHoney, kGolden, {150, 190, 110}, {246, 170, 190}, {140, 96, 60}, kGreenEyes}, 1, 1.7f,
     {"Pebble curls up when she's scared, but she's braver than she looks.",
      "I'm Juniper! Will you battle us? Pebble's been so looking forward to it."},
     "Pebble's uncurled and ready. One more?",
     "You won! Pebble, it's all right, you can come out now.",
     "We did it, Pebble! See? Brave as a boulder.",
     "Pebble", "curlstone", 1, 6, 0, 0},
    {"Fen", "Ember league", kPlaceOrchard, {4.0f, 8.0f}, kTowardMiddle,
     {0, 4, kUmber, kBlack, {92, 146, 198}, {246, 236, 210}, {80, 64, 56}, kBrownEyes}, 0, 1.5f,
     {"Fen's the name. I pick apples, Ripple splashes the trees. Teamwork!",
      "Beat us and the Ember champion will want to meet you. Ready?"},
     "Back for more? Ripple's all splashed up.",
     "Soaked and beaten! Go on, the champion waits at the caldera.",
     "Splash! Better luck next time, {P}.",
     "Ripple", "ribbontail", 0, 8, 0, 0},
    {"Marigold", "the Ember champion", kPlaceCaldera, {0.0f, 0.0f}, 0.0f,
     {1, 5, kPeach, {220, 120, 40}, {214, 90, 56}, {245, 196, 81}, {110, 60, 40}, kGreyEyes}, 1, 1.5f,
     {"So you beat all four of the Ember league. Sunny and I have been watching.",
      "Welcome to the caldera, {P}. Show me the fire in you and {D}!"},
     "A rematch at the caldera? Sunny never says no.",
     "What a battle! The Ember title is yours, and {D}'s. The Flame league will want you now.",
     "Not today! Train a little more and come back to the ring.",
     "Sunny", "blazeplume", 0, 10, 2, 1},
    // ---------------------------------------------------------------- Flame
    {"Hazel", "Flame league", kPlaceKeeper, {-5.5f, 9.5f}, kTowardMiddle,
     {1, 1, kSand, kChestnut, {110, 170, 190}, {240, 238, 232}, {100, 80, 60}, kGreenEyes}, 1, 1.6f,
     {"I'm Hazel, Rowan's apprentice. Whisper and I learned from the best.",
      "The Flame league doesn't go easy. Shall we?"},
     "Whisper's feathers are all fluffed up. Again?",
     "Well flown! I'll tell Rowan you're the real thing.",
     "Whisper wins! Rowan would say: patience, then try again.",
     "Whisper", "crestwing", 1, 10, 2, 1},
    {"Barnaby", "Flame league", kPlaceMill, {12.0f, 8.0f}, kTowardMiddle,
     {0, 0, kHoney, kSilver, {176, 140, 90}, {236, 228, 210}, {90, 66, 48}, kBlueEyes}, 0, 1.2f,
     {"Barnaby. I keep the mill. Cobble keeps the millstones warm.",
      "Slow and steady, that's us. Let's see if steady wins, eh?"},
     "The wheel's turning, and so are we. Another round?",
     "Ground to flour! You've a fine dragon there, {P}.",
     "Steady wins! Come back when the wheel turns your way.",
     "Cobble", "cindershell", 0, 12, 2, 1},
    {"Isolde", "Flame league", kPlaceVault, {-4.0f, 8.0f}, kTowardMiddle,
     {1, 5, kFair, {236, 236, 240}, {160, 190, 230}, {250, 250, 255}, {120, 130, 160}, kBlueEyes}, 1, 1.45f,
     {"I came from the far north with Snowdrop. The cold here feels like home.",
      "They say you're good, {P}. Snowdrop would like to see for herself."},
     "Snowdrop's breath is frosty today. Once more?",
     "The ice melts... you've won. Snowdrop, we'll try again.",
     "Snowdrop's frost holds. Warm up and come back.",
     "Snowdrop", "flurrytail", 0, 14, 2, 1},
    {"Pim", "Flame league", kPlaceArena, {7.0f, 4.0f}, kTowardMiddle,
     {0, 4, kCocoa, kBlack, {230, 190, 70}, {80, 120, 170}, {70, 56, 50}, kBrownEyes}, 1, 1.8f,
     {"I'm Pim! I've worked out every move Fernling knows. On paper.",
      "Now let's try it for real! Beat us and it's the Flame champion next."},
     "I've rewritten my notes! Rematch?",
     "My notes were wrong! Go on, the champion's at the caldera.",
     "It worked! The notes were right! Um, well battled, {P}.",
     "Fernling", "lilyfin", 0, 16, 2, 1},
    {"Captain Rook", "the Flame champion", kPlaceCaldera, {0.0f, 0.0f}, 0.0f,
     {0, 0, kUmber, kBlack, {60, 70, 110}, {245, 196, 81}, {40, 32, 36}, kBrownEyes}, 0, 1.25f,
     {"Captain Rook, at your service. Bramble and I sailed the lake before the festival.",
      "Four Flame challengers beaten! Let's see how {D} does against a captain."},
     "Back at the caldera! Bramble's smoking with excitement.",
     "Well fought! The Flame title is yours. The Blaze league awaits you, keeper.",
     "The captain holds the ring! Come back when you're ready.",
     "Bramble", "kindlemoss", 1, 18, 4, 2},
    // ---------------------------------------------------------------- Blaze
    {"Silas", "Blaze league", kPlaceSanctuary, {-6.5f, 11.3f}, kTowardMiddle,
     {0, 5, {206, 150, 112}, {150, 92, 50}, {140, 80, 150}, {236, 200, 110}, {90, 60, 50}, kGreenEyes}, 0, 1.4f,
     {"A song for every battle, and Clover dances to all of them.",
      "I'm Silas. Shall we make a new song, {P}? The Blaze league kind."},
     "Clover's humming. That means she wants another dance.",
     "That's a song I'll sing for years. Well won!",
     "Clover dances on! A sad verse for you, but a short one.",
     "Clover", "bloomstone", 1, 18, 4, 2},
    {"Ondine", "Blaze league", kPlaceMill, {-12.0f, 8.0f}, kTowardMiddle,
     {1, 2, kUmber, {40, 60, 90}, {60, 140, 180}, {220, 240, 240}, {60, 80, 90}, kBlueEyes}, 1, 1.55f,
     {"Undertow and I swim the river every morning, right up to the falls.",
      "I'm Ondine. The Blaze league is where the current gets strong. Dive in?"},
     "The river's high today. So is Undertow. Again?",
     "Swept away! You're a strong swimmer, {P}.",
     "The current wins! Come back when the water's calmer.",
     "Undertow", "ribbontail", 2, 21, 4, 2},
    {"Thistle", "Blaze league", kPlaceTrailhead, {6.4f, 6.4f}, kTowardMiddle,
     {1, 4, kHoney, {140, 90, 160}, {110, 130, 100}, {200, 110, 70}, {80, 60, 50}, kGreyEyes}, 1, 1.65f,
     {"Thistle. I climb the cold heights, and Hailstone climbs with me.",
      "Up there you learn to be tough. Let's see how tough you are!"},
     "Hailstone's ready for another climb. Are you?",
     "Knocked off the mountain! That was a great battle.",
     "We reached the top! Keep climbing, {P}.",
     "Hailstone", "frostcurl", 0, 23, 4, 2},
    {"Corwin", "Blaze league", kPlaceKeeper, {6.0f, 10.0f}, kTowardMiddle,
     {0, 1, kPeach, kGolden, {170, 170, 180}, {180, 50, 60}, {70, 70, 80}, kBlueEyes}, 0, 1.5f,
     {"Sir Corwin! Well, nearly a sir. Ashwing and I are knights in training.",
      "Beat us and the Blaze champion will see you. En garde, {P}!"},
     "En garde again! Ashwing insists.",
     "A true champion's blow! To the caldera with you, brave keeper.",
     "Victory for the knights! Train hard and come back.",
     "Ashwing", "blazeplume", 2, 26, 4, 2},
    {"Seraphine", "the Blaze champion", kPlaceCaldera, {0.0f, 0.0f}, 0.0f,
     {1, 5, kSand, {30, 24, 36}, {120, 40, 70}, {230, 180, 90}, {40, 30, 40}, kGreyEyes}, 1, 1.4f,
     {"The Blaze league, beaten. Nightshade and I rarely see anyone get this far.",
      "The caldera glows for you, {P}. Let's make it blaze."},
     "The ring is lit again. Nightshade is waiting.",
     "Magnificent. The Blaze title is yours. Only the Starfire league remains.",
     "The night falls on you this time. Come back stronger.",
     "Nightshade", "duskwing", 1, 28, 6, 3},
    // ---------------------------------------------------------------- Starfire
    {"Aurelia", "Starfire league", kPlaceMarket, {4.9f, 18.4f}, kTowardMiddle,
     {1, 3, kPeach, kGolden, {40, 50, 100}, {250, 236, 160}, {60, 60, 90}, kBlueEyes}, 1, 1.6f,
     {"I map the stars, and Moonpetal glows like one. The Starfire league is the last, you know.",
      "I'm Aurelia. Let's see if your star shines as bright."},
     "The stars are out. Moonpetal is glowing. Another?",
     "Your star is brighter tonight. Well done, {P}.",
     "Moonpetal shines on! The stars will wait for you.",
     "Moonpetal", "puffback", 3, 28, 6, 3},
    {"Bastian", "Starfire league", kPlaceOrchard, {8.0f, 5.0f}, kTowardMiddle,
     {0, 0, {206, 150, 112}, kSilver, {120, 90, 60}, {200, 160, 80}, {70, 50, 40}, kBrownEyes}, 0, 1.2f,
     {"I planted this orchard sixty years ago. Old Smoky helped, mostly by napping.",
      "I'm Bastian. Old roots, strong branches. Care to test them?"},
     "The old roots are still strong. Once more?",
     "You've grown tall, young keeper. The orchard tips its branches to you.",
     "Old roots hold! Come back when you've grown.",
     "Old Smoky", "cindershell", 2, 31, 6, 3},
    {"Nyx", "Starfire league", kPlaceVault, {2.0f, 8.0f}, kTowardMiddle,
     {1, 1, {240, 220, 230}, {60, 40, 80}, {70, 50, 100}, {180, 160, 220}, {40, 30, 50}, {150, 120, 190}}, 1, 1.35f,
     {"Shh. Umbra and I keep the secrets of the cold, dark places.",
      "I'm Nyx. Few see Umbra coming. Will you?"},
     "Umbra's in the shadows again. Can you find her?",
     "You saw us coming. Impressive, {P}.",
     "Gone into the dark! Come back when your eyes are sharper.",
     "Umbra", "duskwing", 2, 34, 6, 3},
    {"Lark", "Starfire league", kPlaceArena, {-7.0f, 4.0f}, kTowardMiddle,
     {0, 4, kSand, {220, 120, 40}, {240, 240, 245}, {90, 170, 220}, {90, 90, 110}, kGreenEyes}, 0, 1.45f,
     {"Lark, sky racer! Zephyr's the fastest thing in the valley. Ask anyone!",
      "Beat us and it's the Starfire champion. Nobody's beaten Zephyr, though!"},
     "Zephyr wants a rematch! Faster this time!",
     "Nobody beats Zephyr! ...Except you. The champion's at the caldera!",
     "Too fast! Zephyr's still the fastest in the valley!",
     "Zephyr", "crestwing", 3, 38, 6, 3},
    {"Solenne", "the Starfire champion", kPlaceCaldera, {0.0f, 0.0f}, 0.0f,
     {1, 5, kUmber, {236, 236, 244}, {250, 246, 236}, {196, 222, 250}, {200, 190, 170}, {240, 196, 72}}, 1, 1.4f,
     {"The whole valley has been talking about you, {P}. And about {D}.",
      "Starlight came down with the star dragon, long ago. Show us your brightest!"},
     "The ring is ready, and so is Starlight. One more?",
     "Starlight dims... You are the Starfire champions now, you and {D}. The valley cheers!",
     "Starlight shines on. Come back, {P}; you're so close.",
     "Starlight", "glimmermoth", 3, 40, 6, 3},
};

bool valid(int id) { return id >= 0 && id < kChallengers; }

// Gleam: a first win pays well, a rematch less (once a day); a final pays more than both.
constexpr u32 kFirstWin[kLeagues] = {40, 60, 90, 130}, kAgainWin[kLeagues] = {15, 25, 35, 50};
constexpr u32 kFinalFirst[kLeagues] = {150, 250, 400, 600}, kFinalAgain[kLeagues] = {50, 80, 120, 160};

}  // namespace

int idOf(int league, int slot) {
    if (league < 0) league = 0;
    if (league >= kLeagues) league = kLeagues - 1;
    if (slot < 0) slot = 0;
    if (slot >= kSlots) slot = kSlots - 1;
    return league * kSlots + slot;
}
int leagueOf(int id) { return valid(id) ? id / kSlots : 0; }
int slotOf(int id) { return valid(id) ? id % kSlots : 0; }
bool isChampion(int id) { return valid(id) && slotOf(id) == kChampion; }

const Challenger& challenger(int id) { return kTable[valid(id) ? id : 0]; }

void palette(const Look& look, Rgb out[kPalCount]) {
    // The player bodies' fixed colours (tools/people/looks.py), then theirs.
    static constexpr Rgb kFixed[kPalCount] = {{255, 222, 196}, {126, 152, 76}, {216, 102, 60}, {132, 78, 44}, {126, 84, 54},
                                               {86, 56, 40},    {32, 24, 30},   {255, 255, 255}, {255, 206, 120}, {236, 124, 124}};
    for (int k = 0; k < kPalCount; ++k) out[k] = kFixed[k];
    out[kPalBase] = look.skin;
    out[kPalHorn] = look.hairColour;
    out[kPalAccent] = look.outfit;
    out[kPalPattern] = look.trim;
    out[kPalMembrane] = look.boots;
    out[kPalIris] = look.eyes;
}

Dragon dragonOf(int id) {
    const Challenger& c = challenger(id);
    int kind = findKind(c.kind);
    if (kind < 0) kind = 0;
    Rng rng(0xC4A11E00u + static_cast<u32>(id) * 7919u);
    Dragon d;
    d.id = 0xB0000000u + static_cast<u32>(id);  // apart from the save's (the renderer's caches go by id)
    d.stage = Stage::Adult;
    rollKind(d, kind, c.variant, rng);
    d.genome.build = static_cast<u8>(rng.below(3));
    d.genome.size = static_cast<u8>(rng.below(256));
    d.xp = trainer::xpForLevel(c.level);
    for (u8& t : d.trained) t = c.trained;
    std::snprintf(d.name, sizeof(d.name), "%s", c.dragonName);
    d.needs = Needs{};
    d.bond = 600;
    return d;
}

Vec2 spotOf(int id) { return isChampion(id) ? ringSide(1) : challenger(id).at; }

float facingOf(int id) {
    const Challenger& c = challenger(id);
    if (c.facing != kTowardMiddle) return c.facing;
    const Vec2 at = spotOf(id);
    return std::atan2(at.x, -at.y);  // (0 faces the place's front, +Y: this faces its anchor)
}

// ---------------------------------------------------------------------------- progress
int currentLeague(const SaveData& s) { return s.progress.battleLeague < kLeagues ? s.progress.battleLeague : kLeagues - 1; }

bool leagueWon(const SaveData& s, int league) { return league >= 0 && league < kLeagues && s.progress.battleLeague > league; }

bool beaten(const SaveData& s, int id) {
    if (!valid(id)) return false;
    const int league = leagueOf(id);
    if (isChampion(id)) return leagueWon(s, league);
    return leagueWon(s, league) || ((s.progress.battleBeaten[league] >> slotOf(id)) & 1u);
}

int beatenCount(const SaveData& s, int league) {
    int n = 0;
    for (int slot = 0; slot < kPerLeague; ++slot) n += beaten(s, idOf(league, slot));
    return n;
}

bool championOpen(const SaveData& s, int league) { return beatenCount(s, league) == kPerLeague; }

bool standing(const SaveData& s, int id) { return valid(id) && leagueOf(id) == currentLeague(s); }

int nextChallenger(const SaveData& s) {
    if (trainer::tracked(s) == Tracked::BattleBoard && standing(s, s.progress.trackId) && !beaten(s, s.progress.trackId))
        return s.progress.trackId;
    const int league = currentLeague(s);
    for (int slot = 0; slot < kPerLeague; ++slot)
        if (!beaten(s, idOf(league, slot))) return idOf(league, slot);
    return idOf(league, kChampion);
}

// ---------------------------------------------------------------------------- after a battle
Reward record(SaveData& s, int dragonIndex, int id, battle::Outcome o, s32 today) {
    Reward r;
    if (!valid(id) || dragonIndex < 0 || dragonIndex >= s.dragonCount) return r;
    Dragon& d = s.dragons[dragonIndex];
    const Challenger& c = challenger(id);
    const int league = leagueOf(id);
    const bool champion = isChampion(id);
    r.growth = battle::grow(d, battle::battleXp(trainer::levelOf(d), c.level, o, champion ? 1.5f : 1.0f));
    if (o == battle::Outcome::GaveUp) return r;
    trainer::count(s, kCountBattles);
    const bool won = o == battle::Outcome::Won;
    trainer::recordBattle(d, won);
    if (!won) return r;
    r.firstWin = !beaten(s, id);
    const bool paid = !trainer::claimToday(s, kClaimBattle + id, today);  // (the day's bit, taken either way)
    if (r.firstWin) r.gleam = champion ? kFinalFirst[league] : kFirstWin[league];
    else if (!paid) r.gleam = champion ? kFinalAgain[league] : kAgainWin[league];
    r.paidBefore = !r.firstWin && paid;
    s.gleam += r.gleam;
    if (champion) {
        const int before = d.battleTitle;
        trainer::recordLeague(d, league + 1);
        r.title = d.battleTitle > before;
        if (r.firstWin) {  // the league is yours: the next one's challengers arrive, and a prize
            r.leagueWon = true;
            s.progress.battleLeague = static_cast<u8>(league + 1);
            static constexpr Food kFoods[kLeagues] = {Food::EmberCandy, Food::GlimmerCookie, Food::Starfruit, Food::Starfruit};
            static constexpr u8 kCounts[kLeagues] = {3, 3, 3, 5};
            static constexpr Trinket kTrinkets[kLeagues] = {Trinket::Crystal, Trinket::Pearl, Trinket::Fossil, Trinket::Pearl};
            r.prizeFood = static_cast<u8>(kFoods[league]);
            r.prizeCount = kCounts[league];
            r.prizeTrinket = static_cast<u8>(kTrinkets[league]);
            u16& pouch = s.pouch[static_cast<int>(kFoods[league])];
            pouch = static_cast<u16>(pouch + r.prizeCount > 999 ? 999 : pouch + r.prizeCount);
            u16& hoard = s.hoard[static_cast<int>(kTrinkets[league])];
            if (hoard < 0xFFFF) ++hoard;
        }
    } else if (r.firstWin) {
        s.progress.battleBeaten[league] = static_cast<u8>(s.progress.battleBeaten[league] | (1u << slotOf(id)));
        r.championOpened = championOpen(s, league);
    }
    return r;
}

// ---------------------------------------------------------------------------- where things are
// The caldera: the crater's floor is flat for ~33 m round its anchor; the way in arrives from +Y.
// The ring's middle a little toward the far wall, you on its near side, the champion the far.
Vec2 ringCentre() { return {0.0f, -4.0f}; }
Vec2 ringSide(int side) { return side == 0 ? Vec2{0.0f, 1.5f} : Vec2{0.0f, -9.5f}; }

int boardCount() { return 2; }
BoardSpot board(int i) {
    if (i == 1) return {kPlaceCaldera, {-7.0f, 9.0f}, 0.0f};  // by the way into the crater
    return {kPlaceArena, {7.8f, 19.2f}, 0.0f};                // outside the arena's gate, across from the challenges'
}

}  // namespace ec::league
