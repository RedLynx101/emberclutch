#include "app/valley_ext.hpp"

#include "app/cove.hpp"  // Driftwood Cove (workstream C)
#include "app/glade.hpp"  // the pageant
#include "app/battle_feature.hpp"  // 1.0 battles (workstream B)
#include "app/wildlife.hpp"  // the valley's critters (workstream L)
#include "app/roamers_feature.hpp"  // roaming trainers and duels (workstream D)

namespace ec::vext {
namespace {

// Each 1.0 feature adds one line here (its functions live in its own files).
const Feature kFeatures[] = {
    {"none", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},  // (keeps the table non-empty)
    {"cove", cove::folk, cove::act, cove::active, cove::update, nullptr, cove::drawTop, cove::drawBottom, nullptr, nullptr, cove::drawOver},  // Driftwood Cove (workstream C)
    glade::kFeature,  // the pageant at Moonpetal Glade (app/glade.cpp)
    kLeagueFeature,  // 1.0 battles: the league's challengers, champions and boards (workstream B)
    kHollowFeature,  // 1.0 battles: Frostspire Hollow's keeper and floors (workstream B)
    wildlife::kFeature,  // the valley's critters: your partner's part in a moment (workstream L)
    kRoamerFeature,  // roaming trainers walking the paths, and their friendly duels (workstream D)
};

}  // namespace

int featureCount() { return static_cast<int>(sizeof(kFeatures) / sizeof(kFeatures[0])); }
const Feature& feature(int i) { return kFeatures[i >= 0 && i < featureCount() ? i : 0]; }

int activeFeature(const App& app) {
    for (int i = 0; i < featureCount(); ++i)
        if (kFeatures[i].active && kFeatures[i].active(app)) return i;
    return -1;
}

}  // namespace ec::vext
