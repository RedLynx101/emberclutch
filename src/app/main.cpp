// Emberclutch — Phase 0 skeleton.
// A themed citro2d prototype that runs the real core simulation on hardware:
// pick a starter egg, rub it warm, watch it hatch, and care for the hatchling.
// The dragon is a 2D placeholder until the Phase 1 citro3d pipeline lands.
#include <3ds.h>
#include <citro2d.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#include "app/theme.hpp"
#include "core/clock.hpp"
#include "core/dragon.hpp"
#include "core/genetics.hpp"

using namespace ec;

namespace {

constexpr float kTopW = 400, kBotW = 320, kH = 240;
constexpr s64 kNtpToUnix = 2208988800LL;  // osGetTime() counts from 1900-01-01
constexpr const char* kSaveDir = "sdmc:/3ds/emberclutch";
constexpr const char* kSavePath = "sdmc:/3ds/emberclutch/dev-save.bin";

enum class Scene { Title, PickStarter, Den };

// Dev-only save: a raw struct dump. Replaced by the versioned A/B format in Phase 1.
struct DevSave {
    char magic[4] = {'E', 'M', 'B', 'd'};
    u32 version = 1;
    u32 dragonSize = sizeof(Dragon);
    s64 lastSim = 0;
    s64 devOffset = 0;
    u8 hasDragon = 0;
    Dragon dragon{};
};

struct App {
    Scene scene = Scene::Title;
    C2D_TextBuf textBuf = nullptr;
    DevSave save{};
    Rng rng{1};
    float t = 0;  // seconds since boot, for animation
    int starterHover = 0;
    float lastTouchX = -1, lastTouchY = -1;
    float petCooldown = 0;
    const char* toast = nullptr;
    float toastTime = 0;
};

s64 nowLocal(const App& app) {
    return static_cast<s64>(osGetTime() / 1000) - kNtpToUnix + app.save.devOffset;
}

bool loadSave(DevSave& out) {
    FILE* f = std::fopen(kSavePath, "rb");
    if (!f) return false;
    DevSave tmp;
    const bool ok = std::fread(&tmp, sizeof(tmp), 1, f) == 1 && std::memcmp(tmp.magic, "EMBd", 4) == 0 &&
                    tmp.version == 1 && tmp.dragonSize == sizeof(Dragon);
    std::fclose(f);
    if (ok) out = tmp;
    return ok;
}

void writeSave(const DevSave& s) {
    mkdir("sdmc:/3ds", 0777);
    mkdir(kSaveDir, 0777);
    FILE* f = std::fopen(kSavePath, "wb");
    if (!f) return;
    std::fwrite(&s, sizeof(s), 1, f);
    std::fclose(f);
}

// ---------------------------------------------------------------- drawing helpers

u32 withAlpha(u32 c, float a) {
    const u32 alpha = static_cast<u32>((a < 0 ? 0 : (a > 1 ? 1 : a)) * 255.0f);
    return (c & 0x00FFFFFF) | (alpha << 24);
}

u32 fromRgb(Rgb c, u8 a = 255) { return theme::rgba(c.r, c.g, c.b, a); }

void text(App& app, const char* s, float x, float y, float scale, u32 color, u32 flags = C2D_AlignCenter) {
    C2D_Text t;
    C2D_TextParse(&t, app.textBuf, s);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor | flags, x, y, 0.5f, scale, scale, color);
}

void verticalGradient(float x, float y, float w, float h, u32 top, u32 bottom) {
    C2D_DrawRectangle(x, y, 0, w, h, top, top, bottom, bottom);
}

// Soft additive-looking glow from stacked translucent circles.
void glow(float x, float y, float radius, u32 color, float strength) {
    for (int i = 5; i >= 1; --i) {
        const float r = radius * (0.4f + 0.14f * i);
        C2D_DrawCircleSolid(x, y, 0, r, withAlpha(color, strength * 0.12f));
    }
}

void heart(float cx, float cy, float size, u32 color) {
    const float r = size * 0.3f;
    C2D_DrawCircleSolid(cx - r * 0.9f, cy - r * 0.3f, 0, r, color);
    C2D_DrawCircleSolid(cx + r * 0.9f, cy - r * 0.3f, 0, r, color);
    C2D_DrawTriangle(cx - size * 0.56f, cy - r * 0.05f, color, cx + size * 0.56f, cy - r * 0.05f, color, cx,
                     cy + size * 0.55f, color, 0);
}

