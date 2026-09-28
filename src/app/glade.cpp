// Moonpetal Glade (1.0, D90): the pageant's feature in the valley. Its people (the host, the
// judges, the two stall keepers), the board (the leagues and today's shows: enter one, or dress
// up first), the stalls (a thing tried on your dragon before you buy it) and the show itself
// (app/glade_show.cpp). See app/glade.hpp.
#include "app/glade.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "app/audio.hpp"
#include "app/dialogue.hpp"
#include "app/glade_show.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "app/wardrobe.hpp"
#include "core/accessories.hpp"
#include "core/clock.hpp"
#include "core/kinds.hpp"
#include "core/pageant.hpp"
#include "core/people.hpp"
#include "core/place_layout.hpp"
#include "core/trainer.hpp"

namespace ec::glade {
namespace {

enum class Mode : u8 { None, Board, Stall, Show };

struct Glade {
    Mode mode = Mode::None;
    // A scripted start (autotest): opened once the valley lends the feature its stage.
    Mode pending = Mode::None;
    int pendingLeague = 1, pendingSlot = 0;
    int league = 1, slot = 0;  // the board's pick
    int stall = 0;             // 0 the accessories, 1 the dyes
    int pick = -1;             // the stall's thing picked (an accessory or a dye; -1 none)
    Dragon preview;            // the partner trying it on
    bool previewing = false;
    bool heard[3] = {};        // the host's hello and the keepers' (this visit to the game)
    bool toWardrobe = false;   // Dress up: the wardrobe opens after this frame
};

Glade& gs() {
    static Glade g;
    return g;
}

u32 col(u8 r, u8 g, u8 b, float a = 1.0f) { return withAlpha(theme::rgba(r, g, b), a); }

// ---- The glade's people: bodies from the people kit, dressed their own way.
enum Who : u8 { kHost, kJudge0, kJudge1, kJudge2, kMilliner, kDyer, kWhoCount };
struct Npc {
    Person body;
    u8 hair, hairColour, skin, eyes;
    Rgb outfit[2];
    u8 voice;  // (Noah's rule: 0 men, 1 women and children)
    float pitch;
};
constexpr Npc kNpcs[kWhoCount] = {
    {Person::PlayerB, 5, 5, 1, 1, {{112, 64, 158}, {236, 196, 110}}, 1, 1.5f},   // Celestine: silver hair, a violet gown
    {Person::PlayerB, 3, 4, 0, 2, {{64, 150, 150}, {246, 226, 180}}, 1, 1.4f},   // Plume
    {Person::PlayerA, 0, 5, 2, 0, {{128, 90, 60}, {214, 190, 140}}, 0, 1.2f},    // Wick
    {Person::PlayerB, 2, 5, 3, 3, {{226, 190, 80}, {150, 110, 60}}, 1, 1.35f},   // Tansy
    {Person::PlayerB, 1, 3, 0, 2, {{110, 160, 96}, {236, 150, 170}}, 1, 1.65f},  // Linnet, the milliner
    {Person::PlayerA, 4, 2, 2, 1, {{66, 70, 140}, {176, 60, 70}}, 0, 1.3f},     // Madder, the dyer
};

void npcView(int who, r3d::PersonView& p) {
    const Npc& n = kNpcs[who];
    const u8 look[kLookParts] = {static_cast<u8>(n.body == Person::PlayerB), n.hair, n.hairColour, n.skin, 0, n.eyes};
    p.form = static_cast<u8>(n.body);
    playerPalette(look, p.pal);
    p.pal[kPalAccent] = n.outfit[0];
    p.pal[kPalPattern] = n.outfit[1];
    p.hair = static_cast<s8>(n.hair);
}

Speaker speakerOf(int who) {
    Speaker s;
    s.name = who == kHost ? str::kHostName : who == kMilliner ? str::kMilliner : who == kDyer ? str::kDyer
                                                                                                : str::kJudgeNames[who - kJudge0];
    s.title = who == kHost ? str::kHostTitle : who == kMilliner ? str::kMillinerTitle : who == kDyer ? str::kDyerTitle : "";
    s.voice = kNpcs[who].voice;
    s.pitch = kNpcs[who].pitch;
    s.tint = kNpcs[who].outfit[1];
    return s;
}

void say(App& app, int who, const char* const* lines, int count) {
    Talk t;
    for (int i = 0; i < count && i < kMaxLines; ++i) t.lines[t.count++] = lines[i];
    startSpeech(app, speakerOf(who), t);
}

// Where a stall's keeper stands, and the spot in front of it you shop from (world).
void stallSpots(const Valley& v, int stall, Vec3& keeper, float& keeperHeading, Vec3& customer) {
    const Layout& L = gladeLayout();
    const ValleyPlaceInfo* p = gladePlace(v);
    const Vec3 s = L.stalls[stall];
    keeper = gladePoint(v, {s.x, s.y});
    keeperHeading = (p ? p->heading : 0.0f) + s.z + 3.14159265f;
    customer = keeper + Vec3{std::sin(keeperHeading), -std::cos(keeperHeading), 0} * 2.4f;
    customer.z = v.heightAt(customer.x, customer.y);
}

// ---- The feature
int folk(const App& app, const Valley& v, Vec3 near, float radius, vext::Folk* out, int cap) {
    (void)app;
    const ValleyPlaceInfo* p = gladePlace(v);
    if (!p || std::hypot(near.x - p->at.x, near.y - p->at.y) > radius + 25.0f) return 0;
    const Layout& L = gladeLayout();
    int n = 0;
    auto add = [&](int who, Vec3 at, float heading, const char* name, const char* prompt, float reach) {
        if (n >= cap) return;
        vext::Folk& f = out[n++];
        f = vext::Folk{};
        npcView(who, f.look);
        f.look.at = at;
        f.look.heading = heading;
        f.name = name;
        f.prompt = prompt;
        f.id = static_cast<u8>(who);
        f.reach = reach;
        f.voice = kNpcs[who].voice;
        f.pitch = kNpcs[who].pitch;
    };
    const Vec3 middle = gladePoint(v, {0, 3});
    Vec3 host;
    float hostHeading;
    if (!showHostSpot(v, host, hostHeading)) {  // by the board, or at the stage's side during a show
        host = gladePoint(v, {L.board.x + 1.3f, L.board.y - 0.4f});
        hostHeading = headingTo(host, middle);
    }
    add(kHost, host, hostHeading, str::kHostName, str::kPromptPageant, 2.6f);
    const Vec3 stage = gladePoint(v, {L.stage.x, L.stage.y});
    const Vec3 table = gladePoint(v, L.judges);
    const Vec3 toStage = normalize(Vec3{stage.x - table.x, stage.y - table.y, 0}), along{-toStage.y, toStage.x, 0};
    for (int k = 0; k < 3 && showOn(); ++k) {  // at the table for a show, looking at the stage (not to be spoken to)
        Vec3 at = table + along * ((k - 1) * 1.1f) - toStage * 0.5f;
        at.z = v.heightAt(at.x, at.y);
        add(kJudge0 + k, at, headingTo(at, stage), str::kJudgeNames[k], "", 0.0f);
    }
    for (int s = 0; s < 2; ++s) {
        Vec3 keeper, customer;
        float heading;
        stallSpots(v, s, keeper, heading, customer);
        add(s ? kDyer : kMilliner, keeper, heading, s ? str::kDyer : str::kMilliner, str::kPromptStall, 2.8f);
    }
    return n;
}

void openBoard(App& app) {
    Glade& g = gs();
    g.mode = Mode::Board;
    g.league = pageant::boardLeague(app.game);
    g.slot = 0;
    for (int k = kShowSlots - 1; k >= 0; --k)
        if (!pageant::slotWon(app.game, g.league, k)) g.slot = k;  // the first not yet won
}

void openStall(int stall) {
    Glade& g = gs();
    g.mode = Mode::Stall;
    g.stall = stall;
    g.pick = -1;
    g.previewing = false;
}

void act(App& app, const vext::Folk& who, vext::Stage& stage) {
    (void)stage;
    Glade& g = gs();
    if (who.id == kHost) {
        if (!g.heard[0]) say(app, kHost, str::kHostHello, 3);
        else say(app, kHost, &str::kHostAgain, 1);
        g.heard[0] = true;
        openBoard(app);
    } else if (who.id == kMilliner || who.id == kDyer) {
        const int stall = who.id == kDyer ? 1 : 0;
        if (!g.heard[1 + stall]) say(app, who.id, stall ? &str::kDyerHello : &str::kMillinerHello, 1);
        g.heard[1 + stall] = true;
        openStall(stall);
        audio::playSfx(audio::Sfx::VillageBell);
    }
}

bool active(const App& app) {
    (void)app;
    return gs().mode != Mode::None || gs().pending != Mode::None;
}

// Out to the wardrobe with your partner (back at the glade after).
void dressUp(App& app, const vext::Stage& stage) {
    Glade& g = gs();
    if (stage.partner < 0) return;
    g.mode = Mode::None;
    g.toWardrobe = false;
    r3d::releaseValley();
    app.game.world.inValley = 0;
    openWardrobe(app, stage.partner, SceneId::Valley);
}

void faceTo(vext::Stage& stage, Vec3 you, Vec3 pal, Vec3 look) {
    stage.you = you;
    stage.youHeading = headingTo(you, look);
    stage.pal = pal;
    stage.palHeading = headingTo(pal, look);
}

void update(App& app, const Input& in, vext::Stage& stage) {
    Glade& g = gs();
    if (!stage.valley) return;
    const Valley& v = *stage.valley;
    if (g.pending != Mode::None) {  // a scripted start
        const Mode want = g.pending;
        g.pending = Mode::None;
        if (want == Mode::Board) {
            openBoard(app);
        } else if (want == Mode::Stall) {
            const int pick = g.pick;  // (a script's thing to try on)
            openStall(g.stall);
            g.pick = pick;
            g.previewing = pick >= 0;
        } else if (want == Mode::Show && stage.partner >= 0) {
            g.mode = Mode::Show;
            beginShow(app, stage, g.pendingLeague, g.pendingSlot);
        }
    }
    if (g.toWardrobe) {
        dressUp(app, stage);
        return;
    }
    const Layout& L = gladeLayout();
    switch (g.mode) {
        case Mode::Board: {
            if (in.down & KEY_B) {
                g.mode = Mode::None;
                audio::playSfx(audio::Sfx::Back);
                break;
            }
            // Before the board with the host, your partner behind you; the camera takes in the
            // stage beyond.
            const Vec3 board = gladePoint(v, L.board);
            const Vec3 you = gladePoint(v, {L.board.x + 0.6f, L.board.y + 1.8f});
            const Vec3 pal = gladePoint(v, {L.board.x - 1.6f, L.board.y + 3.8f});
            faceTo(stage, you, pal, board);
            stage.youClip = "idle";
            stage.camSet = true;
            stage.eye = gladePoint(v, {L.board.x + 7.5f, L.board.y + 8.0f}, 3.6f);
            stage.target = gladePoint(v, {L.board.x + 1.5f, L.board.y - 3.0f}, 1.2f);
            break;
        }
        case Mode::Stall: {
            if (in.down & KEY_B) {
                g.mode = Mode::None;
                audio::playSfx(audio::Sfx::Back);
                break;
            }
            Vec3 keeper, customer;
            float heading;
            stallSpots(v, g.stall, keeper, heading, customer);
            const Vec3 out{std::sin(heading), -std::cos(heading), 0};  // from the stall toward you
            const Vec3 side{-out.y, out.x, 0};
            // You at the counter; your partner a step out and aside, turned to the camera, which
            // stands off to the side and frames it (and the stall behind): trying things on.
            const float size = stage.shown ? kindSize(*stage.shown) * (stage.shown->stage == Stage::Adult ? 1.0f : 0.5f) : 1.0f;
            Vec3 pal = customer + out * (1.0f + size) + side * (1.2f + size);
            pal.z = v.heightAt(pal.x, pal.y);
            stage.camSet = true;
            stage.eye = pal + out * (3.0f + 2.6f * size) - side * (1.4f + 1.2f * size) + Vec3{0, 0, 1.4f + 1.1f * size};
            stage.target = pal + Vec3{0, 0, 0.9f * size + 0.2f} - side * 0.6f;
            stage.you = customer;
            stage.youHeading = headingTo(customer, keeper);
            stage.pal = pal;
            stage.palHeading = headingTo(pal, stage.eye);
            stage.youClip = "idle";
            break;
        }
        case Mode::Show:
            if (!updateShow(app, in, stage)) g.mode = Mode::None;
            break;
        case Mode::None: break;
    }
}

void view(App& app, const vext::Stage& stage, r3d::ValleyView& view) {
    Glade& g = gs();
    if (g.mode == Mode::Show) {
        showView(app, stage, view);
        return;
    }
    if (g.mode == Mode::Stall && g.previewing && stage.shown && view.dragon) {  // trying it on
        g.preview = *stage.shown;
        if (g.stall == 0 && g.pick >= 0) g.preview.wear[static_cast<int>(accessoryInfo(g.pick).slot)] = static_cast<u8>(g.pick);
        if (g.stall == 1 && g.pick >= 0) g.preview.dye = static_cast<u8>(g.pick);
        view.dragon = &g.preview;
    }
    view.lead = false;  // (standing still to shop or read the board)
}

// ---- The board
void boardTop(App& app, const vext::Stage& stage) {
    const Glade& g = gs();
    const s32 day = dayIndex(nowLocal(app));
    const int theme = pageant::showTheme(g.league, g.slot, day);
    const ThemeInfo& t = themeInfo(theme);
    panel({40, 150, 320, 78}, col(40, 26, 56, 0.82f));
    textCentered(app, t.name, 200, 166, 0.8f, theme::kClutchGold, 300, Face::Title);
    textCentered(app, t.blurb, 200, 188, 0.45f, theme::kShell, 300);
    char elements[48] = {}, styles[48] = {}, line[128];
    int at = 0;
    for (int e = 0; e < elementCount() && at < 40; ++e)
        if ((t.elements >> e) & 1u) at += std::snprintf(elements + at, sizeof(elements) - at, at ? "/%s" : "%s", elementName(e));
    at = 0;
    for (int b = 0; b < kStyleTags && at < 40; ++b)
        if ((t.styles >> b) & 1u) at += std::snprintf(styles + at, sizeof(styles) - at, at ? " and %s" : "%s", styleName(b));
    std::snprintf(line, sizeof(line), str::kBoardFavours, elements, styles);
    textCentered(app, line, 200, 204, 0.4f, withAlpha(theme::kShell, 0.85f), 310);
    if (stage.shown) {  // how yours would do in it, the first two rounds
        Rgb pal[kPalCount];
        kindPalette(stage.shown->kind, stage.shown->variant, stage.shown->id, pal);
        applyDye(stage.shown->dye, pal);
        std::snprintf(line, sizeof(line), str::kBoardYourLook, static_cast<int>(pageant::lookScore(*stage.shown, theme, pal)),
                      static_cast<int>(pageant::poiseScore(*stage.shown)));
        textCentered(app, line, 200, 219, 0.4f, theme::kClutchGold, 300);
    }
}

void boardBottom(App& app, const Input& in, const vext::Stage& stage) {
    Glade& g = gs();
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    textCentered(app, str::kBoardTitle, 160, 13, 0.6f, theme::kClutchGold, 300, Face::Title);
    const s32 day = dayIndex(nowLocal(app));
    for (int l = 1; l <= kLeagues; ++l) {  // the leagues' tabs
        const Rect r{6.0f + (l - 1) * 78.0f, 26, 74, 24};
        const bool open = pageant::leagueOpen(app.game, l), on = g.league == l;
        panel(r, on ? theme::kClutchGold : withAlpha(theme::kShell, open ? 0.2f : 0.07f));
        textCentered(app, trainer::leagueName(l), r.x + r.w / 2, r.y + 12, 0.42f,
                     on ? theme::kDenPlum : withAlpha(theme::kShell, open ? 1.0f : 0.4f), r.w - 4);
        if (in.released && r.contains(in.rx, in.ry) && !on) {
            if (open) {
                g.league = l;
                audio::playSfx(audio::Sfx::Tap);
            } else {
                char line[64];
                std::snprintf(line, sizeof(line), str::kBoardLocked, trainer::leagueName(l - 1));
                showToastf(app, "%s", line);
                audio::playSfx(audio::Sfx::Error);
            }
        }
    }
    char line[96];
    if (app.game.progress.showLeague >= g.league) std::snprintf(line, sizeof(line), "%s", str::kBoardLeagueWon);
    else std::snprintf(line, sizeof(line), str::kBoardShowsWon, pageant::slotsWon(app.game, g.league));
    textCentered(app, line, 160, 58, 0.4f, withAlpha(theme::kShell, 0.8f), 300);
    for (int k = 0; k < kShowSlots; ++k) {  // today's four shows
        const Rect r{8.0f + (k % 2) * 154.0f, 68.0f + (k / 2) * 52.0f, 150, 48};
        const int theme = pageant::showTheme(g.league, k, day);
        const bool on = g.slot == k, won = pageant::slotWon(app.game, g.league, k);
        panel(r, on ? withAlpha(theme::kClutchGold, 0.55f) : withAlpha(theme::kShell, 0.14f));
        const ThemeInfo& t = themeInfo(theme);
        const Rgb a = t.colours[0], b = t.colours[1];
        C2D_DrawCircleSolid(r.x + 14, r.y + 16, 0.5f, 7, fromRgb(a));
        C2D_DrawCircleSolid(r.x + 22, r.y + 22, 0.5f, 5, fromRgb(b));
        text(app, t.name, r.x + 32, r.y + 6, 0.42f, theme::kShell, C2D_AlignLeft, r.w - 36);
        if (won) text(app, str::kBoardWon, r.x + 32, r.y + 24, 0.36f, theme::kClutchGold, C2D_AlignLeft, 50);
        if (pageant::paidToday(app.game, g.league, k, day))
            text(app, str::kBoardPaid, r.x + 32 + (won ? 34 : 0), r.y + 24, 0.32f, withAlpha(theme::kShell, 0.6f), C2D_AlignLeft, 100);
        if (in.released && r.contains(in.rx, in.ry) && !on) {
            g.slot = k;
            audio::playSfx(audio::Sfx::Tap);
        }
    }
    const Dragon* d = stage.partner >= 0 && stage.partner < app.game.dragonCount ? &app.game.dragons[stage.partner] : nullptr;
    const bool enter = button(app, {8, 178, 150, 30}, str::kBoardEnter, in, theme::kClutchGold) || (in.down & KEY_A);
    if (enter) {
        if (!d) {
            showToast(app, str::kBoardAlone);
            audio::playSfx(audio::Sfx::Error);
        } else if (!trainer::spendEnergy(app.game.dragons[stage.partner], trainer::kEnergyShow)) {
            showToastf(app, str::kBoardTired, d->name);
            audio::playSfx(audio::Sfx::Error);
        } else {  // (the show starts in the next update, which has the stage to move)
            g.pendingLeague = g.league;
            g.pendingSlot = g.slot;
            g.pending = Mode::Show;
            audio::playSfx(audio::Sfx::Confirm);
            return;
        }
    }
    if (d && button(app, {162, 178, 150, 30}, str::kDressUp, in)) {
        g.toWardrobe = true;
        audio::playSfx(audio::Sfx::Confirm);
    }
    if (button(app, {110, 212, 100, 26}, str::kBack, in)) {
        g.mode = Mode::None;
        audio::playSfx(audio::Sfx::Back);
    }
}

// ---- The stalls
int stallItems(const App& app, int stall, int* out) {
    if (stall == 0) {
        acc::stallPicks(dayIndex(nowLocal(app)), out);
        int n = 0;
        while (n < acc::kStallShown && out[n] >= 0) ++n;
        return n;
    }
    int n = 0;
    for (int dye = 1; dye < dyeCount(); ++dye) out[n++] = dye;
    return n;
}

void stallTop(App& app) {
    const Glade& g = gs();
    char gleam[24];
    std::snprintf(gleam, sizeof(gleam), "%lu", static_cast<unsigned long>(app.game.gleam));
    panel({300, 6, 94, 22}, col(40, 26, 56, 0.8f));
    C2D_DrawCircleSolid(314, 17, 0, 6, theme::kClutchGold);
    text(app, gleam, 386, 9, 0.45f, theme::kShell, C2D_AlignRight);
    panel({8, 6, 200, 26}, col(40, 26, 56, 0.8f));
    text(app, g.stall ? str::kStallDyes : str::kStallAccessories, 16, 9, 0.55f, theme::kClutchGold, C2D_AlignLeft, 186, Face::Title);
    if (g.pick < 0) return;
    char line[96];
    panel({60, 196, 280, 36}, col(40, 26, 56, 0.82f));
    if (g.stall == 0) {
        const Accessory& a = accessoryInfo(g.pick);
        textCentered(app, a.name, 200, 206, 0.5f, theme::kShell, 270);
        char styles[64] = {};
        int at = 0;
        for (int b = 0; b < kStyleTags && at < 56; ++b)
            if ((a.styles >> b) & 1u) at += std::snprintf(styles + at, sizeof(styles) - at, at ? ", %s" : "%s", styleName(b));
        std::snprintf(line, sizeof(line), str::kStallStyles, styles);
        textCentered(app, line, 200, 222, 0.38f, withAlpha(theme::kShell, 0.8f), 270);
    } else {
        textCentered(app, dyeInfo(g.pick).name, 200, 213, 0.55f, theme::kShell, 270);
    }
}

void stallBottom(App& app, const Input& in, const vext::Stage& stage) {
    Glade& g = gs();
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    int items[kDyeCount + acc::kStallShown];
    const int n = stallItems(app, g.stall, items);
    const int cols = g.stall ? 4 : 3;
    const float w = g.stall ? 76.0f : 102.0f, h = g.stall ? 50.0f : 72.0f;
    for (int i = 0; i < n; ++i) {
        const Rect r{6.0f + (i % cols) * (w + 2), 6.0f + (i / cols) * (h + 2), w, h};
        const int it = items[i];
        const bool owned = g.stall ? trainer::ownsDye(app.game, it) : trainer::ownsAccessory(app.game, it);
        const bool on = g.pick == it;
        panel(r, on ? withAlpha(theme::kClutchGold, 0.5f) : withAlpha(theme::kShell, 0.14f));
        char price[32];
        const bool prize = g.stall && dyeInfo(it).source != WearSource::Stall;
        if (owned) std::snprintf(price, sizeof(price), "%s", str::kStallOwned);
        else if (prize) std::snprintf(price, sizeof(price), "%s", str::kStallPrize);
        else std::snprintf(price, sizeof(price), str::kPrice, static_cast<unsigned long>(g.stall ? dyeInfo(it).price : accessoryInfo(it).price));
        if (g.stall) {
            drawDyeSwatch(it, r.x + 16, r.y + r.h / 2, 22);
            text(app, dyeInfo(it).name, r.x + 30, r.y + 8, 0.36f, theme::kShell, C2D_AlignLeft, r.w - 32);
            text(app, price, r.x + 30, r.y + 28, 0.32f, owned ? theme::kSkyTeal : theme::kClutchGold, C2D_AlignLeft, r.w - 32);
        } else {
            drawAccessoryIcon(it, r.x + r.w / 2, r.y + 24, 34);
            textCentered(app, accessoryInfo(it).name, r.x + r.w / 2, r.y + 50, 0.34f, theme::kShell, r.w - 6);
            textCentered(app, price, r.x + r.w / 2, r.y + 63, 0.34f, owned ? theme::kSkyTeal : theme::kClutchGold, r.w - 6);
        }
        if (in.released && r.contains(in.rx, in.ry) && !on) {
            g.pick = it;
            g.previewing = true;
            audio::playSfx(audio::Sfx::Equip);
        }
    }
    if (g.stall == 0) textCentered(app, str::kGladeStallTomorrow, 160, 158, 0.36f, withAlpha(theme::kShell, 0.6f), 300);
    const bool canBuy = g.pick >= 0 && !(g.stall ? trainer::ownsDye(app.game, g.pick) : trainer::ownsAccessory(app.game, g.pick)) &&
                        !(g.stall && dyeInfo(g.pick).source != WearSource::Stall);
    if (canBuy) {
        char label[32];
        std::snprintf(label, sizeof(label), str::kBuyFor,
                      static_cast<unsigned long>(g.stall ? dyeInfo(g.pick).price : accessoryInfo(g.pick).price));
        if (button(app, {8, 172, 150, 30}, label, in, theme::kClutchGold)) {
            const bool ok = g.stall ? acc::buyDye(app.game, g.pick) : acc::buyAccessory(app.game, g.pick);
            if (ok) {
                audio::playSfx(audio::Sfx::Register);
                showToast(app, g.stall ? str::kStallBoughtDye : str::kStallBoughtThing);
                saveNow(app);
            } else {
                audio::playSfx(audio::Sfx::Error);
                showToast(app, str::kNotEnoughGleam);
            }
        }
    }
    if (stage.partner >= 0 && button(app, {162, 172, 150, 30}, str::kWardrobe, in)) {
        g.toWardrobe = true;
        audio::playSfx(audio::Sfx::Confirm);
    }
    if (button(app, {110, 208, 100, 28}, str::kBack, in)) {
        g.mode = Mode::None;
        audio::playSfx(audio::Sfx::Back);
    }
}

void drawTop(App& app, const vext::Stage& stage) {
    switch (gs().mode) {
        case Mode::Board: boardTop(app, stage); break;
        case Mode::Stall: stallTop(app); break;
        case Mode::Show: showDrawTop(app, stage); break;
        case Mode::None: break;
    }
}

void drawBottom(App& app, const Input& in, const vext::Stage& stage) {
    switch (gs().mode) {
        case Mode::Board: boardBottom(app, in, stage); break;
        case Mode::Stall: stallBottom(app, in, stage); break;
        case Mode::Show: showDrawBottom(app, in, stage); break;
        case Mode::None: verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum); break;
    }
}

}  // namespace

