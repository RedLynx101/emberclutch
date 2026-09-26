// The Market (Alpha 2 WP5): stalls under bunting. Buy food for Gleam into the pouch, sell the
// hoard's trinkets, and the egg of the day, labelled (D24): one a day (core/market).
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/care_ui.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/clock.hpp"
#include "core/egg.hpp"
#include "core/genetics.hpp"
#include "core/items.hpp"
#include "core/market.hpp"
#include "core/kinds.hpp"

namespace ec {
namespace {

u32 col(u8 r, u8 g, u8 b, float a = 1.0f) { return withAlpha(theme::rgba(r, g, b), a); }

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

void drawTop(App& app) {
    // The market square: sky, stalls with striped awnings, bunting.
    verticalGradient(0, 0, kTopW, kScreenH, col(140, 190, 236), col(250, 226, 186));
    C2D_DrawRectSolid(0, 190, 0, kTopW, 50, col(214, 190, 150));
    for (int i = 0; i < 4; ++i) {
        const float x = 20.0f + i * 96;
        C2D_DrawRectSolid(x, 132, 0, 76, 60, col(200, 168, 128));
        C2D_DrawRectSolid(x + 4, 150, 0, 68, 18, col(170, 130, 90));
        for (int s = 0; s < 5; ++s)
            C2D_DrawRectSolid(x - 4 + s * 17, 112, 0, 9, 22, s % 2 ? col(250, 240, 220) : col(214, 86, 70));
    }
    for (int i = 0; i < 21; ++i)
        C2D_DrawTriangle(i * 20.0f - 4, 70, col(245, 196, 81), i * 20.0f + 12, 70, col(245, 196, 81), i * 20.0f + 4, 84,
                         i % 2 ? col(63, 167, 168) : col(232, 102, 43), 0);
    Item today[kStallSpots];
    stallToday(app.game, dayIndex(nowLocal(app)), today);
    if (app.marketTab == kGoods && app.goodsPick < kStallSpots && today[app.goodsPick] != Item::Count) {  // on the stall
        const Item it = today[app.goodsPick];
        const ItemInfo& info = itemInfo(it);
        C2D_DrawEllipseSolid(160, 150, 0, 80, 14, col(120, 84, 56, 0.35f));
        care::drawItem(it, 200, 118 + 3 * std::sin(app.t * 1.6f), 1.2f);
        panel({30, 160, 340, 40}, col(250, 240, 220, 0.88f));
        textCentered(app, info.name, 200, 170, 0.6f, theme::kDenPlum, 320, Face::Title);
        textCentered(app, info.blurb, 200, 189, 0.42f, theme::kDenPlum, 330);
    }
    if (app.marketTab == kEgg && r3d::ready()) {  // the egg of the day, on the middle stall
        static EggMotion rock;
        rock.update(app.dt, 0.0f, app.rng);
        if (rock.rock < 0.02f) rock.knock(0.04f, 0);
        r3d::drawShowcase(app, todaysEgg(app), &rock, nowLocal(app), 0.3f * std::sin(app.t * 0.6f));
    }
    textCentered(app, str::kMarket, 200, 28, 1.0f, theme::kDenPlum, 380, Face::Title);
    char gleam[32];
    std::snprintf(gleam, sizeof(gleam), "%lu", static_cast<unsigned long>(app.game.gleam));
    C2D_DrawCircleSolid(372, 16, 0, 7, theme::kClutchGold);
    text(app, gleam, 362, 9, 0.5f, theme::kDenPlum, C2D_AlignRight);
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
        C2D_DrawCircleSolid(r.x + 14, r.y + r.h / 2, 0, 6, have ? theme::kSkyTeal : withAlpha(theme::kSkyTeal, 0.3f));
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
