#include "app/prefetch.hpp"

#include <3ds.h>

#include <cstdio>
#include <cstring>

namespace ec::prefetch {
namespace {

constexpr int kSlots = 24;

enum class State : u8 { Free, Queued, Reading, Done, Failed };

struct Slot {
    State state = State::Free;
    char path[80] = {};
    std::vector<u8> bytes;
};

Slot g_slots[kSlots];
LightLock g_lock;
LightEvent g_wake;
Thread g_thread = nullptr;
volatile bool g_quit = false;
bool g_started = false;

bool readAll(const char* path, std::vector<u8>& out) {
    FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    out.resize(static_cast<std::size_t>(std::ftell(f)));
    std::fseek(f, 0, SEEK_SET);
    const bool ok = std::fread(out.data(), 1, out.size(), f) == out.size();
    std::fclose(f);
    return ok;
}

void worker(void*) {
    while (!g_quit) {
        LightEvent_Wait(&g_wake);
        for (;;) {
            if (g_quit) return;
            Slot* next = nullptr;
            LightLock_Lock(&g_lock);
            for (Slot& s : g_slots)
                if (s.state == State::Queued) {
                    next = &s;
                    s.state = State::Reading;
                    break;
                }
            LightLock_Unlock(&g_lock);
            if (!next) break;
            std::vector<u8> bytes;  // read outside the lock
            const bool ok = readAll(next->path, bytes);
            LightLock_Lock(&g_lock);
            next->bytes.swap(bytes);
            next->state = ok ? State::Done : State::Failed;
            LightLock_Unlock(&g_lock);
        }
    }
}

void start() {
    if (g_started) return;
    LightLock_Init(&g_lock);
    LightEvent_Init(&g_wake, RESET_ONESHOT);
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    g_quit = false;
    g_thread = threadCreate(worker, nullptr, 16 * 1024, prio + 1, -2, false);  // below the game's
    g_started = g_thread != nullptr;
}

Slot* find(const char* path) {
    for (Slot& s : g_slots)
        if (s.state != State::Free && std::strcmp(s.path, path) == 0) return &s;
    return nullptr;
}

}  // namespace

void want(const char* path) {
    start();
    if (!g_started || std::strlen(path) >= sizeof(Slot::path)) return;
    LightLock_Lock(&g_lock);
    bool queued = false;
    if (!find(path))
        for (Slot& s : g_slots)
            if (s.state == State::Free) {
                std::snprintf(s.path, sizeof(s.path), "%s", path);
                s.state = State::Queued;
                queued = true;
                break;
            }
    LightLock_Unlock(&g_lock);
    if (queued) LightEvent_Signal(&g_wake);
}

bool ready(const char* path) {
    if (!g_started) return false;
    LightLock_Lock(&g_lock);
    const Slot* s = find(path);
    const bool r = s && (s->state == State::Done || s->state == State::Failed);
    LightLock_Unlock(&g_lock);
    return r;
}

bool take(const char* path, std::vector<u8>& out, bool& found) {
    found = false;
    if (!g_started) return false;
    for (;;) {
        LightLock_Lock(&g_lock);
        Slot* s = find(path);
        if (!s) {
            LightLock_Unlock(&g_lock);
            return false;
        }
        found = true;
        if (s->state == State::Done || s->state == State::Failed) {
            const bool ok = s->state == State::Done;
            out.swap(s->bytes);
            std::vector<u8>().swap(s->bytes);
            s->state = State::Free;
            s->path[0] = 0;
            LightLock_Unlock(&g_lock);
            return ok;
        }
        LightLock_Unlock(&g_lock);
        svcSleepThread(1000 * 1000);  // being read: wait for it rather than read it twice
    }
}

void shutdown() {
    if (!g_started) return;
    g_quit = true;
    LightEvent_Signal(&g_wake);
    threadJoin(g_thread, U64_MAX);
    threadFree(g_thread);
    g_thread = nullptr;
    g_started = false;
}

}  // namespace ec::prefetch
