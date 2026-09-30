// 1.0's interface (workstream U): the Journal's tracked goal (core/guide: what can be tracked,
// what's tracked now, where in the valley it points) and the gentle tutorial's tips (core/tips:
// shown once each, short enough for their card, reset from the settings).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "check.hpp"
#include "core/clock.hpp"
#include "core/story.hpp"
#include "core/guide.hpp"
#include "core/league.hpp"
#include "core/place_layout.hpp"
#include "core/save.hpp"
#include "core/tips.hpp"
#include "core/valley.hpp"
#include "core/villagers.hpp"
#include "core/world.hpp"

using namespace ec;

namespace {

const Valley& valley() {
    static Valley v;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        FILE* f = std::fopen("../romfs/valley/skyreach.evl", "rb");
        if (!f) return v;
        std::vector<u8> data;
        u8 buf[4096];
        std::size_t n;
        while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) data.insert(data.end(), buf, buf + n);
        std::fclose(f);
        loadValley(data.data(), data.size(), v);
    }
    return v;
}

float distTo(Vec2 a, Vec3 b) { return std::hypot(a.x - b.x, a.y - b.y); }

Vec3 placeAt(const Valley& v, int place) {
    const ValleyPlaceInfo* p = v.place(static_cast<u8>(place));
    return p ? p->at : Vec3{};
}

}  // namespace

// What's tracked: the quest in hand until something is picked; a pick holds while it's open and
// lets go (back to the quest in hand) when it's done or tapped again. (The quests begin as the story
// says: core/story, D137.)
TEST(guide_current_and_picking) {
    constexpr s64 now = 20000 * kDay + 12 * kHour;
    static SaveData s;
    s = SaveData{};
    world::startWorld(s);
    CHECK(guide::current(s).kind == Tracked::None);  // nothing begun yet
    story::startQuest(s, story::kQKeepersApprentice);
    guide::Goal g = guide::current(s);
    CHECK(g.kind == Tracked::Quest && g.id == story::kQKeepersApprentice && !guide::picked(s, g));
    guide::Goal list[8];
    CHECK(guide::trackables(s, list, 8) == 1 && list[0] == g);
    story::finishQuest(s, story::kQKeepersApprentice, now);
    story::startQuest(s, story::kQHilltop);
    story::startQuest(s, story::kQMeadow);
    story::startQuest(s, story::kQColdHeights);
    const int n = guide::trackables(s, list, 8);
    CHECK(n == 3 && list[0].id == story::kQHilltop && list[1].id == story::kQMeadow && list[2].id == story::kQColdHeights);
    CHECK(guide::current(s).id == story::kQHilltop);  // the earliest begun
    guide::toggle(s, list[2]);                         // the cold heights, picked
    CHECK(guide::picked(s, list[2]) && guide::current(s) == list[2]);
    guide::toggle(s, list[2]);                         // tapped again: back to the quest in hand
    CHECK(!guide::picked(s, list[2]) && guide::current(s).id == story::kQHilltop);
    guide::toggle(s, list[1]);
    story::finishQuest(s, story::kQMeadow, now);       // its quest done: the pick lets go by itself
    CHECK(!guide::open(s, list[1]) && guide::current(s).id == story::kQHilltop);
    // The boards and the Hollow join the list once their places are found, and places track too.
    world::findPlace(s, kPlaceArena);
    world::findPlace(s, kPlaceHollow);
    const int m = guide::trackables(s, list, 8);
    CHECK(list[m - 2].kind == Tracked::BattleBoard && list[m - 1].kind == Tracked::Hollow);
    s.progress.battleLeague = kLeagues;  // every league won: nothing left on its board
    CHECK(!guide::open(s, {Tracked::BattleBoard, 0}));
    guide::toggle(s, {Tracked::Place, kPlaceLake});
    CHECK(guide::current(s).kind == Tracked::Place && guide::current(s).id == kPlaceLake);
    CHECK(!guide::open(s, {Tracked::Place, 99}) && !guide::open(s, {Tracked::Quest, 63}));
    CHECK(guide::trackables(s, list, 1) == 1);  // never past the room given
}

