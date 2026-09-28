// The valley's critters (core/critters, workstream L) on the real valley: the ground each kind
// lives on, who's out by day and by night, the same critters for the same seed, scattering when
// you run at them, each kind's A moment (the songbird hops over at a whistle, the rabbit always
// escapes your dragon's chase, the butterfly lands on its head, the frog croaks back and leaps
// into the water, the ducks paddle over and never leave the water, the fox boops noses), the
// rewards and their daily cap, the save's fields, and the meshes and their triangle budget.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "check.hpp"
#include "core/critters.hpp"
#include "core/dragon.hpp"
#include "core/genetics.hpp"
#include "core/save.hpp"
#include "core/valley.hpp"

using namespace ec;
using namespace ec::critters;

namespace {

const Valley& lifeValley() {
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
    }
    return v;
}

const char* kindName(Kind k) {
    static const char* const kNames[] = {"songbird", "rabbit", "snow hare", "butterfly", "frog", "duck", "fox"};
    return static_cast<int>(k) < kKinds ? kNames[static_cast<int>(k)] : "?";
}

Around at(const Valley& v, Vec3 you, float day = 1.0f, float dusk = 0.0f) {
    Around a;
    a.you = {you.x, you.y, v.heightAt(you.x, you.y)};
    a.day = day;
    a.dusk = dusk;
    a.night = 1.0f - day - dusk;
    return a;
}

Vec3 forwardOf(float h) { return {std::sin(h), -std::cos(h), 0}; }
float headingTo(Vec3 from, Vec3 to) { return std::atan2(to.x - from.x, -(to.y - from.y)); }
float flat(Vec3 a, Vec3 b) { return std::hypot(a.x - b.x, a.y - b.y); }

// Runs the life for `seconds`, the moment's partner fed back in (as the scene does), counting events.
struct Tally {
    int ev[16] = {};
};
void run(Life& life, const Valley& v, Around& a, float seconds, Tally* tally = nullptr, float dt = 1.0f / 30) {
    for (float t = 0; t < seconds; t += dt) {
        update(life, v, a, dt);
        if (life.moment.active() && life.moment.takesPal) {
            a.pal = life.moment.pal;
            a.palHeading = life.moment.palHeading;
            a.palHead = a.pal + Vec3{0, 0, 1.1f};
        }
        if (tally)
            for (int k = 0; k < life.eventCount; ++k) ++tally->ev[static_cast<int>(life.events[k].ev)];
    }
}

// A spot of this kind of ground near `from` (a spiral out), for placing you.
bool findGround(const Valley& v, Vec3 from, bool (*want)(const Ground&), Vec3& out, float reach = 400.0f) {
    for (float r = 0; r < reach; r += 3.0f)
        for (int k = 0; k < 24; ++k) {
            const float a = k * (6.2831853f / 24);
            const float x = from.x + std::cos(a) * r, y = from.y + std::sin(a) * r;
            if (!v.inside(x, y)) continue;
            if (want(groundAt(v, x, y))) {
                out = {x, y, v.heightAt(x, y)};
                return true;
            }
            if (r == 0) break;
        }
    return false;
}

bool openGrass(const Ground& g) { return g.grass && !g.steep && !g.water && !g.nearPlace; }
bool dryShore(const Ground& g) { return g.shore && !g.steep && !g.nearPlace; }

Vec3 placeAt(const Valley& v, u8 id) {
    const ValleyPlaceInfo* p = v.place(id);
    return p ? p->at : Vec3{};
}

int countKind(const Life& life, Kind k) {
    int n = 0;
    for (const Critter& c : life.c) n += c.alive && c.kind == k && c.state != State::Gone;
    return n;
}

}  // namespace

TEST(critters_ground) {
    const Valley& v = lifeValley();
    CHECK(v.n > 1);
    if (v.n < 2) return;
    // Over a grid across the valley: every kind of ground the critters need is there.
    int water = 0, deep = 0, shore = 0, snow = 0, grass = 0, flowers = 0, cover = 0, total = 0;
    for (float y = v.y0 + 10; y < v.y0 + v.size(); y += 12)
        for (float x = v.x0 + 10; x < v.x0 + v.size(); x += 12) {
            const Ground g = groundAt(v, x, y);
            ++total;
            water += g.water, deep += g.deep, shore += g.shore, snow += g.snow, grass += g.grass;
            flowers += g.flowers, cover += g.cover;
            CHECK(!(g.water && (g.grass || g.snow || g.shore)));
        }
    std::printf("  ground over %d spots: water %d (deep %d), shore %d, snow %d, grass %d, flowers %d, cover %d; water level %.1f\n",
                total, water, deep, shore, snow, grass, flowers, cover, v.water);
    CHECK(water > 0 && deep > 0 && shore > 0 && snow > 0 && grass > total / 5 && flowers > 0 && cover > 0);
    // The places: snow round Frostspire Hollow in the cold north, the Sanctuary's meadow grassy
    // with flowers, the lake wet.
    Vec3 s;
    CHECK(findGround(v, placeAt(v, kPlaceHollow), [](const Ground& g) { return g.snow && !g.steep; }, s, 120.0f));
    CHECK(findGround(v, placeAt(v, kPlaceSanctuary), [](const Ground& g) { return g.grass && g.flowers; }, s, 200.0f));
    CHECK(findGround(v, placeAt(v, kPlaceLake), [](const Ground& g) { return g.deep; }, s, 80.0f));
}

