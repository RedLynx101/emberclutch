#include "app/battle_view.hpp"

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "app/audio.hpp"
#include "app/tips_ui.hpp"
#include "app/autotest.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/challenges.hpp"
#include "core/kinds.hpp"
#include "core/people.hpp"
#include "core/place_layout.hpp"
#include "core/rig.hpp"
#include "core/trainer.hpp"
#include "core/valley.hpp"

namespace ec::bview {
namespace {

using battle::Ev;
using battle::Event;

constexpr float kPi = 3.14159265f;

enum class Phase : u8 { Idle, FadeOut, Enter, Choose, Play, End, Results };

struct Figure {
    Animator anim;
    float blink = 0, blinkIn = 2;
};

struct Pop {  // a number rising from someone hit or healed
    Vec3 at;
    char text[12] = {};
    float t = -1;
    bool good = false;
};

struct Side {
    Dragon* dragon = nullptr;  // (drawn: the partner's copy, or the setup's foe)
    DenActor actor;
    ClipId clip = ClipId::Count;
    Vec3 home, pos;
    float heading = 0;
    float lungeT = -1, lunge = 0;  // a body move's run at the other and back (seconds in, how far)
    float knockT = -1;             // knocked back by a hit
    float flash = 0;               // a hit's white flash, fading
    float jaw = 0, jawHold = 0;    // its jaw's opening, and how long it's held open (a breath, a roar)
    float shownHp = 0;             // its health as the events so far have played
    float bar = 0;                 // the bar, draining toward it
    bool lying = false;            // tired out
};

struct State {
    Phase phase = Phase::Idle;
    float t = 0;  // seconds in this phase
    Setup setup;
    battle::Battle bt;
    Dragon pal;  // your partner as drawn
    Side side[2];
    Vec3 youAt;
    float youHeading = 0;
    Figure you, trainer;
    // This turn's events, one at a time.
    Event ev[battle::kMaxEvents];
    int evCount = 0, evAt = 0;
    float evT = 0, evLen = 0;
    bool faster = false;
    char line[80] = {};
    int cursor = 0;
    int picked = -1;  // a move tapped on the bottom screen
    bool askGiveUp = false;
    battle::Outcome outcome = battle::Outcome::Lost;
    Results results;
    // Effects and the camera.
    Particles fx;
    challenge::BreathFx breath;
    Pop pops[4];
    float shake = 0;
    Vec3 eye, target;
    bool camSnap = true;
    std::vector<Solid> solids;  // the places' solids near the battle, for the camera (gathered on its first frame)
    const Valley* solidsOf = nullptr;
    float fade = 0;
    bool foeShown = false;
    int foeOther = 0;  // the foe's place in the view's other dragons (its head: r3d::otherHead)
    float walkT = 0;  // the foe coming in
    float bannerT = 0;
};

State& st() {
    static State s;
    return s;
}

bool g_autoplay = false;
bool g_breathOnly = false;  // scripted runs: both sides breathe every turn (for looking at breath)

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
float smooth(float k) {
    k = clampf(k, 0.0f, 1.0f);
    return k * k * (3 - 2 * k);
}
float headingTo(Vec3 from, Vec3 to) { return std::atan2(to.x - from.x, -(to.y - from.y)); }
Vec3 forwardOf(float heading) { return {std::sin(heading), -std::cos(heading), 0}; }


// ---- The dragons' clips and the people's.
int formOf(const Dragon& d) { return d.stage == Stage::Hatchling ? kFormHatchling : kFormGrown; }

void playClip(Side& sd, ClipId c, float fade = 0.2f, bool restart = false, float rate = 1.0f) {
    if (!sd.dragon || (c == sd.clip && !restart)) return;
    const AnimLibrary* lib = r3d::animsFor(*sd.dragon);
    const int* clips = lib ? r3d::clipIndexFor(*sd.dragon, formOf(*sd.dragon)) : nullptr;
    const int index = clips ? clips[static_cast<int>(c)] : -1;
    if (index < 0) return;
    sd.actor.anim.play(index, fade, restart);
    sd.actor.anim.rate = rate;
    sd.clip = c;
}

bool clipDone(const Side& sd) {
    const AnimLibrary* lib = sd.dragon ? r3d::animsFor(*sd.dragon) : nullptr;
    return !lib || sd.actor.anim.finished(*lib);
}

void playPerson(Figure& f, const char* clip, bool restart = false) {
    const AnimLibrary* lib = r3d::personAnims();
    if (!lib) return;
    const int c = lib->find(clip);
    if (c >= 0) f.anim.play(c, 0.2f, restart);
}

bool personDone(const Figure& f) {
    const AnimLibrary* lib = r3d::personAnims();
    return !lib || f.anim.clip < 0 || f.anim.finished(*lib);
}

void stepFigure(App& app, Figure& f) {
    f.blinkIn -= app.dt;
    if (f.blinkIn <= 0) {
        f.blink = 1;
        f.blinkIn = 1.5f + app.rng.below(3500) * 0.001f;
    }
    f.blink = std::fmax(0.0f, f.blink - app.dt * 7.0f);
    if (const AnimLibrary* lib = r3d::personAnims()) {
        u8 events[4];
        f.anim.update(*lib, app.dt, events, 4);
        if (personDone(f)) playPerson(f, "idle");
    }
}

// ---- Where things are on the dragons.
float sizeOf(const Side& sd) { return sd.dragon ? dragonSize(*sd.dragon) : 1.0f; }

// The mouth: the head as it was last posed (yours the valley's partner, theirs its other dragon),
// a little forward along its heading; before it's been drawn, about where a head is.
Vec3 mouthOf(const Side& sd, int who, int foeOther) {
    const float size = sizeOf(sd);
    const Vec3 fwd = forwardOf(sd.heading);
    Vec3 head;
    const bool posed = who == 0 ? r3d::headOf(0, head) : r3d::otherHead(foeOther, head);
    if (posed && length(head - sd.pos) < 6.0f * size + 2.0f) return head + fwd * (0.35f * size);
    return sd.pos + Vec3{0, 0, 1.45f * size} + fwd * (1.5f * size);
}

Vec3 chestOf(const Side& sd) { return sd.pos + Vec3{0, 0, 0.8f * sizeOf(sd)} + forwardOf(sd.heading) * (0.5f * sizeOf(sd)); }
Vec3 topOf(const Side& sd) { return sd.pos + Vec3{0, 0, 1.9f * sizeOf(sd)}; }

audio::Sfx breathSound(int element) {
    switch (challenge::breathFor(element).kind) {
        case challenge::Breath::Flame: return audio::Sfx::BreathFlame;
        case challenge::Breath::Mist: return audio::Sfx::BreathMist;
        case challenge::Breath::Gust: return audio::Sfx::BreathGust;
        case challenge::Breath::Spores: return audio::Sfx::BreathSpores;
        case challenge::Breath::Frost: return audio::Sfx::BreathFrost;
        default: return audio::Sfx::BreathLight;
    }
}

// The line a battle shows: its first letter up ("the wild Gustling" starts a sentence).
void setLine(State& s, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(s.line, sizeof(s.line), fmt, args);
    va_end(args);
    if (s.line[0] >= 'a' && s.line[0] <= 'z') s.line[0] = static_cast<char>(s.line[0] - 'a' + 'A');
}

void pop(State& s, Vec3 at, int amount, bool good) {
    Pop* p = &s.pops[0];
    for (Pop& q : s.pops)
        if (q.t < 0 || q.t > p->t) p = &q;
    p->at = at;
    p->t = 0;
    p->good = good;
    std::snprintf(p->text, sizeof(p->text), good ? "+%d" : "-%d", amount);
}

// ---- The events: each starts (its line, sound, clip, effect), then plays for its length.
float beginEvent(App& app, State& s, const Event& e) {
    Side& me = s.side[e.side];
    Side& other = s.side[1 - e.side];
    const battle::Battler& b = s.bt.side[e.side];
    const battle::MoveInfo& m = battle::moveInfo(e.move);
    const float pitch = e.side == 0 ? 1.0f : 0.9f;
    switch (e.kind) {
        case Ev::Use: {
            setLine(s, str::kBattleUsed, b.name, m.name);
            if (e.side == 1 && s.setup.trainer) playPerson(s.trainer, "point", true);  // (sending it in: workstream D)
            if (e.side == 0) playPerson(s.you, "point", true);
            if (m.kind == battle::MoveKind::Body) {
                me.lungeT = 0;
                const float gap = length(other.home - me.home);
                me.lunge = std::fmax(0.3f, gap - kReachPerSize * (sizeOf(me) + sizeOf(other)));  // (nose to nose, no nearer)
                if (me.dragon && me.dragon->stage != Stage::Adult) me.lunge *= 0.6f;  // (on its lead)
                playClip(me, ClipId::Pounce, 0.12f, true, 1.25f);
                audio::playSfx(audio::Sfx::Swipe, pitch);
                return 0.5f;
            }
            if (m.kind == battle::MoveKind::Breath) {
                audio::playSfx(breathSound(m.element), pitch * (me.dragon && me.dragon->stage != Stage::Adult ? 1.15f : 1.0f));
                me.jawHold = 0.75f + 0.2f;  // open for the whole breath (and a moment after)
                return 0.75f;
            }
            if (m.effect == battle::Effect::Heal) playClip(me, ClipId::Shake, 0.2f, true);
            else if (m.effect == battle::Effect::RaiseMight || m.effect == battle::Effect::RaiseBreath ||
                     m.effect == battle::Effect::RaiseWit || m.effect == battle::Effect::RaiseWing ||
                     m.effect == battle::Effect::RaiseGuard)
                playClip(me, ClipId::WingFlutter, 0.2f, true);
            else {
                audio::playSfx(audio::Sfx::Rumble, pitch);  // a roar at them
                me.jawHold = 0.7f;
            }
            return 0.8f;
        }
        case Ev::Miss:
            if (m.kind == battle::MoveKind::Status) setLine(s, "%s", str::kBattleMissed);
            else setLine(s, str::kBattleDodged, b.name);
            playClip(me, ClipId::Hop, 0.12f, true);
            audio::playSfx(audio::Sfx::Whiff, pitch);
            return 0.75f;
        case Ev::Hit: {
            if (e.flags & battle::kCrit) setLine(s, "%s", str::kBattleCrit);
            else if (e.flags & battle::kStrong) setLine(s, "%s", str::kBattleStrong);
            else if (e.flags & battle::kWeak) setLine(s, "%s", str::kBattleWeak);
            const bool big = (e.flags & (battle::kCrit | battle::kStrong)) != 0;
            audio::playSfx(big ? audio::Sfx::HitBig : audio::Sfx::Hit, pitch);
            me.knockT = 0;
            me.flash = 1;
            s.shake = big ? 0.32f : 0.16f;
            pop(s, topOf(me), e.amount, false);
            s.fx.emit(Fx::Puff, chestOf(me), big ? 6 : 3, sizeOf(me) * 1.4f);
            if (big) {  // (a wince for their own, a fist pump for the other's: workstream D)
                const char* wince = (e.flags & battle::kCrit) ? "surprised" : "worried";
                if (e.side == 0 || s.setup.trainer) playPerson(e.side == 0 ? s.you : s.trainer, wince, true);
                if (e.side == 1 || s.setup.trainer) playPerson(e.side == 0 ? s.trainer : s.you, "fist_pump", true);
            }
            return (e.flags & (battle::kCrit | battle::kStrong | battle::kWeak)) ? 1.0f : 0.7f;
        }
        case Ev::StatUp:
        case Ev::StatDown: {
            const char* stat = str::kBattleStageNames[e.stat < 5 ? e.stat : 0];
            if (e.kind == Ev::StatUp) setLine(s, e.amount > 1 ? str::kBattleRoseSharply : str::kBattleRose, b.name, stat);
            else setLine(s, str::kBattleFell, b.name, stat);
            audio::playSfx(e.kind == Ev::StatUp ? audio::Sfx::StatUp : audio::Sfx::StatDown, pitch);
            s.fx.emit(e.kind == Ev::StatUp ? Fx::Sparkle : Fx::Puff, chestOf(me), 6, sizeOf(me) * 1.3f);
            return 0.95f;
        }
        case Ev::NoEffect:
            setLine(s, "%s", str::kBattleNothing);
            audio::playSfx(audio::Sfx::Error, 1.0f, 0.5f);
            return 0.8f;
        case Ev::Heal:
            setLine(s, str::kBattleHealed, b.name);
            audio::playSfx(audio::Sfx::StatUp, 1.25f);
            s.fx.emit(Fx::Heart, chestOf(me), 4, sizeOf(me) * 1.2f);
            pop(s, topOf(me), e.amount, true);
            return 0.95f;
        case Ev::Faint:
            setLine(s, str::kBattleTiredOut, b.name);
            audio::playSfx(audio::Sfx::Faint, e.side == 0 ? 1.0f : 0.9f);
            playClip(me, ClipId::LieDown, 0.25f, true);
            me.lying = true;
            return 1.6f;
        case Ev::TimeUp:
            setLine(s, str::kBattleTimeUp, s.bt.side[e.side].name);
            return 1.6f;
    }
    (void)app;
    return 0.8f;
}

void nextEvent(App& app, State& s) {
    if (s.evAt < s.evCount) {
        const Event& e = s.ev[s.evAt];
        s.evLen = beginEvent(app, s, e);
        s.evT = 0;
        // The health it shows from now on (the bar drains toward it while the event plays).
        if (e.kind == Ev::Hit) s.side[e.side].shownHp = std::fmax(0.0f, s.side[e.side].shownHp - e.amount);
        if (e.kind == Ev::Heal) s.side[e.side].shownHp += e.amount;
    }
}

void startTurn(App& app, State& s, int slot) {
    const int theirs = battle::chooseMove(s.bt, 1, s.setup.skill, app.rng);
    battle::resolveTurn(s.bt, slot, theirs, app.rng);
    s.evCount = s.bt.logCount;
    for (int i = 0; i < s.evCount; ++i) s.ev[i] = s.bt.log[i];
    s.evAt = 0;
    s.askGiveUp = false;
    s.phase = Phase::Play;
    s.t = 0;
    nextEvent(app, s);
}

void toChoose(State& s) {
    s.phase = Phase::Choose;
    s.t = 0;
    s.picked = -1;
    setLine(s, str::kBattleWhatWill, s.bt.side[0].name);
    for (int k = 0; k < 2; ++k) s.side[k].shownHp = static_cast<float>(s.bt.side[k].hp);  // (in step with the rules)
    if (!battle::usable(s.bt.side[0], s.cursor))
        for (int k = 0; k < kMoveSlots; ++k)
            if (battle::usable(s.bt.side[0], k)) {
                s.cursor = k;
                break;
            }
}

void toEnd(App& app, State& s, battle::Outcome o) {
    s.phase = Phase::End;
    s.t = 0;
    s.outcome = o;
    const bool won = o == battle::Outcome::Won;
    audio::playSfx(won ? audio::Sfx::Victory : audio::Sfx::Defeat);
    playPerson(s.you, won ? "cheer" : o == battle::Outcome::GaveUp ? "bow" : "slump", true);  // (workstream D: slump, bow)
    if (s.setup.trainer) playPerson(s.trainer, won ? "slump" : "cheer", true);
    Side& winner = s.side[won ? 0 : 1];
    if (o != battle::Outcome::GaveUp && !winner.lying) playClip(winner, ClipId::TailWag, 0.25f, true);
    if (won) s.fx.emit(Fx::Heart, chestOf(s.side[0]) + Vec3{0, 0, 0.6f}, 5, sizeOf(s.side[0]) * 1.2f);
    if (o == battle::Outcome::GaveUp) setLine(s, "%s", str::kBattleGaveUpTitle);
    else setLine(s, "%s", won ? str::kBattleWon : str::kBattleLost);
    (void)app;
}

void toResults(App& app, State& s) {
    s.phase = Phase::Results;
    s.t = 0;
    s.results = Results{};
    std::snprintf(s.results.title, sizeof(s.results.title), "%s",
                  s.outcome == battle::Outcome::Won ? str::kBattleWon
                  : s.outcome == battle::Outcome::Lost ? str::kBattleLost : str::kBattleGaveUpTitle);
    if (s.setup.finish) s.setup.finish(app, s.outcome, s.results);
    if (s.results.levelUp) {
        audio::playSfx(audio::Sfx::LevelUp);
        showTip(app, tips::kTipLevelUp);
    }
}

// ---- The camera: over your shoulder (the side the staging found clear), your dragon before you
// and theirs beyond, leaning toward the one about to be hit; a shake on the hits.
void frame(App& app, State& s, const Valley* v) {
    const Vec3 mid = (s.side[0].home + s.side[1].home) * 0.5f;
    Vec3 eye, target;
    cameraFor(s.setup, s.youAt, s.side[0].home, s.side[1].home, std::fmax(sizeOf(s.side[0]), sizeOf(s.side[1])), eye, target);
    if (s.phase == Phase::Play && s.evAt < s.evCount) {  // lean toward the one it happens to
        const Event& e = s.ev[s.evAt];
        const int to = e.kind == Ev::Use ? 1 - e.side : e.side;
        target = target + (s.side[to].home - mid) * 0.18f;
    }
    if (s.phase == Phase::Results || s.phase == Phase::End) {  // closer on the winner
        const int w = s.outcome == battle::Outcome::Won ? 0 : 1;
        target = target + (s.side[w].home - mid) * 0.3f;
        eye = eye + (s.side[w].home - eye) * 0.12f;
    }
    if (v) {  // (a spire, a wall, a slope or a bowl's rim in its way: nearer the dragons, then above the ground)
        if (s.solidsOf != v) {
            s.solids.clear();
            for (const Solid& w : worldSolids(*v))
                if (std::hypot(w.at.x - mid.x, w.at.y - mid.y) < 36.0f) s.solids.push_back(w);
            s.solidsOf = v;
        }
        for (int k = 0; k < 8 && !viewClear(*v, s.solids, eye, target); ++k) eye = eye + (target - eye) * 0.12f;
        eye.z = std::fmax(eye.z, v->heightAt(eye.x, eye.y) + 1.4f);
    }
    if (s.camSnap) {
        s.eye = eye;
        s.target = target;
        s.camSnap = false;
    } else {
        const float e = std::fmin(1.0f, app.dt * 2.5f);
        s.eye = s.eye + (eye - s.eye) * e;
        s.target = s.target + (target - s.target) * std::fmin(1.0f, e * 1.5f);
    }
}

// ---- Each frame: the dragons' places (a lunge, a knock back), their clips and eyes.
void stepSides(App& app, State& s, const Valley* v) {
    for (int k = 0; k < 2; ++k) {
        Side& sd = s.side[k];
        const Side& other = s.side[1 - k];
        if (!sd.dragon) continue;
        const Vec3 toward = normalize(Vec3{other.home.x - sd.home.x, other.home.y - sd.home.y, 0});
        const float gap = std::fmax(0.1f, std::hypot(other.home.x - sd.home.x, other.home.y - sd.home.y));
        Vec3 at = sd.home;
        float moved = 0;
        if (sd.lungeT >= 0) {  // out to the other (0.22 s), a beat there, then back (to 0.85 s)
            sd.lungeT += app.dt;
            const float t = sd.lungeT;
            const float out = t < 0.22f ? smooth(t / 0.22f) : t < 0.42f ? 1.0f : 1.0f - smooth((t - 0.42f) / 0.43f);
            moved = sd.lunge * out;
            if (t > 0.85f) sd.lungeT = -1;
        }
        if (sd.knockT >= 0) {
            sd.knockT += app.dt;
            moved -= 0.35f * std::sin(clampf(sd.knockT / 0.3f, 0.0f, 1.0f) * kPi);
            if (sd.knockT > 0.3f) sd.knockT = -1;
        }
        // (The battle's ground is where the feature stood them, a ring's top too: toward the other
        // it runs from its own height to theirs.)
        at = at + toward * moved;
        at.z = sd.home.z + (other.home.z - sd.home.z) * clampf(moved / gap, 0.0f, 1.0f);
        sd.pos = at;
        sd.flash = std::fmax(0.0f, sd.flash - app.dt * 4.0f);
        if (s.phase != Phase::Enter || k == 0) {
            const float want = headingTo(sd.home, other.home);
            sd.heading += clampf(std::remainder(want - sd.heading, 2 * kPi), -4.0f * app.dt, 4.0f * app.dt);
        }
        // Clips: a one-shot done goes back to standing (or lying, tired out).
        if (clipDone(sd) && sd.clip != ClipId::Idle && sd.clip != ClipId::Trot) {
            if (sd.lying) {
                if (sd.clip != ClipId::LieLoop) playClip(sd, ClipId::LieLoop, 0.3f);
            } else {
                playClip(sd, ClipId::Idle, 0.3f);
            }
        }
        if (const AnimLibrary* lib = r3d::animsFor(*sd.dragon)) {
            u8 events[8];
            sd.actor.anim.update(*lib, app.dt, events, 8);
        }
        sd.actor.eyes.update(sd.lying ? 1.0f : 0.0f, app.dt);
        // The jaw: wide open while it's held (a breath, a roar), eased shut after.
        const float jawRate = s.faster || g_autoplay ? 2.0f : 1.0f;  // (in step with the events)
        sd.jawHold -= app.dt * jawRate;
        sd.jaw = sd.jawHold > 0 ? std::fmin(1.3f, sd.jaw + app.dt * 10.0f) : std::fmax(0.0f, sd.jaw - app.dt * 5.0f);
        sd.actor.jawOpen = sd.jaw;
    }
    // A breath: its stream from the open mouth to the other's chest while the move is used (the
    // jaw opens first).
    if (s.phase == Phase::Play && s.evAt < s.evCount) {
        const Event& e = s.ev[s.evAt];
        const battle::MoveInfo& m = battle::moveInfo(e.move);
        Side& me = s.side[e.side];
        if (e.kind == Ev::Use && m.kind == battle::MoveKind::Breath && s.evT > 0.08f && s.evT < 0.5f && me.jaw > 0.6f &&
            s.breath.count() < 60) {
            s.breath.look = challenge::breathFor(m.element);
            s.breath.emit(s.breath.look, mouthOf(me, e.side, s.foeOther), chestOf(s.side[1 - e.side]), 2, 0.35f);
        }
    }
}

// ---- Drawing: the effects over the picture (as the challenges draw theirs).
void drawEffects(const State& s) {
    static const u32 kHeart = theme::kRose, kSpark = theme::kShell, kPuff = theme::rgba(0xE8, 0xDC, 0xC4);
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
                break;
            default: C2D_DrawCircleSolid(x, y, 0, sz * 0.5f, withAlpha(kPuff, a * 0.45f)); break;
        }
    }
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
            case challenge::Breath::Mist: C2D_DrawCircleSolid(x, y, 0, sz, withAlpha(k, a * 0.4f)); break;
            case challenge::Breath::Gust: {
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
    // A hit's flash on the one hit, and the numbers rising.
    for (int k = 0; k < 2; ++k) {
        const Side& sd = s.side[k];
        float x, y, ppu;
        if (sd.flash > 0.01f && r3d::project(chestOf(sd), x, y, ppu))
            C2D_DrawCircleSolid(x, y, 0, 1.3f * sizeOf(sd) * ppu, withAlpha(theme::kShell, 0.55f * sd.flash));
    }
}

void hpBar(float x, float y, float w, float h, float frac) {
    frac = clampf(frac, 0.0f, 1.0f);
    panel({x, y, w, h}, theme::kTrack);
    const u32 c = frac > 0.5f ? theme::kSkyTeal : frac > 0.2f ? theme::kClutchGold : theme::kRose;
    if (frac > 0) panel({x, y, w * frac, h}, c);
}

void elementDots(const battle::Battler& b, float x, float y) {
    const KindInfo& k = kindInfo(b.kind);
    for (int i = 0; i < k.elementCount && i < 2; ++i) {
        C2D_DrawCircleSolid(x + i * 11.0f, y, 0, 4.5f, theme::kDenPlum);
        C2D_DrawCircleSolid(x + i * 11.0f, y, 0, 3.5f, fromRgb(battle::elementColour(k.elements[i])));
    }
}

// A battler's panel: its name, level, elements and health.
void battlerPanel(App& app, const State& s, int k, float x, float y) {
    const battle::Battler& b = s.bt.side[k];
    panel({x, y, 184, k == 0 ? 46.0f : 38.0f}, withAlpha(theme::kDenPlum, 0.82f));
    char name[32];
    std::snprintf(name, sizeof(name), "%s", b.name);
    if (name[0] >= 'a' && name[0] <= 'z') name[0] = static_cast<char>(name[0] - 'a' + 'A');
    text(app, name, x + 8, y + 4, 0.48f, theme::kShell, C2D_AlignLeft, 118);
    char lv[12];
    std::snprintf(lv, sizeof(lv), "Lv %d", b.level);
    text(app, lv, x + 176, y + 4, 0.44f, theme::kClutchGold, C2D_AlignRight);
    elementDots(b, x + 136, y + 11);
    hpBar(x + 8, y + 25, 168, 7, s.side[k].bar / b.maxHp);
    if (k == 0) {
        char hp[24];
        std::snprintf(hp, sizeof(hp), "%d / %d", static_cast<int>(s.side[k].bar + 0.5f), b.maxHp);
        text(app, hp, x + 176, y + 32, 0.38f, withAlpha(theme::kShell, 0.85f), C2D_AlignRight);
    }
}

// The wheel of elements, small, for learning it: each beats the next; Lumen and Shade apart.
void wheel(float cx, float cy, float r) {
    static constexpr u8 kOrder[6] = {battle::kEmber, battle::kFrost, battle::kStone, battle::kGale, battle::kGrove, battle::kTide};
    for (int i = 0; i < 6; ++i) {
        const float a = -kPi / 2 + i * kPi / 3, b = a + kPi / 3;
        const float x0 = cx + std::cos(a) * r, y0 = cy + std::sin(a) * r, x1 = cx + std::cos(b) * r, y1 = cy + std::sin(b) * r;
        const u32 c = withAlpha(theme::kShell, 0.45f);
        C2D_DrawLine(x0, y0, c, x1, y1, c, 1.5f, 0);
        const float mx = x0 + (x1 - x0) * 0.62f, my = y0 + (y1 - y0) * 0.62f;  // an arrowhead along it
        const float dx = (x1 - x0) / r, dy = (y1 - y0) / r;
        C2D_DrawTriangle(mx + dx * 3, my + dy * 3, c, mx - dx * 2 - dy * 3, my - dy * 2 + dx * 3, c, mx - dx * 2 + dy * 3,
                         my - dy * 2 - dx * 3, c, 0);
    }
    for (int i = 0; i < 6; ++i) {
        const float a = -kPi / 2 + i * kPi / 3;
        C2D_DrawCircleSolid(cx + std::cos(a) * r, cy + std::sin(a) * r, 0, 5.0f, theme::kDenPlum);
        C2D_DrawCircleSolid(cx + std::cos(a) * r, cy + std::sin(a) * r, 0, 4.0f, fromRgb(battle::elementColour(kOrder[i])));
    }
    const float px = cx + r + 14;
    C2D_DrawCircleSolid(px, cy - 6, 0, 4.0f, fromRgb(battle::elementColour(battle::kLumen)));
    C2D_DrawCircleSolid(px, cy + 6, 0, 4.0f, fromRgb(battle::elementColour(battle::kShade)));
    C2D_DrawLine(px, cy - 2, withAlpha(theme::kShell, 0.45f), px, cy + 2, withAlpha(theme::kShell, 0.45f), 1.5f, 0);
}

}  // namespace

