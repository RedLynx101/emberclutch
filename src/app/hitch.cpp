#include "app/hitch.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>

#include "app/perf.hpp"

namespace ec::hitch {
namespace {

constexpr int kMarks = 4, kKept = 16;
constexpr const char* kScenes[] = {"title", "starter", "den", "map", "sanctuary", "vault",
                                   "nesting stone", "wanderings", "market", "valley"};

struct Hitch {
    float ms = 0;
    int scene = 0;
    double at = 0;  // seconds into the session
    const char* marks[kMarks] = {};
    char perf[96] = {};
};

const char* g_marks[kMarks] = {};
int g_markCount = 0;
Hitch g_kept[kKept];
int g_total = 0;
double g_clock = 0;

}  // namespace

void mark(const char* what) {
    for (int i = 0; i < g_markCount; ++i)
        if (g_marks[i] == what) return;
    if (g_markCount < kMarks) g_marks[g_markCount++] = what;
}

void endFrame(float ms, int scene) {
    g_clock += ms / 1000.0;
    if (ms > kHitchMs && g_clock > 5.0) {  // (not the first seconds: the splash loads things)
        Hitch& h = g_kept[g_total % kKept];
        h = Hitch{};
        h.ms = ms;
        h.scene = scene;
        h.at = g_clock;
        for (int i = 0; i < g_markCount; ++i) h.marks[i] = g_marks[i];
        std::snprintf(h.perf, sizeof(h.perf), "%s", perf::line());
        ++g_total;
    }
    g_markCount = 0;
}

int count() { return g_total; }

void write() {
    if (g_total == 0) return;
    FILE* f = std::fopen("sdmc:/3ds/emberclutch/hitches.txt", "a");
    if (!f) return;
    const std::time_t now = std::time(nullptr);
    char date[32];
    std::strftime(date, sizeof(date), "%Y-%m-%d %H:%M", std::localtime(&now));
    std::fprintf(f, "session %s: %d long frame(s)\n", date, g_total);
    const int first = g_total > kKept ? g_total - kKept : 0;
    for (int n = first; n < g_total; ++n) {
        const Hitch& h = g_kept[n % kKept];
        const int scene = h.scene >= 0 && h.scene < static_cast<int>(sizeof(kScenes) / sizeof(kScenes[0])) ? h.scene : 0;
        std::fprintf(f, "  %7.1f s  %6.0f ms  %-13s  marks:", h.at, h.ms, kScenes[scene]);
        if (!h.marks[0]) std::fprintf(f, " none");
        for (const char* m : h.marks)
            if (m) std::fprintf(f, " [%s]", m);
        std::fprintf(f, "  (%s)\n", h.perf);
    }
    std::fclose(f);
}

}  // namespace ec::hitch