TEST(critters_spawn_by_ground_and_time) {
    const Valley& v = lifeValley();
    if (v.n < 2) return;
    int byDay[kKinds] = {}, byNight[kKinds] = {}, groups = 0, wrong = 0;
    // Standing a moment at each place and at spots across the valley, by day and by night.
    std::vector<Vec3> spots;
    for (const ValleyPlaceInfo& p : v.places) spots.push_back(p.at + Vec3{25, 10, 0});
    for (float y = v.y0 + 60; y < v.y0 + v.size() - 60; y += 90)
        for (float x = v.x0 + 60; x < v.x0 + v.size() - 60; x += 90) spots.push_back({x, y, 0});
    for (int night = 0; night < 2; ++night)
        for (std::size_t k = 0; k < spots.size(); ++k) {
            Life life;
            reset(life, 1234u + static_cast<u32>(k));
            Around a = at(v, spots[k], night ? 0.0f : 1.0f);
            run(life, v, a, 1.2f);
            for (const Critter& c : life.c) {
                if (!c.alive) continue;
                (night ? byNight : byDay)[static_cast<int>(c.kind)]++;
                groups += c.slot == 0;
                const float h = v.heightAt(c.pos.x, c.pos.y);
                bool ok = true;
                switch (c.kind) {
                    case Kind::Duck: ok = h < v.water - 0.25f; break;
                    case Kind::Frog: ok = h > v.water - 0.3f && h < v.water + 1.5f; break;
                    default: ok = h > v.water; break;  // (dry land; a soaring bird: high over anything)
                }
                if (!ok) {
                    ++wrong;
                    std::printf("  FAIL? a %s at (%.0f %.0f) ground %.2f water %.2f\n", kindName(c.kind), c.pos.x, c.pos.y, h, v.water);
                }
            }
        }
    std::printf("  %zu spots, %d groups. By day:", spots.size(), groups);
    for (int k = 0; k < kKinds; ++k) std::printf(" %s %d", kindName(static_cast<Kind>(k)), byDay[k]);
    std::printf(". By night:");
    for (int k = 0; k < kKinds; ++k) std::printf(" %s %d", kindName(static_cast<Kind>(k)), byNight[k]);
    std::printf("\n");
    CHECK(wrong == 0);
    CHECK(byDay[int(Kind::Songbird)] > 0 && byDay[int(Kind::Rabbit)] > 0 && byDay[int(Kind::Butterfly)] > 0);
    CHECK(byDay[int(Kind::Frog)] > 0 && byDay[int(Kind::Duck)] > 0);
    CHECK(byDay[int(Kind::Fox)] == 0 && byNight[int(Kind::Fox)] > 0);
    CHECK(byNight[int(Kind::Songbird)] == 0 && byNight[int(Kind::Butterfly)] == 0 && byNight[int(Kind::Duck)] == 0);
    CHECK(byNight[int(Kind::Frog)] > 0);
    // Snow hares in the snow round Frostspire Hollow.
    Vec3 snow;
    int hares = 0;
    if (findGround(v, placeAt(v, kPlaceHollow) + Vec3{40, 0, 0}, [](const Ground& g) { return g.snow && !g.steep && !g.nearPlace; },
                   snow, 150.0f))
        for (u32 seed = 1; seed <= 12; ++seed) {
            Life life;
            reset(life, seed);
            Around a = at(v, snow);
            run(life, v, a, 1.0f);
            hares += countKind(life, Kind::SnowHare);
        }
    std::printf("  snow hares near Frostspire Hollow over 12 days: %d\n", hares);
    CHECK(hares > 0);
}

