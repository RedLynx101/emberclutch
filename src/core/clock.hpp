// Time helpers. All times are "local unix seconds": the 3DS RTC holds local time with
// no time zone, so we treat it as-is. Callers pass time in; core never reads a clock.
#pragma once

#include "core/types.hpp"

namespace ec {

constexpr s64 kHour = 3600;
constexpr s64 kDay = 24 * kHour;
constexpr s64 kMaxCatchUp = 14 * kDay;

// Elapsed seconds that may be simulated. A clock moved backwards counts as zero
// (never punished), and huge forward jumps are capped.
inline s64 safeElapsed(s64 last, s64 now, s64 cap = kMaxCatchUp) {
    if (now <= last) return 0;
    const s64 d = now - last;
    return d > cap ? cap : d;
}

inline s32 dayIndex(s64 localUnix) {
    return static_cast<s32>(localUnix >= 0 ? localUnix / kDay : (localUnix - kDay + 1) / kDay);
}

inline int hourOfDay(s64 localUnix) { return static_cast<int>((localUnix - dayIndex(localUnix) * kDay) / kHour); }

// Dragons sleep 22:00–07:00 local.
inline bool isNight(s64 localUnix) {
    const int h = hourOfDay(localUnix);
    return h >= 22 || h < 7;
}

}  // namespace ec