void egg(float cx, float cy, float w, float h, Rgb shell, Rgb glowC, float glowAmt) {
    glow(cx, cy + h * 0.1f, w * 0.9f, fromRgb(glowC), glowAmt);
    C2D_DrawEllipseSolid(cx - w / 2, cy - h / 2, 0, w, h, fromRgb(shell));
    // Inner light shining through the shell.
    C2D_DrawEllipseSolid(cx - w * 0.3f, cy - h * 0.12f, 0, w * 0.6f, h * 0.55f, fromRgb(glowC, static_cast<u8>(90 * glowAmt)));
    // Speckles.
    static constexpr float kSpots[][3] = {{-0.2f, -0.25f, 0.06f}, {0.18f, -0.1f, 0.05f}, {-0.05f, 0.2f, 0.07f},
                                          {0.22f, 0.25f, 0.04f},  {-0.25f, 0.1f, 0.04f}};
    for (const auto& s : kSpots)
        C2D_DrawCircleSolid(cx + s[0] * w, cy + s[1] * h, 0, s[2] * w, withAlpha(theme::kDenPlum, 0.25f));
    // Highlight.
    C2D_DrawEllipseSolid(cx - w * 0.28f, cy - h * 0.38f, 0, w * 0.18f, h * 0.22f, withAlpha(theme::kShell, 0.5f));
}

struct Pulse {
    float brightness, speed;
};

Pulse moodPulse(Mood m) {
    switch (m) {
        case Mood::Joyful: return {1.0f, 6.0f};
        case Mood::Content: return {0.85f, 3.0f};
        case Mood::Restless: return {0.7f, 9.0f};
        case Mood::Sulky: return {0.45f, 1.5f};
        default: return {0.25f, 1.0f};
    }
}

float heartglowLevel(const Dragon& d, float t) {
    const Pulse p = moodPulse(moodOf(d));
    return p.brightness * (0.75f + 0.25f * std::sin(t * p.speed));
}