const vext::Feature kFeature{"pageant", folk, act, active, update, view, drawTop, drawBottom};

const Layout& gladeLayout() {
    static const Layout kLayout{};
    return kLayout;
}

const ValleyPlaceInfo* gladePlace(const Valley& v) { return v.place(kPlaceGlade); }

Vec3 gladePoint(const Valley& v, Vec2 local, float lift) {
    const ValleyPlaceInfo* p = gladePlace(v);
    if (!p) return {};
    return placeToWorld3(v, *p, {local.x, local.y, lift});
}

float headingTo(Vec3 from, Vec3 to) { return std::atan2(to.x - from.x, -(to.y - from.y)); }

}  // namespace ec::glade

namespace ec {

void pageantCommand(App& app, const char* text) {
    using namespace glade;
    char word[16] = {};
    float a[4] = {-1, -1, -1, -1};
    const int got = std::sscanf(text, " %15s %f %f %f %f", word, &a[0], &a[1], &a[2], &a[3]);
    if (got < 1) return;
    Glade& g = gs();
    Dragon* d = hasDragon(app) ? &activeDragon(app) : nullptr;
    const std::string w = word;
    if (w == "give") {
        for (int k = 0; k < accessoryCount(); ++k) trainer::giveAccessory(app.game, k);
        for (int k = 1; k < dyeCount(); ++k) trainer::giveDye(app.game, k);
    } else if (w == "wear" && d) {
        for (int s = 0; s < kWearSlots; ++s) {
            const int k = static_cast<int>(a[s]);
            if (k < 0 || k >= accessoryCount()) continue;
            trainer::giveAccessory(app.game, k);
            acc::putOn(app.game, *d, k);
        }
    } else if (w == "bare" && d) {
        for (int s = 0; s < kWearSlots; ++s) acc::takeOff(*d, static_cast<WearSlot>(s));
        d->dye = 0;
    } else if (w == "dye" && d) {
        const int k = static_cast<int>(a[0]);
        trainer::giveDye(app.game, k);
        acc::dyeWith(app.game, *d, k);
    } else if (w == "kind" && d && d->stage != Stage::Egg) {
        const int kind = static_cast<int>(a[0]), variant = a[1] >= 0 ? static_cast<int>(a[1]) : 0;
        if (kind >= 0 && kind < kindCount()) {
            d->kind = static_cast<u8>(kind);
            d->variant = static_cast<u8>(variant % kKindVariants);
        }
    } else if ((w == "grown" || w == "hatchling") && d && d->stage != Stage::Egg) {
        d->stage = w == "grown" ? Stage::Adult : Stage::Hatchling;
        const s64 now = nowLocal(app);
        d->hatchedAt = w == "grown" ? now - 40 * kDay : now - 1 * kHour;  // (in-stage growth: grown full, a hatchling new)
    } else if (w == "clean" && d) {
        bathe(*d);
        for (float& m : d->mud) m = 0;
    } else if (w == "bond" && d) {
        d->bond = static_cast<u16>(a[0] < 0 ? 0 : (a[0] > 1000 ? 1000 : a[0]));
        d->careStars = 30;
        d->needs.energy = 100;
    } else if (w == "wardrobe" && d) {
        openWardrobe(app, app.careIndex > 0 ? app.careIndex : 0, app.scene);
    } else if (w == "spin") {
        wardrobeTurn(a[0]);
    } else if (w == "tab") {
        wardrobeTab(static_cast<int>(a[0]));
    } else if (w == "board") {
        g.pending = Mode::Board;
    } else if (w == "stall") {
        g.stall = a[0] > 0 ? 1 : 0;
        g.pending = Mode::Stall;
        g.pick = a[1] >= 0 ? static_cast<int>(a[1]) : -1;
        g.previewing = g.pick >= 0;
    } else if (w == "show") {
        g.pendingLeague = a[0] >= 1 ? static_cast<int>(a[0]) : 1;
        g.pendingSlot = a[1] >= 0 ? static_cast<int>(a[1]) : 0;
        g.pending = Mode::Show;
    } else if (w == "autoplay") {
        setShowAutoplay(std::strstr(text, "on") != nullptr);
    }
    saveNow(app);
}

}  // namespace ec
