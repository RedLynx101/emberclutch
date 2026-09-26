// Fruit Catch (Beta WP11): at Honeyroot Orchard, with Maple (it's her quest). You flick a fruit
// from the basket on the bottom screen up and away down the meadow; your dragon watches it go,
// runs, and leaps high, snaps it up or dives for it at the last moment (a juvenile or a hatchling
// hops and tumbles: the hop version, softer throws); then it trots back and eats it. Points for
// the distance and the style, golden pears double; eight throws. A miss: it eats it anyway.
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/challenge_stage.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"

namespace ec::fruit {
namespace {

using stage::Set;

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

enum class Step : u8 { Ready, Flying, Caught, Return, Eat, End };

struct Play {
    challenge::FruitSetup setup{};
    bool young = false;
    u32 seed = 0;
    int throwIndex = 0, score = 0, caught = 0;
    Step step = Step::Ready;
    float t = 0;
    challenge::Toss toss;
    challenge::CatchPlan plan;
    challenge::Catcher catcher;
    challenge::Fruit fruit = challenge::Fruit::Apple;
    Vec3 home;                 // where the dragon waits, beside you
    float homeHeading = 0;
    Vec3 hand;                 // where a throw leaves your hand
    Vec3 runFrom, stopAt;      // the dragon's run
    Vec3 fruitAt;              // the fruit, as drawn
    bool fruitShown = false, inMouth = false;
    Vec3 bounceVel;            // a missed fruit bouncing on the grass
    float spin = 0;
    // The stylus on the basket: its recent positions (a flick throws).
    bool holding = false;
    Vec2 samples[8];
    float sampleT[8] = {};
    int sampleCount = 0;
    Vec2 heldAt;
    float groundZ = 0;
};

Play& play() {
    static Play p;
    return p;
}

constexpr float kFruitSize = 0.34f;  // metres across as drawn (a big storybook apple, readable far off)
constexpr Vec2 kBasket{160, 178};    // the basket's fruit on the bottom screen

Vec3 forwardOf(float heading) { return {std::sin(heading), -std::cos(heading), 0}; }

void nextThrow(Set& s, Play& p) {
    p.step = Step::Ready;
    p.t = 0;
    p.fruit = challenge::fruitFor(p.throwIndex, p.seed);
    p.fruitShown = p.inMouth = false;
    s.dragonAt = p.home;
    s.dragonHeading = p.homeHeading;
    stage::playDragon(s, ClipId::Idle, 0.3f);
    if (p.fruit == challenge::Fruit::Golden) stage::popup(s, str::kGoldenNext, theme::kClutchGold);
}

// A throw leaves your hand: the dragon's plan to catch it (or not) is set there and then.
void throwIt(App& app, Set& s, Play& p, float fx, float fy) {
    challenge::Toss t;
    if (!challenge::tossFrom(fx, fy, p.hand, s.youHeading, p.young, t)) {
        audio::playSfx(audio::Sfx::Bounce, 1.3f, 0.3f);  // too soft: it drops back in the basket
        return;
    }
    p.toss = t;
    p.plan = challenge::planCatch(t, p.catcher, p.groundZ);
    p.step = Step::Flying;
    p.t = 0;
    p.fruitShown = true;
    p.runFrom = s.dragonAt;
    const Vec3 to{p.plan.at.x, p.plan.at.y, p.groundZ};
    Vec3 d = to - p.runFrom;
    d.z = 0;
    const float len = std::sqrt(d.x * d.x + d.y * d.y);
    const float reach = p.young ? 0.3f : 0.7f;
    p.stopAt = len > reach ? p.runFrom + d * ((len - reach) / len) : p.runFrom;
    audio::playSfx(audio::Sfx::FruitToss);
    stage::playPerson(s.you, "wave", 1.4f, 0.1f, true);
    (void)app;
}

}  // namespace

void begin(App& app, Set& s) {
    Play& p = play();
    p = Play{};
    p.young = challenge::young(s.shown);
    p.setup = challenge::fruitSetup(s.cup, p.young);
    p.seed = app.rng.next();
    s.youAt = stage::onGround(s, challenge::fruitYou());
    s.youHeading = stage::worldHeading(s, 0.0f);  // down the meadow, the orchard's front
    p.home = stage::onGround(s, challenge::fruitDragon(p.young));
    p.homeHeading = s.youHeading;
    p.groundZ = p.home.z;
    const Vec3 fwd = forwardOf(s.youHeading), right{-fwd.y, fwd.x, 0};
    p.hand = s.youAt + fwd * 0.35f + right * -0.25f + Vec3{0, 0, 1.25f};
    float run = s.natRun;
    p.catcher = challenge::catcherFor(s.shown, p.home, run, stage::dragonSize(s));
    s.riding = false;
    s.host = Villager::Market;
    s.hostShown = true;
    s.hostAt = stage::onGround(s, {-2.4f, 11.6f});
    // The camera behind you, a little up, looking down the meadow.
    s.wantEye = s.youAt - fwd * 6.2f + right * -0.9f + Vec3{0, 0, 3.5f};
    s.wantTarget = s.youAt + fwd * 11.0f + Vec3{0, 0, 0.6f};
    s.snapCam = true;
    stage::playPerson(s.you, "idle");
    stage::playPerson(s.hostFig, "idle");
    nextThrow(s, p);
    stage::popup(s, str::kFlickFruit, theme::kShell);
}

void update(App& app, Set& s, const Input& in) {
    Play& p = play();
    p.t += app.dt;
    if (in.down & KEY_X) {
        stage::giveUp(s);
        return;
    }

    float speed = 0;
    switch (p.step) {
        case Step::Ready:
            if (s.autoplay && p.t > 0.9f) {
                const float power = 0.5f + 0.25f * ((p.throwIndex * 37) % 10) / 10.0f;
                throwIt(app, s, p, ((p.throwIndex * 53) % 7 - 3) * 60.0f, -(250 + 1500 * power));
            }
            break;
        case Step::Flying: {
            const challenge::CatchPlan& c = p.plan;
            // The fruit, until it's caught (or down on the grass).
            if (p.t < c.t) {
                p.fruitAt = challenge::fruitAt(p.toss, p.t);
                p.spin += app.dt * 9.0f;
            }
            // The dragon: a moment watching, the run, a wait under it, then the catch.
            const float jumpFor = p.young ? 0.5f : 0.7f;
            Vec3 at = s.dragonAt;
            if (p.t < c.leaveAt) {
                s.dragonHeading = stage::headingTo(s.dragonAt, c.at);
            } else {
                const float runFor = std::fmax(0.05f, c.arriveAt - c.leaveAt);
                const float u = clampf((p.t - c.leaveAt) / runFor, 0.0f, 1.0f);
                at = lerp(p.runFrom, p.stopAt, u);
                if (u < 1.0f) {
                    speed = length(p.stopAt - p.runFrom) / runFor;
                    stage::playDragon(s, p.young ? ClipId::Scamper : ClipId::Gallop, 0.15f);
                    s.dragonHeading = stage::headingTo(p.runFrom, p.stopAt);
                } else if (c.style != challenge::Style::Missed && p.t < c.t - jumpFor * 0.5f) {
                    stage::playDragon(s, ClipId::Idle, 0.2f);
                    s.dragonHeading = stage::headingTo(at, c.at);
                }
            }
            // The jump (its peak at the catch), the dive (a lunge forward, low), or a lunge.
            float up = 0;
            if (c.style == challenge::Style::Dive || c.style == challenge::Style::Tumble) {
                const float u = clampf((p.t - (c.t - 0.35f)) / 0.35f, 0.0f, 1.0f);
                if (u > 0) {
                    stage::playDragon(s, ClipId::Pounce, 0.1f);
                    at = at + forwardOf(s.dragonHeading) * (p.catcher.dive * u);
                    up = 0.35f * std::sin(u * 3.14159f);
                }
            } else if (c.style != challenge::Style::Missed) {
                const float u = (p.t - (c.t - jumpFor * 0.5f)) / jumpFor;
                if (u > 0 && u < 1) {
                    stage::playDragon(s, p.young ? ClipId::Hop : ClipId::LeapCatch, 0.1f, s.clip != ClipId::Hop && s.clip != ClipId::LeapCatch);
                    up = std::fmax(c.jump, 0.15f) * 4.0f * u * (1.0f - u);
                }
            }
            at.z = s.valley->heightAt(at.x, at.y) + up;
            s.dragonAt = at;
            if (p.t >= c.t) {
                p.step = Step::Caught;
                p.t = 0;
                if (c.style == challenge::Style::Missed) {  // down on the grass, bouncing
                    p.fruitAt = challenge::fruitAt(p.toss, c.t);
                    p.fruitAt.z = p.groundZ + kFruitSize * 0.5f;
                    p.bounceVel = p.toss.vel * 0.3f;
                    p.bounceVel.z = std::fabs(p.toss.vel.z) * 0.35f;
                    audio::playSfx(audio::Sfx::Bounce, 1.0f, 0.7f);
                } else {
                    p.inMouth = true;
                    const bool golden = p.fruit == challenge::Fruit::Golden;
                    const int pts = challenge::catchPoints(c, golden, p.young);
                    p.score += pts;
                    ++p.caught;
                    char line[48];
                    std::snprintf(line, sizeof(line), "+%d  %s", pts, challenge::styleName(c.style));
                    stage::popup(s, line, golden ? theme::kClutchGold : theme::kShell);
                    audio::playSfx(audio::Sfx::FruitCatch);
                    if (c.style == challenge::Style::Dive || c.style == challenge::Style::SkyLeap) audio::playSfx(audio::Sfx::Giggle, 1.1f, 0.6f);
                    stage::burst(s, Fx::Sparkle, c.at, golden ? 10 : 6, 1.4f);
                    stage::playPerson(s.hostFig, "cheer", 1.0f, 0.2f, true);
                }
            }
            break;
        }
        case Step::Caught: {  // the landing (a dive: a shake), or the miss: the fruit bounces, it runs up
            const challenge::CatchPlan& c = p.plan;
            if (c.style == challenge::Style::Missed) {
                if (p.fruitAt.z > p.groundZ + kFruitSize * 0.5f || p.bounceVel.z > 0.3f) {
                    p.bounceVel.z -= challenge::kGravity * app.dt;
                    p.fruitAt = p.fruitAt + p.bounceVel * app.dt;
                    if (p.fruitAt.z < p.groundZ + kFruitSize * 0.5f) {
                        p.fruitAt.z = p.groundZ + kFruitSize * 0.5f;
                        p.bounceVel = p.bounceVel * 0.45f;
                        p.bounceVel.z = std::fabs(p.bounceVel.z);
                    }
                    p.spin += app.dt * 6.0f;
                }
                // It runs on up to the fruit, then eats it where it lies.
                Vec3 d = p.fruitAt - s.dragonAt;
                d.z = 0;
                const float len = std::sqrt(d.x * d.x + d.y * d.y), reach = p.young ? 0.35f : 0.8f;
                if (len > reach + 0.05f) {  // (it stops at `reach`: a margin, or it closes in forever)
                    const float step = std::fmin(len - reach, p.catcher.run * app.dt);
                    s.dragonAt = s.dragonAt + d * (step / len);
                    s.dragonAt.z = s.valley->heightAt(s.dragonAt.x, s.dragonAt.y);
                    s.dragonHeading = stage::headingTo(s.dragonAt, p.fruitAt);
                    speed = p.catcher.run;
                    stage::playDragon(s, p.young ? ClipId::Scamper : ClipId::Gallop, 0.15f);
                } else {
                    p.step = Step::Eat;
                    p.t = 0;
                    stage::playDragon(s, ClipId::Eat, 0.2f, true);
                    stage::popup(s, str::kAteItAnyway, theme::kShell);
                    audio::playSfx(audio::Sfx::Sniff);
                }
            } else {
                s.dragonAt.z = s.valley->heightAt(s.dragonAt.x, s.dragonAt.y);
                if (p.t < 0.05f && (c.style == challenge::Style::Dive || c.style == challenge::Style::Tumble)) {
                    stage::playDragon(s, ClipId::Shake, 0.2f, true);
                    audio::playSfx(audio::Sfx::Thump, 1.0f, 0.6f);
                }
                if (p.t > 0.6f) {
                    p.step = Step::Return;
                    p.t = 0;
                }
            }
            break;
        }
        case Step::Return: {  // back to its place beside you (the fruit in its mouth)
            Vec3 d = p.home - s.dragonAt;
            d.z = 0;
            const float len = std::sqrt(d.x * d.x + d.y * d.y);
            const bool far = len > 5.0f;  // from far off it bounds back; near, it trots the last bit
            const float pace = far ? p.catcher.run * 0.75f : std::fmax(s.natTrot * 1.4f, 2.5f);
            if (len > 0.15f) {
                const float step = std::fmin(len, pace * app.dt);
                s.dragonAt = s.dragonAt + d * (step / len);
                s.dragonAt.z = s.valley->heightAt(s.dragonAt.x, s.dragonAt.y);
                s.dragonHeading = stage::headingTo(s.dragonAt, p.home);
                speed = pace;
                stage::playDragon(s, far ? (p.young ? ClipId::Scamper : ClipId::Gallop) : ClipId::Trot, 0.25f);
            } else if (p.inMouth) {
                p.step = Step::Eat;
                p.t = 0;
                s.dragonHeading = stage::headingTo(s.dragonAt, s.youAt);
                stage::playDragon(s, ClipId::Eat, 0.2f, true);
            } else {
                ++p.throwIndex;
                if (p.throwIndex >= p.setup.throws) {
                    p.step = Step::End;
                    p.t = 0;
                } else {
                    nextThrow(s, p);
                }
            }
            break;
        }
        case Step::Eat:  // munch munch, and a happy wag
            if (p.t > 0.35f && p.t - app.dt <= 0.35f) audio::playSfx(audio::Sfx::Munch);
            if (p.t > 0.8f && p.t - app.dt <= 0.8f) {
                audio::playSfx(audio::Sfx::Munch, 1.05f);
                stage::burst(s, Fx::Heart, stage::mouthOf(s) + Vec3{0, 0, 0.4f}, 2);
                p.fruitShown = p.inMouth = false;
            }
            if (p.t > 1.3f) {
                if (length(p.home - s.dragonAt) > 0.3f) {  // (a miss: eaten where it fell) back home
                    p.step = Step::Return;
                    p.t = 0;
                } else {
                    ++p.throwIndex;
                    if (p.throwIndex >= p.setup.throws) {
                        p.step = Step::End;
                        p.t = 0;
                        stage::playDragon(s, ClipId::TailWag, 0.2f, true);
                    } else {
                        nextThrow(s, p);
                    }
                }
            }
            break;
        case Step::End:
            if (p.t > 1.4f) {
                char line[64];
                std::snprintf(line, sizeof(line), "%d points: %d of %d caught  (goal %d)", p.score, p.caught, p.setup.throws, p.setup.goal);
                stage::finish(app, s, p.score, challenge::fruitOutcome(p.setup, p.score), line);
            }
            break;
    }
    if (p.inMouth) p.fruitAt = stage::mouthOf(s);
    // Maple watches the dragon; your throw's done, you stand easy.
    s.hostHeading = stage::headingTo(s.hostAt, s.dragonAt);
    if (stage::personClipDone(s.you)) stage::playPerson(s.you, "idle", 1.0f, 0.3f);
    if (stage::personClipDone(s.hostFig)) stage::playPerson(s.hostFig, "idle", 1.0f, 0.3f);
    if (p.step == Step::Ready && stage::dragonClipDone(s)) stage::playDragon(s, ClipId::Idle, 0.3f);
    stage::stepDragon(app, s, speed);
    // The camera follows the dragon out a little way.
    const Vec3 fwd = forwardOf(s.youHeading), right{-fwd.y, fwd.x, 0};
    const Vec3 lane = s.youAt + fwd * 11.0f;
    s.wantEye = s.youAt - fwd * 6.2f + right * -0.9f + Vec3{0, 0, 3.5f};
    s.wantTarget = lerp(lane, s.dragonAt, 0.35f) + Vec3{0, 0, 0.6f};
}

void scene(App& app, Set& s) {
    Play& p = play();
    const Vec3 fwd = forwardOf(s.youHeading), right{-fwd.y, fwd.x, 0};
    r3d::ChallengeProp basket;  // the basket at your feet
    basket.kind = r3d::PropKind::Basket;
    basket.at = s.youAt + right * -0.9f + fwd * 0.2f;
    basket.at.z = s.valley->heightAt(basket.at.x, basket.at.y);
    basket.yaw = s.youHeading;
    basket.scale = 1.1f;
    basket.look = basketLook();
    stage::addProp(s, basket);
    if (p.fruitShown) {
        r3d::ChallengeProp f;
        f.kind = r3d::PropKind::Fruit;
        f.variant = static_cast<u8>(p.fruit);
        f.at = p.fruitAt - Vec3{0, 0, kFruitSize * 0.5f};
        f.yaw = s.youHeading;
        f.pitch = p.inMouth ? 0.0f : p.spin;
        f.scale = kFruitSize * (p.inMouth ? 0.75f : 1.0f);
        f.look = fruitLook(p.fruit);
        stage::addProp(s, f);
    }
    (void)app;
}

void hud(App& app, Set& s) {
    Play& p = play();
    {  // little distance flags down the left of the meadow: how far a throw goes
        const Vec3 fwd = forwardOf(s.youHeading), left{-fwd.y, fwd.x, 0};
        const int step = p.young ? 2 : 5, count = p.young ? 4 : 5;
        const u32 pole = theme::rgba(150, 104, 66), cloth = fromRgb(challenge::cupColour(s.cup));
        for (int k = count; k >= 1; --k) {
            Vec3 at = s.youAt + fwd * static_cast<float>(k * step) + left * 2.8f;
            at.z = s.valley->heightAt(at.x, at.y);
            float x, y, ppu, xt, yt, pt;
            if (!r3d::project(at, x, y, ppu) || !r3d::project(at + Vec3{0, 0, 1.2f}, xt, yt, pt)) continue;
            C2D_DrawLine(x, y, pole, xt, yt, pole, std::fmax(1.0f, 0.05f * ppu), 0);
            C2D_DrawTriangle(xt, yt, cloth, xt + 0.5f * pt, yt + 0.14f * pt, cloth, xt, yt + 0.3f * pt, cloth, 0);
            char num[8];
            std::snprintf(num, sizeof(num), "%d", k * step);
            text(app, num, xt - 2, yt - 0.34f * pt - 6, std::fmax(0.3f, std::fmin(0.5f, pt / 90.0f)), theme::kShell, C2D_AlignRight);
        }
    }
    // The flying fruit's shadow on the grass and a glint trail: where it is, how high.
    if (p.fruitShown && !p.inMouth) {
        float x, y, ppu;
        const Vec3 ground{p.fruitAt.x, p.fruitAt.y, s.valley->heightAt(p.fruitAt.x, p.fruitAt.y) + 0.05f};
        if (r3d::project(ground, x, y, ppu)) {
            const float h = p.fruitAt.z - ground.z, k = clampf(1.0f - h / 12.0f, 0.3f, 1.0f);
            C2D_DrawEllipseSolid(x - 0.3f * ppu, y - 0.1f * ppu, 0, 0.6f * ppu, 0.2f * ppu, withAlpha(theme::kDenPlum, 0.35f * k));
        }
        if (p.step == Step::Flying)
            for (int k = 1; k <= 4; ++k) {
                const Vec3 back = challenge::fruitAt(p.toss, std::fmax(0.0f, p.t - k * 0.05f));
                if (r3d::project(back, x, y, ppu))
                    C2D_DrawCircleSolid(x, y, 0, std::fmax(1.0f, 0.025f * ppu * (5 - k)),
                                        withAlpha(p.fruit == challenge::Fruit::Golden ? theme::kClutchGold : theme::kShell, 0.15f * (5 - k)));
            }
    }
    char line[32];
    std::snprintf(line, sizeof(line), str::kThrowOf, std::min(p.throwIndex + 1, p.setup.throws), p.setup.throws);
    panel({8, 8, 110, 28}, withAlpha(theme::kDenPlum, 0.7f));
    textCentered(app, line, 63, 22, 0.5f, theme::kShell, 106);
    std::snprintf(line, sizeof(line), str::kPoints, p.score);
    panel({282, 8, 110, 28}, withAlpha(theme::kDenPlum, 0.7f));
    textCentered(app, line, 337, 22, 0.55f, p.score >= p.setup.goal ? theme::kClutchGold : theme::kShell, 106);
}

void bottom(App& app, Set& s, const Input& in) {
    Play& p = play();
    textCentered(app, str::kFlickFruit, 160, 20, 0.5f, theme::kClutchGold, 300);
    // The throws to come: a fruit each (the golden ones shine).
    for (int i = 0; i < p.setup.throws; ++i) {
        const float x = 160 + (i - (p.setup.throws - 1) * 0.5f) * 26.0f;
        const challenge::Fruit f = challenge::fruitFor(i, p.seed);
        const bool gone = i < p.throwIndex || (i == p.throwIndex && p.step != Step::Ready);
        C2D_DrawCircleSolid(x, 48, 0, 8, withAlpha(fromRgb(challenge::fruitColour(f)), gone ? 0.2f : 1.0f));
        if (f == challenge::Fruit::Golden && !gone) C2D_DrawCircleSolid(x - 2, 45, 0, 2.5f, theme::kShell);
    }
    // The score against the goal.
    const Rect bar{40, 66, 240, 10};
    panel(bar, theme::kTrack);
    panel({bar.x, bar.y, bar.w * clampf(static_cast<float>(p.score) / std::max(1, p.setup.goal), 0, 1), bar.h}, theme::kClutchGold);
    char line[40];
    std::snprintf(line, sizeof(line), str::kGoalPoints, p.setup.goal);
    textCentered(app, line, 160, 88, 0.4f, withAlpha(theme::kShell, 0.8f), 300);
    // The basket, with the next fruit on top (touch it, flick it up and away).
    C2D_DrawEllipseSolid(kBasket.x - 58, kBasket.y + 4, 0, 116, 44, theme::rgba(150, 104, 62));
    C2D_DrawEllipseSolid(kBasket.x - 50, kBasket.y + 2, 0, 100, 16, theme::rgba(120, 80, 46));
    for (int k = 0; k < 5; ++k)  // wicker
        C2D_DrawLine(kBasket.x - 50 + k * 25, kBasket.y + 10, theme::rgba(204, 164, 104), kBasket.x - 44 + k * 22, kBasket.y + 44,
                     theme::rgba(204, 164, 104), 2, 0);
    const bool ready = p.step == Step::Ready;
    Vec2 at = kBasket;
    at.y -= 8;
    if (p.holding) at = p.heldAt;
    if (ready) {
        const u32 c = fromRgb(challenge::fruitColour(p.fruit));
        C2D_DrawCircleSolid(at.x, at.y, 0, 22, c);
        C2D_DrawCircleSolid(at.x - 7, at.y - 8, 0, 6, withAlpha(theme::kShell, 0.5f));
        C2D_DrawRectSolid(at.x - 1.5f, at.y - 30, 0, 3, 9, theme::rgba(110, 80, 50));
        C2D_DrawTriangle(at.x + 1, at.y - 27, theme::rgba(96, 170, 70), at.x + 14, at.y - 32, theme::rgba(96, 170, 70), at.x + 5,
                         at.y - 20, theme::rgba(96, 170, 70), 0);
    }
    // The flick: hold the fruit, sweep up and let go; its speed at the end throws it.
    if (ready && !s.autoplay) {
        if (in.touching) {
            if (!p.holding && std::hypot(in.tx - kBasket.x, in.ty - (kBasket.y - 8)) < 40) {
                p.holding = true;
                p.sampleCount = 0;
            }
            if (p.holding) {
                p.heldAt = {in.tx, in.ty};
                if (p.sampleCount == 8) {
                    for (int k = 1; k < 8; ++k) p.samples[k - 1] = p.samples[k], p.sampleT[k - 1] = p.sampleT[k];
                    p.sampleCount = 7;
                }
                p.samples[p.sampleCount] = {in.tx, in.ty};
                p.sampleT[p.sampleCount++] = app.t;
            }
        } else if (p.holding) {
            p.holding = false;
            int k = 0;  // the stroke's last tenth of a second
            while (k < p.sampleCount - 2 && app.t - p.sampleT[k] > 0.12f) ++k;
            const float dt = std::fmax(1.0f / 60.0f, p.sampleT[p.sampleCount - 1] - p.sampleT[k]);
            const Vec2 a = p.samples[k], b = p.samples[p.sampleCount - 1];
            if (p.sampleCount >= 2) throwIt(app, s, p, (b.x - a.x) / dt, (b.y - a.y) / dt);
        }
    }
    textCentered(app, str::kGiveUp, 160, 228, 0.4f, withAlpha(theme::kShell, 0.6f), 300);
}

}  // namespace ec::fruit
