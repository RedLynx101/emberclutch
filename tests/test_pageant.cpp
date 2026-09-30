// 1.0's beauty (D90): the accessories and dyes (core/accessories), their meshes (core/wear_mesh)
// and how they're fitted on every kind in both forms (core/wear_fit), and the pageant's shows
// (core/pageant): themes, the three rounds, rivals by league, judging, placings, the board and
// its rewards once a day.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/accessories.hpp"
#include "core/clock.hpp"
#include "core/kinds.hpp"
#include "core/pageant.hpp"
#include "core/rig.hpp"
#include "core/save.hpp"
#include "core/trainer.hpp"
#include "core/wear_fit.hpp"
#include "core/wear_mesh.hpp"

using namespace ec;

namespace {

constexpr s64 kT0 = 1767225600;  // 2026-01-01 00:00 local
constexpr s32 kDay0 = 20454;

Dragon hatched(u32 id, int kind = 0) {
    Rng rng(id);
    Dragon d = makeEgg(id, makePurebred(Element::Ember, rng), Sex::Female, kT0);
    d.incubationSeconds = kIncubationSeconds;
    tryHatch(d, kT0 + 10 * kHour, rng);
    rollKind(d, kind, 0, rng);
    d.stage = Stage::Adult;
    return d;
}

std::vector<u8> readAll(const std::string& path) {
    std::vector<u8> data;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return data;
    std::fseek(f, 0, SEEK_END);
    data.resize(static_cast<std::size_t>(std::ftell(f)));
    std::fseek(f, 0, SEEK_SET);
    if (std::fread(data.data(), 1, data.size(), f) != data.size()) data.clear();
    std::fclose(f);
    return data;
}

Vec3 column(const Mat34& m, int c) { return {m.m[0][c], m.m[1][c], m.m[2][c]}; }

}  // namespace

TEST(accessories_table_is_sound) {
    CHECK(accessoryCount() == kAccessoryCount && kAccessoryCount <= kAccessoryBytes * 8);
    std::set<std::string> names;
    int perSlot[kWearSlots] = {}, prizes = 0;
    for (int a = 0; a < kAccessoryCount; ++a) {
        const Accessory& x = accessoryInfo(a);
        names.insert(x.name);
        ++perSlot[static_cast<int>(x.slot)];
        CHECK(x.styles != 0);
        CHECK(x.price > 0);
        prizes += x.source == WearSource::Prize;
        // Its mesh is one of its slot's.
        const int shape = static_cast<int>(x.shape);
        const int first[kWearSlots + 1] = {0, static_cast<int>(WearShape::NeckBow), static_cast<int>(WearShape::Saddle),
                                          static_cast<int>(WearShape::TailBow), kWearShapes};
        const int s = static_cast<int>(x.slot);
        CHECK(shape >= first[s] && shape < first[s + 1]);
    }
    CHECK(names.size() == static_cast<std::size_t>(kAccessoryCount));
    for (int n : perSlot) CHECK(n == 8);
    CHECK(prizes >= 5);
    // Every style is carried by something, and something at the stall for each slot.
    for (int b = 0; b < kStyleTags; ++b) {
        bool carried = false;
        for (int a = 0; a < kAccessoryCount; ++a) carried |= (accessoryInfo(a).styles >> b) & 1u;
        CHECK(carried);
        CHECK(std::strlen(styleName(b)) > 0);
    }
    CHECK(dyeCount() == kDyeCount && kDyeCount <= 32);
    CHECK(std::strcmp(dyeInfo(0).name, "Natural") == 0);
    for (int d = 1; d < kDyeCount; ++d) CHECK(dyeInfo(d).source == WearSource::Prize || dyeInfo(d).price > 0);
}

