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

// Fonts (WP10, D33): Nunito for the interface, Cinzel Decorative for titles, loaded from
// romfs:/fonts (the system font stands in if they're missing). Each is scaled to the system
// font's line height, so a text scale means the same size whichever font draws it.
enum class Face : u8 { Ui, Title };
void loadFonts();  // after C2D_Init and the first text buffer
void freeFonts();

// Draws s with its top at y (x: its centre, or left edge with C2D_AlignLeft). With maxWidth
// > 0 it shrinks to fit that width.
void text(App& app, const char* s, float x, float y, float scale, u32 color, u32 flags = C2D_AlignCenter,
          float maxWidth = 0, Face face = Face::Ui);
// Centred on (cx, cy) both ways, shrunk to maxWidth if given.
void textCentered(App& app, const char* s, float cx, float cy, float scale, u32 color, float maxWidth = 0,
                  Face face = Face::Ui);
float textWidth(App& app, const char* s, float scale, Face face = Face::Ui);
void verticalGradient(float x, float y, float w, float h, u32 top, u32 bottom);
void glow(float x, float y, float radius, u32 color, float strength);
void heart(float cx, float cy, float size, u32 color);
void egg(float cx, float cy, float w, float h, Rgb shell, Rgb glowC, float glowAmt);
void embers(float t, float w);

void panel(const Rect& r, u32 color);
bool button(App& app, const Rect& r, const char* label, const Input& in, u32 color = 0);  // 0: the shell colour

// Over the top screen, drawn by main.cpp after the scene: the toast (a pill that fades in and
// out) and, just after a save, a little egg with a turning ring in the corner.
void drawToast(App& app);
void drawSaveIcon(App& app);
void gauge(App& app, float x, float y, const char* label, float value);

// Mood -> heartglow brightness (0..1) with its pulse.
float heartglowLevel(const Dragon& d, float t);

// 2D placeholder dragon until the 3D renderer (WP4) takes over.
void dragonPlaceholder(const Dragon& d, float cx, float groundY, float scale, float t);

}  // namespace ec
