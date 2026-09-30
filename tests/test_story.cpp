// The story engine (core/story, D137): the scripts' tables, a new game's first quests (and their
// branches when things are done early), the save's story block and the migration of a Beta save,
// the feelings on lines, and a story bot that plays every quest in the game through to its end.
#include <cstring>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/care.hpp"
#include "core/clock.hpp"
#include "core/save.hpp"
#include "core/story.hpp"
#include "core/trainer.hpp"
#include "core/valley.hpp"
#include "core/world.hpp"

using namespace ec;

namespace {

constexpr s64 kNoon = 20000 * kDay + 12 * kHour;  // (a day long after 1970, at noon)

void freshGame(SaveData& s) {
    s = SaveData{};
    world::startWorld(s);
    std::strcpy(s.playerName, "Noah");
}

Dragon& addDragon(SaveData& s, Stage stage, const char* name) {
    Dragon& d = s.dragons[s.dragonCount++];
    d = Dragon{};
    d.id = s.nextId++;
    d.stage = stage;
    d.sex = s.dragonCount % 2 ? Sex::Female : Sex::Male;
    std::strncpy(d.name, name, sizeof(d.name) - 1);
    return d;
}

// A talk with someone read to its end (the dialogue box would); what it moved on.
story::News talk(SaveData& s, int person, s64 now, std::string* said = nullptr) {
    const Talk t = story::talkTo(s, person, now);
    story::News n;
    if (t.count == 0) return n;
    if (said) {
        said->clear();
        for (int i = 0; i < t.count; ++i) *said += std::string(t.lines[i]) + "\n";
    }
    story::finishTalk(s, t, now, &n);
    return n;
}

}  // namespace

// The tables: everyone and everything the scripts name is there; the villagers map one to one.
TEST(the_story_tables) {
    CHECK(story::questCount() == story::kQuestCount && story::questCount() >= 20);
    CHECK(story::personCount() == story::kPersonIdCount);
    CHECK(story::findPerson("rowan") == story::kPRowan && story::findPerson("nobody") == -1);
    for (int v = 0; v < kVillagers; ++v) {
        const int p = story::personOfVillager(static_cast<Villager>(v));
        CHECK(p >= 0 && story::person(p).villager == v);
    }
    static SaveData s;
    freshGame(s);
    for (int q = 0; q < story::questCount(); ++q) {
        const story::QuestView v = story::view(s, q, kNoon);
        CHECK(v.title[0] && v.stepCount >= 1 && v.stepCount <= 6 && !v.started && !v.done);
    }
    // feelings on the game's own lines: "[huff] Whatever." -> huff, and the words after
    const char* rest = nullptr;
    CHECK(story::splitFeel("[huff] Whatever.", &rest) == story::Feel::Huff && std::strcmp(rest, "Whatever.") == 0);
    CHECK(story::splitFeel("Plain words.", &rest) == story::Feel::Calm && std::strcmp(rest, "Plain words.") == 0);
    CHECK(story::splitFeel("[nonsense] Hm.", &rest) == story::Feel::Calm);
    CHECK(std::strcmp(story::feelName(story::Feel::Love), "love") == 0);
}