TEST(accessories_owned_worn_and_dyed) {
    SaveData s;
    Dragon d = hatched(3);
    CHECK(acc::wornCount(d) == 0 && acc::worn(d, WearSlot::Head) == -1);
    CHECK(!acc::putOn(s, d, 0));  // not yours yet
    trainer::giveAccessory(s, 0);
    trainer::giveAccessory(s, 2);
    trainer::giveAccessory(s, 9);
    CHECK(acc::putOn(s, d, 0) && acc::worn(d, WearSlot::Head) == 0);
    CHECK(acc::putOn(s, d, 2) && acc::worn(d, WearSlot::Head) == 2);  // the flower crown took the hat's place
    CHECK(acc::putOn(s, d, 9) && acc::wornCount(d) == 2);
    CHECK(acc::wornStyles(d) == (accessoryInfo(2).styles | accessoryInfo(9).styles));
    int owned[8];
    CHECK(acc::ownedFor(s, WearSlot::Head, owned, 8) == 2 && owned[0] == 0 && owned[1] == 2);
    acc::takeOff(d, WearSlot::Head);
    CHECK(acc::worn(d, WearSlot::Head) == -1 && acc::wornCount(d) == 1);
    d.wear[0] = 9;  // something from another slot in the head's place doesn't count as worn
    CHECK(acc::worn(d, WearSlot::Head) == -1);
    // Dyes: natural is always yours; the rest once owned.
    CHECK(acc::dyeWith(s, d, 0) && d.dye == 0);
    CHECK(!acc::dyeWith(s, d, 4));
    trainer::giveDye(s, 4);
    CHECK(acc::dyeWith(s, d, 4) && d.dye == 4);
    CHECK(!acc::dyeWith(s, d, kDyeCount));
}

TEST(dyes_tint_toward_their_colour) {
    for (int k = 0; k < kindCount(); ++k) {
        Rgb own[kPalCount], dyed[kPalCount];
        kindPalette(k, 0, 0, own);
        for (int i = 0; i < kPalCount; ++i) dyed[i] = own[i];
        applyDye(0, dyed);
        for (int i = 0; i < kPalCount; ++i) CHECK(dyed[i].r == own[i].r && dyed[i].g == own[i].g && dyed[i].b == own[i].b);
        for (int dye = 1; dye < kDyeCount; ++dye) {
            for (int i = 0; i < kPalCount; ++i) dyed[i] = own[i];
            applyDye(dye, dyed);
            const Rgb want = dyeInfo(dye).main;
            // The body is nearer the dye's colour than it was: a closer match in hue, or for the
            // grey dyes (coal, snow) nearer their lightness.
            auto saturation = [](Rgb c) {
                const int mx = std::max(c.r, std::max(c.g, c.b)), mn = std::min(c.r, std::min(c.g, c.b));
                return mx ? static_cast<float>(mx - mn) / mx : 0.0f;
            };
            auto luma = [](Rgb c) { return 0.3f * c.r + 0.59f * c.g + 0.11f * c.b; };
            const bool grey = saturation(want) < 0.25f;
            const bool nearer = grey ? std::fabs(luma(dyed[kPalBase]) - luma(want)) <= std::fabs(luma(own[kPalBase]) - luma(want)) + 5
                                     : acc::colourMatch(dyed[kPalBase], want) >= acc::colourMatch(own[kPalBase], want) - 0.02f;
            if (!nearer)
                std::printf("  %s dyed %s: (%d %d %d) -> (%d %d %d)\n", kindInfo(k).name, dyeInfo(dye).name, own[kPalBase].r,
                            own[kPalBase].g, own[kPalBase].b, dyed[kPalBase].r, dyed[kPalBase].g, dyed[kPalBase].b);
            CHECK(nearer);
            // The eyes, the heartglow and the horns keep the kind's.
            for (u8 slot : {kPalIris, kPalPupil, kPalGlint, kPalGlow, kPalHorn})
                CHECK(dyed[slot].r == own[slot].r && dyed[slot].g == own[slot].g && dyed[slot].b == own[slot].b);
        }
    }
}

