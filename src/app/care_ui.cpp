#include "app/care_ui.hpp"

#include <citro2d.h>

#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/egg.hpp"
#include "core/genetics.hpp"
#include "core/market.hpp"
#include "core/profile.hpp"
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

constexpr Tool kTools[] = {Tool::Hand, Tool::Food, Tool::Brush, Tool::Cloth, Tool::Sponge, Tool::Ball};  // the last: a toy
constexpr int kToolCount = sizeof(kTools) / sizeof(kTools[0]);
constexpr Rect kBowlDrop{262, kFoodRowY - 38, 54, 34};  // drop a food here to fill the bowl
constexpr float kOrbRadiusPx = 17;
constexpr float kOrbTreatAfter = 1500;  // pixels of rolling before a treat drops out
constexpr float kOrbRefill = 40;        // seconds until the orb has another

bool isToy(Tool t) { return t >= Tool::Ball; }
Item toyItem(Tool t) {  // the Market thing a toy tool is (the ball comes with the den)
    return t == Tool::Feather ? Item::FeatherWand : t == Tool::Rope ? Item::TugRope : Item::PuzzleOrb;
}

std::size_t toolSprite(Tool t) {
    switch (t) {
        case Tool::Food: return care_food_skyberry_idx;
        case Tool::Brush: return care_brush_idx;
        case Tool::Cloth: return care_cloth_idx;
        case Tool::Sponge: return care_sponge_idx;
        case Tool::Ball: return care_ball_idx;
        case Tool::Feather: return care_item_featherwand_idx;
        case Tool::Rope: return care_item_tugrope_idx;
        case Tool::Orb: return care_item_puzzleorb_idx;
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
// The dragon's gaze follows the stylus smoothly (it twitched with every pixel of it); a gaze
// that had faded away starts where it's aimed.
void gazeAt(App& app, Vec3 target, float weight) {
    DenActor& a = actor(app);
    const float k = a.gazeWeight > 0.05f ? std::fmin(1.0f, app.dt * 8.0f) : 1.0f;
    a.gazeLocal = a.gazeLocal + (target - a.gazeLocal) * k;
    a.gazeWeight = weight;
}

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
            audio::playSfx(audio::Sfx::Grumble);
        }
        return;
    }
    if (kind == Stroke::None) return;
    gazeAt(app, h.local, 0.55f);  // lean toward the hand
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
            audio::playSfx(audio::Sfx::Giggle);
            emit(app, kFxHeart, {in.tx, in.ty - 10}, 4);
            if (!c.sweetFound) {
                c.sweetFound = true;
                addBond(d, 3);
                if (!(d.known & kKnownSweetSpot)) showToastf(app, str::kFoundSweetSpot, sweetSpotText(d));
                else showToast(app, str::kSweetSpot);
                d.known |= kKnownSweetSpot;  // (the profile shows it from now on)
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
    gazeAt(app, r3d::closeUpLocal({in.tx, in.ty}), 0.8f);
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
        audio::playSfx(audio::Sfx::Grumble);
        return;
    }
    --c.bitesLeft;
    audio::playSfx(audio::Sfx::Munch);
    emit(app, kFxCrumb, mouth, 5);
    const bool favourite = taste == Taste::Favorite;
    if (favourite && static_cast<int>(c.food) == d.favoriteFood) d.known |= kKnownFavourite;
    if (c.bitesLeft <= 0) {
        c.holdingFood = false;
        feed(d, info.belly, favourite);
        useFood(app.game, c.food);  // eaten: one less in the pouch
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
        if (c.groom.brush(d, h.region, moved / 520.0f * brushRate(app.game), withGrain)) {  // the silver brush: faster
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

// The sponge: suds while it sits in the tub. Rinsed, it hops out and shakes off; the tub stays
// out, and the sponge on it again brings it back in for another wash (it did nothing until
// the tool was picked again: Noah, run 3).
void useSponge(App& app, const Input& in, Dragon& d, bool hit, float moved) {
    CareState& c = app.care;
    DenBehavior& b = actor(app).behavior;
    if (!hit || moved < 0.5f) return;
    if (b.activity != Activity::Bath) {
        if (c.bathOut && !d.upset) {
            c.suds = 0;
            b.care(Care::Bath, d);
        }
        return;
    }
    if (b.step != 2) return;
    c.suds = std::fmin(1.0f, c.suds + moved / 900.0f);
    if (app.rng.chance(1, 2)) emit(app, kFxSuds, {in.tx, in.ty}, 1);
    if ((c.soundWait -= app.dt) <= 0) {
        c.soundWait = 0.5f;
        audio::playSfx(audio::Sfx::Suds);
    }
    (void)d;
}

void rinse(App& app, Dragon& d) {
    CareState& c = app.care;
    audio::playSfx(audio::Sfx::WaterPour);
    for (int i = 0; i < 10; ++i) emit(app, kFxDrop, {frand(app, 80, 240), frand(app, 40, 90)}, 1);
    if (c.suds > 0.3f) {
        bathe(d);
        if (bathShine(app.game) > 0) {  // bubble soap: a lasting sparkle
            groom(d, bathShine(app.game));
            for (int i = 0; i < 3; ++i) emit(app, kFxSparkle, {frand(app, 100, 220), frand(app, 50, 120)}, 4);
        }
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
        audio::playSfx(audio::Sfx::BallRoll);
    } else {
        return;
    }
    actor(app).behavior.ball = &app.ball;
    actor(app).behavior.care(Care::Throw, d);
    play(d, 6);
    markVisit(d, nowLocal(app));
}

// The feather wand: the dragon watches it; flicked near its face, a swat; let go there, a pounce.
void useFeather(App& app, const Input& in, Dragon& d) {
    CareState& c = app.care;
    DenActor& a = actor(app);
    a.behavior.care(Care::Dangle, d);
    a.gazeLocal = r3d::closeUpLocal({in.tx, in.ty});
    a.gazeWeight = 0.9f;
    Vec2 mouth;
    c.featherNear = r3d::mouthOnCloseUp(mouth) && std::hypot(in.tx - mouth.x, in.ty - mouth.y) < 80;
    if (c.featherNear && c.stroke.speed > 140 && c.swatWait <= 0) {
        c.swatWait = 0.9f;
        a.behavior.care(Care::Swat, d);
        audio::playSfx(audio::Sfx::FeatherFlutter);
        play(d, 3);
        markVisit(d, nowLocal(app));
        if (app.rng.chance(1, 2)) emit(app, kFxHeart, {in.tx, in.ty - 10}, 1);
    }
}

// The tug rope: it bites on and tugs while you hold it; pulling hard, a playful growl.
void useRope(App& app, const Input& in, Dragon& d, float moved) {
    CareState& c = app.care;
    DenBehavior& b = actor(app).behavior;
    b.care(Care::TugPull, d);
    if (b.activity != Activity::Tug) return;
    if (moved > 3 && c.tugWait <= 0) {
        c.tugWait = 0.7f;
        audio::playSfx(audio::Sfx::RopeTug);
        play(d, 2);
        markVisit(d, nowLocal(app));
        emit(app, kFxDust, {in.tx, in.ty}, 1);
    }
}

// The puzzle orb: pushed about the close-up; after enough rolling a treat drops out and the
// dragon gobbles it.
void stepOrb(App& app, Dragon& d) {
    CareState& c = app.care;
    DenActor& a = actor(app);
    c.orbWait -= app.dt;
    if (!c.orbHeld) {  // rolling on: friction, and the edges of the view bounce it back
        c.orbAt.x += c.orbVel.x * app.dt;
        c.orbAt.y += c.orbVel.y * app.dt;
        const float k = std::fmax(0.0f, 1.0f - 2.2f * app.dt);
        c.orbVel = {c.orbVel.x * k, c.orbVel.y * k};
        const float lo = kView.x + kOrbRadiusPx, hi = kView.x + kView.w - kOrbRadiusPx;
        const float top = kView.y + kOrbRadiusPx, bottom = kView.y + kView.h - kOrbRadiusPx;
        if (c.orbAt.x < lo || c.orbAt.x > hi) c.orbVel.x = -c.orbVel.x * 0.7f;
        if (c.orbAt.y < top || c.orbAt.y > bottom) c.orbVel.y = -c.orbVel.y * 0.7f;
        c.orbAt = {std::fmax(lo, std::fmin(hi, c.orbAt.x)), std::fmax(top, std::fmin(bottom, c.orbAt.y))};
    }
    const float speed = std::hypot(c.orbVel.x, c.orbVel.y);
    c.orbSpin += c.orbVel.x * app.dt / kOrbRadiusPx;
    if (speed > 20) {
        a.gazeLocal = r3d::closeUpLocal(c.orbAt);  // eyes on the orb
        a.gazeWeight = 0.7f;
        if (c.orbWait <= 0) c.orbRolled += speed * app.dt;
    }
    if (c.orbRolled > kOrbTreatAfter && c.treatT < 0) {  // out it drops, and off to the mouth
        c.orbRolled = 0;
        c.orbWait = kOrbRefill;
        c.treatFrom = c.orbAt;
        c.treatT = 0;
        audio::playSfx(audio::Sfx::TreatDrop);
        showToast(app, str::kTreat);
    }
    if (c.treatT >= 0 && (c.treatT += app.dt / 0.7f) >= 1) {
        c.treatT = -1;
        feed(d, 6, false);
        play(d, 8);
        addBond(d, 1);
        markVisit(d, nowLocal(app));
        a.behavior.care(Care::Feed, d);
        audio::playSfx(audio::Sfx::Munch);
        Vec2 mouth;
        if (r3d::mouthOnCloseUp(mouth)) emit(app, kFxCrumb, mouth, 5);
    }
}

void useOrb(App& app, const Input& in, float moved) {
    CareState& c = app.care;
    const Vec2 touch{in.tx, in.ty};
    if (!c.orbHeld && std::hypot(touch.x - c.orbAt.x, touch.y - c.orbAt.y) < kOrbRadiusPx + 14) {
        c.orbHeld = true;  // picked up: it goes where the stylus goes
        if (c.orbWait > 0 && c.treatT < 0) showToast(app, str::kOrbEmpty);
    }
    if (!c.orbHeld) return;
    const Vec2 before = c.orbAt;
    c.orbAt = touch;
    if (app.dt > 0) c.orbVel = {(c.orbAt.x - before.x) / app.dt, (c.orbAt.y - before.y) / app.dt};
    (void)moved;
}

void selectTool(App& app, Dragon& d, Tool t) {
    CareState& c = app.care;
    if (isToy(t)) c.toy = t;
    if (c.tool == t) return;
    if (c.tool == Tool::Rope) actor(app).behavior.care(Care::TugLetGo, d);  // the rope goes with it
    if (c.tool == Tool::Sponge && c.bathOut) actor(app).behavior.care(Care::BathDone, d);  // leaving the bath
    c.tool = t;
    c.holdingFood = false;
    if (t == Tool::Sponge && !d.upset) {
        audio::playSfx(audio::Sfx::TubSlide);
        c.bathOut = true;
        c.suds = 0;
        actor(app).behavior.care(Care::Bath, d);
    }
    audio::playSfx(audio::Sfx::Tap);
}

u32 glowOf(const Dragon& d, u8 alpha = 255) {
    return fromRgb(heartglowColor(static_cast<Element>(d.genome.elementA)), alpha);
}

// One of the family: a heart in its colour, its name and breed (or "Unknown").
void kinBox(App& app, Rect r, const SaveData& s, int who, const char* role) {
    panel(r, withAlpha(theme::kShell, who >= 0 ? 0.16f : 0.07f));
    if (who < 0) {
        textCentered(app, role ? role : str::kUnknownKin, r.x + r.w / 2, r.y + r.h / 2, 0.36f, withAlpha(theme::kShell, 0.45f),
                     r.w - 6);
        return;
    }
    const Dragon& k = s.dragons[who];
    heart(r.x + 11, r.y + r.h / 2, 7, glowOf(k));
    text(app, k.name, r.x + 20, r.y + 3, 0.38f, theme::kShell, C2D_AlignLeft, r.w - 23);
    text(app, breedName(k.genome), r.x + 20, r.y + r.h / 2 + 1, 0.32f, withAlpha(theme::kShell, 0.7f), C2D_AlignLeft, r.w - 23);
}

void profileAbout(App& app, const Dragon& d, s64 now) {
    char line[96];
    if (d.stage == Stage::Egg) {  // what's inside is a surprise until it hatches
        std::snprintf(line, sizeof(line), "%s %s  -  %d%% %s", breedName(d.genome), str::kEggSuffix,
                      static_cast<int>(eggProgress(d) * 100), str::kIncubated);
        textCentered(app, line, 160, 110, 0.5f, theme::kShell, 300);
        textCentered(app, originText(d), 160, 134, 0.42f, withAlpha(theme::kShell, 0.75f), 300);
        return;
    }
    std::snprintf(line, sizeof(line), "%s %s %s  -  %s %d", sexName(d.sex), breedName(d.genome), stageName(d.stage),
                  str::kDay, daysSinceHatch(d, now) + 1);
    textCentered(app, line, 160, 76, 0.46f, theme::kShell, 304);
    std::snprintf(line, sizeof(line), "%s  -  %s", personalityName(d.personality), str::kBond);
    const float w = textWidth(app, line, 0.44f);
    text(app, line, 160 - (w + 70) / 2, 86, 0.44f, withAlpha(theme::kShell, 0.85f), C2D_AlignLeft);
    for (int h = 0; h < 5; ++h)  // bond, a heart per 200
        heart(160 - (w + 70) / 2 + w + 10 + h * 13, 94, 5.5f, d.bond >= (h + 1) * 200 ? glowOf(d) : withAlpha(theme::kShell, 0.25f));
    // Stats (left) and looks (right).
    const Stats st = statsOf(d);
    const char* const names[3] = {str::kStatWing, str::kStatWit, str::kStatSpark};
    const int values[3] = {st.wing, st.wit, st.spark};
    for (int k = 0; k < 3; ++k) {
        const float y = 110 + k * 16;
        text(app, names[k], 14, y, 0.4f, theme::kShell, C2D_AlignLeft);
        C2D_DrawRectSolid(56, y + 4, 0.5f, 90, 7, withAlpha(theme::kShell, 0.15f));
        C2D_DrawRectSolid(56, y + 4, 0.5f, 90 * values[k] / 100.0f, 7, theme::kClutchGold);
    }
    const Genome& g = d.genome;
    std::snprintf(line, sizeof(line), "%s build, %s horns", buildName(g.build), hornsName(g.horns));
    text(app, line, 160, 110, 0.36f, theme::kShell, C2D_AlignLeft, 152);
    std::snprintf(line, sizeof(line), "%s frill, %s wings", frillName(g.frill), wingsName(g.wings));
    text(app, line, 160, 126, 0.36f, theme::kShell, C2D_AlignLeft, 152);
    const char* rare = rareName(g.rareFlags);
    std::snprintf(line, sizeof(line), "%s tail, %s%s%s", tailName(g.tailTip), patternName(g.pattern), rare ? ", " : "",
                  rare ? rare : "");
    text(app, line, 160, 142, 0.36f, theme::kShell, C2D_AlignLeft, 152);
    // What you've found out.
    if (d.known & kKnownSweetSpot) std::snprintf(line, sizeof(line), str::kSweetSpotIs, sweetSpotText(d));
    else std::snprintf(line, sizeof(line), "%s", str::kSweetSpotUnknown);
    textCentered(app, line, 160, 168, 0.4f, theme::kClutchGold, 300);
    if ((d.known & kKnownFavourite) && d.favoriteFood < static_cast<int>(Food::Count))
        std::snprintf(line, sizeof(line), str::kFavouriteIs, foodInfo(static_cast<Food>(d.favoriteFood)).name);
    else std::snprintf(line, sizeof(line), "%s", str::kFavouriteUnknown);
    textCentered(app, line, 160, 184, 0.4f, theme::kClutchGold, 300);
}

void profileFamily(App& app, const Dragon& d) {
    const SaveData& s = app.game;
    const Family f = familyOf(s, d);
    const u32 line = withAlpha(theme::kShell, 0.35f);
    const bool parents = f.mother >= 0 || f.father >= 0;
    if (parents) {
        const float gx[4] = {6, 84, 162, 240};  // grandparents, then parents under their pairs
        for (int k = 0; k < 4; ++k) {
            const float px = k < 2 ? 82 : 238;
            C2D_DrawLine(gx[k] + 37, 104, line, px, 118, line, 1.5f, 0.5f);
            kinBox(app, {gx[k], 70, 74, 34}, s, f.grand[k], nullptr);
        }
        C2D_DrawLine(82, 152, line, 160, 164, line, 1.5f, 0.5f);
        C2D_DrawLine(238, 152, line, 160, 164, line, 1.5f, 0.5f);
        kinBox(app, {27, 118, 110, 34}, s, f.mother, str::kMother);
        kinBox(app, {183, 118, 110, 34}, s, f.father, str::kFather);
    } else {
        panel({30, 92, 260, 44}, withAlpha(theme::kShell, 0.1f));
        textCentered(app, originText(d), 160, 114, 0.46f, theme::kShell, 250);
        C2D_DrawLine(160, 136, line, 160, 164, line, 1.5f, 0.5f);
    }
    const Rect me{95, 164, 130, 32};
    panel(me, withAlpha(theme::kClutchGold, 0.3f));
    heart(me.x + 12, me.y + me.h / 2, 7.5f, glowOf(d));
    text(app, d.name, me.x + 22, me.y + 2, 0.42f, theme::kShell, C2D_AlignLeft, me.w - 26);
    char young[32];
    if (f.young > 0) std::snprintf(young, sizeof(young), str::kYoungCount, f.young);
    else std::snprintf(young, sizeof(young), "%s", breedName(d.genome));
    text(app, young, me.x + 22, me.y + 17, 0.34f, withAlpha(theme::kShell, 0.75f), C2D_AlignLeft, me.w - 26);
}

// The profile (tap the heartglow; WP8): about it (its looks, stats, what you've found out)
// and its family; renaming (D27) and sending it to the Sanctuary. The top screen shows it posing.
void drawProfile(App& app, const Input& in, Dragon& d, s64 now) {
    CareState& c = app.care;
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 240, withAlpha(theme::kDenPlum, 0.97f));
    glow(298, 16, 14, glowOf(d), heartglowLevel(d, app.t));
    heart(298, 16, 11, glowOf(d));
    textCentered(app, d.name, 150, 19, 0.8f, theme::kClutchGold, 180, Face::Title);
    if (denRoster(app.game).presentCount() > 1) {  // the others in the den
        if (button(app, {6, 4, 34, 28}, "<", in)) cycleCare(app, -1);
        if (button(app, {240, 4, 34, 28}, ">", in)) cycleCare(app, 1);
    }
    drawProfilePages(app, in, d, now, c.profileTab);
    if (button(app, {6, 202, 100, 34}, str::kRename, in)) {
        c.profileOpen = false;
        app.keyboard = KeyboardFor::Rename;  // opens after this frame (main.cpp)
    }
    if (button(app, {110, 202, 100, 34}, str::kToSanctuary, in)) {  // off to the keepers
        const DenRoster r = denRoster(app.game);
        if (r.presentCount() + r.eggCount <= 1) {
            showToast(app, str::kStayHome);
        } else if (storeAway(app.game, app.careIndex)) {
            showToastf(app, str::kSentAway, d.name);
            c.profileOpen = false;
            fixCare(app);
            saveNow(app);
            return;
        }
    }
    if (button(app, {214, 202, 100, 34}, str::kProfileClose, in) || (in.down & KEY_B)) {
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
        case Tool::Feather: return str::kHintFeather;
        case Tool::Rope: return str::kHintRope;
        case Tool::Orb: return str::kHintOrb;
        default: return str::kHintPet;
    }
}

}  // namespace