// Where the goals point, on the real valley: the keeper, the den's lantern, a place not found yet
// as a search area round it (not centred on it), Fig's woods, the stray's meadow, the orchard's
// board, the nearest dark lantern, the nearest of Fig's pages; every step of every quest somewhere.
TEST(guide_targets_in_the_valley) {
    const Valley& v = valley();
    CHECK(v.n > 0);
    if (v.n == 0) return;
    constexpr s64 now = 20000 * kDay + 12 * kHour;
    static SaveData s;
    s = SaveData{};
    world::startWorld(s);
    const Vec2 den{placeAt(v, kPlaceDen).x, placeAt(v, kPlaceDen).y};
    story::startQuest(s, story::kQKeepersApprentice);
    guide::Target t = guide::target(s, v, guide::current(s), den, now);  // meet the keeper: where he stands
    const VillagerInfo& keeper = villagerInfo(Villager::Keeper);
    CHECK(t.valid && t.place == keeper.place && distTo(t.at, placeAt(v, kPlaceKeeper)) < 15);
    s.story.quest[story::kQKeepersApprentice] = 2;
    t = guide::target(s, v, guide::current(s), den, now);  // the den's lantern
    const PlaceLayout& dl = placeLayout(kPlaceDen);
    CHECK(t.valid && !t.area && dl.hasLantern);
    CHECK(std::hypot(t.at.x - placeToWorld(*v.place(kPlaceDen), {dl.lantern.x, dl.lantern.y}).x,
                     t.at.y - placeToWorld(*v.place(kPlaceDen), {dl.lantern.x, dl.lantern.y}).y) < 0.01f);
    story::finishQuest(s, story::kQKeepersApprentice, now);
    story::startQuest(s, story::kQHilltop);
    t = guide::target(s, v, {Tracked::Quest, story::kQHilltop}, den, now);  // find the Nesting Stone: a search area
    CHECK(t.valid && t.area && t.place == kPlaceStone && t.radius >= 28);
    const float off = distTo(t.at, placeAt(v, kPlaceStone));
    CHECK(off > 1.0f && off < t.radius);  // it's in the circle, not at its middle
    story::startQuest(s, story::kQMarketDay);
    s.story.quest[story::kQMarketDay] = 2;
    t = guide::target(s, v, {Tracked::Quest, story::kQMarketDay}, den, now);  // Fig in the Whisperwood: a soft circle
    CHECK(t.valid && t.area && t.place == kPlaceMarket && t.radius > 10);
    s.story.quest[story::kQMarketDay] = 3;
    t = guide::target(s, v, {Tracked::Quest, story::kQMarketDay}, den, now);  // a Fruit Catch: the orchard's board
    CHECK(t.valid && !t.area && t.place == kPlaceOrchard && distTo(t.at, placeAt(v, kPlaceOrchard)) < 20);
    story::startQuest(s, story::kQMeadow);
    s.story.quest[story::kQMeadow] = 2;
    t = guide::target(s, v, {Tracked::Quest, story::kQMeadow}, den, now);  // the meadow's stray
    CHECK(t.valid && t.area && t.place == kPlaceSanctuary && t.radius > 10);
    story::startQuest(s, story::kQWings);  // growing up has no place: nothing to point at
    CHECK(!guide::target(s, v, {Tracked::Quest, story::kQWings}, den, now).valid);
    // The festival's lanterns: the nearest one still dark, from where you stand.
    story::startQuest(s, story::kQLanternFestival);
    for (int p = 0; p < kPlaceCount; ++p) world::findPlace(s, p);
    const Vec3 stone = placeAt(v, kPlaceStone);
    t = guide::target(s, v, {Tracked::Quest, story::kQLanternFestival}, {stone.x, stone.y}, now);
    CHECK(t.valid && t.place == kPlaceStone);
    world::lightLantern(s, kPlaceStone);
    t = guide::target(s, v, {Tracked::Quest, story::kQLanternFestival}, {stone.x, stone.y}, now);
    CHECK(t.valid && t.place != kPlaceStone && t.place != kPlaceArena);
    // Fig's pages: the nearest one not picked up yet.
    story::startQuest(s, story::kQFigMap);
    const Vec3 mill = placeAt(v, kPlaceMill);
    t = guide::target(s, v, {Tracked::Quest, story::kQFigMap}, {mill.x, mill.y}, now);
    CHECK(t.valid && t.place == kPlaceMill);
    // A place tracked from the Journal's places: its door once found.
    t = guide::target(s, v, {Tracked::Place, kPlaceTrailhead}, den, now);
    CHECK(t.valid && !t.area && t.place == kPlaceTrailhead);
    // The league's board: the challenger to battle next, where they stand.
    const int next = league::nextChallenger(s);
    t = guide::target(s, v, {Tracked::BattleBoard, 0}, den, now);
    CHECK(t.valid && t.place == league::challenger(next).place);
    const Vec2 stands = placeToWorld(*v.place(league::challenger(next).place), league::spotOf(next));
    CHECK(std::hypot(t.at.x - stands.x, t.at.y - stands.y) < 0.5f);
    CHECK(!guide::target(s, v, {}, den, now).valid);
    // Every quest's every step points somewhere sensible (or nowhere).
    for (int q = 0; q < story::questCount(); ++q) {
        const int steps = story::view(s, q, now).stepCount;
        for (int st = 1; st <= steps; ++st) {
            s.story.quest[q] = static_cast<u8>(st);
            const guide::Target a = guide::target(s, v, {Tracked::Quest, q}, den, now);
            if (!a.valid) continue;
            CHECK(v.inside(a.at.x, a.at.y) && a.place >= 0 && a.place < kPlaceCount);
            CHECK(!a.area || (a.radius > 5 && a.radius < 80));
        }
    }
}

