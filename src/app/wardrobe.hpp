// The wardrobe (1.0, D90; app/scene_wardrobe.cpp): dress a dragon in what you own, one slot at a
// time, and dye it; it turns on the top screen as it tries things on. Opened from Moonpetal Glade
// (the show's board and the stalls) and from the den (openWardrobe with SceneId::Den).
#pragma once

#include "app/scenes.hpp"

namespace ec {

extern const SceneFns kWardrobeScene;

// Opens the wardrobe for dragon `index` (SaveData::dragons; an egg has nothing to wear); Done or B
// goes back to `back`: SceneId::Den the den, SceneId::Valley out at Moonpetal Glade.
void openWardrobe(App& app, int index, SceneId back);
// The music under it: whatever was playing where it was opened (main.cpp musicFor).
const char* wardrobeMusic(const App& app);
// Scripted runs: the dragon turned to `spin` radians (and held there while the pad is untouched).
void wardrobeTurn(float spin);
void wardrobeTab(int tab);  // 0-3 the slots, 4 the dye

// Little pictures for lists (app/wear_icons.cpp): an accessory in its colours, a dye's swatch,
// centred at (x, y), about `size` pixels across.
void drawAccessoryIcon(int accessory, float x, float y, float size);
void drawDyeSwatch(int dye, float x, float y, float size);

}  // namespace ec
