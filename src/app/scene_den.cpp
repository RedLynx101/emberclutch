// The den: egg care, then the dragon's care loop, in the 3D den room (render3d): the room
// lit for the time of day, the dragons' life, and the particles that go with it.
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
#include "core/daylight.hpp"
#include "core/egg.hpp"
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
        extra[i] = d;  // same stage, but content and awake by day: they get on with their own lives
        extra[i].id = 0xFFFFFF01u + i;
        extra[i].upset = false;
        extra[i].napping = false;
        extra[i].needs = Needs{80, 80, 80, 80};
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

// Animation markers become sounds. Voices are pitched per dragon (up for babies, down for
// grown-ups); a sniff around sometimes brings a curious chirp, sometimes a sneeze.
void playEventSound(App& app, u8 event, const Dragon& d, s64 now) {
    const float voice = voicePitch(d, now);
    switch (event) {
        case kAnimFootstep: audio::playSfx(audio::Sfx::Step, 0.95f + 0.1f * (d.genome.size / 255.0f)); break;
        case kAnimChomp: audio::playSfx(audio::Sfx::Munch); break;
        case kAnimSwallow: audio::playSfx(audio::Sfx::Gulp, voice); break;
        case kAnimPurr: audio::playSfx(audio::Sfx::Purr, voice); break;
        case kAnimThump:
        case kAnimLand: audio::playSfx(audio::Sfx::Thump); break;
        case kAnimFlap: audio::playSfx(audio::Sfx::Flap); break;
        case kAnimYawn: audio::playSfx(audio::Sfx::Yawn, voice); break;
        case kAnimShake: audio::playSfx(audio::Sfx::Brush, 1.3f); break;
        case kAnimSniff:
            switch (app.rng.below(4)) {
                case 0: audio::playSfx(audio::Sfx::Sneeze, voice); break;
                case 1: audio::playSfx(audio::Sfx::Chirp, voice); break;
                default: break;
            }
            break;
        case kAnimCall:
            audio::playSfx(d.stage >= Stage::Adolescent ? audio::Sfx::Rumble : audio::Sfx::Trill, voice);
            break;
        case kAnimWhimper: audio::playSfx(audio::Sfx::Whimper, voice); break;
        case kAnimSqueak: audio::playSfx(audio::Sfx::Squeak, voice); break;
        case kAnimSneeze: audio::playSfx(audio::Sfx::Sneeze, voice); break;
        default: break;
    }
}

// The den's sound beds under the music: the hearth always, the night outside after dark,
// and the egg's warm hum (louder the warmer it is).
void denBeds(const Dragon& d, s64 now) {
    const DayBlend light = dayBlend(now);
    audio::setBed(audio::Bed::Hearth, 1.0f);
    audio::setBed(audio::Bed::Night, light.weight(kLightNight) + 0.3f * light.weight(kLightEvening));
    if (d.stage == Stage::Egg) audio::setBed(audio::Bed::EggHum, 0.35f + 0.65f * d.warmth / 100.0f);
}

