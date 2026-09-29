#include "core/villagers.hpp"

#include <cstdio>
#include <cstring>

#include "core/campaign.hpp"
#include "core/valley.hpp"
#include "core/world.hpp"

namespace ec {
namespace {

// Voices (Noah, 2026-09-28): 0 is the alphabet made from his own voice, for the men; 1 the
// recording he sent for the women and the child (romfs/voice/v1, v2).
const VillagerInfo kVillagers_[kVillagers] = {
    {"keeper", "Old Rowan", "the valley's keeper", kPlaceKeeper, {2.5f, 6.0f}, 0.0f, 0, 1.2f},
    {"market", "Maple", "keeps the Market", kPlaceMarket, {3.0f, -2.0f}, 0.3f, 1, 1.45f},
    {"sanctuary", "Bram", "keeps the Sanctuary", kPlaceSanctuary, {-2.0f, 8.0f}, 0.0f, 0, 1.45f},
    {"steward", "Wren", "the arena's steward", kPlaceArena, {4.0f, 24.0f}, 0.0f, 1, 1.35f},
    {"child", "Pip", "loves dragons", kPlaceMarket, {-8.0f, 6.0f}, -0.6f, 1, 1.8f},
    {"traveller", "Sable", "a traveller", kPlaceTrailhead, {-4.0f, 5.0f}, 0.4f, 0, 1.35f},
};

Talk lines(std::initializer_list<const char*> l, u32 sets = 0) {
    Talk t;
    for (const char* line : l)
        if (t.count < kMaxLines) t.lines[t.count++] = line;
    t.sets = sets;
    return t;
}

int stepOf(const SaveData& s, int quest) {
    const campaign::QuestView v = campaign::view(s, quest);
    return v.done ? 99 : (v.started ? v.stepIndex : -1);
}

// The festival night (the Lantern Festival's last step): the star dragon comes down.
Talk festival() {
    return lines({"Look up, {P}. Every lantern in the valley is burning, all the way to the isles.",
                  "There! Over the lake... it's the star dragon. It found its way back.",
                  "It's coming down to the great lantern. Oh! It left something in the light.",
                  "An egg. A star-born egg, for the keeper who lit the way. That's you now.",
                  "Happy Lantern Festival. Take good care of it, and of {D}."},
                 kFlagFestival);
}

Talk keeper(const SaveData& s) {
    const WorldState& w = s.world;
    if (!(w.flags & kFlagMetKeeper))
        return lines({"Oh! A new keeper, and a young dragon with you. Welcome to Skyreach Valley, {P}.",
                      "I'm Rowan. I've kept the dragons here since the falls were a trickle.",
                      "The Lantern Festival is almost here, but the lanterns have all gone dark. Every one.",
                      "A dragon's breath can light them again. {D} has a warm little breath, I can tell.",
                      "Start with the lantern by your den. Then come and tell me."},
                     kFlagMetKeeper);
    if (stepOf(s, 2) == 2 && !(w.flags & kFlagHeardStory))
        return lines({"You lit the hilltop lantern! Sit a moment, and I'll tell you why they matter.",
                      "Long ago, a dragon fell from the stars and landed on our hill, lost and tired.",
                      "We lit every lantern in the valley so it could find its way home, and it did.",
                      "Ever since, the festival lights the way for it. Some say it still comes back to look."},
                     kFlagHeardStory);
    if (stepOf(s, 0) < 99)
        return lines({"The lantern by your den first. Stand by it and let {D} breathe on it."});
    if (stepOf(s, 1) >= 0 && stepOf(s, 1) < 99)
        return lines({"Maple at the Market could use a hand. And her lantern needs lighting too."});
    if (stepOf(s, 7) == 2)
        return festival();
    if (stepOf(s, 7) >= 1 && stepOf(s, 7) < 99)
        return lines({"Every lantern's alight but the great one. Win the Lantern Trial at the arena!",
                      "Wren will set it up. I'll be there, in the front row."});
    if (stepOf(s, 7) == 99)
        return lines({"You're the valley's keeper now, {P}. I'll just sit on my porch and watch you two fly."});
    return lines({"Every lantern you light, the valley feels a little more awake.",
                  "The Nesting Stone, the meadow, the cold heights... they're all waiting."});
}

Talk market(const SaveData& s) {
    const WorldState& w = s.world;
    if (!(w.flags & kFlagMetMarket))
        return lines({"Welcome, welcome! I'm Maple. Food, toys, treasures, and an egg of the day!",
                      "Say, you look like someone who's good at catching things.",
                      "The orchard's fruit is ripe. A Fruit Catch at the orchard would help me no end.",
                      "Win one and I'll light the village lantern with you. Deal?"},
                     kFlagMetMarket);
    if (w.cups[static_cast<int>(Challenge::FruitCatch)] == 0)
        return lines({"The orchard's just east of here. Your dragon catches, you cheer!"});
    return lines({"Look at all this fruit! Thank you, {P}. The stalls have new things every day, you know.",
                  "Come by tomorrow and there'll be something else on the goods stall."});
}

Talk sanctuary(const SaveData& s) {
    const WorldState& w = s.world;
    if (!(w.flags & kFlagMetSanctuary))
        return lines({"Easy there, easy. Oh, hello. I'm Bram. I look after the dragons who rest here.",
                      "One of the little ones wandered off into the tall flowers this morning.",
                      "I can't find her anywhere. Maybe {D}'s nose could sniff her out?"},
                     kFlagMetSanctuary);
    if (!(w.flags & kFlagFoundStray))
        return lines({"Take {D} through the flowers in the meadow, walking or on its back.",
                      "Its nose will catch her scent when you're close."});
    return lines({"You found her! She's napping in the hay now, happy as anything.",
                  "Thank you. Any dragon of yours can rest here whenever the den's too full."});
}

Talk steward(const SaveData& s) {
    const WorldState& w = s.world;
    if (stepOf(s, 7) == 2) {  // the festival night comes first, met or not
        Talk t = festival();
        t.sets |= kFlagMetSteward;
        return t;
    }
    if (!(w.flags & kFlagMetSteward))
        return lines({"A new challenger! I'm Wren, the arena's steward.",
                      "Sky Rings, the Lantern Trial and Fruit Catch: four cups each, Ember to Starfire.",
                      "Ribbons for the den, trophies, Gleam... and bragging rights, of course."},
                     kFlagMetSteward);
    if (stepOf(s, 7) >= 1 && stepOf(s, 7) < 99)
        return lines({"The Lantern Trial! Every lantern in the valley's lit, so tonight's the night.",
                      "Breathe true, and the great lantern will light the whole valley."});
    return lines({"Ready when you are. Pick a challenge on the board."});
}

Talk child(const SaveData& s) {
    if (stepOf(s, 5) >= 2)
        return lines({"I saw it! I saw the star dragon over the floating isles! It was all sparkly!"});
    if (s.world.flags & kFlagRode)
        return lines({"You RODE a dragon?! When I grow up I'm going to fly all the way to the isles."});
    return lines({"Is that your dragon? Can I pet it? Hi, {D}!",
                  "Did you know the floating isles have a lantern on top? Only dragons can get up there."});
}

Talk traveller(const SaveData& s) {
    const WorldState& w = s.world;
    if (!(w.flags & kFlagMetTraveller))
        return lines({"Evening. I'm Sable. I walk the long trails and see what the valley's hiding.",
                      "Out past the gap there's a meadow where star shards fall.",
                      "Send {D} on a Wandering from the trailhead. Dragons have a nose for shards."},
                     kFlagMetTraveller);
    if (!(w.flags & kFlagWandered))
        return lines({"The trail starts right here. Take a Wandering and see what {D} brings back."});
    return lines({"A star shard! Hold it up to the lantern here and it'll light itself.",
                  "The star dragon must be near. The festival's going to be something this year."});
}

}  // namespace

const VillagerInfo& villagerInfo(Villager v) {
    return kVillagers_[static_cast<int>(v) < kVillagers ? static_cast<int>(v) : 0];
}

Talk talkTo(const SaveData& s, Villager v) {
    switch (v) {
        case Villager::Keeper: return keeper(s);
        case Villager::Market: return market(s);
        case Villager::Sanctuary: return sanctuary(s);
        case Villager::Steward: return steward(s);
        case Villager::Child: return child(s);
        case Villager::Traveller: return traveller(s);
        default: return Talk{};
    }
}

bool finishTalk(SaveData& s, Villager v, const Talk& t) {
    (void)v;
    const u32 before = s.world.flags;
    s.world.flags |= t.sets;
    return s.world.flags != before;
}

void fillLine(const char* line, const SaveData& s, char* out, int cap) {
    const char* partner = "your dragon";
    for (int i = 0; i < s.dragonCount; ++i)
        if (s.dragons[i].id == s.world.partnerId && s.dragons[i].name[0]) partner = s.dragons[i].name;
    const char* you = s.playerName[0] ? s.playerName : "friend";
    int n = 0;
    for (const char* p = line; *p && n < cap - 1; ++p) {
        if (p[0] == '{' && (p[1] == 'D' || p[1] == 'P') && p[2] == '}') {
            for (const char* q = p[1] == 'D' ? partner : you; *q && n < cap - 1; ++q) out[n++] = *q;
            p += 2;
        } else {
            out[n++] = *p;
        }
    }
    out[n] = 0;
}

}  // namespace ec
