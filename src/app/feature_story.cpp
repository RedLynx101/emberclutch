// The story in the valley (D137, the Living Valley pass): the story's own people where its spots put
// them (Fig asleep in the Whisperwood or about his daily round, Primrose at the Glade, Tove at the
// Vault after your first glide), the things to pick up and the signs to read (a glint and a prompt),
// and the mailbox by the den's door. A beside one: their talk (core/story), the pickup's words, or
// the mailbox, which has the bottom screen while it's open. Everyone who belongs to a feature of
// their own (the fisher, the Hollow's keeper, the Glade's people, the champions) stays theirs.
#include <cmath>
#include <cstdio>
#include <cstring>

#include "app/audio.hpp"
#include "app/dialogue.hpp"
#include "app/glade_show.hpp"
#include "app/scenes.hpp"
#include "app/story_app.hpp"
#include "app/strings.hpp"
#include "app/valley_ext.hpp"
#include "app/wildlife.hpp"
#include "core/behavior.hpp"
#include "core/kinds.hpp"
#include "core/people.hpp"
#include "core/challenge_mesh.hpp"
#include "core/clock.hpp"
#include "core/daylight.hpp"
#include "core/place_layout.hpp"
#include "core/rig.hpp"
#include "core/story.hpp"
#include "core/valley.hpp"

