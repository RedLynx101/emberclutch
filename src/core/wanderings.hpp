// The Wanderings (Alpha 2 WP4, GDD 11): take a juvenile-or-older dragon along, close the 3DS
// and walk; the system pedometer counts your steps. When it comes back it has found things
// in proportion to the distance: Gleam, trinkets for the hoard, and rarely a wild egg (the
// main way to find breeds you didn't start with). One dragon wanders at a time; its needs
// drain as usual and it keeps its bed; it comes home muddy (D46).
#pragma once

#include "core/save.hpp"

namespace ec {

enum class Trinket : u8 { Pebble, Coin, Feather, Crystal, Pearl, Fossil, Count };
constexpr int kTrinkets = static_cast<int>(Trinket::Count);
const char* trinketName(Trinket t);
u32 trinketValue(Trinket t);  // in Gleam, when sold (the Market, WP5)

constexpr u32 kStepsPerFind = 400;  // a chance to find something every this many steps

struct WanderFinds {
    u32 steps = 0;
    u32 gleam = 0;
    u8 trinkets[kTrinkets] = {};
    int wildEgg = -1;  // its SaveData index (in a nest or the Cold Vault), -1: none
};

// Who's out wandering (SaveData index), or -1.
int wandererIndex(const SaveData& s);
// Why this dragon can't go now (nullptr: it can): too young, not in the den, someone's out.
const char* cantWander(const SaveData& s, int index);
// Sets it off: remembers the step counter and the time.
bool setOff(SaveData& s, int index, u32 stepCount, s64 now);
// Steps walked since it set off (the counter can be reset by the system: then all of it).
u32 stepsSince(const Dragon& d, u32 stepCount);
// What a trip of `steps` turns up for this dragon (adults and the curious find more; at most
// one wild egg a trip). Pure: nothing is added anywhere.
WanderFinds rollFinds(const Dragon& d, u32 steps, Rng& rng);
// Brings it home: rolls the finds and adds them to the save (Gleam, the hoard, a wild egg in
// a free nest or the Vault), muddies its legs, belly and tail, and marks the visit.
WanderFinds comeBack(SaveData& s, int index, u32 stepCount, s64 now, Rng& rng);

}  // namespace ec
