// The den: egg care, then the dragon's care loop, in the 3D den room (render3d): the room
// lit for the time of day, the dragons' life, and the particles that go with it.
#include <cmath>
#include <cstdio>
#include <cstring>

#include "app/audio.hpp"
#include "app/autotest.hpp"
#include "app/care_ui.hpp"
#include "app/photo.hpp"
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
#include "core/profile.hpp"
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
void playEventSound(App& app, u8 event, const Dragon& d, Activity doing, s64 now, float gain = 1.0f) {
    const float voice = voicePitch(d, now);
    auto play = [&](audio::Sfx s, float pitch) { audio::playSfx(s, pitch, gain); };
    switch (event) {
        case kAnimFootstep: play(audio::Sfx::Step, 0.95f + 0.1f * (d.genome.size / 255.0f)); break;
        case kAnimChomp: play(audio::Sfx::Munch, 1.0f); break;
        case kAnimSwallow: play(audio::Sfx::Gulp, voice); break;
        case kAnimPurr: play(audio::Sfx::Purr, voice); break;
        case kAnimThump:
        case kAnimLand: play(doing == Activity::Kick ? audio::Sfx::LegKick : audio::Sfx::Thump, 1.0f); break;
        case kAnimFlap: play(audio::Sfx::Flap, 1.0f); break;
        case kAnimYawn: play(audio::Sfx::Yawn, voice); break;
        case kAnimShake: play(doing == Activity::Bath ? audio::Sfx::ShakeSpray : audio::Sfx::Brush, doing == Activity::Bath ? 1.0f : 1.3f); break;
        case kAnimSniff:
            switch (app.rng.below(4)) {
                case 0: play(audio::Sfx::Sneeze, voice); break;
                case 1: play(audio::Sfx::Chirp, voice); break;
                default: play(audio::Sfx::Sniff, voice); break;
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
    const int look = r3d::lookFor(d);  // a Tallneck's legs aren't a Classic's
    const ModelData* m = r3d::model(g.form, look);
    const AnimBinding* bind = r3d::binding(g.form, look);
    if (!m || !bind) return;
    const int build = d.genome.build < kModelBuilds ? d.genome.build : kBuildNeutral;
    actor.updateSpeeds(*m, *bind, *r3d::anims(), r3d::clipIndex(g.form), g.form * kLookCount + look, g.t, build,
                       sizeScale(d.genome));
}

// Moves the den's dragons along: behavior decides, animation follows (core/den_actor). Each
// bed's actor is set up for the dragon that sleeps there when it arrives.
// The toys as the dragons find them this frame (WP7): on the floor unless in a mouth or
// mid tug-of-war; the orb rolls about with its own physics and stays where it stops.
void denToys(App& app, const DenRoster& r) {
    DenToys& t = app.denToys;
    const SaveData& s = app.game;
    bool held[kDenToys] = {};
    for (int b = 0; b < kDenDragons; ++b) {
        if (r.dragon[b] < 0 || r.away[b] || app.actorId[b] != s.dragons[r.dragon[b]].id) continue;
        const DenBehavior& db = app.actors[b].behavior;
        if (db.carrying >= 0 && db.carrying < kDenToys) held[db.carrying] = true;
        if (db.activity == Activity::TugWar && db.step == 2) held[1] = true;
    }
    for (int k = 0; k < kDenToys; ++k) {
        t.here[k] = owns(s, static_cast<Item>(k)) && !held[k];
        t.at[k] = toyAt(s, k);
    }
    t.bowlFood = bowlFood(s) != Food::Count;
    app.denOrbWait -= app.dt;
    t.orbTreat = app.denOrbWait <= 0;
    Ball& orb = app.denOrb;
    if (!owns(s, Item::PuzzleOrb)) {
        orb.active = false;
    } else if (!orb.active) {  // first seen this session: where it was left
        orb = Ball{};
        orb.radius = kOrbRadius;
        orb.active = true;
        orb.pos = {t.at[2].x, t.at[2].y, kOrbRadius};
    }
    t.orb = orb.active ? &orb : nullptr;
}

void denLife(App& app, const DenRoster& r, s64 now) {
    const AnimLibrary* lib = r3d::anims();
    if (!lib) return;
    const DenLayout den;
    denToys(app, r);
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
        a.behavior.toys = &app.denToys;
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
        for (int i = 0; i < n; ++i) playEventSound(app, events[i], d, a.behavior.activity, now, yours ? 1.0f : 0.6f);
        // A gulp when a meal is finished; a happy squeak when a game of chase ends.
        const Activity activity = a.behavior.activity;
        const float gain = yours ? 1.0f : 0.6f;
        if (lastActivity[b] == Activity::Eat && activity != Activity::Eat)
            audio::playSfx(audio::Sfx::Gulp, voicePitch(d, now), gain);
        if ((lastActivity[b] == Activity::Chase || lastActivity[b] == Activity::Flee) && activity == Activity::Hop)
            audio::playSfx(audio::Sfx::Squeak, voicePitch(d, now), gain);
        lastActivity[b] = activity;
        DenBehavior& db = a.behavior;  // what it did with the toys (WP7)
        Dragon& mine = app.game.dragons[r.dragon[b]];
        if (db.ateFromBowl) {
            db.ateFromBowl = false;
            if (eatFromBowl(app.game, r.dragon[b], now)) audio::playSfx(audio::Sfx::Gulp, voicePitch(d, now), gain);
        }
        if (db.gotTreat) {  // the orb's treat
            db.gotTreat = false;
            app.denOrbWait = 90;
            feed(mine, 6, false);
            play(mine, 8);
            audio::playSfx(audio::Sfx::TreatDrop, 1.0f, gain);
            audio::playSfx(audio::Sfx::Munch, voicePitch(d, now), gain);
        }
        if (db.nudged && app.denOrb.active) {
            db.nudged = false;
            app.denOrb.launch(app.denOrb.pos, {db.nudge.x * 2.4f, db.nudge.y * 2.4f, 0.6f});
            audio::playSfx(audio::Sfx::OrbRattle, 1.0f, gain);
        }
        if (db.knocked) {
            db.knocked = false;
            setToyAt(app.game, 0, db.knock);
        }
        if (activity == Activity::Play || activity == Activity::TugWar) play(mine, 0.5f * app.dt);
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
// The hatching's moments (Noah's direction, WP12a): shaking, a held breath, the burst.
constexpr float kStillAt = 2.6f;     // it stops shaking, glowing its brightest...
constexpr float kBurstAt = 3.0f;     // ...then flashes and bursts into bits
constexpr float kShapedAt = kBurstAt + kBlobSwell + kBlobShape;  // the hatchling has its shape
constexpr float kBlinkAt = kShapedAt + 0.45f;  // its first blink and cry
constexpr float kNameAt = kBlinkAt + 1.3f;     // then the keyboard
constexpr float kFlashTime = 0.45f;
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
        audio::playSfx(audio::Sfx::EggHeartbeat, 1.0f, gain);
        e.dubIn = audio::has(audio::Sfx::EggHeartbeat) ? -1.0f : 14.0f / h.bpm;  // the stand-in needs its "dub"
        if (careNest(app) >= 0) app.eggs[careNest(app)].knock(0.012f * h.strength, 0.0f);  // a flutter you can see
    }
    if (e.dubIn >= 0 && (e.dubIn -= app.dt) < 0) audio::playSfx(audio::Sfx::Thump, 0.6f, gain * 0.7f);
}

// A quarter turn in the nest. Turns a few hours apart count (up to four): the hatchling
// starts out fonder of you.
void turnTheEgg(App& app, Dragon& d, s64 now) {
    if (careNest(app) >= 0) app.eggs[careNest(app)].turn();
    audio::playSfx(audio::Sfx::EggTurn);  // the shell on straw
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

// The burst: the egg flies apart in a warm flash, and where it stood a small white blob starts
// to swell (it has hatched: its den life begins in the nest).
void pop(App& app, Dragon& d, s64 now) {
    HatchState& h = app.hatch;
    const DenLayout den;
    const Vec2 nest = den.eggNests[h.nest];
    const int bed = makeRoomForHatchling(app.game);  // checked before the hatching began (a wanderer's is lent)
    h.popped = true;
    h.t = std::fmax(h.t, kBurstAt);
    h.flash = 1.0f;
    if (const ShardShape* shapes = r3d::eggShards()) {
        BurstGround ground;
        ground.nest = nest;
        ground.room = den.room;
        ground.wallRadius = den.wallRadius;
        h.burst.start({nest.x, nest.y, kEggNestFloor}, shapes, ground, app.rng);
        r3d::setBurst(&h.burst, &d);  // its egg's colours
    }
    tryHatch(d, now, app.rng);  // incubation is complete: it hatches
    h.dex = dexSee(app.game, d);  // into the Dragondex (told once it's named)
    d.denSlot = static_cast<u8>(bed >= 0 ? bed : 0);
    markVisit(d, now);
    app.eggs[h.nest] = EggMotion{};
    app.eggCracks[h.nest] = -1;
    audio::playSfx(audio::Sfx::EggCrack);
    audio::playSfx(audio::Sfx::EggHatch);
    app.fx.emit(Fx::Sparkle, {nest.x, nest.y, 0.6f}, 18, 0.9f);
    app.fx.emit(Fx::Puff, {nest.x, nest.y, 0.3f}, 10, 0.8f);
    DenActor& a = app.actors[d.denSlot];
    a.reset(den, d.id * 2654435761u + 17, d.denSlot);
    a.behavior.hatchAt = nest;
    a.behavior.force(Activity::Hatch);
    a.lift = 0;
    const BlobShape blob = blobAt(0);
    a.morph = blob.morph;
    a.blobScale = blob.scale;
    app.actorId[d.denSlot] = d.id;
    saveNow(app);
}

// Its look revealed (D54): "It's a Pebbleback Tide!", "It's a Cinderveined Ember!", "It's a
// Glimmertide!", "It's an Aurora..."
void announce(App& app, const Dragon& d) {
    char kind[40];
    kindName(d, kind, sizeof(kind));
    const bool vowel = std::strchr("AEIOUaeiou", kind[0]) != nullptr;
    showToastf(app, vowel ? str::kItsAn : str::kItsA, kind);
}

// The hatching, a few seconds long: the egg shakes harder and harder, stills glowing, and
// bursts in a warm flash; the hatchling takes shape out of a white blob in the nest, blinks at
// its first light and cries; then the keyboard names it. Skippable (A, B or a tap) once seen.
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
    const Vec2 nest = den.eggNests[h.nest];
    if (!h.popped) {
        EggMotion& egg = app.eggs[h.nest];
        egg.glowBoost = std::fmin(1.0f, h.t / kStillAt);  // the cracks and the light inside, brighter
        if (h.t < kStillAt && (h.nextKnock -= app.dt) <= 0) {  // shaking harder and harder
            const float k = std::fmin(1.0f, h.t / kStillAt);
            h.nextKnock = 0.5f - 0.3f * k;
            egg.knock(0.06f + 0.2f * k, app.rng.below(628) * 0.01f);
            audio::playSfx(k > 0.6f && app.rng.chance(1, 2) ? audio::Sfx::EggCrack : audio::Sfx::EggKnock,
                           0.9f + 0.3f * k);
            app.fx.emit(Fx::Puff, {nest.x, nest.y, 0.1f}, 1, 0.5f);
        }
        if (h.t >= kBurstAt || skip) pop(app, d, now);  // after a held breath, still and glowing
        if (!skip) return;
    }
    DenActor& a = app.actors[d.denSlot < kDenDragons ? d.denSlot : 0];
    if (skip && h.t < kNameAt) {  // straight to the naming: the hatchling whole, the pieces landed
        h.t = kNameAt;
        h.flash = 0;
        h.burst.settleNow();
    }
    h.flash = std::fmax(0.0f, h.flash - app.dt / kFlashTime);
    const BlobShape blob = blobAt(h.t - kBurstAt);
    a.morph = blob.morph;
    a.blobScale = blob.scale;
    if (h.t < kShapedAt && (h.sparkIn -= app.dt) <= 0) {  // sparkles as it takes shape
        h.sparkIn = 0.07f;
        const float ang = app.rng.below(628) * 0.01f, r = 0.25f + 0.25f * blob.scale;
        app.fx.emit(Fx::Sparkle, {nest.x + r * std::cos(ang), nest.y + r * std::sin(ang), 0.25f + 0.4f * blob.scale}, 1,
                    0.5f);
    }
    if (!h.blinked && h.t >= kBlinkAt) {
        h.blinked = true;
        a.eyes.blink = 0;  // its first blink
        audio::playSfx(audio::Sfx::HatchCry, voicePitch(d, now));  // its very first cry
        announce(app, d);
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
        a.morph = a.blobScale = 1;
        h.burst.vanish();  // the pieces sink into the straw
        a.behavior.care(Care::Greet, d);  // out of the nest to say hello
        app.game.settings.seenHatch = 1;
        showToastf(app, str::kSayHello, d.name);
        if (h.dex.completed >= 0) {  // then the Dragondex's news, the biggest
            u8 a, b;
            breedAlleles(h.dex.completed, a, b);
            queueToastf(app, str::kDexComplete, breedName(static_cast<Element>(a), static_cast<Element>(b)));
        } else if (h.dex.newRare) {
            queueToastf(app, str::kDexRare, rareName(d.genome.rareFlags));
        } else if (h.dex.newEntry) {
            char kind[40];
            kindName(d, kind, sizeof(kind));
            queueToastf(app, str::kDexNew, kind);
        }
        saveNow(app);
    }
}

// Particles move on; the room adds its own life: embers over the hearth, motes in the
// sunbeam, glints on the hoard.
int denShown(App& app, const int* order, int count, r3d::DenDragon* shown);

void denEffects(App& app, s64 now) {
    if (r3d::roomReady()) {
        const DayBlend light = dayBlend(now);
        const float daylight = light.weight(kLightDay) + 0.4f * light.weight(kLightEvening);
        app.ambience.update(app.fx, DenLayout{}, daylight, app.dt);
    }
    // Starspeckle (a rare trait): tiny specks twinkling across its back.
    const DenRoster r = denRoster(app.game);
    int order[r3d::kDenShown];
    r3d::DenDragon shown[r3d::kDenShown];
    const int n = denShown(app, order, denOrder(app, r, order), shown);
    for (int i = 0; i < n; ++i) {
        const Dragon& d = *shown[i].dragon;
        Vec3 chest, hips;
        if (d.stage == Stage::Egg || !(d.genome.rareFlags & kRareStarspeckle) || !app.rng.chance(1, 9) ||
            !r3d::backOf(i, chest, hips))
            continue;
        const float t = app.rng.below(1000) / 1000.0f;
        const Vec3 at = lerp(chest, hips, t);
        const float side = (app.rng.below(1000) / 1000.0f - 0.5f) * 0.5f * bodyScale(d, now);
        app.fx.emit(Fx::Glint, {at.x + side, at.y - side * 0.5f, at.z}, 1, 0.35f);
    }
    app.fx.update(app.dt);
}

// What's been bought for the den (WP7): the toys where they lie, the bowl's food, the decor.
// A toy in a dragon's mouth rides there (the rope crosswise, or out toward you in a tug); one
// let go of lies where it fell, and stays there.
void denThings(App& app, const DenRoster& r, s64 now) {
    static r3d::DenThings t;
    static float yaw[kToys] = {0.7f, -0.5f, 0.0f, 0.3f};
    SaveData& s = app.game;
    int order[r3d::kDenShown];
    const int shown = denOrder(app, r, order);
    t.ropeSpan = false;
    for (int bed = 0; bed < kDenDragons; ++bed) {
        if (r.dragon[bed] < 0 || r.away[bed] || app.actorId[bed] != s.dragons[r.dragon[bed]].id) continue;
        DenBehavior& b = app.actors[bed].behavior;
        const Vec2 fwd{std::sin(b.heading), -std::cos(b.heading)}, side{std::cos(b.heading), std::sin(b.heading)};
        if (b.dropToy >= 0) {
            const float ahead = 0.7f * b.size;
            setToyAt(s, b.dropToy, {b.pos.x + fwd.x * ahead, b.pos.y + fwd.y * ahead});
            yaw[b.dropToy] = b.heading + 1.5708f;
            b.dropToy = -1;
        }
        Vec3 mouth;
        if (b.carrying == 1 && r3d::mouthOf(drawIndexOf(order, shown, r.dragon[bed]), mouth)) {
            t.ropeSpan = true;
            if (b.activity == Activity::Tug) {  // out toward you
                t.ropeA = mouth;
                t.ropeB = {mouth.x + fwd.x * 0.9f, mouth.y + fwd.y * 0.9f, std::fmax(0.1f, mouth.z - 0.35f)};
            } else {
                t.ropeA = {mouth.x - side.x * 0.42f, mouth.y - side.y * 0.42f, mouth.z - 0.04f};
                t.ropeB = {mouth.x + side.x * 0.42f, mouth.y + side.y * 0.42f, mouth.z - 0.04f};
            }
        }
    }
    for (int k = 0; k < kToys; ++k) {
        t.toy[k] = owns(s, static_cast<Item>(k));
        const Vec2 at = toyAt(s, k);
        t.toyAt[k] = {at.x, at.y, k == 2 ? kOrbRadius : 0.0f};
        t.toyYaw[k] = yaw[k];
    }
    if (app.denOrb.active) {  // the orb rolls about with its own physics (nudged by the dragons)
        Ball& orb = app.denOrb;
        const DenLayout den;
        if (orb.step(den, app.dt) == BallEvent::Bounce && orb.lastImpact > 1.0f)
            audio::playSfx(audio::Sfx::Bounce, 1.2f, 0.5f);
        const Vec2 saved = toyAt(s, 2);
        if (orb.resting && std::hypot(orb.pos.x - saved.x, orb.pos.y - saved.y) > 0.05f) setToyAt(s, 2, {orb.pos.x, orb.pos.y});
        const float speed = std::hypot(orb.vel.x, orb.vel.y);
        if (speed > 1e-3f && !orb.resting)
            t.orbSpin = normalize(mul(quatAxisAngle(normalize(Vec3{-orb.vel.y, orb.vel.x, 0}), speed * app.dt / orb.radius), t.orbSpin));
        t.toyAt[2] = orb.pos;
    }
    // A tug-of-war: the rope between the two mouths.
    int tuggers[2], nt = 0;
    for (int bed = 0; bed < kDenDragons && nt < 2; ++bed)
        if (r.dragon[bed] >= 0 && !r.away[bed] && app.actorId[bed] == s.dragons[r.dragon[bed]].id &&
            app.actors[bed].behavior.activity == Activity::TugWar && app.actors[bed].behavior.step == 2)
            tuggers[nt++] = bed;
    Vec3 ma, mb;
    if (nt == 2 && r3d::mouthOf(drawIndexOf(order, shown, r.dragon[tuggers[0]]), ma) &&
        r3d::mouthOf(drawIndexOf(order, shown, r.dragon[tuggers[1]]), mb)) {
        t.ropeSpan = true;
        t.ropeA = ma;
        t.ropeB = mb;
    }
    t.bowlFood = bowlFood(s);
    for (int p = 0; p < kDecorSpots; ++p) t.decor[p] = decorAt(s, p);
    if (const int breed = bannerBreed(s); breed >= 0) {  // a completed breed's banner (the Dragondex)
        Rgb base, accent, glow;
        breedColours(breed, base, accent, glow);
        t.breedBanner = true;
        t.breedLook = breedBannerLook(base, accent, glow);
    }
    const DayBlend light = dayBlend(now);
    t.daylight = light.weight(kLightDay) + 0.4f * light.weight(kLightEvening);
    r3d::setDenThings(&t);
}

void update(App& app, const Input& in) {
    if (photo::active(app)) {  // the den holds still for the picture
        photo::update(app, in);
        return;
    }
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
    denThings(app, r, nowLocal(app));
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
    if (app.hatch.burst.active) {  // the pieces fly, land, lie in the nest, then sink away
        app.hatch.burst.update(app.dt);
        if (!app.hatch.burst.active) r3d::setBurst(nullptr, nullptr);
    }
}

// While its profile is open (WP8): the dragon posing, turning slowly in its heartglow's light.
void drawProfileTop(App& app, const Dragon& d, s64 now) {
    const Rgb glow = heartglowColor(static_cast<Element>(d.genome.elementA));
    verticalGradient(0, 0, kTopW, kScreenH, theme::rgba(40, 28, 52), theme::kDenPlum);
    C2D_DrawEllipseSolid(80, 150, 0, 240, 70, withAlpha(fromRgb(glow), 0.18f));
    C2D_DrawEllipseSolid(130, 196, 0, 140, 22, withAlpha(theme::rgba(0, 0, 0), 0.25f));
    if (r3d::ready()) r3d::drawShowcase(app, d, nullptr, now, 0.6f * std::sin(app.t * 0.35f));
    char line[64], kind[40];
    kindName(d, kind, sizeof(kind));
    std::snprintf(line, sizeof(line), "%s  -  %s %s", d.name, kind, stageName(d.stage));
    textCentered(app, line, 200, 18, 0.6f, theme::kClutchGold, 380, Face::Title);
}

// The den's dragons and eggs as drawDen takes them, in `order` (denOrder).
int denShown(App& app, const int* order, int count, r3d::DenDragon* shown) {
    int n = 0;
    for (int k = 0; k < count; ++k) {
        const Dragon& o = app.game.dragons[order[k]];
        if (o.stage == Stage::Egg) {
            if (!r3d::eggReady()) continue;
            shown[n++] = {&o, nullptr, &app.eggs[o.denSlot], static_cast<s8>(o.denSlot)};
        } else {
            const int bed = o.denSlot;
            shown[n++] = {&o, app.actorId[bed] == o.id ? &app.actors[bed] : nullptr};
        }
    }
    return n;
}

bool profileShown(App& app) {
    return app.care.profileOpen && activeDragon(app).stage != Stage::Egg && !app.hatch.active;
}

// Before the frame: the den's dragons posed while the GPU finishes the last one (WP11d).
void prepare(App& app) {
    if (!r3d::ready() || profileShown(app)) return;
    if (r3d::roomReady()) app.topClear = r3d::backdrop(nowLocal(app));
    const DenRoster r = denRoster(app.game);
    int order[r3d::kDenShown];
    const int count = denOrder(app, r, order);
    r3d::DenDragon shown[r3d::kDenShown];
    r3d::poseAhead(app, shown, denShown(app, order, count, shown), nowLocal(app));
}

void drawTop(App& app) {
    const Dragon& d = activeDragon(app);
    const s64 now = nowLocal(app);
    if (profileShown(app)) {
        drawProfileTop(app, d, now);
        return;
    }
    // With the room, the dark beyond its cutaway is the screen's clear colour (prepare), not a
    // full-screen quad over it (WP11d: a whole screen's fill saved).
    const bool room = r3d::ready() && r3d::roomReady();
    if (!room) {
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
        const int n = denShown(app, order, count, shown);
        r3d::setDenClose(photo::active(app) && app.photo.close);
        r3d::drawDen(app, shown, n, now, &app.fx);
        // With company in the den, a little heart floats over the one you're caring for.
        Vec3 head;
        float hx, hy, ppu;
        if (count > 1 && !photo::active(app) && r3d::headOf(0, head) && r3d::project({head.x, head.y, head.z + 0.35f}, hx, hy, ppu)) {
            const float bob = 2.0f * std::sin(app.t * 3.0f);
            heart(hx, hy - 6 + bob, 9, withAlpha(theme::kClutchGold, 0.9f));
        }
    }

    if (photo::active(app)) {  // no names or hints: the frame, when it's the picture
        photo::drawTop(app, d, now);
        return;
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
        else {
            char kind[40];
            kindName(d, kind, sizeof(kind));
            std::snprintf(line, sizeof(line), "%s  -  %s %s %s", d.name, sexName(d.sex), kind, stageName(d.stage));
        }
        text(app, line, 200, 8, 0.6f, theme::kShell);
        std::snprintf(line, sizeof(line), "%s %d  -  %s  -  %s%s%s", str::kDay, daysSinceHatch(d, now) + 1,
                      moodName(moodOf(d)), personalityName(d.personality), d.napping ? "  -  " : "",
                      d.napping ? str::kNapping : "");
        text(app, line, 200, 26, 0.45f, theme::kClutchGold);
    }
    if (!app.hatch.active)  // the map, and how to switch
        text(app, count > 1 ? str::kSwitchHint : str::kMapHint, 392, 226, 0.4f, withAlpha(theme::kShell, 0.6f),
             C2D_AlignRight);
    if (app.hatch.flash > 0)  // the burst's warm flash
        C2D_DrawRectSolid(0, 0, 0, kTopW, kScreenH, withAlpha(theme::rgba(255, 214, 150), 0.85f * app.hatch.flash));
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
    if (photo::active(app)) {
        photo::drawBottom(app, in);
        return;
    }
    Dragon& d = activeDragon(app);
    const s64 now = nowLocal(app);
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    const int nest = careNest(app);
    // Pet the dragon itself, or rub the egg. The hatching: the whole hatchling taking shape
    // among its shell's pieces, then its face for the first blink.
    const r3d::CloseUpView view = !app.hatch.active          ? care::view(app)
                                  : app.hatch.t < kBlinkAt - 0.25f ? r3d::CloseUpView::Body
                                                                   : r3d::CloseUpView::Face;
    if (r3d::ready()) r3d::drawCloseUp(app, d, careActor(app), nest >= 0 ? &app.eggs[nest] : nullptr, now, view);
    if (app.hatch.flash > 0)
        C2D_DrawRectSolid(0, 0, 0, kBotW, kScreenH, withAlpha(theme::rgba(255, 214, 150), 0.7f * app.hatch.flash));
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

const SceneFns kDenScene{update, drawTop, drawBottom, prepare};

}  // namespace ec
