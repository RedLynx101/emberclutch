// The mailbox by the den's door (D137, the Living Valley pass): the letters that have come (the
// newest first, the unread ones sealed in gold) and a letter opened on a cream paper card, its words
// wrapped to the card and paged ("--" in a letter starts a new page). Opening one reads it: its gift
// or the quest it asks you on (core/story readLetter), told the game's way (app/story_app).
#include <citro2d.h>

#include <cstdio>
#include <cstring>

#include "app/audio.hpp"
#include "app/story_app.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/story.hpp"

namespace ec {
namespace {

constexpr int kRowsShown = 4;
constexpr float kCardX = 14, kCardY = 30, kCardW = 292, kCardH = 168;
constexpr float kTextScale = 0.43f, kLineH = 14.5f, kTextW = 266;
constexpr int kLinesAPage = 9;

// The opened letter's words, wrapped: lines into pages.
struct Pages {
    char text[2048] = {};
    const char* line[96] = {};
    int lineCount = 0;
    int pageStart[16] = {};
    int pageCount = 0;
    int letter = -1;
};
Pages g_pages;

void wrapLetter(App& app, int l) {
    Pages& p = g_pages;
    p = Pages{};
    p.letter = l;
    const story::LetterView v = story::letter(app.game, l);
    int at = 0;
    p.pageStart[p.pageCount++] = 0;
    for (int k = 0; k < v.count && p.lineCount < 90; ++k) {
        if (std::strcmp(v.paragraphs[k], "--") == 0) {  // a new page
            if (p.pageCount < 16) p.pageStart[p.pageCount++] = p.lineCount;
            continue;
        }
        char para[512];
        fillLine(v.paragraphs[k], app.game, para, sizeof(para));
        // word by word into lines no wider than the card
        const char* s = para;
        while (*s && p.lineCount < 90) {
            int end = 0, lastSpace = -1;
            char tryLine[256];
            while (s[end] && end < 250) {
                if (s[end] == ' ') {
                    std::memcpy(tryLine, s, static_cast<std::size_t>(end));
                    tryLine[end] = 0;
                    if (textWidth(app, tryLine, kTextScale) > kTextW) break;
                    lastSpace = end;
                }
                ++end;
            }
            int take = end;
            if (s[end]) {
                if (lastSpace > 0) take = lastSpace;
            } else {
                std::memcpy(tryLine, s, static_cast<std::size_t>(end));
                tryLine[end] = 0;
                if (textWidth(app, tryLine, kTextScale) > kTextW && lastSpace > 0) take = lastSpace;
            }
            if (at + take + 1 >= static_cast<int>(sizeof(p.text))) break;
            std::memcpy(p.text + at, s, static_cast<std::size_t>(take));
            p.text[at + take] = 0;
            p.line[p.lineCount++] = p.text + at;
            at += take + 1;
            s += take;
            while (*s == ' ') ++s;
            // a page full: the next starts
            if (p.lineCount - p.pageStart[p.pageCount - 1] >= kLinesAPage && *s && p.pageCount < 16)
                p.pageStart[p.pageCount++] = p.lineCount;
        }
        if (p.lineCount < 90) {  // a blank line between paragraphs (not at a page's top)
            if (p.lineCount > p.pageStart[p.pageCount - 1] && p.lineCount - p.pageStart[p.pageCount - 1] < kLinesAPage) {
                p.line[p.lineCount++] = "";
            }
        }
    }
}

void openLetter(App& app, int l) {
    MailboxState& m = app.mailbox;
    m.letter = l;
    m.page = 0;
    wrapLetter(app, l);
    story::News n;
    if (story::readLetter(app.game, l, nowLocal(app), &n)) storyNews(app, n);
    audio::playSfx(audio::Sfx::QuestPage);
}

}  // namespace

void openMailbox(App& app) {
    MailboxState& m = app.mailbox;
    m = MailboxState{};
    m.open = true;
    audio::playSfx(audio::Sfx::MailArrive, 1.0f, 0.6f);
    int box[64];
    const int n = story::mailbox(app.game, box, 64);  // an unread letter waiting: straight to it
    for (int k = 0; k < n; ++k)
        if (!story::letter(app.game, box[k]).read) {
            openLetter(app, box[k]);
            break;
        }
}

bool mailboxOpen(const App& app) { return app.mailbox.open; }

bool updateMailbox(App& app, const Input& in) {
    MailboxState& m = app.mailbox;
    if (!m.open) return false;
    if (m.letter >= 0) {
        const int pages = g_pages.letter == m.letter ? g_pages.pageCount : 1;
        if (in.down & KEY_A) {
            if (m.page + 1 < pages) {
                ++m.page;
                audio::playSfx(audio::Sfx::QuestPage);
            } else {
                m.letter = -1;
                audio::playSfx(audio::Sfx::Back);
            }
        } else if (in.down & KEY_B) {
            m.letter = -1;
            audio::playSfx(audio::Sfx::Back);
        }
        return true;
    }
    int box[64];
    const int n = story::mailbox(app.game, box, 64);
    if (in.down & KEY_B) {
        m.open = false;
        audio::playSfx(audio::Sfx::Back);
        return false;
    }
    if ((in.down & KEY_DOWN) && m.first + kRowsShown < n) ++m.first;
    if ((in.down & KEY_UP) && m.first > 0) --m.first;
    if ((in.down & KEY_A) && n > 0) openLetter(app, box[m.first]);
    return true;
}

void drawMailbox(App& app, const Input& in) {
    MailboxState& m = app.mailbox;
    if (!m.open) return;
    verticalGradient(0, 0, 320, 240, theme::kDusk, theme::kDenPlum);
    if (m.letter >= 0) {  // a letter, on its card
        if (g_pages.letter != m.letter) wrapLetter(app, m.letter);
        const story::LetterView v = story::letter(app.game, m.letter);
        char head[96];
        std::snprintf(head, sizeof(head), str::kMailFrom, v.from >= 0 ? story::person(v.from).name : "");
        textCentered(app, head, 160, 14, 0.42f, withAlpha(theme::kShell, 0.8f), 300);
        panel({kCardX + 4, kCardY + 4, kCardW, kCardH}, withAlpha(theme::rgba(40, 24, 40), 0.35f));  // (its shadow)
        panel({kCardX, kCardY, kCardW, kCardH}, theme::rgba(250, 243, 226));
        heart(kCardX + kCardW - 16, kCardY + 12, 7.0f, theme::rgba(214, 40, 64));  // (the seal)
        const u32 ink = theme::rgba(86, 52, 70);
        text(app, v.subject, kCardX + 12, kCardY + 6, 0.5f, ink, C2D_AlignLeft, kCardW - 40);
        const int from = g_pages.pageStart[m.page < g_pages.pageCount ? m.page : 0];
        const int to = m.page + 1 < g_pages.pageCount ? g_pages.pageStart[m.page + 1] : g_pages.lineCount;
        for (int k = from; k < to && k - from < kLinesAPage + 1; ++k)
            text(app, g_pages.line[k], kCardX + 12, kCardY + 28 + (k - from) * kLineH, kTextScale, ink, C2D_AlignLeft);
        const bool more = m.page + 1 < g_pages.pageCount;
        if (g_pages.pageCount > 1) {
            char at[32];
            std::snprintf(at, sizeof(at), "%d/%d", m.page + 1, g_pages.pageCount);
            text(app, at, kCardX + kCardW - 10, kCardY + kCardH - 16, 0.36f, withAlpha(ink, 0.7f), C2D_AlignRight);
        }
        if (button(app, {100, 204, 120, 30}, more ? str::kMailNextPage : str::kMailFold, in)) {
            if (more) {
                ++m.page;
                audio::playSfx(audio::Sfx::QuestPage);
            } else {
                m.letter = -1;
                audio::playSfx(audio::Sfx::Back);
            }
        }
        return;
    }
    // The list: the newest first; a gold seal on the unread.
    textCentered(app, str::kMailbox, 160, 14, 0.55f, theme::kClutchGold, 300, Face::Title);
    int box[64];
    const int n = story::mailbox(app.game, box, 64);
    if (n == 0) {
        textCentered(app, str::kMailEmpty, 160, 110, 0.45f, withAlpha(theme::kShell, 0.75f), 280);
    }
    if (m.first > n - kRowsShown) m.first = n - kRowsShown > 0 ? n - kRowsShown : 0;
    for (int k = 0; k < kRowsShown && m.first + k < n; ++k) {
        const int l = box[m.first + k];
        const story::LetterView v = story::letter(app.game, l);
        const Rect r{10, 32.0f + k * 40, 300, 36};
        panel(r, v.read ? withAlpha(theme::kShell, 0.12f) : withAlpha(theme::kClutchGold, 0.28f));
        if (!v.read) heart(r.x + 14, r.y + 18, 6.0f, theme::rgba(214, 40, 64));
        char subject[96];
        fillLine(v.subject, app.game, subject, sizeof(subject));
        text(app, subject, r.x + 28, r.y + 3, 0.46f, v.read ? withAlpha(theme::kShell, 0.75f) : theme::kClutchGold, C2D_AlignLeft, 260);
        char from[64];
        std::snprintf(from, sizeof(from), str::kMailFrom, v.from >= 0 ? story::person(v.from).name : "");
        text(app, from, r.x + 28, r.y + 19, 0.38f, withAlpha(theme::kShell, 0.7f), C2D_AlignLeft, 260);
        if (in.released && r.contains(in.rx, in.ry)) openLetter(app, l);
    }
    if (n > kRowsShown) {
        char at[48];
        std::snprintf(at, sizeof(at), "%d-%d / %d", m.first + 1, m.first + kRowsShown < n ? m.first + kRowsShown : n, n);
        text(app, at, 148, 210, 0.36f, withAlpha(theme::kShell, 0.6f), C2D_AlignCenter);  // (between the arrows and Close: run 23, it hid under Close)
        if (m.first > 0 && button(app, {10, 200, 40, 34}, "^", in)) --m.first;
        if (m.first + kRowsShown < n && button(app, {56, 200, 40, 34}, "v", in)) ++m.first;
    }
    if (button(app, {200, 200, 110, 34}, str::kMailClose, in)) {
        m.open = false;
        audio::playSfx(audio::Sfx::Back);
    }
}

}  // namespace ec