// Particles for what a den dragon just did: dust at its feet, crumbs when it chomps, hearts
// when it purrs, and a "z" now and then while it sleeps. i: its place in the den (0 = yours).
void effectsFor(App& app, int i, const Dragon& d, const DenActor& a, const u8* events, int n, s64 now) {
    const float s = moveScaleOf(d, now);
    const Vec3 feet{a.behavior.pos.x, a.behavior.pos.y, 0};
    Vec3 head;
    const bool haveHead = r3d::headOf(i, head);
    for (int e = 0; e < n; ++e) {
        switch (events[e]) {
            case kAnimFootstep: app.fx.emit(Fx::Puff, feet, 1, s * 0.6f); break;
            case kAnimThump:
            case kAnimLand: app.fx.emit(Fx::Puff, feet, 4, s); break;
            case kAnimChomp:
                if (haveHead) app.fx.emit(Fx::Crumb, head, 4, s);
                break;
            case kAnimPurr:
                if (haveHead) app.fx.emit(Fx::Heart, head, 1, s);
                break;
            default: break;
        }
    }
    if (a.behavior.activity == Activity::Sleep && haveHead) {
        if ((app.zzz[i] -= app.dt) <= 0) {
            app.fx.emit(Fx::Zzz, head, 1, s);
            app.zzz[i] = 1.6f;
        }
    } else {
        app.zzz[i] = 0.6f;
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
        app.actors[0].reset(den, d.id * 2654435761u + 17, 0);  // yours: the big nest, the nook
        app.actors[1].reset(den, 101, 1);
        app.actors[2].reset(den, 202, 2);
        app.actors[1].behavior.pos = {-1.9f, 1.0f};
        app.actors[2].behavior.pos = {2.0f, 1.3f};
        app.actors[0].behavior.care(Care::Greet, d);  // hello!
        app.actorsReady = true;
    }
    const bool night = isNight(now);
    u8 events[8];
    DenBehavior* crowd[3] = {&app.actors[0].behavior, &app.actors[1].behavior, &app.actors[2].behavior};
    shareCrowd(crowd, app.denTest ? 3 : 1);  // they walk around each other
    matchSpeeds(app.actors[0], d, now);
    const int n = app.actors[0].update(d, night, moveScaleOf(d, now), app.dt, *lib, clipsFor(d, now), events, 8);
    for (int i = 0; i < n; ++i) playEventSound(app, events[i], d, now);
    // A gulp when a meal is finished.
    static Activity lastActivity = Activity::Idle;
    const Activity activity = app.actors[0].behavior.activity;
    if (lastActivity == Activity::Eat && activity != Activity::Eat) audio::playSfx(audio::Sfx::Gulp, voicePitch(d, now));
    lastActivity = activity;
    effectsFor(app, 0, d, app.actors[0], events, n, now);
    if (app.denTest) {
        const Dragon* extra[2];
        standIns(d, extra);
        for (int i = 0; i < 2; ++i) {
            matchSpeeds(app.actors[i + 1], *extra[i], now);
            const int m = app.actors[i + 1].update(*extra[i], night, moveScaleOf(*extra[i], now), app.dt, *lib,
                                                   clipsFor(*extra[i], now), events, 8);
            effectsFor(app, i + 1, *extra[i], app.actors[i + 1], events, m, now);
        }
    }
}

// The egg between rubs: it settles, the dragon inside knocks as hatching nears, and it
// cracks open in stages, each with a crackle.
void eggLife(App& app, const Dragon& d) {
    if (app.egg.update(app.dt, eggProgress(d), app.rng)) audio::playSfx(audio::Sfx::EggKnock);
    const int cracks = eggCracks(d);
    if (app.eggCracks >= 0 && cracks > app.eggCracks) {  // not for cracks it had when loaded
        audio::playSfx(audio::Sfx::EggCrack);
        app.egg.knock(0.2f, 0);
    }
    app.eggCracks = cracks;
}

// ------------------------------------------------------------------ egg care and the hatching
constexpr float kPopAt = 2.6f;       // the cap comes off
constexpr float kRiseFrom = 2.9f;    // the hatchling starts climbing out...
constexpr float kRiseTime = 1.2f;    // ...for this long
constexpr float kBlinkAt = 4.8f;     // its first blink
constexpr float kNameAt = 6.2f;      // then the keyboard
constexpr float kSink = 0.45f;       // how deep in the shell it starts (den units)
constexpr float kShellStays = 180;   // seconds the empty shell stays in the nest
constexpr Rect kEggArea{100, 60, 120, 120};

// Listening: the heartbeat inside, a "lub-dub" at its pace, clearer as the egg grows; and
// what it sounds like (a hint at the temperament it will hatch with).
void listen(App& app, const Dragon& d) {
    EggCare& e = app.eggCare;
    e.listening = 4.0f;
    e.beatIn = 0.25f;
    e.dubIn = -1;
    showToast(app, eggProgress(d) < 0.3f ? str::kHeartFaint
                   : eggCracks(d) >= 2   ? str::kHeartScratching
                                         : str::kHeartTemperament[static_cast<int>(temperamentOf(d))]);
}

