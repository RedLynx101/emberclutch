// The roaming trainers (core/roamers, workstream D): their table, the day's roster, the paths as
// a network on the real valley (junctions, stops short of the places, the crossings round the
// places' walls), a day's walk (continuous, on the paths, sitting at viewpoints, never stopping in
// the Market), and the friendly duels (fair levels, balance, the day's Gleam, the record). And the
// villagers' doings by the hour (core/routines).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/anim.hpp"
#include "core/battle.hpp"
#include "core/clock.hpp"
#include "core/kinds.hpp"
#include "core/people.hpp"
#include "core/place_layout.hpp"
#include "core/roamers.hpp"
#include "core/routines.hpp"
#include "core/save.hpp"
#include "core/trainer.hpp"
#include "core/valley.hpp"
#include "core/villagers.hpp"

using namespace ec;

namespace {

const Valley& roamValley() {
    static Valley v;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        FILE* f = std::fopen("../romfs/valley/skyreach.evl", "rb");
        if (!f) return v;
        std::vector<u8> data;
        u8 buf[4096];
        std::size_t n;
        while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) data.insert(data.end(), buf, buf + n);
        std::fclose(f);
        loadValley(data.data(), data.size(), v);
        addPlaceDecks(v);  // (the mill's bridge, as the game)
    }
    return v;
}

// The places' walls and the villagers, as the game stages them.
const std::vector<Solid>& roamSolids() {
    static std::vector<Solid> s;
    if (s.empty()) {
        const Valley& v = roamValley();
        s = worldSolids(v);
        for (int k = 0; k < kVillagers; ++k) {
            const VillagerInfo& info = villagerInfo(static_cast<Villager>(k));
            if (const ValleyPlaceInfo* p = v.place(static_cast<u8>(info.place))) s.push_back({placeToWorld(*p, info.at), 0.6f});
        }
    }
    return s;
}

const roam::PathNet& roamNet() {
    static roam::PathNet net;
    if (!net.ok()) roam::buildNet(roamValley(), roamSolids(), net);
    return net;
}

float gap(Vec2 p) {  // how far clear of the nearest wall
    float best = 1e9f;
    for (const Solid& s : roamSolids()) best = std::fmin(best, std::hypot(p.x - s.at.x, p.y - s.at.y) - s.radius);
    return best;
}

Dragon grown(int kind, int level, int trained, u32 seed) {
    Rng rng(seed * 2654435761u + 5);
    Dragon d;
    d.id = seed;
    d.stage = Stage::Adult;
    rollKind(d, kind, 0, rng);
    d.xp = trainer::xpForLevel(level);
    for (u8& t : d.trained) t = static_cast<u8>(trained);
    return d;
}

}  // namespace

TEST(roamers_table) {
    std::set<std::string> names, dragons;
    for (int id = 0; id < roam::kRoamers; ++id) {
        const roam::Roamer& r = roam::roamer(id);
        CHECK(names.insert(r.name).second && dragons.insert(r.dragonName).second);
        CHECK(r.title && r.greet[0] && r.greet[1] && r.hello[0] && r.hello[1] && r.again && r.won && r.lost);
        CHECK(std::strlen(r.greet[0]) <= 28 && std::strlen(r.greet[1]) <= 28);  // (a bubble's worth)
        CHECK(findKind(r.kind) >= 0 && r.variant < kKindVariants && r.skill <= 3 && r.edge >= -1 && r.edge <= 1);
        CHECK(r.pace >= 0.8f && r.pace <= 1.6f && r.voice <= 1 && r.pitch > 0.9f && r.pitch < 2.2f);
        const Person body = static_cast<Person>(r.person);
        CHECK(r.person < kPeople);
        const bool player = body == Person::PlayerA || body == Person::PlayerB;
        CHECK(player ? r.look.hair < kHairStyles : r.look.hair == roam::kNoHair);  // (a villager's body keeps its own)
        Rgb pal[kPalCount];
        roam::palette(id, pal);
        CHECK(pal[kPalBase].r == r.look.skin.r && pal[kPalAccent].g == r.look.outfit.g && pal[kPalHorn].b == r.look.hairColour.b);
    }
    // Voices: the men on Noah's recording, the women and children on the other.
    CHECK(roam::roamer(2).voice == 0 && roam::roamer(3).voice == 1);
}

