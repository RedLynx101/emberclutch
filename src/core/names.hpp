// Name suggestions for a new hatchling (D27): the keyboard opens with one filled in, and
// "Another" rolls the next. Mostly names that suit its element, sometimes a cosy one.
#pragma once

#include <cstddef>

#include "core/dragon.hpp"

namespace ec {

constexpr std::size_t kNameMax = sizeof(Dragon{}.name);  // bytes, with the terminator

// Writes a suggestion for `d` into out (cap bytes). Different `roll`s give different names.
void suggestName(const Dragon& d, u32 roll, char* out, std::size_t cap);

// Copies a typed name (UTF-8) into a dragon's name field: trimmed of spaces, cut at a whole
// character so it fits. False (and `out` untouched) if nothing is left.
bool setName(char* out, std::size_t cap, const char* typed);

}  // namespace ec
