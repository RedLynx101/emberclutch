#include "app/audio.hpp"

#include <3ds.h>
#include <tremor/ivorbisfile.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>

namespace ec::audio {
namespace {

constexpr int kMusicCh = 0, kStingerCh = 1, kSfxFirst = 2, kSfxCount = 8, kBedFirst = kSfxFirst + kSfxCount;
// ndsp's 24 channels. The beds share the ones after the effects' (sounds, 1.0): a bed takes a
// free one as it starts and gives it back when it falls silent, so there can be more beds than
// channels left (seventeen for fourteen), as only a handful are ever heard at once.
constexpr int kChannels = 24, kBedChannels = kChannels - kBedFirst;
static_assert(kBedChannels > 0, "channels left for the beds");
constexpr int kMaxTakes = 4;
constexpr int kBufFrames = 4096;  // per streaming buffer (~128 ms at 32 kHz)
// Preloaded sounds are queued as slices of at most this many frames, one wave buffer
// each. Azahar's HLE DSP decodes a whole wave buffer into a vector and erases the played
// samples from its front every audio frame, so one long buffer costs it time quadratic in
// its length: two 15 s beds took the den from 100% to ~55% emulation speed. The 3DS
// itself doesn't mind; short slices are how the music already streams.
constexpr u32 kSliceFrames = 4096;
constexpr int kSfxSlices = 12;       // per sound-effect channel: up to ~2.2 s at 22 kHz in full slices
constexpr int kBedRing = 6;          // slices queued ahead per bed (~1.1 s at 22 kHz), refilled as they finish
constexpr int kStingerSlices = 64;
constexpr int kNumBufs = 3;
constexpr float kFadeSeconds = 0.7f;

const char* const kSfxFiles[] = {
    "ui-tap",       "ui-confirm",   "ui-back",        "ui-error",      "ui-toast",    "ui-save",
    "dragon-chirp", "dragon-trill", "dragon-purr",    "dragon-squeak", "dragon-whimper", "dragon-yawn",
    "dragon-sneeze", "dragon-rumble", "step",         "thump",         "flap",        "egg-knock",
    "egg-crack",    "egg-hatch",    "munch",          "gulp",          "brush",       "polish-sparkle",
    "splash",       "ball-bounce",
    // sound brief 2
    "ball-roll", "ball-pickup", "dragon-grumble", "dragon-sniff", "dragon-giggle", "leg-kick", "tub-slide", "suds",
    "water-pour", "shake-spray", "egg-heartbeat", "egg-turn", "hatch-first-cry", "rope-tug", "feather-flutter",
    "orb-rattle", "treat-drop", "bowl-clink", "nest-settle", "egg-lay", "coin", "register", "map-open",
    "travel-whoosh", "trail-depart",
    // sound brief 3: the valley and flying
    "wingbeat", "takeoff", "landing", "dive-whoosh", "water-skim", "splash-big", "dragon-step-grass",
    // Beta 1
    "step-grass", "step-stone", "step-wood", "mount", "find-sparkle", "lantern-light", "lantern-relight", "door-wood",
    "quest-page", "breath-flame", "breath-frost", "breath-gust", "breath-light", "breath-mist", "breath-spores",
    "village-bell", "crowd-cheer", "crowd-aww", "whistle-start", "ring-pass", "fruit-toss", "fruit-catch", "star-shimmer",
    // 1.0
    "hop-on", "hop-off", "nuzzle", "shutter", "equip", "level-up", "unlock", "notice", "battle-start", "swipe", "hit",
    "hit-big", "whiff", "faint", "stat-up", "stat-down", "victory", "defeat", "pose", "twirl", "ribbon", "cast", "plop",
    "bite", "reel", "shell-pick", "ice-crack", "step-sand", "step-snow", "burst", "brake",
    // the valley's critters (workstream L)
    "bird-chirp", "bird-flutter", "rabbit-hop", "frog-croak", "duck-quack", "fox-yip", "butterfly-land", "whistle-call",
    "critter-friend", "leaf-rustle",
    "roamer-hello", "hands-clap", "soft-snore"};  // (workstream D)
static_assert(sizeof(kSfxFiles) / sizeof(kSfxFiles[0]) == static_cast<int>(Sfx::Count), "one file per Sfx");
// Stand-ins (D35) for sound brief 2, in Sfx order from BallRoll: what plays until the sound's
// own file arrives, and how it's retuned. They match what these moments played before.
struct StandIn {
    Sfx use;
    float pitch, gain;
};
constexpr int kFirstBrief2 = static_cast<int>(Sfx::BallRoll);
constexpr StandIn kStandIns[] = {
    {Sfx::Bounce, 0.6f, 0.35f},  {Sfx::Squeak, 1.4f, 0.5f},  {Sfx::Rumble, 1.4f, 0.6f},  {Sfx::Chirp, 1.5f, 0.3f},
    {Sfx::Trill, 1.25f, 1.0f},   {Sfx::Thump, 1.2f, 0.8f},   {Sfx::Thump, 0.8f, 0.7f},   {Sfx::Splash, 1.25f, 1.0f},
    {Sfx::Splash, 1.0f, 1.0f},   {Sfx::Brush, 1.3f, 1.0f},   {Sfx::Thump, 0.5f, 1.0f},   {Sfx::Brush, 0.7f, 0.6f},
    {Sfx::Squeak, 1.0f, 1.0f},   {Sfx::Purr, 1.2f, 1.0f},    {Sfx::Flap, 1.8f, 0.35f},   {Sfx::Bounce, 1.5f, 0.4f},
    {Sfx::Sparkle, 1.0f, 1.0f},  {Sfx::Confirm, 1.0f, 1.0f}, {Sfx::Purr, 0.9f, 0.8f},    {Sfx::EggKnock, 1.0f, 1.0f},
    {Sfx::Sparkle, 1.0f, 1.0f},  {Sfx::Confirm, 1.0f, 1.0f}, {Sfx::Confirm, 1.0f, 1.0f}, {Sfx::Flap, 1.0f, 1.0f},
    {Sfx::Chirp, 1.2f, 1.0f},
    // brief 3 (its files arrived with it: these only play if one goes missing)
    {Sfx::Flap, 0.8f, 1.0f},     {Sfx::Flap, 0.7f, 1.0f},    {Sfx::Thump, 0.8f, 1.0f},   {Sfx::Flap, 1.4f, 0.4f},
    {Sfx::Splash, 1.2f, 0.6f},   {Sfx::Splash, 0.7f, 1.0f},  {Sfx::Step, 1.0f, 1.0f},
    // Beta 1 (their files are in: these only if one goes missing)
    {Sfx::Step, 1.2f, 0.6f},     {Sfx::Step, 1.4f, 0.6f},    {Sfx::Step, 1.1f, 0.7f},    {Sfx::Flap, 1.1f, 0.8f},
    {Sfx::Sparkle, 1.2f, 1.0f},  {Sfx::Sparkle, 0.8f, 1.0f}, {Sfx::Sparkle, 0.9f, 0.8f}, {Sfx::Thump, 1.1f, 0.8f},
    {Sfx::Toast, 1.0f, 1.0f},    {Sfx::Rumble, 1.3f, 0.8f},  {Sfx::Sparkle, 1.4f, 0.7f}, {Sfx::Flap, 1.5f, 0.6f},
    {Sfx::Sparkle, 1.6f, 0.7f},  {Sfx::Splash, 1.5f, 0.5f},  {Sfx::Brush, 0.8f, 0.6f},  {Sfx::Confirm, 0.8f, 1.0f},
    {Sfx::Confirm, 1.2f, 1.0f},  {Sfx::Whimper, 1.0f, 0.6f}, {Sfx::Toast, 1.4f, 1.0f},  {Sfx::Sparkle, 1.3f, 1.0f},
    {Sfx::Flap, 1.6f, 0.5f},     {Sfx::Munch, 1.2f, 0.8f},   {Sfx::Sparkle, 0.7f, 1.0f},
    // 1.0 (until the synths are in)
    {Sfx::Mount, 1.3f, 0.8f},    {Sfx::Thump, 1.2f, 0.6f},   {Sfx::Purr, 1.3f, 0.7f},    {Sfx::Tap, 0.7f, 1.0f},
    {Sfx::Sparkle, 1.1f, 0.8f},  {Sfx::Confirm, 1.3f, 1.0f}, {Sfx::Sparkle, 0.9f, 1.0f}, {Sfx::Toast, 1.2f, 0.8f},
    {Sfx::WhistleStart, 1.0f, 1.0f}, {Sfx::Flap, 1.6f, 0.7f}, {Sfx::Thump, 1.0f, 1.0f}, {Sfx::Thump, 0.8f, 1.0f},
    {Sfx::Flap, 2.0f, 0.4f},     {Sfx::Whimper, 0.8f, 0.8f}, {Sfx::Sparkle, 1.4f, 0.7f}, {Sfx::Rumble, 1.5f, 0.5f},
    {Sfx::CrowdCheer, 1.0f, 1.0f}, {Sfx::CrowdAww, 1.0f, 1.0f}, {Sfx::Sparkle, 1.2f, 0.6f}, {Sfx::Flap, 1.3f, 0.6f},
    {Sfx::Confirm, 1.1f, 1.0f},  {Sfx::Flap, 1.8f, 0.5f},    {Sfx::Splash, 1.6f, 0.5f},  {Sfx::Splash, 1.2f, 0.8f},
    {Sfx::BallRoll, 1.4f, 0.6f}, {Sfx::BowlClink, 1.3f, 0.7f}, {Sfx::EggCrack, 0.7f, 0.9f}, {Sfx::Step, 0.9f, 0.6f},
    {Sfx::Step, 0.8f, 0.6f},     {Sfx::DiveWhoosh, 1.2f, 0.8f}, {Sfx::Flap, 0.6f, 0.7f},
    // the valley's critters (workstream L; their synths are in: these only if one goes missing)
    {Sfx::Chirp, 1.8f, 0.4f},    {Sfx::Flap, 1.6f, 0.6f},    {Sfx::Thump, 1.6f, 0.4f},   {Sfx::Rumble, 1.8f, 0.4f},
    {Sfx::Squeak, 0.8f, 0.5f},   {Sfx::Squeak, 1.2f, 0.5f},  {Sfx::Sparkle, 1.5f, 0.5f}, {Sfx::WhistleStart, 1.2f, 0.5f},
    {Sfx::Confirm, 1.2f, 0.8f},  {Sfx::Brush, 1.0f, 0.6f},
    {Sfx::WhistleStart, 1.3f, 0.5f}, {Sfx::Thump, 1.9f, 0.4f}, {Sfx::Purr, 0.7f, 0.5f},  // (workstream D)
};
static_assert(sizeof(kStandIns) / sizeof(kStandIns[0]) == static_cast<int>(Sfx::Count) - kFirstBrief2,
              "a stand-in for every brief 2 sound");
// Sounds with a place in the mix of their own, whoever plays them: the footsteps quiet and
// muffled, soft paws on the den floor rather than clicks (Noah, after the 3DS run 2026-09-24).
struct Tone {
    Sfx sfx;
    float gain;
    float lowpassHz;  // 0: as recorded
};
constexpr Tone kTones[] = {{Sfx::Step, 0.3f, 650.0f}, {Sfx::DragonStep, 0.55f, 0.0f}};
const char* const kBedFiles[] = {"amb-hearth", "amb-night", "egg-hum", "amb-market", "amb-wind-high", "amb-meadow",
                                 "amb-valley-night", "amb-lake", "wing-flutter", "amb-stream", "amb-waterfall",
                                 "amb-village",
                                 // 1.0 (sounds): the new places' and the rush of speed
                                 "amb-cove", "amb-caldera", "amb-glade-night", "amb-hollow", "amb-rush"};
// Under the music and the voices. (The stream's, the falls' and the village's were missing
// until 1.0, so those three read past the table's end.)
const float kBedGain[] = {0.55f, 0.5f, 0.6f, 0.45f, 0.5f, 0.42f, 0.45f, 0.45f, 0.35f, 0.45f, 0.45f,
                          0.42f, 0.45f, 0.45f, 0.4f, 0.45f, 0.5f};
// The den's beds stay loaded (they come and go all the time there); the Market's and the
// valley's load when a scene first wants them and go again a few seconds after they fall
// silent: preloaded, the valley's five took 1.6 MB of the den's linear memory (run 15's build).
const bool kBedResident[] = {true, true, true, false, false, false, false, false, false, false, false, false,
                             false, false, false, false, false};
constexpr float kBedUnloadAfter = 3.0f;  // seconds silent
static_assert(sizeof(kBedFiles) / sizeof(kBedFiles[0]) == static_cast<int>(Bed::Count), "one file per Bed");
static_assert(sizeof(kBedGain) / sizeof(kBedGain[0]) == static_cast<int>(Bed::Count), "one gain per Bed");
static_assert(sizeof(kBedResident) / sizeof(kBedResident[0]) == static_cast<int>(Bed::Count), "one flag per Bed");

struct Stream {
    OggVorbis_File vf;
    bool open = false;
    int channels = 2;
    s64 loopStart = 0;
    ndspWaveBuf wbuf[kNumBufs];
    s16* data[kNumBufs] = {};
    s64 bufAt[kNumBufs] = {};  // each buffer's first sample, counted from the track's start as heard
    s64 queued = 0;            // samples queued since the track opened
    long rate = 32000;
};

struct Clip {
    s16* data = nullptr;
    u32 frames = 0;
    u32 rate = 22050;
    bool stereo = false;
};

bool g_ok = false;
Stream g_stream;
Thread g_thread = nullptr;
LightEvent g_event;
LightLock g_lock;
volatile bool g_quit = false;
volatile u32 g_dbgLoops = 0, g_dbgSwitches = 0;
volatile int g_dbgStage = 0;

// Requests from the main thread (guarded by g_lock), applied on the streaming thread.
char g_wanted[32] = {};     // track the game wants
char g_current[32] = {};    // track the stream is playing
volatile bool g_switch = false;
char g_stingerWanted[32] = {};
volatile bool g_stingerReq = false;

// Main-thread state.
char g_sent[32] = {};  // last track handed to the streaming thread (never re-sent while it switches)
float g_gain = 0.0f;   // fade gain of the loop
float g_duck = 1.0f;  // lowered while a stinger plays
float g_musicVol = 0.8f, g_sfxVol = 0.9f;

s16* g_stingerData = nullptr;
ndspWaveBuf g_stingerBufs[kStingerSlices];
Clip g_clips[static_cast<int>(Sfx::Count)][kMaxTakes];
u8 g_takes[static_cast<int>(Sfx::Count)] = {};     // takes loaded
u8 g_nextTake[static_cast<int>(Sfx::Count)] = {};  // the one to play next
ndspWaveBuf g_sfxBufs[kSfxCount][kSfxSlices];
int g_nextSfx = 0;
Clip g_beds[static_cast<int>(Bed::Count)];
ndspWaveBuf g_bedBufs[static_cast<int>(Bed::Count)][kBedRing];
u32 g_bedNext[static_cast<int>(Bed::Count)] = {};  // the frame the next slice starts at
float g_bedLevel[static_cast<int>(Bed::Count)] = {}, g_bedWant[static_cast<int>(Bed::Count)] = {};
bool g_bedOn[static_cast<int>(Bed::Count)] = {};
float g_bedIdle[static_cast<int>(Bed::Count)] = {};  // seconds a loaded, non-resident bed has been silent
int g_bedCh[static_cast<int>(Bed::Count)] = {};      // its channel while on
bool g_bedChBusy[kBedChannels] = {};                 // the bed channels taken

// Queues frames [0, frames) of interleaved PCM16 on `ch` as consecutive slices, one wave
// buffer each (see kSliceFrames): as many as fit in `count` buffers, each at least
// kSliceFrames long. Returns the number queued.
int queueSlices(int ch, const s16* data, int channels, u32 frames, ndspWaveBuf* bufs, int count) {
    u32 slice = (frames + count - 1) / count;
    if (slice < kSliceFrames) slice = kSliceFrames;
    int n = 0;
    for (u32 at = 0; at < frames && n < count; at += slice, ++n) {
        ndspWaveBuf& w = bufs[n];
        std::memset(&w, 0, sizeof(w));
        w.data_pcm16 = const_cast<s16*>(data) + static_cast<size_t>(at) * channels;
        w.nsamples = frames - at < slice ? frames - at : slice;
        ndspChnWaveBufAdd(ch, &w);
    }
    return n;
}

void setMix(int ch, float vol) {
    float mix[12] = {};
    mix[0] = mix[1] = vol;
    ndspChnSetMix(ch, mix);
}

s64 parseLoopStart(OggVorbis_File* vf) {
    vorbis_comment* vc = ov_comment(vf, -1);
    if (!vc) return 0;
    for (int i = 0; i < vc->comments; ++i)
        if (strncasecmp(vc->user_comments[i], "LOOPSTART=", 10) == 0) return std::atoll(vc->user_comments[i] + 10);
    return 0;
}

// Fill `out` with `frames` frames, looping back to LOOPSTART at the end of the file.
int decodeInto(Stream& s, s16* out, int frames) {
    const int want = frames * s.channels * 2;
    int got = 0, bitstream = 0;
    char* dst = reinterpret_cast<char*>(out);
    int loops = 0;
    while (got < want) {
        const long r = ov_read(&s.vf, dst + got, want - got, &bitstream);
        if (r == 0) {  // end of file: jump back to the loop point (sample-accurate)
            if (++loops > 2 || ov_pcm_seek(&s.vf, s.loopStart) != 0) break;
            continue;
        }
        if (r < 0) continue;  // a hole in the data: keep reading
        got += static_cast<int>(r);
    }
    return got / (s.channels * 2);
}

void closeStream() {
    if (g_stream.open) {
        ndspChnWaveBufClear(kMusicCh);
        ov_clear(&g_stream.vf);  // also closes the FILE
        g_stream.open = false;
    }
    g_current[0] = '\0';
}

bool openVorbis(const char* slug, OggVorbis_File* vf) {
    char path[64];
    std::snprintf(path, sizeof(path), "romfs:/music/%s.ogg", slug);
    FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    if (ov_open(f, vf, nullptr, 0) != 0) {
        std::fclose(f);
        return false;
    }
    return true;
}

void setupChannel(int ch, int channels, long rate) {
    ndspChnReset(ch);
    ndspChnSetInterp(ch, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(ch, static_cast<float>(rate));
    ndspChnSetFormat(ch, channels == 2 ? NDSP_FORMAT_STEREO_PCM16 : NDSP_FORMAT_MONO_PCM16);
}

bool openStream(const char* slug) {
    if (!openVorbis(slug, &g_stream.vf)) return false;
    vorbis_info* vi = ov_info(&g_stream.vf, -1);
    g_stream.channels = vi->channels;
    g_stream.loopStart = parseLoopStart(&g_stream.vf);
    g_stream.queued = 0;
    g_stream.rate = vi->rate;
    g_stream.open = true;
    setupChannel(kMusicCh, vi->channels, vi->rate);
    setMix(kMusicCh, 0.0f);
    for (auto& w : g_stream.wbuf) std::memset(&w, 0, sizeof(w));  // all free
    std::snprintf(g_current, sizeof(g_current), "%s", slug);
    return true;
}

// Stingers are short: decode fully into linear memory, then play once.
void startStinger(const char* slug) {
    OggVorbis_File vf;
    if (!openVorbis(slug, &vf)) return;
    vorbis_info* vi = ov_info(&vf, -1);
    const int channels = vi->channels;
    const s64 total = ov_pcm_total(&vf, -1);
    ndspChnWaveBufClear(kStingerCh);
    if (g_stingerData) linearFree(g_stingerData);
    g_stingerData = static_cast<s16*>(linearAlloc(static_cast<size_t>(total) * channels * 2));
    if (!g_stingerData) {
        ov_clear(&vf);
        return;
    }
    char* dst = reinterpret_cast<char*>(g_stingerData);
    const long want = static_cast<long>(total * channels * 2);
    long got = 0, sinceRest = 0;
    int bitstream = 0;
    while (got < want) {
        const long r = ov_read(&vf, dst + got, static_cast<int>(want - got), &bitstream);
        if (r == 0) break;
        if (r > 0) {
            got += r;
            sinceRest += r;
        }
        // This thread runs above the game's priority (music mustn't starve): decoding a whole
        // stinger at once froze the game ~100 ms as an egg began to hatch (3DS run 7). A short
        // rest every 16 KB lets the game's frames through; the stinger starts a little later.
        if (sinceRest >= 16 * 1024) {
            sinceRest = 0;
            svcSleepThread(1000000);
        }
    }
    setupChannel(kStingerCh, channels, vi->rate);
    ov_clear(&vf);
    DSP_FlushDataCache(g_stingerData, got);
    setMix(kStingerCh, g_musicVol);
    queueSlices(kStingerCh, g_stingerData, channels, static_cast<u32>(got / (channels * 2)), g_stingerBufs,
                kStingerSlices);
}

void streamThread(void*) {
    while (!g_quit) {
        ++g_dbgLoops;
        if (g_switch) {
            ++g_dbgSwitches;
            g_dbgStage = 1;
            char want[32];
            LightLock_Lock(&g_lock);
            std::memcpy(want, g_wanted, sizeof(want));
            g_switch = false;
            LightLock_Unlock(&g_lock);
            closeStream();
            // A missing track stays silent instead of being retried every frame.
            if (want[0] && !openStream(want)) {
                std::snprintf(g_current, sizeof(g_current), "%s", want);
                g_dbgStage = 3;
            } else {
                g_dbgStage = 2;
            }
        }
        if (g_stingerReq) {
            char want[32];
            LightLock_Lock(&g_lock);
            std::memcpy(want, g_stingerWanted, sizeof(want));
            g_stingerReq = false;
            LightLock_Unlock(&g_lock);
            startStinger(want);
        }
        if (g_stream.open) {
            for (int i = 0; i < kNumBufs; ++i) {
                ndspWaveBuf& w = g_stream.wbuf[i];
                if (w.status != NDSP_WBUF_FREE && w.status != NDSP_WBUF_DONE) continue;
                g_dbgStage = 4;
                const int n = decodeInto(g_stream, g_stream.data[i], kBufFrames);
                g_dbgStage = 2;
                if (n <= 0) break;
                w.data_pcm16 = g_stream.data[i];
                w.nsamples = static_cast<u32>(n);
                g_stream.bufAt[i] = g_stream.queued;
                g_stream.queued += n;
                DSP_FlushDataCache(g_stream.data[i], n * g_stream.channels * 2);
                ndspChnWaveBufAdd(kMusicCh, &w);
            }
        }
        LightEvent_Wait(&g_event);
    }
}

void onDspFrame(void*) { LightEvent_Signal(&g_event); }

// Minimal RIFF/WAVE reader for 16-bit PCM clips.
bool loadWav(const char* path, Clip& clip) {
    FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    const long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    u8* buf = static_cast<u8*>(std::malloc(static_cast<size_t>(size)));
    const bool read = buf && std::fread(buf, 1, static_cast<size_t>(size), f) == static_cast<size_t>(size);
    std::fclose(f);
    bool ok = false;
    if (read && size > 12 && std::memcmp(buf, "RIFF", 4) == 0 && std::memcmp(buf + 8, "WAVE", 4) == 0) {
        u16 channels = 1, bits = 16;
        u32 rate = 22050;
        for (long p = 12; p + 8 <= size;) {
            u32 len;
            std::memcpy(&len, buf + p + 4, 4);
            if (std::memcmp(buf + p, "fmt ", 4) == 0) {
                std::memcpy(&channels, buf + p + 10, 2);
                std::memcpy(&rate, buf + p + 12, 4);
                std::memcpy(&bits, buf + p + 22, 2);
            } else if (std::memcmp(buf + p, "data", 4) == 0 && bits == 16 && p + 8 + static_cast<long>(len) <= size) {
                clip.data = static_cast<s16*>(linearAlloc(len));
                if (clip.data) {
                    std::memcpy(clip.data, buf + p + 8, len);
                    DSP_FlushDataCache(clip.data, len);
                    clip.frames = len / (2u * channels);
                    clip.rate = rate;
                    clip.stereo = channels == 2;
                    ok = true;
                }
                break;
            }
            p += 8 + len + (len & 1);
        }
    }
    std::free(buf);
    return ok;
}

}  // namespace

bool init() {
    if (R_FAILED(ndspInit())) return false;
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspSetMasterVol(1.0f);
    for (int i = 0; i < kNumBufs; ++i) {
        g_stream.data[i] = static_cast<s16*>(linearAlloc(kBufFrames * 2 * sizeof(s16)));
        if (!g_stream.data[i]) return false;
    }
    for (int i = 0; i < static_cast<int>(Sfx::Count); ++i) {  // a missing sound just stays silent
        for (int k = 0; k < kMaxTakes; ++k) {
            char path[64];
            if (k == 0)
                std::snprintf(path, sizeof(path), "romfs:/sfx/%s.wav", kSfxFiles[i]);
            else
                std::snprintf(path, sizeof(path), "romfs:/sfx/%s-%d.wav", kSfxFiles[i], k + 1);
            if (!loadWav(path, g_clips[i][k])) break;
            g_takes[i] = static_cast<u8>(k + 1);
        }
    }
    for (int b = 0; b < static_cast<int>(Bed::Count); ++b) {  // started when a scene asks
        if (!kBedResident[b]) continue;  // (loaded when first wanted)
        char path[64];
        std::snprintf(path, sizeof(path), "romfs:/sfx/%s.wav", kBedFiles[b]);
        loadWav(path, g_beds[b]);
    }
    LightEvent_Init(&g_event, RESET_ONESHOT);
    LightLock_Init(&g_lock);
    ndspSetCallback(onDspFrame, nullptr);
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    g_quit = false;
    g_thread = threadCreate(streamThread, nullptr, 32 * 1024, prio - 1, -2, false);
    g_ok = g_thread != nullptr;
    return g_ok;
}

void shutdown() {
    if (!g_ok) return;
    g_quit = true;
    LightEvent_Signal(&g_event);
    threadJoin(g_thread, U64_MAX);
    threadFree(g_thread);
    closeStream();
    for (int ch = 0; ch < kChannels; ++ch) ndspChnWaveBufClear(ch);
    for (auto* d : g_stream.data) if (d) linearFree(d);
    for (auto& takes : g_clips)
        for (auto& c : takes) if (c.data) linearFree(c.data);
    for (auto& c : g_beds) if (c.data) linearFree(c.data);
    if (g_stingerData) linearFree(g_stingerData);
    ndspExit();
    g_ok = false;
}

bool ok() { return g_ok; }

DebugInfo debugInfo() { return {g_dbgLoops, g_dbgSwitches, g_dbgStage, g_gain}; }

void playMusic(const char* slug) {
    if (!g_ok) return;
    const char* s = slug ? slug : "";
    if (std::strcmp(s, g_wanted) == 0) return;
    LightLock_Lock(&g_lock);
    std::snprintf(g_wanted, sizeof(g_wanted), "%s", s);
    LightLock_Unlock(&g_lock);
}

const char* currentMusic() { return g_current; }

double musicSeconds() {
    if (!g_ok || !g_stream.open || !ndspChnIsPlaying(kMusicCh)) return -1.0;
    const u16 seq = ndspChnGetWaveBufSeq(kMusicCh);
    for (int i = 0; i < kNumBufs; ++i)
        if (g_stream.wbuf[i].sequence_id == seq && g_stream.wbuf[i].status == NDSP_WBUF_PLAYING)
            return static_cast<double>(g_stream.bufAt[i] + ndspChnGetSamplePos(kMusicCh)) / g_stream.rate;
    return -1.0;
}

namespace {
// A field of a track's entry in loops.json ("show-stage": {..., "bpm": 101.03, ...}); 0 if not there.
double loopField(const char* slug, const char* field) {
    static char text[4096];
    static bool read = false;
    if (!read) {
        read = true;
        if (FILE* f = std::fopen("romfs:/music/loops.json", "rb")) {
            const std::size_t n = std::fread(text, 1, sizeof(text) - 1, f);
            text[n] = 0;
            std::fclose(f);
        }
    }
    char key[48];
    std::snprintf(key, sizeof(key), "\"%s\"", slug);
    const char* at = std::strstr(text, key);
    if (!at) return 0.0;
    const char* end = std::strchr(at, '}');
    std::snprintf(key, sizeof(key), "\"%s\":", field);
    const char* f = std::strstr(at, key);
    return f && (!end || f < end) ? std::atof(f + std::strlen(key)) : 0.0;
}
}  // namespace

float musicBpm(const char* slug) { return static_cast<float>(loopField(slug, "bpm")); }

float musicLoopStart(const char* slug) {
    const double rate = loopField(slug, "rate");
    return rate > 0 ? static_cast<float>(loopField(slug, "loopStart") / rate) : 0.0f;
}

bool hasMusic(const char* slug) {
    struct Seen {
        char slug[24];
        bool there;
    };
    static Seen seen[16];
    static int count = 0;
    for (int i = 0; i < count; ++i)
        if (std::strcmp(seen[i].slug, slug) == 0) return seen[i].there;
    char path[64];
    std::snprintf(path, sizeof(path), "romfs:/music/%s.ogg", slug);
    FILE* f = std::fopen(path, "rb");
    const bool there = f != nullptr;
    if (f) std::fclose(f);
    if (count < 16) {
        std::snprintf(seen[count].slug, sizeof(seen[count].slug), "%s", slug);
        seen[count++].there = there;
    }
    return there;
}

void playStinger(const char* slug) {
    if (!g_ok || !slug) return;
    LightLock_Lock(&g_lock);
    std::snprintf(g_stingerWanted, sizeof(g_stingerWanted), "%s", slug);
    g_stingerReq = true;
    LightLock_Unlock(&g_lock);
    LightEvent_Signal(&g_event);
}

bool has(Sfx s) { return s < Sfx::Count && g_takes[static_cast<int>(s)] > 0; }

void playSfx(Sfx s, float pitch, float gain) {
    if (!g_ok) return;
    const int i = static_cast<int>(s);
    if (g_takes[i] == 0) {  // not here yet: its stand-in
        if (i >= kFirstBrief2) {
            const StandIn& in = kStandIns[i - kFirstBrief2];
            if (g_takes[static_cast<int>(in.use)] > 0) playSfx(in.use, pitch * in.pitch, gain * in.gain);
        }
        return;
    }
    const Clip& c = g_clips[i][g_nextTake[i]];
    g_nextTake[i] = static_cast<u8>((g_nextTake[i] + 1) % g_takes[i]);  // takes in turn
    // A free channel if there is one, else the one used longest ago.
    int slot = g_nextSfx;
    for (int k = 0; k < kSfxCount; ++k) {
        const int j = (g_nextSfx + k) % kSfxCount;
        if (!ndspChnIsPlaying(kSfxFirst + j)) {
            slot = j;
            break;
        }
    }
    const int ch = kSfxFirst + slot;
    g_nextSfx = (slot + 1) % kSfxCount;
    ndspChnWaveBufClear(ch);
    setupChannel(ch, c.stereo ? 2 : 1, static_cast<long>(c.rate * pitch));
    float lowpass = 0.0f;
    for (const Tone& t : kTones) {
        if (t.sfx == s) {
            gain *= t.gain;
            lowpass = t.lowpassHz;
        }
    }
    if (lowpass > 0.0f)
        ndspChnIirBiquadSetParamsLowPassFilter(ch, lowpass, 0.707f);  // turns the channel's filter on
    else
        ndspChnIirBiquadSetEnable(ch, false);  // the channel may have played a muffled sound last
    setMix(ch, g_sfxVol * gain);
    queueSlices(ch, c.data, c.stereo ? 2 : 1, c.frames, g_sfxBufs[slot], kSfxSlices);
}

Clip g_letters[26];
int g_voice = -1;

bool loadVoice(u8 voice) {
    if (!g_ok) return false;
    if (g_voice == voice) return true;
    freeVoice();
    int loaded = 0;
    for (int k = 0; k < 26; ++k) {
        char path[48];
        std::snprintf(path, sizeof(path), "romfs:/voice/v%d/%c.wav", voice + 1, 'a' + k);
        loaded += loadWav(path, g_letters[k]);
    }
    g_voice = voice;
    return loaded > 0;
}

void freeVoice() {
    for (int k = 0; k < kSfxCount; ++k) ndspChnWaveBufClear(kSfxFirst + k);  // (none still reads a letter)
    for (Clip& c : g_letters) {
        if (c.data) linearFree(c.data);
        c = Clip{};
    }
    g_voice = -1;
}

void playLetter(char c, float pitch, float gain) {
    if (!g_ok || g_voice < 0) return;
    const char lower = static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    if (lower < 'a' || lower > 'z') return;
    const Clip& clip = g_letters[lower - 'a'];
    if (!clip.data) return;
    int slot = g_nextSfx;
    for (int k = 0; k < kSfxCount; ++k) {
        const int j = (g_nextSfx + k) % kSfxCount;
        if (!ndspChnIsPlaying(kSfxFirst + j)) {
            slot = j;
            break;
        }
    }
    const int ch = kSfxFirst + slot;
    g_nextSfx = (slot + 1) % kSfxCount;
    ndspChnWaveBufClear(ch);
    setupChannel(ch, 1, static_cast<long>(clip.rate * pitch));
    ndspChnIirBiquadSetEnable(ch, false);
    setMix(ch, g_sfxVol * gain * 0.7f);
    const u32 frames = clip.frames < 2600 ? clip.frames : 2600;  // just the letter's start: a quick blip
    queueSlices(ch, clip.data, 1, frames, g_sfxBufs[slot], 1);
}

void setBed(Bed b, float level) {
    const int i = static_cast<int>(b);
    g_bedWant[i] = level < 0.0f ? 0.0f : (level > 1.0f ? 1.0f : level);
    if (g_ok && g_bedWant[i] > 0.0f && !g_beds[i].data) {  // first wanted: loaded now (a scene's start)
        char path[64];
        std::snprintf(path, sizeof(path), "romfs:/sfx/%s.wav", kBedFiles[i]);
        loadWav(path, g_beds[i]);
    }
}

void setVolumes(u8 music, u8 sfx) {
    g_musicVol = music / 100.0f;
    g_sfxVol = sfx / 100.0f;
}

void update(float dt) {
    if (!g_ok) return;
    // Fade out, switch on the streaming thread, fade back in.
    const bool pending = std::strcmp(g_wanted, g_current) != 0;
    if (pending) {
        g_gain -= dt / kFadeSeconds;
        if (g_gain <= 0.0f) {
            g_gain = 0.0f;
            // Hand the switch over once; while the thread closes/opens, g_current is briefly
            // empty, and re-sending would restart the stream forever.
            if (std::strcmp(g_wanted, g_sent) != 0) {
                std::memcpy(g_sent, g_wanted, sizeof(g_sent));
                g_switch = true;
                LightEvent_Signal(&g_event);
            }
        }
    } else if (g_gain < 1.0f) {
        g_gain += dt / kFadeSeconds;
        if (g_gain > 1.0f) g_gain = 1.0f;
    }
    const float duckTarget = ndspChnIsPlaying(kStingerCh) ? 0.2f : 1.0f;
    g_duck += (duckTarget - g_duck) * (dt * 4.0f > 1.0f ? 1.0f : dt * 4.0f);
    setMix(kMusicCh, g_gain * g_duck * g_musicVol);
    // Beds ease toward their wanted level (about a second), then the wish lapses: a scene
    // keeps a bed going by asking again every frame. A bed holds a channel only while it is
    // wanted or still fading out.
    const float k = dt * 1.5f > 1.0f ? 1.0f : dt * 1.5f;
    for (int b = 0; b < static_cast<int>(Bed::Count); ++b) {
        const float want = g_bedWant[b];
        g_bedWant[b] = 0.0f;
        g_bedLevel[b] += (want - g_bedLevel[b]) * k;
        const Clip& c = g_beds[b];
        if (!c.data) continue;
        if (!g_bedOn[b] && want > 0.0f) {
            int slot = -1;
            for (int j = 0; j < kBedChannels && slot < 0; ++j)
                if (!g_bedChBusy[j]) slot = j;
            if (slot < 0) {  // every bed channel taken: it waits (silent) for one to come free
                g_bedLevel[b] = 0.0f;
                continue;
            }
            g_bedChBusy[slot] = true;
            g_bedCh[b] = kBedFirst + slot;
            const int ch = g_bedCh[b];
            setupChannel(ch, c.stereo ? 2 : 1, c.rate);
            ndspChnSetInterp(ch, NDSP_INTERP_LINEAR);
            setMix(ch, 0.0f);
            std::memset(g_bedBufs[b], 0, sizeof(g_bedBufs[b]));  // all free: the ring fills below
            g_bedNext[b] = 0;
            g_bedOn[b] = true;
        }
        if (!g_bedOn[b]) {
            // Silent a while (its channel long cleared): a non-resident bed's memory goes back.
            if (!kBedResident[b] && (g_bedIdle[b] += dt) > kBedUnloadAfter) {
                linearFree(g_beds[b].data);
                g_beds[b] = Clip{};
                g_bedIdle[b] = 0.0f;
            }
            continue;
        }
        // The loop plays as a ring of slices: each finished one takes the next stretch of
        // the bed (wrapping to its start) and goes back in the queue.
        const int ch = g_bedCh[b];
        const int channels = c.stereo ? 2 : 1;
        for (ndspWaveBuf& w : g_bedBufs[b]) {
            if (w.status != NDSP_WBUF_FREE && w.status != NDSP_WBUF_DONE) continue;
            const u32 at = g_bedNext[b];
            const u32 n = c.frames - at < kSliceFrames ? c.frames - at : kSliceFrames;
            std::memset(&w, 0, sizeof(w));
            w.data_pcm16 = c.data + static_cast<size_t>(at) * channels;
            w.nsamples = n;
            ndspChnWaveBufAdd(ch, &w);
            g_bedNext[b] = at + n >= c.frames ? 0 : at + n;
        }
        if (want <= 0.0f && g_bedLevel[b] < 0.002f) {
            ndspChnWaveBufClear(ch);
            g_bedChBusy[ch - kBedFirst] = false;  // the channel goes back to the pool
            g_bedOn[b] = false;
            g_bedLevel[b] = 0.0f;
            g_bedIdle[b] = 0.0f;
        } else {
            setMix(ch, g_bedLevel[b] * kBedGain[b] * g_sfxVol);
        }
    }
}

}  // namespace ec::audio
