#include "app/ui_draw.hpp"

#include <cmath>

#include "app/audio.hpp"
#include "app/theme.hpp"
#include "core/clock.hpp"
#include "core/genetics.hpp"

namespace ec {

u32 withAlpha(u32 c, float a) {
    const u32 alpha = static_cast<u32>((a < 0 ? 0 : (a > 1 ? 1 : a)) * 255.0f);
    return (c & 0x00FFFFFF) | (alpha << 24);
}

u32 fromRgb(Rgb c, u8 a) { return theme::rgba(c.r, c.g, c.b, a); }

void text(App& app, const char* s, float x, float y, float scale, u32 color, u32 flags) {
    C2D_Text t;
    C2D_TextParse(&t, app.textBuf, s);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor | flags, x, y, 0.5f, scale, scale, color);
}

void verticalGradient(float x, float y, float w, float h, u32 top, u32 bottom) {
    C2D_DrawRectangle(x, y, 0, w, h, top, top, bottom, bottom);
}

// Soft additive-looking glow from stacked translucent circles.
void glow(float x, float y, float radius, u32 color, float strength) {
    for (int i = 5; i >= 1; --i) {
        const float r = radius * (0.4f + 0.14f * i);
        C2D_DrawCircleSolid(x, y, 0, r, withAlpha(color, strength * 0.12f));
    }
}

void heart(float cx, float cy, float size, u32 color) {
    const float r = size * 0.3f;
    C2D_DrawCircleSolid(cx - r * 0.9f, cy - r * 0.3f, 0, r, color);
    C2D_DrawCircleSolid(cx + r * 0.9f, cy - r * 0.3f, 0, r, color);
    C2D_DrawTriangle(cx - size * 0.56f, cy - r * 0.05f, color, cx + size * 0.56f, cy - r * 0.05f, color, cx,
                     cy + size * 0.55f, color, 0);
}

void egg(float cx, float cy, float w, float h, Rgb shell, Rgb glowC, float glowAmt) {
    glow(cx, cy + h * 0.1f, w * 0.9f, fromRgb(glowC), glowAmt);
    C2D_DrawEllipseSolid(cx - w / 2, cy - h / 2, 0, w, h, fromRgb(shell));
    // Inner light shining through the shell.
    C2D_DrawEllipseSolid(cx - w * 0.3f, cy - h * 0.12f, 0, w * 0.6f, h * 0.55f, fromRgb(glowC, static_cast<u8>(90 * glowAmt)));
    // Speckles.
    static constexpr float kSpots[][3] = {{-0.2f, -0.25f, 0.06f}, {0.18f, -0.1f, 0.05f}, {-0.05f, 0.2f, 0.07f},
                                          {0.22f, 0.25f, 0.04f},  {-0.25f, 0.1f, 0.04f}};
    for (const auto& s : kSpots)
        C2D_DrawCircleSolid(cx + s[0] * w, cy + s[1] * h, 0, s[2] * w, withAlpha(theme::kDenPlum, 0.25f));
    // Highlight.
    C2D_DrawEllipseSolid(cx - w * 0.28f, cy - h * 0.38f, 0, w * 0.18f, h * 0.22f, withAlpha(theme::kShell, 0.5f));
}

namespace {

struct Pulse {
    float brightness, speed;
};

Pulse moodPulse(Mood m) {
    switch (m) {
        case Mood::Joyful: return {1.0f, 6.0f};
        case Mood::Content: return {0.85f, 3.0f};
        case Mood::Restless: return {0.7f, 9.0f};
        case Mood::Sulky: return {0.45f, 1.5f};
        default: return {0.25f, 1.0f};
    }
}

}  // namespace

float heartglowLevel(const Dragon& d, float t) {
    const Pulse p = moodPulse(moodOf(d));
    return p.brightness * (0.75f + 0.25f * std::sin(t * p.speed));
}

