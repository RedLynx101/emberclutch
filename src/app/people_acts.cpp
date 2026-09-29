#include "app/people_acts.hpp"

#include <cmath>

#include "app/audio.hpp"
#include "app/glade_show.hpp"
#include "core/clock.hpp"
#include "core/league.hpp"
#include "core/people.hpp"
#include "core/routines.hpp"
#include "core/villagers.hpp"

namespace ec::acts {
namespace {

// Each villager's own: the one-shot playing (if any), the time to the next, the next snore.
struct Rest {
    bool started = false;
    const char* once = nullptr;
    float nextNow = 6.0f;
    float snoreIn = 2.0f;
};
Rest g_rest[kVillagers];

routine::Doing doingOf(const App& app, int k) {
    return routine::villager(static_cast<Villager>(k < 0 || k >= kVillagers ? 0 : k), hourOfDay(nowLocal(app)));
}

// The audience: two keepers from about the valley, in their own colours, stood by the stage's left
// front corner (in the wide shots, beside the results' card rather than under it on the benches;
// two keeps the view in budget, the judges being there too).
constexpr int kAudience = 2;
constexpr Rgb kFixedBoots{110, 78, 56};
const league::Look kLooks[kAudience] = {
    {1, 1, {240, 192, 152}, {132, 78, 44}, {80, 160, 160}, {250, 238, 206}, kFixedBoots, {76, 128, 78}},
    {0, 0, {164, 108, 72}, {46, 40, 46}, {222, 172, 64}, {96, 64, 44}, kFixedBoots, {86, 56, 40}},
};
// Where they stand (the glade's frame: the stage's middle is (0, -8), its radius 4.6).
constexpr Vec2 kSpots[kAudience] = {{-6.0f, -3.2f}, {-6.9f, -1.6f}};
Animator g_audience[kAudience];
float g_clapIn = 0;

}  // namespace

bool villagerStill(const App& app, int villager) {
    const routine::Doing d = doingOf(app, villager);
    return d.seated || d.asleep;
}

void villagerRest(App& app, int k, Animator& anim, float dist) {
    const AnimLibrary* lib = r3d::personAnims();
    if (!lib || k < 0 || k >= kVillagers) return;
    Rest& r = g_rest[k];
    const routine::Doing d = doingOf(app, k);
    if (r.once && anim.clip == lib->find(r.once) && !anim.finished(*lib)) return;  // (a now-and-then plays out)
    r.once = nullptr;
    r.nextNow -= app.dt;
    if (d.now && r.nextNow <= 0 && dist < 60.0f) {
        r.nextNow = routine::nextGap(app.rng.next());
        const int c = lib->find(d.now);
        if (c >= 0) {
            anim.play(c, 0.3f, true);
            anim.rate = 1.0f;
            r.once = d.now;
            return;
        }
    }
    const int loop = lib->find(d.clip);
    if (loop >= 0 && anim.clip != loop) {
        anim.play(loop, d.seated ? 0.6f : 0.3f);  // (a slow ease down onto the ground)
        anim.rate = 1.0f;
        if (!r.started) anim.time = k * 0.7f;  // (not all breathing together)
    }
    r.started = true;
    if (d.asleep && dist < 7.0f && (r.snoreIn -= app.dt) <= 0) {  // a soft snore, now and then
        r.snoreIn = 3.5f + static_cast<float>(app.rng.below(2500)) * 0.001f;
        audio::playSfx(audio::Sfx::Snore, static_cast<Villager>(k) == Villager::Child ? 1.3f : 1.0f, 0.5f);
    }
}

bool addPerson(r3d::ValleyView& view, const r3d::PersonView& p) {
    if (view.peopleCount < r3d::kMaxPeopleShown) {
        view.people[view.peopleCount++] = p;
        return true;
    }
    const float mine = std::hypot(p.at.x - view.eye.x, p.at.y - view.eye.y);
    int far = -1;
    float farthest = mine;
    for (int i = 1; i < view.peopleCount; ++i) {  // (never the first: you)
        const float d = std::hypot(view.people[i].at.x - view.eye.x, view.people[i].at.y - view.eye.y);
        if (d > farthest) {
            farthest = d;
            far = i;
        }
    }
    if (far < 0) return false;
    view.people[far] = p;
    return true;
}

void showAudience(App& app, const Valley& v, r3d::ValleyView& view, bool applause) {
    const AnimLibrary* lib = r3d::personAnims();
    if (!lib || !glade::gladePlace(v)) return;
    const int clip = lib->find(applause ? "clap" : "idle");
    const Vec3 stage = glade::gladePoint(v, {0.0f, -8.0f}, 0.0f);
    for (int k = 0; k < kAudience; ++k) {
        Animator& a = g_audience[k];
        if (clip >= 0 && a.clip != clip) {
            a.play(clip, 0.3f);
            a.time = 0.23f * k;  // (not all in step)
            a.rate = 0.9f + 0.1f * k;
        }
        a.update(*lib, app.dt, nullptr, 0);
        r3d::PersonView p;
        p.form = static_cast<u8>(kLooks[k].body ? Person::PlayerB : Person::PlayerA);
        league::palette(kLooks[k], p.pal);
        p.hair = static_cast<s8>(kLooks[k].hair);
        p.at = glade::gladePoint(v, kSpots[k], 0.0f);
        p.heading = glade::headingTo(p.at, stage);
        p.anim = &a;
        addPerson(view, p);
    }
    if (applause && (g_clapIn -= app.dt) <= 0) {
        g_clapIn = 1.1f + static_cast<float>(app.rng.below(500)) * 0.001f;
        audio::playSfx(audio::Sfx::Clap, 1.0f, 0.55f);
    }
}

}  // namespace ec::acts
