// The Nesting Stone (Alpha 2 WP3, breeds-and-genetics section 4): choose a female and a male
// from the den; the hint says what they still need; ready, they nest together and their egg
// comes the next day (core/breeding settleToNest, layDueEgg). The pair stands on the stone on
// the top screen and nuzzles when they settle.
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/breeding.hpp"
#include "core/den_roster.hpp"
#include "core/genetics.hpp"
#include "core/rig.hpp"

namespace ec {
namespace {

u32 col(u8 r, u8 g, u8 b, float a = 1.0f) { return withAlpha(theme::rgba(r, g, b), a); }

// The den's dragons of one sex.
int denOf(const App& app, Sex sex, int* out) {
    const DenRoster r = denRoster(app.game);
    int n = 0;
    for (int b = 0; b < kDenDragons; ++b)
        if (r.dragon[b] >= 0 && !r.away[b] && app.game.dragons[r.dragon[b]].sex == sex) out[n++] = r.dragon[b];
    return n;
}

const int* clipsOf(const Dragon& d, s64 now) { return r3d::clipIndex(growthFor(d.stage, stageProgress(d, now)).form); }

// Stands the picked pair on the stone, facing each other, and keeps their animations going
// (no behavior: they stay put).
void poseThePair(App& app, s64 now) {
    const AnimLibrary* lib = r3d::anims();
    if (!lib) return;
    const int picks[2] = {app.stoneMother, app.stoneFather};
    for (int i = 0; i < 2; ++i) {
        if (picks[i] < 0) continue;
        const Dragon& d = app.game.dragons[picks[i]];
        DenActor& a = app.stoneActors[i];
        const int* clips = clipsOf(d, now);
        if (app.stoneIds[i] != d.id) {
            a = DenActor{};
            app.stoneIds[i] = d.id;
            const float apart = 0.55f + 0.9f * bodyScale(d, now);
            a.behavior.pos = {i == 0 ? -apart : apart, 0};
            a.behavior.heading = i == 0 ? 1.5708f : -1.5708f;  // facing each other
            a.anim.play(clips[static_cast<int>(ClipId::Idle)], 0.0f, true);
        }
        if (app.stoneCourt > 0 && a.anim.clip != clips[static_cast<int>(ClipId::Nuzzle)])
            a.anim.play(clips[static_cast<int>(ClipId::Nuzzle)], 0.4f);
        else if (app.stoneCourt <= 0 && a.anim.clip == clips[static_cast<int>(ClipId::Nuzzle)])
            a.anim.play(clips[static_cast<int>(ClipId::Idle)], 0.5f);
        a.eyes.update(app.stoneCourt > 0 ? 0.6f : 0.0f, app.dt);
        a.anim.update(*lib, app.dt, nullptr, 0);
    }
}

void update(App& app, const Input& in) {
    const s64 now = nowLocal(app);
    app.simAccum += app.dt;
    if (app.simAccum >= 1.0f) {
        tickWorld(app);
        app.simAccum = 0;
    }
    if (app.stoneCourt > 0) app.stoneCourt -= app.dt;
    poseThePair(app, now);
    if (in.down & KEY_B) {
        openMap(app);
        audio::playSfx(audio::Sfx::Back);
    }
}

void drawTop(App& app) {
    const s64 now = nowLocal(app);
    // A hilltop at golden hour, the flat old stone with its straw.
    verticalGradient(0, 0, kTopW, kScreenH, col(236, 168, 150), col(250, 216, 168));
    C2D_DrawEllipseSolid(-40, 120, 0, 260, 90, col(170, 150, 150));
    C2D_DrawEllipseSolid(200, 110, 0, 260, 100, col(160, 142, 150));
    C2D_DrawEllipseSolid(-60, 150, 0, 520, 160, col(132, 150, 100));
    C2D_DrawEllipseSolid(70, 158, 0, 260, 60, col(120, 116, 124));   // the stone
    C2D_DrawEllipseSolid(80, 150, 0, 240, 50, col(158, 154, 160));
    C2D_DrawEllipseSolid(120, 158, 0, 160, 30, col(214, 184, 110, 0.8f));  // straw
    const bool mother = app.stoneMother >= 0, father = app.stoneFather >= 0;
    if (r3d::ready()) {
        if (mother && father) {
            r3d::drawPair(app, app.game.dragons[app.stoneMother], app.stoneActors[0], app.game.dragons[app.stoneFather],
                          app.stoneActors[1], now);
            if (app.stoneCourt > 0) {  // hearts rise between them
                Vec3 head;
                float x, y, ppu;
                for (int i = 0; i < 2; ++i)
                    if (r3d::headOf(i, head) && r3d::project({head.x, head.y, head.z + 0.3f}, x, y, ppu)) {
                        const float rise = std::fmod(app.t * 30.0f + i * 20, 40.0f);
                        heart(x + (i ? -8 : 8), y - rise, 8, withAlpha(theme::kRose, 1.0f - rise / 40.0f));
                    }
            }
        } else if (mother || father) {
            static EggMotion none;
            r3d::drawShowcase(app, app.game.dragons[mother ? app.stoneMother : app.stoneFather], &none, now, 0.0f);
        }
    }
    textCentered(app, str::kNestingStone, 200, 26, 1.0f, theme::kDenPlum, 380, Face::Title);
}

// A column of name buttons for one sex; returns the one tapped (or -1).
void column(App& app, const Input& in, float x, const char* heading, Sex sex, int& pick) {
    text(app, heading, x + 70, 46, 0.5f, theme::kClutchGold);
    int list[kDenDragons];
    const int n = denOf(app, sex, list);
    if (n == 0) text(app, str::kNoneInDen, x + 70, 76, 0.42f, withAlpha(theme::kShell, 0.7f), C2D_AlignCenter, 136);
    for (int k = 0; k < n; ++k) {
        const Dragon& d = app.game.dragons[list[k]];
        const Rect r{x, 66.0f + k * 34, 140, 30};
        const bool picked = pick == list[k];
        panel(r, picked ? theme::kClutchGold : withAlpha(theme::kShell, 0.2f));
        char line[48];
        std::snprintf(line, sizeof(line), "%s  (%s)", d.name, stageName(d.stage));
        textCentered(app, line, r.x + r.w / 2, r.y + r.h / 2, 0.45f, picked ? theme::kDenPlum : theme::kShell, r.w - 10);
        if (in.released && r.contains(in.rx, in.ry)) {
            pick = picked ? -1 : list[k];
            audio::playSfx(audio::Sfx::Tap);
        }
    }
}

void drawBottom(App& app, const Input& in) {
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    textCentered(app, str::kNestingStone, 160, 16, 0.75f, theme::kClutchGold, 300, Face::Title);
    // Picks must still be den dragons of the right sex.
    auto valid = [&](int i, Sex s) {
        return i >= 0 && i < app.game.dragonCount && app.game.dragons[i].location == Location::Den &&
               app.game.dragons[i].stage != Stage::Egg && app.game.dragons[i].sex == s;
    };
    if (!valid(app.stoneMother, Sex::Female)) app.stoneMother = -1;
    if (!valid(app.stoneFather, Sex::Male)) app.stoneFather = -1;
    column(app, in, 12, str::kHer, Sex::Female, app.stoneMother);
    column(app, in, 168, str::kHim, Sex::Male, app.stoneFather);

    const s64 now = nowLocal(app);
    const char* hint = str::kPickAPair;
    bool ready = false;
    char line[96];
    if (app.game.nestA != 0) {  // a pair is already nesting
        const char* names[2] = {"", ""};
        for (int i = 0; i < app.game.dragonCount; ++i) {
            if (app.game.dragons[i].id == app.game.nestA) names[0] = app.game.dragons[i].name;
            if (app.game.dragons[i].id == app.game.nestB) names[1] = app.game.dragons[i].name;
        }
        std::snprintf(line, sizeof(line), str::kNestingNow, names[0], names[1]);
        hint = line;
    } else if (app.stoneMother >= 0 && app.stoneFather >= 0) {
        const BreedBlock block = breedingBlock(app.game.dragons[app.stoneMother], app.game.dragons[app.stoneFather], now);
        hint = breedBlockHint(block);
        ready = block == BreedBlock::None;
    }
    textCentered(app, hint, 160, 180, 0.45f, ready ? theme::kClutchGold : theme::kShell, 300);
    if (button(app, {16, 200, 150, 34}, str::kNestTogether, in, ready ? 0 : withAlpha(theme::kShell, 0.5f)) && ready) {
        if (settleToNest(app.game, app.stoneMother, app.stoneFather, now)) {
            app.stoneCourt = 4.0f;
            audio::playSfx(audio::Sfx::NestSettle);
            audio::playSfx(audio::Sfx::Trill, 1.1f);
            showToast(app, str::kEggTomorrow);
            saveNow(app);
        }
    }
    if (button(app, {176, 200, 128, 34}, str::kMap, in)) openMap(app);
}

}  // namespace

const SceneFns kNestingStoneScene{update, drawTop, drawBottom};

}  // namespace ec
