#include "core/accessories.hpp"

#include <cmath>

#include "core/rng.hpp"
#include "core/save.hpp"
#include "core/trainer.hpp"

namespace ec {
namespace {

constexpr Rgb kGold{230, 184, 82}, kSilver{214, 222, 236}, kBrass{186, 140, 70}, kWhite{250, 246, 236};
constexpr Rgb kLeaf{96, 152, 80}, kRuby{236, 64, 70}, kIce{150, 220, 255}, kMoon{235, 240, 255};

using S = WearSlot;
using H = WearShape;
using W = WearSource;

// The accessories (D90): eight a slot, most at the glade's stall, the finest won at the shows,
// a few only from Frostspire Hollow or the valley's finds. Keep the order: the save keeps them
// by index (Progress::accessories, Dragon::wear).
constexpr Accessory kAccessories[kAccessoryCount] = {
    // head
    {"Straw sunhat", S::Head, H::SunHat, 110, {{236, 206, 140}, {214, 86, 70}}, kWhite, {255, 220, 120}, kStyleCute | kStyleFloral, W::Stall},
    {"Little top hat", S::Head, H::TopHat, 180, {{62, 50, 72}, {150, 60, 90}}, kGold, {255, 220, 150}, kStyleElegant, W::Stall},
    {"Flower crown", S::Head, H::FlowerCrown, 140, {{110, 170, 90}, {245, 160, 190}}, {255, 236, 150}, {255, 236, 150}, kStyleFloral | kStyleCute, W::Stall},
    {"Frost tiara", S::Head, H::Tiara, 300, {{214, 228, 246}, {170, 210, 250}}, kSilver, kIce, kStyleFrosty | kStyleElegant, W::Prize},
    {"Ember crown", S::Head, H::Crown, 320, {{232, 180, 70}, {200, 60, 50}}, kGold, kRuby, kStyleFiery | kStyleFestive, W::Prize},
    {"Star circlet", S::Head, H::Circlet, 220, {{200, 208, 230}, {80, 90, 150}}, kSilver, {255, 230, 140}, kStyleStarry | kStyleElegant, W::Stall},
    {"Party hat", S::Head, H::PartyHat, 80, {{240, 110, 90}, {250, 220, 110}}, kWhite, {255, 250, 230}, kStyleFestive | kStyleCute, W::Stall},
    {"Feather crest", S::Head, H::FeatherCrest, 260, {{210, 90, 60}, {60, 150, 150}}, kBrass, {250, 200, 90}, kStyleWild | kStyleFestive, W::Hollow},
    // neck
    {"Ribbon bow", S::Neck, H::NeckBow, 60, {{214, 60, 70}, {250, 200, 200}}, kGold, {255, 220, 150}, kStyleCute, W::Stall},
    {"Cosy scarf", S::Neck, H::Scarf, 110, {{90, 150, 200}, {245, 240, 230}}, kWhite, {255, 250, 240}, kStyleCute | kStyleFrosty, W::Stall},
    {"Leather collar", S::Neck, H::Collar, 80, {{130, 84, 52}, {96, 60, 40}}, kBrass, {230, 190, 110}, kStyleWild, W::Stall},
    {"Silver bell", S::Neck, H::Bell, 100, {{60, 110, 180}, {40, 80, 140}}, kSilver, {255, 250, 220}, kStyleCute | kStyleFestive, W::Stall},
    {"Moon pendant", S::Neck, H::Pendant, 280, {{200, 206, 226}, {120, 110, 170}}, kSilver, kMoon, kStyleStarry | kStyleElegant, W::Prize},
    {"Frilled ruff", S::Neck, H::Ruff, 160, {{250, 246, 240}, {236, 170, 190}}, kWhite, {255, 240, 240}, kStyleElegant | kStyleFestive, W::Stall},
    {"Flower lei", S::Neck, H::Lei, 130, {{250, 150, 90}, {240, 110, 150}}, kLeaf, {255, 236, 120}, kStyleFloral | kStyleFestive, W::Find},
    {"Ember amulet", S::Neck, H::Pendant, 260, {{150, 60, 40}, {90, 40, 30}}, kGold, kRuby, kStyleFiery | kStyleWild, W::Hollow},
    // back
    {"Riding saddle", S::Back, H::Saddle, 240, {{140, 90, 56}, {180, 60, 56}}, kBrass, {240, 200, 120}, kStyleWild | kStyleElegant, W::Stall},
    {"Royal cape", S::Back, H::Cape, 340, {{110, 50, 130}, {250, 236, 210}}, kGold, {255, 220, 130}, kStyleElegant | kStyleFestive, W::Prize},
    {"Patchwork blanket", S::Back, H::Blanket, 140, {{230, 140, 110}, {120, 170, 150}}, {245, 220, 160}, {250, 240, 210}, kStyleCute, W::Stall},
    {"Parade sash", S::Back, H::Sash, 100, {{200, 50, 60}, {250, 220, 110}}, kGold, {255, 230, 140}, kStyleFestive, W::Stall},
    {"Starry cloak", S::Back, H::Cape, 320, {{40, 50, 100}, {80, 90, 160}}, kSilver, {255, 236, 150}, kStyleStarry | kStyleElegant, W::Prize},
    {"Frost mantle", S::Back, H::Blanket, 280, {{240, 246, 255}, {150, 200, 240}}, kSilver, kIce, kStyleFrosty | kStyleElegant, W::Hollow},
    {"Blossom garland", S::Back, H::Garland, 170, {{110, 170, 90}, {250, 180, 210}}, {255, 240, 170}, {255, 240, 170}, kStyleFloral | kStyleCute, W::Stall},
    {"Ember saddle", S::Back, H::Saddle, 300, {{170, 50, 40}, {240, 160, 60}}, kGold, kRuby, kStyleFiery | kStyleFestive, W::Prize},
    // tail
    {"Tail bow", S::Tail, H::TailBow, 50, {{246, 150, 190}, {255, 240, 246}}, kWhite, {255, 240, 246}, kStyleCute, W::Stall},
    {"Silk ribbons", S::Tail, H::TailRibbons, 90, {{120, 170, 230}, {250, 210, 120}}, kGold, {255, 240, 200}, kStyleElegant | kStyleFestive, W::Stall},
    {"Gold tail ring", S::Tail, H::TailRing, 120, {{232, 186, 80}, {200, 150, 60}}, kGold, {255, 230, 150}, kStyleElegant, W::Stall},
    {"Star charm", S::Tail, H::StarCharm, 240, {{210, 216, 236}, {255, 216, 110}}, kSilver, {255, 230, 140}, kStyleStarry | kStyleFestive, W::Prize},
    {"Tail bell", S::Tail, H::TailBell, 70, {{220, 70, 70}, {250, 214, 110}}, kGold, {255, 240, 200}, kStyleCute | kStyleFestive, W::Stall},
    {"Snowflake charm", S::Tail, H::SnowCharm, 240, {{214, 228, 246}, {170, 220, 255}}, kSilver, kIce, kStyleFrosty | kStyleStarry, W::Hollow},
    {"Flower wreath", S::Tail, H::TailWreath, 100, {{110, 170, 90}, {255, 210, 110}}, {250, 150, 180}, {255, 236, 150}, kStyleFloral, W::Find},
    {"Ember tassel", S::Tail, H::Tassel, 150, {{230, 90, 40}, {250, 190, 80}}, kGold, {255, 200, 100}, kStyleFiery | kStyleWild, W::Find},
};

// The dyes: natural (its own colours), eight at the dye stall, four won at the shows.
constexpr DyeInfo kDyes[kDyeCount] = {
    {"Natural", {0, 0, 0}, {0, 0, 0}, 0, W::Stall, 0},
    {"Rose", {214, 96, 120}, {250, 206, 214}, 90, W::Stall, 0.35f},
    {"Marigold", {236, 160, 60}, {252, 226, 170}, 90, W::Stall, 0.35f},
    {"Meadow", {110, 170, 80}, {214, 238, 180}, 90, W::Stall, 0.35f},
    {"Sky", {96, 160, 220}, {206, 230, 250}, 90, W::Stall, 0.35f},
    {"Lilac", {160, 120, 210}, {226, 212, 246}, 110, W::Stall, 0.35f},
    {"Coal", {60, 58, 64}, {140, 136, 144}, 110, W::Stall, 0.75f},
    {"Snow", {238, 242, 250}, {255, 255, 255}, 110, W::Stall, 0.8f},
    {"Coral", {240, 130, 110}, {255, 214, 196}, 110, W::Stall, 0.35f},
    {"Midnight", {40, 50, 110}, {120, 130, 200}, 0, W::Prize, 0.55f},
    {"Gilded", {220, 176, 70}, {255, 240, 180}, 0, W::Prize, 0.4f},
    {"Frostfire", {120, 210, 240}, {240, 250, 255}, 0, W::Prize, 0.45f},
    {"Ember red", {200, 50, 40}, {250, 180, 120}, 0, W::Prize, 0.4f},
};

u8 clampByte(float v) { return static_cast<u8>(v <= 0 ? 0 : (v >= 255 ? 255 : v + 0.5f)); }
float luma(Rgb c) { return 0.3f * c.r + 0.59f * c.g + 0.11f * c.b; }

// The dye's colour at another colour's lightness, then that far toward it.
Rgb tint(Rgb from, Rgb toward, float amount, float depth) {
    // Mostly its own lightness (a pale belly stays pale, a dark stripe dark), `depth` of the way
    // to the dye's, so a pale kind still takes a colour and coal darkens.
    const float lt = std::fmax(1.0f, luma(toward));
    const float lf = luma(from) + (lt - luma(from)) * depth;
    const float k = (lf + 8.0f) / lt;
    float r = toward.r * k, g = toward.g * k, b = toward.b * k;
    const float mx = std::fmax(r, std::fmax(g, b));
    if (mx > 255.0f) {  // too bright to scale up: as bright as it goes, then toward white (the hue kept)
        const float s = 255.0f / mx;
        r *= s, g *= s, b *= s;
        const float now = 0.3f * r + 0.59f * g + 0.11f * b;
        const float w = std::fmin(1.0f, std::fmax(0.0f, (lf - now) / std::fmax(1.0f, 255.0f - now)));
        r += (255.0f - r) * w, g += (255.0f - g) * w, b += (255.0f - b) * w;
    }
    return {clampByte(from.r + (r - from.r) * amount), clampByte(from.g + (g - from.g) * amount),
            clampByte(from.b + (b - from.b) * amount)};
}

// A colour's hue (0..360), saturation and value (0..1).
void hsv(Rgb c, float& h, float& s, float& v) {
    const float r = c.r / 255.0f, g = c.g / 255.0f, b = c.b / 255.0f;
    const float mx = std::fmax(r, std::fmax(g, b)), mn = std::fmin(r, std::fmin(g, b)), d = mx - mn;
    v = mx;
    s = mx > 0 ? d / mx : 0;
    if (d < 1e-5f) {
        h = 0;
        return;
    }
    if (mx == r) h = 60.0f * std::fmod((g - b) / d + 6.0f, 6.0f);
    else if (mx == g) h = 60.0f * ((b - r) / d + 2.0f);
    else h = 60.0f * ((r - g) / d + 4.0f);
}

}  // namespace

const char* styleName(int bit) {
    static const char* const kNames[kStyleTags] = {"cute", "elegant", "wild", "festive", "frosty", "fiery", "floral", "starry"};
    return bit >= 0 && bit < kStyleTags ? kNames[bit] : "";
}

int accessoryCount() { return kAccessoryCount; }
const Accessory& accessoryInfo(int a) { return kAccessories[a >= 0 && a < kAccessoryCount ? a : 0]; }

const char* slotName(WearSlot slot) {
    switch (slot) {
        case WearSlot::Head: return "Head";
        case WearSlot::Neck: return "Neck";
        case WearSlot::Back: return "Back";
        case WearSlot::Tail: return "Tail";
        default: return "";
    }
}

int dyeCount() { return kDyeCount; }
const DyeInfo& dyeInfo(int dye) { return kDyes[dye >= 0 && dye < kDyeCount ? dye : 0]; }

void applyDye(int dye, Rgb pal[kPalCount]) {
    if (dye <= 0 || dye >= kDyeCount) return;
    const DyeInfo& d = kDyes[dye];
    // Strongly on the body, softer on the pale belly and the pattern (they keep a little of
    // the kind's own), the wings' membrane between.
    pal[kPalBase] = tint(pal[kPalBase], d.main, 0.75f, d.depth);
    pal[kPalAccent] = tint(pal[kPalAccent], d.light, 0.6f, d.depth);
    pal[kPalPattern] = tint(pal[kPalPattern], d.main, 0.5f, d.depth);
    pal[kPalMembrane] = tint(pal[kPalMembrane], d.light, 0.55f, d.depth);
}

namespace acc {

int worn(const Dragon& d, WearSlot slot) {
    const int s = static_cast<int>(slot);
    if (s < 0 || s >= kWearSlots) return -1;
    const u8 a = d.wear[s];
    return a < kAccessoryCount && kAccessories[a].slot == slot ? a : -1;
}

int wornCount(const Dragon& d) {
    int n = 0;
    for (int s = 0; s < kWearSlots; ++s) n += worn(d, static_cast<WearSlot>(s)) >= 0;
    return n;
}

u8 wornStyles(const Dragon& d) {
    u8 styles = 0;
    for (int s = 0; s < kWearSlots; ++s)
        if (const int a = worn(d, static_cast<WearSlot>(s)); a >= 0) styles |= kAccessories[a].styles;
    return styles;
}

bool putOn(const SaveData& s, Dragon& d, int a) {
    if (a < 0 || a >= kAccessoryCount || !trainer::ownsAccessory(s, a)) return false;
    d.wear[static_cast<int>(kAccessories[a].slot)] = static_cast<u8>(a);
    return true;
}

void takeOff(Dragon& d, WearSlot slot) {
    const int s = static_cast<int>(slot);
    if (s >= 0 && s < kWearSlots) d.wear[s] = kNone;
}

bool dyeWith(const SaveData& s, Dragon& d, int dye) {
    if (dye < 0 || dye >= kDyeCount || !trainer::ownsDye(s, dye)) return false;
    d.dye = static_cast<u8>(dye);
    return true;
}

int ownedFor(const SaveData& s, WearSlot slot, int* out, int cap) {
    int n = 0;
    for (int a = 0; a < kAccessoryCount && n < cap; ++a)
        if (kAccessories[a].slot == slot && trainer::ownsAccessory(s, a)) out[n++] = a;
    return n;
}

void stallPicks(s32 day, int out[kStallShown]) {
    int pool[kAccessoryCount], n = 0;
    for (int a = 0; a < kAccessoryCount; ++a)
        if (kAccessories[a].source == WearSource::Stall) pool[n++] = a;
    // Every day's picks from the whole stock (not only what you lack), so buying one doesn't
    // shuffle the rest; one of each slot first, so the stall always has something for each.
    Rng rng(static_cast<std::uint64_t>(static_cast<u32>(day)) * 0x9E3779B97F4A7C15ull + 0xACCE55u);
    int k = 0;
    for (int slot = 0; slot < kWearSlots && k < kStallShown; ++slot) {
        int of[kAccessoryCount], m = 0;
        for (int i = 0; i < n; ++i)
            if (static_cast<int>(kAccessories[pool[i]].slot) == slot) of[m++] = i;
        if (m == 0) continue;
        const int pick = of[rng.below(static_cast<u32>(m))];
        out[k++] = pool[pick];
        pool[pick] = pool[--n];
    }
    while (k < kStallShown && n > 0) {
        const int pick = static_cast<int>(rng.below(static_cast<u32>(n)));
        out[k++] = pool[pick];
        pool[pick] = pool[--n];
    }
    for (; k < kStallShown; ++k) out[k] = -1;
    // Shown by slot, head to tail.
    for (int i = 1; i < kStallShown; ++i)
        for (int j = i; j > 0 && out[j] >= 0 && (out[j - 1] < 0 || out[j] < out[j - 1]); --j) {
            const int t = out[j];
            out[j] = out[j - 1];
            out[j - 1] = t;
        }
}

bool buyAccessory(SaveData& s, int a) {
    if (a < 0 || a >= kAccessoryCount || trainer::ownsAccessory(s, a) || s.gleam < kAccessories[a].price) return false;
    s.gleam -= kAccessories[a].price;
    trainer::giveAccessory(s, a);
    return true;
}

bool buyDye(SaveData& s, int dye) {
    if (dye <= 0 || dye >= kDyeCount || kDyes[dye].source != WearSource::Stall || trainer::ownsDye(s, dye) ||
        s.gleam < kDyes[dye].price)
        return false;
    s.gleam -= kDyes[dye].price;
    trainer::giveDye(s, dye);
    return true;
}

int unownedFrom(const SaveData& s, WearSource source, u32 seed) {
    int pool[kAccessoryCount], n = 0;
    for (int a = 0; a < kAccessoryCount; ++a)
        if (kAccessories[a].source == source && !trainer::ownsAccessory(s, a)) pool[n++] = a;
    if (n == 0) return -1;
    Rng rng(static_cast<std::uint64_t>(seed) * 0xD1B54A32D192ED03ull + 0x9121u);
    return pool[rng.below(static_cast<u32>(n))];
}

int prizeDye(const SaveData& s, u32 seed) {
    int pool[kDyeCount], n = 0;
    for (int d = 1; d < kDyeCount; ++d)
        if (kDyes[d].source == WearSource::Prize && !trainer::ownsDye(s, d)) pool[n++] = d;
    if (n == 0) return 0;
    Rng rng(static_cast<std::uint64_t>(seed) * 0x9E3779B97F4A7C15ull + 0xD7Eu);
    return pool[rng.below(static_cast<u32>(n))];
}

float colourMatch(Rgb a, Rgb b) {
    float ha, sa, va, hb, sb, vb;
    hsv(a, ha, sa, va);
    hsv(b, hb, sb, vb);
    if (sa < 0.15f || sb < 0.15f) return 0.8f;  // white, grey, black, gold-ish pale: goes with anything
    float d = std::fabs(ha - hb);
    if (d > 180) d = 360 - d;
    // Close hues match; a near-complement (150..210 apart) pleases too; the ones between clash.
    if (d < 40) return 1.0f - d / 200.0f;
    if (d > 150) return 0.75f + (d - 150) / 240.0f;
    return 0.8f - 0.5f * (d - 40) / 110.0f;
}

float suitsColours(const Dragon& d, const Rgb pal[kPalCount]) {
    float sum = 0;
    int n = 0;
    for (int s = 0; s < kWearSlots; ++s) {
        const int a = worn(d, static_cast<WearSlot>(s));
        if (a < 0) continue;
        const Accessory& x = kAccessories[a];
        sum += 0.6f * colourMatch(x.colour[0], pal[kPalBase]) + 0.4f * colourMatch(x.colour[0], pal[kPalAccent]);
        ++n;
    }
    return n ? sum / n : 0.5f;
}

}  // namespace acc
}  // namespace ec
