#include "core/names.hpp"

#include <cstdio>
#include <cstring>

namespace ec {
namespace {

constexpr int kPerList = 12;
// One list per element, in Element order.
constexpr const char* kElementNames[kElementCount][kPerList] = {
    {"Cinder", "Pyra", "Ashby", "Sparky", "Blaze", "Kindle", "Flicker", "Emberly", "Sol", "Tinder", "Scorch", "Soot"},
    {"Marlo", "Ripple", "Nerissa", "Kelpie", "Brine", "Coral", "Wade", "Misty", "Splash", "Pearl", "Delta", "Tully"},
    {"Zephyr", "Gusty", "Skye", "Whisk", "Aria", "Cirrus", "Breeze", "Flurry", "Swoop", "Nimbus", "Wren", "Pip"},
    {"Fern", "Bramble", "Sprout", "Clover", "Willow", "Thistle", "Acorn", "Juniper", "Sage", "Ivy", "Rowan", "Moss"},
    {"Sleet", "Crystal", "Flake", "Rime", "Glacia", "Aspen", "Frosty", "Blizz", "Yuki", "Icicle", "Hail", "Pebble"},
    {"Glimmer", "Lux", "Nova", "Halo", "Dawn", "Twinkle", "Opal", "Star", "Beam", "Aurora", "Lumi", "Sunny"},
};
constexpr const char* kCosyNames[kPerList] = {"Bean",   "Mochi",  "Biscuit", "Noodle", "Pepper", "Toffee",
                                              "Dumpling", "Pudding", "Waffle", "Nutmeg", "Button", "Pickle"};

u32 mix(u32 x) {
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    x ^= x >> 16;
    return x;
}

}  // namespace

void suggestName(const Dragon& d, u32 roll, char* out, std::size_t cap) {
    if (!out || cap == 0) return;
    const u32 h = mix(d.id * 2654435761u + roll * 0x9E3779B9u + 0xA11CEu);
    const int element = d.genome.elementA < kElementCount ? d.genome.elementA : 0;
    const char* name = h % 10 < 7 ? kElementNames[element][(h >> 8) % kPerList] : kCosyNames[(h >> 8) % kPerList];
    std::snprintf(out, cap, "%s", name);
}

bool setName(char* out, std::size_t cap, const char* typed) {
    if (!out || cap < 2 || !typed) return false;
    while (*typed == ' ') ++typed;
    std::size_t len = std::strlen(typed);
    while (len > 0 && typed[len - 1] == ' ') --len;
    if (len > cap - 1) {
        len = cap - 1;
        while (len > 0 && (static_cast<unsigned char>(typed[len]) & 0xC0) == 0x80) --len;  // not mid-character
    }
    if (len == 0) return false;
    std::memcpy(out, typed, len);
    out[len] = '\0';
    return true;
}

}  // namespace ec
