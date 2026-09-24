// citro2d drawing helpers in the Emberclutch theme (eggshell panels, ember gauges, glows).
#pragma once

#include "app/app.hpp"
#include "core/types.hpp"

namespace ec {

struct Rect {
    float x, y, w, h;
    bool contains(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

u32 withAlpha(u32 c, float a);
u32 fromRgb(Rgb c, u8 a = 255);

void text(App& app, const char* s, float x, float y, float scale, u32 color, u32 flags = C2D_AlignCenter);
void verticalGradient(float x, float y, float w, float h, u32 top, u32 bottom);
void glow(float x, float y, float radius, u32 color, float strength);
void heart(float cx, float cy, float size, u32 color);
void egg(float cx, float cy, float w, float h, Rgb shell, Rgb glowC, float glowAmt);
void embers(float t, float w);

void panel(const Rect& r, u32 color);
bool button(App& app, const Rect& r, const char* label, const Input& in);
void gauge(App& app, float x, float y, const char* label, float value);

// Mood -> heartglow brightness (0..1) with its pulse.
float heartglowLevel(const Dragon& d, float t);

// 2D placeholder dragon until the 3D renderer (WP4) takes over.
void dragonPlaceholder(const Dragon& d, float cx, float groundY, float scale, float t);

}  // namespace ec
