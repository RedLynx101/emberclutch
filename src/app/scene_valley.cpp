// Skyreach Valley (Beta, D73-D86): the open world. You walk it with your travel partner at your
// side (on its lead while it's small, D81), ride a grown partner into the air (the arcade
// flight of WP1), or look about with a free camera. Places are found by coming near (a stinger,
// a toast, the Journal); at each one's door A goes in (the den, the Market, the Nesting Stone,
// the Sanctuary, the Cold Vault, the trailhead's Wanderings), and leaving puts you back outside;
// A at a festival lantern has your partner breathe it alight; the Lantern Festival's quests move
// on with the world. The bottom screen is the painted map: where you are, the places found
// (tap one to travel there), what A does here, and the buttons.
#include <cmath>
#include <cstdio>
#include <vector>

#include "app/audio.hpp"
#include "app/dialogue.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/campaign.hpp"
#include "core/clock.hpp"
#include "core/daylight.hpp"
#include "core/dragondex.hpp"
#include "core/finds.hpp"
#include "core/flight.hpp"
#include "core/genetics.hpp"
#include "core/kinds.hpp"
#include "core/market.hpp"
#include "core/model.hpp"
#include "core/people.hpp"
#include "core/place_layout.hpp"
#include "core/rig.hpp"
#include "core/valley.hpp"
#include "core/villagers.hpp"
#include "core/walker.hpp"
#include "core/wanderings.hpp"
#include "core/world.hpp"