// ---------------------------------------------------------------------------- the card
void Results::add(const char* fmt, ...) {
    if (count >= 7) return;
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(lines[count], sizeof(lines[count]), fmt, args);
    va_end(args);
    ++count;
}

// ---------------------------------------------------------------------------- staging
// Where the camera stands for a battle (before any leaning or shaking): behind you and a little
// to one side, high enough to see over you, looking past your dragon at theirs.
void cameraFor(const Setup& setup, Vec3 you, Vec3 pal, Vec3 foe, float size, Vec3& eye, Vec3& target) {
    const Vec3 a = normalize(Vec3{foe.x - pal.x, foe.y - pal.y, 0});
    const Vec3 side{a.y * setup.camSide, -a.x * setup.camSide, 0};
    const float big = clampf(size, 0.6f, 1.6f);
    const float k = 0.8f + 0.3f * big;
    const float r = setup.camReach;
    eye = you - a * (4.8f * k * r) + side * (5.4f * k * r) + Vec3{0, 0, (3.4f + 1.0f * big) * (0.8f + 0.2f * r)};
    target = lerp(pal, foe, 0.5f) + Vec3{0, 0, 0.8f + 0.5f * big};
}

bool viewClear(const Valley& v, const std::vector<Solid>& solids, Vec3 eye, Vec3 target) {
    if (v.heightAt(eye.x, eye.y) > eye.z - 1.4f) return false;
    const Vec2 a{eye.x, eye.y}, d{target.x - eye.x, target.y - eye.y};
    const float len2 = d.x * d.x + d.y * d.y;
    for (const Solid& w : solids) {
        // (the nearer 65% of the line: past that it's among the dragons, where nothing stands)
        const float t = len2 > 1e-4f ? clampf(((w.at.x - a.x) * d.x + (w.at.y - a.y) * d.y) / len2, 0.0f, 0.65f) : 0.0f;
        const float gap = std::hypot(a.x + d.x * t - w.at.x, a.y + d.y * t - w.at.y);
        if (gap < w.radius + (t < 0.01f ? 1.2f : 0.4f)) return false;
    }
    for (int k = 1; k < 8; ++k) {  // the line above the ground all the way (a rim between)
        const Vec3 p = lerp(eye, target, k / 8.0f);
        if (v.heightAt(p.x, p.y) > p.z - 0.6f) return false;
    }
    return true;
}

