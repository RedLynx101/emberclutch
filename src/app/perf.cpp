#include "app/perf.hpp"

#include <3ds.h>

#include <cstdio>

namespace ec::perf {
namespace {

constexpr const char* kNames[Count] = {"upd", "aud", "pose", "sub", "room", "top", "bot"};

u64 g_ticks[Count] = {};  // this frame so far
float g_ms[Count] = {};   // shown: smoothed over the last frames
Section g_current = Count;
char g_line[96] = {};

float toMs(u64 ticks) { return static_cast<float>(ticks) / (SYSCLOCK_ARM11 / 1000.0f); }

}  // namespace

void frameStart() {
    for (int s = 0; s < Count; ++s) {
        g_ms[s] = g_ms[s] * 0.9f + toMs(g_ticks[s]) * 0.1f;
        g_ticks[s] = 0;
    }
}

Scope::Scope(Section s) : section_(s), outer_(g_current), start_(svcGetSystemTick()) {
    g_current = s;
}

Scope::~Scope() {
    const u64 spent = svcGetSystemTick() - start_;
    g_ticks[section_] += spent;
    if (outer_ != Count) g_ticks[outer_] -= spent;  // the outer section counts its own time only
    g_current = outer_;
}

float ms(Section s) { return g_ms[s]; }

const char* line() {
    int n = 0;
    for (int s = 0; s < Count && n < static_cast<int>(sizeof(g_line)); ++s)
        n += std::snprintf(g_line + n, sizeof(g_line) - n, "%s%s %.1f", s ? " " : "", kNames[s], g_ms[s]);
    return g_line;
}

}  // namespace ec::perf