void drawFood(Food f, float x, float y, float scale) { sprite(foodSprite(f), x, y, scale); }

void drawProfilePages(App& app, const Input& in, const Dragon& d, s64 now, u8& tab) {
    static const char* const kTabs[2] = {str::kTabAbout, str::kTabFamily};
    for (int k = 0; k < 2; ++k) {
        const Rect r{40.0f + k * 122.0f, 38, 118, 24};
        const bool on = tab == k;
        panel(r, on ? theme::kClutchGold : withAlpha(theme::kShell, 0.2f));
        textCentered(app, kTabs[k], r.x + r.w / 2, r.y + r.h / 2, 0.44f, on ? theme::kDenPlum : theme::kShell, r.w - 6);
        if (in.released && r.contains(in.rx, in.ry) && !on) {
            tab = static_cast<u8>(k);
            audio::playSfx(audio::Sfx::Tap);
        }
    }
    if (in.down & (KEY_L | KEY_R)) tab ^= 1;
    if (tab == 0) profileAbout(app, d, now);
    else profileFamily(app, d);
}

void drawItem(Item i, float x, float y, float scale) {
    if (i < Item::Count) sprite(care_item_featherwand_idx + static_cast<std::size_t>(i), x, y, scale);
}

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
    c.swatWait -= app.dt;
    c.tugWait -= app.dt;
    stepOrb(app, d);
    // The ball: physics while it's free, the mouth while it's carried.
    DenBehavior& b = a.behavior;
    b.ball = &app.ball;
    if (b.holdingBall) {
        if (!app.ball.held) audio::playSfx(audio::Sfx::BallPickup);  // caught it
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
    int toys = 1;  // the ball, and the Market's toys bought
    for (Tool t : {Tool::Feather, Tool::Rope, Tool::Orb}) toys += owns(app.game, toyItem(t));
    for (int i = 0; i < kToolCount; ++i) {
        const Rect r{6.0f + i * 52.0f, kTrayY + 1, 48, 36};
        const Tool t = i == kToolCount - 1 ? c.toy : kTools[i];
        const bool selected = c.tool == t;
        panel(r, withAlpha(selected ? theme::kClutchGold : theme::kShell, selected ? 0.55f : 0.16f));
        sprite(toolSprite(t), r.x + r.w * 0.5f, r.y + r.h * 0.5f, isToy(t) && t != Tool::Ball ? 0.46f : 0.52f);
        if (i == kToolCount - 1 && toys > 1)  // more toys: a little mark, and the picker on a second tap
            C2D_DrawTriangle(r.x + r.w - 10, r.y + 8, theme::kShell, r.x + r.w - 4, r.y + 8, theme::kShell, r.x + r.w - 7,
                             r.y + 3, theme::kShell, 0.5f);
        if (in.released && !c.stroke.down && r.contains(in.rx, in.ry)) {
            if (selected && isToy(t)) c.toyRow = !c.toyRow;
            else selectTool(app, d, t);
        }
    }
    if (!isToy(c.tool)) c.toyRow = false;
    // The toy picker.
    if (c.toyRow) {
        panel({2, kFoodRowY, 316, 36}, withAlpha(theme::kDenPlum, 0.7f));
        int k = 0;
        for (Tool t : {Tool::Ball, Tool::Feather, Tool::Rope, Tool::Orb}) {
            if (t != Tool::Ball && !owns(app.game, toyItem(t))) continue;
            const Rect r{5.0f + k++ * 42.0f, kFoodRowY + 2, 38, 32};
            if (c.tool == t) panel(r, withAlpha(theme::kClutchGold, 0.45f));
            sprite(toolSprite(t), r.x + r.w / 2, r.y + r.h / 2, 0.42f);
            if (in.released && !c.stroke.down && r.contains(in.rx, in.ry)) {
                selectTool(app, d, t);
                c.toyRow = false;
            }
        }
        if (k == 1) text(app, str::kMoreToys, 56, kFoodRowY + 10, 0.4f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft);
    }
    // The food picker.
    if (c.tool == Tool::Food) {
        panel({2, kFoodRowY, 316, 36}, withAlpha(theme::kDenPlum, 0.7f));
        for (int f = 0; f < static_cast<int>(Food::Count); ++f) {
            const Rect r{5.0f + f * 31.0f, kFoodRowY + 2, 30, 32};
            const int have = pouchCount(app.game, static_cast<Food>(f));  // the pouch (WP5)
            sprite(foodSprite(static_cast<Food>(f)), r.x + 15, r.y + 16, 0.44f);
            if (have == 0) C2D_DrawRectSolid(r.x, r.y, 0.5f, r.w, r.h, withAlpha(theme::kDenPlum, 0.6f));
            char count[8];
            std::snprintf(count, sizeof(count), "%d", have);
            text(app, count, r.x + r.w - 2, r.y + r.h - 12, 0.34f, have ? theme::kShell : withAlpha(theme::kShell, 0.5f),
                 C2D_AlignRight);
            if (in.touching && !c.holdingFood && r.contains(in.tx, in.ty) && !c.stroke.down) {
                if (have == 0) {
                    if (in.tapped) showToast(app, str::kNoneLeft);
                    continue;
                }
                c.holdingFood = true;  // picked up: it follows the stylus until let go
                c.food = static_cast<Food>(f);
                c.bitesLeft = foodInfo(c.food).bites;
            }
        }
        onUi = onUi || (in.ty >= kFoodRowY && !c.holdingFood);
        if (owns(app.game, Item::FoodBowl)) {  // drop a food here to fill the bowl (WP7)
            panel(kBowlDrop, withAlpha(c.holdingFood && kBowlDrop.contains(in.tx, in.ty) ? theme::kClutchGold : theme::kDenPlum,
                                       0.75f));
            drawItem(Item::FoodBowl, kBowlDrop.x + 17, kBowlDrop.y + kBowlDrop.h / 2, 0.42f);
            char left[8];
            std::snprintf(left, sizeof(left), "%d/%d", app.game.bowlLeft, kBowlPortions);
            text(app, left, kBowlDrop.x + kBowlDrop.w - 4, kBowlDrop.y + 10, 0.38f, theme::kShell, C2D_AlignRight);
            onUi = onUi || (kBowlDrop.contains(in.tx, in.ty) && !c.holdingFood);
        }
    }
    if (c.toyRow) onUi = onUi || in.ty >= kFoodRowY;
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
    // L / R with a brush or cloth in hand: it turns the other flank to you, whether or not it's
    // being groomed right now (it only listened mid-stroke before, so it seemed to do nothing).
    // care() ignores a sleeping or upset one.
    if ((in.down & (KEY_L | KEY_R)) && (c.tool == Tool::Brush || c.tool == Tool::Cloth)) {
        b.groomSide = (in.down & KEY_L) ? -1 : 1;
        b.care(Care::GroomBody, d);
    }

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
                case Tool::Feather: useFeather(app, in, d); break;
                case Tool::Rope: useRope(app, in, d, moved); break;
                case Tool::Orb: useOrb(app, in, moved); break;
                default: break;
            }
        }
    } else if (c.stroke.down && !in.touching) {  // let go
        const Vec2 lastTouch = c.stroke.last;
        const Stroke end = c.stroke.end();
        if (c.tool == Tool::Food && c.holdingFood && owns(app.game, Item::FoodBowl) && kBowlDrop.contains(lastTouch.x, lastTouch.y)) {
            const Food f = bowlFood(app.game);
            if (fillBowl(app.game, c.food)) {
                audio::playSfx(audio::Sfx::BowlClink);
                showToast(app, str::kIntoBowl);
            } else {
                audio::playSfx(audio::Sfx::Error);
                showToast(app, app.game.bowlLeft >= kBowlPortions ? str::kBowlFull
                               : f != Food::Count && f != c.food ? str::kBowlOneFood
                                                                 : str::kNoneLeft);
            }
        }
        if (c.orbHeld && std::hypot(c.orbVel.x, c.orbVel.y) > 120) audio::playSfx(audio::Sfx::OrbRattle);  // off it rolls
        c.orbHeld = false;
        if (!d.upset && c.tool == Tool::Rope) {  // let go: off it trots, proud
            b.care(Care::TugLetGo, d);
            play(d, 6);
            emit(app, kFxHeart, {in.rx, in.ry - 10}, 2);
        }
        if (!d.upset && c.tool == Tool::Feather && c.featherNear && b.activity == Activity::Bat) {  // pounce!
            b.care(Care::Play, d);
            play(d, 6);
            emit(app, kFxHeart, {in.rx, in.ry - 10}, 3);
        }
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
            case Tool::Feather:  // the wand swings with the stroke
                sprite(care_item_featherwand_idx, in.tx - 14, in.ty + 12, 0.85f,
                       std::fmax(-0.6f, std::fmin(0.6f, c.stroke.dir.x * std::fmin(1.0f, c.stroke.speed / 300.0f) * 0.6f)));
                break;
            case Tool::Rope: {  // from your hand to its teeth, sagging a little
                Vec2 mouth;
                if (b.activity == Activity::Tug && r3d::mouthOnCloseUp(mouth)) {
                    const u32 ropeC = theme::rgba(238, 222, 186), shade = theme::rgba(170, 140, 110);
                    Vec2 prev{in.tx, in.ty};
                    for (int s = 1; s <= 8; ++s) {
                        const float t = s / 8.0f;
                        const Vec2 p{in.tx + (mouth.x - in.tx) * t, in.ty + (mouth.y - in.ty) * t + 14 * std::sin(t * 3.1416f)};
                        C2D_DrawLine(prev.x, prev.y + 1.5f, shade, p.x, p.y + 1.5f, shade, 5, 0.5f);
                        C2D_DrawLine(prev.x, prev.y, ropeC, p.x, p.y, ropeC, 4, 0.5f);
                        prev = p;
                    }
                }
                C2D_DrawCircleSolid(in.tx, in.ty, 0.5f, 7, theme::rgba(214, 86, 70));  // the knot in your hand
                break;
            }
            default: break;
        }
    }
    if (c.tool == Tool::Orb) {  // the orb stays on the close-up, wherever it rolled
        sprite(care_item_puzzleorb_idx, c.orbAt.x, c.orbAt.y, c.orbWait > 0 ? 0.5f : 0.55f, c.orbSpin);
        if (c.orbWait > 0) C2D_DrawCircleSolid(c.orbAt.x, c.orbAt.y, 0.5f, 6, withAlpha(theme::kDenPlum, 0.5f));
    }
    if (c.treatT >= 0) {  // the treat, on its way into the mouth
        Vec2 mouth;
        if (!r3d::mouthOnCloseUp(mouth)) mouth = {160, 80};
        const float t = c.treatT;
        sprite(care_food_glimmercookie_idx, c.treatFrom.x + (mouth.x - c.treatFrom.x) * t,
               c.treatFrom.y + (mouth.y - c.treatFrom.y) * t - 40 * std::sin(t * 3.1416f), 0.4f, t * 6);
    }
    if (!(in.touching && c.stroke.down)) {
        const char* hint = hintFor(c.tool);
        const float y = c.tool == Tool::Food || c.toyRow ? kFoodRowY - 17 : kTrayY - 17;
        const float w = textWidth(app, hint, 0.4f) + 16;
        panel({160 - w / 2, y, w, 15}, withAlpha(theme::kDenPlum, 0.6f));
        textCentered(app, hint, 160, y + 7.5f, 0.4f, withAlpha(theme::kShell, 0.9f), 300);
    }
}

}  // namespace ec::care
