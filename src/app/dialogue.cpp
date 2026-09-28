#include "app/dialogue.hpp"

#include <citro2d.h>

#include <cmath>
#include <cstring>

#include "app/audio.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/campaign.hpp"

#include "people_t3x.h"  // the villagers' portraits (gfx/people.t3s, in Villager order)

namespace ec {
namespace {

constexpr float kLettersPerSecond = 42.0f;
constexpr float kTextScale = 0.48f, kTextWidth = 228.0f;
C2D_SpriteSheet g_portraits = nullptr;

// Breaks a line into the box's width at spaces, before it's shown: letter by letter, a word
// never jumps down a line halfway through.
void wrapLine(App& app, char* s) {
    int start = 0, space = -1;
    auto tooWide = [&](int end) {
        const char keep = s[end];
        s[end] = 0;
        const bool wide = textWidth(app, s + start, kTextScale) > kTextWidth;
        s[end] = keep;
        return wide;
    };
    const int n = static_cast<int>(std::strlen(s));
    for (int i = 0; i <= n; ++i) {
        if (s[i] != ' ' && s[i] != 0) continue;
        if (space >= start && tooWide(i)) {
            s[space] = '\n';
            start = space + 1;
        }
        space = i;
    }
}

void loadLine(App& app) {
    DialogueState& d = app.talk;
    fillLine(d.talk.lines[d.line], app.game, d.text, sizeof(d.text));
    wrapLine(app, d.text);
    d.shown = 0;
    d.blipFor = 0;
}

void finish(App& app) {
    DialogueState& d = app.talk;
    d.active = false;
    audio::freeVoice();
    if (!d.custom && finishTalk(app.game, d.who, d.talk)) {
        const campaign::News n = campaign::update(app.game);
        if (n.finished >= 0) {
            audio::playStinger("quest-done");
            showToastf(app, str::kQuestFinished, campaign::view(app.game, n.finished).title);
        } else if (n.stepped >= 0 || n.started >= 0) {
            audio::playSfx(audio::Sfx::QuestPage);
        }
        saveNow(app);
    }
}

}  // namespace

void startTalk(App& app, Villager v) {
    DialogueState& d = app.talk;
    d = DialogueState{};
    d.who = v;
    d.talk = talkTo(app.game, v);
    if (d.talk.count == 0) return;
    d.active = true;
    audio::loadVoice(villagerInfo(v).voice);
    loadLine(app);
}

void startLines(App& app, Villager v, const Talk& lines) {
    DialogueState& d = app.talk;
    d = DialogueState{};
    d.who = v;
    d.talk = lines;
    d.talk.sets = 0;
    if (d.talk.count == 0) return;
    d.active = true;
    audio::loadVoice(villagerInfo(v).voice);
    loadLine(app);
}

void startSpeech(App& app, const Speaker& who, const Talk& lines) {
    DialogueState& d = app.talk;
    d = DialogueState{};
    d.custom = true;
    d.name = who.name;
    d.title = who.title;
    d.voice = who.voice;
    d.pitch = who.pitch;
    d.portrait = who.portrait;
    d.tint = who.tint;
    d.talk = lines;
    d.talk.sets = 0;
    if (d.talk.count == 0) return;
    d.active = true;
    audio::loadVoice(who.voice);
    loadLine(app);
}

bool talking(const App& app) { return app.talk.active; }

bool updateTalk(App& app, const Input& in) {
    DialogueState& d = app.talk;
    if (!d.active) return false;
    const int len = static_cast<int>(std::strlen(d.text));
    const int before = static_cast<int>(d.shown);
    if (d.shown < len) {
        d.shown += kLettersPerSecond * app.dt;
        if (d.shown > len) d.shown = static_cast<float>(len);
        // A voiced blip every other letter shown (a letter's sound, quick and high).
        const float pitch = d.custom ? d.pitch : villagerInfo(d.who).pitch;
        for (int k = app.game.settings.voiceEnabled ? before : len; k < static_cast<int>(d.shown); ++k)  // U: voices off in the settings
            if (k % 2 == 0 && ((d.text[k] >= 'a' && d.text[k] <= 'z') || (d.text[k] >= 'A' && d.text[k] <= 'Z')))
                audio::playLetter(d.text[k], pitch * (0.95f + 0.1f * ((k * 7) % 5) / 4.0f), 0.8f);
    }
    if ((in.down & (KEY_A | KEY_B)) || in.tapped) {
        if (d.shown < len && !(in.down & KEY_B)) {
            d.shown = static_cast<float>(len);  // the rest of the line at once
        } else if (d.line + 1 < d.talk.count && !(in.down & KEY_B)) {
            ++d.line;
            loadLine(app);
            audio::playSfx(audio::Sfx::Tap, 1.3f, 0.5f);
        } else {
            finish(app);
        }
    }
    return d.active || true;
}

void drawTalk(App& app) {
    const DialogueState& d = app.talk;
    if (!d.active) return;
    const char* whoName = d.custom ? d.name : villagerInfo(d.who).name;
    const char* whoTitle = d.custom ? d.title : villagerInfo(d.who).title;
    const int face = d.custom ? d.portrait : static_cast<int>(d.who);
    // The box, low on the screen; the name on a tab above it, the portrait at its left.
    const Rect box{8, 134, 304, 98};
    panel({box.x - 2, box.y - 2, box.w + 4, box.h + 4}, withAlpha(theme::kClutchGold, 0.9f));
    panel(box, theme::rgba(252, 244, 228));
    const float nameW = textWidth(app, whoName, 0.55f) + 22;
    panel({box.x + 58, box.y - 20, nameW, 22}, theme::kDenPlum);
    text(app, whoName, box.x + 69, box.y - 17, 0.55f, theme::kClutchGold, C2D_AlignLeft, nameW);
    // The portrait, in a round frame.
    C2D_DrawCircleSolid(box.x + 30, box.y + 30, 0.5f, 26, theme::kDenPlum);
    C2D_DrawCircleSolid(box.x + 30, box.y + 30, 0.5f, 23, d.custom ? fromRgb(d.tint) : theme::rgba(250, 226, 196));
    if (!g_portraits) g_portraits = C2D_SpriteSheetLoadFromMem(people_t3x, people_t3x_size);
    const float bob = std::sin(app.t * 7.0f) * (d.shown < std::strlen(d.text) ? 1.2f : 0.0f);  // talking
    if (g_portraits && face >= 0 && static_cast<std::size_t>(face) < C2D_SpriteSheetCount(g_portraits)) {
        const C2D_Image img = C2D_SpriteSheetGetImage(g_portraits, static_cast<std::size_t>(face));
        C2D_DrawImageAt(img, box.x + 30 - 24, box.y + 30 - 26 + bob, 0.5f, nullptr, 0.75f, 0.75f);
    } else if (whoName[0]) {  // no portrait: their initial
        const char initial[2] = {whoName[0], 0};
        textCentered(app, initial, box.x + 30, box.y + 30 + bob, 0.9f, theme::kDenPlum, 40, Face::Title);
    }
    text(app, whoTitle, box.x + 30, box.y + 60, 0.32f, withAlpha(theme::kDenPlum, 0.7f), C2D_AlignCenter, 56);
    // The line so far.
    char shown[160];
    const int n = static_cast<int>(d.shown);
    std::memcpy(shown, d.text, static_cast<std::size_t>(n));
    shown[n] = 0;
    text(app, shown, box.x + 66, box.y + 10, kTextScale, theme::kDenPlum, C2D_AlignLeft);
    if (n >= static_cast<int>(std::strlen(d.text))) {  // a little arrow: A for more
        const float bob = 2 * std::sin(app.t * 6);
        C2D_DrawTriangle(box.x + box.w - 18, box.y + box.h - 16 + bob, theme::kDenPlum, box.x + box.w - 8,
                         box.y + box.h - 16 + bob, theme::kDenPlum, box.x + box.w - 13, box.y + box.h - 9 + bob,
                         theme::kDenPlum, 0.5f);
    }
}

}  // namespace ec
