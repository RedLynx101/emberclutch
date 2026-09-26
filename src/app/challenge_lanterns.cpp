// The Lantern Trial (Beta WP10): in the arena, before the festival stage, a row of crystal
// lanterns light up in a pattern, each with its own chime; then you tap them on the bottom screen
// (or pick with the D-pad and A) in the same order, and your dragon turns and breathes each one
// alight, its element's own breath (flame, spores, a gust, mist, frost, light). A lantern more
// each round; a wrong one costs a heart and shows the round again. A clever dragon (Wit 7 and
// up) glances at the right lantern when you hesitate. Wren judges from the stage; the campaign's
// last quest wins it on the festival night, and then the great lantern blazes.
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/challenge_stage.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/kinds.hpp"

namespace ec::lanterns {
namespace {

using stage::Set;

constexpr float kLanternScale = 1.5f;  // the crystal lanterns drawn big: they read from behind you

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

enum class Step : u8 { Intro, Show, Input, Breath, Cleared, Wrong, End };

struct Play {
    challenge::Trial trial;
    Step step = Step::Intro;
    float t = 0;
    int shown = 0;  // the pattern's lantern being shown
    float lit[challenge::kTrialLanterns] = {};
    int lantern = -1;  // the one breathed at
    float travel = 0.55f;
    float idle = 0;
    int hint = -1;
    float hintT = 0;
    int cursor = 0;
    float turnTo = 0;  // the heading the dragon turns to
    bool tapped = false;
};

Play& play() {
    static Play p;
    return p;
}

int element(const Set& s) { return kindInfo(s.shown.kind < kindCount() ? s.shown.kind : 0).elements[0]; }

audio::Sfx breathSound(int e) {
    switch (challenge::breathFor(e).kind) {
        case challenge::Breath::Flame: return audio::Sfx::BreathFlame;
        case challenge::Breath::Mist: return audio::Sfx::BreathMist;
        case challenge::Breath::Gust: return audio::Sfx::BreathGust;
        case challenge::Breath::Spores: return audio::Sfx::BreathSpores;
        case challenge::Breath::Frost: return audio::Sfx::BreathFrost;
        default: return audio::Sfx::BreathLight;
    }
}

Vec3 crystalAt(const Set& s, int k) {
    const Play& p = play();
    return stage::onGround(s, challenge::trialLantern(p.trial.setup.lanterns, k), kCrystalHeight * kLanternScale);
}

void chime(int k, float gain = 0.9f) { audio::playSfx(audio::Sfx::LanternLight, challenge::trialPitch(k), gain); }

void go(Play& p, Step step) {
    p.step = step;
    p.t = 0;
    p.shown = 0;
    p.idle = 0;
    p.hint = -1;
}

// A lantern breathed at: the dragon turns, opens its mouth, breathes; the puffs fly to it.
void breatheAt(App& app, Set& s, Play& p, int k) {
    p.lantern = k;
    p.turnTo = stage::headingTo(s.dragonAt, crystalAt(s, k));
    const float breath = s.shown.stats[3] ? s.shown.stats[3] : 5;
    p.travel = clampf(0.66f - 0.02f * breath, 0.4f, 0.66f);  // a strong Breath reaches quicker
    go(p, Step::Breath);
    audio::playSfx(breathSound(element(s)), 1.0f + (challenge::young(s.shown) ? 0.15f : 0.0f));
    (void)app;
}

// Where each lantern's button is on the bottom screen: the arc as seen from behind you (the
// arena's -X on your right).
Vec2 buttonAt(int lanterns, int k) {
    const Vec2 l = challenge::trialLantern(lanterns, k);
    return {160 - l.x * 21.0f, 206 + l.y * 18.0f};
}

}  // namespace

void begin(App& app, Set& s) {
    Play& p = play();
    p = Play{};
    p.trial.begin(s.cup, app.rng.next());
    s.youAt = stage::onGround(s, challenge::trialYou());
    s.dragonAt = stage::onGround(s, challenge::trialDragon());
    const Vec3 middle = stage::onGround(s, challenge::trialLantern(p.trial.setup.lanterns, 0)) * 0.5f +
                        stage::onGround(s, challenge::trialLantern(p.trial.setup.lanterns, p.trial.setup.lanterns - 1)) * 0.5f;
    s.youHeading = stage::headingTo(s.youAt, middle);
    s.dragonHeading = p.turnTo = stage::headingTo(s.dragonAt, middle);
    s.dragonPitch = s.dragonRoll = 0;
    s.riding = false;
    s.host = Villager::Steward;
    s.hostShown = true;
    s.hostAt = stage::onGround(s, {2.8f, -16.2f}, 1.0f);  // on the festival stage
    s.hostHeading = stage::worldHeading(s, 0);
    const float big = stage::dragonSize(s);  // a big dragon on the star: the camera looks over it
    s.wantEye = stage::onGround(s, {0.9f, 8.6f + 2.6f * big}, 3.0f + 3.4f * big);
    s.wantTarget = stage::onGround(s, {0.0f, -2.6f}, 1.4f);
    s.snapCam = true;
    stage::playDragon(s, ClipId::Idle);
    stage::playPerson(s.you, "idle");
    stage::playPerson(s.hostFig, "idle");
    stage::popup(s, str::kWatch, theme::kShell);
    go(p, Step::Intro);
}

void update(App& app, Set& s, const Input& in) {
    Play& p = play();
    challenge::Trial& tr = p.trial;
    const int n = tr.setup.lanterns;
    p.t += app.dt;
    for (float& l : p.lit) l = std::fmax(0.0f, l - app.dt * 1.6f);
    if (in.down & KEY_X) {
        stage::giveUp(s);
        return;
    }
    switch (p.step) {
        case Step::Intro:
            if (p.t > 1.4f) go(p, Step::Show);
            break;
        case Step::Show: {  // each lantern of the round's pattern lit in turn, with its chime
            const float step = tr.setup.show;
            const int i = static_cast<int>(p.t / step);
            if (i < tr.length() && i >= p.shown) {
                p.shown = i + 1;
                const int k = tr.pattern[i];
                p.lit[k] = 1.3f;
                chime(k);
                p.turnTo = stage::headingTo(s.dragonAt, crystalAt(s, k));  // it watches too
                stage::burst(s, Fx::Sparkle, crystalAt(s, k), 3, 1.2f);
            }
            if (p.t > step * tr.length() + 0.35f) {
                go(p, Step::Input);
                stage::popup(s, str::kYourTurn, theme::kClutchGold);
                audio::playSfx(audio::Sfx::Confirm, 1.1f, 0.6f);
            }
            break;
        }
        case Step::Input: {
            p.idle += app.dt;
            if (in.down & KEY_DLEFT) p.cursor = (p.cursor + n - 1) % n;
            if (in.down & KEY_DRIGHT) p.cursor = (p.cursor + 1) % n;
            int pick = -1;
            if (in.down & KEY_A) pick = p.cursor;
            if (p.tapped) pick = p.cursor;  // (a button tapped on the bottom screen)
            p.tapped = false;
            if (s.autoplay && p.t > 0.55f) pick = tr.pattern[tr.input];
            if (pick >= 0) {
                breatheAt(app, s, p, pick);
            } else if (p.idle > 4.0f && p.hint < 0 && s.shown.stats[1] >= 7) {  // a clever dragon's glance
                p.hint = tr.pattern[tr.input];
                p.hintT = 1.2f;
                p.turnTo = stage::headingTo(s.dragonAt, crystalAt(s, p.hint));
                audio::playSfx(audio::Sfx::Trill, 1.3f, 0.5f);
            }
            if (p.hint >= 0 && (p.hintT -= app.dt) <= 0) p.idle = -99;  // once a turn
            break;
        }
        case Step::Breath: {
            const int k = p.lantern;
            s.actor.jawOpen = clampf(p.t * 6.0f, 0.0f, 1.0f) * (p.t < p.travel ? 1.0f : 0.0f);
            if (p.t < 0.3f && s.breath.count() < 50)  // a stream for its first moments
                s.breath.emit(challenge::breathFor(element(s)), stage::mouthOf(s), crystalAt(s, k), 3, p.travel * 0.8f);
            if (p.t >= p.travel) {
                const challenge::Trial::Press r = tr.press(k);
                s.actor.jawOpen = 0;
                if (r == challenge::Trial::Press::Wrong || r == challenge::Trial::Press::Lost) {
                    p.lit[k] = 0.25f;
                    audio::playSfx(audio::Sfx::Grumble, 1.1f, 0.7f);
                    stage::burst(s, Fx::Puff, crystalAt(s, k), 6, 1.5f);
                    stage::playPerson(s.hostFig, "surprised", 1.0f, 0.15f, true);
                    if (r == challenge::Trial::Press::Lost) {
                        go(p, Step::End);
                    } else {
                        stage::popup(s, str::kWatchAgain, theme::kRose);
                        go(p, Step::Wrong);
                    }
                    break;
                }
                p.lit[k] = 1.4f;
                chime(k);
                stage::burst(s, Fx::Sparkle, crystalAt(s, k), 6, 1.5f);
                if (r == challenge::Trial::Press::Right) {
                    go(p, Step::Input);
                    p.t = 0.3f;
                } else {
                    for (float& l : p.lit) l = 1.2f;  // the round: every lantern flashes
                    audio::playSfx(audio::Sfx::CrowdCheer, 1.1f, r == challenge::Trial::Press::Won ? 1.0f : 0.5f);
                    stage::playPerson(s.hostFig, "cheer", 1.0f, 0.2f, true);
                    stage::playDragon(s, ClipId::TailWag, 0.2f, true);
                    stage::burst(s, Fx::Heart, stage::mouthOf(s) + Vec3{0, 0, 0.5f}, 3);
                    if (r == challenge::Trial::Press::Won) {
                        go(p, Step::End);
                    } else {
                        stage::popup(s, str::kRoundCleared, theme::kClutchGold);
                        go(p, Step::Cleared);
                    }
                }
            }
            break;
        }
        case Step::Cleared:
        case Step::Wrong:
            if (p.t > 1.5f) {
                go(p, Step::Show);
                stage::playDragon(s, ClipId::Idle, 0.3f);
                stage::playPerson(s.hostFig, "idle", 1.0f, 0.3f);
            }
            break;
        case Step::End:
            if (tr.won) {
                for (float& l : p.lit) l = 1.2f;
                if (p.t < 0.05f) stage::playPerson(s.you, "cheer", 1.0f, 0.2f, true);
            }
            if (p.t > 2.0f) {
                char line[64];
                if (tr.won)
                    std::snprintf(line, sizeof(line), "Every round!  %d point%s, %d heart%s left", tr.score, tr.score == 1 ? "" : "s",
                                  tr.hearts, tr.hearts == 1 ? "" : "s");
                else
                    std::snprintf(line, sizeof(line), "%d of %d rounds, %d points", tr.roundsCleared(), tr.setup.rounds, tr.score);
                stage::finish(app, s, tr.score, challenge::trialOutcome(tr), line);
            }
            break;
    }
    // The dragon turns to what it's looking at; you and Wren watch.
    const float err = std::remainder(p.turnTo - s.dragonHeading, 6.2831853f);
    s.dragonHeading += clampf(err, -5.0f * app.dt, 5.0f * app.dt);
    if (stage::dragonClipDone(s) && s.clip != ClipId::Idle) stage::playDragon(s, ClipId::Idle, 0.3f);
    if (stage::personClipDone(s.you)) stage::playPerson(s.you, "idle", 1.0f, 0.3f);
    if (stage::personClipDone(s.hostFig)) stage::playPerson(s.hostFig, "idle", 1.0f, 0.3f);
    stage::stepDragon(app, s, 0);
    (void)in;
}

void scene(App& app, Set& s) {
    Play& p = play();
    const int n = p.trial.setup.lanterns;
    for (int k = 0; k < n; ++k) {
        r3d::ChallengeProp prop;
        prop.kind = r3d::PropKind::Crystal;
        prop.at = stage::onGround(s, challenge::trialLantern(n, k));
        prop.yaw = stage::headingTo(prop.at, s.youAt) + 3.14159265f;
        prop.scale = kLanternScale;
        const float glow = clampf(p.lit[k], 0.0f, 1.0f);
        prop.look = crystalLook(k, glow);
        stage::addProp(s, prop);
    }
    (void)app;
}

void hud(App& app, Set& s) {
    Play& p = play();
    const int n = p.trial.setup.lanterns;
    for (int k = 0; k < n; ++k) {  // each lit crystal's light round it
        const float l = clampf(p.lit[k], 0.0f, 1.0f);
        float x, y, ppu;
        if (l <= 0.02f || !r3d::project(crystalAt(s, k), x, y, ppu)) continue;
        const u32 c = fromRgb(challenge::trialColour(k));
        C2D_DrawCircleSolid(x, y, 0, 1.4f * ppu, withAlpha(c, 0.18f * l));
        C2D_DrawCircleSolid(x, y, 0, 0.7f * ppu, withAlpha(c, 0.3f * l));
        C2D_DrawCircleSolid(x, y, 0, 0.25f * ppu, withAlpha(theme::kShell, 0.7f * l));
    }
    char line[32];
    std::snprintf(line, sizeof(line), str::kRoundOf, std::min(p.trial.round + 1, p.trial.setup.rounds), p.trial.setup.rounds);
    panel({8, 8, 118, 28}, withAlpha(theme::kDenPlum, 0.7f));
    textCentered(app, line, 67, 22, 0.5f, theme::kShell, 114);
    panel({300, 8, 92, 28}, withAlpha(theme::kDenPlum, 0.7f));
    for (int h = 0; h < p.trial.setup.hearts; ++h)
        heart(318 + h * 26.0f, 22, 16, h < p.trial.hearts ? theme::kRose : withAlpha(theme::kShell, 0.2f));
}

void bottom(App& app, Set& s, const Input& in) {
    Play& p = play();
    const int n = p.trial.setup.lanterns;
    const bool yourTurn = p.step == Step::Input || p.step == Step::Breath;
    textCentered(app, yourTurn ? str::kTapLanterns : str::kWatch, 160, 22, 0.5f, yourTurn ? theme::kClutchGold : theme::kShell, 300);
    // How far through the round you are.
    for (int i = 0; i < p.trial.length(); ++i) {
        const float x = 160 + (i - (p.trial.length() - 1) * 0.5f) * 16.0f;
        const bool done = yourTurn ? i < p.trial.input : (p.step == Step::Show && i < p.shown);
        C2D_DrawCircleSolid(x, 46, 0, 5, done ? theme::kClutchGold : withAlpha(theme::kShell, 0.25f));
    }
    for (int k = 0; k < n; ++k) {
        const Vec2 b = buttonAt(n, k);
        const u32 c = fromRgb(challenge::trialColour(k));
        const float l = clampf(p.lit[k], 0.0f, 1.0f);
        if (yourTurn && k == p.cursor) C2D_DrawCircleSolid(b.x, b.y, 0, 29, withAlpha(theme::kShell, 0.5f));
        C2D_DrawCircleSolid(b.x, b.y, 0, 26 + 4 * l, withAlpha(c, 0.25f + 0.4f * l));
        // A crystal: a tall diamond, lit brighter.
        const u32 face = withAlpha(c, 0.55f + 0.45f * l), shine = withAlpha(theme::kShell, 0.25f + 0.6f * l);
        C2D_DrawTriangle(b.x, b.y - 20, face, b.x + 11, b.y, face, b.x - 11, b.y, face, 0);
        C2D_DrawTriangle(b.x - 11, b.y, face, b.x + 11, b.y, face, b.x, b.y + 16, face, 0);
        C2D_DrawTriangle(b.x, b.y - 20, shine, b.x + 4, b.y - 2, shine, b.x - 3, b.y - 2, shine, 0);
        if (p.hint == k && p.hintT > 0) {
            const float tw = 6 + 3 * std::sin(app.t * 12);
            C2D_DrawRectSolid(b.x + 16 - tw / 2, b.y - 20, 0, tw, 2, theme::kShell);
            C2D_DrawRectSolid(b.x + 15, b.y - 20 - tw / 2 + 1, 0, 2, tw, theme::kShell);
        }
        if (p.step == Step::Input && in.released && std::hypot(in.rx - b.x, in.ry - b.y) < 27) {
            p.cursor = k;
            p.tapped = true;
        }
    }
    textCentered(app, str::kGiveUp, 160, 226, 0.4f, withAlpha(theme::kShell, 0.6f), 300);
    (void)s;
}

}  // namespace ec::lanterns
