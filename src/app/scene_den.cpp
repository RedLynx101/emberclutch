// The den: egg care, then the hatchling's care loop. 2D placeholder art until WP4-WP6.
#include <cmath>
#include <cstdio>

#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/clock.hpp"
#include "core/genetics.hpp"

namespace ec {
namespace {

Rgb glowOf(const Dragon& d) { return heartglowColor(static_cast<Element>(d.genome.elementA)); }

void update(App& app, const Input& in) {
    // Dev time skip (also in the dev menu): R+A = +1 hour, R+X = +1 day.
    if ((in.held & KEY_R) && (in.down & KEY_A)) app.save.devOffset += kHour;
    if ((in.held & KEY_R) && (in.down & KEY_X)) app.save.devOffset += kDay;

    app.simAccum += app.dt;
    app.saveAccum += app.dt;
    if (app.simAccum >= 1.0f || (in.held & KEY_R)) {
        const s64 now = nowLocal(app);
        simulate(app.save.dragon, app.save.lastSim, now);
        app.save.lastSim = now;
        app.simAccum = 0;
    }
    if (app.saveAccum >= 60.0f) {
        writeDevSave(app.save);
        app.saveAccum = 0;
    }
}

void drawTop(App& app) {
    const Dragon& d = app.save.dragon;
    const s64 now = nowLocal(app);
    const bool night = isNight(now);
    verticalGradient(0, 0, kTopW, kScreenH, night ? theme::kDenPlum : theme::kDusk,
                     night ? theme::rgba(20, 14, 28) : theme::kDenPlum);
    C2D_DrawEllipseSolid(40, 190, 0, 320, 50, withAlpha(theme::kEmber, 0.18f));  // nest rug
    embers(app.t, kTopW);

    char line[96];
    if (d.stage == Stage::Egg) {
        const float progress = static_cast<float>(d.incubationSeconds) / kIncubationSeconds;
        egg(200, 130, 70, 92, {250, 240, 225}, glowOf(d), 0.3f + 0.7f * d.warmth / 100.0f);
        std::snprintf(line, sizeof(line), "%s %s  -  %d%% %s", breedName(d.genome), str::kEggSuffix,
                      static_cast<int>(progress * 100), str::kIncubated);
        text(app, line, 200, 14, 0.6f, theme::kShell);
        if (d.warmth <= 20) text(app, str::kGettingCold, 200, 205, 0.5f, theme::kRose);
    } else {
        dragonPlaceholder(d, 200, 205, bodyScale(d, now), app.t);
        std::snprintf(line, sizeof(line), "%s  -  %s %s %s", d.name, sexName(d.sex), breedName(d.genome),
                      stageName(d.stage));
        text(app, line, 200, 8, 0.6f, theme::kShell);
        std::snprintf(line, sizeof(line), "%s %d  -  %s  -  %s%s%s", str::kDay, daysSinceHatch(d, now) + 1,
                      moodName(moodOf(d)), personalityName(d.personality), d.napping ? "  -  " : "",
                      d.napping ? str::kNapping : "");
        text(app, line, 200, 26, 0.45f, theme::kClutchGold);
    }
    if (app.toast) text(app, app.toast, 200, 222, 0.5f, theme::kShell);
}

void drawEggBottom(App& app, const Input& in, Dragon& d, s64 now) {
    text(app, str::kRubEgg, 160, 10, 0.55f, theme::kShell);
    egg(160, 120, 80, 104, {250, 240, 225}, glowOf(d), 0.3f + 0.7f * d.warmth / 100.0f);
    if (in.touching && in.tx > 100 && in.tx < 220 && in.ty > 60 && in.ty < 180) {
        if (app.lastTouchX >= 0) {
            const float dx = in.tx - app.lastTouchX, dy = in.ty - app.lastTouchY;
            warmEgg(d, std::sqrt(dx * dx + dy * dy) * 0.05f);
        }
        app.lastTouchX = in.tx;
        app.lastTouchY = in.ty;
    }
    gauge(app, 20, 196, str::kWarmth, d.warmth);
    if (tryHatch(d, now, app.rng)) {
        markVisit(d, now);
        showToast(app, str::kHatched);
        writeDevSave(app.save);
    }
}

void drawCareBottom(App& app, const Input& in, Dragon& d, s64 now) {
    gauge(app, 12, 6, str::kBelly, d.needs.belly);
    gauge(app, 88, 6, str::kEnergy, d.needs.energy);
    gauge(app, 164, 6, str::kShine, d.needs.shine);
    gauge(app, 240, 6, str::kPlay, d.needs.play);

    // Heartglow orb mirrors the dragon's mood.
    const float level = heartglowLevel(d, app.t);
    glow(282, 88, 30, fromRgb(glowOf(d)), level);
    heart(282, 88, 22, fromRgb(glowOf(d), static_cast<u8>(120 + 135 * level)));
    char line[48];
    std::snprintf(line, sizeof(line), "%s %d", str::kBond, d.bond);
    text(app, line, 282, 116, 0.45f, theme::kShell);

    // Petting area: stroke the stylus across the pad.
    const Rect pad{12, 44, 220, 96};
    panel(pad, withAlpha(theme::kShell, 0.18f));
    text(app, d.upset ? str::kMakeUpHint : str::kStrokeToPet, pad.x + pad.w / 2, pad.y + 38, 0.5f, theme::kShell);
    app.petCooldown -= app.dt;
    if (in.touching && pad.contains(in.tx, in.ty) && app.lastTouchX >= 0 && app.petCooldown <= 0) {
        const float dx = in.tx - app.lastTouchX, dy = in.ty - app.lastTouchY;
        if (dx * dx + dy * dy > 25) {
            pet(d, 3);
            markVisit(d, now);
            app.petCooldown = 0.25f;
        }
    }
    if (in.touching) {
        app.lastTouchX = in.tx;
        app.lastTouchY = in.ty;
    }

    const float by = 150;
    if (button(app, {12, by, 70, 36}, str::kFeed, in)) {
        const bool fav = d.favoriteFood == d.genome.elementA;
        feed(d, 35, fav);
        markVisit(d, now);
        showToast(app, fav ? str::kFedFavorite : str::kFed);
    }
    if (button(app, {88, by, 70, 36}, str::kGroom, in)) {
        groom(d, 40);
        markVisit(d, now);
        showToast(app, str::kGroomed);
    }
    if (button(app, {164, by, 70, 36}, str::kPlayBtn, in)) {
        play(d, 35);
        markVisit(d, now);
        showToast(app, str::kPlayed);
    }
    if (d.upset && button(app, {240, by, 70, 36}, str::kMakeUp, in)) {
        makeUp(d);
        feed(d, 20, true);
        markVisit(d, now);
        showToast(app, str::kMadeUp);
    }
}

void drawBottom(App& app, const Input& in) {
    Dragon& d = app.save.dragon;
    const s64 now = nowLocal(app);
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    if (d.stage == Stage::Egg) {
        drawEggBottom(app, in, d, now);
    } else {
        drawCareBottom(app, in, d, now);
    }
    if (!in.touching) app.lastTouchX = app.lastTouchY = -1;
}

}  // namespace

const SceneFns kDenScene{update, drawTop, drawBottom};

}  // namespace ec