// A new game: nothing begins by itself; the first egg hatching brings Rowan's letter; reading it
// begins the keeper's apprentice; meeting him, the den's lantern, and back to him to finish it.
TEST(the_keepers_apprentice) {
    static SaveData s;
    freshGame(s);
    s.gleam = 0;
    story::News n = story::update(s, kNoon);
    CHECK(n.started < 0 && n.mail == 0 && story::currentQuest(s) == -1);
    addDragon(s, Stage::Egg, "");
    CHECK(story::update(s, kNoon).mail == 0);  // an egg isn't hatched yet
    Dragon& d = s.dragons[0];
    d.stage = Stage::Hatchling;
    std::strcpy(d.name, "Ember");
    s.world.partnerId = d.id;
    n = story::update(s, kNoon);
    CHECK(n.mail == 1 && story::unreadMail(s) == 1 && story::letterDelivered(s, story::kLRowanHello));
    int box[8];
    CHECK(story::mailbox(s, box, 8) == 1 && box[0] == story::kLRowanHello);
    CHECK(story::readLetter(s, story::kLRowanHello, kNoon, &n) && n.started == story::kQKeepersApprentice);
    CHECK(!story::readLetter(s, story::kLRowanHello, kNoon) && story::unreadMail(s) == 0);  // once
    CHECK(story::questStep(s, story::kQKeepersApprentice) == 1 && story::currentQuest(s) == story::kQKeepersApprentice);
    std::string said;
    n = talk(s, story::kPRowan, kNoon, &said);
    CHECK(said.find("letter") != std::string::npos && said.find("Cinder") != std::string::npos);
    CHECK(story::questStep(s, story::kQKeepersApprentice) == 2 && story::flag(s, story::kFMetRowan));
    CHECK((s.world.flags & kFlagMetKeeper) != 0);
    world::lightLantern(s, kPlaceDen);
    n = story::update(s, kNoon);
    CHECK(n.stepped == story::kQKeepersApprentice && story::questStep(s, story::kQKeepersApprentice) == 3);
    n = talk(s, story::kPRowan, kNoon, &said);
    CHECK(n.finished == story::kQKeepersApprentice && n.gleam == 60 && s.gleam == 60);
    CHECK(story::questDone(s, story::kQKeepersApprentice) && story::questStep(s, story::kQMarketDay) == 1);
    CHECK(said.find("Maple") != std::string::npos);
}

// Things done early (Noah: "what if something has already been done?"): meeting Rowan with the den's
// lantern already lit finishes his quest at once; Market day with the Fruit Catch won and the lantern
// lit moves straight to telling Maple once Fig is found.
TEST(the_story_follows_what_is_done) {
    static SaveData s;
    freshGame(s);
    Dragon& d = addDragon(s, Stage::Adult, "Ember");
    s.world.partnerId = d.id;
    world::lightLantern(s, kPlaceDen);
    std::string said;
    story::News n = talk(s, story::kPRowan, kNoon, &said);
    CHECK(n.finished == story::kQKeepersApprentice && said.find("already") != std::string::npos);
    CHECK(story::questStep(s, story::kQMarketDay) == 1);
    world::findPlace(s, kPlaceMarket);
    world::lightLantern(s, kPlaceMarket);
    s.world.cups[static_cast<int>(Challenge::FruitCatch)] = 2;
    talk(s, story::kPMaple, kNoon);  // meeting Maple: on to finding Fig
    CHECK(story::questStep(s, story::kQMarketDay) == 2);
    talk(s, story::kPFig, kNoon, &said);  // found (a grown dragon wakes him its own way)
    CHECK(said.find("BIG") != std::string::npos);
    CHECK(story::questStep(s, story::kQMarketDay) == 5);  // the cup and the lantern: done already
    n = talk(s, story::kPMaple, kNoon, &said);
    CHECK(n.finished == story::kQMarketDay && story::flag(s, story::kFFigFriend) && said.find("upside down") != std::string::npos);
}

