#include "app/photo.hpp"

#include <cmath>
#include <cstdio>
#include <ctime>

#include "app/audio.hpp"
#include "app/screenshot.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/genetics.hpp"

namespace ec::photo {
namespace {

constexpr float kFlashTime = 0.35f;

void cameraIcon(float cx, float cy, float s, u32 body, u32 lens) {
    C2D_DrawRectSolid(cx - 0.42f * s, cy - 0.78f * s, 0, 0.5f * s, 0.3f * s, body);  // the viewfinder
    C2D_DrawRectSolid(cx - s, cy - 0.55f * s, 0, 2 * s, 1.25f * s, body);
    C2D_DrawCircleSolid(cx, cy + 0.07f * s, 0, 0.47f * s, lens);
    C2D_DrawCircleSolid(cx, cy + 0.07f * s, 0, 0.28f * s, body);
    C2D_DrawCircleSolid(cx + 0.72f * s, cy - 0.36f * s, 0, 0.1f * s, lens);  // the flash
}

void snap(App& app) {
    if (app.photo.snap || !screenshot::requestPhoto()) return;  // the last one is still being written
    app.photo.snap = true;  // this frame is drawn framed, and taken
}

// The frame: a gold border with a little diamond at each corner, the wordmark small at the top
// right, and a plate at the bottom with the name and the date.
void drawFrame(App& app, const Dragon& d, s64 now) {
    const u32 gold = theme::kClutchGold, dark = withAlpha(theme::rgba(30, 18, 36), 0.85f);
    constexpr float kIn = 5, kT = 4;
    C2D_DrawRectSolid(0, 0, 0, kTopW, kIn, dark);  // a dark edge outside the gold
    C2D_DrawRectSolid(0, kScreenH - kIn, 0, kTopW, kIn, dark);
    C2D_DrawRectSolid(0, kIn, 0, kIn, kScreenH - 2 * kIn, dark);
    C2D_DrawRectSolid(kTopW - kIn, kIn, 0, kIn, kScreenH - 2 * kIn, dark);
    C2D_DrawRectSolid(kIn, kIn, 0, kTopW - 2 * kIn, kT, gold);
    C2D_DrawRectSolid(kIn, kScreenH - kIn - kT, 0, kTopW - 2 * kIn, kT, gold);
    C2D_DrawRectSolid(kIn, kIn, 0, kT, kScreenH - 2 * kIn, gold);
    C2D_DrawRectSolid(kTopW - kIn - kT, kIn, 0, kT, kScreenH - 2 * kIn, gold);
    const float cx[2] = {kIn + kT / 2, kTopW - kIn - kT / 2}, cy[2] = {kIn + kT / 2, kScreenH - kIn - kT / 2};
    for (float x : cx)
        for (float y : cy) {
            constexpr float r = 7;
            C2D_DrawTriangle(x - r, y, gold, x, y - r, gold, x + r, y, gold, 0);
            C2D_DrawTriangle(x - r, y, gold, x + r, y, gold, x, y + r, gold, 0);
            C2D_DrawCircleSolid(x, y, 0, 2.2f, theme::kEmber);
        }
    text(app, str::kGameTitle, kTopW - 16, 12, 0.5f, withAlpha(gold, 0.9f), C2D_AlignRight, 0, Face::Title);
    char name[40], date[48];
    if (d.stage == Stage::Egg)
        std::snprintf(name, sizeof(name), "%s %s", breedName(d.genome), str::kEggSuffix);
    else
        std::snprintf(name, sizeof(name), "%s", d.name);
    const std::time_t t = static_cast<std::time_t>(now);  // the 3DS clock is local time already
    std::strftime(date, sizeof(date), "%d %B %Y", std::gmtime(&t));
    const float w = std::fmax(textWidth(app, name, 0.62f, Face::Title), textWidth(app, date, 0.42f)) + 36;
    panel({200 - w / 2 - 2, 190, w + 4, 38}, gold);
    panel({200 - w / 2, 192, w, 34}, theme::rgba(52, 35, 63));
    textCentered(app, name, 200, 203, 0.62f, gold, w - 12, Face::Title);
    textCentered(app, date, 200, 219, 0.42f, theme::kShell, w - 12);
}

}  // namespace

void open(App& app) {
    app.photo = PhotoState{};
    app.photo.active = true;
    app.care.holdingFood = false;
    audio::playSfx(audio::Sfx::Tap);
}

void update(App& app, const Input& in) {
    PhotoState& p = app.photo;
    if (p.snap) {  // last frame was the picture: the shutter
        p.snap = false;
        p.flash = 1;
        audio::playSfx(audio::Sfx::Tap, 1.4f, 0.8f);
    }
    if (p.flash > 0) p.flash = std::fmax(0.0f, p.flash - app.dt / kFlashTime);
    if ((in.down & KEY_A) || p.tapped) snap(app);
    p.tapped = false;
    if (in.down & KEY_X) p.close = !p.close;
    if (in.down & (KEY_DLEFT | KEY_DRIGHT)) cycleCare(app, (in.down & KEY_DRIGHT) ? 1 : -1);  // whose name
    if (in.down & KEY_B) {
        p.active = false;
        audio::playSfx(audio::Sfx::Back);
    }
}

void drawTop(App& app, const Dragon& d, s64 now) {
    if (app.photo.snap) drawFrame(app, d, now);
    if (app.photo.flash > 0)
        C2D_DrawRectSolid(0, 0, 0, kTopW, kScreenH, withAlpha(theme::rgba(255, 255, 255), 0.8f * app.photo.flash));
}

void drawBottom(App& app, const Input& in) {
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    textCentered(app, str::kPhotoMode, 160, 20, 0.8f, theme::kClutchGold, 300, Face::Title);
    textCentered(app, str::kPhotoHint, 160, 44, 0.45f, withAlpha(theme::kShell, 0.8f), 300);
    const Rect big{100, 64, 120, 92};
    panel({big.x - 3, big.y - 3, big.w + 6, big.h + 6}, withAlpha(theme::kClutchGold, 0.6f));
    const bool down = in.touching && big.contains(in.tx, in.ty);
    panel(big, down ? theme::kClutchGold : theme::kShell);
    cameraIcon(160, 104, 30, theme::rgba(52, 35, 63), down ? theme::kShell : theme::kClutchGold);
    text(app, str::kSnap, 160, 136, 0.5f, theme::rgba(52, 35, 63));
    if (in.released && big.contains(in.rx, in.ry)) app.photo.tapped = true;
    const Dragon& d = activeDragon(app);
    char who[48];
    if (d.stage == Stage::Egg)
        std::snprintf(who, sizeof(who), "%s %s", breedName(d.genome), str::kEggSuffix);
    else
        std::snprintf(who, sizeof(who), "%s", d.name);
    textCentered(app, who, 160, 176, 0.55f, theme::kShell, 200);
    if (button(app, {40, 162, 40, 28}, "<", in)) cycleCare(app, -1);
    if (button(app, {240, 162, 40, 28}, ">", in)) cycleCare(app, 1);
    if (button(app, {16, 200, 136, 32}, app.photo.close ? str::kPhotoWide : str::kPhotoClose, in))
        app.photo.close = !app.photo.close;
    if (button(app, {168, 200, 136, 32}, str::kBack, in)) {
        app.photo.active = false;
        audio::playSfx(audio::Sfx::Back);
    }
}

bool cameraButton(App& app, const Input& in) {
    const Rect r{kButtonX, kButtonY, kButtonW, kButtonH};
    const bool down = in.touching && r.contains(in.tx, in.ty);
    panel(r, withAlpha(theme::kDenPlum, down ? 0.9f : 0.55f));
    cameraIcon(r.x + r.w / 2, r.y + r.h / 2 + 1, 9, withAlpha(theme::kShell, 0.9f), theme::kClutchGold);
    return in.released && r.contains(in.rx, in.ry);
}

}  // namespace ec::photo
