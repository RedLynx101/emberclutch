// Roaming trainers in the valley (workstream D; the walks, the roster and the duels' rules are
// core/roamers). Every frame, whoever has the valley, each trainer out today is where their day's
// walk has them (their clock stops while they stand for you, talk or duel): walking the paths
// (the walk clip keeping pace), sitting at a viewpoint, looking about at a place; their dragon
// follows at their heels (a Follower) and is drawn when it's the nearest within 40 m (one at a
// time: a dragon is ~1,000 triangles), on its light model in the Market. They wave as you come by
// (a bubble with a word of hello) and stop when you're close; press A: their lines, "Duel?", then
// a friendly duel staged right there (feature_league's stageBattle, app/battle_view). While
// someone else has the valley (a battle, a show) the ones near you stop and watch, clapping.
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "app/audio.hpp"
#include "app/autotest.hpp"
#include "app/battle_feature.hpp"
#include "app/battle_view.hpp"
#include "app/dialogue.hpp"
#include "app/glade_show.hpp"
#include "app/render3d.hpp"
#include "app/roamers_feature.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/clock.hpp"
#include "core/kinds.hpp"
#include "core/people.hpp"
#include "core/place_layout.hpp"
#include "core/rig.hpp"
#include "core/roamers.hpp"
#include "core/trainer.hpp"
#include "core/valley.hpp"
#include "core/villagers.hpp"
#include "core/walker.hpp"

