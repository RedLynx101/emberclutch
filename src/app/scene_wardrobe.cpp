// The wardrobe (1.0, D90): a dragon dressed from what you own, a slot at a time (head, neck, back,
// tail) and its dye. It stands on the top screen in a curtained corner, turning slowly (the circle
// pad turns it), and gives a little hop as each thing goes on. What you pick is on at once (it's
// saved as you leave). Opened from Moonpetal Glade and from the den (openWardrobe).
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/tips_ui.hpp"
#include "app/render3d.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "app/wardrobe.hpp"
#include "core/accessories.hpp"
#include "core/kinds.hpp"
#include "core/trainer.hpp"
#include "core/valley.hpp"

namespace ec {
namespace {

constexpr int kTabs = kWearSlots + 1;  // the four slots, then the dye
constexpr int kCols = 5, kRows = 3;

struct WardrobeScene {
    SceneId back = SceneId::Den;
    int dragon = -1;
    int tab = 0;
    float spin = 0.5f;
    float idle = 0;       // seconds since the pad turned it: then it turns on its own
    bool held = false;    // a scripted turn: held still
    float hop = 0;        // seconds of the little hop left
    float zoom = 0;       // the camera on the slot being dressed (0: the whole dragon)
    int picked = -1;      // the last thing tapped (its name and styles shown); a dye as -2 - dye
    const char* music = nullptr;
};

WardrobeScene& ws() {
    static WardrobeScene s;
    return s;
}

u32 col(u8 r, u8 g, u8 b, float a = 1.0f) { return withAlpha(theme::rgba(r, g, b), a); }

Dragon* dressed(App& app) {
    const WardrobeScene& s = ws();
    return s.dragon >= 0 && s.dragon < app.game.dragonCount ? &app.game.dragons[s.dragon] : nullptr;
}

void leave(App& app) {
    WardrobeScene& s = ws();
    audio::playSfx(audio::Sfx::Back);
    saveNow(app);
    if (s.back == SceneId::Valley) {
        backToValley(app);  // back out where you stood at the glade, dressed (D131)
    } else {
        app.scene = s.back;
    }
}

void update(App& app, const Input& in) {
    WardrobeScene& s = ws();
    app.simAccum += app.dt;
    if (app.simAccum >= 1.0f) {
        tickWorld(app);
        app.simAccum = 0;
    }
    if (in.down & (KEY_B | KEY_START)) {
        leave(app);
        return;
    }
    if (in.down & KEY_L) s.tab = (s.tab + kTabs - 1) % kTabs;
    if (in.down & KEY_R) s.tab = (s.tab + 1) % kTabs;
    // The circle pad turns it; let go and after a moment it turns slowly on its own.
    if (std::fabs(in.padX) > 0.2f) {
        s.spin += in.padX * 2.4f * app.dt;
        s.idle = 0;
        s.held = false;
    } else if (!s.held && (s.idle += app.dt) > 2.0f) {
        s.spin += 0.35f * app.dt;
    }
    if (s.hop > 0) s.hop -= app.dt;
    const float want = s.tab < kWearSlots && s.tab != static_cast<int>(WearSlot::Back) ? 1.0f : s.tab == static_cast<int>(WearSlot::Back) ? 0.5f : 0.0f;
    s.zoom += (want - s.zoom) * std::fmin(1.0f, app.dt * 3.0f);
}

void drawTop(App& app) {
    WardrobeScene& s = ws();
    // A corner of a dressing tent: dusk behind, rose curtains drawn back, a warm pool of light.
    verticalGradient(0, 0, kTopW, kScreenH, col(62, 44, 84), col(150, 104, 126));
    for (int k = 0; k < 14; ++k) {  // lanterns' glimmer through the canvas
        const float x = 40 + k * 23.0f + 9 * std::sin(k * 2.3f), y = 26 + 30 * std::fabs(std::sin(k * 1.7f));
        glow(x, y, 6, col(255, 226, 170), 0.25f + 0.2f * std::sin(app.t * 1.3f + k));
    }
    C2D_DrawEllipseSolid(200 - 110, 176, 0, 220, 44, col(255, 230, 190, 0.22f));
    C2D_DrawEllipseSolid(200 - 80, 186, 0, 160, 26, col(90, 50, 60, 0.3f));
    for (int side = 0; side < 2; ++side) {
        const float x0 = side ? kTopW - 78 : 0;
        C2D_DrawRectSolid(x0, 0, 0, 78, kScreenH, col(170, 66, 88));
        for (int f = 0; f < 4; ++f) C2D_DrawRectSolid(x0 + 10 + f * 17, 0, 0, 5, kScreenH, col(130, 44, 66));
        C2D_DrawRectSolid(side ? x0 : x0 + 70, 0, 0, 8, kScreenH, col(214, 150, 90));  // the gold edge
    }
    C2D_DrawRectSolid(0, 0, 0, kTopW, 14, col(214, 150, 90));
    const Dragon* d = dressed(app);
    if (d && r3d::ready())
        r3d::drawDressing(app, *d, nowLocal(app), s.spin, s.tab < kWearSlots ? s.tab : -1, s.zoom, s.hop > 0 ? ClipId::Hop : ClipId::Idle);
    textCentered(app, str::kWardrobe, 200, 28, 0.85f, theme::kShell, 300, Face::Title);
    if (d) {
        char line[64];
        std::snprintf(line, sizeof(line), "%s  -  %s", d->name, kindTitle(*d));
        textCentered(app, line, 200, 222, 0.5f, theme::kShell, 240);
    }
}

// A cell of the grid: its box, highlighted if it's what's on; true when tapped.
bool cell(App& app, const Input& in, int i, bool on, Rect& r) {
    r = {8.0f + (i % kCols) * 61.0f, 38.0f + (i / kCols) * 51.0f, 57, 48};
    panel(r, on ? withAlpha(theme::kClutchGold, 0.55f) : withAlpha(theme::kShell, 0.14f));
    return in.released && r.contains(in.rx, in.ry);
}

void drawBottom(App& app, const Input& in) {
    WardrobeScene& s = ws();
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    Dragon* d = dressed(app);
    static const char* const kTabNames[kTabs] = {str::kSlotHead, str::kSlotNeck, str::kSlotBack, str::kSlotTail, str::kWardrobeDye};
    for (int k = 0; k < kTabs; ++k) {
        const Rect r{6.0f + k * 62.0f, 5, 58, 27};
        const bool on = s.tab == k;
        panel(r, on ? theme::kClutchGold : withAlpha(theme::kShell, 0.2f));
        textCentered(app, kTabNames[k], r.x + r.w / 2, r.y + r.h / 2, 0.42f, on ? theme::kDenPlum : theme::kShell, r.w - 4);
        if (in.released && r.contains(in.rx, in.ry) && !on) {
            s.tab = k;
            audio::playSfx(audio::Sfx::Tap);
        }
    }
    if (!d || d->stage == Stage::Egg) {
        textCentered(app, str::kWardrobeEgg, 160, 110, 0.5f, theme::kShell, 290);
    } else if (s.tab < kWearSlots) {  // a slot: nothing, then what you own for it
        const WearSlot slot = static_cast<WearSlot>(s.tab);
        int owned[kAccessoryCount];
        const int n = acc::ownedFor(app.game, slot, owned, kAccessoryCount);
        const int wearing = acc::worn(*d, slot);
        Rect r;
        if (cell(app, in, 0, wearing < 0, r) && wearing >= 0) {
            acc::takeOff(*d, slot);
            s.picked = -1;
            audio::playSfx(audio::Sfx::Tap);
        }
        textCentered(app, str::kWardrobeNone, r.x + r.w / 2, r.y + r.h / 2, 0.42f, theme::kShell, r.w - 4);
        for (int i = 0; i < n && i + 1 < kCols * kRows; ++i) {
            const int a = owned[i];
            if (cell(app, in, i + 1, wearing == a, r) && wearing != a && acc::putOn(app.game, *d, a)) {
                s.picked = a;
                s.hop = 0.9f;
                audio::playSfx(audio::Sfx::Equip);
            }
            drawAccessoryIcon(a, r.x + r.w / 2, r.y + 18, 28);
            textCentered(app, accessoryInfo(a).name, r.x + r.w / 2, r.y + 40, 0.3f, theme::kShell, r.w - 4);
        }
        if (n == 0) {
            char line[96];
            std::snprintf(line, sizeof(line), str::kWardrobeEmpty, slotName(slot));
            text(app, line, 76, 50, 0.38f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft, 236);
        }
    } else {  // the dye: its own colours, then the dyes you own
        int shown = 0;
        Rect r;
        for (int dye = 0; dye < dyeCount() && shown < kCols * kRows; ++dye) {
            if (!trainer::ownsDye(app.game, dye)) continue;
            if (cell(app, in, shown, d->dye == dye, r) && d->dye != dye && acc::dyeWith(app.game, *d, dye)) {
                s.picked = -2 - dye;
                s.hop = 0.9f;
                audio::playSfx(audio::Sfx::Equip);
            }
            drawDyeSwatch(dye, r.x + r.w / 2, r.y + 18, 26);
            textCentered(app, dye ? dyeInfo(dye).name : str::kWardrobeNatural, r.x + r.w / 2, r.y + 40, 0.3f, theme::kShell, r.w - 4);
            ++shown;
        }
    }
    // What was last put on, and its styles.
    char line[96] = {};
    if (s.picked >= 0) {
        const Accessory& a = accessoryInfo(s.picked);
        int at = std::snprintf(line, sizeof(line), "%s:", a.name);
        for (int b = 0; b < kStyleTags && at < static_cast<int>(sizeof(line)) - 12; ++b)
            if ((a.styles >> b) & 1u) at += std::snprintf(line + at, sizeof(line) - at, " %s", styleName(b));
    } else if (s.picked <= -2) {
        std::snprintf(line, sizeof(line), str::kWardrobeDyed, dyeInfo(-2 - s.picked).name);
    }
    text(app, line[0] ? line : str::kWardrobeHelp, 10, 194, 0.36f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft, 196);
    if (button(app, {214, 204, 100, 32}, str::kDone, in, theme::kClutchGold)) leave(app);
}

}  // namespace

void openWardrobe(App& app, int index, SceneId back) {
    showTip(app, tips::kTipWardrobe);
    WardrobeScene& s = ws();
    const int tab = s.tab;
    s = WardrobeScene{};
    s.tab = tab;  // (the slot you had open last time)
    s.dragon = index;
    s.back = back;
    s.music = audio::currentMusic();
    app.scene = SceneId::Wardrobe;
    audio::playSfx(audio::Sfx::Notice);
}

const char* wardrobeMusic(const App& app) {
    (void)app;
    return ws().music;
}

void wardrobeTurn(float spin) {
    ws().spin = spin;
    ws().held = true;
}

void wardrobeTab(int tab) {
    ws().tab = tab < 0 ? 0 : (tab >= kTabs ? kTabs - 1 : tab);
    ws().zoom = 0;
}

const SceneFns kWardrobeScene{update, drawTop, drawBottom};

}  // namespace ec
