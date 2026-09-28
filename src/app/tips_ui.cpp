#include "app/tips_ui.hpp"

#include <cmath>
#include <cstring>

#include "app/audio.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/trainer.hpp"

namespace ec {
namespace {

constexpr int kQueue = 4;
constexpr float kSlide = 0.3f, kFade = 0.35f;
constexpr Rect kCard{8, 40, 252, 54};

struct TipCard {
    int showing = -1;  // the tip on the card now (-1: none)
    float t = 0;       // seconds it's been up
    float life = 0;    // seconds it stays, by how much there is to read
    int queue[kQueue] = {};
    int waiting = 0;
};
TipCard g_card;

void next() {
    g_card.showing = -1;
    if (g_card.waiting == 0) return;
    g_card.showing = g_card.queue[0];
    for (int i = 1; i < g_card.waiting; ++i) g_card.queue[i - 1] = g_card.queue[i];
    --g_card.waiting;
    g_card.t = 0;
    const tips::TipInfo& info = tips::info(g_card.showing);
    g_card.life = 3.5f + 0.05f * static_cast<float>(std::strlen(info.text));  // about 7 s for a full card
}

float smooth(float x) {
    x = x < 0 ? 0 : (x > 1 ? 1 : x);
    return x * x * (3 - 2 * x);
}

}  // namespace

void showTip(App& app, tips::Tip tip) {
    if (!tips::due(app.game, tip) || !hasDragon(app)) return;
    trainer::markTip(app.game, tip);  // once: the next save keeps it
    if (g_card.waiting < kQueue) g_card.queue[g_card.waiting++] = tip;
    if (g_card.showing < 0) {
        next();
        audio::playSfx(audio::Sfx::Tap, 1.4f, 0.4f);  // a soft page-turn of a tick
    }
}

void resetTips(App& app) {
    tips::reset(app.game);
    g_card = TipCard{};
}

bool g_hold = false;

void holdTips(bool hold) { g_hold = hold; }

void drawTipCard(App& app) {
    if (g_card.showing < 0 || g_hold) return;
    if (app.menu == MenuPage::Closed) g_card.t += app.dt;  // (the right eye's pass has dt 0)
    if (g_card.t >= g_card.life) {
        next();
        if (g_card.showing < 0) return;
    }
    const float in = smooth(g_card.t / kSlide);
    const float out = smooth((g_card.life - g_card.t) / kFade);
    const float a = std::fmin(in, out);
    const float x = kCard.x - (1.0f - in) * (kCard.w + 12);
    const Rect r{x, kCard.y, kCard.w, kCard.h};
    panel({r.x, r.y + 2, r.w, r.h}, withAlpha(theme::kDenPlum, 0.35f * a));  // a soft shadow
    panel(r, withAlpha(theme::kShell, 0.95f * a));
    panel({r.x + 3, r.y + 4, 3, r.h - 8}, withAlpha(theme::kEmber, 0.9f * a));  // an ember at its edge
    // A little lantern's light by the title.
    glow(r.x + 18, r.y + 12, 9, theme::kClutchGold, 0.8f * a);
    C2D_DrawCircleSolid(r.x + 18, r.y + 12, 0, 4.2f, withAlpha(theme::kEmber, a));
    C2D_DrawCircleSolid(r.x + 17, r.y + 11, 0, 1.6f, withAlpha(theme::kShell, a));
    const tips::TipInfo& info = tips::info(g_card.showing);
    text(app, info.title, r.x + 30, r.y + 3, 0.5f, withAlpha(theme::kDenPlum, a), C2D_AlignLeft, r.w - 36, Face::Title);
    // Its lines, one under the other.
    char line[64];
    const char* s = info.text;
    for (int k = 0; k < 2 && *s; ++k) {
        std::size_t n = std::strcspn(s, "\n");
        if (n >= sizeof(line)) n = sizeof(line) - 1;
        std::memcpy(line, s, n);
        line[n] = 0;
        text(app, line, r.x + 12, r.y + 22 + k * 14, 0.42f, withAlpha(theme::kDusk, a), C2D_AlignLeft, r.w - 18);
        s += n;
        if (*s == '\n') ++s;
    }
}

}  // namespace ec
