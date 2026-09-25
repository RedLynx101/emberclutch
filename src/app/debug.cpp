#include "app/debug.hpp"

#include <cstdio>

#include "app/audio.hpp"
#include "app/perf.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "core/den_roster.hpp"
#include "core/genetics.hpp"
#include "core/items.hpp"
#include "core/names.hpp"
#include "app/theme.hpp"
#include "app/storage.hpp"
#include "app/ui_draw.hpp"
#include "core/clock.hpp"

namespace ec {
namespace {

u32 okOr(bool ok) { return ok ? theme::kShell : theme::kRose; }

void line(App& app, float y, const char* s, u32 color) { text(app, s, 6, y, 0.42f, color, C2D_AlignLeft); }

// Jump the dragon to the start of its next stage: enough days since hatching and enough stars.
void forceNextStage(Dragon& d, s64 now) {
    if (d.stage == Stage::Egg || d.stage == Stage::Adult) return;
    const Stage next = static_cast<Stage>(static_cast<int>(d.stage) + 1);
    d.hatchedAt = now - static_cast<s64>(stageMinDay(next)) * kDay - 60;
    if (d.careStars < stageMinStars(next)) d.careStars = static_cast<u16>(stageMinStars(next));
    simulate(d, now, now);  // re-evaluates the stage
}

// Swap the dragon to the next starter breed (keeps sex, stage and care): for checking
// every breed's parts in the renderer.
// Through all 21 breeds: the purebreds, then the hybrids (a purebred of each parent element, bred).
void nextBreed(Dragon& d, Rng& rng) {
    u8 a, b;
    breedAlleles((breedIndex(d.genome) + 1) % kBreedCount, a, b);
    d.genome = a == b ? makePurebred(static_cast<Element>(a), rng)
                      : breed(makePurebred(static_cast<Element>(a), rng), makePurebred(static_cast<Element>(b), rng), rng);
}

}  // namespace

const char* gpuProbeName(u8 probe) {
    static const char* const kNames[] = {"", "no room", "no den dragons", "no close-up", "no particles"};
    return probe < sizeof(kNames) / sizeof(kNames[0]) ? kNames[probe] : "";
}

void debugDrawOverlay(App& app) {
    if (!EC_DEV || !app.overlay) return;
    // The numbers are written out four times a second, not every frame: the overlay is on
    // when the frame is measured, and new text every frame was a cost of its own (WP11d).
    constexpr int kLines = 5;
    static char lines[kLines][112];
    static u32 colours[kLines];
    static float wait = 0;
    if ((wait -= app.dt) <= 0) {
        wait = 0.25f;
        const RenderStats& s = app.stats;
        std::snprintf(lines[0], sizeof(lines[0]), "%4.1fms  CPU %.1f  GPU %.1f  CMD %d%%  %s", app.frameMs,
                      C3D_GetProcessingTime(), C3D_GetDrawingTime(), static_cast<int>(C3D_GetCmdBufUsage() * 100),
                      gpuProbeName(app.gpuProbe));
        colours[0] = okOr(app.frameMs <= kBudgetFrameMs);
        std::snprintf(lines[1], sizeof(lines[1]), "TRI %lu/%lu +%lu/%lu  DRAW %lu/%lu  BONE %lu/%lu",
                      static_cast<unsigned long>(s.tris), static_cast<unsigned long>(kBudgetTris),
                      static_cast<unsigned long>(app.bottomTris), static_cast<unsigned long>(kBudgetCloseTris),
                      static_cast<unsigned long>(s.draws), static_cast<unsigned long>(kBudgetDraws),
                      static_cast<unsigned long>(s.maxBonesPerDraw), static_cast<unsigned long>(kBudgetBones));
        colours[1] = okOr(s.tris <= kBudgetTris && app.bottomTris <= kBudgetCloseTris && s.draws <= kBudgetDraws &&
                          s.maxBonesPerDraw <= kBudgetBones);
        // (No application memory: libctru gives the heap all of it at start, so it reads 0 on the 3DS.)
        std::snprintf(lines[2], sizeof(lines[2]), "LIN %.1fMB  VRAM %.2fMB  romfs %s", linearSpaceFree() / 1048576.0f,
                      vramSpaceFree() / 1048576.0f, app.romfsOk ? "ok" : "MISSING");
        colours[2] = okOr(app.romfsOk);
        const audio::DebugInfo ai = audio::debugInfo();
        std::snprintf(lines[3], sizeof(lines[3]), "AUDIO %s  %s  L%lu S%lu st%d g%.2f",
                      audio::ok() ? "ok" : "OFF (no DSP fw?)", audio::currentMusic()[0] ? audio::currentMusic() : "-",
                      static_cast<unsigned long>(ai.loops), static_cast<unsigned long>(ai.switches), ai.stage, ai.gain);
        colours[3] = okOr(audio::ok());
        std::snprintf(lines[4], sizeof(lines[4]), "%s", perf::line());  // where the CPU time goes (WP11d)
        colours[4] = theme::kShell;
    }
    C2D_DrawRectSolid(0, 0, 0, 262, 70, withAlpha(theme::kDenPlum, 0.75f));
    for (int i = 0; i < kLines; ++i) line(app, 2 + 13 * i, lines[i], colours[i]);
}

// Alpha 2 WP1 before breeding and the Market: a random starter dragon into a free bed (or an
// egg into a free nest, else the Vault), to fill the den.
void devAddDragon(App& app, bool asEgg) {
    SaveData& s = app.game;
    if (s.dragonCount >= kMaxDragons) return;
    if (!asEgg && bedForHatchling(s) < 0) {
        showToast(app, str::kDenFull);
        return;
    }
    const s64 now = nowLocal(app);
    Rng& rng = app.rng;
    const Genome g = makePurebred(static_cast<Element>(rng.below(3)), rng);
    const Sex sex = rollSex(rng);
    Dragon d = makeEgg(s.nextId++, g, sex, now, rollLook(rng));
    if (asEgg) {
        if (!placeEgg(s, d)) showToast(app, str::kEggToVault);
    } else {
        d.incubationSeconds = kIncubationSeconds;
        tryHatch(d, now, rng);
        d.denSlot = static_cast<u8>(makeRoomForHatchling(s));
        suggestName(d, rng.next(), d.name, sizeof(d.name));
    }
    s.dragons[s.dragonCount++] = d;
    dexSeeAll(s);
    saveNow(app);
}

// Alpha 2 WP8: three generations at once, to see a family tree: two pairs of grandparents
// and the parents in the Sanctuary, and their grandchild, just hatched, in a free bed.
void devAddFamily(App& app) {
    SaveData& s = app.game;
    if (s.dragonCount + 7 > static_cast<int>(kMaxDragons) || bedForHatchling(s) < 0) {
        showToast(app, str::kDenFull);
        return;
    }
    const s64 now = nowLocal(app);
    Rng& rng = app.rng;
    auto adult = [&](Element e, Sex sex, u32 mother, u32 father) {
        const Genome g = makePurebred(e, rng);
        Dragon d = makeEgg(s.nextId++, g, sex, now - 30 * kDay, rollLook(rng));
        d.incubationSeconds = kIncubationSeconds;
        tryHatch(d, now - 29 * kDay, rng);
        d.stage = Stage::Adult;
        d.motherId = mother;
        d.fatherId = father;
        d.origin = mother ? Origin::Bred : Origin::Wild;
        d.location = Location::Sanctuary;
        suggestName(d, rng.next(), d.name, sizeof(d.name));
        s.dragons[s.dragonCount++] = d;
        return d.id;
    };
    const u32 a = adult(Element::Ember, Sex::Female, 0, 0), b = adult(Element::Tide, Sex::Male, 0, 0);
    const u32 c = adult(Element::Gale, Sex::Female, 0, 0), e = adult(Element::Frost, Sex::Male, 0, 0);
    const u32 mum = adult(Element::Ember, Sex::Female, a, b), dad = adult(Element::Gale, Sex::Male, c, e);
    const Dragon& mother = s.dragons[s.dragonCount - 2];
    const Dragon& father = s.dragons[s.dragonCount - 1];
    const Genome g = breed(mother.genome, father.genome, rng);
    const Sex sex = rollSex(rng);
    Dragon kid = makeEgg(s.nextId++, g, sex, now, inheritLook(mother.look, father.look, rng));
    kid.motherId = mum;
    kid.fatherId = dad;
    kid.origin = Origin::Bred;
    kid.incubationSeconds = kIncubationSeconds;
    tryHatch(kid, now, rng);
    kid.denSlot = static_cast<u8>(makeRoomForHatchling(s));
    suggestName(kid, rng.next(), kid.name, sizeof(kid.name));
    s.dragons[s.dragonCount++] = kid;
    dexSeeAll(s);
    saveNow(app);
}

bool debugMenu(App& app, const Input& in) {
    if (!EC_DEV) return false;
    if (in.down & KEY_SELECT) app.devMenu = !app.devMenu;
    if (!app.devMenu) return false;
    if (in.down & (KEY_L | KEY_R)) app.devPage = static_cast<u8>(app.devPage ^ 1);

    Dragon& d = activeDragon(app);
    const s64 now = nowLocal(app);
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDenPlum, theme::rgba(20, 14, 28));
    text(app, app.devPage ? "DEV MENU 2/2  (L/R page, SELECT close)" : "DEV MENU 1/2  (L/R page, SELECT close)", 160,
         4, 0.5f, theme::kClutchGold);