float dragonSize(const Dragon& d) {
    const float stage = d.stage == Stage::Adult ? 1.0f : d.stage == Stage::Adolescent ? 0.8f : d.stage == Stage::Juvenile ? 0.6f : 0.45f;
    return kindSize(d) * stage;
}

bool goodGround(const Valley& v, Vec3 from, Vec2 at) {
    if (!v.inside(at.x, at.y)) return false;
    const float h = v.heightAt(at.x, at.y);
    return h > v.water - 0.2f && std::fabs(h - from.z) < 1.6f;
}

// ---------------------------------------------------------------------------- the battle
void start(App& app, const Setup& setup) {
    showTip(app, tips::kTipBattle);
    State& s = st();
    const Phase was = s.phase;
    s = State{};
    s.setup = setup;
    if (setup.partner < 0 || setup.partner >= app.game.dragonCount) return;
    s.pal = app.game.dragons[setup.partner];
    battle::Battler mine = battle::makeBattler(s.pal), theirs = battle::makeBattler(s.setup.foe);
    std::snprintf(theirs.name, sizeof(theirs.name), "%s", setup.foeName[0] ? setup.foeName : theirs.name);
    battle::begin(s.bt, mine, theirs);
    for (int k = 0; k < 2 && g_breathOnly; ++k) {  // (a scripted run's look at breath: nothing but)
        for (int slot = 0; slot < kMoveSlots; ++slot)
            if (battle::validMove(s.bt.side[k].moves[slot]) &&
                battle::moveInfo(s.bt.side[k].moves[slot]).kind == battle::MoveKind::Breath) {
                const u8 breath = s.bt.side[k].moves[slot];
                for (u8& m : s.bt.side[k].moves) m = breath;
                break;
            }
    }
    s.side[0].dragon = &s.pal;
    s.side[1].dragon = &s.setup.foe;
    s.side[0].home = s.side[0].pos = setup.palAt;
    s.side[1].home = setup.foeAt;
    s.side[1].pos = setup.foeFrom;
    s.side[0].heading = headingTo(setup.palAt, setup.foeAt);
    s.side[1].heading = headingTo(setup.foeFrom, setup.foeAt);
    for (int k = 0; k < 2; ++k) s.side[k].shownHp = s.side[k].bar = static_cast<float>(s.bt.side[k].maxHp);
    s.youAt = setup.youAt;
    s.youHeading = headingTo(setup.youAt, setup.foeAt);
    s.phase = was == Phase::Idle ? Phase::FadeOut : Phase::Enter;  // (floor to floor in the Hollow: no fade)
    s.fade = 0;
    s.camSnap = true;
    playPerson(s.you, "idle");
    playPerson(s.trainer, s.setup.trainer ? "bow" : "idle");  // (a trainer bows as it begins: workstream D)
    s.fx.clear();
    s.breath.clear();
    if (s.phase == Phase::Enter) audio::playSfx(audio::Sfx::BattleStart);
}

