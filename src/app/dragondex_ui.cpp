// The Dragondex (D55, D66; WP12): the collection book, opened from the system menu. The bottom
// screen is the book, seven breeds a page, their four looks across; the top shows the one
// picked, turning slowly, once you've met it. A completed breed's banner can be hung in the den.
#include "app/dragondex_ui.hpp"

#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/render3d.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/dragondex.hpp"
#include "core/profile.hpp"

namespace ec {
namespace {

constexpr int kRowsPerPage = 7;
constexpr int kPages = (kBreedCount + kRowsPerPage - 1) / kRowsPerPage;
constexpr float kRowY = 30, kRowH = 22, kCellX = 118, kCellW = 46, kCellH = 18;
const char* const kLookLetter[kLookCount] = {"C", "P", "T", "W"};

u8 kRareBit(int k) {
    static constexpr u8 kBits[4] = {kRareIridescent, kRareMelanistic, kRareLeucistic, kRareStarspeckle};
    return kBits[k];
}

}  // namespace

void openDex(App& app) {
    app.menu = MenuPage::Dex;
    app.dexPage = static_cast<u8>(app.dexPick / kRowsPerPage);
}

void drawDexTop(App& app) {
    const s64 now = nowLocal(app);
    verticalGradient(0, 0, kTopW, kScreenH, theme::rgba(40, 28, 52), theme::kDenPlum);
    textCentered(app, str::kDex, 200, 16, 0.75f, theme::kClutchGold, 380, Face::Title);
    const int breed = app.dexPick, look = app.dexLook;
    const SaveData& s = app.game;
    char line[64];
    if (dexHas(s, breed, look)) {
        static Dragon shown;
        shown = dexDragon(breed, look);
        C2D_DrawEllipseSolid(120, 190, 0, 160, 26, withAlpha(theme::rgba(0, 0, 0), 0.25f));
        if (r3d::ready()) r3d::drawShowcase(app, shown, nullptr, now, 0.6f * std::sin(app.t * 0.4f));
        lookBreedName(static_cast<u8>(look), shown.genome, line, sizeof(line));
        textCentered(app, line, 200, 42, 0.6f, theme::kShell, 380);
    } else {
        egg(200, 132, 64, 84, {70, 58, 84}, {90, 70, 110}, 0.2f);
        textCentered(app, "?", 200, 132, 1.2f, withAlpha(theme::kShell, 0.6f), 60, Face::Title);
        textCentered(app, str::kDexUnknown, 200, 42, 0.5f, withAlpha(theme::kShell, 0.7f), 380);
    }
    u8 a, b;
    breedAlleles(breed, a, b);
    int seen = 0;
    for (int l = 0; l < kLookCount; ++l) seen += dexHas(s, breed, l);
    std::snprintf(line, sizeof(line), str::kDexBreedLine, breedName(static_cast<Element>(a), static_cast<Element>(b)),
                  seen, kLookCount);
    textCentered(app, line, 200, 214, 0.45f, withAlpha(theme::kShell, 0.8f), 380);
    if (dexComplete(s, breed)) textCentered(app, str::kDexDone, 200, 228, 0.42f, theme::kClutchGold, 380);
}

void drawDexBottom(App& app, const Input& in) {
    SaveData& s = app.game;
    char line[48];
    std::snprintf(line, sizeof(line), "%s  %d / %d", str::kDex, dexCount(s), kDexEntries);
    text(app, line, 160, 6, 0.55f, theme::kClutchGold);
    if (in.down & (KEY_L | KEY_DLEFT)) app.dexPage = static_cast<u8>((app.dexPage + kPages - 1) % kPages);
    if (in.down & (KEY_R | KEY_DRIGHT)) app.dexPage = static_cast<u8>((app.dexPage + 1) % kPages);
    for (int row = 0; row < kRowsPerPage; ++row) {
        const int breed = app.dexPage * kRowsPerPage + row;
        if (breed >= kBreedCount) break;
        const float y = kRowY + row * kRowH;
        u8 a, b;
        breedAlleles(breed, a, b);
        text(app, breedName(static_cast<Element>(a), static_cast<Element>(b)), 10, y + 3, 0.42f, theme::kShell,
             C2D_AlignLeft, 92);
        if (dexComplete(s, breed)) heart(106, y + 9, 9, theme::kClutchGold);
        Rgb base, accent, glow;
        breedColours(breed, base, accent, glow);
        for (int look = 0; look < kLookCount; ++look) {
            const Rect r{kCellX + look * (kCellW + 2), y, kCellW, kCellH};
            const bool have = dexHas(s, breed, look);
            const bool picked = breed == app.dexPick && look == app.dexLook;
            if (picked) panel({r.x - 2, r.y - 2, r.w + 4, r.h + 4}, theme::kClutchGold);
            panel(r, have ? fromRgb(look == kLookWild ? glow : base) : theme::rgba(40, 30, 50));
            text(app, have ? kLookLetter[look] : "?", r.x + r.w / 2, r.y + 2, 0.42f,
                 have ? theme::rgba(30, 20, 36) : withAlpha(theme::kShell, 0.4f));
            if (in.tapped && r.contains(in.tx, in.ty)) {
                app.dexPick = static_cast<u8>(breed);
                app.dexLook = static_cast<u8>(look);
                audio::playSfx(audio::Sfx::Tap);
            }
        }
    }
    // The rare traits met.
    text(app, str::kDexRares, 10, 184, 0.4f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft);
    for (int k = 0; k < 4; ++k) {
        const bool met = dexRare(s, kRareBit(k));
        text(app, met ? rareName(kRareBit(k)) : "?", 142 + k * 48, 184, 0.36f,
             met ? theme::kClutchGold : withAlpha(theme::kShell, 0.4f), C2D_AlignCenter, 46);
    }
    std::snprintf(line, sizeof(line), "%d / %d", app.dexPage + 1, kPages);
    text(app, line, 88, 212, 0.42f, withAlpha(theme::kShell, 0.7f), C2D_AlignLeft);
    if (button(app, {8, 204, 36, 30}, "<", in)) app.dexPage = static_cast<u8>((app.dexPage + kPages - 1) % kPages);
    if (button(app, {48, 204, 36, 30}, ">", in)) app.dexPage = static_cast<u8>((app.dexPage + 1) % kPages);
    if (dexComplete(s, app.dexPick)) {
        const bool up = bannerBreed(s) == app.dexPick;
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
