// 1.0's interface (workstream U): the Journal's tracked goal (core/guide: what can be tracked,
// what's tracked now, where in the valley it points) and the gentle tutorial's tips (core/tips:
// shown once each, short enough for their card, reset from the settings).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "check.hpp"
#include "core/campaign.hpp"
#include "core/guide.hpp"
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
// lets go (back to the quest in hand) when it's done or tapped again.
TEST(guide_current_and_picking) {
    static SaveData s;
    s = SaveData{};
    world::startWorld(s);
    campaign::update(s);
    guide::Goal g = guide::current(s);
    CHECK(g.kind == Tracked::Quest && g.id == 0 && !guide::picked(s, g));
    guide::Goal list[8];
    CHECK(guide::trackables(s, list, 8) == 1 && list[0] == g);
    // Quest 0 done: three quests begin (Market day, then its followers after it).
    s.world.flags |= kFlagEnteredValley | kFlagMetKeeper;
    world::lightLantern(s, kPlaceDen);
    campaign::update(s);
    CHECK(guide::current(s).id == 1);
    world::findPlace(s, kPlaceMarket);
    s.world.cups[static_cast<int>(Challenge::FruitCatch)] = 1;
    world::lightLantern(s, kPlaceMarket);
    campaign::update(s);
    const int n = guide::trackables(s, list, 8);
    CHECK(n == 3 && list[0].id == 2 && list[1].id == 3 && list[2].id == 4);
    CHECK(guide::current(s).id == 2);  // the earliest begun
    guide::toggle(s, list[2]);         // the cold heights, picked
    CHECK(guide::picked(s, list[2]) && guide::current(s) == list[2]);
    guide::toggle(s, list[2]);         // tapped again: back to the quest in hand
    CHECK(!guide::picked(s, list[2]) && guide::current(s).id == 2);
    guide::toggle(s, list[1]);
    // Its quest done: the pick lets go by itself.
    s.world.flags |= kFlagFoundStray;
    world::findPlace(s, kPlaceSanctuary);
    world::lightLantern(s, kPlaceSanctuary);
    campaign::update(s);
    CHECK(!guide::open(s, list[1]) && guide::current(s).id == 2);
    // The boards and the Hollow join the list once their places are found, and places track too.
    world::findPlace(s, kPlaceArena);
    world::findPlace(s, kPlaceHollow);
    const int m = guide::trackables(s, list, 8);
    CHECK(list[m - 2].kind == Tracked::BattleBoard && list[m - 1].kind == Tracked::Hollow);
    s.progress.battleLeague = kLeagues;  // every league won: nothing left on its board
    CHECK(!guide::open(s, {Tracked::BattleBoard, 0}));
    guide::toggle(s, {Tracked::Place, kPlaceLake});
    CHECK(guide::current(s).kind == Tracked::Place && guide::current(s).id == kPlaceLake);
    CHECK(!guide::open(s, {Tracked::Place, 99}) && !guide::open(s, {Tracked::Quest, 42}));
    CHECK(guide::trackables(s, list, 1) == 1);  // never past the room given
}

// Where the goals point, on the real valley: the keeper, the den's lantern, a place not found yet
// as a search area round it (not centred on it), the stray's meadow, the orchard's board, the
// nearest dark lantern.
TEST(guide_targets_in_the_valley) {
    const Valley& v = valley();
    CHECK(v.n > 0);
    if (v.n == 0) return;
    static SaveData s;
    s = SaveData{};
    world::startWorld(s);
    campaign::update(s);
    const Vec2 den{placeAt(v, kPlaceDen).x, placeAt(v, kPlaceDen).y};
    guide::Target t = guide::target(s, v, guide::current(s), den);  // head out: the den's door
    CHECK(t.valid && !t.area && t.place == kPlaceDen && distTo(t.at, placeAt(v, kPlaceDen)) < 30);
    s.world.flags |= kFlagEnteredValley;
    campaign::update(s);
    t = guide::target(s, v, guide::current(s), den);  // meet the keeper: where he stands
    const VillagerInfo& keeper = villagerInfo(Villager::Keeper);
    CHECK(t.valid && t.place == keeper.place && distTo(t.at, placeAt(v, kPlaceKeeper)) < 15);
    s.world.flags |= kFlagMetKeeper;
    campaign::update(s);
    t = guide::target(s, v, guide::current(s), den);  // the den's lantern
    const PlaceLayout& dl = placeLayout(kPlaceDen);
    CHECK(t.valid && !t.area && dl.hasLantern);
    CHECK(std::hypot(t.at.x - placeToWorld(*v.place(kPlaceDen), {dl.lantern.x, dl.lantern.y}).x,
                     t.at.y - placeToWorld(*v.place(kPlaceDen), {dl.lantern.x, dl.lantern.y}).y) < 0.01f);
    world::lightLantern(s, kPlaceDen);
    campaign::update(s);
    t = guide::target(s, v, guide::current(s), den);  // find the Market: a search area
    CHECK(t.valid && t.area && t.place == kPlaceMarket && t.radius >= 28);
    const float off = distTo(t.at, placeAt(v, kPlaceMarket));
    CHECK(off > 1.0f && off < t.radius);  // it's in the circle, not at its middle
    world::findPlace(s, kPlaceMarket);
    campaign::update(s);
    t = guide::target(s, v, guide::current(s), den);  // win a Fruit Catch: the orchard's board
    CHECK(t.valid && !t.area && t.place == kPlaceOrchard && distTo(t.at, placeAt(v, kPlaceOrchard)) < 20);
    // The meadow's stray: a soft circle in the flowers.
    s.world.quest[3] = 2;
    t = guide::target(s, v, {Tracked::Quest, 3}, den);
    CHECK(t.valid && t.area && t.place == kPlaceSanctuary && t.radius > 10);
    // Growing up has no place: nothing to point at.
    s.world.quest[5] = 1;
    CHECK(!guide::target(s, v, {Tracked::Quest, 5}, den).valid);
    // The festival's lanterns: the nearest one still dark, from where you stand.
    s.world.quest[7] = 1;
    for (int p = 0; p < kPlaceCount; ++p) world::findPlace(s, p);
    const Vec3 stone = placeAt(v, kPlaceStone);
    t = guide::target(s, v, {Tracked::Quest, 7}, {stone.x, stone.y});
    CHECK(t.valid && t.place == kPlaceStone);
    world::lightLantern(s, kPlaceStone);
    t = guide::target(s, v, {Tracked::Quest, 7}, {stone.x, stone.y});
    CHECK(t.valid && t.place != kPlaceStone && t.place != kPlaceArena);
    // A place tracked from the Journal's places: its door once found.
    t = guide::target(s, v, {Tracked::Place, kPlaceTrailhead}, den);
    CHECK(t.valid && !t.area && t.place == kPlaceTrailhead);
    CHECK(!guide::target(s, v, {}, den).valid);
    // Every quest's every step points somewhere sensible (or nowhere, for growing up).
    for (int q = 0; q < campaign::questCount(); ++q)
        for (int st = 1; st <= 3; ++st) {
            s.world.quest[q] = static_cast<u8>(st);
            const guide::Target a = guide::target(s, v, {Tracked::Quest, q}, den);
            if (!a.valid) continue;
            CHECK(v.inside(a.at.x, a.at.y) && a.place >= 0 && a.place < kPlaceCount);
            CHECK(!a.area || (a.radius > 5 && a.radius < 80));
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
