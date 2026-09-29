#include "app/wildlife.hpp"

#include <citro2d.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "app/audio.hpp"
#include "app/autotest.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/clock.hpp"
#include "core/critters.hpp"
#include "core/daylight.hpp"
#include "core/valley.hpp"

namespace ec::wildlife {
namespace {

using critters::Kind;
using critters::PalMove;

struct State {
    critters::Life life;
    critters::Mesh mesh;
    critters::Offer offer;
    critters::Around around;   // this frame's (a moment begins from it)
    const Valley* valley = nullptr;
    float lastTick = -1;       // app.t at the last tick (a gap: a new visit, the day's critters)
    float builtAt = -1;        // app.t the mesh was built for (the second eye's picture reuses it)
    float ambientFor = 0;      // to the next chirp, croak or quack allowed (they never pile up)
    int partner = -1;
    float camYaw = 0;          // the way the camera looks (the autotest's spawns go in front of it)
    PalMove move = PalMove::Free;
    float moveT = 0;           // seconds into the partner's current part
    float cheerFor = 0;        // you cheering (a chase that ended with everyone happy)
    int journalPick = -1;      // the Journal's row tapped
};

State& st() {
    static State s;
    return s;
}

int indexOf(Kind k) { return static_cast<int>(k) < critters::kKinds ? static_cast<int>(k) : 0; }

// A critter's sound, softer the further off it is (and the ambient ones spaced out).
void hear(State& s, const critters::Event& e) {
    using audio::Sfx;
    using critters::Ev;
    const float d = std::hypot(e.at.x - s.around.you.x, e.at.y - s.around.you.y);
    const float near = std::fmin(1.0f, std::fmax(0.0f, 1.25f - d / 22.0f));
    if (near <= 0.02f) return;
    const bool ambient = e.ev == Ev::Chirp || e.ev == Ev::Croak || e.ev == Ev::Quack;
    if (ambient && !s.life.moment.active()) {
        if (s.ambientFor > 0) return;
        s.ambientFor = 0.35f;
    }
    switch (e.ev) {
        case Ev::Chirp: audio::playSfx(Sfx::BirdChirp, e.pitch, 0.55f * near); break;
        case Ev::Flutter: audio::playSfx(Sfx::BirdFlutter, e.pitch, std::fmax(0.3f, 0.85f * near)); break;
        case Ev::Hop: audio::playSfx(Sfx::RabbitHop, 1.0f, 0.5f * near); break;
        case Ev::Croak: audio::playSfx(Sfx::FrogCroak, e.pitch, 0.6f * near); break;
        case Ev::Splash: audio::playSfx(Sfx::Splash, 1.5f, 0.5f * near); break;
        case Ev::Quack: audio::playSfx(Sfx::DuckQuack, e.pitch, 0.55f * near); break;
        case Ev::Rustle: audio::playSfx(Sfx::LeafRustle, 1.0f, 0.6f * near); break;
        case Ev::Yip: audio::playSfx(Sfx::FoxYip, 1.0f, 0.75f); break;
        case Ev::Shimmer: audio::playSfx(Sfx::ButterflyLand, 1.0f, 0.7f); break;
        case Ev::YouWhistle: audio::playSfx(Sfx::Whistle, 1.0f, 0.8f); break;
        case Ev::YouCroak: audio::playSfx(Sfx::FrogCroak, 0.72f, 0.75f); break;
        default: break;
    }
}

// A moment's heart: the Journal, the rewards and their toasts.
void befriended(App& app, State& s, Kind k) {
    Dragon* partner = s.partner >= 0 && s.partner < app.game.dragonCount ? &app.game.dragons[s.partner] : nullptr;
    const critters::Reward r = critters::befriend(app.game, partner, k, dayIndex(nowLocal(app)));
    audio::playSfx(audio::Sfx::CritterFriend);
    const int i = indexOf(k);
    if (k == Kind::Butterfly && !partner) showToast(app, str::kCritterFriendAlone);
    else showToastf(app, str::kCritterFriend[i], partner ? partner->name : "");
    static char line[64];
    if (r.firstEver) {
        std::snprintf(line, sizeof(line), str::kCritterFirst, str::kCritterName[i], r.gleam);
        queueToastf(app, "%s", line);
    } else if (r.gleam > 0) {
        std::snprintf(line, sizeof(line), str::kCritterDaily, r.gleam);
        queueToastf(app, "%s", line);
    }
    if (k == Kind::Rabbit || k == Kind::SnowHare) s.cheerFor = 1.6f;
    saveNow(app);
}

ClipId clipFor(PalMove m, float t, bool baby) {
    switch (m) {
        case PalMove::Stalk: return ClipId::Stalk;
        case PalMove::Pounce: return ClipId::Pounce;
        case PalMove::Run: return baby ? ClipId::Scamper : ClipId::Gallop;
        case PalMove::Sit: return t < 0.7f ? ClipId::Sit : ClipId::SitLoop;
        case PalMove::Sneeze: return ClipId::Sneeze;
        case PalMove::Sniff: return ClipId::Nuzzle;
        case PalMove::Happy: return t < 0.9f ? ClipId::PlayBow : ClipId::TailWag;
        case PalMove::Free: break;
    }
    return ClipId::Count;
}

// A little drawing of each for the Journal (theme colours, a few shapes).
void icon(Kind k, float x, float y, bool known) {
    namespace p = theme::paint;
    if (!known) {
        C2D_DrawCircleSolid(x, y, 0.5f, 5.5f, withAlpha(theme::kShell, 0.16f));
        return;
    }
    switch (k) {
        case Kind::Songbird:
            C2D_DrawCircleSolid(x - 1, y + 1, 0.5f, 4.5f, fromRgb(p::kWoodLight));
            C2D_DrawCircleSolid(x + 3, y - 2, 0.5f, 3.0f, fromRgb(p::kWood));
            C2D_DrawTriangle(x + 5.5f, y - 3, fromRgb(p::kCoin), x + 5.5f, y - 0.5f, fromRgb(p::kCoin), x + 8.5f, y - 1.8f,
                             fromRgb(p::kCoin), 0.5f);
            break;
        case Kind::Rabbit:
        case Kind::SnowHare: {
            const u32 c = fromRgb(k == Kind::Rabbit ? p::kWoodLight : p::kSnow);
            C2D_DrawRectSolid(x - 2.5f, y - 9, 0.5f, 2, 6, c);
            C2D_DrawRectSolid(x + 0.5f, y - 9, 0.5f, 2, 6, c);
            C2D_DrawCircleSolid(x, y + 1, 0.5f, 4.5f, c);
            break;
        }
        case Kind::Butterfly:
            C2D_DrawTriangle(x, y, fromRgb(p::kFlower[0]), x - 7, y - 5, fromRgb(p::kFlower[0]), x - 6, y + 4, fromRgb(p::kFlower[0]), 0.5f);
            C2D_DrawTriangle(x, y, fromRgb(p::kFlower[2]), x + 7, y - 5, fromRgb(p::kFlower[2]), x + 6, y + 4, fromRgb(p::kFlower[2]), 0.5f);
            break;
        case Kind::Frog:
            C2D_DrawCircleSolid(x, y + 1, 0.5f, 5.0f, fromRgb(p::kLeafLight));
            C2D_DrawCircleSolid(x - 2.5f, y - 3, 0.5f, 1.6f, theme::kDenPlum);
            C2D_DrawCircleSolid(x + 2.5f, y - 3, 0.5f, 1.6f, theme::kDenPlum);
            break;
        case Kind::Duck:
            C2D_DrawCircleSolid(x - 1, y + 1.5f, 0.5f, 4.5f, fromRgb(p::kCloud));
            C2D_DrawCircleSolid(x + 2.5f, y - 3, 0.5f, 2.6f, fromRgb(p::kCloud));
            C2D_DrawTriangle(x + 4.5f, y - 4, fromRgb(p::kSunset), x + 4.5f, y - 1.5f, fromRgb(p::kSunset), x + 7.5f, y - 2.5f,
                             fromRgb(p::kSunset), 0.5f);
            break;
        case Kind::Fox:
            C2D_DrawTriangle(x - 6, y - 3, fromRgb(p::kSunset), x + 6, y - 3, fromRgb(p::kSunset), x, y + 5, fromRgb(p::kSunset), 0.5f);
            C2D_DrawTriangle(x - 6, y - 3, fromRgb(p::kSunset), x - 3, y - 3, fromRgb(p::kSunset), x - 5, y - 8, fromRgb(p::kSunset), 0.5f);
            C2D_DrawTriangle(x + 3, y - 3, fromRgb(p::kSunset), x + 6, y - 3, fromRgb(p::kSunset), x + 5, y - 8, fromRgb(p::kSunset), 0.5f);
            C2D_DrawCircleSolid(x, y + 4, 0.5f, 1.2f, theme::kDenPlum);
            break;
        case Kind::Count: break;
    }
}

}  // namespace

void tick(App& app, const Valley& v, const Here& h) {
    State& s = st();
    const s64 now = nowLocal(app);
    if (s.valley != &v || s.lastTick < 0 || app.t - s.lastTick > 0.5f) {  // a new visit: the day's critters
        critters::reset(s.life, 0xC0FFEEu ^ (static_cast<u32>(dayIndex(now)) * 2654435761u));
        s.move = PalMove::Free;
        s.cheerFor = 0;
        s.offer = {};
    }
    s.valley = &v;
    s.lastTick = app.t;
    s.partner = h.partner;
    if (s.builtAt < 0) s.camYaw = h.youHeading;  // (till the first picture)
    critters::Around& a = s.around;
    a = critters::Around{};
    a.you = h.you;
    a.youSpeed = h.youSpeed;
    a.riding = h.riding;
    a.hasPal = h.partner >= 0;
    a.pal = h.pal;
    a.palSpeed = h.palSpeed;
    a.palHeading = h.palHeading;
    Vec3 head;  // its head as last drawn (the valley's partner is the renderer's dragon 0)
    if (a.hasPal && !h.riding && r3d::headOf(0, head) && std::hypot(head.x - h.pal.x, head.y - h.pal.y) < 6.0f) {
        a.palHead = head;
        a.palHeadSet = true;
    }
    const DayBlend b = dayBlend(now);
    a.day = b.weight(kLightDay);
    a.dusk = b.weight(kLightEvening);
    a.night = b.weight(kLightNight);
    const int f = vext::activeFeature(app);
    a.quiet = f >= 0 && std::strcmp(vext::feature(f).name, kFeature.name) != 0;  // (a battle, a show: none new)
    critters::update(s.life, v, a, app.dt);
    s.ambientFor -= app.dt;
    s.cheerFor -= app.dt;
    for (int k = 0; k < s.life.eventCount; ++k) {
        const critters::Event& e = s.life.events[k];
        if (e.ev == critters::Ev::Befriend) {
            befriended(app, s, e.kind);
        } else if (e.ev == critters::Ev::Seen) {
            if (!critters::seen(app.game, e.kind)) {  // the first ever: into the Journal
                critters::markSeen(app.game, e.kind);
                queueToastf(app, str::kCritterSpotted, str::kCritterName[indexOf(e.kind)]);
            }
        } else {
            hear(s, e);
        }
    }
}

bool offer(Vec3 you, Vec3 forward, bool hasPal) {
    State& s = st();
    s.offer = s.valley ? critters::offer(s.life, you, forward, hasPal) : critters::Offer{};
    return s.offer.who >= 0;
}

const char* prompt() {
    const State& s = st();
    return s.offer.who >= 0 ? str::kCritterPrompt[indexOf(s.life.c[s.offer.who].kind)] : nullptr;
}

void act(App& app) {
    State& s = st();
    if (s.valley && s.offer.who >= 0) critters::begin(s.life, *s.valley, s.offer, s.around);
    s.offer = {};
    (void)app;
}

void fillView(App& app, r3d::ValleyView& view) {
    State& s = st();
    if (!s.valley || s.valley != view.valley) return;
    if (s.builtAt != app.t) {
        critters::buildMesh(s.life, view.eye, view.target, s.mesh);
        s.builtAt = app.t;
    }
    const Vec3 look = view.target - view.eye;
    if (std::hypot(look.x, look.y) > 0.01f) s.camYaw = std::atan2(look.x, -look.y);
    view.critterPos = s.mesh.pos;
    view.critterCol = s.mesh.col;
    view.critterVerts = s.mesh.verts;
}

bool active(const App& app) {
    (void)app;
    const State& s = st();
    return s.valley && s.life.moment.active() && s.life.moment.takesPal;
}

void update(App& app, const Input& in, vext::Stage& stage) {
    State& s = st();
    critters::Moment& m = s.life.moment;
    // B lets it be; a trip (you somewhere else at once: a map pin, an autotest's goto) ends it too.
    if ((in.down & KEY_B) || std::hypot(stage.you.x - s.around.you.x, stage.you.y - s.around.you.y) > 5.0f) {
        critters::endMoment(s.life);
        return;
    }
    if (m.move != s.move) {
        s.move = m.move;
        s.moveT = 0;
    }
    s.moveT += app.dt;
    stage.pal = {m.pal.x, m.pal.y, stage.pal.z};
    stage.palHeading = m.palHeading;
    const bool baby = stage.shown && stage.shown->stage == ec::Stage::Hatchling;
    stage.palClip = clipFor(m.move, s.moveT, baby);
    stage.palClipRate = m.move == PalMove::Run ? 1.25f : 1.0f;
    stage.youClip = s.cheerFor > 0 ? "cheer" : "idle";
    // You turn to watch.
    if (m.who >= 0 && m.who < critters::kMaxCritters) {
        const Vec3 at = s.life.c[m.who].alive ? s.life.c[m.who].pos : m.pal;
        const float want = std::atan2(at.x - stage.you.x, -(at.y - stage.you.y));
        const float err = std::remainder(want - stage.youHeading, 6.2831853f);
        stage.youHeading += std::fmax(-3.0f * app.dt, std::fmin(3.0f * app.dt, err));
    }
    if (m.act == critters::Act::Still && s.around.palHeadSet && s.valley) {  // up close: the butterfly on its head
        const Vec3 head = s.around.palHead;
        const float h = std::fmax(0.4f, head.z - m.pal.z);
        const Vec3 fwd{std::sin(m.palHeading), -std::cos(m.palHeading), 0}, right{-std::cos(m.palHeading), -std::sin(m.palHeading), 0};
        stage.camSet = true;
        stage.eye = head + fwd * (1.0f + 1.3f * h) + right * (0.7f + 0.7f * h) + Vec3{0, 0, 0.35f + 0.4f * h};
        stage.eye.z = std::fmax(stage.eye.z, s.valley->heightAt(stage.eye.x, stage.eye.y) + 0.5f);
        stage.target = head + Vec3{0, 0, 0.28f * h};
    }
    // The meadow carries on under it (the scene's beds wait while a feature has the valley).
    audio::setBed(audio::Bed::Meadow, s.around.day + 0.5f * s.around.dusk);
    audio::setBed(audio::Bed::ValleyNight, s.around.night + 0.5f * s.around.dusk);
}

const vext::Feature kFeature{"critters", nullptr, nullptr, active, update, nullptr, nullptr, nullptr};

void drawJournal(App& app, const Input& in) {
    State& s = st();
    const SaveData& g = app.game;
    char line[80];
    int friends = 0;
    for (int k = 0; k < critters::kKinds; ++k) friends += critters::befriended(g, static_cast<Kind>(k));
    text(app, str::kCrittersTitle, 10, 59, 0.45f, theme::kClutchGold, C2D_AlignLeft, 180);
    std::snprintf(line, sizeof(line), str::kCrittersCount, friends, critters::kKinds);
    text(app, line, 310, 60, 0.38f, withAlpha(theme::kShell, 0.8f), C2D_AlignRight, 120);
    for (int k = 0; k < critters::kKinds; ++k) {
        const Kind kind = static_cast<Kind>(k);
        const bool known = critters::seen(g, kind), met = critters::befriended(g, kind);
        const Rect r{8, 74.0f + k * 16, 304, 15};
        panel(r, withAlpha(s.journalPick == k ? theme::kClutchGold : theme::kShell, s.journalPick == k ? 0.3f : 0.1f));
        icon(kind, r.x + 11, r.y + 8, known);
        text(app, known ? str::kCritterName[k] : str::kCrittersUnseen, r.x + 24, r.y + 1, 0.4f,
             known ? theme::kShell : withAlpha(theme::kShell, 0.5f), C2D_AlignLeft, 150);
        if (met) {
            std::snprintf(line, sizeof(line), str::kCrittersFriends, critters::friendCount(g, kind));
            text(app, line, r.x + r.w - 6, r.y + 2, 0.36f, theme::kClutchGold, C2D_AlignRight, 110);
        } else if (known) {
            text(app, str::kCrittersSeen, r.x + r.w - 6, r.y + 2, 0.36f, withAlpha(theme::kShell, 0.7f), C2D_AlignRight, 110);
        }
        if (in.tapped && r.contains(in.tx, in.ty)) {
            s.journalPick = s.journalPick == k ? -1 : k;
            audio::playSfx(audio::Sfx::Tap);
        }
    }
    const char* note = s.journalPick < 0 ? str::kCrittersTap
                       : critters::seen(g, static_cast<Kind>(s.journalPick)) ? str::kCritterNote[s.journalPick]
                                                                             : str::kCrittersUnseenNote;
    text(app, note, 10, 188, 0.33f, withAlpha(theme::kShell, s.journalPick < 0 ? 0.55f : 0.9f), C2D_AlignLeft, 300);
    const s32 today = dayIndex(nowLocal(app));
    int todays = 0;
    for (int k = 0; k < critters::kKinds; ++k) todays += g.progress.critterDay == today && (g.progress.critterToday >> k & 1u);
    std::snprintf(line, sizeof(line), str::kCrittersToday, todays);
    text(app, line, 100, 206, 0.33f, withAlpha(theme::kShell, 0.75f), C2D_AlignLeft, 110);
    std::snprintf(line, sizeof(line), str::kCrittersTreats, critters::kDailyPaid - critters::paidToday(g, today));
    text(app, line, 100, 220, 0.33f, withAlpha(theme::kShell, 0.75f), C2D_AlignLeft, 110);
    if (button(app, {6, 202, 88, 34}, str::kCrittersPlaces, in)) {
        app.care.journalTab = 2;
        audio::playSfx(audio::Sfx::Tap);
    }
}

void command(App& app, const char* text) {
    State& s = st();
    char word[16] = {};
    int arg = -1;
    std::sscanf(text, "%15s %d", word, &arg);
    if (std::strcmp(word, "journal") == 0) {
        app.care.page = CarePage::Journal;
        app.care.journalTab = 4;
        return;
    }
    if (std::strcmp(word, "friends") == 0) {  // every kind spotted and befriended a few times (the Journal's shot)
        Progress& p = app.game.progress;
        p.critterSeen = 0x7F;
        p.critterFriends = 0x3D;  // (not the rabbits, not a snow hare: spotted only)
        for (int k = 0; k < critters::kKinds; ++k) p.critterCounts[k] = (p.critterFriends >> k & 1u) ? static_cast<u8>(2 + 3 * k) : 0;
        return;
    }
    if (!s.valley) return;
    if (std::strcmp(word, "clear") == 0) {
        for (critters::Critter& c : s.life.c) c.alive = false;
        critters::endMoment(s.life);
        return;
    }
    if (std::strcmp(word, "spawn") == 0 && arg >= 0 && arg < critters::kKinds) {
        const int i = critters::spawnNear(s.life, *s.valley, static_cast<Kind>(arg), s.around.you, s.camYaw, 30.0f);
        autotest::log("critters spawn %d: critter %d at (%.1f %.1f)", arg, i, i >= 0 ? s.life.c[i].pos.x : 0.0f,
                      i >= 0 ? s.life.c[i].pos.y : 0.0f);
        return;
    }
    if (std::strcmp(word, "act") == 0) {  // A on the nearest (of that kind), whichever way you face
        critters::Offer o;
        float best = 16.0f;
        for (int i = 0; i < critters::kMaxCritters; ++i) {
            const critters::Critter& c = s.life.c[i];
            if (!c.alive || (c.state != critters::State::Idle && c.state != critters::State::Move)) continue;
            if ((arg >= 0 && static_cast<int>(c.kind) != arg) || (c.kind == Kind::Duck && c.slot != 0)) continue;
            const float d = std::hypot(c.pos.x - s.around.you.x, c.pos.y - s.around.you.y);
            if (d < best) {
                best = d;
                o = {i, critters::actFor(c.kind), d};
            }
        }
        s.offer = o;
        act(app);
        autotest::log("critters act %d: critter %d, moment %d", arg, o.who, static_cast<int>(s.life.moment.act));
        return;
    }
    if (std::strcmp(word, "log") == 0) {
        for (int i = 0; i < critters::kMaxCritters; ++i) {
            const critters::Critter& c = s.life.c[i];
            if (c.alive)
                autotest::log("critter %d: kind %d state %d at (%.1f %.1f %.1f) %.1f m from you", i, static_cast<int>(c.kind),
                              static_cast<int>(c.state), c.pos.x, c.pos.y, c.pos.z,
                              std::hypot(c.pos.x - s.around.you.x, c.pos.y - s.around.you.y));
        }
    }
}

}  // namespace ec::wildlife
