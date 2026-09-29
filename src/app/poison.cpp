// `make POISON=1`: the allocators wrapped so every new block comes filled with garbage, as the
// real 3DS leaves memory. The emulator hands out zeroed memory, so a read of memory never written
// (a field left unset, a GPU buffer drawn past what was filled) goes unseen there and misbehaves
// only on the hardware (run 20: a freeze at Continue that Azahar never showed).
#if EC_POISON

#include <3ds.h>

#include <cstddef>
#include <cstring>

extern "C" {
void* __real_malloc(std::size_t size);
void* __real_realloc(void* p, std::size_t size);
void* __real_memalign(std::size_t align, std::size_t size);
void* __real_linearAlloc(std::size_t size);
void* __real_linearMemAlign(std::size_t size, std::size_t align);

void* __wrap_malloc(std::size_t size) {
    void* p = __real_malloc(size);
    if (p) std::memset(p, 0xA5, size);
    return p;
}

void* __wrap_realloc(void* old, std::size_t size) {
    // (the grown part garbage too: the old block's size isn't known here, so only a fresh block)
    void* p = __real_realloc(old, size);
    if (p && !old) std::memset(p, 0xA5, size);
    return p;
}

void* __wrap_memalign(std::size_t align, std::size_t size) {
    void* p = __real_memalign(align, size);
    if (p) std::memset(p, 0xA5, size);
    return p;
}

void* __wrap_linearAlloc(std::size_t size) {
    void* p = __real_linearAlloc(size);
    if (p) {
        std::memset(p, 0xA5, size);
        GSPGPU_FlushDataCache(p, size);
    }
    return p;
}

void* __wrap_linearMemAlign(std::size_t size, std::size_t align) {
    void* p = __real_linearMemAlign(size, align);
    if (p) {
        std::memset(p, 0xA5, size);
        GSPGPU_FlushDataCache(p, size);
    }
    return p;
}
}

#endif
