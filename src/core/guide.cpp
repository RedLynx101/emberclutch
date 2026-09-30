#include "core/guide.hpp"

#include <cmath>

#include "core/league.hpp"
#include "core/place_layout.hpp"
#include "core/save.hpp"
#include "core/story.hpp"
#include "core/valley.hpp"
#include "core/villagers.hpp"
#include "core/world.hpp"

namespace ec::guide {
namespace {

// Spots the game keeps elsewhere, in their places' frames (keep them in step): the challenges'
// notice boards (app/scene_challenge kBoards). (A quest's search areas, the stray's flowers and the
// cold heights' edges, are the story's now: story/*.story `where area`.)
constexpr Vec2 kArenaBoard{-5.0f, 20.5f};
constexpr Vec2 kOrchardBoard{-3.5f, 11.0f};
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

// Where a quest's step points (the story's `where`, D137): a place, its lantern, a challenge's board,
// someone (where the story stands them, else a villager's own spot), a search area, a spot, the
// nearest lantern still dark, or the nearest thing of a group to pick up.
Target questTarget(const SaveData& s, const Valley& v, int quest, Vec2 from, s64 now) {
    const story::StepWhere w = story::stepWhere(s, quest);
    switch (w.kind) {
        case story::Where::None: return {};
        case story::Where::Place: return placeTarget(s, v, w.a);
        case story::Where::Lantern: return lanternTarget(s, v, w.a);
        case story::Where::Cup: return cupTarget(v, static_cast<u32>(w.a));
        case story::Where::Person: {
            story::Spot sp;
            if (story::spotOf(s, w.a, now, sp)) return spot(v, sp.place, sp.at);
            const int villagerNo = story::person(w.a).villager;
            if (villagerNo >= 0) return villager(v, static_cast<Villager>(villagerNo));
            return {};
        }
        case story::Where::Area: return area(v, w.a, w.at, w.radius);
        case story::Where::Spot: return spot(v, w.a, w.at);
        case story::Where::Unlit: return nearestUnlit(s, v, from);
        case story::Where::Group: {
            story::Pickup found[24];
            const int n = story::pickups(s, now, found, 24);
            Target best;
            float bestD = 1e30f;
            for (int k = 0; k < n; ++k) {
                if (found[k].group != w.a) continue;
                const Target t = spot(v, found[k].place, found[k].at);
                const float d = std::hypot(t.at.x - from.x, t.at.y - from.y);
                if (t.valid && d < bestD) {
                    bestD = d;
                    best = t;
                }
            }
            return best;
        }
    }
    return {};
}

}  // namespace

bool open(const SaveData& s, const Goal& g) {
    switch (g.kind) {
        case Tracked::Quest: {
            const int at = story::questStep(s, g.id);
            return g.id >= 0 && g.id < story::questCount() && at != 0 && at != story::kQuestDone;
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
    const int q = story::currentQuest(s);
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
    for (int q = 0; q < story::questCount(); ++q) add({Tracked::Quest, q});
    add({Tracked::BattleBoard, 0});
    add({Tracked::ShowBoard, 0});
    add({Tracked::Hollow, 0});
    return n;
}

Target target(const SaveData& s, const Valley& v, const Goal& g, Vec2 from, s64 now) {
    switch (g.kind) {
        case Tracked::Quest: return questTarget(s, v, g.id, from, now);
        // The league: the challenger to battle next (the one tracked from the board, else the first
        // not yet beaten, then the champion at the caldera); the shows' glade; the Hollow's mouth.
        case Tracked::BattleBoard: {
            const int id = league::nextChallenger(s);
            return spot(v, league::challenger(id).place, league::spotOf(id));
        }
        case Tracked::ShowBoard: return placeTarget(s, v, kPlaceGlade);
        case Tracked::Hollow: return placeTarget(s, v, kPlaceHollow);
        case Tracked::Place: return placeTarget(s, v, g.id);
        default: return {};
    }
}

}  // namespace ec::guide