    struct Entry {
        const char* label;
        int id;
    };
    static constexpr Entry kPage1[] = {
        {"+1 hour", 0},   {"+1 day", 1},     {"+7 days", 2},      {"Fill needs", 3},
        {"Drain needs", 4}, {"Hatch now", 5}, {"Next stage", 6},   {"Next breed", 10},
        {"Next activity", 12}, {"Add dragon", 11}, {"Overlay", 7}, {"Save now", 8},
        {"Reset save", 9}, {"Dust/mud/bath", 13}, {"Add egg", 14}, {"Breed-ready", 15},
    };
    static constexpr Entry kPage2[] = {
        {"+1,000 steps", 20}, {"+10,000 steps", 21}, {"Gleam +100", 22}, {"All things", 23},
        {"Next decor", 24}, {"Fill bowl", 25}, {"Add family", 26}, {"Next look", 27},
        {"Force look", 28}, {"GPU probe", 29}, {"Next rare", 30}, {"Dex: this breed", 31}, {"Mix looks", 32}, {"Zoomies", 33}, {"Stereo preview", 34}, {"Next game", 35}, {"Valley test", 36},
    };
    const Entry* items = app.devPage ? kPage2 : kPage1;
    const int kCount = app.devPage ? static_cast<int>(sizeof(kPage2) / sizeof(kPage2[0]))
                                   : static_cast<int>(sizeof(kPage1) / sizeof(kPage1[0]));
    for (int i = 0; i < kCount; ++i) {
        const Rect r{8.0f + (i % 2) * 156.0f, 20.0f + (i / 2) * 25.0f, 148, 22};
        if (!button(app, r, items[i].label, in)) continue;
        switch (items[i].id) {
            case 0: app.game.devOffset += kHour; break;
            case 1: app.game.devOffset += kDay; break;
            case 2: app.game.devOffset += 7 * kDay; break;
            case 3: d.needs = Needs{100, 100, 100, 100}; d.upset = false; break;
            case 4: d.needs = Needs{5, 5, 5, 5}; break;
            case 5: if (d.stage == Stage::Egg) d.incubationSeconds = kIncubationSeconds; break;
            case 6: forceNextStage(d, now); break;
            case 7: app.overlay = !app.overlay; break;
            case 8: saveNow(app); showToast(app, "Saved."); audio::playSfx(audio::Sfx::Save); break;
            case 9: deleteGame(); resetForNewGame(app); app.slots = SaveSlots{}; app.scene = SceneId::Title; app.devMenu = false; break;
            case 10: nextBreed(d, app.rng); break;
            case 11: devAddDragon(app, false); break;
            case 14: devAddDragon(app, true); break;
            case 20: app.devSteps += 1000; break;
            case 21: app.devSteps += 10000; break;
            case 22: app.game.gleam += 100; break;
            case 23:  // everything from the Market's stalls (WP7), toys set down, decor up
                for (int k = 0; k < kItems; ++k) {
                    app.game.gleam += itemInfo(static_cast<ec::Item>(k)).price;
                    buyItem(app.game, static_cast<ec::Item>(k));
                }
                break;
            case 24:  // every spot to its next piece (see each look in turn)
                for (int spot = 0; spot < kDecorSpots; ++spot) {
                    const int now = static_cast<int>(decorAt(app.game, spot));
                    for (int step = 1; step <= kItems; ++step) {
                        const ec::Item next = static_cast<ec::Item>((now + step) % kItems);
                        if (decorSpot(itemInfo(next).kind) == spot && owns(app.game, next)) {
                            putUp(app.game, next);
                            break;
                        }
                    }
                }
                break;
            case 26: devAddFamily(app); break;
            case 30: {  // the rare traits in turn (WP12): none, iridescent, melanistic, leucistic, starspeckle
                static const u8 kRares[] = {0, kRareIridescent, kRareMelanistic, kRareLeucistic, kRareStarspeckle};
                static const char* const kRareNames[] = {"none", "iridescent", "melanistic", "leucistic", "starspeckle"};
                int k = 0;
                while (k < 5 && kRares[k] != d.genome.rareFlags) ++k;
                k = (k + 1) % 5;
                d.genome.rareFlags = kRares[k];
                showToastf(app, "Rare trait: %s", kRareNames[k]);
                break;
            }
            case 31: {  // WP12: every look of this one's breed in the Dragondex (completes it: the banner)
                const int breed = breedIndex(d.genome);
                for (int l = 0; l < kLookCount; ++l) dexSee(app.game, dexDragon(breed, l));
                showToastf(app, "Dragondex: %s complete", breedName(d.genome));
                break;
            }
            case 36:  // Beta WP1: fly your dragon over the placeholder valley
                app.devMenu = false;
                openValley(app);
                break;
            case 34:  // WP11e: the top screen as the right eye sees it at full depth (the emulator
                      // shows one eye)
                app.stereoPreview = !app.stereoPreview;
                if (app.stereoPreview) {  // and the right eye's shift at the subject, twice and four times as far
                    r3d::setEye(1.0f);
                    char how[48];
                    std::snprintf(how, sizeof(how), "right eye: %+.1f %+.1f %+.1f px", r3d::eyeShift(0.8f),
                                  r3d::eyeShift(1.0f), r3d::eyeShift(2.0f));
                    r3d::setEye(0.0f);
                    showToastf(app, "Stereo preview: %s", how);
                } else {
                    showToast(app, "Stereo preview: off");
                }
                break;
            case 35: {  // run 13: the den's games in turn (the first two dragons), and tail chasing (this one)
                static int game = 0;
                static const Activity kGames[] = {Activity::Spar, Activity::Stalk, Activity::Chase, Activity::Nuzzle,
                                                  Activity::TailChase};
                static const char* const kNames[] = {"spar", "stalk and pounce", "chase", "nuzzle", "tail chase"};
                const int k = game++ % 5;
                if (kGames[k] == Activity::TailChase) {
                    if (DenActor* a = careActor(app)) a->behavior.force(Activity::TailChase);
                } else {
                    app.social.next = kGames[k];
                }
                showToastf(app, "Game: %s", kNames[k]);
                break;
            }
            case 33:  // WP12c: a burst of laps round the den at a run
                if (DenActor* a = careActor(app)) a->behavior.force(Activity::Zoomies);
                break;
            case 32: {  // WP12's budgets: the den's dragons in three different looks (not classic)
                const DenRoster r = denRoster(app.game);
                for (int bed = 0; bed < kDenDragons; ++bed)
                    if (r.dragon[bed] >= 0) app.game.dragons[r.dragon[bed]].look = static_cast<u8>(1 + bed % 3);
                showToast(app, "Looks: Pebbleback, Tallneck, wild");
                break;
            }
            case 29:  // WP11d: each part's share of the GPU's time, one left out at a time
                app.gpuProbe = static_cast<u8>((app.gpuProbe + 1) % 5);
                showToastf(app, "GPU probe: %s", app.gpuProbe ? gpuProbeName(app.gpuProbe) : "everything drawn");
                break;
            case 28: {  // every dragon in one look, in turn, then their own again
                const int next = r3d::forceLook() + 1 < kLookCount ? r3d::forceLook() + 1 : -1;
                r3d::setForceLook(next);
                showToastf(app, "Looks: %s", next < 0 ? "their own" : lookName(static_cast<u8>(next), d.genome));
                break;
            }
            case 27: {  // this dragon's own look (D54), in turn
                d.look = static_cast<u8>((d.look + 1) % kLookCount);
                char name[40];
                lookBreedName(d.look, d.genome, name, sizeof(name));
                showToastf(app, "Look: %s", name);
                break;
            }
            case 25:
                app.game.owned |= 1u << static_cast<int>(ec::Item::FoodBowl);
                for (u8& k : app.game.bowl) k = static_cast<u8>(k == 0xFF ? Food::HearthBread : static_cast<Food>(k));
                break;
            case 15:  // an adult ready for the Nesting Stone: grown, trusting, content, rested
                while (d.stage != Stage::Egg && d.stage != Stage::Adult) forceNextStage(d, now);
                if (d.bond < 400) d.bond = 400;
                if (d.bondHigh < d.bond) d.bondHigh = d.bond;
                d.needs = Needs{95, 95, 95, 95};
                d.upset = false;
                d.lastBredAt = 0;
                break;
            case 13: {  // see the dirt (D46) without waiting: all dusty, then muddy too, then a bath
                if (d.mud[kRegionBelly] > 50.0f) {
                    bathe(d);
                } else if (d.dirt[kRegionBack] > 50.0f) {
                    static const float kWet[kRegionCount] = {100, 100, 100, 100, 100, 100, 100, 40};  // all over, to see it
                    for (int r = 0; r < kRegionCount; ++r) d.mud[r] = kWet[r];
                } else {
                    for (float& dust : d.dirt) dust = 100.0f;
                }
                break;
            }
            case 12: {  // every behavior state is reachable from here (WP5)
                DenBehavior& b = (careActor(app) ? *careActor(app) : app.actors[0]).behavior;
                int next = (static_cast<int>(b.activity) + 1) % static_cast<int>(Activity::Count);
                if (static_cast<Activity>(next) == Activity::Hatch) next = 0;  // only the hatching starts there
                b.force(static_cast<Activity>(next));
                break;
            }
        }
    }
    char buf[80];
    const DenBehavior& b = (careActor(app) ? *careActor(app) : app.actors[0]).behavior;
    std::snprintf(buf, sizeof(buf), "+%lldh  stars %d  %s  %s/%d  (%.1f, %.1f)  v%.2f  %s", static_cast<long long>(app.game.devOffset / kHour),
                  d.careStars, stageName(d.stage), careActor(app) ? activityName(b.activity) : "-", b.step, b.pos.x,
                  b.pos.y, b.speed, lookName(d.look, d.genome));
    text(app, buf, 160, 226, 0.4f, theme::kAsh);
    return true;
}

}  // namespace ec
