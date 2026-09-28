// The battle league in the valley (1.0, D90): the current league's four challengers stand about
// the valley at their places (their spots in the places' frames), each league's champion at
// Emberpeak Caldera, and the league's board by the arena's gate and at the caldera. A beside a
// challenger: their lines, then "Battle?" on the bottom screen; the battle is right there
// (app/battle_view), a champion's on the caldera's ring. Afterwards: experience, Gleam, the
// dragon's record and title, the league's progress (core/league), and their parting words.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "app/audio.hpp"
#include "app/battle_feature.hpp"
#include "app/battle_view.hpp"
#include "app/dialogue.hpp"
#include "app/render3d.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/care.hpp"
#include "core/challenge_mesh.hpp"
#include "core/clock.hpp"
#include "core/league.hpp"
#include "core/people.hpp"
#include "core/place_layout.hpp"
#include "core/trainer.hpp"
#include "core/valley.hpp"
#include "core/villagers.hpp"
#include "core/wanderings.hpp"
#include "core/world.hpp"

namespace ec {
namespace {

constexpr u8 kBoardFolk = 100;  // the boards' ids among the feature's folk (the challengers: 0 .. 19)
constexpr float kPi = 3.14159265f;

enum class Mode : u8 { None, Ask, Battle, Board };

struct League {
    Mode mode = Mode::None;
    int id = -1;           // the challenger
    int board = 0;         // the board being read
    int partner = -1;      // your dragon in the battle (app.game.dragons)
    int pendingStart = -1; // a scripted run's battle, set up on the next update
    int pendingTalk = -1;  // ...or walking up to a challenger and pressing A
    bool pendingBoard = false;
    float t = 0;
    std::vector<Solid> solids;  // the places' walls and the villagers, for staging (built once)
    const Valley* solidsOf = nullptr;
};

League& lg() {
    static League s;
    return s;
}


Vec3 at3(const Valley& v, int place, Vec2 local) {
    const ValleyPlaceInfo* p = v.place(static_cast<u8>(place));
    return p ? placeToWorld3(v, *p, {local.x, local.y, 0}) : Vec3{};
}

r3d::PersonView lookOf(const Valley& v, int id) {
    const league::Challenger& c = league::challenger(id);
    r3d::PersonView p;
    p.form = static_cast<u8>(c.look.body ? Person::PlayerB : Person::PlayerA);
    league::palette(c.look, p.pal);
    p.hair = static_cast<s8>(c.look.hair);
    p.at = at3(v, c.place, league::spotOf(id));
    const ValleyPlaceInfo* place = v.place(c.place);
    p.heading = (place ? place->heading : 0.0f) + league::facingOf(id);
    return p;
}

Speaker speakerOf(int id) {
    const league::Challenger& c = league::challenger(id);
    Speaker sp;
    sp.name = c.name;
    sp.title = c.title;
    sp.voice = c.voice;
    sp.pitch = c.pitch;
    sp.tint = c.look.outfit;
    return sp;
}

void say(App& app, int id, const char* a, const char* b = nullptr) {
    Talk t;
    t.lines[t.count++] = a;
    if (b) t.lines[t.count++] = b;
    startSpeech(app, speakerOf(id), t);
}

// ---- Staging a battle where you stand: the challenger stays at their spot; you step back along
// the way you came to about eleven metres off, on good ground clear of walls (turning the line a
// little if it isn't); your dragon before you, theirs before them.
const std::vector<Solid>& solidsFor(const Valley& v) {
    League& s = lg();
    if (s.solidsOf != &v) {
        s.solids = worldSolids(v);
        for (int k = 0; k < kVillagers; ++k) {
            const VillagerInfo& info = villagerInfo(static_cast<Villager>(k));
            s.solids.push_back({{at3(v, info.place, info.at).x, at3(v, info.place, info.at).y}, 0.6f});
        }
        s.solidsOf = &v;
    }
    return s.solids;
}

bool clear(const std::vector<Solid>& solids, Vec2 at, float r) {
    for (const Solid& w : solids)
        if (std::hypot(at.x - w.at.x, at.y - w.at.y) < w.radius + r) return false;
    return true;
}

bool placeBattle(const Valley& v, Vec3 chAt, Vec3 you, float heading, float sizeYou, float sizeFoe, bool small, bview::Setup& out) {
    const std::vector<Solid>& solids = solidsFor(v);
    Vec2 dir{you.x - chAt.x, you.y - chAt.y};
    float d = std::hypot(dir.x, dir.y);
    if (d < 0.5f) dir = {std::sin(heading), -std::cos(heading)}, d = 1;
    dir = {dir.x / d, dir.y / d};
    static constexpr float kTurns[] = {0.0f, 0.45f, -0.45f, 0.9f, -0.9f, 1.4f, -1.4f, 2.0f, -2.0f, kPi};
    for (float dist : {11.0f, 9.5f, 8.0f})
        for (float turn : kTurns) {
            const Vec2 w{dir.x * std::cos(turn) - dir.y * std::sin(turn), dir.x * std::sin(turn) + dir.y * std::cos(turn)};
            const Vec2 side{w.y, -w.x};
            const Vec2 youAt{chAt.x + w.x * dist, chAt.y + w.y * dist};
            const float palOut = small ? 1.0f : 1.2f + 1.3f * sizeYou;
            const Vec2 palAt{youAt.x - w.x * palOut + (small ? side.x * 1.3f : 0), youAt.y - w.y * palOut + (small ? side.y * 1.3f : 0)};
            const float foeOut = 1.2f + 1.3f * sizeFoe;
            const Vec2 foeAt{chAt.x + w.x * foeOut, chAt.y + w.y * foeOut};
            if (!bview::goodGround(v, chAt, youAt) || !bview::goodGround(v, chAt, palAt) || !bview::goodGround(v, chAt, foeAt)) continue;
            if (!clear(solids, youAt, 0.6f) || !clear(solids, palAt, 0.9f * sizeYou) || !clear(solids, foeAt, 0.9f * sizeFoe)) continue;
            out.youAt = {youAt.x, youAt.y, v.heightAt(youAt.x, youAt.y)};
            out.palAt = {palAt.x, palAt.y, v.heightAt(palAt.x, palAt.y)};
            out.foeAt = {foeAt.x, foeAt.y, v.heightAt(foeAt.x, foeAt.y)};
            out.foeFrom = {chAt.x - side.x * 1.6f, chAt.y - side.y * 1.6f, 0};
            out.foeFrom.z = v.heightAt(out.foeFrom.x, out.foeFrom.y);
            // The camera over whichever shoulder is clear (else the right, and it rises above the ground).
            out.camSide = 1.0f;
            for (float sideSign : {1.0f, -1.0f}) {
                out.camSide = sideSign;
                Vec3 eye, target;
                bview::cameraFor(out, out.youAt, out.palAt, out.foeAt, std::fmax(sizeYou, sizeFoe), eye, target);
                if (clear(solids, {eye.x, eye.y}, 1.2f) && v.heightAt(eye.x, eye.y) < eye.z - 1.2f) break;
                out.camSide = 1.0f;
            }
            return true;
        }
    return false;
}

void finishBattle(App& app, battle::Outcome o, bview::Results& out);
void battleDone(App& app, battle::Outcome o);

// Sets up and starts a challenger's battle (after "Battle?", or a scripted run's).
bool beginBattle(App& app, vext::Stage& st, int id) {
    League& s = lg();
    if (!st.valley || st.partner < 0 || st.partner >= app.game.dragonCount) return false;
    const Valley& v = *st.valley;
    const league::Challenger& c = league::challenger(id);
    bview::Setup setup;
    setup.partner = st.partner;
    setup.foe = league::dragonOf(id);
    std::snprintf(setup.foeName, sizeof(setup.foeName), "%s", c.dragonName);
    std::snprintf(setup.intro, sizeof(setup.intro), str::kBattleWantsTo, c.name);
    setup.skill = c.skill;
    setup.trainer = true;
    setup.trainerLook = lookOf(v, id);
    const Dragon& mine = app.game.dragons[st.partner];
    const float sizeYou = bview::dragonSize(mine), sizeFoe = bview::dragonSize(setup.foe);
    const bool small = mine.stage != Stage::Adult;
    if (league::isChampion(id)) {  // the final: on the caldera's ring, you on its near side
        const Vec3 you = at3(v, kPlaceCaldera, league::ringSide(0)), them = at3(v, kPlaceCaldera, league::ringSide(1));
        setup.trainerLook.at = them;
        if (!placeBattle(v, them, you, 0, sizeYou, sizeFoe, small, setup)) return false;
    } else if (!placeBattle(v, setup.trainerLook.at, st.you, st.youHeading, sizeYou, sizeFoe, small, setup)) {
        return false;
    }
    setup.finish = finishBattle;
    setup.done = battleDone;
    s.id = id;
    s.partner = st.partner;
    s.mode = Mode::Battle;
    bview::start(app, setup);
    return true;
}

// ---- After the battle: the record, and the card's lines.
void finishBattle(App& app, battle::Outcome o, bview::Results& out) {
    League& s = lg();
    const int index = s.partner;
    if (index < 0 || index >= app.game.dragonCount) return;
    Dragon& d = app.game.dragons[index];
    const league::Reward r = league::record(app.game, index, s.id, o, dayIndex(nowLocal(app)));
    if (r.growth.xp) out.add(str::kBattleXp, d.name, static_cast<unsigned long>(r.growth.xp));
    if (r.growth.levelAfter > r.growth.levelBefore) {
        out.add(str::kBattleLevelUp, d.name, r.growth.levelAfter);
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
        audio::playSfx(audio::Sfx::Coin, 1.0f, 0.7f);
    } else if (r.paidBefore) {
        out.add("%s", str::kBattlePaid);
    }
    if (r.title) out.add(str::kBattleTitle, d.name, trainer::battleTitleName(d.battleTitle));
    if (r.prizeFood != 0xFF)
        out.add(str::kBattlePrize, r.prizeCount, foodInfo(static_cast<Food>(r.prizeFood)).name,
                trinketName(static_cast<Trinket>(r.prizeTrinket)));
    if (r.championOpened) {
        out.add("%s", str::kBattleChampionOpen);
        audio::playSfx(audio::Sfx::Unlock);
    }
    if (r.leagueWon) {
        audio::playStinger("cup-won");
        if (app.game.progress.battleLeague < kLeagues)
            out.add(str::kBattleNextLeague, trainer::leagueName(app.game.progress.battleLeague + 1));
        else
            out.add("%s", str::kBattleAllWon);
    }
    saveNow(app);
}

void battleDone(App& app, battle::Outcome o) {
    League& s = lg();
    const int id = s.id;
    s.mode = Mode::None;
    s.id = -1;
    if (bview::autoplay() || id < 0) return;
    const league::Challenger& c = league::challenger(id);
    say(app, id, o == battle::Outcome::Won ? c.beaten : c.victory);
}

// ---------------------------------------------------------------------------- the feature
int folk(const App& app, const Valley& v, Vec3 near, float radius, vext::Folk* out, int cap) {
    const League& s = lg();
    int n = 0;
    const int league = league::currentLeague(app.game);
    for (int slot = 0; slot < league::kSlots && n < cap; ++slot) {
        const int id = league::idOf(league, slot);
        if (s.mode == Mode::Battle && s.id == id) continue;  // (the battle draws them)
        const league::Challenger& c = league::challenger(id);
        if (!v.place(c.place)) continue;
        vext::Folk& f = out[n];
        f.look = lookOf(v, id);
        if (std::hypot(f.look.at.x - near.x, f.look.at.y - near.y) > radius) continue;
        f.shown = true;
        f.name = c.name;
        f.prompt = league::isChampion(id) && !league::championOpen(app.game, league) ? str::kPromptTalk
                   : league::beaten(app.game, id)                                    ? str::kPromptRematch
                                                                                     : str::kPromptBattle;
        f.id = static_cast<u8>(id);
        f.voice = c.voice;
        f.pitch = c.pitch;
        f.reach = 2.6f;
        ++n;
    }
    for (int b = 0; b < league::boardCount() && n < cap; ++b) {  // the boards: spots, drawn as props
        const league::BoardSpot spot = league::board(b);
        if (!v.place(spot.place)) continue;
        vext::Folk& f = out[n];
        f = vext::Folk{};
        f.look.at = at3(v, spot.place, spot.at);
        if (std::hypot(f.look.at.x - near.x, f.look.at.y - near.y) > radius) continue;
        f.shown = false;
        f.name = str::kLeagueBoardName;
        f.prompt = str::kPromptRead;
        f.id = static_cast<u8>(kBoardFolk + b);
        f.reach = 3.0f;
        ++n;
    }
    return n;
}

void act(App& app, const vext::Folk& who, vext::Stage& st) {
    League& s = lg();
    if (who.id >= kBoardFolk) {
        s.mode = Mode::Board;
        s.board = who.id - kBoardFolk;
        s.t = 0;
        audio::playSfx(audio::Sfx::QuestPage);
        return;
    }
    const int id = who.id;
    const league::Challenger& c = league::challenger(id);
    const int league = league::leagueOf(id);
    if (league::isChampion(id) && !league::championOpen(app.game, league)) {
        say(app, id, str::kChampionWaits);  // (their league's name comes from the board)
        return;
    }
    if (st.partner < 0) {
        say(app, id, str::kBattleNoPartner);
        return;
    }
    if (league::beaten(app.game, id)) say(app, id, c.again);
    else say(app, id, c.hello[0], c.hello[1]);
    s.mode = Mode::Ask;
    s.id = id;
    s.t = 0;
}

bool active(const App& app) {
    const League& s = lg();
    (void)app;
    return s.mode != Mode::None || s.pendingStart >= 0 || s.pendingTalk >= 0 || s.pendingBoard;
}

void goBattle(App& app, vext::Stage& st) {
    League& s = lg();
    Dragon& d = app.game.dragons[st.partner];
    if (!trainer::spendEnergy(d, trainer::kEnergyBattle)) {
        s.mode = Mode::None;
        audio::playSfx(audio::Sfx::Error);
        say(app, s.id, str::kBattleTooTired);
        return;
    }
    if (!beginBattle(app, st, s.id)) {  // (nowhere to stand: rare) its Energy back
        d.needs.energy = std::fmin(100.0f, d.needs.energy + trainer::kEnergyBattle);
        s.mode = Mode::None;
    }
}

// A scripted run: you in front of a challenger (`dist` metres), turned to them.
bool standBefore(vext::Stage& st, int id, float dist) {
    if (!st.valley || st.partner < 0) return false;
    const r3d::PersonView them = lookOf(*st.valley, id);
    st.you = them.at + Vec3{std::sin(them.heading), -std::cos(them.heading), 0} * dist;
    st.youHeading = std::atan2(them.at.x - st.you.x, -(them.at.y - st.you.y));
    st.pal = st.you + Vec3{std::sin(them.heading + 1.2f), -std::cos(them.heading + 1.2f), 0} * 1.6f;
    return true;
}

void update(App& app, const Input& in, vext::Stage& st) {
    League& s = lg();
    s.t += app.dt;
    if (s.pendingStart >= 0) {  // a scripted run: straight in, standing by them
        const int id = s.pendingStart;
        s.pendingStart = -1;
        s.id = id;
        if (!standBefore(st, id, 3.0f) || !beginBattle(app, st, id)) s.mode = Mode::None;
        return;
    }
    if (s.pendingTalk >= 0) {  // a scripted run: up to them, and A
        const int id = s.pendingTalk;
        s.pendingTalk = -1;
        if (standBefore(st, id, 2.0f)) {
            vext::Folk who;
            who.id = static_cast<u8>(id);
            act(app, who, st);
        }
        return;
    }
    if (s.pendingBoard) {
        s.pendingBoard = false;
        s.mode = Mode::Board;
        s.board = 0;
        s.t = 0;
    }
    switch (s.mode) {
        case Mode::Ask:
            if (in.down & KEY_A) goBattle(app, st);
            else if (in.down & KEY_B) {
                s.mode = Mode::None;
                audio::playSfx(audio::Sfx::Back);
            }
            break;
        case Mode::Battle:
            bview::update(app, in, st);
            if (!bview::running() && s.mode == Mode::Battle) s.mode = Mode::None;
            break;
        case Mode::Board: {
            if (s.t > 0.3f && (in.down & (KEY_A | KEY_B))) {
                s.mode = Mode::None;
                audio::playSfx(audio::Sfx::Back);
                break;
            }
            // The camera before the board.
            if (!st.valley) break;
            const league::BoardSpot b = league::board(s.board);
            const ValleyPlaceInfo* p = st.valley->place(b.place);
            if (!p) break;
            const float face = p->heading + b.facing;
            const Vec3 at = at3(*st.valley, b.place, b.at), fwd{std::sin(face), -std::cos(face), 0};
            st.camSet = true;
            st.eye = at + fwd * 6.0f + Vec3{0, 0, 2.4f};
            st.target = at + Vec3{0, 0, 1.5f};
            break;
        }
        case Mode::None: break;
    }
}

void view(App& app, const vext::Stage& st, r3d::ValleyView& view) {
    if (lg().mode == Mode::Battle) bview::view(app, st, view);
}

void drawTop(App& app, const vext::Stage& st) {
    League& s = lg();
    (void)st;
    if (s.mode == Mode::Battle) {
        bview::drawTop(app);
        return;
    }
    if (s.mode == Mode::Board) {
        char line[48];
        const int league = league::currentLeague(app.game);
        std::snprintf(line, sizeof(line), str::kBoardTitle, trainer::leagueName(league + 1));
        const float w = textWidth(app, line, 0.7f, Face::Title) + 40;
        panel({200 - w / 2, 10, w, 34}, withAlpha(fromRgb(challenge::cupColour(league + 1)), 0.92f));
        textCentered(app, line, 200, 27, 0.7f, theme::kDenPlum, 380, Face::Title);
    }
}

// The board: the league's four, whether beaten and where they stand; its champion; the wheel.
void drawBoard(App& app, const Input& in) {
    League& s = lg();
    const int league = league::currentLeague(app.game);
    char line[80];
    std::snprintf(line, sizeof(line), str::kBoardTitle, trainer::leagueName(league + 1));
    text(app, line, 12, 6, 0.6f, theme::kClutchGold, C2D_AlignLeft, 200, Face::Title);
    std::snprintf(line, sizeof(line), str::kBoardTitles, app.game.progress.battleLeague);
    text(app, line, 308, 10, 0.4f, withAlpha(theme::kShell, 0.75f), C2D_AlignRight, 120);
    const int tracked = trainer::tracked(app.game) == Tracked::BattleBoard ? app.game.progress.trackId : -1;
    for (int slot = 0; slot < league::kSlots; ++slot) {
        const int id = league::idOf(league, slot);
        const league::Challenger& c = league::challenger(id);
        const bool champion = league::isChampion(id);
        const bool beaten = league::beaten(app.game, id);
        const Rect r{8, 32.0f + slot * 31.0f, 304, 28};
        panel(r, withAlpha(champion ? theme::kEmber : theme::kShell, champion ? 0.3f : 0.14f));
        C2D_DrawCircleSolid(r.x + 14, r.y + 14, 0, 9, fromRgb(c.look.outfit));
        C2D_DrawCircleSolid(r.x + 14, r.y + 11, 0, 5, fromRgb(c.look.skin));
        text(app, c.name, r.x + 30, r.y + 2, 0.44f, theme::kShell, C2D_AlignLeft, 110);
        std::snprintf(line, sizeof(line), "%s, %s Lv %d", c.dragonName, kindInfo(findKind(c.kind) >= 0 ? findKind(c.kind) : 0).title,
                      c.level);
        text(app, line, r.x + 30, r.y + 15, 0.34f, withAlpha(theme::kShell, 0.75f), C2D_AlignLeft, 130);
        if (champion) std::snprintf(line, sizeof(line), "%s", beaten ? str::kBoardWon : league::championOpen(app.game, league) ? str::kBoardOpen : str::kBoardLocked);
        else std::snprintf(line, sizeof(line), str::kBoardAt, world::placeInfo(c.place).name);
        text(app, line, r.x + 166, r.y + 8, 0.36f, withAlpha(theme::kShell, 0.85f), C2D_AlignLeft, 90);
        if (beaten) {
            const Rect tag{r.x + r.w - 52, r.y + 3, 48, 22};
            panel(tag, theme::kClutchGold);
            textCentered(app, champion ? str::kBoardWon : str::kBoardBeaten, tag.x + tag.w / 2, tag.y + 11, 0.34f, theme::kDenPlum,
                         tag.w - 4);
        } else if (champion && !league::championOpen(app.game, league)) {
            // (nothing to track yet)
        } else {
            const Rect tr{r.x + r.w - 52, r.y + 3, 48, 22};
            const bool on = tracked == id;
            panel(tr, on ? theme::kClutchGold : withAlpha(theme::kShell, 0.2f));
            textCentered(app, on ? str::kBoardTracking : str::kBoardTrack, tr.x + tr.w / 2, tr.y + 11, 0.34f,
                         on ? theme::kDenPlum : theme::kShell, tr.w - 4);
            if (in.released && tr.contains(in.rx, in.ry)) {
                trainer::track(app.game, on ? Tracked::None : Tracked::BattleBoard, id);
                audio::playSfx(audio::Sfx::Tap);
            }
        }
    }
    // The wheel, to learn it.
    text(app, str::kBoardWheel, 12, 192, 0.36f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft, 120);
    static constexpr u8 kOrder[6] = {battle::kEmber, battle::kFrost, battle::kStone, battle::kGale, battle::kGrove, battle::kTide};
    for (int i = 0; i < 6; ++i) {
        const float x = 18.0f + i * 22.0f;
        C2D_DrawCircleSolid(x, 214, 0, 6, fromRgb(battle::elementColour(kOrder[i])));
        if (i < 5) C2D_DrawTriangle(x + 9, 211, theme::kShell, x + 14, 214, theme::kShell, x + 9, 217, theme::kShell, 0);
    }
    text(app, battle::elementLabel(kOrder[0]), 12, 222, 0.28f, withAlpha(theme::kShell, 0.7f), C2D_AlignLeft, 40);
    text(app, str::kBoardPair, 12, 180, 0.3f, withAlpha(theme::kShell, 0.6f), C2D_AlignLeft, 200);
    if (button(app, {208, 196, 104, 36}, str::kDone, in, theme::kClutchGold)) {
        s.mode = Mode::None;
        audio::playSfx(audio::Sfx::Back);
    }
}

// "Battle?": who they are and what their dragon is, yours, its Energy.
void drawAsk(App& app, const Input& in, const vext::Stage& st) {
    League& s = lg();
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    if (s.id < 0) return;
    const league::Challenger& c = league::challenger(s.id);
    char line[80];
    std::snprintf(line, sizeof(line), str::kBattleAskTitle, c.name);
    textCentered(app, line, 160, 22, 0.66f, theme::kClutchGold, 300, Face::Title);
    const int kind = findKind(c.kind) >= 0 ? findKind(c.kind) : 0;
    std::snprintf(line, sizeof(line), str::kBattleAskTheirs, c.dragonName, kindInfo(kind).title, c.level);
    textCentered(app, line, 160, 56, 0.46f, theme::kShell, 300);
    char elements[32];
    kindElements(kind, elements, sizeof(elements));
    textCentered(app, elements, 160, 74, 0.4f, withAlpha(theme::kShell, 0.75f), 300);
    if (st.partner >= 0 && st.partner < app.game.dragonCount) {
        const Dragon& d = app.game.dragons[st.partner];
        std::snprintf(line, sizeof(line), str::kBattleAskYours, d.name, trainer::levelOf(d));
        textCentered(app, line, 160, 102, 0.46f, theme::kShell, 300);
        std::snprintf(line, sizeof(line), str::kBattleAskEnergy, static_cast<int>(d.needs.energy + 0.5f),
                      static_cast<int>(trainer::kEnergyBattle));
        textCentered(app, line, 160, 122, 0.4f,
                     trainer::canSpend(d, trainer::kEnergyBattle) ? withAlpha(theme::kShell, 0.75f) : theme::kRose, 300);
    }
    if (button(app, {24, 170, 128, 48}, str::kBattleAskNot, in)) {
        s.mode = Mode::None;
        audio::playSfx(audio::Sfx::Back);
    } else if (button(app, {168, 170, 128, 48}, str::kBattleAskGo, in, theme::kClutchGold)) {
        vext::Stage copy = st;
        goBattle(app, copy);  // (the battle takes its places from the stage on its first update)
    }
}

void drawBottom(App& app, const Input& in, const vext::Stage& st) {
    League& s = lg();
    switch (s.mode) {
        case Mode::Battle: bview::drawBottom(app, in); break;
        case Mode::Ask: drawAsk(app, in, st); break;
        case Mode::Board:
            verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
            drawBoard(app, in);
            break;
        case Mode::None: verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum); break;
    }
}

}  // namespace