TEST(glade_stalls_and_rewards) {
    int a[acc::kStallShown], b[acc::kStallShown];
    acc::stallPicks(kDay0, a);
    acc::stallPicks(kDay0, b);
    bool slots[kWearSlots] = {};
    for (int k = 0; k < acc::kStallShown; ++k) {
        CHECK(a[k] == b[k] && a[k] >= 0);
        CHECK(accessoryInfo(a[k]).source == WearSource::Stall);
        slots[static_cast<int>(accessoryInfo(a[k]).slot)] = true;
        for (int j = 0; j < k; ++j) CHECK(a[j] != a[k]);
    }
    for (bool s : slots) CHECK(s);
    bool changes = false;
    for (int day = 1; day < 8; ++day) {
        acc::stallPicks(kDay0 + day, b);
        for (int k = 0; k < acc::kStallShown; ++k) changes |= a[k] != b[k];
    }
    CHECK(changes);
    SaveData s;
    s.gleam = 100;
    const int hat = 1;  // the top hat, 180
    CHECK(!acc::buyAccessory(s, hat) && s.gleam == 100);
    s.gleam = 500;
    CHECK(acc::buyAccessory(s, hat) && s.gleam == 320 && trainer::ownsAccessory(s, hat));
    CHECK(!acc::buyAccessory(s, hat));  // once
    CHECK(acc::buyDye(s, 1) && trainer::ownsDye(s, 1) && s.gleam == 230);
    CHECK(!acc::buyDye(s, 9));  // a prize dye isn't sold
    CHECK(!acc::buyDye(s, 0));
    // Prizes: one you haven't got, until you have them all.
    std::set<int> got;
    for (u32 seed = 0; seed < 64; ++seed) {
        const int p = acc::unownedFrom(s, WearSource::Prize, seed);
        if (p < 0) break;
        CHECK(accessoryInfo(p).source == WearSource::Prize && !trainer::ownsAccessory(s, p));
        trainer::giveAccessory(s, p);
        got.insert(p);
    }
    CHECK(acc::unownedFrom(s, WearSource::Prize, 7) == -1 && got.size() >= 5);
    CHECK(acc::unownedFrom(s, WearSource::Hollow, 1) >= 0 && acc::unownedFrom(s, WearSource::Find, 1) >= 0);
    for (int n = 0; n < 8; ++n) {
        const int d = acc::prizeDye(s, static_cast<u32>(n));
        if (d == 0) break;
        CHECK(dyeInfo(d).source == WearSource::Prize);
        trainer::giveDye(s, d);
    }
    CHECK(acc::prizeDye(s, 3) == 0);
}

TEST(colours_match_clash_and_complement) {
    CHECK(acc::colourMatch({200, 60, 60}, {200, 60, 60}) > 0.99f);
    CHECK(acc::colourMatch({128, 128, 128}, {40, 200, 90}) == 0.8f);            // grey goes with anything
    CHECK(acc::colourMatch({230, 140, 40}, {40, 110, 230}) >= 0.75f);           // orange and blue
    CHECK(acc::colourMatch({230, 40, 40}, {60, 220, 60}) < acc::colourMatch({230, 40, 40}, {230, 80, 40}));
    CHECK(acc::colourMatch({220, 50, 50}, {50, 200, 60}) < 0.6f);               // red and green: a clash here
}