TEST(critters_deterministic_and_bounded) {
    const Valley& v = lifeValley();
    if (v.n < 2) return;
    Vec3 meadow;
    CHECK(findGround(v, placeAt(v, kPlaceSanctuary) + Vec3{60, 60, 0}, openGrass, meadow));
    Life a, b;
    reset(a, 77);
    reset(b, 77);
    Around aa = at(v, meadow), ab = at(v, meadow);
    run(a, v, aa, 6.0f);
    run(b, v, ab, 6.0f);
    bool same = true;
    int alive = 0;
    for (int i = 0; i < kMaxCritters; ++i) {
        same = same && a.c[i].alive == b.c[i].alive && a.c[i].kind == b.c[i].kind &&
               std::fabs(a.c[i].pos.x - b.c[i].pos.x) < 1e-4f && std::fabs(a.c[i].pos.y - b.c[i].pos.y) < 1e-4f;
        alive += a.c[i].alive;
    }
    CHECK(same);
    CHECK(alive > 0);
    // Another day's seed: other critters, other spots.
    Life c;
    reset(c, 78);
    Around ac = at(v, meadow);
    run(c, v, ac, 1.0f);
    bool differs = false;
    for (int i = 0; i < kMaxCritters; ++i)
        differs = differs || c.c[i].alive != a.c[i].alive || c.c[i].kind != a.c[i].kind || flat(c.c[i].pos, a.c[i].pos) > 0.5f;
    CHECK(differs);
    // Walking a long way: they come and go round you, never more than the cap, none left far behind.
    Life w;
    reset(w, 5);
    Vec3 you = meadow;
    Around aw = at(v, you);
    aw.youSpeed = 2.8f;
    int most = 0;
    for (int step = 0; step < 600; ++step) {
        you.x += 2.8f / 30;
        if (!v.inside(you.x + 60, you.y)) break;
        aw.you = {you.x, you.y, v.heightAt(you.x, you.y)};
        update(w, v, aw, 1.0f / 30);
        int n = 0;
        for (const Critter& k : w.c) {
            n += k.alive;
            if (k.alive) CHECK(flat(k.pos, aw.you) <= kDropRadius + 1.0f);
        }
        most = n > most ? n : most;
    }
    CHECK(most <= kMaxCritters && most > 0);
}

TEST(critters_scatter_and_stay_away) {
    const Valley& v = lifeValley();
    if (v.n < 2) return;
    // A flock from the grid: run at it and every bird takes wing and flies off, and the spot
    // stays empty while you're about.
    int tried = 0, scattered = 0;
    for (u32 seed = 1; seed <= 30 && tried < 6; ++seed) {
        Vec3 meadow;
        if (!findGround(v, placeAt(v, kPlaceSanctuary) + Vec3{static_cast<float>(seed * 13 % 90), static_cast<float>(seed * 29 % 90), 0},
                        openGrass, meadow))
            continue;
        Life life;
        reset(life, seed);
        Around a = at(v, meadow);
        run(life, v, a, 1.0f);
        int bird = -1;
        for (int i = 0; i < kMaxCritters; ++i)
            if (life.c[i].alive && life.c[i].kind == Kind::Songbird && life.c[i].state != State::Soar && flat(life.c[i].pos, a.you) > 10)
                bird = i;
        if (bird < 0) continue;
        ++tried;
        const u32 cell = life.c[bird].cell;
        const Vec3 flock = life.c[bird].pos;
        // Run at it from 12 m off.
        const float h = headingTo(flock, a.you);
        Vec3 you = flock + forwardOf(h) * 12.0f;
        a.youSpeed = 5.6f;
        bool allFled = false;
        Tally tally;
        for (int f = 0; f < 30 * 12; ++f) {
            const float d = flat(you, flock);
            if (d > 0.5f) you = you + (flock - you) * (5.6f / 30 / d);
            a.you = {you.x, you.y, v.heightAt(you.x, you.y)};
            update(life, v, a, 1.0f / 30);
            for (int k = 0; k < life.eventCount; ++k) ++tally.ev[static_cast<int>(life.events[k].ev)];
            bool any = false, calm = false;
            for (const Critter& c : life.c)
                if (c.alive && c.cell == cell) {
                    any = true;
                    calm = calm || c.state == State::Idle || c.state == State::Move;
                }
            if (!calm && any) allFled = true;
            if (!any) break;
        }
        bool back = false;
        a.youSpeed = 0;
        for (int f = 0; f < 30 * 6; ++f) {
            update(life, v, a, 1.0f / 30);
            for (const Critter& c : life.c) back = back || (c.alive && c.cell == cell);
        }
        if (allFled && !back && tally.ev[static_cast<int>(Ev::Flutter)] > 0) ++scattered;
    }
    std::printf("  flocks run at: %d, scattered and stayed away: %d\n", tried, scattered);
    CHECK(tried > 0 && scattered == tried);
}