const vext::Feature kLeagueFeature{"league", folk, act, active, update, view, drawTop, drawBottom};

void drawLeagueBoards(App& app, const Valley& v, s64 now) {
    r3d::ChallengeProp props[2];
    int n = 0;
    bool near = false;
    const int league = league::currentLeague(app.game);
    for (int b = 0; b < league::boardCount() && n < 2; ++b) {
        const league::BoardSpot spot = league::board(b);
        const ValleyPlaceInfo* p = v.place(spot.place);
        if (!p) continue;
        r3d::ChallengeProp& prop = props[n++];
        prop.kind = r3d::PropKind::Board;
        prop.at = placeToWorld3(v, *p, {spot.at.x, spot.at.y, 0});
        prop.yaw = p->heading + spot.facing + kPi;  // its posters to its front
        // The league's colours: dark wood, an ember roof, the league's colour in its posters.
        const Rgb cup = challenge::cupColour(league + 1);
        prop.look.colour[0] = {104, 70, 50};
        prop.look.colour[1] = {196, 84, 60};
        prop.look.colour[2] = {250, 240, 220};
        prop.look.colour[3] = cup;
        float x, y, ppu;
        near = near || (r3d::project(prop.at, x, y, ppu) && ppu > 0.4f);
    }
    if (!near || n == 0) return;
    Rgb top, horizon, tint;
    valleySky(now, top, horizon, tint);
    r3d::drawChallengeProps(app, props, n, horizon, now);
}

