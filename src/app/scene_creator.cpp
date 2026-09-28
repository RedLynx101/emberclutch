// Your look (Beta WP12, D74): the creator after a new game's name (then the egg), and from the
// menu's settings any time. You stand on the top screen in the light (L and R turn you round);
// the bottom screen has a row per choice: clothes, hair, hair colour, skin, outfit, eyes (up and
// down pick one, left and right change it). Each change gets a little nod, Done a cheer.
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/people.hpp"

namespace ec {
namespace {

struct CreatorScene {
    Animator anim;
    SceneId back = SceneId::Den;
    int row = 0;  // kLookParts rows, then Done
    float spin = 0.35f;
    float blinkIn = 3.3f, blink = 0;
    float doneIn = -1;  // the cheer before leaving (< 0: not done)
};

CreatorScene& cs() {
    static CreatorScene s;
    return s;
}

const char* const kRows[kLookParts] = {str::kLookClothes, str::kLookHair, str::kLookHairColour,
                                       str::kLookSkin, str::kLookOutfit, str::kLookEyes};

void play(CreatorScene& s, const char* name, float fade = 0.2f) {
    const AnimLibrary* lib = r3d::personAnims();
    if (!lib) return;
    const int c = lib->find(name);
    if (c >= 0) s.anim.play(c, fade, true);
}

void change(App& app, int part, int by) {
    u8& v = app.game.world.look[part];
    v = static_cast<u8>((v + kLookChoices[part] + by) % kLookChoices[part]);
    play(cs(), "nod");
    audio::playSfx(audio::Sfx::Tap, 1.0f + 0.05f * v);
}

void finish(App& app) {
    CreatorScene& s = cs();
    if (s.doneIn >= 0) return;
    app.game.world.lookMade = 1;
    s.doneIn = 1.2f;
    play(s, "cheer");
    audio::playSfx(audio::Sfx::Confirm);
}

void update(App& app, const Input& in) {
    CreatorScene& s = cs();
    const AnimLibrary* lib = r3d::personAnims();
    if (lib && s.anim.clip < 0) play(s, "idle", 0.0f);
    if (s.doneIn >= 0) {
        s.doneIn -= app.dt;
        if (s.doneIn < 0) {
            app.scene = s.back;
            if (hasDragon(app)) saveNow(app);
        }
    } else {
        if (in.down & KEY_DOWN) s.row = (s.row + 1) % (kLookParts + 1);
        if (in.down & KEY_UP) s.row = (s.row + kLookParts) % (kLookParts + 1);
        if (s.row < kLookParts && (in.down & (KEY_LEFT | KEY_RIGHT))) change(app, s.row, in.down & KEY_LEFT ? -1 : 1);
        if ((in.down & KEY_A) && s.row == kLookParts) finish(app);
        if (in.down & (KEY_B | KEY_START)) finish(app);
    }
    // L and R turn you (run 19: the pad chooses); let go and you ease back to a three-quarter view.
    const float turn = (in.held & KEY_R ? 1.0f : 0.0f) - (in.held & KEY_L ? 1.0f : 0.0f);
    if (turn != 0) s.spin += turn * 2.4f * app.dt;
    else s.spin += (0.35f - std::remainder(s.spin, 6.2831853f)) * std::fmin(1.0f, app.dt * 1.5f);
    if (lib) {
        s.anim.update(*lib, app.dt, nullptr, 0);
        if (s.anim.finished(*lib)) play(s, "idle", 0.3f);
    }
    s.blinkIn -= app.dt;
    if (s.blinkIn <= 0) {
        s.blink = 1;
        s.blinkIn = 1.5f + app.rng.below(3000) * 0.001f;
    }
    s.blink = std::fmax(0.0f, s.blink - app.dt * 7.0f);
}

void drawTop(App& app) {
    CreatorScene& s = cs();
    verticalGradient(0, 0, kTopW, kScreenH, theme::rgba(150, 196, 236), theme::rgba(252, 232, 200));
    C2D_DrawEllipseSolid(200 - 70, 196, 0, 140, 26, withAlpha(theme::rgba(120, 90, 70), 0.25f));
    r3d::PersonView p;
    p.form = static_cast<u8>(playerBody(app.game.world.look));
    p.heading = s.spin;
    p.anim = &s.anim;
    playerPalette(app.game.world.look, p.pal);
    p.hair = static_cast<s8>(app.game.world.look[kLookHair] % kHairStyles);
    p.blink = s.blink;
    r3d::drawPersonShowcase(app, p, nowLocal(app));
    textCentered(app, str::kYourLook, 200, 22, 0.9f, theme::kDenPlum, 380, Face::Title);
    textCentered(app, app.game.playerName, 200, 222, 0.55f, withAlpha(theme::kDenPlum, 0.8f), 380);
}

void drawBottom(App& app, const Input& in) {
    CreatorScene& s = cs();
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    const u8* look = app.game.world.look;
    Rgb pal[kPalCount];
    for (int r = 0; r < kLookParts; ++r) {
        const float y = 10 + r * 29;
        const bool here = s.row == r;
        panel({16, y, 288, 25}, here ? withAlpha(theme::kClutchGold, 0.35f) : withAlpha(theme::kShell, 0.08f));
        text(app, kRows[r], 26, y + 5, 0.45f, here ? theme::kClutchGold : theme::kShell, C2D_AlignLeft, 100);
        if (button(app, {128, y + 1, 30, 23}, "<", in)) {
            s.row = r;
            change(app, r, -1);
        }
        if (button(app, {268, y + 1, 30, 23}, ">", in)) {
            s.row = r;
            change(app, r, 1);
        }
        textCentered(app, lookChoiceName(r, look[r]), 213, y + 12, 0.45f, theme::kShell, 104);
        // A swatch for the colours.
        if (r == kLookHairColour || r == kLookSkin || r == kLookOutfit || r == kLookEyes) {
            playerPalette(look, pal);
            const Rgb c = r == kLookHairColour ? pal[kPalHorn] : r == kLookSkin ? pal[kPalBase]
                        : r == kLookOutfit ? pal[kPalAccent] : pal[kPalIris];
            C2D_DrawCircleSolid(170, y + 12, 0.5f, 6.5f, theme::kDenPlum);
            C2D_DrawCircleSolid(170, y + 12, 0.5f, 5.0f, theme::rgba(c.r, c.g, c.b));
        }
    }
    // The controls at the bottom left, big enough to read; Done at the bottom right (run 19).
    text(app, str::kLookHelp, 18, 186, 0.42f, withAlpha(theme::kShell, 0.85f), C2D_AlignLeft, 170);
    if (button(app, {196, 190, 108, 40}, str::kDone, in, s.row == kLookParts ? theme::kClutchGold : 0)) finish(app);
}

}  // namespace

void openCreator(App& app, SceneId back) {
    CreatorScene& s = cs();
    s = CreatorScene{};
    s.back = back;
    app.scene = SceneId::Creator;
}

const SceneFns kCreatorScene{update, drawTop, drawBottom};

}  // namespace ec
