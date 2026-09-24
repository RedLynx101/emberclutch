// Emberclutch core — shared enums and small helpers.
// Everything under src/core is pure C++17: no libctru, no clock, no files.
#pragma once

#include <cstdint>

namespace ec {

using u8 = std::uint8_t;
using s8 = std::int8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

enum class Element : u8 { Ember, Tide, Gale, Grove, Frost, Lumen, Count };
constexpr int kElementCount = static_cast<int>(Element::Count);

enum class Stage : u8 { Egg, Hatchling, Juvenile, Adolescent, Adult };

enum class Mood : u8 { Upset, Sulky, Restless, Content, Joyful };

enum class Personality : u8 { Brave, Shy, Playful, Proud, Sleepy, Curious, Count };

// Breeding needs one of each (decision D19).
enum class Sex : u8 { Female, Male };

// Species-level layout. Dragons are Draconic + (Wings | Breath). The equine line
// (horse, pegasus, unicorn, alicorn) reuses the same records — see docs/future.
enum class BodyPlan : u8 { Draconic, Equine };
enum Module : u8 { kModWings = 1, kModHorn = 2, kModFins = 4, kModBreath = 8 };

struct Rgb {
    u8 r, g, b;
};

// h, s, v are full-range bytes (h: 0..255 maps to 0..360 degrees).
Rgb hsvToRgb(u8 h, u8 s, u8 v);

const char* elementName(Element e);
const char* stageName(Stage s);
const char* moodName(Mood m);
const char* personalityName(Personality p);
const char* sexName(Sex s);

}  // namespace ec
