#include "app/trace.hpp"

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

#include <cstdarg>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <malloc.h>

namespace ec::trace {
namespace {

constexpr const char* kTrace = "sdmc:/3ds/emberclutch/trace.txt";
constexpr const char* kPrev = "sdmc:/3ds/emberclutch/trace-prev.txt";
constexpr const char* kHangs = "sdmc:/3ds/emberclutch/hangs.txt";
constexpr int kLines = 64;
constexpr int kWidth = 208;
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

// The blip watch: the last three finished frames' samples and what drew them.
constexpr int kWatchPoints = 24;
const u8* g_watchFb = nullptr;
unsigned g_watchNext[3] = {};    // this frame's triangles, draws, scene (kept as it finishes)
float g_watch[3][kWatchPoints][3] = {};
unsigned g_watchInfo[3][3] = {};
int g_watchCount = 0, g_blips = 0;
int g_holes = 0, g_depthFrames = 0;  // valley frames with the ground's depth gone, of those looked at

// The depth fixes under trial (D113), their holes counted apart; what each valley frame did (its
// haze table made again, tiles built), for the holes; the probes after a hole.
constexpr int kTrialFrames = 10000, kModeRun = 60, kModes = 5;
int g_mode = 0, g_modeUsed = -1, g_modeDrawn = -1;  // the next frame's; the one drawing's; the last drawn's
int g_modeHoles[kModes] = {}, g_modeFrames[kModes] = {};
bool g_trialDone = false;
bool g_factLut = false, g_factLutDone = false;
int g_factBuilt = 0, g_factBuiltDone = 0;
int g_lutFrames = 0, g_lutHoles = 0, g_builtFrames = 0, g_builtHoles = 0;
char g_note[112] = {}, g_noteDone[112] = {};
bool g_probeNow = false;
int g_probes = 0, g_lastProbe = -1000, g_maps = 0;
C3D_RenderTarget* g_probeTop = nullptr;
// The commands the first tile's draw sent (D114): this frame's, the last finished frame's.
constexpr int kCmdWords = 600;
u32 g_cmd[kCmdWords], g_cmdDone[kCmdWords];
int g_cmdCount = 0, g_cmdDoneCount = 0, g_cmdDumps = 0;
u32* g_cmdBase = nullptr;
u32 g_cmdFrom = 0;
char g_tileLine[172] = {};
int g_tileAt = 0;

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
    char text[172];
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
    note("trace on (0.9.8: marks and GPU checkpoints for %lu frames a view; the blip and depth watches, the ground trial)", kChecked);
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
        note("f%lu view %x: %lu frames in %.1f s, blips %d, holes %d of %d (by fix %d/%d %d/%d %d/%d %d/%d %d/%d; built %d/%d)",
             n, static_cast<unsigned>(view), g_frames, (osGetTime() - g_lastBeat) / 1000.0, g_blips, g_holes, g_depthFrames,
             g_modeHoles[0], g_modeFrames[0], g_modeHoles[1], g_modeFrames[1], g_modeHoles[2], g_modeFrames[2], g_modeHoles[3],
             g_modeFrames[3], g_modeHoles[4], g_modeFrames[4], g_builtHoles, g_builtFrames);
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
    g_target->used = false;  // (half a picture isn't shown: run 21's flashes as a screen began)
    C3D_FrameEnd(0);    // (what's drawn so far goes to the GPU)
    C3D_FrameBegin(0);  // waits for the GPU to finish all of it
    mark("gpu: %s drawn", what);
    C3D_FrameDrawOn(g_target);  // drawing goes on where it was
}

void watchBeforeFrameEnd(unsigned tris, unsigned draws, int scene) {
    if (!g_on) return;
    g_watchFb = reinterpret_cast<const u8*>(gfxGetFramebuffer(GFX_TOP, GFX_LEFT, nullptr, nullptr));
    g_watchNext[0] = tris, g_watchNext[1] = draws, g_watchNext[2] = static_cast<unsigned>(scene);
    g_modeDrawn = g_modeUsed;  // (what this frame drew with, for its depth read after it)
    g_modeUsed = -1;
    g_factLutDone = g_factLut, g_factBuiltDone = g_factBuilt;
    g_factLut = false, g_factBuilt = 0;
    std::memcpy(g_noteDone, g_note, sizeof(g_note));
    g_note[0] = 0;
    std::memcpy(g_cmdDone, g_cmd, sizeof(u32) * static_cast<std::size_t>(g_cmdCount));
    g_cmdDoneCount = g_cmdCount;
    g_cmdCount = 0;
    if (g_probeNow && g_tileAt > 0) note("  tiles:%s", g_tileLine);
    g_tileAt = 0;
    g_tileLine[0] = 0;
    g_probeNow = false;
}

void watchAfterFrameBegin() {
    if (!g_on || !g_watchFb) return;
    constexpr int kW = 400, kH = 240;
    GSPGPU_InvalidateDataCache(g_watchFb, kW * kH * 3);
    std::memmove(g_watch[1], g_watch[0], sizeof(g_watch[0]) * 2);
    std::memmove(g_watchInfo[1], g_watchInfo[0], sizeof(g_watchInfo[0]) * 2);
    for (int i = 0; i < kWatchPoints; ++i) {  // eight across, three down the lower half
        const int x = 20 + (i % 8) * 50, y = 150 + (i / 8) * 35;
        const u8* p = g_watchFb + (x * kH + (kH - 1 - y)) * 3;  // (the screen lies on its side, BGR)
        g_watch[0][i][0] = p[2], g_watch[0][i][1] = p[1], g_watch[0][i][2] = p[0];
    }
    std::memcpy(g_watchInfo[0], g_watchNext, sizeof(g_watchNext));
    g_watchFb = nullptr;
    if (++g_watchCount < 3) return;
    auto apart = [](const float (*a)[3], const float (*b)[3]) {
        float d = 0;
        for (int i = 0; i < kWatchPoints; ++i)
            for (int c = 0; c < 3; ++c) d += std::fabs(a[i][c] - b[i][c]);
        return d / (kWatchPoints * 3);
    };
    const float before = apart(g_watch[1], g_watch[2]), after = apart(g_watch[0], g_watch[1]),
                across = apart(g_watch[0], g_watch[2]);
    if (before > 25.0f && after > 25.0f && across < before * 0.4f) {
        ++g_blips;
        float was[3] = {}, blip[3] = {};
        for (int i = 0; i < kWatchPoints; ++i)
            for (int c = 0; c < 3; ++c) was[c] += g_watch[2][i][c] / kWatchPoints, blip[c] += g_watch[1][i][c] / kWatchPoints;
        note("blip %d: scene %u, %u triangles %u draws; lower half %.0f %.0f %.0f -> %.0f %.0f %.0f (apart %.0f, then %.0f)", g_blips,
             g_watchInfo[1][2], g_watchInfo[1][0], g_watchInfo[1][1], was[0], was[1], was[2], blip[0], blip[1], blip[2], before,
             across);
    }
}

namespace {

// How empty each half of a screen's depth is (the buffer in 8 x 8 tiles, the screen on its side,
// 240 across and 400 down: the first half of each row of tiles is the screen's lower half).
bool depthEmpty(C3D_RenderTarget* t, float& lower, float& upper) {
    const C3D_FrameBuf& fb = t->frameBuf;
    if (!fb.depthBuf || fb.depthFmt != GPU_RB_DEPTH16) return false;  // (citro2d's screens: 16 bits)
    const int tw = fb.width / 8, th = fb.height / 8;
    const u16* d = static_cast<const u16*>(fb.depthBuf);
    static s32 invalidated = 1;
    const s32 r = GSPGPU_InvalidateDataCache(fb.depthBuf, static_cast<u32>(fb.width) * fb.height * 2);
    if (r != invalidated) note("depth watch: invalidate %08lX", static_cast<unsigned long>(invalidated = r));
    int zero[2] = {}, seen[2] = {};
    for (int ty = 0; ty < th; ty += 2)
        for (int tx = 0; tx < tw; ++tx) {
            const int half = tx < tw / 2 ? 0 : 1;
            ++seen[half];
            zero[half] += d[(ty * tw + tx) * 64 + 27] == 0;  // (a pixel inside each tile)
        }
    lower = zero[0] / static_cast<float>(seen[0] ? seen[0] : 1);
    upper = zero[1] / static_cast<float>(seen[1] ? seen[1] : 1);
    return true;
}

// The lower half's mean colour (RGBA8 in tiles, each pixel's bytes A, B, G, R).
void colourMean(C3D_RenderTarget* t, int rgb[3]) {
    const C3D_FrameBuf& fb = t->frameBuf;
    rgb[0] = rgb[1] = rgb[2] = -1;
    if (!fb.colorBuf || fb.colorFmt != GPU_RB_RGBA8) return;
    const int tw = fb.width / 8, th = fb.height / 8;
    const u8* c = static_cast<const u8*>(fb.colorBuf);
    GSPGPU_InvalidateDataCache(fb.colorBuf, static_cast<u32>(fb.width) * fb.height * 4);
    long sum[3] = {};
    int n = 0;
    for (int ty = 0; ty < th; ty += 2)
        for (int tx = 0; tx < tw / 2; ++tx, ++n) {
            const u8* p = c + ((ty * tw + tx) * 64 + 27) * 4;
            sum[0] += p[3], sum[1] += p[2], sum[2] += p[1];
        }
    for (int k = 0; k < 3; ++k) rgb[k] = static_cast<int>(sum[k] / (n ? n : 1));
}

// A map of which parts of the top screen have depth ('#') and which don't ('.'), 40 x 12 over the
// screen as seen (left to right perhaps mirrored), a line each.
void depthMap(C3D_RenderTarget* t, const char* what) {
    const C3D_FrameBuf& fb = t->frameBuf;
    if (!fb.depthBuf || fb.depthFmt != GPU_RB_DEPTH16) return;
    const int tw = fb.width / 8;
    const u16* d = static_cast<const u16*>(fb.depthBuf);
    note("depth map (%s):", what);
    for (int row = 0; row < 12; ++row) {
        char line[41];
        for (int col = 0; col < 40; ++col) {
            const int sy = row * 20 + 10, sx = col * 10 + 5;  // the screen's point
            const int x = 239 - sy, y = sx;                   // in the buffer
            const int m = (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
            line[col] = d[((y / 8) * tw + x / 8) * 64 + m] ? '#' : '.';
        }
        line[40] = 0;
        note("  %s", line);
    }
}

// A command list, decoded: each command's register and value (all its values up to four, else its
// first and how many), a handful to a line.
void dumpCommands(const u32* w, int n, const char* what) {
    note("commands (%s): %d words", what, n);
    char line[172];
    int at = 0;
    for (int i = 0; i + 1 < n;) {
        const u32 h = w[i + 1];
        const int extra = static_cast<int>((h >> 20) & 0xFF), reg = static_cast<int>(h & 0xFFFF);
        const int mask = static_cast<int>((h >> 16) & 0xF);
        char item[96];
        int k = std::snprintf(item, sizeof(item), " %03x%s%s", reg, (h >> 31) ? "+" : "", mask != 0xF ? "&" : "");
        if (mask != 0xF) k += std::snprintf(item + k, sizeof(item) - static_cast<std::size_t>(k), "%x", mask);
        if (extra < 4) {
            for (int j = 0; j <= extra && i + 2 + j - 1 < n; ++j)
                k += std::snprintf(item + k, sizeof(item) - static_cast<std::size_t>(k), "%s%lx", j ? "," : "=",
                                   static_cast<unsigned long>(j == 0 ? w[i] : w[i + 1 + j]));
        } else {
            k += std::snprintf(item + k, sizeof(item) - static_cast<std::size_t>(k), "=%lx x%d", static_cast<unsigned long>(w[i]), extra + 1);
        }
        if (at + k > 160) {
            note(" %s", line);
            at = 0;
        }
        std::memcpy(line + at, item, static_cast<std::size_t>(k) + 1);
        at += k;
        i += 2 + extra + (extra & 1);
    }
    if (at) note(" %s", line);
}

}  // namespace

void watchDepth(C3D_RenderTarget_tag* top, int valleyScene) {
    if (!g_on || !top || static_cast<int>(g_watchNext[2]) != valleyScene) return;
    g_probeTop = top;
    float lower = 0, upper = 0;
    if (!depthEmpty(top, lower, upper)) return;
    const bool hole = lower > 0.6f;
    const int mode = g_modeDrawn;
    ++g_depthFrames;
    if (mode >= 0 && mode < kModes) {
        ++g_modeFrames[mode];
        g_modeHoles[mode] += hole;
    }
    if (g_factLutDone) ++g_lutFrames, g_lutHoles += hole;
    if (g_factBuiltDone) ++g_builtFrames, g_builtHoles += hole;
    if (g_depthFrames <= 3 || (g_depthFrames % 1500) == 0)
        note("depth watch: lower half %.0f%% empty, upper %.0f%% (fix %d)", lower * 100.0f, upper * 100.0f, mode);
    if (g_depthFrames == 150 && !hole) depthMap(top, "a whole frame, for comparison");
    if (g_cmdDoneCount > 0 && ((hole && g_cmdDumps < 3) || (!hole && g_depthFrames >= 160 && g_cmdDumps == 0))) {
        char what[48];
        std::snprintf(what, sizeof(what), "%s, fix %d", hole ? "a hole" : "a whole frame", mode);
        dumpCommands(g_cmdDone, g_cmdDoneCount, what);
        ++g_cmdDumps;
    }
    if (hole) {
        if (++g_holes <= 40)
            note("depth hole %d (fix %d): %u triangles %u draws; lower half %.0f%% empty, upper %.0f%%; %s", g_holes, mode,
                 g_watchNext[0], g_watchNext[1], lower * 100.0f, upper * 100.0f, g_noteDone);
        if (g_maps < 4) {
            char what[32];
            std::snprintf(what, sizeof(what), "hole %d, fix %d", g_holes, mode);
            depthMap(top, what);
            ++g_maps;
        }
    }
    // The next frame probed (its depth read after each part): after a hole a few times, spaced out,
    // and once early on as a reference.
    if ((hole && g_probes < 12 && g_depthFrames - g_lastProbe >= 40) || g_depthFrames == 200) {
        g_probeNow = true;
        g_lastProbe = g_depthFrames;
        ++g_probes;
        note("probe %d%s: the next frame's depth after each part", g_probes, hole ? "" : " (a reference, no hole)");
    }
    // The fix for the frame about to be drawn: the trial's turn, then the one that did best.
    if (g_depthFrames < kTrialFrames) {
        g_mode = (g_depthFrames / kModeRun) % kModes;
    } else if (!g_trialDone) {
        g_trialDone = true;
        auto rate = [](int m) { return g_modeFrames[m] >= 300 ? g_modeHoles[m] / static_cast<float>(g_modeFrames[m]) : 1.0f; };
        g_mode = 0;
        for (int m = 1; m < kModes; ++m)
            if (rate(m) < rate(g_mode) * 0.8f) g_mode = m;  // (a clear gain only)
        note("depth trial done: holes by fix %d/%d %d/%d %d/%d %d/%d %d/%d; fix %d from now on", g_modeHoles[0], g_modeFrames[0],
             g_modeHoles[1], g_modeFrames[1], g_modeHoles[2], g_modeFrames[2], g_modeHoles[3], g_modeFrames[3], g_modeHoles[4],
             g_modeFrames[4], g_mode);
    }
}

int g_heldFix = -1;  // (scripted runs: one fix held)

int depthMode() {
    if (g_heldFix >= 0) return g_modeUsed = g_heldFix;
    if (!g_on) return 0;
    g_modeUsed = g_mode;
    return g_mode;
}

void holdFix(int fix) { g_heldFix = fix < kModes ? fix : -1; }

void commands(bool begin) {
    if (!g_on || g_probeNow || g_live) return;  // (not while checkpoints split the frame's commands)
    u32* base = nullptr;
    u32 size = 0, offset = 0;
    GPUCMD_GetBuffer(&base, &size, &offset);
    if (begin) {
        g_cmdBase = base, g_cmdFrom = offset;
        g_cmdCount = 0;
        return;
    }
    if (base != g_cmdBase || offset < g_cmdFrom) return;
    g_cmdCount = static_cast<int>(offset - g_cmdFrom > static_cast<u32>(kCmdWords) ? kCmdWords : offset - g_cmdFrom);
    std::memcpy(g_cmd, base + g_cmdFrom, sizeof(u32) * static_cast<std::size_t>(g_cmdCount));
}

void tileProbe(int i, int tx, int ty, int lod, int count) {
    if (!g_on || !g_probeNow || !g_target || !g_probeTop) return;
    C2D_Flush();
    g_target->used = false;
    C3D_FrameEnd(0);
    C3D_FrameBegin(0);
    float lower = 0, upper = 0;
    depthEmpty(g_probeTop, lower, upper);
    const int filled = static_cast<int>(100.0f - (lower + upper) * 50.0f + 0.5f);
    if (g_tileAt > 130) {
        note("  tiles:%s", g_tileLine);
        g_tileAt = 0;
    }
    g_tileAt += std::snprintf(g_tileLine + g_tileAt, sizeof(g_tileLine) - static_cast<std::size_t>(g_tileAt), " %d(%d,%d L%d n%d):%d%%", i,
                              tx, ty, lod, count, filled);
    C3D_FrameDrawOn(g_target);
}

void frameFacts(bool lutRebuilt, int built) {
    g_factLut = g_factLut || lutRebuilt;
    g_factBuilt += built;
}

void frameNote(const char* fmt, ...) {
    if (!g_on) return;
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(g_note, sizeof(g_note), fmt, args);
    va_end(args);
}

void checkpoint(const char* part) {
    if (g_live) {
        gpu(part);
        return;
    }
    if (!g_on || !g_probeNow || !g_target) return;
    C2D_Flush();
    g_target->used = false;
    C3D_FrameEnd(0);
    C3D_FrameBegin(0);
    float lower = 0, upper = 0;
    int rgb[3] = {-1, -1, -1};
    if (g_probeTop) {  // (the top screen's, whichever is being drawn)
        depthEmpty(g_probeTop, lower, upper);
        colourMean(g_probeTop, rgb);
    }
    note("  after %s (fix %d): lower half %.0f%% empty, upper %.0f%%; its colour %d %d %d", part, g_modeUsed, lower * 100.0f,
         upper * 100.0f, rgb[0], rgb[1], rgb[2]);
    C3D_FrameDrawOn(g_target);
}

bool hung(const char* part) {
    for (int i = 0; i < g_hangCount; ++i)
        if (std::strcmp(g_hangs[i], part) == 0) return true;
    return false;
}

}  // namespace ec::trace
