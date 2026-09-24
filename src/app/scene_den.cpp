// The den: egg care, then the dragon's care loop, in the 3D den room (render3d): the room
// lit for the time of day, the dragons' life, and the particles that go with it.
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/autotest.hpp"
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
#include "core/items.hpp"
#include "core/prop_mesh.hpp"
#include "core/rig.hpp"

namespace ec {
namespace {

Rgb glowOf(const Dragon& d) { return heartglowColor(static_cast<Element>(d.genome.elementA)); }

// Babies squeak high; voices deepen with growth and vary a little per dragon (size gene).
float voicePitch(const Dragon& d, s64 now) {
    return (1.45f - 0.55f * bodyScale(d, now)) * (1.08f - 0.16f * (d.genome.size / 255.0f));
}

// The dragon's size relative to an adult, for walking speed and hop height.
float moveScaleOf(const Dragon& d, s64 now) {
    return growthScale(growthFor(d.stage, stageProgress(d, now))) * sizeScale(d.genome);
}

// Animation markers become sounds. Voices are pitched per dragon (up for babies, down for
// grown-ups); a sniff around sometimes brings a curious chirp, sometimes a sneeze.
// The other den dragons are a little quieter than the one you're with (gain).
void playEventSound(App& app, u8 event, const Dragon& d, s64 now, float gain = 1.0f) {
    const float voice = voicePitch(d, now);
    auto play = [&](audio::Sfx s, float pitch) { audio::playSfx(s, pitch, gain); };
    switch (event) {
        case kAnimFootstep: play(audio::Sfx::Step, 0.95f + 0.1f * (d.genome.size / 255.0f)); break;
        case kAnimChomp: play(audio::Sfx::Munch, 1.0f); break;
        case kAnimSwallow: play(audio::Sfx::Gulp, voice); break;
        case kAnimPurr: play(audio::Sfx::Purr, voice); break;
        case kAnimThump:
        case kAnimLand: play(audio::Sfx::Thump, 1.0f); break;
        case kAnimFlap: play(audio::Sfx::Flap, 1.0f); break;
        case kAnimYawn: play(audio::Sfx::Yawn, voice); break;
        case kAnimShake: play(audio::Sfx::Brush, 1.3f); break;
        case kAnimSniff:
            switch (app.rng.below(4)) {
                case 0: play(audio::Sfx::Sneeze, voice); break;
                case 1: play(audio::Sfx::Chirp, voice); break;
                default: break;
            }
            break;
        case kAnimCall: play(d.stage >= Stage::Adolescent ? audio::Sfx::Rumble : audio::Sfx::Trill, voice); break;
        case kAnimWhimper: play(audio::Sfx::Whimper, voice); break;
        case kAnimSqueak: play(audio::Sfx::Squeak, voice); break;
        case kAnimSneeze: play(audio::Sfx::Sneeze, voice); break;
        default: break;
    }
}

// The den's sound beds under the music: the hearth always, the night outside after dark,
// and the eggs' warm hum (louder the warmer the warmest one is).
void denBeds(const App& app, const DenRoster& r, s64 now) {
    const DayBlend light = dayBlend(now);
    audio::setBed(audio::Bed::Hearth, 1.0f);
    audio::setBed(audio::Bed::Night, light.weight(kLightNight) + 0.3f * light.weight(kLightEvening));
    float warmth = -1;
    for (int e = 0; e < kDenEggs; ++e)
        if (r.egg[e] >= 0) warmth = std::fmax(warmth, app.game.dragons[r.egg[e]].warmth);
    if (warmth >= 0) audio::setBed(audio::Bed::EggHum, 0.35f + 0.65f * warmth / 100.0f);
}

// The den in drawing order: the one you care for first (full detail; the close-up and the
// care tools use drawing index 0), then the other dragons by bed, then the eggs by nest.
// Fills `order` with SaveData::dragons indices; returns how many.
int denOrder(const App& app, const DenRoster& r, int* order) {
    int n = 0;
    auto add = [&](int i) {
        if (i < 0) return;
        for (int k = 0; k < n; ++k)
            if (order[k] == i) return;
        order[n++] = i;
    };
    if (careBed(app) >= 0 || careNest(app) >= 0) add(app.careIndex);
    for (int b = 0; b < kDenDragons; ++b)
        if (!r.away[b]) add(r.dragon[b]);  // one out on the Wanderings isn't here
    for (int e = 0; e < kDenEggs; ++e) add(r.egg[e]);
    return n;
}

int drawIndexOf(const int* order, int n, int index) {
    for (int k = 0; k < n; ++k)
        if (order[k] == index) return k;
    return -1;
}

// Particles for what a den dragon just did: dust at its feet, crumbs when it chomps, hearts
// when it purrs, and a "z" now and then while it sleeps. i: its drawing index (its head);
// bed: its bed (its "z" timer).
void effectsFor(App& app, int i, int bed, const Dragon& d, const DenActor& a, const u8* events, int n, s64 now) {
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
        if ((app.zzz[bed] -= app.dt) <= 0) {
            app.fx.emit(Fx::Zzz, head, 1, s);
            app.zzz[bed] = 1.6f;
        }
    } else {
        app.zzz[bed] = 0.6f;
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

// Moves the den's dragons along: behavior decides, animation follows (core/den_actor). Each
// bed's actor is set up for the dragon that sleeps there when it arrives.
void denLife(App& app, const DenRoster& r, s64 now) {
    const AnimLibrary* lib = r3d::anims();
    if (!lib) return;
    const DenLayout den;
    int order[r3d::kDenShown], shown = denOrder(app, r, order);
    DenBehavior* crowd[kDenDragons];
    const Dragon* who[kDenDragons];
    int crowdCount = 0;
    for (int b = 0; b < kDenDragons; ++b) {
        if (r.dragon[b] < 0 || r.away[b]) {
            app.actorId[b] = 0;
            continue;
        }
        const Dragon& d = app.game.dragons[r.dragon[b]];
        DenActor& a = app.actors[b];
        if (app.actorId[b] != d.id) {  // a new arrival (or a new session): into the den
            a.reset(den, d.id * 2654435761u + 17, b);
            a.behavior.pos = {den.home.x + (b - 1) * 1.8f, den.home.y + (b == 1 ? 0.6f : -0.2f)};
            if (r.dragon[b] == app.careIndex) a.behavior.care(Care::Greet, d);  // hello!
            app.actorId[b] = d.id;
        }
        if (r.dragon[b] != app.careIndex) a.behavior.ball = nullptr;  // the ball is for yours
        who[crowdCount] = &d;
        crowd[crowdCount++] = &a.behavior;
    }
    shareCrowd(crowd, crowdCount);  // they walk around each other
    const bool night = isNight(now);
    const DayBlend light = dayBlend(now);
    denSocial(app.social, crowd, who, crowdCount, night, light.weight(kLightDay) + 0.4f * light.weight(kLightEvening),
              app.dt, app.rng);  // games of chase, nuzzles, the sunbeam, snuggling at night
    static Activity lastActivity[kDenDragons] = {};
    for (int b = 0; b < kDenDragons; ++b) {
        if (r.dragon[b] < 0 || r.away[b] || (app.hatch.active && !app.hatch.popped && r.dragon[b] == app.hatch.index))
            continue;
        const Dragon& d = app.game.dragons[r.dragon[b]];
        DenActor& a = app.actors[b];
        const bool yours = r.dragon[b] == app.careIndex;
        u8 events[8];
        matchSpeeds(a, d, now);
        const int n = a.update(d, night, moveScaleOf(d, now), app.dt, *lib, clipsFor(d, now), events, 8);
        for (int i = 0; i < n; ++i) playEventSound(app, events[i], d, now, yours ? 1.0f : 0.6f);
        // A gulp when a meal is finished; a happy squeak when a game of chase ends.
        const Activity activity = a.behavior.activity;
        const float gain = yours ? 1.0f : 0.6f;
        if (lastActivity[b] == Activity::Eat && activity != Activity::Eat)
            audio::playSfx(audio::Sfx::Gulp, voicePitch(d, now), gain);
        if ((lastActivity[b] == Activity::Chase || lastActivity[b] == Activity::Flee) && activity == Activity::Hop)
            audio::playSfx(audio::Sfx::Squeak, voicePitch(d, now), gain);
        lastActivity[b] = activity;
        Vec3 head;  // hearts while two nuzzle
        if (activity == Activity::Nuzzle && a.behavior.step == 2 && app.rng.chance(1, 40) &&
            r3d::headOf(drawIndexOf(order, shown, r.dragon[b]), head))
            app.fx.emit(Fx::Heart, head, 1, moveScaleOf(d, now));
        effectsFor(app, drawIndexOf(order, shown, r.dragon[b]), b, d, a, events, n, now);
        if (autotest::shooting())
            autotest::log("bed %d %s: %s/%d partner %d at (%.1f %.1f)", b, d.name, activityName(a.behavior.activity),
                          a.behavior.step, a.behavior.partner, a.behavior.pos.x, a.behavior.pos.y);
    }
}

// Each egg between rubs: it settles, the dragon inside knocks as hatching nears, and it
// cracks open in stages, each with a crackle.
void eggLife(App& app, const DenRoster& r) {
    for (int e = 0; e < kDenEggs; ++e) {
        if (r.egg[e] < 0) {
            app.eggCracks[e] = -1;
            continue;
        }
        const Dragon& d = app.game.dragons[r.egg[e]];
        const float gain = r.egg[e] == app.careIndex ? 1.0f : 0.6f;
        if (app.eggs[e].update(app.dt, eggProgress(d), app.rng)) audio::playSfx(audio::Sfx::EggKnock, 1.0f, gain);
        const int cracks = eggCracks(d);
        if (app.eggCracks[e] >= 0 && cracks > app.eggCracks[e]) {  // not for cracks it had when loaded
            audio::playSfx(audio::Sfx::EggCrack, 1.0f, gain);
            app.eggs[e].knock(0.2f, 0);
        }
        app.eggCracks[e] = cracks;
    }
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
        if (careNest(app) >= 0) app.eggs[careNest(app)].knock(0.012f * h.strength, 0.0f);  // a flutter you can see
    }
    if (e.dubIn >= 0 && (e.dubIn -= app.dt) < 0) audio::playSfx(audio::Sfx::Thump, 0.6f, gain * 0.7f);
}

// A quarter turn in the nest. Turns a few hours apart count (up to four): the hatchling
// starts out fonder of you.
void turnTheEgg(App& app, Dragon& d, s64 now) {
    if (careNest(app) >= 0) app.eggs[careNest(app)].turn();
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

void startHatch(App& app, int index, int nest) {
    app.hatch = HatchState{};
    app.hatch.active = true;
    app.hatch.index = index;
    app.hatch.nest = nest;
    app.careIndex = index;  // everyone gathers round: the bottom screen shows this one
    app.hatch.skippable = app.game.settings.seenHatch != 0;
    app.eggCare = EggCare{};
    audio::playStinger("hatching");
    showToast(app, str::kHatching);
}

// The cap comes off: it has hatched, and its den life begins in the nest.
void pop(App& app, Dragon& d, s64 now) {
    HatchState& h = app.hatch;
    const DenLayout den;
    const Vec2 nest = den.eggNests[h.nest];
    const int bed = bedForHatchling(app.game);  // checked free before the hatching began
    h.popped = true;
    h.t = std::fmax(h.t, kPopAt);
    h.shell = app.eggs[h.nest];  // the empty shell keeps its spin
    h.shell.capLift = 0;
    h.shellTime = kShellStays;
    tryHatch(d, now, app.rng);  // incubation is complete: it hatches
    d.denSlot = static_cast<u8>(bed >= 0 ? bed : 0);
    markVisit(d, now);
    app.eggs[h.nest] = EggMotion{};
    app.eggCracks[h.nest] = -1;
    audio::playSfx(audio::Sfx::EggCrack);
    audio::playSfx(audio::Sfx::EggHatch);
    app.fx.emit(Fx::Sparkle, {nest.x, nest.y, 0.9f}, 16, 0.7f);
    app.fx.emit(Fx::Puff, {nest.x, nest.y, 0.3f}, 8, 0.7f);
    DenActor& a = app.actors[d.denSlot];
    a.reset(den, d.id * 2654435761u + 17, d.denSlot);
    a.behavior.hatchAt = nest;
    a.behavior.force(Activity::Hatch);
    a.lift = -kSink;
    app.actorId[d.denSlot] = d.id;
    saveNow(app);
}

// The hatching, a few seconds long: the egg shakes harder and harder, the cap pops, the
// hatchling climbs out, shakes off, blinks at its first light and looks at you; then the
// keyboard names it. Skippable (A, B or a tap) once it has been seen.
void hatchLife(App& app, const Input& in, s64 now) {
    HatchState& h = app.hatch;
    if (h.index < 0 || h.index >= app.game.dragonCount) {
        h.active = false;
        return;
    }
    Dragon& d = app.game.dragons[h.index];
    h.t += app.dt;
    const bool skip = h.skippable && ((in.down & (KEY_A | KEY_B)) || in.tapped);
    const DenLayout den;
    if (!h.popped) {
        if ((h.nextKnock -= app.dt) <= 0) {  // shaking harder and harder
            const float k = std::fmin(1.0f, h.t / kPopAt);
            h.nextKnock = 0.5f - 0.3f * k;
            app.eggs[h.nest].knock(0.06f + 0.2f * k, app.rng.below(628) * 0.01f);
            audio::playSfx(k > 0.6f && app.rng.chance(1, 2) ? audio::Sfx::EggCrack : audio::Sfx::EggKnock,
                           0.9f + 0.3f * k);
            app.fx.emit(Fx::Puff, {den.eggNests[h.nest].x, den.eggNests[h.nest].y, 0.1f}, 1, 0.5f);
        }
        if (h.t >= kPopAt || skip) pop(app, d, now);
        return;
    }
    DenActor& a = app.actors[d.denSlot < kDenDragons ? d.denSlot : 0];
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

// What's been bought for the den (WP7): the toys where they lie, the bowl's food, the decor.
void denThings(App& app, s64 now) {
    static r3d::DenThings t;
    static constexpr float kYaw[kToys] = {0.7f, -0.5f, 0.0f, 0.3f};
    const SaveData& s = app.game;
    for (int k = 0; k < kToys; ++k) {
        t.toy[k] = owns(s, static_cast<Item>(k));
        const Vec2 at = toyAt(s, k);
        t.toyAt[k] = {at.x, at.y, k == 2 ? kOrbRadius : 0.0f};
        t.toyYaw[k] = kYaw[k];
    }
    t.bowlFood = bowlFood(s);
    for (int p = 0; p < kDecorSpots; ++p) t.decor[p] = decorAt(s, p);
    const DayBlend light = dayBlend(now);
    t.daylight = light.weight(kLightDay) + 0.4f * light.weight(kLightEvening);
    r3d::setDenThings(&t);
}

void update(App& app, const Input& in) {
    // Dev time skip (also in the dev menu): R+A = +1 hour, R+X = +1 day.
    if ((in.held & KEY_R) && (in.down & KEY_A)) app.game.devOffset += kHour;
    if ((in.held & KEY_R) && (in.down & KEY_X)) app.game.devOffset += kDay;

    app.simAccum += app.dt;
    app.saveAccum += app.dt;
    if (app.simAccum >= 1.0f || (in.held & KEY_R)) {
        tickWorld(app);
        app.simAccum = 0;
    }
    if (app.saveAccum >= 60.0f) {
        saveNow(app);
        app.saveAccum = 0;
    }
    fixCare(app);
    // The D-pad moves the care between the den's dragons and eggs; X opens the map.
    if (!app.hatch.active && (in.down & (KEY_DLEFT | KEY_DRIGHT))) cycleCare(app, (in.down & KEY_DRIGHT) ? 1 : -1);
    if (!app.hatch.active && (in.down & KEY_X) && !(in.held & KEY_R)) {
        openMap(app);
        return;
    }
    const DenRoster r = denRoster(app.game);
    Dragon& d = activeDragon(app);
    if (d.stage != Stage::Egg && careActor(app)) care::update(app, d);
    denLife(app, r, nowLocal(app));
    denEffects(app, nowLocal(app));
    denThings(app, nowLocal(app));
    denBeds(app, r, nowLocal(app));
    eggLife(app, r);
    if (d.stage == Stage::Egg) heartbeat(app, d);
    // A ready egg hatches as soon as there's a bed for the hatchling.
    if (!app.hatch.active)
        for (int e = 0; e < kDenEggs; ++e)
            if (r.egg[e] >= 0 && app.game.dragons[r.egg[e]].incubationSeconds >= kIncubationSeconds &&
                bedForHatchling(app.game) >= 0) {
                startHatch(app, r.egg[e], e);
                break;
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

    // Everyone in the den, the one you care for first (full detail).
    const DenRoster r = denRoster(app.game);
    int order[r3d::kDenShown];
    const int count = denOrder(app, r, order);
    if (r3d::ready()) {
        r3d::DenDragon shown[r3d::kDenShown];
        int n = 0;
        for (int k = 0; k < count; ++k) {
            const Dragon& o = app.game.dragons[order[k]];
            if (o.stage == Stage::Egg) {
                if (!r3d::eggReady()) continue;
                shown[n++] = {&o, nullptr, &app.eggs[o.denSlot], static_cast<s8>(o.denSlot)};
            } else {
                const int bed = o.denSlot;
                const bool justHatched = order[k] == app.hatch.index && app.hatch.shellTime > 0;
                shown[n++] = {&o, app.actorId[bed] == o.id ? &app.actors[bed] : nullptr,
                              justHatched ? &app.hatch.shell : nullptr, static_cast<s8>(app.hatch.nest)};
            }
        }
        r3d::drawDen(app, shown, n, now, &app.fx);
        // With company in the den, a little heart floats over the one you're caring for.
        Vec3 head;
        float hx, hy, ppu;
        if (count > 1 && r3d::headOf(0, head) && r3d::project({head.x, head.y, head.z + 0.35f}, hx, hy, ppu)) {
            const float bob = 2.0f * std::sin(app.t * 3.0f);
            heart(hx, hy - 6 + bob, 9, withAlpha(theme::kClutchGold, 0.9f));
        }
    }

    char line[96];
    if (d.stage == Stage::Egg) {
        const float progress = static_cast<float>(d.incubationSeconds) / kIncubationSeconds;
        if (!room || !r3d::eggReady()) {  // the 2D egg if its model is missing
            float ex = 200, ey = 130, ppu = 84;
            const DenLayout den;
            const Vec2 nest = den.eggNests[careNest(app) > 0 ? careNest(app) : 0];
            if (room) r3d::project({nest.x, nest.y, 0.55f}, ex, ey, ppu);  // unchanged if it fails
            egg(ex, ey, 0.8f * ppu, 1.05f * ppu, {250, 240, 225}, glowOf(d), 0.3f + 0.7f * d.warmth / 100.0f);
        }
        std::snprintf(line, sizeof(line), "%s %s  -  %d%% %s", breedName(d.genome), str::kEggSuffix,
                      static_cast<int>(progress * 100 > 100 ? 100 : progress * 100), str::kIncubated);
        text(app, line, 200, 14, 0.6f, theme::kShell);
        if (d.warmth <= 20) text(app, str::kGettingCold, 200, 184, 0.5f, theme::kRose);
    } else {
        if (!r3d::ready()) dragonPlaceholder(d, 200, 205, bodyScale(d, now), app.t);
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
    if (!app.hatch.active)  // the map, and how to switch
        text(app, count > 1 ? str::kSwitchHint : str::kMapHint, 392, 226, 0.4f, withAlpha(theme::kShell, 0.6f),
             C2D_AlignRight);
}

void drawEggBottom(App& app, const Input& in, Dragon& d, s64 now) {
    const bool waiting = d.incubationSeconds >= kIncubationSeconds && bedForHatchling(app.game) < 0;
    text(app, waiting ? str::kNoBed : str::kHintEgg, 160, 10, 0.5f, waiting ? theme::kClutchGold : theme::kShell,
         C2D_AlignCenter, 300);
    if (!r3d::ready() || !r3d::eggReady())  // otherwise the 3D egg is already drawn (drawCloseUp)
        egg(160, 120, 80, 104, {250, 240, 225}, glowOf(d), 0.3f + 0.7f * d.warmth / 100.0f);
    EggCare& e = app.eggCare;
    const int nest = careNest(app);
    if (in.touching && kEggArea.contains(in.tx, in.ty) && nest >= 0) {
        if (app.lastTouchX >= 0) {
            const float dx = in.tx - app.lastTouchX, dy = in.ty - app.lastTouchY;
            const float stroke = std::sqrt(dx * dx + dy * dy);
            warmEgg(d, stroke * 0.05f);
            app.eggs[nest].rub(stroke / 100.0f, dx, dy);  // it rocks under your hand
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
    if (button(app, {236, 30, 76, 26}, str::kToVault, in)) {  // off to the Cold Vault
        const DenRoster r = denRoster(app.game);
        if (r.presentCount() + r.eggCount <= 1) {
            showToast(app, str::kStayHome);
        } else if (storeAway(app.game, app.careIndex)) {
            showToast(app, str::kEggAway);
            fixCare(app);
            saveNow(app);
            return;
        } else {
            showToast(app, str::kVaultFull);
        }
    }
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
    fixCare(app);
    Dragon& d = activeDragon(app);
    const s64 now = nowLocal(app);
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    const int nest = careNest(app);
    if (r3d::ready())  // pet the dragon itself, or rub the egg
        r3d::drawCloseUp(app, d, careActor(app), nest >= 0 ? &app.eggs[nest] : nullptr, now,
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