// The save keeps the story (every quest's step, the flags, counters, mailbox and days); a Beta
// save's quests migrate: done stays done, begun begins again, the people met are met.
TEST(the_story_saves_and_migrates) {
    static SaveData s, back;
    freshGame(s);
    story::startQuest(s, story::kQHilltop);
    story::finishQuest(s, story::kQKeepersApprentice, kNoon);
    story::setFlag(s, story::kFFigFriend);
    story::setVar(s, story::kVPagesFound, 3);
    story::deliver(s, story::kLMapleFlyer);
    story::markEvent(s, 0, kNoon);
    std::vector<u8> buf(maxEncodedSize());
    const std::size_t n = encodeSave(s, 1, kNoon, buf.data(), buf.size());
    CHECK(n > 0 && decodeSave(buf.data(), n, back) == LoadResult::Ok);
    CHECK(std::memcmp(&back.story, &s.story, sizeof(s.story)) == 0);
    CHECK(!story::flag(back, story::kFMigrated));  // (a v2 save: nothing to migrate)

    static SaveData old;
    freshGame(old);
    old.world.quest[0] = 0xFF;  // Beta: the keeper's apprentice done, Market day done, the meadow begun
    old.world.quest[1] = 0xFF;
    old.world.quest[3] = 2;
    old.world.flags |= kFlagMetKeeper | kFlagMetMarket | kFlagMetSanctuary;
    story::migrate(old, kNoon);
    CHECK(story::questDone(old, story::kQKeepersApprentice) && story::questDone(old, story::kQMarketDay));
    CHECK(story::questStep(old, story::kQMeadow) == 2);  // (its first step, meeting Bram, already done)
    CHECK(story::flag(old, story::kFMetRowan) && story::flag(old, story::kFMetBram) && story::flag(old, story::kFFigFriend));
    CHECK(story::flag(old, story::kFMigrated) && story::letterDelivered(old, story::kLNewsLivingValley));
    CHECK(!story::letterDelivered(old, story::kLRowanHello));  // (he's met: no invitation)

    // A real v1 save (the emulator's, 0.9.13): it loads, and its story is migrated.
    std::vector<u8> bytes;
    if (FILE* f = std::fopen("data/save_v1.bin", "rb")) {
        std::fseek(f, 0, SEEK_END);
        bytes.resize(static_cast<std::size_t>(std::ftell(f)));
        std::fseek(f, 0, SEEK_SET);
        if (std::fread(bytes.data(), 1, bytes.size(), f) != bytes.size()) bytes.clear();
        std::fclose(f);
    }
    CHECK(!bytes.empty());
    static SaveData v1;
    SaveHeaderInfo info;
    CHECK(decodeSave(bytes.data(), bytes.size(), v1, &info) == LoadResult::Ok && info.version == 1);
    CHECK(story::flag(v1, story::kFMigrated));
}