TEST(critters_whistle) {
    const Valley& v = lifeValley();
    if (v.n < 2) return;
    Vec3 meadow;
    CHECK(findGround(v, placeAt(v, kPlaceSanctuary) + Vec3{40, 40, 0}, openGrass, meadow));
    Life life;
    reset(life, 3);
    Around a = at(v, meadow);
    run(life, v, a, 0.2f);
    const float heading = 0.4f;
    const int first = spawnNear(life, v, Kind::Songbird, a.you, heading, 30.0f);
    CHECK(first >= 0);
    if (first < 0) return;
    // Walk up softly to 5 m and face them.
    const Vec3 flock = life.c[first].pos;
    const Vec3 you = flock + forwardOf(headingTo(flock, a.you)) * 5.0f;
    a.you = {you.x, you.y, v.heightAt(you.x, you.y)};
    run(life, v, a, 0.5f);
    const Offer o = offer(life, a.you, forwardOf(headingTo(a.you, flock)), false);
    CHECK(o.who >= 0 && o.act == Act::Whistle);
    if (o.who < 0) return;
    CHECK(begin(life, v, o, a));
    Tally t;
    float nearest = 99;
    for (int f = 0; f < 30 * 6; ++f) {
        update(life, v, a, 1.0f / 30);
        for (int k = 0; k < life.eventCount; ++k) ++t.ev[static_cast<int>(life.events[k].ev)];
        nearest = std::fmin(nearest, flat(life.c[o.who].pos, a.you));
    }
    std::printf("  whistled: the bird came to %.2f m; chirps %d, befriended %d\n", nearest, t.ev[int(Ev::Chirp)], t.ev[int(Ev::Befriend)]);
    CHECK(nearest < 1.4f && nearest > 0.5f);
    CHECK(t.ev[int(Ev::Befriend)] == 1 && t.ev[int(Ev::YouWhistle)] == 1 && t.ev[int(Ev::Chirp)] >= 2);
    CHECK(!life.moment.active());
}

TEST(critters_chase_always_escapes) {
    const Valley& v = lifeValley();
    if (v.n < 2) return;
    int chases = 0, clean = 0;
    for (u32 seed = 1; seed <= 40; ++seed) {
        Vec3 spot;
        if (!findGround(v, placeAt(v, static_cast<u8>(seed % kPlaceCount)) + Vec3{30.0f + seed, -20.0f, 0}, openGrass, spot)) continue;
        for (int hare = 0; hare < 2; ++hare) {
            Life life;
            reset(life, seed);
            Around a = at(v, spot);
            a.hasPal = true;
            const float heading = seed * 0.7f;
            a.pal = a.you + Vec3{std::cos(heading) * 1.8f, std::sin(heading) * 1.8f, 0};
            a.pal.z = v.heightAt(a.pal.x, a.pal.y);
            a.palHeading = heading;
            run(life, v, a, 0.1f);
            const int r = spawnNear(life, v, hare ? Kind::SnowHare : Kind::Rabbit, a.you, heading, 20.0f);
            if (r < 0) continue;
            const Vec3 you = life.c[r].pos + forwardOf(headingTo(life.c[r].pos, a.you)) * 5.0f;
            a.you = {you.x, you.y, v.heightAt(you.x, you.y)};
            a.pal = a.you + Vec3{1.5f, 0.5f, 0};
            a.pal.z = v.heightAt(a.pal.x, a.pal.y);
            const Offer o = offer(life, a.you, forwardOf(headingTo(a.you, life.c[r].pos)), true);
            if (o.who != r || o.act != Act::Chase) continue;
            CHECK(offer(life, a.you, forwardOf(headingTo(a.you, life.c[r].pos)), false).who != r);  // (no dragon: no chase)
            if (!begin(life, v, o, a)) continue;
            ++chases;
            bool caught = false, dry = true, near = true, befriended = false, pounced = false, ran = false;
            for (int f = 0; f < 30 * 7 && life.moment.active(); ++f) {
                update(life, v, a, 1.0f / 30);
                for (int k = 0; k < life.eventCount; ++k) befriended = befriended || life.events[k].ev == Ev::Befriend;
                if (!life.moment.active()) break;
                a.pal = life.moment.pal;
                a.palHeading = life.moment.palHeading;
                pounced = pounced || life.moment.move == PalMove::Pounce;
                ran = ran || life.moment.move == PalMove::Run;
                const Critter& c = life.c[r];
                if (c.alive && c.state != State::Gone && flat(c.pos, a.pal) < 0.9f) caught = true;
                if (v.heightAt(a.pal.x, a.pal.y) < v.water - 0.3f) dry = false;
                if (flat(a.pal, a.you) > 14.5f) near = false;
            }
            const bool hidden = !life.c[r].alive || life.c[r].state == State::Gone;
            if (!caught && dry && near && befriended && pounced && ran && hidden && !life.moment.active()) ++clean;
            else
                std::printf("  chase %u/%d: caught %d dry %d near %d befriended %d pounced %d ran %d hidden %d\n", seed, hare, caught, dry,
                            near, befriended, pounced, ran, hidden);
        }
    }
    std::printf("  chases: %d, each escaped cleanly: %d\n", chases, clean);
    CHECK(chases >= 10 && clean == chases);
}

