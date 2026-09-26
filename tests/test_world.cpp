// Beta's world (core/world) and the Lantern Festival (core/campaign): places found and lanterns
// lit; the quests follow the world in any order and never stick; the save keeps it all and an
// older save starts a fresh world with the den found.
#include <cstring>
#include <vector>

#include "check.hpp"
#include "core/campaign.hpp"
#include "core/items.hpp"
#include "core/market.hpp"
#include "core/save.hpp"
#include "core/valley.hpp"
#include "core/villagers.hpp"
#include "core/world.hpp"

using namespace ec;

TEST(places_and_lanterns) {
    static SaveData s;
    s = SaveData{};
    world::startWorld(s);
    CHECK(world::placeFound(s, kPlaceDen) && world::placesFound(s) == 1);
    CHECK(world::findPlace(s, kPlaceMarket) && !world::findPlace(s, kPlaceMarket) && world::placesFound(s) == 2);
    CHECK(world::placeCount() == kPlaceCount && world::lanternCount() == 8);
    CHECK(!world::lightLantern(s, kPlaceLake));  // no lantern there
    CHECK(world::lightLantern(s, kPlaceDen) && !world::lightLantern(s, kPlaceDen) && world::lanternsLit(s) == 1);
    for (int p = 0; p < world::placeCount(); ++p) CHECK(world::placeInfo(p).name[0] != 0 && world::placeInfo(p).findRadius > 0);
}

// The quests: the first begins at once; steps move on when the world allows, several at once;
// later quests wait for earlier ones; the festival needs every lantern and ends in the egg.
TEST(the_lantern_festival) {
    static SaveData s;
    s = SaveData{};
    world::startWorld(s);
    s.gleam = 0;
    campaign::News n = campaign::update(s);
    CHECK(n.started == 0 && campaign::currentQuest(s) == 0);
    CHECK(std::strcmp(campaign::view(s, 0).step, "Head out of the den with your dragon") == 0);
    CHECK(!campaign::view(s, 1).started);
    // Everything for quest 1 at once: it finishes in one update, and Market day begins.
    s.world.flags |= kFlagEnteredValley | kFlagMetKeeper;
    world::lightLantern(s, kPlaceDen);
    n = campaign::update(s);
    CHECK(n.finished == 0 && s.world.quest[0] == campaign::kQuestDone && n.gleam == 60 && s.gleam == 60);
    CHECK(campaign::view(s, 1).started && campaign::currentQuest(s) == 1);
    // Out of order: the Market's lantern lit before the Fruit Catch is won; the quest waits on the cup.
    world::findPlace(s, kPlaceMarket);
    world::lightLantern(s, kPlaceMarket);
    campaign::update(s);
    CHECK(campaign::view(s, 1).stepIndex == 1);
    s.world.cups[static_cast<int>(Challenge::FruitCatch)] = 1;
    n = campaign::update(s);
    CHECK(n.finished == 1 && campaign::view(s, 2).started && campaign::view(s, 3).started && campaign::view(s, 4).started);
    CHECK(!campaign::view(s, 7).started);  // the festival waits for the rest
    // Every other quest done, every lantern lit but the arena's: the festival, then the egg.
    for (int p = 0; p < kPlaceCount; ++p)
        if (world::placeInfo(p).lantern && p != kPlaceArena) world::lightLantern(s, p);
    for (int p = 0; p < kPlaceCount; ++p) world::findPlace(s, p);
    s.world.flags |= kFlagHeardStory | kFlagFoundStray | kFlagGlided | kFlagRode | kFlagMetTraveller | kFlagWandered;
    s.world.cups[static_cast<int>(Challenge::SkyRings)] = 1;
    campaign::update(s);
    for (int q = 0; q < 7; ++q) CHECK(s.world.quest[q] == campaign::kQuestDone);
    CHECK(campaign::view(s, 7).stepIndex == 1);  // all lanterns: on to the trial
    s.world.cups[static_cast<int>(Challenge::LanternTrial)] = 2;
    s.world.flags |= kFlagFestival;
    n = campaign::update(s);
    CHECK(n.finished == 7 && n.starEgg && campaign::currentQuest(s) == -1);
    CHECK(!campaign::update(s).starEgg);  // once
}

