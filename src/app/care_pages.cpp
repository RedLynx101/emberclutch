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
#include "app/ui_draw.hpp"
#include "core/campaign.hpp"
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
    const bool rideable = d.stage == Stage::Adult;
    std::snprintf(line, sizeof(line), rideable ? str::kOutingRide : str::kOutingLead, d.name);
    textCentered(app, line, 160, 104, 0.42f, withAlpha(theme::kShell, 0.85f), 300);
    if (!isPartner) {
        std::snprintf(line, sizeof(line), str::kMakePartner, d.name);
        if (button(app, {40, 122, 240, 30}, line, in)) {
            s.world.partnerId = d.id;
            audio::playSfx(audio::Sfx::Confirm);
            saveNow(app);
        }
    }
    if (button(app, {40, 158, 240, 34}, str::kHeadOut, in, theme::kClutchGold)) {
        s.world.partnerId = d.id;
        saveNow(app);
        audio::playSfx(audio::Sfx::Confirm);
        app.care.page = CarePage::None;
        openValley(app);
        return;
    }
    back(app, in);
}

// ------------------------------------------------------------------------------ Journal
void drawJournal(App& app, const Input& in, Dragon& d, s64 now) {
    header(app, str::kJournal);
    CareState& c = app.care;
    const char* tabs[4] = {str::kJournalQuests, d.stage == Stage::Egg ? str::kJournalEgg : d.name, str::kJournalPlaces,
                           str::kDex};
    for (int t = 0; t < 4; ++t) {
        const Rect r{8.0f + t * 77.0f, 32, 73, 24};
        panel(r, withAlpha(c.journalTab == t ? theme::kClutchGold : theme::kShell, c.journalTab == t ? 0.6f : 0.16f));
        textCentered(app, tabs[t], r.x + r.w / 2, r.y + 12, 0.42f, c.journalTab == t ? theme::kDenPlum : theme::kShell, 70);
        if (in.tapped && r.contains(in.tx, in.ty)) {
            audio::playSfx(audio::Sfx::Tap);
            if (t == 3) {  // the Dragondex is its own book
                openDex(app);
                return;
            }
            c.journalTab = static_cast<u8>(t);
        }
    }
    const SaveData& s = app.game;
    char line[96];
    if (c.journalTab == 0) {  // quests (the Lantern Festival, WP14)
        int shown = 0;
        for (int q = 0; q < campaign::questCount(); ++q) {
            const campaign::QuestView v = campaign::view(s, q);
            if (!v.started) continue;
            const float y = 64 + shown * 32;
            if (y > 180) break;
            panel({10, y, 300, 29}, withAlpha(v.done ? theme::kSkyTeal : theme::kShell, v.done ? 0.3f : 0.12f));
            text(app, v.title, 16, y + 2, 0.45f, v.done ? withAlpha(theme::kShell, 0.7f) : theme::kClutchGold, C2D_AlignLeft, 200);
            text(app, v.done ? str::kQuestDone : v.step, 16, y + 15, 0.36f, withAlpha(theme::kShell, 0.85f), C2D_AlignLeft, 290);
            ++shown;
        }
        if (shown == 0) text(app, str::kNoQuests, 160, 110, 0.45f, withAlpha(theme::kShell, 0.75f), C2D_AlignCenter, 290);
    } else if (c.journalTab == 1) {  // what you know of this one: the profile's About page
        profileAbout(app, d, now);
    } else {  // places found and finds
        int shown = 0;
        for (int p = 0; p < world::placeCount(); ++p) {
            if (!world::placeFound(s, p)) continue;
            const float y = 64 + (shown / 2) * 22;
            if (y > 178) break;
            const float x = 14 + (shown % 2) * 150;
            C2D_DrawCircleSolid(x + 5, y + 8, 0.5f, 4, fromRgb(world::placeInfo(p).pin));
            text(app, world::placeInfo(p).name, x + 14, y + 1, 0.42f, theme::kShell, C2D_AlignLeft, 136);
            ++shown;
        }
        std::snprintf(line, sizeof(line), str::kPlacesFound, shown, world::placeCount());
        text(app, line, 14, 184, 0.4f, withAlpha(theme::kShell, 0.75f), C2D_AlignLeft, 150);
        int finds = 0;
        for (int i = 0; i < kFindSpots; ++i) finds += findDone(s, i);
        std::snprintf(line, sizeof(line), str::kFindsFound, finds, kFindSpots);  // the finds (WP7)
        text(app, line, 170, 184, 0.4f, withAlpha(theme::kClutchGold, 0.85f), C2D_AlignLeft, 140);
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
