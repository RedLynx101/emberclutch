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

// citro2d's buffer for a frame, both screens (run 21: a busy valley filled it; its shapes then
// skip themselves, but its text doesn't check and wrote on past the end, over the GPU's other
// data: the map's fog and the panels gone, things flickering out, a freeze). An object is a
// glyph, a rectangle, a circle, a line or an image.
constexpr int kTwoDObjects = 16383;  // the most whose four vertices each a 16-bit index reaches
struct TwoDUse {
    int used = 0;     // objects the last frame drew (-1: citro2d isn't the build mirrored)
    int skipped = 0;  // texts left out for want of room
};
int twoDLeft();          // room left this frame, in objects (a large number if unknown)
void twoDEndFrame();     // just before C3D_FrameEnd (the buffer empties then)
TwoDUse twoDLastFrame();

// The frame's GPU data out of the CPU's cache before the GPU reads it (run 21: on the 3DS some
// busy valley frames drew with stale commands: the map's fog and the panels gone or black in
// the same frames as things flickered out; the emulator has no cache). citro3d flushes the whole
// linear heap as the frame ends; the command list and citro2d's buffers are flushed here too,
// and the heap flush is tried once a second to see whether it holds on the hardware.
void gpuFrameBegun();    // just after C3D_FrameBegin
void gpuFrameFlush();    // just before C3D_FrameEnd
s32 gpuHeapFlushResult();  // the last try's result (0: fine)

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
// A line that may need two (1.0's pass for clipped words: a rumour shrunk to 68% to fit its row): as
// text() at `scale` while that shrinks it no further than 87%, else split at the space nearest its
// middle into two lines at `small`, `gap` apart, centred on where the one line's middle would be.
// Returns true if it took two.
bool textFit(App& app, const char* s, float x, float y, float scale, float small, float gap, u32 color, u32 flags,
             float maxWidth);
// Dev builds (1.0's last pass for clipped words): the screen being drawn, by its width, so text running
// off it, or squeezed under three quarters of its size to fit, is logged on an autotest's shot frames.
void textScreen(float width);
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
