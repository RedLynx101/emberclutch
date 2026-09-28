// A pageant show on Moonpetal Glade's stage (1.0, D90). Your dragon stands in a row with three
// rivals picked for the league (view.others, drawn light); Celestine welcomes everyone and calls
// each round. Look and Poise: the camera goes along the row, each dragon shows itself (a pose, then
// a sit) and the three judges hold up their cards. The Performance: you cue your dragon's tricks
// in time (buttons, and a tap on the stars) on the bottom screen while it performs them on the
// stage; then the rivals'. Last the placings, with the ribbon, Gleam and prizes (core/pageant).
#include "app/glade_show.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "app/audio.hpp"
#include "app/autotest.hpp"
#include "app/dialogue.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/accessories.hpp"
#include "core/clock.hpp"
#include "core/kinds.hpp"
#include "core/pageant.hpp"
#include "core/rig.hpp"
#include "core/trainer.hpp"

namespace ec::glade {
namespace {

using pageant::Cue;
using pageant::Hit;

enum class Phase : u8 { Intro, Look, Poise, Judges, Perform, PerformCards, Rivals, Results, Done };

constexpr float kTurn = 2.8f;       // seconds a dragon is shown in a round
constexpr float kCardsAt = 1.5f;    // ...and when its cards go up
constexpr float kLaneSpeed = 105;   // the Performance's cues, pixels a second
constexpr float kRingX = 58;        // where they're hit

struct Show {
    int partner = -1;
    int league = 1, slot = 0, theme = 0;
    int spot[kEntrants] = {};       // each entrant's place on the stage (entrant 0 is you)
    int onSpot[kEntrants] = {};     // and who stands on each spot
    pageant::Rival rivals[kRivals];
    DenActor actors[kRivals];
    ClipId rivalClip[kRivals] = {ClipId::Count, ClipId::Count};
    float score[kEntrants][kRounds] = {};
    float cards[kEntrants][kRounds][kJudges] = {};
    bool shown[kEntrants][kRounds] = {};
    float total[kEntrants] = {};
    int order[kEntrants] = {};
    int place = 0;
    Phase phase = Phase::Intro;
    float t = 0;       // seconds into the phase (or the turn)
    int turn = 0;      // the spot being judged
    bool said = false; // the host's word for this phase has been said
    // The Performance.
    pageant::Routine routine;
    Hit hits[pageant::kMaxCues] = {};
    bool judged[pageant::kMaxCues] = {};
    float perfT = 0;
    int lastBeat = -1;
    const char* feedback = nullptr;
    float feedbackT = 0;
    ClipId palClip = ClipId::Idle;
    float palClipFor = 0;
    ClipId queued = ClipId::Count;  // the same trick again: a frame of idle first
    const char* youClip = "idle";
    // Afterwards.
    pageant::ShowReward reward;
    // The camera, eased toward where the phase wants it.
    Vec3 eye, target;
    bool camSet = false;
    // The host's lines (the dialogue box keeps the pointers).
    char lines[5][160] = {};
    int attempt = 0;
};

Show& sh() {
    static Show s;
    return s;
}
bool g_on = false;
bool g_autoplay = false;

u32 col(u8 r, u8 g, u8 b, float a = 1.0f) { return withAlpha(theme::rgba(r, g, b), a); }
float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

const Dragon& entrantDragon(const App& app, int e) {
    const Show& s = sh();
    return e == 0 ? app.game.dragons[s.partner] : s.rivals[e - 1].dragon;
}
const char* entrantTrainer(const App& app, int e) { return e == 0 ? app.game.playerName : sh().rivals[e - 1].trainer; }

Speaker host() {
    Speaker s;
    s.name = str::kHostName;
    s.title = str::kHostTitle;
    s.voice = 1;  // (Noah's rule: women and children)
    s.pitch = 1.5f;
    s.tint = {236, 196, 110};
    return s;
}

void hostSays(App& app, int count) {
    if (g_autoplay) return;  // (scripted runs: the show goes on without waiting for A)
    Talk t;
    for (int i = 0; i < count && i < 5; ++i) t.lines[t.count++] = sh().lines[i];
    startSpeech(app, host(), t);
}

// Places on the stage (world): a spot, its floor raised by the stage's.
Vec3 spotAt(const Valley& v, int spot) {
    const Layout& L = gladeLayout();
    return gladePoint(v, L.rivals[spot], L.stage.z);
}
Vec3 audience(const Valley& v) { return gladePoint(v, {0, 12}); }

// A rival's clip (restarted when asked for again).
void playRival(App& app, int r, ClipId clip) {
    Show& s = sh();
    const Dragon& d = s.rivals[r].dragon;
    const int* clips = r3d::clipIndexFor(d, kFormGrown);
    if (!clips || !r3d::animsFor(d)) return;
    int index = clips[static_cast<int>(clip)];
    if (index < 0) index = clips[static_cast<int>(ClipId::Idle)];
    s.actors[r].anim.play(index, 0.25f, true);
    s.rivalClip[r] = clip;
    (void)app;
}

// An entrant's clip: yours through the stage the valley lends, a rival's on its own actor.
void pose(App& app, int e, ClipId clip, float seconds = 1.2f) {
    Show& s = sh();
    if (e == 0) {
        if (s.palClip == clip) s.queued = clip;  // (the same again: idle for a frame first)
        else s.palClip = clip;
        s.palClipFor = seconds;
    } else {
        playRival(app, e - 1, clip);
    }
}

ClipId trickClip(Cue c) {
    switch (c) {
        case Cue::A: return ClipId::Hop;
        case Cue::B: return ClipId::PlayBow;
        case Cue::X: return ClipId::WingFlutter;
        case Cue::Y: return ClipId::TailWag;
        default: return ClipId::Pounce;
    }
}

u32 cueColour(Cue c) {
    switch (c) {
        case Cue::A: return col(220, 80, 80);
        case Cue::B: return col(230, 190, 70);
        case Cue::X: return col(80, 130, 220);
        case Cue::Y: return col(80, 180, 110);
        default: return col(236, 150, 210);
    }
}
const char* cueLabel(Cue c) {
    static const char* const kLabels[] = {"A", "B", "X", "Y", ""};
    return kLabels[static_cast<int>(c) < 5 ? static_cast<int>(c) : 4];
}
u32 cueKey(Cue c) {
    switch (c) {
        case Cue::A: return KEY_A;
        case Cue::B: return KEY_B;
        case Cue::X: return KEY_X;
        case Cue::Y: return KEY_Y;
        default: return 0;
    }
}

// The camera for the phase: along the row (as near as the dragon's size allows), at the judges,
// or the whole stage.
void wantCamera(const App& app, const Valley& v, Vec3& eye, Vec3& target) {
    const Show& s = sh();
    const Layout& L = gladeLayout();
    const bool one = (s.phase == Phase::Look || s.phase == Phase::Poise || s.phase == Phase::Rivals) && s.turn < kEntrants;
    if (one || s.phase == Phase::Perform || s.phase == Phase::PerformCards) {
        const int spot = one ? s.turn : s.spot[0];
        const Dragon& d = entrantDragon(app, s.onSpot[spot]);
        const float size = kindSize(d) * (d.stage == Stage::Adult ? 1.0f : d.stage == Stage::Hatchling ? 0.45f : 0.7f);
        const float r = 1.2f + 2.6f * size;  // how much to frame, metres
        const Vec2 at = L.rivals[spot];
        // (looking down a good way: the far valley out of the picture keeps the view in budget)
        eye = gladePoint(v, {at.x + 0.35f * r, at.y + 2.3f * r}, L.stage.z + 1.6f * r);
        target = gladePoint(v, at, L.stage.z + 0.4f * r);
    } else if (s.phase == Phase::Judges) {  // from the stage's side of their table: their faces
        const Vec2 toStage{L.stage.x - L.judges.x, L.stage.y - L.judges.y};
        const float len = std::sqrt(toStage.x * toStage.x + toStage.y * toStage.y);
        const Vec2 d{toStage.x / len, toStage.y / len};
        eye = gladePoint(v, {L.judges.x + d.x * 5.0f - d.y * 1.5f, L.judges.y + d.y * 5.0f + d.x * 1.5f}, 4.2f);
        target = gladePoint(v, L.judges, 1.2f);
    } else {
        eye = gladePoint(v, {L.stage.x, L.stage.y + 15.0f}, L.stage.z + 8.5f);
        target = gladePoint(v, {L.stage.x, L.stage.y}, L.stage.z + 1.3f);
    }
}

// The round a phase judges (-1: none).
int roundOf(Phase p) {
    return p == Phase::Look ? kRoundLook : p == Phase::Poise ? kRoundPoise
         : (p == Phase::PerformCards || p == Phase::Rivals) ? kRoundPerformance : -1;
}

void cardsUp(int e, int round) {
    Show& s = sh();
    if (s.shown[e][round]) return;
    s.shown[e][round] = true;
    s.total[e] += s.cards[e][round][0] + s.cards[e][round][1] + s.cards[e][round][2];
    const float avg = (s.cards[e][round][0] + s.cards[e][round][1] + s.cards[e][round][2]) / 3.0f;
    audio::playSfx(audio::Sfx::Pose, 0.8f + avg * 0.05f, 0.8f);
}

// ---- The Performance: judge a press against the cues near now.
void pressed(App& app, Cue c) {
    Show& s = sh();
    const Dragon& d = app.game.dragons[s.partner];
    const float perfect = pageant::perfectWindow(d), good = pageant::goodWindow(d);
    int best = -1;
    float bestErr = 1e9f;
    for (int i = 0; i < s.routine.count; ++i) {
        if (s.judged[i] || s.routine.cue[i] != c) continue;
        const float err = s.perfT - s.routine.at[i];
        if (std::fabs(err) <= good && std::fabs(err) < std::fabs(bestErr)) {
            best = i;
            bestErr = err;
        }
    }
    if (best < 0) return;  // (a press with nothing near: no harm)
    s.judged[best] = true;
    s.hits[best] = pageant::judgeHit(bestErr, perfect, good);
    s.feedback = s.hits[best] == Hit::Perfect ? str::kShowPerfect : str::kShowGood;
    s.feedbackT = 0.8f;
    pose(app, 0, trickClip(c), 0.9f);
    audio::playSfx(s.hits[best] == Hit::Perfect ? audio::Sfx::Twirl : audio::Sfx::Tap, s.hits[best] == Hit::Perfect ? 1.1f : 1.0f);
}

void updatePerform(App& app, const Input& in) {
    Show& s = sh();
    const float before = s.perfT;
    s.perfT += app.dt;
    // A soft tick on each beat, so the cues have a pulse to land on.
    const int beat = static_cast<int>(std::floor(s.perfT / s.routine.beat));
    if (beat != s.lastBeat && s.perfT >= 0) {
        s.lastBeat = beat;
        audio::playSfx(audio::Sfx::Tap, beat % 4 == 0 ? 1.5f : 1.25f, 0.35f);
    }
    if (g_autoplay) {  // scripted runs: every cue on its moment
        for (int i = 0; i < s.routine.count; ++i)
            if (!s.judged[i] && s.routine.at[i] > before && s.routine.at[i] <= s.perfT) pressed(app, s.routine.cue[i]);
    } else {
        for (Cue c : {Cue::A, Cue::B, Cue::X, Cue::Y})
            if (in.down & cueKey(c)) pressed(app, c);
        if (in.tapped && in.ty > 150) pressed(app, Cue::Touch);  // the stars: a tap on the pad below the lane
    }
    // Cues gone by unhit are missed.
    const Dragon& d = app.game.dragons[s.partner];
    for (int i = 0; i < s.routine.count; ++i)
        if (!s.judged[i] && s.perfT - s.routine.at[i] > pageant::goodWindow(d)) {
            s.judged[i] = true;
            s.hits[i] = Hit::Miss;
            s.feedback = str::kShowMiss;
            s.feedbackT = 0.8f;
            audio::playSfx(audio::Sfx::Whiff, 1.0f, 0.5f);
        }
    if (s.perfT >= s.routine.length) {
        s.score[0][kRoundPerformance] = pageant::performanceScore(s.hits, s.routine.count);
        Rng rng(static_cast<std::uint64_t>(s.attempt) * 7919u + s.league * 131u + s.slot);
        pageant::judgeCards(s.score[0][kRoundPerformance], kRoundPerformance, rng, s.cards[0][kRoundPerformance]);
        s.phase = Phase::PerformCards;
        s.t = 0;
        pose(app, 0, ClipId::Greet, 1.6f);
    }
}

// ---- The results
void finish(App& app) {
    Show& s = sh();
    float perf[kEntrants];
    for (int e = 0; e < kEntrants; ++e) perf[e] = s.score[e][kRoundPerformance];
    pageant::placings(s.total, perf, s.order);
    for (int k = 0; k < kEntrants; ++k)
        if (s.order[k] == 0) s.place = k;
    Dragon& mine = app.game.dragons[s.partner];
    s.reward = pageant::finishShow(app.game, mine, s.league, s.slot, s.theme, s.place, dayIndex(nowLocal(app)));
    saveNow(app);
    // What the host says: the winner, then you, then your prizes.
    const ThemeInfo& t = themeInfo(s.theme);
    int n = 0;
    std::snprintf(s.lines[n++], sizeof(s.lines[0]), str::kShowWinner, entrantDragon(app, s.order[0]).name, t.name);
    if (s.place == 0) std::snprintf(s.lines[n++], sizeof(s.lines[0]), str::kShowRibbon, t.name);
    else std::snprintf(s.lines[n++], sizeof(s.lines[0]), str::kShowPlaced, mine.name, str::kShowPlaces[s.place]);
    if (s.reward.accessory >= 0) std::snprintf(s.lines[n++], sizeof(s.lines[0]), str::kShowPrize, accessoryInfo(s.reward.accessory).name);
    if (s.reward.leagueWon) {
        std::snprintf(s.lines[n++], sizeof(s.lines[0]), str::kShowTitle, mine.name, trainer::showTitleName(s.league));
        if (s.reward.dye > 0) std::snprintf(s.lines[n++], sizeof(s.lines[0]), str::kShowDyePrize, dyeInfo(s.reward.dye).name);
    }
    if (s.place == 0 && !s.reward.paid && n < 5) std::snprintf(s.lines[n++], sizeof(s.lines[0]), "%s", str::kShowPaidAlready);
    hostSays(app, n);
    audio::playSfx(s.place == 0 ? audio::Sfx::Victory : audio::Sfx::Ribbon);
    if (s.place == 0) audio::playSfx(audio::Sfx::Ribbon, 1.1f, 0.8f);
    s.youClip = s.place == 0 ? "cheer" : "wave";
    for (int k = 0; k < kEntrants; ++k) pose(app, s.order[k], k == 0 ? ClipId::Hop : ClipId::Idle, 2.0f);
}

// Sparkles round a point (world): `count` little gold stars circling it, `radius` metres out.
void sparkles(const App& app, Vec3 at, int count, float radius) {
    float x, y, ppu;
    if (!r3d::project(at, x, y, ppu)) return;
    for (int k = 0; k < count; ++k) {
        const float a = app.t * 1.3f + k * 0.9f, r = (1.0f + 0.4f * std::sin(app.t * 2 + k)) * ppu * radius;
        const float sx = x + std::cos(a) * r, sy = y + std::sin(a) * r * 0.7f;
        const float size = 3 + 2 * std::sin(app.t * 5 + k);
        C2D_DrawRectSolid(sx - size, sy - 0.5f, 0, size * 2, 1.2f, theme::kClutchGold);
        C2D_DrawRectSolid(sx - 0.5f, sy - size, 0, 1.2f, size * 2, theme::kClutchGold);
    }
}

}  // namespace

void setShowAutoplay(bool on) { g_autoplay = on; }
bool showOn() { return g_on; }
bool showJudgesSeen() {
    const Phase p = sh().phase;
    return g_on && (p == Phase::Intro || p == Phase::Judges || p == Phase::Results || p == Phase::Done);
}

bool showHostSpot(const Valley& v, Vec3& at, float& heading) {
    if (!g_on) return false;
    const Layout& L = gladeLayout();  // at the stage's front-left, turned half to the stage, half out
    at = gladePoint(v, {L.stage.x - 6.4f, L.stage.y + 2.8f});
    heading = headingTo(at, gladePoint(v, {L.stage.x, L.stage.y + 5.0f}));
    return true;
}

void beginShow(App& app, vext::Stage& stage, int league, int slot) {
    Show& s = sh();
    const int attempt = s.attempt + 1;
    s = Show{};
    s.attempt = attempt;
    s.partner = stage.partner;
    s.league = league < 1 ? 1 : (league > kLeagues ? kLeagues : league);
    s.slot = slot < 0 || slot >= kShowSlots ? 0 : slot;
    const s32 day = dayIndex(nowLocal(app));
    s.theme = pageant::showTheme(s.league, s.slot, day);
    pageant::makeRivals(app.game, s.league, s.slot, day, s.rivals);
    // Where everyone stands: you somewhere in the row, the rivals in the others.
    Rng rng(static_cast<std::uint64_t>(day) * 131u + s.league * 17u + s.slot * 5u + attempt);
    const int yours = static_cast<int>(rng.below(kEntrants));
    for (int e = 0, next = 0; e < kEntrants; ++e) {
        s.spot[e] = e == 0 ? yours : (next == yours ? ++next : next);
        if (e > 0) ++next;
        s.onSpot[s.spot[e]] = e;
    }
    // The scores for Look and Poise now (as it stands), the rivals' every round, and the cards.
    const Dragon& mine = app.game.dragons[s.partner];
    Rgb pal[kPalCount];
    kindPalette(mine.kind, mine.variant, mine.id, pal);
    applyDye(mine.dye, pal);
    s.score[0][kRoundLook] = pageant::lookScore(mine, s.theme, pal);
    s.score[0][kRoundPoise] = pageant::poiseScore(mine);
    Rng judge(static_cast<std::uint64_t>(day) * 7919u + attempt * 104729u + s.league * 131u + s.slot);
    for (int e = 1; e < kEntrants; ++e)
        for (int round = 0; round < kRounds; ++round) s.score[e][round] = pageant::rivalRound(s.rivals[e - 1], round, s.theme, judge);
    for (int e = 0; e < kEntrants; ++e)
        for (int round = 0; round < kRounds; ++round)
            if (e > 0 || round != kRoundPerformance) pageant::judgeCards(s.score[e][round], round, judge, s.cards[e][round]);
    s.routine = pageant::makeRoutine(s.league, static_cast<u32>(day * 31 + s.slot + attempt));
    s.perfT = -2.0f * s.routine.beat;  // a count-in
    for (int r = 0; r < kRivals; ++r) playRival(app, r, ClipId::Idle);
    // The host's welcome.
    const ThemeInfo& t = themeInfo(s.theme);
    std::snprintf(s.lines[0], sizeof(s.lines[0]), str::kShowWelcome, t.name);
    std::snprintf(s.lines[1], sizeof(s.lines[1]), str::kShowRivals, s.rivals[0].trainer, s.rivals[0].dragon.name,
                  s.rivals[1].trainer, s.rivals[1].dragon.name);
    g_on = true;
    audio::playSfx(audio::Sfx::Notice);
    (void)stage;
}

bool updateShow(App& app, const Input& in, vext::Stage& stage) {
    Show& s = sh();
    if (!stage.valley || s.partner < 0 || s.partner >= app.game.dragonCount) {
        g_on = false;
        return false;
    }
    const Valley& v = *stage.valley;
    const Layout& L = gladeLayout();
    s.t += app.dt;
    if (s.feedbackT > 0) s.feedbackT -= app.dt;
    // You at the stage's front corner, your dragon on its spot, both facing out.
    stage.pal = spotAt(v, s.spot[0]);
    stage.palHeading = headingTo(stage.pal, audience(v));
    stage.you = gladePoint(v, {L.stage.x + 6.2f, L.stage.y + 3.6f});
    stage.youHeading = headingTo(stage.you, spotAt(v, s.spot[0]));
    // Its clip: a trick or a pose for a while, then back to idle.
    if (s.queued != ClipId::Count) {  // a frame of idle, so the same clip starts again next frame
        stage.palClip = ClipId::Idle;
        s.palClip = s.queued;
        s.queued = ClipId::Count;
    } else {
        if (s.palClipFor > 0 && (s.palClipFor -= app.dt) <= 0) s.palClip = ClipId::Idle;
        stage.palClip = s.palClip;
    }
    stage.youClip = s.youClip;
    switch (s.phase) {
        case Phase::Intro:
            if (!s.said) {
                s.said = true;
                hostSays(app, 2);
                break;
            }
            s.phase = Phase::Look;
            s.t = 0;
            s.turn = 0;
            s.said = false;
            break;
        case Phase::Look:
        case Phase::Poise:
        case Phase::Rivals: {
            const int round = roundOf(s.phase);
            if (!s.said) {  // the host calls the round
                s.said = true;
                s.turn = 0;
                s.t = 0;
                if (s.phase == Phase::Look) std::snprintf(s.lines[0], sizeof(s.lines[0]), str::kShowLookLine, themeInfo(s.theme).name);
                else if (s.phase == Phase::Poise) std::snprintf(s.lines[0], sizeof(s.lines[0]), "%s", str::kShowPoiseLine);
                else std::snprintf(s.lines[0], sizeof(s.lines[0]), "%s", str::kShowRivalsPerform);
                hostSays(app, 1);
                break;
            }
            const int e = s.onSpot[s.turn];
            if (s.phase == Phase::Rivals && e == 0) {  // (yours was scored straight after your turn)
                s.turn++;
                s.t = 0;
            } else {
                if (s.t - app.dt <= 0.3f && s.t > 0.3f)
                    pose(app, e, s.phase == Phase::Look ? (e % 2 ? ClipId::WingFlutter : ClipId::LookAround)
                                  : s.phase == Phase::Poise ? ClipId::Sit : ClipId::Hop,
                         kTurn - 0.3f);
                if (s.phase == Phase::Rivals && s.t - app.dt <= 1.1f && s.t > 1.1f) pose(app, e, ClipId::PlayBow, 1.2f);
                if (s.t >= kCardsAt) cardsUp(e, round);
                if (s.t >= kTurn) {
                    pose(app, e, ClipId::Idle, 0.1f);
                    s.turn++;
                    s.t = 0;
                }
            }
            if (s.turn >= kEntrants) {
                s.said = false;
                s.t = 0;
                s.phase = s.phase == Phase::Look ? Phase::Poise : s.phase == Phase::Poise ? Phase::Judges : Phase::Results;
                if (s.phase == Phase::Results) finish(app);
            }
            break;
        }
        case Phase::Judges:  // the judges confer; then the Performance
            if (s.t > 1.6f && !s.said) {
                s.said = true;
                std::snprintf(s.lines[0], sizeof(s.lines[0]), "%s", str::kShowPerfLine);
                hostSays(app, 1);
                break;
            }
            if (s.said) {
                s.phase = Phase::Perform;
                s.t = 0;
                s.said = false;
            }
            break;
        case Phase::Perform: updatePerform(app, in); break;
        case Phase::PerformCards:
            if (s.t >= 0.6f) cardsUp(0, kRoundPerformance);
            if (s.t >= 2.0f) {
                s.phase = Phase::Rivals;
                s.t = 0;
                s.said = false;
            }
            break;
        case Phase::Results:
            if ((s.t > 0.5f && (in.down & KEY_A)) || (g_autoplay && s.t > 5.0f)) s.phase = Phase::Done;
            break;
        case Phase::Done:
            g_on = false;
            return false;
    }
    // The camera eases to where the phase wants it.
    Vec3 eye, target;
    wantCamera(app, v, eye, target);
    if (!s.camSet) {
        s.eye = eye, s.target = target, s.camSet = true;
    } else {
        const float k = clampf(app.dt * 2.6f, 0.0f, 1.0f);
        s.eye = lerp(s.eye, eye, k);
        s.target = lerp(s.target, target, k);
    }
    stage.camSet = true;
    stage.eye = s.eye;
    stage.target = s.target;
    return true;
}

void showView(App& app, const vext::Stage& stage, r3d::ValleyView& view) {
    Show& s = sh();
    if (!stage.valley) return;
    const Valley& v = *stage.valley;
    const Layout& L = gladeLayout();
    // The rivals on their spots, animated here (they idle on while the host talks, too).
    view.otherCount = 0;
    for (int r = 0; r < kRivals && view.otherCount < r3d::kMaxOthers; ++r) {
        const Dragon& d = s.rivals[r].dragon;
        if (const AnimLibrary* lib = r3d::animsFor(d)) {
            s.actors[r].anim.update(*lib, app.dt, nullptr, 0);
            s.actors[r].eyes.update(0.0f, app.dt);
            if (s.rivalClip[r] != ClipId::Idle && s.actors[r].anim.finished(*lib)) playRival(app, r, ClipId::Idle);
        }
        r3d::ValleyDragon& o = view.others[view.otherCount++];
        o.dragon = &d;
        o.actor = &s.actors[r];
        o.at = spotAt(v, s.spot[r + 1]);
        o.heading = headingTo(o.at, audience(v));
        o.lite = true;  // four on a stage: the lighter models
    }
    view.at.z += L.stage.z;  // yours stands up on the stage too
    view.lead = false;
    view.shadow = 0;
}

// ---- 2D over the picture
void showDrawTop(App& app, const vext::Stage& stage) {
    Show& s = sh();
    if (!stage.valley) return;
    const Valley& v = *stage.valley;
    const ThemeInfo& t = themeInfo(s.theme);
    const int round = roundOf(s.phase);
    // The banner: the show and the round.
    panel({100, 6, 200, 34}, col(40, 26, 56, 0.8f));
    textCentered(app, t.name, 200, 17, 0.55f, theme::kClutchGold, 190, Face::Title);
    const char* what = round >= 0 ? roundName(round) : s.phase == Phase::Perform ? roundName(kRoundPerformance)
                     : s.phase == Phase::Results || s.phase == Phase::Done ? str::kShowResults : "";
    textCentered(app, what, 200, 33, 0.4f, theme::kShell, 190);
    // The one being judged: its name over it, and the judges' cards once they're up.
    const bool turns = (s.phase == Phase::Look || s.phase == Phase::Poise || s.phase == Phase::Rivals) && s.turn < kEntrants;
    const int e = turns ? s.onSpot[s.turn] : (s.phase == Phase::Perform || s.phase == Phase::PerformCards) ? 0 : -1;
    if (e >= 0) {
        float x, y, ppu;
        const Vec3 over = spotAt(v, s.spot[e]) + Vec3{0, 0, 3.2f * kindSize(entrantDragon(app, e))};
        if (r3d::project(over, x, y, ppu)) {
            const char* name = entrantDragon(app, e).name;
            const float w = textWidth(app, name, 0.5f) + 16;
            panel({x - w / 2, y - 12, w, 22}, e == 0 ? col(245, 196, 81, 0.9f) : col(250, 240, 220, 0.88f));
            textCentered(app, name, x, y, 0.5f, theme::kDenPlum, w);
        }
        const int r = round >= 0 ? round : kRoundPerformance;
        if (s.shown[e][r]) {
            for (int j = 0; j < kJudges; ++j) {
                const Rect c{110.0f + j * 62.0f, 172, 56, 50};
                panel(c, col(252, 248, 238, 0.95f));
                char num[8];
                std::snprintf(num, sizeof(num), str::kShowPoints, s.cards[e][r][j]);
                textCentered(app, num, c.x + c.w / 2, c.y + 20, 0.75f, theme::kDenPlum, c.w - 4);
                textCentered(app, str::kJudgeNames[j], c.x + c.w / 2, c.y + 41, 0.32f, withAlpha(theme::kDenPlum, 0.7f), c.w - 4);
            }
        }
    }
    if (turns && s.t >= kCardsAt && s.t < kCardsAt + 1.0f && e >= 0) {  // the cards go up: a shimmer, more for more
        const int r = round >= 0 ? round : kRoundPerformance;
        const float avg = (s.cards[e][r][0] + s.cards[e][r][1] + s.cards[e][r][2]) / 3.0f;
        sparkles(app, spotAt(v, s.spot[e]) + Vec3{0, 0, 1.2f}, 2 + static_cast<int>(avg * 0.6f), 1.0f);
    }
    if (s.phase == Phase::Perform && s.feedback && s.feedbackT > 0)  // how that cue went
        textCentered(app, s.feedback, 200, 140 - 20 * (0.8f - s.feedbackT), 0.8f,
                     withAlpha(s.feedback == str::kShowMiss ? theme::kAsh : theme::kClutchGold, clampf(s.feedbackT * 2, 0, 1)), 200);
    if (s.phase == Phase::Results || s.phase == Phase::Done) {  // the placings, and sparkles round the winner
        panel({96, 150, 208, 84}, col(40, 26, 56, 0.85f));
        for (int k = 0; k < kEntrants; ++k) {
            const int who = s.order[k];
            char line[64];
            std::snprintf(line, sizeof(line), "%s  %s", str::kShowPlaces[k], entrantDragon(app, who).name);
            text(app, line, 106, 155 + k * 19, 0.45f, who == 0 ? theme::kClutchGold : theme::kShell, C2D_AlignLeft, 140);
            std::snprintf(line, sizeof(line), str::kShowPoints, s.total[who]);
            text(app, line, 294, 155 + k * 19, 0.45f, who == 0 ? theme::kClutchGold : theme::kShell, C2D_AlignRight);
        }
        sparkles(app, spotAt(v, s.spot[s.order[0]]) + Vec3{0, 0, 1.4f}, 7, 1.2f);
    }
}

void showDrawBottom(App& app, const Input& in, const vext::Stage& stage) {
    (void)stage;
    Show& s = sh();
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    if (s.phase == Phase::Perform) {  // the lane: cues sliding in to the ring
        textCentered(app, str::kShowCueHelp, 160, 14, 0.42f, theme::kShell, 300);
        panel({8, 62, 304, 56}, withAlpha(theme::kShell, 0.1f));
        C2D_DrawCircleSolid(kRingX, 90, 0.5f, 22, withAlpha(theme::kClutchGold, 0.35f));
        C2D_DrawCircleSolid(kRingX, 90, 0.5f, 18, theme::kDenPlum);
        for (int i = s.routine.count - 1; i >= 0; --i) {
            if (s.judged[i] && s.hits[i] != Hit::Miss) continue;  // (hit: gone)
            const float x = kRingX + (s.routine.at[i] - s.perfT) * kLaneSpeed;
            if (x < -20 || x > kBotW + 20) continue;
            const Cue c = s.routine.cue[i];
            const float fade = s.judged[i] ? 0.35f : 1.0f;
            if (c == Cue::Touch) {
                for (int k = 0; k < 5; ++k) {
                    const float a = -1.5708f + k * 1.2566f;
                    C2D_DrawTriangle(x + std::cos(a) * 16, 90 + std::sin(a) * 16, withAlpha(cueColour(c), fade), x + std::cos(a + 0.6f) * 6,
                                     90 + std::sin(a + 0.6f) * 6, withAlpha(cueColour(c), fade), x + std::cos(a - 0.6f) * 6,
                                     90 + std::sin(a - 0.6f) * 6, withAlpha(cueColour(c), fade), 0.5f);
                }
                C2D_DrawCircleSolid(x, 90, 0.5f, 7, withAlpha(cueColour(c), fade));
            } else {
                C2D_DrawCircleSolid(x, 90, 0.5f, 15, withAlpha(theme::kDenPlum, fade));
                C2D_DrawCircleSolid(x, 90, 0.5f, 13, withAlpha(cueColour(c), fade));
                textCentered(app, cueLabel(c), x, 90, 0.6f, withAlpha(theme::kShell, fade), 24);
            }
        }
        // The pad for the stars.
        const Rect pad{60, 150, 200, 80};
        const bool starNear = [&] {
            for (int i = 0; i < s.routine.count; ++i)
                if (!s.judged[i] && s.routine.cue[i] == Cue::Touch && std::fabs(s.routine.at[i] - s.perfT) < 0.6f) return true;
            return false;
        }();
        panel(pad, starNear ? withAlpha(cueColour(Cue::Touch), 0.5f) : withAlpha(theme::kShell, 0.1f));
        textCentered(app, str::kShowTap, pad.x + pad.w / 2, pad.y + pad.h / 2, 0.7f, theme::kShell, pad.w);
        return;
    }
    // The scoreboard: each entrant along the stage, the rounds' points as the cards go up.
    textCentered(app, themeInfo(s.theme).name, 160, 13, 0.55f, theme::kClutchGold, 300, Face::Title);
    static const char* const kHead[kRounds + 1] = {nullptr, nullptr, nullptr, str::kShowTotal};
    for (int c = 0; c <= kRounds; ++c)
        textCentered(app, c < kRounds ? roundName(c) : kHead[c], 150 + c * 44, 36, 0.34f, withAlpha(theme::kShell, 0.7f), 42);
    for (int k = 0; k < kEntrants; ++k) {
        const int e = s.onSpot[k];
        const float y = 48 + k * 34;
        panel({6, y, 308, 30}, e == 0 ? withAlpha(theme::kClutchGold, 0.3f) : withAlpha(theme::kShell, 0.1f));
        text(app, entrantDragon(app, e).name, 12, y + 2, 0.42f, theme::kShell, C2D_AlignLeft, 110);
        text(app, entrantTrainer(app, e), 12, y + 16, 0.32f, withAlpha(theme::kShell, 0.65f), C2D_AlignLeft, 110);
        for (int r = 0; r < kRounds; ++r) {
            if (!s.shown[e][r]) continue;
            char num[8];
            std::snprintf(num, sizeof(num), str::kShowPoints, s.cards[e][r][0] + s.cards[e][r][1] + s.cards[e][r][2]);
            textCentered(app, num, 150 + r * 44, y + 15, 0.45f, theme::kShell, 42);
        }
        char num[8];
        std::snprintf(num, sizeof(num), str::kShowPoints, s.total[e]);
        textCentered(app, num, 150 + kRounds * 44, y + 15, 0.48f, theme::kClutchGold, 42);
    }
    if (s.phase == Phase::Results) {
        char line[48] = {};
        if (s.reward.gleam) std::snprintf(line, sizeof(line), str::kShowGleam, static_cast<unsigned long>(s.reward.gleam));
        text(app, line, 12, 196, 0.45f, theme::kClutchGold, C2D_AlignLeft, 150);
        if (button(app, {176, 190, 136, 40}, str::kShowContinue, in, theme::kClutchGold)) s.phase = Phase::Done;
    }
}

}  // namespace ec::glade
