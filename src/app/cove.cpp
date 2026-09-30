// Driftwood Cove (1.0, D90; workstream C): fishing off the cove's shore, as a valley feature.
// Tam the fisher stands by the water: talk to him (A) and he lends you his spare rod the first
// time, and after that tells you how the fish are biting. At the water's edge, A starts fishing:
// you stand facing the lake with your partner sitting beside you. A casts (the bobber plops out),
// then you wait: the bobber twitches with nibbles (strike at one and it's too soon) and then dips
// right under: A in time strikes, and you reel it in on the bottom screen (hold A, or crank the
// reel round with the stylus), keeping the line's tension in its band while the fish runs, or the
// line snaps or it slips the hook. The catch goes in the pouch (River Fish mostly, now and then
// another food) or is worth Gleam (a shell tangled on the hook, a rare pearl); your partner gets a
// nibble of any food (Love up). B puts the rod away. The fish are plentiful but not endless (the
// day's stock), and shells wash up along the beach each day (A beside one picks it up).
#include "app/cove.hpp"

#include <sys/stat.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "app/audio.hpp"
#include "app/tips_ui.hpp"
#include "app/dialogue.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/challenge_mesh.hpp"
#include "core/clock.hpp"
#include "core/fishing.hpp"
#include "core/kinds.hpp"
#include "core/people.hpp"
#include "core/place_layout.hpp"
#include "core/trainer.hpp"
#include "core/valley.hpp"