void heartbeat(App& app, const Dragon& d) {
    EggCare& e = app.eggCare;
    if (e.listening <= 0) return;
    e.listening -= app.dt;
    const Heartbeat h = heartbeatOf(d);
    const float gain = 0.25f + 0.6f * h.strength;
    if ((e.beatIn -= app.dt) <= 0) {
        e.beatIn += 60.0f / h.bpm;
        if (temperamentOf(d) == Personality::Curious)  // it keeps changing pace
            e.beatIn *= 0.75f + 0.5f * (app.rng.below(100) / 100.0f);
        audio::playSfx(audio::Sfx::Thump, 0.5f, gain);
        e.dubIn = 14.0f / h.bpm;
        app.egg.knock(0.012f * h.strength, 0.0f);  // a flutter you can see
    }
    if (e.dubIn >= 0 && (e.dubIn -= app.dt) < 0) audio::playSfx(audio::Sfx::Thump, 0.6f, gain * 0.7f);
}

// A quarter turn in the nest. Turns a few hours apart count (up to four): the hatchling
// starts out fonder of you.
void turnTheEgg(App& app, Dragon& d, s64 now) {
    app.egg.turn();
    audio::playSfx(audio::Sfx::Brush, 0.7f, 0.6f);  // the shell on straw
    if (turnEgg(d, now)) {
        audio::playSfx(audio::Sfx::Toast);
        markVisit(d, now);
        showToast(app, str::kTurned);
        saveNow(app);
    } else {
        showToast(app, d.eggTurns >= kMaxEggTurns ? str::kTurnedPlenty : str::kTurnedRecently);
    }
}

void startHatch(App& app) {
    app.hatch = HatchState{};
    app.hatch.active = true;
    app.hatch.skippable = app.game.settings.seenHatch != 0;
    app.eggCare = EggCare{};
    audio::playStinger("hatching");
    showToast(app, str::kHatching);
}

// The cap comes off: it has hatched, and its den life begins in the nest.
void pop(App& app, Dragon& d, s64 now) {
    HatchState& h = app.hatch;
    h.popped = true;
    h.t = std::fmax(h.t, kPopAt);
    h.shell = app.egg;  // the empty shell keeps its spin
    h.shell.capLift = 0;
    h.shellTime = kShellStays;
    tryHatch(d, now, app.rng);  // incubation is complete: it hatches
    markVisit(d, now);
    app.egg = EggMotion{};
    app.eggCracks = -1;
    audio::playSfx(audio::Sfx::EggCrack);
    audio::playSfx(audio::Sfx::EggHatch);
    const DenLayout den;
    app.fx.emit(Fx::Sparkle, {den.eggNest.x, den.eggNest.y, 0.9f}, 16, 0.7f);
    app.fx.emit(Fx::Puff, {den.eggNest.x, den.eggNest.y, 0.3f}, 8, 0.7f);
    DenActor& a = app.actors[0];
    a.reset(den, d.id * 2654435761u + 17, 0);
    a.behavior.force(Activity::Hatch);
    a.lift = -kSink;
    app.actorsReady = true;
    saveNow(app);
}

// The hatching, a few seconds long: the egg shakes harder and harder, the cap pops, the
// hatchling climbs out, shakes off, blinks at its first light and looks at you; then the
// keyboard names it. Skippable (A, B or a tap) once it has been seen.
void hatchLife(App& app, const Input& in, s64 now) {
    HatchState& h = app.hatch;
    Dragon& d = activeDragon(app);
    h.t += app.dt;
    const bool skip = h.skippable && ((in.down & (KEY_A | KEY_B)) || in.tapped);
    const DenLayout den;
    if (!h.popped) {
        if ((h.nextKnock -= app.dt) <= 0) {  // shaking harder and harder
            const float k = std::fmin(1.0f, h.t / kPopAt);
            h.nextKnock = 0.5f - 0.3f * k;
            app.egg.knock(0.06f + 0.2f * k, app.rng.below(628) * 0.01f);
            audio::playSfx(k > 0.6f && app.rng.chance(1, 2) ? audio::Sfx::EggCrack : audio::Sfx::EggKnock,
                           0.9f + 0.3f * k);
            app.fx.emit(Fx::Puff, {den.eggNest.x, den.eggNest.y, 0.1f}, 1, 0.5f);
        }
        if (h.t >= kPopAt || skip) pop(app, d, now);
        return;
    }
    DenActor& a = app.actors[0];
    if (skip && h.t < kNameAt) h.t = kNameAt;
    h.shell.capLift = std::fmin(2.0f, h.shell.capLift + app.dt / 0.35f);  // pops, flies up, gone
    const float rise = std::fmin(1.0f, std::fmax(0.0f, (h.t - kRiseFrom) / kRiseTime));
    a.lift = -kSink * (1.0f - rise * (2.0f - rise));  // eased out: it climbs, then settles
    if (!h.blinked && h.t >= kBlinkAt) {
        h.blinked = true;
        a.eyes.blink = 0;  // its first blink
        audio::playSfx(audio::Sfx::Squeak, voicePitch(d, now));
        Vec3 head;
        if (r3d::headOf(0, head)) app.fx.emit(Fx::Heart, head, 2, 0.5f);
    }
    if (!h.asked && h.t >= kNameAt) {
        h.asked = true;
        app.keyboard = KeyboardFor::NameHatchling;  // opens after this frame (main.cpp)
    }
    if (h.named) {
        h.active = false;
        a.lift = 0;
        a.behavior.care(Care::Greet, d);  // out of the nest to say hello
        app.game.settings.seenHatch = 1;
        showToastf(app, str::kSayHello, d.name);
        saveNow(app);
    }
}

