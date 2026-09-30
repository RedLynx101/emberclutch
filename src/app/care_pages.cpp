// The care tray's three places (D85): Outing (your travel partner, and out into the valley
// together), Journal (the quest log, what you know of this dragon, your finds and places, the
// Dragondex) and Den (its decor, spot by spot). Each takes the bottom screen over the close-up
// until Back (or B).
#include <cstdio>

#include "app/audio.hpp"
#include "app/care_ui.hpp"
#include "app/dragondex_ui.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/tips_ui.hpp"
#include "app/tracking_ui.hpp"
#include "app/ui_draw.hpp"
#include "app/wildlife.hpp"  // the Journal's valley critters (workstream L)
#include "core/story.hpp"
#include "core/guide.hpp"
#include "core/finds.hpp"
#include "core/items.hpp"
#include "core/kinds.hpp"
#include "core/world.hpp"

namespace ec::care {
namespace {

constexpr Rect kBack{214, 202, 100, 34};

void header(App& app, const char* title) {
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDenPlum, theme::rgba(30, 20, 40));
    textCentered(app, title, 160, 16, 0.75f, theme::kClutchGold, 300, Face::Title);
}

bool back(App& app, const Input& in) {
    if (button(app, kBack, str::kBack, in) || (in.down & KEY_B)) {
        app.care.page = CarePage::None;
        audio::playSfx(audio::Sfx::Back);
        return true;
    }
    return false;
}

const Dragon* partnerOf(const SaveData& s) {
    for (int i = 0; i < s.dragonCount; ++i)
        if (s.dragons[i].id == s.world.partnerId && s.dragons[i].stage != Stage::Egg) return &s.dragons[i];
    return nullptr;
}

// ------------------------------------------------------------------------------ Outing
void drawOuting(App& app, const Input& in, Dragon& d) {
    header(app, str::kOuting);
    SaveData& s = app.game;
    const Dragon* partner = partnerOf(s);
    char line[96];
    panel({12, 36, 296, 56}, withAlpha(theme::kShell, 0.12f));
    if (partner)
        std::snprintf(line, sizeof(line), str::kPartnerIs, partner->name, kindTitle(*partner));
    else
        std::snprintf(line, sizeof(line), "%s", str::kNoPartner);
    textCentered(app, line, 160, 52, 0.5f, theme::kShell, 290);
    textCentered(app, str::kPartnerWhy, 160, 74, 0.38f, withAlpha(theme::kShell, 0.7f), 290);
    if (d.stage == Stage::Egg) {
        textCentered(app, str::kEggStays, 160, 130, 0.5f, withAlpha(theme::kShell, 0.8f), 290);
        back(app, in);
        return;
    }
    const bool isPartner = partner == &d;
    // Who goes: your travel partner if it's at home, else the one in your care (run 21: Head out
    // took the one in your care along and made it your partner behind your back).
    const Dragon& goer = partner && !partner->wanderSince ? *partner : d;
    const bool rideable = goer.stage == Stage::Adult;
    std::snprintf(line, sizeof(line), rideable ? str::kOutingRide : str::kOutingLead, goer.name);
    textCentered(app, line, 160, 104, 0.42f, withAlpha(theme::kShell, 0.85f), 300);
    if (!isPartner) {
        std::snprintf(line, sizeof(line), str::kMakePartner, d.name);
        if (button(app, {40, 122, 240, 30}, line, in)) {
            s.world.partnerId = d.id;
            audio::playSfx(audio::Sfx::Confirm);
            saveNow(app);
        }
    }
    std::snprintf(line, sizeof(line), str::kHeadOutWith, goer.name);
    if (button(app, {40, 158, 240, 34}, line, in, theme::kClutchGold)) {
        s.world.partnerId = goer.id;
        saveNow(app);
        audio::playSfx(audio::Sfx::Confirm);
        app.care.page = CarePage::None;
        openValley(app);
        return;
    }
    back(app, in);
}

// ------------------------------------------------------------------------------ Journal
// The goals (1.0, D89; the story's since D137): what can be tracked (the begun quests, each with its
// line of the story, then the leagues' boards and the Hollow), the one tracked now flagged in gold; a
// tap on a row tracks it (again: back to the quest in hand). Then the rumours (quests waiting for you
// to ask about them), then the quests done, in teal. Four rows a page.
void journalGoals(App& app, const Input& in) {
    SaveData& s = app.game;
    const s64 now = nowLocal(app);
    constexpr int kRows = 4;
    enum class Kind : u8 { Goal, Rumour, Done };
    struct Row {
        guide::Goal goal;
        Kind kind;
    };
    constexpr int kMax = story::kMaxQuests * 2 + 4;
    static Row rows[kMax];
    int n = 0;
    guide::Goal open[story::kMaxQuests + 4];
    const int tracks = guide::trackables(s, open, story::kMaxQuests + 4);
    for (int k = 0; k < tracks && n < kMax; ++k) rows[n++] = {open[k], Kind::Goal};
    for (int q = 0; q < story::questCount() && n < kMax; ++q) {
        const story::QuestView v = story::view(s, q, now);
        if (v.open && v.rumour[0]) rows[n++] = {{Tracked::Quest, q}, Kind::Rumour};
    }
    for (int q = 0; q < story::questCount() && n < kMax; ++q)
        if (story::questDone(s, q)) rows[n++] = {{Tracked::Quest, q}, Kind::Done};
    if (n == 0) {
        text(app, str::kNoQuests, 160, 110, 0.45f, withAlpha(theme::kShell, 0.75f), C2D_AlignCenter, 290);
        return;
    }
    static int page = 0;
    const int pages = (n + kRows - 1) / kRows;
    if (page >= pages) page = pages - 1;
    const guide::Goal tracked = guide::current(s);
    char title[64], step[160];
    for (int k = 0; k < kRows && page * kRows + k < n; ++k) {
        const Row& row = rows[page * kRows + k];
        const Rect r{8, 62.0f + k * 34, 304, 32};
        const bool done = row.kind == Kind::Done, rumour = row.kind == Kind::Rumour;
        const bool on = row.kind == Kind::Goal && row.goal == tracked;
        panel(r, done     ? withAlpha(theme::kSkyTeal, 0.25f)
                 : rumour ? withAlpha(theme::kDusk, 0.5f)
                          : withAlpha(on ? theme::kClutchGold : theme::kShell, on ? 0.28f : 0.12f));
        if (rumour) {
            const story::QuestView v = story::view(s, row.goal.id, now);
            std::snprintf(title, sizeof(title), str::kRumourTitle, v.title);
            fillLine(v.rumour, s, step, sizeof(step));
        } else {
            goalWords(s, row.goal, title, sizeof(title), step, sizeof(step));
        }
        text(app, title, r.x + 7, r.y + 1, 0.46f,
             done ? withAlpha(theme::kShell, 0.65f) : rumour ? withAlpha(theme::kShell, 0.85f) : theme::kClutchGold, C2D_AlignLeft, 200);
        if (row.goal.kind == Tracked::Quest && !rumour) {  // its line of the story, small at the right
            const story::QuestView v = story::view(s, row.goal.id, now);
            text(app, story::lineName(v.line), r.x + r.w - (done ? 6 : 68), r.y + 3, 0.3f, withAlpha(theme::kShell, 0.55f),
                 C2D_AlignRight, 70);
        }
        text(app, step, r.x + 7, r.y + 16, 0.41f, withAlpha(theme::kShell, done ? 0.6f : 0.88f), C2D_AlignLeft, done ? 290 : 236);
        if (row.kind != Kind::Goal) continue;
        // The flag: gold on the one tracked (its word under it), faint on the others.
        trackFlag(r.x + r.w - 58, r.y + 22, 13, app.t, on);
        text(app, on ? str::kTracking : str::kTrack, r.x + r.w - 6, r.y + 9, 0.36f,
             on ? theme::kClutchGold : withAlpha(theme::kShell, 0.5f), C2D_AlignRight, 40);
        if (in.released && r.contains(in.rx, in.ry)) {
            const bool was = guide::picked(s, row.goal);
            guide::toggle(s, row.goal);
            audio::playSfx(was ? audio::Sfx::Back : audio::Sfx::Confirm);
            if (!was) {
                showToastf(app, str::kNowTracking, title);
                showTip(app, tips::kTipTracked);
            }
            saveNow(app);
        }
    }
    if (pages > 1) {  // more than a page: < 1/2 >
        if (button(app, {6, 202, 34, 34}, "<", in)) page = (page + pages - 1) % pages;
        char at[32];
        std::snprintf(at, sizeof(at), "%d/%d", page + 1, pages);
        textCentered(app, at, 62, 219, 0.4f, theme::kShell);
        if (button(app, {84, 202, 34, 34}, ">", in)) page = (page + 1) % pages;
    } else {
        text(app, str::kTrackHint, 10, 201, 0.4f, withAlpha(theme::kShell, 0.7f), C2D_AlignLeft, 196);
    }
}

void drawJournal(App& app, const Input& in, Dragon& d, s64 now) {
    header(app, str::kJournal);
    showTip(app, tips::kTipJournal);
    CareState& c = app.care;
    const char* tabs[4] = {str::kJournalQuests, d.stage == Stage::Egg ? str::kJournalEgg : d.name, str::kJournalPlaces,
                           str::kDex};
    for (int t = 0; t < 4; ++t) {
        const Rect r{8.0f + t * 77.0f, 32, 73, 24};
        const bool on = c.journalTab == t || (t == 2 && c.journalTab == 4);  // (4: the places' critters page)
        panel(r, withAlpha(on ? theme::kClutchGold : theme::kShell, on ? 0.6f : 0.16f));
        textCentered(app, tabs[t], r.x + r.w / 2, r.y + 12, 0.42f, on ? theme::kDenPlum : theme::kShell, 70);
        if (in.tapped && r.contains(in.tx, in.ty)) {
            audio::playSfx(audio::Sfx::Tap);
            if (t == 3) {  // the Dragondex is its own book
                openDex(app);
                return;
            }
            c.journalTab = static_cast<u8>(t);
        }
    }
    SaveData& s = app.game;
    char line[96];
    if (c.journalTab == 0) {  // goals: the quests (the Lantern Festival, WP14) and 1.0's boards, tracked with a tap
        journalGoals(app, in);
    } else if (c.journalTab == 1) {  // what you know of this one: the profile's About page, and the rest of it
        profileAbout(app, d, now);
        // The rest of its profile (the den's card: its training, record, family). The Journal is
        // drawn in the valley too (X there): the card is the den's, so only from the den.
        if (app.scene == SceneId::Den && d.stage != Stage::Egg && button(app, {6, 202, 110, 34}, str::kFullProfile, in)) {
            c.page = CarePage::None;
            c.profileOpen = true;
            c.profileTab = kTabTraining;
            return;
        }
    } else if (c.journalTab == 4) {  // the valley's critters (workstream L)
        wildlife::drawJournal(app, in);
    } else {  // places found (a tap tracks one) and finds
        int shown = 0;
        for (int p = 0; p < world::placeCount(); ++p) {  // three columns: all eighteen fit
            if (!world::placeFound(s, p)) continue;
            const float y = 62 + (shown / 3) * 18;
            if (y > 160) break;
            const float x = 6 + (shown % 3) * 103;
            const guide::Goal g{Tracked::Place, p};
            const bool on = guide::picked(s, g);
            const Rect r{x, y - 1, 101, 17};
            if (on) panel(r, withAlpha(theme::kClutchGold, 0.3f));
            C2D_DrawCircleSolid(x + 7, y + 7, 0.5f, 3.5f, fromRgb(world::placeInfo(p).pin));
            text(app, world::placeInfo(p).name, x + 14, y, 0.36f, theme::kShell, C2D_AlignLeft, on ? 74 : 85);
            if (on) trackFlag(x + 92, y + 14, 11, app.t, true);
            if (in.released && r.contains(in.rx, in.ry)) {
                guide::toggle(s, g);
                audio::playSfx(on ? audio::Sfx::Back : audio::Sfx::Confirm);
                if (!on) {
                    showToastf(app, str::kNowTracking, world::placeInfo(p).name);
                    showTip(app, tips::kTipTracked);
                }
                saveNow(app);
            }
            ++shown;
        }
        text(app, str::kTrackPlaceHint, 14, 170, 0.34f, withAlpha(theme::kShell, 0.55f), C2D_AlignLeft, 290);
        std::snprintf(line, sizeof(line), str::kPlacesFound, shown, world::placeCount());
        text(app, line, 14, 184, 0.4f, withAlpha(theme::kShell, 0.75f), C2D_AlignLeft, 150);
        int finds = 0;
        for (int i = 0; i < kFindSpots; ++i) finds += findDone(s, i);
        std::snprintf(line, sizeof(line), str::kFindsFound, finds, kFindSpots);  // the finds (WP7)
        text(app, line, 170, 184, 0.4f, withAlpha(theme::kClutchGold, 0.85f), C2D_AlignLeft, 140);
        if (button(app, {6, 202, 110, 34}, str::kCrittersButton, in)) {  // the valley's critters (workstream L)
            c.journalTab = 4;
            audio::playSfx(audio::Sfx::Tap);
        }
    }
    back(app, in);
}

// ------------------------------------------------------------------------------ Den
const ItemKind kSpotKinds[kDecorSpots] = {ItemKind::Rug, ItemKind::Lantern, ItemKind::Perch, ItemKind::Plant,
                                          ItemKind::Banner};

// The next decor of this spot's kind you own after `from` (Item::Count: none), `dir` +1 / -1.
Item cycle(const SaveData& s, int spot, Item from, int dir) {
    Item options[kItems + 1];
    int n = 0, at = 0;
    options[n++] = Item::Count;  // bare
    for (int i = 0; i < kItems; ++i) {
        const Item it = static_cast<Item>(i);
        if (itemInfo(it).kind == kSpotKinds[spot] && owns(s, it)) {
            if (it == from) at = n;
            options[n++] = it;
        }
    }
    return options[(at + n + dir) % n];
}

void drawDen(App& app, const Input& in) {
    header(app, str::kDenTitle);
    SaveData& s = app.game;
    for (int spot = 0; spot < kDecorSpots; ++spot) {
        const float y = 36 + spot * 32;
        panel({10, y, 300, 29}, withAlpha(theme::kShell, 0.12f));
        text(app, str::kDecorSpotNames[spot], 18, y + 7, 0.45f, withAlpha(theme::kShell, 0.8f), C2D_AlignLeft, 70);
        const Item up = decorAt(s, spot);
        if (up != Item::Count) {
            drawItem(up, 118, y + 14, 0.4f);
            text(app, itemInfo(up).name, 136, y + 7, 0.42f, theme::kShell, C2D_AlignLeft, 110);
        } else {
            text(app, str::kDecorNone, 118, y + 7, 0.42f, withAlpha(theme::kShell, 0.5f), C2D_AlignLeft, 120);
        }
        for (int dir : {-1, 1}) {
            const Rect r{dir < 0 ? 246.0f : 280.0f, y + 2, 28, 25};
            if (button(app, r, dir < 0 ? "<" : ">", in)) {
                const Item next = cycle(s, spot, up, dir);
                if (next == Item::Count)
                    takeDown(s, spot);
                else
                    putUp(s, next);
                audio::playSfx(audio::Sfx::Tap);
                saveNow(app);
            }
        }
    }
    text(app, str::kDenHint, 12, 200, 0.36f, withAlpha(theme::kShell, 0.7f), C2D_AlignLeft, 196);
    back(app, in);
}

}  // namespace

bool drawPage(App& app, const Input& in, Dragon& d, s64 now) {
    switch (app.care.page) {
        case CarePage::Outing: drawOuting(app, in, d); return true;
        case CarePage::Journal: drawJournal(app, in, d, now); return true;
        case CarePage::Den: drawDen(app, in); return true;
        default: return false;
    }
}

}  // namespace ec::care