TEST(critters_butterfly_lands) {
    const Valley& v = lifeValley();
    if (v.n < 2) return;
    Vec3 spot;
    CHECK(findGround(v, placeAt(v, kPlaceSanctuary) + Vec3{50, 30, 0}, openGrass, spot));
    for (int withPal = 0; withPal < 2; ++withPal) {
        Life life;
        reset(life, 9);
        Around a = at(v, spot);
        a.hasPal = withPal != 0;
        a.pal = a.you + Vec3{1.6f, 0.4f, 0};
        a.pal.z = v.heightAt(a.pal.x, a.pal.y);
        a.palHead = a.pal + Vec3{0, 0.4f, 1.0f};
        a.palHeadSet = true;
        run(life, v, a, 0.1f);
        const int b = spawnNear(life, v, Kind::Butterfly, a.you, 0.0f, 20.0f);
        CHECK(b >= 0);
        if (b < 0) continue;
        const Vec3 you = life.c[b].pos + forwardOf(headingTo(life.c[b].pos, a.you)) * 3.0f;
        a.you = {you.x, you.y, v.heightAt(you.x, you.y)};
        a.pal = a.you + Vec3{1.6f, 0.4f, 0};
        a.pal.z = v.heightAt(a.pal.x, a.pal.y);
        a.palHead = a.pal + Vec3{0, 0.4f, 1.0f};
        const Offer o = offer(life, a.you, forwardOf(headingTo(a.you, life.c[b].pos)), a.hasPal);
        CHECK(o.act == Act::Still);
        if (o.who < 0) continue;
        CHECK(begin(life, v, o, a));
        CHECK(life.moment.takesPal == a.hasPal);
        float landedAt = -1, onFor = 0;
        bool sneezed = false, befriended = false;
        const Vec3 head = a.hasPal ? a.palHead : a.you + Vec3{0, 0, 1.55f};
        for (int f = 0; f < 30 * 10 && life.moment.active(); ++f) {
            update(life, v, a, 1.0f / 30);
            for (int k = 0; k < life.eventCount; ++k) befriended = befriended || life.events[k].ev == Ev::Befriend;
            sneezed = sneezed || life.moment.move == PalMove::Sneeze;
            if (life.c[o.who].state == State::Perch) {
                if (landedAt < 0) landedAt = f / 30.0f;
                onFor += 1.0f / 30;
                CHECK(length(life.c[o.who].pos - head) < 0.5f);
            }
        }
        std::printf("  butterfly (%s): landed at %.1f s, stayed %.1f s, sneeze %d\n", a.hasPal ? "on the dragon" : "on you", landedAt, onFor,
                    sneezed);
        CHECK(landedAt >= 0 && landedAt < 3.7f && onFor > 3.0f && befriended && !life.moment.active());
        CHECK(sneezed == a.hasPal || !a.hasPal);
        CHECK(life.c[o.who].state == State::Idle || life.c[o.who].state == State::Move);  // (back to its flowers)
    }
}

TEST(critters_frog_croaks_back) {
    const Valley& v = lifeValley();
    if (v.n < 2) return;
    Vec3 shore;
    CHECK(findGround(v, placeAt(v, kPlaceLake), dryShore, shore, 200.0f));
    Life life;
    reset(life, 11);
    Around a = at(v, shore, 0.0f);
    run(life, v, a, 0.1f);
    const int f0 = spawnNear(life, v, Kind::Frog, a.you, 0.0f, 25.0f);
    CHECK(f0 >= 0);
    if (f0 < 0) return;
    const Vec3 frog = life.c[f0].pos;
    // Up to 4 m, softly (walking up close, it'd leap).
    Vec3 you = frog;
    for (int k = 0; k < 16; ++k) {
        const float h = k * 0.3927f;
        const Vec3 p = frog + forwardOf(h) * 4.0f;
        if (v.heightAt(p.x, p.y) > v.water + 0.1f) {
            you = p;
            break;
        }
    }
    a.you = {you.x, you.y, v.heightAt(you.x, you.y)};
    const Offer o = offer(life, a.you, forwardOf(headingTo(a.you, frog)), false);
    CHECK(o.who == f0 && o.act == Act::CroakBack);
    if (o.who < 0) return;
    CHECK(begin(life, v, o, a));
    Tally t;
    run(life, v, a, 4.0f, &t);
    std::printf("  frog: croaks %d, befriended %d, splash %d; now %s\n", t.ev[int(Ev::Croak)], t.ev[int(Ev::Befriend)], t.ev[int(Ev::Splash)],
                life.c[f0].alive ? "splashing" : "gone");
    CHECK(t.ev[int(Ev::Croak)] >= 2 && t.ev[int(Ev::Befriend)] == 1 && t.ev[int(Ev::Splash)] == 1 && t.ev[int(Ev::YouCroak)] == 1);
    CHECK(!life.moment.active());
    // Where it landed is water.
    CHECK(!life.c[f0].alive || v.heightAt(life.c[f0].goal.x, life.c[f0].goal.y) < v.water);
    // Another, walked right up to: it leaps in by itself.
    Life l2;
    reset(l2, 12);
    Around a2 = at(v, shore, 0.0f);
    run(l2, v, a2, 0.1f);
    const int f1 = spawnNear(l2, v, Kind::Frog, a2.you, 0.0f, 25.0f);
    if (f1 >= 0) {
        a2.you = l2.c[f1].pos + Vec3{0.8f, 0.0f, 0};
        a2.youSpeed = 2.0f;
        Tally t2;
        run(l2, v, a2, 2.0f, &t2);
        CHECK(t2.ev[int(Ev::Splash)] >= 1);
    }
}