TEST(roamers_roster) {
    int seen[roam::kRoamers] = {};
    int sizes[roam::kMaxOut + 1] = {};
    for (s32 day = 20000; day < 20400; ++day) {
        u8 ids[roam::kMaxOut];
        const int n = roam::roster(day, ids);
        CHECK(n >= roam::kMinOut && n <= roam::kMaxOut);
        ++sizes[n];
        std::set<int> distinct;
        for (int k = 0; k < n; ++k) {
            CHECK(ids[k] < roam::kRoamers && distinct.insert(ids[k]).second);
            CHECK(roam::isOut(day, ids[k]));
            ++seen[ids[k]];
        }
        u8 again[roam::kMaxOut];
        CHECK(roam::roster(day, again) == n && std::memcmp(ids, again, static_cast<std::size_t>(n)) == 0);  // the same all day
    }
    for (int id = 0; id < roam::kRoamers; ++id) CHECK(seen[id] > 100);  // everyone gets out
    CHECK(sizes[3] > 60 && sizes[4] > 60 && sizes[5] > 60);
    CHECK(!roam::walkingHour(7) && roam::walkingHour(8) && roam::walkingHour(19) && !roam::walkingHour(20));
    const s64 t = 20000LL * kDay + 9 * kHour + 30;
    CHECK(std::fabs(roam::dayClock(t) - (3600.0f + 30.0f)) < 0.01f);
}

TEST(roamers_paths_network) {
    const Valley& v = roamValley();
    const roam::PathNet& net = roamNet();
    CHECK(net.ok() && net.nodes.size() >= 12 && net.edges.size() >= v.paths.size());
    int ends = 0, market = 0, rounded = 0;
    for (std::size_t k = 0; k < net.nodes.size(); ++k) {
        const roam::NetNode& n = net.nodes[k];
        CHECK(!net.links[k].empty());
        ends += n.end;
        market += n.market;
        rounded += !n.end && n.round > 0;
        if (n.end && n.place >= 0) {  // a path's end at a place: stopped short, on open ground
            CHECK(std::hypot(n.at.x - n.view.x, n.at.y - n.view.y) >= 8.0f);
            CHECK(gap(n.at) > 1.0f);
        }
    }
    std::printf("  %zu junctions (%d ends, %d walked round), %zu stretches\n", net.nodes.size(), ends, rounded, net.edges.size());
    CHECK(ends >= 6 && market == 1);
    // Every stretch and every crossing round a junction keeps clear of the walls.
    int close = 0, samples = 0;
    float worst = 1e9f;
    for (const roam::NetEdge& e : net.edges) {
        CHECK(e.length > 1.0f && e.pts.size() >= 2 && e.cum.size() == e.pts.size());
        for (float d = 0; d <= e.length; d += 1.0f) {
            const Vec2 p = roam::along(e.pts, e.cum, d);
            const float g = gap(p);
            ++samples;
            worst = std::fmin(worst, g);
            if (g < 0.2f) {
                if (close < 60) {
                    const Solid* w = nullptr;
                    for (const Solid& s : roamSolids())
                        if (!w || std::hypot(p.x - s.at.x, p.y - s.at.y) - s.radius < std::hypot(p.x - w->at.x, p.y - w->at.y) - w->radius) w = &s;
                    std::printf("  near a wall at (%.1f %.1f): %.2f m (wall at %.1f %.1f r %.1f)\n", p.x, p.y, g, w->at.x, w->at.y, w->radius);
                }
                ++close;
            }
        }
    }
    for (std::size_t k = 0; k < net.nodes.size(); ++k) {
        const std::vector<int>& links = net.links[k];
        CHECK(net.nodes[k].cross.size() == links.size() * (links.size() - 1) / 2);
        for (std::size_t a = 0; a < links.size(); ++a)
            for (std::size_t b = 0; b < links.size(); ++b) {
                if (a == b) continue;
                auto endAt = [&](int ei) {
                    const roam::NetEdge& e = net.edges[static_cast<std::size_t>(ei)];
                    return e.a == static_cast<int>(k) ? e.pts.front() : e.pts.back();
                };
                // The crossing runs from the one stretch's end to the other's.
                const std::vector<Vec2> c = roam::crossing(net, static_cast<int>(k), links[a], links[b]);
                CHECK(c.size() >= 2);
                if (c.size() < 2) continue;
                const Vec2 p = endAt(links[a]), q = endAt(links[b]);
                CHECK(std::hypot(c.front().x - p.x, c.front().y - p.y) < 0.01f && std::hypot(c.back().x - q.x, c.back().y - q.y) < 0.01f);
                std::vector<float> cum(c.size(), 0.0f);
                for (std::size_t i = 1; i < c.size(); ++i) cum[i] = cum[i - 1] + std::hypot(c[i].x - c[i - 1].x, c[i].y - c[i - 1].y);
                for (float d = 0; d <= cum.back(); d += 0.5f) {
                    const Vec2 m = roam::along(c, cum, d);
                    const float g = gap(m);
                    ++samples;
                    worst = std::fmin(worst, g);
                    if (g < 0.2f) {
                        if (close < 60)
                            std::printf("  crossing at junction %d (%d to %d, %d points) near a wall at (%.1f %.1f): %.2f m\n",
                                        static_cast<int>(k), links[a], links[b], static_cast<int>(c.size()), m.x, m.y, g);
                        ++close;
                    }
                }
            }
    }
    std::printf("  %d of %d points near a wall (closest %.2f m)\n", close, samples, worst);
    if (close) {  // (what the junctions look like, to see why)
        for (std::size_t k = 0; k < net.nodes.size(); ++k) {
            const roam::NetNode& n = net.nodes[k];
            std::printf("  junction %d at (%.1f %.1f) place %d round %.0f:", static_cast<int>(k), n.at.x, n.at.y, n.place, n.round);
            for (int ei : net.links[k]) {
                const roam::NetEdge& e = net.edges[static_cast<std::size_t>(ei)];
                const Vec2 p = e.a == static_cast<int>(k) ? e.pts.front() : e.pts.back();
                std::printf(" e%d (%.1f %.1f)", ei, p.x, p.y);
            }
            std::printf("\n");
        }
        const std::vector<Vec2> c = roam::crossing(net, 2, 1, 4);
        for (std::size_t i = 0; i < c.size(); i += 4) std::printf("   (%.1f %.1f) gap %.2f\n", c[i].x, c[i].y, gap(c[i]));
    }
    CHECK(close == 0);
}