// Placeholder dragon: proportions follow the cute -> majestic curve from the art doc.
void dragonPlaceholder(const Dragon& d, float cx, float groundY, float scale, float t) {
    const Genome& g = d.genome;
    const float grow = (scale - 0.25f) / 0.75f;  // 0 hatchling .. 1 adult
    const float s = 0.55f + 0.45f * grow;         // drawn size (keeps babies visible)
    const Rgb base = hsvToRgb(g.baseH, g.baseS, g.baseV);
    const Rgb accent = hsvToRgb(g.accentH, static_cast<u8>(g.baseS / 2), g.accentV);
    const u32 baseC = fromRgb(base), accentC = fromRgb(accent);

    const float bodyW = 90 * s + 30 * grow, bodyH = 62 * s;
    const float bodyX = cx, bodyY = groundY - bodyH * 0.55f;
    const float bob = std::sin(t * 2.0f) * 2.0f;

    // Tail
    C2D_DrawTriangle(bodyX + bodyW * 0.3f, bodyY, baseC, bodyX + bodyW * 0.3f, bodyY + bodyH * 0.35f, baseC,
                     bodyX + bodyW * (0.75f + 0.4f * grow), bodyY + bodyH * 0.3f - 20 * grow, baseC, 0);
    // Wings
    const float wing = 26 + 70 * grow;
    const float flap = std::sin(t * 3.0f) * 4.0f;
    C2D_DrawTriangle(bodyX - 6, bodyY - bodyH * 0.2f, accentC, bodyX + 18, bodyY - bodyH * 0.25f, accentC, bodyX + 4,
                     bodyY - bodyH * 0.25f - wing + flap, accentC, 0);
    // Body + belly
    C2D_DrawEllipseSolid(bodyX - bodyW / 2, bodyY - bodyH / 2 + bob, 0, bodyW, bodyH, baseC);
    C2D_DrawEllipseSolid(bodyX - bodyW * 0.32f, bodyY - bodyH * 0.25f + bob, 0, bodyW * 0.42f, bodyH * 0.7f, accentC);
    // Neck and head: the head shrinks relative to the body as the dragon grows.
    const float neck = 6 + 40 * grow;
    const float headR = 30 * (1.0f - 0.35f * grow) * (0.8f + 0.4f * s);
    const float hx = bodyX - bodyW * 0.38f - neck * 0.4f, hy = bodyY - bodyH * 0.45f - neck + bob;
    C2D_DrawTriangle(bodyX - bodyW * 0.3f, bodyY - bodyH * 0.2f + bob, baseC, bodyX - bodyW * 0.1f,
                     bodyY - bodyH * 0.35f + bob, baseC, hx, hy, baseC, 0);
    // Horns
    const u32 hornC = theme::kClutchGold;
    const float horn = 6 + 18 * grow;
    C2D_DrawTriangle(hx + headR * 0.1f, hy - headR * 0.6f, hornC, hx + headR * 0.5f, hy - headR * 0.4f, hornC,
                     hx + headR * 0.6f, hy - headR * 0.6f - horn, hornC, 0);
    C2D_DrawTriangle(hx - headR * 0.4f, hy - headR * 0.6f, hornC, hx, hy - headR * 0.7f, hornC, hx - headR * 0.05f,
                     hy - headR * 0.75f - horn, hornC, 0);
    C2D_DrawCircleSolid(hx, hy, 0, headR, baseC);
    C2D_DrawEllipseSolid(hx - headR * 1.25f, hy - headR * 0.05f, 0, headR * 0.9f, headR * 0.6f, baseC);  // snout
    // Eyes: big and sparkly when small, calmer when grown. Closed while sleeping.
    const float eyeR = headR * (0.34f - 0.14f * grow);
    const float ex = hx - headR * 0.25f, ey = hy - headR * 0.1f;
    const bool closed = d.napping;
    if (closed) {
        C2D_DrawRectSolid(ex - eyeR, ey, 0, eyeR * 2, 2, theme::kDenPlum);
    } else {
        C2D_DrawCircleSolid(ex, ey, 0, eyeR, theme::kShell);
        C2D_DrawCircleSolid(ex - eyeR * 0.15f, ey + eyeR * 0.1f, 0, eyeR * 0.7f, theme::kDenPlum);
        C2D_DrawCircleSolid(ex - eyeR * 0.35f, ey - eyeR * 0.25f, 0, eyeR * 0.25f, theme::kShell);
    }
    // Heartglow, drawn last so the hatchling's big head never covers it. White-hot core so it
    // reads even when the glow matches the body (Tide on teal).
    const Rgb glowRgb = heartglowColor(static_cast<Element>(g.elementA));
    const float level = heartglowLevel(d, t);
    const float heartX = bodyX - bodyW * 0.08f, heartY = bodyY + bodyH * 0.18f + bob;
    glow(heartX, heartY, 16 + 10 * s, fromRgb(glowRgb), level);
    heart(heartX, heartY, 12 + 6 * s, fromRgb(glowRgb, static_cast<u8>(140 + 115 * level)));
    heart(heartX, heartY - 1, (12 + 6 * s) * 0.5f, withAlpha(theme::kShell, 0.35f + 0.6f * level));
}

void panel(const Rect& r, u32 color) {
    C2D_DrawRectSolid(r.x + 4, r.y, 0, r.w - 8, r.h, color);
    C2D_DrawRectSolid(r.x, r.y + 4, 0, r.w, r.h - 8, color);
    C2D_DrawCircleSolid(r.x + 4, r.y + 4, 0, 4, color);
    C2D_DrawCircleSolid(r.x + r.w - 4, r.y + 4, 0, 4, color);
    C2D_DrawCircleSolid(r.x + 4, r.y + r.h - 4, 0, 4, color);
    C2D_DrawCircleSolid(r.x + r.w - 4, r.y + r.h - 4, 0, 4, color);
}

bool button(App& app, const Rect& r, const char* label, const Input& in) {
    // Fires when the stylus lifts over it (a few pixels forgiving at the edges); lights up
    // while pressed.
    const Rect reach{r.x - 4, r.y - 4, r.w + 8, r.h + 8};
    const bool pressed = in.touching && reach.contains(in.tx, in.ty);
    const bool hit = in.released && reach.contains(in.rx, in.ry);
    if (hit) audio::playSfx(audio::Sfx::Tap);
    panel(r, pressed || hit ? theme::kClutchGold : theme::kShell);
    text(app, label, r.x + r.w / 2, r.y + r.h / 2 - 8, 0.55f, theme::kDenPlum);
    return hit;
}

void gauge(App& app, float x, float y, const char* label, float value) {
    text(app, label, x, y, 0.42f, theme::kShell, C2D_AlignLeft);
    const Rect bar{x, y + 14, 66, 8};
    panel(bar, theme::kDusk);
    const u32 fill = value < 25 ? theme::kRose : (value < 50 ? theme::kEmber : theme::kClutchGold);
    if (value > 2) panel({bar.x, bar.y, bar.w * value / 100.0f, bar.h}, fill);
}

void embers(float t, float w) {
    for (int i = 0; i < 14; ++i) {
        const float seed = i * 37.3f;
        const float x = std::fmod(seed * 7.1f + std::sin(t * 0.5f + i) * 12.0f, w);
        const float y = kScreenH - std::fmod(t * (10 + i % 5 * 4) + seed * 3.0f, kScreenH + 20);
        C2D_DrawCircleSolid(x, y, 0, 1.2f + (i % 3) * 0.6f, withAlpha(theme::kClutchGold, 0.35f + 0.1f * (i % 4)));
    }
}

}  // namespace ec
