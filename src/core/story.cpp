#include "core/story.hpp"

#include <cstring>

#include "core/clock.hpp"
#include "core/save.hpp"
#include "core/trainer.hpp"
#include "core/valley.hpp"
#include "core/world.hpp"

namespace ec::story {
namespace {

// The compiled tables' shapes (tools/story/build_story.py writes them; keep the fields in step).
struct CondDef {
    u8 op, neg;
    u32 a;
    u16 b;
};
struct EffectDef {
    u8 op;
    u32 a;
    u16 b;
};
struct LineDef {
    const char* text;
    u8 feel;
    s8 speaker;
    u8 group;
    u16 cond, condN;
};
struct RuleDef {
    u8 person, important, groups;
    u16 cond, condN, eff, effN, line, lineN;
};
struct StepDef {
    const char* text;
    u16 cond, condN;
    s8 talk;
    u8 where;
    u16 w0;
    s16 w1, w2;
    u16 w3;
};
struct QuestDef {
    const char* id;
    u8 slot;
    const char* title;
    u8 line;
    s8 giver;
    u16 gleam;
    u8 autoStart;
    u16 cond, condN;
    const char* rumour;
    u16 step, stepN, eff, effN;
};
struct LetterDef {
    const char* id;
    u8 slot, from;
    const char* subject;
    u16 cond, condN, eff, effN, line, lineN;
};
struct SpotDef {
    u8 person, place;
    s16 x, y, facing;
    const char* clip;
    u16 cond, condN;
};
struct PickupDef {
    const char* id;
    u8 place;
    s16 x, y;
    const char* prompt;
    u8 sign;
    s8 group;
    u8 glint;
    u16 cond, condN, eff, effN, line, lineN;
};
struct PersonDef {
    const char* id;
    const char* name;
    const char* title;
    u8 voice;
    float pitch;
    s8 villager;
    const char* body;
    const char* portrait;
    Rgb tint;
    u16 metFlag;
};

#include "core/story_data.inc"

// Opcodes, in build_story.py's COND_OPS and EFFECT_OPS order.
enum CondOp : u8 {
    cDone, cBegun, cActive, cNew, cStep, cPast, cFlag, cWorld, cLantern, cPlace, cCup, cLanterns, cGrown, cAdults,
    cHatched, cJuvenile, cDays, cEvent, cEventDays, cHour, cDayMod, cVar, cVarEq, cBit, cLeague, cBeaten, cShows,
    cHollow, cCount, cMail, cRead, cGleam, cPouch, cPartner, cWearing, cTrue, cShowWon
};
enum EffectOp : u8 { eStart, eAdvance, eFinish, eFlag, eUnflag, eWorld, eVar, eAdd, eBit, eGleam, eFood, eWear, eLetter, eEvent, eStarEgg, eTake };

constexpr int kQuestTotal = static_cast<int>(sizeof(kQuests) / sizeof(kQuests[0]));
constexpr int kPersonTotal = static_cast<int>(sizeof(kPersons) / sizeof(kPersons[0]));
constexpr int kLetterTotal = kLetterIdCount;

const char* const kFeelNames[kFeels] = {"calm", "happy", "laugh", "excited", "surprised", "shock", "sad",
                                        "crying", "angry", "huff", "worried", "scared", "sleepy", "love",
                                        "proud", "cool", "shy", "thinking", "wistful", "dizzy"};
const char* const kLineNames[static_cast<int>(Line::Count)] = {"Main story", "Pageant", "Battle league",
                                                                "Frostspire Hollow", "Driftwood Cove", "Errands"};

bool validQuest(int q) { return q >= 0 && q < kQuestTotal; }
u8& slotOf(SaveData& s, int q) { return s.story.quest[kQuests[q].slot]; }
u8 slotOf(const SaveData& s, int q) { return s.story.quest[kQuests[q].slot]; }

const Dragon* partnerOf(const SaveData& s) {
    for (int i = 0; i < s.dragonCount; ++i)
        if (s.dragons[i].id == s.world.partnerId && s.dragons[i].stage != Stage::Egg) return &s.dragons[i];
    return nullptr;
}

bool allLanterns(const SaveData& s) {  // every lantern but the arena's (lit at the festival itself)
    for (int p = 0; p < kPlaceCount; ++p)
        if (world::placeInfo(p).lantern && p != kPlaceArena && !world::lanternLit(s, p)) return false;
    return true;
}

bool term(const SaveData& s, s64 now, const CondDef& c) {
    const StoryState& st = s.story;
    switch (c.op) {
        case cDone: return validQuest(static_cast<int>(c.a)) && slotOf(s, static_cast<int>(c.a)) == kQuestDone;
        case cBegun: return validQuest(static_cast<int>(c.a)) && slotOf(s, static_cast<int>(c.a)) != 0;
        case cActive: {
            if (!validQuest(static_cast<int>(c.a))) return false;
            const u8 at = slotOf(s, static_cast<int>(c.a));
            return at != 0 && at != kQuestDone;
        }
        case cNew: return validQuest(static_cast<int>(c.a)) && slotOf(s, static_cast<int>(c.a)) == 0;
        case cStep: return validQuest(static_cast<int>(c.a)) && slotOf(s, static_cast<int>(c.a)) == c.b;
        case cPast: {
            if (!validQuest(static_cast<int>(c.a))) return false;
            const u8 at = slotOf(s, static_cast<int>(c.a));
            return at == kQuestDone || at > c.b;
        }
        case cFlag: return flag(s, static_cast<int>(c.a));
        case cWorld: return (s.world.flags & c.a) != 0;
        case cLantern: return world::lanternLit(s, static_cast<int>(c.a));
        case cPlace: return world::placeFound(s, static_cast<int>(c.a));
        case cCup: return c.a < static_cast<u32>(kChallenges) && s.world.cups[c.a] >= c.b;
        case cLanterns: return allLanterns(s);
        case cGrown: {
            const Dragon* p = partnerOf(s);
            return (p && p->stage == Stage::Adult) || (s.world.flags & kFlagRode);
        }
        case cAdults: {
            u32 n = 0;
            for (int i = 0; i < s.dragonCount; ++i) n += s.dragons[i].stage == Stage::Adult;
            return n >= c.a;
        }
        case cHatched:
        case cJuvenile:
            for (int i = 0; i < s.dragonCount; ++i)
                if (s.dragons[i].stage >= (c.op == cHatched ? Stage::Hatchling : Stage::Juvenile)) return true;
            return false;
        case cDays: {
            if (!validQuest(static_cast<int>(c.a)) || slotOf(s, static_cast<int>(c.a)) != kQuestDone) return false;
            return dayIndex(now) - st.questDay[kQuests[c.a].slot] >= static_cast<s32>(c.b);
        }
        case cEvent: return c.a < static_cast<u32>(kMaxEvents) && st.eventDay[c.a] != 0;
        case cEventDays:
            return c.a < static_cast<u32>(kMaxEvents) && st.eventDay[c.a] != 0 &&
                   dayIndex(now) - st.eventDay[c.a] >= static_cast<s32>(c.b);
        case cHour: {
            const int h = hourOfDay(now);
            const int a = static_cast<int>(c.a), b = c.b;
            return a <= b ? h >= a && h < b : h >= a || h < b;
        }
        case cDayMod: return c.a > 0 && ((dayIndex(now) % static_cast<s32>(c.a)) + static_cast<s32>(c.a)) % static_cast<s32>(c.a) == c.b;
        case cVar: return c.a < static_cast<u32>(kMaxVars) && st.vars[c.a] >= c.b;
        case cVarEq: return c.a < static_cast<u32>(kMaxVars) && st.vars[c.a] == c.b;
        case cBit: return c.a < static_cast<u32>(kMaxVars) && (st.vars[c.a] & (1u << c.b));
        case cLeague: return s.progress.battleLeague >= c.a;
        case cBeaten: return c.a < static_cast<u32>(kLeagues) && (s.progress.battleBeaten[c.a] & (1u << c.b));
        case cShows: return s.progress.showLeague >= c.a;
        case cHollow: return s.progress.hollowDeepest >= c.a;
        case cCount: return c.a < static_cast<u32>(kRecordCounts) && s.progress.counts[c.a] >= c.b;
        case cMail: return c.a < static_cast<u32>(kLetterTotal) && letterDelivered(s, static_cast<int>(c.a));
        case cRead:
            return c.a < static_cast<u32>(kLetterTotal) && (s.story.mailRead >> kLetters[c.a].slot & 1u);
        case cGleam: return s.gleam >= c.a;
        case cPouch: return c.a < 10 && s.pouch[c.a] >= c.b;
        case cPartner: return partnerOf(s) != nullptr;
        case cWearing: {
            const Dragon* p = partnerOf(s);
            if (!p) return false;
            for (u8 w : p->wear)
                if (w == c.a) return true;
            return false;
        }
        case cTrue: return true;
        case cShowWon: return c.a < static_cast<u32>(kLeagues) && s.progress.showWon[c.a] != 0;
        default: return false;
    }
}

bool holds(const SaveData& s, s64 now, u16 from, u16 n) {
    for (u16 k = 0; k < n; ++k) {
        const CondDef& c = kConds[from + k];
        if (term(s, now, c) == (c.neg != 0)) return false;
    }
    return true;
}

bool questOpen(const SaveData& s, int q, s64 now) {
    return validQuest(q) && slotOf(s, q) == 0 && holds(s, now, kQuests[q].cond, kQuests[q].condN);
}

void applyEffects(SaveData& s, u16 from, u16 n, s64 now, News& news);

void finishQuestIn(SaveData& s, int q, s64 now, News& news) {
    if (!validQuest(q) || slotOf(s, q) == kQuestDone) return;
    const QuestDef& d = kQuests[q];
    slotOf(s, q) = kQuestDone;
    s.story.questDay[d.slot] = dayIndex(now);
    s.gleam += d.gleam;
    news.gleam += d.gleam;
    news.finished = q;
    applyEffects(s, d.eff, d.effN, now, news);
}

void advanceQuest(SaveData& s, int q, s64 now, News& news) {
    if (!validQuest(q)) return;
    u8& at = slotOf(s, q);
    if (at == 0 || at == kQuestDone) return;
    if (at >= kQuests[q].stepN) {
        finishQuestIn(s, q, now, news);
    } else {
        ++at;
        news.stepped = q;
    }
}

void startQuestIn(SaveData& s, int q, News& news) {
    if (!validQuest(q) || slotOf(s, q) != 0) return;
    slotOf(s, q) = 1;
    if (news.started < 0) news.started = q;
}

void deliverIn(SaveData& s, int l, News& news) {
    if (l < 0 || l >= kLetterTotal) return;
    const u64 bit = 1ull << kLetters[l].slot;
    if (s.story.mailIn & bit) return;
    s.story.mailIn |= bit;
    ++news.mail;
}

void applyEffects(SaveData& s, u16 from, u16 n, s64 now, News& news) {
    for (u16 k = 0; k < n; ++k) {
        const EffectDef& e = kEffects[from + k];
        switch (e.op) {
            case eStart: startQuestIn(s, static_cast<int>(e.a), news); break;
            case eAdvance: advanceQuest(s, static_cast<int>(e.a), now, news); break;
            case eFinish: finishQuestIn(s, static_cast<int>(e.a), now, news); break;
            case eFlag: setFlag(s, static_cast<int>(e.a), true); break;
            case eUnflag: setFlag(s, static_cast<int>(e.a), false); break;
            case eWorld: s.world.flags |= e.a; break;
            case eVar:
                if (e.a < static_cast<u32>(kMaxVars)) s.story.vars[e.a] = static_cast<u8>(e.b);
                break;
            case eAdd:
                if (e.a < static_cast<u32>(kMaxVars))
                    s.story.vars[e.a] = static_cast<u8>(s.story.vars[e.a] + e.b > 255 ? 255 : s.story.vars[e.a] + e.b);
                break;
            case eBit:
                if (e.a < static_cast<u32>(kMaxVars)) s.story.vars[e.a] = static_cast<u8>(s.story.vars[e.a] | (1u << e.b));
                break;
            case eGleam:
                s.gleam += e.a;
                news.gleam += e.a;
                break;
            case eFood:
                if (e.a < 10) s.pouch[e.a] = static_cast<u16>(s.pouch[e.a] + e.b > 999 ? 999 : s.pouch[e.a] + e.b);
                break;
            case eWear: trainer::giveAccessory(s, static_cast<int>(e.a)); break;
            case eLetter: deliverIn(s, static_cast<int>(e.a), news); break;
            case eEvent: markEvent(s, static_cast<int>(e.a), now); break;
            case eStarEgg:
                if (!(s.world.flags & kFlagStarEgg)) {
                    s.world.flags |= kFlagStarEgg;
                    news.starEgg = true;
                }
                break;
            case eTake:
                if (e.a < 10) s.pouch[e.a] = static_cast<u16>(s.pouch[e.a] > e.b ? s.pouch[e.a] - e.b : 0);
                break;
            default: break;
        }
    }
}

// A talk's lines: the rule's group for today (a rule that varies says something new each day), each
// line whose own condition holds.
Talk linesOf(const SaveData& s, s64 now, int person, u16 line, u16 lineN, u8 groups, u32 salt) {
    Talk t;
    t.person = static_cast<s8>(person);
    const u32 pick = groups > 1 ? (static_cast<u32>(dayIndex(now)) * 2654435761u + salt * 40503u) % groups : 0;
    for (u16 k = 0; k < lineN && t.count < kMaxLines; ++k) {
        const LineDef& l = kLines[line + k];
        if (l.group != pick || !holds(s, now, l.cond, l.condN)) continue;
        t.lines[t.count] = l.text;
        t.feel[t.count] = l.feel;
        t.speaker[t.count] = l.speaker;
        ++t.count;
    }
    return t;
}

int firstRule(const SaveData& s, int person, s64 now, Talk* out) {
    for (int r = 0; r < kRuleCount; ++r) {
        const RuleDef& d = kRules[r];
        if (d.person != person || !holds(s, now, d.cond, d.condN)) continue;
        Talk t = linesOf(s, now, person, d.line, d.lineN, d.groups, static_cast<u32>(r * 31 + person));
        if (t.count == 0) continue;  // (every line's own condition failed: the next rule)
        t.rule = static_cast<s16>(r);
        if (out) *out = t;
        return r;
    }
    return -1;
}

}  // namespace

// ---------------------------------------------------------------- feelings and lines
const char* feelName(Feel f) { return kFeelNames[static_cast<int>(f) < kFeels ? static_cast<int>(f) : 0]; }

Feel splitFeel(const char* line, const char** rest) {
    if (rest) *rest = line;
    if (!line || line[0] != '[') return Feel::Calm;
    const char* end = std::strchr(line, ']');
    if (!end) return Feel::Calm;
    const std::size_t n = static_cast<std::size_t>(end - line - 1);
    for (int f = 0; f < kFeels; ++f)
        if (std::strlen(kFeelNames[f]) == n && std::strncmp(line + 1, kFeelNames[f], n) == 0) {
            const char* after = end + 1;
            while (*after == ' ') ++after;
            if (rest) *rest = after;
            return static_cast<Feel>(f);
        }
    return Feel::Calm;
}

const char* lineName(Line l) { return kLineNames[static_cast<int>(l) < static_cast<int>(Line::Count) ? static_cast<int>(l) : 0]; }

void News::merge(const News& o) {
    if (o.started >= 0 && started < 0) started = o.started;
    if (o.stepped >= 0) stepped = o.stepped;
    if (o.finished >= 0) finished = o.finished;
    gleam += o.gleam;
    starEgg = starEgg || o.starEgg;
    mail += o.mail;
}

// ---------------------------------------------------------------- people
int personCount() { return kPersonTotal; }

const PersonInfo& person(int p) {
    static PersonInfo info[kPersonTotal];
    static bool ready = false;
    if (!ready) {
        for (int k = 0; k < kPersonTotal; ++k) {
            const PersonDef& d = kPersons[k];
            info[k] = {d.id, d.name, d.title, d.voice, d.pitch, d.villager, d.body, d.portrait, d.tint};
        }
        ready = true;
    }
    return info[p >= 0 && p < kPersonTotal ? p : 0];
}

int personOfVillager(Villager v) {
    for (int k = 0; k < kPersonTotal; ++k)
        if (kPersons[k].villager == static_cast<int>(v)) return k;
    return -1;
}

int findPerson(const char* id) {
    for (int k = 0; k < kPersonTotal; ++k)
        if (std::strcmp(kPersons[k].id, id) == 0) return k;
    return -1;
}

// ---------------------------------------------------------------- quests
int questCount() { return kQuestTotal; }

QuestView view(const SaveData& s, int q, s64 now) {
    QuestView v;
    if (!validQuest(q)) return v;
    const QuestDef& d = kQuests[q];
    const u8 at = slotOf(s, q);
    v.id = d.id;
    v.title = d.title;
    v.rumour = d.rumour;
    v.stepCount = d.stepN;
    v.started = at != 0;
    v.done = at == kQuestDone;
    v.stepIndex = v.done ? d.stepN : (at ? at - 1 : 0);
    v.step = kSteps[d.step + (v.done ? d.stepN - 1 : v.stepIndex)].text;
    v.open = at == 0 && holds(s, now, d.cond, d.condN);
    v.line = static_cast<Line>(d.line);
    v.giver = d.giver;
    return v;
}

int currentQuest(const SaveData& s) {
    for (int pass = 0; pass < 2; ++pass)
        for (int q = 0; q < kQuestTotal; ++q) {
            const u8 at = slotOf(s, q);
            if (at != 0 && at != kQuestDone && (pass == 1 || kQuests[q].line == static_cast<u8>(Line::Main))) return q;
        }
    return -1;
}

News update(SaveData& s, s64 now) {
    News news;
    for (int pass = 0; pass < 16; ++pass) {
        bool moved = false;
        for (int q = 0; q < kQuestTotal; ++q) {
            const QuestDef& d = kQuests[q];
            if (d.autoStart && questOpen(s, q, now)) {
                startQuestIn(s, q, news);
                moved = true;
            }
            const u8 at = slotOf(s, q);
            if (at == 0 || at == kQuestDone) continue;
            const StepDef& st = kSteps[d.step + at - 1];
            if (st.condN > 0 && holds(s, now, st.cond, st.condN)) {  // (what it asks for is true already)
                advanceQuest(s, q, now, news);
                moved = true;
            }
        }
        for (int l = 0; l < kLetterTotal; ++l) {
            const LetterDef& d = kLetters[l];
            if (d.condN == 0 || letterDelivered(s, l) || !holds(s, now, d.cond, d.condN)) continue;
            deliverIn(s, l, news);
            moved = true;
        }
        if (!moved) break;
    }
    return news;
}

// ---------------------------------------------------------------- talks
Talk talkTo(const SaveData& s, int person, s64 now) {
    Talk t;
    t.person = static_cast<s8>(person);
    firstRule(s, person, now, &t);
    return t;
}

bool hasImportantTalk(const SaveData& s, int person, s64 now) {
    const int r = firstRule(s, person, now, nullptr);
    return r >= 0 && kRules[r].important;
}

bool finishTalk(SaveData& s, const Talk& t, s64 now, News* newsOut) {
    News news;
    bool any = false;
    if (t.rule >= 0 && t.rule < kRuleCount) {
        const RuleDef& d = kRules[t.rule];
        applyEffects(s, d.eff, d.effN, now, news);
        any = d.effN > 0;
    }
    if (t.person >= 0 && t.person < kPersonTotal && t.pickup < 0 && !flag(s, kPersons[t.person].metFlag)) {
        setFlag(s, kPersons[t.person].metFlag, true);  // (met: a talk read to its end)
        any = true;
    }
    if (t.pickup >= 0 && t.pickup < kPickupCount) {
        const PickupDef& d = kPickups[t.pickup];
        applyEffects(s, d.eff, d.effN, now, news);
        any = any || d.effN > 0;
    }
    news.merge(update(s, now));
    if (newsOut) *newsOut = news;
    return any || news.started >= 0 || news.stepped >= 0 || news.finished >= 0 || news.mail > 0;
}

// ---------------------------------------------------------------- letters
bool letterDelivered(const SaveData& s, int l) {
    return l >= 0 && l < kLetterTotal && (s.story.mailIn >> kLetters[l].slot & 1u);
}

void deliver(SaveData& s, int l) {
    News n;
    deliverIn(s, l, n);
}

int mailbox(const SaveData& s, int* out, int cap) {
    int n = 0;
    for (int l = kLetterTotal - 1; l >= 0 && n < cap; --l)  // (later letters in the scripts come later in the game)
        if (letterDelivered(s, l)) out[n++] = l;
    return n;
}

int unreadMail(const SaveData& s) {
    int n = 0;
    for (int l = 0; l < kLetterTotal; ++l)
        if (letterDelivered(s, l) && !(s.story.mailRead >> kLetters[l].slot & 1u)) ++n;
    return n;
}

LetterView letter(const SaveData& s, int l) {
    LetterView v;
    if (l < 0 || l >= kLetterTotal) return v;
    const LetterDef& d = kLetters[l];
    v.id = d.id;
    v.subject = d.subject;
    v.from = d.from;
    for (u16 k = 0; k < d.lineN && v.count < 16; ++k) v.paragraphs[v.count++] = kLines[d.line + k].text;
    v.read = s.story.mailRead >> d.slot & 1u;
    return v;
}

bool readLetter(SaveData& s, int l, s64 now, News* newsOut) {
    News news;
    if (!letterDelivered(s, l)) return false;
    const LetterDef& d = kLetters[l];
    const u64 bit = 1ull << d.slot;
    const bool first = !(s.story.mailRead & bit);
    s.story.mailRead |= bit;
    if (first) applyEffects(s, d.eff, d.effN, now, news);
    news.merge(update(s, now));
    if (newsOut) *newsOut = news;
    return first;
}

// ---------------------------------------------------------------- spots and pickups
bool spotOf(const SaveData& s, int person, s64 now, Spot& out) {
    for (int k = 0; k < kSpotCount; ++k) {
        const SpotDef& d = kSpots[k];
        if (d.person != person || !holds(s, now, d.cond, d.condN)) continue;
        out.person = person;
        out.place = d.place;
        out.at = {d.x / 10.0f, d.y / 10.0f};
        out.facing = d.facing / 100.0f;
        out.clip = d.clip;
        return true;
    }
    return false;
}

int pickups(const SaveData& s, s64 now, Pickup* out, int cap) {
    int n = 0;
    for (int k = 0; k < kPickupCount && n < cap; ++k) {
        const PickupDef& d = kPickups[k];
        if (!d.id[0] || !holds(s, now, d.cond, d.condN)) continue;
        Pickup& p = out[n++];
        p.index = k;
        p.id = d.id;
        p.place = d.place;
        p.at = {d.x / 10.0f, d.y / 10.0f};
        p.prompt = d.prompt;
        p.sign = d.sign != 0;
        p.glint = d.glint != 0;
        p.group = d.group;
    }
    return n;
}

Talk pickupTalk(const SaveData& s, int pickup, s64 now) {
    Talk t;
    if (pickup < 0 || pickup >= kPickupCount) return t;
    const PickupDef& d = kPickups[pickup];
    t = linesOf(s, now, kPNarrator, d.line, d.lineN, 1, 0);
    t.pickup = static_cast<s16>(pickup);
    t.person = static_cast<s8>(kPNarrator);
    return t;
}

// ---------------------------------------------------------------- where
StepWhere stepWhere(const SaveData& s, int q) {
    StepWhere w;
    if (!validQuest(q)) return w;
    const QuestDef& d = kQuests[q];
    const u8 at = slotOf(s, q);
    if (at == 0 || at == kQuestDone) return w;
    const StepDef& st = kSteps[d.step + at - 1];
    w.kind = static_cast<Where>(st.where);
    w.a = st.w0;
    w.at = {st.w1 / 10.0f, st.w2 / 10.0f};
    w.radius = st.w3 / 10.0f;
    return w;
}

// ---------------------------------------------------------------- state
bool flag(const SaveData& s, int f) { return f >= 0 && f < kFlagBytes * 8 && (s.story.flags[f >> 3] & (1u << (f & 7))); }

void setFlag(SaveData& s, int f, bool on) {
    if (f < 0 || f >= kFlagBytes * 8) return;
    if (on)
        s.story.flags[f >> 3] = static_cast<u8>(s.story.flags[f >> 3] | (1u << (f & 7)));
    else
        s.story.flags[f >> 3] = static_cast<u8>(s.story.flags[f >> 3] & ~(1u << (f & 7)));
}

u8 var(const SaveData& s, int v) { return v >= 0 && v < kMaxVars ? s.story.vars[v] : 0; }
void setVar(SaveData& s, int v, u8 value) {
    if (v >= 0 && v < kMaxVars) s.story.vars[v] = value;
}

int questStep(const SaveData& s, int q) { return validQuest(q) ? slotOf(s, q) : 0; }
bool questDone(const SaveData& s, int q) { return validQuest(q) && slotOf(s, q) == kQuestDone; }

void startQuest(SaveData& s, int q) {
    News n;
    startQuestIn(s, q, n);
}

void finishQuest(SaveData& s, int q, s64 now, News* newsOut) {
    News n;
    if (validQuest(q) && slotOf(s, q) == 0) slotOf(s, q) = 1;
    finishQuestIn(s, q, now, n);
    if (newsOut) *newsOut = n;
}

void markEvent(SaveData& s, int e, s64 now) {
    if (e >= 0 && e < kMaxEvents && s.story.eventDay[e] == 0) s.story.eventDay[e] = dayIndex(now) ? dayIndex(now) : 1;
}

int stepTerms(const SaveData& s, int q, TermView* out, int cap) {
    static const char* const kOps[] = {"done", "begun", "active", "new", "step", "past", "flag", "world", "lantern",
                                       "place", "cup", "lanterns", "grown", "adults", "hatched", "juvenile", "days",
                                       "event", "eventdays", "hour", "daymod", "var", "vareq", "bit", "league",
                                       "beaten", "shows", "hollow", "count", "mail", "read", "gleam", "pouch",
                                       "partner", "wearing", "true", "showwon"};
    if (!validQuest(q)) return 0;
    const u8 at = slotOf(s, q);
    if (at == 0 || at == kQuestDone) return 0;
    const StepDef& st = kSteps[kQuests[q].step + at - 1];
    int n = 0;
    for (u16 k = 0; k < st.condN && n < cap; ++k) {
        const CondDef& c = kConds[st.cond + k];
        out[n++] = {c.op < sizeof(kOps) / sizeof(kOps[0]) ? kOps[c.op] : "?", c.neg != 0, c.a, c.b};
    }
    return n;
}

int pickupCount() { return kPickupCount; }

void migrate(SaveData& s, s64 now) {
    // Beta's eight quests (core/campaign's order, WorldState::quest) as their new selves: done stays
    // done (a week ago, so what follows them comes along); begun starts over at the first step, and
    // update() moves it on through everything already true.
    static const int kOld[8] = {kQKeepersApprentice, kQMarketDay, kQHilltop, kQMeadow,
                                kQColdHeights,       kQWings,     kQTrailhead, kQLanternFestival};
    const s32 today = dayIndex(now);
    for (int i = 0; i < 8; ++i) {
        const u8 at = s.world.quest[i];
        if (at == 0) continue;
        const int q = kOld[i];
        if (at == 0xFF) {
            slotOf(s, q) = kQuestDone;
            s.story.questDay[kQuests[q].slot] = today - 7;
        } else {
            slotOf(s, q) = 1;
        }
    }
    // The people met, as the world remembers them.
    const u32 w = s.world.flags;
    if (w & kFlagMetKeeper) setFlag(s, kFMetRowan);
    if (w & kFlagMetMarket) setFlag(s, kFMetMaple);
    if (w & kFlagMetSanctuary) setFlag(s, kFMetBram);
    if (w & kFlagMetSteward) setFlag(s, kFMetWren);
    if (w & kFlagMetTraveller) setFlag(s, kFMetSable);
    if (questDone(s, kQMarketDay)) {  // (Fig came in with Market day: met, and a friend)
        setFlag(s, kFMetFig);
        setFlag(s, kFFigFriend);
    }
    // A begun quest whose first step is a talk with someone already met: that step's done.
    for (int q = 0; q < kQuestTotal; ++q) {
        if (slotOf(s, q) != 1) continue;
        const StepDef& st = kSteps[kQuests[q].step];
        if (st.talk >= 0 && flag(s, kPersons[st.talk].metFlag) && kQuests[q].stepN > 1) slotOf(s, q) = 2;
    }
    setFlag(s, kFMigrated);  // (the "news from the valley" letter)
    update(s, now);
}

}  // namespace ec::story
