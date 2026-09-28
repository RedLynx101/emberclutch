// The challenges (Beta WP8-WP11, D73 6A, D74; 1.0, D89): the scene at the arena (Wren's) and at
// Honeyroot Orchard (Maple's). A notice board by each opens it: the picker on the bottom screen
// (the three challenges, their four cups and what each needs, today's prize, your bests) and the
// place on the top; then the challenge itself (challenge_rings.cpp, a race over the valley;
// challenge_lanterns.cpp in the arena; challenge_fruit.cpp down the orchard), if your partner has
// the energy for it; then the results: the score, the cup, a ribbon and the trophy for the den
// (its first win), Gleam and experience (once a day per cup), the partner's own record, what the
// next cup needs, the host's word and a stinger. The valley's landscape is borrowed (scene_valley's)
// and drawn round it all; leaving goes back out on foot.
#include <cmath>
#include <cstdio>
#include <cstring>

#include "app/audio.hpp"
#include "app/autotest.hpp"
#include "app/challenge_stage.hpp"
#include "app/dialogue.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/campaign.hpp"
#include "core/clock.hpp"
#include "core/daylight.hpp"
#include "core/kinds.hpp"
#include "core/people.hpp"
#include "core/place_layout.hpp"
#include "core/rig.hpp"
#include "core/trainer.hpp"
#include "core/world.hpp"