TEST(accessory_meshes_are_cheap_and_face_out) {
    for (int s = 0; s < kWearShapes; ++s) {
        const PropMesh m = wearMesh(static_cast<WearShape>(s));
        CHECK(m.triangles() > 0 && m.triangles() <= kWearMaxTriangles);
        CHECK(m.pos.size() == m.nrm.size() && m.paint.size() == m.pos.size() * 4);
        int wrong = 0;
        for (std::size_t i = 0; i < m.idx.size(); i += 3) {
            const u16 a = m.idx[i], b = m.idx[i + 1], c = m.idx[i + 2];
            CHECK(a < m.pos.size() && b < m.pos.size() && c < m.pos.size());
            if (a >= m.pos.size() || b >= m.pos.size() || c >= m.pos.size()) continue;
            const Vec3 g = cross(m.pos[b] - m.pos[a], m.pos[c] - m.pos[a]);
            if (length(g) < 1e-7f) continue;  // (a pole's sliver)
            wrong += dot(g, m.nrm[a] + m.nrm[b] + m.nrm[c]) < 0;
        }
        for (const Vec3& p : m.pos) CHECK(std::fabs(p.x) < 4 && std::fabs(p.y) < 4.5f && std::fabs(p.z) < 4);
        for (std::size_t v = 0; v < m.pos.size(); ++v) CHECK(m.paint[v * 4] < 4);
        if (wrong) std::printf("  shape %d: %d of %d triangles face in\n", s, wrong, m.triangles());
        CHECK(wrong == 0);
    }
}

TEST(accessories_fit_every_kind) {
    for (int k = 0; k < kindCount(); ++k) {
        for (int form = 0; form < 2; ++form) {
            for (bool lod1 : {false, true}) {
                const std::string path = std::string("../romfs/dragons/") + kindInfo(k).name + "/" +
                                         (form ? "grown" : "hatchling") + (lod1 ? "_lod1" : "") + ".ecm";
                const std::vector<u8> bytes = readAll(path);
                static ModelData m;
                m = ModelData{};
                CHECK(!bytes.empty() && loadModel(bytes.data(), bytes.size(), m));
                if (bytes.empty()) continue;
                WearFit fit;
                CHECK(fitWear(m, kindInfo(k).plan, form == 1, fit));
                const int head = m.skel.find("head"), chest = m.skel.find("chest"), hips = m.skel.find("hips");
                const Vec3 headJoint = m.skel.rest[head].translation();
                float unit[kWearSlots] = {};
                for (int s = 0; s < kWearSlots; ++s) {
                    CHECK(fit.ok[s]);
                    if (!fit.ok[s]) continue;
                    CHECK(fit.boneA[s] >= 0 && fit.boneA[s] < m.skel.count && fit.boneB[s] >= 0 && fit.boneB[s] < m.skel.count);
                    const Mat34& f = fit.frame[s];
                    const Vec3 x = column(f, 0), y = column(f, 1), z = column(f, 2);
                    // Square axes, a sensible size (the kinds are ~1-4 units long).
                    CHECK(std::fabs(dot(normalize(x), normalize(y))) < 1e-3f && std::fabs(dot(normalize(y), normalize(z))) < 1e-3f);
                    CHECK(dot(cross(x, y), z) > 0);
                    unit[s] = length(x);
                    CHECK(unit[s] > 0.02f && unit[s] < 1.2f);
                    CHECK(length(z) > 0.01f && length(y) > 0.01f);
                }
                // Hats clear the eyes (D126): none comes up through a brim, and the hat isn't lifted far.
                const float through = eyesThroughBrim(m, fit.frame[0], kHatBrim, 0.0f);
                CHECK(through <= 1e-4f);
                if (through > 1e-4f) std::printf("  FAIL: %s %s: an eye %.2f through the brim\n", kindInfo(k).name, form ? "grown" : "hatchling", through);
                const Vec3 hat = fit.frame[0].translation(), back = fit.frame[2].translation();
                CHECK(hat.z > headJoint.z - 0.05f);                              // on top of the head
                CHECK(hat.y < m.skel.rest[chest].translation().y);               // well forward of the chest
                CHECK(back.z > m.skel.rest[hips].translation().z && back.z > m.skel.rest[chest].translation().z - 0.3f);
                CHECK(column(fit.frame[2], 2).z > 0.3f * length(column(fit.frame[2], 2)));  // the back's up is up (a wyvern hatchling's is steep)
                CHECK(column(fit.frame[0], 2).z > 0.7f * length(column(fit.frame[0], 2)));  // and the hat's
                if (!lod1)
                    std::printf("  %-11s %-9s head %.2f neck %.2f back %.2f tail %.2f\n", kindInfo(k).name,
                                form ? "grown" : "hatchling", unit[0], unit[1], unit[2], unit[3]);
            }
        }
    }
}

