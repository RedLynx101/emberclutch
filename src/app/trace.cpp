#include "app/trace.hpp"

#include <3ds.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <malloc.h>

namespace ec::trace {
namespace {

constexpr int kLines = 48;
constexpr int kWidth = 176;
char g_ring[kLines][kWidth];
int g_next = 0, g_count = 0;
bool g_on = false;
u64 g_start = 0;

FILE* g_file = nullptr;
long g_written = 0;

// Appended as it comes and flushed (a whole rewrite a mark was too slow): the file starts over
// past 512 KB, the ring's lines first so the trail isn't lost.
void flush(const char* line) {
    if (!g_file || g_written > 512 * 1024) {
        if (g_file) std::fclose(g_file);
        g_file = std::fopen("sdmc:/3ds/emberclutch/trace.txt", "wb");
        g_written = 0;
        if (!g_file) return;
        for (int i = 0; i + 1 < g_count; ++i) {
            const char* old = g_ring[(g_next - g_count + i + kLines) % kLines];
            g_written += std::fprintf(g_file, "%s\n", old);
        }
    }
    g_written += std::fprintf(g_file, "%s\n", line);
    std::fflush(g_file);
}

}  // namespace

void start() {
    FILE* f = std::fopen("sdmc:/3ds/emberclutch/trace.on", "rb");
    g_on = f != nullptr;
    if (f) std::fclose(f);
    g_start = osGetTime();
    if (g_on) mark("trace on");
}

bool on() { return g_on; }

void mark(const char* fmt, ...) {
    if (!g_on) return;
    char text[96];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);
    const struct mallinfo mi = mallinfo();
    std::snprintf(g_ring[g_next], kWidth, "%7.3f %s  [lin %u heap %u]", (osGetTime() - g_start) / 1000.0, text,
                  static_cast<unsigned>(linearSpaceFree() / 1024), static_cast<unsigned>(mi.uordblks / 1024));
    const char* line = g_ring[g_next];
    g_next = (g_next + 1) % kLines;
    if (g_count < kLines) ++g_count;
    flush(line);
}

}  // namespace ec::trace
