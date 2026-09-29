// The valley's people at their doings through the day (workstream D): what each villager does at
// their spot by the hour, from the people's clip library (romfs/anims/person.eca): Maple tidying
// her stall morning and evening, Bram scattering feed from his bucket, Wren writing on her
// clipboard, Pip flying his toy dragon about, Old Rowan and Sable sitting a while in the evening,
// everyone dozing at night (sat down, or on their feet at the stall and the arena), and now and
// then a look about or a stretch. Pure logic (PC-tested: tests/test_roamers.cpp); app/people_acts
// plays it on the villagers.
#pragma once

#include "core/types.hpp"
#include "core/villagers.hpp"

namespace ec::routine {

struct Doing {
    const char* clip = "idle";  // the loop they keep up
    const char* now = nullptr;  // a one-shot now and then while at it (null: none)
    bool seated = false;        // sat on the ground: they stay put (no getting up to wave, no turning)
    bool asleep = false;        // dozing (a soft snore now and then when you're near)
};
Doing villager(Villager v, int hour);
// Seconds to the next now-and-then (8..20 s), spread by a seed so they aren't all in step.
float nextGap(u32 seed);

}  // namespace ec::routine