TEST(pageant_themes_and_board) {
    CHECK(std::strcmp(elementName(0), "Ember") == 0 && std::strcmp(elementName(5), "Frost") == 0 &&
          std::strcmp(elementName(6), "Lumen") == 0 && std::strcmp(elementName(7), "Shade") == 0);
    std::set<std::string> names;
    int favouredBy[8] = {};
    for (int t = 0; t < kThemes; ++t) {
        const ThemeInfo& ti = themeInfo(t);
        names.insert(ti.name);
        CHECK(ti.elements != 0 && ti.styles != 0);
        for (int e = 0; e < 8; ++e) favouredBy[e] += (ti.elements >> e) & 1u;
    }
    CHECK(names.size() == static_cast<std::size_t>(kThemes));
    for (int e = 0; e < 8; ++e) CHECK(favouredBy[e] > 0);  // every element has a show of its own
    CHECK(std::strcmp(themeInfo(0).name, "Frost Ball") == 0);
    // Every kind shines somewhere.
    for (int k = 0; k < kindCount(); ++k) {
        bool some = false;
        for (int t = 0; t < kThemes; ++t) some |= pageant::themeFavours(t, k) > 0;
        CHECK(some);
    }
    // A league's four shows a day are different, and change from day to day.
    bool changed = false;
    for (int l = 1; l <= kLeagues; ++l)
        for (s32 day = kDay0; day < kDay0 + 10; ++day) {
            int seen[kShowSlots];
            for (int k = 0; k < kShowSlots; ++k) {
                seen[k] = pageant::showTheme(l, k, day);
                CHECK(seen[k] >= 0 && seen[k] < kThemes);
                for (int j = 0; j < k; ++j) CHECK(seen[j] != seen[k]);
                changed |= seen[k] != pageant::showTheme(l, k, kDay0);
            }
        }
    CHECK(changed);
    SaveData s;
    CHECK(pageant::leagueOpen(s, 1) && !pageant::leagueOpen(s, 2) && pageant::boardLeague(s) == 1);
    s.progress.showLeague = 2;
    CHECK(pageant::leagueOpen(s, 3) && !pageant::leagueOpen(s, 4) && pageant::boardLeague(s) == 3);
    s.progress.showLeague = kLeagues;
    CHECK(pageant::boardLeague(s) == kLeagues);
}

TEST(pageant_rounds_score_what_they_should) {
    SaveData s;
    Dragon d = hatched(11, 0);  // a Pouncer: Ember
    Rgb pal[kPalCount];
    kindPalette(d.kind, d.variant, 0, pal);
    const int carnival = 3, frost = 0;
    CHECK(pageant::themeFavours(carnival, 0) == 2 && pageant::themeFavours(frost, 0) == 0);
    const float bare = pageant::lookScore(d, carnival, pal);
    CHECK(bare > 0 && bare < 100);
    CHECK(pageant::lookScore(d, carnival, pal) > pageant::lookScore(d, frost, pal));  // an Ember shines at the carnival
    // Dressed for it: better; dirty: worse.
    trainer::giveAccessory(s, 4);   // the ember crown (fiery, festive)
    trainer::giveAccessory(s, 31);  // the ember tassel (fiery, wild)
    acc::putOn(s, d, 4);
    acc::putOn(s, d, 31);
    const float dressed = pageant::lookScore(d, carnival, pal);
    CHECK(dressed > bare + 10);
    for (float& r : d.mud) r = 80;
    CHECK(pageant::lookScore(d, carnival, pal) < dressed - 10);
    for (float& r : d.mud) r = 0;
    // Poise: bond, care and mood.
    d.bond = 0;
    d.careStars = 0;
    const float shy = pageant::poiseScore(d);
    d.bond = 800;
    d.careStars = 40;
    const float fond = pageant::poiseScore(d);
    CHECK(fond > shy + 30 && fond <= 100);
    d.needs = Needs{};
    d.needs.belly = d.needs.clean = d.needs.play = d.needs.love = 5;
    CHECK(pageant::poiseScore(d) < fond - 10);
}

