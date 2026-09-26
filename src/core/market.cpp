#include "core/market.hpp"

#include "core/clock.hpp"
#include "core/den_roster.hpp"
#include "core/dragon.hpp"
#include "core/genetics.hpp"
#include "core/kinds.hpp"

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
    e.look = rollLook(rng);
    e.kind = static_cast<u8>(randomKind(rng, 3, 5, 2));
    e.variant = static_cast<u8>(rollVariant(rng));
    static constexpr u32 kPrice[3] = {150, kDailyEggPrice, 400};  // common, harder to find, rare
    e.price = kPrice[static_cast<int>(kindInfo(e.kind).rarity)];
    return e;
}

int buyDailyEgg(SaveData& s, s64 now) {
    const s32 today = dayIndex(now);
    const DailyEgg e = dailyEgg(s, today);
    if (s.eggBoughtDay == today || s.gleam < e.price || s.dragonCount >= static_cast<int>(kMaxDragons)) return -1;
    Dragon egg = makeEgg(s.nextId++, e.genome, e.sex, now, e.look);
    Rng rng(static_cast<std::uint64_t>(egg.id) * 0x9E3779B97F4A7C15ull + 0x3A7u);
    rollKind(egg, e.kind, e.variant, rng);
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

void stallToday(SaveData& s, s32 day, Item out[kStallSpots]) {
    WorldState& w = s.world;
    if (w.stallDay != day) {  // a new day: four things you don't have, at random
        Rng rng(static_cast<std::uint64_t>(static_cast<u32>(day)) * 0xD1B54A32D192ED03ull +
                (s.dragonCount ? s.dragons[0].id : 7) + 0x57A11u);
        Item pool[kItems];
        int n = 0;
        for (int i = 0; i < kItems; ++i)
            if (!owns(s, static_cast<Item>(i))) pool[n++] = static_cast<Item>(i);
        for (int k = 0; k < kStallSpots; ++k) {
            if (n == 0) {
                w.stall[k] = 0xFF;
                continue;
            }
            const int pick = static_cast<int>(rng.below(static_cast<u32>(n)));
            w.stall[k] = static_cast<u8>(pool[pick]);
            pool[pick] = pool[--n];
        }
        w.stallDay = day;
    }
    for (int k = 0; k < kStallSpots; ++k) {
        const u8 v = w.stall[k];
        out[k] = v < kItems && !owns(s, static_cast<Item>(v)) ? static_cast<Item>(v) : Item::Count;
    }
}

Dragon eggOnShow(const SaveData& s, s32 day) {
    const DailyEgg e = dailyEgg(s, day);
    Dragon d = makeEgg(0xFFFFFFF0u, e.genome, e.sex, 0);
    d.kind = e.kind;  // today's kind, in its colouring (the egg shows it)
    d.variant = e.variant;
    d.warmth = 80;
    return d;
}

bool buyFromStall(SaveData& s, s32 day, int spot) {
    Item today[kStallSpots];
    stallToday(s, day, today);
    if (spot < 0 || spot >= kStallSpots || today[spot] == Item::Count) return false;
    if (!buyItem(s, today[spot])) return false;
    s.world.stall[spot] = 0xFF;  // sold: the spot stays empty till tomorrow
    return true;
}

}  // namespace ec
