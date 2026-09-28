#include "core/campaign.hpp"

#include "core/valley.hpp"
#include "core/world.hpp"

namespace ec::campaign {
namespace {

struct Step {
    const char* text;
    Need need;
    u32 arg;  // the flag, the place, or the challenge
};

struct Quest {
    const char* title;
    Step steps[3];
    int after[3];  // quests that must be done first (-1: none)
    u32 gleam;
};

const Quest kQuests_[kQuests] = {
    {"The keeper's apprentice",
     {{"Head out of the den with your dragon", Need::Flag, kFlagEnteredValley},
      {"Meet the old keeper by the waterfall", Need::Flag, kFlagMetKeeper},
      {"Light the lantern by your den", Need::Lantern, kPlaceDen}},
     {-1, -1, -1}, 60},
    {"Market day",
     {{"Find the Market village", Need::Place, kPlaceMarket},
      {"Help the Market's keeper: win a Fruit Catch", Need::Cup, static_cast<u32>(Challenge::FruitCatch)},
      {"Light the Market's lantern", Need::Lantern, kPlaceMarket}},
     {0, -1, -1}, 80},
    {"The hilltop",
     {{"Climb to the Nesting Stone", Need::Place, kPlaceStone},
      {"Light the hilltop lantern", Need::Lantern, kPlaceStone},
      {"Hear the keeper's story", Need::Flag, kFlagHeardStory}},
     {1, -1, -1}, 80},
    {"The meadow",
     {{"Visit the Sanctuary meadow", Need::Place, kPlaceSanctuary},
      {"Find the stray: let your dragon sniff it out", Need::Flag, kFlagFoundStray},
      {"Light the meadow's lantern", Need::Lantern, kPlaceSanctuary}},
     {1, -1, -1}, 80},
    {"The cold heights",
     {{"Climb the path to the Cold Vault", Need::Place, kPlaceVault},
      {"Glide down from the heights", Need::Flag, kFlagGlided},
      {"Light the Vault's lantern", Need::Lantern, kPlaceVault}},
     {1, -1, -1}, 100},
    {"Wings",
     {{"Grow up together: a grown dragon to ride", Need::GrownPartner, 0},
      {"Fly the Sky Rings", Need::Cup, static_cast<u32>(Challenge::SkyRings)},
      {"Light the high lantern on the floating isles", Need::Lantern, kPlaceIsles}},
     {4, -1, -1}, 150},
    {"The trailhead",
     {{"Meet the traveller at the trailhead", Need::Flag, kFlagMetTraveller},
      {"Send your dragon on a Wandering", Need::Flag, kFlagWandered},
      {"Light the traveller's lantern", Need::Lantern, kPlaceTrailhead}},
     {3, -1, -1}, 100},
    {"The Lantern Festival",
     {{"Light every lantern in the valley", Need::AllLanterns, 0},
      {"Win the Lantern Trial at the arena", Need::Cup, static_cast<u32>(Challenge::LanternTrial)},
      {"The festival night", Need::Flag, kFlagFestival}},
     {2, 5, 6}, 300},
};

bool met(const SaveData& s, const Step& st) {
    const WorldState& w = s.world;
    switch (st.need) {
        case Need::Flag: return (w.flags & st.arg) != 0;
        case Need::Place: return world::placeFound(s, static_cast<int>(st.arg));
        case Need::Lantern: return world::lanternLit(s, static_cast<int>(st.arg));
        case Need::Cup: return st.arg < static_cast<u32>(kChallenges) && w.cups[st.arg] > 0;
        case Need::GrownPartner:
            for (int i = 0; i < s.dragonCount; ++i)
                if (s.dragons[i].id == w.partnerId && s.dragons[i].stage == Stage::Adult) return true;
            return (w.flags & kFlagRode) != 0;
        case Need::AllLanterns: {  // every lantern but the arena's (lit at the festival itself)
            for (int p = 0; p < kPlaceCount; ++p)
                if (world::placeInfo(p).lantern && p != kPlaceArena && !world::lanternLit(s, p)) return false;
            return true;
        }
    }
    return false;
}

bool ready(const SaveData& s, const Quest& q) {
    for (int a : q.after)
        if (a >= 0 && s.world.quest[a] != kQuestDone) return false;
    return true;
}

}  // namespace

int questCount() { return kQuests; }

QuestView view(const SaveData& s, int q) {
    QuestView v;
    if (q < 0 || q >= kQuests) return v;
    const Quest& quest = kQuests_[q];
    const u8 at = s.world.quest[q];
    v.title = quest.title;
    v.stepCount = 3;
    v.started = at != 0;
    v.done = at == kQuestDone;
    v.stepIndex = v.done ? 3 : (at ? at - 1 : 0);
    v.step = quest.steps[v.done ? 2 : v.stepIndex].text;
    return v;
}

News update(SaveData& s) {
    News news;
    for (int pass = 0; pass < kQuests * 4; ++pass) {  // until nothing moves (a step can chain into the next)
        bool moved = false;
        for (int q = 0; q < kQuests; ++q) {
            u8& at = s.world.quest[q];
            const Quest& quest = kQuests_[q];
            if (at == kQuestDone) continue;
            if (at == 0) {
                if (!ready(s, quest)) continue;
                at = 1;
                if (news.started < 0) news.started = q;
                moved = true;
            }
            if (met(s, quest.steps[at - 1])) {
                if (at == 3) {
                    at = kQuestDone;
                    s.gleam += quest.gleam;
                    news.gleam += quest.gleam;
                    news.finished = q;
                    if (q == kQuests - 1 && !(s.world.flags & kFlagStarEgg)) {
                        s.world.flags |= kFlagStarEgg;
                        news.starEgg = true;
                    }
                } else {
                    ++at;
                    news.stepped = q;
                }
                moved = true;
            }
        }
        if (!moved) break;
    }
    return news;
}

int currentQuest(const SaveData& s) {
    for (int q = 0; q < kQuests; ++q)
        if (s.world.quest[q] != 0 && s.world.quest[q] != kQuestDone) return q;
    return -1;
}

StepNeed stepNeed(const SaveData& s, int q) {
    if (q < 0 || q >= kQuests) return {};
    const u8 at = s.world.quest[q];
    const Step& st = kQuests_[q].steps[at == 0 || at == kQuestDone ? 0 : at - 1];
    return {st.need, st.arg};
}

}  // namespace ec::campaign
