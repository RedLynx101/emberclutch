// Frostspire Hollow in the valley (1.0, D90): Tove keeps the bowl's mouth; A beside her, her
// words, then where to start (the top, or a checkpoint your dragon has reached). Floor after
// floor a wild dragon comes out of the cave door at the back of the bowl and battles you where
// you stand on the bowl's floor (app/battle_view); win and go deeper (or leave), lose and you're
// back with Tove, no harm done. The deeper, the colder and darker the bowl (its fog, its light).
// The rules are core/hollow's.
#include <cmath>
#include <cstdio>

#include "app/story_app.hpp"
#include "app/audio.hpp"
#include "app/tips_ui.hpp"
#include "app/battle_feature.hpp"
#include "app/battle_view.hpp"
#include "app/dialogue.hpp"
#include "app/render3d.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/accessories.hpp"
#include "core/care.hpp"
#include "core/clock.hpp"
#include "core/hollow.hpp"
#include "core/kinds.hpp"
#include "core/people.hpp"
#include "core/place_layout.hpp"
#include "core/trainer.hpp"
#include "core/valley.hpp"
#include "core/wanderings.hpp"

namespace ec {
namespace {

enum class Mode : u8 { None, Gate, Battle, Between };

struct Hollow {
    Mode mode = Mode::None;
    int floor = 1;       // the floor being battled (or next)
    int partner = -1;    // your dragon (app.game.dragons)
    int pendingFloor = 0;  // a scripted run's floor, set up on the next update
    bool pendingKeeper = false;  // ...or up to Tove and A
    float t = 0;
    Dragon wild;
    int picked = 0;      // the checkpoint picked at the gate
    int nextKind = -1;   // the next floor's wild one's kind (read ahead while you choose)
};

Hollow& ho() {
    static Hollow s;
    return s;
}

Vec3 at3(const Valley& v, Vec2 local, float up = 0) {
    const ValleyPlaceInfo* p = v.place(kPlaceHollow);
    return p ? placeToWorld3(v, *p, {local.x, local.y, up}) : Vec3{};
}

// Tove in her own winter things (D138).
r3d::PersonView keeperLook(const Valley& v) {
    r3d::PersonView p;
    dressAs(story::kPTove, p);
    p.at = at3(v, hollow::keeperSpot());
    const ValleyPlaceInfo* place = v.place(kPlaceHollow);
    p.heading = (place ? place->heading : 0.0f) + hollow::keeperFacing();
    return p;
}

Speaker keeperSpeaker() {
    Speaker sp;
    sp.name = str::kHollowKeeper;
    sp.title = str::kHollowKeeperTitle;
    sp.voice = 1;
    sp.pitch = 1.4f;
    sp.tint = {84, 116, 160};
    return sp;
}

void keeperSays(App& app, const char* line) {
    Talk t;
    t.lines[t.count++] = line;
    t.person = story::kPTove;
    startSpeech(app, keeperSpeaker(), t);
}

void finishFloor(App& app, battle::Outcome o, bview::Results& out);
void floorDone(App& app, battle::Outcome o);

// A floor's battle: the wild one comes out of the cave door; you and your partner on the bowl's
// floor facing it.
bool startFloor(App& app, vext::Stage& st, int floor) {
    Hollow& s = ho();
    if (!st.valley || !st.valley->place(kPlaceHollow) || st.partner < 0 || st.partner >= app.game.dragonCount) return false;
    const Valley& v = *st.valley;
    s.floor = floor < 1 ? 1 : (floor > hollow::kFloors ? hollow::kFloors : floor);
    s.partner = st.partner;
    s.wild = hollow::wildOf(s.floor, dayIndex(nowLocal(app)));
    bview::Setup setup;
    setup.partner = st.partner;
    setup.foe = s.wild;
    const char* kind = kindInfo(s.wild.kind).title;
    std::snprintf(setup.foeName, sizeof(setup.foeName), str::kBattleWildName, kind);
    if (s.wild.name[0]) std::snprintf(setup.foeName, sizeof(setup.foeName), "%.23s", s.wild.name);  // (the Frost Warden, D137)
    if (s.floor == hollow::kFloors) std::snprintf(setup.intro, sizeof(setup.intro), "%s", str::kBattleWarden);
    else if (hollow::guardian(s.floor)) std::snprintf(setup.intro, sizeof(setup.intro), "%s", str::kBattleGuardian);
    else std::snprintf(setup.intro, sizeof(setup.intro), str::kBattleWildAppears, kind);
    setup.skill = hollow::skillAt(s.floor);
    setup.foeScale = s.floor == hollow::kFloors ? 1.35f : hollow::guardian(s.floor) ? 1.15f : 1.0f;
    const Dragon& mine = app.game.dragons[st.partner];
    const float sizeYou = bview::dragonSize(mine), sizeFoe = bview::dragonSize(s.wild) * setup.foeScale;
    Vec2 arena = hollow::arenaSpot();
    arena.x += 1.5f;  // (a step from Tove's camp: over your right shoulder the camera clears her tent)
    // Your dragon toward the mouth, the wild one toward the door, a clear gap more apart than
    // their reaches (the biggest kinds too); you behind yours.
    const bool small = mine.stage != Stage::Adult;
    const float palY = 0.5f * bview::kStandingGap + bview::kReachPerSize * (small ? 0.5f : sizeYou);
    const float foeY = 0.5f * bview::kStandingGap + bview::kReachPerSize * sizeFoe;
    setup.youAt = at3(v, {arena.x, arena.y + palY + 1.2f + 1.3f * sizeYou});
    setup.palAt = small ? at3(v, {arena.x + 1.3f, arena.y + palY + 1.0f}) : at3(v, {arena.x, arena.y + palY});
    setup.foeAt = at3(v, {arena.x, arena.y - foeY});
    // The camera over whichever shoulder has a clear view (the spires ring the bowl, Tove's camp
    // by the corridor); the right if both do (your dragon to the left, clear of its bars).
    const std::vector<Solid> solids = worldSolids(v);
    setup.camReach = 0.74f;  // (nearer: the spires ring the bowl at 13 m, the eye stays ~3 m inside them)
    setup.camSide = 1.0f;
    for (float sideSign : {1.0f, -1.0f}) {
        bview::Setup c = setup;
        c.camSide = sideSign;
        Vec3 eye, target;
        bview::cameraFor(c, c.youAt, c.palAt, c.foeAt, std::fmax(sizeYou, sizeFoe), eye, target);
        if (bview::viewClear(v, solids, eye, target)) {
            setup.camSide = sideSign;
            break;
        }
    }
    setup.foeFrom = at3(v, hollow::wildDoor());
    setup.finish = finishFloor;
    setup.done = floorDone;
    s.mode = Mode::Battle;
    bview::start(app, setup);
    return true;
}

void finishFloor(App& app, battle::Outcome o, bview::Results& out) {
    Hollow& s = ho();
    if (s.partner < 0 || s.partner >= app.game.dragonCount) return;
    Dragon& d = app.game.dragons[s.partner];
    const int deepestBefore = d.frostDeepest;
    const hollow::Reward r = hollow::record(app.game, s.partner, s.floor, s.wild, o, dayIndex(nowLocal(app)), app.rng);
    if (o == battle::Outcome::Won) {
        std::snprintf(out.title, sizeof(out.title), str::kHollowCleared, s.floor);
        audio::playSfx(audio::Sfx::IceCrack, 1.1f, 0.6f);
    }
    if (r.growth.xp) out.add(str::kBattleXp, d.name, static_cast<unsigned long>(r.growth.xp));
    if (r.growth.levelAfter > r.growth.levelBefore) {
        out.add(str::kBattleLevelUp, d.name, r.growth.levelAfter);
        queueToastf(app, "%s", out.lines[out.count - 1]);  // (a toast too, as it grows)
        out.levelUp = true;
    }
    u8 four[kMoveSlots];
    battle::equippedMoves(d, four);
    for (int i = 0; i < r.growth.learnedCount; ++i) {
        bool equipped = false;
        for (u8 m : four) equipped |= m == r.growth.learned[i];
        if (equipped) out.add(str::kBattleLearned, d.name, battle::moveName(r.growth.learned[i]));
        else out.add(str::kBattleLearnedSwap, battle::moveName(r.growth.learned[i]));
    }
    if (r.gleam) {
        out.add(str::kBattleGleam, static_cast<unsigned long>(r.gleam));
        audio::playSfx(audio::Sfx::Coin, 1.0f, 0.6f);
    }
    if (r.trained >= 0) {
        out.add(str::kHollowTrained, d.name, str::kBattleStatNames[r.trained]);
        audio::playSfx(audio::Sfx::StatUp, 1.1f);
    }
    if (o == battle::Outcome::Won && hollow::guardian(s.floor) && d.frostDeepest > deepestBefore && s.floor < hollow::kFloors)
        out.add(str::kHollowCheckpoint, s.floor + 1);
    if (r.prizeFood != 0xFF)
        out.add(str::kBattlePrize, r.prizeCount, foodInfo(static_cast<Food>(r.prizeFood)).name,
                trinketName(static_cast<Trinket>(r.prizeTrinket)));
    if (r.accessory >= 0) out.add(str::kShowPrize, accessoryInfo(r.accessory).name);
    if (o == battle::Outcome::Won && s.floor == hollow::kFloors) out.add("%s", str::kHollowBottom);
    saveNow(app);
}

void floorDone(App& app, battle::Outcome o) {
    Hollow& s = ho();
    if (o == battle::Outcome::Won && s.floor < hollow::kFloors) {
        s.mode = Mode::Between;  // deeper, or leave
        s.t = 0;
        s.nextKind = hollow::wildOf(s.floor + 1, dayIndex(nowLocal(app))).kind;
        return;
    }
    s.mode = Mode::None;
    if (!bview::autoplay()) keeperSays(app, o == battle::Outcome::Won ? str::kHollowBottom : str::kHollowLost);
}

// Deeper: another floor (if it has the Energy).
bool deeper(App& app, vext::Stage& st, int floor) {
    Hollow& s = ho();
    Dragon& d = app.game.dragons[st.partner];
    if (!trainer::spendEnergy(d, trainer::kEnergyBattle)) {
        s.mode = Mode::None;
        audio::playSfx(audio::Sfx::Error);
        keeperSays(app, str::kHollowTooTired);
        return false;
    }
    if (!startFloor(app, st, floor)) {
        d.needs.energy = std::fmin(100.0f, d.needs.energy + trainer::kEnergyBattle);
        s.mode = Mode::None;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------- the feature
int folk(const App& app, const Valley& v, Vec3 near, float radius, vext::Folk* out, int cap) {
    if (cap < 1 || !v.place(kPlaceHollow)) return 0;
    story::Spot away;  // (the story has her elsewhere: at the Vault after your first glide, D137)
    if (story::spotOf(app.game, story::kPTove, nowLocal(app), away)) return 0;
    vext::Folk& f = out[0];
    f.look = keeperLook(v);
    if (std::hypot(f.look.at.x - near.x, f.look.at.y - near.y) > radius) return 0;
    f.shown = true;
    f.name = str::kHollowKeeper;
    f.prompt = str::kPromptHollow;
    f.id = 0;
    f.voice = 1;
    f.pitch = 1.4f;
    f.person = story::kPTove;
    f.reach = 2.6f;
    f.clip = "sit_ground";  // (sat by her camp, getting up to wave as you come: workstream D)
    return 1;
}

void act(App& app, const vext::Folk& who, vext::Stage& st) {
    showTip(app, tips::kTipHollow);
    Hollow& s = ho();
    (void)who;
    if (story::hasImportantTalk(app.game, story::kPTove, nowLocal(app))) {  // the story's Tove first (D137)
        startStoryTalk(app, story::kPTove);
    } else {
        Talk t;
        if (app.game.progress.hollowDeepest == 0) {
            for (const char* line : str::kHollowFirst) t.lines[t.count++] = line;
        } else {
            t.lines[t.count++] = str::kHollowAgain[app.rng.below(3)];
        }
        t.person = story::kPTove;  // (her story self: face, portrait, D138)
        startSpeech(app, keeperSpeaker(), t);
    }
    if (st.partner < 0) return;  // (out alone: just her words)
    s.nextKind = hollow::wildOf(1, dayIndex(nowLocal(app))).kind;
    s.mode = Mode::Gate;
    s.picked = 0;
    s.t = 0;
}

bool active(const App& app) {
    (void)app;
    return ho().mode != Mode::None || ho().pendingFloor > 0 || ho().pendingKeeper;
}

void update(App& app, const Input& in, vext::Stage& st) {
    Hollow& s = ho();
    s.t += app.dt;
    if (s.pendingKeeper) {  // a scripted run: up to Tove, and A
        s.pendingKeeper = false;
        if (st.valley && st.valley->place(kPlaceHollow)) {
            const r3d::PersonView tove = keeperLook(*st.valley);
            const Vec3 fwd{std::sin(tove.heading), -std::cos(tove.heading), 0};
            st.you = tove.at + fwd * 2.0f;
            st.youHeading = std::atan2(tove.at.x - st.you.x, -(tove.at.y - st.you.y));
            st.pal = st.you + Vec3{fwd.y, -fwd.x, 0} * 1.8f;
            act(app, vext::Folk{}, st);
        }
        return;
    }
    if (s.pendingFloor > 0) {  // a scripted run: straight in (Energy aside)
        const int floor = s.pendingFloor;
        s.pendingFloor = 0;
        if (!startFloor(app, st, floor)) s.mode = Mode::None;
        return;
    }
    switch (s.mode) {
        case Mode::Gate:
            if (st.valley && st.valley->place(kPlaceHollow)) {  // you and Tove side on, your dragon beyond you
                const r3d::PersonView tove = keeperLook(*st.valley);
                Vec3 fwd{tove.at.x - st.you.x, tove.at.y - st.you.y, 0};
                const float len = std::hypot(fwd.x, fwd.y);
                fwd = len > 0.01f ? fwd * (1.0f / len) : Vec3{0, 1, 0};
                Vec3 side{fwd.y, -fwd.x, 0};
                if ((st.pal.x - st.you.x) * side.x + (st.pal.y - st.you.y) * side.y > 0) side = side * -1.0f;
                const Vec3 mid = lerp(st.you, tove.at, 0.5f);
                st.camSet = true;
                st.eye = mid + side * 4.6f - fwd * 0.8f + Vec3{0, 0, 1.9f};
                st.target = mid + Vec3{0, 0, 1.1f};
            }
            if (in.down & KEY_B) {
                s.mode = Mode::None;
                audio::playSfx(audio::Sfx::Back);
            } else if (in.down & KEY_A) {
                int cps[hollow::kCheckpoints];
                const int n = hollow::checkpoints(app.game.dragons[st.partner], cps);
                deeper(app, st, cps[s.picked < n ? s.picked : 0]);
            } else if (in.down & (KEY_DLEFT | KEY_DRIGHT)) {
                int cps[hollow::kCheckpoints];
                const int n = hollow::checkpoints(app.game.dragons[st.partner], cps);
                s.picked = (s.picked + (in.down & KEY_DRIGHT ? 1 : n - 1)) % n;
                s.nextKind = hollow::wildOf(cps[s.picked], dayIndex(nowLocal(app))).kind;
                audio::playSfx(audio::Sfx::Tap);
            }
            break;
        case Mode::Battle:
            bview::update(app, in, st);
            if (!bview::running() && s.mode == Mode::Battle) s.mode = Mode::None;
            break;
        case Mode::Between:
            st.camSet = true;  // (held where the battle left it)
            bview::lastCamera(st.eye, st.target);
            if (in.down & KEY_A) {
                deeper(app, st, s.floor + 1);
            } else if (in.down & KEY_B) {
                s.mode = Mode::None;
                audio::playSfx(audio::Sfx::Back);
                keeperSays(app, str::kHollowLeft);
            }
            break;
        case Mode::None: break;
    }
}

void view(App& app, const vext::Stage& st, r3d::ValleyView& view) {
    Hollow& s = ho();
    if (s.mode == Mode::Battle) bview::view(app, st, view);
    if ((s.mode == Mode::Gate || s.mode == Mode::Between) && s.nextKind >= 0) r3d::wantKind(s.nextKind);
    if (s.mode == Mode::Battle || s.mode == Mode::Between) {  // colder and darker, deeper down
        view.fog = hollow::chill(view.fog, s.floor);
        view.tint = hollow::chill(view.tint, s.floor);
    }
}

void drawTop(App& app, const vext::Stage& st) {
    Hollow& s = ho();
    (void)st;
    if (s.mode == Mode::Battle || s.mode == Mode::Between) {  // the cold over it all, and the floor
        const float cold = hollow::depth(s.floor);
        if (cold > 0.01f) C2D_DrawRectSolid(0, 0, 0, kTopW, kScreenH, withAlpha(theme::rgba(40, 60, 100), 0.22f * cold));
    }
    if (s.mode == Mode::Battle) bview::drawTop(app);
    if (s.mode == Mode::Battle || s.mode == Mode::Between) {
        char line[24];
        std::snprintf(line, sizeof(line), str::kHollowFloorOf, s.floor, hollow::kFloors);
        panel({300, 8, 92, 20}, withAlpha(theme::kDenPlum, 0.75f));  // (the top's right corner: the foe's bars at its left)
        textCentered(app, line, 346, 18, 0.4f, hollow::guardian(s.floor) ? theme::kClutchGold : theme::kShell, 88);
    }
}

void drawGate(App& app, const Input& in, const vext::Stage& st) {
    Hollow& s = ho();
    textCentered(app, str::kHollowTitle, 160, 16, 0.62f, theme::kClutchGold, 300, Face::Title);
    if (st.partner < 0 || st.partner >= app.game.dragonCount) return;
    const Dragon& d = app.game.dragons[st.partner];
    char line[80];
    std::snprintf(line, sizeof(line), str::kHollowDeepestOf, d.name, d.frostDeepest);
    textCentered(app, line, 160, 40, 0.44f, theme::kShell, 300);
    std::snprintf(line, sizeof(line), str::kHollowAllDeepest, app.game.progress.hollowDeepest);
    textCentered(app, line, 160, 56, 0.38f, withAlpha(theme::kShell, 0.7f), 300);
    textCentered(app, str::kHollowPick, 160, 80, 0.46f, theme::kShell, 300);
    int cps[hollow::kCheckpoints];
    const int n = hollow::checkpoints(d, cps);
    for (int i = 0; i < n; ++i) {  // the checkpoints, as stones to step onto
        const float x = 160 + (i - (n - 1) * 0.5f) * 50.0f, y = 116;
        const bool on = s.picked == i;
        if (on) C2D_DrawCircleSolid(x, y, 0, 23, theme::kClutchGold);
        C2D_DrawCircleSolid(x, y, 0, 20, fromRgb(hollow::chill({200, 226, 246}, cps[i])));
        std::snprintf(line, sizeof(line), "%d", cps[i]);
        textCentered(app, line, x, y, 0.55f, theme::kDenPlum, 36);
        if (in.released && std::hypot(in.rx - x, in.ry - y) < 22) {
            s.picked = i;
            s.nextKind = hollow::wildOf(cps[i], dayIndex(nowLocal(app))).kind;
            audio::playSfx(audio::Sfx::Tap);
        }
    }
    std::snprintf(line, sizeof(line), str::kBattleAskEnergy, static_cast<int>(d.needs.energy + 0.5f), static_cast<int>(trainer::kEnergyBattle));
    textCentered(app, line, 160, 150, 0.4f, trainer::canSpend(d, trainer::kEnergyBattle) ? withAlpha(theme::kShell, 0.75f) : theme::kRose, 300);
    if (button(app, {24, 176, 128, 48}, str::kHollowLeave, in)) {
        s.mode = Mode::None;
        audio::playSfx(audio::Sfx::Back);
    } else if (button(app, {168, 176, 128, 48}, str::kHollowGoDeeper, in, theme::kClutchGold)) {
        vext::Stage copy = st;
        deeper(app, copy, cps[s.picked < n ? s.picked : 0]);
    }
}

void drawBetween(App& app, const Input& in, const vext::Stage& st) {
    Hollow& s = ho();
    char line[64];
    std::snprintf(line, sizeof(line), str::kHollowCleared, s.floor);
    textCentered(app, line, 160, 20, 0.62f, theme::kClutchGold, 300, Face::Title);
    std::snprintf(line, sizeof(line), str::kHollowNext, s.floor + 1);
    textCentered(app, line, 160, 64, 0.5f, hollow::guardian(s.floor + 1) ? theme::kClutchGold : theme::kShell, 300);
    if (hollow::guardian(s.floor + 1)) textCentered(app, str::kBattleGuardian, 160, 84, 0.42f, theme::kClutchGold, 300);
    if (st.partner >= 0 && st.partner < app.game.dragonCount) {
        const Dragon& d = app.game.dragons[st.partner];
        std::snprintf(line, sizeof(line), str::kBattleAskEnergy, static_cast<int>(d.needs.energy + 0.5f),
                      static_cast<int>(trainer::kEnergyBattle));
        textCentered(app, line, 160, 120, 0.4f,
                     trainer::canSpend(d, trainer::kEnergyBattle) ? withAlpha(theme::kShell, 0.75f) : theme::kRose, 300);
    }
    if (button(app, {24, 176, 128, 48}, str::kHollowLeave, in)) {
        s.mode = Mode::None;
        audio::playSfx(audio::Sfx::Back);
        keeperSays(app, str::kHollowLeft);
    } else if (button(app, {168, 176, 128, 48}, str::kHollowGoDeeper, in, theme::kClutchGold)) {
        vext::Stage copy = st;
        deeper(app, copy, s.floor + 1);
    }
}

void drawBottom(App& app, const Input& in, const vext::Stage& st) {
    Hollow& s = ho();
    if (s.mode == Mode::Battle) {
        bview::drawBottom(app, in);
        return;
    }
    verticalGradient(0, 0, kBotW, kScreenH, fromRgb(hollow::chill({94, 68, 102}, s.floor)), theme::kDenPlum);
    if (s.mode == Mode::Gate) drawGate(app, in, st);
    else if (s.mode == Mode::Between) drawBetween(app, in, st);
}

}  // namespace

const vext::Feature kHollowFeature{"hollow", folk, act, active, update, view, drawTop, drawBottom};

void startHollowKeeper(App& app) {
    (void)app;
    ho().pendingKeeper = true;
}

void startHollowFloor(App& app, int floor) {
    (void)app;
    ho().pendingFloor = floor < 1 ? 1 : (floor > hollow::kFloors ? hollow::kFloors : floor);
}

}  // namespace ec