namespace ec {
namespace {

enum class Mode : u8 { OnFoot, Riding, FreeCam };

// What A does where you stand (checked in this order; Board: the challenges' picker there).
enum class Action : u8 { None, Talk, Shop, Enter, Light, Ride, Call, Board };

struct ValleyScene {
    Valley valley;
    bool loaded = false, tried = false;
    Mode mode = Mode::OnFoot;
    Mode before = Mode::OnFoot;  // (the free camera goes back to it)
    // Riding: the grown partner flown (the arcade flight) and the chase camera.
    Flight flight;
    ChaseCamera cam;
    FlightInput last;
    // On foot: you, your partner, the camera behind you, the walls.
    Walker you;
    Follower pal;
    WalkCamera wcam;
    std::vector<Solid> solids;
    std::vector<CameraWall> camWalls;
    // The partner as drawn and animated.
    int partner = -1;  // its index in the save (-1: out alone)
    Dragon shown;
    DenActor flyer;
    ClipId clip = ClipId::Count;
    bool speedsSet = false;
    float natWalk = 2.2f, natTrot = 4.0f, natRun = 6.0f;
    float skimFor = 0, swimT = 0;
    float tailLastHeading = 0, tailTurn = 0, tailYawV = 0, tailPitchV = 0;  // the wind on its tail (windOnTail)
    // The free camera.
    Vec3 freeEye;
    float freeYaw = 0, freePitch = -0.35f;
    // Here and now.
    Action action = Action::None;
    int actionPlace = -1;
    Villager actionWho = Villager::Keeper;
    u8 actionTab = 0;  // Shop: the Market's page it opens (the egg stand's, the goods stall's)
    float breathT = -1;       // a lantern being lit: seconds in (< 0: none)
    int breathPlace = -1;
    float stepFor = 0;        // your next footstep
    float foundCheck = 0;     // seconds to the next look round for places
    float keepFor = 0;        // seconds to keeping where you are (keepPlace)
    // The people as drawn: your clip, the villagers' (turning to you, waving hello), blinks.
    struct Figure {
        Animator anim;
        float heading = 0;
        float blinkIn = 2, blink = 0;
        bool waved = false;
    };
    Figure youFig;
    Figure folk[kVillagers];
    // A dragon out on the Wanderings (D69): its place on the loop, its clip.
    DenActor wanderActor;
    ClipId wanderClip = ClipId::Count;
    u32 wanderId = 0;
    Vec3 wanderAt;
    float wanderHeading = 0, wanderT = 0;
    // The star dragon (the Lantern Festival): seen high over the floating isles once your dragon
    // has its wings, over the arena on the festival's night, over the lake on nights after.
    Dragon star;
    DenActor starActor;
    bool starShown = false, starSet = false;
    Vec3 starAt;
    float starHeading = 0, starT = 0;
};

ValleyScene& vs() {
    static ValleyScene s;
    return s;
}

bool loadValleyFile(Valley& v) {
    FILE* f = std::fopen("romfs:/valley/skyreach.evl", "rb");
    if (!f) return false;
    std::vector<u8> data;
    u8 buf[4096];
    std::size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) data.insert(data.end(), buf, buf + n);
    std::fclose(f);
    return loadValley(data.data(), data.size(), v);
}

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

bool grownPartner(const ValleyScene& s) { return s.partner >= 0 && s.shown.stage == Stage::Adult; }

// Leaving: where you are is saved (Continue comes back here), the valley's memory freed.
void keepPlace(App& app) {
    ValleyScene& s = vs();
    WorldState& w = app.game.world;
    const Vec3 at = s.mode == Mode::Riding ? s.flight.pos : s.you.pos;
    w.x = at.x;
    w.y = at.y;
    w.heading = s.mode == Mode::Riding ? s.flight.heading : s.you.heading;
}

void leaveTo(App& app, SceneId scene) {
    keepPlace(app);
    r3d::releaseValley();
    app.game.world.inValley = 0;
    app.fromValley = scene != SceneId::Den;
    app.scene = scene;
    app.storePick = app.storePage = 0;
    saveNow(app);
}

// The scene a place's door opens (Count: none of its own yet).
SceneId sceneOf(int place) {
    switch (place) {
        case kPlaceDen: return SceneId::Den;
        case kPlaceMarket: return SceneId::Market;
        case kPlaceStone: return SceneId::NestingStone;
        case kPlaceSanctuary: return SceneId::Sanctuary;
        case kPlaceVault: return SceneId::Vault;
        case kPlaceTrailhead: return SceneId::Wanderings;
        default: return SceneId::Count;
    }
}

// The sky by the time of day: its top, the horizon (the fog), and the light on the land.
struct Sky {
    Rgb top, horizon, tint;
};
Sky skyFor(s64 now) {
    const DayBlend b = dayBlend(now);
    const Sky day{{96, 160, 226}, {214, 232, 244}, {255, 255, 255}};
    const Sky evening{{84, 70, 128}, {244, 170, 120}, {255, 206, 176}};
    const Sky night{{12, 16, 40}, {40, 52, 86}, {110, 124, 176}};
    const float wd = b.weight(kLightDay), we = b.weight(kLightEvening), wn = 1.0f - wd - we;
    auto mix = [&](Rgb Sky::*c) {
        const Rgb a = day.*c, e = evening.*c, n = night.*c;
        return Rgb{static_cast<u8>(a.r * wd + e.r * we + n.r * wn), static_cast<u8>(a.g * wd + e.g * we + n.g * wn),
                   static_cast<u8>(a.b * wd + e.b * we + n.b * wn)};
    };
    return {mix(&Sky::top), mix(&Sky::horizon), mix(&Sky::tint)};
}

// The partner's gaits, measured on its own legs once its body is loaded (no skating).
void measureSpeeds(ValleyScene& s) {
    if (s.speedsSet || s.partner < 0) return;
    const AnimLibrary* lib = r3d::animsFor(s.shown);
    if (!lib) return;
    const int form = s.shown.stage == Stage::Hatchling ? kFormHatchling : kFormGrown;
    const int look = r3d::lookFor(s.shown);
    const ModelData* m = r3d::model(form, look);
    const AnimBinding* bind = r3d::binding(form, look);
    if (!m || !bind) return;
    const int build = s.shown.genome.build < kModelBuilds ? s.shown.genome.build : kBuildNeutral;
    const float size = kindSize(s.shown) * (s.shown.stage == Stage::Adult ? 1.0f : 0.6f);
    s.flyer.updateSpeeds(*m, *bind, *lib, r3d::clipIndexFor(s.shown, form), form * r3d::kLookSlots + look, 1.0f, build,
                         size, form == kFormHatchling);
    s.natWalk = clampf(s.flyer.behavior.walkSpeed, 0.6f, 4.0f);
    s.natRun = clampf(s.flyer.behavior.runSpeed, s.natWalk * 2.0f, 14.0f);
    s.natTrot = s.flyer.behavior.trotSpeed > s.natWalk && s.flyer.behavior.trotSpeed < s.natRun
                    ? s.flyer.behavior.trotSpeed
                    : (s.natWalk + s.natRun) * 0.5f;
    s.flight.walkSpeed = s.natWalk;
    s.flight.runSpeed = clampf(3.0f * s.natRun, s.natWalk * 6.0f, 30.0f);  // run 15 (D81)
    s.pal.walk = s.natWalk;
    s.pal.trot = s.natTrot;
    s.pal.run = std::fmax(s.natRun * 1.4f, 8.5f);  // it keeps up with you running
    s.speedsSet = true;
}

// The partner's clip for how it's moving (flying, swimming, walking, trotting, running).
// The tail in the wind (D84): in flight it swings into a turn (right, it swings right), trails
// straighter and higher in a dive and droops a little climbing, on a spring so it lags and
// overshoots a touch like a real tail; on the ground it settles back to the clip's own sway.
void windOnTail(App& app, ValleyScene& s, bool flying, bool diving) {
    const float dt = std::fmax(app.dt, 1e-4f);
    float turn = std::remainder(s.flight.heading - s.tailLastHeading, 6.2831853f) / dt;  // + left (CCW)
    s.tailLastHeading = s.flight.heading;
    if (!flying) turn = 0;
    s.tailTurn += (turn - s.tailTurn) * std::fmin(1.0f, dt * 6.0f);
    const float nose = flying ? s.flight.pitch : 0.0f;  // + nose down (a dive: the tail streams up in line), - climbing (it droops)
    const float wantYaw = flying ? clampf(-s.tailTurn * 0.55f, -0.95f, 0.95f) : 0.0f;
    const float wantPitch = flying ? clampf(nose * 0.5f, -0.35f, 0.45f) : 0.0f;
    const float wantStraight = !flying ? 0.0f : diving ? 0.85f : clampf(0.3f + 0.4f * std::fabs(s.tailTurn), 0.3f, 0.6f);
    // A spring, a little under-damped.
    constexpr float kStiff = 38.0f, kDamp = 7.5f;
    s.tailYawV += ((wantYaw - s.flyer.tailYaw) * kStiff - s.tailYawV * kDamp) * dt;
    s.tailPitchV += ((wantPitch - s.flyer.tailPitch) * kStiff - s.tailPitchV * kDamp) * dt;
    s.flyer.tailYaw += s.tailYawV * dt;
    s.flyer.tailPitch += s.tailPitchV * dt;
    s.flyer.tailStraight += (wantStraight - s.flyer.tailStraight) * std::fmin(1.0f, dt * 3.0f);
}

// The wanderer out on its loop (D69): walking round its spot as it goes, or (grown) flying
// wide circles over it; the trip moves on as you walk with the 3DS closed.
void animateWanderer(App& app, ValleyScene& s) {
    const int w = wandererIndex(app.game);
    if (w < 0) return;
    const Dragon& d = app.game.dragons[w];
    const WanderSpot spot = wanderSpot(stepsSince(d, stepCount(app)));
    const bool flies = d.stage == Stage::Adult;
    if (s.wanderId != d.id) {
        s.wanderId = d.id;
        s.wanderActor = DenActor{};
        s.wanderClip = ClipId::Count;
        s.wanderT = 0;
    }
    s.wanderT += app.dt;
    const float r = flies ? 26.0f : 7.0f, speed = flies ? 11.0f : 1.6f;
    const float a = s.wanderT * speed / r;
    const float x = spot.at.x + std::cos(a) * r, y = spot.at.y + std::sin(a) * r;
    const float ground = std::fmax(s.valley.heightAt(x, y), s.valley.water);
    s.wanderAt = {x, y, ground + (flies ? 28.0f + 4.0f * std::sin(s.wanderT * 0.4f) : 0.0f)};
    s.wanderHeading = std::atan2(-std::sin(a), -std::cos(a));  // along the circle (counter-clockwise)
    const AnimLibrary* lib = r3d::animsFor(d);
    if (!lib) return;
    const int form = d.stage == Stage::Hatchling ? kFormHatchling : kFormGrown;
    const int* clips = r3d::clipIndexFor(d, form);
    const ClipId want = flies ? ClipId::FlyGlide : ClipId::Walk;
    if (s.wanderClip != want && clips[static_cast<int>(want)] >= 0) {
        s.wanderActor.anim.play(clips[static_cast<int>(want)], 0.3f, true);
        s.wanderClip = want;
    }
    s.wanderActor.anim.update(*lib, app.dt, nullptr, 0);
    s.wanderActor.eyes.update(0.0f, app.dt);
}

void animateStar(App& app, ValleyScene& s) {
    s.starShown = false;
    const int kind = findKind("glimmermoth");
    if (kind < 0) return;
    const SaveData& g = app.game;
    const campaign::QuestView wings = campaign::view(g, 5), festival = campaign::view(g, 7);
    const DayBlend day = dayBlend(nowLocal(app));
    int over = -1;
    if (festival.done) {
        if (day.weight(kLightDay) < 0.35f) over = kPlaceLake;  // on nights after, now and then about
    } else if (festival.started && festival.stepIndex >= 1) {
        over = kPlaceArena;  // every lantern lit: it waits over the arena for the festival
    } else if (wings.done || (wings.started && wings.stepIndex >= 2)) {
        over = kPlaceIsles;  // the first sighting, over the floating isles
    }
    r3d::wantKind(over >= 0 ? kind : -1);
    if (over < 0 || !r3d::kindReady(kind)) return;
    const ValleyPlaceInfo* p = s.valley.place(static_cast<u8>(over));
    if (!p) return;
    if (!s.starSet) {  // grown, in its rare starlit colouring
        s.star = Dragon{};
        s.star.id = 0xFFFFFFF1u;
        s.star.stage = Stage::Adult;
        s.star.kind = static_cast<u8>(kind);
        s.star.variant = kindInfo(kind).rareVariant;
        s.starActor = DenActor{};
        s.starSet = true;
    }
    s.starT += app.dt;
    constexpr float kRadius = 42.0f, kSpeed = 11.0f;
    const float a = s.starT * kSpeed / kRadius;
    const float x = p->at.x + std::cos(a) * kRadius, y = p->at.y + std::sin(a) * kRadius;
    s.starAt = {x, y, p->at.z + 46.0f + 6.0f * std::sin(s.starT * 0.3f)};
    s.starHeading = std::atan2(-std::sin(a), -std::cos(a));
    const AnimLibrary* lib = r3d::animsFor(s.star);
    if (!lib) return;
    const int* clips = r3d::clipIndexFor(s.star, kFormGrown);
    if (s.starActor.anim.clip < 0 && clips[static_cast<int>(ClipId::FlyGlide)] >= 0)
        s.starActor.anim.play(clips[static_cast<int>(ClipId::FlyGlide)], 0.0f, true);
    s.starActor.anim.update(*lib, app.dt, nullptr, 0);
    s.starShown = true;
}

void animatePartner(App& app, ValleyScene& s, bool flying, bool diving, bool swimming, float speed) {
    const AnimLibrary* lib = r3d::animsFor(s.shown);
    if (!lib) return;
    const int form = s.shown.stage == Stage::Hatchling ? kFormHatchling : kFormGrown;
    const int* clips = r3d::clipIndexFor(s.shown, form);
    ClipId want = ClipId::Idle;
    float natural = 0, fastest = 1.6f;
    if (flying) {
        want = diving ? ClipId::FlyDive : s.flight.sinceFlap < 0.8f ? ClipId::FlyFlap : ClipId::FlyGlide;
    } else if (swimming) {
        want = ClipId::Walk;
        natural = s.natWalk * 1.5f;
    } else if (speed > 0.15f) {
        const bool baby = form == kFormHatchling;
        const bool running = speed > s.natTrot * 1.3f, trotting = speed > s.natWalk * 1.3f;
        want = running ? (baby ? ClipId::Scamper : ClipId::Gallop) : trotting ? ClipId::Trot : ClipId::Walk;
        natural = running ? s.natRun : trotting ? s.natTrot : s.natWalk;
        fastest = running ? 2.3f : 1.6f;
    }
    if (want != s.clip) {
        const int index = clips[static_cast<int>(want)];
        if (index >= 0) s.flyer.anim.play(index, 0.3f, want != ClipId::FlyDive && want != ClipId::FlyGlide);
        s.clip = want;
    }
    s.flyer.anim.rate = natural > 0 ? clampf(speed / natural, 0.5f, fastest) : 1.0f;
    u8 events[8];
    const int n = s.flyer.anim.update(*lib, app.dt, events, 8);
    for (int k = 0; k < n; ++k) {
        if (events[k] == kAnimFlap) audio::playSfx(audio::Sfx::Wingbeat, 1.0f, 0.8f);
        if (events[k] == kAnimFootstep && !flying)
            audio::playSfx(swimming ? audio::Sfx::Splash : audio::Sfx::DragonStep,
                           swimming ? 1.3f : 0.94f + 0.12f * (app.rng.below(100) / 100.0f), swimming ? 0.35f : 0.7f);
    }
    s.flyer.eyes.update(0.0f, app.dt);
    windOnTail(app, s, flying, diving);
}

// Where you (or you and your partner, riding) are.
Vec3 hereAt(const ValleyScene& s) { return s.mode == Mode::Riding ? s.flight.pos : s.you.pos; }

// Coming near places finds them (a stinger, a toast, the quests move on).
void lookRound(App& app, ValleyScene& s) {
    if ((s.foundCheck -= app.dt) > 0) return;
    s.foundCheck = 0.4f;
    const Vec3 at = hereAt(s);
    for (const ValleyPlaceInfo& p : s.valley.places) {
        const world::PlaceInfo& info = world::placeInfo(p.id);
        const float d = std::hypot(at.x - p.at.x, at.y - p.at.y);
        const bool reached = info.fromAir ? (s.mode == Mode::Riding && d < info.findRadius * 1.5f) : d < info.findRadius;
        if (reached && world::findPlace(app.game, p.id)) {
            audio::playStinger("place-found");
            showToastf(app, str::kFoundPlace, info.name);
            const campaign::News n = campaign::update(app.game);
            if (n.stepped >= 0 || n.finished >= 0) audio::playSfx(audio::Sfx::QuestPage);
            saveNow(app);
        }
    }
    // A find within reach (WP7): taken, a chime and what it was.
    const bool flying = s.mode == Mode::Riding && !s.flight.grounded;
    const int f = findNear(app.game, s.valley, flying ? s.flight.pos : at, flying);
    if (f >= 0) {
        const FindReward r = takeFind(app.game, f, nowLocal(app), app.rng);
        audio::playSfx(audio::Sfx::FindSparkle);
        if (r.egg >= 0) {
            audio::playStinger("place-found");
            showToast(app, str::kFindEgg);
        } else if (r.trinket >= 0) {
            showToastf(app, str::kFindTrinket, trinketName(static_cast<Trinket>(r.trinket)));
        } else {
            std::snprintf(app.toastText, sizeof(app.toastText), str::kFindGleam, static_cast<unsigned>(r.gleam));
            showToast(app, app.toastText);  // (the toast keeps its text's pointer)
        }
        saveNow(app);
    }
    // The map's fog lifts round you (further seen from the air).
    explore(app.game, s.valley, {at.x, at.y}, flying ? 170.0f : 110.0f);
}

void travelTo(App& app, ValleyScene& s, int place, bool outward);
Vec3 villagerAt(const Valley& v, Villager who);

// A clip by name on a figure (nothing if the library or the clip isn't there).
void playClip(ValleyScene::Figure& f, const char* name, float rate = 1.0f, float fade = 0.2f) {
    const AnimLibrary* lib = r3d::personAnims();
    if (!lib) return;
    const int c = lib->find(name);
    if (c < 0) return;
    f.anim.play(c, fade);
    f.anim.rate = rate;
}

bool playing(const ValleyScene::Figure& f, const char* name) {
    const AnimLibrary* lib = r3d::personAnims();
    return lib && f.anim.clip >= 0 && f.anim.clip == lib->find(name);
}

void blinkFigure(App& app, ValleyScene::Figure& f) {
    f.blinkIn -= app.dt;
    if (f.blinkIn <= 0) {
        f.blink = 1;
        f.blinkIn = 1.5f + app.rng.below(3500) * 0.001f;
    }
    f.blink = std::fmax(0.0f, f.blink - app.dt * 7.0f);
}

// You and the villagers move with what's happening: you walk, jog, run, ride, stand and listen;
// they idle at their places, turn to you as you come near, wave hello once a visit, talk and nod.
void animatePeople(App& app, ValleyScene& s) {
    const AnimLibrary* lib = r3d::personAnims();
    if (!lib) return;
    const Person body = playerBody(app.game.world.look);
    ValleyScene::Figure& me = s.youFig;
    const bool riding = s.mode == Mode::Riding || (s.mode == Mode::FreeCam && s.before == Mode::Riding);
    if (riding) {
        const float roll = s.flight.roll;
        playClip(me, roll > 0.3f ? "ride_lean_left" : roll < -0.3f ? "ride_lean_right" : "ride", 1.0f, 0.3f);
    } else if (s.you.speed < 0.25f || talking(app)) {
        playClip(me, "idle", 1.0f, 0.25f);
    } else if (s.you.speed < 1.3f) {
        playClip(me, "walk", clampf(s.you.speed / personWalkSpeed(body), 0.5f, 2.0f));
    } else {
        playClip(me, "run", clampf(s.you.speed / personRunSpeed(body), 0.8f, 3.0f));
    }
    u8 events[4];
    me.anim.update(*lib, app.dt, events, 4);
    blinkFigure(app, me);
    const bool listening = talking(app);
    for (int k = 0; k < kVillagers; ++k) {
        ValleyScene::Figure& f = s.folk[k];
        const Villager who = static_cast<Villager>(k);
        const Vec3 at = villagerAt(s.valley, who);
        const ValleyPlaceInfo* p = s.valley.place(static_cast<u8>(villagerInfo(who).place));
        const float rest = (p ? p->heading : 0.0f) + villagerInfo(who).facing;
        const float d = std::hypot(s.you.pos.x - at.x, s.you.pos.y - at.y);
        if (d > 90.0f) continue;  // far off: left as they were
        // Turning to you when you're near (and while you talk), else back to their place's way.
        const float want = d < 6.0f ? std::atan2(s.you.pos.x - at.x, -(s.you.pos.y - at.y)) : rest;
        float err = std::remainder(want - f.heading, 6.2831853f);
        f.heading += clampf(err, -3.0f * app.dt, 3.0f * app.dt);
        const bool mine = listening && app.talk.who == who;
        if (mine) {
            playClip(f, "talk", 1.0f, 0.25f);
        } else if (playing(f, "talk")) {
            playClip(f, "nod", 1.0f, 0.2f);
        } else if (!f.waved && d < 7.0f) {
            f.waved = true;
            playClip(f, "wave", 1.0f, 0.2f);
        } else if (f.anim.clip < 0 || ((playing(f, "wave") || playing(f, "nod")) && f.anim.finished(*lib))) {
            playClip(f, "idle", 1.0f, 0.3f);
            f.anim.time = (k * 0.7f);  // not all breathing together
        }
        if (d > 15.0f) f.waved = false;  // a new visit: another hello
        f.anim.update(*lib, app.dt, events, 4);
        blinkFigure(app, f);
    }
}

// Where a villager stands in the valley.
Vec3 villagerAt(const Valley& v, Villager who) {
    const VillagerInfo& info = villagerInfo(who);
    const ValleyPlaceInfo* p = v.place(static_cast<u8>(info.place));
    return p ? placeToWorld3(v, *p, {info.at.x, info.at.y, 0}) : Vec3{};
}

// The Sanctuary's stray, hiding in the meadow's flowers until your partner sniffs her out.
Vec3 strayAt(const Valley& v) {
    const ValleyPlaceInfo* p = v.place(kPlaceSanctuary);
    return p ? placeToWorld3(v, *p, {-46, 58, 0}) : Vec3{};
}

// What A would do here, nearest first: someone to talk to, a door, a lantern, getting on your
// partner, calling it.
void findAction(ValleyScene& s) {
    s.action = Action::None;
    s.actionPlace = -1;
    if (s.mode != Mode::OnFoot) return;
    const Vec3 at = s.you.pos;
    float best = 1e9f;
    for (int k = 0; k < kVillagers; ++k) {
        const Vec3 p = villagerAt(s.valley, static_cast<Villager>(k));
        const float d = std::hypot(at.x - p.x, at.y - p.y);
        if (d < 3.2f && d < best) {
            best = d;
            s.action = Action::Talk;
            s.actionWho = static_cast<Villager>(k);
        }
    }
    if (s.action == Action::Talk) return;
    // The Market's stalls: the egg of the day's stand and the goods stall open their page.
    if (const ValleyPlaceInfo* m = s.valley.place(kPlaceMarket)) {
        const PlaceLayout& L = placeLayout(kPlaceMarket);
        const Vec2 egg = placeToWorld(*m, {L.eggStand.x, L.eggStand.y});
        const Vec2 goods = placeToWorld(*m, {(L.goods[0].x + L.goods[3].x) * 0.5f, (L.goods[0].y + L.goods[3].y) * 0.5f});
        const float de = std::hypot(at.x - egg.x, at.y - egg.y), dg = std::hypot(at.x - goods.x, at.y - goods.y);
        if (std::fmin(de, dg) < 2.6f) {
            s.action = Action::Shop;
            s.actionPlace = kPlaceMarket;
            s.actionTab = de < dg ? 3 : 1;  // scene_market's tabs: the egg, the goods
            return;
        }
    }
    for (const ValleyPlaceInfo& p : s.valley.places) {
        const PlaceLayout& l = placeLayout(p.id);
        if (l.hasDoor && sceneOf(p.id) != SceneId::Count) {
            const Vec2 door = placeToWorld(p, l.door);
            const float d = std::hypot(at.x - door.x, at.y - door.y);
            if (d < 4.5f && d < best) {
                best = d;
                s.action = Action::Enter;
                s.actionPlace = p.id;
            }
        }
        if (l.hasLantern && s.partner >= 0) {
            const Vec2 lan = placeToWorld(p, {l.lantern.x, l.lantern.y});
            const float d = std::hypot(at.x - lan.x, at.y - lan.y);
            if (d < 5.0f && d < best && s.breathT < 0) {
                best = d;
                s.action = Action::Light;
                s.actionPlace = p.id;
            }
        }
    }
    int board = -1;  // the challenges' boards (scene_challenge)
    float boardAt = 0;
    if (challengeBoardNear(s.valley, at, board, boardAt) && boardAt < best) {
        s.action = Action::Board;
        s.actionPlace = board;
    }
    if (s.action == Action::None && grownPartner(s) && std::hypot(at.x - s.pal.pos.x, at.y - s.pal.pos.y) < 4.0f)
        s.action = Action::Ride;
    if (s.action == Action::None && s.partner >= 0 && s.pal.lost()) s.action = Action::Call;
}

// The partner's breath on a lantern: its element's own breath, the lantern alight after a moment.
audio::Sfx breathSound(const Dragon& d) {
    const int e = kindInfo(d.kind < kindCount() ? d.kind : 0).elements[0];
    const char* name = elementName(e);
    if (name[0] == 'E') return audio::Sfx::BreathFlame;          // Ember
    if (name[0] == 'F') return audio::Sfx::BreathFrost;          // Frost
    if (name[0] == 'G' && name[1] == 'a') return audio::Sfx::BreathGust;  // Gale
    if (name[0] == 'G') return audio::Sfx::BreathSpores;         // Grove
    if (name[0] == 'T') return audio::Sfx::BreathMist;           // Tide
    return audio::Sfx::BreathLight;                              // Lumen, Stone, Shade
}

void doAction(App& app, ValleyScene& s) {
    switch (s.action) {
        case Action::Talk:
            startTalk(app, s.actionWho);
            break;
        case Action::Shop:
            audio::playSfx(audio::Sfx::VillageBell);
            app.marketTab = s.actionTab;
            leaveTo(app, SceneId::Market);
            break;
        case Action::Enter:
            audio::playSfx(s.actionPlace == kPlaceMarket ? audio::Sfx::VillageBell : audio::Sfx::DoorWood);
            leaveTo(app, sceneOf(s.actionPlace));
            break;
        case Action::Light:
            if (world::lanternLit(app.game, s.actionPlace)) {
                showToast(app, str::kLanternLit);
                break;
            }
            if (s.actionPlace == kPlaceArena && campaign::view(app.game, campaign::kQuests - 1).stepIndex < 2) {
                showToast(app, str::kLanternFestival);  // the great lantern waits for the festival
                break;
            }
            s.breathT = 0;
            s.breathPlace = s.actionPlace;
            audio::playSfx(breathSound(s.shown));
            break;
        case Action::Ride:
            s.mode = Mode::Riding;
            s.flight = Flight{};
            s.flight.pos = s.pal.pos;
            s.flight.heading = s.pal.heading;
            s.flight.walkSpeed = s.natWalk;
            s.flight.runSpeed = clampf(3.0f * s.natRun, s.natWalk * 6.0f, 30.0f);
            s.cam = ChaseCamera{};
            audio::playSfx(audio::Sfx::Mount);
            if (!(app.game.world.flags & kFlagRode)) {
                app.game.world.flags |= kFlagRode;
                campaign::update(app.game);
            }
            break;
        case Action::Call:
            s.pal.call(s.you, s.valley);
            audio::playSfx(audio::Sfx::Chirp, 1.1f);
            showToastf(app, str::kPartnerCame, s.shown.name);
            break;
        case Action::Board:
            keepPlace(app);
            openChallenges(app, s.actionPlace);
            break;
        case Action::None: break;
    }
}

void getOff(App& app, ValleyScene& s) {
    s.mode = Mode::OnFoot;
    const Vec3 f = s.flight.forward();
    s.you.pos = s.flight.pos + Vec3{-f.y, f.x, 0} * -2.4f;  // down on its left
    s.you.pos.z = s.valley.heightAt(s.you.pos.x, s.you.pos.y);
    s.you.heading = s.flight.heading;
    s.you.speed = 0;
    s.pal.pos = s.flight.pos;
    s.pal.heading = s.flight.heading;
    s.pal.speed = 0;
    s.wcam = WalkCamera{};
    s.wcam.yaw = s.flight.heading;
    audio::playSfx(audio::Sfx::Landing, 1.2f, 0.6f);
    (void)app;
}

void update(App& app, const Input& in) {
    ValleyScene& s = vs();
    app.simAccum += app.dt;
    if (app.simAccum >= 1.0f) {
        tickWorld(app);
        app.simAccum = 0;
    }
    if (in.down & KEY_X) {  // home to the den (from anywhere in the valley)
        audio::playSfx(audio::Sfx::Back);
        leaveTo(app, SceneId::Den);
        return;
    }
    if (!s.loaded) return;
    for (int k = 0; k < kVillagers && s.folk[0].anim.clip < 0 && r3d::personAnims(); ++k) {
        const ValleyPlaceInfo* p = s.valley.place(static_cast<u8>(villagerInfo(static_cast<Villager>(k)).place));
        s.folk[k].heading = (p ? p->heading : 0.0f) + villagerInfo(static_cast<Villager>(k)).facing;
    }
    animatePeople(app, s);
    animateWanderer(app, s);
    animateStar(app, s);
    if (app.autoGoto[2] != 0) {  // an autotest's spot
        app.autoGoto[2] = 0;
        s.mode = Mode::OnFoot;
        s.you.pos = {app.autoGoto[0], app.autoGoto[1], s.valley.heightAt(app.autoGoto[0], app.autoGoto[1])};
        s.you.speed = 0;
        if (s.partner >= 0) s.pal.call(s.you, s.valley);
        s.wcam = WalkCamera{};
        s.wcam.yaw = s.you.heading;
        s.wcam.update(s.you, 0, s.valley, 0.0f, &s.camWalls);
    }
    if (app.autoTravel >= 0) {  // an autotest's trip
        world::findPlace(app.game, app.autoTravel);
        travelTo(app, s, app.autoTravel, false);
        app.autoTravel = -1;
    }
    if (app.autoView[0] == 99.0f && s.starShown) {  // (99: from beside the star dragon, looking at it)
        const float* a = app.autoView;
        if (s.mode != Mode::FreeCam) s.before = s.mode;
        s.mode = Mode::FreeCam;
        s.freeEye = s.starAt + Vec3{a[1], a[2], a[3]};
        const Vec3 d = s.starAt - s.freeEye;
        s.freeYaw = std::atan2(d.x, -d.y);
        s.freePitch = clampf(std::atan2(d.z, std::hypot(d.x, d.y)), -1.2f, 0.4f);
        app.autoView[0] = -1;
    }
    if (app.autoView[0] >= 0 && app.autoView[0] != 99.0f) {  // an autotest's view: the free camera at a place, looking at a point
        if (const ValleyPlaceInfo* p = s.valley.place(static_cast<u8>(app.autoView[0]))) {
            const float* a = app.autoView;
            const Vec3 eye = placeToWorld3(s.valley, *p, {a[1], a[2], 0}), at = placeToWorld3(s.valley, *p, {a[4], a[5], 0});
            if (s.mode != Mode::FreeCam) s.before = s.mode;
            s.mode = Mode::FreeCam;
            s.freeEye = {eye.x, eye.y, p->at.z + a[3]};
            const Vec3 d = Vec3{at.x, at.y, p->at.z + a[6]} - s.freeEye;
            s.freeYaw = std::atan2(d.x, -d.y);
            s.freePitch = std::atan2(d.z, std::hypot(d.x, d.y));
        }
        app.autoView[0] = -1;
    }
    measureSpeeds(s);
    s.keepFor -= app.dt;
    if (s.keepFor <= 0) {  // where you are, kept for the next save (Continue brings you back)
        keepPlace(app);
        s.keepFor = 3.0f;
    }
    if (talking(app)) {  // listening: the world waits (your partner idles beside you)
        updateTalk(app, in);
        if (!talking(app) && wrenOpensChallenges(app)) {  // Wren's hello, or "ready when you are": the picker
            keepPlace(app);
            openChallenges(app, kPlaceArena);
            return;
        }
        animatePartner(app, s, false, false, false, 0);
        return;
    }
    const float dpadX = (in.held & KEY_DRIGHT ? 1.0f : 0.0f) - (in.held & KEY_DLEFT ? 1.0f : 0.0f);
    const float dpadY = (in.held & KEY_DUP ? 1.0f : 0.0f) - (in.held & KEY_DDOWN ? 1.0f : 0.0f);
    const Valley& va = s.valley;
    bool flying = false, diving = false, swimming = false;
    float partnerSpeed = 0;
    if (s.mode == Mode::FreeCam) {  // look about: the pad moves it, L/R turn it, up/down on the D-pad
        s.freeYaw += ((in.held & KEY_R ? 1.0f : 0.0f) - (in.held & KEY_L ? 1.0f : 0.0f)) * 1.6f * app.dt;
        const Vec3 fwd{std::sin(s.freeYaw), -std::cos(s.freeYaw), 0}, right{-fwd.y, fwd.x, 0};
        const float speed = (in.held & KEY_B ? 60.0f : 22.0f) * app.dt;
        s.freeEye = s.freeEye + fwd * (in.padY * speed) + right * (in.padX * -speed) + Vec3{0, 0, dpadY * speed};
        s.freePitch = clampf(s.freePitch + dpadX * 0.8f * app.dt, -1.2f, 0.4f);
        s.freeEye.z = std::fmax(s.freeEye.z, va.heightAt(s.freeEye.x, s.freeEye.y) + 0.6f);
        if (in.down & (KEY_A | KEY_Y)) s.mode = s.before;
        animatePartner(app, s, s.before == Mode::Riding && !s.flight.grounded, false, false, 0);
        return;
    }
    if (s.mode == Mode::OnFoot) {
        WalkInput wi;
        wi.x = clampf(in.padX + dpadX, -1, 1);
        wi.y = clampf(in.padY + dpadY, -1, 1);
        wi.run = in.held & KEY_B;
        s.you.update(wi, s.wcam.yaw, va, s.solids, app.dt);
        s.wcam.update(s.you, (in.held & KEY_R ? 1.0f : 0.0f) - (in.held & KEY_L ? 1.0f : 0.0f), va, app.dt, &s.camWalls);
        if (s.partner >= 0) {
            s.pal.update(s.you, va, s.solids, app.dt);
            partnerSpeed = s.pal.speed;
            swimming = va.heightAt(s.pal.pos.x, s.pal.pos.y) < va.water - 0.4f;
        }
        // Your footsteps, by what's underfoot.
        if (s.you.speed > 0.4f && (s.stepFor -= app.dt * s.you.speed) <= 0) {
            s.stepFor = 1.1f;
            const float d = std::hypot(s.you.pos.x - va.place(kPlaceMarket)->at.x, s.you.pos.y - va.place(kPlaceMarket)->at.y);
            audio::playSfx(d < 38.0f ? audio::Sfx::StepStone : audio::Sfx::StepGrass, 0.95f + 0.1f * (app.rng.below(100) / 100.0f),
                           0.6f);
        }
        findAction(s);
        if (in.down & KEY_A) doAction(app, s);
        if (app.scene != SceneId::Valley) return;
    } else {  // riding
        FlightInput fi;
        fi.steer = clampf(in.padX + dpadX, -1, 1);
        fi.pitch = clampf(in.padY + dpadY, -1, 1);
        fi.bank = (in.held & KEY_R ? 1.0f : 0.0f) - (in.held & KEY_L ? 1.0f : 0.0f);
        fi.flap = in.held & KEY_A;
        fi.dive = in.held & KEY_B;
        const bool wasDiving = s.flight.diving(s.last);
        const bool wasGrounded = s.flight.grounded;
        s.last = fi;
        s.flight.update(fi, va, app.dt);
        s.cam.update(s.flight, va, app.dt);
        if (s.flight.tookOff) audio::playSfx(audio::Sfx::Takeoff);
        if (s.flight.landed) audio::playSfx(audio::Sfx::Landing);
        if (s.flight.splashed) audio::playSfx(audio::Sfx::SplashBig);
        if (s.flight.diving(fi) && !wasDiving) audio::playSfx(audio::Sfx::DiveWhoosh);
        if (wasGrounded && !s.flight.grounded && !s.flight.tookOff && !(app.game.world.flags & kFlagGlided)) {
            app.game.world.flags |= kFlagGlided;  // off an edge into a glide (the cold heights' quest)
            campaign::update(app.game);
        }
        s.skimFor -= app.dt;
        if (s.flight.skimming && s.skimFor <= 0) {
            audio::playSfx(audio::Sfx::WaterSkim, 0.95f + 0.1f * (s.flight.speed / 30.0f));
            s.skimFor = 0.7f;
        }
        flying = !s.flight.grounded;
        diving = s.flight.diving(fi);
        swimming = s.flight.swimming;
        partnerSpeed = s.flight.speed;
        if (s.flight.grounded && (in.down & KEY_DDOWN) && s.flight.speed < 3.0f) getOff(app, s);  // down: get off
    }
    s.swimT = swimming ? s.swimT + app.dt : 0.0f;
    animatePartner(app, s, flying, diving, swimming, partnerSpeed);
    // A lantern being lit: the breath, then the flame.
    if (s.breathT >= 0 && (s.breathT += app.dt) > 1.2f) {
        if (world::lightLantern(app.game, s.breathPlace)) {
            audio::playSfx(audio::Sfx::LanternLight);
            showToastf(app, str::kLanternLitAt, world::placeInfo(s.breathPlace).name);
            const campaign::News n = campaign::update(app.game);
            if (n.finished >= 0) {
                audio::playStinger("quest-done");
                showToastf(app, str::kQuestFinished, campaign::view(app.game, n.finished).title);
            } else if (n.stepped >= 0) {
                audio::playSfx(audio::Sfx::QuestPage);
            }
            saveNow(app);
        }
        s.breathT = -1;
    }
    lookRound(app, s);
    // The stray in the meadow (the Sanctuary's quest): your partner near her, it sniffs her out.
    if (s.mode == Mode::OnFoot && s.partner >= 0 && (app.game.world.flags & kFlagMetSanctuary) &&
        !(app.game.world.flags & kFlagFoundStray)) {
        const Vec3 stray = strayAt(va);
        if (std::hypot(s.pal.pos.x - stray.x, s.pal.pos.y - stray.y) < 9.0f) {
            app.game.world.flags |= kFlagFoundStray;
            audio::playSfx(audio::Sfx::Sniff);
            audio::playSfx(audio::Sfx::FindSparkle);
            showToastf(app, str::kFoundStray, s.shown.name);
            campaign::update(app.game);
            saveNow(app);
        }
    }
    // The beds: the meadow by day, the night, the wind high up, the lake, the falls, the village.
    const DayBlend day = dayBlend(nowLocal(app));
    const float night = day.weight(kLightNight) + 0.5f * day.weight(kLightEvening);
    const Vec3 at = hereAt(s);
    const float surface = std::fmax(va.heightAt(at.x, at.y), va.water);
    const float high = s.mode == Mode::Riding && !s.flight.grounded ? std::fmax(0.0f, at.z - surface) : 0.0f;
    const float nearGround = clampf(1.0f - high / 50.0f, 0.0f, 1.0f);
    audio::setBed(audio::Bed::Meadow, (1.0f - night) * nearGround);
    audio::setBed(audio::Bed::ValleyNight, night * nearGround);
    if (flying) {
        audio::setBed(audio::Bed::WindHigh, clampf((high - 8.0f) / 60.0f, 0.0f, 0.8f) + clampf(partnerSpeed / 40.0f, 0.0f, 0.35f));
        if (s.flight.sinceFlap > 0.8f) audio::setBed(audio::Bed::WingFlutter, clampf(partnerSpeed / 18.0f, 0.3f, 1.0f));
    }
    auto near = [&](int place, float radius) {
        const ValleyPlaceInfo* p = va.place(static_cast<u8>(place));
        return p ? clampf(1.0f - std::hypot(at.x - p->at.x, at.y - p->at.y) / radius, 0.0f, 1.0f) : 0.0f;
    };
    audio::setBed(audio::Bed::Waterfall, near(kPlaceGrotto, 120.0f) * nearGround);
    audio::setBed(audio::Bed::Village, near(kPlaceMarket, 110.0f) * nearGround * (1.0f - night * 0.7f));
    int wet = 0;
    for (int k = 0; k < 5; ++k) {
        const float a = k * 1.2566f, r = k ? 25.0f : 0.0f;
        wet += va.heightAt(at.x + r * std::cos(a), at.y + r * std::sin(a)) < va.water;
    }
    audio::setBed(audio::Bed::Lake, (swimming ? 1.0f : wet / 5.0f) * nearGround);
}

void drawTop(App& app) {
    ValleyScene& s = vs();
    const s64 now = nowLocal(app);
    const Sky sky = skyFor(now);
    verticalGradient(0, 0, kTopW, kScreenH, fromRgb(sky.top), fromRgb(sky.horizon));
    if (!s.loaded) {
        textCentered(app, "The valley couldn't be loaded.", 200, 120, 0.6f, theme::kShell, 380);
        return;
    }
    r3d::ValleyView view;
    view.valley = &s.valley;
    view.fog = sky.horizon;
    view.tint = sky.tint;
    view.lanternsLit = app.game.world.lanternsLit;
    for (int i = 0; i < kFindSpots && view.glintCount < r3d::kMaxGlints; ++i) {  // the finds not yet taken, near
        if (findDone(app.game, i)) continue;
        const Vec3 g = findAt(s.valley, i);
        if (std::hypot(g.x - view.eye.x, g.y - view.eye.y) < (findSpot(i).fromAir ? 260.0f : 90.0f) ||
            std::hypot(g.x - s.you.pos.x, g.y - s.you.pos.y) < 90.0f)
            view.glints[view.glintCount++] = g;
    }
    if (const int w = wandererIndex(app.game); w >= 0 && s.wanderId == app.game.dragons[w].id) {
        view.wanderer = &app.game.dragons[w];
        view.wandererActor = &s.wanderActor;
        view.wandererAt = s.wanderAt;
        view.wandererHeading = s.wanderHeading;
    }
    if (s.starShown) {
        view.skyDragon = &s.star;
        view.skyActor = &s.starActor;
        view.skyAt = s.starAt;
        view.skyHeading = s.starHeading;
    }
    {
        r3d::PersonView& me = view.people[view.peopleCount++];
        me.form = static_cast<u8>(playerBody(app.game.world.look));
        me.at = s.you.pos;
        me.heading = s.you.heading;
        me.anim = &s.youFig.anim;
        playerPalette(app.game.world.look, me.pal);
        me.hair = static_cast<s8>(app.game.world.look[kLookHair] < kHairStyles ? app.game.world.look[kLookHair] : 0);
        me.blink = s.youFig.blink;
        me.seated = s.mode == Mode::Riding || (s.mode == Mode::FreeCam && s.before == Mode::Riding);
        view.lead = !me.seated && s.partner >= 0 && s.shown.stage != Stage::Adult;  // too small to ride: on its lead
        for (int k = 0; k < kVillagers && view.peopleCount < r3d::kMaxPeopleShown; ++k) {
            const Villager who = static_cast<Villager>(k);
            r3d::PersonView& p = view.people[view.peopleCount++];
            p.form = static_cast<u8>(personFor(who));
            p.at = villagerAt(s.valley, who);
            p.heading = s.folk[k].heading;
            p.anim = &s.folk[k].anim;
            villagerPalette(who, p.pal);
            p.blink = s.folk[k].blink;
        }
    }
    if (const ValleyPlaceInfo* market = s.valley.place(kPlaceMarket);
        market && std::hypot(market->at.x - s.you.pos.x, market->at.y - s.you.pos.y) < 160.0f) {
        static Dragon standEgg;  // the egg of the day on its stand, till it's bought
        const s32 day = dayIndex(now);
        if (app.game.eggBoughtDay != day) {
            standEgg = eggOnShow(app.game, day);
            view.marketEgg = &standEgg;
        }
        Item goods[kStallSpots];
        stallToday(app.game, day, goods);
        for (int k = 0; k < kStallSpots && k < 4; ++k) view.goods[k] = goods[k];
    }
    if (s.partner >= 0) {
        view.dragon = &s.shown;
        view.actor = &s.flyer;
    }
    const bool riding = s.mode == Mode::Riding || (s.mode == Mode::FreeCam && s.before == Mode::Riding);
    if (riding) {
        view.at = s.flight.pos;
        if (s.flight.swimming) view.at.z += 0.08f * std::sin(s.swimT * 2.6f);
        view.heading = s.flight.heading;
        view.pitch = s.flight.swimming ? -0.1f : s.flight.pitch;
        view.roll = s.flight.roll;
        view.eye = s.cam.eye;
        view.target = s.cam.target;
        view.riderOn = true;
    } else {
        view.at = s.pal.pos;
        view.heading = s.pal.heading;
        view.eye = s.wcam.eye;
        view.target = s.wcam.target;
        view.you = s.you.pos;
        view.youHeading = s.you.heading;
        view.youSpeed = s.you.speed;
        view.youShown = true;
    }
    if (s.mode == Mode::FreeCam) {
        view.eye = s.freeEye;
        view.target = s.freeEye + Vec3{std::sin(s.freeYaw) * std::cos(s.freePitch), -std::cos(s.freeYaw) * std::cos(s.freePitch),
                                       std::sin(s.freePitch)} * 10.0f;
    }
    // The shadow under a flying partner (D81, a height tell): darker and tighter coming down.
    if (s.partner >= 0) {
        const DayBlend day = dayBlend(now);
        const float lit = day.weight(kLightDay) + 0.6f * day.weight(kLightEvening) + 0.25f * day.weight(kLightNight);
        const Valley& va = s.valley;
        const float surface = std::fmax(va.heightAt(view.at.x, view.at.y), va.water);
        const float high = std::fmax(0.0f, view.at.z - surface);
        view.shadowAt = {view.at.x, view.at.y, surface};
        const bool afloat = riding && s.flight.swimming;
        view.shadow = afloat ? 0.0f : 0.5f * lit * clampf(1.0f - high / 60.0f, 0.0f, 1.0f);
        view.shadowRadius = 1.9f * kindSize(s.shown) * (s.shown.stage == Stage::Adult ? 1.0f : 0.55f) *
                            (1.0f - 0.45f * clampf(high / 60.0f, 0.0f, 1.0f));
    }
    if (r3d::ready()) r3d::drawValley(app, view, now);
    if (r3d::ready()) drawChallengeBoards(app, s.valley, now);
    // What A does here, over the picture.
    const char* hint = nullptr;
    char line[64];
    switch (s.action) {
        case Action::Talk:
            std::snprintf(line, sizeof(line), str::kPromptTalk, villagerInfo(s.actionWho).name);
            hint = line;
            break;
        case Action::Shop:
            hint = s.actionTab == 3 ? str::kPromptEggStand : str::kPromptGoods;
            break;
        case Action::Enter:
            std::snprintf(line, sizeof(line), str::kPromptEnter, world::placeInfo(s.actionPlace).name);
            hint = line;
            break;
        case Action::Light: hint = world::lanternLit(app.game, s.actionPlace) ? nullptr : str::kPromptLight; break;
        case Action::Ride: hint = str::kPromptRide; break;
        case Action::Call: hint = str::kPromptCall; break;
        case Action::Board: hint = str::kPromptBoard; break;
        case Action::None: break;
    }
    if (hint && s.mode == Mode::OnFoot && !talking(app)) {
        const float w = textWidth(app, hint, 0.5f) + 24;
        panel({200 - w / 2, 200, w, 24}, withAlpha(theme::kDenPlum, 0.8f));
        textCentered(app, hint, 200, 212, 0.5f, theme::kShell, w);
    }
}

// The painted map on the bottom screen: north up, the places found as pins (tap one to go
// there), you as a heart pointing the way you face.
constexpr float kMapX = 6, kMapY = 26, kMapSize = 172;

Vec2 mapPoint(const Valley& v, float x, float y) {
    return {kMapX + (x - v.x0) / v.size() * kMapSize, kMapY + (1.0f - (y - v.y0) / v.size()) * kMapSize};
}

void travelTo(App& app, ValleyScene& s, int place, bool outward) {
    const ValleyPlaceInfo* p = s.valley.place(static_cast<u8>(place));
    if (!p) return;
    const PlaceLayout& l = placeLayout(place);
    // (Out of a door: far enough that the camera behind you is outside too, not in the den's arch.)
    const Vec2 front = placeToWorld(*p, outward && l.hasDoor ? Vec2{l.door.x, l.door.y + 11.0f} : l.arrive);
    s.mode = Mode::OnFoot;
    s.you.pos = {front.x, front.y, s.valley.heightAt(front.x, front.y)};
    // Out of its door, its way; or (travelling) looking at it.
    s.you.heading = outward ? p->heading : std::atan2(p->at.x - front.x, -(p->at.y - front.y));
    s.you.speed = 0;
    if (s.partner >= 0) s.pal.call(s.you, s.valley);
    s.wcam = WalkCamera{};
    s.wcam.yaw = s.you.heading;
    s.wcam.update(s.you, 0, s.valley, 0.0f, &s.camWalls);  // there at once (even if nothing moves this frame)
    audio::playSfx(audio::Sfx::TravelWhoosh);
    (void)app;
}

void drawBottom(App& app, const Input& touch) {
    ValleyScene& s = vs();
    static const Input kNothing{};
    const Input& in = talking(app) ? kNothing : touch;  // someone talking: the map and buttons wait
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    textCentered(app, "Skyreach Valley", 160, 12, 0.6f, theme::kClutchGold, 300, Face::Title);
    if (!s.loaded) return;
    const Valley& va = s.valley;
    if (const C2D_Image* map = r3d::valleyMap(va))
        C2D_DrawImageAt(*map, kMapX, kMapY, 0.5f, nullptr, kMapSize / 128.0f, kMapSize / 128.0f);
    // The fog over what you haven't seen yet (WP6), in runs along each row.
    {
        const float cell = kMapSize / kFogCells;
        const u32 fog = withAlpha(theme::rgba(236, 228, 214), 0.82f);
        for (int cy = 0; cy < kFogCells; ++cy)
            for (int cx = 0; cx < kFogCells;) {
                if (explored(app.game, cx, cy)) {
                    ++cx;
                    continue;
                }
                int end = cx;
                while (end < kFogCells && !explored(app.game, end, cy)) ++end;
                C2D_DrawRectSolid(kMapX + cx * cell, kMapY + (kFogCells - 1 - cy) * cell, 0.5f, (end - cx) * cell, cell, fog);
                cx = end;
            }
    }
    // The paths, faint; the places found as pins, the rest as a question mark once you've heard of them.
    for (const std::vector<Vec2>& path : va.paths)
        for (std::size_t k = 1; k < path.size(); ++k) {
            const Vec2 a = mapPoint(va, path[k - 1].x, path[k - 1].y), b = mapPoint(va, path[k].x, path[k].y);
            C2D_DrawLine(a.x, a.y, withAlpha(theme::kShell, 0.5f), b.x, b.y, withAlpha(theme::kShell, 0.5f), 1.0f, 0.5f);
        }
    int tappedPlace = -1;
    for (const ValleyPlaceInfo& p : va.places) {
        const Vec2 m = mapPoint(va, p.at.x, p.at.y);
        if (!world::placeFound(app.game, p.id)) continue;
        const world::PlaceInfo& info = world::placeInfo(p.id);
        C2D_DrawCircleSolid(m.x, m.y, 0.5f, 4.5f, theme::kDenPlum);
        C2D_DrawCircleSolid(m.x, m.y, 0.5f, 3.5f, fromRgb(info.pin));
        if (info.lantern && world::lanternLit(app.game, p.id)) C2D_DrawCircleSolid(m.x + 4, m.y - 4, 0.5f, 1.8f, theme::kClutchGold);
        if (in.tapped && std::hypot(in.tx - m.x, in.ty - m.y) < 9) tappedPlace = p.id;
    }
    // A dragon out on the Wanderings: its loop, faint, and where it's got to (D69).
    if (wandererIndex(app.game) >= 0 && s.wanderId) {
        const Vec2* loop = nullptr;
        const int n = wanderLoop(loop);
        for (int k = 0; k < n; ++k) {
            const Vec2 a = mapPoint(va, loop[k].x, loop[k].y), b = mapPoint(va, loop[(k + 1) % n].x, loop[(k + 1) % n].y);
            C2D_DrawLine(a.x, a.y, withAlpha(theme::kClutchGold, 0.35f), b.x, b.y, withAlpha(theme::kClutchGold, 0.35f), 1.0f, 0.5f);
        }
        const Vec2 w = mapPoint(va, s.wanderAt.x, s.wanderAt.y);
        const float bob = std::sin(app.t * 3.0f);
        C2D_DrawCircleSolid(w.x, w.y, 0.5f, 4.0f + 0.5f * bob, theme::kDenPlum);
        C2D_DrawCircleSolid(w.x, w.y, 0.5f, 3.0f + 0.5f * bob, theme::kClutchGold);
    }
    const Vec3 at = hereAt(s);
    const Vec2 me = mapPoint(va, at.x, at.y);
    const float heading = s.mode == Mode::Riding ? s.flight.heading : s.you.heading;
    C2D_DrawLine(me.x, me.y, theme::kShell, me.x + std::sin(heading) * 10, me.y + std::cos(heading) * 10, theme::kShell, 2, 0.5f);
    heart(me.x, me.y, 9, theme::kRose);
    if (tappedPlace >= 0) {
        travelTo(app, s, tappedPlace, false);
        showToastf(app, str::kTravelledTo, world::placeInfo(tappedPlace).name);
    }
    // The side panel: what you're doing, the quest in hand, the buttons.
    char line[80];
    const float x = 184;
    const char* doing = s.mode == Mode::Riding ? (s.flight.grounded ? str::kRidingGround : str::kFlying)
                        : s.mode == Mode::FreeCam ? str::kFreeCamera : str::kOnFoot;
    text(app, doing, x, 26, 0.45f, theme::kShell, C2D_AlignLeft, 132);
    if (s.mode == Mode::Riding) {
        const Rect bar{x, 44, 128, 8};
        panel(bar, theme::kDenPlum);
        panel({bar.x, bar.y, bar.w * s.flight.stamina, bar.h}, theme::kClutchGold);
    }
    const int q = campaign::currentQuest(app.game);
    if (q >= 0) {
        const campaign::QuestView v = campaign::view(app.game, q);
        text(app, v.title, x, 58, 0.4f, theme::kClutchGold, C2D_AlignLeft, 132);
        text(app, v.step, x, 72, 0.36f, withAlpha(theme::kShell, 0.85f), C2D_AlignLeft, 132);
    }
    std::snprintf(line, sizeof(line), str::kPlacesAndLanterns, world::placesFound(app.game), world::placeCount(),
                  world::lanternsLit(app.game), world::lanternCount());
    text(app, line, x, 104, 0.36f, withAlpha(theme::kShell, 0.7f), C2D_AlignLeft, 132);
    const char* help = s.mode == Mode::Riding ? (s.flight.grounded ? str::kHelpRideGround : str::kHelpFly)
                       : s.mode == Mode::FreeCam ? str::kHelpFreeCam : str::kHelpFoot;
    text(app, help, x, 120, 0.34f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft, 132);
    // Buttons: the free camera, calling your partner, home.
    if (button(app, {x, 164, 62, 30}, s.mode == Mode::FreeCam ? str::kBack : str::kLook, in)) {
        if (s.mode == Mode::FreeCam) {
            s.mode = s.before;
        } else {
            s.before = s.mode;
            s.mode = Mode::FreeCam;
            const Vec3 eye = s.before == Mode::Riding ? s.cam.eye : s.wcam.eye;
            s.freeEye = eye;
            s.freeYaw = s.before == Mode::Riding ? s.flight.heading : s.wcam.yaw;
            s.freePitch = -0.35f;
        }
        audio::playSfx(audio::Sfx::Tap);
    }
    if (s.partner >= 0 && s.mode == Mode::OnFoot && button(app, {x + 66, 164, 62, 30}, str::kCall, in)) {
        s.pal.call(s.you, va);
        audio::playSfx(audio::Sfx::Chirp, 1.1f);
    }
    if (button(app, {x, 200, 128, 32}, str::kHomeX, in)) {
        audio::playSfx(audio::Sfx::Back);
        leaveTo(app, SceneId::Den);
    }
    drawTalk(app);  // someone talking: the box over it all
}

}  // namespace