// Placeholder dragon: proportions follow the cute -> majestic curve from the art doc.
void dragonPlaceholder(const Dragon& d, float cx, float groundY, float scale, float t) {
    const Genome& g = d.genome;
    const float grow = (scale - 0.25f) / 0.75f;  // 0 hatchling .. 1 adult
    const float s = 0.55f + 0.45f * grow;         // drawn size (keeps babies visible)
    const Rgb base = hsvToRgb(g.baseH, g.baseS, g.baseV);
    const Rgb accent = hsvToRgb(g.accentH, static_cast<u8>(g.baseS / 2), g.accentV);
    const u32 baseC = fromRgb(base), accentC = fromRgb(accent);

    const float bodyW = 90 * s + 30 * grow, bodyH = 62 * s;
    const float bodyX = cx, bodyY = groundY - bodyH * 0.55f;
    const float bob = std::sin(t * 2.0f) * 2.0f;

    // Tail
    C2D_DrawTriangle(bodyX + bodyW * 0.3f, bodyY, baseC, bodyX + bodyW * 0.3f, bodyY + bodyH * 0.35f, baseC,
                     bodyX + bodyW * (0.75f + 0.4f * grow), bodyY + bodyH * 0.3f - 20 * grow, baseC, 0);
    // Wings
    const float wing = 26 + 70 * grow;
    const float flap = std::sin(t * 3.0f) * 4.0f;
    C2D_DrawTriangle(bodyX - 6, bodyY - bodyH * 0.2f, accentC, bodyX + 18, bodyY - bodyH * 0.25f, accentC, bodyX + 4,
                     bodyY - bodyH * 0.25f - wing + flap, accentC, 0);
    // Body + belly
    C2D_DrawEllipseSolid(bodyX - bodyW / 2, bodyY - bodyH / 2 + bob, 0, bodyW, bodyH, baseC);
    C2D_DrawEllipseSolid(bodyX - bodyW * 0.32f, bodyY - bodyH * 0.25f + bob, 0, bodyW * 0.42f, bodyH * 0.7f, accentC);
    // Heartglow
    const Rgb glowRgb = heartglowColor(static_cast<Element>(g.elementA));
    const float level = heartglowLevel(d, t);
    glow(bodyX - bodyW * 0.1f, bodyY + bob, 16 + 10 * s, fromRgb(glowRgb), level);
    heart(bodyX - bodyW * 0.1f, bodyY + bob, 12 + 6 * s, fromRgb(glowRgb, static_cast<u8>(140 + 115 * level)));
    // Neck and head: the head shrinks relative to the body as the dragon grows.
    const float neck = 6 + 40 * grow;
    const float headR = 30 * (1.0f - 0.35f * grow) * (0.8f + 0.4f * s);
    const float hx = bodyX - bodyW * 0.38f - neck * 0.4f, hy = bodyY - bodyH * 0.45f - neck + bob;
    C2D_DrawTriangle(bodyX - bodyW * 0.3f, bodyY - bodyH * 0.2f + bob, baseC, bodyX - bodyW * 0.1f,
                     bodyY - bodyH * 0.35f + bob, baseC, hx, hy, baseC, 0);
    // Horns
    const u32 hornC = theme::kClutchGold;
    const float horn = 6 + 18 * grow;
    C2D_DrawTriangle(hx + headR * 0.1f, hy - headR * 0.6f, hornC, hx + headR * 0.5f, hy - headR * 0.4f, hornC,
                     hx + headR * 0.6f, hy - headR * 0.6f - horn, hornC, 0);
    C2D_DrawTriangle(hx - headR * 0.4f, hy - headR * 0.6f, hornC, hx, hy - headR * 0.7f, hornC, hx - headR * 0.05f,
                     hy - headR * 0.75f - horn, hornC, 0);
    C2D_DrawCircleSolid(hx, hy, 0, headR, baseC);
    C2D_DrawEllipseSolid(hx - headR * 1.25f, hy - headR * 0.05f, 0, headR * 0.9f, headR * 0.6f, baseC);  // snout
    // Eyes: big and sparkly when small, calmer when grown. Closed while sleeping.
    const float eyeR = headR * (0.34f - 0.14f * grow);
    const float ex = hx - headR * 0.25f, ey = hy - headR * 0.1f;
    const bool closed = d.napping;
    if (closed) {
        C2D_DrawRectSolid(ex - eyeR, ey, 0, eyeR * 2, 2, theme::kDenPlum);
    } else {
        C2D_DrawCircleSolid(ex, ey, 0, eyeR, theme::kShell);
        C2D_DrawCircleSolid(ex - eyeR * 0.15f, ey + eyeR * 0.1f, 0, eyeR * 0.7f, theme::kDenPlum);
        C2D_DrawCircleSolid(ex - eyeR * 0.35f, ey - eyeR * 0.25f, 0, eyeR * 0.25f, theme::kShell);
    }
}

// ---------------------------------------------------------------- UI widgets

