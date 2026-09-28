// The accessories' and dyes' little pictures for the wardrobe and the glade's stalls (1.0, D90):
// each shape drawn in a few citro2d strokes in its own colours (the accessory table's data), so
// a list reads at a glance; the dragon on the top screen shows the real thing.
#include <cmath>

#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "app/wardrobe.hpp"
#include "core/accessories.hpp"

namespace ec {
namespace {

constexpr float kPi = 3.14159265f;

void tri(float ax, float ay, float bx, float by, float cx, float cy, u32 c) { C2D_DrawTriangle(ax, ay, c, bx, by, c, cx, cy, c, 0.5f); }
void rect(float x, float y, float w, float h, u32 c) { C2D_DrawRectSolid(x, y, 0.5f, w, h, c); }
void disc(float x, float y, float r, u32 c) { C2D_DrawCircleSolid(x, y, 0.5f, r, c); }
void oval(float x, float y, float w, float h, u32 c) { C2D_DrawEllipseSolid(x - w / 2, y - h / 2, 0.5f, w, h, c); }
void line(float ax, float ay, float bx, float by, float width, u32 c) { C2D_DrawLine(ax, ay, c, bx, by, c, width, 0.5f); }
// A ring of short strokes (an outline circle).
void ring(float x, float y, float r, float width, u32 c) {
    constexpr int kSegs = 14;
    for (int k = 0; k < kSegs; ++k) {
        const float a0 = 2 * kPi * k / kSegs, a1 = 2 * kPi * (k + 1) / kSegs;
        line(x + std::cos(a0) * r, y + std::sin(a0) * r, x + std::cos(a1) * r, y + std::sin(a1) * r, width, c);
    }
}
void star(float x, float y, float r, u32 c) {
    for (int k = 0; k < 5; ++k) {
        const float a = -kPi / 2 + 2 * kPi * k / 5, l = a - 0.63f, rr = a + 0.63f;
        tri(x + std::cos(a) * r, y + std::sin(a) * r, x + std::cos(l) * r * 0.4f, y + std::sin(l) * r * 0.4f,
            x + std::cos(rr) * r * 0.4f, y + std::sin(rr) * r * 0.4f, c);
    }
    disc(x, y, r * 0.42f, c);
}
void flower(float x, float y, float r, u32 petal, u32 heart) {
    for (int k = 0; k < 5; ++k) {
        const float a = 2 * kPi * k / 5;
        disc(x + std::cos(a) * r * 0.55f, y + std::sin(a) * r * 0.55f, r * 0.45f, petal);
    }
    disc(x, y, r * 0.35f, heart);
}
void bow(float x, float y, float s, u32 c, u32 knot) {
    tri(x, y, x - s, y - s * 0.6f, x - s, y + s * 0.6f, c);
    tri(x, y, x + s, y - s * 0.6f, x + s, y + s * 0.6f, c);
    tri(x - s * 0.1f, y, x - s * 0.5f, y + s * 1.1f, x - s * 0.2f, y + s * 1.1f, c);
    tri(x + s * 0.1f, y, x + s * 0.5f, y + s * 1.1f, x + s * 0.2f, y + s * 1.1f, c);
    disc(x, y, s * 0.28f, knot);
}
void bell(float x, float y, float s, u32 c, u32 clapper) {
    disc(x, y - s * 0.35f, s * 0.35f, c);
    tri(x - s * 0.35f, y - s * 0.35f, x + s * 0.35f, y - s * 0.35f, x + s * 0.7f, y + s * 0.5f, c);
    tri(x - s * 0.35f, y - s * 0.35f, x + s * 0.7f, y + s * 0.5f, x - s * 0.7f, y + s * 0.5f, c);
    disc(x, y + s * 0.6f, s * 0.18f, clapper);
}

}  // namespace

void drawAccessoryIcon(int accessory, float x, float y, float size) {
    if (accessory < 0 || accessory >= accessoryCount()) return;
    const Accessory& a = accessoryInfo(accessory);
    const u32 c0 = fromRgb(a.colour[0]), c1 = fromRgb(a.colour[1]), trim = fromRgb(a.trim), gem = fromRgb(a.gem);
    const float s = size * 0.5f;  // half the box
    switch (a.shape) {
        case WearShape::SunHat:
            oval(x, y + s * 0.3f, s * 2.0f, s * 0.6f, c0);
            oval(x, y - s * 0.05f, s * 1.0f, s * 0.9f, c0);
            rect(x - s * 0.5f, y + s * 0.05f, s * 1.0f, s * 0.2f, c1);
            flower(x - s * 0.35f, y + s * 0.12f, s * 0.2f, gem, trim);
            break;
        case WearShape::TopHat:
            oval(x, y + s * 0.55f, s * 1.6f, s * 0.4f, c0);
            rect(x - s * 0.45f, y - s * 0.7f, s * 0.9f, s * 1.25f, c0);
            rect(x - s * 0.45f, y + s * 0.2f, s * 0.9f, s * 0.22f, c1);
            break;
        case WearShape::PartyHat:
            tri(x, y - s * 0.95f, x - s * 0.6f, y + s * 0.6f, x + s * 0.6f, y + s * 0.6f, c0);
            tri(x, y - s * 0.95f, x - s * 0.2f, y + s * 0.6f, x + s * 0.2f, y + s * 0.6f, c1);
            disc(x, y - s * 0.9f, s * 0.2f, trim);
            break;
        case WearShape::FlowerCrown:
            ring(x, y, s * 0.7f, 2.5f, c0);
            for (int k = 0; k < 5; ++k) {
                const float ang = kPi * (0.9f + 0.3f * k);
                flower(x + std::cos(ang) * s * 0.7f, y + std::sin(ang) * s * 0.7f, s * 0.26f, k % 2 ? c1 : trim, gem);
            }
            break;
        case WearShape::Tiara:
            rect(x - s * 0.8f, y + s * 0.2f, s * 1.6f, s * 0.2f, c0);
            for (int k = 0; k < 5; ++k) {
                const float px = x + (k - 2) * s * 0.36f, h = k == 2 ? s * 0.9f : (k % 2 ? s * 0.55f : s * 0.4f);
                tri(px - s * 0.14f, y + s * 0.22f, px + s * 0.14f, y + s * 0.22f, px, y + s * 0.22f - h, c0);
                disc(px, y + s * 0.3f, s * 0.09f, gem);
            }
            break;
        case WearShape::Crown:
            rect(x - s * 0.7f, y, s * 1.4f, s * 0.55f, c0);
            for (int k = 0; k < 4; ++k) {
                const float px = x - s * 0.7f + s * 0.35f * (k + 0.5f) * 1.0f;
                tri(px - s * 0.18f, y, px + s * 0.18f, y, px, y - s * 0.6f, c0);
            }
            disc(x, y + s * 0.28f, s * 0.14f, gem);
            break;
        case WearShape::Circlet:
            ring(x, y + s * 0.1f, s * 0.75f, 2.0f, c0);
            star(x, y + s * 0.8f, s * 0.3f, gem);
            break;
        case WearShape::FeatherCrest:
            for (int k = 0; k < 5; ++k) {
                const float ang = -kPi / 2 + (k - 2) * 0.35f;
                const float tx = x + std::cos(ang) * s * 0.95f, ty = y + s * 0.5f + std::sin(ang) * s * 1.3f;
                tri(x - s * 0.08f, y + s * 0.5f, x + s * 0.08f, y + s * 0.5f, tx, ty, k % 2 ? c1 : c0);
            }
            disc(x, y + s * 0.55f, s * 0.2f, trim);
            break;
        case WearShape::NeckBow:
        case WearShape::TailBow:
            bow(x, y, s * 0.8f, c0, a.shape == WearShape::TailBow ? c1 : trim);
            break;
        case WearShape::Scarf:
            rect(x - s * 0.85f, y - s * 0.45f, s * 1.7f, s * 0.5f, c0);
            rect(x - s * 0.85f, y - s * 0.28f, s * 1.7f, s * 0.16f, c1);
            rect(x - s * 0.55f, y, s * 0.35f, s * 0.9f, c0);
            rect(x - s * 0.1f, y, s * 0.35f, s * 0.75f, c1);
            break;
        case WearShape::Collar:
            ring(x, y, s * 0.7f, 5.0f, c0);
            for (int k = 0; k < 5; ++k) {
                const float ang = kPi * (0.2f + 0.15f * k);
                disc(x + std::cos(ang) * s * 0.7f, y + std::sin(ang) * s * 0.7f, s * 0.1f, trim);
            }
            break;
        case WearShape::Bell:
        case WearShape::TailBell:
            ring(x, y - s * 0.35f, s * 0.6f, 3.0f, c0);
            bell(x, y + s * 0.4f, s * 0.55f, trim, gem);
            break;
        case WearShape::Pendant:
            line(x - s * 0.7f, y - s * 0.6f, x, y + s * 0.1f, 1.5f, trim);
            line(x + s * 0.7f, y - s * 0.6f, x, y + s * 0.1f, 1.5f, trim);
            oval(x, y + s * 0.45f, s * 0.45f, s * 0.65f, trim);
            oval(x, y + s * 0.45f, s * 0.32f, s * 0.5f, gem);
            break;
        case WearShape::Ruff:
            for (int k = 0; k < 10; ++k) {
                const float a0 = 2 * kPi * k / 10, a1 = 2 * kPi * (k + 1) / 10;
                tri(x, y, x + std::cos(a0) * s * 0.9f, y + std::sin(a0) * s * 0.9f, x + std::cos(a1) * s * 0.7f,
                    y + std::sin(a1) * s * 0.7f, k % 2 ? c1 : c0);
            }
            disc(x, y, s * 0.4f, theme::kDenPlum);
            break;
        case WearShape::Lei:
        case WearShape::TailWreath:
            ring(x, y, s * 0.65f, 2.0f, a.shape == WearShape::Lei ? trim : c0);
            for (int k = 0; k < 6; ++k) {
                const float ang = 2 * kPi * k / 6;
                flower(x + std::cos(ang) * s * 0.65f, y + std::sin(ang) * s * 0.65f, s * 0.26f,
                       k % 2 ? c1 : (a.shape == WearShape::Lei ? c0 : trim), gem);
            }
            break;
        case WearShape::Saddle:
            oval(x, y + s * 0.1f, s * 1.8f, s * 0.9f, c1);
            oval(x, y - s * 0.05f, s * 1.1f, s * 0.6f, c0);
            rect(x + s * 0.3f, y - s * 0.4f, s * 0.2f, s * 0.35f, c0);
            line(x - s * 0.6f, y + s * 0.3f, x - s * 0.6f, y + s * 0.85f, 2.0f, trim);
            line(x + s * 0.6f, y + s * 0.3f, x + s * 0.6f, y + s * 0.85f, 2.0f, trim);
            break;
        case WearShape::Cape:
            tri(x - s * 0.35f, y - s * 0.8f, x + s * 0.35f, y - s * 0.8f, x + s * 0.85f, y + s * 0.85f, c0);
            tri(x - s * 0.35f, y - s * 0.8f, x + s * 0.85f, y + s * 0.85f, x - s * 0.85f, y + s * 0.85f, c0);
            rect(x - s * 0.85f, y + s * 0.65f, s * 1.7f, s * 0.2f, c1);
            disc(x, y - s * 0.78f, s * 0.14f, trim);
            star(x + s * 0.2f, y + s * 0.1f, s * 0.14f, gem);
            break;
        case WearShape::Blanket:
            rect(x - s * 0.85f, y - s * 0.6f, s * 1.7f, s * 1.2f, trim);
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 2; ++j)
                    rect(x - s * 0.75f + i * s * 0.5f, y - s * 0.5f + j * s * 0.5f, s * 0.5f, s * 0.5f, (i + j) % 2 ? c1 : c0);
            break;
        case WearShape::Sash:
            tri(x - s * 0.9f, y - s * 0.3f, x - s * 0.9f, y + s * 0.2f, x + s * 0.9f, y + s * 0.3f, c0);
            tri(x - s * 0.9f, y - s * 0.3f, x + s * 0.9f, y + s * 0.3f, x + s * 0.9f, y - s * 0.2f, c0);
            flower(x, y, s * 0.35f, c1, gem);
            break;
        case WearShape::Garland:
            for (int k = 0; k < 5; ++k) {
                const float t = k / 4.0f, px = x - s * 0.85f + s * 1.7f * t, py = y - s * 0.2f + s * 0.6f * std::sin(t * kPi);
                flower(px, py, s * 0.25f, k % 2 ? c1 : trim, gem);
            }
            break;
        case WearShape::TailRibbons:
            for (int k = 0; k < 3; ++k) {
                const float oy = y - s * 0.5f + k * s * 0.45f;
                for (int j = 0; j < 4; ++j)
                    line(x - s * 0.8f + j * s * 0.4f, oy + (j % 2 ? s * 0.12f : -s * 0.12f), x - s * 0.4f + j * s * 0.4f,
                         oy + (j % 2 ? -s * 0.12f : s * 0.12f), 3.0f, k % 2 ? c1 : c0);
            }
            break;
        case WearShape::TailRing:
            ring(x, y, s * 0.65f, 6.0f, c0);
            ring(x, y, s * 0.65f, 2.0f, c1);
            break;
        case WearShape::StarCharm:
        case WearShape::SnowCharm:
            ring(x, y - s * 0.55f, s * 0.4f, 2.5f, c0);
            line(x, y - s * 0.15f, x, y + s * 0.2f, 1.5f, trim);
            if (a.shape == WearShape::StarCharm) {
                star(x, y + s * 0.5f, s * 0.42f, gem);
            } else {
                for (int k = 0; k < 3; ++k) {
                    const float ang = kPi * k / 3;
                    line(x - std::cos(ang) * s * 0.4f, y + s * 0.5f - std::sin(ang) * s * 0.4f, x + std::cos(ang) * s * 0.4f,
                         y + s * 0.5f + std::sin(ang) * s * 0.4f, 2.5f, gem);
                }
            }
            break;
        case WearShape::Tassel:
            ring(x, y - s * 0.6f, s * 0.35f, 2.5f, c1);
            disc(x, y - s * 0.1f, s * 0.16f, trim);
            tri(x - s * 0.1f, y, x + s * 0.1f, y, x + s * 0.4f, y + s * 0.9f, c0);
            tri(x - s * 0.1f, y, x + s * 0.4f, y + s * 0.9f, x - s * 0.4f, y + s * 0.9f, c0);
            break;
        case WearShape::Count: break;
    }
}

void drawDyeSwatch(int dye, float x, float y, float size) {
    const float r = size * 0.5f;
    if (dye <= 0) {  // its own colours: a split disc
        disc(x, y, r, theme::kShell);
        disc(x, y, r * 0.8f, theme::kAsh);
        rect(x - r * 0.8f, y - r * 0.08f, r * 1.6f, r * 0.16f, theme::kShell);
        return;
    }
    const DyeInfo& d = dyeInfo(dye);
    disc(x, y, r, theme::kDenPlum);
    disc(x, y, r * 0.88f, fromRgb(d.main));
    disc(x - r * 0.25f, y - r * 0.25f, r * 0.35f, fromRgb(d.light));  // a highlight in its light shade
}

}  // namespace ec
