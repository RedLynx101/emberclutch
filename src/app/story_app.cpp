#include "app/story_app.hpp"

#include "app/audio.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "core/finds.hpp"
#include "core/people.hpp"
#include "core/valley.hpp"

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
    // Fig's map, whole again: the map's fog lifts round the places its pages were found (D137).
    if (n.finished == story::kQFigMap)
        if (const Valley* v = loadedValley())
            for (int place : {kPlaceMill, kPlaceOrchard, kPlaceLake, kPlaceKeeper})
                if (const ValleyPlaceInfo* p = v->place(static_cast<u8>(place))) explore(app.game, *v, {p->at.x, p->at.y}, 130.0f);
    // The end of Act 1: the festival night's talk done, the credits roll (the system menu's page).
    if (n.finished == story::kQLanternFestival) app.menu = MenuPage::Credits;
    if (n.finished >= 0 || n.stepped >= 0 || n.started >= 0 || n.mail > 0) saveNow(app);
}

bool dressAs(int person, r3d::PersonView& p) {
    if (person < 0 || person >= story::personCount()) return false;
    const story::PersonInfo& info = story::person(person);
    const Person body = info.villager >= 0 ? personFor(static_cast<Villager>(info.villager)) : personByName(info.body);
    if (body == Person::Count) return false;
    p.form = static_cast<u8>(body);
    personPalette(body, p.pal);
    p.hair = -1;
    return true;
}

story::News storyUpdate(App& app) {
    const story::News n = story::update(app.game, nowLocal(app));
    storyNews(app, n);
    return n;
}

}  // namespace ec