void openValleyAt(App& app, int place) {
    ValleyScene& s = vs();
    if (!s.tried) {
        s.tried = true;
        s.loaded = loadValleyFile(s.valley);
        if (s.loaded) {
            s.solids = worldSolids(s.valley);
            for (int k = 0; k < kVillagers; ++k) {  // the villagers: walked round, not through
                const Vec3 p = villagerAt(s.valley, static_cast<Villager>(k));
                s.solids.push_back({{p.x, p.y}, 0.45f});
            }
            s.camWalls = cameraWalls(s.valley);
        }
    }
    SaveData& g = app.game;
    // Your partner: the one chosen (D81), else the one you're caring for; eggs stay home.
    s.partner = -1;
    for (int i = 0; i < g.dragonCount; ++i)
        if (g.dragons[i].id == g.world.partnerId && g.dragons[i].stage != Stage::Egg && !g.dragons[i].wanderSince) s.partner = i;
    if (s.partner < 0 && hasDragon(app) && activeDragon(app).stage != Stage::Egg && !activeDragon(app).wanderSince)
        s.partner = app.careIndex;  // (one out on the Wanderings is off on its own)
    if (s.partner >= 0) {
        s.shown = g.dragons[s.partner];
        g.world.partnerId = s.shown.id;
    }
    s.mode = Mode::OnFoot;
    s.flight = Flight{};
    s.cam = ChaseCamera{};
    s.flyer = DenActor{};
    s.clip = ClipId::Count;
    s.speedsSet = false;
    s.skimFor = s.swimT = 0;
    s.breathT = -1;
    s.pal = Follower{};
    s.pal.gap = 1.6f + 1.4f * kindSize(s.shown) * (s.shown.stage == Stage::Adult ? 1.0f : 0.55f);
    app.scene = SceneId::Valley;
    if (!s.loaded) return;
    travelTo(app, s, place, true);
    if (!(g.world.flags & kFlagEnteredValley)) {  // the first step outside: the Lantern Festival's first step
        g.world.flags |= kFlagEnteredValley;
        campaign::update(g);
    }
    g.world.inValley = 1;
}