const char* battleMusic(const App& app) {
    (void)app;
    return bview::running() ? "cup-day" : nullptr;
}

void battleCommand(App& app, const char* args) {
    char word[16] = {};
    int a = 0, b = 0;
    const int n = std::sscanf(args, " %15s %d %d", word, &a, &b);
    if (n < 1) return;
    League& s = lg();
    if (std::strcmp(word, "start") == 0) {
        s.pendingStart = league::idOf(a, b);
    } else if (std::strcmp(word, "talk") == 0) {
        s.pendingTalk = league::idOf(a, b);
    } else if (std::strcmp(word, "hollow") == 0) {
        startHollowFloor(app, a);
    } else if (std::strcmp(word, "board") == 0) {
        s.pendingBoard = true;
    } else if (std::strcmp(word, "level") == 0) {
        for (int i = 0; i < app.game.dragonCount; ++i)
            if (app.game.dragons[i].id == app.game.world.partnerId || i == app.careIndex)
                app.game.dragons[i].xp = trainer::xpForLevel(a);
    } else if (std::strcmp(word, "league") == 0) {
        app.game.progress.battleLeague = static_cast<u8>(a < 0 ? 0 : (a > kLeagues ? kLeagues : a));
        if (n >= 3 && app.game.progress.battleLeague < kLeagues)
            app.game.progress.battleBeaten[app.game.progress.battleLeague] = static_cast<u8>(b);
    } else if (std::strcmp(word, "auto") == 0) {
        bview::setAutoplay(std::strstr(args, "on") != nullptr);
    } else if (std::strcmp(word, "energy") == 0) {
        for (int i = 0; i < app.game.dragonCount; ++i) app.game.dragons[i].needs.energy = static_cast<float>(a);
    }
}

}  // namespace ec
