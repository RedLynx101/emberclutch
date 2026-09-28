// The hands-on care screen (WP7, docs/design/care-interactions.md): the tool tray under the
// close-up, the tool in hand drawn at the stylus, and what each tool does to the dragon.
#pragma once

#include "app/app.hpp"
#include "app/render3d.hpp"

namespace ec::care {

bool loadSprites();
void freeSprites();
// A food's sprite centred at (x, y) (the Market draws them too).
void drawFood(Food f, float x, float y, float scale);
// A thing from the Market (core/items), its icon centred at (x, y).
void drawItem(Item i, float x, float y, float scale);
// The profile's pages (WP8; 1.0) for any dragon or egg: About / Training / Record / Family tabs
// at y 38 (an egg: About and Family) and the page below them (to y 198). The den's profile uses
// them; so do the Sanctuary and the Cold Vault.
enum ProfileTab : u8 { kTabAbout, kTabTraining, kTabRecord, kTabFamily, kProfileTabs };
void drawProfilePages(App& app, const Input& in, Dragon& d, s64 now, u8& tab);
// Just the About page (the Journal shows it under its own tabs); given the input, the wardrobe's
// button too.
void profileAbout(App& app, const Dragon& d, s64 now, const Input* in = nullptr);
// The trainer's pages (1.0, app/profile_ui): Training (its level, stats and moves; a tap on a
// move swaps it) and Record (titles, wins, cups, ribbons, the Hollow).
void profileTraining(App& app, const Input& in, Dragon& d);
void profileRecord(App& app, const Input& in, const Dragon& d);

// The tray's places (D85, app/care_pages): Outing, Journal and Den over the bottom screen.
// True while one is open (it drew the screen).
bool drawPage(App& app, const Input& in, Dragon& d, s64 now);

// Which close-up the tool in hand wants (the face for petting, feeding and play; the whole
// body for grooming and the bath).
r3d::CloseUpView view(const App& app);

// Once a frame from the den's update, before the dragons move: the ball's physics (and its
// sounds), the ball in a dragon's mouth, the tub, easing the dragon's gaze and jaw.
void update(App& app, Dragon& d);

// The bottom screen over the close-up: gauges, the tray, the tool at the stylus and its
// effects. Call after r3d::drawCloseUp.
void drawBottom(App& app, const Input& in, Dragon& d, s64 now);

}  // namespace ec::care