TEST(roamers_a_days_walk) {
    const Valley& v = roamValley();
    const roam::PathNet& net = roamNet();
    const ValleyPlaceInfo* market = v.place(kPlaceMarket);
    CHECK(market != nullptr);
    if (!market || !net.ok()) return;
    const float day = (roam::kOutUntil - roam::kOutFrom) * 3600.0f;
    for (int id = 0; id < roam::kRoamers; ++id) {
        for (s32 d : {20000, 20001, 20417}) {
            roam::Walk w;
            roam::startWalk(net, id, d, w);
            roam::Pose last = roam::walkAt(net, w, 0.0f);
            float jump = 0, marketStill = 0, sat = 0, walked = 0, wet = 0;
            std::set<int> places;
            bool offMap = false, sitAway = false;
            for (float t = 0.5f; t < day; t += 0.5f) {
                const roam::Pose p = roam::walkAt(net, w, t);
                const float step = std::hypot(p.at.x - last.at.x, p.at.y - last.at.y);
                jump = std::fmax(jump, step);
                walked += step;
                if (!v.inside(p.at.x, p.at.y)) offMap = true;
                if (!p.walking && std::hypot(p.at.x - market->at.x, p.at.y - market->at.y) < 45.0f) marketStill += 0.5f;
                if (p.sitting) {
                    sat += 0.5f;
                    if (p.node < 0 || !net.nodes[static_cast<std::size_t>(p.node)].end) sitAway = true;
                }
                if (!p.walking && p.node >= 0 && net.nodes[static_cast<std::size_t>(p.node)].place >= 0)
                    places.insert(net.nodes[static_cast<std::size_t>(p.node)].place);
                const float ground = v.groundAt(p.at.x, p.at.y, v.water + 2.0f);
                if (ground < v.water - 0.5f) wet += 0.5f;  // (wading a ford is fine; swimming isn't)
                last = p;
            }
            CHECK(jump <= roam::roamer(id).pace * 0.5f + 0.02f);  // never a jump: every crossing walked
            CHECK(!offMap && !sitAway && marketStill == 0.0f);
            CHECK(sat > 600.0f);            // sat at viewpoints a good while over the day
            CHECK(places.size() >= 4);      // and saw a good part of the valley
            CHECK(wet < 60.0f);
            if (d == 20000 && id < 3)
                std::printf("  %s: %.1f km, %d places looked at, sat %.0f min\n", roam::roamer(id).name, walked / 1000.0f,
                            static_cast<int>(places.size()), sat / 60.0f);
        }
        // The same walk whether stepped through or asked for at once; a clock gone back starts over.
        roam::Walk a, b;
        roam::startWalk(net, id, 20000, a);
        roam::startWalk(net, id, 20000, b);
        for (float t = 0; t < 5000.0f; t += 7.0f) roam::walkAt(net, a, t);
        const roam::Pose pa = roam::walkAt(net, a, 5000.0f), pb = roam::walkAt(net, b, 5000.0f);
        CHECK(std::hypot(pa.at.x - pb.at.x, pa.at.y - pb.at.y) < 0.01f);
        const roam::Pose back = roam::walkAt(net, a, 100.0f), fresh = roam::walkAt(net, b, 100.0f);
        CHECK(std::hypot(back.at.x - fresh.at.x, back.at.y - fresh.at.y) < 0.01f);
    }
    // Different trainers set off from different places.
    roam::Walk x, y;
    roam::startWalk(net, 0, 20000, x);
    roam::startWalk(net, 1, 20000, y);
    const roam::Pose px = roam::walkAt(net, x, 600.0f), py = roam::walkAt(net, y, 600.0f);
    CHECK(std::hypot(px.at.x - py.at.x, px.at.y - py.at.y) > 5.0f);
}

