// The story in the valley (D137, the Living Valley pass): the story's own people where its spots put
// them (Fig asleep in the Whisperwood or about his daily round, Primrose at the Glade, Tove at the
// Vault after your first glide), the things to pick up and the signs to read (a glint and a prompt),
// and the mailbox by the den's door. A beside one: their talk (core/story), the pickup's words, or
// the mailbox, which has the bottom screen while it's open. Everyone who belongs to a feature of
// their own (the fisher, the Hollow's keeper, the Glade's people, the champions) stays theirs.
#include <cmath>
#include <cstdio>
#include <cstring>

#include "app/dialogue.hpp"
#include "app/glade_show.hpp"
#include "app/scenes.hpp"
#include "app/story_app.hpp"
#include "app/strings.hpp"
#include "app/valley_ext.hpp"
#include "core/people.hpp"
#include "core/challenge_mesh.hpp"
#include "core/daylight.hpp"
#include "core/place_layout.hpp"
#include "core/story.hpp"
#include "core/valley.hpp"

namespace ec {
namespace {

constexpr float kPi = 3.14159265f;
constexpr u8 kPickupId = 100;  // Folk ids: a person's own (story PersonId), 100 + a pickup, the mailbox
constexpr u8 kMailboxId = 250;

// How the story's own people look until their bodies are in (the people kit's player bodies, dressed
// their way), and whether they're the story feature's to stand about at all.
struct Look {
    int person;
    Person body;
    u8 hair, hairColour, skin, eyes;
    Rgb outfit[2];
};
constexpr Look kLooks[] = {
    {story::kPFig, Person::PlayerA, 4, 0, 1, 0, {{104, 142, 76}, {236, 196, 96}}},      // Fig: a moss tunic, a gold scarf
    {story::kPPrimrose, Person::PlayerB, 1, 4, 0, 1, {{240, 150, 180}, {255, 236, 244}}},  // Primrose: pink and cream
    {story::kPTove, Person::PlayerB, 2, 5, 0, 3, {{84, 116, 160}, {240, 240, 250}}},     // Tove: her winter blues
};

void dress(const Look& l, r3d::PersonView& p) {
    const u8 look[kLookParts] = {static_cast<u8>(l.body == Person::PlayerB), l.hair, l.hairColour, l.skin, 0, l.eyes};
    p.form = static_cast<u8>(l.body);
    playerPalette(look, p.pal);
    p.pal[kPalAccent] = l.outfit[0];
    p.pal[kPalPattern] = l.outfit[1];
    p.hair = static_cast<s8>(l.hair);
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
    for (const Look& l : kLooks) {
        story::Spot sp;
        if (n >= cap || !story::spotOf(s, l.person, now, sp)) continue;
        if (l.person == story::kPPrimrose && glade::showOn()) continue;  // (on the stage with Duchess, D138)
        vext::Folk& f = out[n];
        f = vext::Folk{};
        dress(l, f.look);
        f.look.at = where(v, sp.place, sp.at);
        f.look.heading = headingAt(v, sp.place, sp.facing);
        if (!nearby(f.look.at)) continue;
        f.shown = true;
        f.name = story::person(l.person).name;
        f.prompt = str::kPromptTalk;
        f.id = static_cast<u8>(l.person);
        f.person = static_cast<s8>(l.person);
        f.voice = story::person(l.person).voice;
        f.pitch = story::person(l.person).pitch;
        f.reach = 2.6f;
        f.clip = sp.clip[0] ? sp.clip : nullptr;
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
        startStoryTalk(app, who.id);
    }
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

// While nobody has the valley: the pickups' glints (as the finds glint).
void ambient(App& app, const vext::Stage& st, r3d::ValleyView& view) {
    if (!st.valley) return;
    const Valley& v = *st.valley;
    story::Pickup found[16];
    const int picks = story::pickups(app.game, nowLocal(app), found, 16);
    for (int k = 0; k < picks && view.glintCount < r3d::kMaxGlints; ++k) {
        if (found[k].sign || !found[k].glint) continue;
        const Vec3 at = where(v, found[k].place, found[k].at);
        if (std::hypot(at.x - view.eye.x, at.y - view.eye.y) < 60.0f) view.glints[view.glintCount++] = {at.x, at.y, at.z + 0.35f};
    }
}

}  // namespace

const vext::Feature kStoryFeature{"story", folk, act, active, update, nullptr, nullptr, drawBottom, nullptr, ambient, nullptr};

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
