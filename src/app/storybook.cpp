#include "app/storybook.hpp"

#include <cmath>
#include <cstring>

#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/daylight.hpp"

namespace ec::paint {
namespace {

namespace pal = theme::paint;

u32 rgb(Rgb c, float a = 1.0f) { return withAlpha(theme::rgba(c.r, c.g, c.b), a); }

Rgb mix(Rgb a, Rgb b, float t) {
    t = t < 0 ? 0 : (t > 1 ? 1 : t);
    return {static_cast<u8>(a.r + (b.r - a.r) * t), static_cast<u8>(a.g + (b.g - a.g) * t),
            static_cast<u8>(a.b + (b.b - a.b) * t)};
}

// Where the stars are (x, y in the upper sky), fixed so they don't wander between frames.
constexpr float kStars[][2] = {{24, 20}, {58, 64}, {92, 12}, {130, 44}, {166, 22}, {204, 58}, {238, 16}, {262, 42},
                               {300, 70}, {318, 28}, {352, 52}, {380, 18}, {40, 96}, {150, 84}, {226, 92},
                               {286, 104}, {110, 100}, {344, 92}, {196, 34}, {12, 60}};

}  // namespace

Light lightFor(s64 now) {
    Light l;
    valleySky(now, l.top, l.horizon, l.tint);
    const DayBlend b = dayBlend(now);
    l.day = b.weight(kLightDay);
    l.evening = b.weight(kLightEvening);
    l.night = b.weight(kLightNight);
    return l;
}

u32 lit(const Light& l, Rgb c, float a) {
    return withAlpha(theme::rgba(static_cast<u8>(c.r * l.tint.r / 255), static_cast<u8>(c.g * l.tint.g / 255),
                                 static_cast<u8>(c.b * l.tint.b / 255)),
                     a);
}

void sky(const Light& l, float t, float horizonY) {
    verticalGradient(0, 0, kTopW, horizonY, rgb(l.top), rgb(l.horizon));
    C2D_DrawRectSolid(0, horizonY, 0, kTopW, kScreenH - horizonY, rgb(l.horizon));
    const float far = r3d::eyeShift(4.0f);
    // The sun: high by day, low and warm at evening.
    const float sunA = l.day + l.evening * 0.9f;
    if (sunA > 0.05f) {
        const float y = 40 + l.evening * 52;
        const Rgb c = mix(pal::kSun, pal::kSunset, l.evening / (sunA + 1e-3f));
        glow(328 + far, y, 30, rgb(c), 0.9f * sunA);
        C2D_DrawCircleSolid(328 + far, y, 0, 13, rgb(c, sunA));
    }
    // The moon and the stars, twinkling.
    if (l.night > 0.05f) {
        glow(70 + far, 40, 22, rgb(pal::kMoon), 0.6f * l.night);
        C2D_DrawCircleSolid(70 + far, 40, 0, 11, rgb(pal::kMoon, l.night));
        C2D_DrawCircleSolid(76 + far, 36, 0, 9.5f, rgb(l.top, l.night));  // the crescent's shadow
        for (int i = 0; i < 20; ++i) {
            const float tw = 0.55f + 0.45f * std::sin(t * 1.7f + i * 2.3f);
            C2D_DrawCircleSolid(kStars[i][0] + far, kStars[i][1], 0, 0.8f + (i % 3) * 0.35f,
                                rgb(pal::kMoon, l.night * tw));
        }
    }
}

void clouds(const Light& l, float t, float y, float depth) {
    const float shift = r3d::eyeShift(depth);
    const u32 c = lit(l, pal::kCloud, 0.85f - 0.45f * l.night);
    for (int i = 0; i < 4; ++i) {
        const float s = 0.7f + 0.15f * (i % 3);
        const float x = std::fmod(40.0f + i * 131.0f + t * (3.0f + i), 520.0f) - 60 + shift;
        const float cy = y + (i % 2) * 18;
        C2D_DrawEllipseSolid(x - 30 * s, cy - 6 * s, 0, 60 * s, 14 * s, c);
        C2D_DrawCircleSolid(x - 12 * s, cy - 6 * s, 0, 11 * s, c);
        C2D_DrawCircleSolid(x + 6 * s, cy - 11 * s, 0, 14 * s, c);
        C2D_DrawCircleSolid(x + 20 * s, cy - 4 * s, 0, 9 * s, c);
    }
}

void mountains(const Light& l, float baseY, float depth) {
    const float shift = r3d::eyeShift(depth);
    static constexpr float kPeaks[][3] = {{-10, 64, 60}, {70, 88, 76}, {160, 58, 70}, {236, 96, 84}, {330, 70, 68},
                                          {410, 84, 70}};
    for (int i = 0; i < 6; ++i) {
        const float x = kPeaks[i][0] + shift, h = kPeaks[i][1], w = kPeaks[i][2];
        const Rgb c = mix(pal::kMountain, l.horizon, 0.25f + 0.1f * (i % 2));
        C2D_DrawTriangle(x - w, baseY, lit(l, c), x + w, baseY, lit(l, c), x, baseY - h, lit(l, mix(c, pal::kSnow, 0.15f)), 0);
        const float k = 0.28f;  // the snow: the top of the peak
        C2D_DrawTriangle(x - w * k, baseY - h * (1 - k), lit(l, pal::kSnow), x + w * k, baseY - h * (1 - k),
                         lit(l, pal::kSnow), x, baseY - h, lit(l, pal::kSnow), 0);
    }
}

void hill(const Light& l, float cx, float topY, float w, float h, Rgb c, float depth) {
    const float shift = r3d::eyeShift(depth);
    C2D_DrawEllipseSolid(cx - w / 2 + shift, topY, 0, w, h * 2, lit(l, c));
}

void tree(const Light& l, float x, float groundY, float s, int shade, float depth) {
    x += r3d::eyeShift(depth);
    const Rgb leaf = mix(pal::kLeaf, pal::kLeafLight, 0.25f * shade);
    C2D_DrawRectSolid(x - s * 0.07f, groundY - s * 0.42f, 0, s * 0.14f, s * 0.42f, lit(l, pal::kTrunk));
    C2D_DrawEllipseSolid(x - s * 0.3f, groundY - s * 0.06f, 0, s * 0.6f, s * 0.1f, lit(l, pal::kGrass, 0.5f));  // its shadow
    C2D_DrawCircleSolid(x - s * 0.2f, groundY - s * 0.5f, 0, s * 0.26f, lit(l, leaf));
    C2D_DrawCircleSolid(x + s * 0.2f, groundY - s * 0.52f, 0, s * 0.24f, lit(l, leaf));
    C2D_DrawCircleSolid(x, groundY - s * 0.72f, 0, s * 0.3f, lit(l, leaf));
    C2D_DrawCircleSolid(x - s * 0.1f, groundY - s * 0.8f, 0, s * 0.13f, lit(l, pal::kLeafLight, 0.8f));  // the light on it
}

void cottage(const Light& l, float x, float groundY, float w, float h, Rgb roof, float t, float depth) {
    x += r3d::eyeShift(depth);
    const float top = groundY - h;
    C2D_DrawRectSolid(x + w * 0.66f, top - h * 0.5f, 0, w * 0.12f, h * 0.4f, lit(l, pal::kRock));  // the chimney
    C2D_DrawRectSolid(x, top, 0, w, h, lit(l, pal::kWall));
    C2D_DrawTriangle(x - w * 0.12f, top + 1, lit(l, roof), x + w * 1.12f, top + 1, lit(l, roof), x + w * 0.5f,
                     top - h * 0.62f, lit(l, mix(roof, pal::kCloud, 0.15f)), 0);
    C2D_DrawRectSolid(x + w * 0.14f, groundY - h * 0.55f, 0, w * 0.18f, h * 0.55f, lit(l, pal::kWoodDark));  // its door
    const float wx = x + w * 0.56f, wy = top + h * 0.22f, ws = w * 0.24f;
    const float glowAmt = l.night + 0.5f * l.evening;
    C2D_DrawRectSolid(wx, wy, 0, ws, ws, rgb(mix(pal::kWindowDark, pal::kWindowLit, glowAmt)));
    if (glowAmt > 0.2f) glow(wx + ws / 2, wy + ws / 2, ws * 1.4f, rgb(pal::kWindowLit), 0.5f * glowAmt);
    (void)t;
}

void bunting(const Light& l, float x0, float x1, float y, float sag, float t) {
    const u32 string = lit(l, pal::kWoodDark);
    const int n = static_cast<int>((x1 - x0) / 18.0f);
    auto at = [&](float s) { return Vec2{x0 + (x1 - x0) * s, y + sag * 4 * s * (1 - s)}; };
    Vec2 prev = at(0);
    for (int i = 1; i <= 24; ++i) {
        const Vec2 p = at(i / 24.0f);
        C2D_DrawLine(prev.x, prev.y, string, p.x, p.y, string, 1.2f, 0);
        prev = p;
    }
    const float glowAmt = l.night + 0.4f * l.evening;
    for (int i = 0; i < n; ++i) {
        const float s = (i + 0.5f) / n;
        const Vec2 p = at(s);
        const float sway = std::sin(t * 1.6f + i * 0.9f) * 1.6f;
        const Rgb c = i % 5 == 4 ? pal::kAwningCream : pal::kAwning[i % 4];
        C2D_DrawTriangle(p.x - 6, p.y, lit(l, c), p.x + 6, p.y, lit(l, c), p.x + sway, p.y + 13, lit(l, c), 0);
        if (glowAmt > 0.15f && i % 2) {  // little lights along the string after dark
            const Vec2 q = at(s + 0.5f / n);
            glow(q.x, q.y + 2, 6, theme::kClutchGold, glowAmt);
            C2D_DrawCircleSolid(q.x, q.y + 2, 0, 1.6f, withAlpha(theme::kClutchGold, 0.5f + 0.5f * glowAmt));
        }
    }
}

void lantern(const Light& l, float x, float y, float s, float t) {
    const float glowAmt = 0.25f + 0.75f * (l.night + 0.6f * l.evening);
    const float flicker = 0.9f + 0.1f * std::sin(t * 7.0f + x);
    glow(x, y, s * 2.4f, theme::kClutchGold, glowAmt * flicker);
    C2D_DrawRectSolid(x - s * 0.08f, y - s * 1.1f, 0, s * 0.16f, s * 0.5f, lit(l, pal::kWoodDark));
    C2D_DrawEllipseSolid(x - s * 0.5f, y - s * 0.62f, 0, s, s * 1.24f, withAlpha(theme::kEmber, 0.85f + 0.15f * glowAmt));
    C2D_DrawEllipseSolid(x - s * 0.28f, y - s * 0.4f, 0, s * 0.56f, s * 0.8f, withAlpha(theme::kClutchGold, glowAmt * flicker));
    C2D_DrawRectSolid(x - s * 0.3f, y - s * 0.7f, 0, s * 0.6f, s * 0.14f, lit(l, pal::kWoodDark));
    C2D_DrawRectSolid(x - s * 0.3f, y + s * 0.56f, 0, s * 0.6f, s * 0.14f, lit(l, pal::kWoodDark));
}

void hangingSign(App& app, const char* title, float cx, float y, float w) {
    const float h = 30;
    const u32 rope = theme::rgba(pal::kWoodDark.r, pal::kWoodDark.g, pal::kWoodDark.b);
    C2D_DrawLine(cx - w * 0.36f, 0, rope, cx - w * 0.36f, y + 3, rope, 1.5f, 0);
    C2D_DrawLine(cx + w * 0.36f, 0, rope, cx + w * 0.36f, y + 3, rope, 1.5f, 0);
    panel({cx - w / 2, y + 2, w, h}, withAlpha(theme::kDenPlum, 0.3f));  // a soft shadow
    panel({cx - w / 2, y, w, h}, rgb(pal::kWoodLight));
    C2D_DrawRectSolid(cx - w / 2 + 3, y + h - 5, 0, w - 6, 2, rgb(pal::kWood, 0.5f));  // the plank's grain
    C2D_DrawCircleSolid(cx - w * 0.36f, y + 5, 0, 1.8f, rgb(pal::kWoodDark));
    C2D_DrawCircleSolid(cx + w * 0.36f, y + 5, 0, 1.8f, rgb(pal::kWoodDark));
    textCentered(app, title, cx, y + h / 2 + 1, 0.78f, theme::kDenPlum, w - 16, Face::Title);
}

void caption(App& app, const char* line1, const char* line2, float y) {
    if (app.toast && y > 150) return;  // the toast has the bottom for its few seconds
    const bool two = line2 && line2[0];
    const float w1 = textWidth(app, line1, 0.52f), w2 = two ? textWidth(app, line2, 0.4f) : 0;
    const float w = std::fmin(384.0f, std::fmax(160.0f, std::fmax(w1, w2) + 30));
    const float h = two ? 38.0f : 24.0f;
    panel({200 - w / 2, y + 2, w, h}, withAlpha(theme::kDenPlum, 0.3f));
    panel({200 - w / 2, y, w, h}, withAlpha(theme::kShell, 0.94f));
    panel({200 - w / 2 + 3, y + 4, 3, h - 8}, withAlpha(theme::kEmber, 0.9f));
    textCentered(app, line1, 200, y + 12, 0.52f, theme::kDenPlum, w - 20);
    if (two) textCentered(app, line2, 200, y + 28, 0.4f, theme::kDusk, w - 20);
}

void trinket(Trinket t, float x, float y, float s) {
    switch (t) {
        case Trinket::Pebble:
            C2D_DrawEllipseSolid(x - s * 0.5f, y - s * 0.32f, 0, s, s * 0.64f, rgb(pal::kPebble));
            C2D_DrawEllipseSolid(x - s * 0.3f, y - s * 0.25f, 0, s * 0.3f, s * 0.16f, rgb(pal::kCloud, 0.6f));
            break;
        case Trinket::Coin:
            C2D_DrawCircleSolid(x, y, 0, s * 0.45f, rgb(pal::kWood));
            C2D_DrawCircleSolid(x, y, 0, s * 0.38f, rgb(pal::kCoin));
            C2D_DrawCircleSolid(x, y, 0, s * 0.2f, rgb(pal::kWoodLight));
            break;
        case Trinket::Feather:
            C2D_DrawEllipseSolid(x - s * 0.18f, y - s * 0.5f, 0, s * 0.36f, s, rgb(pal::kFeather));
            C2D_DrawLine(x, y - s * 0.45f, rgb(pal::kRock), x + s * 0.05f, y + s * 0.6f, rgb(pal::kRock), 1.2f, 0);
            break;
        case Trinket::Crystal:
            C2D_DrawTriangle(x - s * 0.3f, y, rgb(pal::kCrystal), x + s * 0.3f, y, rgb(pal::kCrystal), x, y - s * 0.6f,
                             rgb(pal::kCloud), 0);
            C2D_DrawTriangle(x - s * 0.3f, y, rgb(pal::kCrystal), x + s * 0.3f, y, rgb(pal::kCrystal), x, y + s * 0.4f,
                             rgb(pal::kRock), 0);
            break;
        case Trinket::Pearl:
            C2D_DrawCircleSolid(x, y, 0, s * 0.36f, rgb(pal::kPearl));
            C2D_DrawCircleSolid(x - s * 0.12f, y - s * 0.12f, 0, s * 0.1f, rgb(pal::kFlower[0], 0.8f));
            break;
        default:  // a fossil tooth
            C2D_DrawTriangle(x - s * 0.28f, y - s * 0.4f, rgb(pal::kFossil), x + s * 0.28f, y - s * 0.4f, rgb(pal::kFossil),
                             x + s * 0.06f, y + s * 0.5f, rgb(pal::kWoodLight), 0);
            break;
    }
}

void coinPile(float x, float y, float s, int coins) {
    if (coins > 9) coins = 9;
    for (int i = 0; i < coins; ++i) {
        const float cx = x + ((i * 7) % 5 - 2) * s * 0.28f, cy = y - (i / 3) * s * 0.18f;
        C2D_DrawEllipseSolid(cx - s * 0.32f, cy - s * 0.12f, 0, s * 0.64f, s * 0.28f, rgb(pal::kWood));
        C2D_DrawEllipseSolid(cx - s * 0.3f, cy - s * 0.15f, 0, s * 0.6f, s * 0.24f, rgb(pal::kCoin));
    }
}

}  // namespace ec::paint
