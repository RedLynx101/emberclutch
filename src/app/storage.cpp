#include "app/storage.hpp"

#include <3ds.h>

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

// The save thread: the next save waiting (the newest wins), and the one being written.
u8 g_queued[32 * 1024];
u8 g_writing[32 * 1024];
std::size_t g_queuedLen = 0;
int g_queuedSlot = 0;
bool g_hasQueued = false;
bool g_busy = false;
volatile bool g_failed = false;
volatile bool g_quit = false;
bool g_started = false;
Thread g_thread = nullptr;
LightEvent g_wake;
LightLock g_lock;

bool writeSlot(int target, const u8* buf, std::size_t n) {
    mkdir("sdmc:/3ds", 0777);
    mkdir(kDir, 0777);
    FILE* f = std::fopen(kSlotPath[target], "wb");
    if (!f) return false;
    const bool ok = std::fwrite(buf, 1, n, f) == n;
    const bool closed = std::fclose(f) == 0;
    return ok && closed;
}

void saveThread(void*) {
    while (!g_quit) {
        LightEvent_Wait(&g_wake);
        for (;;) {
            LightLock_Lock(&g_lock);
            if (!g_hasQueued) {
                g_busy = false;
                LightLock_Unlock(&g_lock);
                break;
            }
            const std::size_t n = g_queuedLen;
            const int slot = g_queuedSlot;
            std::memcpy(g_writing, g_queued, n);
            g_hasQueued = false;
            g_busy = true;
            LightLock_Unlock(&g_lock);
            if (!writeSlot(slot, g_writing, n)) g_failed = true;
        }
    }
}

void startThread() {
    if (g_started) return;
    LightEvent_Init(&g_wake, RESET_ONESHOT);
    LightLock_Init(&g_lock);
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    g_quit = false;
    g_thread = threadCreate(saveThread, nullptr, 32 * 1024, prio + 1, -2, false);  // below the game's
    g_started = g_thread != nullptr;
}

void waitIdle() {
    if (!g_started) return;
    for (;;) {
        LightLock_Lock(&g_lock);
        const bool idle = !g_hasQueued && !g_busy;
        LightLock_Unlock(&g_lock);
        if (idle) return;
        svcSleepThread(2 * 1000 * 1000);
    }
}

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
    waitIdle();
    const std::size_t n = encodeSave(data, slots.seq + 1, savedAt, g_bufA, sizeof(g_bufA));
    if (n == 0) return false;
    const int target = slots.slot < 0 ? 0 : slots.slot ^ 1;
    if (!writeSlot(target, g_bufA, n)) return false;
    slots.seq += 1;
    slots.slot = target;
    return true;
}

bool saveGameAsync(const SaveData& data, SaveSlots& slots, s64 savedAt) {
    startThread();
    if (!g_started) return saveGame(data, slots, savedAt);
    const std::size_t n = encodeSave(data, slots.seq + 1, savedAt, g_bufA, sizeof(g_bufA));
    if (n == 0) return false;
    const int target = slots.slot < 0 ? 0 : slots.slot ^ 1;
    LightLock_Lock(&g_lock);
    std::memcpy(g_queued, g_bufA, n);
    g_queuedLen = n;
    g_queuedSlot = target;
    g_hasQueued = true;
    LightLock_Unlock(&g_lock);
    LightEvent_Signal(&g_wake);
    slots.seq += 1;  // (the loader takes the newest good slot, should this write fail)
    slots.slot = target;
    return true;
}

bool saveWriteFailed() {
    const bool failed = g_failed;
    g_failed = false;
    return failed;
}

void finishSaves() {
    if (!g_started) return;
    waitIdle();
    g_quit = true;
    LightEvent_Signal(&g_wake);
    threadJoin(g_thread, U64_MAX);
    threadFree(g_thread);
    g_thread = nullptr;
    g_started = false;
}

void deleteGame() {
    waitIdle();
    std::remove(kSlotPath[0]);
    std::remove(kSlotPath[1]);
    std::remove(kLegacyPath);
    for (int cup = 1; cup <= 4; ++cup) {  // Sky Rings' ghosts of your best runs (app/challenge_rings)
        char ghost[64];
        std::snprintf(ghost, sizeof(ghost), "sdmc:/3ds/emberclutch/ghost-rings-%d.bin", cup);
        std::remove(ghost);
    }
}

}  // namespace ec
