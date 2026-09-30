#include "app/dialogue.hpp"

#include <citro2d.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "app/audio.hpp"
#include "app/emotes.hpp"
#include "app/story_app.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/story.hpp"

#include "people_t3x.h"  // the villagers' old portraits (gfx/people.t3s, in Villager order): until theirs are in

namespace ec {
namespace {

constexpr float kLettersPerSecond = 42.0f;
constexpr float kTextScale = 0.48f, kTextWidth = 228.0f;
C2D_SpriteSheet g_portraits = nullptr;   // the villagers' old sheet
C2D_SpriteSheet g_face = nullptr;        // the speaker's own sheet (romfs/portraits/<id>.t3x: a frame per feeling)
int g_facePerson = -2;                   // whose it is (-2: none tried)

// Who says the line now: a story person's name, title, voice and portrait; else the custom speaker
// (a challenger, a host) or the villager.
struct Who {
    const char* name = "";
    const char* title = "";
    u8 voice = 0;
    float pitch = 1.0f;
    int person = -1;    // a story person (-1: none)
    int villager = -1;  // their old portrait (people_t3x)
    Rgb tint{250, 226, 196};
    bool narration = false;
};

Who whoNow(const App& app) {
    const DialogueState& d = app.talk;
    Who w;
    const int p = d.speaker >= 0 ? d.speaker : d.talk.person;
    if (p == story::kPNarrator) {
        w.narration = true;
        w.person = p;
        return w;
    }
    if (p >= 0) {
        const story::PersonInfo& info = story::person(p);
        w.name = info.name;
        w.title = info.title;
        w.voice = info.voice;
        w.pitch = info.pitch;
        w.person = p;
        w.villager = info.villager;
        w.tint = info.tint;
        if (d.custom && d.speaker < 0 && d.portrait >= 0) w.villager = d.portrait;
        return w;
    }
    if (d.custom) {
        w.name = d.name;
        w.title = d.title;
        w.voice = d.voice;
        w.pitch = d.pitch;
        w.villager = d.portrait;
        w.tint = d.tint;
        return w;
    }
    const VillagerInfo& v = villagerInfo(d.who);
    w.name = v.name;
    w.title = v.title;
    w.voice = v.voice;
    w.pitch = v.pitch;
    w.villager = static_cast<int>(d.who);
    return w;
}

void freeFace() {
    if (g_face) C2D_SpriteSheetFree(g_face);
    g_face = nullptr;
    g_facePerson = -2;
}

// The speaker's own portraits, loaded as they start speaking (one sheet at a time).
C2D_SpriteSheet faceOf(int person) {
    if (person == g_facePerson) return g_face;
    freeFace();
    g_facePerson = person;
    if (person >= 0 && story::person(person).portrait[0]) {
        char path[64];
        std::snprintf(path, sizeof(path), "romfs:/portraits/%s.t3x", story::person(person).portrait);
        g_face = C2D_SpriteSheetLoad(path);
    }
    return g_face;
}

// Breaks a line into the box's width at spaces, before it's shown: letter by letter, a word
// never jumps down a line halfway through.
void wrapLine(App& app, char* s, float width) {
    int start = 0, space = -1;
    auto tooWide = [&](int end) {
        const char keep = s[end];
        s[end] = 0;
        const bool wide = textWidth(app, s + start, kTextScale) > width;
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
    const u8 before = d.line > 0 ? d.feel : 0xFF;
    const char* line = d.talk.lines[d.line];
    const char* rest = line;
    d.feel = d.talk.feel[d.line];
    const story::Feel tagged = story::splitFeel(line, &rest);  // (a "[tag]" on the game's own lines)
    if (rest != line) d.feel = static_cast<u8>(tagged);
    d.speaker = d.talk.speaker[d.line];
    fillLine(rest, app.game, d.text, sizeof(d.text));
    const Who w = whoNow(app);
    wrapLine(app, d.text, w.narration ? kTextWidth + 56 : kTextWidth);
    d.shown = 0;
    d.blipFor = 0;
    if (!w.narration && w.voice != d.voiceLoaded) {  // (someone else speaks: their voice)
        audio::loadVoice(w.voice);
        d.voiceLoaded = w.voice;
    }
    // Its feeling pops (a new feeling, or the first line's): the icon and its sound.
    const story::Feel f = static_cast<story::Feel>(d.feel);
    if (!w.narration && emote::hasIcon(f) && d.feel != before) {
        d.emoteT = 0;
        emote::play(f);
    } else if (!emote::hasIcon(f) || w.narration) {
        d.emoteT = -1;
    }
}

bool begin(App& app) {
    DialogueState& d = app.talk;
    if (d.talk.count == 0) return false;
    d.active = true;
    d.voiceLoaded = 0xFF;
    loadLine(app);
    return true;
}

void finish(App& app) {
    DialogueState& d = app.talk;
    d.active = false;
    audio::freeVoice();
    freeFace();
    const s64 now = nowLocal(app);
    if (d.talk.rule >= 0 || d.talk.pickup >= 0 || d.talk.person >= 0) {  // the story's (D137)
        story::News n;
        if (story::finishTalk(app.game, d.talk, now, &n)) {
            storyNews(app, n, false);
            saveNow(app);
        }
    } else if (d.talk.sets) {
        const u32 was = app.game.world.flags;
        app.game.world.flags |= d.talk.sets;
        if (app.game.world.flags != was) storyUpdate(app);
    }
}

}  // namespace

bool startStoryTalk(App& app, int person) {
    DialogueState& d = app.talk;
    d = DialogueState{};
    const int v = person >= 0 ? story::person(person).villager : -1;
    d.who = v >= 0 ? static_cast<Villager>(v) : Villager::Keeper;
    d.talk = story::talkTo(app.game, person, nowLocal(app));
    return begin(app);
}

bool startPickupTalk(App& app, int pickup) {
    DialogueState& d = app.talk;
    d = DialogueState{};
    d.talk = story::pickupTalk(app.game, pickup, nowLocal(app));
    return begin(app);
}

void startTalk(App& app, Villager v) {
    const int person = story::personOfVillager(v);
    startStoryTalk(app, person);
    app.talk.who = v;
}

void startLines(App& app, Villager v, const Talk& lines) {
    DialogueState& d = app.talk;
    d = DialogueState{};
    d.who = v;
    d.talk = lines;
    d.talk.sets = 0;
    d.talk.rule = d.talk.pickup = -1;
    d.talk.person = static_cast<s8>(story::personOfVillager(v));
    begin(app);
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
    begin(app);
}

bool talking(const App& app) { return app.talk.active; }

int talkSpeaker(const App& app) {
    if (!app.talk.active) return -1;
    return app.talk.speaker >= 0 ? app.talk.speaker : app.talk.talk.person;
}

bool updateTalk(App& app, const Input& in) {
    DialogueState& d = app.talk;
    if (!d.active) return false;
    if (d.emoteT >= 0) d.emoteT += app.dt;
    const Who w = whoNow(app);
    const emote::Voice voice = emote::voiceOf(static_cast<story::Feel>(d.feel));
    const int len = static_cast<int>(std::strlen(d.text));
    const int before = static_cast<int>(d.shown);
    if (d.shown < len) {
        d.shown += kLettersPerSecond * voice.speed * app.dt;
        if (d.shown > len) d.shown = static_cast<float>(len);
        // A voiced blip every other letter shown (a letter's sound, quick and high), tuned by the feeling.
        for (int k = app.game.settings.voiceEnabled && !w.narration ? before : len; k < static_cast<int>(d.shown); ++k)
            if (k % 2 == 0 && ((d.text[k] >= 'a' && d.text[k] <= 'z') || (d.text[k] >= 'A' && d.text[k] <= 'Z'))) {
                const float wobble = voice.wobble * std::sin(static_cast<float>(k) * 2.3f);
                audio::playLetter(d.text[k], w.pitch * voice.pitch * (0.95f + 0.1f * ((k * 7) % 5) / 4.0f + wobble), voice.volume);
            }
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
    return true;
}

void drawTalk(App& app) {
    const DialogueState& d = app.talk;
    if (!d.active) return;
    const Who w = whoNow(app);
    const story::Feel feel = static_cast<story::Feel>(d.feel);
    const Rect box{8, 134, 304, 98};
    panel({box.x - 2, box.y - 2, box.w + 4, box.h + 4}, withAlpha(theme::kClutchGold, 0.9f));
    panel(box, w.narration ? theme::rgba(244, 236, 250) : theme::rgba(252, 244, 228));
    char shown[160];
    const int n = static_cast<int>(d.shown);
    std::memcpy(shown, d.text, static_cast<std::size_t>(n));
    shown[n] = 0;
    if (w.narration) {  // narration: no name, no face, the words across the whole box
        text(app, shown, box.x + 14, box.y + 10, kTextScale, withAlpha(theme::kDenPlum, 0.85f), C2D_AlignLeft);
    } else {
        // The name on a tab above the box, the portrait at its left in a round frame.
        const float nameW = textWidth(app, w.name, 0.55f) + 22;
        panel({box.x + 58, box.y - 20, nameW, 22}, theme::kDenPlum);
        text(app, w.name, box.x + 69, box.y - 17, 0.55f, theme::kClutchGold, C2D_AlignLeft, nameW);
        C2D_DrawCircleSolid(box.x + 30, box.y + 30, 0.5f, 26, theme::kDenPlum);
        C2D_DrawCircleSolid(box.x + 30, box.y + 30, 0.5f, 23, fromRgb(w.tint));
        const float bob = std::sin(app.t * 7.0f) * (d.shown < std::strlen(d.text) ? 1.2f : 0.0f);  // talking
        const float shake = (feel == story::Feel::Shock || feel == story::Feel::Angry) && d.emoteT >= 0 && d.emoteT < 0.4f
                                ? std::sin(d.emoteT * 70.0f) * 1.5f
                                : 0.0f;
        C2D_SpriteSheet face = w.person >= 0 ? faceOf(w.person) : nullptr;
        if (face && C2D_SpriteSheetCount(face) > 0) {
            const std::size_t count = C2D_SpriteSheetCount(face);
            const std::size_t frame = static_cast<std::size_t>(emote::portraitFrame(feel)) < count
                                          ? static_cast<std::size_t>(emote::portraitFrame(feel)) : 0;
            const C2D_Image img = C2D_SpriteSheetGetImage(face, frame);
            C2D_DrawImageAt(img, box.x + 30 - 24 + shake, box.y + 30 - 26 + bob, 0.5f, nullptr, 0.75f, 0.75f);
        } else {
            if (!g_portraits) g_portraits = C2D_SpriteSheetLoadFromMem(people_t3x, people_t3x_size);
            if (g_portraits && w.villager >= 0 && static_cast<std::size_t>(w.villager) < C2D_SpriteSheetCount(g_portraits)) {
                const C2D_Image img = C2D_SpriteSheetGetImage(g_portraits, static_cast<std::size_t>(w.villager));
                C2D_DrawImageAt(img, box.x + 30 - 24 + shake, box.y + 30 - 26 + bob, 0.5f, nullptr, 0.75f, 0.75f);
            } else if (w.name[0]) {  // no portrait: their initial
                const char initial[2] = {w.name[0], 0};
                textCentered(app, initial, box.x + 30 + shake, box.y + 30 + bob, 0.9f, theme::kDenPlum, 40, Face::Title);
            }
        }
        if (d.emoteT >= 0) emote::draw(app, feel, box.x + 50, box.y + 6, 18.0f, d.emoteT);
        text(app, w.title, box.x + 30, box.y + 60, 0.32f, withAlpha(theme::kDenPlum, 0.7f), C2D_AlignCenter, 56);
        text(app, shown, box.x + 66, box.y + 10, kTextScale, theme::kDenPlum, C2D_AlignLeft);
    }
    if (n >= static_cast<int>(std::strlen(d.text))) {  // a little arrow: A for more
        const float bob = 2 * std::sin(app.t * 6);
        C2D_DrawTriangle(box.x + box.w - 18, box.y + box.h - 16 + bob, theme::kDenPlum, box.x + box.w - 8,
                         box.y + box.h - 16 + bob, theme::kDenPlum, box.x + box.w - 13, box.y + box.h - 9 + bob,
                         theme::kDenPlum, 0.5f);
    }
}

}  // namespace ec