// Particles move on; the room adds its own life: embers over the hearth, motes in the
// sunbeam, glints on the hoard.
void denEffects(App& app, s64 now) {
    if (r3d::roomReady()) {
        const DayBlend light = dayBlend(now);
        const float daylight = light.weight(kLightDay) + 0.4f * light.weight(kLightEvening);
        app.ambience.update(app.fx, DenLayout{}, daylight, app.dt);
    }
    app.fx.update(app.dt);
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
    Dragon& d = activeDragon(app);
    if (d.stage != Stage::Egg && app.actorsReady) care::update(app, d);
    denLife(app, nowLocal(app));
    denEffects(app, nowLocal(app));
    denBeds(d, nowLocal(app));
    if (d.stage == Stage::Egg) {
        eggLife(app, d);
        heartbeat(app, d);
        if (!app.hatch.active && d.incubationSeconds >= kIncubationSeconds) startHatch(app);
    }
    if (app.hatch.active) hatchLife(app, in, nowLocal(app));
    if (app.hatch.shellTime > 0) {  // the empty shell settles in the nest, then is cleared away
        app.hatch.shellTime -= app.dt;
        app.hatch.shell.update(app.dt, 0.0f, app.rng);
    }
}

void drawTop(App& app) {
    const Dragon& d = activeDragon(app);
    const s64 now = nowLocal(app);
    const bool room = r3d::ready() && r3d::roomReady();
    if (room) {
        const u32 dark = r3d::backdrop(now);  // the dark beyond the cutaway room
        verticalGradient(0, 0, kTopW, kScreenH, dark, dark);
    } else {
        const bool night = isNight(now);
        verticalGradient(0, 0, kTopW, kScreenH, night ? theme::kDenPlum : theme::kDusk,
                         night ? theme::rgba(20, 14, 28) : theme::kDenPlum);
        C2D_DrawEllipseSolid(40, 190, 0, 320, 50, withAlpha(theme::kEmber, 0.18f));  // nest rug
        embers(app.t, kTopW);
    }

    char line[96];
    if (d.stage == Stage::Egg) {
        const float progress = static_cast<float>(d.incubationSeconds) / kIncubationSeconds;
        // The egg rests in the egg nest by the hearth (the 2D egg if its model is missing).
        float ex = 200, ey = 130, ppu = 84;
        if (room) {
            const r3d::DenDragon inNest[1] = {{&d, nullptr, &app.egg}};
            r3d::drawDen(app, inNest, r3d::eggReady() ? 1 : 0, now, &app.fx);
            const DenLayout den;
            r3d::project({den.eggNest.x, den.eggNest.y, 0.55f}, ex, ey, ppu);  // unchanged if it fails
        }
        if (!room || !r3d::eggReady())
            egg(ex, ey, 0.8f * ppu, 1.05f * ppu, {250, 240, 225}, glowOf(d), 0.3f + 0.7f * d.warmth / 100.0f);
        std::snprintf(line, sizeof(line), "%s %s  -  %d%% %s", breedName(d.genome), str::kEggSuffix,
                      static_cast<int>(progress * 100), str::kIncubated);
        text(app, line, 200, 14, 0.6f, theme::kShell);
        if (d.warmth <= 20) text(app, str::kGettingCold, 200, 184, 0.5f, theme::kRose);
    } else {
        if (r3d::ready()) {
            r3d::DenDragon shown[3] = {
                {&d, app.actorsReady ? &app.actors[0] : nullptr, app.hatch.shellTime > 0 ? &app.hatch.shell : nullptr}};
            int count = 1;
            if (app.denTest) {
                const Dragon* extra[2];
                standIns(d, extra);
                for (int i = 0; i < 2; ++i) shown[count++] = {extra[i], app.actorsReady ? &app.actors[i + 1] : nullptr};
            }
            r3d::drawDen(app, shown, count, now, &app.fx);
        } else {
            dragonPlaceholder(d, 200, 205, bodyScale(d, now), app.t);
        }
        if (app.hatch.active)  // no name yet
            std::snprintf(line, sizeof(line), "%s  -  %s %s", str::kHatching, sexName(d.sex), breedName(d.genome));
        else
            std::snprintf(line, sizeof(line), "%s  -  %s %s %s", d.name, sexName(d.sex), breedName(d.genome),
                          stageName(d.stage));
        text(app, line, 200, 8, 0.6f, theme::kShell);
        std::snprintf(line, sizeof(line), "%s %d  -  %s  -  %s%s%s", str::kDay, daysSinceHatch(d, now) + 1,
                      moodName(moodOf(d)), personalityName(d.personality), d.napping ? "  -  " : "",
                      d.napping ? str::kNapping : "");
        text(app, line, 200, 26, 0.45f, theme::kClutchGold);
    }
}