bool running() { return st().phase != Phase::Idle; }
void lastCamera(Vec3& eye, Vec3& target) {
    eye = st().eye;
    target = st().target;
}
void stop() { st().phase = Phase::Idle; }
void setAutoplay(bool on) { g_autoplay = on; }
void setBreathOnly(bool on) { g_breathOnly = on; }
bool autoplay() { return g_autoplay; }

void update(App& app, const Input& in, vext::Stage& stage) {
    State& s = st();
    if (s.phase == Phase::Idle) return;
    const Valley* v = stage.valley;
    s.t += app.dt;
    s.fx.update(app.dt);
    s.breath.update(app.dt);
    for (Pop& p : s.pops)
        if (p.t >= 0 && (p.t += app.dt) > 1.0f) p.t = -1;
    s.shake *= std::exp(-app.dt * 7.0f);
    for (Side& sd : s.side) {  // the bars drain toward what they show
        const float step = std::fmax(8.0f, std::fabs(sd.shownHp - sd.bar) * 4.0f) * app.dt;
        sd.bar = sd.bar < sd.shownHp ? std::fmin(sd.shownHp, sd.bar + step) : std::fmax(sd.shownHp, sd.bar - step);
    }
    switch (s.phase) {
        case Phase::FadeOut:
            s.fade = clampf(s.t / 0.25f, 0.0f, 1.0f);
            if (s.t >= 0.25f) {
                s.phase = Phase::Enter;
                s.t = 0;
                s.camSnap = true;
                audio::playSfx(audio::Sfx::BattleStart);
            }
            break;
        case Phase::Enter: {
            s.fade = clampf(1.0f - s.t / 0.35f, 0.0f, 1.0f);
            s.bannerT = s.t;
            // The foe comes in (from beside its trainer, out of the cave door) once it's loaded.
            s.foeShown = s.foeShown || r3d::kindReady(s.setup.foe.kind) || s.t > 2.5f;
            Side& foe = s.side[1];
            if (s.foeShown) {
                const float walk = std::fmax(0.3f, length(foe.home - s.setup.foeFrom) / 3.5f);
                s.walkT += app.dt;
                const float k = clampf(s.walkT / walk, 0.0f, 1.0f);
                foe.home = s.setup.foeAt;
                const Vec3 at = lerp(s.setup.foeFrom, s.setup.foeAt, smooth(k));
                foe.lungeT = -1;
                if (k < 1.0f) {
                    playClip(foe, ClipId::Trot);
                    foe.heading = headingTo(at, s.setup.foeAt);
                } else if (foe.clip == ClipId::Trot) {
                    playClip(foe, ClipId::Idle, 0.3f);
                }
                foe.pos = at;
                if (k >= 1.0f && s.t > 1.8f) toChoose(s);
            }
            break;
        }
        case Phase::Choose: {
            if (s.askGiveUp) {
                if (in.down & KEY_B) s.askGiveUp = false;
                break;
            }
            int pick = s.picked;
            s.picked = -1;
            if (in.down & (KEY_DLEFT | KEY_DRIGHT)) s.cursor ^= 1;
            if (in.down & (KEY_DUP | KEY_DDOWN)) s.cursor ^= 2;
            if (in.down & (KEY_DLEFT | KEY_DRIGHT | KEY_DUP | KEY_DDOWN)) audio::playSfx(audio::Sfx::Tap, 1.1f, 0.6f);
            if (in.down & KEY_A) pick = s.cursor;
            if (g_autoplay && s.t > 0.7f) pick = battle::chooseMove(s.bt, 0, 3, app.rng);
            if (pick >= 0) {
                if (battle::usable(s.bt.side[0], pick)) {
                    audio::playSfx(audio::Sfx::Confirm);
                    s.cursor = pick;
                    startTurn(app, s, pick);
                } else {
                    audio::playSfx(audio::Sfx::Error);
                }
            }
            break;
        }
        case Phase::Play: {
            if ((in.down & KEY_A) || in.tapped) s.faster = true;
            s.evT += app.dt * (s.faster || g_autoplay ? 2.0f : 1.0f);
            if (s.evT >= s.evLen) {
                ++s.evAt;
                if (s.evAt < s.evCount) {
                    nextEvent(app, s);
                } else if (s.bt.over) {
                    s.faster = false;
                    toEnd(app, s, s.bt.winner == 0 ? battle::Outcome::Won : battle::Outcome::Lost);
                } else {
                    s.faster = false;
                    toChoose(s);
                }
            }
            break;
        }
        case Phase::End:
            if (s.t > 2.0f || (s.t > 0.6f && (in.down & KEY_A))) toResults(app, s);
            break;
        case Phase::Results:
            if ((s.t > 0.8f && (in.down & KEY_A)) || (g_autoplay && s.t > 3.0f)) {
                audio::playSfx(audio::Sfx::Confirm);
                const battle::Outcome o = s.outcome;
                void (*done)(App&, battle::Outcome) = s.setup.done;
                s.phase = Phase::Idle;
                if (done) done(app, o);  // (may start another battle: the Hollow's next floor)
                return;
            }
            break;
        case Phase::Idle: break;
    }
    stepSides(app, s, v);
    stepFigure(app, s.you);
    stepFigure(app, s.trainer);
    frame(app, s, v);
    // The stage lent by the valley: you where you stand, your partner at its place, our camera.
    if (s.phase != Phase::FadeOut) {
        stage.you = s.youAt;
        stage.youHeading = s.youHeading;
        stage.pal = s.side[0].home;
        stage.palHeading = s.side[0].heading;
    }
    const float wobble = s.shake;
    const Vec3 jolt{std::sin(app.t * 53.0f) * wobble * 0.5f, std::cos(app.t * 47.0f) * wobble * 0.5f, std::sin(app.t * 61.0f) * wobble * 0.3f};
    stage.camSet = s.phase != Phase::FadeOut;
    stage.eye = s.eye + jolt;
    stage.target = s.target + jolt * 0.6f;
}

