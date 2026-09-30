#include "app/story_app.hpp"

#include "app/audio.hpp"
#include "app/strings.hpp"

namespace ec {

void storyNews(App& app, const story::News& n, bool queue) {
    const s64 now = nowLocal(app);
    auto toast = [&](const char* fmt, const char* arg) {
        if (queue)
            queueToastf(app, fmt, arg);
        else
            showToastf(app, fmt, arg);
    };
    if (n.finished >= 0) {
        audio::playStinger("quest-done");
        toast(str::kQuestFinished, story::view(app.game, n.finished, now).title);
    } else if (n.started >= 0) {
        audio::playSfx(audio::Sfx::QuestPage);
        toast(str::kQuestStarted, story::view(app.game, n.started, now).title);
    } else if (n.stepped >= 0) {
        audio::playSfx(audio::Sfx::QuestPage);
    }
    if (n.mail > 0) {
        int box[1];
        if (story::mailbox(app.game, box, 1) == 1) queueToastf(app, str::kMailArrived, story::letter(app.game, box[0]).subject);
        audio::playSfx(audio::Sfx::Notice);
    }
    if (n.starEgg) giveStarEgg(app);
    // The end of Act 1: the festival night's talk done, the credits roll (the system menu's page).
    if (n.finished == story::kQLanternFestival) app.menu = MenuPage::Credits;
    if (n.finished >= 0 || n.stepped >= 0 || n.started >= 0 || n.mail > 0) saveNow(app);
}

story::News storyUpdate(App& app) {
    const story::News n = story::update(app.game, nowLocal(app));
    storyNews(app, n);
    return n;
}

}  // namespace ec
