#include "app/people_acts.hpp"

#include "app/audio.hpp"
#include "app/render3d.hpp"
#include "core/clock.hpp"
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

}  // namespace ec::acts