namespace ec {
namespace {

constexpr float kPi = 3.14159265f;
constexpr u8 kPickupId = 100;  // Folk ids: a person's own (story PersonId), 100 + a pickup, the mailbox
constexpr u8 kMailboxId = 250;

// The story's own people this feature stands about (their bodies: dressAs, D138).
constexpr int kOwnPeople[] = {story::kPFig, story::kPPrimrose, story::kPTove};

// Cinder, Rowan's old Blazeplume, and Custard, Bram's sheepdog (D138): not people, so not folk
// drawn as figures. Cinder is a dragon in the view's others (asleep on the porch, awake when
// spoken to or on the festival night); Custard is added to the critters' triangles, his tail going.
Dragon g_cinder;
DenActor g_cinderActor;
bool g_cinderMade = false;
ClipId g_cinderClip = ClipId::Count;
int g_cinderLook = -1;  // (the clip library his clip came from: replayed when his own kind loads)
float g_dogClock = 0;

void makeCinder() {
    g_cinder = Dragon{};
    const int kind = findKind("blazeplume");
    Rng rng(0xC1DE5ull);
    rollKind(g_cinder, kind >= 0 ? kind : 0, 0, rng);
    g_cinder.id = 0xC1D00001u;  // (never a save's)
    g_cinder.stage = Stage::Adult;
    g_cinder.genome.size = 235;  // a big old fellow
    g_cinder.genome.build = 1;
    std::snprintf(g_cinder.name, sizeof(g_cinder.name), "Cinder");
    g_cinderMade = true;
}

Vec3 where(const Valley& v, int place, Vec2 local) {
    const ValleyPlaceInfo* p = v.place(static_cast<u8>(place));
    return p ? placeToWorld3(v, *p, {local.x, local.y, 0}) : Vec3{};
}

float headingAt(const Valley& v, int place, float facing) {
    const ValleyPlaceInfo* p = v.place(static_cast<u8>(place));
    return (p ? p->heading : 0.0f) + facing;
}

// The mailbox: beside the den's door, a little to its right as you come out.
Vec3 mailboxAt(const Valley& v) {
    const PlaceLayout& l = placeLayout(kPlaceDen);
    const Vec2 door = l.hasDoor ? l.door : Vec2{0, 0};
    return where(v, kPlaceDen, {door.x + 2.2f, door.y + 1.4f});
}

int folk(const App& app, const Valley& v, Vec3 near, float radius, vext::Folk* out, int cap) {
    const SaveData& s = app.game;
    const s64 now = nowLocal(app);
    int n = 0;
    auto nearby = [&](Vec3 at) { return std::hypot(at.x - near.x, at.y - near.y) <= radius; };
    // The story's own people, where their spots put them.
    for (const int who : kOwnPeople) {
        story::Spot sp;
        if (n >= cap || !story::spotOf(s, who, now, sp)) continue;
        if (who == story::kPPrimrose && glade::showOn()) continue;  // (on the stage with Duchess, D138)
        vext::Folk& f = out[n];
        f = vext::Folk{};
        dressAs(who, f.look);
        f.look.at = where(v, sp.place, sp.at);
        f.look.heading = headingAt(v, sp.place, sp.facing);
        if (!nearby(f.look.at)) continue;
        f.shown = true;
        f.name = story::person(who).name;
        f.prompt = str::kPromptTalk;
        f.id = static_cast<u8>(who);
        f.person = static_cast<s8>(who);
        f.voice = story::person(who).voice;
        f.pitch = story::person(who).pitch;
        f.reach = 2.6f;
        f.clip = sp.clip[0] ? sp.clip : nullptr;
        ++n;
    }
    // Cinder and Custard, where the story has them (spoken to as the story's own: their talks).
    for (const int who : {story::kPCinder, story::kPCustard}) {
        story::Spot sp;
        if (n >= cap || !story::spotOf(s, who, now, sp)) continue;
        vext::Folk& f = out[n];
        f = vext::Folk{};
        f.look.at = where(v, sp.place, sp.at);
        if (!nearby(f.look.at)) continue;
        f.shown = false;
        f.name = story::person(who).name;
        f.prompt = who == story::kPCustard ? str::kPromptPet : str::kPromptTalk;
        f.id = static_cast<u8>(who);
        f.person = static_cast<s8>(who);
        f.reach = who == story::kPCinder ? 3.4f : 2.0f;
        ++n;
    }
    // Things to pick up and signs to read.
    story::Pickup found[16];
    const int picks = story::pickups(s, now, found, 16);
    for (int k = 0; k < picks && n < cap; ++k) {
        vext::Folk& f = out[n];
        f = vext::Folk{};
        f.look.at = where(v, found[k].place, found[k].at);
        if (!nearby(f.look.at)) continue;
        f.shown = false;
        f.name = "";
        f.prompt = found[k].prompt;
        f.id = static_cast<u8>(kPickupId + found[k].index);
        f.reach = 2.2f;
        ++n;
    }
    // The mailbox.
    if (n < cap) {
        vext::Folk& f = out[n];
        f = vext::Folk{};
        f.look.at = mailboxAt(v);
        if (nearby(f.look.at)) {
            f.shown = false;
            f.name = "";
            f.prompt = story::unreadMail(s) ? str::kPromptMailNew : str::kPromptMail;
            f.id = kMailboxId;
            f.reach = 2.0f;
            ++n;
        }
    }
    return n;
}

void act(App& app, const vext::Folk& who, vext::Stage& st) {
    (void)st;
    if (who.id == kMailboxId) {
        openMailbox(app);
    } else if (who.id >= kPickupId) {
        startPickupTalk(app, who.id - kPickupId);
    } else {
        if (who.id == story::kPCustard) audio::playSfx(audio::Sfx::Bark, 1.0f, 0.8f);  // (wuff!)
        startStoryTalk(app, who.id);
    }
}

// Every frame: Cinder's breathing and waking, Custard's tail.
void tick(App& app, const vext::Stage& st) {
    g_dogClock += app.dt;
    story::Spot sp;
    if (!st.valley || !story::spotOf(app.game, story::kPCinder, nowLocal(app), sp)) return;
    const Vec3 at = where(*st.valley, sp.place, sp.at);
    if (std::hypot(at.x - st.you.x, at.y - st.you.y) > 80.0f) return;
    if (!g_cinderMade) makeCinder();
    if (!r3d::kindReady(g_cinder.kind)) {  // (his own model first: a clip index means another clip in another library)
        r3d::wantKind(g_cinder.kind);
        return;
    }
    if (r3d::lookFor(g_cinder) != g_cinderLook) {
        g_cinderLook = r3d::lookFor(g_cinder);
        g_cinderClip = ClipId::Count;
    }
    const AnimLibrary* lib = r3d::animsFor(g_cinder);
    if (!lib) return;
    const int* clips = r3d::clipIndexFor(g_cinder, kFormGrown);
    const bool spoken = talkSpeaker(app) == story::kPCinder;
    ClipId want = ClipId::Idle;
    if (std::strcmp(sp.clip, "sleep") == 0 && !spoken)  // (asleep: whichever the grown form has)
        for (ClipId c : {ClipId::Sleep, ClipId::CurlUp, ClipId::LieLoop})
            if (clips[static_cast<int>(c)] >= 0) {
                want = c;
                break;
            }
    if (want != g_cinderClip && clips[static_cast<int>(want)] >= 0) {
        g_cinderActor.anim.play(clips[static_cast<int>(want)], 0.6f, true);
        g_cinderClip = want;
    }
    g_cinderActor.anim.update(*lib, app.dt, nullptr, 0);
    g_cinderActor.eyes.update(want != ClipId::Idle ? 1.0f : 0.0f, app.dt);
}

bool active(const App& app) { return mailboxOpen(app); }

void update(App& app, const Input& in, vext::Stage& st) {
    (void)st;
    updateMailbox(app, in);
}

void drawBottom(App& app, const Input& in, const vext::Stage& st) {
    (void)st;
    drawMailbox(app, in);
}

// While nobody has the valley: the pickups' glints (as the finds glint), Cinder and Custard.
void ambient(App& app, const vext::Stage& st, r3d::ValleyView& view) {
    if (!st.valley) return;
    const Valley& v = *st.valley;
    const s64 now = nowLocal(app);
    story::Spot sp;
    if (g_cinderMade && story::spotOf(app.game, story::kPCinder, now, sp)) {
        const Vec3 at = where(v, sp.place, sp.at);
        if (std::hypot(at.x - view.eye.x, at.y - view.eye.y) < 70.0f && view.otherCount < r3d::kMaxOthers) {
            if (!r3d::kindReady(g_cinder.kind)) {
                r3d::wantKind(g_cinder.kind);
            } else {
                r3d::ValleyDragon& d = view.others[view.otherCount++];
                d = r3d::ValleyDragon{};
                d.dragon = &g_cinder;
                d.actor = &g_cinderActor;
                d.at = at;
                d.heading = headingAt(v, sp.place, sp.facing);
                d.lodFar = 12.0f;  // (his lighter model past a few steps: the Lodge is a busy picture)
            }
        }
    }
    if (story::spotOf(app.game, story::kPCustard, now, sp)) {
        const Vec3 at = where(v, sp.place, sp.at);
        critters::Mesh* m = wildlife::frameMesh();
        if (m && view.critterPos == m->pos && std::hypot(at.x - view.eye.x, at.y - view.eye.y) < 44.0f) {
            critters::DogPose p;
            p.at = at;
            p.heading = headingAt(v, sp.place, sp.facing);
            if (std::hypot(st.you.x - at.x, st.you.y - at.y) < 7.0f)  // (he turns to you, tail going hard)
                p.heading = std::atan2(st.you.x - at.x, -(st.you.y - at.y));
            p.wag = talkSpeaker(app) == story::kPCustard || std::hypot(st.you.x - at.x, st.you.y - at.y) < 7.0f ? 1.0f : 0.4f;
            p.sit = hourOfDay(now) >= 20 || hourOfDay(now) < 6 ? 1.0f : 0.0f;  // (flopped down for the night)
            p.clock = g_dogClock;
            critters::addDog(*m, p);
            view.critterVerts = m->verts;
        }
    }
    story::Pickup found[16];
    const int picks = story::pickups(app.game, nowLocal(app), found, 16);
    for (int k = 0; k < picks && view.glintCount < r3d::kMaxGlints; ++k) {
        if (found[k].sign || !found[k].glint) continue;
        const Vec3 at = where(v, found[k].place, found[k].at);
        if (std::hypot(at.x - view.eye.x, at.y - view.eye.y) < 60.0f) view.glints[view.glintCount++] = {at.x, at.y, at.z + 0.35f};
    }
}

}  // namespace

const vext::Feature kStoryFeature{"story", folk, act, active, update, nullptr, nullptr, drawBottom, tick, ambient, nullptr};

// The mailbox (its flag up while a letter waits) and the signs, after the valley with its camera.
void drawStoryProps(App& app, const Valley& v, s64 now) {
    r3d::ChallengeProp props[6];
    int n = 0;
    bool near = false;
    auto add = [&](r3d::PropKind kind, Vec3 at, float yaw, u8 variant, const PropLook& look) {
        if (n >= 6) return;
        r3d::ChallengeProp& p = props[n++];
        p.kind = kind;
        p.variant = variant;
        p.at = at;
        p.yaw = yaw;
        p.look = look;
        float x, y, ppu;
        near = near || (r3d::project(at, x, y, ppu) && ppu > 0.3f);
    };
    add(r3d::PropKind::Mailbox, mailboxAt(v), headingAt(v, kPlaceDen, kPi), story::unreadMail(app.game) > 0 ? 1 : 0, mailboxLook());
    story::Pickup found[16];
    const int picks = story::pickups(app.game, now, found, 16);
    for (int k = 0; k < picks; ++k)
        if (found[k].sign) add(r3d::PropKind::Sign, where(v, found[k].place, found[k].at), headingAt(v, found[k].place, kPi), 0, signLook());
    if (!near || n == 0) return;
    Rgb top, horizon, tint;
    valleySky(now, top, horizon, tint);
    r3d::drawChallengeProps(app, props, n, horizon, now);
}

}  // namespace ec