// The story bot: day after day it talks to everyone (at noon, in the evening and at night), reads
// every letter, uses what there is to pick up, and makes true whatever the quests' steps ask for;
// every quest in the game must come to its end, and every line it hears fits the box.
TEST(the_story_bot_plays_everything) {
    static SaveData s;
    freshGame(s);
    std::strcpy(s.playerName, "Fifteen Letters");  // (the longest names: the lines must still fit)
    Dragon& d = addDragon(s, Stage::Hatchling, "Fifteen Letters");
    s.world.partnerId = d.id;
    const u32 partner = d.id;
    auto partnerOf = [&]() -> Dragon& {
        for (int i = 0; i < s.dragonCount; ++i)
            if (s.dragons[i].id == partner) return s.dragons[i];
        return s.dragons[0];
    };
    int longest = 0, heard = 0;
    auto check = [&](const Talk& t) {
        for (int i = 0; i < t.count; ++i) {
            char line[200];
            fillLine(t.lines[i], s, line, sizeof(line));
            const int len = static_cast<int>(std::strlen(line));
            if (len > longest) longest = len;
            CHECK(len < 160);  // (DialogueState::text)
            ++heard;
        }
    };
    // What a step asks for, made true (a negated term, or one about other quests, is left alone).
    auto force = [&](const story::TermView& t, s64 now) {
        const std::string op = t.op;
        if (t.neg) return;
        if (op == "flag") story::setFlag(s, static_cast<int>(t.a));
        else if (op == "world") s.world.flags |= t.a;
        else if (op == "lantern") { world::findPlace(s, static_cast<int>(t.a)); world::lightLantern(s, static_cast<int>(t.a)); }
        else if (op == "place") world::findPlace(s, static_cast<int>(t.a));
        else if (op == "cup") { if (s.world.cups[t.a] < t.b) s.world.cups[t.a] = static_cast<u8>(t.b); }
        else if (op == "lanterns") {
            for (int p = 0; p < kPlaceCount; ++p)
                if (world::placeInfo(p).lantern && p != kPlaceArena) { world::findPlace(s, p); world::lightLantern(s, p); }
        }
        else if (op == "grown") partnerOf().stage = Stage::Adult;
        else if (op == "adults") {
            int adults = 0;
            for (int i = 0; i < s.dragonCount; ++i) adults += s.dragons[i].stage == Stage::Adult;
            while (adults < static_cast<int>(t.a)) { addDragon(s, Stage::Adult, "Pal"); ++adults; }
        }
        else if (op == "hatched" || op == "juvenile") { if (partnerOf().stage < Stage::Juvenile) partnerOf().stage = Stage::Juvenile; }
        else if (op == "var") { if (story::var(s, static_cast<int>(t.a)) < t.b) story::setVar(s, static_cast<int>(t.a), static_cast<u8>(t.b)); }
        else if (op == "bit") story::setVar(s, static_cast<int>(t.a), static_cast<u8>(story::var(s, static_cast<int>(t.a)) | (1u << t.b)));
        else if (op == "league") { if (s.progress.battleLeague < t.a) s.progress.battleLeague = static_cast<u8>(t.a); }
        else if (op == "beaten") s.progress.battleBeaten[t.a] = static_cast<u8>(s.progress.battleBeaten[t.a] | (1u << t.b));
        else if (op == "shows") { if (s.progress.showLeague < t.a) s.progress.showLeague = static_cast<u8>(t.a); }
        else if (op == "showwon") s.progress.showWon[t.a] = 1;
        else if (op == "hollow") { if (s.progress.hollowDeepest < t.a) s.progress.hollowDeepest = static_cast<u8>(t.a); }
        else if (op == "count") { if (s.progress.counts[t.a] < t.b) s.progress.counts[t.a] = t.b; }
        else if (op == "gleam") { if (s.gleam < t.a) s.gleam = t.a; }
        else if (op == "pouch") { if (s.pouch[t.a] < t.b) s.pouch[t.a] = t.b; }
        (void)now;
    };
    const int hours[3] = {12, 19, 23};
    int round = 0;
    for (; round < 120; ++round) {
        bool allDone = true;
        for (int q = 0; q < story::questCount(); ++q) allDone = allDone && story::questDone(s, q);
        if (allDone) break;
        for (int h : hours) {
            const s64 now = kNoon + round * kDay + (h - 12) * kHour;
            story::update(s, now);
            int box[64];
            const int mail = story::mailbox(s, box, 64);
            for (int k = 0; k < mail; ++k) story::readLetter(s, box[k], now);
            for (int p = 1; p < story::personCount(); ++p) {
                if (!story::hasImportantTalk(s, p, now)) continue;
                const Talk t = story::talkTo(s, p, now);
                check(t);
                story::finishTalk(s, t, now);
            }
            story::Pickup found[24];
            const int n = story::pickups(s, now, found, 24);
            for (int k = 0; k < n; ++k) {
                if (found[k].sign) continue;
                const Talk t = story::pickupTalk(s, found[k].index, now);
                check(t);
                story::finishTalk(s, t, now);
            }
            for (int q = 0; q < story::questCount(); ++q) {  // what each step waits for
                story::TermView terms[8];
                const int tn = story::stepTerms(s, q, terms, 8);
                for (int k = 0; k < tn; ++k) force(terms[k], now);
            }
            if (s.pouch[static_cast<int>(Food::HearthBread)] < 3) s.pouch[static_cast<int>(Food::HearthBread)] = 3;
            story::update(s, now);
        }
    }
    int done = 0;
    for (int q = 0; q < story::questCount(); ++q) {
        done += story::questDone(s, q);
        if (!story::questDone(s, q)) {
            const story::QuestView v = story::view(s, q, kNoon + round * kDay);
            std::printf("  unfinished: %s (step %d: %s)\n", v.id, v.stepIndex + 1, v.step);
        }
    }
    CHECK(done == story::questCount());
    CHECK((s.world.flags & kFlagStarEgg) && story::flag(s, story::kFAct1Done));
    std::printf("  %d of %d quests done in %d days; %d lines heard, the longest %d letters\n", done, story::questCount(), round,
                heard, longest);
}

void runStoryTests() {
    RUN(the_story_tables);
    RUN(the_keepers_apprentice);
    RUN(the_story_follows_what_is_done);
    RUN(the_story_saves_and_migrates);
    RUN(the_story_bot_plays_everything);
}
