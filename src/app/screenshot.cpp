#include "app/screenshot.hpp"

#include <3ds.h>
#include <citro3d.h>
#include <sys/stat.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>

#include "app/audio.hpp"
#include "app/strings.hpp"

namespace ec::screenshot {
namespace {

constexpr const char* kDir = "sdmc:/3ds/emberclutch/screenshots";
constexpr int kH = 240, kTopW = 400, kBotW = 320;
constexpr int kW = kTopW, kImgH = 2 * kH;  // the top screen, then the bottom one centred under it
constexpr int kPixels = kW * kImgH * 3;     // 1200-byte rows: already a multiple of 4

constexpr const char* kSceneNames[] = {"Title",        "PickStarter", "Den",    "Map",   "Sanctuary",
                                       "Vault", "NestingStone", "Wanderings", "Market"};
static_assert(sizeof(kSceneNames) / sizeof(kSceneNames[0]) == static_cast<int>(SceneId::Count));

bool g_wanted = false;
u8* g_top = nullptr;
u8* g_bottom = nullptr;
char g_line[192] = {};  // the log line, with this frame's numbers
int g_next = 0;         // the next free shot number (found on the first shot)

// The SD card is slow: on the 3DS a picture took long enough to stall the game (Noah, run 2).
// The screens are copied at once; the file is written by a thread of its own while the game
// goes on, and the toast shows when it's done.
struct Job {
    std::vector<u8> file;  // the whole BMP
    char line[192] = {};
    int number = 0;
    bool ok = false;
};
Job g_job;
Thread g_thread = nullptr;
std::atomic<bool> g_finished{false};

bool exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

void shotPath(char* out, std::size_t cap, int n) { std::snprintf(out, cap, "%s/shot_%04d.bmp", kDir, n); }

// The framebuffers hold each screen turned a quarter (240 tall columns, bottom to top), BGR,
// like a BMP's pixels. The GPU wrote them: drop any stale cached lines before reading.
void copyScreen(u8* img, const u8* fb, int width, int x0, int y0) {
    GSPGPU_InvalidateDataCache(fb, width * kH * 3);
    for (int y = 0; y < kH; ++y) {
        u8* row = img + ((kImgH - 1 - (y0 + y)) * kW + x0) * 3;  // BMP rows run bottom to top
        for (int x = 0; x < width; ++x) std::memcpy(row + x * 3, fb + (x * kH + (kH - 1 - y)) * 3, 3);
    }
}

void buildBmp(std::vector<u8>& file) {
    file.assign(54 + kPixels, 0);
    u8* h = file.data();
    auto put32 = [&](int at, u32 v) { std::memcpy(h + at, &v, 4); };
    h[0] = 'B';
    h[1] = 'M';
    put32(2, 54 + kPixels);
    put32(10, 54);
    put32(14, 40);
    put32(18, kW);
    put32(22, kImgH);
    h[26] = 1;
    h[28] = 24;
    put32(34, kPixels);
    copyScreen(h + 54, g_top, kTopW, 0, 0);
    copyScreen(h + 54, g_bottom, kBotW, (kTopW - kBotW) / 2, kH);
}

// On the writing thread: the file (one write: row by row is slower still), then the log line.
void writeJob(void*) {
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/emberclutch", 0777);
    mkdir(kDir, 0777);
    char path[64];
    if (g_next == 0) {  // after the last one already on the card
        g_next = 1;
        while (g_next < 9999 && (shotPath(path, sizeof(path), g_next), exists(path))) ++g_next;
    }
    shotPath(path, sizeof(path), g_next);
    FILE* f = std::fopen(path, "wb");
    bool ok = f && std::fwrite(g_job.file.data(), 1, g_job.file.size(), f) == g_job.file.size();
    if (f) ok = std::fclose(f) == 0 && ok;
    if (ok) {
        char log[64];
        std::snprintf(log, sizeof(log), "%s/log.txt", kDir);
        if (FILE* l = std::fopen(log, "a")) {
            std::fprintf(l, "shot_%04d  %s\n", g_next, g_job.line);
            std::fclose(l);
        }
        g_job.number = g_next++;
    }
    g_job.ok = ok;
    g_finished = true;
}

// A picture is written (or not): the toast, and the 576 KB given back.
void announce(App& app) {
    g_finished = false;
    std::vector<u8>().swap(g_job.file);
    if (!g_job.ok) {
        showToast(app, str::kScreenshotFailed);
        audio::playSfx(audio::Sfx::Error);
        return;
    }
    char num[8];
    std::snprintf(num, sizeof(num), "%d", g_job.number);
    showToastf(app, str::kScreenshotSaved, num);
    audio::playSfx(audio::Sfx::Tap, 1.4f, 0.6f);  // a light click, like a shutter
}

// The last picture's thread, once it's done.
void collect(App& app) {
    if (!g_thread || !g_finished) return;
    threadJoin(g_thread, U64_MAX);
    threadFree(g_thread);
    g_thread = nullptr;
    announce(app);
}

}  // namespace

void request() {
    if (!g_thread) g_wanted = true;  // one at a time: Y while the last one is still writing does nothing
}

void beforeFrameEnd(const App& app) {
    if (!g_wanted) return;
    g_wanted = false;
    g_top = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, nullptr, nullptr);
    g_bottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, nullptr, nullptr);
    const std::time_t t = std::time(nullptr);
    char when[24];
    std::strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", std::gmtime(&t));  // the 3DS clock is local time
    std::snprintf(g_line, sizeof(g_line),
                  "%s  %-12s %4.1fms CPU %.1f GPU %.1f  TRI %lu+%lu  LIN %.1fMB  build %s %s", when,
                  kSceneNames[static_cast<int>(app.scene)], app.frameMs, C3D_GetProcessingTime(), C3D_GetDrawingTime(),
                  static_cast<unsigned long>(app.stats.tris - app.bottomTris),
                  static_cast<unsigned long>(app.bottomTris), linearSpaceFree() / 1048576.0f, __DATE__, __TIME__);
}

void afterFrameBegin(App& app) {
    collect(app);
    if (!g_top) return;
    buildBmp(g_job.file);
    std::memcpy(g_job.line, g_line, sizeof(g_line));
    g_top = g_bottom = nullptr;
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    // A little below the game's own priority, on the game's core: it writes while the game
    // waits for the screen each frame.
    g_thread = threadCreate(writeJob, nullptr, 32 * 1024, std::min<s32>(prio + 1, 0x3F), -2, false);
    if (!g_thread) {  // no thread to be had: write it here, the game waits
        writeJob(nullptr);
        announce(app);
    }
}

void finish() {
    if (!g_thread) return;
    threadJoin(g_thread, U64_MAX);  // a picture still being written when the game closes
    threadFree(g_thread);
    g_thread = nullptr;
}

}  // namespace ec::screenshot