struct Rect {
    float x, y, w, h;
    bool contains(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

void panel(const Rect& r, u32 color) {
    C2D_DrawRectSolid(r.x + 4, r.y, 0, r.w - 8, r.h, color);
    C2D_DrawRectSolid(r.x, r.y + 4, 0, r.w, r.h - 8, color);
    C2D_DrawCircleSolid(r.x + 4, r.y + 4, 0, 4, color);
    C2D_DrawCircleSolid(r.x + r.w - 4, r.y + 4, 0, 4, color);
    C2D_DrawCircleSolid(r.x + 4, r.y + r.h - 4, 0, 4, color);
    C2D_DrawCircleSolid(r.x + r.w - 4, r.y + r.h - 4, 0, 4, color);
}

bool button(App& app, const Rect& r, const char* label, bool pressed, float tx, float ty) {
    const bool hit = pressed && r.contains(tx, ty);
    panel(r, hit ? theme::kClutchGold : theme::kShell);
    text(app, label, r.x + r.w / 2, r.y + r.h / 2 - 8, 0.55f, theme::kDenPlum);
    return hit;
}

void gauge(App& app, float x, float y, const char* label, float value) {
    text(app, label, x, y, 0.42f, theme::kShell, C2D_AlignLeft);
    const Rect bar{x, y + 14, 66, 8};
    panel(bar, theme::kDusk);
    const u32 fill = value < 25 ? theme::kRose : (value < 50 ? theme::kEmber : theme::kClutchGold);
    if (value > 2) panel({bar.x, bar.y, bar.w * value / 100.0f, bar.h}, fill);
}

void embers(float t, float w) {
    for (int i = 0; i < 14; ++i) {
        const float seed = i * 37.3f;
        const float x = std::fmod(seed * 7.1f + std::sin(t * 0.5f + i) * 12.0f, w);
        const float y = kH - std::fmod(t * (10 + i % 5 * 4) + seed * 3.0f, kH + 20);
        C2D_DrawCircleSolid(x, y, 0, 1.2f + (i % 3) * 0.6f, withAlpha(theme::kClutchGold, 0.35f + 0.1f * (i % 4)));
    }
}

void showToast(App& app, const char* msg) {
    app.toast = msg;
    app.toastTime = 2.0f;
}

// ---------------------------------------------------------------- scenes

void drawTitleTop(App& app) {
    verticalGradient(0, 0, kTopW, kH, theme::kDenPlum, theme::kDusk);
    embers(app.t, kTopW);
    const float pulse = 0.7f + 0.3f * std::sin(app.t * 2.5f);
    egg(200, 130, 64, 84, {255, 236, 205}, {255, 140, 40}, pulse);
    text(app, "Emberclutch", 200, 22, 1.1f, theme::kClutchGold);
    text(app, "raise, breed and fly with dragons", 200, 200, 0.5f, theme::kShell);
}

void drawTitleBottom(App& app) {
    verticalGradient(0, 0, kBotW, kH, theme::kDusk, theme::kDenPlum);
    panel({40, 90, 240, 60}, theme::kShell);
    const float a = 0.6f + 0.4f * std::sin(app.t * 3.0f);
    text(app, "Touch to begin", 160, 108, 0.7f, withAlpha(theme::kDenPlum, a));
    text(app, "Phase 0 prototype", 160, 212, 0.4f, theme::kAsh);
}

constexpr Element kStarters[3] = {Element::Ember, Element::Tide, Element::Gale};
constexpr const char* kStarterBlurb[3] = {"Warm-hearted and bold. Breathes flame.",
                                          "Gentle and clever. Loves the water.",
                                          "Light and quick. Born to fly."};

void drawPickTop(App& app) {
    verticalGradient(0, 0, kTopW, kH, theme::kDenPlum, theme::kDusk);
    embers(app.t, kTopW);
    const Element e = kStarters[app.starterHover];
    const float pulse = 0.75f + 0.25f * std::sin(app.t * 2.0f);
    egg(200, 120, 70, 92, {250, 240, 225}, heartglowColor(e), pulse);
    char line[64];
    std::snprintf(line, sizeof(line), "%s egg", elementName(e));
    text(app, line, 200, 18, 0.9f, theme::kClutchGold);
    text(app, kStarterBlurb[app.starterHover], 200, 192, 0.5f, theme::kShell);
}

void drawPickBottom(App& app, bool pressed, float tx, float ty) {
    verticalGradient(0, 0, kBotW, kH, theme::kDusk, theme::kDenPlum);
    text(app, "Choose your first egg", 160, 16, 0.6f, theme::kShell);
    for (int i = 0; i < 3; ++i) {
        const Rect r{16.0f + i * 100.0f, 60, 88, 120};
        const bool selected = i == app.starterHover;
        panel(r, selected ? theme::kShell : withAlpha(theme::kShell, 0.35f));
        egg(r.x + r.w / 2, r.y + 50, 36, 48, {250, 240, 225}, heartglowColor(kStarters[i]), selected ? 0.9f : 0.5f);
        text(app, elementName(kStarters[i]), r.x + r.w / 2, r.y + 92, 0.5f, theme::kDenPlum);
        if (pressed && r.contains(tx, ty)) {
            if (selected) {
                const s64 now = nowLocal(app);
                app.rng = Rng(static_cast<std::uint64_t>(osGetTime()) ^ 0xEC0DDull);
                app.save.dragon = makeEgg(1, makePurebred(kStarters[i], app.rng), now);
                std::snprintf(app.save.dragon.name, sizeof(app.save.dragon.name), "Kindle");
                app.save.hasDragon = 1;
                app.save.lastSim = now;
                app.scene = Scene::Den;
                writeSave(app.save);
            } else {
                app.starterHover = i;
            }
        }
    }
    text(app, "Tap again to choose", 160, 204, 0.45f, theme::kAsh);
}

void drawDenTop(App& app) {
    const Dragon& d = app.save.dragon;
    const s64 now = nowLocal(app);
    const bool night = isNight(now);
    verticalGradient(0, 0, kTopW, kH, night ? theme::kDenPlum : theme::kDusk, night ? theme::rgba(20, 14, 28) : theme::kDenPlum);
    C2D_DrawEllipseSolid(40, 190, 0, 320, 50, withAlpha(theme::kEmber, 0.18f));  // nest rug
    embers(app.t, kTopW);

    char line[96];
    if (d.stage == Stage::Egg) {
        const float progress = static_cast<float>(d.incubationSeconds) / kIncubationSeconds;
        egg(200, 130, 70, 92, {250, 240, 225}, heartglowColor(static_cast<Element>(d.genome.elementA)),
            0.3f + 0.7f * d.warmth / 100.0f);
        std::snprintf(line, sizeof(line), "%s egg  -  %d%% incubated", breedName(d.genome), static_cast<int>(progress * 100));
        text(app, line, 200, 14, 0.6f, theme::kShell);
        if (d.warmth <= 20) text(app, "It's getting cold...", 200, 205, 0.5f, theme::kRose);
    } else {
        dragonPlaceholder(d, 200, 205, bodyScale(d, now), app.t);
        std::snprintf(line, sizeof(line), "%s  -  %s %s", d.name, breedName(d.genome), stageName(d.stage));
        text(app, line, 200, 8, 0.6f, theme::kShell);
        std::snprintf(line, sizeof(line), "Day %d  -  %s  -  %s%s", daysSinceHatch(d, now) + 1, moodName(moodOf(d)),
                      personalityName(d.personality), d.napping ? "  -  napping" : "");
        text(app, line, 200, 26, 0.45f, theme::kClutchGold);
    }
    if (app.toast) text(app, app.toast, 200, 222, 0.5f, theme::kShell);
}

void drawDenBottom(App& app, bool pressed, bool held, float tx, float ty) {
    Dragon& d = app.save.dragon;
    const s64 now = nowLocal(app);
    verticalGradient(0, 0, kBotW, kH, theme::kDusk, theme::kDenPlum);

    if (d.stage == Stage::Egg) {
        text(app, "Rub the egg to keep it warm", 160, 10, 0.55f, theme::kShell);
        egg(160, 120, 80, 104, {250, 240, 225}, heartglowColor(static_cast<Element>(d.genome.elementA)),
            0.3f + 0.7f * d.warmth / 100.0f);
        if (held && tx > 100 && tx < 220 && ty > 60 && ty < 180) {
            if (app.lastTouchX >= 0) {
                const float dx = tx - app.lastTouchX, dy = ty - app.lastTouchY;
                warmEgg(d, std::sqrt(dx * dx + dy * dy) * 0.05f);
            }
            app.lastTouchX = tx;
            app.lastTouchY = ty;
        }
        gauge(app, 20, 196, "Warmth", d.warmth);
        if (tryHatch(d, now, app.rng)) {
            markVisit(d, now);
            showToast(app, "It hatched! Say hello to Kindle.");
            writeSave(app.save);
        }
    } else {
        gauge(app, 12, 6, "Belly", d.needs.belly);
        gauge(app, 88, 6, "Energy", d.needs.energy);
        gauge(app, 164, 6, "Shine", d.needs.shine);
        gauge(app, 240, 6, "Play", d.needs.play);

        // Heartglow orb mirrors the dragon's mood.
        const float level = heartglowLevel(d, app.t);
        const Rgb glowRgb = heartglowColor(static_cast<Element>(d.genome.elementA));
        glow(282, 88, 30, fromRgb(glowRgb), level);
        heart(282, 88, 22, fromRgb(glowRgb, static_cast<u8>(120 + 135 * level)));
        char line[48];
        std::snprintf(line, sizeof(line), "Bond %d", d.bond);
        text(app, line, 282, 116, 0.45f, theme::kShell);

        // Petting area: stroke the stylus across the pad.
        const Rect pad{12, 44, 220, 96};
        panel(pad, withAlpha(theme::kShell, 0.18f));
        text(app, d.upset ? "Hold still, then offer a treat..." : "Stroke here to pet", pad.x + pad.w / 2, pad.y + 38, 0.5f,
             theme::kShell);
        app.petCooldown -= 1.0f / 60.0f;
        if (held && pad.contains(tx, ty) && app.lastTouchX >= 0 && app.petCooldown <= 0) {
            const float dx = tx - app.lastTouchX, dy = ty - app.lastTouchY;
            if (dx * dx + dy * dy > 25) {
                pet(d, 3);
                markVisit(d, now);
                app.petCooldown = 0.25f;
            }
        }
        if (held) {
            app.lastTouchX = tx;
            app.lastTouchY = ty;
        }

        const float by = 150;
        if (button(app, {12, by, 70, 36}, "Feed", pressed, tx, ty)) {
            const bool fav = d.favoriteFood == d.genome.elementA;
            feed(d, 35, fav);
            markVisit(d, now);
            showToast(app, fav ? "Its favourite! A happy wiggle." : "Munch munch.");
        }
        if (button(app, {88, by, 70, 36}, "Groom", pressed, tx, ty)) {
            groom(d, 40);
            markVisit(d, now);
            showToast(app, "Scales polished to a shine.");
        }
        if (button(app, {164, by, 70, 36}, "Play", pressed, tx, ty)) {
            play(d, 35);
            markVisit(d, now);
            showToast(app, "Fetch! It bounds after the ball.");
        }
        if (d.upset && button(app, {240, by, 70, 36}, "Make up", pressed, tx, ty)) {
            makeUp(d);
            feed(d, 20, true);
            markVisit(d, now);
            showToast(app, "Its heartglow lights up again.");
        }
    }
    if (!held) app.lastTouchX = app.lastTouchY = -1;
    text(app, "DEV  R+A: +1 hour   R+X: +1 day   START: save & quit", 160, 222, 0.38f, theme::kAsh);
}

}  // namespace

int main() {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS * 2);
    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    App app;
    app.textBuf = C2D_TextBufNew(4096);
    if (loadSave(app.save) && app.save.hasDragon) {
        const s64 now = nowLocal(app);
        simulate(app.save.dragon, app.save.lastSim, now);  // catch up on time away
        app.save.lastSim = now;
        markVisit(app.save.dragon, now);
    }

