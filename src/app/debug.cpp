#include "app/debug.hpp"

#include <cstdio>

#include "app/theme.hpp"
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

}  // namespace

void debugDrawOverlay(App& app) {
    if (!EC_DEV || !app.overlay) return;
    const RenderStats& s = app.stats;
    char buf[96];
    C2D_DrawRectSolid(0, 0, 0, 236, 44, withAlpha(theme::kDenPlum, 0.75f));
    std::snprintf(buf, sizeof(buf), "%4.1fms  CPU %.1f  GPU %.1f  CMD %d%%", app.frameMs, C3D_GetProcessingTime(),
                  C3D_GetDrawingTime(), static_cast<int>(C3D_GetCmdBufUsage() * 100));
    line(app, 2, buf, okOr(app.frameMs <= kBudgetFrameMs));
    std::snprintf(buf, sizeof(buf), "TRI %lu/%lu  DRAW %lu/%lu  BONE %lu/%lu", static_cast<unsigned long>(s.tris),
                  static_cast<unsigned long>(kBudgetTris), static_cast<unsigned long>(s.draws),
                  static_cast<unsigned long>(kBudgetDraws), static_cast<unsigned long>(s.maxBonesPerDraw),
                  static_cast<unsigned long>(kBudgetBones));
    line(app, 15, buf,
         okOr(s.tris <= kBudgetTris && s.draws <= kBudgetDraws && s.maxBonesPerDraw <= kBudgetBones));
    std::snprintf(buf, sizeof(buf), "LIN %.1fMB  VRAM %.2fMB  APP %.1fMB  romfs %s", linearSpaceFree() / 1048576.0f,
                  vramSpaceFree() / 1048576.0f, osGetMemRegionFree(MEMREGION_APPLICATION) / 1048576.0f,
                  app.romfsOk ? "ok" : "MISSING");
    line(app, 28, buf, okOr(app.romfsOk));
}

bool debugMenu(App& app, const Input& in) {
    if (!EC_DEV) return false;
    if (in.down & KEY_SELECT) app.devMenu = !app.devMenu;
    if (!app.devMenu) return false;

    Dragon& d = app.save.dragon;
    const s64 now = nowLocal(app);
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDenPlum, theme::rgba(20, 14, 28));
    text(app, "DEV MENU  (SELECT to close)", 160, 4, 0.5f, theme::kClutchGold);

    struct Item {
        const char* label;
        int id;
    };
    static constexpr Item kItems[] = {
        {"+1 hour", 0}, {"+1 day", 1},        {"+7 days", 2},     {"Fill needs", 3}, {"Drain needs", 4},
        {"Hatch now", 5}, {"Next stage", 6}, {"Overlay", 7},     {"Save now", 8},   {"Reset save", 9},
    };
    for (int i = 0; i < 10; ++i) {
        const Rect r{8.0f + (i % 2) * 156.0f, 26.0f + (i / 2) * 38.0f, 148, 32};
        if (!button(app, r, kItems[i].label, in)) continue;
        switch (kItems[i].id) {
            case 0: app.save.devOffset += kHour; break;
            case 1: app.save.devOffset += kDay; break;
            case 2: app.save.devOffset += 7 * kDay; break;
            case 3: d.needs = Needs{100, 100, 100, 100}; d.upset = false; break;
            case 4: d.needs = Needs{5, 5, 5, 5}; break;
            case 5: if (d.stage == Stage::Egg) d.incubationSeconds = kIncubationSeconds; break;
            case 6: forceNextStage(d, now); break;
            case 7: app.overlay = !app.overlay; break;
            case 8: writeDevSave(app.save); showToast(app, "Saved."); break;
            case 9: app.save = DevSave{}; app.scene = SceneId::Title; app.devMenu = false; writeDevSave(app.save); break;
        }
    }
    char buf[80];
    std::snprintf(buf, sizeof(buf), "clock offset +%lldh  stars %d  stage %s", static_cast<long long>(app.save.devOffset / kHour),
                  d.careStars, stageName(d.stage));
    text(app, buf, 160, 222, 0.4f, theme::kAsh);
    return true;
}

}  // namespace ec
