// The profile's trainer pages (1.0, D90; workstream U): Training (its level and the experience
// bar, its five stats as its kind's points and what training added, its four battle moves, swapped
// from what it knows with a tap) and Record (its titles, wins, the challenge cups won with it, its
// ribbons and the deepest floor of Frostspire Hollow). What it knows and wears comes through
// app/profile_hooks (workstreams B and P).
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/care_ui.hpp"
#include "app/profile_hooks.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/tips_ui.hpp"
#include "app/ui_draw.hpp"
#include "core/genetics.hpp"
#include "core/kinds.hpp"
#include "core/trainer.hpp"
#include "core/world.hpp"

namespace ec::care {
namespace {

constexpr int kStatMax = 10 + kMaxTrained;  // a kind's best plus all training can add
int g_pickSlot = -1;                        // a move slot being swapped (-1: none)
u32 g_pickFor = 0;                          // ...on this dragon

u32 moveColour(const hooks::MoveView& m) {
    return m.element >= 0 ? fromRgb(elementGlow(m.element)) : withAlpha(theme::kShell, 0.8f);
}

// A little cup: bowl, stem and foot; bright when won, a faint outline of one when not.
void trophy(float cx, float cy, float s, bool won) {
    const u32 c = won ? theme::kClutchGold : withAlpha(theme::kShell, 0.18f);
    C2D_DrawTriangle(cx - s * 0.5f, cy - s * 0.5f, c, cx + s * 0.5f, cy - s * 0.5f, c, cx, cy + s * 0.12f, c, 0.5f);
    C2D_DrawRectSolid(cx - s * 0.5f, cy - s * 0.55f, 0.5f, s, s * 0.18f, c);
    C2D_DrawRectSolid(cx - s * 0.07f, cy, 0.5f, s * 0.14f, s * 0.3f, c);
    C2D_DrawRectSolid(cx - s * 0.28f, cy + s * 0.28f, 0.5f, s * 0.56f, s * 0.14f, c);
    if (won) C2D_DrawCircleSolid(cx - s * 0.18f, cy - s * 0.32f, 0.5f, s * 0.08f, withAlpha(theme::kShell, 0.8f));
}

// A rosette: two tails and a round face, in a colour of its own.
void rosette(float cx, float cy, float r, u32 c) {
    C2D_DrawTriangle(cx - r * 0.7f, cy, c, cx - r * 0.1f, cy, c, cx - r * 0.6f, cy + r * 1.7f, c, 0.5f);
    C2D_DrawTriangle(cx + r * 0.1f, cy, c, cx + r * 0.7f, cy, c, cx + r * 0.6f, cy + r * 1.7f, c, 0.5f);
    C2D_DrawCircleSolid(cx, cy, 0.5f, r, c);
    C2D_DrawCircleSolid(cx, cy, 0.5f, r * 0.55f, withAlpha(theme::kShell, 0.85f));
}

void heading(App& app, const char* s, float x, float y) {
    text(app, s, x, y, 0.4f, theme::kClutchGold, C2D_AlignLeft);
}

// Swapping a move: what it knows, two columns; a tap takes it (or Cancel).
void movePicker(App& app, const Input& in, Dragon& d) {
    C2D_DrawRectSolid(0, 62, 0.5f, 320, 138, withAlpha(theme::kDenPlum, 0.97f));
    text(app, str::kPickMove, 12, 66, 0.46f, theme::kClutchGold, C2D_AlignLeft, 210);
    if (button(app, {236, 64, 76, 18}, str::kCancel, in)) {
        g_pickSlot = -1;
        return;
    }
    u8 known[12];
    const int n = hooks::knownMoves(d, known, 12);
    for (int k = 0; k < n; ++k) {
        hooks::MoveView m;
        if (!hooks::moveView(known[k], m)) continue;
        const Rect r{10.0f + (k % 2) * 152.0f, 84.0f + (k / 2) * 17.0f, 148, 15};
        panel(r, withAlpha(theme::kShell, 0.14f));
        C2D_DrawCircleSolid(r.x + 8, r.y + r.h / 2, 0.5f, 3.5f, moveColour(m));
        text(app, m.name, r.x + 16, r.y + 1, 0.34f, theme::kShell, C2D_AlignLeft, 90);
        char pow[16];
        if (m.status) std::snprintf(pow, sizeof(pow), "%s", str::kStatusMove);
        else std::snprintf(pow, sizeof(pow), str::kPower, m.power);
        text(app, pow, r.x + r.w - 4, r.y + 2, 0.3f, withAlpha(theme::kShell, 0.7f), C2D_AlignRight);
        if (in.released && r.contains(in.rx, in.ry)) {
            if (hooks::equipMove(d, g_pickSlot, known[k])) {
                audio::playSfx(audio::Sfx::Confirm);
                showToastf(app, str::kMoveSwapped, d.name);
                saveNow(app);
            }
            g_pickSlot = -1;
            return;
        }
    }
}

}  // namespace

void profileTraining(App& app, const Input& in, Dragon& d) {
    showTip(app, tips::kTipProfile);
    if (g_pickSlot >= 0 && g_pickFor != d.id) g_pickSlot = -1;  // (another dragon now)
    char line[64];
    // Its level and how far to the next.
    const int level = trainer::levelOf(d);
    std::snprintf(line, sizeof(line), str::kLevel, level);
    text(app, line, 12, 62, 0.72f, theme::kClutchGold, C2D_AlignLeft, 100, Face::Title);
    u32 into = 0, span = 0;
    trainer::levelProgress(d, into, span);
    const Rect bar{118, 70, 190, 8};
    panel(bar, theme::kTrack);
    if (span == 0) {
        panel(bar, theme::kClutchGold);
        std::snprintf(line, sizeof(line), "%s", str::kXpTop);
    } else {
        if (into > 0) panel({bar.x, bar.y, std::fmax(4.0f, bar.w * into / static_cast<float>(span)), bar.h}, theme::kClutchGold);
        std::snprintf(line, sizeof(line), str::kXpToNext, static_cast<unsigned long>(into), static_cast<unsigned long>(span));
    }
    text(app, line, bar.x + bar.w, 80, 0.34f, withAlpha(theme::kShell, 0.75f), C2D_AlignRight, bar.w);
    // Its stats: the kind's points (gold) and what training added (ember), out of what's possible.
    for (int k = 0; k < kDragonStats; ++k) {
        const float y = 98 + k * 15;
        text(app, str::kStatNames[k], 12, y, 0.38f, theme::kShell, C2D_AlignLeft, 48);
        const int base = trainer::statPoints(d, k) - (d.trained[k] > kMaxTrained ? kMaxTrained : d.trained[k]);
        const int total = trainer::statPoints(d, k);
        const Rect sb{62, y + 4, 70, 7};
        panel(sb, theme::kTrack);
        const float per = sb.w / kStatMax;
        C2D_DrawRectSolid(sb.x, sb.y, 0.5f, per * base, sb.h, theme::kClutchGold);
        if (total > base) C2D_DrawRectSolid(sb.x + per * base, sb.y, 0.5f, per * (total - base), sb.h, theme::kEmber);
        std::snprintf(line, sizeof(line), "%d", total);
        text(app, line, 152, y, 0.38f, theme::kShell, C2D_AlignRight);
    }
    // The key: kind and trained.
    C2D_DrawRectSolid(14, 177, 0.5f, 7, 7, theme::kClutchGold);
    text(app, kindTitle(d), 24, 173, 0.32f, withAlpha(theme::kShell, 0.7f), C2D_AlignLeft, 56);
    C2D_DrawRectSolid(86, 177, 0.5f, 7, 7, theme::kEmber);
    text(app, str::kTabTraining, 96, 173, 0.32f, withAlpha(theme::kShell, 0.7f), C2D_AlignLeft, 60);
    // Its four moves (workstream B's core/battle through the hooks); a tap swaps one.
    heading(app, str::kMoves, 166, 92);
    u8 moves[kMoveSlots];
    hooks::equippedMoves(d, moves);
    int filled = 0;
    for (int k = 0; k < kMoveSlots; ++k) {
        const Rect r{164, 108.0f + k * 22, 148, 19};
        hooks::MoveView m;
        const bool has = hooks::moveView(moves[k], m);
        filled += has;
        panel(r, withAlpha(g_pickSlot == k ? theme::kClutchGold : theme::kShell, g_pickSlot == k ? 0.4f : 0.13f));
        if (has) {
            C2D_DrawCircleSolid(r.x + 9, r.y + r.h / 2, 0.5f, 4.5f, moveColour(m));
            text(app, m.name, r.x + 18, r.y + 2, 0.38f, theme::kShell, C2D_AlignLeft, 86);
            if (m.status) std::snprintf(line, sizeof(line), "%s", str::kStatusMove);
            else std::snprintf(line, sizeof(line), str::kPower, m.power);
            text(app, line, r.x + r.w - 5, r.y + 4, 0.32f, withAlpha(theme::kShell, 0.7f), C2D_AlignRight);
        } else {
            textCentered(app, str::kNoMove, r.x + r.w / 2, r.y + r.h / 2, 0.4f, withAlpha(theme::kShell, 0.35f));
        }
        if (in.released && r.contains(in.rx, in.ry) && g_pickSlot < 0) {
            u8 known[1];
            if (hooks::knownMoves(d, known, 1) == 0) {
                showToast(app, str::kNoMovesYet);
            } else {
                g_pickSlot = k;
                g_pickFor = d.id;
                audio::playSfx(audio::Sfx::Tap);
            }
        }
    }
    if (filled == 0) text(app, str::kNoMovesYet, 238, 184, 0.3f, withAlpha(theme::kShell, 0.55f), C2D_AlignCenter, 148);
    if (g_pickSlot >= 0) movePicker(app, in, d);
}

void profileRecord(App& app, const Input& in, const Dragon& d) {
    (void)in;
    char line[80];
    // Its titles, from the leagues it has won (core/trainer).
    heading(app, str::kTitles, 14, 66);
    if (d.battleTitle || d.showTitle) {
        std::snprintf(line, sizeof(line), "%s%s%s", trainer::battleTitleName(d.battleTitle),
                      d.battleTitle && d.showTitle ? "  -  " : "", trainer::showTitleName(d.showTitle));
        text(app, line, 64, 66, 0.4f, theme::kShell, C2D_AlignLeft, 160);
    } else {
        text(app, str::kNoTitles, 64, 66, 0.4f, withAlpha(theme::kShell, 0.5f), C2D_AlignLeft, 160);
    }
    // Your friendly duels won with the roaming trainers (all your dragons', workstream D).
    std::snprintf(line, sizeof(line), str::kDuelsWon, static_cast<int>(app.game.progress.duelsWon));
    text(app, line, 308, 68, 0.32f, withAlpha(theme::kShell, app.game.progress.duelsWon ? 0.8f : 0.45f), C2D_AlignRight, 90);
    // Its wins, and the Hollow's deepest floor.
    const int values[4] = {d.battleWins, d.showWins, d.wildWins, d.frostDeepest};
    const char* const labels[4] = {str::kBattleWins, str::kShowWins, str::kWildWins, str::kHollowDeepest};
    for (int k = 0; k < 4; ++k) {
        const Rect r{12.0f + k * 75.0f, 84, 71, 36};
        panel(r, withAlpha(theme::kShell, 0.12f));
        std::snprintf(line, sizeof(line), "%d", values[k]);
        textCentered(app, line, r.x + r.w / 2, r.y + 12, 0.6f, values[k] ? theme::kClutchGold : withAlpha(theme::kShell, 0.4f));
        textCentered(app, labels[k], r.x + r.w / 2, r.y + 28, 0.32f, withAlpha(theme::kShell, 0.75f), r.w - 4);
    }
    // The challenge cups won with it: a row for each challenge, Ember to Starfire.
    std::snprintf(line, sizeof(line), "%s  %d", str::kCupsWon, trainer::cupCount(d));
    heading(app, line, 14, 126);
    for (int c = 0; c < kChallenges; ++c) {
        const float y = 142 + c * 15;
        text(app, str::kChallengeShort[c], 14, y, 0.33f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft, 72);
        for (int cup = 1; cup <= kCups; ++cup) trophy(98 + (cup - 1) * 16, y + 6, 10, trainer::wonCup(d, c, cup));
    }
    // The columns run from the Ember cup to the Starfire.
    text(app, str::kCupNames[0], 91, 186, 0.3f, withAlpha(theme::kShell, 0.5f), C2D_AlignLeft);
    text(app, str::kCupNames[kCups - 1], 153, 186, 0.3f, withAlpha(theme::kShell, 0.5f), C2D_AlignRight);
    // Its ribbons (the pageant's themes won).
    std::snprintf(line, sizeof(line), "%s  %d", str::kRibbons, trainer::ribbonCount(d));
    heading(app, line, 176, 126);
    int shown = 0;
    for (int t = 0; t < 16; ++t) {
        if (!((d.ribbons >> t) & 1u)) continue;
        const Rgb c = hsvToRgb(static_cast<u8>(t * 37), 150, 235);
        rosette(186 + (shown % 7) * 18, 148 + (shown / 7) * 22, 5.5f, fromRgb(c));
        ++shown;
    }
    if (shown == 0) text(app, str::kWearNothing, 178, 144, 0.36f, withAlpha(theme::kShell, 0.45f), C2D_AlignLeft, 130);
}

}  // namespace ec::care
