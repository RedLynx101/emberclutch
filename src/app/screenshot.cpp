#include "app/screenshot.hpp"

#include <3ds.h>
#include <citro3d.h>
#include <sys/stat.h>

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

constexpr const char* kSceneNames[] = {"Title",        "PickStarter", "Den",    "Map",   "Sanctuary",
                                       "Vault", "NestingStone", "Wanderings", "Market"};
static_assert(sizeof(kSceneNames) / sizeof(kSceneNames[0]) == static_cast<int>(SceneId::Count));

bool g_wanted = false;
u8* g_top = nullptr;
u8* g_bottom = nullptr;
char g_line[192] = {};  // the log line, with this frame's numbers
int g_next = 0;         // the next free shot number (found on the first shot)

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

bool save(const char* path) {
    const int size = kW * kImgH * 3;  // 1200-byte rows: already a multiple of 4
    std::vector<u8> file(54 + size, 0);  // one write: row by row is slow on the SD card
    u8* h = file.data();
    auto put32 = [&](int at, u32 v) { std::memcpy(h + at, &v, 4); };
    h[0] = 'B';
    h[1] = 'M';
    put32(2, 54 + size);
    put32(10, 54);
    put32(14, 40);
    put32(18, kW);
    put32(22, kImgH);
    h[26] = 1;
    h[28] = 24;
    put32(34, size);
    copyScreen(h + 54, g_top, kTopW, 0, 0);
    copyScreen(h + 54, g_bottom, kBotW, (kTopW - kBotW) / 2, kH);
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    const bool ok = std::fwrite(file.data(), 1, file.size(), f) == file.size();
    return std::fclose(f) == 0 && ok;
}

}  // namespace

void request() { g_wanted = true; }

void beforeFrameEnd(const App& app) {
    if (!g_wanted) return;
    g_wanted = false;
    g_top = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, nullptr, nullptr);
    g_bottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, nullptr, nullptr);
    const std::time_t t = std::time(nullptr);
    char when[24];
    std::strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", std::gmtime(&t));  // the 3DS clock is local time
    std::snprintf(g_line, sizeof(g_line),
                  "%s  %-12s %4.1fms CPU %.1f GPU %.1f  TRI %lu+%lu  LIN %.1fMB APP %.1fMB  build %s %s", when,
                  kSceneNames[static_cast<int>(app.scene)], app.frameMs, C3D_GetProcessingTime(), C3D_GetDrawingTime(),
                  static_cast<unsigned long>(app.stats.tris - app.bottomTris),
                  static_cast<unsigned long>(app.bottomTris), linearSpaceFree() / 1048576.0f,
                  osGetMemRegionFree(MEMREGION_APPLICATION) / 1048576.0f, __DATE__, __TIME__);
}

void afterFrameBegin(App& app) {
    if (!g_top) return;
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/emberclutch", 0777);
    mkdir(kDir, 0777);
    char path[64];
    if (g_next == 0) {  // after the last one already on the card
        g_next = 1;
        while (g_next < 9999 && (shotPath(path, sizeof(path), g_next), exists(path))) ++g_next;
    }
    shotPath(path, sizeof(path), g_next);
    const bool ok = save(path);
    g_top = g_bottom = nullptr;
    if (!ok) {
        showToast(app, str::kScreenshotFailed);
        audio::playSfx(audio::Sfx::Error);
        return;
    }
    char log[64];
    std::snprintf(log, sizeof(log), "%s/log.txt", kDir);
    if (FILE* f = std::fopen(log, "a")) {
        std::fprintf(f, "shot_%04d  %s\n", g_next, g_line);
        std::fclose(f);
    }
    char num[8];
    std::snprintf(num, sizeof(num), "%d", g_next++);
    showToastf(app, str::kScreenshotSaved, num);
    audio::playSfx(audio::Sfx::Tap, 1.4f, 0.6f);  // a light click, like a shutter
}

}  // namespace ec::screenshot