namespace ec::cove {
namespace {

using fishing::Catch;

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
Vec3 forwardOf(float heading) { return {std::sin(heading), -std::cos(heading), 0}; }
Vec3 rightOf(float heading) { return {-std::cos(heading), -std::sin(heading), 0}; }  // (heading 0 faces -Y: its right is -X)

// ---------------------------------------------------------------------- the day
// The day's catch (the stock), the shells picked and Tam's rod, in the save's progress block
// (core/trainer Progress::cove*): they renew each day and don't come back by restarting.
struct Day {
    s32& day;
    u8& fish;    // catches landed today
    u8& shells;  // the beach's spots picked today, a bit each
    u8& rod;     // Tam has lent you his rod (for good)
};

// Today's: on a new day the stock renews and new shells wash up (the rod stays lent).
Day today(const App& app) {
    Progress& p = const_cast<App&>(app).game.progress;  // (the day's bookkeeping, even from a const look)
    const s32 d = dayIndex(nowLocal(app));
    if (p.coveDay != d) {
        p.coveDay = d;
        p.coveFish = 0;
        p.coveShells = 0;
    }
    return {p.coveDay, p.coveFish, p.coveShells, p.coveRod};
}

bool hasRod(const App& app) { return today(app).rod || app.game.progress.counts[kCountFish] > 0; }
int fishLeft(const App& app) {
    const int left = fishing::kFishPerDay - today(app).fish;
    return left > 0 ? left : 0;
}

// ---------------------------------------------------------------------- where things are
struct Spots {
    const Valley* of = nullptr;
    bool ok = false;
    fishing::CoveSpots local;
    Vec3 fish, cast, partner, fisher;
    float facing = 0, fisherHeading = 0;
    Vec3 shells[fishing::kShellSpots];
};

const Spots& spots(const Valley& v) {
    static Spots s;
    if (s.of == &v) return s;
    s = Spots{};
    s.of = &v;
    const ValleyPlaceInfo* p = v.place(kPlaceCove);
    if (!p) return s;
    s.ok = true;
    s.local = fishing::coveSpots(v);
    const fishing::CoveSpots& L = s.local;
    auto at = [&](Vec2 local, float up = 0) {  // on the ground there, or on the jetty's deck
        const Vec2 w = placeToWorld(*p, local);
        return Vec3{w.x, w.y, v.groundAt(w.x, w.y, p->at.z + 2.0f) + up};
    };
    s.fish = at(L.fishSpot);
    s.partner = at(L.partner);
    s.fisher = at(L.fisher);
    const Vec2 c = placeToWorld(*p, L.castTo);
    s.cast = {c.x, c.y, v.water};
    s.facing = p->heading;
    s.fisherHeading = p->heading + L.fisherFacing;
    for (int k = 0; k < fishing::kShellSpots; ++k) s.shells[k] = at(L.shells[k]);
    return s;
}

// ---------------------------------------------------------------------- fishing
enum class Step : u8 { Off, Ready, Casting, Waiting, Bite, Reeling, Landed, Lost };

struct State {
    Step step = Step::Off;
    float t = 0, fishingFor = 0;  // seconds in this step; since you sat down to fish
    bool autoplay = false;
    Rng rng{1};
    Catch on = Catch::RiverFish;  // what's on the line (settled at the cast)
    fishing::Bite bite;
    fishing::Reel reel;
    int nibbles = 0;              // nibbles shown so far
    float twitch = 0;             // a nibble's twitch, fading
    float dip = 0;                // the bobber pulled under (the bite)
    Vec3 bobber;
    float clickIn = 0;            // the reel's next click
    float rippleIn = 0;           // the running fish's next ripple
    // The stylus cranking the reel round.
    bool cranking = false;
    float crankAngle = 0, crankSpeed = 0, crankShown = 0;
    // What happened.
    const char* lost = nullptr;
    char landedLine[64] = {}, landedLine2[64] = {};
    float nibbleT = -1;           // your partner's nibble: seconds in (< 0: none)
    // Ripples on the water: where and how old.
    struct Ripple {
        Vec3 at;
        float age = 9, size = 1;
    } ripples[4];
    int nextRipple = 0;
    // You, your partner, where you face.
    Vec3 you, pal;
    float facing = 0;
};

State& st() {
    static State s;
    return s;
}

void ripple(State& f, Vec3 at, float size) {
    f.ripples[f.nextRipple] = {at, 0.0f, size};
    f.nextRipple = (f.nextRipple + 1) % 4;
}

// Your hands (where the renderer drew them this frame: D134, the rod floated ahead of a guessed
// hand) and the rod's tip: raised while you wait, pulled down toward the fish as you reel, and in a
// cast swung back over your shoulder, whipped forward and let out over the water.
constexpr float kCastTime = 0.75f;
constexpr float kReleaseAt = 0.55f;  // (of the cast: the whip forward, where the line flies)
Vec3 handOf(const State& f) {
    Vec3 grip;
    if (r3d::youGrip(grip) && length(grip - f.you) < 1.5f) return grip;
    return f.you + forwardOf(f.facing) * 0.22f + rightOf(f.facing) * 0.12f + Vec3{0, 0, 0.5f};
}
float easeIn(float x) { x = clampf(x, 0.0f, 1.0f); return x * x * (3 - 2 * x); }
Vec3 tipOf(const State& f, float t) {
    const bool reeling = f.step == Step::Reeling;
    const float bend = reeling ? f.reel.tension : 0.0f;
    const float wobble = reeling && f.reel.running() ? 0.08f * std::sin(t * 23.0f) : 0.0f;
    const Vec3 fwd = forwardOf(f.facing), up{0, 0, 1};
    Vec3 dir = fwd * (1.5f + 0.3f * bend) + up * (1.25f - 0.6f * bend + wobble);
    if (f.step == Step::Casting) {
        const float u = f.t / kCastTime;
        const Vec3 back = fwd * -0.6f + up * 1.8f, whip = fwd * 1.75f + up * 0.5f, rest = dir;
        dir = u < 0.3f ? lerp(rest, back, easeIn(u / 0.3f))
            : u < kReleaseAt ? lerp(back, whip, easeIn((u - 0.3f) / (kReleaseAt - 0.3f)))
                             : lerp(whip, rest, easeIn((u - kReleaseAt) / (1.0f - kReleaseAt)));
    }
    return handOf(f) + dir;
}

// Where the bobber hangs from the tip before a cast.
Vec3 dangleOf(const State& f, float t) { return tipOf(f, t) - Vec3{0, 0, 0.55f} + forwardOf(f.facing) * (0.05f * std::sin(t * 2.0f)); }

// Your partner's size as drawn (the kind's, smaller while young; 0: out alone), as the challenges'.
float partnerSize(const vext::Stage& stage) {
    if (!stage.shown) return 0.0f;
    const Dragon& d = *stage.shown;
    const float grown = d.stage == Stage::Adult ? 1.0f : d.stage == Stage::Adolescent ? 0.8f : d.stage == Stage::Juvenile ? 0.6f : 0.45f;
    return kindSize(d) * grown;
}

// The ground or the lake's surface, whichever is higher (the camera stays above both).
float surfaceAt(const Valley& v, float x, float y) { return std::fmax(v.heightAt(x, y), v.water); }

int partnerIndex(const App& app) {
    const SaveData& g = app.game;
    for (int i = 0; i < g.dragonCount; ++i)
        if (g.dragons[i].id == g.world.partnerId && g.dragons[i].stage != Stage::Egg) return i;
    return -1;
}

void startFishing(App& app) {
    showTip(app, tips::kTipFishing);
    State& f = st();
    f.step = Step::Ready;
    f.t = 0;
    f.fishingFor = 0;
    f.rng = Rng(app.rng.next() | 1u);
    f.lost = nullptr;
    f.nibbleT = -1;
    for (State::Ripple& r : f.ripples) r.age = 9;
    audio::playSfx(audio::Sfx::Equip, 1.0f, 0.7f);
}

void cast(App& app, State& f, vext::Stage& stage) {
    if (fishLeft(app) <= 0) {  // the day's fish are caught
        showToast(app, str::kFishResting);
        audio::playSfx(audio::Sfx::Error, 1.0f, 0.5f);
        return;
    }
    const int hour = hourOfDay(nowLocal(app));
    f.on = fishing::rollCatch(f.rng, hour);
    f.bite = fishing::rollBite(f.rng, hour, f.on);
    f.nibbles = 0;
    f.twitch = f.dip = 0;
    f.step = Step::Casting;
    f.t = 0;
    audio::playSfx(audio::Sfx::Cast);
    (void)stage;
}

// Landed: into the pouch (or its worth in Gleam), counted, a nibble for your partner.
void land(App& app, State& f) {
    const fishing::CatchInfo& info = fishing::catchInfo(f.on);
    Day d = today(app);
    if (d.fish < 255) ++d.fish;
    std::snprintf(f.landedLine, sizeof(f.landedLine), str::kLandedCatch, info.name);
    f.landedLine2[0] = 0;
    if (info.food != Food::Count) {
        u16& n = app.game.pouch[static_cast<int>(info.food)];
        if (n + info.foodCount <= 99) {
            n = static_cast<u16>(n + info.foodCount);
            char what[40];
            std::snprintf(what, sizeof(what), "%s x%d", foodInfo(info.food).name, static_cast<int>(n));
            std::snprintf(f.landedLine2, sizeof(f.landedLine2), str::kIntoPouch, what);
        } else {  // a full pouch: its worth instead
            const u32 worth = 15u * static_cast<u32>(info.foodCount);
            app.game.gleam += worth;
            std::snprintf(f.landedLine2, sizeof(f.landedLine2), str::kPouchFull, static_cast<unsigned long>(worth));
        }
        const int p = partnerIndex(app);
        if (p >= 0) {
            fishing::nibble(app.game.dragons[p]);
            f.nibbleT = 0;
        }
    }
    if (info.gleam) {
        app.game.gleam += info.gleam;
        std::snprintf(f.landedLine2, sizeof(f.landedLine2), str::kCatchWorth, static_cast<unsigned long>(info.gleam));
        audio::playSfx(audio::Sfx::Coin, 1.0f, 0.8f);
    }
    if (info.fish) trainer::count(app.game, kCountFish);
    if (f.on == Catch::Shell || f.on == Catch::Pearl) trainer::count(app.game, kCountShells);
    audio::playSfx(f.on == Catch::BigFish ? audio::Sfx::SplashBig : audio::Sfx::Splash);
    audio::playSfx(audio::Sfx::Chirp, 1.1f, 0.7f);
    if (f.on == Catch::Pearl) audio::playStinger("place-found");
    f.step = Step::Landed;
    f.t = 0;
    saveNow(app);
}

void lose(State& f, const char* why, audio::Sfx sound) {
    f.lost = why;
    f.step = Step::Lost;
    f.t = 0;
    f.dip = 0;
    audio::playSfx(sound, 1.0f, 0.6f);
}

// Tam's words: the first time he lends you his rod; after that a tip, and how the fish are today.
void talkToFisher(App& app) {
    Day d = today(app);
    Talk t;
    if (!hasRod(app)) {
        for (const char* line : str::kFisherHello) t.lines[t.count++] = line;
        d.rod = 1;
        saveNow(app);
    } else {
        static char countLine[64];
        const int caught = app.game.progress.counts[kCountFish];
        t.lines[t.count++] = str::kFisherTips[(d.day + caught) % 4];
        if (fishLeft(app) <= 0) {
            t.lines[t.count++] = str::kFisherRested;
        } else if (caught >= 5) {
            std::snprintf(countLine, sizeof(countLine), str::kFisherCount, caught);
            t.lines[t.count++] = countLine;
        } else {
            t.lines[t.count++] = str::kFisherBiting;
        }
    }
    Speaker who;
    who.name = str::kFisherName;
    who.title = str::kFisherTitle;
    who.voice = 0;  // (a man's: Noah's voice)
    who.pitch = 1.25f;
    who.tint = {226, 184, 70};
    startSpeech(app, who, t);
}

void pickShell(App& app, int k) {
    Day d = today(app);
    const u8 bit = static_cast<u8>(1u << k);
    if (!(fishing::shellsToday(d.day) & bit) || (d.shells & bit)) return;
    d.shells = static_cast<u8>(d.shells | bit);
    const u32 worth = fishing::shellGleam(d.day, k);
    app.game.gleam += worth;
    trainer::count(app.game, kCountShells);
    char line[40];
    std::snprintf(line, sizeof(line), str::kShellFound, fishing::shellName(d.day, k));
    char both[72];
    std::snprintf(both, sizeof(both), "%s  +%lu Gleam", line, static_cast<unsigned long>(worth));
    showToastf(app, "%s", both);
    audio::playSfx(audio::Sfx::ShellPick);
    if (fishing::shellAt(d.day, k) == Catch::Pearl) {
        audio::playSfx(audio::Sfx::Coin, 1.1f, 0.8f);
        audio::playStinger("place-found");
    }
}

// The fish (or whatever it is) up out of the water: from the bobber to your hand, held up high on
// your right where the camera sees it (in front of you, you'd hide it).
Vec3 catchAt(const State& f) {
    const Vec3 held = f.you + rightOf(f.facing) * 0.6f + forwardOf(f.facing) * 0.15f + Vec3{0, 0, 1.2f};
    const float u = clampf(f.t / 0.6f, 0.0f, 1.0f);
    return f.bobber + (held - f.bobber) * u + Vec3{0, 0, 1.6f * 4.0f * u * (1.0f - u)};
}

float catchSize(Catch c) {
    switch (c) {
        case Catch::BigFish: return 1.05f;  // (storybook-big: it reads at the camera's distance)
        case Catch::RiverFish: return 0.75f;
        default: return 1.0f;
    }
}

}  // namespace

// ---------------------------------------------------------------------- the feature
// Tam fishes by the water while you're not by him (workstream D): his rod out over the lake.
bool tamFishing(const Spots& s, Vec3 you) { return s.ok && std::hypot(you.x - s.fisher.x, you.y - s.fisher.y) > 8.0f; }

int folk(const App& app, const Valley& v, Vec3 near, float radius, vext::Folk* out, int cap) {
    const Spots& s = spots(v);
    if (!s.ok || cap <= 0 || std::hypot(near.x - s.fish.x, near.y - s.fish.y) > radius + 40.0f) return 0;
    int n = 0;
    {  // Tam, by the water in his yellow oilskin
        vext::Folk& f = out[n++];
        f = vext::Folk{};
        static const u8 kLook[kLookParts] = {0, 0, 5, 2, 1, 3};  // a tunic, tousled silver hair, sand skin, grey eyes
        f.look.form = static_cast<u8>(playerBody(kLook));
        playerPalette(kLook, f.look.pal);
        f.look.pal[kPalAccent] = {226, 184, 70};  // the oilskin
        f.look.pal[kPalPattern] = {64, 84, 112};  // navy trousers
        f.look.hair = 0;
        f.look.at = s.fisher;
        f.look.heading = s.fisherHeading;
        f.name = str::kFisherName;
        f.prompt = str::kPromptTalk;
        f.id = 0;
        f.voice = 0;
        f.pitch = 1.25f;
        f.clip = tamFishing(s, near) ? "fish" : nullptr;  // his rod out till you come near (drawOver draws it; workstream D)
    }
    if (n < cap && !active(app)) {  // the water's edge: fishing
        vext::Folk& f = out[n++];
        f = vext::Folk{};
        f.shown = false;
        f.look.at = s.fish;
        f.name = "";
        f.prompt = str::kPromptFish;
        f.id = 1;
        f.reach = 2.2f;
    }
    // The day's shells not yet picked up (drawn by drawCoveThings).
    const Day d = today(app);
    const u8 lying = static_cast<u8>(fishing::shellsToday(d.day) & ~d.shells);
    for (int k = 0; k < fishing::kShellSpots && n < cap; ++k) {
        if (!(lying & (1u << k))) continue;
        vext::Folk& f = out[n++];
        f = vext::Folk{};
        f.shown = false;
        f.look.at = s.shells[k];
        f.name = "";
        f.prompt = str::kPromptShell;
        f.id = static_cast<u8>(10 + k);
        f.reach = 1.6f;
    }
    return n;
}

void act(App& app, const vext::Folk& who, vext::Stage& stage) {
    if (who.id == 0) {
        talkToFisher(app);
    } else if (who.id == 1) {
        if (!hasRod(app)) {
            showToast(app, str::kNoRod);
            audio::playSfx(audio::Sfx::Error, 1.0f, 0.5f);
        } else {
            startFishing(app);
        }
    } else if (who.id >= 10 && who.id < 10 + fishing::kShellSpots) {
        pickShell(app, who.id - 10);
    }
    (void)stage;
}

bool active(const App& app) {
    (void)app;
    return st().step != Step::Off;
}

void update(App& app, const Input& in, vext::Stage& stage) {
    State& f = st();
    if (!stage.valley) return;
    const Spots& s = spots(*stage.valley);
    if (!s.ok) {
        f.step = Step::Off;
        return;
    }
    const float dt = app.dt;
    f.t += dt;
    f.fishingFor += dt;
    // You at the water's edge facing the lake, your partner sitting beside you, the camera over
    // your shoulder.
    f.you = s.fish;
    // Your partner sits on your left, a big one further off and a little ahead; the camera stands
    // over your right shoulder, further back and higher for a big partner (and far enough that
    // it draws on its light model), looking out past you to the bobber.
    const float size = partnerSize(stage);
    const Vec3 fwd = forwardOf(s.facing), right = rightOf(s.facing);
    f.pal = s.partner - right * (1.4f * std::fmax(0.0f, size - 0.45f)) + fwd * (0.5f * size);
    f.facing = s.facing;
    stage.you = f.you;
    stage.youHeading = f.facing;
    stage.pal = f.pal;
    stage.palHeading = f.facing - 0.35f;  // (turned a little toward you)
    stage.camSet = true;
    stage.eye = s.fish + right * (2.8f + 1.2f * size) - fwd * (5.2f + 3.5f * size);
    stage.eye.z = surfaceAt(*stage.valley, stage.eye.x, stage.eye.y) + 2.6f + 1.3f * size;
    stage.target = s.fish - right * (0.4f + 0.6f * size) + fwd * 5.0f;
    stage.target.z = surfaceAt(*stage.valley, s.fish.x, s.fish.y) + 0.2f + 0.5f * size;
    stage.youClip = "fish";  // (the rod held out: workstream D)
    if (f.nibbleT >= 0) {  // a nibble of the catch
        f.nibbleT += dt;
        stage.palClip = ClipId::Eat;
        if (f.nibbleT > 0.4f && f.nibbleT - dt <= 0.4f) audio::playSfx(audio::Sfx::Munch);
        if (f.nibbleT > 1.6f) f.nibbleT = -1;
    } else {
        stage.palClip = f.fishingFor < 1.0f ? ClipId::Sit : ClipId::SitLoop;
    }
    f.twitch = std::fmax(0.0f, f.twitch - dt * 3.0f);
    for (State::Ripple& r : f.ripples) r.age += dt;
    const bool press = (in.down & KEY_A) != 0;
    // B puts the rod away (but not with a fish on the line).
    if ((in.down & KEY_B) && f.step != Step::Bite && f.step != Step::Reeling && f.step != Step::Casting) {
        f.step = Step::Off;
        audio::playSfx(audio::Sfx::Back);
        return;
    }
    switch (f.step) {
        case Step::Ready:
            if (press || (f.autoplay && f.t > 1.2f && fishLeft(app) > 0)) cast(app, f, stage);
            break;
        case Step::Casting: {  // back, whipped forward, and the bobber flies out in an arc from the tip
            stage.youClip = "cast";
            const float u = clampf(f.t / kCastTime, 0.0f, 1.0f);
            const Vec3 from = tipOf(f, app.t);
            if (u < kReleaseAt) {
                f.bobber = from - Vec3{0, 0, 0.3f};  // (on the line's end, swung with the rod)
            } else {
                const float w = (u - kReleaseAt) / (1.0f - kReleaseAt);
                f.bobber = from + (s.cast - from) * w + Vec3{0, 0, 2.2f * 4.0f * w * (1.0f - w)};
            }
            if (u >= 1.0f) {
                f.bobber = s.cast;
                f.step = Step::Waiting;
                f.t = 0;
                ripple(f, f.bobber, 1.0f);
                audio::playSfx(audio::Sfx::Plop);
            }
            break;
        }
        case Step::Waiting: {  // nibbles, then the bite
            f.bobber = s.cast + Vec3{0, 0, 0.02f * std::sin(app.t * 2.2f) - 0.06f * f.twitch};
            if (f.nibbles < f.bite.nibbles && f.t >= f.bite.nibbleAt[f.nibbles]) {
                ++f.nibbles;
                f.twitch = 1.0f;
                ripple(f, f.bobber, 0.5f);
                audio::playSfx(audio::Sfx::Plop, 1.6f, 0.35f);
            }
            if (press) {  // on a nibble: too soon; otherwise, reel in to cast again
                if (f.twitch > 0.1f) lose(f, str::kTooSoon, audio::Sfx::Whimper);
                else {
                    f.step = Step::Ready;
                    f.t = 0;
                    showToast(app, str::kNothingYet);
                }
                break;
            }
            if (f.t >= f.bite.wait) {
                f.step = Step::Bite;
                f.t = 0;
                f.dip = 1;
                ripple(f, f.bobber, 1.4f);
                audio::playSfx(audio::Sfx::Bite);
            }
            break;
        }
        case Step::Bite:  // under! strike now
            f.bobber = s.cast - Vec3{0, 0, 0.14f};
            if (press || (f.autoplay && f.t > 0.25f)) {
                f.step = Step::Reeling;
                f.t = 0;
                f.reel.start(fishing::catchInfo(f.on).strength, f.rng.next());
                f.clickIn = 0;
                audio::playSfx(audio::Sfx::Reel, 1.2f);
            } else if (f.t > f.bite.window) {
                lose(f, str::kTooSlow, audio::Sfx::Whimper);
            }
            break;
        case Step::Reeling: {
            // How hard you reel: A held, or the stylus's speed round the reel (drawBottom measures it).
            float rate = (in.held & KEY_A) ? 1.0f : clampf(f.crankSpeed / 7.5f, 0.0f, 1.0f);
            if (f.autoplay) rate = !f.reel.running() && f.reel.tension < 0.55f ? 1.0f : 0.0f;
            const fishing::Reel::Step r = f.reel.update(rate, dt);
            // The fish drawn toward you as it comes in, fighting about as it runs.
            const Vec3 near = f.you + forwardOf(f.facing) * 1.8f;
            f.bobber = s.cast + (Vec3{near.x, near.y, s.cast.z} - s.cast) * f.reel.progress +
                       rightOf(f.facing) * (f.reel.running() ? 0.5f * std::sin(app.t * 7.0f) : 0.0f) - Vec3{0, 0, 0.1f};
            if (rate > 0.1f && (f.clickIn -= dt) <= 0) {  // the reel's clicks, higher as the line tightens
                audio::playSfx(audio::Sfx::Reel, 0.8f + 0.6f * f.reel.tension, 0.5f);
                f.clickIn = 0.32f - 0.18f * rate;
            }
            if (f.reel.running() && (f.rippleIn -= dt) <= 0) {
                ripple(f, f.bobber, 0.6f);
                f.rippleIn = 0.35f;
            }
            if (r == fishing::Reel::Step::Caught) land(app, f);
            else if (r == fishing::Reel::Step::Snapped) lose(f, str::kSnapped, audio::Sfx::Grumble);
            else if (r == fishing::Reel::Step::Escaped) lose(f, str::kEscaped, audio::Sfx::Whimper);
            break;
        }
        case Step::Landed:
            stage.youClip = f.t < 1.6f ? "cheer" : "idle";
            if (f.t > 3.2f && (press || f.autoplay || f.t > 6.0f)) {
                f.step = Step::Ready;
                f.t = f.autoplay ? 0.0f : 1.0f;
            }
            break;
        case Step::Lost:
            if (f.t > 1.8f && (press || f.autoplay || f.t > 4.0f)) {
                f.step = Step::Ready;
                f.t = 0;
            }
            break;
        case Step::Off: break;
    }
}

void drawCoveThings(App& app, const Valley& v, s64 now) {
    const Spots& s = spots(v);
    if (!s.ok || !r3d::ready()) return;
    r3d::ChallengeProp props[fishing::kShellSpots + 2];
    int n = 0;
    // The day's shells on the wet sand (near enough to see).
    const Day d = today(app);
    const u8 lying = static_cast<u8>(fishing::shellsToday(d.day) & ~d.shells);
    for (int k = 0; k < fishing::kShellSpots; ++k) {
        if (!(lying & (1u << k))) continue;
        float x, y, ppu;
        if (!r3d::project(s.shells[k], x, y, ppu) || ppu < 5.0f || x < -30 || x > kTopW + 30) continue;
        r3d::ChallengeProp& p = props[n++];
        p.kind = r3d::PropKind::Shell;
        p.variant = static_cast<u8>(fishing::shellKind(d.day, k));
        p.at = s.shells[k];
        p.yaw = k * 1.9f + d.day * 0.7f;
        p.scale = 1.8f;  // (storybook-big, so they read on the sand)
        p.look = shellLook(p.variant, k + d.day);
    }
    // Fishing: the bobber (hanging from the rod, flying, floating) and the catch held up.
    const State& f = st();
    if (f.step != Step::Off) {
        r3d::ChallengeProp& b = props[n++];
        b.kind = r3d::PropKind::Bobber;
        b.at = f.step == Step::Ready || f.step == Step::Lost ? dangleOf(f, app.t)
               : f.step == Step::Landed                     ? catchAt(f) + Vec3{0, 0, 0.45f}  // (on the line, just above the catch)
                                                            : f.bobber;
        b.pitch = f.step == Step::Bite ? 0.5f : 0.0f;
        b.scale = 2.4f;  // (storybook-big: it reads out on the water)
        b.look = bobberLook();
        if (f.step == Step::Landed && f.t < 2.8f) {
            r3d::ChallengeProp& c = props[n++];
            c.at = catchAt(f);
            c.yaw = f.facing + 1.5708f;  // held side on, facing along the shore
            switch (f.on) {
                case Catch::RiverFish:
                case Catch::BigFish:
                    c.kind = r3d::PropKind::Fish;
                    c.roll = 0.35f * std::sin(app.t * 14.0f) * (f.t < 1.2f ? 1.0f : 0.3f);  // a wriggle
                    c.scale = catchSize(f.on);
                    c.look = fishLook(f.on == Catch::BigFish);
                    break;
                case Catch::Shell:
                case Catch::Pearl:
                    c.kind = r3d::PropKind::Shell;
                    c.variant = f.on == Catch::Pearl ? 3 : 0;
                    c.scale = 2.2f;
                    c.look = shellLook(c.variant, 0);
                    break;
                default: {  // a food: the fruit's shape in its colours (a root, a sprig of berries, a melon)
                    const challenge::Fruit shape = f.on == Catch::Honeyroot  ? challenge::Fruit::Pear
                                                   : f.on == Catch::Skyberry ? challenge::Fruit::Plum
                                                                             : challenge::Fruit::Apple;
                    c.kind = r3d::PropKind::Fruit;
                    c.variant = static_cast<u8>(shape);
                    c.scale = f.on == Catch::Frostmelon ? 0.42f : 0.3f;
                    c.look = fruitLook(shape);
                    c.look.colour[0] = f.on == Catch::Honeyroot  ? Rgb{214, 168, 90}
                                       : f.on == Catch::Skyberry ? Rgb{120, 150, 230}
                                                                 : Rgb{176, 226, 214};
                    break;
                }
            }
        }
    }
    if (!n) return;
    Rgb top, horizon, tint;
    valleySky(now, top, horizon, tint);
    r3d::drawChallengeProps(app, props, n, horizon, now);
}

// Tam's rod while he fishes (workstream D): from his hands out over the lake, the line down to it.
void drawOver(App& app, const vext::Stage& stage) {
    if (!stage.valley || active(app)) return;
    const Spots& s = spots(*stage.valley);
    if (!tamFishing(s, stage.you) || std::hypot(stage.you.x - s.fisher.x, stage.you.y - s.fisher.y) > 70.0f) return;
    const Vec3 fwd = forwardOf(s.fisherHeading);
    const Vec3 hand = s.fisher + fwd * 0.3f + rightOf(s.fisherHeading) * 0.1f + Vec3{0, 0, 0.55f};
    const Vec3 tip = hand + fwd * 1.5f + Vec3{0, 0, 1.15f + 0.03f * std::sin(app.t * 1.3f)};
    Vec3 end = tip + fwd * 1.2f;
    end.z = stage.valley->water;
    float hx, hy, hp, tx, ty, tp, ex, ey, ep;
    if (!r3d::project(hand, hx, hy, hp) || !r3d::project(tip, tx, ty, tp) || !r3d::project(end, ex, ey, ep)) return;
    const u32 rod = theme::rgba(120, 82, 50);
    C2D_DrawLine(hx, hy, rod, tx, ty, rod, std::fmax(1.2f, 0.035f * hp), 0);
    C2D_DrawLine(tx, ty, withAlpha(theme::kShell, 0.7f), ex, ey, withAlpha(theme::kShell, 0.7f), 1.0f, 0);
}

void drawTop(App& app, const vext::Stage& stage) {
    State& f = st();
    (void)stage;
    // The rod and its line: from your hand to the tip, then down to the bobber, sagging when slack.
    const Vec3 hand = handOf(f), tip = tipOf(f, app.t);
    float hx, hy, hp, tx, ty, tp;
    const u32 rod = theme::rgba(120, 82, 50);
    if (r3d::project(hand, hx, hy, hp) && r3d::project(tip, tx, ty, tp)) {
        C2D_DrawLine(hx, hy, rod, tx, ty, rod, std::fmax(1.5f, 0.035f * hp), 0);
        const Vec3 end = f.step == Step::Ready || f.step == Step::Lost ? dangleOf(f, app.t) + Vec3{0, 0, 0.2f}
                         : f.step == Step::Landed                     ? catchAt(f)
                                                                      : f.bobber + Vec3{0, 0, 0.12f};
        const float slack = f.step == Step::Reeling ? 0.9f * (1.0f - f.reel.tension) : f.step == Step::Waiting ? 0.7f : 0.15f;
        float px = tx, py = ty;
        for (int k = 1; k <= 8; ++k) {
            const float u = k / 8.0f;
            const Vec3 at = tip + (end - tip) * u - Vec3{0, 0, slack * 4.0f * u * (1.0f - u)};
            float x, y, pp;
            if (!r3d::project(at, x, y, pp)) break;
            C2D_DrawLine(px, py, withAlpha(theme::kShell, 0.8f), x, y, withAlpha(theme::kShell, 0.8f), 1.0f, 0);
            px = x;
            py = y;
        }
    }
    // Ripples on the water: rings spreading and fading.
    for (const State::Ripple& r : f.ripples) {
        if (r.age > 1.2f) continue;
        const float rad = (0.15f + r.age * 1.1f) * r.size, a = 0.6f * (1.0f - r.age / 1.2f);
        float px = 0, py = 0;
        bool have = false;
        for (int k = 0; k <= 16; ++k) {
            const float ang = k * 6.2831853f / 16;
            float x, y, pp;
            if (!r3d::project(r.at + Vec3{std::cos(ang) * rad, std::sin(ang) * rad, 0.02f}, x, y, pp)) {
                have = false;
                continue;
            }
            if (have) C2D_DrawLine(px, py, withAlpha(theme::kShell, a), x, y, withAlpha(theme::kShell, a), 1.2f, 0);
            px = x;
            py = y;
            have = true;
        }
    }
    // The bite: a "!" over the bobber.
    if (f.step == Step::Bite) {
        float x, y, pp;
        if (r3d::project(f.bobber + Vec3{0, 0, 0.9f}, x, y, pp)) {
            C2D_DrawCircleSolid(x, y, 0, 13, withAlpha(theme::kRose, 0.9f));
            textCentered(app, "!", x, y, 0.8f, theme::kShell, 20, Face::Title);
        }
    }
    // Your partner's nibble: hearts rising from it.
    if (f.nibbleT >= 0) {
        float x, y, pp;
        if (r3d::project(f.pal + Vec3{0, 0, 1.6f}, x, y, pp))
            for (int k = 0; k < 3; ++k) {
                const float u = std::fmod(f.nibbleT * 0.8f + k * 0.33f, 1.0f);
                heart(x - 10 + k * 10, y - u * 30, 9, withAlpha(theme::kRose, 1.0f - u));
            }
    }
    // A word on the top screen: what's happening now.
    const char* line = nullptr;
    switch (f.step) {
        case Step::Bite: line = str::kStrikeNow; break;
        case Step::Landed: line = f.landedLine; break;
        case Step::Lost: line = f.lost; break;
        default: break;
    }
    if (line && line[0]) {
        const float w = std::fmin(380.0f, textWidth(app, line, 0.6f) + 28);
        panel({200 - w / 2, 14, w, 30}, withAlpha(theme::kDenPlum, 0.8f));
        textCentered(app, line, 200, 29, 0.6f, f.step == Step::Bite ? theme::kClutchGold : theme::kShell, 370);
    }
}

void drawBottom(App& app, const Input& in, const vext::Stage& stage) {
    State& f = st();
    (void)stage;
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    textCentered(app, str::kCoveTitle, 160, 14, 0.6f, theme::kClutchGold, 300, Face::Title);
    char line[80];
    if (f.step == Step::Reeling) {
        // The line's tension: a gauge down the left, its band in green.
        const Rect gauge{22, 40, 22, 150};
        panel(gauge, theme::kTrack);
        const float lo = gauge.y + gauge.h * (1.0f - fishing::kBandHigh), hi = gauge.y + gauge.h * (1.0f - fishing::kBandLow);
        C2D_DrawRectSolid(gauge.x, lo, 0, gauge.w, hi - lo, withAlpha(theme::rgba(110, 196, 110), 0.55f));
        const float ty = gauge.y + gauge.h * (1.0f - f.reel.tension);
        const u32 needle = f.reel.tension > fishing::kBandHigh ? theme::kRose : f.reel.inBand() ? theme::kClutchGold : theme::kAsh;
        C2D_DrawRectSolid(gauge.x - 4, ty - 3, 0, gauge.w + 8, 6, needle);
        textCentered(app, str::kTension, gauge.x + gauge.w / 2, gauge.y + gauge.h + 12, 0.36f, withAlpha(theme::kShell, 0.8f), 70);
        // The fish coming in: along a line from the water (right) to you (left).
        const Rect run{76, 44, 224, 10};
        panel(run, theme::kTrack);
        const float fx = run.x + run.w * (1.0f - f.reel.progress);
        panel({fx, run.y, run.x + run.w - fx, run.h}, theme::kSkyTeal);  // the line reeled in behind it
        heart(run.x - 10, run.y + 5, 10, theme::kRose);                   // you, at the near end
        C2D_DrawEllipseSolid(fx - 9, run.y - 12, 0, 18, 9, f.reel.running() ? theme::kRose : theme::kShell);
        text(app, str::kTheFish, run.x, run.y + 14, 0.36f, withAlpha(theme::kShell, 0.75f), C2D_AlignLeft, 100);
        // The reel: crank it round with the stylus.
        const Vec2 c{196, 138};
        const float rr = 48;
        C2D_DrawCircleSolid(c.x, c.y, 0, rr + 6, withAlpha(theme::kShell, 0.12f));
        C2D_DrawCircleSolid(c.x, c.y, 0, rr, withAlpha(theme::rgba(150, 110, 70), 0.9f));
        C2D_DrawCircleSolid(c.x, c.y, 0, rr * 0.35f, theme::rgba(120, 82, 50));
        const float kx = c.x + std::cos(f.crankAngle) * rr * 0.78f, ky = c.y + std::sin(f.crankAngle) * rr * 0.78f;
        C2D_DrawLine(c.x, c.y, theme::rgba(90, 64, 40), kx, ky, theme::rgba(90, 64, 40), 5, 0);
        C2D_DrawCircleSolid(kx, ky, 0, 9, theme::kClutchGold);
        if (in.touching && std::hypot(in.tx - c.x, in.ty - c.y) < rr + 28) {
            const float a = std::atan2(in.ty - c.y, in.tx - c.x);
            if (f.cranking) {
                const float da = std::remainder(a - f.crankAngle, 6.2831853f);
                f.crankShown = std::fmax(0.0f, std::fabs(da) / std::fmax(1.0f / 120.0f, app.dt));
                f.crankAngle = a;
            } else {
                f.crankAngle = a;
                f.cranking = true;
            }
        } else {
            f.cranking = false;
            f.crankShown = 0;
        }
        f.crankSpeed = f.crankSpeed + (f.crankShown - f.crankSpeed) * std::fmin(1.0f, app.dt * 10.0f);  // (smoothed, radians a second)
        if (in.held & KEY_A) f.crankAngle += app.dt * 9.0f;  // (A reels: the handle turns)
        text(app, str::kReelHint, 160, 200, 0.36f, withAlpha(theme::kShell, 0.8f), C2D_AlignCenter, 300);
        return;
    }
    f.cranking = false;
    f.crankSpeed = 0;
    // Otherwise: what to do, the day's fish, your counts.
    const char* what = f.step == Step::Ready     ? str::kCastHint
                       : f.step == Step::Waiting ? str::kWatchBobber
                       : f.step == Step::Bite    ? str::kStrikeNow
                                                 : "";
    textCentered(app, what, 160, 60, 0.55f, f.step == Step::Bite ? theme::kClutchGold : theme::kShell, 300);
    if (f.step == Step::Landed) {
        textCentered(app, f.landedLine, 160, 60, 0.55f, theme::kClutchGold, 300);
        textCentered(app, f.landedLine2, 160, 84, 0.45f, theme::kShell, 300);
        const int p = partnerIndex(app);
        if (f.nibbleT >= 0 && p >= 0) {
            std::snprintf(line, sizeof(line), str::kNibbleOf, app.game.dragons[p].name);
            textCentered(app, line, 160, 104, 0.42f, theme::kRose, 300);
        }
    } else if (f.step == Step::Lost && f.lost) {
        textCentered(app, f.lost, 160, 84, 0.45f, theme::kRose, 300);
    }
    if (f.step == Step::Waiting) textCentered(app, str::kReelIn, 160, 84, 0.4f, withAlpha(theme::kShell, 0.6f), 300);
    const int left = fishLeft(app);
    if (left > 0) std::snprintf(line, sizeof(line), str::kFishLeft, left);
    else std::snprintf(line, sizeof(line), "%s", str::kFishResting);
    textCentered(app, line, 160, 140, 0.45f, left > 0 ? theme::kSkyTeal : theme::kRose, 300);
    std::snprintf(line, sizeof(line), str::kFishCaught, static_cast<int>(app.game.progress.counts[kCountFish]),
                  static_cast<int>(app.game.progress.counts[kCountShells]));
    textCentered(app, line, 160, 162, 0.42f, withAlpha(theme::kShell, 0.8f), 300);
    if (f.step == Step::Ready) textCentered(app, str::kPutAway, 160, 214, 0.42f, withAlpha(theme::kShell, 0.7f), 300);
}

void autotest(App& app, int what) {
    const Valley* v = loadedValley();
    if (what == 0) {
        talkToFisher(app);
    } else if (what == 1) {
        if (v && spots(*v).ok) {
            today(app).rod = 1;
            startFishing(app);
        }
    } else if (what == 2) {
        const Day d = today(app);
        const u8 lying = static_cast<u8>(fishing::shellsToday(d.day) & ~d.shells);
        for (int k = 0; k < fishing::kShellSpots; ++k)
            if (lying & (1u << k)) {
                if (v && spots(*v).ok) {  // stand by it first, so the picture shows it going
                    app.autoGoto[0] = spots(*v).shells[k].x;
                    app.autoGoto[1] = spots(*v).shells[k].y - 1.2f;
                    app.autoGoto[2] = 1;
                }
                pickShell(app, k);
                break;
            }
    }
}

void setAutoplay(bool on) { st().autoplay = on; }

}  // namespace ec::cove
