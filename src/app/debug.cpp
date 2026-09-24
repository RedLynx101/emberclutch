#include "app/debug.hpp"

#include <cstdio>

#include "app/audio.hpp"
#include "app/strings.hpp"
#include "core/den_roster.hpp"
#include "core/genetics.hpp"
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
void nextBreed(Dragon& d, Rng& rng) {
    const u8 next = static_cast<u8>((d.genome.elementA + 1) % 3);  // Ember -> Tide -> Gale
    d.genome = makePurebred(static_cast<Element>(next), rng);
}

}  // namespace

void debugDrawOverlay(App& app) {
    if (!EC_DEV || !app.overlay) return;
    const RenderStats& s = app.stats;
    char buf[96];
    C2D_DrawRectSolid(0, 0, 0, 262, 57, withAlpha(theme::kDenPlum, 0.75f));
    std::snprintf(buf, sizeof(buf), "%4.1fms  CPU %.1f  GPU %.1f  CMD %d%%", app.frameMs, C3D_GetProcessingTime(),
                  C3D_GetDrawingTime(), static_cast<int>(C3D_GetCmdBufUsage() * 100));
    line(app, 2, buf, okOr(app.frameMs <= kBudgetFrameMs));
    std::snprintf(buf, sizeof(buf), "TRI %lu/%lu +%lu/%lu  DRAW %lu/%lu  BONE %lu/%lu", static_cast<unsigned long>(s.tris),
                  static_cast<unsigned long>(kBudgetTris), static_cast<unsigned long>(app.bottomTris),
                  static_cast<unsigned long>(kBudgetCloseTris), static_cast<unsigned long>(s.draws),
                  static_cast<unsigned long>(kBudgetDraws), static_cast<unsigned long>(s.maxBonesPerDraw),
                  static_cast<unsigned long>(kBudgetBones));
    line(app, 15, buf,
         okOr(s.tris <= kBudgetTris && app.bottomTris <= kBudgetCloseTris && s.draws <= kBudgetDraws &&
              s.maxBonesPerDraw <= kBudgetBones));
    std::snprintf(buf, sizeof(buf), "LIN %.1fMB  VRAM %.2fMB  APP %.1fMB  romfs %s", linearSpaceFree() / 1048576.0f,
                  vramSpaceFree() / 1048576.0f, osGetMemRegionFree(MEMREGION_APPLICATION) / 1048576.0f,
                  app.romfsOk ? "ok" : "MISSING");
    line(app, 28, buf, okOr(app.romfsOk));
    const audio::DebugInfo ai = audio::debugInfo();
    std::snprintf(buf, sizeof(buf), "AUDIO %s  %s  L%lu S%lu st%d g%.2f", audio::ok() ? "ok" : "OFF (no DSP fw?)",
                  audio::currentMusic()[0] ? audio::currentMusic() : "-", static_cast<unsigned long>(ai.loops),
                  static_cast<unsigned long>(ai.switches), ai.stage, ai.gain);
    line(app, 41, buf, okOr(audio::ok()));
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
    Dragon d = makeEgg(s.nextId++, makePurebred(static_cast<Element>(rng.below(3)), rng), rollSex(rng), now);
    if (asEgg) {
        if (!placeEgg(s, d)) showToast(app, str::kEggToVault);
    } else {
        d.incubationSeconds = kIncubationSeconds;
        tryHatch(d, now, rng);
        d.denSlot = static_cast<u8>(bedForHatchling(s));
        suggestName(d, rng.next(), d.name, sizeof(d.name));
    }
    s.dragons[s.dragonCount++] = d;
    saveNow(app);
}

bool debugMenu(App& app, const Input& in) {
    if (!EC_DEV) return false;
    if (in.down & KEY_SELECT) app.devMenu = !app.devMenu;
    if (!app.devMenu) return false;

    Dragon& d = activeDragon(app);
    const s64 now = nowLocal(app);
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDenPlum, theme::rgba(20, 14, 28));
    text(app, "DEV MENU  (SELECT to close)", 160, 4, 0.5f, theme::kClutchGold);

    struct Item {
        const char* label;
        int id;
    };
    static constexpr Item kItems[] = {
        {"+1 hour", 0},   {"+1 day", 1},     {"+7 days", 2},      {"Fill needs", 3},
        {"Drain needs", 4}, {"Hatch now", 5}, {"Next stage", 6},   {"Next breed", 10},
        {"Next activity", 12}, {"Add dragon", 11}, {"Overlay", 7}, {"Save now", 8},
        {"Reset save", 9}, {"Dusty / bath", 13}, {"Add egg", 14},
    };
    constexpr int kCount = sizeof(kItems) / sizeof(kItems[0]);
    for (int i = 0; i < kCount; ++i) {
        const Rect r{8.0f + (i % 2) * 156.0f, 20.0f + (i / 2) * 25.0f, 148, 22};
        if (!button(app, r, kItems[i].label, in)) continue;
        switch (kItems[i].id) {
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
            case 13: {  // see the dust (D46) without waiting a day: all dusty, then a bath
                const bool dusty = d.dirt[kRegionBack] > 50.0f;
                if (dusty) bathe(d);
                else for (float& dust : d.dirt) dust = 100.0f;
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
    std::snprintf(buf, sizeof(buf), "+%lldh  stars %d  %s  %s/%d  (%.1f, %.1f)  v%.2f", static_cast<long long>(app.game.devOffset / kHour),
                  d.careStars, stageName(d.stage), careActor(app) ? activityName(b.activity) : "-", b.step, b.pos.x,
                  b.pos.y, b.speed);
    text(app, buf, 160, 226, 0.4f, theme::kAsh);
    return true;
}

}  // namespace ec
