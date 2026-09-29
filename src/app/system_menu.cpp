#include "app/system_menu.hpp"

#include <cstdio>

#include "app/audio.hpp"
#include "app/tips_ui.hpp"
#include "app/dragondex_ui.hpp"
#include "app/storage.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"

namespace ec {
namespace {

constexpr u8 kVolumeStep = 10;

void heading(App& app, const char* s) {
    textCentered(app, s, 160, 22, 0.9f, theme::kClutchGold, 300, Face::Title);
}

// One volume row: the label, [-], the level, [+]. Returns true if it changed.
bool volumeRow(App& app, const Input& in, float y, const char* label, u8& level) {
    text(app, label, 24, y + 6, 0.55f, theme::kShell, C2D_AlignLeft, 110);
    bool changed = false;
    if (button(app, {140, y, 36, 30}, "-", in) && level > 0) {
        level = level >= kVolumeStep ? static_cast<u8>(level - kVolumeStep) : 0;
        changed = true;
    }
    const Rect bar{184, y + 11, 84, 9};
    panel(bar, theme::kDusk);
    if (level > 0) panel({bar.x, bar.y, bar.w * level / 100.0f, bar.h}, theme::kClutchGold);
    if (button(app, {276, y, 36, 30}, "+", in) && level < 100) {
        level = static_cast<u8>(level + kVolumeStep > 100 ? 100 : level + kVolumeStep);
        changed = true;
    }
    return changed;
}

void closeMenu(App& app) {
    if (app.menu == MenuPage::Settings && hasDragon(app)) saveNow(app);  // keep the new volumes
    app.menu = MenuPage::Closed;
}

void mainPage(App& app, const Input& in) {
    heading(app, str::kGameTitle);
    const bool inGame = hasDragon(app) && app.scene != SceneId::Title && app.scene != SceneId::PickStarter;
    // (Map: the way out of the den, alone too; out in the valley it only led back to the den's
    // door, run 21: not there)
    const bool mapShown = inGame && app.scene != SceneId::Valley;
    const float step = inGame ? 38.0f : 50.0f, h = inGame ? 32.0f : 36.0f;
    float y = inGame ? 44.0f : 58.0f;
    if (button(app, {60, y, 200, h}, str::kResume, in) || (in.down & KEY_B)) closeMenu(app);
    y += step;
    if (inGame) {
        if (mapShown && button(app, {60, y, 200, h}, str::kMap, in)) {
            closeMenu(app);
            openMap(app);
        }
        if (mapShown) y += step;
        if (button(app, {60, y, 200, h}, str::kDex, in)) openDex(app);
        y += step;
    }
    if (button(app, {60, y, 200, h}, str::kSettings, in)) app.menu = MenuPage::Settings;
    y += step;
    if (button(app, {60, y, 200, h}, hasDragon(app) ? str::kSaveQuit : str::kQuit, in)) {
        if (hasDragon(app)) saveNow(app);
        app.quit = true;
    }
}

// An on/off setting as a button ("3D: On"); a tap flips it. Returns true if it changed.
bool toggleButton(App& app, const Input& in, const Rect& r, const char* label, u8& value) {
    char line[32];
    std::snprintf(line, sizeof(line), str::kToggleIs, label, value ? str::kOn : str::kOff);
    if (!button(app, r, line, in, value ? 0 : withAlpha(theme::kShell, 0.55f))) return false;
    value = value ? 0 : 1;
    return true;
}

// 1.0 (U): the volumes, the 3D and the voices, the tips shown again, your look, the clock's
// note, deleting the save.
void settingsPage(App& app, const Input& in) {
    heading(app, str::kSettings);
    Settings& s = app.game.settings;
    bool changed = volumeRow(app, in, 42, str::kMusic, s.musicVolume);
    changed = volumeRow(app, in, 76, str::kSounds, s.sfxVolume) || changed;
    if (changed) audio::setVolumes(s.musicVolume, s.sfxVolume);
    toggleButton(app, in, {16, 114, 140, 28}, str::kStereo3d, s.stereo3d);  // main.cpp reads it each frame
    toggleButton(app, in, {164, 114, 140, 28}, str::kVoices, s.voiceEnabled);  // the letters' blips (dialogue)
    if (hasDragon(app) && button(app, {16, 148, 140, 28}, str::kTipsAgain, in)) {
        resetTips(app);
        showToast(app, str::kTipsReset);
    }
    if (hasDragon(app) && button(app, {164, 148, 140, 28}, str::kYourLook, in)) {  // the creator, any time
        saveNow(app);
        app.menu = MenuPage::Closed;
        openCreator(app, app.scene);
    }
    text(app, str::kClockNote, 160, 181, 0.38f, withAlpha(theme::kShell, 0.75f), C2D_AlignCenter, 300);
    if (hasDragon(app) && button(app, {16, 200, 136, 34}, str::kDeleteSave, in, theme::kRose))
        app.menu = MenuPage::DeleteAsk;
    if (button(app, {168, 200, 136, 34}, str::kBack, in) || (in.down & KEY_B)) {
        if (hasDragon(app)) saveNow(app);
        app.menu = MenuPage::Main;
    }
}

// Deleting the save asks twice.
void deletePage(App& app, const Input& in, bool sure) {
    heading(app, str::kDeleteSave);
    if (!sure) {
        textCentered(app, str::kDeleteAsk, 160, 82, 0.6f, theme::kShell, 296);
        textCentered(app, str::kDeleteBody, 160, 108, 0.45f, withAlpha(theme::kShell, 0.8f), 296);
        if (button(app, {16, 180, 136, 40}, str::kKeepIt, in) || (in.down & KEY_B)) app.menu = MenuPage::Settings;
        if (button(app, {168, 180, 136, 40}, str::kDelete, in, theme::kRose)) app.menu = MenuPage::DeleteSure;
        return;
    }
    textCentered(app, str::kDeleteSure, 160, 94, 0.55f, theme::kShell, 296);
    if (button(app, {16, 180, 136, 40}, str::kNo, in) || (in.down & KEY_B)) app.menu = MenuPage::Settings;
    if (button(app, {168, 180, 136, 40}, str::kYesDelete, in, theme::kRose)) {
        deleteGame();
        app.slots = SaveSlots{};
        resetForNewGame(app);
        app.scene = SceneId::Title;
        app.menu = MenuPage::Closed;
        showToast(app, str::kDeleted);
    }
}

}  // namespace

void toggleSystemMenu(App& app) {
    if (app.menu == MenuPage::Closed) {
        app.menu = MenuPage::Main;
        audio::playSfx(audio::Sfx::Tap);
    } else {
        closeMenu(app);
        audio::playSfx(audio::Sfx::Back);
    }
}

void drawSystemMenu(App& app, const Input& in) {
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    embers(app.t, kBotW);
    switch (app.menu) {
        case MenuPage::Main: mainPage(app, in); break;
        case MenuPage::Settings: settingsPage(app, in); break;
        case MenuPage::DeleteAsk: deletePage(app, in, false); break;
        case MenuPage::DeleteSure: deletePage(app, in, true); break;
        case MenuPage::Dex: drawDexBottom(app, in); break;
        default: break;
    }
}

void dimTopForMenu(App& app) {
    (void)app;
    C2D_DrawRectSolid(0, 0, 0.5f, kTopW, kScreenH, withAlpha(theme::kDenPlum, 0.6f));
}

}  // namespace ec
