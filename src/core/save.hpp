// Save format — see docs/tech/architecture.md section 8.
//
// File = header + payload, little-endian, every field written explicitly (never a raw
// struct dump), so saves survive compiler, padding and struct-layout changes.
//
//   header (32 bytes): "EMBC" | u16 version | u16 flags | u32 seq | u32 payloadSize |
//                      u32 crc32(payload) | s64 savedAt | u32 reserved
//   payload:           player block | settings | u16 dragonCount | dragon records...
//
// Two slots (save.a / save.b) are written alternately; the valid one with the higher
// `seq` wins, so an interrupted write can only ever damage the older copy.
#pragma once

#include <cstddef>

#include "core/dragon.hpp"
#include "core/story_state.hpp"
#include "core/trainer.hpp"
#include "core/world.hpp"

namespace ec {

constexpr u16 kSaveVersion = 2;  // 2: the story block (D137; a v1 save's quests migrate to it)
constexpr int kBowlSlots = 6;  // portions the food bowl holds
constexpr u32 kMaxDragons = 200;
constexpr std::size_t kSaveHeaderSize = 32;

struct Settings {
    u8 musicVolume = 80;  // 0..100
    u8 sfxVolume = 90;
    u8 voiceEnabled = 1;
    u8 stereo3d = 1;
    u8 seenHatch = 0;  // the hatching has been watched once: it can be skipped after that
};

constexpr int kDexKindSlots = 64;  // the Dragondex's room for kinds (36 planned, then second layers)

struct SaveData {
    char playerName[16] = {};
    s64 lastSim = 0;    // local unix time the simulation last advanced to
    s64 devOffset = 0;  // dev-build clock offset (0 in release)
    u32 nextId = 1;     // next creature id
    // A pair nesting at the Nesting Stone (Alpha 2 WP3): their ids (0: none) and the day they
    // settled; the egg comes the next day (core/breeding layDueEgg).
    u32 nestA = 0, nestB = 0;
    s32 nestDay = 0;
    // Alpha 2: Gleam (the valley's money) and the den's hoard of trinkets (core/wanderings Trinket).
    u32 gleam = 50;
    u16 hoard[6] = {};
    // The pouch (WP5): food by core/care Food. Everyone starts with some bread, a drumstick
    // or two and a candy (and the starter's favourite, scene_starter); older saves get the same.
    u16 pouch[10] = {0, 0, 0, 0, 0, 0, 8, 2, 1, 0};
    s32 eggBoughtDay = -1000000;  // the day the Market's egg was last bought (one a day)
    // Things bought to keep (WP7, core/items): a bit per Item; the decor in each of the five
    // spots (0xFF: none); where the toys lie on the den floor (hundredths); the food bowl.
    u32 owned = 0;
    u8 decor[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    s16 toyPos[4][2] = {};
    // The food bowl: up to kBowlSlots portions of any foods, in the order they went in (0xFF:
    // an empty place; the portions are always the first ones). One food at a time before, 3 of
    // it (Noah wanted to put in whatever he had, 2026-09-24).
    u8 bowl[kBowlSlots] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    // The Dragondex (WP12, core/dragondex): per breed, the looks seen (a bit each); the rare
    // traits seen; the breeds completed (rewarded, their banners yours); the breed banner hung
    // in the den (0xFF: none).
    u8 dexLooks[kBreedCount] = {};
    u8 dexRares = 0;
    u32 dexDone = 0;
    u8 bannerBreed = 0xFF;
    // DR3 (D80): the Dragondex by kind (the breed book above is archived with the breeds): per
    // kind, the colourings seen (a bit each); the kinds completed (a bit each); the kind whose
    // banner hangs in the den (0xFF: none). Room for 64 kinds.
    u8 dexKinds[kDexKindSlots] = {};
    u64 dexKindsDone = 0;
    u8 bannerKind = 0xFF;
    // Beta (D73-D86): the world: your look, where you are, your partner, places, lanterns,
    // quests, cups (core/world).
    WorldState world{};
    // 1.0 (D90): accessories and dyes owned, the leagues, Frostspire Hollow, the day's rewards,
    // the tracked goal, the tips, your records (core/trainer).
    Progress progress{};
    // The Living Valley pass (D137): the story's quests, flags, counters, mailbox and days (core/story).
    story::StoryState story{};
    Settings settings{};
    u16 dragonCount = 0;
    Dragon dragons[kMaxDragons];
};

// Upper bound for an encoded save; callers size their buffers with it.
std::size_t maxEncodedSize();

// Encodes into `out` (capacity `cap`). Returns the byte count, or 0 if it doesn't fit.
std::size_t encodeSave(const SaveData& data, u32 seq, s64 savedAt, u8* out, std::size_t cap);

enum class LoadResult : u8 { Ok, Empty, BadMagic, TooNew, Truncated, BadCrc, BadData };
const char* loadResultName(LoadResult r);

struct SaveHeaderInfo {
    u16 version = 0;
    u32 seq = 0;
    s64 savedAt = 0;
};

// Decodes and migrates older versions. `out` is only modified on success.
LoadResult decodeSave(const u8* data, std::size_t size, SaveData& out, SaveHeaderInfo* info = nullptr);

// Given the two slots' bytes, returns 0 or 1 for the newest valid slot, or -1 if neither
// is valid.
int pickNewestSlot(const u8* a, std::size_t aSize, const u8* b, std::size_t bSize);

u32 crc32(const u8* data, std::size_t size);

}  // namespace ec
