#include "app/autotest.hpp"
#include "app/trace.hpp"

#include <3ds.h>
#include <sys/stat.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "app/audio.hpp"
#include "app/cove.hpp"  // Driftwood Cove (workstream C)
#include "app/glade.hpp"  // the pageant
#include "app/battle_feature.hpp"  // 1.0 battles (workstream B)
#include "app/wildlife.hpp"  // the valley's critters (workstream L)
#include "app/roamers_feature.hpp"  // roaming trainers (workstream D)
#include "app/scenes.hpp"
#include "core/story.hpp"
#include "core/trainer.hpp"
#include "core/clock.hpp"
#include "core/valley.hpp"
#include "core/wanderings.hpp"
#include "core/world.hpp"

namespace ec::autotest {
namespace {

constexpr const char* kScript = "sdmc:/3ds/emberclutch/autotest.txt";
constexpr const char* kShots = "sdmc:/3ds/emberclutch/shots";

enum class Op : u8 { Wait, Tap, Hold, Drag, Key, KeyHold, Pad, Shot, ShotIn, Name, Skip, Overlay, Splash, Travel, Light, View,
                     Creator, Wander, Festival, Goto, Challenge, Autoplay, Cups, Valley, Hour, Quit, TrialFix,
                     Sound,  // (sounds, 1.0: bed, sfx, sfxcheck)
                     Open, Xp, Record, Needs, Track, Tips, Gleam, Hoard, Wear,  // (U: Open .. Wear)
                     Energy, Cove,  // (workstream C)
                     Pageant, Ground,
                     Battle /* 1.0 battles (workstream B) */, Critters /* workstream L */,
                     Roamer /* roaming trainers (workstream D) */ };  // the pageant's own commands (app/glade.hpp pageantCommand)

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
float g_bedHold[static_cast<int>(audio::Bed::Count)] = {};  // sounds (1.0): beds held every frame by `bed`
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

[[maybe_unused]] bool parse(const char* line, Cmd& c) {  // (the player build runs no scripts)
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
    else if (w == "hour") { c.op = Op::Hour; nums(1); }  // (the places: daylight whatever the PC's clock)
    else if (w == "overlay") { c.op = Op::Overlay; c.a[0] = std::strcmp(rest, "on") == 0; }
    else if (w == "splash") { c.op = Op::Splash; }
    else if (w == "travel") { c.op = Op::Travel; nums(1); }
    else if (w == "light") { c.op = Op::Light; }
    else if (w == "trialfix") { c.op = Op::TrialFix; nums(1); }  // trialfix <n>: the flicker trial's fix held (-1: its turns)
    else if (w == "view") { c.op = Op::View; nums(7); }
    else if (w == "creator") { c.op = Op::Creator; }
    else if (w == "wander") { c.op = Op::Wander; nums(1); }
    else if (w == "festival") { c.op = Op::Festival; }
    else if (w == "goto") {  // (x y, and optionally a point to face: fx fy)
        c.op = Op::Goto;
        nums(4);
        c.a[4] = std::strchr(rest, ' ') && std::strchr(std::strchr(rest, ' ') + 1, ' ') ? 1.0f : 0.0f;
    }
    else if (w == "challenge") { c.op = Op::Challenge; nums(2); }
    else if (w == "autoplay") { c.op = Op::Autoplay; c.a[0] = std::strcmp(rest, "on") == 0; }
    else if (w == "cups") { c.op = Op::Cups; nums(3); }
    else if (w == "valley") { c.op = Op::Valley; }
    else if (w == "ground") { c.op = Op::Ground; nums(1); }  // the look lab: ground 0 smooth, 1 faceted, 2 faceted + texture  // out into the valley at the den's door (X did it before run 19)
    else if (w == "energy") { c.op = Op::Energy; nums(1); }  // energy <0..100>: every dragon's (the challenges, workstream C)
    else if (w == "cove") { c.op = Op::Cove; nums(1); }      // cove <0 talk to Tam, 1 fish, 2 a shell> (workstream C)
    else if (w == "quit") { c.op = Op::Quit; }
    // Sounds (1.0): `bed <index> <level>` holds a bed at a level every frame (0 lets it go),
    // `sfx <index>` plays a sound effect, `sfxcheck` logs the effects with no file of their own.
    else if (w == "bed" || w == "sfx" || w == "sfxcheck") { c.op = Op::Sound; c.text = w; nums(2); }
    // U (1.0 interface): straight into a scene, a dragon's experience and record, its needs, the
    // tracked goal, the tips, Gleam and the hoard.
    else if (w == "open") { c.op = Op::Open; c.text = rest; }
    else if (w == "xp") { c.op = Op::Xp; nums(1); }
    else if (w == "record") { c.op = Op::Record; }
    else if (w == "needs") { c.op = Op::Needs; nums(5); }
    else if (w == "track") { c.op = Op::Track; nums(2); }
    else if (w == "tips") { c.op = Op::Tips; c.a[0] = std::strcmp(rest, "reset") == 0; }
    else if (w == "gleam") { c.op = Op::Gleam; nums(1); }
    else if (w == "hoard") { c.op = Op::Hoard; nums(1); }
    else if (w == "wear") { c.op = Op::Wear; nums(5); }
    else if (w == "pg") { c.op = Op::Pageant; c.text = rest; }  // the pageant: pg give / wear / show ...
    else if (w == "battle") { c.op = Op::Battle; c.text = rest; }  // 1.0 battles (workstream B)
    else if (w == "critters") { c.op = Op::Critters; c.text = rest; }  // the valley's critters (app/wildlife.hpp command)
    else if (w == "roamer") { c.op = Op::Roamer; c.text = rest; }  // roaming trainers (workstream D)
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
            case Op::Hour: {  // skip ahead to that hour of the day (the places' shots)
                const s64 into = nowLocal(app) % kDay, want = static_cast<s64>(c.a[0] * kHour);
                app.game.devOffset += ((want - into) % kDay + kDay) % kDay;
                done = true;
                break;
            }
            case Op::Overlay: app.overlay = c.a[0] != 0; done = true; break;
            case Op::Splash: app.splash = kSplashSeconds; done = true; break;
            case Op::Travel: app.autoTravel = static_cast<int>(c.a[0]); done = true; break;
            case Op::Creator: openCreator(app, app.scene); done = true; break;
            case Op::Valley: openMap(app); done = true; break;
            case Op::Ground: r3d::setGroundLook(static_cast<int>(c.a[0])); done = true; break;
            case Op::Goto:
                app.autoGoto[0] = c.a[0];
                app.autoGoto[1] = c.a[1];
                app.autoGoto[2] = 1;
                app.autoGoto[3] = c.a[2];
                app.autoGoto[4] = c.a[3];
                app.autoGoto[5] = c.a[4];
                done = true;
                break;
            case Op::Festival: {  // the Lantern Festival's eve: the main story's other quests done, every lantern but the arena's lit
                const s64 now = nowLocal(app);
                for (int q : {story::kQKeepersApprentice, story::kQMarketDay, story::kQHilltop, story::kQMeadow, story::kQColdHeights,
                              story::kQWings, story::kQTrailhead})
                    story::finishQuest(app.game, q, now);
                for (int p = 0; p < kPlaceCount; ++p) {
                    world::findPlace(app.game, p);
                    if (world::placeInfo(p).lantern && p != kPlaceArena) world::lightLantern(app.game, p);
                }
                story::startQuest(app.game, story::kQLanternFestival);
                story::update(app.game, now);
                done = true;
                break;
            }
            case Op::Wander:  // the dragon cared for sets off (if no one's out), and that many steps are walked
                if (wandererIndex(app.game) < 0) setOff(app.game, app.careIndex, stepCount(app), nowLocal(app));
                app.devSteps += static_cast<u32>(c.a[0]);
                done = true;
                break;
            case Op::View:
                for (int k = 0; k < 7; ++k) app.autoView[k] = c.a[k];
                done = true;
                break;
            case Op::TrialFix:
                trace::holdFix(static_cast<int>(c.a[0]));
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
            case Op::Challenge:  // (from the valley: it lends the challenges its landscape)
                openChallengeCup(app, static_cast<int>(c.a[0]), static_cast<int>(c.a[1]));
                done = true;
                break;
            case Op::Autoplay:
                setChallengeAutoplay(c.a[0] != 0);
                cove::setAutoplay(c.a[0] != 0);  // (Driftwood Cove fishes by itself too)
                battleCommand(app, c.a[0] != 0 ? "auto on" : "auto off");  // 1.0 battles (workstream B)
                done = true;
                break;
            case Op::Battle: battleCommand(app, c.text.c_str()); done = true; break;  // 1.0 battles (workstream B)
            case Op::Critters: wildlife::command(app, c.text.c_str()); done = true; break;  // (workstream L)
            case Op::Roamer: roamerCommand(app, c.text.c_str()); done = true; break;  // (workstream D)
            case Op::Cups:  // each challenge's highest cup won (its ribbons with it)
                app.game.world.ribbons = 0;
                for (int k = 0; k < kChallenges; ++k) {
                    app.game.world.cups[k] = static_cast<u8>(std::clamp(static_cast<int>(c.a[k]), 0, kCups));
                    for (int cup = 0; cup < app.game.world.cups[k]; ++cup) app.game.world.ribbons |= static_cast<u16>(1u << (k * kCups + cup));
                }
                done = true;
                break;
            case Op::Cove: cove::autotest(app, static_cast<int>(c.a[0])); done = true; break;
            case Op::Energy:
                for (int i = 0; i < app.game.dragonCount; ++i) app.game.dragons[i].needs.energy = std::clamp(c.a[0], 0.0f, 100.0f);
                done = true;
                break;
            case Op::Quit: app.quit = true; done = true; break;
            case Op::Sound: {  // (sounds, 1.0)
                const int i = static_cast<int>(c.a[0]);
                if (c.text == "bed" && i >= 0 && i < static_cast<int>(audio::Bed::Count)) g_bedHold[i] = c.a[1];
                if (c.text == "sfx" && i >= 0 && i < static_cast<int>(audio::Sfx::Count))
                    audio::playSfx(static_cast<audio::Sfx>(i));
                if (c.text == "sfxcheck") {
                    std::string missing;
                    for (int k = 0; k < static_cast<int>(audio::Sfx::Count); ++k)
                        if (!audio::has(static_cast<audio::Sfx>(k))) missing += " " + std::to_string(k);
                    log("sfxcheck: audio %s; with no file of their own (a stand-in plays):%s", audio::ok() ? "ok" : "off",
                        missing.empty() ? " none" : missing.c_str());
                }
                done = true;
                break;
            }
            case Op::Open:  // U: market, wander, sanctuary, vault, den, valley, stone
                if (c.text.rfind("market", 0) == 0) app.scene = SceneId::Market;
                else if (c.text.rfind("stone", 0) == 0) app.scene = SceneId::NestingStone;
                else if (c.text.rfind("wander", 0) == 0) app.scene = SceneId::Wanderings;
                else if (c.text.rfind("valley", 0) == 0) openValley(app);
                else if (c.text.rfind("sanctuary", 0) == 0) app.scene = SceneId::Sanctuary;
                else if (c.text.rfind("vault", 0) == 0) app.scene = SceneId::Vault;
                else app.scene = SceneId::Den;
                done = true;
                break;
            case Op::Xp: trainer::gainXp(activeDragon(app), static_cast<u32>(c.a[0])); done = true; break;
            case Op::Record: {  // U: a well-travelled dragon's record, for the profile's pages
                Dragon& d = activeDragon(app);
                d.battleTitle = 3;
                d.showTitle = 2;
                d.battleWins = 14;
                d.showWins = 6;
                d.wildWins = 23;
                d.frostDeepest = 12;
                d.ribbons = 0xB5;
                for (int k = 0; k < kDragonStats; ++k) d.trained[k] = static_cast<u8>(2 + k * 4);
                for (int ch = 0; ch < kChallenges; ++ch)
                    for (int cup = 1; cup <= 3 - ch; ++cup) trainer::recordCup(d, ch, cup);
                trainer::gainXp(d, 1650);
                done = true;
                break;
            }
            case Op::Needs: {
                Needs& n = activeDragon(app).needs;
                n.belly = c.a[0];
                n.clean = c.a[1];
                n.play = c.a[2];
                n.love = c.a[3];
                n.energy = c.a[4];
                done = true;
                break;
            }
            case Op::Track: trainer::track(app.game, static_cast<Tracked>(static_cast<int>(c.a[0])), static_cast<int>(c.a[1])); done = true; break;
            case Op::Tips: app.game.progress.tips = c.a[0] != 0 ? 0u : 0xFFFFFFFFu; done = true; break;
            case Op::Gleam: app.game.gleam = static_cast<u32>(c.a[0]); done = true; break;
            case Op::Wear: {  // what the dragon cared for wears (core/accessories ids, 255 none) and its dye
                Dragon& d = activeDragon(app);
                for (int k = 0; k < kWearSlots; ++k) d.wear[k] = static_cast<u8>(c.a[k]);
                d.dye = static_cast<u8>(c.a[4]);
                done = true;
                break;
            }
            case Op::Hoard:
                for (int k = 0; k < kTrinkets; ++k) app.game.hoard[k] = static_cast<u16>(c.a[0]);
                done = true;
                break;
            case Op::Pageant: pageantCommand(app, c.text.c_str()); done = true; break;
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
    for (int b = 0; b < static_cast<int>(audio::Bed::Count); ++b)  // (sounds, 1.0)
        if (g_bedHold[b] > 0) audio::setBed(static_cast<audio::Bed>(b), g_bedHold[b]);
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
