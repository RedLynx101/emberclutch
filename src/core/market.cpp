#include "core/market.hpp"

#include "core/clock.hpp"
#include "core/den_roster.hpp"
#include "core/genetics.hpp"

namespace ec {
namespace {

constexpr u16 kPouchMax = 99;
constexpr u32 kDailyEggPrice = 250;

}  // namespace

bool marketOpen(const SaveData& s) {
    for (int i = 0; i < s.dragonCount; ++i)
        if (s.dragons[i].stage >= Stage::Juvenile) return true;
    return false;
}

u32 foodPrice(Food f) {
    static constexpr u32 kPrice[static_cast<int>(Food::Count)] = {15, 15, 15, 15, 15, 15, 5, 12, 20, 25};
    return f < Food::Count ? kPrice[static_cast<int>(f)] : 0;
}

int pouchCount(const SaveData& s, Food f) { return f < Food::Count ? s.pouch[static_cast<int>(f)] : 0; }

bool buyFood(SaveData& s, Food f) {
    if (f >= Food::Count || s.gleam < foodPrice(f) || s.pouch[static_cast<int>(f)] >= kPouchMax) return false;
    s.gleam -= foodPrice(f);
    ++s.pouch[static_cast<int>(f)];
    return true;
}

bool useFood(SaveData& s, Food f) {
    if (f >= Food::Count || s.pouch[static_cast<int>(f)] == 0) return false;
    --s.pouch[static_cast<int>(f)];
    return true;
}

bool sellTrinket(SaveData& s, Trinket t) {
    if (t >= Trinket::Count || s.hoard[static_cast<int>(t)] == 0) return false;
    --s.hoard[static_cast<int>(t)];
    s.gleam += trinketValue(t);
    return true;
}

DailyEgg dailyEgg(const SaveData& s, s32 day) {
    Rng rng(static_cast<std::uint64_t>(static_cast<u32>(day)) * 0x9E3779B97F4A7C15ull +
            (s.dragonCount ? s.dragons[0].id : 1));
    DailyEgg e;
    const Element el = static_cast<Element>(rng.chance(2, 3) ? 3 + rng.below(3) : rng.below(3));
    e.genome = makePurebred(el, rng);
    e.sex = rollSex(rng);
    e.price = kDailyEggPrice;
    return e;
}

int buyDailyEgg(SaveData& s, s64 now) {
    const s32 today = dayIndex(now);
    if (s.eggBoughtDay == today || s.gleam < kDailyEggPrice || s.dragonCount >= static_cast<int>(kMaxDragons)) return -1;
    const DailyEgg e = dailyEgg(s, today);
    Dragon egg = makeEgg(s.nextId++, e.genome, e.sex, now);
    egg.origin = Origin::Market;
    if (!placeEgg(s, egg) && vaultCount(s) >= kVaultEggs) {
        --s.nextId;
        return -1;
    }
    s.gleam -= e.price;
    s.eggBoughtDay = today;
    s.dragons[s.dragonCount] = egg;
    return s.dragonCount++;
}

}  // namespace ec
