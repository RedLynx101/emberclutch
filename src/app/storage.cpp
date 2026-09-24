#include "app/storage.hpp"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace ec {
namespace {

constexpr const char* kDir = "sdmc:/3ds/emberclutch";
constexpr const char* kSlotPath[2] = {"sdmc:/3ds/emberclutch/save.a", "sdmc:/3ds/emberclutch/save.b"};
constexpr const char* kLegacyPath = "sdmc:/3ds/emberclutch/dev-save.bin";

// Encode/decode buffers are static: the 3DS main thread stack is small.
u8 g_bufA[32 * 1024];
u8 g_bufB[32 * 1024];

std::size_t readFile(const char* path, u8* buf, std::size_t cap) {
    FILE* f = std::fopen(path, "rb");
    if (!f) return 0;
    const std::size_t n = std::fread(buf, 1, cap, f);
    std::fclose(f);
    return n;
}

// The pre-WP8 dev save (a raw struct dump) — imported once so dev dragons carry over.
struct LegacyDevSave {
    char magic[4];
    u32 version;
    u32 dragonSize;
    s64 lastSim;
    s64 devOffset;
    u8 hasDragon;
    Dragon dragon;
};

bool importLegacy(SaveData& out) {
    static LegacyDevSave legacy;
    FILE* f = std::fopen(kLegacyPath, "rb");
    if (!f) return false;
    const bool ok = std::fread(&legacy, sizeof(legacy), 1, f) == 1 && std::memcmp(legacy.magic, "EMBd", 4) == 0 &&
                    legacy.version == 2 && legacy.dragonSize == sizeof(Dragon) && legacy.hasDragon;
    std::fclose(f);
    if (!ok) return false;
    out = SaveData{};
    out.lastSim = legacy.lastSim;
    out.devOffset = legacy.devOffset;
    out.dragons[0] = legacy.dragon;
    out.dragonCount = 1;
    out.nextId = legacy.dragon.id + 1;
    return true;
}

}  // namespace

bool loadGame(SaveData& out, SaveSlots& slots) {
    static_assert(sizeof(g_bufA) >= 27 * 1024, "save buffer must hold a full save");
    const std::size_t na = readFile(kSlotPath[0], g_bufA, sizeof(g_bufA));
    const std::size_t nb = readFile(kSlotPath[1], g_bufB, sizeof(g_bufB));
    const int newest = pickNewestSlot(na ? g_bufA : nullptr, na, nb ? g_bufB : nullptr, nb);
    for (int attempt = 0; attempt < 2 && newest >= 0; ++attempt) {
        const int s = attempt == 0 ? newest : newest ^ 1;
        SaveHeaderInfo info;
        const u8* buf = s == 0 ? g_bufA : g_bufB;
        const std::size_t n = s == 0 ? na : nb;
        if (decodeSave(n ? buf : nullptr, n, out, &info) == LoadResult::Ok) {
            slots.seq = info.seq;
            slots.slot = s;
            return true;
        }
    }
    if (na == 0 && nb == 0 && importLegacy(out)) {
        slots = SaveSlots{};
        return true;
    }
    return false;
}

bool saveGame(const SaveData& data, SaveSlots& slots, s64 savedAt) {
    const std::size_t n = encodeSave(data, slots.seq + 1, savedAt, g_bufA, sizeof(g_bufA));
    if (n == 0) return false;
    mkdir("sdmc:/3ds", 0777);
    mkdir(kDir, 0777);
    const int target = slots.slot < 0 ? 0 : slots.slot ^ 1;
    FILE* f = std::fopen(kSlotPath[target], "wb");
    if (!f) return false;
    const bool ok = std::fwrite(g_bufA, 1, n, f) == n;
    const bool closed = std::fclose(f) == 0;
    if (!ok || !closed) return false;
    slots.seq += 1;
    slots.slot = target;
    return true;
}

void deleteGame() {
    std::remove(kSlotPath[0]);
    std::remove(kSlotPath[1]);
    std::remove(kLegacyPath);
}

}  // namespace ec