TEST(pageant_performance_routine_and_timing) {
    for (int l = 1; l <= kLeagues; ++l) {
        const pageant::Routine r = pageant::makeRoutine(l, 99);
        CHECK(r.count >= 8 && r.count <= pageant::kMaxCues);
        for (int i = 0; i < r.count; ++i) {
            CHECK(static_cast<int>(r.cue[i]) < static_cast<int>(pageant::Cue::Count));
            if (i) CHECK(r.at[i] - r.at[i - 1] >= 0.49f * r.beat);
            if (i >= 2) CHECK(!(r.cue[i] == r.cue[i - 1] && r.cue[i] == r.cue[i - 2]));
        }
        CHECK(r.at[0] >= 2 * r.beat && r.length > r.at[r.count - 1]);
        CHECK(r.length < 22.0f);
        if (l > 1) CHECK(r.count >= pageant::makeRoutine(l - 1, 99).count);
    }
    Dragon d = hatched(5);
    d.bond = 0;
    const float p0 = pageant::perfectWindow(d), g0 = pageant::goodWindow(d);
    d.bond = 1000;
    CHECK(pageant::perfectWindow(d) > p0 && pageant::goodWindow(d) > g0 && g0 > p0);
    CHECK(pageant::judgeHit(0.01f, p0, g0) == pageant::Hit::Perfect);
    CHECK(pageant::judgeHit(-0.12f, p0, g0) == pageant::Hit::Good);
    CHECK(pageant::judgeHit(0.4f, p0, g0) == pageant::Hit::Miss);
    pageant::Hit all[10];
    for (auto& h : all) h = pageant::Hit::Perfect;
    CHECK(pageant::performanceScore(all, 10) > 99.9f);
    all[4] = pageant::Hit::Good;
    const float oneGood = pageant::performanceScore(all, 10);
    all[5] = pageant::Hit::Miss;
    CHECK(pageant::performanceScore(all, 10) < oneGood && oneGood < 100);
    for (auto& h : all) h = pageant::Hit::Miss;
    CHECK(pageant::performanceScore(all, 10) == 0);
}

