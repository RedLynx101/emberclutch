// The Market (Alpha 2 WP5): stalls under bunting. Buy food for Gleam into the pouch, sell the
// hoard's trinkets, and the egg of the day, labelled (D24): one a day (core/market).
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/care_ui.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/storybook.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/tips_ui.hpp"
#include "app/ui_draw.hpp"
#include "core/clock.hpp"
#include "core/egg.hpp"
#include "core/genetics.hpp"
#include "core/items.hpp"
#include "core/market.hpp"
#include "core/kinds.hpp"

namespace ec {
namespace {


enum Tab : u8 { kFood, kGoods, kSell, kEgg, kTabs };

// Today's egg as a dragon record (for drawing it; nothing is saved).
Dragon todaysEgg(const App& app) { return eggOnShow(app.game, dayIndex(nowLocal(app))); }

void update(App& app, const Input& in) {
    audio::setBed(audio::Bed::Market, 1.0f);  // the stalls' murmur
    app.simAccum += app.dt;
    if (app.simAccum >= 1.0f) {
        tickWorld(app);
        app.simAccum = 0;
    }
    if (in.down & KEY_L) app.marketTab = static_cast<u8>((app.marketTab + kTabs - 1) % kTabs);
    if (in.down & KEY_R) app.marketTab = static_cast<u8>((app.marketTab + 1) % kTabs);
    if (in.down & KEY_B) {
        openMap(app);
        audio::playSfx(audio::Sfx::Back);
    }
}

// ------------------------------------------------------------------ the top screen (1.0, D89)
// The market square as a storybook picture (workstream U): the sky and its light by the time of
// day, cottages and trees behind, bunting overhead, and four stalls in a row, one for each page
// below (food, the day's goods, Maple's trade in trinkets, the egg of the day), each showing
// what it has; the page you're on lights its stall. A parchment card says what's in front of you.
constexpr float kStallW = 88, kStallY = 146;  // a stall's width; its counter's top
float stallX(int tab) { return 12.0f + tab * 96.0f; }

void stall(App& app, const paint::Light& l, int tab, bool on) {
    namespace pal = theme::paint;
    const float x = stallX(tab), w = kStallW, cx = x + w / 2;
    if (on) glow(cx, 128, 56, theme::kClutchGold, 0.45f + 0.1f * std::sin(app.t * 2.0f));
    // Posts, then the counter: a front board with the stall's name, a lighter plank on top.
    C2D_DrawRectSolid(x + 3, 104, 0, 5, 86, paint::lit(l, pal::kWoodDark));
    C2D_DrawRectSolid(x + w - 8, 104, 0, 5, 86, paint::lit(l, pal::kWoodDark));
    C2D_DrawRectSolid(x, kStallY + 4, 0, w, 40, paint::lit(l, pal::kWood));
    C2D_DrawRectSolid(x - 3, kStallY, 0, w + 6, 6, paint::lit(l, pal::kWoodLight));
    for (int k = 1; k < 3; ++k)  // the boards of its front
        C2D_DrawRectSolid(x, kStallY + 4 + k * 13, 0, w, 1.2f, paint::lit(l, pal::kWoodDark, 0.5f));
    static const char* const kNames[4] = {str::kStallFood, str::kStallGoods, str::kStallTrade, str::kStallEgg};
    const Rect plaque{x + 8, kStallY + 13, w - 16, 18};
    panel(plaque, on ? theme::kClutchGold : paint::lit(l, pal::kAwningCream));
    textCentered(app, kNames[tab], cx, plaque.y + plaque.h / 2, 0.42f, theme::kDenPlum, plaque.w - 6);
    // The awning: stripes in the stall's colour and cream, scalloped at its edge.
    const Rgb stripe = pal::kAwning[tab];
    constexpr int kStripes = 6;
    const float tw = (w - 8) / kStripes, bw = (w + 10) / kStripes;  // a stripe's width at the top and the edge
    for (int s = 0; s < kStripes; ++s) {
        const u32 c = paint::lit(l, s % 2 ? pal::kAwningCream : stripe);
        const float tl = x + 4 + s * tw, tr = tl + tw, bl = x - 5 + s * bw, br = bl + bw;
        C2D_DrawTriangle(tl, 84, c, tr, 84, c, br, 104, c, 0);
        C2D_DrawTriangle(tl, 84, c, br, 104, c, bl, 104, c, 0);
        C2D_DrawCircleSolid(bl + bw * 0.5f, 104, 0, bw * 0.5f, c);
    }
    C2D_DrawRectSolid(x + 4, 82, 0, w - 8, 3, paint::lit(l, pal::kWoodDark));
    // A lantern at its corner: bright on the stall you're at.
    if (on || l.night + l.evening > 0.3f) paint::lantern(l, x + w - 6, 118, on ? 7.0f : 5.5f, app.t + tab);
}

// What each stall has out on its counter.
void stallWares(App& app, const paint::Light& l, int tab, const Item* today, s32 day) {
    namespace pal = theme::paint;
    const float x = stallX(tab), cx = x + kStallW / 2, top = kStallY;
    switch (tab) {
        case kFood:  // two baskets of the day's foods
            for (int k = 0; k < 2; ++k) {
                const float bx = cx - 21 + k * 42;
                care::drawFood(static_cast<Food>((day + k * 3) % static_cast<int>(Food::Count)), bx - 5, top - 16, 0.34f);
                care::drawFood(static_cast<Food>((day + k * 3 + 1) % static_cast<int>(Food::Count)), bx + 6, top - 13, 0.34f);
                C2D_DrawRectSolid(bx - 17, top - 10, 0, 34, 10, paint::lit(l, pal::kStraw));  // the basket's front
                C2D_DrawRectSolid(bx - 17, top - 7, 0, 34, 1.2f, paint::lit(l, pal::kWood, 0.6f));
            }
            break;
        case kGoods:  // the day's four things (a spot sold today stays bare); the one picked held up
            for (int k = 0; k < kStallSpots; ++k) {
                if (today[k] == Item::Count) continue;
                const bool picked = app.marketTab == kGoods && app.goodsPick == k;
                if (picked) continue;
                care::drawItem(today[k], x + 12 + k * 21, top - 9, 0.3f);
            }
            if (app.marketTab == kGoods && app.goodsPick < kStallSpots && today[app.goodsPick] != Item::Count) {
                const float bob = 3 * std::sin(app.t * 1.6f);
                C2D_DrawEllipseSolid(cx - 16, top - 5, 0, 32, 6, withAlpha(theme::kDenPlum, 0.25f));
                care::drawItem(today[app.goodsPick], cx, top - 30 + bob, 0.7f);
            }
            break;
        case kSell: {  // Maple's scale, and the trinkets she'd buy from your hoard
            const u32 brass = paint::lit(l, pal::kCoin), dark = paint::lit(l, pal::kWoodDark);
            C2D_DrawRectSolid(cx - 1.5f, top - 34, 0, 3, 34, dark);
            C2D_DrawRectSolid(cx - 26, top - 34, 0, 52, 3, brass);
            C2D_DrawEllipseSolid(cx - 34, top - 18, 0, 20, 6, brass);
            C2D_DrawEllipseSolid(cx + 14, top - 18, 0, 20, 6, brass);
            C2D_DrawLine(cx - 24, top - 32, dark, cx - 30, top - 16, dark, 1, 0);
            C2D_DrawLine(cx - 24, top - 32, dark, cx - 18, top - 16, dark, 1, 0);
            C2D_DrawLine(cx + 24, top - 32, dark, cx + 18, top - 16, dark, 1, 0);
            C2D_DrawLine(cx + 24, top - 32, dark, cx + 30, top - 16, dark, 1, 0);
            int shown = 0;
            for (int k = 0; k < kTrinkets && shown < 2; ++k)
                if (app.game.hoard[k] > 0) paint::trinket(static_cast<Trinket>(k), shown++ ? cx + 24 : cx - 24, top - 21, 10);
            paint::coinPile(cx + 26, top - 2, 9, 4);
            break;
        }
        default: {  // the egg of the day on a straw nest (the 3D egg is drawn over it on its page)
            C2D_DrawEllipseSolid(cx - 22, top - 9, 0, 44, 13, paint::lit(l, pal::kStraw));
            if (app.marketTab != kEgg || !r3d::ready()) {
                const DailyEgg e = dailyEgg(app.game, day);
                if (app.game.eggBoughtDay != day) {
                    const KindInfo& k = kindInfo(e.kind);
                    egg(cx, top - 20, 20, 26, k.variants[e.variant % kKindVariants].egg[0], elementGlow(k.elements[0]), 0.6f);
                }
            }
            break;
        }
    }
}

void drawTop(App& app) {
    namespace pal = theme::paint;
    const s64 now = nowLocal(app);
    const s32 day = dayIndex(now);
    const paint::Light l = paint::lightFor(now);
    paint::sky(l, app.t, 150);
    paint::clouds(l, app.t, 60, 3.0f);
    // Behind the square: hills, the village's cottages and trees.
    paint::hill(l, 90, 112, 300, 60, pal::kHillFar, 2.6f);
    paint::hill(l, 320, 104, 320, 70, pal::kHillFar, 2.6f);
    paint::tree(l, 18, 150, 44, 0, 1.6f);
    paint::cottage(l, 40, 150, 46, 34, pal::kRoof, app.t, 1.6f);
    paint::tree(l, 112, 146, 36, 1, 1.6f);
    paint::cottage(l, 150, 148, 40, 30, pal::kRoofBlue, app.t, 1.6f);
    paint::cottage(l, 238, 150, 50, 36, pal::kRoof, app.t, 1.6f);
    paint::tree(l, 306, 148, 42, 2, 1.6f);
    paint::cottage(l, 330, 150, 44, 32, pal::kRoofBlue, app.t, 1.6f);
    // The square's cobbles.
    verticalGradient(0, 150, kTopW, 90, paint::lit(l, pal::kCobbleDark), paint::lit(l, pal::kCobble));
    for (int i = 0; i < 22; ++i) {
        const float cx = std::fmod(i * 73.0f + 19, 410.0f) - 5, cy = 196 + std::fmod(i * 37.0f, 42.0f);
        C2D_DrawEllipseSolid(cx - 9, cy - 3, 0, 18, 6, paint::lit(l, pal::kCobbleLight, 0.55f));
    }
    // The stalls, their wares, the bunting over it all.
    Item today[kStallSpots];
    stallToday(app.game, day, today);
    for (int tab = 0; tab < kTabs; ++tab) {
        stall(app, l, tab, app.marketTab == tab);
        stallWares(app, l, tab, today, day);
    }
    if (app.marketTab == kEgg && r3d::ready() && app.game.eggBoughtDay != day) {  // the egg of the day, rocking on its nest
        static EggMotion rock;
        rock.update(app.dt, 0.0f, app.rng);
        if (rock.rock < 0.02f) rock.knock(0.04f, 0);
        r3d::frameShowcase(4.2f, stallX(kEgg) + kStallW / 2 - kTopW / 2, 4);
        r3d::drawShowcase(app, todaysEgg(app), &rock, now, 0.3f * std::sin(app.t * 0.6f));
    }
    paint::bunting(l, -10, 410, 44, 18, app.t);
    paint::hangingSign(app, str::kMarket, 200, 6, 170);
    // Your Gleam, on a little dark pill so it reads against any sky.
    char line[80];
    std::snprintf(line, sizeof(line), "%lu", static_cast<unsigned long>(app.game.gleam));
    const float gw = textWidth(app, line, 0.5f) + 30;
    panel({392 - gw, 8, gw, 20}, withAlpha(theme::kDenPlum, 0.8f));
    C2D_DrawCircleSolid(392 - gw + 11, 18, 0, 6, theme::kClutchGold);
    C2D_DrawCircleSolid(392 - gw + 11, 18, 0, 3, withAlpha(theme::kEmber, 0.6f));
    text(app, line, 386, 10, 0.5f, theme::kShell, C2D_AlignRight);
    // What's in front of you.
    char sub[96];
    sub[0] = 0;
    switch (app.marketTab) {
        case kFood: {
            int pouch = 0;
            for (int f = 0; f < static_cast<int>(Food::Count); ++f) pouch += pouchCount(app.game, static_cast<Food>(f));
            std::snprintf(sub, sizeof(sub), str::kPouchHolds, pouch);
            paint::caption(app, str::kStallFoodCap, sub, 198);
            break;
        }
        case kGoods:
            if (app.goodsPick < kStallSpots && today[app.goodsPick] != Item::Count) {
                const ItemInfo& info = itemInfo(today[app.goodsPick]);
                paint::caption(app, info.name, info.blurb, 198);
            } else {
                paint::caption(app, str::kGoodsCap, str::kStallTomorrow, 198);
            }
            break;
        case kSell: {
            int hoard = 0;
            for (int k = 0; k < kTrinkets; ++k) hoard += app.game.hoard[k];
            std::snprintf(sub, sizeof(sub), str::kHoardCount, hoard);
            paint::caption(app, str::kTradeCap, sub, 198);
            break;
        }
        default: {
            const DailyEgg e = dailyEgg(app.game, day);
            if (app.game.eggBoughtDay == day) {
                paint::caption(app, str::kEggTomorrowMarket, nullptr, 206);
                break;
            }
            std::snprintf(line, sizeof(line), str::kTodaysEgg, e.sex == Sex::Female ? "female" : "male", kindInfo(e.kind).title);
            std::snprintf(sub, sizeof(sub), str::kEggRarityPrice, rarityName(kindInfo(e.kind).rarity),
                          static_cast<unsigned long>(e.price));
            paint::caption(app, line, sub, 198);
            break;
        }
    }
}

void foodTab(App& app, const Input& in) {
    for (int f = 0; f < static_cast<int>(Food::Count); ++f) {
        const Rect r{8.0f + (f % 5) * 61.0f, 58.0f + (f / 5) * 64.0f, 57, 60};
        const Food food = static_cast<Food>(f);
        panel(r, withAlpha(theme::kShell, 0.18f));
        care::drawFood(food, r.x + r.w / 2, r.y + 20, 0.5f);
        char line[32];
        std::snprintf(line, sizeof(line), str::kPrice, static_cast<unsigned long>(foodPrice(food)));
        textCentered(app, line, r.x + r.w / 2, r.y + 43, 0.34f, theme::kClutchGold, r.w - 4);
        std::snprintf(line, sizeof(line), "x%d", pouchCount(app.game, food));
        text(app, line, r.x + r.w - 3, r.y + 2, 0.32f, withAlpha(theme::kShell, 0.8f), C2D_AlignRight);
        if (in.released && r.contains(in.rx, in.ry)) {
            if (buyFood(app.game, food)) {
                audio::playSfx(audio::Sfx::Register);
                showToast(app, str::kBought);
                saveNow(app);
            } else {
                audio::playSfx(audio::Sfx::Error);
                showToast(app, str::kNotEnoughGleam);
            }
        }
    }
    textCentered(app, str::kTapToBuy, 160, 192, 0.4f, withAlpha(theme::kShell, 0.75f), 300);
}

// Today's goods stall (D86), in the way of a cozy life-sim's shop: four things you don't have
// yet, a new pick each day; tap one to see it on the stall, then buy it (it goes to the den, or
// into the chest: the Den page in the tray puts decor up). A spot sold today stays empty.
void goodsTab(App& app, const Input& in) {
    SaveData& s = app.game;
    const s32 day = dayIndex(nowLocal(app));
    Item today[kStallSpots];
    stallToday(s, day, today);
    if (app.goodsPick >= kStallSpots) app.goodsPick = 0;
    for (int k = 0; k < kStallSpots; ++k) {
        const Rect r{8.0f + k * 77.0f, 42.0f, 73, 120};
        const bool picked = app.goodsPick == k;
        panel(r, picked ? withAlpha(theme::kClutchGold, 0.5f) : withAlpha(theme::kShell, 0.18f));
        if (today[k] == Item::Count) {  // sold today (or nothing left for you): the stall's bare shelf
            textCentered(app, str::kStallEmpty, r.x + r.w / 2, r.y + 50, 0.4f, withAlpha(theme::kShell, 0.55f), r.w - 6);
        } else {
            const ItemInfo& info = itemInfo(today[k]);
            care::drawItem(today[k], r.x + r.w / 2, r.y + 36, 0.8f);
            textCentered(app, info.name, r.x + r.w / 2, r.y + 76, 0.38f, theme::kShell, r.w - 6);
            char price[24];
            std::snprintf(price, sizeof(price), str::kPrice, static_cast<unsigned long>(info.price));
            textCentered(app, price, r.x + r.w / 2, r.y + 96, 0.4f, theme::kClutchGold, r.w - 6);
        }
        if (in.released && r.contains(in.rx, in.ry) && !picked) {
            app.goodsPick = static_cast<u8>(k);
            audio::playSfx(audio::Sfx::Tap);
        }
    }
    const int k = app.goodsPick;
    const Rect act{90, 172, 140, 26};
    if (today[k] != Item::Count) {
        char label[32];
        std::snprintf(label, sizeof(label), str::kBuyFor, static_cast<unsigned long>(itemInfo(today[k]).price));
        if (button(app, act, label, in)) {
            const Item it = today[k];
            if (buyFromStall(s, day, k)) {
                audio::playSfx(audio::Sfx::Register);
                showToast(app, decorSpot(itemInfo(it).kind) >= 0 || static_cast<int>(it) < kToys ? str::kBoughtThing
                                                                                                    : str::kBoughtKeep);
                saveNow(app);
            } else {
                audio::playSfx(audio::Sfx::Error);
                showToast(app, str::kNotEnoughGleam);
            }
        }
    } else {
        textCentered(app, str::kStallTomorrow, 160, 185, 0.42f, withAlpha(theme::kShell, 0.8f), 220);
    }
}

void sellTab(App& app, const Input& in) {
    int shown = 0;
    for (int k = 0; k < kTrinkets; ++k) {
        const Trinket t = static_cast<Trinket>(k);
        const Rect r{8.0f + (k % 2) * 154.0f, 58.0f + (k / 2) * 42.0f, 150, 38};
        const int have = app.game.hoard[k];
        panel(r, withAlpha(theme::kShell, have ? 0.2f : 0.08f));
        paint::trinket(t, r.x + 14, r.y + r.h / 2, have ? 13.0f : 10.0f);  // (U: its little picture)
        char line[48];
        std::snprintf(line, sizeof(line), "%s x%d", trinketName(t), have);
        text(app, line, r.x + 26, r.y + 4, 0.42f, withAlpha(theme::kShell, have ? 1.0f : 0.5f), C2D_AlignLeft, r.w - 30);
        std::snprintf(line, sizeof(line), str::kPrice, static_cast<unsigned long>(trinketValue(t)));
        text(app, line, r.x + 26, r.y + 20, 0.36f, theme::kClutchGold, C2D_AlignLeft, r.w - 30);
        shown += have;
        if (in.released && r.contains(in.rx, in.ry) && sellTrinket(app.game, t)) {
            audio::playSfx(audio::Sfx::Coin);
            showToast(app, str::kSold);
            saveNow(app);
        }
    }
    textCentered(app, shown ? str::kTapToSell : str::kHoardEmpty, 160, 192, 0.4f, withAlpha(theme::kShell, 0.75f), 300);
}

void eggTab(App& app, const Input& in) {
    const DailyEgg e = dailyEgg(app.game, dayIndex(nowLocal(app)));
    char line[64];
    std::snprintf(line, sizeof(line), str::kTodaysEgg, e.sex == Sex::Female ? "female" : "male", kindInfo(e.kind).title);
    textCentered(app, line, 160, 80, 0.6f, theme::kShell, 300);
    std::snprintf(line, sizeof(line), str::kPrice, static_cast<unsigned long>(e.price));
    textCentered(app, line, 160, 104, 0.5f, theme::kClutchGold, 300);
    egg(160, 140, 26, 34, kindInfo(e.kind).variants[e.variant % kKindVariants].egg[0], elementGlow(kindInfo(e.kind).elements[0]), 0.8f);
    const bool sold = app.game.eggBoughtDay == dayIndex(nowLocal(app));
    if (sold) {
        textCentered(app, str::kEggTomorrowMarket, 160, 180, 0.45f, withAlpha(theme::kShell, 0.8f), 300);
    } else if (button(app, {100, 170, 120, 28}, str::kBuyEgg, in)) {
        if (buyDailyEgg(app.game, nowLocal(app)) >= 0) {
            audio::playSfx(audio::Sfx::Register);
            audio::playSfx(audio::Sfx::EggLay);
            showToast(app, str::kEggBought);
            saveNow(app);
        } else {
            audio::playSfx(audio::Sfx::Error);
            showToast(app, app.game.gleam < e.price ? str::kNotEnoughGleam : str::kVaultFull);
        }
    }
}

void drawBottom(App& app, const Input& in) {
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    showTip(app, tips::kTipMarket);  // U: the tutorial
    // Tabs along the top (L / R too).
    static const char* const kTabNames[kTabs] = {str::kTabFood, str::kTabGoods, str::kTabSell, str::kTabEgg};
    for (int k = 0; k < kTabs; ++k) {
        const Rect r{6.0f + k * 78.0f, 6, 74, 30};
        const bool on = app.marketTab == k;
        panel(r, on ? theme::kClutchGold : withAlpha(theme::kShell, 0.2f));
        textCentered(app, kTabNames[k], r.x + r.w / 2, r.y + r.h / 2, 0.42f, on ? theme::kDenPlum : theme::kShell, r.w - 6);
        if (in.released && r.contains(in.rx, in.ry) && !on) {
            app.marketTab = static_cast<u8>(k);
            audio::playSfx(audio::Sfx::Tap);
        }
    }
    switch (app.marketTab) {
        case kFood: foodTab(app, in); break;
        case kGoods: goodsTab(app, in); break;
        case kSell: sellTab(app, in); break;
        default: eggTab(app, in); break;
    }
    if (button(app, {96, 204, 128, 32}, str::kMap, in)) openMap(app);
}

}  // namespace

const SceneFns kMarketScene{update, drawTop, drawBottom};

}  // namespace ec
