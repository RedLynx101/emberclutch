#include "app/valley_ext.hpp"

#include "app/glade.hpp"  // the pageant

namespace ec::vext {
namespace {

// Each 1.0 feature adds one line here (its functions live in its own files).
const Feature kFeatures[] = {
    {"none", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},  // (keeps the table non-empty)
    glade::kFeature,  // the pageant at Moonpetal Glade (app/glade.cpp)
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
