// Choose your first egg (D18): since DR3 one of the three common kinds, the Pouncer (Ember),
// the Puffback (Grove) or the Curlstone (Stone) (D79).
#include <cmath>
#include <cstdio>

#include "app/audio.hpp"
#include "app/autotest.hpp"
#include "app/scenes.hpp"
#include "app/strings.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/genetics.hpp"
#include "core/kinds.hpp"

namespace ec {
namespace {

constexpr Element kStarters[3] = {Element::Ember, Element::Grove, Element::Tide};  // (the old genome's: sex, size, food)
const char* const kStarterKinds[3] = {"pouncer", "puffback", "curlstone"};

int starterKind(int i) {
    const int k = findKind(kStarterKinds[i]);
    return k >= 0 ? k : 0;
}

void drawTop(App& app) {
    verticalGradient(0, 0, kTopW, kScreenH, theme::kDenPlum, theme::kDusk);
    embers(app.t, kTopW);
    const KindInfo& k = kindInfo(starterKind(app.starterHover));
    const float pulse = 0.75f + 0.25f * std::sin(app.t * 2.0f);
    egg(200, 120, 70, 92, k.variants[0].egg[0], elementGlow(k.elements[0]), pulse);
    char line[64];
    std::snprintf(line, sizeof(line), "%s %s", k.title, str::kEggSuffix);
    textCentered(app, line, 200, 30, 0.95f, theme::kClutchGold, 380, Face::Title);
    text(app, k.blurb, 200, 184, 0.5f, theme::kShell, C2D_AlignCenter, 380);
}

void chooseStarter(App& app, int i) {
    const s64 now = nowLocal(app);
    // A scripted run (autotest) gets the same dragons every time.
    app.rng = Rng(autotest::active() ? 0xA070ull : static_cast<std::uint64_t>(osGetTime()) ^ 0xEC0DDull);
    const Genome g = makePurebred(kStarters[i], app.rng);
    const Sex sex = rollSex(app.rng);
    app.game.dragons[0] = makeEgg(app.game.nextId++, g, sex, now, rollLook(app.rng));
    rollKind(app.game.dragons[0], starterKind(i), static_cast<int>(app.rng.below(kKindVariants - 1)), app.rng);
    std::snprintf(app.game.dragons[0].name, sizeof(app.game.dragons[0].name), "%s", str::kDefaultName);
    app.game.dragonCount = 1;
    app.game.pouch[static_cast<int>(kStarters[i])] = 3;  // a few of the breed's favourite food
    app.game.lastSim = now;
    app.scene = SceneId::Den;
    audio::playSfx(audio::Sfx::Confirm);
    saveNow(app);
}

void drawBottom(App& app, const Input& in) {
    verticalGradient(0, 0, kBotW, kScreenH, theme::kDusk, theme::kDenPlum);
    textCentered(app, str::kChooseEgg, 160, 28, 0.7f, theme::kShell, 300, Face::Title);
    if (in.down & KEY_LEFT) app.starterHover = (app.starterHover + 2) % 3;
    if (in.down & KEY_RIGHT) app.starterHover = (app.starterHover + 1) % 3;
    for (int i = 0; i < 3; ++i) {
        const Rect r{16.0f + i * 100.0f, 60, 88, 120};
        const bool selected = i == app.starterHover;
        panel(r, selected ? theme::kShell : withAlpha(theme::kShell, 0.35f));
        const KindInfo& k = kindInfo(starterKind(i));
        egg(r.x + r.w / 2, r.y + 50, 36, 48, k.variants[0].egg[0], elementGlow(k.elements[0]), selected ? 0.9f : 0.5f);
        text(app, k.title, r.x + r.w / 2, r.y + 92, 0.5f, theme::kDenPlum);
        if (in.released && r.contains(in.rx, in.ry)) {
            if (selected) {
                chooseStarter(app, i);
                return;
            }
            app.starterHover = i;
        }
    }
    if (in.down & KEY_A) chooseStarter(app, app.starterHover);
    text(app, str::kTapAgain, 160, 204, 0.45f, theme::kAsh);
}

}  // namespace

const SceneFns kStarterScene{nullptr, drawTop, drawBottom};

}  // namespace ec
