#include "core/guide.hpp"

#include <cmath>

#include "core/campaign.hpp"
#include "core/place_layout.hpp"
#include "core/save.hpp"
#include "core/valley.hpp"
#include "core/villagers.hpp"
#include "core/world.hpp"

namespace ec::guide {
namespace {

// Spots the game keeps elsewhere, in their places' frames (keep them in step): the challenges'
// notice boards (app/scene_challenge kBoards) and the stray in the meadow (app/scene_valley strayAt).
constexpr Vec2 kArenaBoard{-5.0f, 20.5f};
constexpr Vec2 kOrchardBoard{-3.5f, 11.0f};
constexpr Vec2 kStray{-46.0f, 58.0f};
constexpr float kStrayArea = 40.0f;    // metres: the flowers she hides in
constexpr float kHeightsArea = 60.0f;  // the cold heights' edges, to glide from
constexpr float kPlaceAreaMin = 60.0f; // a place not found yet: at least this wide a search (a map's 10 px)

Target spot(const Valley& v, int place, Vec2 local) {
    Target t;
    const ValleyPlaceInfo* p = v.place(static_cast<u8>(place));
    if (!p) return t;
    t.valid = true;
    t.at = placeToWorld(*p, local);
    t.place = place;
    return t;
}

Target area(const Valley& v, int place, Vec2 local, float radius) {
    Target t = spot(v, place, local);
    t.area = t.valid;
    t.radius = radius;
    return t;
}

Target villager(const Valley& v, Villager who) {
    const VillagerInfo& info = villagerInfo(who);
    return spot(v, info.place, info.at);
}

// A place: its door (or its middle) once found; before that a search area round it, its middle
// nudged off the place (a fixed way for each) so the circle doesn't give the spot away.
Target placeTarget(const SaveData& s, const Valley& v, int place) {
    const PlaceLayout& l = placeLayout(place);
    if (world::placeFound(s, place)) return spot(v, place, l.hasDoor ? l.door : Vec2{0, 0});
    const float r = std::fmin(75.0f, std::fmax(kPlaceAreaMin, world::placeInfo(place).findRadius * 2.5f));
    const float a = 2.39996f * static_cast<float>(place + 1);  // the golden angle: a different way for each
    return area(v, place, {std::cos(a) * r * 0.45f, std::sin(a) * r * 0.45f}, r);
}

Target lanternTarget(const SaveData& s, const Valley& v, int place) {
    const PlaceLayout& l = placeLayout(place);
    if (!world::placeFound(s, place)) return placeTarget(s, v, place);
    return spot(v, place, l.hasLantern ? Vec2{l.lantern.x, l.lantern.y} : Vec2{0, 0});
}

Target cupTarget(const Valley& v, u32 challenge) {
    if (challenge == static_cast<u32>(Challenge::FruitCatch)) return spot(v, kPlaceOrchard, kOrchardBoard);
    return spot(v, kPlaceArena, kArenaBoard);
}

// The festival's last lanterns: the nearest one still dark (the arena's is lit at the festival).
Target nearestUnlit(const SaveData& s, const Valley& v, Vec2 from) {
    Target best;
    float bestD = 1e30f;
    for (int p = 0; p < kPlaceCount; ++p) {
        if (!world::placeInfo(p).lantern || p == kPlaceArena || world::lanternLit(s, p)) continue;
        const Target t = lanternTarget(s, v, p);
        if (!t.valid) continue;
        const float d = std::hypot(t.at.x - from.x, t.at.y - from.y);
        if (d < bestD) {
            bestD = d;
            best = t;
        }
    }
    return best;
}

Target questTarget(const SaveData& s, const Valley& v, int quest, Vec2 from) {
    const campaign::StepNeed n = campaign::stepNeed(s, quest);
    switch (n.need) {
        case campaign::Need::Flag:
            switch (n.arg) {
                case kFlagEnteredValley: return placeTarget(s, v, kPlaceDen);
                case kFlagMetKeeper:
                case kFlagHeardStory: return villager(v, Villager::Keeper);
                case kFlagFoundStray: return area(v, kPlaceSanctuary, kStray, kStrayArea);
                case kFlagGlided: return area(v, kPlaceVault, {0, 0}, kHeightsArea);
                case kFlagMetTraveller: return villager(v, Villager::Traveller);
                case kFlagWandered: return placeTarget(s, v, kPlaceTrailhead);
                case kFlagFestival: return villager(v, Villager::Steward);
                default: return {};
            }
        case campaign::Need::Place: return placeTarget(s, v, static_cast<int>(n.arg));
        case campaign::Need::Lantern: return lanternTarget(s, v, static_cast<int>(n.arg));
        case campaign::Need::Cup: return cupTarget(v, n.arg);
        case campaign::Need::AllLanterns: return nearestUnlit(s, v, from);
        case campaign::Need::GrownPartner: return {};  // no place for it: time and care
    }
    return {};
}

}  // namespace

bool open(const SaveData& s, const Goal& g) {
    switch (g.kind) {
        case Tracked::Quest: {
            const campaign::QuestView q = campaign::view(s, g.id);
            return g.id >= 0 && g.id < campaign::questCount() && q.started && !q.done;
        }
        case Tracked::BattleBoard: return world::placeFound(s, kPlaceArena) && s.progress.battleLeague < kLeagues;
        case Tracked::ShowBoard: return world::placeFound(s, kPlaceGlade) && s.progress.showLeague < kLeagues;
        case Tracked::Hollow: return world::placeFound(s, kPlaceHollow);
        case Tracked::Place: return g.id >= 0 && g.id < kPlaceCount;
        default: return false;
    }
}

Goal current(const SaveData& s) {
    const Goal g{trainer::tracked(s), s.progress.trackId};
    if (g.kind != Tracked::None && open(s, g)) return g;
    const int q = campaign::currentQuest(s);
    if (q >= 0) return {Tracked::Quest, q};
    return {};
}

bool picked(const SaveData& s, const Goal& g) {
    return g.kind != Tracked::None && trainer::tracked(s) == g.kind && s.progress.trackId == g.id;
}

void toggle(SaveData& s, const Goal& g) {
    if (picked(s, g) || g.kind == Tracked::None)
        trainer::track(s, Tracked::None, 0);
    else
        trainer::track(s, g.kind, g.id);
}

int trackables(const SaveData& s, Goal* out, int cap) {
    int n = 0;
    auto add = [&](Goal g) {
        if (n < cap && open(s, g)) out[n++] = g;
    };
    for (int q = 0; q < campaign::questCount(); ++q) add({Tracked::Quest, q});
    add({Tracked::BattleBoard, 0});
    add({Tracked::ShowBoard, 0});
    add({Tracked::Hollow, 0});
    return n;
}

Target target(const SaveData& s, const Valley& v, const Goal& g, Vec2 from) {
    switch (g.kind) {
        case Tracked::Quest: return questTarget(s, v, g.id, from);
        // The leagues' boards and the Hollow's mouth (workstreams B and P may point these at the
        // next challenger or the day's show once they stand about the valley).
        case Tracked::BattleBoard: return spot(v, kPlaceArena, {0, 12});
        case Tracked::ShowBoard: return placeTarget(s, v, kPlaceGlade);
        case Tracked::Hollow: return placeTarget(s, v, kPlaceHollow);
        case Tracked::Place: return placeTarget(s, v, g.id);
        default: return {};
    }
}

}  // namespace ec::guide
