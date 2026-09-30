#include "app/tracking_ui.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/story.hpp"
#include "core/villagers.hpp"
#include "core/valley.hpp"
#include "core/world.hpp"

namespace ec {

void goalWords(const SaveData& s, const guide::Goal& g, char* title, int titleCap, char* step, int stepCap) {
    title[0] = step[0] = 0;
    switch (g.kind) {
        case Tracked::Quest: {  // (the story's, D137: its step's words with your names in them)
            const story::QuestView q = story::view(s, g.id, 0);
            std::snprintf(title, titleCap, "%s", q.title);
            fillLine(q.done ? str::kQuestDone : q.step, s, step, stepCap);
            break;
        }
        case Tracked::BattleBoard:
            std::snprintf(title, titleCap, "%s", str::kGoalBattle);
            std::snprintf(step, stepCap, str::kGoalBattleStep, trainer::leagueName(s.progress.battleLeague + 1));
            break;
        case Tracked::ShowBoard:
            std::snprintf(title, titleCap, "%s", str::kGoalShow);
            std::snprintf(step, stepCap, str::kGoalShowStep, trainer::leagueName(s.progress.showLeague + 1));
            break;
        case Tracked::Hollow:
            std::snprintf(title, titleCap, "%s", str::kGoalHollow);
            if (s.progress.hollowDeepest) std::snprintf(step, stepCap, str::kGoalHollowStep, s.progress.hollowDeepest);
            else std::snprintf(step, stepCap, "%s", str::kGoalHollowNew);
            break;
        case Tracked::Place:
            if (g.id >= 0 && g.id < world::placeCount()) {
                std::snprintf(title, titleCap, "%s", world::placeInfo(g.id).name);
                std::snprintf(step, stepCap, str::kGoalPlaceStep, world::placeInfo(g.id).name);
            }
            break;
        default: break;
    }
}

void trackFlag(float x, float y, float size, float t, bool on) {
    const float wave = on ? std::sin(t * 4.0f) * size * 0.07f : 0.0f;
    const float top = y - size;
    const u32 pole = on ? theme::kDenPlum : withAlpha(theme::kShell, 0.35f);
    C2D_DrawRectSolid(x - 0.8f, top, 0.5f, 1.6f, size, pole);
    if (on) {  // a plum edge so it reads on the pale map as on the dark pages
        C2D_DrawTriangle(x, top - 1.2f, theme::kDenPlum, x + size * 0.82f, top + size * 0.24f + wave, theme::kDenPlum, x,
                         top + size * 0.58f, theme::kDenPlum, 0.5f);
        C2D_DrawTriangle(x + 0.8f, top + 0.4f, theme::kClutchGold, x + size * 0.68f, top + size * 0.24f + wave,
                         theme::kClutchGold, x + 0.8f, top + size * 0.44f, theme::kEmber, 0.5f);
        C2D_DrawCircleSolid(x, y, 0.5f, size * 0.18f, theme::kDenPlum);
    } else {
        const u32 c = withAlpha(theme::kShell, 0.3f);
        C2D_DrawTriangle(x + 0.8f, top, c, x + size * 0.7f, top + size * 0.24f, c, x + 0.8f, top + size * 0.48f, c, 0.5f);
    }
}

void drawTrackedOnMap(App& app, const Valley& v, float mapX, float mapY, float mapSize) {
    const guide::Goal g = guide::current(app.game);
    if (g.kind == Tracked::None || v.size() <= 0) return;
    const guide::Target t = guide::target(app.game, v, g, {app.game.world.x, app.game.world.y}, nowLocal(app));
    if (!t.valid) return;
    const float k = mapSize / v.size();
    float mx = mapX + (t.at.x - v.x0) * k, my = mapY + (1.0f - (t.at.y - v.y0) / v.size()) * mapSize;
    mx = std::fmax(mapX + 2, std::fmin(mapX + mapSize - 2, mx));
    my = std::fmax(mapY + 2, std::fmin(mapY + mapSize - 2, my));
    if (t.area) {  // somewhere in here: a soft circle breathing, a ring of dots turning round it
        const float r = std::fmax(11.0f, t.radius * k);
        const float pulse = 0.5f + 0.5f * std::sin(app.t * 2.4f);
        C2D_DrawCircleSolid(mx, my, 0.5f, r, withAlpha(theme::kClutchGold, 0.2f + 0.12f * pulse));
        for (int i = 0; i < 16; ++i) {
            const float a = app.t * 0.5f + i * 0.3927f;
            C2D_DrawCircleSolid(mx + std::cos(a) * r, my + std::sin(a) * r, 0.5f, 1.4f, withAlpha(theme::kEmber, 0.95f));
        }
        trackFlag(mx, my + 4, 10, app.t, true);
        return;
    }
    // Here: the flag, with a ping going out from its foot.
    const float ping = std::fmod(app.t, 1.4f) / 1.4f;
    C2D_DrawCircleSolid(mx, my, 0.5f, 3 + 10 * ping, withAlpha(theme::kClutchGold, 0.55f * (1.0f - ping)));
    trackFlag(mx, my, 13, app.t, true);
}

int textTwoLines(App& app, const char* s, float x, float y, float scale, u32 color, float w, float lineGap) {
    if (textWidth(app, s, scale) <= w) {
        text(app, s, x, y, scale, color, C2D_AlignLeft);
        return 1;
    }
    const int len = static_cast<int>(std::strlen(s));
    int cut = -1;
    for (int i = 1; i < len - 1; ++i)
        if (s[i] == ' ' && (cut < 0 || std::abs(i - len / 2) < std::abs(cut - len / 2))) cut = i;
    if (cut < 0 || len >= 128) {
        text(app, s, x, y, scale, color, C2D_AlignLeft, w);
        return 1;
    }
    char a[128], b[128];
    std::memcpy(a, s, cut);
    a[cut] = 0;
    std::snprintf(b, sizeof(b), "%s", s + cut + 1);
    text(app, a, x, y, scale, color, C2D_AlignLeft, w);
    text(app, b, x, y + lineGap, scale, color, C2D_AlignLeft, w);
    return 2;
}

void drawTrackedPanel(App& app, float x, float y, float w) {
    const guide::Goal g = guide::current(app.game);
    if (g.kind == Tracked::None) return;
    char title[64], step[96];
    goalWords(app.game, g, title, sizeof(title), step, sizeof(step));
    trackFlag(x + 3, y + 20, 11, app.t, true);
    text(app, str::kTracking, x + 13, y, 0.3f, withAlpha(theme::kShell, 0.6f), C2D_AlignLeft, w - 13);
    text(app, title, x + 13, y + 9, 0.4f, theme::kClutchGold, C2D_AlignLeft, w - 13);
    textTwoLines(app, step, x, y + 24, 0.34f, withAlpha(theme::kShell, 0.85f), w, 11);
}

}  // namespace ec
