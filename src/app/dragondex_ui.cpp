// The Dragondex (D55, D66; WP12; by kind since DR3, D80): the collection book, opened from the
// system menu. The bottom screen is the book, seven kinds a page, each with its typing (a dot per
// element, D83) and its four colourings across (the last is the rare one); the top shows the one
// picked, turning slowly, once you've met it, and always its typing, so a missing crossbreed
// says which two types to pair. A completed kind's banner can be hung in the den.
#include "app/dragondex_ui.hpp"

#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/render3d.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/dragondex.hpp"
#include "core/kinds.hpp"
#include "core/profile.hpp"

namespace ec {
namespace {

constexpr int kRowsPerPage = 7;
constexpr float kRowY = 30, kRowH = 22, kCellX = 118, kCellW = 46, kCellH = 18;

int pages() { return (kindCount() + kRowsPerPage - 1) / kRowsPerPage; }

// A kind's typing as chips centred on cx: each element's name on its glow colour.
void typingChips(App& app, const KindInfo& k, float cx, float y) {
    float widths[2] = {}, total = 0;
    for (int e = 0; e < k.elementCount; ++e) {
        widths[e] = textWidth(app, elementName(k.elements[e]), 0.45f) + 14;
        total += widths[e] + (e ? 6 : 0);
    }
    float x = cx - total / 2;
    for (int e = 0; e < k.elementCount; ++e) {
        const Rgb g = elementGlow(k.elements[e]);
        panel({x, y, widths[e], 17}, fromRgb(g, 230));
        textCentered(app, elementName(k.elements[e]), x + widths[e] / 2, y + 8.5f, 0.45f, theme::rgba(30, 20, 36), widths[e]);
        x += widths[e] + 6;
    }
}

}  // namespace

void openDex(App& app) {
    app.menu = MenuPage::Dex;
    if (app.dexPick >= kindCount()) app.dexPick = 0;
    if (app.dexLook >= kKindVariants) app.dexLook = 0;
    app.dexPage = static_cast<u8>(app.dexPick / kRowsPerPage);
}

void drawDexTop(App& app) {
    const s64 now = nowLocal(app);
    verticalGradient(0, 0, kTopW, kScreenH, theme::rgba(40, 28, 52), theme::kDenPlum);
    textCentered(app, str::kDex, 200, 16, 0.75f, theme::kClutchGold, 380, Face::Title);
    const int kind = app.dexPick < kindCount() ? app.dexPick : 0, variant = app.dexLook % kKindVariants;
    const KindInfo& k = kindInfo(kind);
    const SaveData& s = app.game;
    char line[80];
    if (dexHas(s, kind, variant)) {
        static Dragon shown;
        shown = dexDragon(kind, variant, dexStage(s, kind, variant));  // (as you know it: young, if yours are)
        C2D_DrawEllipseSolid(120 + r3d::eyeShift(), 190, 0, 160, 26, withAlpha(theme::rgba(0, 0, 0), 0.25f));
        if (r3d::ready()) r3d::drawShowcase(app, shown, nullptr, now, 0.6f * std::sin(app.t * 0.4f));
        std::snprintf(line, sizeof(line), "%s %s", k.variants[variant].name, k.title);
        textCentered(app, line, 200, 42, 0.6f, theme::kShell, 380);
    } else {
        egg(200, 132, 64, 84, {70, 58, 84}, {90, 70, 110}, 0.2f);
        textCentered(app, "?", 200, 132, 1.2f, withAlpha(theme::kShell, 0.6f), 60, Face::Title);
        textCentered(app, str::kDexUnknown, 200, 42, 0.5f, withAlpha(theme::kShell, 0.7f), 380);
        // How you might find it (D83): a crossbreed's two types to pair; a first kind's own.
        if (k.elementCount > 1)
            std::snprintf(line, sizeof(line), "A crossbreed: pair a %s kind with a %s kind", elementName(k.elements[0]),
                          elementName(k.elements[1]));
        else
            std::snprintf(line, sizeof(line), "One of the first kinds: %s", elementName(k.elements[0]));
        textCentered(app, line, 200, 188, 0.45f, withAlpha(theme::kShell, 0.75f), 380);
    }
    typingChips(app, k, 200, 56);
    int seen = 0;
    for (int v = 0; v < kKindVariants; ++v) seen += dexHas(s, kind, v);
    char els[32];
    kindElements(kind, els, sizeof(els));
    std::snprintf(line, sizeof(line), "%s (%s, %s): %d of %d colourings", k.title, els, rarityName(k.rarity), seen,
                  kKindVariants);
    textCentered(app, line, 200, 214, 0.42f, withAlpha(theme::kShell, 0.8f), 390);
    if (dexComplete(s, kind)) textCentered(app, str::kDexDone, 200, 228, 0.42f, theme::kClutchGold, 380);
}

void drawDexBottom(App& app, const Input& in) {
    SaveData& s = app.game;
    char line[48];
    const int np = pages();
    std::snprintf(line, sizeof(line), "%s  %d / %d", str::kDex, dexCount(s), dexEntries());
    text(app, line, 160, 6, 0.55f, theme::kClutchGold);
    if (in.down & (KEY_L | KEY_DLEFT)) app.dexPage = static_cast<u8>((app.dexPage + np - 1) % np);
    if (in.down & (KEY_R | KEY_DRIGHT)) app.dexPage = static_cast<u8>((app.dexPage + 1) % np);
    for (int row = 0; row < kRowsPerPage; ++row) {
        const int kind = app.dexPage * kRowsPerPage + row;
        if (kind >= kindCount()) break;
        const KindInfo& k = kindInfo(kind);
        const float y = kRowY + row * kRowH;
        text(app, k.title, 10, y + 3, 0.42f, theme::kShell, C2D_AlignLeft, 74);
        for (int e = 0; e < k.elementCount; ++e)  // its typing: a dot per element (D83)
            C2D_DrawCircleSolid(88 + e * 10, y + 9, 0.5f, 4, fromRgb(elementGlow(k.elements[e])));
        if (dexComplete(s, kind)) heart(109, y + 9, 8, theme::kClutchGold);
        Rgb base, accent, glow;
        kindColours(kind, base, accent, glow);
        for (int v = 0; v < kKindVariants; ++v) {
            const Rect r{kCellX + v * (kCellW + 2), y, kCellW, kCellH};
            const bool have = dexHas(s, kind, v);
            const bool rare = v == k.rareVariant;
            const bool picked = kind == app.dexPick && v == app.dexLook;
            if (picked) panel({r.x - 2, r.y - 2, r.w + 4, r.h + 4}, theme::kClutchGold);
            panel(r, have ? fromRgb(k.variants[v].pal[kPalBase]) : theme::rgba(40, 30, 50));
            char cell[4];
            std::snprintf(cell, sizeof(cell), "%s", rare ? "*" : (v == 0 ? "1" : v == 1 ? "2" : "3"));
            text(app, have ? cell : "?", r.x + r.w / 2, r.y + 2, 0.42f,
                 have ? theme::rgba(30, 20, 36) : withAlpha(theme::kShell, 0.4f));
            if (in.tapped && r.contains(in.tx, in.ty)) {
                app.dexPick = static_cast<u8>(kind);
                app.dexLook = static_cast<u8>(v);
                audio::playSfx(audio::Sfx::Tap);
            }
        }
    }
    // The rare colourings met (the last column: the rarest to find).
    std::snprintf(line, sizeof(line), "Rare colourings found: %d of %d", dexRareCount(s), kindCount());
    text(app, line, 10, 184, 0.4f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft, 300);
    std::snprintf(line, sizeof(line), "%d / %d", app.dexPage + 1, np);
    text(app, line, 88, 212, 0.42f, withAlpha(theme::kShell, 0.7f), C2D_AlignLeft);
    if (button(app, {8, 204, 36, 30}, "<", in)) app.dexPage = static_cast<u8>((app.dexPage + np - 1) % np);
    if (button(app, {48, 204, 36, 30}, ">", in)) app.dexPage = static_cast<u8>((app.dexPage + 1) % np);
    if (dexComplete(s, app.dexPick)) {
        const bool up = bannerKind(s) == app.dexPick;
        if (button(app, {116, 204, 106, 30}, up ? str::kDexTakeDown : str::kDexHang, in)) {
            if (up)
                takeDownBanner(s);
            else
                hangBanner(s, app.dexPick);
            audio::playSfx(audio::Sfx::Confirm);
            saveNow(app);
        }
    }
    if (button(app, {228, 204, 84, 30}, str::kBack, in) || (in.down & KEY_B)) app.menu = MenuPage::Main;
}

}  // namespace ec
