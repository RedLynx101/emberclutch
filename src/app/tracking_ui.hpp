// The Journal's tracked goal on screen (1.0, D89; workstream U; core/guide): its words, a flag
// for it, its marker on the valley's map (a flag where to go, or a soft circle where to search)
// and the valley's side-panel lines for it.
#pragma once

#include "app/app.hpp"
#include "core/guide.hpp"

namespace ec {

struct Valley;

// A goal's title and what to do next ("Market day" / "Light the Market's lantern").
void goalWords(const SaveData& s, const guide::Goal& g, char* title, int titleCap, char* step, int stepCap);

// The tracking flag (the Journal's rows, the map's marker): its pole's foot at (x, y).
void trackFlag(float x, float y, float size, float t, bool on);

// For the lead, from the valley's map (scene_valley drawBottom, after the pins, before your
// heart): the tracked goal's flag, or its search area, on a map of the whole valley drawn at
// (mapX, mapY), mapSize pixels square (north up).
void drawTrackedOnMap(App& app, const Valley& valley, float mapX, float mapY, float mapSize);

// For the lead, in the valley's side panel instead of the quest in hand: a "Tracking" tag, the
// tracked goal's title and its step over two lines, from (x, y) at most w wide (about 44 px tall).
void drawTrackedPanel(App& app, float x, float y, float w);

// Two lines of s at `scale`, each at most w wide: split at the space nearest the middle when it
// doesn't fit on one (the second shrinks if it must). Returns the lines drawn.
int textTwoLines(App& app, const char* s, float x, float y, float scale, u32 color, float w, float lineGap);

}  // namespace ec
