// Beta's world (core/world) and the Lantern Festival (core/campaign): places found and lanterns
// lit; the quests follow the world in any order and never stick; the save keeps it all and an
// older save starts a fresh world with the den found.
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/campaign.hpp"
#include "core/items.hpp"
#include "core/market.hpp"
#include "core/anim.hpp"
#include "core/model.hpp"
#include "core/people.hpp"
#include "core/finds.hpp"
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

// The people: every model loads with the game's reader, binds the one clip library (every
// bone a track), has its body and eyes (the player six hair styles); their colours follow the
// creator's choices; the walk and run clips keep pace with the body.
std::vector<u8> readAll(const char* path) {
    std::vector<u8> data;
    if (FILE* f = std::fopen(path, "rb")) {
        std::fseek(f, 0, SEEK_END);
        data.resize(static_cast<std::size_t>(std::ftell(f)));
        std::fseek(f, 0, SEEK_SET);
        if (std::fread(data.data(), 1, data.size(), f) != data.size()) data.clear();
        std::fclose(f);
    }
    return data;
}

TEST(the_people) {
    static AnimLibrary lib;
    const std::vector<u8> clips = readAll("../romfs/anims/person.eca");
    CHECK(!clips.empty() && loadAnims(clips.data(), clips.size(), lib));
    for (const char* name : {"idle", "walk", "run", "wave", "talk", "nod", "cheer", "crouch_pet", "ride", "sit_loop"})
        CHECK(lib.find(name) >= 0);
    for (int k = 0; k < kPeople; ++k) {
        const Person who = static_cast<Person>(k);
        std::string path = personFile(who);
        path.replace(0, 6, "../romfs");  // romfs:/people/... -> ../romfs/people/...
        const std::vector<u8> bytes = readAll(path.c_str());
        static ModelData m;
        m = ModelData{};
        CHECK(!bytes.empty() && loadModel(bytes.data(), bytes.size(), m));
        CHECK(m.findMesh(kMeshBody, kGroupBody, 0) && m.findMesh(kMeshPart, kGroupEyes, 0));
        CHECK(m.skel.find("eyes") >= 0 && m.skel.find("head") >= 0);
        AnimBinding bind;
        bindAnims(lib, m.skel, bind);
        for (int b = 0; b < m.skel.count; ++b) CHECK(bind.libBone[b] >= 0);
        if (who == Person::PlayerA || who == Person::PlayerB)
            for (int h = 0; h < kHairStyles; ++h) CHECK(m.findMesh(kMeshPart, kGroupHair, static_cast<u8>(h)) != nullptr);
        CHECK(personWalkSpeed(who) > 0.3f && personRunSpeed(who) > personWalkSpeed(who));
    }
    u8 look[kLookParts] = {1, 2, 3, 4, 2, 1};
    Rgb a[kPalCount], b[kPalCount];
    playerPalette(look, a);
    CHECK(playerBody(look) == Person::PlayerB);
    look[kLookSkin] = 0;
    playerPalette(look, b);
    CHECK(a[kPalBase].r != b[kPalBase].r && a[kPalHorn].r == b[kPalHorn].r);
    look[kLookHairColour] = 200;  // out of range: the first
    playerPalette(look, b);
    CHECK(b[kPalHorn].r == 132);
    villagerPalette(Villager::Traveller, a);
    CHECK(a[kPalGlow].r == 255);  // the lantern's flame
    CHECK(std::strcmp(lookChoiceName(kLookHair, 2), "Ponytail") == 0 && lookChoiceName(kLookHair, 9)[0] == 0);
    CHECK(personFor(Villager::Child) == Person::Child && personHips(Person::Child) < personHips(Person::PlayerA));
}

// The finds: each taken once, what it held into the save (an egg into a nest or the Vault),
// the islands' only from the air; the map's fog clears round you and is saved.
TEST(finds_and_the_fog) {
    static Valley v;
    static bool loaded = false;
    if (!loaded) {
        const std::vector<u8> bytes = readAll("../romfs/valley/skyreach.evl");
        loaded = !bytes.empty() && loadValley(bytes.data(), bytes.size(), v);
    }
    CHECK(loaded);
    if (!loaded) return;
    static SaveData s;
    s = SaveData{};
    world::startWorld(s);
    s.dragons[0] = Dragon{};
    s.dragons[0].id = 1;
    s.dragonCount = 1;
    s.nextId = 2;
    Rng rng(5);
    int eggs = 0, air = 0;
    for (int i = 0; i < kFindSpots; ++i) {
        const FindSpot& f = findSpot(i);
        CHECK(f.gleam > 0 || f.trinket >= 0 || f.egg);  // every spot holds something
        const Vec3 at = findAt(v, i);
        CHECK(std::isfinite(at.z) && at.z > v.water - 1);
        air += f.fromAir;
        if (!f.fromAir) CHECK(v.heightAt(at.x, at.y) > v.water + 0.3f);  // walked to: on dry land
        if (f.island >= 0) CHECK(findNear(s, v, at + Vec3{0, 0, 30}, true) < 0);  // not from far above
        CHECK(findNear(s, v, at, f.fromAir) == i);
        const u32 gleam = s.gleam;
        const FindReward r = takeFind(s, i, 0, rng);
        CHECK(findDone(s, i) && findNear(s, v, at, true) != i);
        CHECK(r.gleam == 0 || s.gleam == gleam + r.gleam);
        if (r.egg >= 0) {
            ++eggs;
            CHECK(s.dragons[r.egg].stage == Stage::Egg && s.dragons[r.egg].origin == Origin::Wild);
        }
        CHECK(takeFind(s, i, 0, rng).gleam == 0);  // once
    }
    CHECK(eggs == 2 && air == 9);
    // The fog: clearing round a spot, then saved and read back.
    CHECK(explore(s, v, {0, 0}, 150) && !explore(s, v, {0, 0}, 150));
    const float cell = v.size() / kFogCells;
    const int cx = static_cast<int>((0 - v.x0) / cell), cy = static_cast<int>((0 - v.y0) / cell);
    CHECK(explored(s, cx, cy) && !explored(s, 0, 0));
    std::vector<u8> buf(maxEncodedSize());
    const std::size_t n = encodeSave(s, 1, 0, buf.data(), buf.size());
    static SaveData back;
    CHECK(n > 0 && decodeSave(buf.data(), n, back, nullptr) == LoadResult::Ok);
    CHECK(back.world.finds == s.world.finds && explored(back, cx, cy) && !explored(back, 0, 0));
}

void runWorldTests() {
    RUN(places_and_lanterns);
    RUN(the_lantern_festival);
    RUN(the_world_saves);
    RUN(the_goods_stall);
    RUN(the_villagers_talk);
    RUN(the_people);
    RUN(finds_and_the_fog);
}