void view(App& app, const vext::Stage& stage, r3d::ValleyView& view) {
    State& s = st();
    if (s.phase == Phase::Idle || s.phase == Phase::FadeOut) return;
    r3d::wantKind(s.setup.foe.kind);  // (read ahead while it comes in)
    const Valley* v = stage.valley;
    // Your partner (our copy and our clips), where the battle puts it.
    view.dragon = &s.pal;
    view.actor = &s.side[0].actor;
    view.at = s.side[0].pos;
    view.heading = s.side[0].heading;
    view.pitch = view.roll = 0;
    view.riderOn = false;
    if (v) view.shadowAt = {view.at.x, view.at.y, std::fmax(v->heightAt(view.at.x, view.at.y), v->water)};
    // Theirs.
    if (s.foeShown && view.otherCount < r3d::kMaxOthers) {
        s.foeOther = view.otherCount;
        r3d::ValleyDragon& o = view.others[view.otherCount++];
        o.dragon = &s.setup.foe;
        o.actor = &s.side[1].actor;
        o.at = s.side[1].pos;
        o.heading = s.side[1].heading;
        o.scale = s.setup.foeScale;
        o.lodFar = 9.0f;  // (close up it's the lighter model past ~9 m: the top screen's budget)
    }
    // You, and a challenger behind their dragon.
    if (view.peopleCount > 0) {
        r3d::PersonView& me = view.people[0];
        me.at = s.youAt;
        me.heading = s.youHeading;
        me.anim = &s.you.anim;
        me.blink = s.you.blink;
        me.seated = false;
    }
    view.you = s.youAt;
    view.youHeading = s.youHeading;
    view.youSpeed = 0;
    // Only you and the challenger: the rest of the valley's people away while it's on (the top
    // screen's budget: two dragons and two people close up).
    view.peopleCount = view.peopleCount > 0 ? 1 : 0;
    if (s.setup.trainer) {
        const int slot = view.peopleCount < r3d::kMaxPeopleShown ? view.peopleCount++ : r3d::kMaxPeopleShown - 1;
        r3d::PersonView& p = view.people[slot];
        p = s.setup.trainerLook;
        p.heading = headingTo(p.at, s.youAt);
        p.anim = &s.trainer.anim;
        p.blink = s.trainer.blink;
        if (autotest::shooting())
            autotest::log("battle people %d, trainer in slot %d at (%.1f %.1f %.1f) form %d; you (%.1f %.1f) eye (%.1f %.1f %.1f)",
                          view.peopleCount, slot, p.at.x, p.at.y, p.at.z, p.form, s.youAt.x, s.youAt.y, view.eye.x, view.eye.y,
                          view.eye.z);
    }
    (void)app;
}