// The tips: each shown once (a bit in the save), short enough for the card, all again from the
// settings.
TEST(tips_table_and_seen) {
    CHECK(tips::kTipCount <= tips::kMaxTips);
    for (int t = 0; t < tips::kTipCount; ++t) {
        const tips::TipInfo& i = tips::info(t);
        CHECK(i.title[0] && std::strlen(i.title) <= static_cast<std::size_t>(tips::kTitleChars));
        int lines = 1, run = 0, longest = 0;
        for (const char* c = i.text; *c; ++c) {
            if (*c == '\n') {
                ++lines;
                run = 0;
            } else if (++run > longest) {
                longest = run;
            }
        }
        CHECK(lines <= 2 && longest <= tips::kLineChars && longest > 0);
        if (lines > 2 || longest > tips::kLineChars) std::printf("  tip %d too long: %s\n", t, i.text);
        for (int u = 0; u < t; ++u) CHECK(std::strcmp(tips::info(u).title, i.title) != 0);
    }
    static SaveData s;
    s = SaveData{};
    CHECK(tips::due(s, tips::kTipValley) && tips::shownCount(s) == 0);
    trainer::markTip(s, tips::kTipValley);
    trainer::markTip(s, tips::kTipEnergy);
    CHECK(!tips::due(s, tips::kTipValley) && tips::due(s, tips::kTipRide) && tips::shownCount(s) == 2);
    CHECK(!tips::due(s, tips::kTipCount) && !tips::due(s, -1));
    tips::reset(s);
    CHECK(tips::due(s, tips::kTipValley) && tips::shownCount(s) == 0);
}

void runInterfaceTests() {
    RUN(guide_current_and_picking);
    RUN(guide_targets_in_the_valley);
    RUN(tips_table_and_seen);
}