namespace ec {
namespace {

constexpr float kDragonNear = 40.0f;  // a trainer's dragon is drawn this near you (the nearest one)
constexpr float kStopFor = 5.5f;      // they stop for you this near
constexpr float kGreetAt = 9.0f;      // and wave hello this near
constexpr float kWatchAt = 30.0f;     // someone else's battle or show: those this near stop to watch

enum class Mode : u8 { None, Ask, Duel, Words, Watch };  // (Words: their parting words after a duel)

struct Out {  // a trainer out today
    int id = -1;
    roam::Walk walk;
    float held = 0;  // seconds they've stood for you (their day runs that far behind)
    roam::Pose pose;
    Vec3 at;
    bool placed = false;
    float heading = 0;  // as drawn (eased round)
    r3d::PersonView look;
    Animator anim;
    const char* clip = nullptr;
    float blink = 0, blinkIn = 2;
    float greetT = 0;  // the hello's bubble, seconds left
    int greetLine = 0;
    bool greeted = false;
    float fidgetIn = 6;  // seconds to a look about while they stand at a place
    float clapIn = 0;    // (watching a battle: the next clap heard)
    // Their dragon at their heels.
    Dragon dragon;
    int level = -1;  // (the level it was made at)
    DenActor actor;
    Follower pal;
    ClipId palClip = ClipId::Count;
    bool palSet = false, speedsSet = false;
    float natWalk = 2.2f, natTrot = 4.0f, natRun = 6.0f;
};

struct State {
    s32 day = INT_MIN;
    int count = 0;
    Out out[roam::kMaxOut];
    roam::PathNet net;
    const Valley* netOf = nullptr;
    std::vector<Solid> solids;  // the places' walls and the villagers
    bool about = false;         // (their walking hours)
    Mode mode = Mode::None;
    int who = -1;        // the one asked, duelling or watched (index in out)
    int talkWith = -1;   // the one whose lines are open
    int partner = -1;
    int foeLevel = 1;
    s32 dueledDay = INT_MIN;
    bool dueled[roam::kRoamers] = {};  // a duel with them today (their rematch lines)
    int pendingNear = -1, pendingTalk = -1, pendingDuel = -1, pendingWatch = -1;
    float watchFor = 0;
    int forceLevel = 0;  // (scripted runs: their dragon's level; 0 fair)
    s32 forceDay = INT_MIN;  // (scripted runs: the roster of this day, whatever the clock says)
    float t = 0;
};

State& st() {
    static State s;
    return s;
}

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
float headingTo(Vec3 from, Vec3 to) { return std::atan2(to.x - from.x, -(to.y - from.y)); }
Vec3 forwardOf(float heading) { return {std::sin(heading), -std::cos(heading), 0}; }
float flat(Vec3 a, Vec3 b) { return std::hypot(a.x - b.x, a.y - b.y); }

bool ours(const App& app) {
    const int f = vext::activeFeature(app);
    return f >= 0 && std::strcmp(vext::feature(f).name, kRoamerFeature.name) == 0;
}

Speaker speakerOf(int id) {
    const roam::Roamer& r = roam::roamer(id);
    Speaker sp;
    sp.name = r.name;
    sp.title = r.title;
    sp.voice = r.voice;
    sp.pitch = r.pitch;
    sp.tint = r.look.outfit;
    return sp;
}

void say(App& app, int k, const char* a, const char* b = nullptr) {
    State& s = st();
    if (k < 0 || k >= s.count) return;
    Talk t;
    t.lines[t.count++] = a;
    if (b) t.lines[t.count++] = b;
    s.talkWith = k;
    startSpeech(app, speakerOf(s.out[k].id), t);
}

bool dueledToday(const App& app, int id) {
    const State& s = st();
    return s.dueledDay == dayIndex(nowLocal(app)) && id >= 0 && id < roam::kRoamers && s.dueled[id];
}

int partnerLevel(const App& app, const vext::Stage& stage) {
    return stage.partner >= 0 && stage.partner < app.game.dragonCount ? trainer::levelOf(app.game.dragons[stage.partner]) : 1;
}

int levelFor(const App& app, const vext::Stage& stage, int id) {
    const State& s = st();
    return s.forceLevel > 0 ? s.forceLevel : roam::duelLevel(partnerLevel(app, stage), id);
}

// ---- Today's trainers: the roster once a day, the paths' network once a valley.
void refresh(App& app, const Valley& v) {
    State& s = st();
    if (s.netOf != &v) {
        s.solids = worldSolids(v);
        for (int k = 0; k < kVillagers; ++k) {
            const VillagerInfo& info = villagerInfo(static_cast<Villager>(k));
            if (const ValleyPlaceInfo* p = v.place(static_cast<u8>(info.place))) s.solids.push_back({placeToWorld(*p, info.at), 0.6f});
        }
        roam::buildNet(v, s.solids, s.net);
        s.netOf = &v;
        s.day = INT_MIN;
    }
    const s64 now = nowLocal(app);
    s.about = roam::walkingHour(hourOfDay(now)) && s.net.ok();
    const s32 day = s.forceDay != INT_MIN ? s.forceDay : dayIndex(now);
    if (day == s.day) return;
    s.day = day;
    u8 ids[roam::kMaxOut];
    s.count = roam::roster(day, ids);
    for (int k = 0; k < s.count; ++k) {
        Out& o = s.out[k];
        o = Out{};
        o.id = ids[k];
        roam::startWalk(s.net, o.id, day, o.walk);
        const roam::Roamer& r = roam::roamer(o.id);
        o.look.form = r.person;
        roam::palette(o.id, o.look.pal);
        o.look.hair = r.look.hair == roam::kNoHair ? -1 : static_cast<s8>(r.look.hair);
        o.look.anim = &o.anim;
        o.greetLine = (day + k) & 1;
        o.fidgetIn = 4.0f + 2.0f * k;
    }
    if (s.mode != Mode::Duel) {
        s.mode = Mode::None;
        s.who = s.talkWith = -1;
    }
}

// ---- Their clips.
void play(Out& o, const char* name, float rate = 1.0f, float fade = 0.25f, bool restart = false) {
    const AnimLibrary* lib = r3d::personAnims();
    if (!lib) return;
    const int c = lib->find(name);
    if (c < 0) return;
    if (o.clip != name || restart) o.anim.play(c, fade, restart);
    o.anim.rate = rate;
    o.clip = name;
}

bool once(const Out& o, const char* name) {  // a one-shot of theirs still playing
    const AnimLibrary* lib = r3d::personAnims();
    return lib && o.clip == name && !o.anim.finished(*lib);
}

void animatePerson(App& app, Out& o, bool stopped, bool watching, bool speaking, bool nearYou) {
    const AnimLibrary* lib = r3d::personAnims();
    if (!lib) return;
    const Person body = static_cast<Person>(o.look.form);
    if (speaking) {
        play(o, "talk");
    } else if (once(o, "wave") || once(o, "fist_pump") || once(o, "look_around") || once(o, "stretch")) {
        // (a gesture plays out)
    } else if (watching) {  // someone else's battle or show: clapping, now and then a cheer
        o.fidgetIn -= app.dt;
        if (o.fidgetIn <= 0) {
            o.fidgetIn = 3.0f + static_cast<float>(app.rng.below(3000)) * 0.001f;
            play(o, "fist_pump", 1.0f, 0.2f, true);
        } else {
            play(o, "clap");
            if (nearYou && (o.clapIn -= app.dt) <= 0) {  // (their claps, heard when they're close)
                o.clapIn = 1.4f + static_cast<float>(app.rng.below(800)) * 0.001f;
                audio::playSfx(audio::Sfx::Clap, 1.1f, 0.35f);
            }
        }
    } else if (o.pose.walking && !stopped) {
        play(o, "walk", clampf(o.pose.speed / personWalkSpeed(body), 0.6f, 2.2f));
    } else if (o.pose.sitting && !stopped) {
        play(o, "sit_ground", 1.0f, 0.6f);  // (a slow ease down)
    } else {
        o.fidgetIn -= app.dt;
        if (o.fidgetIn <= 0 && !nearYou) {  // stood at a place: a look about, now and then a stretch
            o.fidgetIn = 7.0f + static_cast<float>(app.rng.below(6000)) * 0.001f;
            play(o, app.rng.chance(1, 4) ? "stretch" : "look_around", 1.0f, 0.3f, true);
        } else {
            play(o, "idle", 1.0f, 0.3f);
        }
    }
    o.anim.update(*lib, app.dt, nullptr, 0);
    o.blinkIn -= app.dt;
    if (o.blinkIn <= 0) {
        o.blink = 1;
        o.blinkIn = 1.5f + static_cast<float>(app.rng.below(3500)) * 0.001f;
    }
    o.blink = std::fmax(0.0f, o.blink - app.dt * 7.0f);
    o.look.blink = o.blink;
}

// ---- Their dragon at their heels: made at your partner's fair level (the one they'd duel with),
// following them (it lies down beside them while they sit), animated by how fast it goes.
void measureGaits(Out& o) {
    if (o.speedsSet) return;
    const AnimLibrary* lib = r3d::animsFor(o.dragon);
    const int look = r3d::lookFor(o.dragon);
    const ModelData* m = r3d::model(kFormGrown, look);
    const AnimBinding* bind = r3d::binding(kFormGrown, look);
    if (!lib || !m || !bind) return;
    const int build = o.dragon.genome.build < kModelBuilds ? o.dragon.genome.build : kBuildNeutral;
    o.actor.updateSpeeds(*m, *bind, *lib, r3d::clipIndexFor(o.dragon, kFormGrown), kFormGrown * r3d::kLookSlots + look, 1.0f, build,
                         kindSize(o.dragon), false);
    o.natWalk = clampf(o.actor.behavior.walkSpeed, 0.6f, 4.0f);
    o.natRun = clampf(o.actor.behavior.runSpeed, o.natWalk * 2.0f, 14.0f);
    o.natTrot = o.actor.behavior.trotSpeed > o.natWalk && o.actor.behavior.trotSpeed < o.natRun ? o.actor.behavior.trotSpeed
                                                                                                 : (o.natWalk + o.natRun) * 0.5f;
    o.pal.walk = o.natWalk;
    o.pal.trot = o.natTrot;
    o.pal.run = std::fmax(o.natRun * 1.4f, 6.0f);
    o.speedsSet = true;
}

void animateDragon(App& app, Out& o, const Valley& v, const vext::Stage& stage, float speed, int level) {
    State& s = st();
    if (o.level != level) {
        o.dragon = roam::dragonOf(o.id, level);
        o.level = level;
    }
    Walker lead;  // (the trainer, as the follower sees them)
    lead.pos = o.at;
    lead.heading = o.heading;
    lead.speed = speed;
    if (!o.palSet) {
        o.pal = Follower{};
        o.pal.gap = 1.2f + 0.9f * kindSize(o.dragon);
        o.pal.call(lead, v);
        o.palSet = true;
        o.speedsSet = false;
        o.palClip = ClipId::Count;
    }
    measureGaits(o);
    // (round the walls near it, and round you and your partner: it doesn't walk into them)
    static std::vector<Solid> near;
    near.clear();
    for (const Solid& w : s.solids)
        if (std::hypot(w.at.x - o.pal.pos.x, w.at.y - o.pal.pos.y) < w.radius + 16.0f) near.push_back(w);
    near.push_back({{stage.you.x, stage.you.y}, 0.6f});
    if (stage.shown) near.push_back({{stage.pal.x, stage.pal.y}, 1.1f * kindSize(*stage.shown)});
    o.pal.update(lead, v, near, app.dt);
    const AnimLibrary* lib = r3d::animsFor(o.dragon);
    if (!lib) return;
    const int* clips = r3d::clipIndexFor(o.dragon, kFormGrown);
    ClipId want = ClipId::Idle;
    float rate = 1.0f;
    if (o.pal.speed > 0.15f) {
        const bool running = o.pal.speed > o.natTrot * 1.3f, trotting = o.pal.speed > o.natWalk * 1.3f;
        want = running ? ClipId::Gallop : trotting ? ClipId::Trot : ClipId::Walk;
        rate = clampf(o.pal.speed / (running ? o.natRun : trotting ? o.natTrot : o.natWalk), 0.5f, 2.0f);
    } else if (o.pose.sitting) {
        want = ClipId::LieLoop;  // (lying down beside them while they take in the view)
    }
    if (want != o.palClip && clips[static_cast<int>(want)] >= 0) {
        o.actor.anim.play(clips[static_cast<int>(want)], 0.35f, true);
        o.palClip = want;
    }
    o.actor.anim.rate = rate;
    o.actor.anim.update(*lib, app.dt, nullptr, 0);
    o.actor.eyes.update(0.0f, app.dt);
}

// ---- Every frame, whoever has the valley.
void tick(App& app, const vext::Stage& stage) {
    State& s = st();
    if (!stage.valley) return;
    const Valley& v = *stage.valley;
    refresh(app, v);
    s.t += app.dt;
    if (!s.about) return;
    const float clock = roam::dayClock(nowLocal(app));
    const int feat = vext::activeFeature(app);
    const bool mine = ours(app), busy = !mine && (bview::running() || glade::showOn());  // (a battle or a show to watch)
    const bool listening = talking(app);
    const int level = levelFor(app, stage, 0);  // (their dragons: about your partner's level)
    int nearest = -1;
    float nearestD = 70.0f;
    for (int k = 0; k < s.count; ++k) {
        Out& o = s.out[k];
        const float d = o.placed ? flat(stage.you, o.at) : 1e9f;
        const bool engaged = (s.who == k && (s.mode == Mode::Ask || s.mode == Mode::Duel || s.mode == Mode::Words)) ||
                             (listening && s.talkWith == k);
        const bool watching = busy && d < kWatchAt && !engaged;
        const bool forYou = feat < 0 && !stage.riding && d < kStopFor;
        const bool stopped = engaged || watching || forYou;
        if (stopped) o.held += app.dt;  // (their day waits)
        o.pose = roam::walkAt(s.net, o.walk, clock - o.held);
        const float z = o.placed ? o.at.z : v.heightAt(o.pose.at.x, o.pose.at.y);
        o.at = {o.pose.at.x, o.pose.at.y, v.groundAt(o.pose.at.x, o.pose.at.y, z + 0.6f)};
        // Turned to you when they stop for you (or to the battle they watch), else their way.
        const float want = (forYou || watching || engaged) && d < kWatchAt ? headingTo(o.at, stage.you) : o.pose.heading;
        if (!o.placed) o.heading = want;
        o.heading += clampf(std::remainder(want - o.heading, 6.2831853f), -4.0f * app.dt, 4.0f * app.dt);
        o.placed = true;
        o.look.at = o.at;
        o.look.heading = o.heading;
        // A wave and a word as you come by (once a pass).
        if (!o.greeted && d < kGreetAt && feat < 0 && !listening) {
            o.greeted = true;
            o.greetT = 2.8f;
            play(o, "wave", 1.0f, 0.2f, true);
            audio::playSfx(audio::Sfx::Greet, roam::roamer(o.id).pitch > 1.5f ? 1.15f : 1.0f, 0.8f);
        }
        if (d > 20.0f) o.greeted = false;
        o.greetT = std::fmax(0.0f, o.greetT - app.dt);
        const float seen = s.mode == Mode::Watch && s.who == k ? 0.0f : d;  // (the one watched: as if near)
        if (seen < 90.0f) animatePerson(app, o, stopped, watching, engaged && listening, d < 8.0f);
        if (seen < 70.0f && !(s.mode == Mode::Duel && s.who == k))
            animateDragon(app, o, v, stage, stopped || !o.pose.walking ? 0.0f : o.pose.speed, level);
        else
            o.palSet = false;  // (far off: it catches up at their side when you're near again)
        if (seen < nearestD) {
            nearestD = seen;
            nearest = k;
        }
    }
    if (nearest >= 0 && (feat < 0 || (mine && s.mode != Mode::Duel)))
        r3d::wantKind(s.out[nearest].dragon.kind);  // (read ahead as you come near)
}

// ---- The duel.
void finishDuel(App& app, battle::Outcome o, bview::Results& out);
void duelDone(App& app, battle::Outcome o);

bool beginDuel(App& app, vext::Stage& stage, int k) {
    State& s = st();
    if (!stage.valley || stage.partner < 0 || stage.partner >= app.game.dragonCount || k < 0 || k >= s.count) return false;
    const Valley& v = *stage.valley;
    Out& o = s.out[k];
    const roam::Roamer& r = roam::roamer(o.id);
    bview::Setup setup;
    setup.partner = stage.partner;
    s.foeLevel = levelFor(app, stage, o.id);
    setup.foe = roam::dragonOf(o.id, s.foeLevel);
    std::snprintf(setup.foeName, sizeof(setup.foeName), "%s", r.dragonName);
    std::snprintf(setup.intro, sizeof(setup.intro), str::kDuelWants, r.name);
    setup.skill = r.skill;
    setup.trainer = true;
    setup.trainerLook = o.look;
    setup.trainerLook.anim = nullptr;
    const Dragon& mine = app.game.dragons[stage.partner];
    const float sizeYou = bview::dragonSize(mine), sizeFoe = bview::dragonSize(setup.foe);
    // Away from the nearest place's buildings (else just round them).
    Vec2 middle{o.at.x + std::sin(o.heading) * 20.0f, o.at.y - std::cos(o.heading) * 20.0f};
    float best = 80.0f;
    for (const ValleyPlaceInfo& p : v.places)
        if (flat(p.at, o.at) < best) {
            best = flat(p.at, o.at);
            middle = {p.at.x, p.at.y};
        }
    if (!stageBattle(v, o.at, stage.you, stage.youHeading, middle, sizeYou, sizeFoe, mine.stage != Stage::Adult, setup)) return false;
    setup.finish = finishDuel;
    setup.done = duelDone;
    s.who = k;
    s.partner = stage.partner;
    s.mode = Mode::Duel;
    bview::start(app, setup);
    return true;
}

void finishDuel(App& app, battle::Outcome o, bview::Results& out) {
    State& s = st();
    if (s.partner < 0 || s.partner >= app.game.dragonCount || s.who < 0 || s.who >= s.count) return;
    Dragon& d = app.game.dragons[s.partner];
    const int id = s.out[s.who].id;
    const s32 today = dayIndex(nowLocal(app));
    const roam::Reward r = roam::record(app.game, s.partner, id, s.foeLevel, o, today);
    if (s.dueledDay != today) {
        s.dueledDay = today;
        for (bool& b : s.dueled) b = false;
    }
    s.dueled[id] = true;
    if (r.growth.xp) out.add(str::kBattleXp, d.name, static_cast<unsigned long>(r.growth.xp));
    if (r.growth.levelAfter > r.growth.levelBefore) {
        out.add(str::kBattleLevelUp, d.name, r.growth.levelAfter);
        queueToastf(app, "%s", out.lines[out.count - 1]);
        out.levelUp = true;
    }
    u8 four[kMoveSlots];
    battle::equippedMoves(d, four);
    for (int i = 0; i < r.growth.learnedCount; ++i) {
        bool equipped = false;
        for (u8 m : four) equipped |= m == r.growth.learned[i];
        if (equipped) out.add(str::kBattleLearned, d.name, battle::moveName(r.growth.learned[i]));
        else out.add(str::kBattleLearnedSwap, battle::moveName(r.growth.learned[i]));
    }
    if (r.gleam) {
        out.add(str::kBattleGleam, static_cast<unsigned long>(r.gleam));
        audio::playSfx(audio::Sfx::Coin, 1.0f, 0.7f);
    } else if (r.paidBefore) {
        out.add("%s", str::kBattlePaid);
    }
    if (o == battle::Outcome::Won) out.add(str::kDuelsWonLine, static_cast<int>(app.game.progress.duelsWon));
    saveNow(app);
}

void duelDone(App& app, battle::Outcome o) {
    State& s = st();
    const int k = s.who;
    s.mode = Mode::None;
    if (k < 0 || k >= s.count) {
        s.who = -1;
        return;
    }
    const roam::Roamer& r = roam::roamer(s.out[k].id);
    say(app, k, o == battle::Outcome::Won ? r.lost : r.won);  // (their words: they lost, or they won)
    s.out[k].palSet = false;  // (their dragon back at their side, from where the duel had it)
    s.out[k].greeted = true;
    s.mode = Mode::Words;  // (the camera on them while they speak)
}

void goDuel(App& app, vext::Stage& stage) {
    State& s = st();
    Dragon& d = app.game.dragons[stage.partner];
    if (!trainer::spendEnergy(d, trainer::kEnergyBattle)) {
        s.mode = Mode::None;
        audio::playSfx(audio::Sfx::Error);
        say(app, s.who, str::kDuelTooTired);
        return;
    }
    if (!beginDuel(app, stage, s.who)) {  // (nowhere to stand: rare) its Energy back
        d.needs.energy = std::fmin(100.0f, d.needs.energy + trainer::kEnergyBattle);
        s.mode = Mode::None;
        showToast(app, str::kBattleNoRoom);
    }
}

// ---------------------------------------------------------------------------- the feature
int folk(const App& app, const Valley& v, Vec3 near, float radius, vext::Folk* out, int cap) {
    const State& s = st();
    (void)app;
    (void)v;
    if (!s.about) return 0;
    int n = 0;
    for (int k = 0; k < s.count && n < cap; ++k) {
        const Out& o = s.out[k];
        if (!o.placed || (s.mode == Mode::Duel && s.who == k)) continue;  // (the duel draws them)
        if (flat(o.at, near) > radius && !(s.mode == Mode::Watch && s.who == k)) continue;
        const roam::Roamer& r = roam::roamer(o.id);
        vext::Folk& f = out[n++];
        f = vext::Folk{};
        f.look = o.look;
        f.live = &o.look;
        f.name = r.name;
        f.prompt = str::kPromptDuel;
        f.id = static_cast<u8>(k);
        f.voice = r.voice;
        f.pitch = r.pitch;
        f.reach = 2.6f;
    }
    return n;
}

// While they talk and ask: the camera over your shoulder, on them (their dragon in the picture).
void talkCamera(vext::Stage& stage, const Out& o) {
    Vec3 dir = o.at - stage.you;
    dir.z = 0;
    const float l = std::fmax(0.01f, length(dir));
    dir = dir * (1.0f / l);
    Vec3 side{-dir.y, dir.x, 0};
    if (dot(side, stage.pal - stage.you) > 0) side = side * -1.0f;  // (away from your partner: 7 m off, its lighter model)
    stage.camSet = true;
    stage.eye = stage.you - dir * 6.0f + side * 1.8f + Vec3{0, 0, 2.6f};
    if (stage.valley) stage.eye.z = std::fmax(stage.eye.z, stage.valley->heightAt(stage.eye.x, stage.eye.y) + 1.2f);
    stage.target = o.at - dir * 0.6f + Vec3{0, 0, o.look.form == static_cast<u8>(Person::Child) ? 0.6f : 0.85f};
}

void act(App& app, const vext::Folk& who, vext::Stage& stage) {
    State& s = st();
    const int k = who.id;
    if (k < 0 || k >= s.count) return;
    const roam::Roamer& r = roam::roamer(s.out[k].id);
    s.who = k;
    if (stage.partner < 0) {
        say(app, k, str::kDuelNoPartner);
        s.who = -1;
        return;
    }
    if (dueledToday(app, s.out[k].id)) say(app, k, r.again);
    else say(app, k, r.hello[0], r.hello[1]);
    s.mode = Mode::Ask;
    s.t = 0;
    talkCamera(stage, s.out[k]);  // (it holds while their lines play)
}

bool active(const App& app) {
    const State& s = st();
    (void)app;
    return s.mode != Mode::None || s.pendingNear >= 0 || s.pendingTalk >= 0 || s.pendingDuel >= 0 || s.pendingWatch >= 0;
}

// Scripted runs: you `dist` metres before trainer k, turned to them, your partner a step aside.
bool standBy(vext::Stage& stage, int k, float dist) {
    State& s = st();
    if (!stage.valley || k < 0 || k >= s.count || !s.out[k].placed) return false;
    const Out& o = s.out[k];
    const Vec3 spot = o.at + forwardOf(o.heading) * dist;
    stage.you = {spot.x, spot.y, stage.valley->heightAt(spot.x, spot.y)};
    stage.youHeading = headingTo(stage.you, o.at);
    stage.pal = stage.you + forwardOf(stage.youHeading + 1.3f) * 1.8f;
    stage.palHeading = stage.youHeading;
    return true;
}

// (a scripted jump: the scene stands you there too, the walking camera set behind you)
void jumpTo(App& app, Vec3 at) {
    app.autoGoto[0] = at.x;
    app.autoGoto[1] = at.y;
    app.autoGoto[2] = 1;
}

void update(App& app, const Input& in, vext::Stage& stage) {
    State& s = st();
    s.t += app.dt;
    if (s.pendingNear >= 0) {
        const int k = s.pendingNear;
        s.pendingNear = -1;
        if (k < s.count) {
            Out& o = s.out[k];
            // A few steps ahead on their way: where their walk has them in 9 seconds.
            roam::Walk later = o.walk;
            const roam::Pose p = roam::walkAt(s.net, later, roam::dayClock(nowLocal(app)) - o.held + 9.0f);
            const Vec3 at{p.at.x, p.at.y, 0};
            const bool moving = flat(at, o.at) > 3.0f;
            const Vec3 spot = moving ? at : o.at + forwardOf(o.heading) * 7.0f;
            if (stage.valley) {
                stage.you = {spot.x, spot.y, stage.valley->heightAt(spot.x, spot.y)};
                stage.youHeading = headingTo(stage.you, o.at);
                stage.pal = stage.you + forwardOf(stage.youHeading + 1.3f) * 1.8f;
                stage.palHeading = stage.youHeading;
                jumpTo(app, stage.you);
            }
            o.greeted = false;
        }
        return;
    }
    if (s.pendingTalk >= 0) {
        const int k = s.pendingTalk;
        s.pendingTalk = -1;
        if (standBy(stage, k, 2.0f)) {
            jumpTo(app, stage.you);
            vext::Folk who;
            who.id = static_cast<u8>(k);
            act(app, who, stage);
        }
        return;
    }
    if (s.pendingDuel >= 0) {
        const int k = s.pendingDuel;
        s.pendingDuel = -1;
        if (standBy(stage, k, 3.0f)) jumpTo(app, stage.you);
        if (!beginDuel(app, stage, k)) {
            s.mode = Mode::None;
            showToast(app, str::kBattleNoRoom);
        }
        return;
    }
    if (s.pendingWatch >= 0) {
        s.who = s.pendingWatch < s.count ? s.pendingWatch : -1;
        s.pendingWatch = -1;
        s.mode = s.who >= 0 ? Mode::Watch : Mode::None;
    }
    switch (s.mode) {
        case Mode::Ask:
            if (s.who >= 0 && s.who < s.count) talkCamera(stage, s.out[s.who]);
            if (in.down & KEY_A) goDuel(app, stage);
            else if (in.down & KEY_B) {
                s.mode = Mode::None;
                s.who = -1;
                audio::playSfx(audio::Sfx::Back);
            }
            break;
        case Mode::Duel:
            bview::update(app, in, stage);
            if (!bview::running() && s.mode == Mode::Duel) {
                s.mode = Mode::None;
                s.who = -1;
            }
            if (s.mode == Mode::Words && s.who >= 0 && s.who < s.count) talkCamera(stage, s.out[s.who]);  // (it holds while they speak)
            break;
        case Mode::Words:
            if (!talking(app) || s.who < 0 || s.who >= s.count) {
                s.mode = Mode::None;
                s.who = -1;
            } else {
                talkCamera(stage, s.out[s.who]);
            }
            break;
        case Mode::Watch: {  // the camera beside them, a little ahead, as they walk
            s.watchFor -= app.dt;
            if (s.watchFor <= 0 || (in.down & KEY_B) || s.who < 0) {
                s.mode = Mode::None;
                s.who = -1;
                break;
            }
            const Out& o = s.out[s.who];
            const Vec3 fwd = forwardOf(o.heading), side{-fwd.y, fwd.x, 0};
            stage.camSet = true;
            stage.eye = o.at + fwd * 5.5f - side * 5.0f + Vec3{0, 0, 2.6f};  // (on the side away from their dragon)
            if (stage.valley) stage.eye.z = std::fmax(stage.eye.z, stage.valley->heightAt(stage.eye.x, stage.eye.y) + 1.5f);
            stage.target = o.at + fwd * 0.5f + side * 0.8f + Vec3{0, 0, 0.8f};
            break;
        }
        case Mode::None: break;
    }
}

// Their dragons into the view: the nearest within reach, if a slot's free and its kind is in.
void addDragons(App& app, const vext::Stage& stage, r3d::ValleyView& view, Vec3 from) {
    State& s = st();
    (void)app;
    if (!s.about || !stage.valley || view.otherCount >= r3d::kMaxOthers) return;
    int pick = -1;
    float best = kDragonNear;
    for (int k = 0; k < s.count; ++k) {
        const Out& o = s.out[k];
        if (!o.palSet || (s.mode == Mode::Duel && s.who == k) || !r3d::kindReady(o.dragon.kind)) continue;
        const float d = flat(o.pal.pos, from);
        if (d < best) {
            best = d;
            pick = k;
        }
    }
    if (pick < 0) return;
    Out& o = s.out[pick];
    r3d::ValleyDragon& d = view.others[view.otherCount++];
    d = r3d::ValleyDragon{};
    d.dragon = &o.dragon;
    d.actor = &o.actor;
    d.at = o.pal.pos;
    d.heading = o.pal.heading;
    d.lite = true;  // (always its light model: a trainer's dragon about is a passer-by, ~1,100 triangles)
}

void ambient(App& app, const vext::Stage& stage, r3d::ValleyView& view) {
    if (bview::running() || glade::showOn()) return;  // (a battle's or a show's view has its hands full)
    addDragons(app, stage, view, stage.you);
}

void view(App& app, const vext::Stage& stage, r3d::ValleyView& view) {
    State& s = st();
    if (s.mode == Mode::Duel) bview::view(app, stage, view);
    if ((s.mode == Mode::Ask || s.mode == Mode::Words) && s.who >= 0) {  // their dragon by them while they talk
        r3d::wantKind(s.out[s.who].dragon.kind);
        addDragons(app, stage, view, s.out[s.who].at);
    }
    if (s.mode == Mode::Watch && s.who >= 0) addDragons(app, stage, view, s.out[s.who].at);
}

// The hello's bubble over their head.
void drawOver(App& app, const vext::Stage& stage) {
    State& s = st();
    (void)stage;
    if (!s.about) return;
    for (int k = 0; k < s.count; ++k) {
        const Out& o = s.out[k];
        if (o.greetT <= 0) continue;
        float x, y, ppu;
        const float top = o.look.form == static_cast<u8>(Person::Child) ? 1.25f : 1.6f;
        if (!r3d::project(o.at + Vec3{0, 0, top}, x, y, ppu) || x < -40 || x > 440 || y < 0 || y > 240) continue;
        const char* line = roam::roamer(o.id).greet[o.greetLine];
        const float a = clampf(o.greetT / 0.4f, 0.0f, 1.0f);
        const float w = textWidth(app, line, 0.45f) + 16;
        const float bx = clampf(x - w / 2, 4, 396 - w), by = clampf(y - 30, 4, 200);
        panel({bx, by, w, 20}, withAlpha(theme::kShell, 0.92f * a));
        C2D_DrawTriangle(x - 5, by + 19, withAlpha(theme::kShell, 0.92f * a), x + 5, by + 19, withAlpha(theme::kShell, 0.92f * a), x,
                         by + 26, withAlpha(theme::kShell, 0.92f * a), 0);
        textCentered(app, line, bx + w / 2, by + 10, 0.45f, withAlpha(theme::kDenPlum, a), w);
    }
}

void drawTop(App& app, const vext::Stage& stage) {
    State& s = st();
    if (s.mode == Mode::Duel) bview::drawTop(app);
    if (s.mode == Mode::Watch) drawOver(app, stage);
}

// "Duel?": who they are and their dragon (at your partner's level), yours, its Energy, the prize.
void drawAsk(App& app, const Input& in, const vext::Stage& stage) {
    State& s = st();
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    if (s.who < 0 || s.who >= s.count) return;
    const Out& o = s.out[s.who];
    const roam::Roamer& r = roam::roamer(o.id);
    char line[80];
    std::snprintf(line, sizeof(line), str::kDuelAskTitle, r.name);
    textCentered(app, line, 160, 22, 0.66f, theme::kClutchGold, 300, Face::Title);
    const int kind = findKind(r.kind) >= 0 ? findKind(r.kind) : 0;
    const int level = levelFor(app, stage, o.id);
    std::snprintf(line, sizeof(line), str::kBattleAskTheirs, r.dragonName, kindInfo(kind).title, level);
    textCentered(app, line, 160, 52, 0.46f, theme::kShell, 300);
    char elements[32];
    kindElements(kind, elements, sizeof(elements));
    textCentered(app, elements, 160, 70, 0.4f, withAlpha(theme::kShell, 0.75f), 300);
    textCentered(app, str::kDuelFair, 160, 86, 0.36f, withAlpha(theme::kShell, 0.6f), 300);
    if (stage.partner >= 0 && stage.partner < app.game.dragonCount) {
        const Dragon& d = app.game.dragons[stage.partner];
        std::snprintf(line, sizeof(line), str::kBattleAskYours, d.name, trainer::levelOf(d));
        textCentered(app, line, 160, 108, 0.46f, theme::kShell, 300);
        std::snprintf(line, sizeof(line), str::kBattleAskEnergy, static_cast<int>(d.needs.energy + 0.5f),
                      static_cast<int>(trainer::kEnergyBattle));
        textCentered(app, line, 160, 126, 0.4f,
                     trainer::canSpend(d, trainer::kEnergyBattle) ? withAlpha(theme::kShell, 0.75f) : theme::kRose, 300);
    }
    if (roam::paidToday(app.game, o.id, dayIndex(nowLocal(app)))) std::snprintf(line, sizeof(line), "%s", str::kDuelPrizeTaken);
    else std::snprintf(line, sizeof(line), str::kDuelPrize, static_cast<unsigned long>(roam::duelGleam(level)));
    textCentered(app, line, 160, 146, 0.38f, withAlpha(theme::kClutchGold, 0.9f), 300);
    if (button(app, {24, 172, 128, 48}, str::kBattleAskNot, in)) {
        s.mode = Mode::None;
        s.who = -1;
        audio::playSfx(audio::Sfx::Back);
    } else if (button(app, {168, 172, 128, 48}, str::kDuelAskGo, in, theme::kClutchGold)) {
        vext::Stage copy = stage;
        goDuel(app, copy);  // (the battle takes its places from the stage on its first update)
    }
}

void drawBottom(App& app, const Input& in, const vext::Stage& stage) {
    State& s = st();
    switch (s.mode) {
        case Mode::Duel: bview::drawBottom(app, in); break;
        case Mode::Ask: drawAsk(app, in, stage); break;
        case Mode::Watch: {
            verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
            char line[48];
            std::snprintf(line, sizeof(line), str::kDuelWatching, s.who >= 0 ? roam::roamer(s.out[s.who].id).name : "");
            textCentered(app, line, 160, 120, 0.6f, theme::kShell, 300, Face::Title);
            break;
        }
        case Mode::Words:
        case Mode::None: verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum); break;
    }
}

}  // namespace