TEST(the_world_saves) {
    static SaveData s;
    s = SaveData{};
    world::startWorld(s);
    s.world.look[kLookHair] = 4;
    s.world.lookMade = 1;
    s.world.x = 120.5f;
    s.world.y = -40.25f;
    s.world.heading = 1.5f;
    s.world.inValley = 1;
    s.world.partnerId = 7;
    world::findPlace(s, kPlaceIsles);
    world::lightLantern(s, kPlaceStone);
    s.world.quest[2] = 2;
    s.world.cups[1] = 3;
    s.world.flags = kFlagMetKeeper | kFlagGlided;
    s.world.ribbons = 5;
    std::vector<u8> buf(maxEncodedSize());
    const std::size_t n = encodeSave(s, 1, 1000, buf.data(), buf.size());
    static SaveData out;
    CHECK(decodeSave(buf.data(), n, out) == LoadResult::Ok);
    CHECK(std::memcmp(out.world.look, s.world.look, sizeof(s.world.look)) == 0 && out.world.lookMade == 1);
    CHECK(out.world.x == s.world.x && out.world.y == s.world.y && out.world.heading == s.world.heading && out.world.inValley);
    CHECK(out.world.partnerId == 7 && out.world.placesFound == s.world.placesFound && out.world.lanternsLit == s.world.lanternsLit);
    CHECK(out.world.quest[2] == 2 && out.world.cups[1] == 3 && out.world.flags == s.world.flags && out.world.ribbons == 5);
}

// The Market's goods stall (D86): four different things you don't have, the same all day; one
// bought leaves its spot empty till tomorrow; a new day, a new pick; nothing left, nothing shown.
TEST(the_goods_stall) {
    static SaveData s;
    s = SaveData{};
    s.gleam = 100000;
    Item a[kStallSpots], b[kStallSpots];
    stallToday(s, 100, a);
    stallToday(s, 100, b);
    for (int k = 0; k < kStallSpots; ++k) {
        CHECK(a[k] != Item::Count && a[k] == b[k] && !owns(s, a[k]));
        for (int j = 0; j < k; ++j) CHECK(a[j] != a[k]);
    }
    CHECK(buyFromStall(s, 100, 2) && owns(s, a[2]));
    stallToday(s, 100, b);
    CHECK(b[2] == Item::Count && b[0] == a[0] && b[1] == a[1] && b[3] == a[3]);
    CHECK(!buyFromStall(s, 100, 2));
    stallToday(s, 101, b);  // tomorrow: four again, none of them the one bought
    for (int k = 0; k < kStallSpots; ++k) CHECK(b[k] != Item::Count && b[k] != a[2]);
    for (int i = 0; i < kItems; ++i) buyItem(s, static_cast<Item>(i));
    stallToday(s, 102, b);
    for (int k = 0; k < kStallSpots; ++k) CHECK(b[k] == Item::Count);
}

// The villagers: first meetings settle a flag once, names fill in, and the festival night is told
// by Rowan or Wren once the Lantern Trial is won.
TEST(the_villagers_talk) {
    static SaveData s;
    s = SaveData{};
    world::startWorld(s);
    std::strcpy(s.playerName, "Noah");
    s.dragons[0] = Dragon{};
    s.dragons[0].id = 7;
    std::strcpy(s.dragons[0].name, "Ember");
    s.dragonCount = 1;
    s.world.partnerId = 7;
    campaign::update(s);
    Talk t = talkTo(s, Villager::Keeper);
    CHECK(t.count >= 3 && (t.sets & kFlagMetKeeper));
    char line[160];
    fillLine(t.lines[0], s, line, sizeof(line));
    CHECK(std::strstr(line, "Noah") && !std::strchr(line, '{'));
    fillLine(t.lines[3], s, line, sizeof(line));
    CHECK(std::strstr(line, "Ember") != nullptr);
    CHECK(finishTalk(s, Villager::Keeper, t) && !finishTalk(s, Villager::Keeper, t));
    CHECK(!(talkTo(s, Villager::Keeper).sets & kFlagMetKeeper));  // met: the next talk is ordinary
    for (int v = 0; v < kVillagers; ++v) {
        const Talk k = talkTo(s, static_cast<Villager>(v));
        CHECK(k.count >= 1);
        for (int i = 0; i < k.count; ++i) {
            fillLine(k.lines[i], s, line, sizeof(line));
            CHECK(std::strlen(line) < 150);  // fits the box
        }
    }
    // The meadow stray: Bram asks, then waits for it.
    finishTalk(s, Villager::Sanctuary, talkTo(s, Villager::Sanctuary));
    CHECK((s.world.flags & kFlagMetSanctuary) && std::strstr(talkTo(s, Villager::Sanctuary).lines[0], "meadow"));
    // The festival night: every quest but the last done, the trial won.
    for (int q = 0; q < 7; ++q) s.world.quest[q] = campaign::kQuestDone;
    for (int p = 0; p < kPlaceCount; ++p) {
        world::findPlace(s, p);
        if (world::placeInfo(p).lantern) world::lightLantern(s, p);
    }
    s.world.cups[static_cast<int>(Challenge::LanternTrial)] = 1;
    campaign::update(s);
    CHECK(campaign::view(s, 7).stepIndex == 2);
    t = talkTo(s, Villager::Steward);
    CHECK(t.sets & kFlagFestival);
    finishTalk(s, Villager::Steward, t);
    CHECK(campaign::update(s).starEgg);
}

void runWorldTests() {
    RUN(places_and_lanterns);
    RUN(the_lantern_festival);
    RUN(the_world_saves);
    RUN(the_goods_stall);
    RUN(the_villagers_talk);
}