TEST(pageant_rivals_judges_and_placings) {
    SaveData s;
    s.dragons[0] = hatched(1, 2);  // a Curlstone of yours
    s.dragonCount = 1;
    float lastMid = 0;
    for (int l = 1; l <= kLeagues; ++l) {
        pageant::Rival a[kRivals], b[kRivals];
        pageant::makeRivals(s, l, 1, kDay0, a);
        pageant::makeRivals(s, l, 1, kDay0, b);
        std::set<int> newKinds;
        float mid = 0;
        for (int i = 0; i < kRivals; ++i) {
            CHECK(a[i].dragon.kind == b[i].dragon.kind && a[i].dragon.variant == b[i].dragon.variant);
            CHECK(std::strcmp(a[i].dragon.name, b[i].dragon.name) == 0 && std::strcmp(a[i].trainer, b[i].trainer) == 0);
            CHECK(a[i].dragon.stage == Stage::Adult && a[i].dragon.kind < kindCount());
            CHECK(a[i].dragon.id >= 0xFEE00000u);
            for (int j = 0; j < i; ++j) {
                CHECK(std::strcmp(a[i].dragon.name, a[j].dragon.name) != 0);
                CHECK(std::strcmp(a[i].trainer, a[j].trainer) != 0);
                CHECK(a[i].dragon.id != a[j].dragon.id);
            }
            if (a[i].dragon.kind != 2) newKinds.insert(a[i].dragon.kind);
            CHECK(acc::wornCount(a[i].dragon) <= 2);
            mid += a[i].strength / kRivals;
        }
        CHECK(newKinds.size() <= 1);  // at most one kind you don't have
        CHECK(mid > lastMid);
        lastMid = mid;
        Rng rng(4);
        for (int round = 0; round < kRounds; ++round) {
            const float v = pageant::rivalRound(a[0], round, 0, rng);
            CHECK(v >= 5 && v <= 98);
        }
    }
    Rng rng(9);
    for (float score : {0.0f, 33.0f, 71.0f, 100.0f}) {
        float cards[kJudges];
        pageant::judgeCards(score, kRoundPoise, rng, cards);
        for (float c : cards) {
            CHECK(c >= 1 && c <= 10);
            CHECK(std::fabs(c * 2 - std::round(c * 2)) < 1e-4f);
            CHECK(std::fabs(c - score / 10) <= 1.01f || c == 1 || c == 10);
        }
    }
    static_assert(kEntrants == 3, "these placings are for three");
    const float totals[kEntrants] = {60, 72, 60}, perf[kEntrants] = {20, 5, 25};
    int order[kEntrants];
    pageant::placings(totals, perf, order);
    CHECK(order[0] == 1 && order[1] == 2 && order[2] == 0);
    const float tied[kEntrants] = {60, 60, 60}, same[kEntrants] = {5, 5, 5};
    pageant::placings(tied, same, order);
    CHECK(order[0] == 0);  // a full tie: yours
}

TEST(pageant_rewards_board_and_titles) {
    SaveData s;
    Dragon d = hatched(7);
    s.gleam = 0;
    const s32 today = kDay0;
    // Fourth place: a thank-you, nothing on the board.
    pageant::ShowReward r = pageant::finishShow(s, d, 1, 0, 2, 3, today);
    CHECK(r.gleam > 0 && !pageant::slotWon(s, 1, 0) && d.showWins == 0 && s.progress.counts[kCountShows] == 1);
    // A win: the slot, a ribbon for the theme, Gleam and a prize.
    const u32 before = s.gleam;
    r = pageant::finishShow(s, d, 1, 0, 2, 0, today);
    CHECK(r.paid && r.firstWin && r.accessory >= 0 && trainer::ownsAccessory(s, r.accessory));
    CHECK(s.gleam == before + r.gleam && r.gleam >= 50);
    CHECK(pageant::slotWon(s, 1, 0) && d.showWins == 1 && ((d.ribbons >> 2) & 1u) && trainer::ribbonCount(d) == 1);
    CHECK(pageant::paidToday(s, 1, 0, today) && !pageant::paidToday(s, 1, 0, today + 1));
    // The same show again today: it counts, but pays nothing more.
    r = pageant::finishShow(s, d, 1, 0, 5, 0, today);
    CHECK(!r.paid && r.gleam == 0 && !r.firstWin && d.showWins == 2);
    // Tomorrow it pays again (less).
    r = pageant::finishShow(s, d, 1, 0, 5, 0, today + 1);
    CHECK(r.paid && r.gleam > 0 && r.gleam < 50);
    // The other three: the league's title, a prize dye, the next league open.
    CHECK(!pageant::leagueOpen(s, 2));
    for (int slot = 1; slot < kShowSlots; ++slot) r = pageant::finishShow(s, d, 1, slot, slot, 0, today + 1);
    CHECK(r.leagueWon && s.progress.showLeague == 1 && d.showTitle == 1 && r.dye > 0 && trainer::ownsDye(s, r.dye));
    CHECK(std::strcmp(trainer::showTitleName(d.showTitle), "Ember Darling") == 0);
    CHECK(pageant::leagueOpen(s, 2) && pageant::boardLeague(s) == 2 && pageant::slotsWon(s, 1) == 4);
    // Winning it again later gives no second title or bonus.
    r = pageant::finishShow(s, d, 1, 2, 1, 0, today + 3);
    CHECK(!r.leagueWon && s.progress.showLeague == 1);
}