TEST(critters_ducks_come_and_stay_afloat) {
    const Valley& v = lifeValley();
    if (v.n < 2) return;
    int calls = 0, good = 0;
    for (u32 seed = 1; seed <= 8; ++seed) {
        Vec3 shore;
        if (!findGround(v, placeAt(v, kPlaceLake) + Vec3{static_cast<float>(seed) * 9.0f, 0, 0}, dryShore, shore, 200.0f)) continue;
        Life life;
        reset(life, seed);
        Around a = at(v, shore);
        run(life, v, a, 0.1f);
        const int m = spawnNear(life, v, Kind::Duck, a.you, 0.0f, 13.0f);
        if (m < 0) continue;
        const Offer o = offer(life, a.you, forwardOf(headingTo(a.you, life.c[m].pos)), true);
        if (o.who != m || o.act != Act::Call) continue;
        const float before = flat(life.c[m].pos, a.you);
        CHECK(begin(life, v, o, a));
        ++calls;
        bool afloat = true, befriended = false;
        float nearest = before;
        for (int f = 0; f < 30 * 16; ++f) {
            update(life, v, a, 1.0f / 30);
            for (int k = 0; k < life.eventCount; ++k) befriended = befriended || life.events[k].ev == Ev::Befriend;
            for (const Critter& c : life.c)
                if (c.alive && c.kind == Kind::Duck && v.heightAt(c.pos.x, c.pos.y) > v.water - 0.25f) afloat = false;
            nearest = std::fmin(nearest, flat(life.c[m].pos, a.you));
        }
        if (afloat && befriended && nearest < std::fmax(before - 2.0f, 3.6f)) ++good;
        else std::printf("  ducks %u: afloat %d befriended %d from %.1f m to %.1f m\n", seed, afloat, befriended, before, nearest);
    }
    std::printf("  duck calls: %d, came over and stayed afloat: %d\n", calls, good);
    CHECK(calls > 0 && good == calls);
}

TEST(critters_fox_boops) {
    const Valley& v = lifeValley();
    if (v.n < 2) return;
    Vec3 spot;
    CHECK(findGround(v, placeAt(v, kPlaceSanctuary) + Vec3{-60, 40, 0}, openGrass, spot));
    Life life;
    reset(life, 21);
    Around a = at(v, spot, 0.0f, 0.3f);
    a.hasPal = true;
    a.pal = a.you + Vec3{1.6f, 0.3f, 0};
    a.pal.z = v.heightAt(a.pal.x, a.pal.y);
    run(life, v, a, 0.1f);
    const int fx = spawnNear(life, v, Kind::Fox, a.you, 1.0f, 20.0f);
    CHECK(fx >= 0);
    if (fx < 0) return;
    const Vec3 you = life.c[fx].pos + forwardOf(headingTo(life.c[fx].pos, a.you)) * 7.0f;
    a.you = {you.x, you.y, v.heightAt(you.x, you.y)};
    a.pal = a.you + Vec3{1.6f, 0.3f, 0};
    a.pal.z = v.heightAt(a.pal.x, a.pal.y);
    a.palHead = a.pal + Vec3{0, 0, 1.0f};
    a.palHeadSet = true;
    const Offer o = offer(life, a.you, forwardOf(headingTo(a.you, life.c[fx].pos)), true);
    CHECK(o.who == fx && o.act == Act::Quiet);
    if (o.who < 0) return;
    CHECK(begin(life, v, o, a));
    Tally t;
    float nearest = 99;
    bool sniffed = false;
    for (int f = 0; f < 30 * 12 && (life.moment.active() || life.c[fx].alive); ++f) {
        update(life, v, a, 1.0f / 30);
        for (int k = 0; k < life.eventCount; ++k) ++t.ev[static_cast<int>(life.events[k].ev)];
        if (life.moment.active()) {
            a.pal = life.moment.pal;
            a.palHeading = life.moment.palHeading;
            a.palHead = a.pal + forwardOf(a.palHeading) * 0.5f + Vec3{0, 0, 1.0f};
            sniffed = sniffed || life.moment.move == PalMove::Sniff;
        }
        if (life.c[fx].alive) nearest = std::fmin(nearest, flat(life.c[fx].pos, a.pal));
    }
    std::printf("  fox: came to %.2f m of the dragon, befriended %d, yips %d; %s\n", nearest, t.ev[int(Ev::Befriend)], t.ev[int(Ev::Yip)],
                life.c[fx].alive ? "still about" : "off to the woods");
    CHECK(nearest < 1.6f && sniffed && t.ev[int(Ev::Befriend)] == 1 && t.ev[int(Ev::Yip)] == 1);
    CHECK(!life.moment.active() && !life.c[fx].alive);
}

