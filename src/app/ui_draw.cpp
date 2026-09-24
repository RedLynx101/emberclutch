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

namespace {

C2D_Font g_fonts[2] = {nullptr, nullptr};
float g_norm[2] = {1.0f, 1.0f};  // font scale that matches the system font's line height

// Parses s in a face; returns the scale that draws it at `scale` (shrunk to maxWidth).
float prepare(App& app, C2D_Text& t, const char* s, float scale, Face face, float maxWidth) {
    const int f = static_cast<int>(face);
    C2D_TextFontParse(&t, g_fonts[f], app.textBuf, s);
    C2D_TextOptimize(&t);
    float k = scale * g_norm[f];
    if (maxWidth > 0) {
        float w = 0;
        C2D_TextGetDimensions(&t, k, k, &w, nullptr);
        if (w > maxWidth) k *= maxWidth / w;
    }
    return k;
}

}  // namespace

void loadFonts() {
    const FINF_s* sys = C2D_FontGetInfo(nullptr);
    const float sysLine = sys && sys->lineFeed ? sys->lineFeed : 30.0f;
    static const char* const kFiles[2] = {"romfs:/fonts/ui.bcfnt", "romfs:/fonts/title.bcfnt"};
    for (int i = 0; i < 2; ++i) {
        g_fonts[i] = C2D_FontLoad(kFiles[i]);
        const FINF_s* info = g_fonts[i] ? C2D_FontGetInfo(g_fonts[i]) : nullptr;
        g_norm[i] = info && info->lineFeed ? sysLine / info->lineFeed : 1.0f;
    }
}

void freeFonts() {
    for (C2D_Font& f : g_fonts) {
        if (f) C2D_FontFree(f);
        f = nullptr;
    }
}

void text(App& app, const char* s, float x, float y, float scale, u32 color, u32 flags, float maxWidth, Face face) {
    C2D_Text t;
    const float k = prepare(app, t, s, scale, face, maxWidth);
    C2D_DrawText(&t, C2D_WithColor | flags, x, y, 0.5f, k, k, color);
}

void textCentered(App& app, const char* s, float cx, float cy, float scale, u32 color, float maxWidth, Face face) {
    C2D_Text t;
    const float k = prepare(app, t, s, scale, face, maxWidth);
    float h = 0;
    C2D_TextGetDimensions(&t, k, k, nullptr, &h);
    C2D_DrawText(&t, C2D_WithColor | C2D_AlignCenter, cx, cy - h * 0.46f, 0.5f, k, k, color);  // letters sit a touch high
}

float textWidth(App& app, const char* s, float scale, Face face) {
    C2D_Text t;
    const float k = prepare(app, t, s, scale, face, 0);
    float w = 0;
    C2D_TextGetDimensions(&t, k, k, &w, nullptr);
    return w;
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

namespace {

// A quarter disc as a fan of triangles from angle a0: pieces that never overlap, so a
// translucent panel's corners are as see-through as the rest of it.
void cornerFan(float cx, float cy, float r, float a0, u32 color) {
    constexpr int kSeg = 3;
    constexpr float kStep = 1.5707963f / kSeg;
    for (int i = 0; i < kSeg; ++i) {
        const float t0 = a0 + i * kStep, t1 = t0 + kStep;
        C2D_DrawTriangle(cx, cy, color, cx + std::cos(t0) * r, cy + std::sin(t0) * r, color, cx + std::cos(t1) * r,
                         cy + std::sin(t1) * r, color, 0);
    }
}

}  // namespace

void panel(const Rect& r, u32 color) {
    const float k = std::fmin(4.0f, std::fmin(r.w, r.h) * 0.5f);  // corner radius
    C2D_DrawRectSolid(r.x + k, r.y, 0, r.w - 2 * k, r.h, color);
    C2D_DrawRectSolid(r.x, r.y + k, 0, k, r.h - 2 * k, color);
    C2D_DrawRectSolid(r.x + r.w - k, r.y + k, 0, k, r.h - 2 * k, color);
    cornerFan(r.x + k, r.y + k, k, 3.1415927f, color);
    cornerFan(r.x + r.w - k, r.y + k, k, 4.712389f, color);
    cornerFan(r.x + r.w - k, r.y + r.h - k, k, 0.0f, color);
    cornerFan(r.x + k, r.y + r.h - k, k, 1.5707963f, color);
}

bool button(App& app, const Rect& r, const char* label, const Input& in, u32 color) {
    // Fires when the stylus lifts over it (a few pixels forgiving at the edges); lights up
    // while pressed.
    const Rect reach{r.x - 4, r.y - 4, r.w + 8, r.h + 8};
    const bool pressed = in.touching && reach.contains(in.tx, in.ty);
    const bool hit = in.released && reach.contains(in.rx, in.ry);
    if (hit) audio::playSfx(audio::Sfx::Tap);
    panel({r.x, r.y + 2, r.w, r.h}, withAlpha(theme::kDenPlum, 0.35f));  // a soft drop shadow
    panel(r, pressed || hit ? theme::kClutchGold : (color ? color : theme::kShell));
    textCentered(app, label, r.x + r.w / 2, r.y + r.h / 2, r.h >= 40 ? 0.62f : 0.55f, theme::kDenPlum, r.w - 12);
    return hit;
}

void drawToast(App& app) {
    if (!app.toast) return;
    // In over 0.2 s, out over the last 0.35 s (toasts live 3 s, showToast).
    const float a = std::fmax(0.0f, std::fmin(1.0f, std::fmin((3.0f - app.toastTime) / 0.2f, app.toastTime / 0.35f)));
    const float w = std::fmin(384.0f, textWidth(app, app.toast, 0.5f) + 28.0f);
    panel({200 - w / 2, 207, w, 26}, withAlpha(theme::kDenPlum, 0.86f * a));
    panel({200 - w / 2 + 3, 209, 3, 22}, withAlpha(theme::kClutchGold, 0.9f * a));  // an ember at its edge
    textCentered(app, app.toast, 200, 220, 0.5f, withAlpha(theme::kShell, a), 370);
}

void drawSaveIcon(App& app) {
    if (app.saveFlash <= 0) return;
    const float a = std::fmin(1.0f, app.saveFlash / 0.4f);
    const float cx = 384, cy = 18;
    for (int i = 0; i < 6; ++i) {  // a ring of embers turning around the egg
        const float ang = app.t * 5.0f + i * 1.0472f;
        C2D_DrawCircleSolid(cx + std::cos(ang) * 11, cy + std::sin(ang) * 11, 0, 1.2f + 0.25f * i,
                            withAlpha(theme::kClutchGold, a * (0.25f + 0.12f * i)));
    }
    C2D_DrawEllipseSolid(cx - 5, cy - 6.5f, 0, 10, 13, withAlpha(theme::kShell, a));
    C2D_DrawEllipseSolid(cx - 2.5f, cy - 1, 0, 5, 5, withAlpha(theme::kEmber, a * 0.7f));
}

void gauge(App& app, float x, float y, const char* label, float value) {
    text(app, label, x, y, 0.42f, theme::kShell, C2D_AlignLeft);
    const Rect bar{x, y + 14, 66, 8};
    panel(bar, theme::kTrack);
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