void drawEggBottom(App& app, const Input& in, Dragon& d, s64 now) {
    text(app, str::kHintEgg, 160, 10, 0.5f, theme::kShell);
    if (!r3d::ready() || !r3d::eggReady())  // otherwise the 3D egg is already drawn (drawCloseUp)
        egg(160, 120, 80, 104, {250, 240, 225}, glowOf(d), 0.3f + 0.7f * d.warmth / 100.0f);
    EggCare& e = app.eggCare;
    if (in.touching && kEggArea.contains(in.tx, in.ty)) {
        if (app.lastTouchX >= 0) {
            const float dx = in.tx - app.lastTouchX, dy = in.ty - app.lastTouchY;
            const float stroke = std::sqrt(dx * dx + dy * dy);
            warmEgg(d, stroke * 0.05f);
            app.egg.rub(stroke / 100.0f, dx, dy);  // it rocks under your hand
            // A hand resting on the shell listens.
            e.still = stroke < 1.5f ? e.still + app.dt : 0.0f;
            if (e.still > 0.8f && e.listening <= 0) listen(app, d);
        }
        app.lastTouchX = in.tx;
        app.lastTouchY = in.ty;
    } else {
        e.still = 0;
    }
    gauge(app, 20, 196, str::kWarmth, d.warmth);
    if (button(app, {150, 198, 76, 32}, str::kTurn, in)) turnTheEgg(app, d, now);
    if (button(app, {234, 198, 76, 32}, str::kListen, in)) listen(app, d);
}

void drawHatchBottom(App& app, const Dragon& d) {
    text(app, str::kHatching, 160, 10, 0.6f, theme::kClutchGold);
    if (!r3d::ready() || (d.stage == Stage::Egg && !r3d::eggReady()))
        egg(160, 120, 80, 104, {250, 240, 225}, glowOf(d), 1.0f);
    if (app.hatch.skippable) text(app, str::kSkipHint, 160, 218, 0.45f, withAlpha(theme::kShell, 0.7f));
}

void drawBottom(App& app, const Input& in) {
    Dragon& d = activeDragon(app);
    const s64 now = nowLocal(app);
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    if (r3d::ready())  // pet the dragon itself, or rub the egg
        r3d::drawCloseUp(app, d, app.actorsReady ? &app.actors[0] : nullptr, &app.egg, now,
                         app.hatch.active ? r3d::CloseUpView::Face : care::view(app));
    if (app.hatch.active) {
        drawHatchBottom(app, d);
    } else if (d.stage == Stage::Egg) {
        drawEggBottom(app, in, d, now);
    } else {
        care::drawBottom(app, in, d, now);
    }
    if (!in.touching) app.lastTouchX = app.lastTouchY = -1;
}

}  // namespace

const SceneFns kDenScene{update, drawTop, drawBottom};

}  // namespace ec
