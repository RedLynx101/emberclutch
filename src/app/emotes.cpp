#include "app/emotes.hpp"

#include <citro2d.h>

#include <cmath>

#include "app/audio.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"

namespace ec::emote {
namespace {

using story::Feel;

// The pop: in from nothing past full size and back (a quarter second), then a gentle bob.
float popScale(float t) {
    if (t < 0.0f) return 0.0f;
    if (t < 0.16f) return 1.25f * (t / 0.16f);
    if (t < 0.26f) return 1.25f - 0.25f * ((t - 0.16f) / 0.1f);
    return 1.0f;
}

void teardrop(float x, float y, float r, u32 c) {  // a drop, its point up
    C2D_DrawCircleSolid(x, y, 0.5f, r, c);
    C2D_DrawTriangle(x - r * 0.92f, y - r * 0.3f, c, x + r * 0.92f, y - r * 0.3f, c, x, y - r * 2.2f, c, 0.5f);
    C2D_DrawCircleSolid(x - r * 0.35f, y - r * 0.25f, 0.5f, r * 0.28f, withAlpha(0xFFFFFFFF, 0.6f));
}

void sparkle(float x, float y, float r, u32 c) {  // a four-pointed star
    C2D_DrawTriangle(x, y - r, c, x - r * 0.22f, y, c, x + r * 0.22f, y, c, 0.5f);
    C2D_DrawTriangle(x, y + r, c, x - r * 0.22f, y, c, x + r * 0.22f, y, c, 0.5f);
    C2D_DrawTriangle(x - r, y, c, x, y - r * 0.22f, c, x, y + r * 0.22f, c, 0.5f);
    C2D_DrawTriangle(x + r, y, c, x, y - r * 0.22f, c, x, y + r * 0.22f, c, 0.5f);
}

void noteGlyph(float x, float y, float s, u32 c) {  // an eighth note
    C2D_DrawEllipseSolid(x - s * 0.34f, y - s * 0.2f, 0.5f, s * 0.62f, s * 0.44f, c);
    C2D_DrawRectSolid(x + s * 0.2f, y - s * 1.35f, 0.5f, s * 0.13f, s * 1.2f, c);
    C2D_DrawTriangle(x + s * 0.33f, y - s * 1.35f, c, x + s * 0.8f, y - s * 0.95f, c, x + s * 0.33f, y - s * 1.0f, c, 0.5f);
}

void stroke(float x0, float y0, float x1, float y1, float w, u32 c) { C2D_DrawLine(x0, y0, c, x1, y1, c, w, 0.5f); }

}  // namespace

bool hasIcon(Feel f) { return f != Feel::Calm && f != Feel::Cool && f != Feel::Wistful; }

void draw(App& app, Feel f, float x, float y, float size, float t, float alpha) {
    if (!hasIcon(f) || alpha <= 0.01f) return;
    const float s = size * popScale(t) * 0.5f;  // (half: the icon's radius)
    if (s <= 0.5f) return;
    const float bob = t > 0.26f ? std::sin((t - 0.26f) * 5.0f) * size * 0.05f : 0.0f;
    y += bob;
    const u32 gold = withAlpha(theme::kClutchGold, alpha), white = withAlpha(0xFFFFFFFF, alpha);
    const u32 ink = withAlpha(theme::kDenPlum, alpha), red = withAlpha(theme::rgba(226, 64, 70), alpha);
    const u32 blue = withAlpha(theme::rgba(120, 190, 250), alpha), grey = withAlpha(theme::rgba(120, 124, 150), alpha);
    switch (f) {
        case Feel::Happy:
        case Feel::Laugh:
            noteGlyph(x - s * 0.45f, y + s * 0.4f, s * 0.8f, gold);
            noteGlyph(x + s * 0.55f, y - s * 0.05f + (f == Feel::Laugh ? std::sin(t * 18) * s * 0.12f : 0), s * 0.62f, gold);
            break;
        case Feel::Excited:
        case Feel::Proud:
            sparkle(x, y, s * 0.9f, gold);
            sparkle(x + s * 0.8f, y - s * 0.6f, s * 0.45f, white);
            sparkle(x - s * 0.8f, y + s * 0.2f, s * 0.35f, white);
            break;
        case Feel::Surprised:
        case Feel::Shock: {
            const float shake = f == Feel::Shock ? std::sin(t * 60) * s * 0.08f : 0.0f;
            const int marks = f == Feel::Shock ? 2 : 1;
            for (int k = 0; k < marks; ++k) {
                const float mx = x + shake + (marks == 2 ? (k ? s * 0.32f : -s * 0.32f) : 0.0f);
                C2D_DrawTriangle(mx - s * 0.2f, y - s, red, mx + s * 0.2f, y - s, red, mx, y + s * 0.35f, red, 0.5f);
                C2D_DrawCircleSolid(mx, y + s * 0.72f, 0.5f, s * 0.16f, red);
            }
            break;
        }
        case Feel::Sad: {  // a cloud of gloom, and a little rain
            C2D_DrawCircleSolid(x - s * 0.4f, y, 0.5f, s * 0.45f, grey);
            C2D_DrawCircleSolid(x + s * 0.35f, y - s * 0.05f, 0.5f, s * 0.5f, grey);
            C2D_DrawCircleSolid(x, y - s * 0.35f, 0.5f, s * 0.48f, grey);
            for (int k = 0; k < 3; ++k) {
                const float rx = x - s * 0.5f + s * 0.5f * k, fall = std::fmod(t * 1.5f + k * 0.33f, 1.0f) * s * 0.6f;
                stroke(rx, y + s * 0.5f + fall, rx - s * 0.1f, y + s * 0.75f + fall, 1.5f, blue);
            }
            break;
        }
        case Feel::Crying:
            teardrop(x - s * 0.5f, y + s * 0.3f + std::fmod(t, 0.8f) * s * 0.6f, s * 0.3f, blue);
            teardrop(x + s * 0.5f, y + s * 0.3f + std::fmod(t + 0.4f, 0.8f) * s * 0.6f, s * 0.3f, blue);
            break;
        case Feel::Angry: {  // the vein mark: four bent strokes round a gap
            const float g = s * 0.2f, l = s * 0.75f, w = std::fmax(2.0f, s * 0.22f);
            stroke(x - g, y - g, x - g, y - l, w, red);
            stroke(x - g, y - g, x - l, y - g, w, red);
            stroke(x + g, y - g, x + g, y - l, w, red);
            stroke(x + g, y - g, x + l, y - g, w, red);
            stroke(x - g, y + g, x - g, y + l, w, red);
            stroke(x - g, y + g, x - l, y + g, w, red);
            stroke(x + g, y + g, x + g, y + l, w, red);
            stroke(x + g, y + g, x + l, y + g, w, red);
            break;
        }
        case Feel::Huff:  // a puff of steam drifting off
            for (int k = 0; k < 3; ++k) {
                const float u = std::fmod(t * 0.9f + k * 0.33f, 1.0f);
                C2D_DrawCircleSolid(x + s * (0.2f + u), y - s * (0.2f + 0.4f * u), 0.5f, s * (0.25f + 0.3f * u),
                                    withAlpha(0xFFFFFFFF, alpha * (1.0f - u) * 0.85f));
            }
            break;
        case Feel::Worried: teardrop(x + s * 0.4f, y + s * 0.2f, s * 0.35f, blue); break;
        case Feel::Scared:
            teardrop(x + s * 0.5f + std::sin(t * 40) * 1.5f, y + s * 0.2f, s * 0.32f, blue);
            teardrop(x - s * 0.45f + std::sin(t * 40 + 1) * 1.5f, y - s * 0.1f, s * 0.26f, blue);
            break;
        case Feel::Sleepy:
            for (int k = 0; k < 3; ++k) {
                const float u = std::fmod(t * 0.5f + k * 0.33f, 1.0f);
                text(app, k == 1 ? "Z" : "z", x - s * 0.4f + s * 0.5f * k, y + s * 0.6f - s * 1.4f * u, 0.35f + 0.25f * u * s / 12.0f,
                     withAlpha(theme::kShell, alpha * (1.0f - u * 0.8f)));
            }
            break;
        case Feel::Love: heart(x, y, s * (1.0f + 0.1f * std::sin(t * 8.0f)), red); break;
        case Feel::Shy: text(app, "...", x, y - s * 0.3f, 0.5f * s / 10.0f, withAlpha(theme::kShell, alpha)); break;
        case Feel::Thinking: text(app, "?", x, y - s * 0.7f, 0.9f * s / 10.0f, gold, C2D_AlignCenter, 0.0f, Face::Title); break;
        case Feel::Dizzy:
            for (int k = 0; k < 3; ++k) {
                const float a = t * 5.0f + k * 2.094f;
                sparkle(x + std::cos(a) * s * 0.8f, y + std::sin(a) * s * 0.35f, s * 0.3f, gold);
            }
            break;
        default: break;
    }
    (void)ink;
}

void play(Feel f) {
    using audio::Sfx;
    Sfx s = Sfx::Count;
    switch (f) {
        case Feel::Happy: s = Sfx::EmoteHappy; break;
        case Feel::Laugh: s = Sfx::EmoteLaugh; break;
        case Feel::Excited: s = Sfx::EmoteExcited; break;
        case Feel::Surprised: s = Sfx::EmoteSurprised; break;
        case Feel::Shock: s = Sfx::EmoteShock; break;
        case Feel::Sad: s = Sfx::EmoteSad; break;
        case Feel::Crying: s = Sfx::EmoteCrying; break;
        case Feel::Angry: s = Sfx::EmoteAngry; break;
        case Feel::Huff: s = Sfx::EmoteHuff; break;
        case Feel::Worried: s = Sfx::EmoteWorried; break;
        case Feel::Scared: s = Sfx::EmoteScared; break;
        case Feel::Sleepy: s = Sfx::EmoteSleepy; break;
        case Feel::Love: s = Sfx::EmoteLove; break;
        case Feel::Proud: s = Sfx::EmoteProud; break;
        case Feel::Cool: s = Sfx::EmoteCool; break;
        case Feel::Shy: s = Sfx::EmoteShy; break;
        case Feel::Thinking: s = Sfx::EmoteThinking; break;
        case Feel::Wistful: s = Sfx::EmoteWistful; break;
        case Feel::Dizzy: s = Sfx::EmoteDizzy; break;
        default: break;
    }
    if (s != Sfx::Count) audio::playSfx(s, 1.0f, 0.75f);
}

Voice voiceOf(Feel f) {
    switch (f) {
        case Feel::Happy: return {1.08f, 1.05f, 0.0f, 0.8f};
        case Feel::Laugh: return {1.14f, 1.15f, 0.03f, 0.85f};
        case Feel::Excited: return {1.16f, 1.25f, 0.0f, 0.9f};
        case Feel::Surprised: return {1.15f, 1.1f, 0.0f, 0.85f};
        case Feel::Shock: return {1.22f, 1.3f, 0.05f, 0.95f};
        case Feel::Sad: return {0.88f, 0.8f, 0.0f, 0.65f};
        case Feel::Crying: return {0.95f, 0.8f, 0.07f, 0.7f};
        case Feel::Angry: return {0.86f, 1.2f, 0.0f, 0.95f};
        case Feel::Huff: return {0.92f, 1.1f, 0.0f, 0.85f};
        case Feel::Worried: return {1.02f, 1.0f, 0.03f, 0.65f};
        case Feel::Scared: return {1.1f, 1.2f, 0.08f, 0.7f};
        case Feel::Sleepy: return {0.9f, 0.7f, 0.0f, 0.6f};
        case Feel::Love: return {1.1f, 0.95f, 0.02f, 0.8f};
        case Feel::Proud: return {1.0f, 1.0f, 0.0f, 0.9f};
        case Feel::Cool: return {0.9f, 0.85f, 0.0f, 0.75f};
        case Feel::Shy: return {1.06f, 0.85f, 0.0f, 0.55f};
        case Feel::Thinking: return {0.96f, 0.85f, 0.0f, 0.7f};
        case Feel::Wistful: return {0.94f, 0.8f, 0.0f, 0.7f};
        case Feel::Dizzy: return {1.0f, 0.9f, 0.1f, 0.75f};
        default: return {};
    }
}

int portraitFrame(Feel f) {
    switch (f) {
        case Feel::Happy: case Feel::Laugh: case Feel::Excited: case Feel::Love: case Feel::Proud: return 1;
        case Feel::Sad: case Feel::Crying: case Feel::Worried: case Feel::Scared: return 2;
        case Feel::Angry: case Feel::Huff: return 3;
        case Feel::Surprised: case Feel::Shock: case Feel::Dizzy: return 4;
        default: return 0;
    }
}

}  // namespace ec::emote