void drawTop(App& app) {
    State& s = st();
    if (s.phase == Phase::Idle) return;
    drawEffects(s);
    for (const Pop& p : s.pops) {  // the numbers, rising and fading
        float x, y, ppu;
        if (p.t < 0 || !r3d::project(p.at + Vec3{0, 0, p.t * 0.8f}, x, y, ppu)) continue;
        const float a = p.t < 0.7f ? 1.0f : 1.0f - (p.t - 0.7f) / 0.3f;
        text(app, p.text, x + 1, y + 1, 0.62f, withAlpha(theme::kDenPlum, a * 0.8f), C2D_AlignCenter);
        text(app, p.text, x, y, 0.62f, withAlpha(p.good ? theme::kSkyTeal : theme::kShell, a), C2D_AlignCenter);
    }
    if (s.phase != Phase::FadeOut && s.phase != Phase::Results) {
        battlerPanel(app, s, 1, 8, app.overlay ? 96.0f : 8.0f);  // (below the dev overlay's lines)
        battlerPanel(app, s, 0, 208, 140);
        // What happened, in the box along the bottom.
        panel({8, 192, 384, 40}, withAlpha(theme::kDenPlum, 0.85f));
        text(app, s.line, 20, 204, 0.5f, theme::kShell, C2D_AlignLeft, 360);
    }
    if (s.phase == Phase::Enter && s.bannerT < 2.2f && s.setup.intro[0]) {  // the banner as it begins
        const float a = clampf(std::fmin(s.bannerT / 0.25f, (2.2f - s.bannerT) / 0.4f), 0.0f, 1.0f);
        const float w = textWidth(app, s.setup.intro, 0.72f, Face::Title) + 40;
        panel({200 - w / 2, 84, w, 38}, withAlpha(theme::kEmber, 0.9f * a));
        textCentered(app, s.setup.intro, 200, 103, 0.72f, withAlpha(theme::kShell, a), 380, Face::Title);
    }
    if (s.phase == Phase::Results) {  // the card
        const float k = clampf(s.t * 3.0f, 0.0f, 1.0f);
        const float h = 44.0f + 18.0f * s.results.count;
        const Rect card{60, 28 + (1 - k) * 50, 280, h};
        const u32 edge = s.outcome == battle::Outcome::Won ? theme::kClutchGold : theme::kAsh;
        panel({card.x - 3, card.y - 3, card.w + 6, card.h + 6}, withAlpha(edge, 0.95f * k));
        panel(card, withAlpha(theme::kShell, 0.96f * k));
        textCentered(app, s.results.title, 200, card.y + 18, 0.68f, withAlpha(theme::kDenPlum, k), 260, Face::Title);
        for (int i = 0; i < s.results.count; ++i)
            textCentered(app, s.results.lines[i], 200, card.y + 42 + i * 18.0f, 0.44f, withAlpha(theme::kDenPlum, k), 266);
    }
    if (s.fade > 0.01f) C2D_DrawRectSolid(0, 0, 0, kTopW, kScreenH, withAlpha(theme::kDenPlum, s.fade));
    if (autotest::shooting())
        autotest::log("battle phase %d turn %d: %lu triangles, %lu draws; hp %d/%d vs %d/%d", static_cast<int>(s.phase), s.bt.turn,
                      static_cast<unsigned long>(app.stats.tris), static_cast<unsigned long>(app.stats.draws), s.bt.side[0].hp,
                      s.bt.side[0].maxHp, s.bt.side[1].hp, s.bt.side[1].maxHp);
}