TEST(critters_rewards) {
    static SaveData s;
    s = SaveData{};
    Rng rng(4);
    Dragon d = makeEgg(1, makePurebred(Element::Ember, rng), Sex::Female, 0);
    d.stage = Stage::Hatchling;
    d.needs.play = 50;
    d.needs.love = 99;
    const u32 gleam = s.gleam;
    const s32 day = 100;
    Reward r = befriend(s, &d, Kind::Rabbit, day);
    CHECK(r.firstEver && r.firstToday && r.paid && r.gleam == kGleamFirst + kGleamDaily);
    CHECK(s.gleam == gleam + kGleamFirst + kGleamDaily);
    CHECK(std::fabs(d.needs.play - 55.0f) < 0.01f);
    CHECK(befriended(s, Kind::Rabbit) && seen(s, Kind::Rabbit) && friendCount(s, Kind::Rabbit) == 1);
    CHECK(!befriended(s, Kind::Fox));
    r = befriend(s, &d, Kind::Rabbit, day);  // again today: no Gleam, still a pinch of Play
    CHECK(!r.firstEver && !r.firstToday && r.gleam == 0 && r.paid);
    r = befriend(s, &d, Kind::Butterfly, day);  // Love, capped at 100
    CHECK(r.firstToday && d.needs.love <= 100.0f);
    for (int k = 0; k < 10; ++k) befriend(s, &d, Kind::Frog, day);
    CHECK(paidToday(s, day) == kDailyPaid);  // (no farming: the day's pinches are used up)
    const float play = d.needs.play;
    r = befriend(s, &d, Kind::Frog, day);
    CHECK(!r.paid && d.needs.play == play && friendCount(s, Kind::Frog) == 11);
    const u32 g2 = s.gleam;
    r = befriend(s, &d, Kind::Rabbit, day + 1);  // a new day: paid again, the day's Gleam again
    CHECK(r.paid && r.firstToday && !r.firstEver && r.gleam == kGleamDaily && s.gleam == g2 + kGleamDaily);
    CHECK(paidToday(s, day + 1) == 1 && paidToday(s, day) == 0);
    const u16 bond = d.bond;
    befriend(s, &d, Kind::Fox, day + 1);
    CHECK(d.bond >= bond);
    r = befriend(s, nullptr, Kind::Duck, day + 1);  // out alone: the Journal and the Gleam, no pinch
    CHECK(!r.paid && r.gleam > 0);
    markSeen(s, Kind::SnowHare);
    CHECK(seen(s, Kind::SnowHare) && !befriended(s, Kind::SnowHare));
    for (int k = 0; k < 300; ++k) befriend(s, nullptr, Kind::Songbird, day + 2);
    CHECK(friendCount(s, Kind::Songbird) == 255);
}

TEST(critters_save_round_trip) {
    static SaveData s, out;
    s = SaveData{};
    s.progress.critterSeen = 0x5B;
    s.progress.critterFriends = 0x13;
    s.progress.critterDay = 20123;
    s.progress.critterToday = 0x11;
    s.progress.critterPaid = 4;
    for (int k = 0; k < 8; ++k) s.progress.critterCounts[k] = static_cast<u8>(k * 30 + 1);
    s.progress.coveDay = 20120;
    std::vector<u8> buf(maxEncodedSize());
    const std::size_t n = encodeSave(s, 3, 1000, buf.data(), buf.size());
    CHECK(n > 0);
    CHECK(decodeSave(buf.data(), n, out) == LoadResult::Ok);
    CHECK(out.progress.critterSeen == 0x5B && out.progress.critterFriends == 0x13 && out.progress.critterDay == 20123);
    CHECK(out.progress.critterToday == 0x11 && out.progress.critterPaid == 4 && out.progress.coveDay == 20120);
    CHECK(std::memcmp(out.progress.critterCounts, s.progress.critterCounts, 8) == 0);
}

