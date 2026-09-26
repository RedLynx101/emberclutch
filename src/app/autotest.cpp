#include "app/autotest.hpp"

#include <3ds.h>
#include <sys/stat.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "core/clock.hpp"
#include "core/valley.hpp"
#include "core/world.hpp"

namespace ec::autotest {
namespace {

constexpr const char* kScript = "sdmc:/3ds/emberclutch/autotest.txt";
constexpr const char* kShots = "sdmc:/3ds/emberclutch/shots";

enum class Op : u8 { Wait, Tap, Hold, Drag, Key, KeyHold, Pad, Shot, ShotIn, Name, Skip, Overlay, Splash, Travel, Light, View, Quit };

struct Cmd {
    Op op = Op::Wait;
    float a[7] = {};
    u32 key = 0;
    std::string text;
};

std::vector<Cmd> g_cmds;
std::size_t g_at = 0;
float g_time = 0;  // seconds into the current command
int g_frame = 0;   // frames into it
bool g_active = false;
bool g_wasTouching = false;
float g_lastX = 0, g_lastY = 0;
std::string g_names;  // queued names, '\n'-separated
std::string g_pending, g_armed;
std::string g_later;       // shotin: a shot partway into the next command
float g_laterAt = -1;
u8* g_fbTop = nullptr;
u8* g_fbBottom = nullptr;

u32 keyNamed(const char* s) {
    static const struct {
        const char* name;
        u32 key;
    } kKeys[] = {{"A", KEY_A},       {"B", KEY_B},         {"X", KEY_X},       {"Y", KEY_Y},
                 {"L", KEY_L},       {"R", KEY_R},         {"START", KEY_START}, {"SELECT", KEY_SELECT},
                 {"UP", KEY_DUP},    {"DOWN", KEY_DDOWN},  {"LEFT", KEY_DLEFT}, {"RIGHT", KEY_DRIGHT}};
    u32 keys = 0;  // "UP+B": held together
    char one[16];
    while (*s) {
        std::size_t n = std::strcspn(s, "+");
        if (n >= sizeof(one)) n = sizeof(one) - 1;
        std::memcpy(one, s, n);
        one[n] = 0;
        for (const auto& k : kKeys)
            if (std::strcmp(one, k.name) == 0) keys |= k.key;
        s += std::strcspn(s, "+");
        if (*s == '+') ++s;
    }
    return keys;
}

bool parse(const char* line, Cmd& c) {
    char word[16] = {}, rest[96] = {};
    if (std::sscanf(line, " %15s %95[^\r\n]", word, rest) < 1 || word[0] == '#') return false;
    const std::string w = word;
    auto nums = [&](int n) {
        const char* p = rest;
        for (int i = 0; i < n; ++i) {
            char* end = nullptr;
            c.a[i] = std::strtof(p, &end);
            p = end;
        }
    };
    if (w == "wait") { c.op = Op::Wait; nums(1); }
    else if (w == "tap") { c.op = Op::Tap; nums(2); }
    else if (w == "hold") { c.op = Op::Hold; nums(3); }
    else if (w == "pad") { c.op = Op::Pad; nums(3); }  // pad <x> <y> <seconds>: the circle pad held (-1..1, up +y)
    else if (w == "drag") { c.op = Op::Drag; nums(6); }
    else if (w == "key") { c.op = Op::Key; c.key = keyNamed(rest); }
    else if (w == "keyhold") {
        char k[16] = {};
        std::sscanf(rest, "%15s %f", k, &c.a[0]);
        c.op = Op::KeyHold;
        c.key = keyNamed(k);
    }
    else if (w == "shot") { c.op = Op::Shot; c.text = rest; }
    else if (w == "shotin") {
        char name[64] = {};
        std::sscanf(rest, "%f %63s", &c.a[0], name);
        c.op = Op::ShotIn;
        c.text = name;
    }
    else if (w == "name") { c.op = Op::Name; c.text = rest; }
    else if (w == "skip") { c.op = Op::Skip; nums(1); }
    else if (w == "overlay") { c.op = Op::Overlay; c.a[0] = std::strcmp(rest, "on") == 0; }
    else if (w == "splash") { c.op = Op::Splash; }
    else if (w == "travel") { c.op = Op::Travel; nums(1); }
    else if (w == "light") { c.op = Op::Light; }
    else if (w == "view") { c.op = Op::View; nums(7); }
    else if (w == "quit") { c.op = Op::Quit; }
    else return false;
    return true;
}

// The framebuffers hold each screen turned a quarter (240 tall columns, bottom to top), BGR:
// exactly a BMP's row order once read column by column.
void saveBmp(const char* path, const u8* fb, int width) {
    if (!fb) return;
    FILE* f = std::fopen(path, "wb");
    if (!f) return;
    const int height = 240, row = width * 3, size = 54 + row * height;
    u8 h[54] = {'B', 'M'};
    auto put32 = [&](int at, u32 v) { std::memcpy(h + at, &v, 4); };
    put32(2, size);
    put32(10, 54);
    put32(14, 40);
    put32(18, width);
    put32(22, height);
    h[26] = 1;
    h[28] = 24;
    put32(34, row * height);
    std::fwrite(h, 1, 54, f);
    std::vector<u8> line(row);
    for (int y = height - 1; y >= 0; --y) {  // BMP rows run bottom to top
        for (int x = 0; x < width; ++x) {
            const u8* p = fb + (x * height + (height - 1 - y)) * 3;
            line[x * 3] = p[0];
            line[x * 3 + 1] = p[1];
            line[x * 3 + 2] = p[2];
        }
        std::fwrite(line.data(), 1, row, f);
    }
    std::fclose(f);
}

}  // namespace

bool start(App& app) {
#if EC_DEV
    FILE* f = std::fopen(kScript, "r");
    if (!f) return false;
    char line[128];
    while (std::fgets(line, sizeof(line), f)) {
        Cmd c;
        if (parse(line, c)) g_cmds.push_back(c);
    }
    std::fclose(f);
    mkdir(kShots, 0777);
    g_active = !g_cmds.empty();
    (void)app;
    return g_active;
#else
    (void)app;
    return false;
#endif
}

bool active() { return g_active; }

Input next(App& app) {
    Input in;
    bool touching = false;
    float x = g_lastX, y = g_lastY;
    bool ran = false;  // a command used this frame (its clock moves on)
    while (g_active && g_at < g_cmds.size()) {
        const Cmd& c = g_cmds[g_at];
        bool done = false;
        switch (c.op) {
            case Op::Wait: done = g_time >= c.a[0]; break;
            case Op::Tap:
                touching = g_frame < 3;
                x = c.a[0];
                y = c.a[1];
                done = g_frame >= 4;
                break;
            case Op::Hold:
                touching = g_time < c.a[2];
                x = c.a[0];
                y = c.a[1];
                done = !touching && !g_wasTouching;
                break;
            case Op::Drag: {
                const float k = c.a[4] > 0 ? std::min(1.0f, g_time / c.a[4]) : 1.0f;
                touching = g_time < c.a[4] + c.a[5] + 0.05f;
                x = c.a[0] + (c.a[2] - c.a[0]) * k;
                y = c.a[1] + (c.a[3] - c.a[1]) * k;
                done = !touching && !g_wasTouching;
                break;
            }
            case Op::Key:
                if (g_frame == 0) in.down = in.held = c.key;
                done = g_frame >= 2;
                break;
            case Op::Pad:
                if (g_time < c.a[2]) {
                    in.padX = c.a[0];
                    in.padY = c.a[1];
                }
                done = g_time >= c.a[2];
                break;
            case Op::KeyHold:
                in.held = g_time < c.a[0] ? c.key : 0;
                if (g_frame == 0) in.down = c.key;
                done = g_time >= c.a[0] + 0.05f;
                break;
            case Op::Shot: g_pending = c.text; done = true; break;
            case Op::ShotIn: g_later = c.text; g_laterAt = c.a[0]; done = true; break;
            case Op::Name: g_names += c.text + "\n"; done = true; break;
            case Op::Skip: app.game.devOffset += static_cast<s64>(c.a[0] * kHour); done = true; break;
            case Op::Overlay: app.overlay = c.a[0] != 0; done = true; break;
            case Op::Splash: app.splash = kSplashSeconds; done = true; break;
            case Op::Travel: app.autoTravel = static_cast<int>(c.a[0]); done = true; break;
            case Op::View:
                for (int k = 0; k < 7; ++k) app.autoView[k] = c.a[k];
                done = true;
                break;
            case Op::Light:
                for (int p = 0; p < kPlaceCount; ++p)
                    if (world::placeInfo(p).lantern) {
                        world::findPlace(app.game, p);
                        world::lightLantern(app.game, p);
                    }
                done = true;
                break;
            case Op::Quit: app.quit = true; done = true; break;
        }
        if (!done) {
            ran = true;
            if (g_laterAt >= 0 && g_time >= g_laterAt) {  // shotin's moment
                g_pending = g_later;
                g_laterAt = -1;
            }
            break;
        }
        const bool shot = c.op == Op::Shot;
        ++g_at;
        g_time = 0;
        g_frame = 0;
        if (in.down || touching || shot) break;  // this frame belongs to the finished command (a shot: as it was)
    }
    if (ran) {
        g_time += app.dt;
        ++g_frame;
    }
    in.touching = touching;
    in.tapped = touching && !g_wasTouching;
    in.released = !touching && g_wasTouching;
    in.tx = x;
    in.ty = y;
    in.rx = g_lastX;
    in.ry = g_lastY;
    if (touching) {
        g_lastX = x;
        g_lastY = y;
    }
    g_wasTouching = touching;
    if (g_at >= g_cmds.size() && !app.quit) app.quit = true;  // the script is over
    return in;
}

void beforeFrameEnd() {
    if (g_pending.empty()) return;
    g_fbTop = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, nullptr, nullptr);
    g_fbBottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, nullptr, nullptr);
    g_armed = g_pending;
    g_pending.clear();
}