namespace ec {

namespace stage {
Set& get() {
    static Set s;
    return s;
}
}  // namespace stage

namespace {

using stage::Set;

enum class Phase : u8 { Picker, Play, Results };

struct Scene {
    Phase phase = Phase::Picker;
    int pick = static_cast<int>(Challenge::SkyRings);  // the challenge shown in the picker
    int cup = challenge::kEmber;
    bool explained[kChallenges] = {};  // the host has told you how it goes (this visit)
    challenge::Reward reward;
    const char* hostLine = "";
    float resultsT = 0;
    bool autoplay = false;
    int autoStart = -1;  // a scripted run: this challenge's cup starts at once
};

Scene& sc() {
    static Scene s;
    return s;
}

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
u32 col(u8 r, u8 g, u8 b, float a = 1.0f) { return withAlpha(theme::rgba(r, g, b), a); }

// The notice boards: by the arena's gate (across from Wren) and by the orchard's cart.
struct Board {
    int place;
    Vec2 at;
};
constexpr Board kBoards[] = {{kPlaceArena, {-5.0f, 20.5f}}, {kPlaceOrchard, {-3.5f, 11.0f}}};

// Where everyone stands at a place while you pick (its frame), and the camera.
struct PickerLayout {
    Vec2 you, dragon, host;
    Vec3 eye, target;  // z: above the ground
};
PickerLayout pickerLayout(int place) {
    if (place == kPlaceOrchard) return {{-2.6f, 13.9f}, {-5.2f, 14.6f}, {0.8f, 12.4f}, {3.6f, 22.5f, 4.2f}, {-2.4f, 9.8f, 1.5f}};
    return {{-4.1f, 23.6f}, {-6.9f, 24.0f}, {1.6f, 24.2f}, {-0.6f, 35.5f, 5.6f}, {-2.6f, 20.5f, 1.6f}};
}

const Dragon* partnerOf(const App& app) {
    const Set& s = stage::get();
    return s.partner >= 0 && s.partner < app.game.dragonCount ? &app.game.dragons[s.partner] : nullptr;
}

// The partner's gaits, measured on its own legs (no skating), as the valley does.
void measureSpeeds(Set& s) {
    if (s.speedsSet || s.partner < 0) return;
    const AnimLibrary* lib = r3d::animsFor(s.shown);
    if (!lib) return;
    const int form = s.shown.stage == Stage::Hatchling ? kFormHatchling : kFormGrown;
    const int look = r3d::lookFor(s.shown);
    const ModelData* m = r3d::model(form, look);
    const AnimBinding* bind = r3d::binding(form, look);
    if (!m || !bind) return;
    const int build = s.shown.genome.build < kModelBuilds ? s.shown.genome.build : kBuildNeutral;
    s.actor.updateSpeeds(*m, *bind, *lib, r3d::clipIndexFor(s.shown, form), form * r3d::kLookSlots + look, 1.0f, build,
                         stage::dragonSize(s), form == kFormHatchling);
    s.natWalk = clampf(s.actor.behavior.walkSpeed, 0.6f, 4.0f);
    s.natRun = clampf(s.actor.behavior.runSpeed, s.natWalk * 2.0f, 14.0f);
    s.natTrot = s.actor.behavior.trotSpeed > s.natWalk && s.actor.behavior.trotSpeed < s.natRun ? s.actor.behavior.trotSpeed
                                                                                                 : (s.natWalk + s.natRun) * 0.5f;
    s.speedsSet = true;
}

// Everyone to their places at the board, idle; the camera on them.
void standAtBoard(App& app, Set& s) {
    const PickerLayout L = pickerLayout(s.place);
    s.youAt = stage::onGround(s, L.you);
    s.dragonAt = stage::onGround(s, L.dragon);
    const Vec3 board = stage::onGround(s, s.place == kPlaceOrchard ? kBoards[1].at : kBoards[0].at);
    s.youHeading = stage::headingTo(s.youAt, board);
    s.dragonHeading = stage::headingTo(s.dragonAt, board + Vec3{0, 0, 0});
    s.dragonPitch = s.dragonRoll = 0;
    s.riding = false;
    s.host = s.place == kPlaceOrchard ? Villager::Market : Villager::Steward;
    s.hostShown = true;
    s.hostAt = stage::onGround(s, L.host);
    s.hostHeading = stage::headingTo(s.hostAt, s.youAt);
    s.wantEye = stage::onGround(s, {L.eye.x, L.eye.y}, L.eye.z);
    s.wantTarget = stage::onGround(s, {L.target.x, L.target.y}, L.target.z);
    s.camEase = 2.5f;
    stage::playDragon(s, ClipId::Idle);
    stage::playPerson(s.you, "idle");
    stage::playPerson(s.hostFig, "idle");
    s.breath.clear();
    (void)app;
}

void backToPicker(App& app) {
    Set& s = stage::get();
    sc().phase = Phase::Picker;
    s.place = challenge::placeOf(static_cast<Challenge>(sc().pick));
    standAtBoard(app, s);
}

void startCup(App& app) {
    Scene& c = sc();
    Set& s = stage::get();
    if (s.partner >= 0 && s.partner < app.game.dragonCount) {  // a cup's worth of energy (D89)
        Dragon& d = app.game.dragons[s.partner];
        trainer::spendEnergy(d, trainer::kEnergyChallenge);
        s.shown.needs.energy = d.needs.energy;
    }
    s.pick = static_cast<Challenge>(c.pick);
    s.cup = c.cup;
    s.place = challenge::placeOf(s.pick);
    s.autoplay = c.autoplay;
    s.finished = s.quit = false;
    s.score = 0;
    s.t = 0;
    s.detail[0] = s.detail2[0] = 0;
    s.fx.clear();
    s.breath.clear();
    s.popupT = 0;
    s.snapCam = true;
    s.clip = ClipId::Count;
    c.phase = Phase::Play;
    audio::playSfx(audio::Sfx::Confirm);
    switch (s.pick) {
        case Challenge::SkyRings: rings::begin(app, s); break;
        case Challenge::LanternTrial: lanterns::begin(app, s); break;
        default: fruit::begin(app, s); break;
    }
    // The first time this visit, the host says how it goes (the run waits while they talk).
    if (!c.explained[c.pick] && !s.autoplay) {
        c.explained[c.pick] = true;
        Talk t;
        const char* const* lines = s.pick == Challenge::SkyRings       ? str::kHostRings
                                   : s.pick == Challenge::LanternTrial ? str::kHostLanterns
                                                                       : str::kHostFruit;
        t.lines[0] = lines[0];
        t.lines[1] = lines[1];
        t.count = 2;
        startLines(app, s.pick == Challenge::FruitCatch ? Villager::Market : Villager::Steward, t);
    }
}

// A run starts if the cup's open to this partner (the energy and all), else a word why.
bool tryStartCup(App& app) {
    Scene& c = sc();
    const challenge::Entry e = challenge::entry(app.game, partnerOf(app), static_cast<Challenge>(c.pick), c.cup);
    if (e == challenge::Entry::Open) {
        startCup(app);
        return true;
    }
    audio::playSfx(audio::Sfx::Error);
    if (e == challenge::Entry::Tired) stage::popup(stage::get(), str::kTooTired, theme::kRose);
    return false;
}

// The run is over: record it, tell the campaign, a stinger, the host's word.
void showResults(App& app) {
    Scene& c = sc();
    Set& s = stage::get();
    c.phase = Phase::Results;
    c.resultsT = 0;
    const Challenge ch = s.pick;
    Dragon* partner = s.partner >= 0 && s.partner < app.game.dragonCount ? &app.game.dragons[s.partner] : nullptr;
    c.reward = challenge::record(app.game, ch, s.cup, s.outcome, s.score, dayIndex(nowLocal(app)), partner);
    if (partner) {  // (the one drawn keeps up with its record)
        s.shown.xp = partner->xp;
        s.shown.cupsWon = partner->cupsWon;
    }
    // Its trophy up on the den's shelf (a cup's first win), a level gained.
    if (c.reward.firstWin) queueToastf(app, str::kTrophyUp, challenge::cupName(s.cup));
    if (c.reward.levels > 0 && partner) {
        char line[64];
        std::snprintf(line, sizeof(line), str::kLevelUpTo, partner->name, trainer::levelOf(*partner));
        if (c.reward.firstWin) queueToastf(app, "%s", line);
        else showToastf(app, "%s", line);
        audio::playSfx(audio::Sfx::LevelUp);
    }
    const char* stinger = "results-try-again";
    if (s.outcome == challenge::Outcome::Won) stinger = c.reward.firstWin ? "cup-won" : "results-first";
    else if (s.outcome == challenge::Outcome::Placed) stinger = "results-placed";
    audio::playStinger(stinger);
    const bool crowd = s.place == kPlaceArena;
    if (!s.riding && s.outcome != challenge::Outcome::TryAgain) {  // on the ground: a happy wag, you cheer
        stage::playDragon(s, ClipId::TailWag, 0.25f, true);
        stage::playPerson(s.you, "cheer", 1.0f, 0.2f, true);
        stage::burst(s, Fx::Heart, stage::mouthOf(s) + Vec3{0, 0, 0.5f}, s.outcome == challenge::Outcome::Won ? 4 : 2);
    }
    if (s.outcome == challenge::Outcome::Won) {
        audio::playSfx(crowd ? audio::Sfx::CrowdCheer : audio::Sfx::Giggle);
        audio::playSfx(audio::Sfx::Coin, 1.0f, 0.7f);
        c.hostLine = str::kHostWon[app.rng.below(3)];
    } else if (s.outcome == challenge::Outcome::Placed) {
        if (crowd) audio::playSfx(audio::Sfx::CrowdCheer, 1.1f, 0.5f);
        c.hostLine = str::kHostPlaced[app.rng.below(2)];
    } else {
        if (crowd) audio::playSfx(audio::Sfx::CrowdAww);
        c.hostLine = str::kHostTry[app.rng.below(2)];
    }
    // The Lantern Trial won with every other lantern alight: the great lantern on the stage blazes.
    if (ch == Challenge::LanternTrial && s.outcome == challenge::Outcome::Won) {
        bool others = true;
        for (int p = 0; p < kPlaceCount; ++p)
            if (world::placeInfo(p).lantern && p != kPlaceArena && !world::lanternLit(app.game, p)) others = false;
        if (others && world::lightLantern(app.game, kPlaceArena)) {
            audio::playSfx(audio::Sfx::LanternRelight);
            queueToastf(app, "%s", str::kGreatLantern);
        }
    }
    const campaign::News n = campaign::update(app.game);
    if (n.finished >= 0) queueToastf(app, str::kQuestFinished, campaign::view(app.game, n.finished).title);
    else if (n.stepped >= 0) audio::playSfx(audio::Sfx::QuestPage);
    if (s.pick == Challenge::SkyRings) rings::end(app, s);
    saveNow(app);
}

// ---------------------------------------------------------------------- drawing the stage
void drawFx(App& app, const Set& s) {
    static const u32 kHeart = theme::rgba(0xF2, 0x6D, 0x85), kSpark = theme::rgba(0xFF, 0xF4, 0xC8),
                     kPuff = theme::rgba(0xE8, 0xDC, 0xC4);
    for (int i = 0; i < s.fx.count(); ++i) {
        const Particle& p = s.fx[i];
        float x, y, ppu;
        if (!r3d::project(p.pos, x, y, ppu) || x < -20 || x > kTopW + 20 || y < -20 || y > kScreenH + 20) continue;
        const float a = p.alpha(), sz = std::fmax(1.5f, p.sizeNow() * ppu);
        switch (p.kind) {
            case Fx::Heart: heart(x, y, sz, withAlpha(kHeart, a)); break;
            case Fx::Sparkle:
            case Fx::Glint:
                C2D_DrawRectSolid(x - sz, y - sz * 0.12f, 0, sz * 2, sz * 0.24f, withAlpha(kSpark, a));
                C2D_DrawRectSolid(x - sz * 0.12f, y - sz, 0, sz * 0.24f, sz * 2, withAlpha(kSpark, a));
                C2D_DrawCircleSolid(x, y, 0, sz * 0.45f, withAlpha(kSpark, a * 0.4f));
                break;
            default: C2D_DrawCircleSolid(x, y, 0, sz * 0.5f, withAlpha(kPuff, a * 0.4f)); break;
        }
    }
    // The breath: each element's own puffs.
    const challenge::BreathLook& L = s.breath.look;
    for (int i = 0; i < s.breath.count(); ++i) {
        const challenge::Puff& p = s.breath[i];
        if (p.age < 0) continue;
        float x, y, ppu;
        if (!r3d::project(p.pos, x, y, ppu)) continue;
        const float t = p.t(), sz = std::fmax(1.5f, (p.size0 + (p.size1 - p.size0) * t) * ppu);
        const float a = t < 0.15f ? t / 0.15f : 1.0f - (t - 0.15f) / 0.85f;
        const Rgb c{static_cast<u8>(L.a.r + (L.b.r - L.a.r) * t), static_cast<u8>(L.a.g + (L.b.g - L.a.g) * t),
                    static_cast<u8>(L.a.b + (L.b.b - L.a.b) * t)};
        const u32 k = fromRgb(c);
        switch (L.kind) {
            case challenge::Breath::Flame:
                C2D_DrawCircleSolid(x, y, 0, sz * 1.5f, withAlpha(k, a * 0.3f));
                C2D_DrawCircleSolid(x, y, 0, sz * 0.7f, withAlpha(k, a));
                break;
            case challenge::Breath::Mist: C2D_DrawCircleSolid(x, y, 0, sz, withAlpha(k, a * 0.35f)); break;
            case challenge::Breath::Gust: {  // a streak along its way
                float x2, y2, p2;
                if (r3d::project(p.pos - p.vel * 0.06f, x2, y2, p2))
                    C2D_DrawLine(x, y, withAlpha(k, a), x2, y2, withAlpha(k, 0), std::fmax(1.0f, sz * 0.5f), 0);
                break;
            }
            case challenge::Breath::Spores:
                C2D_DrawCircleSolid(x, y, 0, sz, withAlpha(k, a));
                C2D_DrawCircleSolid(x, y, 0, sz * 2.2f, withAlpha(k, a * 0.2f));
                break;
            case challenge::Breath::Frost:
                C2D_DrawRectSolid(x - sz, y - sz * 0.15f, 0, sz * 2, sz * 0.3f, withAlpha(k, a));
                C2D_DrawRectSolid(x - sz * 0.15f, y - sz, 0, sz * 0.3f, sz * 2, withAlpha(k, a));
                break;
            case challenge::Breath::Light:
                C2D_DrawCircleSolid(x, y, 0, sz * 2.0f, withAlpha(k, a * 0.25f));
                C2D_DrawCircleSolid(x, y, 0, sz * 0.8f, withAlpha(theme::kShell, a));
                break;
        }
    }
}

void drawStage(App& app, Set& s) {
    const s64 now = nowLocal(app);
    Rgb top, horizon, tint;
    valleySky(now, top, horizon, tint);
    verticalGradient(0, 0, kTopW, kScreenH, fromRgb(top), fromRgb(horizon));
    if (!s.valley || !r3d::ready()) return;
    r3d::ValleyView view;
    view.valley = s.valley;
    view.fog = horizon;
    view.tint = tint;
    view.lanternsLit = app.game.world.lanternsLit;
    r3d::PersonView& me = view.people[view.peopleCount++];
    me.form = static_cast<u8>(playerBody(app.game.world.look));
    me.at = s.youAt;
    me.heading = s.youHeading;
    me.anim = &s.you.anim;
    playerPalette(app.game.world.look, me.pal);
    me.hair = static_cast<s8>(app.game.world.look[kLookHair] < kHairStyles ? app.game.world.look[kLookHair] : 0);
    me.blink = s.you.blink;
    me.seated = s.riding;
    if (s.hostShown) {
        r3d::PersonView& h = view.people[view.peopleCount++];
        h.form = static_cast<u8>(personFor(s.host));
        h.at = s.hostAt;
        h.heading = s.hostHeading;
        h.anim = &s.hostFig.anim;
        villagerPalette(s.host, h.pal);
        h.blink = s.hostFig.blink;
    }
    if (s.partner >= 0) {
        view.dragon = &s.shown;
        view.actor = &s.actor;
        view.at = s.dragonAt;
        view.heading = s.dragonHeading;
        view.pitch = s.dragonPitch;
        view.roll = s.dragonRoll;
        view.riderOn = s.riding;
        const float surface = std::fmax(s.valley->heightAt(s.dragonAt.x, s.dragonAt.y), s.valley->water);
        const float high = std::fmax(0.0f, s.dragonAt.z - surface);
        const DayBlend day = dayBlend(now);
        const float lit = day.weight(kLightDay) + 0.6f * day.weight(kLightEvening) + 0.25f * day.weight(kLightNight);
        view.shadowAt = {s.dragonAt.x, s.dragonAt.y, surface};
        view.shadow = 0.5f * lit * clampf(1.0f - high / 60.0f, 0.0f, 1.0f);
        view.shadowRadius = 1.9f * stage::dragonSize(s) * (1.0f - 0.45f * clampf(high / 60.0f, 0.0f, 1.0f));
    }
    for (int i = 0; i < s.otherCount && view.otherCount < r3d::kMaxOthers; ++i) view.others[view.otherCount++] = s.others[i];
    view.focus = s.focus;
    view.eye = s.eye;
    view.target = s.target;
    r3d::drawValley(app, view, now);
    r3d::drawChallengeProps(app, s.props, s.propCount, horizon, now);
    drawFx(app, s);
}

// The boards as props (the scene draws them itself; the valley's own view has them too).
void addBoards(Set& s) {
    for (const Board& b : kBoards) {
        const ValleyPlaceInfo* p = s.valley ? s.valley->place(static_cast<u8>(b.place)) : nullptr;
        if (!p) continue;
        r3d::ChallengeProp prop;
        prop.kind = r3d::PropKind::Board;
        prop.at = placeToWorld3(*s.valley, *p, {b.at.x, b.at.y, 0});
        prop.yaw = p->heading + 3.14159265f;  // its posters face the place's front
        prop.look = boardLook();
        stage::addProp(s, prop);
    }
}

// ---------------------------------------------------------------------- the picker's pictures
void ringIcon(float x, float y, float r, u32 c) {  // a hoop: its band as short thick strokes
    constexpr int kSegs = 16;
    for (int k = 0; k < kSegs; ++k) {
        const float a = k * 6.2831853f / kSegs, b = (k + 1) * 6.2831853f / kSegs, rr = r * 0.8f;
        C2D_DrawLine(x + std::cos(a) * rr, y + std::sin(a) * rr, c, x + std::cos(b) * rr, y + std::sin(b) * rr, c, r * 0.36f, 0);
    }
}
void lanternIcon(float x, float y, float r, u32 c) {
    C2D_DrawRectSolid(x - r * 0.18f, y + r * 0.35f, 0, r * 0.36f, r * 0.75f, col(150, 140, 156));
    C2D_DrawTriangle(x, y - r, c, x + r * 0.5f, y, c, x - r * 0.5f, y, c, 0);
    C2D_DrawTriangle(x - r * 0.5f, y, c, x + r * 0.5f, y, c, x, y + r * 0.55f, c, 0);
}
void fruitIcon(float x, float y, float r, u32 c) {
    C2D_DrawCircleSolid(x, y + r * 0.1f, 0, r * 0.8f, c);
    C2D_DrawRectSolid(x - r * 0.06f, y - r * 0.95f, 0, r * 0.12f, r * 0.35f, col(110, 80, 50));
    C2D_DrawTriangle(x + r * 0.05f, y - r * 0.8f, col(96, 170, 70), x + r * 0.6f, y - r * 1.0f, col(96, 170, 70), x + r * 0.2f,
                     y - r * 0.5f, col(96, 170, 70), 0);
}
void challengeIcon(Challenge c, float x, float y, float r, u32 bg) {
    switch (c) {
        case Challenge::SkyRings: ringIcon(x, y, r, col(250, 204, 90)); break;
        case Challenge::LanternTrial: lanternIcon(x, y, r, col(170, 220, 250)); break;
        default: fruitIcon(x, y, r, col(226, 72, 64)); break;
    }
}
void star(float x, float y, float r, u32 c) {
    for (int k = 0; k < 5; ++k) {
        const float a = -1.5708f + k * 1.2566f, b = a + 0.6283f, d = a - 0.6283f;
        C2D_DrawTriangle(x + std::cos(a) * r, y + std::sin(a) * r, c, x + std::cos(b) * r * 0.42f, y + std::sin(b) * r * 0.42f, c,
                         x + std::cos(d) * r * 0.42f, y + std::sin(d) * r * 0.42f, c, 0);
    }
    C2D_DrawCircleSolid(x, y, 0, r * 0.42f, c);
}

// ---------------------------------------------------------------------- the scene
void update(App& app, const Input& in) {
    Scene& c = sc();
    Set& s = stage::get();
    app.simAccum += app.dt;
    if (app.simAccum >= 1.0f) {
        tickWorld(app);
        app.simAccum = 0;
    }
    if (!s.valley) {  // (the valley must be loaded: back out if not)
        openValleyAt(app, kPlaceArena);
        return;
    }
    measureSpeeds(s);
    s.propCount = 0;
    s.otherCount = 0;
    if (talking(app)) {
        updateTalk(app, in);
    } else if (c.phase == Phase::Picker) {
        if (c.autoStart >= 0) {
            c.pick = c.autoStart;
            c.autoStart = -1;
            startCup(app);
            return;
        }
        if (in.down & KEY_L) c.pick = (c.pick + kChallenges - 1) % kChallenges;
        if (in.down & KEY_R) c.pick = (c.pick + 1) % kChallenges;
        if (in.down & (KEY_L | KEY_R)) audio::playSfx(audio::Sfx::Tap);
        if (in.down & KEY_DLEFT) c.cup = c.cup > challenge::kEmber ? c.cup - 1 : c.cup;
        if (in.down & KEY_DRIGHT) c.cup = c.cup < challenge::kStarfire ? c.cup + 1 : c.cup;
        if ((in.down & KEY_A) && tryStartCup(app)) return;
        if (in.down & KEY_B) {
            audio::playSfx(audio::Sfx::Back);
            openValleyAt(app, s.place);
            return;
        }
    } else if (c.phase == Phase::Play) {
        s.t += app.dt;
        switch (s.pick) {
            case Challenge::SkyRings: rings::update(app, s, in); break;
            case Challenge::LanternTrial: lanterns::update(app, s, in); break;
            default: fruit::update(app, s, in); break;
        }
        if (s.quit) {
            if (s.pick == Challenge::SkyRings) rings::end(app, s);
            backToPicker(app);
        } else if (s.finished) {
            showResults(app);
        }
    } else {  // results
        c.resultsT += app.dt;
        s.t += app.dt;
        if (!s.riding && stage::dragonClipDone(s)) stage::playDragon(s, ClipId::Idle, 0.3f);
        if (stage::personClipDone(s.you)) stage::playPerson(s.you, "idle", 1.0f, 0.3f);
        if (c.autoplay && c.resultsT > 6.0f) backToPicker(app);
        if (c.resultsT > 1.0f && (in.down & KEY_A)) tryStartCup(app);
        if (c.resultsT > 1.0f && (in.down & KEY_B)) backToPicker(app);
    }
    // The world goes on round it: the meadow (and at the arena the crowd's murmur, the village bed),
    // people blink and breathe, the effects drift, the camera eases.
    if (!s.riding) {
        audio::setBed(audio::Bed::Meadow, 0.5f);
        if (s.place == kPlaceArena) audio::setBed(audio::Bed::Village, 0.35f);
    }
    s.fx.update(app.dt);
    s.breath.update(app.dt);
    if (s.popupT > 0) s.popupT -= app.dt;
    for (stage::Figure* f : {&s.you, &s.hostFig}) {
        f->blinkIn -= app.dt;
        if (f->blinkIn <= 0) {
            f->blink = 1;
            f->blinkIn = 1.5f + app.rng.below(3500) * 0.001f;
        }
        f->blink = std::fmax(0.0f, f->blink - app.dt * 7.0f);
        if (const AnimLibrary* lib = r3d::personAnims()) {
            u8 ev[4];
            f->anim.update(*lib, app.dt, ev, 4);
        }
    }
    if (c.phase != Phase::Play || talking(app)) stage::stepDragon(app, s, 0);
    s.actor.eyes.update(0.0f, app.dt);
    if (s.snapCam) {
        s.eye = s.wantEye;
        s.target = s.wantTarget;
        s.snapCam = false;
    } else {
        const float k = std::fmin(1.0f, app.dt * s.camEase);
        s.eye = s.eye + (s.wantEye - s.eye) * k;
        s.target = s.target + (s.wantTarget - s.target) * std::fmin(1.0f, k * 1.6f);
    }
    const float floor = s.valley->heightAt(s.eye.x, s.eye.y) + 1.0f;
    if (s.eye.z < floor) s.eye.z = floor;
}

void drawTop(App& app) {
    Scene& c = sc();
    Set& s = stage::get();
    s.propCount = 0;
    s.otherCount = 0;
    s.focus = 0;
    if (c.phase == Phase::Picker) {
        addBoards(s);
    } else {
        switch (s.pick) {
            case Challenge::SkyRings: rings::scene(app, s); break;
            case Challenge::LanternTrial: lanterns::scene(app, s); break;
            default: fruit::scene(app, s); break;
        }
        if (c.phase == Phase::Results && c.reward.firstWin) {  // the trophy, turning before you
            const Vec3 fwd = normalize(s.target - s.eye), right = normalize(cross(fwd, Vec3{0, 0, 1}));
            r3d::ChallengeProp p;
            p.kind = r3d::PropKind::Trophy;
            p.variant = static_cast<u8>(s.pick);
            // Up on the left, above the card (the dragon has the middle).
            p.at = s.eye + fwd * 3.2f + right * -1.2f + Vec3{0, 0, 0.3f - 0.08f * std::sin(app.t * 2.0f)};
            p.yaw = app.t * 1.4f;
            p.scale = 1.4f * clampf(c.resultsT * 2.0f, 0.0f, 1.0f);
            p.look = trophyLook(s.pick, s.cup);
            stage::addProp(s, p);
        }
    }
    drawStage(app, s);
    if (autotest::shooting())  // (scripted runs: the top screen's load behind each picture)
        autotest::log("challenge %d cup %d phase %d: %lu triangles, %lu draws, %d props", static_cast<int>(s.pick), s.cup,
                      static_cast<int>(c.phase), static_cast<unsigned long>(app.stats.tris),
                      static_cast<unsigned long>(app.stats.draws), s.propCount);
    if (c.phase == Phase::Play || c.phase == Phase::Results) {
        switch (s.pick) {
            case Challenge::SkyRings: rings::hud(app, s); break;
            case Challenge::LanternTrial: lanterns::hud(app, s); break;
            default: fruit::hud(app, s); break;
        }
    }
    if (s.popupT > 0) {  // a line over it all, rising a little as it fades
        const float a = clampf(s.popupT / 0.4f, 0.0f, 1.0f);
        const float y = 70 - (1.6f - std::fmin(1.6f, s.popupT)) * 8;
        const float w = textWidth(app, s.popup, 0.7f) + 28;
        panel({200 - w / 2, y - 16, w, 32}, withAlpha(theme::kDenPlum, 0.75f * a));
        textCentered(app, s.popup, 200, y, 0.7f, withAlpha(s.popupColour, a), 380);
    }
    if (c.phase == Phase::Picker) {  // the challenge and cup on a ribbon across the top
        const Challenge ch = static_cast<Challenge>(c.pick);
        char line[64];
        std::snprintf(line, sizeof(line), "%s - %s", challenge::name(ch), challenge::cupName(c.cup));
        const float w = textWidth(app, line, 0.7f, Face::Title) + 40;
        panel({200 - w / 2, 10, w, 34}, withAlpha(fromRgb(challenge::cupColour(c.cup)), 0.92f));
        textCentered(app, line, 200, 27, 0.7f, theme::kDenPlum, 380, Face::Title);
        const world::PlaceInfo& at = world::placeInfo(challenge::placeOf(ch));
        std::snprintf(line, sizeof(line), str::kHeldAt, at.name);
        const float lw = textWidth(app, line, 0.45f) + 20;
        panel({200 - lw / 2, 45, lw, 16}, withAlpha(theme::kDenPlum, 0.6f));
        textCentered(app, line, 200, 53, 0.45f, theme::kShell, 380);
    }
    if (c.phase == Phase::Results) {  // the card
        const float k = clampf(c.resultsT * 3.0f, 0.0f, 1.0f);
        const Rect card{56, 104 + (1 - k) * 60, 288, 98};  // (clear of the toasts below)
        panel({card.x - 3, card.y - 3, card.w + 6, card.h + 6}, withAlpha(fromRgb(challenge::cupColour(s.cup)), 0.95f * k));
        panel(card, withAlpha(theme::rgba(252, 244, 228), 0.95f * k));
        char line[64];
        const bool won = s.outcome == challenge::Outcome::Won;
        if (won) std::snprintf(line, sizeof(line), str::kCupWon, challenge::cupName(s.cup));
        textCentered(app, won ? line : s.outcome == challenge::Outcome::Placed ? str::kCupPlaced : str::kCupTryAgain, 200,
                     card.y + 18, 0.66f, withAlpha(theme::kDenPlum, k), 270, Face::Title);
        textCentered(app, s.detail, 200, card.y + 42, 0.5f, withAlpha(theme::kDenPlum, k), 270);
        if (c.reward.paidToday) std::snprintf(line, sizeof(line), "%s", str::kPaidToday);
        else if (c.reward.gleam) std::snprintf(line, sizeof(line), str::kGleamXp, static_cast<unsigned long>(c.reward.gleam),
                                               static_cast<unsigned long>(c.reward.xp));
        else std::snprintf(line, sizeof(line), str::kXpOnly, static_cast<unsigned long>(c.reward.xp));
        char extra[64] = {};
        if (c.reward.firstWin) std::snprintf(extra, sizeof(extra), "%s", str::kNewRibbon);
        else if (c.reward.dragonFirst && partnerOf(app))
            std::snprintf(extra, sizeof(extra), str::kDragonFirstCup, partnerOf(app)->name, challenge::cupName(s.cup));
        else if (c.reward.best) std::snprintf(extra, sizeof(extra), "%s", str::kNewBest);
        textCentered(app, line, 200, card.y + 62, c.reward.paidToday ? 0.42f : 0.48f, withAlpha(theme::rgba(196, 140, 40), k), 270);
        textCentered(app, extra, 200, card.y + 81, 0.44f, withAlpha(theme::kRose, k), 270);
    }
}

void pickerBottom(App& app, const Input& in) {
    Scene& c = sc();
    Set& s = stage::get();
    text(app, str::kChallenges, 10, 4, 0.62f, theme::kClutchGold, C2D_AlignLeft, 200, Face::Title);
    char line[80];
    std::snprintf(line, sizeof(line), "%lu", static_cast<unsigned long>(app.game.gleam));
    C2D_DrawCircleSolid(300, 14, 0, 7, theme::kClutchGold);
    text(app, line, 288, 7, 0.5f, theme::kShell, C2D_AlignRight);
    // The three challenges.
    for (int k = 0; k < kChallenges; ++k) {
        const Challenge ch = static_cast<Challenge>(k);
        const Rect r{8.0f + k * 102.0f, 30, 98, 58};
        const bool on = c.pick == k;
        const u32 bg = on ? theme::kClutchGold : withAlpha(theme::kShell, 0.16f);
        panel(r, bg);
        challengeIcon(ch, r.x + 18, r.y + 22, 11, bg);
        text(app, challenge::name(ch), r.x + 34, r.y + 9, 0.42f, on ? theme::kDenPlum : theme::kShell, C2D_AlignLeft, r.w - 38);
        for (int cup = challenge::kEmber; cup <= challenge::kStarfire; ++cup)  // its ribbons so far
            C2D_DrawCircleSolid(r.x + 38 + (cup - 1) * 13, r.y + 38, 0, 4.5f,
                                challenge::ribbon(app.game, ch, cup) ? fromRgb(challenge::cupColour(cup))
                                                                     : withAlpha(on ? theme::kDenPlum : theme::kShell, 0.25f));
        if (in.released && r.contains(in.rx, in.ry) && !on) {
            c.pick = k;
            audio::playSfx(audio::Sfx::Tap);
        }
    }
    const Challenge ch = static_cast<Challenge>(c.pick);
    text(app, challenge::blurb(ch), 160, 93, 0.4f, withAlpha(theme::kShell, 0.85f), C2D_AlignCenter, 304);
    // Today's prize for the cup picked: its first win's, the day's, or won already (D89).
    {
        const s32 today = dayIndex(nowLocal(app));
        if (!challenge::ribbon(app.game, ch, c.cup))
            std::snprintf(line, sizeof(line), str::kPrizeFirst, static_cast<unsigned long>(challenge::firstPrize(c.cup)));
        else if (!trainer::claimedToday(app.game, challenge::claimBit(ch, c.cup), today))
            std::snprintf(line, sizeof(line), str::kPrizeToday, static_cast<unsigned long>(challenge::dayPrize(c.cup)));
        else
            std::snprintf(line, sizeof(line), "%s", str::kPrizeTaken);
        text(app, line, 160, 106, 0.38f, theme::kClutchGold, C2D_AlignCenter, 304);
    }
    // The four cups.
    const Dragon* partner = partnerOf(app);
    for (int cup = challenge::kEmber; cup <= challenge::kStarfire; ++cup) {
        const float x = 44.0f + (cup - 1) * 77.0f, y = 144;
        const bool on = c.cup == cup;
        const bool open = challenge::entry(app.game, partner, ch, cup) == challenge::Entry::Open;
        const bool won = challenge::ribbon(app.game, ch, cup);
        const u32 cc = fromRgb(challenge::cupColour(cup));
        if (on) C2D_DrawCircleSolid(x, y, 0, 25, theme::kShell);
        C2D_DrawCircleSolid(x, y, 0, 22, open || won ? cc : withAlpha(cc, 0.3f));
        if (won) {  // its ribbon's tails, and a star
            C2D_DrawTriangle(x - 12, y + 14, cc, x - 4, y + 16, cc, x - 14, y + 30, cc, 0);
            C2D_DrawTriangle(x + 12, y + 14, cc, x + 4, y + 16, cc, x + 14, y + 30, cc, 0);
            star(x, y, 12, theme::kShell);
        } else if (open) {
            star(x, y, 11, withAlpha(theme::kShell, 0.55f));
        } else {  // a little padlock
            C2D_DrawRectSolid(x - 7, y - 3, 0, 14, 11, withAlpha(theme::kDenPlum, 0.8f));
            C2D_DrawCircleSolid(x, y - 5, 0, 6, withAlpha(theme::kDenPlum, 0.8f));
            C2D_DrawCircleSolid(x, y - 5, 0, 3.4f, withAlpha(cc, 0.3f));
        }
        const char* nm = challenge::cupName(cup);
        char shortName[16];
        std::snprintf(shortName, sizeof(shortName), "%.*s", static_cast<int>(std::strcspn(nm, " ")), nm);
        textCentered(app, shortName, x, y + 32, 0.4f, on ? theme::kClutchGold : theme::kShell, 76);
        if (in.released && std::hypot(in.rx - x, in.ry - y) < 26) {
            if (on && open) {
                if (tryStartCup(app)) return;
            }
            c.cup = cup;
            audio::playSfx(audio::Sfx::Tap);
        }
    }
    // What the cup picked needs, or your best at it.
    const challenge::Entry e = challenge::entry(app.game, partner, ch, c.cup);
    const int best = challenge::best(app.game, ch, c.cup);
    if (e != challenge::Entry::Open) {
        std::snprintf(line, sizeof(line), "%s", challenge::entryText(e));
    } else {
        // Your best, and what the cup asks: rivals to beat, points, lanterns and rounds.
        const challenge::CupNeeds n = challenge::cupNeeds(ch, c.cup, partner && challenge::young(*partner));
        char b[24] = {}, g[32] = {};
        if (!best) std::snprintf(b, sizeof(b), "%s", str::kNoBest);
        else if (ch == Challenge::SkyRings) std::snprintf(b, sizeof(b), str::kBestTime, best / 10.0f);
        else std::snprintf(b, sizeof(b), str::kBestPoints, best);
        if (ch == Challenge::SkyRings) std::snprintf(g, sizeof(g), str::kBeatRivals, n.rivals);
        else if (ch == Challenge::FruitCatch) std::snprintf(g, sizeof(g), str::kGoalPoints, n.goal);
        else std::snprintf(g, sizeof(g), str::kTrialNeeds, n.lanterns, n.rounds);
        std::snprintf(line, sizeof(line), "%s    %s", b, g);
    }
    textCentered(app, line, 160, 193, 0.42f, e == challenge::Entry::Open ? theme::kShell : theme::kRose, 300);
    if (button(app, {8, 204, 92, 32}, str::kLeave, in)) {
        audio::playSfx(audio::Sfx::Back);
        openValleyAt(app, s.place);
        return;
    }
    const bool open = e == challenge::Entry::Open;
    if (button(app, {108, 204, 204, 32}, str::kStartCup, in, open ? theme::kClutchGold : withAlpha(theme::kAsh, 0.6f)))
        tryStartCup(app);
}

void resultsBottom(App& app, const Input& in) {
    Scene& c = sc();
    Set& s = stage::get();
    const Challenge ch = s.pick;
    char line[80];
    std::snprintf(line, sizeof(line), "%s - %s", challenge::name(ch), challenge::cupName(s.cup));
    textCentered(app, line, 160, 18, 0.55f, theme::kClutchGold, 300, Face::Title);
    // The host's word, beside their portrait's colour.
    const VillagerInfo& host = villagerInfo(s.host);
    panel({12, 40, 296, 52}, withAlpha(theme::kShell, 0.14f));
    text(app, host.name, 22, 46, 0.42f, theme::kClutchGold, C2D_AlignLeft, 120);
    text(app, c.hostLine, 22, 64, 0.42f, theme::kShell, C2D_AlignLeft, 280);
    // The run, and your best.
    const int best = challenge::best(app.game, ch, s.cup);
    text(app, s.detail, 160, 98, 0.48f, theme::kShell, C2D_AlignCenter, 300);
    if (s.detail2[0]) text(app, s.detail2, 160, 114, 0.42f, withAlpha(theme::kShell, 0.8f), C2D_AlignCenter, 300);
    if (ch == Challenge::SkyRings) std::snprintf(line, sizeof(line), str::kBestTime, best / 10.0f);
    else std::snprintf(line, sizeof(line), str::kBestPoints, best);
    if (best) text(app, line, 160, s.detail2[0] ? 129 : 118, 0.42f, withAlpha(theme::kShell, 0.7f), C2D_AlignCenter, 300);
    // What's on the shelf for it now: a ribbon per cup won.
    for (int cup = challenge::kEmber; cup <= challenge::kStarfire; ++cup) {
        const float x = 106.0f + (cup - 1) * 36.0f;
        const bool won = challenge::ribbon(app.game, ch, cup);
        C2D_DrawCircleSolid(x, 158, 0, 11, won ? fromRgb(challenge::cupColour(cup)) : withAlpha(theme::kShell, 0.15f));
        if (won) star(x, 158, 6, theme::kShell);
    }
    // After a win: what the next cup asks (trophies that mean something, D89).
    if (s.outcome == challenge::Outcome::Won) {
        const Dragon* partner = partnerOf(app);
        char next[80] = {};
        const char* who = "";
        if (s.cup >= challenge::kStarfire) {
            std::snprintf(next, sizeof(next), "%s", str::kAllCupsWon);
        } else {
            const int cup = s.cup + 1;
            const challenge::CupNeeds n = challenge::cupNeeds(ch, cup, partner && challenge::young(*partner));
            if (ch == Challenge::SkyRings) std::snprintf(next, sizeof(next), str::kNextRivals, challenge::cupName(cup), n.rivals);
            else if (ch == Challenge::FruitCatch) std::snprintf(next, sizeof(next), str::kNextGoal, challenge::cupName(cup), n.goal);
            else std::snprintf(next, sizeof(next), str::kNextTrial, challenge::cupName(cup), n.lanterns, n.rounds, n.hearts);
            if (partner && n.grown && partner->stage != Stage::Adult) who = str::kNextGrown;
            else if (partner && n.juvenile && partner->stage == Stage::Hatchling) who = str::kNextJuvenile;
        }
        text(app, next, 160, 172, 0.4f, theme::kClutchGold, C2D_AlignCenter, 304);
        if (who[0]) text(app, who, 160, 184, 0.36f, withAlpha(theme::kShell, 0.7f), C2D_AlignCenter, 304);
    }
    if (c.resultsT > 1.0f) {
        if (button(app, {8, 196, 146, 38}, str::kAgain, in)) tryStartCup(app);
        else if (button(app, {166, 196, 146, 38}, str::kDone, in, theme::kClutchGold)) backToPicker(app);
    }
}

void drawBottom(App& app, const Input& touch) {
    Scene& c = sc();
    Set& s = stage::get();
    static const Input kNothing{};
    const Input& in = talking(app) ? kNothing : touch;
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    switch (c.phase) {
        case Phase::Picker: pickerBottom(app, in); break;
        case Phase::Play:
            switch (s.pick) {
                case Challenge::SkyRings: rings::bottom(app, s, in); break;
                case Challenge::LanternTrial: lanterns::bottom(app, s, in); break;
                default: fruit::bottom(app, s, in); break;
            }
            break;
        case Phase::Results: resultsBottom(app, in); break;
    }
    drawTalk(app);
}

}  // namespace

// ---------------------------------------------------------------------- the stage's helpers
namespace stage {

Vec3 onGround(const Set& s, Vec2 local, float up) {
    const ValleyPlaceInfo* p = s.valley ? s.valley->place(static_cast<u8>(s.place)) : nullptr;
    return p ? placeToWorld3(*s.valley, *p, {local.x, local.y, up}) : Vec3{};
}

float worldHeading(const Set& s, float local) {
    const ValleyPlaceInfo* p = s.valley ? s.valley->place(static_cast<u8>(s.place)) : nullptr;
    return (p ? p->heading : 0.0f) + local;
}

float headingTo(Vec3 from, Vec3 to) { return std::atan2(to.x - from.x, -(to.y - from.y)); }

void playDragon(Set& s, ClipId c, float fade, bool restart) {
    if (c == s.clip && !restart) return;
    const AnimLibrary* lib = r3d::animsFor(s.shown);
    if (!lib || s.partner < 0) return;
    const int form = s.shown.stage == Stage::Hatchling ? kFormHatchling : kFormGrown;
    const int* clips = r3d::clipIndexFor(s.shown, form);
    const int index = clips ? clips[static_cast<int>(c)] : -1;
    if (index >= 0) s.actor.anim.play(index, fade, restart);
    s.clip = c;
}

bool dragonClipDone(const Set& s) {
    const AnimLibrary* lib = r3d::animsFor(s.shown);
    return !lib || s.actor.anim.finished(*lib);
}

void stepDragon(App& app, Set& s, float speed) {
    const AnimLibrary* lib = r3d::animsFor(s.shown);
    if (!lib || s.partner < 0) return;
    float natural = 0, fastest = 1.6f;
    switch (s.clip) {
        case ClipId::Walk: natural = s.natWalk; break;
        case ClipId::Trot: natural = s.natTrot; break;
        case ClipId::Gallop:
        case ClipId::Scamper:
            natural = s.natRun;
            fastest = 2.3f;
            break;
        default: break;
    }
    s.actor.anim.rate = natural > 0 && speed > 0 ? clampf(speed / natural, 0.5f, fastest) : 1.0f;
    u8 events[8];
    const int n = s.actor.anim.update(*lib, app.dt, events, 8);
    for (int k = 0; k < n; ++k) {
        if (events[k] == kAnimFlap) audio::playSfx(audio::Sfx::Wingbeat, 1.0f, 0.7f);
        if (events[k] == kAnimFootstep && !s.riding)
            audio::playSfx(audio::Sfx::DragonStep, 0.94f + 0.12f * (app.rng.below(100) / 100.0f), 0.6f);
    }
}

void playPerson(Figure& f, const char* clip, float rate, float fade, bool restart) {
    const AnimLibrary* lib = r3d::personAnims();
    if (!lib) return;
    const int c = lib->find(clip);
    if (c < 0) return;
    f.anim.play(c, fade, restart);
    f.anim.rate = rate;
}

bool personClipDone(const Figure& f) {
    const AnimLibrary* lib = r3d::personAnims();
    return !lib || f.anim.clip < 0 || f.anim.finished(*lib);
}

void addProp(Set& s, const r3d::ChallengeProp& p) {
    if (s.propCount < kMaxProps) s.props[s.propCount++] = p;
}

void popup(Set& s, const char* text, u32 colour) {
    std::snprintf(s.popup, sizeof(s.popup), "%s", text);
    s.popupT = 1.6f;
    s.popupColour = colour;
}

float dragonSize(const Set& s) {
    const float stageScale = s.shown.stage == Stage::Adult        ? 1.0f
                             : s.shown.stage == Stage::Adolescent ? 0.8f
                             : s.shown.stage == Stage::Juvenile   ? 0.6f
                                                                  : 0.45f;
    return kindSize(s.shown) * stageScale;
}

Vec3 mouthOf(const Set& s) {
    Vec3 head;
    const Vec3 fwd{std::sin(s.dragonHeading), -std::cos(s.dragonHeading), 0};
    if (r3d::headOf(0, head) && length(head - s.dragonAt) < 8.0f) return head + fwd * (0.35f * dragonSize(s));
    return s.dragonAt + Vec3{0, 0, 1.3f * dragonSize(s)} + fwd * (1.4f * dragonSize(s));
}

void finish(App& app, Set& s, int score, challenge::Outcome o, const char* detail) {
    s.finished = true;
    s.score = score;
    s.outcome = o;
    std::snprintf(s.detail, sizeof(s.detail), "%s", detail ? detail : "");
    (void)app;
}

void giveUp(Set& s) {
    s.quit = true;
    audio::playSfx(audio::Sfx::Back);
}

void burst(Set& s, Fx kind, Vec3 at, int count, float scale) { s.fx.emit(kind, at, count, scale); }

}  // namespace stage

// ---------------------------------------------------------------------- what the valley calls
bool challengeBoardNear(const Valley& v, Vec3 at, int& place, float& distance) {
    for (const Board& b : kBoards) {
        const ValleyPlaceInfo* p = v.place(static_cast<u8>(b.place));
        if (!p) continue;
        const Vec2 w = placeToWorld(*p, b.at);
        const float d = std::hypot(at.x - w.x, at.y - w.y);
        if (d < 3.4f) {
            place = b.place;
            distance = d;
            return true;
        }
    }
    return false;
}

void drawChallengeBoards(App& app, const Valley& v, s64 now) {
    r3d::ChallengeProp props[2];
    int n = 0;
    Vec3 eye;
    bool near = false;
    for (const Board& b : kBoards) {
        const ValleyPlaceInfo* p = v.place(static_cast<u8>(b.place));
        if (!p) continue;
        r3d::ChallengeProp& prop = props[n++];
        prop.kind = r3d::PropKind::Board;
        prop.at = placeToWorld3(v, *p, {b.at.x, b.at.y, 0});
        prop.yaw = p->heading + 3.14159265f;
        prop.look = boardLook();
        float x, y, ppu;
        near = near || (r3d::project(prop.at, x, y, ppu) && ppu > 0.4f);
    }
    (void)eye;
    if (!near) return;  // far off: not worth a pass
    Rgb top, horizon, tint;
    valleySky(now, top, horizon, tint);
    r3d::drawChallengeProps(app, props, n, horizon, now);
}

void openChallenges(App& app, int place) {
    Set& s = stage::get();
    Scene& c = sc();
    s.valley = loadedValley();
    // Your partner: the one chosen (D81), else the one you're caring for; eggs stay home.
    s.partner = -1;
    const SaveData& g = app.game;
    for (int i = 0; i < g.dragonCount; ++i)
        if (g.dragons[i].id == g.world.partnerId && g.dragons[i].stage != Stage::Egg) s.partner = i;
    if (s.partner < 0 && hasDragon(app) && activeDragon(app).stage != Stage::Egg) s.partner = app.careIndex;
    if (s.partner >= 0) s.shown = g.dragons[s.partner];
    s.actor = DenActor{};
    s.clip = ClipId::Count;
    s.speedsSet = false;
    s.place = place == kPlaceOrchard ? kPlaceOrchard : kPlaceArena;
    s.snapCam = true;
    c.phase = Phase::Picker;
    // What's offered first: the orchard's own; at the arena the Lantern Trial on the festival night
    // (and for a dragon too young to ride), else Sky Rings.
    const campaign::QuestView festival = campaign::view(app.game, campaign::kQuests - 1);
    const bool trialNight = festival.started && !festival.done && festival.stepIndex >= 1;
    const bool canRide = s.partner >= 0 && s.shown.stage == Stage::Adult;
    c.pick = static_cast<int>(s.place == kPlaceOrchard        ? Challenge::FruitCatch
                              : trialNight || !canRide ? Challenge::LanternTrial
                                                             : Challenge::SkyRings);
    // The first cup not yet won (the Starfire once they all are).
    const int won = app.game.world.cups[c.pick];
    c.cup = won < challenge::kStarfire ? won + 1 : challenge::kStarfire;
    for (bool& e : c.explained) e = false;
    app.scene = SceneId::Challenge;
    audio::playSfx(audio::Sfx::DoorWood, 1.2f, 0.6f);
    if (s.valley) standAtBoard(app, s);
    // Meeting Wren at her board counts as meeting her.
    if (s.place == kPlaceArena && !(app.game.world.flags & kFlagMetSteward)) {
        startTalk(app, Villager::Steward);
    }
}

void openChallengeCup(App& app, int challengeId, int cup) {
    const int place = challenge::placeOf(static_cast<Challenge>(challengeId % kChallenges));
    openChallenges(app, place);
    app.talk.active = false;
    sc().pick = challengeId % kChallenges;
    sc().cup = cup < challenge::kEmber ? challenge::kEmber : (cup > challenge::kStarfire ? challenge::kStarfire : cup);
    sc().autoStart = cup > 0 ? sc().pick : -1;  // (cup 0: the picker, waiting)
}

void setChallengeAutoplay(bool on) { sc().autoplay = on; }

bool wrenOpensChallenges(const App& app) {
    // Anything she says but the festival night's story (that ends the campaign, not in a menu).
    return app.talk.who == Villager::Steward && !(app.talk.talk.sets & kFlagFestival);
}

const char* challengeMusic(const App& app) {
    const Scene& c = sc();
    // The festival night's trial: the festival's own music.
    if (c.phase != Phase::Picker && stage::get().pick == Challenge::LanternTrial &&
        campaign::view(app.game, campaign::kQuests - 1).stepIndex == 1 && campaign::view(app.game, campaign::kQuests - 1).started)
        return "lantern-festival";
    return "cup-day";
}

const SceneFns kChallengeScene{update, drawTop, drawBottom};

}  // namespace ec