TEST(roamers_duels) {
    // A fair fight: their dragon about your partner's level.
    for (int id = 0; id < roam::kRoamers; ++id) {
        const roam::Roamer& r = roam::roamer(id);
        for (int level : {1, 5, 20, kLevelCap}) {
            const int l = roam::duelLevel(level, id);
            CHECK(l >= 1 && l <= kLevelCap && std::abs(l - level) <= 1);
            const Dragon d = roam::dragonOf(id, l);
            CHECK(d.stage == Stage::Adult && trainer::levelOf(d) == l && std::strcmp(d.name, r.dragonName) == 0);
            CHECK(d.id >= 0xB1000000u && d.id < 0xB2000000u && d.kind == findKind(r.kind));
        }
    }
    // Balance: a keeper's dragon at its own level, a few trained points, wins about half.
    Rng rng(77);
    for (int id = 0; id < roam::kRoamers; ++id) {
        int wins = 0;
        constexpr int kRuns = 300;
        for (int i = 0; i < kRuns; ++i) {
            const int level = 5 + static_cast<int>(rng.below(20));
            const Dragon mine = grown(static_cast<int>(rng.below(static_cast<u32>(kindCount()))), level, level / 4, 50000 + id * 1000 + i);
            const Dragon foe = roam::dragonOf(id, roam::duelLevel(level, id));
            battle::Battle bt;
            battle::begin(bt, battle::makeBattler(mine), battle::makeBattler(foe));
            while (!bt.over) battle::resolveTurn(bt, battle::chooseMove(bt, 0, 3, rng), battle::chooseMove(bt, 1, roam::roamer(id).skill, rng), rng);
            wins += bt.winner == 0;
        }
        const float rate = static_cast<float>(wins) / kRuns;
        std::printf("  against %-8s (%+d, skill %d): %.2f\n", roam::roamer(id).name, roam::roamer(id).edge, roam::roamer(id).skill, rate);
        CHECK(rate > 0.3f && rate < 0.85f);
    }
}

TEST(roamers_rewards_and_record) {
    static SaveData s;
    s = SaveData{};
    s.dragons[0] = grown(findKind("pouncer"), 10, 2, 1);
    s.dragonCount = 1;
    const s32 day = 20100;
    // A loss teaches a little; nothing else.
    roam::Reward r = roam::record(s, 0, 0, 10, battle::Outcome::Lost, day);
    CHECK(r.growth.xp > 0 && r.gleam == 0 && s.progress.duelsWon == 0 && s.dragons[0].battleWins == 0);
    CHECK(s.progress.counts[kCountBattles] == 1);
    // Giving up: nothing at all.
    r = roam::record(s, 0, 0, 10, battle::Outcome::GaveUp, day);
    CHECK(r.growth.xp == 0 && s.progress.counts[kCountBattles] == 1);
    // The first win of the day pays a little; again the same day, only experience.
    const u32 before = s.gleam;
    r = roam::record(s, 0, 0, 10, battle::Outcome::Won, day);
    CHECK(r.gleam == roam::duelGleam(10) && r.gleam > 0 && r.gleam <= roam::kGleamCap && s.gleam == before + r.gleam);
    CHECK(s.progress.duelsWon == 1 && s.dragons[0].battleWins == 1 && roam::paidToday(s, 0, day) && !roam::paidToday(s, 1, day));
    r = roam::record(s, 0, 0, 10, battle::Outcome::Won, day);
    CHECK(r.gleam == 0 && r.paidBefore && r.growth.xp > 0 && s.progress.duelsWon == 2);
    // Another trainer's first win the same day still pays; the next day the first again.
    r = roam::record(s, 0, 3, 10, battle::Outcome::Won, day);
    CHECK(r.gleam > 0 && !r.paidBefore);
    r = roam::record(s, 0, 0, 10, battle::Outcome::Won, day + 1);
    CHECK(r.gleam > 0 && !r.paidBefore && !roam::paidToday(s, 3, day + 1));
    // Modest, and capped: less than a league's rematch at any level.
    CHECK(roam::duelGleam(1) >= 8 && roam::duelGleam(50) == roam::kGleamCap && roam::kGleamCap <= 30);
    CHECK(battle::battleXp(10, 10, battle::Outcome::Won, 0.8f) < battle::battleXp(10, 10, battle::Outcome::Won));
    // The record keeps across a save.
    s.progress.roamDay = 4321;
    s.progress.roamPaid = 0x91;
    s.progress.duelsWon = 777;
    std::vector<u8> buf(maxEncodedSize());
    const std::size_t n = encodeSave(s, 1, 1767571200, buf.data(), buf.size());
    static SaveData out;
    CHECK(n > 0 && decodeSave(buf.data(), n, out) == LoadResult::Ok);
    CHECK(out.progress.roamDay == 4321 && out.progress.roamPaid == 0x91 && out.progress.duelsWon == 777);
    CHECK(out.dragons[0].battleWins == s.dragons[0].battleWins);
}

