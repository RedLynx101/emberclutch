// The story engine (D137, the Living Valley pass): quests, talks, letters, where the story's people
// stand and the things you pick up, all written in plain text (story/*.story) and compiled into
// tables by tools/story/build_story.py. Every quest starts with someone asking (a talk or a letter)
// and ends by going back to them; a step is done when what it asks for is true of the world, or when
// a talk moves it on, so the quests follow whatever you've already done. Every line carries a
// feeling (the face, the body, an emote, the voice). Pure logic (PC-tested: tests/test_story.cpp).
#pragma once

#include "core/math3d.hpp"
#include "core/story_ids.hpp"
#include "core/story_state.hpp"
#include "core/villagers.hpp"

namespace ec {

struct SaveData;

namespace story {

// ---- Feelings (the tags on lines: [happy] ...)
enum class Feel : u8 {
    Calm, Happy, Laugh, Excited, Surprised, Shock, Sad, Crying, Angry, Huff, Worried,
    Scared, Sleepy, Love, Proud, Cool, Shy, Thinking, Wistful, Dizzy, Count
};
constexpr int kFeels = static_cast<int>(Feel::Count);
const char* feelName(Feel f);
// A line from the game's own string tables may start with a tag ("[huff] Whatever."): its feeling,
// and `rest` past it; calm and the whole line otherwise.
Feel splitFeel(const char* line, const char** rest);

// ---- The lines of quests (the Journal groups them)
enum class Line : u8 { Main, Pageant, League, Hollow, Cove, Errand, Count };
const char* lineName(Line l);

// ---- People who speak
struct PersonInfo {
    const char* id;
    const char* name;
    const char* title;
    u8 voice;
    float pitch;
    s8 villager;          // a core/villagers Villager, or -1
    const char* body;     // the model (romfs/people/<body>.ecm), or "" (the villager's own; nobody)
    const char* portrait; // their portraits (romfs/portraits/<id>.t3x), or ""
    Rgb tint;             // the portrait's disc
};
int personCount();
const PersonInfo& person(int p);
int personOfVillager(Villager v);  // -1 if the story doesn't know them
int findPerson(const char* id);    // -1 if none

// ---- Quests
int questCount();
struct QuestView {
    const char* id = "";
    const char* title = "";
    const char* step = "";     // what to do now (the done quest's last step's)
    const char* rumour = "";   // a hint while it waits to be asked about
    int stepIndex = 0, stepCount = 0;  // (0-based)
    bool started = false, done = false;
    bool open = false;         // not begun, and everything it waits for is done
    Line line = Line::Main;
    int giver = -1;            // a person
};
QuestView view(const SaveData& s, int quest, s64 now);
// The quest to show first: the earliest begun main-line quest not done, else any begun one; -1.
int currentQuest(const SaveData& s);

struct News {
    int started = -1;     // a quest begun
    int stepped = -1;     // a quest moved on a step
    int finished = -1;    // a quest done (its reward given)
    u32 gleam = 0;        // Gleam given
    bool starEgg = false; // the star-born egg is to be given (the game makes it)
    int mail = 0;         // letters that arrived
    void merge(const News& o);
};
// Moves every quest on as far as the world allows and delivers the letters that are due. Call after
// anything changes the world (and now and then: letters come by the day).
News update(SaveData& s, s64 now);

// ---- Talks
// What `person` says now: the first of their rules that holds (count 0: nothing to say).
Talk talkTo(const SaveData& s, int person, s64 now);
// Whether their talk now is about something (a quest, a first meeting), not just chatter: a
// feature's own person (the fisher, a champion) says it before their usual business.
bool hasImportantTalk(const SaveData& s, int person, s64 now);
// The talk read to its end: its effects, then update(). True if anything changed.
bool finishTalk(SaveData& s, const Talk& t, s64 now, News* news = nullptr);

// ---- Letters (the mailbox by the den's door)
int mailbox(const SaveData& s, int* out, int cap);  // delivered letters, the newest first
int unreadMail(const SaveData& s);
struct LetterView {
    const char* id = "";
    const char* subject = "";
    int from = -1;
    const char* paragraphs[16] = {};  // "--" alone starts a new page
    int count = 0;
    bool read = false;
};
LetterView letter(const SaveData& s, int l);
// Opens it: read, and its effects the first time (a quest begun, a gift). True if anything changed.
bool readLetter(SaveData& s, int l, s64 now, News* news = nullptr);
void deliver(SaveData& s, int l);  // (tests and the dev menu)

// ---- Where the story's people stand (the story feature draws them; a villager stands there instead
// of at their routine while a spot holds, the festival night)
struct Spot {
    int person = -1;
    int place = 0;     // core/valley ValleyPlace
    Vec2 at;           // in the place's frame
    float facing = 0;  // radians from the place's front
    const char* clip = "";
};
bool spotOf(const SaveData& s, int person, s64 now, Spot& out);

// ---- Things to pick up, and signs to read
struct Pickup {
    int index = -1;
    const char* id = "";
    int place = 0;
    Vec2 at;
    const char* prompt = "";
    bool sign = false, glint = false;
    int group = -1;
};
int pickups(const SaveData& s, s64 now, Pickup* out, int cap);
Talk pickupTalk(const SaveData& s, int pickup, s64 now);

// ---- Where a quest's step points (core/guide makes it a spot in the valley)
enum class Where : u8 { None, Place, Lantern, Cup, Person, Area, Spot, Unlit, Group };
struct StepWhere {
    Where kind = Where::None;
    int a = 0;        // the place, the challenge, the person or the group
    Vec2 at;          // (area, spot) in the place's frame
    float radius = 0; // (area)
};
StepWhere stepWhere(const SaveData& s, int quest);

// ---- The state, for the game's own code (a flag the glide sets, the pages ...)
bool flag(const SaveData& s, int f);
void setFlag(SaveData& s, int f, bool on = true);
u8 var(const SaveData& s, int v);
void setVar(SaveData& s, int v, u8 value);
int questStep(const SaveData& s, int quest);  // 0 not begun, 1.. the step (1-based), kQuestDone
bool questDone(const SaveData& s, int quest);
void startQuest(SaveData& s, int quest);      // (tests and the dev menu)
void finishQuest(SaveData& s, int quest, s64 now, News* news = nullptr);
bool letterDelivered(const SaveData& s, int l);
void markEvent(SaveData& s, int e, s64 now);

// A save from before the story engine (save v1: the Beta campaign's eight quests): the same progress
// in the new quests, the people met, the letters that would have come marked read.
void migrate(SaveData& s, s64 now);

// ---- For the tests' story bot (tests/test_story.cpp): the terms a begun quest's step waits for.
struct TermView {
    const char* op = "";  // the condition's name, as the scripts write it ("lantern", "cup" ...)
    bool neg = false;
    u32 a = 0;
    u16 b = 0;
};
int stepTerms(const SaveData& s, int quest, TermView* out, int cap);
int pickupCount();

}  // namespace story
}  // namespace ec