void openValley(App& app) { openValleyAt(app, kPlaceDen); }

// Continue, left in the valley: back where you were (on foot, your partner at your side), if
// that's still somewhere to stand; else out of the den's door.
void resumeValley(App& app) {
    openValleyAt(app, kPlaceDen);
    ValleyScene& s = vs();
    const WorldState& w = app.game.world;
    if (!s.loaded || !std::isfinite(w.x) || !std::isfinite(w.y) || !std::isfinite(w.heading)) return;
    const Valley& v = s.valley;
    const float margin = 40.0f;
    if (w.x < v.x0 + margin || w.y < v.y0 + margin || w.x > v.x0 + v.size() - margin || w.y > v.y0 + v.size() - margin)
        return;
    if (v.heightAt(w.x, w.y) < v.water - 0.3f) return;  // in the water (it was flown over): the den's door
    s.you.pos = {w.x, w.y, v.heightAt(w.x, w.y)};
    s.you.heading = w.heading;
    s.you.speed = 0;
    if (s.partner >= 0) s.pal.call(s.you, v);
    s.wcam = WalkCamera{};
    s.wcam.yaw = s.you.heading;
    s.wcam.update(s.you, 0, v, 0.0f, &s.camWalls);
}

const SceneFns kValleyScene{update, drawTop, drawBottom};

// For the challenges (scene_challenge): the landscape they're held in, and its sky.
const Valley* loadedValley() { return vs().loaded ? &vs().valley : nullptr; }

void valleySky(s64 now, Rgb& top, Rgb& horizon, Rgb& tint) {
    const Sky s = skyFor(now);
    top = s.top;
    horizon = s.horizon;
    tint = s.tint;
}

}  // namespace ec
