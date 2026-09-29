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

constexpr const char* kTrace = "sdmc:/3ds/emberclutch/trace.txt";
constexpr const char* kPrev = "sdmc:/3ds/emberclutch/trace-prev.txt";
constexpr const char* kHangs = "sdmc:/3ds/emberclutch/hangs.txt";
constexpr int kLines = 64;
constexpr int kWidth = 176;
constexpr unsigned long kChecked = 3;  // frames of marks and GPU checkpoints after each change of view
constexpr u64 kBeatMs = 5000;          // a line this often in between
char g_ring[kLines][kWidth];
int g_next = 0, g_count = 0;
int g_pending = 0;  // lines at the ring's end not written yet
bool g_on = false;
bool g_live = true;  // marks kept: the start, then the first frames of each view
u64 g_start = 0, g_lastWrite = 0, g_lastBeat = 0;
unsigned long g_frames = 0;  // since the last beat
C3D_RenderTarget* g_target = nullptr;

constexpr int kMaxHangs = 8;
char g_hangs[kMaxHangs][32];
int g_hangCount = 0;

FILE* g_file = nullptr;
long g_written = 0;

const char* line(int ago) { return g_ring[(g_next - 1 - ago + 2 * kLines) % kLines]; }

// Appends the lines not written yet and flushes. The file starts over past 512 KB, with the
// ring's older lines first so the trail isn't lost.
void write() {
    if (!g_pending) return;
    if (!g_file || g_written > 512 * 1024) {
        if (g_file) std::fclose(g_file);
        g_file = std::fopen(kTrace, "wb");
        g_written = 0;
        if (!g_file) return;
        g_pending = g_count;
    }
    for (int ago = g_pending - 1; ago >= 0; --ago) g_written += std::fprintf(g_file, "%s\n", line(ago));
    std::fflush(g_file);
    g_pending = 0;
    g_lastWrite = osGetTime();
}

void add(const char* fmt, va_list args) {
    char text[112];
    std::vsnprintf(text, sizeof(text), fmt, args);
    const struct mallinfo mi = mallinfo();
    std::snprintf(g_ring[g_next], kWidth, "%7.3f %s  [lin %u heap %u]", (osGetTime() - g_start) / 1000.0, text,
                  static_cast<unsigned>(linearSpaceFree() / 1024), static_cast<unsigned>(mi.uordblks / 1024));
    g_next = (g_next + 1) % kLines;
    if (g_count < kLines) ++g_count;
    if (++g_pending >= kLines) write();  // (the ring is full: out before a line is lost)
}

// A line kept whatever the window (the start's news, the beat).
void note(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void note(const char* fmt, ...) {
    if (!g_on) return;
    va_list args;
    va_start(args, fmt);
    add(fmt, args);
    va_end(args);
}

void readHangs() {
    FILE* f = std::fopen(kHangs, "rb");
    if (!f) return;
    char buf[64];
    while (g_hangCount < kMaxHangs && std::fgets(buf, sizeof(buf), f)) {
        buf[std::strcspn(buf, "\r\n")] = 0;
        if (!buf[0]) continue;
        std::snprintf(g_hangs[g_hangCount++], sizeof(g_hangs[0]), "%.31s", buf);
    }
    std::fclose(f);
}

// The last session's trail: if its last GPU checkpoint was sent and never drawn, that part hung.
// Its end is enough (the trail's last lines are always the last thing written).
void findHang(char* part, std::size_t size) {
    part[0] = 0;
    FILE* f = std::fopen(kTrace, "rb");
    if (!f) return;
    static char tail[4096];
    std::fseek(f, 0, SEEK_END);
    const long end = std::ftell(f);
    std::fseek(f, end > static_cast<long>(sizeof(tail)) - 1 ? end - static_cast<long>(sizeof(tail)) + 1 : 0, SEEK_SET);
    const std::size_t got = std::fread(tail, 1, sizeof(tail) - 1, f);
    std::fclose(f);
    tail[got] = 0;
    const char* last = nullptr;
    for (const char* p = std::strstr(tail, " gpu: "); p; p = std::strstr(p + 1, " gpu: ")) last = p + 6;
    if (!last) return;
    const char* sent = std::strstr(last, " sent  [");
    const char* eol = std::strchr(last, '\n');
    if (!sent || (eol && sent > eol)) return;  // (the last one was drawn)
    std::snprintf(part, size, "%.*s", static_cast<int>(sent - last), last);
}

}  // namespace

void start() {
    readHangs();
    FILE* f = std::fopen("sdmc:/3ds/emberclutch/trace.on", "rb");
    g_on = f != nullptr;
    if (f) std::fclose(f);
    g_start = g_lastBeat = osGetTime();
    if (!g_on) return;
    char part[32];
    findHang(part, sizeof(part));
    if (part[0] && !hung(part) && g_hangCount < kMaxHangs) {  // the list grows by it
        std::snprintf(g_hangs[g_hangCount++], sizeof(g_hangs[0]), "%s", part);
        if (FILE* h = std::fopen(kHangs, "ab")) {
            std::fprintf(h, "%s\n", part);
            std::fclose(h);
        }
    }
    std::remove(kPrev);
    std::rename(kTrace, kPrev);  // (the session before, whole)
    note("trace on (0.9.3: marks and GPU checkpoints for %lu frames a view)", kChecked);
    if (part[0]) note("the last session froze in: %s", part);
    for (int i = 0; i < g_hangCount; ++i) note("drawn the safe way (hangs.txt): %s", g_hangs[i]);
    write();
}

void frame(unsigned long n, int view) {
    static int last = -1;
    static unsigned long from = 0;
    if (view != last) {
        last = view;
        from = n;
    }
    g_live = n < from + kChecked;
    ++g_frames;
    if (g_on && osGetTime() - g_lastBeat >= kBeatMs) {
        note("f%lu view %x: %lu frames in %.1f s", n, static_cast<unsigned>(view), g_frames,
             (osGetTime() - g_lastBeat) / 1000.0);
        g_lastBeat = osGetTime();
        g_frames = 0;
    }
}

bool on() { return g_on; }

void mark(const char* fmt, ...) {
    if (!g_on || !g_live) return;
    va_list args;
    va_start(args, fmt);
    add(fmt, args);
    va_end(args);
}

void sync() {
    if (g_on && (g_live || osGetTime() - g_lastWrite > kBeatMs)) write();
}

void target(C3D_RenderTarget_tag* t) { g_target = t; }

void gpu(const char* what) {
    if (!g_on || !g_live || !g_target) return;
    mark("gpu: %s sent", what);
    write();
    C2D_Flush();
    C3D_FrameEnd(0);    // (what's drawn so far goes to the GPU, and to the screen)
    C3D_FrameBegin(0);  // waits for the GPU to finish all of it
    mark("gpu: %s drawn", what);
    C3D_FrameDrawOn(g_target);  // drawing goes on where it was
}

bool hung(const char* part) {
    for (int i = 0; i < g_hangCount; ++i)
        if (std::strcmp(g_hangs[i], part) == 0) return true;
    return false;
}

}  // namespace ec::trace
