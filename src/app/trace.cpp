#include "app/trace.hpp"

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <malloc.h>

namespace ec::trace {
namespace {

constexpr int kLines = 64;
constexpr int kWidth = 176;
constexpr unsigned long kWindow = 600;  // frames of marks after the start and each scene change
constexpr unsigned long kChecked = 3;   // frames of GPU checkpoints after each scene change
char g_ring[kLines][kWidth];
int g_next = 0, g_count = 0;
int g_pending = 0;  // lines at the ring's end not written yet
bool g_on = false;
bool g_live = true;       // marks kept (the window)
bool g_checking = false;  // GPU checkpoints (the first frames of a scene)
u64 g_start = 0, g_lastWrite = 0;
C3D_RenderTarget* g_target = nullptr;

FILE* g_file = nullptr;
long g_written = 0;

const char* line(int ago) { return g_ring[(g_next - 1 - ago + 2 * kLines) % kLines]; }

// Appends the lines not written yet and flushes. The file starts over past 512 KB, with the
// ring's older lines first so the trail isn't lost.
void write() {
    if (!g_pending) return;
    if (!g_file || g_written > 512 * 1024) {
        if (g_file) std::fclose(g_file);
        g_file = std::fopen("sdmc:/3ds/emberclutch/trace.txt", "wb");
        g_written = 0;
        if (!g_file) return;
        g_pending = g_count;
    }
    for (int ago = g_pending - 1; ago >= 0; --ago) g_written += std::fprintf(g_file, "%s\n", line(ago));
    std::fflush(g_file);
    g_pending = 0;
    g_lastWrite = osGetTime();
}

}  // namespace

void start() {
    FILE* f = std::fopen("sdmc:/3ds/emberclutch/trace.on", "rb");
    g_on = f != nullptr;
    if (f) std::fclose(f);
    g_start = osGetTime();
    if (g_on) {
        mark("trace on (0.9.2: GPU checkpoints for %lu frames a scene)", kChecked);
        write();
    }
}

void frame(unsigned long n, int scene) {
    static int last = -1;
    static unsigned long from = 0;
    if (scene != last) {
        last = scene;
        from = n;
    }
    g_live = n < from + kWindow;
    g_checking = g_on && n < from + kChecked;
}

bool on() { return g_on; }

void mark(const char* fmt, ...) {
    if (!g_on || !g_live) return;
    char text[112];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);
    const struct mallinfo mi = mallinfo();
    std::snprintf(g_ring[g_next], kWidth, "%7.3f %s  [lin %u heap %u]", (osGetTime() - g_start) / 1000.0, text,
                  static_cast<unsigned>(linearSpaceFree() / 1024), static_cast<unsigned>(mi.uordblks / 1024));
    g_next = (g_next + 1) % kLines;
    if (g_count < kLines) ++g_count;
    if (++g_pending >= kLines) write();  // (the ring is full: out before a line is lost)
}

void sync() {
    if (!g_on) return;
    if (g_checking || osGetTime() - g_lastWrite > 1000) write();
}

void target(C3D_RenderTarget_tag* t) { g_target = t; }

void gpu(const char* what) {
    if (!g_checking || !g_target) return;
    mark("gpu: %s sent", what);
    write();
    C2D_Flush();
    C3D_FrameEnd(0);    // (what's drawn so far goes to the GPU, and to the screen)
    C3D_FrameBegin(0);  // waits for the GPU to finish all of it
    mark("gpu: %s drawn", what);
    C3D_FrameDrawOn(g_target);  // drawing goes on where it was
}

}  // namespace ec::trace