TEST(pageant_balance_by_league) {
    // A cared-for dragon, dressed for the theme and performing well, wins the Ember league's
    // shows almost always; a bare, unloved one loses to Starfire's rivals nearly always.
    SaveData s;
    s.dragons[0] = hatched(21, 0);
    s.dragonCount = 1;
    auto winRate = [&](Dragon d, int league, float performance, bool dress) {
        int wins = 0, shows = 0;
        for (s32 day = kDay0; day < kDay0 + 30; ++day)
            for (int slot = 0; slot < kShowSlots; ++slot) {
                const int theme = pageant::showTheme(league, slot, day);
                Dragon me = d;
                if (dress) {
                    for (int a = 0; a < kAccessoryCount; ++a)  // something for the theme in each slot
                        if ((accessoryInfo(a).styles & themeInfo(theme).styles) && me.wear[static_cast<int>(accessoryInfo(a).slot)] == kNone)
                            me.wear[static_cast<int>(accessoryInfo(a).slot)] = static_cast<u8>(a);
                }
                Rgb pal[kPalCount];
                kindPalette(me.kind, me.variant, 0, pal);
                pageant::Rival rivals[kRivals];
                pageant::makeRivals(s, league, slot, day, rivals);
                Rng rng(static_cast<std::uint64_t>(day) * 7 + slot);
                float totals[kEntrants] = {}, perf[kEntrants] = {};
                const float mine[kRounds] = {pageant::lookScore(me, theme, pal), pageant::poiseScore(me), performance};
                for (int round = 0; round < kRounds; ++round) {
                    float cards[kJudges];
                    for (int e = 0; e < kEntrants; ++e) {
                        const float v = e == 0 ? mine[round] : pageant::rivalRound(rivals[e - 1], round, theme, rng);
                        pageant::judgeCards(v, round, rng, cards);
                        totals[e] += cards[0] + cards[1] + cards[2];
                        if (round == kRoundPerformance) perf[e] = v;
                    }
                }
                int order[kEntrants];
                pageant::placings(totals, perf, order);
                wins += order[0] == 0;
                ++shows;
            }
        return static_cast<float>(wins) / shows;
    };
    Dragon star = s.dragons[0];
    star.bond = 700;
    star.careStars = 30;
    const float emberGood = winRate(star, 1, 85, true);
    Dragon bare = s.dragons[0];
    bare.bond = 20;
    bare.careStars = 0;
    const float starfireBare = winRate(bare, 4, 50, false);
    const float starfireGood = winRate(star, 4, 95, true);
    std::printf("  wins: Ember, well kept %.2f; Starfire, bare %.2f; Starfire, well kept %.2f\n", emberGood, starfireBare,
                starfireGood);
    CHECK(emberGood > 0.85f);
    CHECK(starfireBare < 0.05f);
    CHECK(starfireGood > 0.2f);  // hard, but it can be done
}

void runPageantTests() {
    RUN(accessories_table_is_sound);
    RUN(accessories_owned_worn_and_dyed);
    RUN(dyes_tint_toward_their_colour);
    RUN(glade_stalls_and_rewards);
    RUN(colours_match_clash_and_complement);
    RUN(accessory_meshes_are_cheap_and_face_out);
    RUN(accessories_fit_every_kind);
    RUN(pageant_themes_and_board);
    RUN(pageant_rounds_score_what_they_should);
    RUN(pageant_performance_routine_and_timing);
    RUN(pageant_rivals_judges_and_placings);
    RUN(pageant_rewards_board_and_titles);
    RUN(pageant_balance_by_league);
}