TEST(critters_meshes) {
    const Valley& v = lifeValley();
    static Mesh m;
    // One of each in front of the camera: its own handful of triangles, all near it.
    for (int k = 0; k < kKinds; ++k) {
        Life life;
        reset(life, 1);
        Critter& c = life.c[0];
        c.alive = true;
        c.kind = static_cast<Kind>(k);
        c.born = 2;
        c.pos = {0, 0, 0};
        buildMesh(life, {0, 10, 2}, {0, 0, 0}, m);
        CHECK(m.tris() == trianglesOf(c.kind));
        CHECK(m.tris() >= 6 && m.tris() <= 60);
        bool finite = true, near = true;
        for (int i = 0; i < m.verts; ++i) {
            finite = finite && std::isfinite(m.pos[i].x) && std::isfinite(m.pos[i].y) && std::isfinite(m.pos[i].z);
            near = near && length(m.pos[i]) < 1.2f;
        }
        CHECK(finite && near);
        // Every state of it builds (flying, fleeing, leaping, perched, visiting, sitting up ...).
        for (int st = 0; st <= static_cast<int>(State::Soar); ++st) {
            c.state = static_cast<State>(st);
            c.puff = 0.5f;
            c.air = 0.2f;
            buildMesh(life, {0, 10, 2}, {0, 0, 0}, m);
            CHECK(m.tris() > 0 && m.tris() <= trianglesOf(c.kind));
        }
        // Behind the camera, or far off: not drawn.
        c.state = State::Idle;
        buildMesh(life, {0, 10, 2}, {0, 20, 2}, m);
        CHECK(m.tris() == 0);
        c.pos = {0, -200, 0};
        buildMesh(life, {0, 10, 2}, {0, 0, 0}, m);
        CHECK(m.tris() == 0);
    }
    // A frog's splash ring.
    {
        Life life;
        Critter& c = life.c[0];
        c.alive = true;
        c.kind = Kind::Frog;
        c.state = State::Gone;
        c.t = 0.3f;
        c.goal = {0, 0, 0};
        buildMesh(life, {0, 10, 2}, {0, 0, 0}, m);
        CHECK(m.tris() == kRingTris);
    }
    // A crowd: never past the budget, the nearest kept.
    Life life;
    for (int i = 0; i < kMaxCritters; ++i) {
        Critter& c = life.c[i];
        c.alive = true;
        c.kind = Kind::Fox;
        c.born = 2;
        c.pos = {static_cast<float>(i % 8) - 4.0f, -static_cast<float>(i / 8) * 2.0f, 0};
    }
    buildMesh(life, {0, 10, 2}, {0, 0, 0}, m);
    CHECK(m.tris() <= kMaxTris && m.tris() > kMaxTris - trianglesOf(Kind::Fox));
    // On the valley: standing about the places by day and dusk, the camera behind you.
    if (v.n < 2) return;
    int most = 0, total = 0, views = 0;
    for (const ValleyPlaceInfo& p : v.places)
        for (int dusk = 0; dusk < 2; ++dusk) {
            Life l;
            reset(l, 40u + p.id);
            const Vec3 spot = p.at + Vec3{18, 14, 0};
            Around a = at(v, spot, dusk ? 0.2f : 1.0f, dusk ? 0.8f : 0.0f);
            run(l, v, a, 3.0f);
            for (float h = 0; h < 6.28f; h += 1.57f) {
                const Vec3 eye = a.you - forwardOf(h) * 9.0f + Vec3{0, 0, 5.5f};
                buildMesh(l, eye, a.you + forwardOf(h) * 4.0f, m);
                most = m.tris() > most ? m.tris() : most;
                total += m.tris();
                ++views;
            }
        }
    std::printf("  critter triangles in %d views about the places: %.0f on average, %d at most (budget %d)\n", views,
                total / static_cast<float>(views), most, kMaxTris);
    CHECK(most <= kMaxTris);
}

void runCritterTests() {
    RUN(critters_ground);
    RUN(critters_spawn_by_ground_and_time);
    RUN(critters_deterministic_and_bounded);
    RUN(critters_scatter_and_stay_away);
    RUN(critters_whistle);
    RUN(critters_chase_always_escapes);
    RUN(critters_butterfly_lands);
    RUN(critters_frog_croaks_back);
    RUN(critters_ducks_come_and_stay_afloat);
    RUN(critters_fox_boops);
    RUN(critters_rewards);
    RUN(critters_save_round_trip);
    RUN(critters_meshes);
}
