// The den: egg care, then the dragon's care loop. The dragon is 3D (render3d); the room
// is still 2D until WP6.
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/clock.hpp"
#include "core/genetics.hpp"
#include "core/rig.hpp"

namespace ec {
namespace {

Rgb glowOf(const Dragon& d) { return heartglowColor(static_cast<Element>(d.genome.elementA)); }

// Babies squeak high; voices deepen with growth and vary a little per dragon (size gene).
float voicePitch(const Dragon& d, s64 now) {
    return (1.45f - 0.55f * bodyScale(d, now)) * (1.08f - 0.16f * (d.genome.size / 255.0f));
}

// Dev "3-dragon test": the other two starters at the same stage stand beside the dragon, so
// the budget overlay shows a full Alpha 2 den (architecture section 1).
int standIns(const Dragon& d, const Dragon** out) {
    static Dragon extra[2];
    for (int i = 0; i < 2; ++i) {
        Rng rng(40 + i);
        extra[i] = d;
        extra[i].id = 0xFFFFFF01u + i;
        extra[i].genome = makePurebred(static_cast<Element>((d.genome.elementA + 1 + i) % 3), rng);
        extra[i].sex = i == 0 ? Sex::Female : Sex::Male;
        out[i] = &extra[i];
    }
    return 2;
}

// The dragon's size relative to an adult, for walking speed and hop height.
float moveScaleOf(const Dragon& d, s64 now) {
    return growthScale(growthFor(d.stage, stageProgress(d, now))) * sizeScale(d.genome);
}

// Animation markers become sounds (placeholders until the Suno set arrives, D35).
void playEventSound(u8 event, const Dragon& d, s64 now) {
    switch (event) {
        case kAnimFootstep: audio::playSfx(audio::Sfx::Step, 0.95f + 0.1f * (d.genome.size / 255.0f)); break;
        case kAnimChomp: audio::playSfx(audio::Sfx::Munch); break;
        case kAnimPurr: audio::playSfx(audio::Sfx::Purr, voicePitch(d, now)); break;
        case kAnimThump:
        case kAnimLand: audio::playSfx(audio::Sfx::Thump); break;
        case kAnimFlap: audio::playSfx(audio::Sfx::Flap); break;
        case kAnimYawn: audio::playSfx(audio::Sfx::Yawn, voicePitch(d, now)); break;
        case kAnimShake: audio::playSfx(audio::Sfx::Brush, 1.3f); break;
        default: break;
    }
}

// The clips for this dragon's current body (hatchlings have a few of their own).
const int* clipsFor(const Dragon& d, s64 now) { return r3d::clipIndex(growthFor(d.stage, stageProgress(d, now)).form); }

// Walk and trot at the speed this dragon's feet actually move (no skating).
void matchSpeeds(DenActor& actor, const Dragon& d, s64 now) {
    const Growth g = growthFor(d.stage, stageProgress(d, now));
    const ModelData* m = r3d::model(g.form);
    const AnimBinding* bind = r3d::binding(g.form);
    if (!m || !bind) return;
    const int build = d.genome.build < kModelBuilds ? d.genome.build : kBuildNeutral;
    actor.updateSpeeds(*m, *bind, *r3d::anims(), r3d::clipIndex(g.form), g.form, g.t, build, sizeScale(d.genome));
}

// Moves the den's dragons along: behavior decides, animation follows (core/den_actor).
void denLife(App& app, s64 now) {
    const AnimLibrary* lib = r3d::anims();
    Dragon& d = activeDragon(app);
    if (!lib || d.stage == Stage::Egg) return;
    const DenLayout den;
    if (!app.actorsReady) {
        app.actors[0].reset(den, d.id * 2654435761u + 17);
        app.actors[1].reset(den, 101);
        app.actors[2].reset(den, 202);
        app.actors[1].behavior.pos = {-1.9f, 1.0f};
        app.actors[2].behavior.pos = {2.0f, 1.3f};
        app.actors[0].behavior.care(Care::Greet, d);  // hello!
        app.actorsReady = true;
    }
    const bool night = isNight(now);
    u8 events[8];
    matchSpeeds(app.actors[0], d, now);
    const int n = app.actors[0].update(d, night, moveScaleOf(d, now), app.dt, *lib, clipsFor(d, now), events, 8);
    for (int i = 0; i < n; ++i) playEventSound(events[i], d, now);
    if (app.denTest) {
        const Dragon* extra[2];
        standIns(d, extra);
        for (int i = 0; i < 2; ++i) {
            matchSpeeds(app.actors[i + 1], *extra[i], now);
            app.actors[i + 1].update(*extra[i], night, moveScaleOf(*extra[i], now), app.dt, *lib,
                                     clipsFor(*extra[i], now), events, 8);
        }
    }
}

void update(App& app, const Input& in) {
    // Dev time skip (also in the dev menu): R+A = +1 hour, R+X = +1 day.
    if ((in.held & KEY_R) && (in.down & KEY_A)) app.game.devOffset += kHour;
    if ((in.held & KEY_R) && (in.down & KEY_X)) app.game.devOffset += kDay;

    app.simAccum += app.dt;
    app.saveAccum += app.dt;
    if (app.simAccum >= 1.0f || (in.held & KEY_R)) {
        const s64 now = nowLocal(app);
        simulate(activeDragon(app), app.game.lastSim, now);
        app.game.lastSim = now;
        app.simAccum = 0;
    }
    if (app.saveAccum >= 60.0f) {
        saveNow(app);
        app.saveAccum = 0;
    }
    denLife(app, nowLocal(app));
}

void drawTop(App& app) {
    const Dragon& d = activeDragon(app);
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
        if (r3d::ready()) {
            r3d::DenDragon shown[3] = {{&d, app.actorsReady ? &app.actors[0] : nullptr}};
            int count = 1;
            if (app.denTest) {
                const Dragon* extra[2];
                standIns(d, extra);
                for (int i = 0; i < 2; ++i) shown[count++] = {extra[i], app.actorsReady ? &app.actors[i + 1] : nullptr};
            }
            r3d::drawDen(app, shown, count, now);
        } else {
            dragonPlaceholder(d, 200, 205, bodyScale(d, now), app.t);
        }
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
        audio::playSfx(audio::Sfx::Crack);
        audio::playSfx(audio::Sfx::HatchPop, voicePitch(d, now));
        audio::playStinger("hatching");  // silent until the Suno stinger exists
        app.actorsReady = false;          // the new hatchling starts its den life with a hello
        showToast(app, str::kHatched);
        saveNow(app);
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
            // The close-up shows the head above the chin; hold L for a belly rub (WP7 maps
            // strokes onto the body properly). The reaction clips purr on their own.
            const PetZone zone = (in.held & KEY_L) ? PetZone::Belly
                                 : in.ty < pad.y + pad.h * 0.5f ? PetZone::Head
                                                                 : PetZone::Chin;
            app.actors[0].behavior.care(Care::Pet, d, zone);
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
        app.actors[0].behavior.care(fav ? Care::FeedFavorite : Care::Feed, d);
        audio::playSfx(audio::Sfx::Munch);
        if (fav) audio::playSfx(audio::Sfx::Chirp, voicePitch(d, now));
        showToast(app, fav ? str::kFedFavorite : str::kFed);
    }
    if (button(app, {88, by, 70, 36}, str::kGroom, in)) {
        groom(d, 40);
        markVisit(d, now);
        app.actors[0].behavior.care(Care::Groom, d);
        audio::playSfx(audio::Sfx::Brush);
        showToast(app, str::kGroomed);
    }
    if (button(app, {164, by, 70, 36}, str::kPlayBtn, in)) {
        play(d, 35);
        markVisit(d, now);
        app.actors[0].behavior.care(Care::Play, d);
        audio::playSfx(audio::Sfx::Chirp, voicePitch(d, now));
        showToast(app, str::kPlayed);
    }
    if (d.upset && button(app, {240, by, 70, 36}, str::kMakeUp, in)) {
        makeUp(d);
        feed(d, 20, true);
        markVisit(d, now);
        app.actors[0].behavior.care(Care::MakeUp, d);
        audio::playSfx(audio::Sfx::Purr, voicePitch(d, now));
        showToast(app, str::kMadeUp);
    }
}

void drawBottom(App& app, const Input& in) {
    Dragon& d = activeDragon(app);
    const s64 now = nowLocal(app);
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    if (d.stage != Stage::Egg && r3d::ready())  // pet the dragon itself
        r3d::drawCloseUp(app, d, app.actorsReady ? &app.actors[0] : nullptr, now);
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