    float simAccum = 0, saveAccum = 0;
    while (aptMainLoop()) {
        hidScanInput();
        const u32 down = hidKeysDown(), held = hidKeysHeld();
        if (down & KEY_START) break;

        touchPosition touch;
        hidTouchRead(&touch);
        const bool touching = held & KEY_TOUCH;
        const bool tapped = down & KEY_TOUCH;
        const float tx = touch.px, ty = touch.py;

        const float dt = 1.0f / 60.0f;
        app.t += dt;
        if (app.toast && (app.toastTime -= dt) <= 0) app.toast = nullptr;

        if (app.scene == Scene::Den) {
            // Dev time skip for testing growth without waiting days.
            if ((held & KEY_R) && (down & KEY_A)) app.save.devOffset += kHour;
            if ((held & KEY_R) && (down & KEY_X)) app.save.devOffset += kDay;

            simAccum += dt;
            saveAccum += dt;
            if (simAccum >= 1.0f || (held & KEY_R)) {
                const s64 now = nowLocal(app);
                simulate(app.save.dragon, app.save.lastSim, now);
                app.save.lastSim = now;
                simAccum = 0;
            }
            if (saveAccum >= 60.0f) {
                writeSave(app.save);
                saveAccum = 0;
            }
        }

        C2D_TextBufClear(app.textBuf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C2D_TargetClear(top, theme::kDenPlum);
        C2D_SceneBegin(top);
        switch (app.scene) {
            case Scene::Title: drawTitleTop(app); break;
            case Scene::PickStarter: drawPickTop(app); break;
            case Scene::Den: drawDenTop(app); break;
        }

        C2D_TargetClear(bottom, theme::kDenPlum);
        C2D_SceneBegin(bottom);
        switch (app.scene) {
            case Scene::Title:
                drawTitleBottom(app);
                if (tapped) app.scene = app.save.hasDragon ? Scene::Den : Scene::PickStarter;
                break;
            case Scene::PickStarter: drawPickBottom(app, tapped, tx, ty); break;
            case Scene::Den: drawDenBottom(app, tapped, touching, tx, ty); break;
        }

        C3D_FrameEnd(0);
    }

    if (app.save.hasDragon) writeSave(app.save);
    C2D_TextBufDelete(app.textBuf);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
