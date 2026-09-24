#include "app/app.hpp"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#include "app/scenes.hpp"

namespace ec {
namespace {

constexpr s64 kNtpToUnix = 2208988800LL;  // osGetTime() counts from 1900-01-01
constexpr const char* kSaveDir = "sdmc:/3ds/emberclutch";
constexpr const char* kSavePath = "sdmc:/3ds/emberclutch/dev-save.bin";

}  // namespace

s64 nowLocal(const App& app) {
    return static_cast<s64>(osGetTime() / 1000) - kNtpToUnix + app.save.devOffset;
}

void showToast(App& app, const char* msg) {
    app.toast = msg;
    app.toastTime = 3.0f;
}

bool loadDevSave(DevSave& out) {
    FILE* f = std::fopen(kSavePath, "rb");
    if (!f) return false;
    DevSave tmp;
    const bool ok = std::fread(&tmp, sizeof(tmp), 1, f) == 1 && std::memcmp(tmp.magic, "EMBd", 4) == 0 &&
                    tmp.version == DevSave{}.version && tmp.dragonSize == sizeof(Dragon);
    std::fclose(f);
    if (ok) out = tmp;
    return ok;
}

void writeDevSave(const DevSave& s) {
    mkdir("sdmc:/3ds", 0777);
    mkdir(kSaveDir, 0777);
    FILE* f = std::fopen(kSavePath, "wb");
    if (!f) return;
    std::fwrite(&s, sizeof(s), 1, f);
    std::fclose(f);
}

const SceneFns& sceneFns(SceneId id) {
    switch (id) {
        case SceneId::PickStarter: return kStarterScene;
        case SceneId::Den: return kDenScene;
        default: return kTitleScene;
    }
}

}  // namespace ec
