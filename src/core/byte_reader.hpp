// Bounds-checked little-endian reader for asset files (models, animations). Assets come
// from romfs, but files are never trusted: any overrun turns ok() false and reads zeros.
#pragma once

#include <cstddef>
#include <cstring>

#include "core/math3d.hpp"
#include "core/types.hpp"

namespace ec {

class ByteReader {
public:
    ByteReader(const u8* p, std::size_t n) : p_(p), n_(n) {}
    bool ok() const { return ok_; }
    void bytes(void* out, std::size_t len) {
        if (!ok_ || at_ + len > n_) {
            ok_ = false;
            std::memset(out, 0, len);
            return;
        }
        std::memcpy(out, p_ + at_, len);
        at_ += len;
    }
    u8 u8v() { u8 v; bytes(&v, 1); return v; }
    s8 s8v() { return static_cast<s8>(u8v()); }
    u16 u16v() { u8 b[2]; bytes(b, 2); return static_cast<u16>(b[0] | (b[1] << 8)); }
    s16 s16v() { return static_cast<s16>(u16v()); }
    u32 u32v() { u8 b[4]; bytes(b, 4); return u32(b[0]) | (u32(b[1]) << 8) | (u32(b[2]) << 16) | (u32(b[3]) << 24); }
    float f32() { const u32 bits = u32v(); float f; std::memcpy(&f, &bits, 4); return f; }
    Vec3 vec3() { const float x = f32(), y = f32(); return {x, y, f32()}; }
    void skip(std::size_t len) { if (at_ + len > n_) ok_ = false; else at_ += len; }

private:
    const u8* p_;
    std::size_t n_, at_ = 0;
    bool ok_ = true;
};

}  // namespace ec
