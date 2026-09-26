#include "app/dialogue.hpp"

#include <cstring>

#include "app/audio.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/campaign.hpp"

namespace ec {
namespace {

constexpr float kLettersPerSecond = 42.0f;

void loadLine(App& app) {
    DialogueState& d = app.talk;
    fillLine(d.talk.lines[d.line], app.game, d.text, sizeof(d.text));
    d.shown = 0;
    d.blipFor = 0;
}

void finish(App& app) {
    DialogueState& d = app.talk;
    d.active = false;
    audio::freeVoice();
    if (finishTalk(app.game, d.who, d.talk)) {
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
        const VillagerInfo& who = villagerInfo(d.who);
        for (int k = before; k < static_cast<int>(d.shown); ++k)
            if (k % 2 == 0 && ((d.text[k] >= 'a' && d.text[k] <= 'z') || (d.text[k] >= 'A' && d.text[k] <= 'Z')))
                audio::playLetter(d.text[k], who.pitch * (0.95f + 0.1f * ((k * 7) % 5) / 4.0f), 0.8f);
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
    const VillagerInfo& who = villagerInfo(d.who);
    // The box, low on the screen; the name on a tab above it, the portrait at its left.
    const Rect box{8, 134, 304, 98};
    panel({box.x - 2, box.y - 2, box.w + 4, box.h + 4}, withAlpha(theme::kClutchGold, 0.9f));
    panel(box, theme::rgba(252, 244, 228));
    const float nameW = textWidth(app, who.name, 0.55f) + 22;
    panel({box.x + 58, box.y - 20, nameW, 22}, theme::kDenPlum);
    text(app, who.name, box.x + 69, box.y - 17, 0.55f, theme::kClutchGold, C2D_AlignLeft, nameW);
    // The portrait: a round face in their colours until the people's portraits arrive.
    C2D_DrawCircleSolid(box.x + 30, box.y + 30, 0.5f, 25, theme::kDenPlum);
    C2D_DrawCircleSolid(box.x + 30, box.y + 30, 0.5f, 22, theme::rgba(250, 214, 180));
    char initial[2] = {who.name[0] == 'O' ? who.name[4] : who.name[0], 0};
    textCentered(app, initial, box.x + 30, box.y + 30, 0.9f, theme::kDenPlum, 40, Face::Title);
    text(app, who.title, box.x + 30, box.y + 60, 0.32f, withAlpha(theme::kDenPlum, 0.7f), C2D_AlignCenter, 56);
    // The line so far.
    char shown[160];
    const int n = static_cast<int>(d.shown);
    std::memcpy(shown, d.text, static_cast<std::size_t>(n));
    shown[n] = 0;
    text(app, shown, box.x + 64, box.y + 10, 0.48f, theme::kDenPlum, C2D_AlignLeft, box.w - 72);
    if (n >= static_cast<int>(std::strlen(d.text))) {  // a little arrow: A for more
        const float bob = 2 * std::sin(app.t * 6);
        C2D_DrawTriangle(box.x + box.w - 18, box.y + box.h - 16 + bob, theme::kDenPlum, box.x + box.w - 8,
                         box.y + box.h - 16 + bob, theme::kDenPlum, box.x + box.w - 13, box.y + box.h - 9 + bob,
                         theme::kDenPlum, 0.5f);
    }
}

}  // namespace ec
