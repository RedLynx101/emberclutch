// The Sanctuary (dragons) and the Cold Vault (eggs), Alpha 2 WP2 (GDD 8): who's kept there,
// in a grid on the bottom screen and one at a time on the top; bring one home to the den.
// Keepers look after the dragons (needs never below 50, no growing up while away); eggs
// wait, cool, until a nest is free (core/den_roster, core/dragon simulate).
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/care_ui.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/den_roster.hpp"
#include "core/egg.hpp"
#include "core/genetics.hpp"
#include "core/kinds.hpp"

namespace ec {
namespace {

constexpr int kCols = 3, kRows = 2, kPerPage = kCols * kRows;

u32 col(u8 r, u8 g, u8 b, float a = 1.0f) { return withAlpha(theme::rgba(r, g, b), a); }

bool vault(const App& app) { return app.scene == SceneId::Vault; }

// Who's kept here, in save order.
int listed(const App& app, int* out) {
    const Location where = vault(app) ? Location::Vault : Location::Sanctuary;
    int n = 0;
    for (int i = 0; i < app.game.dragonCount; ++i)
        if (app.game.dragons[i].location == where) out[n++] = i;
    return n;
}

void keepPickInRange(App& app, int n) {
    if (app.storePick >= n) app.storePick = n > 0 ? n - 1 : 0;
    if (app.storePick < 0) app.storePick = 0;
    app.storePage = app.storePick / kPerPage;
}

void update(App& app, const Input& in) {
    int list[kMaxDragons];
    const int n = listed(app, list);
    if (in.down & KEY_DRIGHT) ++app.storePick;
    if (in.down & KEY_DLEFT) --app.storePick;
    if (in.down & KEY_DDOWN) app.storePick += kCols;
    if (in.down & KEY_DUP) app.storePick -= kCols;
    if (in.down & KEY_R) app.storePick += kPerPage;
    if (in.down & KEY_L) app.storePick -= kPerPage;
    keepPickInRange(app, n);
    if (in.down & KEY_B) {
        openMap(app);
        audio::playSfx(audio::Sfx::Back);
    }
    // The keepers' time passes like everyone else's.
    app.simAccum += app.dt;
    if (app.simAccum >= 1.0f) {
        tickWorld(app);
        app.simAccum = 0;
    }
}

// The top screen's setting: the keepers' meadow, or the vault's cool blue hall.
void drawSetting(App& app) {
    if (vault(app)) {
        verticalGradient(0, 0, kTopW, kScreenH, col(40, 56, 88), col(96, 130, 170));
        for (int i = 0; i < 9; ++i) {  // icicles
            const float x = 20.0f + i * 45, h = 18.0f + 12 * ((i * 7) % 3);
            C2D_DrawTriangle(x - 7, 0, col(210, 232, 250, 0.8f), x + 7, 0, col(190, 216, 244, 0.8f), x, h,
                             col(240, 250, 255, 0.9f), 0);
        }
        C2D_DrawEllipseSolid(90 + r3d::eyeShift(), 170, 0, 220, 60, col(150, 180, 214, 0.6f));  // a frosty plinth
    } else {
        verticalGradient(0, 0, kTopW, kScreenH, col(126, 176, 226), col(250, 222, 176));
        const float hills = r3d::eyeShift(2.5f), fence = r3d::eyeShift(1.2f);  // behind the dragon in 3D
        C2D_DrawEllipseSolid(-80 + hills, 130, 0, 330, 150, col(140, 182, 110));
        C2D_DrawEllipseSolid(150 + hills, 140, 0, 340, 150, col(122, 168, 100));
        for (int i = 0; i < 12; ++i) C2D_DrawRectSolid(10.0f + i * 34 + fence, 196, 0, 5, 30, col(150, 110, 70));
        C2D_DrawRectSolid(10 + fence, 204, 0, 380, 4, col(150, 110, 70));
    }
}

void drawTop(App& app) {
    drawSetting(app);
    int list[kMaxDragons];
    const int n = listed(app, list);
    if (n == 0) {
        textCentered(app, vault(app) ? str::kVault : str::kSanctuary, 200, 30, 1.0f, theme::kDenPlum, 380, Face::Title);
        return;
    }
    keepPickInRange(app, n);
    const Dragon& d = app.game.dragons[list[app.storePick]];
    static EggMotion egg;  // a gentle rock
    egg.update(app.dt, 0.0f, app.rng);
    if (egg.rock < 0.02f) egg.knock(0.05f, 0);
    if (r3d::ready()) r3d::drawShowcase(app, d, &egg, nowLocal(app), 0.5f * std::sin(app.t * 0.4f));
    char line[80];
    if (d.stage == Stage::Egg) {
        std::snprintf(line, sizeof(line), "%s %s  -  %d%% %s", kindTitle(d), str::kEggSuffix,
                      static_cast<int>(eggProgress(d) * 100), str::kIncubated);
    } else {
        char kind[40];
    kindName(d, kind, sizeof(kind));
    std::snprintf(line, sizeof(line), "%s  -  %s %s %s", d.name, sexName(d.sex), kind, stageName(d.stage));
    }
    textCentered(app, line, 200, 16, 0.6f, theme::kShell, 390);
}

// One card: a heart (dragons) or egg (eggs) in its element's glow, its name, its stage.
void card(App& app, const Rect& r, const Dragon& d, bool picked) {
    panel(r, picked ? withAlpha(theme::kClutchGold, 0.85f) : withAlpha(theme::kShell, 0.2f));
    const Rgb glowC = kindGlow(d);
    if (d.stage == Stage::Egg) {
        egg(r.x + 16, r.y + r.h * 0.5f, 18, 24, kindShell(d), glowC, 0.3f + 0.7f * eggProgress(d));
    } else {
        glow(r.x + 16, r.y + r.h * 0.5f, 12, fromRgb(glowC), 0.6f);
        heart(r.x + 16, r.y + r.h * 0.5f, 12, fromRgb(glowC));
    }
    const u32 ink = picked ? theme::kDenPlum : theme::kShell;
    char line[40];
    if (d.stage == Stage::Egg) {
        text(app, kindTitle(d), r.x + 32, r.y + 10, 0.42f, ink, C2D_AlignLeft, r.w - 36);
        std::snprintf(line, sizeof(line), "%d%%", static_cast<int>(eggProgress(d) * 100));
    } else {
        text(app, d.name, r.x + 32, r.y + 10, 0.45f, ink, C2D_AlignLeft, r.w - 36);
        std::snprintf(line, sizeof(line), "%s %s", sexName(d.sex), stageName(d.stage));
    }
    text(app, line, r.x + 32, r.y + 30, 0.36f, withAlpha(ink, 0.8f), C2D_AlignLeft, r.w - 36);
}

void drawBottom(App& app, const Input& in) {
    verticalGradient(0, 0, kBotW, kScreenH, vault(app) ? col(60, 80, 116) : theme::kDusk, theme::kDenPlum);
    int list[kMaxDragons];
    const int n = listed(app, list);
    keepPickInRange(app, n);
    char line[64];
    if (!vault(app) && n == 1)
        std::snprintf(line, sizeof(line), str::kSanctuaryOne, static_cast<int>(kMaxDragons));
    else
        std::snprintf(line, sizeof(line), vault(app) ? str::kVaultCount : str::kSanctuaryCount, n,
                      vault(app) ? kVaultEggs : static_cast<int>(kMaxDragons));
    if (n == 0) app.storeProfile = false;
    if (app.storeProfile) {  // the picked one's profile (WP8): about it, its family
        const Dragon& d = app.game.dragons[list[app.storePick]];
        char title[40];
        if (d.stage == Stage::Egg) std::snprintf(title, sizeof(title), "%s %s", kindTitle(d), str::kEggSuffix);
        else std::snprintf(title, sizeof(title), "%s", d.name);
        textCentered(app, title, 160, 19, 0.75f, theme::kClutchGold, 300, Face::Title);
        care::drawProfilePages(app, in, d, nowLocal(app), app.storeProfileTab);
    } else {
        textCentered(app, vault(app) ? str::kVault : str::kSanctuary, 160, 14, 0.75f, theme::kClutchGold, 300, Face::Title);
        textCentered(app, line, 160, 32, 0.4f, withAlpha(theme::kShell, 0.75f), 300);
    }
    if (n == 0) {
        textCentered(app, vault(app) ? str::kVaultEmpty : str::kSanctuaryEmpty, 160, 110, 0.48f, theme::kShell, 290);
    }
    const int first = app.storePage * kPerPage;
    for (int k = 0; k < kPerPage && first + k < n && !app.storeProfile; ++k) {
        const Rect r{8.0f + (k % kCols) * 103.0f, 44.0f + (k / kCols) * 60.0f, 98, 54};
        card(app, r, app.game.dragons[list[first + k]], first + k == app.storePick);
        if (in.released && r.contains(in.rx, in.ry)) {
            app.storePick = first + k;
            audio::playSfx(audio::Sfx::Tap);
        }
    }
    const int pages = (n + kPerPage - 1) / kPerPage;
    if (pages > 1 && !app.storeProfile) {
        std::snprintf(line, sizeof(line), "%d / %d", app.storePage + 1, pages);
        textCentered(app, line, 160, 176, 0.42f, theme::kShell);
        if (button(app, {96, 164, 32, 26}, "<", in) && app.storePage > 0) app.storePick = (app.storePage - 1) * kPerPage;
        if (button(app, {192, 164, 32, 26}, ">", in) && app.storePage + 1 < pages) app.storePick = (app.storePage + 1) * kPerPage;
    }
    if (n > 0 && button(app, {8, 200, 112, 34}, app.storeProfile ? str::kProfileClose : str::kProfile, in)) {
        app.storeProfile = !app.storeProfile;
        if (vault(app)) app.storeProfileTab = 1;  // an egg's page is its family
    }
    if (n > 0 && button(app, {124, 200, 100, 34}, str::kToTheDen, in)) {
        const int idx = list[app.storePick];
        if (bringHome(app.game, idx, nowLocal(app))) {
            const Dragon& d = app.game.dragons[idx];
            if (d.stage == Stage::Egg)
                showToast(app, str::kEggHome);
            else
                showToastf(app, str::kIsHome, d.name);
            audio::playSfx(audio::Sfx::Confirm);
            saveNow(app);
        } else {
            showToast(app, app.game.dragons[idx].stage == Stage::Egg ? str::kNestsFull : str::kDenFull);
            audio::playSfx(audio::Sfx::Error);
        }
    }
    if (button(app, {228, 200, 84, 34}, str::kMap, in)) {
        app.storeProfile = false;
        openMap(app);
    }
}

}  // namespace

const SceneFns kSanctuaryScene{update, drawTop, drawBottom};
const SceneFns kVaultScene{update, drawTop, drawBottom};

}  // namespace ec