// The villagers' doings by the hour (core/routines): every clip there in the people's library
// (the loops loop, the now-and-thens play once), everyone asleep deep in the night and nobody at
// noon, sitting only on the ground-sitting clips, the doings their props suit.
TEST(villagers_doings) {
    static AnimLibrary lib;
    std::vector<u8> bytes;
    if (FILE* f = std::fopen("../romfs/anims/person.eca", "rb")) {
        u8 buf[4096];
        std::size_t n;
        while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) bytes.insert(bytes.end(), buf, buf + n);
        std::fclose(f);
    }
    CHECK(!bytes.empty() && loadAnims(bytes.data(), bytes.size(), lib));
    for (int k = 0; k < kVillagers; ++k) {
        const Villager v = static_cast<Villager>(k);
        int asleep = 0;
        for (int hour = 0; hour < 24; ++hour) {
            const routine::Doing d = routine::villager(v, hour);
            const int c = lib.find(d.clip);
            CHECK(c >= 0 && lib.clips[static_cast<std::size_t>(c)].loop);
            if (d.now) {
                const int n = lib.find(d.now);
                CHECK(n >= 0 && !lib.clips[static_cast<std::size_t>(n)].loop);
            }
            const bool ground = std::strcmp(d.clip, "sit_ground") == 0 || std::strcmp(d.clip, "doze") == 0;
            CHECK(d.seated == ground);
            CHECK(d.asleep == (std::strcmp(d.clip, "doze") == 0 || std::strcmp(d.clip, "doze_stand") == 0));
            asleep += d.asleep;
        }
        CHECK(routine::villager(v, 2).asleep && !routine::villager(v, 12).asleep && !routine::villager(v, 9).asleep);
        CHECK(asleep >= 7 && asleep <= 11);  // a night's sleep
        CHECK(std::strcmp(routine::villager(v, 26).clip, routine::villager(v, 2).clip) == 0);  // (hours wrap)
    }
    CHECK(std::strcmp(routine::villager(Villager::Market, 7).clip, "tidy") == 0);        // her stall, set out
    CHECK(std::strcmp(routine::villager(Villager::Sanctuary, 7).clip, "scatter") == 0);  // the bucket of feed
    CHECK(std::strcmp(routine::villager(Villager::Steward, 12).clip, "write") == 0);     // the clipboard
    CHECK(std::strcmp(routine::villager(Villager::Child, 10).clip, "fly_toy") == 0);     // the toy dragon
    CHECK(routine::villager(Villager::Traveller, 19).seated && routine::villager(Villager::Keeper, 19).seated);
    float lo = 1e9f, hi = 0;
    for (u32 seed = 0; seed < 500; ++seed) {
        const float g = routine::nextGap(seed);
        lo = std::fmin(lo, g);
        hi = std::fmax(hi, g);
    }
    CHECK(lo >= 8.0f && hi <= 20.0f && hi - lo > 8.0f);
}

void runRoamerTests() {
    RUN(villagers_doings);
    RUN(roamers_table);
    RUN(roamers_roster);
    RUN(roamers_paths_network);
    RUN(roamers_a_days_walk);
    RUN(roamers_duels);
    RUN(roamers_rewards_and_record);
}