const vext::Feature kRoamerFeature{"roamers", folk, act, active, update, view, drawTop, drawBottom, tick, ambient, drawOver};

void roamerCommand(App& app, const char* args) {
    char word[16] = {};
    int a = 0;
    float b = 0;
    const int n = std::sscanf(args, " %15s %d %f", word, &a, &b);
    if (n < 1) return;
    State& s = st();
    if (std::strcmp(word, "near") == 0) s.pendingNear = a;
    else if (std::strcmp(word, "talk") == 0) s.pendingTalk = a;
    else if (std::strcmp(word, "duel") == 0) s.pendingDuel = a;
    else if (std::strcmp(word, "watch") == 0) {
        s.pendingWatch = a;
        s.watchFor = n >= 3 && b > 0 ? b : 8.0f;
    } else if (std::strcmp(word, "day") == 0) {
        s.forceDay = a;  // (refreshed on the next frame)
        s.day = INT_MIN;
    } else if (std::strcmp(word, "level") == 0) {
        s.forceLevel = a > 0 ? (a > kMaxLevel ? kMaxLevel : a) : 0;
    } else if (std::strcmp(word, "list") == 0) {
        for (int k = 0; k < s.count; ++k) {
            const Out& o = s.out[k];
            autotest::log("roamer %d: %s (%s) at (%.1f %.1f) %s%s", k, roam::roamer(o.id).name, roam::roamer(o.id).kind, o.at.x, o.at.y,
                          o.pose.walking ? "walking" : "paused", o.pose.sitting ? ", sitting" : "");
        }
    }
    (void)app;
}

}  // namespace ec