void drawBottom(App& app, const Input& in) {
    State& s = st();
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    if (s.phase == Phase::Idle) return;
    const battle::Battler& me = s.bt.side[0];
    const battle::Battler& foe = s.bt.side[1];
    char line[80];
    if (s.phase == Phase::Results) {
        text(app, s.results.title, 160, 14, 0.62f, theme::kClutchGold, C2D_AlignCenter, 300, Face::Title);
        // Its level and the way to the next.
        const Dragon& d = app.game.dragons[s.setup.partner < app.game.dragonCount ? s.setup.partner : 0];
        u32 into = 0, span = 0;
        trainer::levelProgress(d, into, span);
        std::snprintf(line, sizeof(line), "%s  Lv %d", d.name, trainer::levelOf(d));
        text(app, line, 24, 58, 0.5f, theme::kShell, C2D_AlignLeft, 200);
        hpBar(24, 80, 272, 9, span ? static_cast<float>(into) / span : 1.0f);
        if (span) {
            std::snprintf(line, sizeof(line), "%lu / %lu exp to level %d", static_cast<unsigned long>(into),
                          static_cast<unsigned long>(span), trainer::levelOf(d) + 1);
            text(app, line, 296, 94, 0.4f, withAlpha(theme::kShell, 0.8f), C2D_AlignRight, 272);
        }
        std::snprintf(line, sizeof(line), "Energy %d", static_cast<int>(d.needs.energy + 0.5f));
        text(app, line, 24, 94, 0.4f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft, 120);
        if (s.t > 0.8f && button(app, {80, 190, 160, 38}, str::kBattleContinue, in, theme::kClutchGold)) {
            audio::playSfx(audio::Sfx::Confirm);
            const battle::Outcome o = s.outcome;
            void (*done)(App&, battle::Outcome) = s.setup.done;
            s.phase = Phase::Idle;
            if (done) done(app, o);
        }
        return;
    }
    if (s.phase == Phase::Choose && !s.askGiveUp) {
        text(app, s.line, 12, 6, 0.5f, theme::kClutchGold, C2D_AlignLeft, 296);  // ("What will it do?")
    } else {
        text(app, s.phase == Phase::Play ? str::kBattleFaster : str::kBattleWatching, 12, 6, 0.44f,
             withAlpha(theme::kShell, 0.6f), C2D_AlignLeft, 296);
    }
    // The four moves.
    for (int k = 0; k < kMoveSlots; ++k) {
        const Rect r{8.0f + (k & 1) * 156.0f, 30.0f + (k >> 1) * 76.0f, 148, 70};
        const int id = me.moves[k];
        const bool there = battle::validMove(id);
        const bool ok = there && battle::usable(me, k);
        const bool on = s.phase == Phase::Choose && s.cursor == k && !s.askGiveUp;
        if (on) panel({r.x - 3, r.y - 3, r.w + 6, r.h + 6}, theme::kClutchGold);
        panel(r, withAlpha(theme::kShell, ok ? 0.18f : 0.08f));
        if (!there) {
            textCentered(app, "---", r.x + r.w / 2, r.y + r.h / 2, 0.5f, withAlpha(theme::kShell, 0.3f));
            continue;
        }
        const battle::MoveInfo& m = battle::moveInfo(id);
        panel({r.x, r.y, 8, r.h}, fromRgb(battle::elementColour(m.element)));
        text(app, m.name, r.x + 16, r.y + 6, 0.5f, withAlpha(theme::kShell, ok ? 1.0f : 0.45f), C2D_AlignLeft, r.w - 22);
        if (m.kind == battle::MoveKind::Status) std::snprintf(line, sizeof(line), "%s", m.blurb);
        else std::snprintf(line, sizeof(line), str::kBattlePowerAcc, battle::elementLabel(m.element), m.power, m.accuracy);
        text(app, line, r.x + 16, r.y + 28, 0.36f, withAlpha(theme::kShell, ok ? 0.85f : 0.4f), C2D_AlignLeft, r.w - 22);
        const char* tag = nullptr;
        u32 tagColour = theme::kClutchGold;
        if (!ok) {
            tag = m.effect == battle::Effect::Heal ? str::kBattleUsedUp : str::kBattleNeedsBreather;
            tagColour = theme::kAsh;
        } else if (m.kind != battle::MoveKind::Status) {
            const float eff = battle::effectiveness(m.element, foe.kind);
            if (eff > 1.01f) tag = str::kBattleStrongTag;
            else if (eff < 0.99f) {
                tag = str::kBattleWeakTag;
                tagColour = theme::kAsh;
            }
        }
        if (tag) text(app, tag, r.x + r.w - 8, r.y + 50, 0.4f, tagColour, C2D_AlignRight, r.w - 20);
        if (s.phase == Phase::Choose && !s.askGiveUp && in.released && r.contains(in.rx, in.ry)) {
            s.cursor = k;
            s.picked = k;
        }
    }
    // Giving up, and the wheel.
    if (s.phase == Phase::Choose && !s.askGiveUp && button(app, {8, 196, 96, 36}, str::kBattleGiveUp, in)) {
        s.askGiveUp = true;
        audio::playSfx(audio::Sfx::Tap);
    }
    wheel(236, 212, 17);
    text(app, str::kBattleHelp, 118, 184, 0.34f, withAlpha(theme::kShell, 0.55f), C2D_AlignLeft, 190);
    if (s.askGiveUp) {
        panel({40, 70, 240, 104}, withAlpha(theme::kDenPlum, 0.96f));
        textCentered(app, str::kBattleGiveUpAsk, 160, 96, 0.52f, theme::kShell, 220);
        if (button(app, {52, 124, 100, 36}, str::kBattleYes, in)) {
            s.askGiveUp = false;
            audio::playSfx(audio::Sfx::Back);
            toEnd(app, s, battle::Outcome::GaveUp);
        } else if (button(app, {168, 124, 100, 36}, str::kBattleNo, in, theme::kClutchGold)) {
            s.askGiveUp = false;
            audio::playSfx(audio::Sfx::Tap);
        }
    }
}

}  // namespace ec::bview
