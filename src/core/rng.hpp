// Small, fast, seedable PRNG (xorshift64*). Deterministic across 3DS and PC so
// breeding bugs can be reproduced from a seed.
#pragma once

#include <cstdint>

namespace ec {

class Rng {
public:
    explicit Rng(std::uint64_t seed) : state_(seed ? seed : 0x9E3779B97F4A7C15ull) {}

    std::uint32_t next() {
        state_ ^= state_ >> 12;
        state_ ^= state_ << 25;
        state_ ^= state_ >> 27;
        return static_cast<std::uint32_t>((state_ * 0x2545F4914F6CDD1Dull) >> 32);
    }

    // Uniform in [0, n).
    std::uint32_t below(std::uint32_t n) {
        return static_cast<std::uint32_t>((static_cast<std::uint64_t>(next()) * n) >> 32);
    }

    // Uniform in [lo, hi] inclusive.
    int range(int lo, int hi) { return lo + static_cast<int>(below(static_cast<std::uint32_t>(hi - lo + 1))); }

    // True with probability num/den.
    bool chance(std::uint32_t num, std::uint32_t den) { return below(den) < num; }

    std::uint64_t state() const { return state_; }

private:
    std::uint64_t state_;
};

}  // namespace ec
