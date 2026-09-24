#include "app/care_ui.hpp"

#include <citro2d.h>

#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/genetics.hpp"
#include "care.h"      // sprite indices (gfx/care.t3s, tools/blender/care_sprites.py)
#include "care_t3x.h"  // the sprite atlas, linked into the program

namespace ec::care {
namespace {

C2D_SpriteSheet g_sheet = nullptr;

enum FxKind : u8 { kFxHeart, kFxDust, kFxSparkle, kFxSuds, kFxDrop, kFxCrumb, kFxSteam };

constexpr float kTopBar = 26;      // the need gauges
constexpr float kTrayY = 202;      // the tool tray
constexpr float kFoodRowY = 164;   // the food picker, above the tray
constexpr Rect kView{0, kTopBar, 320, kFoodRowY - kTopBar};  // where the stylus meets the dragon
constexpr float kFlickSpeed = 160;  // px/s: faster than this on release throws the ball

constexpr Tool kTools[] = {Tool::Hand, Tool::Food, Tool::Brush, Tool::Cloth, Tool::Sponge, Tool::Ball};
constexpr int kToolCount = sizeof(kTools) / sizeof(kTools[0]);

std::size_t toolSprite(Tool t) {
    switch (t) {
        case Tool::Food: return care_food_skyberry_idx;
        case Tool::Brush: return care_brush_idx;
        case Tool::Cloth: return care_cloth_idx;
        case Tool::Sponge: return care_sponge_idx;
        case Tool::Ball: return care_ball_idx;
        default: return care_hand_idx;
    }
}

std::size_t foodSprite(Food f) { return care_food_firepepper_idx + static_cast<std::size_t>(f); }

void sprite(std::size_t index, float x, float y, float scale = 1.0f, float angle = 0.0f) {
    if (!g_sheet) return;
    C2D_Sprite s;
    C2D_SpriteFromSheet(&s, g_sheet, index);
    C2D_SpriteSetCenter(&s, 0.5f, 0.5f);
    C2D_SpriteSetPos(&s, x, y);
    C2D_SpriteSetScale(&s, scale, scale);
    C2D_SpriteSetRotation(&s, angle);
    C2D_SpriteSetDepth(&s, 0.5f);
    C2D_DrawSprite(&s);
}

float frand(App& app, float lo, float hi) { return lo + (hi - lo) * (app.rng.next() * (1.0f / 4294967296.0f)); }

void emit(App& app, u8 kind, Vec2 at, int count) {
    CareState& c = app.care;
    for (int i = 0; i < count && c.fxCount < 64; ++i) {
        CareFx& f = c.fx[c.fxCount++];
        f.kind = kind;
        f.pos = {at.x + frand(app, -6, 6), at.y + frand(app, -6, 6)};
        switch (kind) {
            case kFxHeart: f.vel = {frand(app, -12, 12), frand(app, -46, -30)}; f.maxLife = 1.0f; f.size = frand(app, 6, 9); break;
            case kFxDust: f.vel = {frand(app, -30, 30), frand(app, -26, -8)}; f.maxLife = 0.7f; f.size = frand(app, 3, 6); break;
            case kFxSparkle: f.vel = {frand(app, -20, 20), frand(app, -20, 20)}; f.maxLife = 0.6f; f.size = frand(app, 2, 4); break;
            case kFxSuds: f.vel = {frand(app, -4, 4), frand(app, -6, 2)}; f.maxLife = 6.0f; f.size = frand(app, 3, 7); break;
            case kFxDrop: f.vel = {frand(app, -8, 8), frand(app, 4, 14)}; f.maxLife = 2.5f; f.size = frand(app, 3, 6); break;
            case kFxCrumb: f.vel = {frand(app, -40, 40), frand(app, -30, 10)}; f.maxLife = 0.8f; f.size = frand(app, 1.5f, 3); break;
            default: f.vel = {frand(app, -6, 6), frand(app, -30, -18)}; f.maxLife = 1.2f; f.size = frand(app, 5, 9); break;
        }
        f.life = f.maxLife;
    }
}

void stepFx(App& app) {
    CareState& c = app.care;
    for (int i = 0; i < c.fxCount;) {
        CareFx& f = c.fx[i];
        f.life -= app.dt;
        if (f.life <= 0) {
            f = c.fx[--c.fxCount];
            continue;
        }
        if (f.kind == kFxCrumb || f.kind == kFxDrop) f.vel.y += 90 * app.dt;  // they fall
        f.pos.x += f.vel.x * app.dt;
        f.pos.y += f.vel.y * app.dt;
        ++i;
    }
}

void drawFx(App& app) {
    static const u32 kHeartC = theme::rgba(0xF2, 0x6D, 0x85), kDustC = theme::rgba(0xC8, 0xB4, 0x96),
                     kSparkC = theme::rgba(0xFF, 0xF6, 0xD0), kSudsC = theme::rgba(0xF4, 0xFA, 0xFF),
                     kDropC = theme::rgba(0x8C, 0xC8, 0xF0), kCrumbC = theme::rgba(0x9A, 0x62, 0x34),
                     kSteamC = theme::rgba(0xE8, 0xE8, 0xF0);
    for (int i = 0; i < app.care.fxCount; ++i) {
        const CareFx& f = app.care.fx[i];
        const float a = std::fmin(1.0f, f.life / (f.maxLife * 0.4f));
        switch (f.kind) {
            case kFxHeart: heart(f.pos.x, f.pos.y, f.size, withAlpha(kHeartC, a)); break;
            case kFxDust: C2D_DrawCircleSolid(f.pos.x, f.pos.y, 0.5f, f.size * (1.6f - a * 0.6f), withAlpha(kDustC, a * 0.7f)); break;
            case kFxSparkle:
                C2D_DrawRectSolid(f.pos.x - f.size, f.pos.y - 0.6f, 0.5f, f.size * 2, 1.2f, withAlpha(kSparkC, a));
                C2D_DrawRectSolid(f.pos.x - 0.6f, f.pos.y - f.size, 0.5f, 1.2f, f.size * 2, withAlpha(kSparkC, a));
                break;
            case kFxSuds:
                C2D_DrawCircleSolid(f.pos.x, f.pos.y, 0.5f, f.size, withAlpha(kSudsC, a * 0.85f));
                C2D_DrawCircleSolid(f.pos.x - f.size * 0.3f, f.pos.y - f.size * 0.3f, 0.5f, f.size * 0.3f, withAlpha(0xFFFFFFFF, a));
                break;
            case kFxDrop: C2D_DrawEllipseSolid(f.pos.x - f.size * 0.4f, f.pos.y - f.size * 0.6f, 0.5f, f.size * 0.8f, f.size * 1.2f, withAlpha(kDropC, a * 0.8f)); break;
            case kFxCrumb: C2D_DrawCircleSolid(f.pos.x, f.pos.y, 0.5f, f.size, withAlpha(kCrumbC, a)); break;
            default: C2D_DrawCircleSolid(f.pos.x, f.pos.y, 0.5f, f.size * (1.8f - a * 0.8f), withAlpha(kSteamC, a * 0.2f)); break;
        }
    }
}

// ------------------------------------------------------------------------------ tools
DenActor& actor(App& app) {  // the dragon you're caring for (care::update runs once it's set up)
    DenActor* a = careActor(app);
    return a ? *a : app.actors[0];
}

void noteSample(CareState& c, Vec2 p, float t) {
    if (c.sampleCount == 6) {
        for (int i = 1; i < 6; ++i) c.samples[i - 1] = c.samples[i], c.sampleT[i - 1] = c.sampleT[i];
        c.sampleCount = 5;
    }
    c.samples[c.sampleCount] = p;
    c.sampleT[c.sampleCount++] = t;
}

// The hand: strokes where you touch; the sweet spot, pokes, rough handling; held still off
// the dragon, a call.
void useHand(App& app, const Input& in, Dragon& d, bool hit, const TouchHit& h, Stroke kind) {
    CareState& c = app.care;
    DenBehavior& b = actor(app).behavior;
    if (!hit) {
        if (!c.onDragon && c.stroke.distance < 8) {  // held still off the dragon: "come here"
            c.stillTime += app.dt;
            if (c.stillTime > 0.7f && !c.reported) {
                c.reported = true;
                b.care(Care::Call, d);
                showToast(app, str::kComeHere);
            }
        }
        return;
    }
    if (kind == Stroke::Rough) {
        if (!c.reported) {
            c.reported = true;
            b.care(Care::Rough, d);
        }
        return;
    }
    if (kind == Stroke::None) return;
    actor(app).gazeLocal = h.local;  // lean toward the hand
    actor(app).gazeWeight = 0.55f;
    if ((c.petTick -= app.dt) <= 0) {
        c.petTick = 0.25f;
        pet(d, 3);
        markVisit(d, nowLocal(app));
        b.care(Care::Pet, d, h.zone);
    }
    if ((c.heartWait -= app.dt) <= 0) {
        c.heartWait = 0.35f;
        emit(app, kFxHeart, {in.tx, in.ty - 10}, 1);
    }
    if (atSweetSpot(d, h.zone, h.outward)) {
        c.sweetTime += app.dt;
        if (c.sweetTime > 0.8f && c.sweetCooldown <= 0) {
            c.sweetCooldown = 6.0f;
            b.care(Care::SweetSpot, d);
            emit(app, kFxHeart, {in.tx, in.ty - 10}, 4);
            if (!c.sweetFound) {
                c.sweetFound = true;
                addBond(d, 3);
                showToast(app, str::kSweetSpot);
            }
        }
    } else {
        c.sweetTime = 0;
    }
}

// Food: bites when it's held to the mouth; the jaw opens as it comes close.
void useFood(App& app, const Input& in, Dragon& d) {
    CareState& c = app.care;
    DenActor& a = actor(app);
    if (!c.holdingFood) return;
    a.behavior.care(Care::OfferFood, d);
    a.gazeLocal = r3d::closeUpLocal({in.tx, in.ty});
    a.gazeWeight = 0.8f;
    Vec2 mouth;
    if (!r3d::mouthOnCloseUp(mouth)) return;
    const float dist = std::hypot(in.tx - mouth.x, in.ty - mouth.y);
    const float open = dist < 22 ? 1.0f : (dist > 95 ? 0.0f : 1.0f - (dist - 22) / 73);
    a.jawOpen = c.chomp > 0 ? 0.0f : open;
    if (dist > 30 || c.biteWait > 0) return;
    const Taste taste = tasteOf(d, c.food);
    const FoodInfo& info = foodInfo(c.food);
    c.biteWait = 0.55f;
    c.chomp = 0.18f;
    if (taste == Taste::Disliked) {
        a.behavior.feedBite(true, false, false);
        c.holdingFood = false;
        showToast(app, str::kRefused);
        return;
    }
    --c.bitesLeft;
    audio::playSfx(audio::Sfx::Munch);
    emit(app, kFxCrumb, mouth, 5);
    const bool favourite = taste == Taste::Favorite;
    if (c.bitesLeft <= 0) {
        c.holdingFood = false;
        feed(d, info.belly, favourite);
        markVisit(d, nowLocal(app));
        a.behavior.feedBite(false, true, favourite);
        audio::playSfx(audio::Sfx::Gulp);
        if (favourite) {
            audio::playSfx(audio::Sfx::Trill);
            emit(app, kFxHeart, mouth, 5);
        }
        showToast(app, favourite ? str::kFedFavorite : str::kFed);
    } else {
        a.behavior.feedBite(false, false, favourite);
    }
}

// The brush and the cloth: strokes on the dragon fill its regions.
void useGroomTool(App& app, const Input& in, Dragon& d, bool hit, const TouchHit& h, float moved) {
    CareState& c = app.care;
    DenBehavior& b = actor(app).behavior;
    if (!hit || moved < 0.5f) return;
    b.care(h.region == kRegionBelly ? Care::GroomBelly : Care::GroomBody, d);
    const Vec2 at{in.tx, in.ty};
    if (c.tool == Tool::Brush) {
        const Vec2 dir = c.stroke.dir;
        const bool withGrain = dir.x * h.grain.x + dir.y * h.grain.y > 0.2f;
        const float dusty = d.dirt[h.region];
        if (c.groom.brush(d, h.region, moved / 520.0f, withGrain)) {
            emit(app, kFxSparkle, at, 6);
            audio::playSfx(audio::Sfx::Toast);
        }
        if (dusty > 5 && app.rng.chance(1, 3)) emit(app, kFxDust, at, 1);
        if ((c.soundWait -= app.dt) <= 0) {
            c.soundWait = 0.32f;
            audio::playSfx(audio::Sfx::Brush);
        }
    } else {  // the cloth: polishing counts fully once a region is brushed
        const float amount = moved / 450.0f * (c.groom.brushed[h.region] >= 0.5f ? 1.0f : 0.5f);
        if (c.groom.polish(d, h.region, amount)) emit(app, kFxSparkle, at, 8);
        if (app.rng.chance(1, 2)) emit(app, kFxSparkle, at, 1);
        if ((c.soundWait -= app.dt) <= 0) {
            c.soundWait = 0.45f;
            audio::playSfx(audio::Sfx::Sparkle);
        }
    }
    markVisit(d, nowLocal(app));
    if (c.groom.checkGleam()) {  // the whole dragon, brushed and polished
        for (int i = 0; i < 4; ++i) emit(app, kFxSparkle, {frand(app, 60, 260), frand(app, 50, 150)}, 5);
        audio::playSfx(audio::Sfx::Trill, 1.0f);
        addBond(d, 3);
        showToast(app, str::kGleaming);
    }
}

// The sponge: suds while it sits in the tub.
void useSponge(App& app, const Input& in, Dragon& d, bool hit, float moved) {
    CareState& c = app.care;
    const DenBehavior& b = actor(app).behavior;
    if (!hit || moved < 0.5f || b.activity != Activity::Bath || b.step != 2) return;
    c.suds = std::fmin(1.0f, c.suds + moved / 900.0f);
    if (app.rng.chance(1, 2)) emit(app, kFxSuds, {in.tx, in.ty}, 1);
    if ((c.soundWait -= app.dt) <= 0) {
        c.soundWait = 0.5f;
        audio::playSfx(audio::Sfx::Splash, 1.25f);
    }
    (void)d;
}

void rinse(App& app, Dragon& d) {
    CareState& c = app.care;
    audio::playSfx(audio::Sfx::Splash);
    for (int i = 0; i < 10; ++i) emit(app, kFxDrop, {frand(app, 80, 240), frand(app, 40, 90)}, 1);
    if (c.suds > 0.3f) {
        bathe(d);
        markVisit(d, nowLocal(app));
        showToast(app, str::kSplash);
    }
    c.suds = 0;
    actor(app).behavior.care(Care::BathDone, d);
}

// The ball: a flick throws it into the den, a slow drag rolls it.
void releaseBall(App& app, Dragon& d) {
    CareState& c = app.care;
    if (c.sampleCount < 2 || app.ball.held) return;
    const Vec2 a = c.samples[0], z = c.samples[c.sampleCount - 1];
    const float dt = std::fmax(0.016f, c.sampleT[c.sampleCount - 1] - c.sampleT[0]);
    const float vx = (z.x - a.x) / dt, vy = (z.y - a.y) / dt;
    const float speed = std::hypot(vx, vy);
    const float dragged = std::hypot(z.x - c.stroke.start.x, z.y - c.stroke.start.y);
    const DenLayout den;
    const Vec3 from{den.player.x, den.player.y + 0.3f, 1.0f};
    if (speed > kFlickSpeed && vy < 0) {  // flicked up and away: into the den
        const float s = std::fmin(speed, 900.0f);
        app.ball.launch(from, {vx * 0.006f, -vy * 0.011f + 1.2f, 1.5f + s * 0.0035f});
    } else if (dragged > 30) {  // a slow drag: roll it along the floor
        app.ball.launch({from.x, from.y, app.ball.radius}, {(z.x - c.stroke.start.x) * 0.012f, 2.2f, 0});
    } else {
        return;
    }
    actor(app).behavior.ball = &app.ball;
    actor(app).behavior.care(Care::Throw, d);
    play(d, 6);
    markVisit(d, nowLocal(app));
}

void selectTool(App& app, Dragon& d, Tool t) {
    CareState& c = app.care;
    if (c.tool == t) return;
    if (c.tool == Tool::Sponge && c.bathOut) actor(app).behavior.care(Care::BathDone, d);  // leaving the bath
    c.tool = t;
    c.holdingFood = false;
    if (t == Tool::Sponge && !d.upset) {
        c.bathOut = true;
        c.suds = 0;
        actor(app).behavior.care(Care::Bath, d);
    }
    audio::playSfx(audio::Sfx::Tap);
}

// The little profile card (tap the heartglow): who it is, and renaming (D27). Alpha 2's
// Dragons tab grows this into the full profile.
void drawProfile(App& app, const Input& in, const Dragon& d, s64 now) {
    CareState& c = app.care;
    panel({22, 34, 276, 166}, withAlpha(theme::kDenPlum, 0.94f));
    text(app, d.name, 160, 42, 0.85f, theme::kClutchGold, C2D_AlignCenter, 200);
    if (denRoster(app.game).dragonCount + denRoster(app.game).eggCount > 1) {  // the others in the den
        if (button(app, {30, 42, 34, 28}, "<", in)) cycleCare(app, -1);
        if (button(app, {256, 42, 34, 28}, ">", in)) cycleCare(app, 1);
    }
    char line[80];
    std::snprintf(line, sizeof(line), "%s %s %s", sexName(d.sex), breedName(d.genome), stageName(d.stage));
    text(app, line, 160, 74, 0.5f, theme::kShell);
    std::snprintf(line, sizeof(line), "%s: %s", str::kPersonality, personalityName(d.personality));
    text(app, line, 160, 94, 0.5f, theme::kShell);
    std::snprintf(line, sizeof(line), "%s %d   -   %s %d", str::kBond, d.bond, str::kDay, daysSinceHatch(d, now) + 1);
    text(app, line, 160, 114, 0.5f, theme::kShell);
    if (button(app, {40, 150, 112, 36}, str::kRename, in)) {
        c.profileOpen = false;
        app.keyboard = KeyboardFor::Rename;  // opens after this frame (main.cpp)
    }
    if (button(app, {168, 150, 112, 36}, str::kProfileClose, in) || (in.down & KEY_B)) {
        c.profileOpen = false;
        audio::playSfx(audio::Sfx::Back);
    }
}

const char* hintFor(Tool t) {
    switch (t) {
        case Tool::Food: return str::kHintFood;
        case Tool::Brush: return str::kHintBrush;
        case Tool::Cloth: return str::kHintCloth;
        case Tool::Sponge: return str::kHintBath;
        case Tool::Ball: return str::kHintBall;
        default: return str::kHintPet;
    }
}

}  // namespace

bool loadSprites() {
    if (!g_sheet) g_sheet = C2D_SpriteSheetLoadFromMem(care_t3x, care_t3x_size);
    return g_sheet != nullptr;
}

void freeSprites() {
    if (g_sheet) C2D_SpriteSheetFree(g_sheet);
    g_sheet = nullptr;
}

r3d::CloseUpView view(const App& app) {
    if (const DenActor* a = careActor(const_cast<App&>(app))) {  // asleep, curled up: its face is tucked away
        if (a->behavior.activity == Activity::Sleep) return r3d::CloseUpView::Body;
    }
    switch (app.care.tool) {
        case Tool::Brush:
        case Tool::Cloth:
        case Tool::Sponge: return r3d::CloseUpView::Body;
        case Tool::Food: return r3d::CloseUpView::Feed;
        default: return r3d::CloseUpView::Face;
    }
}

void update(App& app, Dragon& d) {
    CareState& c = app.care;
    DenActor& a = actor(app);
    const DenLayout den;
    // The dragon's gaze and jaw ease back unless the tool in hand holds them this frame.
    const float k = std::fmin(1.0f, app.dt * 5.0f);
    a.gazeWeight -= a.gazeWeight * k;
    a.jawOpen -= a.jawOpen * k * 1.5f;
    c.biteWait -= app.dt;
    c.chomp -= app.dt;
    c.sweetCooldown -= app.dt;
    // The ball: physics while it's free, the mouth while it's carried.
    DenBehavior& b = a.behavior;
    b.ball = &app.ball;
    if (b.holdingBall) {
        app.ball.held = true;
        Vec3 mouth;
        if (r3d::mouthOf(0, mouth)) app.ball.pos = mouth;
    } else if (app.ball.held && !b.dropBall) {
        app.ball.held = false;  // it was interrupted (petted, called away...): drop where it is
        app.ball.release(app.ball.pos, {0, 0, 0});
    }
    if (b.dropBall) {
        b.dropBall = false;
        app.ball.release(app.ball.pos, {0, -0.9f, 0.4f});  // it rolls toward you
    }
    if (app.ball.step(den, app.dt) == BallEvent::Bounce && app.ball.lastImpact > 1.2f)
        audio::playSfx(audio::Sfx::Bounce, 0.9f + std::fmin(0.3f, app.ball.lastImpact * 0.05f));
    const bool inFlight = app.ball.active && !app.ball.resting && !app.ball.held;
    r3d::followInDen(app.ball.pos, inFlight ? 1.0f : 0.0f);
    // The tub goes away once the dragon is out of it.
    if (c.bathOut && c.tool != Tool::Sponge && b.activity != Activity::Bath) c.bathOut = false;
    r3d::setProps(&app.ball, c.bathOut, b.tubAt, b.tubSize);
    // An Ember dragon in the bath steams a little.
    if (b.activity == Activity::Bath && b.step == 2 && bathMoodOf(d) == BathMood::Grudging && app.rng.chance(1, 20))
        emit(app, kFxSteam, {frand(app, 110, 210), frand(app, 34, 62)}, 1);  // above its head
    stepFx(app);
}

void drawBottom(App& app, const Input& in, Dragon& d, s64 now) {
    CareState& c = app.care;
    DenBehavior& b = actor(app).behavior;
    // Needs along the top, the heartglow at the top right.
    gauge(app, 8, 4, str::kBelly, d.needs.belly);
    gauge(app, 76, 4, str::kEnergy, d.needs.energy);
    gauge(app, 144, 4, str::kShine, d.needs.shine);
    gauge(app, 212, 4, str::kPlay, d.needs.play);
    const float level = heartglowLevel(d, app.t);
    glow(298, 16, 14, fromRgb(heartglowColor(static_cast<Element>(d.genome.elementA))), level);
    heart(298, 16, 11, fromRgb(heartglowColor(static_cast<Element>(d.genome.elementA)), static_cast<u8>(120 + 135 * level)));

    // The heartglow opens the profile card; while it's open, the tools rest.
    if (in.released && !c.stroke.down && Rect{276, 0, 44, 36}.contains(in.rx, in.ry)) {
        c.profileOpen = !c.profileOpen;
        c.holdingFood = false;
        audio::playSfx(c.profileOpen ? audio::Sfx::Tap : audio::Sfx::Back);
        return;
    }
    if (c.profileOpen) {
        drawProfile(app, in, d, now);
        return;
    }

    // The tray.
    panel({0, kTrayY - 2, 320, 42}, withAlpha(theme::kDenPlum, 0.82f));
    const Vec2 touch{in.tx, in.ty};
    bool onUi = in.ty >= kTrayY - 2;
    for (int i = 0; i < kToolCount; ++i) {
        const Rect r{6.0f + i * 52.0f, kTrayY + 1, 48, 36};
        const bool selected = c.tool == kTools[i];
        panel(r, withAlpha(selected ? theme::kClutchGold : theme::kShell, selected ? 0.55f : 0.16f));
        sprite(toolSprite(kTools[i]), r.x + r.w * 0.5f, r.y + r.h * 0.5f, 0.52f);
        if (in.released && !c.stroke.down && r.contains(in.rx, in.ry)) selectTool(app, d, kTools[i]);
    }
    // The food picker.
    if (c.tool == Tool::Food) {
        panel({2, kFoodRowY, 316, 36}, withAlpha(theme::kDenPlum, 0.7f));
        for (int f = 0; f < static_cast<int>(Food::Count); ++f) {
            const Rect r{5.0f + f * 31.0f, kFoodRowY + 2, 30, 32};
            sprite(foodSprite(static_cast<Food>(f)), r.x + 15, r.y + 16, 0.44f);
            if (in.touching && !c.holdingFood && r.contains(in.tx, in.ty) && !c.stroke.down) {
                c.holdingFood = true;  // picked up: it follows the stylus until let go
                c.food = static_cast<Food>(f);
                c.bitesLeft = foodInfo(c.food).bites;
            }
        }
        onUi = onUi || (in.ty >= kFoodRowY && !c.holdingFood);
    }
    // The bath's rinse button.
    const Rect rinseR{250, 132, 64, 28};
    if (c.tool == Tool::Sponge && b.activity == Activity::Bath && b.step == 2) {
        panel(rinseR, withAlpha(theme::kSkyTeal, 0.75f));
        sprite(care_ladle_idx, rinseR.x + 14, rinseR.y + 14, 0.38f);
        text(app, str::kRinse, rinseR.x + 42, rinseR.y + 8, 0.42f, theme::kShell);
        if (in.released && !c.stroke.down && rinseR.contains(in.rx, in.ry)) rinse(app, d);
        onUi = onUi || rinseR.contains(in.tx, in.ty);
    }
    // Making up with an upset dragon comes first.
    if (d.upset && button(app, {100, kFoodRowY - 34, 120, 30}, str::kMakeUp, in)) {
        makeUp(d);
        feed(d, 20, true);
        markVisit(d, now);
        b.care(Care::MakeUp, d);
        audio::playSfx(audio::Sfx::Purr, 1.0f);
        showToast(app, str::kMadeUp);
    }
    if (in.down & KEY_L) b.groomSide = -1;
    if (in.down & KEY_R) b.groomSide = 1;

    // The stylus on the dragon.
    Stroke kind = Stroke::None;
    TouchHit h;
    bool hit = false;
    float moved = 0;
    if (in.touching && (!onUi || c.holdingFood) && (kView.contains(in.tx, in.ty) || c.holdingFood)) {
        if (!c.stroke.down) {
            c.stroke.begin(touch);
            c.sampleCount = 0;
            c.reported = false;
            c.stillTime = 0;
            c.onDragon = r3d::pickCloseUp(touch, h);
        }
        const Vec2 before = c.stroke.last;
        kind = c.stroke.update(touch, app.dt);
        moved = std::hypot(touch.x - before.x, touch.y - before.y);
        noteSample(c, touch, app.t);
        hit = r3d::pickCloseUp(touch, h);
        if (hit) c.lastHit = h;
        c.hadHit = hit;
        if (!d.upset) {
            switch (c.tool) {
                case Tool::Hand: useHand(app, in, d, hit, h, kind); break;
                case Tool::Food: useFood(app, in, d); break;
                case Tool::Brush:
                case Tool::Cloth: useGroomTool(app, in, d, hit, h, moved); break;
                case Tool::Sponge: useSponge(app, in, d, hit, moved); break;
                default: break;
            }
        }
    } else if (c.stroke.down && !in.touching) {  // let go
        const Stroke end = c.stroke.end();
        if (!d.upset) {
            if (c.tool == Tool::Hand && end == Stroke::Poke && c.hadHit &&
                (c.lastHit.zone == PetZone::Head || c.lastHit.zone == PetZone::Cheek))
                b.care(Care::Poke, d);
            if (c.tool == Tool::Ball) releaseBall(app, d);
        }
        c.holdingFood = false;  // a food let go goes back in the tray
        c.hadHit = false;
    }

    // The tool in hand, at the stylus.
    drawFx(app);
    Vec2 mouth;
    if (EC_DEV && app.overlay && c.tool == Tool::Food && r3d::mouthOnCloseUp(mouth)) {  // where bites happen
        C2D_DrawCircleSolid(mouth.x, mouth.y, 0.5f, 3, theme::rgba(0, 255, 120));
        C2D_DrawCircleSolid(mouth.x, mouth.y, 0.5f, 1.5f, theme::rgba(0, 0, 0));
    }
    if (in.touching && c.stroke.down) {
        switch (c.tool) {
            case Tool::Hand: sprite(c.hadHit ? care_hand_press_idx : care_hand_idx, in.tx, in.ty - 6, 0.75f); break;
            case Tool::Food:
                if (c.holdingFood) sprite(foodSprite(c.food), in.tx, in.ty, 0.6f);
                break;
            case Tool::Brush: {
                const float ang = std::atan2(c.stroke.dir.y, c.stroke.dir.x);
                sprite(care_brush_idx, in.tx, in.ty - 4, 0.7f, c.stroke.dir.x == 0 && c.stroke.dir.y == 0 ? 0.0f : ang * 0.35f);
                break;
            }
            case Tool::Cloth: sprite(care_cloth_idx, in.tx, in.ty - 4, 0.62f); break;
            case Tool::Sponge: sprite(care_sponge_idx, in.tx, in.ty - 4, 0.62f); break;
            case Tool::Ball:
                if (!app.ball.held) sprite(care_ball_idx, in.tx, in.ty, 0.5f);
                break;
            default: break;
        }
    } else {
        const char* hint = hintFor(c.tool);
        const float y = c.tool == Tool::Food ? kFoodRowY - 17 : kTrayY - 17;
        const float w = textWidth(app, hint, 0.4f) + 16;
        panel({160 - w / 2, y, w, 15}, withAlpha(theme::kDenPlum, 0.6f));
        textCentered(app, hint, 160, y + 7.5f, 0.4f, withAlpha(theme::kShell, 0.9f), 300);
    }
}

}  // namespace ec::care
