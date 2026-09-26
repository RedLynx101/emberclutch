#include "core/save.hpp"

#include <cstring>

#include "core/kinds.hpp"

namespace ec {
namespace {

constexpr u8 kMagic[4] = {'E', 'M', 'B', 'C'};
// Each section and each dragon record is prefixed with its byte size, so a newer build
// can append fields (older records get defaults) and skip fields it doesn't know.
constexpr std::size_t kDragonRecordV1 = 132;

class Writer {
public:
    Writer(u8* out, std::size_t cap) : out_(out), cap_(cap) {}
    void u8v(u8 v) { put(&v, 1); }
    void u16v(u16 v) { u8 b[2] = {u8(v), u8(v >> 8)}; put(b, 2); }
    void u32v(u32 v) { u8 b[4] = {u8(v), u8(v >> 8), u8(v >> 16), u8(v >> 24)}; put(b, 4); }
    void s32v(s32 v) { u32v(static_cast<u32>(v)); }
    void s64v(s64 v) { u32v(static_cast<u32>(static_cast<std::uint64_t>(v))); u32v(static_cast<u32>(static_cast<std::uint64_t>(v) >> 32)); }
    void f32v(float v) { u32 bits; std::memcpy(&bits, &v, 4); u32v(bits); }
    void bytes(const void* p, std::size_t n) { put(p, n); }
    std::size_t pos() const { return pos_; }
    bool ok() const { return ok_; }
    // Patch a u16 written earlier (section sizes).
    void patchU16(std::size_t at, u16 v) { if (at + 2 <= cap_) { out_[at] = u8(v); out_[at + 1] = u8(v >> 8); } }

private:
    void put(const void* p, std::size_t n) {
        if (!ok_ || pos_ + n > cap_) { ok_ = false; return; }
        std::memcpy(out_ + pos_, p, n);
        pos_ += n;
    }
    u8* out_;
    std::size_t cap_, pos_ = 0;
    bool ok_ = true;
};

class Reader {
public:
    Reader(const u8* in, std::size_t size) : in_(in), size_(size) {}
    u8 u8v() { u8 v = 0; get(&v, 1); return v; }
    u16 u16v() { u8 b[2] = {}; get(b, 2); return u16(b[0] | (b[1] << 8)); }
    u32 u32v() { u8 b[4] = {}; get(b, 4); return u32(b[0]) | (u32(b[1]) << 8) | (u32(b[2]) << 16) | (u32(b[3]) << 24); }
    s32 s32v() { return static_cast<s32>(u32v()); }
    s64 s64v() { const std::uint64_t lo = u32v(), hi = u32v(); return static_cast<s64>(lo | (hi << 32)); }
    float f32v() { const u32 bits = u32v(); float v; std::memcpy(&v, &bits, 4); return v; }
    void bytes(void* p, std::size_t n) { get(p, n); }
    std::size_t pos() const { return pos_; }
    void seek(std::size_t p) { if (p > size_) ok_ = false; else pos_ = p; }
    bool ok() const { return ok_; }

private:
    void get(void* p, std::size_t n) {
        if (!ok_ || pos_ + n > size_) { ok_ = false; std::memset(p, 0, n); return; }
        std::memcpy(p, in_ + pos_, n);
        pos_ += n;
    }
    const u8* in_;
    std::size_t size_, pos_ = 0;
    bool ok_ = true;
};

void writeGenome(Writer& w, const Genome& g) {
    const u8 f[16] = {g.elementA, g.elementB, g.build,   g.horns,   g.frill,   g.wings,    g.tailTip, g.pattern,
                      g.baseH,    g.baseS,    g.baseV,   g.accentH, g.accentV, g.patternH, g.size,    g.rareFlags};
    w.bytes(f, 16);
}

Genome readGenome(Reader& r) {
    u8 f[16];
    r.bytes(f, 16);
    Genome g{};
    g.elementA = f[0]; g.elementB = f[1]; g.build = f[2]; g.horns = f[3]; g.frill = f[4]; g.wings = f[5];
    g.tailTip = f[6]; g.pattern = f[7]; g.baseH = f[8]; g.baseS = f[9]; g.baseV = f[10]; g.accentH = f[11];
    g.accentV = f[12]; g.patternH = f[13]; g.size = f[14]; g.rareFlags = f[15];
    return g;
}

void writeDragon(Writer& w, const Dragon& d) {
    const std::size_t sizeAt = w.pos();
    w.u16v(0);  // record size, patched below
    const std::size_t start = w.pos();
    w.u32v(d.id);
    w.u8v(static_cast<u8>(d.bodyPlan));
    w.u8v(d.modules);
    writeGenome(w, d.genome);
    w.u8v(static_cast<u8>(d.sex));
    w.u32v(d.motherId);
    w.u32v(d.fatherId);
    w.s64v(d.lastBredAt);
    w.u8v(static_cast<u8>(d.personality));
    w.u8v(d.favoriteFood);
    w.bytes(d.name, 16);
    w.u8v(static_cast<u8>(d.stage));
    w.u8v(static_cast<u8>(d.location));
    w.s64v(d.laidAt);
    w.s64v(d.hatchedAt);
    w.s64v(d.lastVisitAt);
    w.f32v(d.warmth);
    w.s32v(d.incubationSeconds);
    w.f32v(d.needs.belly);
    w.f32v(d.needs.energy);
    w.f32v(d.needs.clean);
    w.f32v(d.needs.play);
    w.u16v(d.bond);
    w.u16v(d.bondHigh);
    w.u16v(d.careStars);
    w.u8v(d.upset);
    w.u8v(d.napping);
    w.f32v(d.sulkyHours);
    w.s32v(d.day);
    w.f32v(d.dayLowestSum);
    w.f32v(d.dayHours);
    w.u8v(d.dayVisited);
    for (float dust : d.dirt) w.u16v(static_cast<u16>(dust * 100.0f + 0.5f));  // hundredths (added 2026-09-24)
    w.u8v(d.eggTurns);  // egg care (added 2026-09-24)
    w.s64v(d.lastTurnedAt);
    w.u8v(d.denSlot);  // Alpha 2: its bed or nest in the den
    w.s64v(d.wanderSince);  // Alpha 2: out on the Wanderings
    w.u32v(d.wanderSteps);
    w.u8v(static_cast<u8>(d.origin));  // Alpha 2: the profile (where its egg came from, what you know)
    w.u8v(d.known);
    w.u8v(d.look);  // Alpha 2 WP12: its look (D54)
    for (float m : d.mud) w.u8v(static_cast<u8>(m + 0.5f));  // Alpha 2 WP12: mud (D46)
    w.u8v(d.kind);  // DR3 (D80): the kind, its colouring, stats, manner and traits
    w.u8v(d.variant);
    for (u8 s : d.stats) w.u8v(s);
    w.u8v(d.manner);
    w.u8v(d.traitCount);
    for (u8 t : d.traits) w.u8v(t);
    w.patchU16(sizeAt, static_cast<u16>(w.pos() - start));
}

bool inRange(u8 v, u8 count) { return v < count; }

bool readDragon(Reader& r, Dragon& d) {
    const u16 size = r.u16v();
    const std::size_t start = r.pos();
    if (size < kDragonRecordV1) return false;
    d = Dragon{};
    d.id = r.u32v();
    const u8 plan = r.u8v();
    d.modules = r.u8v();
    d.genome = readGenome(r);
    const u8 sex = r.u8v();
    d.motherId = r.u32v();
    d.fatherId = r.u32v();
    d.lastBredAt = r.s64v();
    const u8 personality = r.u8v();
    d.favoriteFood = r.u8v();
    r.bytes(d.name, 16);
    d.name[15] = '\0';
    const u8 stage = r.u8v();
    const u8 location = r.u8v();
    d.laidAt = r.s64v();
    d.hatchedAt = r.s64v();
    d.lastVisitAt = r.s64v();
    d.warmth = r.f32v();
    d.incubationSeconds = r.s32v();
    d.needs.belly = r.f32v();
    d.needs.energy = r.f32v();
    d.needs.clean = r.f32v();
    d.needs.play = r.f32v();
    d.bond = r.u16v();
    d.bondHigh = r.u16v();
    d.careStars = r.u16v();
    d.upset = r.u8v() != 0;
    d.napping = r.u8v() != 0;
    d.sulkyHours = r.f32v();
    d.day = r.s32v();
    d.dayLowestSum = r.f32v();
    d.dayHours = r.f32v();
    d.dayVisited = r.u8v() != 0;
    if (r.pos() + 2 * kRegionCount <= start + size)  // older records have no dirt: clean
        for (float& dust : d.dirt) {
            const float v = r.u16v() / 100.0f;
            dust = v > 100.0f ? 100.0f : v;
        }
    if (r.pos() + 9 <= start + size) {  // older records: never turned
        d.eggTurns = r.u8v();
        if (d.eggTurns > kMaxEggTurns) d.eggTurns = kMaxEggTurns;
        d.lastTurnedAt = r.s64v();
    }
    if (r.pos() + 1 <= start + size) d.denSlot = r.u8v();  // older records: slot 0 (den_roster sorts it out)
    if (r.pos() + 12 <= start + size) {  // older records: at home
        d.wanderSince = r.s64v();
        d.wanderSteps = r.u32v();
    }
    if (r.pos() + 2 <= start + size) {  // older records: from parents if it has them, else the first egg
        const u8 origin = r.u8v();
        d.origin = origin < static_cast<u8>(Origin::Count) ? static_cast<Origin>(origin) : Origin::Starter;
        d.known = r.u8v();
    } else if (d.motherId != 0) {
        d.origin = Origin::Bred;
    }
    if (r.pos() + 1 <= start + size) {  // older records: a look by the base odds, fixed by its id (D66)
        const u8 look = r.u8v();
        d.look = look < kLookCount ? look : static_cast<u8>(kLookClassic);
    } else {
        d.look = lookForOldDragon(d.id);
    }
    if (r.pos() + kRegionCount <= start + size)  // older records: no mud
        for (float& m : d.mud) {
            const u8 v = r.u8v();
            m = v > 100 ? 100.0f : v;
        }
    constexpr std::size_t kKindBytes = 2 + kDragonStats + 2 + kDragonTraits;
    bool kinded = false;
    if (r.pos() + kKindBytes <= start + size) {  // DR3: its kind
        d.kind = r.u8v();
        d.variant = r.u8v();
        for (u8& s : d.stats) s = r.u8v();
        d.manner = r.u8v();
        d.traitCount = r.u8v();
        for (u8& t : d.traits) t = r.u8v();
        kinded = d.kind < kindCount() && d.variant < kKindVariants && d.manner < mannerCount() &&
                 d.traitCount <= kDragonTraits;
        for (int i = 0; i < d.traitCount && kinded; ++i) kinded = d.traits[i] < traitCount();
    }
    if (!kinded) migrateToKind(d);  // from before the revamp (D80): a new kind, fixed by its id
    r.seek(start + size);  // skip fields from newer builds

    if (!inRange(plan, 2) || !inRange(sex, 2) || !inRange(personality, static_cast<u8>(Personality::Count)) ||
        !inRange(stage, 5) || !inRange(location, 3) || !inRange(d.genome.elementA, kElementCount) ||
        !inRange(d.genome.elementB, kElementCount))
        return false;
    d.bodyPlan = static_cast<BodyPlan>(plan);
    d.sex = static_cast<Sex>(sex);
    d.personality = static_cast<Personality>(personality);
    d.stage = static_cast<Stage>(stage);
    d.location = static_cast<Location>(location);
    return r.ok();
}

}  // namespace

u32 crc32(const u8* data, std::size_t size) {
    static u32 table[256];
    static bool ready = false;
    if (!ready) {
        for (u32 i = 0; i < 256; ++i) {
            u32 c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        ready = true;
    }
    u32 crc = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < size; ++i) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

std::size_t maxEncodedSize() {
    // header + player/settings sections (generous) + dragons with room for growth
    return kSaveHeaderSize + 256 + kMaxDragons * (kDragonRecordV1 + 2 + 64);
}

std::size_t encodeSave(const SaveData& data, u32 seq, s64 savedAt, u8* out, std::size_t cap) {
    if (data.dragonCount > kMaxDragons || cap < kSaveHeaderSize) return 0;
    Writer w(out + kSaveHeaderSize, cap - kSaveHeaderSize);

    // Player section
    std::size_t at = w.pos();
    w.u16v(0);
    std::size_t start = w.pos();
    w.bytes(data.playerName, 16);
    w.s64v(data.lastSim);
    w.s64v(data.devOffset);
    w.u32v(data.nextId);
    w.u32v(data.nestA);  // Alpha 2: the pair at the Nesting Stone
    w.u32v(data.nestB);
    w.s32v(data.nestDay);
    w.u32v(data.gleam);  // Alpha 2: Gleam and the hoard
    for (u16 n : data.hoard) w.u16v(n);
    for (u16 n : data.pouch) w.u16v(n);  // Alpha 2: the pouch and the Market's egg of the day
    w.s32v(data.eggBoughtDay);
    w.u32v(data.owned);  // Alpha 2: things bought to keep, the decor, the toys, the food bowl
    for (u8 k : data.decor) w.u8v(k);
    for (const auto& p : data.toyPos)
        for (s16 v : p) w.u16v(static_cast<u16>(v));
    int portions = 0;  // the bowl: first as before (its last food and how many, for older builds)...
    while (portions < kBowlSlots && data.bowl[portions] != 0xFF) ++portions;
    w.u8v(portions ? data.bowl[portions - 1] : 0xFF);
    w.u8v(static_cast<u8>(portions));
    for (u8 k : data.bowl) w.u8v(k);  // ...then every portion (Alpha 2, after run 4)
    for (u8 k : data.dexLooks) w.u8v(k);  // Alpha 2 WP12: the Dragondex
    w.u8v(data.dexRares);
    w.u32v(data.dexDone);
    w.u8v(data.bannerBreed);
    w.u8v(static_cast<u8>(kDexKindSlots));  // DR3: the Dragondex by kind (its size first, so it can grow)
    for (u8 k : data.dexKinds) w.u8v(k);
    w.u32v(static_cast<u32>(data.dexKindsDone));
    w.u32v(static_cast<u32>(data.dexKindsDone >> 32));
    w.u8v(data.bannerKind);
    w.patchU16(at, static_cast<u16>(w.pos() - start));

    // Settings section
    at = w.pos();
    w.u16v(0);
    start = w.pos();
    w.u8v(data.settings.musicVolume);
    w.u8v(data.settings.sfxVolume);
    w.u8v(data.settings.voiceEnabled);
    w.u8v(data.settings.stereo3d);
    w.u8v(data.settings.seenHatch);
    w.patchU16(at, static_cast<u16>(w.pos() - start));

    w.u16v(data.dragonCount);
    for (u16 i = 0; i < data.dragonCount; ++i) writeDragon(w, data.dragons[i]);
    if (!w.ok()) return 0;

    const u32 payloadSize = static_cast<u32>(w.pos());
    Writer h(out, kSaveHeaderSize);
    h.bytes(kMagic, 4);
    h.u16v(kSaveVersion);
    h.u16v(0);
    h.u32v(seq);
    h.u32v(payloadSize);
    h.u32v(crc32(out + kSaveHeaderSize, payloadSize));
    h.s64v(savedAt);
    h.u32v(0);
    return kSaveHeaderSize + payloadSize;
}

const char* loadResultName(LoadResult r) {
    switch (r) {
        case LoadResult::Ok: return "ok";
        case LoadResult::Empty: return "empty";
        case LoadResult::BadMagic: return "not a save";
        case LoadResult::TooNew: return "made by a newer version";
        case LoadResult::Truncated: return "truncated";
        case LoadResult::BadCrc: return "corrupted (checksum)";
        case LoadResult::BadData: return "corrupted (data)";
    }
    return "?";
}

static LoadResult readHeader(const u8* data, std::size_t size, SaveHeaderInfo& info, u32& payloadSize, u32& crc) {
    if (!data || size == 0) return LoadResult::Empty;
    if (size < kSaveHeaderSize) return LoadResult::Truncated;
    if (std::memcmp(data, kMagic, 4) != 0) return LoadResult::BadMagic;
    Reader h(data + 4, kSaveHeaderSize - 4);
    info.version = h.u16v();
    h.u16v();  // flags
    info.seq = h.u32v();
    payloadSize = h.u32v();
    crc = h.u32v();
    info.savedAt = h.s64v();
    if (info.version > kSaveVersion) return LoadResult::TooNew;
    if (info.version == 0) return LoadResult::BadData;
    if (kSaveHeaderSize + static_cast<std::size_t>(payloadSize) > size) return LoadResult::Truncated;
    if (crc32(data + kSaveHeaderSize, payloadSize) != crc) return LoadResult::BadCrc;
    return LoadResult::Ok;
}

LoadResult decodeSave(const u8* data, std::size_t size, SaveData& out, SaveHeaderInfo* infoOut) {
    SaveHeaderInfo info;
    u32 payloadSize = 0, crc = 0;
    const LoadResult hr = readHeader(data, size, info, payloadSize, crc);
    if (hr != LoadResult::Ok) return hr;

    // Decode into a scratch copy so `out` is untouched on failure. Static: SaveData is
    // large (200 dragons) and must not live on the 3DS's small stack.
    static SaveData tmp;
    tmp = SaveData{};
    Reader r(data + kSaveHeaderSize, payloadSize);

    // Version migrations go here: `if (info.version < 2) { ...defaults for new fields... }`.
    u16 sectionSize = r.u16v();
    std::size_t start = r.pos();
    r.bytes(tmp.playerName, 16);
    tmp.playerName[15] = '\0';
    tmp.lastSim = r.s64v();
    tmp.devOffset = r.s64v();
    tmp.nextId = r.u32v();
    if (sectionSize >= 16 + 8 + 8 + 4 + 12) {  // older saves: no one nesting
        tmp.nestA = r.u32v();
        tmp.nestB = r.u32v();
        tmp.nestDay = r.s32v();
    }
    if (sectionSize >= 16 + 8 + 8 + 4 + 12 + 4 + 12) {  // older saves: no Gleam, an empty hoard
        tmp.gleam = r.u32v();
        for (u16& n : tmp.hoard) n = r.u16v();
    }
    if (sectionSize >= 16 + 8 + 8 + 4 + 12 + 16 + 24) {  // older saves: the starting pouch
        for (u16& n : tmp.pouch) n = r.u16v();
        tmp.eggBoughtDay = r.s32v();
    }
    if (sectionSize >= 16 + 8 + 8 + 4 + 12 + 16 + 24 + 27) {  // older saves: nothing bought yet
        tmp.owned = r.u32v();
        for (u8& k : tmp.decor) k = r.u8v();
        for (auto& p : tmp.toyPos)
            for (s16& v : p) v = static_cast<s16>(r.u16v());
        const u8 food = r.u8v(), left = r.u8v();
        if (sectionSize >= 16 + 8 + 8 + 4 + 12 + 16 + 24 + 27 + kBowlSlots) {
            for (u8& k : tmp.bowl) k = r.u8v();
        } else {  // older saves: one food, `left` portions of it
            for (int k = 0; k < kBowlSlots; ++k) tmp.bowl[k] = k < left && k < 3 ? food : 0xFF;
        }
    }
    if (sectionSize >= 16 + 8 + 8 + 4 + 12 + 16 + 24 + 27 + kBowlSlots + kBreedCount + 6) {  // older: an empty book
        for (u8& k : tmp.dexLooks) k = static_cast<u8>(r.u8v() & ((1u << kLookCount) - 1));
        tmp.dexRares = static_cast<u8>(r.u8v() & 0x0F);
        tmp.dexDone = r.u32v() & ((1u << kBreedCount) - 1);
        tmp.bannerBreed = r.u8v();
        if (tmp.bannerBreed >= kBreedCount) tmp.bannerBreed = 0xFF;
    }
    if (r.pos() + 1 <= start + sectionSize) {  // DR3: the Dragondex by kind (older saves: empty; main.cpp fills it)
        const int slots = r.u8v();
        for (int k = 0; k < slots; ++k) {
            const u8 v = r.u8v();
            if (k < kDexKindSlots) tmp.dexKinds[k] = static_cast<u8>(v & ((1u << kKindVariants) - 1));
        }
        const u64 lo = r.u32v(), hi = r.u32v();
        tmp.dexKindsDone = lo | (hi << 32);
        tmp.bannerKind = r.u8v();
        if (tmp.bannerKind >= kindCount()) tmp.bannerKind = 0xFF;
    }
    r.seek(start + sectionSize);

    sectionSize = r.u16v();
    start = r.pos();
    tmp.settings.musicVolume = r.u8v();
    tmp.settings.sfxVolume = r.u8v();
    tmp.settings.voiceEnabled = r.u8v();
    tmp.settings.stereo3d = r.u8v();
    if (sectionSize >= 5) tmp.settings.seenHatch = r.u8v();
    r.seek(start + sectionSize);

    tmp.dragonCount = r.u16v();
    if (tmp.dragonCount > kMaxDragons) return LoadResult::BadData;
    for (u16 i = 0; i < tmp.dragonCount; ++i)
        if (!readDragon(r, tmp.dragons[i])) return LoadResult::BadData;
    if (!r.ok()) return LoadResult::Truncated;

    out = tmp;
    if (infoOut) *infoOut = info;
    return LoadResult::Ok;
}

int pickNewestSlot(const u8* a, std::size_t aSize, const u8* b, std::size_t bSize) {
    SaveHeaderInfo ia, ib;
    u32 ps, crc;
    const bool okA = readHeader(a, aSize, ia, ps, crc) == LoadResult::Ok;
    const bool okB = readHeader(b, bSize, ib, ps, crc) == LoadResult::Ok;
    if (okA && okB) return ib.seq > ia.seq ? 1 : 0;
    if (okA) return 0;
    if (okB) return 1;
    return -1;
}

}  // namespace ec