void afterFrameBegin() {
    if (g_armed.empty()) return;
    char path[128];
    std::snprintf(path, sizeof(path), "%s/%s_top.bmp", kShots, g_armed.c_str());
    saveBmp(path, g_fbTop, 400);
    std::snprintf(path, sizeof(path), "%s/%s_bottom.bmp", kShots, g_armed.c_str());
    saveBmp(path, g_fbBottom, 320);
    g_armed.clear();
}

bool shooting() { return g_active && !g_pending.empty(); }

void log(const char* fmt, ...) {
    if (!g_active) return;
    char path[96];
    std::snprintf(path, sizeof(path), "%s/log.txt", kShots);
    FILE* f = std::fopen(path, "a");
    if (!f) return;
    std::fprintf(f, "[%s] ", g_pending.c_str());
    va_list args;
    va_start(args, fmt);
    std::vfprintf(f, fmt, args);
    va_end(args);
    std::fputs("\n", f);
    std::fclose(f);
}

bool typedName(char* out, std::size_t cap) {
    const std::size_t nl = g_names.find('\n');
    if (nl == std::string::npos) return false;
    std::snprintf(out, cap, "%s", g_names.substr(0, nl).c_str());
    g_names.erase(0, nl + 1);
    return true;
}

void finish() {
    if (!g_active) return;
    if (!g_armed.empty()) {  // a picture from the very last frame: let its transfer finish
        gspWaitForVBlank();
        gspWaitForVBlank();
        afterFrameBegin();
    }
    char path[96];
    std::snprintf(path, sizeof(path), "%s/done.txt", kShots);
    if (FILE* f = std::fopen(path, "w")) {
        std::fputs("done\n", f);
        std::fclose(f);
    }
}

}  // namespace ec::autotest
