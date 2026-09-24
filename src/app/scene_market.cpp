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

namespace ec {
namespace {

u32 col(u8 r, u8 g, u8 b, float a = 1.0f) { return withAlpha(theme::rgba(r, g, b), a); }

enum Tab : u8 { kFood, kGoods, kSell, kEgg, kTabs };
constexpr int kGoodsPerPage = 10;
constexpr int kGoodsPages = (kItems + kGoodsPerPage - 1) / kGoodsPerPage;

// Today's egg as a dragon record (for drawing it; nothing is saved).
Dragon todaysEgg(const App& app) {
    const DailyEgg e = dailyEgg(app.game, dayIndex(nowLocal(app)));
    Dragon d = makeEgg(0xFFFFFFF0u, e.genome, e.sex, 0);
    d.warmth = 80;
    return d;
}

void update(App& app, const Input& in) {
    audio::setBed(audio::Bed::Market, 1.0f);  // the stalls' murmur (silent until amb-market arrives)
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
    if (app.marketTab == kGoods) {  // the thing picked, on the middle stall
        const Item it = static_cast<Item>(app.goodsPick);
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

// Things to keep (WP7): toys, grooming things, warm stones and decor, a page at a time. Tap
// one to see it on the stall; then buy it, or put it up in the den (or take it down).
void goodsTab(App& app, const Input& in) {
    SaveData& s = app.game;
    for (int k = 0; k < kGoodsPerPage; ++k) {
        const int i = app.goodsPage * kGoodsPerPage + k;
        if (i >= kItems) break;
        const Item it = static_cast<Item>(i);
        const Rect r{8.0f + (k % 5) * 61.0f, 42.0f + (k / 5) * 66.0f, 57, 62};
        const bool picked = app.goodsPick == i;
        panel(r, picked ? withAlpha(theme::kClutchGold, 0.5f) : withAlpha(theme::kShell, 0.18f));
        care::drawItem(it, r.x + r.w / 2, r.y + 24, 0.62f);
        char line[32];
        if (owns(s, it)) std::snprintf(line, sizeof(line), "%s", isUp(s, it) || i < kToys ? str::kInTheDen : str::kYours);
        else std::snprintf(line, sizeof(line), str::kPrice, static_cast<unsigned long>(itemInfo(it).price));
        textCentered(app, line, r.x + r.w / 2, r.y + 52, 0.32f, owns(s, it) ? theme::kShell : theme::kClutchGold, r.w - 4);
        if (in.released && r.contains(in.rx, in.ry) && !picked) {
            app.goodsPick = static_cast<u8>(i);
            audio::playSfx(audio::Sfx::Tap);
        }
    }
    // What can be done with the one picked.
    const Item it = static_cast<Item>(app.goodsPick);
    const ItemInfo& info = itemInfo(it);
    const bool decor = decorSpot(info.kind) >= 0;
    const Rect act{90, 176, 140, 24};
    char label[32];
    if (!owns(s, it)) {
        std::snprintf(label, sizeof(label), str::kBuyFor, static_cast<unsigned long>(info.price));
        if (button(app, act, label, in)) {
            if (buyItem(s, it)) {
                audio::playSfx(audio::Sfx::Register);
                showToast(app, decor || static_cast<int>(it) < kToys ? str::kBoughtThing : str::kBoughtKeep);
                saveNow(app);
            } else {
                audio::playSfx(audio::Sfx::Error);
                showToast(app, str::kNotEnoughGleam);
            }
        }
    } else if (decor && !isUp(s, it)) {
        if (button(app, act, str::kPutUp, in)) {
            putUp(s, it);
            audio::playSfx(audio::Sfx::Confirm);
            showToast(app, str::kUpInDen);
            saveNow(app);
        }
    } else if (decor) {
        if (button(app, act, str::kTakeDown, in)) {
            takeDown(s, decorSpot(info.kind));
            audio::playSfx(audio::Sfx::Back);
            showToast(app, str::kBackInChest);
            saveNow(app);
        }
    } else {
        textCentered(app, static_cast<int>(it) < kToys ? str::kInTheDen : str::kYours, 160, 188, 0.45f,
                     withAlpha(theme::kShell, 0.8f), 200);
    }
    // The stall's pages.
    if (app.goodsPage > 0 && button(app, {8, 176, 40, 24}, "<", in)) app.goodsPage = static_cast<u8>(app.goodsPage - 1);
    if (app.goodsPage + 1 < kGoodsPages && button(app, {272, 176, 40, 24}, ">", in))
        app.goodsPage = static_cast<u8>(app.goodsPage + 1);
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
    std::snprintf(line, sizeof(line), str::kTodaysEgg, e.sex == Sex::Female ? "female" : "male", breedName(e.genome));
    textCentered(app, line, 160, 80, 0.6f, theme::kShell, 300);
    std::snprintf(line, sizeof(line), str::kPrice, static_cast<unsigned long>(e.price));
    textCentered(app, line, 160, 104, 0.5f, theme::kClutchGold, 300);
    egg(160, 140, 26, 34, {250, 240, 225}, heartglowColor(static_cast<Element>(e.genome.elementA)), 0.8f);
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
