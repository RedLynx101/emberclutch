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
#include "core/league.hpp"
#include "core/story.hpp"
#include "core/trainer.hpp"
#include "core/world.hpp"

namespace ec::care {
namespace {

constexpr int kStatMax = 10 + kMaxTrained;  // a kind's best plus all training can add
int g_pickSlot = -1;                        // a move slot being swapped (-1: none)
u32 g_pickFor = 0;                          // ...on this dragon
bool g_pickFresh = false;                   // ...opened by this frame's tap (which isn't the picker's to take)

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

// A badge (D138): a medallion on two ribbon tails with its league's mark (a spark, a flame, two
// flames, a star; the Champion's a crown); a faint ring until it's won.
void flame(float cx, float cy, float h, u32 c) {
    C2D_DrawCircleSolid(cx, cy + h * 0.18f, 0.5f, h * 0.3f, c);
    C2D_DrawTriangle(cx - h * 0.29f, cy + h * 0.12f, c, cx + h * 0.29f, cy + h * 0.12f, c, cx, cy - h * 0.5f, c, 0.5f);
}

void badge(float cx, float cy, float r, int which, bool won) {
    if (!won) {
        C2D_DrawCircleSolid(cx, cy, 0.5f, r, withAlpha(theme::kShell, 0.16f));
        C2D_DrawCircleSolid(cx, cy, 0.5f, r * 0.72f, theme::kDenPlum);
        return;
    }
    const u32 c = theme::kBadge[which], face = withAlpha(theme::kShell, 0.92f);
    C2D_DrawTriangle(cx - r * 0.75f, cy, c, cx - r * 0.05f, cy, c, cx - r * 0.65f, cy + r * 1.6f, c, 0.5f);
    C2D_DrawTriangle(cx + r * 0.05f, cy, c, cx + r * 0.75f, cy, c, cx + r * 0.65f, cy + r * 1.6f, c, 0.5f);
    C2D_DrawCircleSolid(cx, cy, 0.5f, r, c);
    C2D_DrawCircleSolid(cx, cy, 0.5f, r * 0.74f, face);
    const float h = r * 1.15f;
    switch (which) {
        case 0: flame(cx, cy + r * 0.1f, h * 0.7f, c); break;
        case 1: flame(cx, cy, h, c); break;
        case 2:
            flame(cx - r * 0.24f, cy + r * 0.08f, h * 0.8f, c);
            flame(cx + r * 0.24f, cy + r * 0.08f, h * 0.8f, c);
            break;
        case 3: {  // a five-pointed star
            const float ro = r * 0.6f, ri = r * 0.25f;
            for (int i = 0; i < 5; ++i) {
                const float a0 = -1.5708f + i * 1.2566f, a1 = a0 + 0.6283f, a2 = a0 + 1.2566f;
                const float ox = cx + ro * std::cos(a0), oy = cy + ro * std::sin(a0);
                const float ix = cx + ri * std::cos(a1), iy = cy + ri * std::sin(a1);
                const float nx = cx + ro * std::cos(a2), ny = cy + ro * std::sin(a2);
                C2D_DrawTriangle(cx, cy, c, ox, oy, c, ix, iy, c, 0.5f);
                C2D_DrawTriangle(cx, cy, c, ix, iy, c, nx, ny, c, 0.5f);
            }
            break;
        }
        default: {  // the Champion's crown
            const float w = r * 1.1f, b = cy + r * 0.32f;
            C2D_DrawRectSolid(cx - w * 0.5f, b - r * 0.28f, 0.5f, w, r * 0.28f, c);
            for (int i = 0; i < 3; ++i) {
                const float x = cx - w * 0.5f + w * 0.5f * i;
                C2D_DrawTriangle(x - r * 0.2f, b - r * 0.26f, c, x + r * 0.2f, b - r * 0.26f, c, x, b - r * 0.78f, c, 0.5f);
            }
            break;
        }
    }
}

void heading(App& app, const char* s, float x, float y) {
    text(app, s, x, y, 0.4f, theme::kClutchGold, C2D_AlignLeft);
}

// Swapping a move: what it knows, two columns; a tap takes it (or Cancel).
void movePicker(App& app, const Input& tap, Dragon& d) {
    // The tap that opened it is spent (1.0 passed it on: the slot's own tap took whichever move lay under the
    // stylus, and the picker was gone before it was ever seen).
    const Input none{};
    const Input& in = g_pickFresh ? none : tap;
    g_pickFresh = false;
    C2D_DrawRectSolid(0, 62, 0.5f, 320, 138, withAlpha(theme::kDenPlum, 0.97f));
    text(app, str::kPickMove, 12, 65, 0.5f, theme::kClutchGold, C2D_AlignLeft, 214);
    if (button(app, {236, 64, 76, 20}, str::kCancel, in)) {
        g_pickSlot = -1;
        return;
    }
    u8 known[12];
    const int n = hooks::knownMoves(d, known, 12);
    for (int k = 0; k < n; ++k) {  // (six rows of two at most, down to the buttons)
        hooks::MoveView m;
        if (!hooks::moveView(known[k], m)) continue;
        const Rect r{10.0f + (k % 2) * 152.0f, 88.0f + (k / 2) * 18.5f, 148, 17};
        panel(r, withAlpha(theme::kShell, 0.14f));
        C2D_DrawCircleSolid(r.x + 8, r.y + r.h / 2, 0.5f, 3.8f, moveColour(m));
        char pow[16];
        if (m.status) std::snprintf(pow, sizeof(pow), "%s", str::kStatusMove);
        else std::snprintf(pow, sizeof(pow), str::kPower, m.power);
        const float powW = textWidth(app, pow, 0.36f);
        text(app, m.name, r.x + 16, r.y + 1, 0.4f, theme::kShell, C2D_AlignLeft, r.w - 16 - powW - 10);
        text(app, pow, r.x + r.w - 4, r.y + 2, 0.36f, withAlpha(theme::kShell, 0.75f), C2D_AlignRight);
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
    // (After 1.0.1, Noah: "the text is a tad small". The page's words are a size up, 0.32-0.38 to 0.38-0.46, and
    // its rows respaced to hold them: the stats 16 px apart, the moves' rows a pixel taller, the key under them.)
    // Its level and how far to the next.
    const int level = trainer::levelOf(d);
    std::snprintf(line, sizeof(line), str::kLevel, level);
    text(app, line, 12, 62, 0.72f, theme::kClutchGold, C2D_AlignLeft, 100, Face::Title);
    u32 into = 0, span = 0;
    trainer::levelProgress(d, into, span);
    const Rect bar{118, 68, 190, 9};
    panel(bar, theme::kTrack);
    if (span == 0) {
        panel(bar, theme::kClutchGold);
        std::snprintf(line, sizeof(line), "%s", str::kXpTop);
    } else {
        if (into > 0) panel({bar.x, bar.y, std::fmax(4.0f, bar.w * into / static_cast<float>(span)), bar.h}, theme::kClutchGold);
        std::snprintf(line, sizeof(line), str::kXpToNext, static_cast<unsigned long>(into), static_cast<unsigned long>(span));
    }
    text(app, line, bar.x + bar.w, 78, 0.42f, withAlpha(theme::kShell, 0.8f), C2D_AlignRight, 100);  // (clear of "Moves" under it)
    // Its stats: the kind's points (gold) and what training added (ember), out of what's possible.
    for (int k = 0; k < kDragonStats; ++k) {
        const float y = 97 + k * 16;
        text(app, str::kStatNames[k], 12, y, 0.46f, theme::kShell, C2D_AlignLeft, 52);
        const int base = trainer::statPoints(d, k) - (d.trained[k] > kMaxTrained ? kMaxTrained : d.trained[k]);
        const int total = trainer::statPoints(d, k);
        const Rect sb{68, y + 5, 60, 8};
        panel(sb, theme::kTrack);
        const float per = sb.w / kStatMax;
        C2D_DrawRectSolid(sb.x, sb.y, 0.5f, per * base, sb.h, theme::kClutchGold);
        if (total > base) C2D_DrawRectSolid(sb.x + per * base, sb.y, 0.5f, per * (total - base), sb.h, theme::kEmber);
        std::snprintf(line, sizeof(line), "%d", total);
        text(app, line, 154, y, 0.46f, theme::kShell, C2D_AlignRight);
    }
    // The key: kind and trained.
    C2D_DrawRectSolid(14, 183, 0.5f, 8, 8, theme::kClutchGold);
    text(app, kindTitle(d), 25, 179, 0.38f, withAlpha(theme::kShell, 0.75f), C2D_AlignLeft, 58);
    C2D_DrawRectSolid(88, 183, 0.5f, 8, 8, theme::kEmber);
    text(app, str::kTabTraining, 99, 179, 0.38f, withAlpha(theme::kShell, 0.75f), C2D_AlignLeft, 58);
    // Its four moves (workstream B's core/battle through the hooks); a tap swaps one.
    text(app, str::kMoves, 166, 93, 0.44f, theme::kClutchGold, C2D_AlignLeft);
    u8 moves[kMoveSlots];
    hooks::equippedMoves(d, moves);
    int filled = 0;
    for (int k = 0; k < kMoveSlots; ++k) {
        hooks::MoveView m;
        filled += hooks::moveView(moves[k], m);
    }
    for (int k = 0; k < kMoveSlots; ++k) {
        const Rect r{164, 110.0f + k * 22, 148, 20};
        hooks::MoveView m;
        const bool has = hooks::moveView(moves[k], m);
        if (filled == 0) {  // none yet: one panel saying so, in its rows' place (the words lay over the fourth "-")
            if (k == 0) {
                const Rect all{164, 110, 148, 86};
                panel(all, withAlpha(theme::kShell, 0.13f));
                textCentered(app, str::kNoMovesYetA, all.x + all.w / 2, all.y + all.h / 2 - 9, 0.42f, withAlpha(theme::kShell, 0.7f), 140);
                textCentered(app, str::kNoMovesYetB, all.x + all.w / 2, all.y + all.h / 2 + 9, 0.42f, withAlpha(theme::kShell, 0.7f), 140);
            }
        } else {
            panel(r, withAlpha(g_pickSlot == k ? theme::kClutchGold : theme::kShell, g_pickSlot == k ? 0.4f : 0.13f));
            if (has) {
                C2D_DrawCircleSolid(r.x + 9, r.y + r.h / 2, 0.5f, 4.5f, moveColour(m));
                if (m.status) std::snprintf(line, sizeof(line), "%s", str::kStatusMove);
                else std::snprintf(line, sizeof(line), str::kPower, m.power);
                const float powW = textWidth(app, line, 0.38f);
                text(app, m.name, r.x + 18, r.y + 2, 0.44f, theme::kShell, C2D_AlignLeft, r.w - 18 - powW - 11);
                text(app, line, r.x + r.w - 5, r.y + 3, 0.38f, withAlpha(theme::kShell, 0.75f), C2D_AlignRight);
            } else {
                textCentered(app, str::kNoMove, r.x + r.w / 2, r.y + r.h / 2, 0.44f, withAlpha(theme::kShell, 0.35f));
            }
        }
        if (in.released && r.contains(in.rx, in.ry) && g_pickSlot < 0) {
            u8 known[1];
            if (hooks::knownMoves(d, known, 1) == 0) {
                showToast(app, str::kNoMovesYet);
            } else {
                g_pickSlot = k;
                g_pickFor = d.id;
                g_pickFresh = true;
                audio::playSfx(audio::Sfx::Tap);
            }
        }
    }
    if (g_pickSlot >= 0) movePicker(app, in, d);
}

void profileRecord(App& app, const Input& in, const Dragon& d) {
    char line[80];
    // Its titles, from the leagues it has won (core/trainer).
    heading(app, str::kTitles, 14, 66);
    if (d.battleTitle || d.showTitle) {
        std::snprintf(line, sizeof(line), "%s%s%s", trainer::battleTitleName(d.battleTitle),
                      d.battleTitle && d.showTitle ? "  -  " : "", trainer::showTitleName(d.showTitle));
        text(app, line, 64, 66, 0.4f, theme::kShell, C2D_AlignLeft, 140);
    } else {
        text(app, str::kNoTitles, 64, 66, 0.4f, withAlpha(theme::kShell, 0.5f), C2D_AlignLeft, 140);
    }
    // Your badge case (all your dragons', D138): a league's badge once it's won, the Champion's from
    // Wren once Solenne is beaten; a tap names one.
    for (int b = 0; b < 5; ++b) {
        const bool won = b < kLeagues ? league::leagueWon(app.game, b) : story::questDone(app.game, story::kQLeagueStarfire);
        const float cx = 226.0f + b * 20.0f, cy = 71.0f;
        badge(cx, cy, 7.0f, b, won);
        if (in.released && Rect{cx - 10, cy - 9, 20, 22}.contains(in.rx, in.ry)) {
            if (won) showToastf(app, "%s", b == kLeagues ? str::kBadgeChampion : str::kBadgeNames[b]);
            else showToastf(app, str::kBadgeNotYet, str::kBadgeNames[b]);
            audio::playSfx(audio::Sfx::Tap);
        }
    }
    // Its wins, the Hollow's deepest floor, and your friendly duels with the roaming trainers (all
    // your dragons', workstream D).
    const int values[5] = {d.battleWins, d.showWins, d.wildWins, d.frostDeepest, app.game.progress.duelsWon};
    const char* const labels[5] = {str::kBattleWins, str::kShowWins, str::kWildWins, str::kHollowDeepest, str::kYourDuels};
    for (int k = 0; k < 5; ++k) {
        const Rect r{12.0f + k * 60.0f, 86, 56, 34};
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
