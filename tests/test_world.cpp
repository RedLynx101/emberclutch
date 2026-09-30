// Beta's world (core/world): places found and lanterns lit; the save keeps it all and an older
// save starts a fresh world with the den found. (The quests are the story's: tests/test_story.cpp.)
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/accessories.hpp"
#include "core/trainer.hpp"
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
    // The settings' clips (workstream D): the ones held a while loop, the gestures play once and
    // finish; sitting and dozing on the ground carry the root down (legs flat on it), the rest
    // stand where they are (or bob a little).
    for (const char* name : {"clap", "sit_clap", "sit_ground", "doze", "doze_stand", "fish", "scatter", "write", "tidy", "fly_toy"}) {
        const int c = lib.find(name);
        CHECK(c >= 0 && lib.clips[static_cast<std::size_t>(c)].loop);
    }
    for (const char* name : {"stretch", "fist_pump", "point", "worried", "slump", "bow", "cast"}) {
        const int c = lib.find(name);
        CHECK(c >= 0 && !lib.clips[static_cast<std::size_t>(c)].loop);
        if (c < 0) continue;
        Animator a;
        a.play(c, 0.0f);
        a.update(lib, 3.0f, nullptr, 0);
        CHECK(a.finished(lib));  // (a scene waiting for one to end moves on)
    }
    for (const char* name : {"sit_ground", "doze", "clap", "fish", "bow"}) {
        const int c = lib.find(name);
        if (c < 0) continue;
        const bool ground = std::strcmp(name, "sit_ground") == 0 || std::strcmp(name, "doze") == 0;
        const AnimClip& clip = lib.clips[static_cast<std::size_t>(c)];
        const float up = clip.root.empty() ? 0.0f : clip.root[1];
        CHECK(ground ? (up < -0.18f && up > -0.25f) : std::fabs(up) < 0.05f);
    }
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
    int eggs = 0, air = 0, wear = 0;
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
        if (r.accessory >= 0) {  // (the 3rd, 7th and 12th treasures: the found things to wear)
            ++wear;
            CHECK(trainer::ownsAccessory(s, r.accessory) && accessoryInfo(r.accessory).source == WearSource::Find);
        }
        if (r.egg >= 0) {
            ++eggs;
            CHECK(s.dragons[r.egg].stage == Stage::Egg && s.dragons[r.egg].origin == Origin::Wild);
        }
        CHECK(takeFind(s, i, 0, rng).gleam == 0);  // once
    }
    CHECK(eggs == 2 && air == 10 && wear == 3);
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
    // The day's little finds (run 19): scattered on dry, gentle ground, taken once, back the next day
    // in new spots; the treasures stay found.
    CHECK(renewFinds(s, 500) && !renewFinds(s, 500));
    const u32 treasures = s.world.finds & ((1u << kFindSpots) - 1);
    Vec3 first[kDailyFinds];
    for (int k = 0; k < kDailyFinds; ++k) {
        const int i = kFindSpots + k;
        first[k] = findAt(v, s, i);
        CHECK(!findDone(s, i) && first[k].z - 0.6f > v.water + 0.5f);
        CHECK(findNear(s, v, first[k], false) == i && findNear(s, v, first[k], true) != i);
        const u32 gleam = s.gleam;
        const FindReward r = takeFind(s, i, 0, rng);
        CHECK(findDone(s, i) && (r.gleam > 0 || r.food >= 0 || r.trinket >= 0) && (r.gleam == 0 || s.gleam == gleam + r.gleam));
    }
    CHECK(renewFinds(s, 501));
    CHECK((s.world.finds & ((1u << kFindSpots) - 1)) == treasures && !findDone(s, kFindSpots));
    CHECK(length(findAt(v, s, kFindSpots) - first[0]) > 1.0f);  // somewhere new
}

void runWorldTests() {
    RUN(places_and_lanterns);
    RUN(the_world_saves);
    RUN(the_goods_stall);
    RUN(the_people);
    RUN(finds_and_the_fog);
}
