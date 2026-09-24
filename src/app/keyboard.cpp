#include "app/keyboard.hpp"

#include <3ds.h>

#include <cstdio>
#include <cstring>

#include "app/audio.hpp"
#include "app/autotest.hpp"
#include "app/strings.hpp"
#include "core/names.hpp"

namespace ec {
namespace {

// One keyboard: `initial` filled in, `left` on the left button, OK on the right. The typed
// text lands in buf.
SwkbdButton ask(const char* hint, const char* initial, const char* left, char* buf, std::size_t cap) {
    if (autotest::active()) {  // a scripted run: no keyboard, the script's name (or the one offered)
        if (!autotest::typedName(buf, cap)) std::snprintf(buf, cap, "%s", initial[0] ? initial : "Tester");
        return SWKBD_BUTTON_RIGHT;
    }
    SwkbdState kb;
    swkbdInit(&kb, SWKBD_TYPE_WESTERN, 2, static_cast<int>(kNameMax - 1));
    swkbdSetHintText(&kb, hint);
    swkbdSetInitialText(&kb, initial);
    swkbdSetButton(&kb, SWKBD_BUTTON_LEFT, left, false);
    swkbdSetButton(&kb, SWKBD_BUTTON_RIGHT, str::kOk, true);
    swkbdSetValidation(&kb, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
    buf[0] = '\0';
    return swkbdInputText(&kb, buf, cap);
}

}  // namespace

void runKeyboard(App& app) {
    const KeyboardFor what = app.keyboard;
    app.keyboard = KeyboardFor::None;
    char buf[64];
    if (what == KeyboardFor::PlayerName) {  // a new game: your name, then the egg
        if (ask(str::kYourNameHint, app.game.playerName, str::kBack, buf, sizeof(buf)) == SWKBD_BUTTON_RIGHT) {
            char name[sizeof(app.game.playerName)];
            if (setName(name, sizeof(name), buf)) {
                resetForNewGame(app);
                std::memcpy(app.game.playerName, name, sizeof(name));
                app.scene = SceneId::PickStarter;
                audio::playSfx(audio::Sfx::Confirm);
            }
        }
        app.titleConfirm = 0;
        return;
    }
    if (!hasDragon(app)) return;
    Dragon& d = activeDragon(app);
    if (what == KeyboardFor::NameHatchling) {
        // "Another" rolls the next suggestion; OK keeps what's typed. If the keyboard can't
        // open at all, the suggestion stands (it can be renamed later).
        char suggestion[kNameMax];
        for (int tries = 0; tries < 100; ++tries) {
            suggestName(d, app.nameRoll++, suggestion, sizeof(suggestion));
            const SwkbdButton b = ask(str::kNameHint, suggestion, str::kAnotherName, buf, sizeof(buf));
            if (b == SWKBD_BUTTON_RIGHT && setName(d.name, sizeof(d.name), buf)) break;
            if (b != SWKBD_BUTTON_LEFT) {
                setName(d.name, sizeof(d.name), suggestion);
                break;
            }
        }
        app.hatch.named = true;
        audio::playSfx(audio::Sfx::Confirm);
    } else if (what == KeyboardFor::Rename) {
        if (ask(str::kRenameHint, d.name, str::kCancel, buf, sizeof(buf)) == SWKBD_BUTTON_RIGHT &&
            setName(d.name, sizeof(d.name), buf)) {
            audio::playSfx(audio::Sfx::Confirm);
            showToast(app, str::kRenamed);
            saveNow(app);
        }
    }
}

}  // namespace ec
